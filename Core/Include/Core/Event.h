#pragma once
#include <concepts>

namespace Mupfel {

	/**
	* @brief This is the abstract base class that defines an Event.
	* Currently events are basically just small data objects that hold information
	* about the relevant event.
	* 
	* Also there is a timestamp that can be retrieved using GetTimeStamp for events
	* that need a more fine-grained timing than just the frame the event happend in.
	*/
	class Event
	{
	public:
		/**
		 * @brief Destructor.
		 */
		virtual ~Event() = default;

		Event();

		/**
		 * @brief Returns the event timestamp.
		 * @return event timestamp.
		 */
		double GetTimeStamp() const;
	protected:
		/**
		 * @brief Timestamp of the Event.
		 */
		double ts;
	};

	/* This concept combines all type-related constraints for the child classes of Event. */
	template <typename T>
	concept EventType = std::derived_from<T, Event> && std::copy_constructible<T>;

}