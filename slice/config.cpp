#include "config.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

float Config::get(const char* parameter) {
    const auto entry = config.find(parameter);
    if (entry == config.end()) throw std::runtime_error(std::string("Missing configuration parameter: ") + parameter);
    return entry->second;
}

void Config::loadConfig(const char* filename) {
    std::ifstream input(filename);
    if (!input) throw std::runtime_error(std::string("Cannot open configuration: ") + filename);
    config.clear();
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream entry(line);
        std::string name, separator;
        float value;
        if (entry >> name >> separator >> value && separator == "=" && std::isfinite(value)) {
            config[name] = value;
        }
    }
}
