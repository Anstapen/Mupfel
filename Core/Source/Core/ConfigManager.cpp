#include "ConfigManager.h"
#include <fstream>
#include <algorithm>
#include <iostream>

using namespace Mupfel;

void ConfigManager::LoadConfig(const std::string path) {

    if(path.empty())
    {
        return;
    }

    std::ifstream in(path);

    std::string rawConfig = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

    if(rawConfig.empty())
    {
        return;
    }

    /* remove spaces that may have been added by the user. */
    CleanUpRawConfig(rawConfig);

    std::vector<std::string> configEntries = SplitRawConfig(rawConfig);

    FindExistingEntries(configEntries);
}

void ConfigManager::SaveConfig(const std::string path) {
    if(path.empty() || configEntries.empty())
    {
        return;
    }

    std::ofstream out(path);

    for(auto& [key, value] : configEntries)
    {
        out << key << '=' << value << '\n';
    }
}

void ConfigManager::CleanUpRawConfig(std::string config) {

    auto is_space = [] (int c) {
        return (c == ' ');
    };

    config.erase(remove_if(config.begin(), config.end(), is_space), config.end());
}

std::vector<std::string> ConfigManager::SplitRawConfig(const std::string config) {
    size_t pos = 0;
    std::string copied_config = config;
    std::string token;
    std::vector<std::string> configEntries;
    while ((pos = copied_config.find('\n')) != std::string::npos) {
        token = copied_config.substr(0, pos);
        configEntries.push_back(token);
        copied_config.erase(0, pos + 1);
    }
    configEntries.push_back(copied_config);

    return configEntries;
}

void ConfigManager::FindExistingEntries(const std::vector<std::string> &config) {
    if(config.empty())
    {
        return;
    }

    for(const auto &entry : config)
    {
        std::string key;
        std::string value;
        size_t pos = entry.find('=');

        if(pos == std::string::npos)
        {
            /* configuration entry was not valid */
            continue;
        }

        key = entry.substr(0, pos);
        value = entry.substr(pos + 1, std::string::npos);

        if(!configEntries.contains(key))
        {
            configEntries[key] = value;
        }
    }
}
