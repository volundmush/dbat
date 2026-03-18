#pragma once
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "Typedefs.hpp"
#include "Command.hpp"
#include "HasMisc.hpp"

#include "CharacterShared.hpp"
#include "StatHandler.hpp"

struct CharacterPrototype;

extern StatHandler<CharacterPrototype> npcProtoStats;

struct CharacterPrototype : public CharacterBase, public HasProtoScript {

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

inline std::string format_as(const CharacterPrototype& mob) {
    return fmt::format("MobProto {} '{}'", mob.vn, mob.getName());
}

inline std::string format_as_diagnostic(const CharacterPrototype& mob) {
    std::vector<std::string> parts;
    parts.emplace_back(format_as(static_cast<const CharacterBase&>(mob)));
    parts.emplace_back(format_as(static_cast<const HasProtoScript&>(mob)));
    if(mob.func) parts.emplace_back("Has SpecialFunc");
    // TODO: Add in Stats handling...
    return fmt::format("Character Prototype:\r\n{}", fmt::join(parts, "\r\n"));
}

extern std::map<mob_vnum, std::shared_ptr<CharacterPrototype>> mob_proto;

extern int vnum_mobile(char *searchname, Character *ch);

extern mob_rnum real_mobile(mob_vnum vnum);

void to_json(nlohmann::json& j, const CharacterPrototype& c);
void from_json(const nlohmann::json& j, CharacterPrototype& c);
