#pragma once
#include "GUID.h"

namespace Mupfel {
	/**
	* @brief This is the abstract base class that defines an Event.
	* Currently events are basically just small data objects that hold information
	* about the relevant event.
	* 
	* Every event needs to be able to return a 64bit GUID.
	* This GUID shall be the same for every child class of IEvent that is why
	* there is a templated class in between that calls a static function of the
	* child class.
	* 
	* Also there is a timestamp that can be retrieved using GetTimeStamp for events
	* that need a more fine-grained timing than just the frame the event happend in.
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

	/**
	 * @brief The CRTP (Curiously Recurring Template Pattern) solution, so that a call
	 * to GetGUID() always returns the same value for every object of Derived.
	 * @tparam Derived the class that inherits from Event.
	 */
	template <typename Derived>
	class Event : public IEvent {
	public:
		constexpr uint64_t GetGUID() const override {
			return Derived::GetGUIDStatic();
		}
	};
}