#include "Collider.h"
#include <cassert>

using namespace Mupfel;

float Mupfel::Collider::GetBoundingBox() const
{
	return spatial.bounding_box_size;
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
	spatial.bounding_box_size = radius * 2.0f;
}

float Mupfel::Collider::GetCircle() const
{
	assert(info.type == ShapeType::Circle);
	return radius;
}
