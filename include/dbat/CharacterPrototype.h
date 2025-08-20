#pragma once
#include "ThingPrototype.h"
#include "CharacterShared.h"

struct CharacterPrototype : public ThingPrototype {
    Race race{Race::human};
    std::optional<SubRace> subrace{};
    Sensei sensei{Sensei::commoner};
    Sex sex{Sex::male};
    struct mob_special_data mob_specials{};
    Size size{Size::undefined};
    FlagHandler<CharacterFlag> character_flags{};
    FlagHandler<MobFlag> mob_flags{};
    FlagHandler<Race> bio_genomes{};
    FlagHandler<Mutation> mutations{};
    char *clan{};
    int crank{}; // clan rank

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
};