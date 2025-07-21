//
// Created by basti on 10/24/2021.
//
#include <boost/algorithm/string.hpp>


#include "dbat/structs.h"
#include "dbat/races.h"
#include "dbat/send.h"
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
#include "dbat/attack.h"
#include "dbat/modifiers.h"
#include "dbat/bitarray.h"


std::string char_data::juggleRaceName(bool capitalized) {

    std::string apparent = fmt::format("{}", race);

    switch (race) {
        case Race::hoshijin:
            if (mimic) apparent = fmt::format("{}", *mimic);
            break;
        case Race::halfbreed:
        case Race::android:
        case Race::saiyan:
            apparent = getAppearance(Appearance::seeming);
            break;
    }

    if (capitalized) {
        apparent[0] = std::toupper(apparent[0]);
    }
    return apparent;
}

void char_data::restore_by(char_data *ch) {
    this->restore(true);

    ::act("You have been fully healed by $N!", false, this, nullptr, ch, TO_CHAR | TO_SLEEP);
}

void char_data::restore(bool announce) {
    restoreVitals(announce);
    restoreLimbs(announce);
    restoreStatus(announce);
    restoreLF(announce);
}

void char_data::resurrect(ResurrectionMode mode) {
    // First, fully heal the character.
    restore(true);
    for(auto f : {AFF_ETHEREAL, AFF_SPIRIT}) affect_flags.set(f, false);
    player_flags.set(PLR_PDEATH, false);
    // Send them to their starting room and have them 'look'.
    char_from_room(this);
    if (GET_DROOM(this) != NOWHERE && GET_DROOM(this) != 0 && GET_DROOM(this) != 1) {
        char_to_room(this, real_room(GET_DROOM(this)));
    } else {
        char_to_room(this, real_room(sensei::getStartRoom(sensei)));
    }
    look_at_room(in_room, this, 0);

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
        send_to_char(this,
                     "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength,itution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        std::map<std::string, int> statReductions = {
            {"strength", -8},
            {"constitution", -8},
            {"intelligence", -8},
            {"wisdom", -8},
            {"speed", -8},
            {"agility", -8}
        };

        for (auto& [attr, reduction] : statReductions) {
            int baseStat = this->getBaseStat(attr);
            int effectiveReduction = std::max(reduction, 15 - baseStat);
            statReductions[attr] = effectiveReduction;
        }

        assign_affect(this, AFF_WEAKENED_STATE, SKILL_WARP, dur,
                      statReductions["strength"],
                      statReductions["constitution"],
                      statReductions["intelligence"],
                      statReductions["agility"],
                      statReductions["wisdom"],
                      statReductions["speed"]);

        if (losschance >= 100) {
            int psloss = rand_number(100, 300);
            modPractices(-psloss);
            send_to_char(this, "@R...and a loss of @r%d@R PS!@n", psloss);
        }
    }
    GET_DTIME(this) = 0;
    ::act("$n's body forms in a pool of @Bblue light@n.", true, this, nullptr, nullptr, TO_ROOM);
}

void char_data::ghostify() {
    restore(true);
    for(auto f : {AFF_SPIRIT, AFF_ETHEREAL, AFF_KNOCKED, AFF_SLEEP, AFF_PARALYZE}) affect_flags.set(f, false);

    // upon death, ghost-bodies gain new natural limbs... unless they're a
    // cyborg and want to keep their implants.
    if (!PRF_FLAGGED(this, PRF_LKEEP)) {
        for(auto f : {CharacterFlag::cyber_left_arm, CharacterFlag::cyber_right_arm, CharacterFlag::cyber_left_leg, CharacterFlag::cyber_right_leg}) character_flags.set(f, false);
    }

}

void char_data::teleport_to(IDXTYPE rnum) {
    char_from_room(this);
    char_to_room(this, real_room(rnum));
    look_at_room(IN_ROOM(this), this, 0);
    update_pos(this);
}

bool char_data::in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum) {
    return this->getRoomVnum() >= low_rnum && this->getRoomVnum() <= high_rnum;
}

bool char_data::in_past() {
    return this->getWhereFlag(WhereFlag::pendulum_past);
}

bool char_data::is_newbie() {
    return GET_MAX_HIT(this) <= 10000;
}

