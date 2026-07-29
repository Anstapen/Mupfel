#pragma once
#include <cstdint>
#include <string>
#include <concepts>
#include <bitset>
#include "Logger/Logger.h"

namespace Mupfel
{
typedef uint32_t SceneHandle;

/* Forward declaration to register it as a friend. */
class Application;

class Scene
{
	friend class Application;

public:
	static constexpr uint32_t MAX_SCENES = 64;
	static constexpr uint32_t INVALID_HANDLE = MAX_SCENES;
	using SceneMask = std::bitset<MAX_SCENES>;

public:
	virtual ~Scene() = default;
	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnSwitchIn() = 0;
	virtual void OnSwitchOut() = 0;
	SceneHandle	 GetHandle() const;

protected:
	virtual void Serialize(const std::string& path) = 0;
	virtual void Deserialize(const std::string& path) = 0;

	Scene(SceneHandle in_handle, const std::string& name) : handle(in_handle) { logger = Logger::Create(name); }

protected:
	Logger::SafeLoggerPtr logger;
private:
	const SceneHandle handle;
};

/* This concept combines all type-related constraints for the child classes of Event. */
template <typename T>
concept SceneType = std::derived_from<T, Scene>;

} // namespace Mupfel
