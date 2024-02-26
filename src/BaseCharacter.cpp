#include "dbat/structs.h"
#include "dbat/races.h"
#include "dbat/utils.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"
#include "dbat/config.h"
#include "dbat/mail.h"
#include "dbat/dg_comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/interpreter.h"
#include "dbat/players.h"
#include "dbat/transformation.h"
#include "dbat/weather.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/constants.h"
#include "dbat/guild.h"
#include "dbat/entity.h"

static std::string robot = "Robotic-Humanoid", robot_lower = "robotic-humanoid", unknown = "UNKNOWN";


std::string Character::juggleRaceName(bool capitalized) {

    auto apparent = race;

    switch (apparent) {
        case RaceID::Hoshijin:
            if (mimic) apparent = *mimic;
            break;
        case RaceID::Halfbreed:
            switch (RACIAL_PREF(this)) {
                case 1:
                    apparent = RaceID::Human;
                    break;
                case 2:
                    apparent = RaceID::Saiyan;
                    break;
            }
            break;
        case RaceID::Android:
            switch (RACIAL_PREF(this)) {
                case 1:
                    apparent = RaceID::Android;
                    break;
                case 2:
                    apparent = RaceID::Human;
                    break;
                case 3:
                    if (capitalized) {
                        return robot;
                    } else {
                        return robot_lower;
                    }
            }
            break;
        case RaceID::Saiyan:
            if (PLR_FLAGGED(this, PLR_TAILHIDE)) {
                apparent = RaceID::Human;
            }
            break;
    }

    if (capitalized) {
        return race::getName(apparent);
    } else {
        auto out = race::getName(apparent);
        to_lower(out);
        return out;
    }
}

int Character::getArmor() {
    int out = get(CharNum::ArmorWishes) * 5000;
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
            out += obj->getAffectModifier(APPLY_AC, -1);
    }
    return out;
}

int64_t Character::getExperience() {
    return exp;
}

int64_t Character::setExperience(int64_t value) {
    exp = value;
    if(exp < 0) exp = 0;
    return exp;
}

