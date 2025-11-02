#pragma once
#include "ArchetypeArray.h"
#include <algorithm>
#include <cassert>

using namespace Mupfel;

bool ArchetypeArray::Has(Entity e) const
{
	return e.Index() < sparse.size() && sparse[e.Index()] != invalid_entry;
}

void ArchetypeArray::Remove(Entity e)
{

	if (!Has(e))
	{
		return;
	}

	size_t comp_index = sparse[e.Index()];
	size_t last_index = dense.size() - 1;

	/* Swap the element that should be removed with the last one */
	std::swap(dense[comp_index], dense[last_index]);

	for (auto& e : components)
	{
		if (!e)
		{
			continue;
		}

		e.get()->swap(comp_index, last_index);
		e.get()->pop_back();
	}

	/* the component order changed, update the sparse list */
	sparse[dense[comp_index]] = comp_index;

	/* Delete the last component */
	dense.pop_back();

	/* invalidate the component reference */
	sparse[e.Index()] = invalid_entry;
}