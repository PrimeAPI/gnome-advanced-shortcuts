#include "config_loader.hpp"
#include "logger.hpp"
#include "constants.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>

namespace PrimeCuts {

std::string ConfigLoader::getDefaultConfigPath() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + Constants::DEFAULT_CONFIG_SUBDIR + Constants::DEFAULT_CONFIG_FILENAME;
    }
    return std::string("./") + Constants::DEFAULT_CONFIG_FILENAME;
}

void ConfigLoader::createDefaultConfig(Config& config) {
    config.clear();
    
    // SSH Group
    Group sshGroup("SSH Connections", "Quick SSH connections to servers", "network-server");
    sshGroup.actions = {
        Action("ssh_prod", "Production Server", "SSH to production server", "network-server", 
               ActionType::TERMINAL_COMMAND, "ssh user@prod.example.com", {"ssh", "prod", "production"}),
        Action("ssh_dev", "Development Server", "SSH to development server", "network-server", 
               ActionType::TERMINAL_COMMAND, "ssh user@dev.example.com", {"ssh", "dev", "development"}),
        Action("ssh_staging", "Staging Server", "SSH to staging server", "network-server", 
               ActionType::TERMINAL_COMMAND, "ssh user@staging.example.com", {"ssh", "staging", "stage"})
    };
    
    // Services Group
    Group servicesGroup("Services", "Start, stop, and restart system services", "applications-system");
    servicesGroup.actions = {
        Action("restart_apache", "Restart Apache", "Restart Apache web server", "applications-internet", 
               ActionType::TERMINAL_COMMAND, "sudo systemctl restart apache2", {"apache", "restart", "web"}),
        Action("restart_nginx", "Restart Nginx", "Restart Nginx web server", "applications-internet", 
               ActionType::TERMINAL_COMMAND, "sudo systemctl restart nginx", {"nginx", "restart", "web"}),
        Action("restart_mysql", "Restart MySQL", "Restart MySQL database server", "applications-databases", 
               ActionType::TERMINAL_COMMAND, "sudo systemctl restart mysql", {"mysql", "restart", "database", "db"}),
        Action("docker_status", "Docker Status", "Check Docker service status", "applications-system", 
               ActionType::TERMINAL_COMMAND, "sudo systemctl status docker", {"docker", "status", "container"})
    };
    
    // Development Group
    Group devGroup("Development", "Development tools and shortcuts", "applications-development");
    devGroup.actions = {
        Action("code_project", "Open VS Code", "Open current project in VS Code", "code", 
               ActionType::COMMAND, "code .", {"code", "vscode", "editor"}),
        Action("git_status", "Git Status", "Show git repository status", "git", 
               ActionType::TERMINAL_COMMAND, "git status", {"git", "status", "repo"}),
        Action("npm_install", "NPM Install", "Run npm install in current directory", "package-manager", 
               ActionType::TERMINAL_COMMAND, "npm install", {"npm", "install", "node"})
    };
    
    // Websites Group
    Group websitesGroup("Websites", "Quick access to frequently used websites", "applications-internet");
    websitesGroup.actions = {
        Action("github", "GitHub", "Open GitHub in browser", "github", 
               ActionType::URL, "https://github.com", {"github", "git", "repo"}),
        Action("stackoverflow", "Stack Overflow", "Open Stack Overflow", "stackoverflow", 
               ActionType::URL, "https://stackoverflow.com", {"stack", "overflow", "help", "code"}),
        Action("localhost", "Localhost", "Open localhost:3000", "applications-internet", 
               ActionType::URL, "http://localhost:3000", {"localhost", "local", "dev"}),
        Action("docs", "Documentation", "Open project documentation", "help-contents", 
               ActionType::URL, "https://docs.example.com", {"docs", "documentation", "help"})
    };
    
    config.groups = {sshGroup, servicesGroup, devGroup, websitesGroup};
    
    // Global settings
    config.global_settings[Constants::SETTING_TERMINAL_COMMAND] = Constants::DEFAULT_TERMINAL_COMMAND;
    config.global_settings[Constants::SETTING_BROWSER_COMMAND] = Constants::DEFAULT_BROWSER_COMMAND;
    config.global_settings[Constants::SETTING_ENABLE_NOTIFICATIONS] = "true";
}