// This returns the exact amount that was modified by.
int64_t Character::modExperience(int64_t value, bool applyBonuses) {

    if(value < 0) {
        // removing experience. We can do this easily.
        auto cur = getExperience();
        auto new_value = setExperience(cur + value);
        // return the actual amount substracted as a negative.
        return cur - new_value;
    }

    // Adding experience may involve bonuses.
    auto gain = value;
    auto cur = getExperience();

    if(!applyBonuses) {
        gain *= (1.0 + getAffectModifier(APPLY_EXP_GAIN_MULT));

        if (AFF_FLAGGED(this, AFF_WUNJO)) {
            gain *= 1.15;
        }
        if (PLR_FLAGGED(this, PLR_IMMORTAL)) {
            gain *= 0.95;
        }

        int64_t diff = gain * 0.15;

        if (gain > 0) {

            // TODO: Modify the spar booster with APPLY_EXP_GAIN_MULT 0.25
            if (auto obj = GET_EQ(this, WEAR_SH); obj && obj->getVN() == 1127) {
                int64_t spar = gain;
                gain += gain * 0.25;
                spar = gain - spar;
                this->sendf("@D[@BBooster EXP@W: @G+%s@D]\r\n", add_commas(spar).c_str());
            }

            // Post-100 gains.
            if (GET_LEVEL(this) == 100 && GET_ADMLEVEL(this) < 1) {
                if (IS_KANASSAN(this) || IS_DEMON(this)) {
                    diff = diff * 1.3;
                }
                if (IS_ANDROID(this)) {
                    diff = diff * 1.2;
                }

                if (rand_number(1, 5) >= 2) {
                    if (IS_HUMAN(this)) {
                        this->gainBasePL(diff * 0.8);
                    } else {
                        this->gainBasePL(diff);
                    }
                    this->sendf("@D[@G+@Y%s @RPL@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2) {
                    if (IS_HALFBREED(this)) {
                        this->gainBaseST(diff * 0.85);
                    } else {
                        this->gainBaseST(diff);
                    }
                    this->sendf("@D[@G+@Y%s @gSTA@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2) {
                    this->gainBaseKI(diff);
                    this->sendf("@D[@G+@Y%s @CKi@D]@n", add_commas(diff).c_str());
                }
            }
        }
    }

    // Amount gained cannot be negative.
    gain = std::max<int64_t>(gain, 0);

    if (MINDLINK(this) && gain > 0 && LINKER(this) == 0) {
        if (GET_LEVEL(this) + 20 < GET_LEVEL(MINDLINK(this)) || GET_LEVEL(this) - 20 > GET_LEVEL(MINDLINK(this))) {
            MINDLINK(this)->sendf("The level difference between the two of you is too great to gain from mind read.\r\n");
        } else {
            act("@GYou've absorbed some new experiences from @W$n@G!@n", false, this, nullptr, MINDLINK(this),
                TO_VICT);
            int64_t read = gain * 0.12;
            gain -= read;
            if (read == 0)
                read = 1;
            MINDLINK(this)->modExperience(read, false);
            act("@RYou sense that @W$N@R has stolen some of your experiences with $S mind!@n", false, this,
                nullptr, MINDLINK(this), TO_CHAR);
        }
    }

    if(GET_LEVEL(this) < 100) {
        int64_t tnl = level_exp(this, GET_LEVEL(this) + 1);

        if(cur < tnl && (cur + gain) >= tnl) {
            this->sendf("@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");
        }

        int64_t max_over_tnl = tnl * 5;
        if((cur + gain) >= max_over_tnl) {
            gain = max_over_tnl - getExperience();
            this->sendf("@WYou -@RNEED@W- to @ylevel@W. You can't hold any more experience!@n\r\n");
        }

    }

    if(gain) setExperience(cur + gain);
    return gain;

}

void Character::gazeAtMoon() {
    if(OOZARU_RACE(this) && checkFlag(FlagType::PC, PLR_TAIL)) {
        if(form == FormID::Oozaru || form == FormID::GoldenOozaru) return;
        FormID toForm = FormID::Oozaru;
        if(transforms.contains(FormID::SuperSaiyan)
        || transforms.contains(FormID::SuperSaiyan2)
        || transforms.contains(FormID::SuperSaiyan3)
        || transforms.contains(FormID::SuperSaiyan4))
            toForm = FormID::GoldenOozaru;

        trans::handleEchoTransform(this, toForm);
        form = toForm;
    }
}

static const std::map<std::string, CharAttribute> _attr_names = {
    {"str", CharAttribute::Strength},
    {"wis", CharAttribute::Wisdom},
    {"con", CharAttribute::Constitution},
    {"cha", CharAttribute::Speed},
    {"spd", CharAttribute::Speed},
    {"dex", CharAttribute::Agility},
    {"agi", CharAttribute::Agility},
    {"int", CharAttribute::Intelligence}
};

static const std::map<std::string, CharMoney> _money_names = {
    {"bank", CharMoney::Bank},
    {"gold", CharMoney::Carried},
    {"zenni", CharMoney::Carried}
};

static const std::map<std::string, int> _cond_names = {
    {"hunger", HUNGER},
    {"thirst", THIRST},
    {"drunk", DRUNK}
};

static const std::map<std::string, int> _save_names = {
    {"saving_fortitude", SAVING_FORTITUDE},
    {"saving_reflex", SAVING_REFLEX},
    {"saving_will", SAVING_WILL}
};

static const std::map<std::string, int> _pflags = {
    {"is_killer", PLR_KILLER},
    {"is_thief", PLR_THIEF}
};

static const std::map<std::string, int> _aflags = {
    {"dead", AFF_SPIRIT},
    {"flying", AFF_FLYING}
};

static const std::set<std::string> _senseiCheck = {"sensei", "class"};


DgResults Character::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);

    if(auto attr = _attr_names.find(lmember); attr != _attr_names.end()) {
        if (!arg.empty()) {
            attribute_t addition = atof(arg.c_str());
            mod(attr->second, addition);
        }
        return fmt::format("{}", get(attr->second));
    }

    if(auto mon = _money_names.find(lmember); mon != _money_names.end()) {
        if (!arg.empty()) {
            money_t addition = atoll(arg.c_str());
            mod(mon->second, addition);
        }
        return fmt::format("{}", get(mon->second));
    }

    if(auto con = _cond_names.find(lmember); con != _cond_names.end()) {
        if (!arg.empty()) {
            int addition = atof(arg.c_str());
            GET_COND(this, con->second) = std::clamp<int>(addition, -1, 24);
        }
        return fmt::format("{}", GET_COND(this, con->second));
    }

    if(auto save = _save_names.find(lmember); save != _save_names.end()) {
        if (!arg.empty()) {
            int addition = atof(arg.c_str());
        }
        return "";
    }

    if(auto pf = _pflags.find(lmember); pf != _pflags.end()) {
        if (!arg.empty()) {
            if (!strcasecmp("on", arg.c_str()))
                setFlag(FlagType::PC, pf->second);
            else if (!strcasecmp("off", arg.c_str()))
                clearFlag(FlagType::PC, pf->second);
        }
        return checkFlag(FlagType::PC, pf->second) ? "1" : "0";
    }

    if(auto af = _aflags.find(lmember); af != _aflags.end()) {
        return AFF_FLAGGED(this, af->second) ? "1" : "0";
    }

    if(lmember == "aaaaa") {
        // Is this even used?
        return "0";
    }

    if(lmember == "affect") {
        if(arg.empty()) return "0";
        int affect = get_flag_by_name(affected_bits, (char*)arg.c_str());
        return (affect != NOFLAG && AFF_FLAGGED(this, affect)) ? "1" : "0";
    }

    if(lmember == "alias") {
        return GET_PC_NAME(this);
    }

    if(lmember == "align") {
        if (!arg.empty()) {
            int addition = atof(arg.c_str());
            set(CharAlign::GoodEvil, std::clamp<int>(addition, -1000, 1000));
        }
        return fmt::format("{}", GET_ALIGNMENT(this));
    }

    if(lmember == "canbeseen") {
        if(trig->parent->attach_type != 2) return "0";
        auto owner = (Character*)trig->sc->owner;
        return CAN_SEE(owner, this) ? "1" : "0";
    }

    if(lmember == "carry") {
        return CARRYING(this) ? "1" : "0";
    }

    if(lmember == "clan") {
        return clan && strstr(clan, arg.c_str()) ? "1": "0";
    }

    if(_senseiCheck.contains(lmember)) {
        return sensei::getName(chclass);
    }

    if(lmember == "death") {
        return fmt::format("{}", GET_DTIME(this));
    }

    if(lmember == "drag") {
        return DRAGGING(this) ? "1" : "0";
    }

    if(lmember == "eq") {
        if(arg.empty()) return "";
        else if(arg == "*") {
            if(auto eq = getEquipment(); !eq.empty()) return "1";
            return "0";
        }
        else {
            auto pos = find_eq_pos_script((char*)arg.c_str());
            if(pos == -1) return "";
            auto eq = getEquipment();
            if(eq[pos]) return eq[pos];
            return "";
        }
    }

    if(lmember == "exp") {
        if (!arg.empty()) {
            int64_t addition = std::max<int64_t>(0, atof(arg.c_str()));
            modExperience(addition);
        }
        return fmt::format("{}", GET_EXP(this));
    }

    if(lmember == "fighting") {
        if(fighting) return fighting;
        return "";
    }

    if(lmember == "followers") {
        if(followers && followers->follower) return followers->follower;
        return "";
    }

    if(lmember == "has_item") {
        if(arg.empty()) return "";
        return fmt::format("{}", char_has_item((char*)arg.c_str(), this));
    }

    if(lmember == "hisher") return HSHR(this);
    if(lmember == "heshe") return HSSH(this);
    if(lmember == "himher") return HMHR(this);

    if(lmember == "hitp") {
        if (!arg.empty()) {
            int64_t addition = atof(arg.c_str());
            if (addition > 0) {
                incCurHealth(addition);
            } else {
                decCurHealth(addition);
            }
            update_pos(this);
        }
        return fmt::format("{}", GET_HIT(this));
    }

    if(lmember == "id") return this;

    if(lmember == "is_pc") return IS_NPC(this) ? "1":"0";

    if(lmember == "inventory") {
        if(arg.empty()) {
            if(auto con = getInventory(); !con.empty()) return con.front();
            return "";
        }
        obj_vnum v = atoll(arg.c_str());
        if(auto found = findObjectVnum(v); found) return found;
        return "";
    }

    if(lmember == "level") return fmt::format("{}", GET_LEVEL(this));

    if(lmember == "maxhitp") {
        if(!arg.empty()) {
            int64_t addition = atof(arg.c_str());
        }
        return fmt::format("{}", GET_MAX_HIT(this));
    }

    if(lmember == "mana") {
        if (!arg.empty()) {
            int64_t addition = atof(arg.c_str());
            if (addition > 0) {
                incCurKI(addition);
            } else {
                decCurKI(addition);
            }
        }
        return fmt::format("{}", getCurKI());
    }

    if(lmember == "maxmana") {
        if(!arg.empty()) {
            int64_t addition = atof(arg.c_str());
        }
        return fmt::format("{}", GET_MAX_MANA(this));
    }

    if(lmember == "move") {
        if (!arg.empty()) {
            int64_t addition = atof(arg.c_str());
            if (addition > 0) {
                incCurST(addition);
            } else {
                decCurST(addition);
            }
        }
        return fmt::format("{}", getCurST());
    }

    if(lmember == "maxmove") {
        if(!arg.empty()) {
            int64_t addition = atof(arg.c_str());
        }
        return fmt::format("{}", GET_MAX_MOVE(this));
    }

    if(lmember == "master") {
        if(master) return master;
        return "";
    }

    if(lmember == "name") return GET_NAME(this);

    if(lmember == "next_in_room") {
        auto loc = getRoom();
        if(!loc) return "";
        auto people = loc->getPeople();
        auto found = std::find(people.begin(), people.end(), this);
        if(found != people.end() && ++found != people.end()) return *found;
        return "";
    }

    if(lmember == "pos") {
        if(!arg.empty()) {
            // This is stupid. It's the dumbest way of doing it. But I guess it works.
            for (auto i = POS_SLEEPING; i <= POS_STANDING; i++) {
                /* allows : Sleeping, Resting, Sitting, Fighting, Standing */
                if (iequals(arg, position_types[i])) {
                    position = i;
                    break;
                }
            }
        }
        return position_types[position];
    }

    if(lmember == "prac") {
        if(!arg.empty()) {
            int addition = atof(arg.c_str());
            modPractices(addition);
        }
        return fmt::format("{}", GET_PRACTICES(this));
    }

    if(lmember == "plr") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(player_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::PC, flag) ? "1" : "0";
    }

    if(lmember == "pref") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(preference_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::Pref, flag) ? "1" : "0";
    }

    if(lmember == "room") {
        if(auto r = getRoom(); r) return r;
        return "";
    }

    if(lmember == "race") return race::getName(race);

    if(lmember == "rpp") {
        if(!arg.empty()) {
            int addition = atof(arg.c_str());
            modRPP(addition);
        }
        return fmt::format("{}", getRPP());
    }

    if(lmember == "sex") return genders[(int)GET_SEX(this)];

    if(lmember == "size") {
        if(!arg.empty()) {
            auto ns = search_block((char*)arg.c_str(), size_names, false);
            if(ns > -1) setSize(ns);
        }
        return size_names[getSize()];
    }

    if(lmember == "skill") return skill_percent(this, (char*)arg.c_str());

    if(lmember == "skillset") {
        if(!arg.empty()) {
            char skillname[MAX_INPUT_LENGTH], *amount;
            amount = one_word((char*)arg.c_str(), skillname);
            skip_spaces(&amount);
            if (amount && *amount && is_number(amount)) {
                int skillnum = find_skill_num(skillname, SKTYPE_SKILL);
                if (skillnum > 0) {
                    int new_value = std::clamp<double>(atof(amount), 0, 100);
                    SET_SKILL(this, skillnum, new_value);
                }
            }
        }
        return "";
    }

    if(lmember == "tnl") return fmt::format("{}", level_exp(this, GET_LEVEL(this)+1));

    if(lmember == "vnum") {
        if(!arg.empty()) {
            auto v = atoll(arg.c_str());
            return vn == v ? "1":"0";
        }
        return fmt::format("{}", vn);
    }

    if(lmember == "varexists") return script->hasVar(arg) ? "1" : "0";

    // nothing left to do but try global variables...
    if(script->hasVar(lmember)) {
        return script->getVar(lmember);
    } else {
        script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), lmember.c_str());
    }

    return "";
}