bool char_data::in_northran() {
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

int64_t char_data::calc_soft_cap() {
    return 750000000;
    //auto level = get(CharNum::Level);
    //if(level >= 100) return 5e9;
    //return race::getSoftCap(race, level);
}

bool char_data::is_soft_cap(int64_t type) {
    return is_soft_cap(type, 1.0);
}

bool char_data::is_soft_cap(int64_t type, long double mult) {
    if (IS_NPC(this))
        return true;
    
    return false;

    // Level 100 characters are never softcapped.
    if (get(CharNum::level) >= 100) {
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

int char_data::wearing_android_canister() {
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

bool char_data::hasGravAcclim(int grav) {
    //0 is x2, 1 is x5, 2 is x10, 3 is x50 and 4 is x100, 5 is x1000
    if(gravAcclim[grav] >= 10000)
        return true;
    return false;
}

void char_data::raiseGravAcclim() {
    if (rand_number(1, 140) >= getEffectiveStat("strength")) {
        auto room = getRoom();
        if(!room) return;
        auto gravity = room->getEnvironment(ENV_GRAVITY);

        if(gravity >= 1000 && !hasGravAcclim(5) && hasGravAcclim(4))
            gravAcclim[5] += 1;
        else if(gravity >= 100 && !hasGravAcclim(4) && hasGravAcclim(3))
            gravAcclim[4] += 1;
        else if(gravity >= 50 && !hasGravAcclim(3) && hasGravAcclim(2))
            gravAcclim[3] += 1;
        else if(gravity >= 10 && !hasGravAcclim(2) && hasGravAcclim(1))
            gravAcclim[2] += 1;
        else if(gravity >= 5 && !hasGravAcclim(1) && hasGravAcclim(0))
            gravAcclim[1] += 1;
        else if(gravity >= 2 && !hasGravAcclim(0))
            gravAcclim[0] += 1;
    }
}

int64_t char_data::calcGravCost(int64_t num) {
    double gravity = 1.0;
    if(auto room = getRoom(); room) gravity = room->getEnvironment(ENV_GRAVITY);

    if(gravity >= 1000 && hasGravAcclim(5))
        gravity /= 1000;
    else if(gravity >= 100 && hasGravAcclim(4))
        gravity /= 100;
    else if(gravity >= 50 && hasGravAcclim(3))
        gravity /= 50;
    else if(gravity >= 10 && hasGravAcclim(2))
        gravity /= 10;
    else if(gravity >= 5 && hasGravAcclim(1))
        gravity /= 5;
    else if(gravity >= 2 && hasGravAcclim(0))
        gravity /= 2;

    int64_t cost = (gravity * gravity);

    if (!num) {
        if (cost) {
            send_to_char(this, "You sweat bullets struggling against a mighty burden.\r\n");
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


int64_t char_data::getCurHealth() {
    return getCurPL();
}

int64_t char_data::getMaxHealth() {
    return getMaxPL();
}

double char_data::getCurHealthPercent() {
    return getCurPLPercent();
}

int64_t char_data::getPercentOfCurHealth(double amt) {
    return getPercentOfCurPL(amt);
}

int64_t char_data::getPercentOfMaxHealth(double amt) {
    return getPercentOfMaxPL(amt);
}

bool char_data::isFullHealth() {
    return isFullPL();
}

int64_t char_data::setCurHealth(int64_t amt) {
    return 0;
}

int64_t char_data::setCurHealthPercent(double amt) {
    return 0;
}

double char_data::modCurVitalDam(CharVital type, double dam) {
    return setCurVitalDam(type, getCurVitalDam(type) + dam);
}

double char_data::setCurVitalDam(CharVital type, double dam) {
    if(dam <= 0.0) damages.erase(type);
    else damages[type] = std::min(dam, 1.0);
    auto r = shared_from_this();
    if(damages.empty()) {
        characterSubscriptions.unsubscribe("characterVitalsRecovery", r);
        characterSubscriptions.unsubscribe("lifeforceSystem", r);
    }
    else {
        characterSubscriptions.subscribe("characterVitalsRecovery", r);
        if(!IS_ANDROID(this) && type == CharVital::powerlevel && GET_LIFEPERC(this) > 0 && (getCurHealthPercent() < static_cast<double>(GET_LIFEPERC(this)) / 100) && (getCurLF() > 0)) {
            characterSubscriptions.subscribe("lifeforceSystem", r);
        }
    }
    return getCurVitalDam(type);
}

double char_data::getCurVitalDam(CharVital type) {
    if(auto find = damages.find(type); find != damages.end()) return find->second;
    return 0.0;
}

int64_t char_data::incCurHealth(int64_t amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::powerlevel));
    if (limit_max)
        dmg = std::min(1.0, dmg + (double) std::abs(amt) / (double) getMaxPL());
    else
        dmg += (double) std::abs(amt) / (double) getMaxPL();
    setCurVitalDam(CharVital::powerlevel, 1.0 - dmg);
    return getCurHealth();
};

int64_t char_data::decCurHealth(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    auto dmg = (1.0 - getCurVitalDam(CharVital::powerlevel));
    auto sup = getBaseStat<int>("suppression");
    if (floor > 0)
        fl = (double) floor / (double) getMaxPL();
    if (sup > 0)
        dmg = std::max(fl, dmg - (double) std::abs(amt) / ((double) getMaxPL() * ((double) sup / 100.0)));
    else
        dmg = std::max(fl, dmg - (double) std::abs(amt) / (double) getMaxPL());
    setCurVitalDam(CharVital::powerlevel, 1.0 - dmg);
    return getCurHealth();
}

int64_t char_data::incCurHealthPercent(double amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::powerlevel));
    if (limit_max)
        dmg = std::min(1.0, dmg + std::abs(amt));
    else
        dmg += std::abs(amt);
    setCurVitalDam(CharVital::powerlevel, 1.0 - dmg);
    return getCurHealth();
}

int64_t char_data::decCurHealthPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    auto dmg = (1.0 - getCurVitalDam(CharVital::powerlevel));
    if (floor > 0)
        fl = (double) floor / (double) getMaxPL();
    dmg = std::max(fl, dmg - std::abs(amt));
    setCurVitalDam(CharVital::powerlevel, 1.0 - dmg);
    return getCurHealth();
}

void char_data::restoreHealth(bool announce) {
    if (!isFullHealth()) setCurVitalDam(CharVital::powerlevel, 0.0);
}

int64_t char_data::getMaxPLTrans() {
    auto total = getEffBasePL();

    total += getAffectModifier(APPLY_CVIT_BASE, static_cast<int>(CharVital::powerlevel));
    total *= (1.0 + getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::powerlevel)));
    return total;
}

