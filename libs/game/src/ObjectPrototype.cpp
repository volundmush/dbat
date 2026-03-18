#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/Object.hpp"
#include <nlohmann/json.hpp>

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

void to_json(nlohmann::json &j, const ObjectBase &o) {
    to_json(j, static_cast<const HasVnum &>(o));
    to_json(j, static_cast<const HasMudStrings &>(o));
    to_json(j, static_cast<const HasExtraDescriptions &>(o));
    to_json(j, static_cast<const HasStats &>(o));
    to_json(j, static_cast<const picky_data &>(o));
    j["type_flag"] = o.type_flag;
    if (o.wear_flags)
        j["wear_flags"] = o.wear_flags;
    if (o.item_flags)
        j["item_flags"] = o.item_flags;
    j["size"] = o.size;
    if(o.affect_flags) j["affect_flags"] = o.affect_flags;
    j["affected"] = o.affected;
}

void from_json(const nlohmann::json& j, ObjectBase &o) {
    from_json(j, static_cast<HasVnum &>(o));
    from_json(j, static_cast<HasMudStrings &>(o));
    from_json(j, static_cast<HasExtraDescriptions &>(o));
    from_json(j, static_cast<HasStats &>(o));
    from_json(j, static_cast<picky_data &>(o));
    if (j.contains(+"type_flag")) o.type_flag = j["type_flag"];
    if (j.contains(+"wear_flags")) j.at(+"wear_flags").get_to(o.wear_flags);
    if (j.contains(+"item_flags")) j.at(+"item_flags").get_to(o.item_flags);
    if (j.contains(+"size")) j.at(+"size").get_to(o.size);
    if (j.contains(+"affect_flags")) j.at(+"affect_flags").get_to(o.affect_flags);
    if (j.contains(+"affected")) j.at(+"affected").get_to(o.affected);
}

void to_json(nlohmann::json &j, const ObjectPrototype &o)
{
    to_json(j, static_cast<const ObjectBase &>(o));
    to_json(j, static_cast<const HasProtoScript &>(o));
}

void from_json(const nlohmann::json &j, ObjectPrototype &o)
{
    from_json(j, static_cast<ObjectBase &>(o));
    from_json(j, static_cast<HasProtoScript &>(o));
    auto otype = o.type_flag;
    if ((otype == ITEM_PORTAL || otype == ITEM_HATCH) &&
        (!o.getBaseStat<int>(VAL_DOOR_DCLOCK) ||
         !o.getBaseStat<int>(VAL_DOOR_DCHIDE)))
    {
        for (const auto v : {VAL_DOOR_DCLOCK, VAL_DOOR_DCHIDE})
        {
            o.setBaseStat(v, 20);
        }
    }
    if (otype == ITEM_DRINKCON ||
        otype == ITEM_FOUNTAIN)
    {
        if (o.getBaseStat("weight") < o.getBaseStat(VAL_FOUNTAIN_HOWFULL))
            o.setBaseStat<weight_t>("weight", o.getBaseStat(VAL_FOUNTAIN_HOWFULL) + 5);
    }
    if (otype == ITEM_PORTAL)
    {
        o.setBaseStat<int>("timer", -1);
    }
}
