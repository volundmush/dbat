#include "dbat/game/Coordinates.hpp"
#include <nlohmann/json.hpp>

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

void to_json(nlohmann::json& j, const Coordinates& unit)
{
    if(unit.x != 0) {
        j[+"x"] = unit.x;
    }
    if(unit.y != 0) {
        j[+"y"] = unit.y;
    }
    if(unit.z != 0) {
        j[+"z"] = unit.z;
    }
}

void from_json(const nlohmann::json& j, Coordinates& unit)
{
    if(j.contains(+"x")) {
        j.at(+"x").get_to(unit.x);
    }
    if(j.contains(+"y")) {
        j.at(+"y").get_to(unit.y);
    }
    if(j.contains(+"z")) {
        j.at(+"z").get_to(unit.z);
    }
}
