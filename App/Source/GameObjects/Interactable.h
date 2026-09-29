#pragma once

class Interactable
{
public:
	virtual ~Interactable() = default;
	virtual void CheckEvents() = 0;
};
