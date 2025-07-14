#pragma once

#include "config.hpp"
#include "command_manager.hpp"
#include <gio/gio.h>
#include <memory>

namespace PrimeCuts {

class DBusSearchProvider {
public:
    explicit DBusSearchProvider(std::unique_ptr<CommandManager> command_manager);
    ~DBusSearchProvider();
    
    // Delete copy constructor and assignment operator
    DBusSearchProvider(const DBusSearchProvider&) = delete;
    DBusSearchProvider& operator=(const DBusSearchProvider&) = delete;
    
    bool startService();
    void stopService();
    
    static void handleMethodCall(
        GDBusConnection* connection,
        const gchar* sender,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation,
        gpointer user_data);

private:
    std::unique_ptr<CommandManager> command_manager_;
    GMainLoop* main_loop_;
    GDBusNodeInfo* introspection_data_;
    guint owner_id_;
    guint registration_id_;
    
    void handleGetInitialResultSet(GVariant* parameters, GDBusMethodInvocation* invocation);
    void handleGetSubsearchResultSet(GVariant* parameters, GDBusMethodInvocation* invocation);
    void handleGetResultMetas(GVariant* parameters, GDBusMethodInvocation* invocation);
    void handleActivateResult(GVariant* parameters, GDBusMethodInvocation* invocation);
    
    std::vector<std::string> extractSearchTerms(GVariant* terms_array);
    
    static void onBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void onNameAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void onNameLost(GDBusConnection* connection, const gchar* name, gpointer user_data);
};

} // namespace PrimeCuts
