//
// Created by basti on 10/24/2021.
//
#include "structs.h"
#include "races.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "class.h"
#include "fight.h"
#include "act.movement.h"

static std::string robot = "Robotic-Humanoid", robot_lower = "robotic-humanoid", unknown = "UNKNOWN";


const std::string& char_data::juggleRaceName(bool capitalized) const {
    if(!race) return unknown;

    dbat::race::Race *apparent = race;

    switch(apparent->getID()) {
        case dbat::race::hoshijin:
            if(mimic) apparent = mimic;
            break;
        case dbat::race::halfbreed:
            switch(RACIAL_PREF(this)) {
                case 1:
                    apparent = dbat::race::race_map[dbat::race::human];
                    break;
                case 2:
                    apparent = dbat::race::race_map[dbat::race::saiyan];
                    break;
            }
            break;
        case dbat::race::android:
            switch(RACIAL_PREF(this)) {
                case 1:
                    apparent = dbat::race::race_map[dbat::race::android];
                    break;
                case 2:
                    apparent = dbat::race::race_map[dbat::race::human];
                    break;
                case 3:
                    if(capitalized) {
                        return robot;
                    } else {
                        return robot_lower;
                    }
            }
            break;
        case dbat::race::saiyan:
            if(PLR_FLAGGED(this, PLR_TAILHIDE)) {
                apparent = dbat::race::race_map[dbat::race::human];
            }
            break;
    }

    if(capitalized) {
        return apparent->getName();
    } else {
        return apparent->getNameLower();
    }
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
    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_ETHEREAL);
    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_SPIRIT);
    REMOVE_BIT_AR(PLR_FLAGS(this), PLR_PDEATH);
    // Send them to their starting room and have them 'look'.
    char_from_room(this);
    if (GET_DROOM(this) != NOWHERE && GET_DROOM(this) != 0 && GET_DROOM(this) != 1) {
        char_to_room(this, real_room(GET_DROOM(this)));
    }
    else {
        char_to_room(this, real_room(this->chclass->senseiStartRoom()));
    }
    look_at_room(IN_ROOM(this), this, 0);

    // If Costless, there's not going to be any penalties.
    int dur = 100;
    switch(mode) {
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
        send_to_char(this, "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength, constitution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        int str = -8, intel = -8, wis = -8, spd = -8, con = -8, agl = -8;
        if (this->real_abils.intel <= 16) {
            intel = -4;
        }
        if (this->real_abils.cha <= 16) {
            spd = -4;
        }
        if (this->real_abils.dex <= 16) {
            agl = -4;
        }
        if (this->real_abils.wis <= 16) {
            wis = -4;
        }
        if (this->real_abils.con <= 16) {
            con = -4;
        }
        assign_affect(this, AFF_WEAKENED_STATE, SKILL_WARP, dur, str, con, intel, agl, wis, spd);
        if (losschance >= 100) {
            int psloss = rand_number(100, 300);
            GET_PRACTICES(this, GET_CLASS(this)) -= psloss;
            send_to_char(this, "@R...and a loss of @r%d@R PS!@n", psloss);
            if (GET_PRACTICES(this, GET_CLASS(this)) < 0) {
                GET_PRACTICES(this, GET_CLASS(this)) = 0;
            }
        }
    }
    GET_DTIME(this) = 0;
    ::act("$n's body forms in a pool of @Bblue light@n.", true, this, nullptr, nullptr, TO_ROOM);
}

void char_data::ghostify() {
    restore(true);
    SET_BIT_AR(AFF_FLAGS(this), AFF_SPIRIT);
    SET_BIT_AR(AFF_FLAGS(this), AFF_ETHEREAL);

    // upon death, ghost-bodies gain new natural limbs... unless they're a
    // cyborg and want to keep their implants.
    if (!PRF_FLAGGED(this, PRF_LKEEP)) {
        if (PLR_FLAGGED(this, PLR_CLLEG)) {
            REMOVE_BIT_AR(PLR_FLAGS(this), PLR_CLLEG);
        }
        if (PLR_FLAGGED(this, PLR_CRLEG)) {
            REMOVE_BIT_AR(PLR_FLAGS(this), PLR_CRLEG);
        }
        if (PLR_FLAGGED(this, PLR_CRARM)) {
            REMOVE_BIT_AR(PLR_FLAGS(this), PLR_CRARM);
        }
        if (PLR_FLAGGED(this, PLR_CLARM)) {
            REMOVE_BIT_AR(PLR_FLAGS(this), PLR_CLARM);
        }
    }

    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_KNOCKED);
    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_SLEEP);
    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_PARALYZE);

}

