#include "ProximityAnimator.h"
#include "json.hpp"

void ProximityAnimator::BuildFromJSON(const nlohmann::json& source)
{
    auto it = source.find("enter");
    if (it != source.end())
    {
        on_enter.BuildFromJSON(*it);
    }

    it = source.find("exit");
    if (it != source.end())
    {
        on_exit.BuildFromJSON(*it);
    }
}