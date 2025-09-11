#pragma once
#include <string>
#include <array>

#include "HasMudStrings.h"
#include "HasExtraDescriptions.h"
#include "HasPicky.h"
#include "HasMisc.h"

#include "const/Max.h"
#include "const/WearFlag.h"
#include "const/ItemValues.h"
#include "const/ItemFlag.h"
#include "const/ItemType.h"
#include "const/Size.h"
#include "const/AffectFlag.h"

#include "Flags.h"
#include "affect.h"

struct Object;

template<typename T>
bool OBJWEAR_FLAGGED(T *obj, int flag) {
    return obj->wear_flags.get(static_cast<WearFlag>(flag));
}

template<typename T>
int64_t GET_OBJ_VAL(T* obj, const std::string& val) {
    return obj->getBaseStat(val);
}

template<typename T>
int64_t SET_OBJ_VAL(T* obj, const std::string& val, int newval) {
    return obj->setBaseStat(val, newval);
}

template<typename T>
int64_t MOD_OBJ_VAL(T* obj, const std::string& val, int mod) {
    return obj->modBaseStat(val, mod);
}

extern bool OBJAFF_FLAGGED(Object *obj, AffectFlag flag);
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), flag))

extern bool OBJ_FLAGGED(Object *obj, ItemFlag flag);

struct ObjectBase : public HasVnum, public HasMudStrings, public HasExtraDescriptions, public HasStats, public HasPicky {
    ItemType type_flag{ItemType::unknown};      /* Type of item                        */
    std::vector<affected_type> affected;  /* affects */
    FlagHandler<WearFlag> wear_flags{}; /* Where you can wear it     */
    FlagHandler<ItemFlag> item_flags{}; /* If it hums, glows, etc.  */
    FlagHandler<AffectFlag> affect_flags{};
    Size size{Size::medium};           /* Size class of object                */
   
};

inline std::string format_as(const ObjectBase& ob) {
    std::vector<std::string> parts;
    parts.emplace_back(format_as(static_cast<const HasVnum&>(ob)));
    parts.emplace_back(format_as(static_cast<const HasMudStrings&>(ob)));
    parts.emplace_back(format_as(static_cast<const HasExtraDescriptions&>(ob)));
    parts.emplace_back(fmt::format("Type: {}", magic_enum::enum_name(ob.type_flag)));
    parts.emplace_back(fmt::format("Size: {}", magic_enum::enum_name(ob.size)));
    if(ob.wear_flags) parts.emplace_back(fmt::format("Wear Flags: {}", format_as(ob.wear_flags)));
    if(ob.item_flags) parts.emplace_back(fmt::format("Item Flags: {}", format_as(ob.item_flags)));
    if(ob.affect_flags) parts.emplace_back(fmt::format("Affect Flags: {}", format_as(ob.affect_flags)));
    parts.emplace_back(fmt::format("Affects: {}", ob.affected.size()));
    parts.emplace_back(format_as(static_cast<const HasPicky&>(ob)));

    return fmt::format("Base Object Data:\r\n{}", fmt::join(parts, "\r\n"));
}