#include <gio/gio.h>
#include <iostream>
#include <memory>
#include <string>

#include "config.hpp"
#include "config_loader.hpp"
#include "command_manager.hpp"
#include "logger.hpp"
#include "constants.hpp"

static std::unique_ptr<PrimeCuts::CommandManager> command_manager;
static PrimeCuts::Config app_config;

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

namespace {

bool parseCommandLineArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == PrimeCuts::Constants::ARG_DEBUG) {
            PrimeCuts::Logger::getInstance().setDebugMode(true);
            return true;
        }
    }
    return false;
}

bool initializeConfiguration() {
    PrimeCuts::ConfigLoader loader;
    
    // Try to load configuration, create default if not found
    if (!loader.loadConfig("", app_config)) {
        LOG_ERROR("Failed to load configuration");
        return false;
    }
    
    // Initialize command manager with the loaded config
    command_manager = std::make_unique<PrimeCuts::CommandManager>(app_config);
    
    LOG_DEBUG("Configuration loaded successfully with " + std::to_string(app_config.groups.size()) + " groups");
    
    // Print loaded actions for debugging
    if (PrimeCuts::Logger::getInstance().isDebugEnabled()) {
        for (const auto& group : app_config.groups) {
            LOG_DEBUG("Group: " + group.name + " (" + std::to_string(group.actions.size()) + " actions)");
            for (const auto& action : group.actions) {
                LOG_DEBUG("  - " + action.name + " [" + action.id + "]");
            }
        }
    }
    
    return true;
}

std::vector<std::string> extractSearchTerms(GVariant* parameters, const std::string& method_name) {
    std::vector<std::string> search_terms;
    
    LOG_DEBUG("Processing " + method_name + " request...");
    LOG_DEBUG("Parameters type: " + std::string(g_variant_get_type_string(parameters)));
    
    GVariantIter iter;
    g_variant_iter_init(&iter, parameters);
    
    // For GetSubsearchResultSet, skip the first parameter (previous results)
    if (method_name == PrimeCuts::Constants::METHOD_GET_SUBSEARCH_RESULT_SET) {
        GVariant* previous_results = g_variant_iter_next_value(&iter);
        if (previous_results) {
            LOG_DEBUG("Previous results count: " + std::to_string(g_variant_n_children(previous_results)));
            g_variant_unref(previous_results);
        }
    }
    
    // Get the terms array
    GVariant* terms_array = g_variant_iter_next_value(&iter);
    
    if (terms_array) {
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
        g_variant_unref(terms_array);
    }
    
    LOG_DEBUG("Total search terms extracted: " + std::to_string(search_terms.size()));
    return search_terms;
}

