#include "dbat/game/affect.hpp"
//#include "dbat/game/constants.hpp"
#include "dbat/game/spells.hpp"

#include "dbat/game/const/Position.hpp"
#include "dbat/game/const/CharacterProperties.hpp"

#include "dbat/util/Enum.hpp"
#include <nlohmann/json.hpp>

extern std::unordered_map<Position, std::vector<character_affect_type>> pos_affects;

void to_json(nlohmann::json& j, const affect_t& unit)
{
    if(unit.location != 0) {
        j[+"location"] = unit.location;
    }
    if(unit.modifier != 0.0) {
        j[+"modifier"] = unit.modifier;
    }
    if(unit.specific != 0) {
        j[+"specific"] = unit.specific;
    }
}

void from_json(const nlohmann::json& j, affect_t& unit)
{
    if(j.contains(+"location")) {
        j.at(+"location").get_to(unit.location);
    }
    if(j.contains(+"modifier")) {
        j.at(+"modifier").get_to(unit.modifier);
    }
    if(j.contains(+"specific")) {
        j.at(+"specific").get_to(unit.specific);
    }
}

std::string affect_t::locName()
{
    switch (location)
    {
    case APPLY_NONE:
        return "NONE";
    case APPLY_CATTR_BASE:
        return "ATTR BASE";
    case APPLY_CATTR_MULT:
        return "ATTR MULT";
    case APPLY_CATTR_GAIN_MULT:
        return "ATTR GAIN";
    case APPLY_CATTR_POST:
        return "ATTR POST";
    case APPLY_CVIT_BASE:
        return "VITALS BASE";
    case APPLY_CVIT_MULT:
        return "VITALS MULT";
    case APPLY_CVIT_POST:
        return "VITALS POST";
    case APPLY_CVIT_GAIN_MULT:
        return "VITALS GAIN";
    case APPLY_CVIT_REGEN_MULT:
        return "VITALS REGEN";
    case APPLY_CVIT_DOT_MULT:
        return "VITALS DOT";
    case APPLY_CSTAT_BASE:
        return "STAT BASE";
    case APPLY_CSTAT_MULT:
        return "STAT MULT";
    case APPLY_CSTAT_POST:
        return "STAT POST";
    case APPLY_CSTAT_GAIN_MULT:
        return "STAT GAIN";
    case APPLY_CDIM_BASE:
        return "DIMEN BASE";
    case APPLY_CDIM_MULT:
        return "DIMEN MULT";
    case APPLY_CDIM_POST:
        return "DIMEN POST";
    case APPLY_COMBAT_BASE:
        return "COMBAT BASE";
    case APPLY_COMBAT_MULT:
        return "COMBAT MULT";
    case APPLY_DTYPE_RES:
        return "DAMAGE RES";
    case APPLY_DTYPE_BON:
        return "DAMAGE BON";
    case APPLY_ATKTIER_RES:
        return "ATK TIER RES";
    case APPLY_ATKTIER_BON:
        return "ATK TIER BON";
    case APPLY_TRANS_UPKEEP_CVIT:
        return "TRANS VITALS UPKEEP";
    case APPLY_SKILL:
        return "SKILL";
    default:
        return "UNKNOWN";
    }
}

std::vector<std::string> affect_t::specificNames()
{
    std::vector<std::string> out;

    switch (location)
    {
    case APPLY_NONE:
        return out;
    case APPLY_CATTR_BASE:
    case APPLY_CATTR_MULT:
    case APPLY_CATTR_POST:
    case APPLY_CATTR_GAIN_MULT:
    {
        auto res = dbat::util::getEnumNameList<CharAttribute>([&](CharAttribute v)
                                                 { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_CVIT_BASE:
    case APPLY_CVIT_MULT:
    case APPLY_CVIT_POST:
    case APPLY_CVIT_GAIN_MULT:
    case APPLY_CVIT_DOT_MULT:
    case APPLY_CVIT_REGEN_MULT:
    case APPLY_TRANS_UPKEEP_CVIT:
    {
        auto res = dbat::util::getEnumNameList<CharVital>([&](CharVital v)
                                             { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_CSTAT_BASE:
    case APPLY_CSTAT_MULT:
    case APPLY_CSTAT_POST:
    case APPLY_CSTAT_GAIN_MULT:
    {
        auto res = dbat::util::getEnumNameList<CharStat>([&](CharStat v)
                                             { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_CDIM_BASE:
    case APPLY_CDIM_MULT:
    case APPLY_CDIM_POST:
    {
        auto res = dbat::util::getEnumNameList<CharDim>([&](CharDim v)
                                             { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_COMBAT_BASE:
    case APPLY_COMBAT_MULT:
    {
        auto res = dbat::util::getEnumNameList<ComStat>([&](ComStat v)
                                             { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_DTYPE_RES:
    case APPLY_DTYPE_BON:
    {
        auto res = dbat::util::getEnumNameList<DamType>([&](DamType v)
                                             { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_ATKTIER_BON:
    case APPLY_ATKTIER_RES:
    {
        auto res = dbat::util::getEnumNameList<AtkTier>([&](AtkTier v)
                                             { return specific & static_cast<int>(v); });
        out.insert(out.end(), res.begin(),  res.end());
    }
    break;
    case APPLY_SKILL:
        out.emplace_back(skill_name(specific));
        break;
    }

    return out;
}

bool affect_t::isBitwise()
{
    switch (location)
    {
    case APPLY_SKILL:
        return false;
    default:
        return true;
    }
}

bool affect_t::match(int loc, int spec)
{
    if (loc != location)
        return false;
    if (isBitwise())
        return spec & specific;
    return spec == specific;
}

bool affect_t::isPercent()
{
    switch (location)
    {
    case APPLY_CATTR_MULT:
    case APPLY_CATTR_GAIN_MULT:
    case APPLY_CVIT_MULT:
    case APPLY_CVIT_GAIN_MULT:
    case APPLY_CVIT_REGEN_MULT:
    case APPLY_CVIT_DOT_MULT:
    case APPLY_CSTAT_MULT:
    case APPLY_CSTAT_GAIN_MULT:
    case APPLY_CDIM_MULT:
    case APPLY_COMBAT_MULT:
    case APPLY_DTYPE_BON:
    case APPLY_DTYPE_RES:
    case APPLY_ATKTIER_RES:
    case APPLY_ATKTIER_BON:
    case APPLY_TRANS_UPKEEP_CVIT:
        return true;
    default:
        return false;
    }
}

std::unordered_map<Position, std::vector<character_affect_type>> pos_affects = {
    {POS_DEAD, {}},
    {POS_MORTALLYW, {}},
    {POS_INCAP, {{APPLY_CVIT_REGEN_MULT, -0.25, ~0}}},
    {POS_STUNNED, {{APPLY_CVIT_REGEN_MULT, -0.25, ~0}}},
    {POS_SLEEPING, {{APPLY_CVIT_REGEN_MULT, 1.00, ~0}}},
    {POS_RESTING, {{APPLY_CVIT_REGEN_MULT, 0.50, ~0}}},
    {POS_SITTING, {{APPLY_CVIT_REGEN_MULT, 0.25, ~0}}},
    {POS_FIGHTING, {{APPLY_CVIT_REGEN_MULT, -0.5, ~0}}},
    {POS_STANDING, {}}};
