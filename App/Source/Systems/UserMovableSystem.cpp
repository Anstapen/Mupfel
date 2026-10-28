#include "UserMovableSystem.h"
#include "Mupfel.h"
#include "Events/UserInputEvent.h"
#include "Components/UserMovable.h"

void UserMovableSystem::Update()
{
    for (const auto &evt : Mupfel::Events::Get<MovementEvent>())
    {
        for (const auto& [e, um] : Mupfel::Entities::Each<UserMovable>())
        {
            if (um.onUpdate)
            {
                um.onUpdate(e, evt);
            }
        }
    }
}
