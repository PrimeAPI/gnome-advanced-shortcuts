#pragma once

#include <iostream>
#include <string>

namespace PrimeCuts {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void setDebugMode(bool enabled) {
        debug_enabled_ = enabled;
    }
    
    bool isDebugEnabled() const {
        return debug_enabled_;
    }
    
    void log(LogLevel level, const std::string& message) const {
        if (level == LogLevel::DEBUG && !debug_enabled_) {
            return;
        }
        
        std::string prefix;
        switch (level) {
            case LogLevel::DEBUG: prefix = "[DEBUG] "; break;
            case LogLevel::INFO: prefix = "[INFO] "; break;
            case LogLevel::WARNING: prefix = "[WARNING] "; break;
            case LogLevel::ERROR: prefix = "[ERROR] "; break;
        }
        
        std::cout << prefix << message << std::endl;
    }
    
    void debug(const std::string& message) const {
        log(LogLevel::DEBUG, message);
    }
    
    void info(const std::string& message) const {
        log(LogLevel::INFO, message);
    }
    
    void warning(const std::string& message) const {
        log(LogLevel::WARNING, message);
    }
    
    void error(const std::string& message) const {
        log(LogLevel::ERROR, message);
    }

private:
    Logger() = default;
    bool debug_enabled_ = false;
};

#define LOG_DEBUG(msg) PrimeCuts::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) PrimeCuts::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) PrimeCuts::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) PrimeCuts::Logger::getInstance().error(msg)

} // namespace PrimeCuts
