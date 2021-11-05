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
    this->restore();

    ::act("You have been fully healed by $N!", FALSE, this, 0, ch, TO_CHAR | TO_SLEEP);
    if (GET_SUPPRESS(this) > 0) {
        send_to_char(this, "@mYou are healed to your suppression limit.@n\r\n");
    }
}

void char_data::restore() {
    GET_HIT(this) = gear_pl(this);
    GET_MANA(this) = GET_MAX_MANA(this);
    GET_MOVE(this) = GET_MAX_MOVE(this);
    GET_KI(this) = GET_MAX_KI(this);
    if (GET_SUPPRESS(this) > 0 && GET_HIT(this) > (((gear_pl(this)) / 100) * GET_SUPPRESS(this))) {
        GET_HIT(this) = (((gear_pl(this)) / 100) * GET_SUPPRESS(this));
    }

    null_affect(this, AFF_POISON);
    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_BLIND);
    REMOVE_BIT_AR(AFF_FLAGS(this), AFF_FROZEN);
    GET_LIMBCOND(this, 1) = 100;
    GET_LIMBCOND(this, 2) = 100;
    GET_LIMBCOND(this, 3) = 100;
    GET_LIMBCOND(this, 4) = 100;
    SET_BIT_AR(PLR_FLAGS(this), PLR_HEAD);

    if (AFF_FLAGGED(this, AFF_KNOCKED)) {
        ::act("@W$n is no longer senseless, and wakes up.@n", FALSE, this, 0, 0, TO_ROOM);
        send_to_char(this, "You are no longer knocked out, and wake up!@n\r\n");
        REMOVE_BIT_AR(AFF_FLAGS(this), AFF_KNOCKED);
        GET_POS(this) = POS_SITTING;
    }

    update_pos(this);
    affect_total(this);
}

void char_data::resurrect(ResurrectionMode mode) {
    // First, fully heal the character.
    restore();
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
    ::act("$n's body forms in a pool of @Bblue light@n.", TRUE, this, nullptr, nullptr, TO_ROOM);
}

void char_data::ghostify() {
    restore();
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
                    against = GET_BASE_PL(this);
                    break;
                case 1:
                    against = GET_BASE_KI(this);
                    break;
                case 2:
                    against = GET_BASE_ST(this);
                    break;
            }
            break;
        case dbat::race::Variable:
            against = GET_BASE_PL(this) + GET_BASE_KI(this) + GET_BASE_ST(this);
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
        if (GET_MOVE(this) > cost) {
            GET_MOVE(this) -= cost;
            return 1;
        }
        else {
            GET_MOVE(this) -= GET_MOVE(this) - 1;
            return 0;
        }
    }
    else {
        return GET_MOVE(this) > (cost + num);
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
        hit = std::min(getMaxHealth(), hit+std::abs(amt));
    else
        hit += std::abs(amt);
    return hit;
};

int64_t char_data::decCurHealth(int64_t amt, int64_t floor) {
    hit = std::max(floor, hit-std::abs(amt));
    return hit;
}

int64_t char_data::incCurHealthPercent(double amt, bool limit_max) {
    return incCurHealth((int64_t)(getMaxHealth() * std::abs(amt)), limit_max);
}

int64_t char_data::decCurHealthPercent(double amt, int64_t floor) {
    return decCurHealth((int64_t)getMaxHealth() * std::abs(amt), floor);
}

void char_data::restoreHealth(bool announce) {
    if(!isFullHealth()) hit = getMaxHealth();
}

int64_t char_data::getMaxPL() const {
    return max_hit;
}

