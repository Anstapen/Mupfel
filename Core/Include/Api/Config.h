/**
 * \file   Config.h
 * \brief  Reading and writing configuration entries.
 * 
 * This header provides utility functions to get and set configuration items
 * in "mupfel.ini".
 */
#pragma once
#include "Core/Application.h"
#include "Core/ConfigManager.h"
#include <optional>
#include <string>
#include <utility>

namespace Mupfel::Config
{

/**
 * Reads the entry \a key, converted to T.
 *
 * \return The value, or std::nullopt if the key is absent or does not parse as T.
 */
template <ConfigValue T> [[nodiscard]] inline std::optional<T> Get(const std::string& key)
{
	return Application::GetConfigEntry<T>(key);
}

/**
 * Writes \a value to the entry \a key, in memory.
 * 
 * \warning This function does not save the configuration to disk. It is only saved upon
 * application exit.
 */
template <ConfigValue T> inline void Set(const std::string& key, T value)
{
	Application::SetConfigEntry<T>(key, std::move(value));
}

} // namespace Mupfel::Config
