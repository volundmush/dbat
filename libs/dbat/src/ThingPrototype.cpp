#include "dbat/ThingPrototype.h"

ThingPrototype::~ThingPrototype()
{
    if (name)
        free(name);
    if (room_description)
        free(room_description);
    if (look_description)
        free(look_description);
    if (short_description)
        free(short_description);
}

ThingPrototype &ThingPrototype::operator=(const ThingPrototype &other)
{
    if (this == &other)
        return *this; // self-assignment check

    // basic proto data fields
    vn = other.vn;
    if (name)
        free(name);
    if (room_description)
        free(room_description);
    if (look_description)
        free(look_description);
    if (short_description)
        free(short_description);

    name = other.name ? strdup(other.name) : nullptr;
    room_description = other.room_description ? strdup(other.room_description) : nullptr;
    look_description = other.look_description ? strdup(other.look_description) : nullptr;
    short_description = other.short_description ? strdup(other.short_description) : nullptr;
    extra_descriptions = other.extra_descriptions;
    affect_flags = other.affect_flags;
    stats = other.stats;

    return *this;
}

std::string ThingPrototype::scriptString() const
{
    std::vector<std::string> vnums;
    for (auto p : proto_script)
        vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}