int64_t char_data::getCurPL() const {
    return hit;
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

void char_data::refreshSuppress() {
    if (GET_SUPPRESS(this) > 0 && GET_HIT(this) > ((GET_MAX_HIT(this) / 100) * GET_SUPPRESS(this))) {
        GET_HIT(this) = ((GET_MAX_HIT(this) / 100) * GET_SUPPRESS(this));
        send_to_char(this, "@mYou are healed to your suppression limit.@n\r\n");
    }
}

void char_data::transformPL(int64_t amt) {
    hit += std::abs(amt);
    max_hit += std::abs(amt);
}

void char_data::revertPL(int64_t amt) {
    hit = std::max(1L, hit-std::abs(amt));
    max_hit = std::max(1L, max_hit-std::abs(amt));
}

bool char_data::isFullPL() const {
    return getCurPL() >= getMaxPL();
}

int64_t char_data::getCurKI() const {
    return mana;
}

int64_t char_data::getMaxKI() const {
    return max_mana;
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
    return getCurKI() >= getMaxKI();
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
        mana = std::min(getMaxKI(), mana+std::abs(amt));
    else
        mana += std::abs(amt);
    return mana;
};

int64_t char_data::decCurKI(int64_t amt, int64_t floor) {
    mana = std::max(floor, mana-std::abs(amt));
    return mana;
}

int64_t char_data::incCurKIPercent(double amt, bool limit_max) {
    return incCurKI((int64_t)(getMaxKI() * std::abs(amt)), limit_max);
}

int64_t char_data::decCurKIPercent(double amt, int64_t floor) {
    return decCurKI((int64_t)getMaxKI() * std::abs(amt), floor);
}

void char_data::transformKI(int64_t amt) {
    mana += std::abs(amt);
    max_mana += std::abs(amt);
}

void char_data::revertKI(int64_t amt) {
    mana = std::max(1L, mana-std::abs(amt));
    max_mana = std::max(1L, max_mana-std::abs(amt));
}

void char_data::restoreKI(bool announce) {
    if(!isFullKI()) mana = getMaxKI();
}

int64_t char_data::getCurST() const {
    return move;
}

int64_t char_data::getMaxST() const {
    return max_move;
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
    return getCurST() >= getMaxST();
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
        move = std::min(getMaxST(), move+std::abs(amt));
    else
        move += std::abs(amt);
    return move;
};

int64_t char_data::decCurST(int64_t amt, int64_t floor) {
    move = std::max(floor, move-std::abs(amt));
    return move;
}

int64_t char_data::incCurSTPercent(double amt, bool limit_max) {
    return incCurST((int64_t)(getMaxST() * std::abs(amt)), limit_max);
}

int64_t char_data::decCurSTPercent(double amt, int64_t floor) {
    return decCurST((int64_t)getMaxST() * std::abs(amt), floor);
}

void char_data::transformST(int64_t amt) {
    move += std::abs(amt);
    max_move += std::abs(amt);
}

void char_data::revertST(int64_t amt) {
    move = std::max(1L, move-std::abs(amt));
    max_move = std::max(1L, max_move-std::abs(amt));
}

void char_data::restoreST(bool announce) {
    if(!isFullST()) mana = getMaxST();
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
            ::act("@W$n@W is no longer senseless, and wakes up.@n", FALSE, this, 0, 0, TO_ROOM);
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
            ::act("$n@w's burns are now healed.@n", TRUE, this, 0, 0, TO_ROOM);
        }
        REMOVE_BIT_AR(AFF_FLAGS(this), AFF_BURNED);
    }
}

void char_data::cureStatusPoison(bool announce) {
    ::act("@C$n@W suddenly looks a lot better!@b", FALSE, this, 0, 0, TO_NOTVICT);
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
    auto mult = trans_mult ? this->race->getCurFormMult(this) : 1;
    auto to_add = (int64_t)(amt * mult);
    max_hit += to_add;
    basepl += amt;
    return basepl;
}

int64_t char_data::gainBaseST(int64_t amt, bool trans_mult) {
    auto mult = trans_mult ? this->race->getCurFormMult(this) : 1;
    auto to_add = (int64_t)(amt * mult);
    max_move += to_add;
    basest += amt;
    return basest;
}

int64_t char_data::gainBaseKI(int64_t amt, bool trans_mult) {
    auto mult = trans_mult ? this->race->getCurFormMult(this) : 1;
    auto to_add = (int64_t)(amt * mult);
    max_mana += to_add;
    baseki += amt;
    return baseki;
}

void char_data::gainBaseAll(int64_t amt, bool trans_mult) {
    gainBasePL(amt, trans_mult);
    gainBaseKI(amt, trans_mult);
    gainBaseST(amt, trans_mult);
}

int64_t char_data::loseBasePL(int64_t amt, bool trans_mult) {
    auto mult = trans_mult ? this->race->getCurFormMult(this) : 1;
    auto to_lose = (int64_t)(amt * mult);
    max_hit = std::max(1L, max_hit-to_lose);
    basepl = std::max(1L, basepl-amt);
    return basepl;
}

int64_t char_data::loseBaseST(int64_t amt, bool trans_mult) {
    auto mult = trans_mult ? this->race->getCurFormMult(this) : 1;
    auto to_lose = (int64_t)(amt * mult);
    max_move = std::max(1L, max_move-to_lose);
    basest = std::max(1L, basest-amt);
    return basest;
}

int64_t char_data::loseBaseKI(int64_t amt, bool trans_mult) {
    auto mult = trans_mult ? this->race->getCurFormMult(this) : 1;
    auto to_lose = (int64_t)(amt * mult);
    max_mana = std::max(1L, max_mana-to_lose);
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