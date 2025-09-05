#include "dbat/ObjectPrototype.h"
#include "dbat/Object.h"

std::map<obj_vnum, std::shared_ptr<ObjectPrototype>> obj_proto;

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
    extra_descriptions = other.extra_descriptions;
    stats = other.stats;
    affect_flags = other.affect_flags;

    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;
}
