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