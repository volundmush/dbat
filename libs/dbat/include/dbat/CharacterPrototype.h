#pragma once
#include <map>
#include <memory>

#include "Typedefs.h"
#include "Command.h"
#include "ThingPrototype.h"
#include "CharacterShared.h"
#include "StatHandler.h"

struct CharacterPrototype;

extern StatHandler<CharacterPrototype> npcProtoStats;

struct CharacterPrototype : public CharacterBase, public ThingPrototype {


    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return npcProtoStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return npcProtoStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return npcProtoStats.modBase<R>(this, stat, val);
    }

    SpecialFunc func{nullptr};
};

extern std::map<mob_vnum, std::shared_ptr<CharacterPrototype>> mob_proto;

extern int vnum_mobile(char *searchname, Character *ch);

extern mob_rnum real_mobile(mob_vnum vnum);