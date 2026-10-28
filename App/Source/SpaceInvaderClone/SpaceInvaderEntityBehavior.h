#pragma once
#include "ECS/Entity.h"
#include "Events/UserInputEvent.h"
#include "Components/HealthPoints.h"

void SpaceShipMovement(Mupfel::Entity e, const MovementEvent& event);

void InvaderOnHit(Mupfel::Entity e);

void InvaderOnHPLoss(Mupfel::Entity e, const HealthPoints &hp);

void LaserOnHit(Mupfel::Entity e);