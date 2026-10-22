#include "SpatialGrid.h"
#include "Mupfel.h"

using namespace Mupfel;

SpatialGrid::SpatialGrid(uint32_t in_num_rows, uint32_t in_num_columns) : num_rows(in_num_rows),
                                                                          num_columns(in_num_columns)
{
}


void SpatialGrid::AdvancePosition(GridPoint& point) const
{
    uint32_t index = GridPointToArrayIndex(point);

    if (index > 0)
    {
        --index;
    }

    point = ArrayIndexToGridPoint(index);
}

uint32_t SpatialGrid::GridPointToArrayIndex(const GridPoint& point) const
{
    return (point.y * num_columns) + point.x;
}

SpatialGrid::GridPoint SpatialGrid::ArrayIndexToGridPoint(uint32_t index) const
{
    return {.x = index % num_columns, .y = index / num_columns};
}
