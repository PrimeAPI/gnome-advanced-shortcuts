#include "command_manager.hpp"
#include "logger.hpp"
#include "constants.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cctype>
#include <cstdio>

namespace PrimeCuts {

CommandManager::CommandManager(const Config& config) : config_(config) {
    rebuildActionMap();
}

void CommandManager::updateConfig(const Config& config) {
    config_ = config;
    rebuildActionMap();
}

void CommandManager::rebuildActionMap() {
    action_map_.clear();
    for (auto& group : config_.groups) {
        for (auto& action : group.actions) {
            action_map_[action.id] = &action;
        }
    }
}

std::vector<Action> CommandManager::getAllActions() const {
    std::vector<Action> actions;
    for (const auto& group : config_.groups) {
        for (const auto& action : group.actions) {
            actions.push_back(action);
        }
    }
    return actions;
}

std::vector<std::string> CommandManager::searchActions(const std::vector<std::string>& terms) const {
    // Store search terms for virtual actions
    current_search_terms_ = terms;
    
    std::vector<std::string> matches;
    
    std::stringstream debug_msg;
    debug_msg << "Searching with " << terms.size() << " terms: ";
    for (const auto& term : terms) {
        debug_msg << "'" << term << "' ";
    }
    LOG_DEBUG(debug_msg.str());
    
    // Search regular actions
    for (const auto& group : config_.groups) {
        for (const auto& action : group.actions) {
            if (matchesTerms(action, terms)) {
                matches.push_back(action.id);
                LOG_DEBUG("Action matched: " + action.name + " (ID: " + action.id + ")");
            }
        }
    }
    
    // Add virtual search actions if there are search terms
    if (!terms.empty()) {
        matches.push_back(Constants::SEARCH_GOOGLE_ID);
        matches.push_back(Constants::SEARCH_CHATGPT_ID);
        LOG_DEBUG("Added virtual search actions for: " + joinTerms(terms));
    }
    
    LOG_DEBUG("Total matches found: " + std::to_string(matches.size()));
    return matches;
}

bool CommandManager::matchesTerms(const Action& action, const std::vector<std::string>& terms) const {
    if (terms.empty()) {
        return false;
    }
    
    // Convert search terms to lowercase for case-insensitive matching
    std::vector<std::string> lower_terms;
    for (const auto& term : terms) {
        std::string lower_term = term;
        std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(), ::tolower);
        if (!lower_term.empty()) {
            lower_terms.push_back(lower_term);
        }
    }
    
    if (lower_terms.empty()) {
        return false;
    }
    
    // Check if any term matches
    for (const auto& term : lower_terms) {
        if (matchesSingleTerm(action, term)) {
            return true;
        }
    }
    
    return false;
}

bool CommandManager::matchesSingleTerm(const Action& action, const std::string& lower_term) const {
    auto toLower = [](const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    };
    
    // Check keywords
    for (const auto& keyword : action.keywords) {
        if (toLower(keyword).find(lower_term) != std::string::npos) {
            LOG_DEBUG("MATCH - Action '" + action.name + "' keyword '" + keyword + "' contains '" + lower_term + "'");
            return true;
        }
    }
    
    // Check name
    if (toLower(action.name).find(lower_term) != std::string::npos) {
        LOG_DEBUG("MATCH - Action '" + action.name + "' name contains '" + lower_term + "'");
        return true;
    }
    
    // Check description
    if (toLower(action.description).find(lower_term) != std::string::npos) {
        LOG_DEBUG("MATCH - Action '" + action.name + "' description contains '" + lower_term + "'");
        return true;
    }
    
    // Check ID
    if (toLower(action.id).find(lower_term) != std::string::npos) {
        LOG_DEBUG("MATCH - Action '" + action.name + "' ID contains '" + lower_term + "'");
        return true;
    }
    
    return false;
}

Action* CommandManager::getAction(const std::string& id) {
    // Handle virtual search actions - we need to return them as mutable for the main.cpp getAction calls
    // We'll use static storage to provide mutable access
    if (id == Constants::SEARCH_GOOGLE_ID) {
        static Action google_action;
        google_action = createGoogleSearchAction(current_search_terms_);
        return &google_action;
    }
    if (id == Constants::SEARCH_CHATGPT_ID) {
        static Action chatgpt_action;
        chatgpt_action = createChatGPTSearchAction(current_search_terms_);
        return &chatgpt_action;
    }
    
    auto it = action_map_.find(id);
    return (it != action_map_.end()) ? it->second : nullptr;
}

