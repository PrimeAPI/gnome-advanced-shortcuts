#pragma once

#include "config.hpp"
#include <vector>
#include <string>
#include <map>

namespace PrimeCuts {

class CommandManager {
public:
    explicit CommandManager(const Config& config);
    ~CommandManager() = default;
    
    // Delete copy constructor and assignment operator
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    
    std::vector<std::string> searchActions(const std::vector<std::string>& terms) const;
    Action* getAction(const std::string& id);
    const Action* getAction(const std::string& id) const;
    bool executeAction(const std::string& id, const std::vector<std::string>& terms = {});
    
    void updateConfig(const Config& config);
    std::vector<Action> getAllActions() const;
    
private:
    Config config_;
    std::map<std::string, Action*> action_map_;
    mutable std::vector<std::string> current_search_terms_; // Store current search terms for virtual actions
    
    void rebuildActionMap();
    bool matchesTerms(const Action& action, const std::vector<std::string>& terms) const;
    bool matchesSingleTerm(const Action& action, const std::string& lower_term) const;
    std::string buildTerminalCommand(const std::string& command) const;
    bool executeCommand(const std::string& command) const;
    bool executeTerminalCommand(const std::string& command) const;
    bool executeUrl(const std::string& url) const;

    // Virtual search actions
    Action createGoogleSearchAction(const std::vector<std::string>& terms) const;
    Action createChatGPTSearchAction(const std::vector<std::string>& terms) const;
    std::string joinTerms(const std::vector<std::string>& terms) const;
    std::string urlEncode(const std::string& str) const;
};

} // namespace PrimeCuts
