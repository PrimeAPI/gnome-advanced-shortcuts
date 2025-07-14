#include <gio/gio.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>

static bool debug_mode = false;

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

std::map<std::string, std::string> bookmarks = {
    {"ssh1", "SSH to Server 1"},
    {"ssh2", "SSH to Dev Server"},
    {"ssh3", "SSH to Admin Box"}
};

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
        DEBUG_LOG("Processing GetInitialResultSet request...");
        GVariantIter iter;
        gchar* term;
        std::vector<std::string> matches;

        g_variant_iter_init(&iter, parameters);
        GVariant* terms_array = g_variant_iter_next_value(&iter);
        
        if (terms_array) {
            GVariantIter terms_iter;
            g_variant_iter_init(&terms_iter, terms_array);
            
            while (g_variant_iter_next(&terms_iter, "s", &term)) {
                std::string termStr(term);
                DEBUG_LOG("Search term: '" << termStr << "'");
                for (const auto& [key, label] : bookmarks) {
                    if (label.find(termStr) != std::string::npos || key.find(termStr) != std::string::npos) {
                        matches.push_back(key);
                        DEBUG_LOG("Found match: " << key << " -> " << label);
                    }
                }
                g_free(term);
            }
            g_variant_unref(terms_array);
        }

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        for (const auto& id : matches) {
            g_variant_builder_add(&builder, "s", id.c_str());
        }

        DEBUG_LOG("Returning " << matches.size() << " results");
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
    }

    else if (g_strcmp0(method_name, "GetSubsearchResultSet") == 0) {
        DEBUG_LOG("Processing GetSubsearchResultSet request...");
        GVariant* previous_results_variant;
        GVariant* terms_variant;
        GVariantIter terms_iter;
        gchar* term;
        std::vector<std::string> matches;

        g_variant_get(parameters, "(asas)", &previous_results_variant, &terms_variant);
        g_variant_iter_init(&terms_iter, terms_variant);
        
        // For subsearch, we can just filter the previous results or do a fresh search
        // In this simple case, we'll do a fresh search similar to GetInitialResultSet
        while (g_variant_iter_next(&terms_iter, "s", &term)) {
            std::string termStr(term);
            DEBUG_LOG("Subsearch term: '" << termStr << "'");
            for (const auto& [key, label] : bookmarks) {
                if (label.find(termStr) != std::string::npos || key.find(termStr) != std::string::npos) {
                    matches.push_back(key);
                    DEBUG_LOG("Found subsearch match: " << key << " -> " << label);
                }
            }
            g_free(term);
        }
        g_variant_unref(previous_results_variant);
        g_variant_unref(terms_variant);

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        for (const auto& id : matches) {
            g_variant_builder_add(&builder, "s", id.c_str());
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
                auto it = bookmarks.find(id);
                if (it != bookmarks.end()) {
                    GVariantBuilder meta;
                    g_variant_builder_init(&meta, G_VARIANT_TYPE("a{sv}"));
                    g_variant_builder_add(&meta, "{sv}", "id", g_variant_new_string(id));
                    g_variant_builder_add(&meta, "{sv}", "name", g_variant_new_string(it->second.c_str()));
                    g_variant_builder_add(&meta, "{sv}", "description", g_variant_new_string("PrimeCuts Shortcut"));
                    g_variant_builder_add(&meta, "{sv}", "icon", g_variant_new_string("utilities-terminal"));

                    g_variant_builder_add(&outer, "a{sv}", &meta);
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
        GVariantIter* terms;
        guint32 timestamp;
        g_variant_get(parameters, "(&s@asu)", &id, &terms, &timestamp);

        DEBUG_LOG("Activating result with ID: " << id);
        auto it = bookmarks.find(id);
        if (it != bookmarks.end()) {
            std::string cmd = "gnome-terminal -- bash -c 'echo " + it->second + "; exec bash'";
            DEBUG_LOG("Executing command: " << cmd);
            system(cmd.c_str());
        } else {
            DEBUG_LOG("No bookmark found for ID: " << id);
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
    } else {
        std::cout << "PrimeCuts DBus service running..." << std::endl;
    }
    g_main_loop_run(loop);

    g_bus_unown_name(owner_id);
    g_main_loop_unref(loop);
    g_dbus_node_info_unref(introspection_data);
    return 0;
}
