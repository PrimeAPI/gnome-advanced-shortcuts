#include <gio/gio.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <memory>

#include "config.hpp"
#include "config_loader.hpp"
#include "command_manager.hpp"

static bool debug_mode = false;
static std::unique_ptr<PrimeCuts::CommandManager> command_manager;
static PrimeCuts::Config app_config;

#define DEBUG_LOG(x) do { if (debug_mode) { std::cout << x << std::endl; } } while(0)

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

bool initializeConfiguration() {
    PrimeCuts::ConfigLoader loader;
    
    // Try to load configuration, create default if not found
    if (!loader.loadConfig("", app_config)) {
        std::cerr << "Failed to load configuration" << std::endl;
        return false;
    }
    
    // Initialize command manager with the loaded config
    command_manager = std::make_unique<PrimeCuts::CommandManager>(app_config);
    
    DEBUG_LOG("Configuration loaded successfully with " << app_config.groups.size() << " groups");
    
    // Print loaded actions for debugging
    if (debug_mode) {
        for (const auto& group : app_config.groups) {
            DEBUG_LOG("Group: " << group.name << " (" << group.actions.size() << " actions)");
            for (const auto& action : group.actions) {
                DEBUG_LOG("  - " << action.name << " [" << action.id << "]");
            }
        }
    }
    
    return true;
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
    DEBUG_LOG("DBus method called: " << method_name << " from " << sender);
    
    if (g_strcmp0(method_name, "GetInitialResultSet") == 0) {
        DEBUG_LOG("Processing GetInitialResultSet request... [" << time(nullptr) << "]");
        DEBUG_LOG("Parameters type: " << g_variant_get_type_string(parameters));
        
        GVariantIter iter;
        gchar* term;
        std::vector<std::string> search_terms;

        g_variant_iter_init(&iter, parameters);
        GVariant* terms_array = g_variant_iter_next_value(&iter);
        
        DEBUG_LOG("Terms array is " << (terms_array ? "not null" : "null"));
        
        if (terms_array) {
            DEBUG_LOG("Terms array type: " << g_variant_get_type_string(terms_array));
            DEBUG_LOG("Terms array size: " << g_variant_n_children(terms_array));
            
            GVariantIter terms_iter;
            g_variant_iter_init(&terms_iter, terms_array);
            
            while (g_variant_iter_next(&terms_iter, "s", &term)) {
                std::string termStr(term);
                DEBUG_LOG("Search term: '" << termStr << "' (length: " << termStr.length() << ")");
                search_terms.push_back(termStr);
                g_free(term);
            }
            g_variant_unref(terms_array);
        }
        
        DEBUG_LOG("Total search terms extracted: " << search_terms.size());

        // Use command manager to search for matching actions
        std::vector<std::string> matches = command_manager->searchActions(search_terms);
        
        DEBUG_LOG("Search completed. Found " << matches.size() << " matching actions out of " << app_config.groups.size() << " total groups");

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        for (const auto& id : matches) {
            g_variant_builder_add(&builder, "s", id.c_str());
            DEBUG_LOG("Found match: " << id);
        }

        DEBUG_LOG("Returning " << matches.size() << " results");
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
    }

    else if (g_strcmp0(method_name, "GetSubsearchResultSet") == 0) {
        DEBUG_LOG("Processing GetSubsearchResultSet request... [" << time(nullptr) << "]");
        DEBUG_LOG("Parameters type: " << g_variant_get_type_string(parameters));
        
        gchar* term;
        std::vector<std::string> search_terms;

        // Use a safer approach to extract the parameters
        GVariantIter outer_iter;
        g_variant_iter_init(&outer_iter, parameters);
        
        // Skip the previous results array (first parameter)
        GVariant* previous_results = g_variant_iter_next_value(&outer_iter);
        if (previous_results) {
            DEBUG_LOG("Previous results count: " << g_variant_n_children(previous_results));
            g_variant_unref(previous_results);
        }
        
        // Get the terms array (second parameter)
        GVariant* terms_array = g_variant_iter_next_value(&outer_iter);
        
        DEBUG_LOG("Terms array is " << (terms_array ? "not null" : "null"));
        
        if (terms_array) {
            DEBUG_LOG("Terms array type: " << g_variant_get_type_string(terms_array));
            DEBUG_LOG("Terms array size: " << g_variant_n_children(terms_array));
            
            GVariantIter terms_iter;
            g_variant_iter_init(&terms_iter, terms_array);
            
            while (g_variant_iter_next(&terms_iter, "s", &term)) {
                std::string termStr(term);
                DEBUG_LOG("Subsearch term: '" << termStr << "' (length: " << termStr.length() << ")");
                search_terms.push_back(termStr);
                g_free(term);
            }
            g_variant_unref(terms_array);
        }
        
        DEBUG_LOG("Total subsearch terms extracted: " << search_terms.size());

        // Use command manager to search for matching actions
        std::vector<std::string> matches = command_manager->searchActions(search_terms);
        
        DEBUG_LOG("Subsearch completed. Found " << matches.size() << " matching actions");

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        for (const auto& id : matches) {
            g_variant_builder_add(&builder, "s", id.c_str());
            DEBUG_LOG("Found subsearch match: " << id);
        }

        DEBUG_LOG("Returning " << matches.size() << " subsearch results");
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
    }

    else if (g_strcmp0(method_name, "GetResultMetas") == 0) {
        DEBUG_LOG("Processing GetResultMetas request...");
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
                DEBUG_LOG("Getting meta for ID: " << id);
                PrimeCuts::Action* action = command_manager->getAction(id);
                if (action) {
                    GVariantBuilder meta;
                    g_variant_builder_init(&meta, G_VARIANT_TYPE("a{sv}"));
                    g_variant_builder_add(&meta, "{sv}", "id", g_variant_new_string(id));
                    g_variant_builder_add(&meta, "{sv}", "name", g_variant_new_string(action->name.c_str()));
                    g_variant_builder_add(&meta, "{sv}", "description", g_variant_new_string(action->description.c_str()));
                    g_variant_builder_add(&meta, "{sv}", "icon", g_variant_new_string(action->icon.c_str()));

                    g_variant_builder_add(&outer, "a{sv}", &meta);
                } else {
                    DEBUG_LOG("No action found for ID: " << id);
                }
                g_free(id);
            }
            g_variant_unref(ids_array);
        }

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(aa{sv})", &outer));
    }

    else if (g_strcmp0(method_name, "ActivateResult") == 0) {
        DEBUG_LOG("Processing ActivateResult request...");
        const gchar* id;
        GVariant* terms_variant;
        guint32 timestamp;
        g_variant_get(parameters, "(&s@asu)", &id, &terms_variant, &timestamp);

        DEBUG_LOG("Activating result with ID: " << id);
        
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
            DEBUG_LOG("Failed to execute action with ID: " << id);
        }

        g_dbus_method_invocation_return_value(invocation, nullptr);
    } else {
        DEBUG_LOG("Unknown method called: " << method_name);
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
}

