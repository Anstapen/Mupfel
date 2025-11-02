#pragma once
#include "GUID.h"

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
		virtual ~Event() = default;
		float GetTimeStamp() const;
	protected:
		float ts= 0.0f;
	};

}