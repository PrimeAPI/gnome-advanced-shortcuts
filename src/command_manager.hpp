#pragma once

#include "config.hpp"
#include <vector>
#include <string>

namespace PrimeCuts {

class CommandManager {
public:
    CommandManager(const Config& config);
    ~CommandManager() = default;
    
    std::vector<std::string> searchActions(const std::vector<std::string>& terms);
    Action* getAction(const std::string& id);
    bool executeAction(const std::string& id, const std::vector<std::string>& terms = {});
    
    void updateConfig(const Config& config);
    std::vector<Action> getAllActions() const;
    
private:
    Config config_;
    std::map<std::string, Action*> action_map_;
    
    void rebuildActionMap();
    bool matchesTerms(const Action& action, const std::vector<std::string>& terms);
    std::string buildTerminalCommand(const std::string& command);
    bool executeCommand(const std::string& command);
    bool executeTerminalCommand(const std::string& command);
    bool executeUrl(const std::string& url);
};

} // namespace PrimeCuts
