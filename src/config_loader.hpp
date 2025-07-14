#pragma once

#include "config.hpp"
#include <string>
#include <memory>

namespace PrimeCuts {

class ConfigLoader {
public:
    ConfigLoader() = default;
    ~ConfigLoader() = default;
    
    bool loadConfig(const std::string& config_path, Config& config);
    bool saveConfig(const std::string& config_path, const Config& config);
    void createDefaultConfig(Config& config);
    
private:
    bool loadFromJson(const std::string& content, Config& config);
    std::string saveToJson(const Config& config);
    std::string getDefaultConfigPath();
    
    // JSON parsing helpers
    size_t findMatchingBrace(const std::string& content, size_t start_pos);
    bool parseGroup(const std::string& group_content, Group& group);
    bool parseAction(const std::string& action_content, Action& action);
    void parseGlobalSettings(const std::string& content, Config& config);
    std::string extractStringValue(const std::string& content, const std::string& key);
    std::vector<std::string> extractStringArray(const std::string& content, const std::string& key);
    ActionType stringToActionType(const std::string& type_str);
};

} // namespace PrimeCuts
