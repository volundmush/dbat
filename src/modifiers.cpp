#include "dbat/structs.h"
extern std::unordered_map<int, std::vector<character_affect_type>> pos_affects;

std::vector<std::string> affect_t::getNames() {
    std::vector<std::string> out;

    return out;
}

bool affect_t::isBitwise() {
    switch(location) {
        case APPLY_SKILL:
            return false;
        default:
            return true;
    }
}

bool affect_t::match(int loc, int spec) {
    if(loc != location) return false;
    if(isBitwise())
        return spec | specific;
    return spec == specific;
}

bool affect_t::isPercent() {
    switch(location) {
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

nlohmann::json affect_t::serialize() {
    auto j = nlohmann::json();

    if(location) j["location"] = location;
    if(modifier != 0.0) j["modifier"] = modifier;
    if(specific) j["specific"] = specific;

    return j;
}

void affect_t::deserialize(const nlohmann::json &j) {
    if(j.contains("location")) location = j["location"];
    if(j.contains("modifier")) modifier = j["modifier"];
    if(j.contains("specific")) specific = j["specific"];
}

affect_t::affect_t(const nlohmann::json &j) {
    deserialize(j);
}

std::unordered_map<int, std::vector<character_affect_type>> pos_affects = {
        {POS_DEAD, {}},
        {POS_MORTALLYW, {}},
        {POS_INCAP, {
                            {   APPLY_CVIT_REGEN_MULT, -0.25, ~0}
        }},
        {POS_STUNNED, {
                              { APPLY_CVIT_REGEN_MULT, -0.25, ~0}
        }},
        {POS_SLEEPING, {
                               {APPLY_CVIT_REGEN_MULT, 1.00,  ~0}
        }},
        {POS_RESTING, {
                              { APPLY_CVIT_REGEN_MULT, 0.50,  ~0}
        }},
        {POS_SITTING, {
                              { APPLY_CVIT_REGEN_MULT, 0.25,  ~0}
        }},
        {POS_FIGHTING, {
                               {APPLY_CVIT_REGEN_MULT, -0.5,  ~0}
        }},
        {POS_STANDING, {}}
};