int64_t Character::getCurHealth() {
    return getCurPL();
}

int64_t Character::getMaxHealth() {
    return getMaxPL();
}

double Character::getCurHealthPercent() {
    return getCurPLPercent();
}

int64_t Character::getPercentOfCurHealth(double amt) {
    return getPercentOfCurPL(amt);
}

int64_t Character::getPercentOfMaxHealth(double amt) {
    return getPercentOfMaxPL(amt);
}

bool Character::isFullHealth() {
    return isFullPL();
}

int64_t Character::setCurHealth(int64_t amt) {
    return 0;
}

int64_t Character::setCurHealthPercent(double amt) {
    return 0;
}

int64_t Character::incCurHealth(int64_t amt, bool limit_max) {
    if (limit_max)
        health = std::min(1.0, health + (double) std::abs(amt) / (double) getEffMaxPL());
    else
        health += (double) std::abs(amt) / (double) getEffMaxPL();
    return getCurHealth();
};

int64_t Character::decCurHealth(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getEffMaxPL();
    if (suppression > 0)
        health = std::max(fl, health - (double) std::abs(amt) / ((double) getEffMaxPL() * ((double) suppression / 100.0)));
    else
        health = std::max(fl, health - (double) std::abs(amt) / (double) getEffMaxPL());
    return getCurHealth();
}

int64_t Character::incCurHealthPercent(double amt, bool limit_max) {
    if (limit_max)
        health = std::min(1.0, health + std::abs(amt));
    else
        health += std::abs(amt);
    return getCurHealth();
}

int64_t Character::decCurHealthPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getEffMaxPL();
    health = std::max(fl, health - std::abs(amt));
    return getCurHealth();
}

void Character::restoreHealth(bool announce) {
    if (!isFullHealth()) health = 1;
}

int64_t Character::getMaxPLTrans() {
    auto total = getEffBasePL();

    total += (getAffectModifier(APPLY_ALL_VITALS) + getAffectModifier(APPLY_HIT));
    total *= (1.0 + getAffectModifier(APPLY_VITALS_MULT) + getAffectModifier(APPLY_PL_MULT));
    return total;
}

int64_t Character::getMaxPL() {
    auto total = getMaxPLTrans();
    if (GET_KAIOKEN(this) > 0) {
        total += (total / 10) * GET_KAIOKEN(this);
    }
    if (AFF_FLAGGED(this, AFF_METAMORPH)) {
        total += (total * .6);
    }
    return total;
}

int64_t Character::getCurPL() {
    if (!IS_NPC(this) && suppression > 0) {
        return getEffMaxPL() * std::min(health, health * ((double) suppression / 100));
    } else {
        return getEffMaxPL() * health;
    }
}