bool ConfigLoader::loadConfig(const std::string& config_path, Config& config) {
    std::string path = config_path.empty() ? getDefaultConfigPath() : config_path;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARNING("Config file not found at: " + path);
        LOG_INFO("Creating default configuration...");
        createDefaultConfig(config);
        return saveConfig(path, config);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    return loadFromJson(content, config);
}

bool ConfigLoader::saveConfig(const std::string& config_path, const Config& config) {
    std::string path = config_path.empty() ? getDefaultConfigPath() : config_path;
    
    // Create directory if it doesn't exist
    size_t last_slash = path.find_last_of("/");
    if (last_slash != std::string::npos) {
        std::string dir = path.substr(0, last_slash);
        mkdir(dir.c_str(), 0755);
    }
    
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to create config file at: " + path);
        return false;
    }
    
    std::string json_content = saveToJson(config);
    file << json_content;
    file.close();
    
    LOG_INFO("Configuration saved to: " + path);
    return true;
}

// Simple JSON parser/writer (basic implementation)
bool ConfigLoader::loadFromJson(const std::string& content, Config& config) {
    // This is a simplified JSON parser for the basic structure
    // In a real implementation, you'd want to use a proper JSON library like nlohmann/json
    
    config.clear();
    
    try {
        // Parse JSON manually (basic implementation)
        size_t pos = 0;
        
        // Skip to groups array
        size_t groups_start = content.find("\"groups\"");
        if (groups_start == std::string::npos) {
            LOG_WARNING("No 'groups' section found in config, using default configuration");
            createDefaultConfig(config);
            return true;
        }
        
        size_t array_start = content.find("[", groups_start);
        if (array_start == std::string::npos) {
            LOG_WARNING("Invalid groups array format, using default configuration");
            createDefaultConfig(config);
            return true;
        }
        
        // Find each group object
        size_t current_pos = array_start + 1;
        while (current_pos < content.length()) {
            // Skip whitespace
            while (current_pos < content.length() && (content[current_pos] == ' ' || content[current_pos] == '\n' || content[current_pos] == '\t')) {
                current_pos++;
            }
            
            if (current_pos >= content.length() || content[current_pos] == ']') {
                break; // End of groups array
            }
            
            if (content[current_pos] == '{') {
                // Parse group object
                Group group;
                size_t group_end = findMatchingBrace(content, current_pos);
                if (group_end != std::string::npos) {
                    std::string group_content = content.substr(current_pos, group_end - current_pos + 1);
                    if (parseGroup(group_content, group)) {
                        config.groups.push_back(group);
                    }
                    current_pos = group_end + 1;
                }
            }
            
            // Skip to next group or end
            while (current_pos < content.length() && content[current_pos] != '{' && content[current_pos] != ']') {
                current_pos++;
            }
        }
        
        // Parse global settings
        parseGlobalSettings(content, config);
        
        LOG_INFO("Loaded configuration with " + std::to_string(config.groups.size()) + " groups");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error parsing JSON: " + std::string(e.what()));
        LOG_INFO("Loading default configuration instead...");
        createDefaultConfig(config);
        return true;
    }
}

