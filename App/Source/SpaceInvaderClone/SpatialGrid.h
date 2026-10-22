#pragma once
#include <cstdint>
#include <vector>

class SpatialGrid
{
public:
    struct GridPoint
    {
        uint32_t x;
        uint32_t y;

        bool operator==(const GridPoint& other) const
        {
            return (other.x == x) && (other.y == y);
        }
    };

    SpatialGrid(uint32_t in_num_rows, uint32_t in_num_columns);
    [[nodiscard]] bool GetEntry(const GridPoint &point) const;
    void SetEntry(const GridPoint &point, bool val);

    void AdvancePosition(GridPoint &point) const;



    const uint32_t num_rows;
    const uint32_t num_columns;

private:
    [[nodiscard]] uint32_t GridPointToArrayIndex(const GridPoint &point) const;
    [[nodiscard]] GridPoint ArrayIndexToGridPoint(uint32_t index) const;
};
