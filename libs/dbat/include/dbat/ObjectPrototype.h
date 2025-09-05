#pragma once
#include <map>
#include <memory>
#include "Typedefs.h"
#include "Command.h"
#include "ObjectShared.h"
#include "ThingPrototype.h"
#include "HasPicky.h"
#include "affect.h"

#include "StatHandler.h"

struct ObjectPrototype;

extern StatHandler<ObjectPrototype> itemProtoStats;

struct ObjectPrototype : public ObjectBase, public ThingPrototype, public picky_data {
    ObjectPrototype() = default;
    ObjectPrototype(const Object& other);
    
    ObjectPrototype& operator=(const ObjectPrototype& other);

    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return itemProtoStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return itemProtoStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return itemProtoStats.modBase<R>(this, stat, val);
    }

    SpecialFunc func{nullptr};
};

inline std::string format_as(const ObjectPrototype& z) {
    return fmt::format("ObjectPrototype {} '{}'", z.vn, z.short_description ? z.short_description : "<unnamed>");
}

extern std::map<obj_vnum, std::shared_ptr<ObjectPrototype>> obj_proto;

extern int vnum_object(char *searchname, Character *ch);
extern int vnum_material(char *searchname, Character *ch);
extern int vnum_weapontype(char *searchname, Character *ch);
extern int vnum_armortype(char *searchname, Character *ch);

extern obj_rnum real_object(obj_vnum vnum);