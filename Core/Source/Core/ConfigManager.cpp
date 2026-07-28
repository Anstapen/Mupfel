#include "ConfigManager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <ranges>

using namespace Mupfel;

void ConfigManager::LoadConfig(const std::string path)
{

	if (path.empty())
	{
		return;
	}

	std::ifstream in(path);

	std::string rawConfig = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

	if (rawConfig.empty())
	{
		return;
	}

	/* remove spaces that may have been added by the user. */
	CleanUpRawConfig(rawConfig);

	std::vector<std::string> configEntries = SplitRawConfig(rawConfig);

	FindExistingEntries(configEntries);
}

void ConfigManager::SaveConfig(const std::string path)
{
	if (path.empty() || configEntries.empty())
	{
		return;
	}

	std::ofstream out(path);

	for (auto& [key, value] : configEntries)
	{
		out << key << '=' << value << '\n';
	}
}

void ConfigManager::CleanUpRawConfig(std::string& config)
{
	std::erase(config, ' ');
	std::erase(config, '\t');
}

std::vector<std::string> ConfigManager::SplitRawConfig(const std::string config)
{
	return config | std::views::split('\n') |
		   std::views::transform([](auto&& line) { return std::string(line.begin(), line.end()); }) |
		   std::ranges::to<std::vector>();
}

void ConfigManager::FindExistingEntries(const std::vector<std::string>& config)
{
	if (config.empty())
	{
		return;
	}

	for (const auto& entry : config)
	{
		std::string key;
		std::string value;
		size_t		pos = entry.find('=');

		if (pos == std::string::npos)
		{
			/* configuration entry was not valid */
			continue;
		}

		key = entry.substr(0, pos);
		value = entry.substr(pos + 1, std::string::npos);

		if (!configEntries.contains(key))
		{
			configEntries[key] = value;
		}
	}
}
