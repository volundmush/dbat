#include "dbat/Coordinates.h"

bool Coordinates::operator==(const Coordinates &other) const
{
    return x == other.x && y == other.y && z == other.z;
}

void Coordinates::apply(Direction dir)
{
    switch (dir)
    {
    case Direction::north:
        y += 1;
        break;
    case Direction::east:
        x += 1;
        break;
    case Direction::south:
        y -= 1;
        break;
    case Direction::west:
        x -= 1;
        break;
    case Direction::up:
        z += 1;
        break;
    case Direction::down:
        z -= 1;
        break;
    case Direction::northeast:
        x += 1;
        y += 1;
        break;
    case Direction::southeast:
        x += 1;
        y -= 1;
        break;
    case Direction::southwest:
        x -= 1;
        y -= 1;
        break;
    case Direction::northwest:
        x -= 1;
        y += 1;
        break;
    case Direction::inside:
    case Direction::outside:
        // No movement for inside/outside
        break;
    }
}

Coordinates Coordinates::get_direction_offset(Direction dir)
{
    auto offset = *this;
    offset.apply(dir);
    return offset;
}

Coordinates::operator bool() const
{
    return x != 0 || y != 0 || z != 0;
}