void char_data::teleport_to(IDXTYPE rnum) {
    char_from_room(this);
    char_to_room(this, real_room(rnum));
    look_at_room(IN_ROOM(this), this, 0);
    update_pos(this);
}

bool char_data::in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum) const {
    return GET_ROOM_VNUM(IN_ROOM(this)) >= low_rnum && GET_ROOM_VNUM(IN_ROOM(this)) <= high_rnum;
}

bool char_data::in_past() const {
    return ROOM_FLAGGED(IN_ROOM(this), ROOM_PAST);
}

bool char_data::is_newbie() const {
    return GET_LEVEL(this) < 9;
}

bool char_data::in_northran() const {
    return in_room_range(17900, 17999);
}

static std::map<int, uint16_t> grav_threshold = {
        {10, 5000},
        {20, 20000},
        {30, 50000},
        {40, 100000},
        {50, 200000},
        {100, 400000},
        {200, 1000000},
        {300, 5000000},
        {400, 8000000},
        {500, 15000000},
        {1000, 25000000},
        {5000, 100000000},
        {10000, 200000000}
};

bool char_data::can_tolerate_gravity(int grav) const {
    if(IS_NPC(this)) return true;
    int tolerance = 0;
    tolerance = std::max(tolerance, this->chclass->getGravTolerance());
    if(tolerance >= grav)
        return true;
    return GET_MAX_HIT(this) >= grav_threshold[grav];
}


int char_data::calcTier() const {
    int tier = level / 10;
    if((level % 10) == 0)
        tier--;
    tier = std::max(tier, 0);
    tier = std::min(tier, 9);
    return tier;
}

int64_t char_data::calc_soft_cap() const {
    auto tier = calcTier();
    auto softmap = race->getSoftMap(this);
    return level * softmap[tier];
}

bool char_data::is_soft_cap(int64_t type) const {
    return is_soft_cap(type, 1.0);
}

bool char_data::is_soft_cap(int64_t type, long double mult) const {
    if(IS_NPC(this))
        return true;

    // Level 100 characters are never softcapped.
    if(level >= 100) {
        return false;
    }
    auto cur_cap = calc_soft_cap() * mult;

    int64_t against = 0;

    switch(race->getSoftType(this)) {
        case dbat::race::Fixed:
            switch(type) {
                case 0:
                    against = (this->getBasePL());
                    break;
                case 1:
                    against = (this->getBaseKI());
                    break;
                case 2:
                    against = (this->getBaseST());
                    break;
            }
            break;
        case dbat::race::Variable:
            against = (this->getBasePL()) + (this->getBaseKI()) + (this->getBaseST());
            if(IS_ANDROID(this) && type > 0) {
                cur_cap += type;
            }
            break;
    }
    return against >= cur_cap;
}

int char_data::wearing_android_canister() const {
    if(!IS_ANDROID(this))
        return 0;
    auto obj = GET_EQ(this, WEAR_BACKPACK);
    if(!obj)
        return 0;
    switch(GET_OBJ_VNUM(obj)) {
        case 1806:
            return 1;
        case 1807:
            return 2;
        default:
            return 0;
    }
}

int char_data::calcGravCost(int64_t num) {
    int cost = 0;
    if(!this->can_tolerate_gravity(ROOM_GRAVITY(IN_ROOM(this))))
        cost = ROOM_GRAVITY(IN_ROOM(this)) ^ 2;

    if (!num) {
        if(cost) {
            send_to_char(this, "You sweat bullets straining against the current gravity.\r\n");
        }
        if ((this->getCurST()) > cost) {
            this->decCurST(cost);
            return 1;
        }
        else {
            this->decCurST(cost);
            return 0;
        }
    }
    else {
        return (this->getCurST()) > (cost + num);
    }
}