int64_t Character::getUnsuppressedPL() {
        return getEffMaxPL() * health;
}

int64_t Character::getEffBasePL() {
    if (original) return original->getEffBasePL();

    if (!clones.empty()) {
        return getBasePL() / (clones.size() + 1);
    } else {
        return getBasePL();
    }
}

int64_t Character::getBasePL() {
    return get(CharStat::PowerLevel);
}

double Character::getCurPLPercent() {
    return (double) getCurPL() / (double) getMaxPL();
}

int64_t Character::getPercentOfCurPL(double amt) {
    return getCurPL() * std::abs(amt);
}

int64_t Character::getPercentOfMaxPL(double amt) {
    return getMaxPL() * std::abs(amt);
}

bool Character::isFullPL() {
    return health >= 1.0;
}

int64_t Character::getCurKI() {
    return getMaxKI() * energy;
}

int64_t Character::getMaxKI() {
    auto total = getEffBaseKI();
    total += (getAffectModifier(APPLY_ALL_VITALS) + getAffectModifier(APPLY_MANA));
    total *= (1.0 + getAffectModifier(APPLY_VITALS_MULT) + getAffectModifier(APPLY_KI_MULT));
    return total;
}

int64_t Character::getEffBaseKI() {
    if (original) return original->getEffBaseKI();
    if (!clones.empty()) {
        return getBaseKI() / (clones.size() + 1);
    } else {
        return getBaseKI();
    }
}

int64_t Character::getBaseKI() {
    return get(CharStat::Ki);
}

double Character::getCurKIPercent() {
    return (double) getCurKI() / (double) getMaxKI();
}

int64_t Character::getPercentOfCurKI(double amt) {
    return getCurKI() * std::abs(amt);
}

int64_t Character::getPercentOfMaxKI(double amt) {
    return getMaxKI() * std::abs(amt);
}

bool Character::isFullKI() {
    return energy >= 1.0;
}

int64_t Character::setCurKI(int64_t amt) {
    return 0;
}

int64_t Character::setCurKIPercent(double amt) {
    return 0;
}

int64_t Character::incCurKI(int64_t amt, bool limit_max) {
    if (limit_max)
        energy = std::min(1.0, energy + (double) std::abs(amt) / (double) getMaxKI());
    else
        energy += (double) std::abs(amt) / (double) getMaxKI();
    return getCurKI();
};

int64_t Character::decCurKI(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    energy = std::max(fl, energy - (double) std::abs(amt) / (double) getMaxKI());
    return getCurKI();
}

int64_t Character::incCurKIPercent(double amt, bool limit_max) {
    if (limit_max)
        energy = std::min(1.0, energy + std::abs(amt));
    else
        energy += std::abs(amt);
    return getCurKI();
}

int64_t Character::decCurKIPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    energy = std::max(fl, energy - std::abs(amt));
    return getCurKI();
}


void Character::restoreKI(bool announce) {
    if (!isFullKI()) energy = 1;
}

int64_t Character::getCurST() {
    return getMaxST() * stamina;
}

int64_t Character::getMaxST() {
    auto total = getEffBaseST();
    total += (getAffectModifier(APPLY_ALL_VITALS) + getAffectModifier(APPLY_MOVE));
    total *= (1.0 + getAffectModifier(APPLY_VITALS_MULT) + getAffectModifier(APPLY_ST_MULT));
    return total;
}

int64_t Character::getEffBaseST() {
    if (original) return original->getEffBaseST();
    if (!clones.empty()) {
        return getBaseST() / (clones.size() + 1);
    } else {
        return getBaseST();
    }
}

int64_t Character::getBaseST() {
    return get(CharStat::Stamina);
}

double Character::getCurSTPercent() {
    return (double) getCurST() / (double) getMaxST();
}

int64_t Character::getPercentOfCurST(double amt) {
    return getCurST() * std::abs(amt);
}

int64_t Character::getPercentOfMaxST(double amt) {
    return getMaxST() * std::abs(amt);
}

bool Character::isFullST() {
    return stamina >= 1;
}

int64_t Character::setCurST(int64_t amt) {
    return 0;
}

int64_t Character::setCurSTPercent(double amt) {
    return 0;
}

int64_t Character::incCurST(int64_t amt, bool limit_max) {
    if (limit_max)
        stamina = std::min(1.0, stamina + (double) std::abs(amt) / (double) getMaxST());
    else
        stamina += (double) std::abs(amt) / (double) getMaxST();
    return getCurST();
};

int64_t Character::decCurST(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    stamina = std::max(fl, stamina - (double) std::abs(amt) / (double) getMaxST());
    return getCurST();
}

int64_t Character::incCurSTPercent(double amt, bool limit_max) {
    if (limit_max)
        stamina = std::min(1.0, stamina + std::abs(amt));
    else
        stamina += std::abs(amt);
    return getMaxST();
}

int64_t Character::decCurSTPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    stamina = std::max(fl, stamina - std::abs(amt));
    return getCurST();
}


void Character::restoreST(bool announce) {
    if (!isFullST()) stamina = 1;
}


int64_t Character::getCurLF() {
    return getMaxLF() * life;
}

int64_t Character::getMaxLF() {
    auto lb = GET_LIFEBONUSES(this);

    return (IS_DEMON(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.75) + lb
                           : (IS_KONATSU(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.85) +
                    lb : (GET_MAX_MANA(this) * 0.5) +
                                                                         (GET_MAX_MOVE(this) * 0.5) +
                    lb));
}

double Character::getCurLFPercent() {
    return life;
}

int64_t Character::getPercentOfCurLF(double amt) {
    return getCurLF() * std::abs(amt);
}

int64_t Character::getPercentOfMaxLF(double amt) {
    return getMaxLF() * std::abs(amt);
}

bool Character::isFullLF() {
    return life >= 1.0;
}

int64_t Character::setCurLF(int64_t amt) {
    life = std::max<int64_t>(0L, std::abs(amt));
    return getCurLF();
}

int64_t Character::setCurLFPercent(double amt) {
    life = std::max<int64_t>(0L, (int64_t) (getMaxLF() * std::abs(amt)));
    return getCurLF();
}

int64_t Character::incCurLF(int64_t amt, bool limit_max) {
    if (limit_max)
        life = std::min(1.0, stamina + (double) std::abs(amt) / (double) getMaxLF());
    else
        life += (double) std::abs(amt) / (double) getMaxLF();
    return getCurLF();
};

