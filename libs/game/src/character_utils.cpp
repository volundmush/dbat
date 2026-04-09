#include "dbat/game/character_utils.h"
#include "dbat/game/structs.h"
#include "dbat/game/races.h"
#include "dbat/game/utils.h"
#include "dbat/game/spells.h"
#include "dbat/game/comm.h"
#include "dbat/game/class.h"
#include "dbat/game/fight.h"
#include "dbat/game/act.movement.h"

static const std::string robot = "Robotic-Humanoid";
static const std::string robot_lower = "robotic-humanoid";
static const std::string unknown = "UNKNOWN";

const std::string &juggleRaceName(char_data *ch, bool capitalized) {
    if(!ch->race) return unknown;

    dbat::race::Race *apparent = ch->race;

    switch(apparent->getID()) {
        case dbat::race::hoshijin:
            if(ch->mimic) apparent = ch->mimic;
            break;
        case dbat::race::halfbreed:
            switch(RACIAL_PREF(ch)) {
                case 1:
                    apparent = dbat::race::race_map[dbat::race::human];
                    break;
                case 2:
                    apparent = dbat::race::race_map[dbat::race::saiyan];
                    break;
            }
            break;
        case dbat::race::android:
            switch(RACIAL_PREF(ch)) {
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
            if(PLR_FLAGGED(ch, PLR_TAILHIDE)) {
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

void restore_by(char_data *ch, char_data *healer) {
    restore(ch, true);
    act("You have been fully healed by $N!", FALSE, ch, 0, healer, TO_CHAR | TO_SLEEP);
}

void restore(char_data *ch, bool announce) {
    restoreVitals(ch, announce);
    restoreLimbs(ch, announce);
    restoreStatus(ch, announce);
    restoreLF(ch, announce);
}

void resurrect(char_data *ch, ResurrectionMode mode) {
    restore(ch, true);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_PDEATH);
    char_from_room(ch);
    if (GET_DROOM(ch) != NOWHERE && GET_DROOM(ch) != 0 && GET_DROOM(ch) != 1) {
        char_to_room(ch, real_room(GET_DROOM(ch)));
    }
    else {
        char_to_room(ch, real_room(ch->chclass->senseiStartRoom()));
    }
    look_at_room(IN_ROOM(ch), ch, 0);

    int dur = 100;
    switch(mode) {
        case Costless:
            return;
        case Basic:
            if (ch->dcount >= 8 && ch->dcount < 10) {
                dur = 90;
            } else if (ch->dcount >= 5 && ch->dcount < 8) {
                dur = 75;
            } else if (ch->dcount >= 3 && ch->dcount < 5) {
                dur = 60;
            } else if (ch->dcount >= 1 && ch->dcount < 3) {
                dur = 40;
            }
            break;
        case RPP:
            dur = 100;
            break;
    }

    if (GET_LEVEL(ch) > 9) {
        int losschance = axion_dice(0);
        send_to_char(ch, "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength, constitution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        int str = -8, intel = -8, wis = -8, spd = -8, con = -8, agl = -8;
        if (ch->real_abils.intel <= 16) {
            intel = -4;
        }
        if (ch->real_abils.cha <= 16) {
            spd = -4;
        }
        if (ch->real_abils.dex <= 16) {
            agl = -4;
        }
        if (ch->real_abils.wis <= 16) {
            wis = -4;
        }
        if (ch->real_abils.con <= 16) {
            con = -4;
        }
        assign_affect(ch, AFF_WEAKENED_STATE, SKILL_WARP, dur, str, con, intel, agl, wis, spd);
        if (losschance >= 100) {
            int psloss = rand_number(100, 300);
            GET_PRACTICES(ch, GET_CLASS(ch)) -= psloss;
            send_to_char(ch, "@R...and a loss of @r%d@R PS!@n", psloss);
            if (GET_PRACTICES(ch, GET_CLASS(ch)) < 0) {
                GET_PRACTICES(ch, GET_CLASS(ch)) = 0;
            }
        }
    }
    GET_DTIME(ch) = 0;
    act("$n's body forms in a pool of @Bblue light@n.", TRUE, ch, nullptr, nullptr, TO_ROOM);
}

void ghostify(char_data *ch) {
    restore(ch, true);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);

    if (!PRF_FLAGGED(ch, PRF_LKEEP)) {
        if (PLR_FLAGGED(ch, PLR_CLLEG)) {
            REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CLLEG);
        }
        if (PLR_FLAGGED(ch, PLR_CRLEG)) {
            REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRLEG);
        }
        if (PLR_FLAGGED(ch, PLR_CRARM)) {
            REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRARM);
        }
        if (PLR_FLAGGED(ch, PLR_CLARM)) {
            REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CLARM);
        }
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_KNOCKED);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SLEEP);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PARALYZE);
}

