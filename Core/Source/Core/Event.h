#pragma once
#include "GUID.h"

namespace Mupfel {
	/**
	* @brief This class provides a pure virtual getGUID function.
	*/
	class IEvent
	{
	public:
		virtual ~IEvent() = default;
		virtual constexpr uint64_t GetGUID() const = 0;
		float GetTimeStamp();
	protected:
		float ts= 0.0f;
	};

	template <typename Derived>
	class Event : public IEvent {
	public:
		constexpr uint64_t GetGUID() const override {
			return Derived::c_get_guid();
		}
	};
}