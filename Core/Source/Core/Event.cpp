#include "Event.h"
#include "Application.h"

using namespace Mupfel;

Mupfel::Event::Event() : ts(Application::GetTime()) {}

double Event::GetTimeStamp() const {
	return ts;
}