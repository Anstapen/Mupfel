#include "UserMovableSystem.h"
#include "Mupfel.h"
#include "MovementEvent.h"
#include "Components/UserMovable.h"

void UserMovableSystem::Update()
{
    for (const auto &evt : Mupfel::Events::Get<MovementEvent>())
    {
        for (auto [e, um] : Mupfel::Entities::Each<UserMovable>())
        {
            um.onUpdate(e, evt);
        }
    }
}