int64_t char_data::getCurHealth() const {
    return getCurPL();
}

int64_t char_data::getMaxHealth() const {
    return getMaxPL();
}

double char_data::getCurHealthPercent() const {
    return getCurPLPercent();
}

int64_t char_data::getPercentOfCurHealth(double amt) const {
    return getPercentOfCurPL(amt);
}

int64_t char_data::getPercentOfMaxHealth(double amt) const {
    return getPercentOfMaxPL(amt);
}

bool char_data::isFullHealth() const {
    return isFullPL();
}

int64_t char_data::setCurHealth(int64_t amt) {
    hit = std::max(0L, std::abs(amt));
    return hit;
}

int64_t char_data::setCurHealthPercent(double amt) {
    hit = std::max(0L, (int64_t)(getMaxHealth() * std::abs(amt)));
    return hit;
}

int64_t char_data::incCurHealth(int64_t amt, bool limit_max) {
    if(limit_max)
        health = std::min(1.0, health+(double)std::abs(amt) / (double)getEffMaxPL());
    else
        health += (double)std::abs(amt) / (double)getEffMaxPL();
    return getCurHealth();
};

int64_t char_data::decCurHealth(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getEffMaxPL();
    health = std::max(fl, health-(double)std::abs(amt) / (double)getEffMaxPL());
    return getCurHealth();
}

int64_t char_data::incCurHealthPercent(double amt, bool limit_max) {
    if(limit_max)
        health = std::min(1.0, health+std::abs(amt));
    else
        health += std::abs(amt);
    return getCurHealth();
}

int64_t char_data::decCurHealthPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getEffMaxPL();
    health = std::max(fl, health-std::abs(amt));
    return getCurHealth();
}

void char_data::restoreHealth(bool announce) {
    if(!isFullHealth()) health = 1;
}

int64_t char_data::getMaxPLTrans() const {
    auto form = race->getCurForm(this);
    int64_t total = 0;
    if(form.flag) {
        total = (form.bonus + getEffBasePL()) * form.mult;
    } else {
        total = getEffBasePL() * form.mult;
    }
    return total;
}

int64_t char_data::getMaxPL() const {
    auto total = getMaxPLTrans();
    if(GET_KAIOKEN(this) > 0) {
        total += (total / 10) * GET_KAIOKEN(this);
    }
    if(AFF_FLAGGED(this, AFF_METAMORPH)) {
        total += (total * .6);
    }
    return total;
}

int64_t char_data::getCurPL() const {
    if(suppression > 0){
        return getEffMaxPL() * std::min(health, (double)suppression/100);
    } else {
        return getEffMaxPL() * health;
    }
}

int64_t char_data::getEffBasePL() const {
    if(original) return original->getEffBasePL();
    if(clones) {
        return getBasePL() / (clones + 1);
    } else {
        return getBasePL();
    }
}

int64_t char_data::getBasePL() const {
    return basepl;
}

double char_data::getCurPLPercent() const {
    return (double)getCurPL() / (double)getMaxPL();
}

