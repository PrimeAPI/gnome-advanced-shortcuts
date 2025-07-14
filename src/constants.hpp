#pragma once

#include <string>

namespace PrimeCuts {

namespace Constants {
    // D-Bus related constants
    const char* const DBUS_SERVICE_NAME = "de.primeapi.PrimeCuts";
    const char* const DBUS_OBJECT_PATH = "/de/primeapi/PrimeCuts";
    const char* const DBUS_INTERFACE_NAME = "org.gnome.Shell.SearchProvider2";
    
    // Method names
    const char* const METHOD_GET_INITIAL_RESULT_SET = "GetInitialResultSet";
    const char* const METHOD_GET_SUBSEARCH_RESULT_SET = "GetSubsearchResultSet";
    const char* const METHOD_GET_RESULT_METAS = "GetResultMetas";
    const char* const METHOD_ACTIVATE_RESULT = "ActivateResult";
    
    // Default commands
    const char* const DEFAULT_TERMINAL_COMMAND = "gnome-terminal";
    const char* const DEFAULT_BROWSER_COMMAND = "xdg-open";
    
    // Configuration
    const char* const DEFAULT_CONFIG_SUBDIR = "/.config/primecuts/";
    const char* const DEFAULT_CONFIG_FILENAME = "config.json";
    
    // Global settings keys
    const char* const SETTING_TERMINAL_COMMAND = "terminal_command";
    const char* const SETTING_BROWSER_COMMAND = "browser_command";
    const char* const SETTING_ENABLE_NOTIFICATIONS = "enable_notifications";
    
    // Command line arguments
    const char* const ARG_DEBUG = "--debug";
    
    // Virtual search actions
    const char* const SEARCH_GOOGLE_ID = "_search_google";
    const char* const SEARCH_CHATGPT_ID = "_search_chatgpt";
    const char* const GOOGLE_SEARCH_URL = "https://www.google.com/search?q=";
    const char* const CHATGPT_SEARCH_URL = "https://chatgpt.com/?q=";
}

} // namespace PrimeCuts