void handleSearchRequest(GDBusMethodInvocation* invocation, GVariant* parameters, const std::string& method_name) {
    std::vector<std::string> search_terms = extractSearchTerms(parameters, method_name);
    
    // Use command manager to search for matching actions
    std::vector<std::string> matches = command_manager->searchActions(search_terms);
    
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

void handleGetResultMetas(GDBusMethodInvocation* invocation, GVariant* parameters) {
    LOG_DEBUG("Processing GetResultMetas request...");
    
    GVariantIter iter;
    g_variant_iter_init(&iter, parameters);
    GVariant* ids_array = g_variant_iter_next_value(&iter);

    GVariantBuilder outer;
    g_variant_builder_init(&outer, G_VARIANT_TYPE("aa{sv}"));

    if (ids_array) {
        GVariantIter ids_iter;
        g_variant_iter_init(&ids_iter, ids_array);
        
        gchar* id;
        while (g_variant_iter_next(&ids_iter, "s", &id)) {
            LOG_DEBUG("Getting meta for ID: " + std::string(id));
            const PrimeCuts::Action* action = command_manager->getAction(id);
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

void handleActivateResult(GDBusMethodInvocation* invocation, GVariant* parameters) {
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
    bool success = command_manager->executeAction(id, terms);
    if (!success) {
        LOG_DEBUG("Failed to execute action with ID: " + std::string(id));
    }

    g_dbus_method_invocation_return_value(invocation, nullptr);
}

static void handle_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data) 
{
    LOG_DEBUG("DBus method called: " + std::string(method_name) + " from " + std::string(sender));
    
    if (g_strcmp0(method_name, PrimeCuts::Constants::METHOD_GET_INITIAL_RESULT_SET) == 0 ||
        g_strcmp0(method_name, PrimeCuts::Constants::METHOD_GET_SUBSEARCH_RESULT_SET) == 0) {
        handleSearchRequest(invocation, parameters, method_name);
    }
    else if (g_strcmp0(method_name, PrimeCuts::Constants::METHOD_GET_RESULT_METAS) == 0) {
        handleGetResultMetas(invocation, parameters);
    }
    else if (g_strcmp0(method_name, PrimeCuts::Constants::METHOD_ACTIVATE_RESULT) == 0) {
        handleActivateResult(invocation, parameters);
    }
    else {
        LOG_DEBUG("Unknown method called: " + std::string(method_name));
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
}

static GDBusNodeInfo* introspection_data = nullptr;

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    LOG_DEBUG("Bus acquired: " + std::string(name));
    GDBusInterfaceVTable vtable = { handle_method_call, NULL, NULL };
    GError* error = NULL;
    guint registration_id = g_dbus_connection_register_object(
        connection,
        PrimeCuts::Constants::DBUS_OBJECT_PATH,
        g_dbus_node_info_lookup_interface(introspection_data, PrimeCuts::Constants::DBUS_INTERFACE_NAME),
        &vtable,
        NULL, NULL, &error);
    
    if (registration_id > 0) {
        LOG_DEBUG("Object registered successfully with ID: " + std::to_string(registration_id));
    } else {
        LOG_ERROR("Failed to register object: " + std::string(error ? error->message : "Unknown error"));
        if (error) g_error_free(error);
    }
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    LOG_DEBUG("Name acquired successfully: " + std::string(name));
    LOG_INFO("GNOME Shell search provider registered successfully!");
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    LOG_DEBUG("Name lost: " + std::string(name));
    if (connection == nullptr) {
        LOG_ERROR("Failed to connect to DBus session bus");
    } else {
        LOG_WARNING("Lost bus name - another service may have taken over, or GNOME Shell couldn't validate the search provider");
        LOG_WARNING("Check that the search provider configuration is correctly installed.");
    }
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool debug_mode = parseCommandLineArgs(argc, argv);
    
    LOG_DEBUG("Starting PrimeCuts DBus service...");
    
    // Initialize configuration before starting DBus service
    if (!initializeConfiguration()) {
        LOG_ERROR("Failed to initialize configuration. Exiting.");
        return 1;
    }
    
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    
    if (!introspection_data) {
        LOG_ERROR("Failed to parse introspection XML!");
        return 1;
    }
    LOG_DEBUG("Introspection data loaded successfully");

    LOG_DEBUG("Attempting to own bus name: " + std::string(PrimeCuts::Constants::DBUS_SERVICE_NAME));
    guint owner_id = g_bus_own_name(
        G_BUS_TYPE_SESSION,
        PrimeCuts::Constants::DBUS_SERVICE_NAME,
        (GBusNameOwnerFlags)(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE),
        on_bus_acquired,
        on_name_acquired,
        on_name_lost,
        NULL, NULL);

    if (debug_mode) {
        LOG_INFO("PrimeCuts DBus service running in debug mode...");
        LOG_INFO("Configuration loaded with " + std::to_string(app_config.groups.size()) + " action groups.");
    } else {
        LOG_INFO("PrimeCuts DBus service running...");
    }
    
    g_main_loop_run(loop);

    g_bus_unown_name(owner_id);
    g_main_loop_unref(loop);
    g_dbus_node_info_unref(introspection_data);
    return 0;
}