int64_t char_data::getMaxPL() {
    auto total = getMaxPLTrans();
    /*if (GET_KAIOKEN(this) > 0) {
        total += (total / 10) * GET_KAIOKEN(this);
    }
    if (AFF_FLAGGED(this, AFF_METAMORPH)) {
        total += (total * .6);
    }*/
    return total;
}

int64_t char_data::getCurPL() {
    auto dmg = (1.0 - getCurVitalDam(CharVital::powerlevel));
    auto sup = getBaseStat<int>("suppression");
    if (!IS_NPC(this) && sup > 0) {
        return getMaxPL() * std::min(dmg, dmg * ((double) sup / 100));
    } else {
        return getMaxPL() * dmg;
    }
}

int64_t char_data::getUnsuppressedPL() {
    auto dmg = (1.0 - getCurVitalDam(CharVital::powerlevel));
    return getMaxPL() * dmg;
}

int64_t char_data::getEffBasePL() {
    if (original) return original->getEffBasePL();

    if (!clones.empty()) {
        return getBasePL() / (clones.size() + 1);
    } else {
        return getBasePL();
    }
}

int64_t char_data::getBasePL() {
    return getBaseStat("powerlevel");
}

double char_data::getCurPLPercent() {
    return (double) getCurPL() / (double) getMaxPL();
}

int64_t char_data::getPercentOfCurPL(double amt) {
    return getCurPL() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxPL(double amt) {
    return getMaxPL() * std::abs(amt);
}

bool char_data::isFullPL() {
    return getCurVitalDam(CharVital::powerlevel) <= 0.0;
}

int64_t char_data::getCurKI() {
    auto dmg = (1.0 - getCurVitalDam(CharVital::ki));
    return getMaxKI() * dmg;
}

int64_t char_data::getMaxKI() {
    auto total = getEffBaseKI();
    total += (getAffectModifier(APPLY_CVIT_BASE, static_cast<int>(CharVital::ki)));
    total *= (1.0 + getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::ki)));
    return total;
}

int64_t char_data::getEffBaseKI() {
    if (original) return original->getEffBaseKI();
    if (!clones.empty()) {
        return getBaseKI() / (clones.size() + 1);
    } else {
        return getBaseKI();
    }
}

int64_t char_data::getBaseKI() {
    return getBaseStat("ki");
}

double char_data::getCurKIPercent() {
    return (double) getCurKI() / (double) getMaxKI();
}

int64_t char_data::getPercentOfCurKI(double amt) {
    return getCurKI() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxKI(double amt) {
    return getMaxKI() * std::abs(amt);
}

bool char_data::isFullKI() {
    return getCurVitalDam(CharVital::ki) <= 0.0;
}

int64_t char_data::setCurKI(int64_t amt) {
    return 0;
}

int64_t char_data::setCurKIPercent(double amt) {
    return 0;
}

int64_t char_data::incCurKI(int64_t amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::ki));
    if (limit_max)
        dmg = std::min(1.0, dmg + (double) std::abs(amt) / (double) getMaxKI());
    else
        dmg += (double) std::abs(amt) / (double) getMaxKI();
    setCurVitalDam(CharVital::ki, 1.0 - dmg);
    return getCurKI();
};

int64_t char_data::decCurKI(int64_t amt, int64_t floor) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::ki));
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    dmg = std::max(fl, dmg - (double) std::abs(amt) / (double) getMaxKI());
    setCurVitalDam(CharVital::ki, 1.0 - dmg);
    return getCurKI();
}

int64_t char_data::incCurKIPercent(double amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::ki));
    if (limit_max)
        dmg = std::min(1.0, dmg + std::abs(amt));
    else
        dmg += std::abs(amt);
    setCurVitalDam(CharVital::ki, 1.0 - dmg);
    return getCurKI();
}

int64_t char_data::decCurKIPercent(double amt, int64_t floor) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::ki));
    if (!strcasecmp(this->name, "Wayland")) {
        send_to_char(this, "decCurKIPercent called with: %f\r\n", amt);
    }
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    dmg = std::max(fl, dmg - std::abs(amt));
    setCurVitalDam(CharVital::ki, 1.0 - dmg);
    return getCurKI();
}


void char_data::restoreKI(bool announce) {
    setCurVitalDam(CharVital::ki, 0.0);
}

int64_t char_data::getCurST() {
    auto dmg = (1.0 - getCurVitalDam(CharVital::stamina));
    return getMaxST() * dmg;
}

int64_t char_data::getMaxST() {
    auto total = getEffBaseST();
    total += getAffectModifier(APPLY_CVIT_BASE, static_cast<int>(CharVital::stamina));
    total *= (1.0 + getAffectModifier(APPLY_CVIT_MULT, static_cast<int>(CharVital::stamina)));
    return total;
}

int64_t char_data::getEffBaseST() {
    if (original) return original->getEffBaseST();
    if (!clones.empty()) {
        return getBaseST() / (clones.size() + 1);
    } else {
        return getBaseST();
    }
}

int64_t char_data::getBaseST() {
    return getBaseStat("stamina");
}

double char_data::getCurSTPercent() {
    return (double) getCurST() / (double) getMaxST();
}

