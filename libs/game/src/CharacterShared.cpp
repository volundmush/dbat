#include "dbat/game/CharacterShared.hpp"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const time_data& unit)
{
    if(unit.birth != 0) {
        j[+"birth"] = unit.birth;
    }
    if(unit.created != 0) {
        j[+"created"] = unit.created;
    }
    if(unit.maxage != 0) {
        j[+"maxage"] = unit.maxage;
    }
    if(unit.logon != 0) {
        j[+"logon"] = unit.logon;
    }
    if(unit.played != 0.0) {
        j[+"played"] = unit.played;
    }
    if(unit.seconds_aged != 0.0) {
        j[+"seconds_aged"] = unit.seconds_aged;
    }
}

void from_json(const nlohmann::json& j, time_data& unit)
{
    if(j.contains(+"birth")) {
        j.at(+"birth").get_to(unit.birth);
    }
    if(j.contains(+"created")) {
        j.at(+"created").get_to(unit.created);
    }
    if(j.contains(+"maxage")) {
        j.at(+"maxage").get_to(unit.maxage);
    }
    if(j.contains(+"logon")) {
        j.at(+"logon").get_to(unit.logon);
    }
    if(j.contains(+"played")) {
        j.at(+"played").get_to(unit.played);
    }
    if(j.contains(+"seconds_aged")) {
        j.at(+"seconds_aged").get_to(unit.seconds_aged);
    }
}

void to_json(nlohmann::json& j, const alias_data& unit)
{
    if(!unit.name.empty()) {
        j[+"name"] = unit.name;
    }
    if(!unit.replacement.empty()) {
        j[+"replacement"] = unit.replacement;
    }
    if(unit.type != 0) {
        j[+"type"] = unit.type;
    }
}

void from_json(const nlohmann::json& j, alias_data& unit)
{
    if(j.contains(+"name")) {
        j.at(+"name").get_to(unit.name);
    }
    if(j.contains(+"replacement")) {
        j.at(+"replacement").get_to(unit.replacement);
    }
    if(j.contains(+"type")) {
        j.at(+"type").get_to(unit.type);
    }
}

void to_json(nlohmann::json& j, const skill_data& unit)
{
    if(unit.level != 0) {
        j[+"level"] = unit.level;
    }
    if(unit.perfs != 0) {
        j[+"perfs"] = unit.perfs;
    }
}

void from_json(const nlohmann::json& j, skill_data& unit)
{
    if(j.contains(+"level")) {
        j.at(+"level").get_to(unit.level);
    }
    if(j.contains(+"perfs")) {
        j.at(+"perfs").get_to(unit.perfs);
    }
}

void to_json(nlohmann::json &j, const trans_data &t)
{
    if (t.time_spent_in_form != 0.0)
        j["time_spent_in_form"] = t.time_spent_in_form;
    j["visible"] = t.visible;
    j["limit_broken"] = t.limit_broken;
    j["unlocked"] = t.unlocked;
    j["grade"] = t.grade;
    if (!t.vars.empty())
        j["vars"] = t.vars;
    if (!t.description.empty())
        j["description"] = t.description;
    if (!t.appearances.empty())
        j["appearances"] = t.appearances;
}

void from_json(const nlohmann::json &j, trans_data &t)
{
    if (j.contains(+"time_spent_in_form"))
        t.time_spent_in_form = j["time_spent_in_form"];
    if (j.contains(+"visible"))
        t.visible = j["visible"];
    if (j.contains(+"limit_broken"))
        t.limit_broken = j["limit_broken"];
    if (j.contains(+"unlocked"))
        t.unlocked = j["unlocked"];
    if (j.contains(+"grade"))
        t.grade = j["grade"];
    if (j.contains(+"vars"))
        t.vars = j["vars"];
    if (j.contains(+"description"))
        t.description = j["description"].get<std::string>();
    if (j.contains(+"appearances"))
        t.appearances = j["appearances"];
}

void to_json(nlohmann::json &j, const CharacterBase &c) {
    to_json(j, static_cast<const HasVnum&>(c));
    to_json(j, static_cast<const HasMudStrings&>(c));
    to_json(j, static_cast<const HasExtraDescriptions&>(c));
    to_json(j, static_cast<const HasStats&>(c));
    j["race"] = c.race;
    j["sensei"] = c.sensei;
    if(c.model) j["model"] = c.model;
    j["sex"] = c.sex;
    j["size"] = c.size;
    j["position"] = c.position;
    if(c.character_flags) j["character_flags"] = c.character_flags;
    if(c.mob_flags) j["mob_flags"] = c.mob_flags;
    if(c.affect_flags) j["affect_flags"] = c.affect_flags;
    if(c.bio_genomes) j["bio_genomes"] = c.bio_genomes;
    if(c.mutations) j["mutations"] = c.mutations;
}

void from_json(const nlohmann::json &j, CharacterBase &c) {
    from_json(j, static_cast<HasVnum&>(c));
    from_json(j, static_cast<HasMudStrings&>(c));
    from_json(j, static_cast<HasExtraDescriptions&>(c));
    from_json(j, static_cast<HasStats&>(c));
    if (j.contains(+"race")) c.race = j["race"];
    if(j.contains(+"sensei")) c.sensei = j["sensei"];
    if (j.contains(+"model")) c.model = j["model"];
    if(j.contains(+"position")) j.at(+"position").get_to(c.position);
    if (j.contains(+"sex")) c.sex = j["sex"];
    if (j.contains(+"size")) c.size = j["size"];
    if (j.contains(+"character_flags")) j.at(+"character_flags").get_to(c.character_flags);
    if (j.contains(+"mob_flags")) j.at(+"mob_flags").get_to(c.mob_flags);
    if (j.contains(+"affect_flags")) j.at(+"affect_flags").get_to(c.affect_flags);
    if (j.contains(+"bio_genomes")) j.at(+"bio_genomes").get_to(c.bio_genomes);
    if (j.contains(+"mutations")) j.at(+"mutations").get_to(c.mutations);
}
