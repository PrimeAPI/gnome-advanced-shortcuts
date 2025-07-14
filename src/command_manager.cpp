#include "command_manager.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>

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

std::vector<std::string> CommandManager::searchActions(const std::vector<std::string>& terms) {
    std::vector<std::string> matches;
    
    // Debug: print search terms
    std::cout << "DEBUG: Searching with '" << terms.size() << "' terms: ";
    for (const auto& term : terms) {
        std::cout << "'" << term << "' ";
    }
    std::cout << std::endl;
    
    for (const auto& group : config_.groups) {
        for (const auto& action : group.actions) {
            if (matchesTerms(action, terms)) {
                matches.push_back(action.id);
                std::cout << "DEBUG: Action matched: " << action.name << " (ID: " << action.id << ")" << std::endl;
            }
        }
    }
    
    std::cout << "DEBUG: Total matches found: " << matches.size() << std::endl;
    return matches;
}

bool CommandManager::matchesTerms(const Action& action, const std::vector<std::string>& terms) {
    // If no search terms provided, don't return any results
    if (terms.empty()) return false;
    
    // Convert search terms to lowercase for case-insensitive matching
    std::vector<std::string> lower_terms;
    for (const auto& term : terms) {
        std::string lower_term = term;
        std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(), ::tolower);
        // Skip empty terms
        if (!lower_term.empty()) {
            lower_terms.push_back(lower_term);
        }
    }
    
    // If all terms were empty, don't match
    if (lower_terms.empty()) return false;
    
    // At least one term must match for the action to be considered a match
    for (const auto& term : lower_terms) {
        // Check keywords
        for (const auto& keyword : action.keywords) {
            std::string lower_keyword = keyword;
            std::transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(), ::tolower);
            if (lower_keyword.find(term) != std::string::npos) {
                std::cout << "DEBUG: MATCH - Action '" << action.name << "' keyword '" << keyword << "' contains '" << term << "'" << std::endl;
                return true; // Found a match, return immediately
            }
        }
        
        // Check name
        std::string lower_name = action.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(term) != std::string::npos) {
            std::cout << "DEBUG: MATCH - Action '" << action.name << "' name contains '" << term << "'" << std::endl;
            return true; // Found a match, return immediately
        }
        
        // Check description
        std::string lower_desc = action.description;
        std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);
        if (lower_desc.find(term) != std::string::npos) {
            std::cout << "DEBUG: MATCH - Action '" << action.name << "' description contains '" << term << "'" << std::endl;
            return true; // Found a match, return immediately
        }
        
        // Check ID
        std::string lower_id = action.id;
        std::transform(lower_id.begin(), lower_id.end(), lower_id.begin(), ::tolower);
        if (lower_id.find(term) != std::string::npos) {
            std::cout << "DEBUG: MATCH - Action '" << action.name << "' ID contains '" << term << "'" << std::endl;
            return true; // Found a match, return immediately
        }
    }
    
    // No terms matched
    return false;
}

Action* CommandManager::getAction(const std::string& id) {
    auto it = action_map_.find(id);
    return (it != action_map_.end()) ? it->second : nullptr;
}

bool CommandManager::executeAction(const std::string& id, const std::vector<std::string>& terms) {
    Action* action = getAction(id);
    if (!action) {
        std::cerr << "Action not found: " << id << std::endl;
        return false;
    }
    
    std::cout << "Executing action: " << action->name << " (" << action->id << ")" << std::endl;
    
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
            std::cerr << "Unknown action type for: " << id << std::endl;
            return false;
    }
}

std::string CommandManager::buildTerminalCommand(const std::string& command) {
    std::string terminal_cmd = config_.global_settings.count("terminal_command") 
        ? config_.global_settings.at("terminal_command") 
        : "gnome-terminal";
    
    return terminal_cmd + " -- bash -c '" + command + "; echo \"Press Enter to close...\"; read'";
}

bool CommandManager::executeCommand(const std::string& command) {
    std::cout << "Executing command: " << command << std::endl;
    int result = system(command.c_str());
    return result == 0;
}

bool CommandManager::executeTerminalCommand(const std::string& command) {
    std::string full_command = buildTerminalCommand(command);
    std::cout << "Executing terminal command: " << full_command << std::endl;
    int result = system(full_command.c_str());
    return result == 0;
}

bool CommandManager::executeUrl(const std::string& url) {
    std::string browser_cmd = config_.global_settings.count("browser_command") 
        ? config_.global_settings.at("browser_command") 
        : "xdg-open";
    
    std::string full_command = browser_cmd + " '" + url + "'";
    std::cout << "Opening URL: " << url << std::endl;
    int result = system(full_command.c_str());
    return result == 0;
}

} // namespace PrimeCuts