int64_t char_data::getPercentOfCurST(double amt) {
    return getCurST() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxST(double amt) {
    return getMaxST() * std::abs(amt);
}

bool char_data::isFullST() {
    return getCurVitalDam(CharVital::stamina) <= 0.0;
}

int64_t char_data::setCurST(int64_t amt) {
    return 0;
}

int64_t char_data::setCurSTPercent(double amt) {
    return 0;
}

int64_t char_data::incCurST(int64_t amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::stamina));
    if (limit_max)
        dmg = std::min(1.0, dmg + (double) std::abs(amt) / (double) getMaxST());
    else
        dmg += (double) std::abs(amt) / (double) getMaxST();
    setCurVitalDam(CharVital::stamina, 1.0 - dmg);
    return getCurST();
};

int64_t char_data::decCurST(int64_t amt, int64_t floor) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::stamina));
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    dmg = std::max(fl, dmg - (double) std::abs(amt) / (double) getMaxST());
    setCurVitalDam(CharVital::stamina, 1.0 - dmg);
    return getCurST();
}

int64_t char_data::incCurSTPercent(double amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::stamina));
    if (limit_max)
        dmg = std::min(1.0, dmg + std::abs(amt));
    else
        dmg += std::abs(amt);
    setCurVitalDam(CharVital::stamina, 1.0 - dmg);
    return getMaxST();
}

int64_t char_data::decCurSTPercent(double amt, int64_t floor) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::stamina));
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    dmg = std::max(fl, dmg - std::abs(amt));
    setCurVitalDam(CharVital::stamina, 1.0 - dmg);
    return getCurST();
}


void char_data::restoreST(bool announce) {
    setCurVitalDam(CharVital::stamina, 0.0);
}


int64_t char_data::getCurLF() {
    auto dmg = (1.0 - getCurVitalDam(CharVital::lifeforce));
    return getMaxLF() * dmg;
}

int64_t char_data::getMaxLF() {
    auto lb = GET_LIFEBONUSES(this);

    return (IS_DEMON(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.75) + lb
                           : (IS_KONATSU(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.85) +
                    lb : (GET_MAX_MANA(this) * 0.5) +
                                                                         (GET_MAX_MOVE(this) * 0.5) +
                    lb));
}

bool char_data::isFullLF() {
    return getCurVitalDam(CharVital::lifeforce) <= 0.0;
}

int64_t char_data::incCurLF(int64_t amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::lifeforce));
    if (limit_max)
        dmg = std::min(1.0, dmg + (double) std::abs(amt) / (double) getMaxLF());
    else
        dmg += (double) std::abs(amt) / (double) getMaxLF();
    setCurVitalDam(CharVital::lifeforce, 1.0 - dmg);
    return getCurLF();
};

int64_t char_data::decCurLF(int64_t amt, int64_t floor) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::lifeforce));
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    dmg = std::max(fl, dmg - (double) std::abs(amt) / (double) getMaxLF());
    setCurVitalDam(CharVital::lifeforce, 1.0 - dmg);
    return getCurLF();
}

int64_t char_data::incCurLFPercent(double amt, bool limit_max) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::lifeforce));
    if (limit_max)
        dmg = std::min(1.0, dmg + std::abs(amt));
    else
        dmg += std::abs(amt);
    setCurVitalDam(CharVital::lifeforce, 1.0 - dmg);
    return getCurLF();
}

int64_t char_data::decCurLFPercent(double amt, int64_t floor) {
    auto dmg = (1.0 - getCurVitalDam(CharVital::lifeforce));
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    dmg = std::max(fl, dmg - std::abs(amt));
    setCurVitalDam(CharVital::lifeforce, 1.0 - dmg);
    return getCurLF();
}


void char_data::restoreLF(bool announce) {
    setCurVitalDam(CharVital::lifeforce, 0.0);
}


bool char_data::isFullVitals() {
    return isFullHealth() && isFullKI() && isFullST();
}

void char_data::restoreVitals(bool announce) {
    restoreHealth(announce);
    restoreKI(announce);
    restoreST(announce);
}

void char_data::restoreStatus(bool announce) {
    cureStatusKnockedOut(announce);
    cureStatusBurn(announce);
    cureStatusPoison(announce);
}

void char_data::setStatusKnockedOut() {
    affect_flags.set(AFF_KNOCKED, true);
    affect_flags.set(AFF_FLYING, false);
    altitude = 0;
    GET_POS(this) = POS_SLEEPING;
}

void char_data::cureStatusKnockedOut(bool announce) {
    if (AFF_FLAGGED(this, AFF_KNOCKED)) {
        if (announce) {
            ::act("@W$n@W is no longer senseless, and wakes up.@n", false, this, nullptr, nullptr, TO_ROOM);
            send_to_char(this, "You are no longer knocked out, and wake up!@n\r\n");
        }

        if (CARRIED_BY(this)) {
            if (GET_ALIGNMENT(CARRIED_BY(this)) > 50) {
                carry_drop(CARRIED_BY(this), 0);
            } else {
                carry_drop(CARRIED_BY(this), 1);
            }
        }

        affect_flags.set(AFF_KNOCKED, false);
        GET_POS(this) = POS_SITTING;
    }
}

void char_data::cureStatusBurn(bool announce) {
    if (AFF_FLAGGED(this, AFF_BURNED)) {
        if (announce) {
            send_to_char(this, "Your burns are healed now.\r\n");
            ::act("$n@w's burns are now healed.@n", true, this, nullptr, nullptr, TO_ROOM);
        }
        affect_flags.set(AFF_BURNED, false);
    }
}

void char_data::cureStatusPoison(bool announce) {
    ::act("@C$n@W suddenly looks a lot better!@b", false, this, nullptr, nullptr, TO_NOTVICT);
    affect_from_char(this, SPELL_POISON);
}