int64_t Character::decCurLF(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    life = std::max(fl, life - (double) std::abs(amt) / (double) getMaxLF());
    return getCurLF();
}

int64_t Character::incCurLFPercent(double amt, bool limit_max) {
    if (limit_max)
        life = std::min(1.0, life + std::abs(amt));
    else
        life += std::abs(amt);
    return getCurLF();
}

int64_t Character::decCurLFPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    life = std::max(fl, life - std::abs(amt));
    return getCurLF();
}


void Character::restoreLF(bool announce) {
    if (!isFullLF()) life = 1;
}


bool Character::isFullVitals() {
    return isFullHealth() && isFullKI() && isFullST();
}

void Character::restoreVitals(bool announce) {
    restoreHealth(announce);
    restoreKI(announce);
    restoreST(announce);
}

void Character::restoreStatus(bool announce) {
    cureStatusKnockedOut(announce);
    cureStatusBurn(announce);
    cureStatusPoison(announce);
}

void Character::setStatusKnockedOut() {
    setFlag(FlagType::Affect, AFF_KNOCKED);
    clearFlag(FlagType::Affect,AFF_FLYING);
    altitude = 0;
    GET_POS(this) = POS_SLEEPING;
}

void Character::cureStatusKnockedOut(bool announce) {
    if (AFF_FLAGGED(this, AFF_KNOCKED)) {
        if (announce) {
            ::act("@W$n@W is no longer senseless, and wakes up.@n", false, this, nullptr, nullptr, TO_ROOM);
            this->sendf("You are no longer knocked out, and wake up!@n\r\n");
        }

        if (CARRIED_BY(this)) {
            if (GET_ALIGNMENT(CARRIED_BY(this)) > 50) {
                carry_drop(CARRIED_BY(this), 0);
            } else {
                carry_drop(CARRIED_BY(this), 1);
            }
        }

        clearFlag(FlagType::Affect,AFF_KNOCKED);
        GET_POS(this) = POS_SITTING;
    }
}

void Character::cureStatusBurn(bool announce) {
    if (AFF_FLAGGED(this, AFF_BURNED)) {
        if (announce) {
            this->sendf("Your burns are healed now.\r\n");
            ::act("$n@w's burns are now healed.@n", true, this, nullptr, nullptr, TO_ROOM);
        }
        clearFlag(FlagType::Affect,AFF_BURNED);
    }
}

void Character::cureStatusPoison(bool announce) {
    ::act("@C$n@W suddenly looks a lot better!@b", false, this, nullptr, nullptr, TO_NOTVICT);
    affect_from_char(this, SPELL_POISON);
}

static std::map<int, std::string> limb_names = {
        {0, "right arm"},
        {1, "left arm"},
        {2, "right leg"},
        {3, "left leg"}
};