void teleport_to(char_data *ch, IDXTYPE rnum) {
    char_from_room(ch);
    char_to_room(ch, real_room(rnum));
    look_at_room(IN_ROOM(ch), ch, 0);
    update_pos(ch);
}

bool in_room_range(char_data *ch, IDXTYPE low_rnum, IDXTYPE high_rnum) {
    return GET_ROOM_VNUM(IN_ROOM(ch)) >= low_rnum && GET_ROOM_VNUM(IN_ROOM(ch)) <= high_rnum;
}

bool in_past(char_data *ch) {
    return ROOM_FLAGGED(IN_ROOM(ch), ROOM_PAST);
}

bool is_newbie(char_data *ch) {
    return GET_LEVEL(ch) < 9;
}

bool in_northran(char_data *ch) {
    return in_room_range(ch, 17900, 17999);
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

bool can_tolerate_gravity(char_data *ch, int grav) {
    if(IS_NPC(ch)) return true;
    int tolerance = 0;
    tolerance = std::max(tolerance, ch->chclass->getGravTolerance());
    if(tolerance >= grav)
        return true;
    return getMaxPL(ch) >= grav_threshold[grav];
}

int calcTier(char_data *ch) {
    int tier = ch->level / 10;
    if((ch->level % 10) == 0)
        tier--;
    tier = std::max(tier, 0);
    tier = std::min(tier, 9);
    return tier;
}

int64_t calc_soft_cap(char_data *ch) {
    auto tier = calcTier(ch);
    auto softmap = ch->race->getSoftMap(ch);
    return ch->level * softmap[tier];
}

bool is_soft_cap(char_data *ch, int64_t type) {
    return is_soft_cap(ch, type, 1.0);
}

bool is_soft_cap(char_data *ch, int64_t type, long double mult) {
    if(IS_NPC(ch))
        return true;

    if(ch->level >= 100) {
        return false;
    }
    auto cur_cap = calc_soft_cap(ch) * mult;

    int64_t against = 0;

    switch(ch->race->getSoftType(ch)) {
        case dbat::race::Fixed:
            switch(type) {
                case 0:
                    against = (getBasePL(ch));
                    break;
                case 1:
                    against = (getBaseKI(ch));
                    break;
                case 2:
                    against = (getBaseST(ch));
                    break;
            }
            break;
        case dbat::race::Variable:
            against = (getBasePL(ch)) + (getBaseKI(ch)) + (getBaseST(ch));
            if(IS_ANDROID(ch) && type > 0) {
                cur_cap += type;
            }
            break;
    }
    return against >= cur_cap;
}

int wearing_android_canister(char_data *ch) {
    if(!IS_ANDROID(ch))
        return 0;
    auto obj = GET_EQ(ch, WEAR_BACKPACK);
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

int calcGravCost(char_data *ch, int64_t num) {
    int cost = 0;
    if(!can_tolerate_gravity(ch, ROOM_GRAVITY(IN_ROOM(ch))))
        cost = ROOM_GRAVITY(IN_ROOM(ch)) ^ 2;

    if (!num) {
        if(cost) {
            send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
        }
        if ((getCurST(ch)) > cost) {
            decCurST(ch, cost);
            return 1;
        }
        else {
            decCurST(ch, cost);
            return 0;
        }
    }
    else {
        return (getCurST(ch)) > (cost + num);
    }
}

int64_t getCurHealth(char_data *ch) {
    return getCurPL(ch);
}

int64_t getMaxHealth(char_data *ch) {
    return getMaxPL(ch);
}

double getCurHealthPercent(char_data *ch) {
    return getCurPLPercent(ch);
}

int64_t getPercentOfCurHealth(char_data *ch, double amt) {
    return getPercentOfCurPL(ch, amt);
}

int64_t getPercentOfMaxHealth(char_data *ch, double amt) {
    return getPercentOfMaxPL(ch, amt);
}

bool isFullHealth(char_data *ch) {
    return isFullPL(ch);
}

int64_t setCurHealth(char_data *ch, int64_t amt) {
    ch->hit = std::max(0L, std::abs(amt));
    return ch->hit;
}

int64_t setCurHealthPercent(char_data *ch, double amt) {
    ch->hit = std::max(0L, (int64_t)(getMaxHealth(ch) * std::abs(amt)));
    return ch->hit;
}

int64_t incCurHealth(char_data *ch, int64_t amt, bool limit_max) {
    if(limit_max)
        ch->health = std::min(1.0, ch->health+(double)std::abs(amt) / (double)getEffMaxPL(ch));
    else
        ch->health += (double)std::abs(amt) / (double)getEffMaxPL(ch);
    return getCurHealth(ch);
};

int64_t decCurHealth(char_data *ch, int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getEffMaxPL(ch);
    ch->health = std::max(fl, ch->health-(double)std::abs(amt) / (double)getEffMaxPL(ch));
    return getCurHealth(ch);
}

int64_t incCurHealthPercent(char_data *ch, double amt, bool limit_max) {
    if(limit_max)
        ch->health = std::min(1.0, ch->health+std::abs(amt));
    else
        ch->health += std::abs(amt);
    return getCurHealth(ch);
}

int64_t decCurHealthPercent(char_data *ch, double amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getEffMaxPL(ch);
    ch->health = std::max(fl, ch->health-std::abs(amt));
    return getCurHealth(ch);
}

void restoreHealth(char_data *ch, bool announce) {
    if(!isFullHealth(ch)) ch->health = 1;
}

int64_t healCurHealth(char_data *ch, int64_t amt) {
    return incCurHealth(ch, amt, true);
}

int64_t harmCurHealth(char_data *ch, int64_t amt) {
    return decCurHealth(ch, amt, 0);
}

int64_t getMaxPLTrans(char_data *ch) {
    auto form = ch->race->getCurForm(ch);
    int64_t total = 0;
    if(form.flag) {
        total = (form.bonus + getEffBasePL(ch)) * form.mult;
    } else {
        total = getEffBasePL(ch) * form.mult;
    }
    return total;
}

int64_t getMaxPL(char_data *ch) {
    auto total = getMaxPLTrans(ch);
    if(GET_KAIOKEN(ch) > 0) {
        total += (total / 10) * GET_KAIOKEN(ch);
    }
    if(AFF_FLAGGED(ch, AFF_METAMORPH)) {
        total += (total * .6);
    }
    return total;
}

int64_t getCurPL(char_data *ch) {
    if(ch->suppression > 0){
        return getEffMaxPL(ch) * std::min(ch->health, (double)ch->suppression/100);
    } else {
        return getEffMaxPL(ch) * ch->health;
    }
}

int64_t getEffBasePL(char_data *ch) {
    if(ch->original) return getEffBasePL(ch->original);
    if(ch->clones) {
        return getBasePL(ch) / (ch->clones + 1);
    } else {
        return getBasePL(ch);
    }
}

int64_t getBasePL(char_data *ch) {
    return ch->basepl;
}

double getCurPLPercent(char_data *ch) {
    return (double)getCurPL(ch) / (double)getMaxPL(ch);
}

int64_t getPercentOfCurPL(char_data *ch, double amt) {
    return getCurPL(ch) * std::abs(amt);
}

int64_t getPercentOfMaxPL(char_data *ch, double amt) {
    return getMaxPL(ch) * std::abs(amt);
}

bool isFullPL(char_data *ch) {
    return ch->health >= 1.0;
}

int64_t getCurKI(char_data *ch) {
    return getMaxKI(ch) * ch->energy;
}

int64_t getMaxKI(char_data *ch) {
    auto form = ch->race->getCurForm(ch);
    if(form.flag) {
        return (form.bonus + getEffBaseKI(ch)) * form.mult;
    } else {
        return getEffBaseKI(ch);
    }
}

int64_t getEffBaseKI(char_data *ch) {
    if(ch->original) return getEffBaseKI(ch->original);
    if(ch->clones) {
        return getBaseKI(ch) / (ch->clones + 1);
    } else {
        return getBaseKI(ch);
    }
}

int64_t getBaseKI(char_data *ch) {
    return ch->baseki;
}

double getCurKIPercent(char_data *ch) {
    return (double)getCurKI(ch) / (double)getMaxKI(ch);
}

int64_t getPercentOfCurKI(char_data *ch, double amt) {
    return getCurKI(ch) * std::abs(amt);
}

int64_t getPercentOfMaxKI(char_data *ch, double amt) {
    return getMaxKI(ch) * std::abs(amt);
}

bool isFullKI(char_data *ch) {
    return ch->energy >= 1.0;
}

int64_t setCurKI(char_data *ch, int64_t amt) {
    ch->mana = std::max(0L, std::abs(amt));
    return ch->mana;
}

int64_t setCurKIPercent(char_data *ch, double amt) {
    ch->mana = std::max(0L, (int64_t)(getMaxKI(ch) * std::abs(amt)));
    return ch->mana;
}

int64_t incCurKI(char_data *ch, int64_t amt, bool limit_max) {
    if(limit_max)
        ch->energy = std::min(1.0, ch->energy+(double)std::abs(amt) / (double)getMaxKI(ch));
    else
        ch->energy += (double)std::abs(amt) / (double)getMaxKI(ch);
    return getCurKI(ch);
};

int64_t decCurKI(char_data *ch, int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxKI(ch);
    ch->energy = std::max(fl, ch->energy-(double)std::abs(amt) / (double)getMaxKI(ch));
    return getCurKI(ch);
}

int64_t incCurKIPercent(char_data *ch, double amt, bool limit_max) {
    if(limit_max)
        ch->energy = std::min(1.0, ch->energy+std::abs(amt));
    else
        ch->energy += std::abs(amt);
    return getCurKI(ch);
}

int64_t decCurKIPercent(char_data *ch, double amt, int64_t floor) {
    if(!strcasecmp(ch->name, "Wayland")) {
        send_to_char(ch, "decCurKIPercent called with: %f\r\n", amt);
    }
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxKI(ch);
    ch->energy = std::max(fl, ch->energy-std::abs(amt));
    return getCurKI(ch);
}

void restoreKI(char_data *ch, bool announce) {
    if(!isFullKI(ch)) ch->energy = 1;
}

int64_t getCurST(char_data *ch) {
    return getMaxST(ch) * ch->stamina;
}

int64_t getMaxST(char_data *ch) {
    auto form = ch->race->getCurForm(ch);
    if(form.flag) {
        return (form.bonus + getEffBaseST(ch)) * form.mult;
    } else {
        return getEffBaseST(ch);
    }
}

int64_t getEffBaseST(char_data *ch) {
    if(ch->original) return getEffBaseST(ch->original);
    if(ch->clones) {
        return getBaseST(ch) / (ch->clones + 1);
    } else {
        return getBaseST(ch);
    }
}

int64_t getBaseST(char_data *ch) {
    return ch->basest;
}

double getCurSTPercent(char_data *ch) {
    return (double)getCurST(ch) / (double)getMaxST(ch);
}

int64_t getPercentOfCurST(char_data *ch, double amt) {
    return getCurST(ch) * std::abs(amt);
}

int64_t getPercentOfMaxST(char_data *ch, double amt) {
    return getMaxST(ch) * std::abs(amt);
}

bool isFullST(char_data *ch) {
    return ch->stamina >= 1;
}

int64_t setCurST(char_data *ch, int64_t amt) {
    ch->move = std::max(0L, std::abs(amt));
    return ch->move;
}

int64_t setCurSTPercent(char_data *ch, double amt) {
    ch->move = std::max(0L, (int64_t)(getMaxST(ch) * std::abs(amt)));
    return ch->move;
}

int64_t incCurST(char_data *ch, int64_t amt, bool limit_max) {
    if(limit_max)
        ch->stamina = std::min(1.0, ch->stamina+(double)std::abs(amt) / (double)getMaxST(ch));
    else
        ch->stamina += (double)std::abs(amt) / (double)getMaxST(ch);
    return getCurST(ch);
};

int64_t decCurST(char_data *ch, int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxST(ch);
    ch->stamina = std::max(fl, ch->stamina-(double)std::abs(amt) / (double)getMaxST(ch));
    return getCurST(ch);
}

int64_t incCurSTPercent(char_data *ch, double amt, bool limit_max) {
    if(limit_max)
        ch->stamina = std::min(1.0, ch->stamina+std::abs(amt));
    else
        ch->stamina += std::abs(amt);
    return getMaxST(ch);
}

int64_t decCurSTPercent(char_data *ch, double amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxST(ch);
    ch->stamina = std::max(fl, ch->stamina-std::abs(amt));
    return getCurST(ch);
}

void restoreST(char_data *ch, bool announce) {
    if(!isFullST(ch)) ch->stamina = 1;
}

int64_t getCurLF(char_data *ch) {
    return getMaxLF(ch) * ch->life;
}

int64_t getMaxLF(char_data *ch) {
    return (IS_DEMON(ch) ? (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.75) + GET_LIFEBONUSES(ch) : (IS_KONATSU(ch) ? (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.85) + GET_LIFEBONUSES(ch) : (GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5) + GET_LIFEBONUSES(ch)));
}

double getCurLFPercent(char_data *ch) {
    return ch->life;
}

int64_t getPercentOfCurLF(char_data *ch, double amt) {
    return getCurLF(ch) * std::abs(amt);
}

int64_t getPercentOfMaxLF(char_data *ch, double amt) {
    return getMaxLF(ch) * std::abs(amt);
}

bool isFullLF(char_data *ch) {
    return ch->life >= 1.0;
}

int64_t setCurLF(char_data *ch, int64_t amt) {
    ch->life = std::max(0L, std::abs(amt));
    return getCurLF(ch);
}

int64_t setCurLFPercent(char_data *ch, double amt) {
    ch->life = std::max(0L, (int64_t)(getMaxLF(ch) * std::abs(amt)));
    return getCurLF(ch);
}

int64_t incCurLF(char_data *ch, int64_t amt, bool limit_max) {
    if(limit_max)
        ch->life = std::min(1.0, ch->stamina+(double)std::abs(amt) / (double)getMaxLF(ch));
    else
        ch->life += (double)std::abs(amt) / (double)getMaxLF(ch);
    return getCurLF(ch);
};

int64_t decCurLF(char_data *ch, int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxLF(ch);
    ch->life = std::max(fl, ch->life-(double)std::abs(amt) / (double)getMaxLF(ch));
    return getCurLF(ch);
}

int64_t incCurLFPercent(char_data *ch, double amt, bool limit_max) {
    if(limit_max)
        ch->life = std::min(1.0, ch->life+std::abs(amt));
    else
        ch->life += std::abs(amt);
    return getCurLF(ch);
}

int64_t decCurLFPercent(char_data *ch, double amt, int64_t floor) {
    auto fl = 0.0;
    if(floor > 0)
        fl = (double)floor / (double)getMaxLF(ch);
    ch->life = std::max(fl, ch->life-std::abs(amt));
    return getCurLF(ch);
}

void restoreLF(char_data *ch, bool announce) {
    if(!isFullLF(ch)) ch->life = 1;
}

bool isFullVitals(char_data *ch) {
    return isFullHealth(ch) && isFullKI(ch) && isFullST(ch);
}

void restoreVitals(char_data *ch, bool announce) {
    restoreHealth(ch, announce);
    restoreKI(ch, announce);
    restoreST(ch, announce);
}

void restoreStatus(char_data *ch, bool announce) {
    cureStatusKnockedOut(ch, announce);
    cureStatusBurn(ch, announce);
    cureStatusPoison(ch, announce);
}

void setStatusKnockedOut(char_data *ch) {
    SET_BIT_AR(AFF_FLAGS(ch), AFF_KNOCKED);
    if (AFF_FLAGGED(ch, AFF_FLYING)) {
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
        GET_ALT(ch) = 0;
    }
    GET_POS(ch) = POS_SLEEPING;
}

void cureStatusKnockedOut(char_data *ch, bool announce) {
    if (AFF_FLAGGED(ch, AFF_KNOCKED)) {
        if(announce) {
            act("@W$n@W is no longer senseless, and wakes up.@n", FALSE, ch, 0, 0, TO_ROOM);
            send_to_char(ch, "You are no longer knocked out, and wake up!@n\r\n");
        }

        if (CARRIED_BY(ch)) {
            if (GET_ALIGNMENT(CARRIED_BY(ch)) > 50) {
                carry_drop(CARRIED_BY(ch), 0);
            } else {
                carry_drop(CARRIED_BY(ch), 1);
            }
        }

        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_KNOCKED);
        GET_POS(ch) = POS_SITTING;
    }
}

void cureStatusBurn(char_data *ch, bool announce) {
    if (AFF_FLAGGED(ch, AFF_BURNED)) {
        if(announce) {
            send_to_char(ch, "Your burns are healed now.\r\n");
            act("$n@w's burns are now healed.@n", TRUE, ch, 0, 0, TO_ROOM);
        }
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_BURNED);
    }
}