static std::map<int, std::string> limb_names = {
        {0, "right arm"},
        {1, "left arm"},
        {2, "right leg"},
        {3, "left leg"}
};

void char_data::restoreLimbs(bool announce) {
    // restore head...
    GET_LIMBCOND(this, 0) = 100;

    // limbs...
    for (const auto &l: limb_names) {
        if (announce) {
            if (GET_LIMBCOND(this, l.first) <= 0)
                send_to_char(this, "Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(this, l.first) < 50)
                send_to_char(this, "Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(this, l.first) = 100;
    }

    // and lastly, tail.
    this->gainTail(announce);
}


void char_data::gainTail(bool announce) {
    if (!race::hasTail(race)) return;
    if(character_flags.get(CharacterFlag::tail)) return;
    character_flags.set(CharacterFlag::tail, true);
    if(announce) {
        send_to_char(this, "@wYour tail grows back.@n\r\n");
        act("$n@w's tail grows back.@n", true, this, nullptr, nullptr, TO_ROOM);
    }
}

void char_data::loseTail() {
    if (!character_flags.get(CharacterFlag::tail)) return;
    character_flags.set(CharacterFlag::tail, false);
    remove_limb(this, 6);
    GET_TGROWTH(this) = 0;
    oozaru_revert(this);
}

bool char_data::hasTail() {
    return character_flags.get(CharacterFlag::tail);
}

void char_data::addTransform(Form form) {
    transforms.insert({form, trans_data()});
}

void char_data::hideTransform(Form form, bool hide) {
    auto foundForm = transforms.find(form);
    foundForm->second.visible = !hide;
}

bool char_data::removeTransform(Form form) {
    if (transforms.contains(form))
    {
        transforms.erase(form);
        return true;
    }
    //Return if failure
    return false;
}

void char_data::attemptLimitBreak() {
    if(form == Form::base)
        return;
    if(transforms[form].time_spent_in_form > 50000 && rand_number(0, 1000) == 1000) {
        transforms[form].limit_broken = true;
        incCurHealthPercent(0.35);
        incCurKIPercent(0.35);
        incCurSTPercent(0.35);
        send_to_char(this, "@mA rush of energy bursts through your system as you defy your limits.@n\r\n");
        
        affect_flags.set(AFF_LIMIT_BREAKING, true);         
    }
}

void char_data::removeLimitBreak() {
    if (AFF_FLAGGED(this, AFF_LIMIT_BREAKING)) {
        this->affect_flags.set(AFF_LIMIT_BREAKING, false);
        send_to_char(this, "@mYou feel your body finally calm down.@n\r\n");
    }
}

int64_t char_data::getPL() {
    int64_t vitalCalc = (getMaxPL() + getMaxKI()) / 4;
    int attrCalc = (getEffectiveStat("agility") + getEffectiveStat("constitution") + getEffectiveStat("intelligence") + getEffectiveStat("speed")
    + getEffectiveStat("strength") + getEffectiveStat("wisdom")) / 50;

    auto sup = getBaseStat<int>("suppression");

    double suppressed = sup > 0 ? ((double) sup / 100.0) : 1;
    double speed = getEffectiveStat("speednar");

    double pl = vitalCalc * attrCalc * speed * suppressed;

    if(IS_NPC(this)) {
        if(GET_LEVEL(this) < 10)
            pl *= 2;
        else if(GET_LEVEL(this) < 30)
            pl /= 1;
        else if(GET_LEVEL(this) < 50)
            pl /= 4;
        else if(GET_LEVEL(this) < 70)
            pl /= 6;
        else if(GET_LEVEL(this) < 90)
            pl /= 8;
        else
            pl /= 10;
    }

    return pl;
}

void char_data::apply_kaioken(int times, bool announce) {
    GET_KAIOKEN(this) = times;
    character_flags.set(CharacterFlag::powering_up, false);

    if (announce) {
        send_to_char(this, "@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        ::act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, this, nullptr,
              nullptr, TO_ROOM);
    }

}

void char_data::remove_kaioken(int8_t announce) {
    auto kaio = GET_KAIOKEN(this);
    if (!kaio) {
        return;
    }
    GET_KAIOKEN(this) = 0;

    switch (announce) {
        case 1:
            send_to_char(this, "You drop out of kaioken.\r\n");
            ::act("$n@w drops out of kaioken.@n", true, this, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            send_to_char(this, "You lose focus and your kaioken disappears.\r\n");
            ::act("$n loses focus and $s kaioken aura disappears.", true, this, nullptr, nullptr, TO_ROOM);
    }
}


int char_data::getRPP() {
    if(IS_NPC(this)) {
        return 0;
    }

    auto &p = players[id];

    return p.account->rpp;

}

void account_data::modRPP(int amt) {
    rpp += amt;
    if(rpp < 0) {
        rpp = 0;
    }
}

void char_data::modRPP(int amt) {
    if(IS_NPC(this)) {
        return;
    }

    auto &p = players[id];

    p.account->modRPP(amt);
}

int char_data::getPractices() {
    return practice_points;
}

void char_data::modPractices(int amt) {
    practice_points += amt;
    if(practice_points < 0) {
        practice_points = 0;
    }
}


void char_data::login() {
    enter_player_game(desc);
    send_to_char(this, "%s", CONFIG_WELC_MESSG);
    ::act("$n has entered the game.", true, this, nullptr, nullptr, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(this)), true, "%s has entered the game.", GET_NAME(this));
    /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
    auto count = 0;
    auto oldcount = HIGHPCOUNT;
    struct descriptor_data *k;

    for (k = descriptor_list; k; k = k->next) {
        if (!IS_NPC(k->character) && GET_MAX_HIT(k->character) > 5000) {
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
        send_to_char(this, "%s", CONFIG_START_MESSG);
    }
    if (this->getRoomVnum() <= 1 && GET_LOADROOM(this) != NOWHERE) {
        char_from_room(this);
        char_to_room(this, real_room(real_room(GET_LOADROOM(this))));
    } else if (this->getRoomVnum() <= 1) {
        char_from_room(this);
        char_to_room(this, real_room(real_room(300)));
    } else {
        look_at_room(IN_ROOM(this), this, 0);
    }
    if (has_mail(GET_IDNUM(this)))
        send_to_char(this, "\r\nYou have mail waiting.\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWIMM > GET_BOARD(this, 1))
        send_to_char(this,
                     "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWCOD > GET_BOARD(this, 2))
        send_to_char(this,
                     "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWBUI > GET_BOARD(this, 4))
        send_to_char(this,
                     "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWDUO > GET_BOARD(this, 3))
        send_to_char(this,
                     "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
    if (BOARDNEWMORT > GET_BOARD(this, 0))
        send_to_char(this, "\r\n@GThere is a new bulletin board post.@n\r\n");
    if (NEWSUPDATE > GET_LPLAY(this))
        send_to_char(this,
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
            setBaseStat("money_bank", inc);
            send_to_char(this, "Interest happened while you were away, %d times.\r\n"
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
    pref_flags.set(PRF_BUILDWALK, false);
    if (!GET_EQ(this, WEAR_WIELD1) && PLR_FLAGGED(this, PLR_THANDW)) {
        player_flags.set(PLR_THANDW, false);
    }

}

double char_data::getAffectModifier(uint64_t location, uint64_t specific) {
    double total = 0;
    // Personal modifiers.
    for(auto a = affected; a; a = a->next) {
        if(!a->match(location, specific)) continue;
        total += a->modifier;
    }

    // Equipment modifiers.
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
        total += obj->getAffectModifier(location, specific);
    }

    total += race::getModifier(this, location, specific);
    total += trans::getModifier(this, location, specific);

    // Position modifier.
    if(auto find = pos_affects.find(position); find != pos_affects.end()) {
        for(auto &eff : find->second) {
            if(!eff.match(location, specific)) continue;
            total += eff.modifier;
            if(eff.func) total += eff.func(this);
        }
    }

    return total;
}

int char_data::setSize(int val) {
    this->size = static_cast<Size>(val);
    return static_cast<int>(this->size);
}

int char_data::getSize() {
    return static_cast<int>(size) != SIZE_UNDEFINED ? static_cast<int>(size) : race::getSize(race);
}

double getDaysPassed() {
    double ingameDays = era_uptime.day + (era_uptime.month * 30) + (era_uptime.year * 365);
    return ingameDays;
}

double char_data::getPotential() {
    //Gain one potential per RL week, reaches 100 in two years
    double timePotential = 1 + (getDaysPassed() / 7);

    int physiquePotential = 1;
    if(hasGravAcclim(0)) physiquePotential += 1;
    if(hasGravAcclim(1)) physiquePotential += 1;
    if(hasGravAcclim(2)) physiquePotential += 1;
    if(hasGravAcclim(3)) physiquePotential += 1;
    if(hasGravAcclim(4)) physiquePotential += 1;
    if(hasGravAcclim(5)) physiquePotential += 1;
    return timePotential * physiquePotential;
}

void char_data::gainGrowth() {
    double modifier = 1;
    auto r = getRoom();
    if (r->where_flags[WhereFlag::afterlife_hell] || r->where_flags[WhereFlag::afterlife]) {
        modifier = 1.5;
    }

    // Set with the idea its triggered once every 5 mins. Calculates to 0.5 per day. (288 '5 mins' in a day. Double to half the gain)
    double gain = modifier / (288.0 * 2);
    gainGrowth(gain);
}

void char_data::gainGrowth(double gain) {
    // You cannot exceed the amount of days the server has been online for
    double days = getDaysPassed();

    // Roughly increase by a multiplier of 1 every 60 days past the first 60
    double timeMod = std::max((getDaysPassed() / 60.0) , 1.0);

    gain *= timeMod;
    
    auto ltg = getBaseStat("lifetimeGrowth");
    if (ltg + gain < days) {
        // If you have stored growth, we double the amount gained and reduce overgrowth by that amount
        if (getBaseStat("overGrowth") >= gain && ltg + (gain * 2) < days) {
            modBaseStat("overGrowth", -gain);
            gain *= 2;
        }
        for(const auto &s : {"internalGrowth", "lifetimeGrowth"}) modBaseStat(s, gain);
    } else {
        // Any overflow is stored in overGrowth
        double cangain = days - ltg;
        for(const auto &s : {"internalGrowth", "lifetimeGrowth"}) modBaseStat(s, cangain);

        cangain = gain - cangain;
        modBaseStat("overGrowth", cangain);
    }
}

attribute_train_t char_data::get(CharTrain attr) {
    if(auto t = trains.find(attr); t != trains.end()) {
        return t->second;
    }
    return 0;
}

attribute_train_t char_data::set(CharTrain attr, attribute_train_t val) {
    return trains[attr] = std::max<attribute_train_t>(0, val);
}

attribute_train_t char_data::mod(CharTrain attr, attribute_train_t val) {
    return set(attr, get(attr) + val);
}


num_t char_data::get(CharNum stat) {
    if(auto st = nums.find(stat); st != nums.end()) {
        return st->second;
    }
    return 0;
}

num_t char_data::set(CharNum stat, num_t val) {
    return nums[stat] = val;
}

num_t char_data::mod(CharNum stat, num_t val) {
    return set(stat, get(stat) + val);
}


bool char_data::canCarryWeight(weight_t val) {
    double gravity = 1.0;
    if(auto room = getRoom(); room) gravity = room->getEnvironment(ENV_GRAVITY);
    if(gravity >= 1000 && hasGravAcclim(5))
        gravity /= 1000;
    else if(gravity >= 100 && hasGravAcclim(4))
        gravity /= 100;
    else if(gravity >= 50 && hasGravAcclim(3))
        gravity /= 50;
    else if(gravity >= 10 && hasGravAcclim(2))
        gravity /= 10;
    else if(gravity >= 5 && hasGravAcclim(1))
        gravity /= 5;
    else if(gravity >= 2 && hasGravAcclim(0))
        gravity /= 2;

    return getEffectiveStat("carry_available") >= (val * gravity);
}

bool char_data::canCarryWeight(struct obj_data *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

bool char_data::canCarryWeight(struct char_data *obj) {
    return canCarryWeight(obj->getBaseStat("weight_total"));
}

room_vnum char_data::normalizeLoadRoom(room_vnum in) {
    // If they were in the void, then we need to use their last good room.
    room_vnum room = NOWHERE;
    room_vnum lroom = NOWHERE;
    // Handle the void issue...
    if (in == 0 || in == 1) {
        room = GET_WAS_IN(this);
    } else {
        room = in;
    }

    // Personal Pocket Dimensions
    //if (room >= 19800 && room <= 19899) {
        //lroom = room;
    //}
        // those stuck in the pendulum room past get returned to Kami's Lookout.
    if (WHERE_FLAGGED(room, WhereFlag::pendulum_past)) {
        lroom = 1561;
    }
        // the WMAT arena also is not a good place to log off.
    else if (room >= 2002 && room <= 2011) {
        lroom = 1960;
    }
        // The two Minecarts are possible trap zones.
    else if (room == 2069) {
        lroom = 2017;
    }
    else if (room == 2070) {
        lroom = 2046;
    }
        // The higher plane is a problem...
    else if(room == 6030) {
        // Stick them on the side of King Yemma's, in case they're broke/weak.
        lroom = 6029;
    }
        // This is the MUD School. If they're not done then put them
        // back at the start. Otherwise, send them to their Sensei.
    else if (room >= 101 && room <= 139) {
        if (GET_LEVEL(this) == 1) {
            lroom = 100;
            setExperience(0);
        } else {
            lroom = sensei::getStartRoom(sensei);
        }
    }
    else {
        // looks like room might be okay.
        lroom = room;
    }

    // if lroom is valid, save it... else... emergency fallback to mud school.
    if(auto r = get_room(lroom); r) return lroom;
    return CONFIG_MORTAL_START;

}

std::map<int, obj_data *> char_data::getEquipment() {
    std::map<int, obj_data*> out;
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto eq = GET_EQ(this, i); eq) {
            out[i] = eq;
        }
    }
    return out;
}

obj_data* char_data::getEquipSlot(int slot) {
    if(slot < 0 || slot > NUM_WEARS-1) return nullptr;
    return GET_EQ(this, slot);
}


int char_data::getArmor() {
    return getEffectiveStat<int>("armor");
}

void char_data::onAttack(atk::Attack& outgoing) {
    if(form != Form::base)
        trans::onAttack(this, outgoing, form);
    if(technique != Form::base)
        trans::onAttack(this, outgoing, technique);
}

void char_data::onAttacked(atk::Attack& incoming) {
    if(form != Form::base)
        trans::onAttacked(this, incoming, form);
    if(technique != Form::base)
        trans::onAttacked(this, incoming, technique);
}

stat_t char_data::set(CharStat type, stat_t val) {
    return stats[type] = val;
}

stat_t char_data::mod(CharStat type, stat_t val) {
    return set(type, get(type) + val);
}

stat_t char_data::get(CharStat type) {
    if(auto st = stats.find(type); st != stats.end()) {
        return st->second;
    }
    return 0;
}

int64_t char_data::getExperience() {
    return getEffectiveStat("experience");
}

int64_t char_data::setExperience(int64_t value) {
    return setBaseStat("experience", value);
}

// This returns the exact amount that was modified by.
int64_t char_data::modExperience(int64_t value, bool applyBonuses) {

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
        gain *= (1.0 + getAffectModifier(APPLY_CSTAT_GAIN_MULT, static_cast<int>(CharStat::experience)));

        if (AFF_FLAGGED(this, AFF_WUNJO)) {
            gain *= 1.15;
        }
        if (PLR_FLAGGED(this, PLR_IMMORTAL)) {
            gain *= 0.95;
        }

        int64_t diff = gain * 0.15;

        if (gain > 0) {

            // TODO: Modify the spar booster with APPLY_EXP_GAIN_MULT 0.25
            if (auto obj = GET_EQ(this, WEAR_SH); obj && obj->vn == 1127) {
                int64_t spar = gain;
                gain += gain * 0.25;
                spar = gain - spar;
                send_to_char(this, "@D[@BBooster EXP@W: @G+%s@D]\r\n", add_commas(spar).c_str());
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
                        this->gainBaseStat("powerlevel", diff * 0.8);
                    } else {
                        this->gainBaseStat("powerlevel", diff);
                    }
                    send_to_char(this, "@D[@G+@Y%s @RPL@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2) {
                    if (IS_HALFBREED(this)) {
                        this->gainBaseStat("stamina", diff * 0.85);
                    } else {
                        this->gainBaseStat("stamina", diff);
                    }
                    send_to_char(this, "@D[@G+@Y%s @gSTA@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2) {
                    this->gainBaseStat("ki", diff);
                    send_to_char(this, "@D[@G+@Y%s @CKi@D]@n", add_commas(diff).c_str());
                }
            }
        }
    }

    // Amount gained cannot be negative.
    gain = std::max<int64_t>(gain, 0);

    if (MINDLINK(this) && gain > 0 && LINKER(this) == 0) {
        if (GET_INT(this) + 20 < GET_INT(MINDLINK(this)) || GET_INT(this) - 20 > GET_INT(MINDLINK(this))) {
            send_to_char(MINDLINK(this),
                         "The intelligence difference between the two of you is too great to gain from mind read.\r\n");
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
            send_to_char(this, "@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");
        }

        int64_t max_over_tnl = tnl * 5;
        if((cur + gain) >= max_over_tnl) {
            gain = max_over_tnl - getExperience();
            send_to_char(this, "@WYou -@RNEED@W- to @ylevel@W. You can't hold any more experience!@n\r\n");
        }

    }

    if(gain) setExperience(cur + gain);
    return gain;

}

void char_data::gazeAtMoon() {
    if(OOZARU_RACE(this) && character_flags.get(CharacterFlag::tail)) {
        if(form == Form::oozaru || form == Form::golden_oozaru) return;
        Form toForm = Form::oozaru;
        if(transforms.contains(Form::super_saiyan_1)
        || transforms.contains(Form::super_saiyan_2)
        || transforms.contains(Form::super_saiyan_3)
        || transforms.contains(Form::super_saiyan_4))
            toForm = Form::golden_oozaru;

        form = toForm;
    } else if (transforms.contains(Form::lycanthrope)) {
        if (transforms.contains(Form::alpha_lycanthrope)) {
            form = Form::alpha_lycanthrope;
        } else { 
            form = Form::lycanthrope;
        }
    }
    trans::handleEchoTransform(this, form);
}


static const std::map<std::string, std::string> _attr_names = {
    {"str", "strength"},
    {"wis", "wisdom"},
    {"con", "constitution"},
    {"cha", "speed"},
    {"spd", "speed"},
    {"dex", "agility"},
    {"agi", "agility"},
    {"int", "intelligence"},

    {"strength", "strength"},
    {"wisdom", "wisdom"},
    {"constitution", "constitution"},
    {"speed", "speed"},
    {"agility", "agility"},
    {"intelligence", "intelligence"}
};

static const std::map<std::string, std::string> _money_names = {
    {"bank", "money_bank"},
    {"gold", "money_carried"},
    {"zenni", "money_carried"}
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


std::optional<std::string> char_data::dgCallMember(const std::string& member, const std::string& arg) {
    std::string lmember = member;
    boost::to_lower(lmember);
    boost::trim(lmember);

    if(auto attr = _attr_names.find(lmember); attr != _attr_names.end()) {
        if (!arg.empty()) {
            attribute_t addition = atof(arg.c_str());
            modBaseStat(attr->second, addition);
        }
        return fmt::format("{}", getBaseStat(attr->second));
    }

    if(auto mon = _money_names.find(lmember); mon != _money_names.end()) {
        if (!arg.empty()) {
            money_t addition = atoll(arg.c_str());
            modBaseStat(mon->second, addition);
        }
        return fmt::format("{}", (money_t)getBaseStat(mon->second));
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
            GET_SAVE_MOD(this, save->second) += addition;
        }
        return fmt::format("{}", GET_SAVE_MOD(this, save->second));
    }

    if(auto pf = _pflags.find(lmember); pf != _pflags.end()) {
        if (!arg.empty()) {
            if (!strcasecmp("on", arg.c_str()))
                player_flags.set(pf->second, true);
            else if (!strcasecmp("off", arg.c_str()))
                player_flags.set(pf->second, false);
        }
        return player_flags.get(pf->second) ? "1" : "0";
    }

    if(auto af = _aflags.find(lmember); af != _aflags.end()) {
        return AFF_FLAGGED(this, af->second) ? "1" : "0";
    }

    return {};
}

void char_data::setTask(Task t) {
    task = t;
    if(task == Task::nothing) {
        if(wait_input_queue.empty()) characterSubscriptions.unsubscribe("commandWaitQueue", this);
    } else {
        characterSubscriptions.subscribe("commandWaitQueue", this);
    }
}

std::string char_data::getAppearance(Appearance type, bool withTransform) {
    if(withTransform && form != Form::base) {
        if(auto override = trans::getAppearance(this, form, type); override) {
            return override.value();
        }
    }
    
    if(auto find = appearances.find(type); find != appearances.end()) {
        return find->second;
    }
    return race::defaultAppearance(this, type);
}

const char* char_data::getAppearanceStr(Appearance type) {
    static char buf[MAX_STRING_LENGTH];
    snprintf(buf, MAX_STRING_LENGTH, "%s", getAppearance(type).c_str());
    return buf;
}