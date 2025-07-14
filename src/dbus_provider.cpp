#include "dbus_provider.hpp"
#include "logger.hpp"
#include "constants.hpp"
#include <iostream>
#include <sstream>

namespace PrimeCuts {

const char* introspection_xml =
    "<node>"
    "  <interface name='org.gnome.Shell.SearchProvider2'>"
    "    <method name='GetInitialResultSet'>"
    "      <arg type='as' name='terms' direction='in'/>"
    "      <arg type='as' name='results' direction='out'/>"
    "    </method>"
    "    <method name='GetSubsearchResultSet'>"
    "      <arg type='as' name='previous_results' direction='in'/>"
    "      <arg type='as' name='terms' direction='in'/>"
    "      <arg type='as' name='results' direction='out'/>"
    "    </method>"
    "    <method name='GetResultMetas'>"
    "      <arg type='as' name='ids' direction='in'/>"
    "      <arg type='aa{sv}' name='metas' direction='out'/>"
    "    </method>"
    "    <method name='ActivateResult'>"
    "      <arg type='s' name='id' direction='in'/>"
    "      <arg type='as' name='terms' direction='in'/>"
    "      <arg type='u' name='timestamp' direction='in'/>"
    "    </method>"
    "  </interface>"
    "</node>";

DBusSearchProvider::DBusSearchProvider(std::unique_ptr<CommandManager> command_manager)
    : command_manager_(std::move(command_manager))
    , main_loop_(nullptr)
    , introspection_data_(nullptr)
    , owner_id_(0)
    , registration_id_(0) {
}

DBusSearchProvider::~DBusSearchProvider() {
    stopService();
}

bool DBusSearchProvider::startService() {
    main_loop_ = g_main_loop_new(nullptr, FALSE);
    introspection_data_ = g_dbus_node_info_new_for_xml(introspection_xml, nullptr);
    
    if (!introspection_data_) {
        LOG_ERROR("Failed to parse introspection XML!");
        return false;
    }
    
    LOG_DEBUG("Introspection data loaded successfully");
    LOG_DEBUG("Attempting to own bus name: " + std::string(Constants::DBUS_SERVICE_NAME));
    
    owner_id_ = g_bus_own_name(
        G_BUS_TYPE_SESSION,
        Constants::DBUS_SERVICE_NAME,
        (GBusNameOwnerFlags)(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE),
        onBusAcquired,
        onNameAcquired,
        onNameLost,
        this, nullptr);
    
    if (owner_id_ == 0) {
        LOG_ERROR("Failed to own bus name");
        return false;
    }
    
    LOG_INFO("PrimeCuts DBus service starting...");
    g_main_loop_run(main_loop_);
    
    return true;
}

void DBusSearchProvider::stopService() {
    if (owner_id_ > 0) {
        g_bus_unown_name(owner_id_);
        owner_id_ = 0;
    }
    
    if (main_loop_) {
        g_main_loop_unref(main_loop_);
        main_loop_ = nullptr;
    }
    
    if (introspection_data_) {
        g_dbus_node_info_unref(introspection_data_);
        introspection_data_ = nullptr;
    }
}

void DBusSearchProvider::handleMethodCall(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) {
    
    auto* provider = static_cast<DBusSearchProvider*>(user_data);
    LOG_DEBUG("DBus method called: " + std::string(method_name) + " from " + std::string(sender));
    
    if (g_strcmp0(method_name, Constants::METHOD_GET_INITIAL_RESULT_SET) == 0) {
        provider->handleGetInitialResultSet(parameters, invocation);
    } else if (g_strcmp0(method_name, Constants::METHOD_GET_SUBSEARCH_RESULT_SET) == 0) {
        provider->handleGetSubsearchResultSet(parameters, invocation);
    } else if (g_strcmp0(method_name, Constants::METHOD_GET_RESULT_METAS) == 0) {
        provider->handleGetResultMetas(parameters, invocation);
    } else if (g_strcmp0(method_name, Constants::METHOD_ACTIVATE_RESULT) == 0) {
        provider->handleActivateResult(parameters, invocation);
    } else {
        LOG_WARNING("Unknown method called: " + std::string(method_name));
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
}

void DBusSearchProvider::handleGetInitialResultSet(GVariant* parameters, GDBusMethodInvocation* invocation) {
    LOG_DEBUG("Processing GetInitialResultSet request... [" + std::to_string(time(nullptr)) + "]");
    LOG_DEBUG("Parameters type: " + std::string(g_variant_get_type_string(parameters)));
    
    GVariantIter iter;
    g_variant_iter_init(&iter, parameters);
    GVariant* terms_array = g_variant_iter_next_value(&iter);
    
    std::vector<std::string> search_terms = extractSearchTerms(terms_array);
    
    if (terms_array) {
        g_variant_unref(terms_array);
    }
    
    LOG_DEBUG("Total search terms extracted: " + std::to_string(search_terms.size()));
    
    // Use command manager to search for matching actions
    std::vector<std::string> matches = command_manager_->searchActions(search_terms);
    
    LOG_DEBUG("Search completed. Found " + std::to_string(matches.size()) + " matching actions");
    
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    for (const auto& id : matches) {
        g_variant_builder_add(&builder, "s", id.c_str());
        LOG_DEBUG("Found match: " + id);
    }
    
    LOG_DEBUG("Returning " + std::to_string(matches.size()) + " results");
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
}

void DBusSearchProvider::handleGetSubsearchResultSet(GVariant* parameters, GDBusMethodInvocation* invocation) {
    LOG_DEBUG("Processing GetSubsearchResultSet request... [" + std::to_string(time(nullptr)) + "]");
    LOG_DEBUG("Parameters type: " + std::string(g_variant_get_type_string(parameters)));
    
    GVariantIter outer_iter;
    g_variant_iter_init(&outer_iter, parameters);
    
    // Skip the previous results array (first parameter)
    GVariant* previous_results = g_variant_iter_next_value(&outer_iter);
    if (previous_results) {
        LOG_DEBUG("Previous results count: " + std::to_string(g_variant_n_children(previous_results)));
        g_variant_unref(previous_results);
    }
    
    // Get the terms array (second parameter)
    GVariant* terms_array = g_variant_iter_next_value(&outer_iter);
    
    std::vector<std::string> search_terms = extractSearchTerms(terms_array);
    
    if (terms_array) {
        g_variant_unref(terms_array);
    }
    
    LOG_DEBUG("Total subsearch terms extracted: " + std::to_string(search_terms.size()));
    
    // Use command manager to search for matching actions
    std::vector<std::string> matches = command_manager_->searchActions(search_terms);
    
    LOG_DEBUG("Subsearch completed. Found " + std::to_string(matches.size()) + " matching actions");
    
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    for (const auto& id : matches) {
        g_variant_builder_add(&builder, "s", id.c_str());
        LOG_DEBUG("Found subsearch match: " + id);
    }
    
    LOG_DEBUG("Returning " + std::to_string(matches.size()) + " subsearch results");
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
}

void DBusSearchProvider::handleGetResultMetas(GVariant* parameters, GDBusMethodInvocation* invocation) {
    LOG_DEBUG("Processing GetResultMetas request...");
    GVariantIter iter;
    gchar* id;
    
    g_variant_iter_init(&iter, parameters);
    GVariant* ids_array = g_variant_iter_next_value(&iter);
    
    GVariantBuilder outer;
    g_variant_builder_init(&outer, G_VARIANT_TYPE("aa{sv}"));
    
    if (ids_array) {
        GVariantIter ids_iter;
        g_variant_iter_init(&ids_iter, ids_array);
        
        while (g_variant_iter_next(&ids_iter, "s", &id)) {
            LOG_DEBUG("Getting meta for ID: " + std::string(id));
            const Action* action = command_manager_->getAction(id);
            if (action) {
                GVariantBuilder meta;
                g_variant_builder_init(&meta, G_VARIANT_TYPE("a{sv}"));
                g_variant_builder_add(&meta, "{sv}", "id", g_variant_new_string(id));
                g_variant_builder_add(&meta, "{sv}", "name", g_variant_new_string(action->name.c_str()));
                g_variant_builder_add(&meta, "{sv}", "description", g_variant_new_string(action->description.c_str()));
                g_variant_builder_add(&meta, "{sv}", "icon", g_variant_new_string(action->icon.c_str()));
                
                g_variant_builder_add(&outer, "a{sv}", &meta);
            } else {
                LOG_DEBUG("No action found for ID: " + std::string(id));
            }
            g_free(id);
        }
        g_variant_unref(ids_array);
    }
    
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(aa{sv})", &outer));
}

void DBusSearchProvider::handleActivateResult(GVariant* parameters, GDBusMethodInvocation* invocation) {
    LOG_DEBUG("Processing ActivateResult request...");
    const gchar* id;
    GVariant* terms_variant;
    guint32 timestamp;
    g_variant_get(parameters, "(&s@asu)", &id, &terms_variant, &timestamp);
    
    LOG_DEBUG("Activating result with ID: " + std::string(id));
    
    // Extract terms for potential use in command execution
    std::vector<std::string> terms;
    if (terms_variant) {
        GVariantIter terms_iter;
        g_variant_iter_init(&terms_iter, terms_variant);
        gchar* term;
        while (g_variant_iter_next(&terms_iter, "s", &term)) {
            terms.push_back(std::string(term));
            g_free(term);
        }
        g_variant_unref(terms_variant);
    }
    
    // Use command manager to execute the action
    bool success = command_manager_->executeAction(id, terms);
    if (!success) {
        LOG_WARNING("Failed to execute action with ID: " + std::string(id));
    }
    
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

std::vector<std::string> DBusSearchProvider::extractSearchTerms(GVariant* terms_array) {
    std::vector<std::string> search_terms;
    
    LOG_DEBUG("Terms array is " + std::string(terms_array ? "not null" : "null"));
    
    if (terms_array) {
        LOG_DEBUG("Terms array type: " + std::string(g_variant_get_type_string(terms_array)));
        LOG_DEBUG("Terms array size: " + std::to_string(g_variant_n_children(terms_array)));
        
        GVariantIter terms_iter;
        g_variant_iter_init(&terms_iter, terms_array);
        
        gchar* term;
        while (g_variant_iter_next(&terms_iter, "s", &term)) {
            std::string termStr(term);
            LOG_DEBUG("Search term: '" + termStr + "' (length: " + std::to_string(termStr.length()) + ")");
            search_terms.push_back(termStr);
            g_free(term);
        }
    }
    
    return search_terms;
}

void DBusSearchProvider::onBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    auto* provider = static_cast<DBusSearchProvider*>(user_data);
    LOG_DEBUG("Bus acquired: " + std::string(name));
    
    GDBusInterfaceVTable vtable = { handleMethodCall, nullptr, nullptr };
    GError* error = nullptr;
    
    provider->registration_id_ = g_dbus_connection_register_object(
        connection,
        Constants::DBUS_OBJECT_PATH,
        g_dbus_node_info_lookup_interface(provider->introspection_data_, Constants::DBUS_INTERFACE_NAME),
        &vtable,
        provider, nullptr, &error);
    
    if (provider->registration_id_ > 0) {
        LOG_DEBUG("Object registered successfully with ID: " + std::to_string(provider->registration_id_));
    } else {
        LOG_ERROR("Failed to register object: " + std::string(error ? error->message : "Unknown error"));
        if (error) g_error_free(error);
    }
}

void DBusSearchProvider::onNameAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    LOG_DEBUG("Name acquired successfully: " + std::string(name));
    LOG_INFO("GNOME Shell search provider registered successfully!");
}

void DBusSearchProvider::onNameLost(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    LOG_DEBUG("Name lost: " + std::string(name));
    if (connection == nullptr) {
        LOG_ERROR("Failed to connect to DBus session bus");
    } else {
        LOG_WARNING("Lost bus name - another service may have taken over, or GNOME Shell couldn't validate the search provider");
        LOG_INFO("Check that the search provider configuration is correctly installed.");
    }
}

} // namespace PrimeCuts