void Character::restoreLimbs(bool announce) {
    // restore head...
    GET_LIMBCOND(this, 0) = 100;

    // limbs...
    for (const auto &l: limb_names) {
        if (announce) {
            if (GET_LIMBCOND(this, l.first) <= 0)
                this->sendf("Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(this, l.first) < 50)
                this->sendf("Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(this, l.first) = 100;
    }

    // and lastly, tail.
    this->gainTail(announce);
}


void Character::gainTail(bool announce) {
    if (!race::hasTail(race)) return;
    if(checkFlag(FlagType::PC, PLR_TAIL)) return;
    setFlag(FlagType::PC, PLR_TAIL);
    if(announce) {
        this->sendf("@wYour tail grows back.@n\r\n");
        act("$n@w's tail grows back.@n", true, this, nullptr, nullptr, TO_ROOM);
    }
}

void Character::loseTail() {
    if (!checkFlag(FlagType::PC, PLR_TAIL)) return;
    clearFlag(FlagType::PC, PLR_TAIL);
    remove_limb(this, 6);
    GET_TGROWTH(this) = 0;
    oozaru_revert(this);
}

bool Character::hasTail() {
    return checkFlag(FlagType::PC, PLR_TAIL);
}

void Character::addTransform(FormID form) {
    transforms.insert({form, trans_data()});
}

void Character::hideTransform(FormID form, bool hide) {
    auto foundForm = transforms.find(form);
    foundForm->second.visible = !hide;
}

bool Character::removeTransform(FormID form) {
    if (transforms.contains(form))
    {
        transforms.erase(form);
        return true;
    }
    //Return if failure
    return false;
}

int64_t Character::gainBasePL(int64_t amt, bool trans_mult) {
    return mod(CharStat::PowerLevel, amt);
}

int64_t Character::gainBaseST(int64_t amt, bool trans_mult) {
    return mod(CharStat::Stamina, amt);
}

int64_t Character::gainBaseKI(int64_t amt, bool trans_mult) {
    return mod(CharStat::Ki, amt);
}

void Character::gainBaseAll(int64_t amt, bool trans_mult) {
    gainBasePL(amt, trans_mult);
    gainBaseKI(amt, trans_mult);
    gainBaseST(amt, trans_mult);
}

int64_t Character::loseBasePL(int64_t amt, bool trans_mult) {
    return mod(CharStat::PowerLevel, -amt);
}

int64_t Character::loseBaseST(int64_t amt, bool trans_mult) {
    return mod(CharStat::Stamina, -amt);
}

int64_t Character::loseBaseKI(int64_t amt, bool trans_mult) {
    return mod(CharStat::Ki, -amt);
}

void Character::loseBaseAll(int64_t amt, bool trans_mult) {
    loseBasePL(amt, trans_mult);
    loseBaseKI(amt, trans_mult);
    loseBaseST(amt, trans_mult);
}

int64_t Character::gainBasePLPercent(double amt, bool trans_mult) {
    return gainBasePL(get(CharStat::PowerLevel) * amt, trans_mult);
}

int64_t Character::gainBaseKIPercent(double amt, bool trans_mult) {
    return gainBaseKI(get(CharStat::Ki) * amt, trans_mult);
}

int64_t Character::gainBaseSTPercent(double amt, bool trans_mult) {
    return gainBaseST(get(CharStat::Stamina) * amt, trans_mult);
}

int64_t Character::loseBasePLPercent(double amt, bool trans_mult) {
    return loseBasePL(get(CharStat::PowerLevel) * amt, trans_mult);
}

int64_t Character::loseBaseKIPercent(double amt, bool trans_mult) {
    return loseBaseKI(get(CharStat::Ki) * amt, trans_mult);
}

int64_t Character::loseBaseSTPercent(double amt, bool trans_mult) {
    return loseBaseST(get(CharStat::Stamina) * amt, trans_mult);
}

void Character::gainBaseAllPercent(double amt, bool trans_mult) {
    gainBasePLPercent(amt, trans_mult);
    gainBaseKIPercent(amt, trans_mult);
    gainBaseSTPercent(amt, trans_mult);
}

void Character::loseBaseAllPercent(double amt, bool trans_mult) {
    loseBasePLPercent(amt, trans_mult);
    loseBaseKIPercent(amt, trans_mult);
    loseBaseSTPercent(amt, trans_mult);
}


double Character::getMaxCarryWeight() {
    return (getWeight() + 100.0) + (getMaxPL() / 200.0) + (GET_STR(this) * 50) + (IS_BARDOCK(this) ? 10000.0 : 0.0);
}

double Character::getEquippedWeight() {
    double total_weight = 0;

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(this, i)) {
            total_weight += GET_OBJ_WEIGHT(GET_EQ(this, i));
        }
    }
    return total_weight;
}

double Character::getCarriedWeight() {
    return getEquippedWeight() + getInventoryWeight() + (carrying ? carrying->getTotalWeight() : 0);
}

double Character::getAvailableCarryWeight() {
    return getMaxCarryWeight() - getCarriedWeight();
}

double Character::speednar() {
    auto ratio = (double) getCarriedWeight() / (double) getMaxCarryWeight();
    if (ratio >= .05)
        return std::max(0.01, std::min(1.0, 1.0 - ratio));
    return 1.0;
}

int64_t Character::getEffMaxPL() {
    if (IS_NPC(this)) {
        return getMaxPL();
    }
    return getMaxPL() * speednar();
}

bool Character::isWeightedPL() {
    return getMaxPL() > getEffMaxPL();
}

void Character::apply_kaioken(int times, bool announce) {
    GET_KAIOKEN(this) = times;
    clearFlag(FlagType::PC, PLR_POWERUP);

    if (announce) {
        this->sendf("@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        ::act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, this, nullptr,
              nullptr, TO_ROOM);
    }

}

void Character::remove_kaioken(int8_t announce) {
    auto kaio = GET_KAIOKEN(this);
    if (!kaio) {
        return;
    }
    GET_KAIOKEN(this) = 0;

    switch (announce) {
        case 1:
            this->sendf("You drop out of kaioken.\r\n");
            ::act("$n@w drops out of kaioken.@n", true, this, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            this->sendf("You lose focus and your kaioken disappears.\r\n");
            ::act("$n loses focus and $s kaioken aura disappears.", true, this, nullptr, nullptr, TO_ROOM);
    }
}


int Character::getRPP() {
    if(IS_NPC(this)) {
        return 0;
    }

    auto &p = reg.get<PlayerCharacter>(ent);

    return p.account->rpp;

}

void account_data::modRPP(int amt) {
    rpp += amt;
    if(rpp < 0) {
        rpp = 0;
    }
}

void Character::modRPP(int amt) {
    if(IS_NPC(this)) {
        return;
    }

    auto &p = reg.get<PlayerCharacter>(ent);

    return p.account->modRPP(amt);
}

int Character::getPractices() {
    return practice_points;
}

void Character::modPractices(int amt) {
    practice_points += amt;
    if(practice_points < 0) {
        practice_points = 0;
    }
}


void Character::login() {
    enter_player_game(desc);
    this->sendf("%s", CONFIG_WELC_MESSG);
    ::act("$n has entered the game.", true, this, nullptr, nullptr, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(this)), true, "%s has entered the game.", GET_NAME(this));
    /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
    auto count = 0;
    auto oldcount = HIGHPCOUNT;
    struct descriptor_data *k;

    for (k = descriptor_list; k; k = k->next) {
        if (!IS_NPC(k->character) && GET_LEVEL(k->character) > 3) {
            count += 1;
        }

        if (count > PCOUNT) {
            PCOUNT = count;
        }

        if (PCOUNT >= HIGHPCOUNT) {
            oldcount = HIGHPCOUNT;
            HIGHPCOUNT = PCOUNT;
            PCOUNTDATE = ::time(nullptr);
        }

    }

    time.logon = ::time(nullptr);
    greet_mtrigger(this, -1);
    greet_memory_mtrigger(this);

    STATE(desc) = CON_PLAYING;
    if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4) {
        payout(0);
    }
    if (PCOUNT == HIGHPCOUNT) {
        payout(1);
    }
    if (PCOUNT > oldcount) {
        payout(2);
    }

    /*~~~ End PCOUNT and HIGHPCOUNT ~~~*/
    if (GET_LEVEL(this) == 0) {
        do_start(this);
        this->sendf("%s", CONFIG_START_MESSG);
    }
    if (GET_ROOM_VNUM(IN_ROOM(this)) <= 1 && GET_LOADROOM(this) != NOWHERE) {
        removeFromLocation();
        addToLocation(getEntity(GET_LOADROOM(this)));
    } else if (GET_ROOM_VNUM(IN_ROOM(this)) <= 1) {
        removeFromLocation();
        addToLocation(getEntity(300));
    } else {
        lookAtLocation();
    }
    if (has_mail(GET_IDNUM(this)))
        this->sendf("\r\nYou have mail waiting.\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWIMM > GET_BOARD(this, 1))
        this->sendf(
                     "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWCOD > GET_BOARD(this, 2))
        this->sendf(
                     "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWBUI > GET_BOARD(this, 4))
        this->sendf(
                     "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWDUO > GET_BOARD(this, 3))
        this->sendf(
                     "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
    if (BOARDNEWMORT > GET_BOARD(this, 0))
        this->sendf("\r\n@GThere is a new bulletin board post.@n\r\n");
    if (NEWSUPDATE > GET_LPLAY(this))
        this->sendf(
                     "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n",
                     LASTNEWS);

    if (LASTINTEREST != 0 && LASTINTEREST > GET_LINTEREST(this)) {
        int diff = (LASTINTEREST - GET_LINTEREST(this));
        int mult = 0;
        while (diff > 0) {
            if ((diff - 86400) < 0 && mult == 0) {
                mult = 1;
            } else if ((diff - 86400) >= 0) {
                diff -= 86400;
                mult++;
            } else {
                diff = 0;
            }
        }
        if (mult > 3) {
            mult = 3;
        }
        GET_LINTEREST(this) = LASTINTEREST;
        if (GET_BANK_GOLD(this) > 0) {
            int inc = ((GET_BANK_GOLD(this) / 100) * 2);
            if (inc >= 7500) {
                inc = 7500;
            }
            inc *= mult;
            set(CharMoney::Bank, inc);
            this->sendf("Interest happened while you were away, %d times.\r\n"
                                       "@cBank Interest@D: @Y%s@n\r\n", mult, add_commas(inc).c_str());
        }
    }

    if (!IS_ANDROID(this)) {
        char buf3[MAX_INPUT_LENGTH];
        send_to_sense(0, "You sense someone appear suddenly", this);
        sprintf(buf3,
                "@D[@GBlip@D]@Y %s\r\n@RSomeone has suddenly entered your scouter detection range!@n.",
                add_commas(GET_HIT(this)).c_str());
        send_to_scouter(buf3, this, 0, 0);
    }

    desc->has_prompt = 0;
    /* We've updated to 3.1 - some bits might be set wrongly: */
    clearFlag(FlagType::Pref, PRF_BUILDWALK);
    if (!GET_EQ(this, WEAR_WIELD1) && PLR_FLAGGED(this, PLR_THANDW)) {
        clearFlag(FlagType::PC, PLR_THANDW);
    }

}

double Character::getAffectModifier(int location, int specific) {
    double total = 0;
    for(auto a = affected; a; a = a->next) {
        if(location != a->location) continue;
        if(specific != -1 && specific != a->specific) continue;
        total += a->modifier;
    }
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
        total += obj->getAffectModifier(location, specific);
    }

    total += race::getModifier(this, location, specific);
    total += trans::getModifier(this, location, specific);

    return total;
}

align_t Character::get(CharAlign type) {
    if(auto find = aligns.find(type); find != aligns.end()) {
        return find->second;
    }
    return 0;
}

align_t Character::set(CharAlign type, align_t val) {
    return aligns[type] = std::clamp<align_t>(val, -1000, 1000);
}

align_t Character::mod(CharAlign type, align_t val) {
    return set(type, get(type) + val);
}

appearance_t Character::get(CharAppearance type) {
    if(auto find = appearances.find(type); find != appearances.end()) {
        return find->second;
    }
    return 0;
}

appearance_t Character::set(CharAppearance type, appearance_t val) {
    return appearances[type] = std::clamp<appearance_t>(val, 0, 100);
}

appearance_t Character::mod(CharAppearance type, appearance_t val) {
    return set(type, get(type) + val);
}

int Character::setSize(int val) {
    this->size = val;
    return this->size;
}

int Character::getSize() {
    return size != SIZE_UNDEFINED ? size : race::getSize(race);
}


money_t Character::get(CharMoney mon) {
    if(auto find = moneys.find(mon); find != moneys.end()) {
        return find->second;
    }
    return 0;
}

money_t Character::set(CharMoney mon, money_t val) {
    return moneys[mon] = std::min<money_t>(val, 999999999999);
}

money_t Character::mod(CharMoney mon, money_t val) {
    return set(mon, get(mon) + val);
}


attribute_t Character::get(CharAttribute attr, bool base) {
    attribute_t val = 0;
    if(auto stat = attributes.find(attr); stat != attributes.end()) {
        val = stat->second;
    }
    if(!base) {
        val += getAffectModifier((int)attr+1) + getAffectModifier(APPLY_ALL_ATTRS);
        return std::clamp<attribute_t>(val, 5, 100);
    }
    return val;

}

attribute_t Character::set(CharAttribute attr, attribute_t val) {
    return attributes[attr] = std::clamp<attribute_t>(val, 0, 80);
}

attribute_t Character::mod(CharAttribute attr, attribute_t val) {
    return set(attr, get(attr) + val);
}

attribute_train_t Character::get(CharTrain attr) {
    if(auto t = trains.find(attr); t != trains.end()) {
        return t->second;
    }
    return 0;
}

attribute_train_t Character::set(CharTrain attr, attribute_train_t val) {
    return trains[attr] = std::max<attribute_train_t>(0, val);
}

attribute_train_t Character::mod(CharTrain attr, attribute_train_t val) {
    return set(attr, get(attr) + val);
}


num_t Character::get(CharNum stat) {
    if(auto st = nums.find(stat); st != nums.end()) {
        return st->second;
    }
    return 0;
}

num_t Character::set(CharNum stat, num_t val) {
    return nums[stat] = val;
}

num_t Character::mod(CharNum stat, num_t val) {
    return set(stat, get(stat) + val);
}

stat_t Character::set(CharStat type, stat_t val) {
    return stats[type] = std::max<stat_t>(0, val);
}

stat_t Character::mod(CharStat type, stat_t val) {
    return set(type, get(type) + val);
}

stat_t Character::get(CharStat type, bool base) {
    if(auto st = stats.find(type); st != stats.end()) {
        return st->second;
    }
    return 0;
}


bool Character::canCarryWeight(weight_t val) {
    double gravity = myEnvVar(EnvVar::Gravity);
    return getAvailableCarryWeight() >= (val * gravity);
}

bool Character::canCarryWeight(Object *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

bool Character::canCarryWeight(Character *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

weight_t Character::getCurrentBurden() {
    auto total = getTotalWeight();
    auto gravity = myEnvVar(EnvVar::Gravity);
    return total * gravity;
}

double Character::getBurdenRatio() {
    auto total = getCurrentBurden();
    auto max = getMaxCarryWeight();
    if(max == 0) return 0;
    return total / max;
}

void Character::restore_by(Character *ch) {
    this->restore(true);

    ::act("You have been fully healed by $N!", false, this, nullptr, ch, TO_CHAR | TO_SLEEP);
}

void Character::restore(bool announce) {
    restoreVitals(announce);
    restoreLimbs(announce);
    restoreStatus(announce);
    restoreLF(announce);
}

void Character::resurrect(ResurrectionMode mode) {
    // First, fully heal the character.
    restore(true);
    for(auto f : {AFF_ETHEREAL, AFF_SPIRIT}) clearFlag(FlagType::Affect,f);
    clearFlag(FlagType::PC, PLR_PDEATH);
    // Send them to their starting room and have them 'look'.
    removeFromLocation();
    if (GET_DROOM(this) != NOWHERE && GET_DROOM(this) != 0 && GET_DROOM(this) != 1) {
        addToLocation(getEntity(GET_DROOM(this)));
    } else {
        addToLocation(getEntity(sensei::getStartRoom(chclass)));
    }
    lookAtLocation();

    // If Costless, there's not going to be any penalties.
    int dur = 100;
    switch (mode) {
        case Costless:
            return;
        case Basic:
            if (dcount >= 8 && dcount < 10) {
                dur = 90;
            } else if (dcount >= 5 && dcount < 8) {
                dur = 75;
            } else if (dcount >= 3 && dcount < 5) {
                dur = 60;
            } else if (dcount >= 1 && dcount < 3) {
                dur = 40;
            }
            break;
        case RPP:
            dur = 100;
            break;
    }

    // Also no penalties if the character isn't at least level 10.
    if (GET_LEVEL(this) > 9) {
        int losschance = axion_dice(0);
        this->sendf(
                     "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength,itution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        std::map<CharAttribute, int> statReductions = {
            {CharAttribute::Strength, -8},
            {CharAttribute::Constitution, -8},
            {CharAttribute::Intelligence, -8},
            {CharAttribute::Wisdom, -8},
            {CharAttribute::Speed, -8},
            {CharAttribute::Agility, -8}
        };

        for (auto& [attr, reduction] : statReductions) {
            int baseStat = this->get(attr, true);
            int effectiveReduction = std::max(reduction, 15 - baseStat);
            statReductions[attr] = effectiveReduction;
        }

        assign_affect(this, AFF_WEAKENED_STATE, SKILL_WARP, dur,
                      statReductions[CharAttribute::Strength],
                      statReductions[CharAttribute::Constitution],
                      statReductions[CharAttribute::Intelligence],
                      statReductions[CharAttribute::Agility],
                      statReductions[CharAttribute::Wisdom],
                      statReductions[CharAttribute::Speed]);

        if (losschance >= 100) {
            int psloss = rand_number(100, 300);
            modPractices(-psloss);
            this->sendf("@R...and a loss of @r%d@R PS!@n", psloss);
        }
    }
    GET_DTIME(this) = 0;
    ::act("$n's body forms in a pool of @Bblue light@n.", true, this, nullptr, nullptr, TO_ROOM);
}

void Character::ghostify() {
    restore(true);
    for(auto f : {AFF_SPIRIT, AFF_ETHEREAL, AFF_KNOCKED, AFF_SLEEP, AFF_PARALYZE}) clearFlag(FlagType::Affect,f);

    // upon death, ghost-bodies gain new natural limbs... unless they're a
    // cyborg and want to keep their implants.
    if (!PRF_FLAGGED(this, PRF_LKEEP)) {
        for(auto f : {PLR_CLLEG, PLR_CRLEG, PLR_CLARM, PLR_CRARM}) clearFlag(FlagType::PC, f);
    }

}

void Character::teleport_to(IDXTYPE rnum) {
    removeFromLocation();
    auto r = getEntity<Room>(rnum);
    addToLocation(r);
    lookAtLocation();
    update_pos(this);
}

bool Character::in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum) {
    return GET_ROOM_VNUM(IN_ROOM(this)) >= low_rnum && GET_ROOM_VNUM(IN_ROOM(this)) <= high_rnum;
}

bool Character::in_past() {
    return ROOM_FLAGGED(IN_ROOM(this), ROOM_PAST);
}

bool Character::is_newbie() {
    return GET_LEVEL(this) < 9;
}

bool Character::in_northran() {
    return in_room_range(17900, 17999);
}

static std::map<int, uint16_t> grav_threshold = {
        {10,    5000},
        {20,    20000},
        {30,    50000},
        {40,    100000},
        {50,    200000},
        {100,   400000},
        {200,   1000000},
        {300,   5000000},
        {400,   8000000},
        {500,   15000000},
        {1000,  25000000},
        {5000,  100000000},
        {10000, 200000000}
};

int64_t Character::calc_soft_cap() {
    auto level = get(CharNum::Level);
    if(level >= 100) return 5e9;
    return race::getSoftCap(race, level);
}

bool Character::is_soft_cap(int64_t type) {
    return is_soft_cap(type, 1.0);
}

bool Character::is_soft_cap(int64_t type, long double mult) {
    if (IS_NPC(this))
        return true;

    // Level 100 characters are never softcapped.
    if (get(CharNum::Level) >= 100) {
        return false;
    }
    auto cur_cap = calc_soft_cap() * mult;

    int64_t against = 0;

    switch (type) {
        case 0:
            against = (getBasePL());
            break;
        case 1:
            against = (getBaseKI());
            break;
        case 2:
            against = (getBaseST());
            break;
    }

    return against >= cur_cap;
}

int Character::wearing_android_canister() {
    if (!IS_ANDROID(this))
        return 0;
    auto obj = GET_EQ(this, WEAR_BACKPACK);
    if (!obj)
        return 0;
    switch (GET_OBJ_VNUM(obj)) {
        case 1806:
            return 1;
        case 1807:
            return 2;
        default:
            return 0;
    }
}

int64_t Character::calcGravCost(int64_t num) {
    double gravity = myEnvVar(EnvVar::Gravity);
    int64_t cost = (gravity * gravity);

    if (!num) {
        if (cost) {
            this->sendf("You sweat bullets struggling against a mighty burden.\r\n");
        }
        if ((this->getCurST()) > cost) {
            this->decCurST(cost);
            return 1;
        } else {
            this->decCurST(cost);
            return 0;
        }
    } else {
        return (this->getCurST()) > (cost + num);
    }
}

weight_t Character::getWeight(bool base) {
    auto total = weight;

    if(!base) {
        total += getAffectModifier(APPLY_CHAR_WEIGHT);
        total *= (1.0 + getAffectModifier(APPLY_WEIGHT_MULT));
    }

    return total;
}

int Character::getHeight(bool base) {
    int total = get(CharNum::Height);

    if(!base) {
        total += getAffectModifier(APPLY_CHAR_HEIGHT);
        total *= (1.0 + getAffectModifier(APPLY_HEIGHT_MULT));
    }

    return total;
}

int Character::setHeight(int val) {
    return set(CharNum::Height, std::max(0, val));
}

int Character::modHeight(int val) {
    return setHeight(getHeight(true) + val);
}

double Character::getTotalWeight() {
    return getWeight() + getCarriedWeight();
}

double Character::currentGravity() {
    return myEnvVar(EnvVar::Gravity);
}

void Character::ageBy(double addedTime) {
    this->time.secondsAged += addedTime;
}

void Character::setAge(double newAge) {
    this->time.secondsAged = newAge * SECS_PER_GAME_YEAR;
}

bool Character::isPC() {
    return reg.all_of<PlayerCharacter>(ent);
}

bool Character::isNPC() {
    return reg.all_of<NonPlayerCharacter>(ent);
}