static GDBusNodeInfo* introspection_data = nullptr;

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    DEBUG_LOG("Bus acquired: " << name);
    GDBusInterfaceVTable vtable = { handle_method_call, NULL, NULL };
    GError* error = NULL;
    guint registration_id = g_dbus_connection_register_object(
        connection,
        "/de/primeapi/PrimeCuts",
        g_dbus_node_info_lookup_interface(introspection_data, "org.gnome.Shell.SearchProvider2"),
        &vtable,
        NULL, NULL, &error);
    
    if (registration_id > 0) {
        DEBUG_LOG("Object registered successfully with ID: " << registration_id);
    } else {
        DEBUG_LOG("Failed to register object: " << (error ? error->message : "Unknown error"));
        if (error) g_error_free(error);
    }
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    DEBUG_LOG("Name acquired successfully: " << name);
    std::cout << "GNOME Shell search provider registered successfully!" << std::endl;
}

static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data) {
    DEBUG_LOG("Name lost: " << name);
    if (connection == nullptr) {
        std::cerr << "ERROR: Failed to connect to DBus session bus" << std::endl;
    } else {
        std::cerr << "WARNING: Lost bus name - another service may have taken over, or GNOME Shell couldn't validate the search provider" << std::endl;
        std::cerr << "Check that the search provider configuration is correctly installed." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Check for debug flag
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            break;
        }
    }
    
    DEBUG_LOG("Starting PrimeCuts DBus service...");
    
    // Initialize configuration before starting DBus service
    if (!initializeConfiguration()) {
        std::cerr << "Failed to initialize configuration. Exiting." << std::endl;
        return 1;
    }
    
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    
    if (!introspection_data) {
        std::cout << "Failed to parse introspection XML!" << std::endl;
        return 1;
    }
    DEBUG_LOG("Introspection data loaded successfully");

    DEBUG_LOG("Attempting to own bus name: de.primeapi.PrimeCuts");
    guint owner_id = g_bus_own_name(
        G_BUS_TYPE_SESSION,
        "de.primeapi.PrimeCuts",
        (GBusNameOwnerFlags)(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE),
        on_bus_acquired,
        on_name_acquired,
        on_name_lost,
        NULL, NULL);

    if (debug_mode) {
        std::cout << "PrimeCuts DBus service running in debug mode..." << std::endl;
        std::cout << "Configuration loaded with " << app_config.groups.size() << " action groups." << std::endl;
    } else {
        std::cout << "PrimeCuts DBus service running..." << std::endl;
    }
    g_main_loop_run(loop);

    g_bus_unown_name(owner_id);
    g_main_loop_unref(loop);
    g_dbus_node_info_unref(introspection_data);
    return 0;
}
