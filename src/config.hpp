#pragma once

#include <string>
#include <vector>
#include <map>

namespace PrimeCuts {

enum class ActionType {
    COMMAND,
    TERMINAL_COMMAND,
    URL,
    APPLICATION
};

struct Action {
    std::string id;
    std::string name;
    std::string description;
    std::string icon;
    ActionType type;
    std::string command;
    std::vector<std::string> keywords;
    std::map<std::string, std::string> extra_params;
    
    Action() = default;
    Action(const std::string& id, const std::string& name, const std::string& description, 
           const std::string& icon, ActionType type, const std::string& command, 
           const std::vector<std::string>& keywords = {})
        : id(id), name(name), description(description), icon(icon), 
          type(type), command(command), keywords(keywords) {}
};

struct Group {
    std::string name;
    std::string description;
    std::string icon;
    std::vector<Action> actions;
    
    Group() = default;
    Group(const std::string& name, const std::string& description, const std::string& icon)
        : name(name), description(description), icon(icon) {}
};

struct Config {
    std::vector<Group> groups;
    std::map<std::string, std::string> global_settings;
    
    void clear() {
        groups.clear();
        global_settings.clear();
    }
};

} // namespace PrimeCuts
