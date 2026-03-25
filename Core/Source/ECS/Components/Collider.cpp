#include "Collider.h"
#include <cassert>

using namespace Mupfel;

float Mupfel::Collider::GetBoundingBoxX() const
{
	return bounding_box_x;
}

float Mupfel::Collider::GetBoundingBoxY() const
{
	return bounding_box_y;
}

void Mupfel::Collider::SetCircle(float in_radius)
{
	/* Set the shape type */
	info.type = ShapeType::Circle;

	/* Set the radius */
	radius = in_radius;

	/*
		Set the bounding box.
		Sides are radius times 2.
	*/
	bounding_box_x = radius * 2.0f;
	bounding_box_y = radius * 2.0f;
}

float Mupfel::Collider::GetCircle() const
{
	assert(info.type == ShapeType::Circle);
	return radius;
}

void Mupfel::Collider::SetAABB(float x, float y)
{
}