int64_t char_data::getPercentOfCurPL(double amt) const {
    return getCurPL() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxPL(double amt) const {
    return getMaxPL() * std::abs(amt);
}

bool char_data::isFullPL() const {
    return health >= 1.0;
}

int64_t char_data::getCurKI() const {
    return getMaxKI() * energy;
}

int64_t char_data::getMaxKI() const {
    auto form = race->getCurForm(this);
    if(form.flag) {
        return (form.bonus + getEffBaseKI()) * form.mult;
    } else {
        return getEffBaseKI();
    }
}

int64_t char_data::getEffBaseKI() const {
    if(original) return original->getEffBaseKI();
    if(clones) {
        return getBaseKI() / (clones + 1);
    } else {
        return getBaseKI();
    }
}

int64_t char_data::getBaseKI() const {
    return baseki;
}

double char_data::getCurKIPercent() const {
    return (double)getCurKI() / (double)getMaxKI();
}

int64_t char_data::getPercentOfCurKI(double amt) const {
    return getCurKI() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxKI(double amt) const {
    return getMaxKI() * std::abs(amt);
}

bool char_data::isFullKI() const {
    return energy >= 1.0;
}

int64_t char_data::setCurKI(int64_t amt) {
    mana = std::max(0L, std::abs(amt));
    return mana;
}

int64_t char_data::setCurKIPercent(double amt) {
    mana = std::max(0L, (int64_t)(getMaxKI() * std::abs(amt)));
    return mana;
}

int64_t char_data::incCurKI(int64_t amt, bool limit_max) {
    if(limit_max)
        energy = std::min(1.0, energy+(double)std::abs(amt) / (double)getMaxKI());
    else
        energy += (double)std::abs(amt) / (double)getMaxKI();
    return getCurKI();
};

int64_t char_data::decCurKI(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxKI();
    energy = std::max(fl, energy-(double)std::abs(amt) / (double)getMaxKI());
    return getCurKI();
}

int64_t char_data::incCurKIPercent(double amt, bool limit_max) {
    if(limit_max)
        energy = std::min(1.0, energy+std::abs(amt));
    else
        energy += std::abs(amt);
    return getCurKI();
}

int64_t char_data::decCurKIPercent(double amt, int64_t floor) {
    if(!strcasecmp(this->name, "Wayland")) {
        send_to_char(this, "decCurKIPercent called with: %f\r\n", amt);
    }
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxKI();
    energy = std::max(fl, energy-std::abs(amt));
    return getCurKI();
}


void char_data::restoreKI(bool announce) {
    if(!isFullKI()) energy = 1;
}

int64_t char_data::getCurST() const {
    return getMaxST() * stamina;
}

int64_t char_data::getMaxST() const {
    auto form = race->getCurForm(this);
    if(form.flag) {
        return (form.bonus + getEffBaseST()) * form.mult;
    } else {
        return getEffBaseST();
    }
}

int64_t char_data::getEffBaseST() const {
    if(original) return original->getEffBaseST();
    if(clones) {
        return getBaseST() / (clones + 1);
    } else {
        return getBaseST();
    }
}

int64_t char_data::getBaseST() const {
    return basest;
}

double char_data::getCurSTPercent() const {
    return (double)getCurST() / (double)getMaxST();
}

int64_t char_data::getPercentOfCurST(double amt) const {
    return getCurST() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxST(double amt) const {
    return getMaxST() * std::abs(amt);
}

bool char_data::isFullST() const {
    return stamina >= 1;
}

int64_t char_data::setCurST(int64_t amt) {
    move = std::max(0L, std::abs(amt));
    return move;
}

int64_t char_data::setCurSTPercent(double amt) {
    move = std::max(0L, (int64_t)(getMaxST() * std::abs(amt)));
    return move;
}

int64_t char_data::incCurST(int64_t amt, bool limit_max) {
    if(limit_max)
        stamina = std::min(1.0, stamina+(double)std::abs(amt) / (double)getMaxST());
    else
        stamina += (double)std::abs(amt) / (double)getMaxST();
    return getCurST();
};

int64_t char_data::decCurST(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxST();
    stamina = std::max(fl, stamina-(double)std::abs(amt) / (double)getMaxST());
    return getCurST();
}

int64_t char_data::incCurSTPercent(double amt, bool limit_max) {
    if(limit_max)
        stamina = std::min(1.0, stamina+std::abs(amt));
    else
        stamina += std::abs(amt);
    return getMaxST();
}

int64_t char_data::decCurSTPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxST();
    stamina = std::max(fl, stamina-std::abs(amt));
    return getCurST();
}


void char_data::restoreST(bool announce) {
    if(!isFullST()) stamina = 1;
}


int64_t char_data::getCurLF() const {
    return getMaxLF() * life;
}

int64_t char_data::getMaxLF() const {
    return (IS_DEMON(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.75) + GET_LIFEBONUSES(this) : (IS_KONATSU(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.85) + GET_LIFEBONUSES(this) : (GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5) + GET_LIFEBONUSES(this)));
}

double char_data::getCurLFPercent() const {
    return life;
}

int64_t char_data::getPercentOfCurLF(double amt) const {
    return getCurLF() * std::abs(amt);
}

int64_t char_data::getPercentOfMaxLF(double amt) const {
    return getMaxLF() * std::abs(amt);
}

bool char_data::isFullLF() const {
    return life >= 1.0;
}

int64_t char_data::setCurLF(int64_t amt) {
    life = std::max(0L, std::abs(amt));
    return getCurLF();
}

int64_t char_data::setCurLFPercent(double amt) {
    life = std::max(0L, (int64_t)(getMaxLF() * std::abs(amt)));
    return getCurLF();
}

int64_t char_data::incCurLF(int64_t amt, bool limit_max) {
    if(limit_max)
        life = std::min(1.0, stamina+(double)std::abs(amt) / (double)getMaxLF());
    else
        life += (double)std::abs(amt) / (double)getMaxLF();
    return getCurLF();
};

int64_t char_data::decCurLF(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxLF();
    life = std::max(fl, life-(double)std::abs(amt) / (double)getMaxLF());
    return getCurLF();
}

int64_t char_data::incCurLFPercent(double amt, bool limit_max) {
    if(limit_max)
        life = std::min(1.0, life+std::abs(amt));
    else
        life += std::abs(amt);
    return getCurLF();
}

int64_t char_data::decCurLFPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxLF();
    life = std::max(fl, life-std::abs(amt));
    return getCurLF();
}


void char_data::restoreLF(bool announce) {
    if(!isFullLF()) life = 1;
}


bool char_data::isFullVitals() const {
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
    SET_BIT_AR(AFF_FLAGS(this), AFF_KNOCKED);
    if (AFF_FLAGGED(this, AFF_FLYING)) {
        REMOVE_BIT_AR(AFF_FLAGS(this), AFF_FLYING);
        GET_ALT(this) = 0;
    }
    GET_POS(this) = POS_SLEEPING;
}

void char_data::cureStatusKnockedOut(bool announce) {
    if (AFF_FLAGGED(this, AFF_KNOCKED)) {
        if(announce) {
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

        REMOVE_BIT_AR(AFF_FLAGS(this), AFF_KNOCKED);
        GET_POS(this) = POS_SITTING;
    }
}

void char_data::cureStatusBurn(bool announce) {
    if (AFF_FLAGGED(this, AFF_BURNED)) {
        if(announce) {
            send_to_char(this, "Your burns are healed now.\r\n");
            ::act("$n@w's burns are now healed.@n", true, this, nullptr, nullptr, TO_ROOM);
        }
        REMOVE_BIT_AR(AFF_FLAGS(this), AFF_BURNED);
    }
}

void char_data::cureStatusPoison(bool announce) {
    ::act("@C$n@W suddenly looks a lot better!@b", false, this, nullptr, nullptr, TO_NOTVICT);
    affect_from_char(this, SPELL_POISON);
}

static const std::map<int, std::string> limb_names = {
        {1, "right arm"},
        {2, "left arm"},
        {3, "right leg"},
        {4, "left leg"}
};

void char_data::restoreLimbs(bool announce) {
    // restore head...
    GET_LIMBCOND(this, 0) = 100;

    // limbs...
    for(const auto& l : limb_names) {
        if(announce) {
            if(GET_LIMBCOND(this, l.first) <= 0)
                send_to_char(this, "Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(this, l.first) < 50)
                send_to_char(this, "Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(this, l.first) = 100;
    }

    // and lastly, tail.
    this->race->gainTail(this, announce);
}

int64_t char_data::gainBasePL(int64_t amt, bool trans_mult) {
    basepl += amt;
    return basepl;
}

int64_t char_data::gainBaseST(int64_t amt, bool trans_mult) {
    basest += amt;
    return basest;
}

int64_t char_data::gainBaseKI(int64_t amt, bool trans_mult) {
    baseki += amt;
    return baseki;
}

void char_data::gainBaseAll(int64_t amt, bool trans_mult) {
    gainBasePL(amt, trans_mult);
    gainBaseKI(amt, trans_mult);
    gainBaseST(amt, trans_mult);
}

int64_t char_data::loseBasePL(int64_t amt, bool trans_mult) {
    basepl = std::max(1L, basepl-amt);
    return basepl;
}

int64_t char_data::loseBaseST(int64_t amt, bool trans_mult) {
    basest = std::max(1L, basest-amt);
    return basest;
}

int64_t char_data::loseBaseKI(int64_t amt, bool trans_mult) {
    baseki = std::max(1L, baseki-amt);
    return baseki;
}

void char_data::loseBaseAll(int64_t amt, bool trans_mult) {
    loseBasePL(amt, trans_mult);
    loseBaseKI(amt, trans_mult);
    loseBaseST(amt, trans_mult);
}

int64_t char_data::gainBasePLPercent(double amt, bool trans_mult) {
    return gainBasePL(basepl * amt, trans_mult);
}

int64_t char_data::gainBaseKIPercent(double amt, bool trans_mult) {
    return gainBaseKI(baseki * amt, trans_mult);
}

int64_t char_data::gainBaseSTPercent(double amt, bool trans_mult) {
    return gainBaseST(basest * amt, trans_mult);
}

int64_t char_data::loseBasePLPercent(double amt, bool trans_mult) {
    return loseBasePL(basepl * amt, trans_mult);
}

int64_t char_data::loseBaseKIPercent(double amt, bool trans_mult) {
    return loseBaseKI(baseki * amt, trans_mult);
}

int64_t char_data::loseBaseSTPercent(double amt, bool trans_mult) {
    return loseBaseST(basest * amt, trans_mult);
}

void char_data::gainBaseAllPercent(double amt, bool trans_mult) {
    gainBasePLPercent(amt, trans_mult);
    gainBaseKIPercent(amt, trans_mult);
    gainBaseSTPercent(amt, trans_mult);
}

void char_data::loseBaseAllPercent(double amt, bool trans_mult) {
    loseBasePLPercent(amt, trans_mult);
    loseBaseKIPercent(amt, trans_mult);
    loseBaseSTPercent(amt, trans_mult);
}


int64_t char_data::getMaxCarryWeight() const {
    return std::max(1L, (getMaxPL() / 200) + (GET_STR(this) * 50));
}

int64_t char_data::getCurGearWeight() const {
    int64_t total_weight = 0;

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(this, i)) {
            total_weight += GET_OBJ_WEIGHT(GET_EQ(this, i));
        }
    }
    return total_weight;
}

int64_t char_data::getCurCarriedWeight() const {
    return getCurGearWeight() + carry_weight;
}

int64_t char_data::getAvailableCarryWeight() const {
    return getMaxCarryWeight() - getCurCarriedWeight();
}

double char_data::speednar() const {
    auto ratio = (double)getCurCarriedWeight() / (double)getMaxCarryWeight();
    if(ratio >= .05)
        return std::max(0.01,std::min(1.0, 1.0-ratio));
    return 1.0;
}

int64_t char_data::getEffMaxPL() const {
    if(IS_NPC(this)) {
        return getMaxPL();
    }
    return getMaxPL() * speednar();
}

bool char_data::isWeightedPL() const {
    return getMaxPL() > getEffMaxPL();
}

void char_data::apply_kaioken(int times, bool announce) {
    GET_KAIOKEN(this) = times;
    REMOVE_BIT_AR(PLR_FLAGS(this), PLR_POWERUP);

    if(announce) {
        send_to_char(this, "@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        ::act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, this, nullptr, nullptr, TO_ROOM);
    }

}

void char_data::remove_kaioken(int8_t announce) {
    auto kaio = GET_KAIOKEN(this);
    if(!kaio) {
        return;
    }
    GET_KAIOKEN(this) = 0;

    switch(announce) {
        case 1:
            send_to_char(this, "You drop out of kaioken.\r\n");
            ::act("$n@w drops out of kaioken.@n", true, this, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            send_to_char(this, "You lose focus and your kaioken disappears.\r\n");
            ::act("$n loses focus and $s kaioken aura disappears.", true, this, nullptr, nullptr, TO_ROOM);
    }
}