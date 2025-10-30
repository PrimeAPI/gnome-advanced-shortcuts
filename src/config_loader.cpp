#include "config_loader.hpp"
#include "logger.hpp"
#include "json11.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace PrimeCuts {

ActionType stringToActionType(const std::string& s) {
    if (s == "command") return ActionType::COMMAND;
    if (s == "terminal_command") return ActionType::TERMINAL_COMMAND;
    if (s == "url") return ActionType::URL;
    if (s == "application") return ActionType::APPLICATION;
    return ActionType::COMMAND; // Default
}

std::string actionTypeToString(ActionType type) {
    switch (type) {
        case ActionType::COMMAND: return "command";
        case ActionType::TERMINAL_COMMAND: return "terminal_command";
        case ActionType::URL: return "url";
        case ActionType::APPLICATION: return "application";
    }
    return "command";
}

ConfigLoader::ConfigLoader() {}

bool ConfigLoader::loadFromFile(const std::string& filename, Config& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::getInstance().log(LogLevel::ERROR, "Error: Could not open config file: " + filename);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromJson(buffer.str(), config);
}

bool ConfigLoader::saveToFile(const std::string& filename, const Config& config) {
    json11::Json::object json_obj;
    json11::Json::array group_array;

    for (const auto& group : config.groups) {
        json11::Json::object group_obj;
        group_obj["name"] = group.name;
        group_obj["description"] = group.description;
        group_obj["icon"] = group.icon;

        json11::Json::array action_array;
        for (const auto& action : group.actions) {
            json11::Json::object action_obj;
            action_obj["id"] = action.id;
            action_obj["name"] = action.name;
            action_obj["description"] = action.description;
            action_obj["icon"] = action.icon;
            action_obj["type"] = actionTypeToString(action.type);
            action_obj["command"] = action.command;
            action_obj["keywords"] = json11::Json(action.keywords);
            action_array.push_back(action_obj);
        }
        group_obj["actions"] = action_array;
        group_array.push_back(group_obj);
    }
    json_obj["groups"] = group_array;

    json11::Json::object global_settings_obj;
    for (const auto& setting : config.global_settings) {
        global_settings_obj[setting.first] = setting.second;
    }
    json_obj["global_settings"] = global_settings_obj;

    std::ofstream file(filename);
    if (!file.is_open()) {
        Logger::getInstance().log(LogLevel::ERROR, "Error: Could not open config file for writing: " + filename);
        return false;
    }

    file << json11::Json(json_obj).dump();
    return true;
}

// Private methods
bool ConfigLoader::loadFromJson(const std::string& content, Config& config) {
    return parseJson(content, config);
}

bool ConfigLoader::parseJson(const std::string& jsonContent, Config& config) {
    std::string err;
    const auto json = json11::Json::parse(jsonContent, err);

    if (!err.empty()) {
        Logger::getInstance().log(LogLevel::ERROR, "Error: Failed to parse config JSON: " + err);
        return false;
    }

    // Implementation of JSON parsing
    if (json.is_object()) {
        const auto& groups = json["groups"];
        if (groups.is_array()) {
            for (const auto& group_item : groups.array_items()) {
                Group group;
                group.name = group_item["name"].string_value();
                group.description = group_item["description"].string_value();
                group.icon = group_item["icon"].string_value();

                const auto& actions = group_item["actions"];
                if (actions.is_array()) {
                    for (const auto& action_item : actions.array_items()) {
                        Action action;
                        action.id = action_item["id"].string_value();
                        action.name = action_item["name"].string_value();
                        action.description = action_item["description"].string_value();
                        action.icon = action_item["icon"].string_value();
                        action.type = stringToActionType(action_item["type"].string_value());
                        action.command = action_item["command"].string_value();

                        const auto& keywords = action_item["keywords"];
                        if (keywords.is_array()) {
                            for (const auto& keyword_item : keywords.array_items()) {
                                action.keywords.push_back(keyword_item.string_value());
                            }
                        }
                        group.actions.push_back(action);
                    }
                }
                config.groups.push_back(group);
            }
        }

        const auto& global_settings = json["global_settings"];
        if (global_settings.is_object()) {
            config.global_settings["terminal_command"] = global_settings["terminal_command"].string_value();
            config.global_settings["browser_command"] = global_settings["browser_command"].string_value();
            config.global_settings["enable_notifications"] = global_settings["enable_notifications"].string_value();
        }
        return true;
    }
    return false;
}

}
