#include "MovementSystem.h"
#include "Core/Application.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"
#include "ECS/Registry.h"
#include "Core/Profiler.h"
#include <algorithm>
#include <memory>
#include <iostream>

using namespace Mupfel;

MovementSystem::MovementSystem()
{
}

MovementSystem::~MovementSystem()
{
}

void Mupfel::MovementSystem::Init()
{
}

void MovementSystem::DeInit()
{
}

void MovementSystem::Update(double elapsedTime)
{
	Move(elapsedTime);
}

void Mupfel::MovementSystem::Move(double elapsedTime)
{
	// implement cpu based move
}
