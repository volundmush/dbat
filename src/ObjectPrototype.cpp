#include "dbat/ObjectPrototype.h"
#include "dbat/Object.h"

ObjectPrototype &ObjectPrototype::operator=(const ObjectPrototype &other)
{
    // basic proto data fields
    ThingPrototype::operator=(other);
    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;

    return *this;
}

ObjectPrototype::ObjectPrototype(const Object &other)
{
    name = strdup(other.getName());
    room_description = strdup(other.getRoomDescription());
    look_description = strdup(other.getLookDescription());
    short_description = strdup(other.getShortDescription());
    if (!other.extra_descriptions.empty())
    {
        for (const auto &ex : other.extra_descriptions)
        {
            auto e = new extra_descr_data();
            e->keyword = strdup(ex.keyword.c_str());
            e->description = strdup(ex.description.c_str());
            e->next = ex_description;
            ex_description = e;
        }
    }
    stats = other.stats;
    affect_flags = other.affect_flags;

    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;
}
