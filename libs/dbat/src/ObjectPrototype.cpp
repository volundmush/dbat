#include "dbat/ObjectPrototype.h"
#include "dbat/Object.h"

std::map<obj_vnum, std::shared_ptr<ObjectPrototype>> obj_proto;

ObjectPrototype &ObjectPrototype::operator=(const ObjectPrototype &other)
{
    // basic proto data fields
    ObjectBase::operator=(static_cast<const ObjectBase &>(other));

    return *this;
}

ObjectPrototype::ObjectPrototype(const Object &other)
{
    ObjectBase::operator=(static_cast<const ObjectBase &>(other));
}
