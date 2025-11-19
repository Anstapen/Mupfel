#include "Collider.h"

using namespace Mupfel;

float Mupfel::Collider::GetBoundingBox() const
{
	return bounding_box_size;
}

void Mupfel::Collider::SetCircle(float in_radius)
{
	/* Set the shape type */
	type = (uint32_t)ShapeType::Circle;

	/* Set the radius */
	radius = in_radius;

	/*
		Set the bounding box.
		Sides are radius times 2.
	*/
	bounding_box_size = radius * 2.0f;
}