std::string ConfigLoader::saveToJson(const Config& config) {
    std::stringstream json;
    json << "{\n";
    json << "  \"groups\": [\n";
    
    for (size_t g = 0; g < config.groups.size(); ++g) {
        const auto& group = config.groups[g];
        json << "    {\n";
        json << "      \"name\": \"" << group.name << "\",\n";
        json << "      \"description\": \"" << group.description << "\",\n";
        json << "      \"icon\": \"" << group.icon << "\",\n";
        json << "      \"actions\": [\n";
        
        for (size_t a = 0; a < group.actions.size(); ++a) {
            const auto& action = group.actions[a];
            json << "        {\n";
            json << "          \"id\": \"" << action.id << "\",\n";
            json << "          \"name\": \"" << action.name << "\",\n";
            json << "          \"description\": \"" << action.description << "\",\n";
            json << "          \"icon\": \"" << action.icon << "\",\n";
            json << "          \"type\": \"";
            switch (action.type) {
                case ActionType::COMMAND: json << "command"; break;
                case ActionType::TERMINAL_COMMAND: json << "terminal_command"; break;
                case ActionType::URL: json << "url"; break;
                case ActionType::APPLICATION: json << "application"; break;
            }
            json << "\",\n";
            json << "          \"command\": \"" << action.command << "\",\n";
            json << "          \"keywords\": [";
            for (size_t k = 0; k < action.keywords.size(); ++k) {
                json << "\"" << action.keywords[k] << "\"";
                if (k < action.keywords.size() - 1) json << ", ";
            }
            json << "]\n";
            json << "        }";
            if (a < group.actions.size() - 1) json << ",";
            json << "\n";
        }
        
        json << "      ]\n";
        json << "    }";
        if (g < config.groups.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ],\n";
    json << "  \"global_settings\": {\n";
    
    size_t setting_count = 0;
    for (const auto& [key, value] : config.global_settings) {
        json << "    \"" << key << "\": \"" << value << "\"";
        if (setting_count < config.global_settings.size() - 1) json << ",";
        json << "\n";
        setting_count++;
    }
    
    json << "  }\n";
    json << "}\n";
    
    return json.str();
}

// JSON parsing helper functions
size_t ConfigLoader::findMatchingBrace(const std::string& content, size_t start_pos) {
    if (start_pos >= content.length() || content[start_pos] != '{') {
        return std::string::npos;
    }
    
    int brace_count = 1;
    size_t pos = start_pos + 1;
    
    while (pos < content.length() && brace_count > 0) {
        if (content[pos] == '{') {
            brace_count++;
        } else if (content[pos] == '}') {
            brace_count--;
        }
        pos++;
    }
    
    return (brace_count == 0) ? pos - 1 : std::string::npos;
}

std::string ConfigLoader::extractStringValue(const std::string& content, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = content.find(search_key);
    if (key_pos == std::string::npos) {
        return "";
    }
    
    size_t colon_pos = content.find(":", key_pos);
    if (colon_pos == std::string::npos) {
        return "";
    }
    
    size_t quote_start = content.find("\"", colon_pos);
    if (quote_start == std::string::npos) {
        return "";
    }
    
    size_t quote_end = content.find("\"", quote_start + 1);
    if (quote_end == std::string::npos) {
        return "";
    }
    
    return content.substr(quote_start + 1, quote_end - quote_start - 1);
}

std::vector<std::string> ConfigLoader::extractStringArray(const std::string& content, const std::string& key) {
    std::vector<std::string> result;
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = content.find(search_key);
    if (key_pos == std::string::npos) {
        return result;
    }
    
    size_t colon_pos = content.find(":", key_pos);
    if (colon_pos == std::string::npos) {
        return result;
    }
    
    size_t array_start = content.find("[", colon_pos);
    if (array_start == std::string::npos) {
        return result;
    }
    
    size_t array_end = content.find("]", array_start);
    if (array_end == std::string::npos) {
        return result;
    }
    
    std::string array_content = content.substr(array_start + 1, array_end - array_start - 1);
    
    // Parse array elements
    size_t pos = 0;
    while (pos < array_content.length()) {
        size_t quote_start = array_content.find("\"", pos);
        if (quote_start == std::string::npos) {
            break;
        }
        
        size_t quote_end = array_content.find("\"", quote_start + 1);
        if (quote_end == std::string::npos) {
            break;
        }
        
        result.push_back(array_content.substr(quote_start + 1, quote_end - quote_start - 1));
        pos = quote_end + 1;
    }
    
    return result;
}

ActionType ConfigLoader::stringToActionType(const std::string& type_str) {
    if (type_str == "command") return ActionType::COMMAND;
    if (type_str == "terminal_command") return ActionType::TERMINAL_COMMAND;
    if (type_str == "url") return ActionType::URL;
    if (type_str == "application") return ActionType::APPLICATION;
    return ActionType::COMMAND; // default
}

bool ConfigLoader::parseAction(const std::string& action_content, Action& action) {
    action.id = extractStringValue(action_content, "id");
    action.name = extractStringValue(action_content, "name");
    action.description = extractStringValue(action_content, "description");
    action.icon = extractStringValue(action_content, "icon");
    action.command = extractStringValue(action_content, "command");
    action.keywords = extractStringArray(action_content, "keywords");
    
    std::string type_str = extractStringValue(action_content, "type");
    action.type = stringToActionType(type_str);
    
    return !action.id.empty() && !action.name.empty();
}

bool ConfigLoader::parseGroup(const std::string& group_content, Group& group) {
    group.name = extractStringValue(group_content, "name");
    group.description = extractStringValue(group_content, "description");
    group.icon = extractStringValue(group_content, "icon");
    
    // Find actions array
    size_t actions_start = group_content.find("\"actions\"");
    if (actions_start == std::string::npos) {
        return !group.name.empty();
    }
    
    size_t array_start = group_content.find("[", actions_start);
    if (array_start == std::string::npos) {
        return !group.name.empty();
    }
    
    // Find each action object
    size_t current_pos = array_start + 1;
    while (current_pos < group_content.length()) {
        // Skip whitespace
        while (current_pos < group_content.length() && 
               (group_content[current_pos] == ' ' || group_content[current_pos] == '\n' || 
                group_content[current_pos] == '\t')) {
            current_pos++;
        }
        
        if (current_pos >= group_content.length() || group_content[current_pos] == ']') {
            break; // End of actions array
        }
        
        if (group_content[current_pos] == '{') {
            // Parse action object
            Action action;
            size_t action_end = findMatchingBrace(group_content, current_pos);
            if (action_end != std::string::npos) {
                std::string action_content_str = group_content.substr(current_pos, action_end - current_pos + 1);
                if (parseAction(action_content_str, action)) {
                    group.actions.push_back(action);
                }
                current_pos = action_end + 1;
            }
        }
        
        // Skip to next action or end
        while (current_pos < group_content.length() && 
               group_content[current_pos] != '{' && group_content[current_pos] != ']') {
            current_pos++;
        }
    }
    
    return !group.name.empty();
}

void ConfigLoader::parseGlobalSettings(const std::string& content, Config& config) {
    size_t settings_start = content.find("\"global_settings\"");
    if (settings_start == std::string::npos) {
        // Set default global settings
        config.global_settings[Constants::SETTING_TERMINAL_COMMAND] = Constants::DEFAULT_TERMINAL_COMMAND;
        config.global_settings[Constants::SETTING_BROWSER_COMMAND] = Constants::DEFAULT_BROWSER_COMMAND;
        config.global_settings[Constants::SETTING_ENABLE_NOTIFICATIONS] = "true";
        return;
    }
    
    size_t object_start = content.find("{", settings_start);
    if (object_start == std::string::npos) {
        return;
    }
    
    size_t object_end = findMatchingBrace(content, object_start);
    if (object_end == std::string::npos) {
        return;
    }
    
    std::string settings_content = content.substr(object_start, object_end - object_start + 1);
    
    // Extract known settings
    std::string terminal_cmd = extractStringValue(settings_content, Constants::SETTING_TERMINAL_COMMAND);
    if (!terminal_cmd.empty()) {
        config.global_settings[Constants::SETTING_TERMINAL_COMMAND] = terminal_cmd;
    } else {
        config.global_settings[Constants::SETTING_TERMINAL_COMMAND] = Constants::DEFAULT_TERMINAL_COMMAND;
    }
    
    std::string browser_cmd = extractStringValue(settings_content, Constants::SETTING_BROWSER_COMMAND);
    if (!browser_cmd.empty()) {
        config.global_settings[Constants::SETTING_BROWSER_COMMAND] = browser_cmd;
    } else {
        config.global_settings[Constants::SETTING_BROWSER_COMMAND] = Constants::DEFAULT_BROWSER_COMMAND;
    }
    
    std::string notifications = extractStringValue(settings_content, Constants::SETTING_ENABLE_NOTIFICATIONS);
    if (!notifications.empty()) {
        config.global_settings[Constants::SETTING_ENABLE_NOTIFICATIONS] = notifications;
    } else {
        config.global_settings[Constants::SETTING_ENABLE_NOTIFICATIONS] = "true";
    }
}

} // namespace PrimeCuts