void cureStatusPoison(char_data *ch, bool announce) {
    act("@C$n@W suddenly looks a lot better!@b", FALSE, ch, 0, 0, TO_NOTVICT);
    affect_from_char(ch, SPELL_POISON);
}

static const std::map<int, std::string> limb_names = {
        {1, "right arm"},
        {2, "left arm"},
        {3, "right leg"},
        {4, "left leg"}
};

void restoreLimbs(char_data *ch, bool announce) {
    GET_LIMBCOND(ch, 0) = 100;

    for(const auto& l : limb_names) {
        if(announce) {
            if(GET_LIMBCOND(ch, l.first) <= 0)
                send_to_char(ch, "Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(ch, l.first) < 50)
                send_to_char(ch, "Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(ch, l.first) = 100;
    }

    ch->race->gainTail(ch, announce);
}

int64_t gainBasePL(char_data *ch, int64_t amt, bool trans_mult) {
    ch->basepl += amt;
    return ch->basepl;
}

int64_t gainBaseST(char_data *ch, int64_t amt, bool trans_mult) {
    ch->basest += amt;
    return ch->basest;
}

int64_t gainBaseKI(char_data *ch, int64_t amt, bool trans_mult) {
    ch->baseki += amt;
    return ch->baseki;
}

void gainBaseAll(char_data *ch, int64_t amt, bool trans_mult) {
    gainBasePL(ch, amt, trans_mult);
    gainBaseKI(ch, amt, trans_mult);
    gainBaseST(ch, amt, trans_mult);
}

int64_t loseBasePL(char_data *ch, int64_t amt, bool trans_mult) {
    ch->basepl = std::max(1L, ch->basepl-amt);
    return ch->basepl;
}

int64_t loseBaseST(char_data *ch, int64_t amt, bool trans_mult) {
    ch->basest = std::max(1L, ch->basest-amt);
    return ch->basest;
}

int64_t loseBaseKI(char_data *ch, int64_t amt, bool trans_mult) {
    ch->baseki = std::max(1L, ch->baseki-amt);
    return ch->baseki;
}

void loseBaseAll(char_data *ch, int64_t amt, bool trans_mult) {
    loseBasePL(ch, amt, trans_mult);
    loseBaseKI(ch, amt, trans_mult);
    loseBaseST(ch, amt, trans_mult);
}

int64_t gainBasePLPercent(char_data *ch, double amt, bool trans_mult) {
    return gainBasePL(ch, ch->basepl * amt, trans_mult);
}

int64_t gainBaseKIPercent(char_data *ch, double amt, bool trans_mult) {
    return gainBaseKI(ch, ch->baseki * amt, trans_mult);
}

int64_t gainBaseSTPercent(char_data *ch, double amt, bool trans_mult) {
    return gainBaseST(ch, ch->basest * amt, trans_mult);
}

int64_t loseBasePLPercent(char_data *ch, double amt, bool trans_mult) {
    return loseBasePL(ch, ch->basepl * amt, trans_mult);
}

int64_t loseBaseKIPercent(char_data *ch, double amt, bool trans_mult) {
    return loseBaseKI(ch, ch->baseki * amt, trans_mult);
}

int64_t loseBaseSTPercent(char_data *ch, double amt, bool trans_mult) {
    return loseBaseST(ch, ch->basest * amt, trans_mult);
}

void gainBaseAllPercent(char_data *ch, double amt, bool trans_mult) {
    gainBasePLPercent(ch, amt, trans_mult);
    gainBaseKIPercent(ch, amt, trans_mult);
    gainBaseSTPercent(ch, amt, trans_mult);
}

void loseBaseAllPercent(char_data *ch, double amt, bool trans_mult) {
    loseBasePLPercent(ch, amt, trans_mult);
    loseBaseKIPercent(ch, amt, trans_mult);
    loseBaseSTPercent(ch, amt, trans_mult);
}

int64_t getMaxCarryWeight(char_data *ch) {
    return std::max(1L, (getMaxPL(ch) / 200) + (GET_STR(ch) * 50));
}

int64_t getCurGearWeight(char_data *ch) {
    int64_t total_weight = 0;

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(ch, i)) {
            total_weight += GET_OBJ_WEIGHT(GET_EQ(ch, i));
        }
    }
    return total_weight;
}

int64_t getCurCarriedWeight(char_data *ch) {
    return getCurGearWeight(ch) + ch->carry_weight;
}

int64_t getAvailableCarryWeight(char_data *ch) {
    return getMaxCarryWeight(ch) - getCurCarriedWeight(ch);
}

// speednar is in utils.cpp

int64_t getEffMaxPL(char_data *ch) {
    if(IS_NPC(ch)) {
        return getMaxPL(ch);
    }
    return getMaxPL(ch) * speednar(ch);
}

bool isWeightedPL(char_data *ch) {
    return getMaxPL(ch) > getEffMaxPL(ch);
}

void apply_kaioken(char_data *ch, int times, bool announce) {
    GET_KAIOKEN(ch) = times;
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_POWERUP);

    if(announce) {
        send_to_char(ch, "@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
}

void remove_kaioken(char_data *ch, int8_t announce) {
    auto kaio = GET_KAIOKEN(ch);
    if(!kaio) {
        return;
    }
    GET_KAIOKEN(ch) = 0;

    switch(announce) {
        case 1:
            send_to_char(ch, "You drop out of kaioken.\r\n");
            act("$n@w drops out of kaioken.@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
        case 2:
            send_to_char(ch, "You lose focus and your kaioken disappears.\r\n");
            act("$n loses focus and $s kaioken aura disappears.", TRUE, ch, 0, 0, TO_ROOM);
    }
}