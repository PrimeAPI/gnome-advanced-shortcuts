#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include "config.hpp"
#include <string>

namespace PrimeCuts {

class ConfigLoader {
public:
    ConfigLoader();
    bool loadFromFile(const std::string& filename, Config& config);
    bool saveToFile(const std::string& filename, const Config& config);

private:
    bool parseJson(const std::string& content, Config& config);
    bool loadFromJson(const std::string& content, Config& config);
};

}

#endif // CONFIG_LOADER_HPP
