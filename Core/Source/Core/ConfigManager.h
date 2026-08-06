#pragma once
#include <concepts>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Mupfel
{

template <typename T>
concept ConfigValue = std::default_initializable<T> && requires(std::stringstream& ss, T& t, const T& ct) {
	{ ss << ct } -> std::same_as<std::ostream&>;
	{ ss >> t } -> std::same_as<std::istream&>;
};

class ConfigManager
{
public:
	/**
	 * @brief Load a config file from the current working directory.
	 *
	 * @param path The path to the config file.
	 */
	void LoadConfig(const std::string path);

	/**
	 * @brief Save the current config to disk, in the file indicated by \a path.
	 *
	 * @param path The file to which the config should be saved.
	 */
	void SaveConfig(const std::string path);

	/**
	 * @brief Set the configuration entry indicated by \a key to \a value.
	 *
	 * This function takes \a value, converts it to \a T using the conversion
	 * operators of std::stringstream and saves the pair in the internal map.
	 * To save the configuration to disk, SaveConfig() needs to be called.
	 *
	 * @tparam T The type of the value that will be saved.
	 * @param key The name of the configuration entry.
	 * @param value The value of the configuration entry.
	 */
	template <ConfigValue T> void Set(const std::string key, T value);

	/**
	 * @brief Retrieve the configuration value indicated by \a key.
	 *
	 * Attempts to retrieve the configuration value indicated by \a key. The
	 * conversion operator of std::stringstream is used to convert the
	 * entry to \a T. If the entry does not exist, std::nullopt is returned.
	 *
	 * @tparam T The type to which the entry should be converted.
	 * @param key The name of the configuration entry.
	 * @return std::optional<T> The entry, converted to \a T, if present,
	 *         std::nullopt otherwise.
	 */
	template <ConfigValue T> std::optional<T> Get(const std::string key);

private:
	/**
	 * @brief Clean the given raw configuration string from unwanted characters.
	 *
	 * This function takes \a config and removes unwanted characters to simplify
	 * further parsing. Currently only whitespaces are removed.
	 *
	 * @param config The raw config string that should be cleaned.
	 */
	void CleanUpRawConfig(std::string& config);

	/**
	 * @brief Splits the given continuous config string based on the newline
	 * '\n' character.
	 *
	 * @param config The continuous config string.
	 * @return std::vector<std::string> An array of substrings.
	 */
	std::vector<std::string> SplitRawConfig(const std::string config);

	/**
	 * @brief Retrieve configuration entries based on lines.
	 *
	 * This function takes an array of lines (previously split by
	 * SpliRawConfig()) and creates a key-value pair based on the '=' character.
	 *
	 * @param config The array of lines.
	 */
	void FindExistingEntries(const std::vector<std::string>& config);

private:
	/** The map of existing configuration entries. */
	std::unordered_map<std::string, std::string> configEntries;
};

template <ConfigValue T> inline void ConfigManager::Set(const std::string key, T value)
{
	std::stringstream ss;
	if constexpr (std::is_floating_point_v<T>)
	{
		ss << std::setprecision(std::numeric_limits<T>::max_digits10);
	}
	if constexpr (std::is_integral_v<T> && sizeof(T) == 1)
	{
		ss << static_cast<int>(value);
	}
	else
	{
		ss << value;
	}
	configEntries[key] = ss.str();
}

template <ConfigValue T> inline std::optional<T> ConfigManager::Get(const std::string key)
{
	auto it = configEntries.find(key);
	if (it == configEntries.end())
	{
		return std::nullopt;
	}

	if constexpr (std::is_same_v<T, std::string>)
	{
		return it->second;
	}
	else
	{
		std::stringstream ss{it->second};
		T				  t{};
		if (!(ss >> t))
		{
			return std::nullopt;
		}
		return t;
	}
}
} // namespace Mupfel