const Action* CommandManager::getAction(const std::string& id) const {
    // Handle virtual search actions
    if (id == Constants::SEARCH_GOOGLE_ID) {
        static Action google_action = createGoogleSearchAction(current_search_terms_);
        google_action = createGoogleSearchAction(current_search_terms_); // Refresh with current terms
        return &google_action;
    }
    if (id == Constants::SEARCH_CHATGPT_ID) {
        static Action chatgpt_action = createChatGPTSearchAction(current_search_terms_);
        chatgpt_action = createChatGPTSearchAction(current_search_terms_); // Refresh with current terms
        return &chatgpt_action;
    }
    
    // Handle regular actions
    auto it = action_map_.find(id);
    return (it != action_map_.end()) ? it->second : nullptr;
}

bool CommandManager::executeAction(const std::string& id, const std::vector<std::string>& terms) {
    // Handle virtual search actions
    if (id == Constants::SEARCH_GOOGLE_ID) {
        std::string search_query = joinTerms(!terms.empty() ? terms : current_search_terms_);
        std::string encoded_query = urlEncode(search_query);
        std::string url = Constants::GOOGLE_SEARCH_URL + encoded_query;
        return executeUrl(url);
    }
    if (id == Constants::SEARCH_CHATGPT_ID) {
        std::string search_query = joinTerms(!terms.empty() ? terms : current_search_terms_);
        std::string encoded_query = urlEncode(search_query);
        std::string url = Constants::CHATGPT_SEARCH_URL + encoded_query;
        return executeUrl(url);
    }
    
    // Handle regular actions
    Action* action = getAction(id);
    if (!action) {
        LOG_ERROR("Action not found: " + id);
        return false;
    }
    
    LOG_INFO("Executing action: " + action->name + " (" + action->id + ")");
    
    switch (action->type) {
        case ActionType::COMMAND:
            return executeCommand(action->command);
        case ActionType::TERMINAL_COMMAND:
            return executeTerminalCommand(action->command);
        case ActionType::URL:
            return executeUrl(action->command);
        case ActionType::APPLICATION:
            return executeCommand(action->command);
        default:
            LOG_ERROR("Unknown action type for: " + id);
            return false;
    }
}

std::string CommandManager::buildTerminalCommand(const std::string& command) const {
    auto it = config_.global_settings.find(Constants::SETTING_TERMINAL_COMMAND);
    std::string terminal_cmd = (it != config_.global_settings.end()) 
        ? it->second 
        : Constants::DEFAULT_TERMINAL_COMMAND;
    
    return terminal_cmd + " -- bash -c '" + command + "; echo \"Press Enter to close...\"; read'";
}

bool CommandManager::executeCommand(const std::string& command) const {
    LOG_INFO("Executing command: " + command);
    int result = system(command.c_str());
    if (result != 0) {
        LOG_WARNING("Command execution returned non-zero exit code: " + std::to_string(result));
    }
    return result == 0;
}

bool CommandManager::executeTerminalCommand(const std::string& command) const {
    std::string full_command = buildTerminalCommand(command);
    LOG_INFO("Executing terminal command: " + command);
    int result = system(full_command.c_str());
    if (result != 0) {
        LOG_WARNING("Terminal command execution returned non-zero exit code: " + std::to_string(result));
    }
    return result == 0;
}

bool CommandManager::executeUrl(const std::string& url) const {
    auto it = config_.global_settings.find(Constants::SETTING_BROWSER_COMMAND);
    std::string browser_cmd = (it != config_.global_settings.end()) 
        ? it->second 
        : Constants::DEFAULT_BROWSER_COMMAND;
    
    std::string full_command = browser_cmd + " '" + url + "'";
    LOG_INFO("Opening URL: " + url);
    int result = system(full_command.c_str());
    if (result != 0) {
        LOG_WARNING("URL opening returned non-zero exit code: " + std::to_string(result));
    }
    return result == 0;
}

Action CommandManager::createGoogleSearchAction(const std::vector<std::string>& terms) const {
    std::string search_query = joinTerms(terms);
    return Action(
        Constants::SEARCH_GOOGLE_ID,
        "Google",
        "Ask Google for your query",
        "web-browser",
        ActionType::URL,
        Constants::GOOGLE_SEARCH_URL + urlEncode(search_query)
    );
}

Action CommandManager::createChatGPTSearchAction(const std::vector<std::string>& terms) const {
    std::string search_query = joinTerms(terms);
    return Action(
        Constants::SEARCH_CHATGPT_ID,
        "ChatGPT",
        "Ask ChatGPT for your query",
        "web-browser",
        ActionType::URL,
        Constants::CHATGPT_SEARCH_URL + urlEncode(search_query)
    );
}

std::string CommandManager::joinTerms(const std::vector<std::string>& terms) const {
    if (terms.empty()) return "";
    
    std::string result = terms[0];
    for (size_t i = 1; i < terms.size(); ++i) {
        result += " " + terms[i];
    }
    return result;
}

std::string CommandManager::urlEncode(const std::string& str) const {
    std::string encoded;
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            encoded += '%';
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", static_cast<unsigned char>(c));
            encoded += hex;
        }
    }
    return encoded;
}

} // namespace PrimeCuts
