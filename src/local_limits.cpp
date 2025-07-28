/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/local_limits.h"
#include "dbat/send.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/dg_comm.h"
#include "dbat/act.other.h"
#include "dbat/act.item.h"
#include "dbat/vehicles.h"
#include "dbat/act.movement.h"
#include "dbat/constants.h"
#include "dbat/class.h"
#include "dbat/fight.h"

#include "dbat/handler.h"
#include "dbat/dg_scripts.h"

/* local defines */
constexpr int sick_fail = 2;

/* local functions */
static void heal_limb(struct char_data *ch);

static int64_t move_gain(struct char_data *ch);

static int64_t mana_gain(struct char_data *ch);

static int64_t hit_gain(struct char_data *ch);

static void update_flags(struct char_data *ch);

static int wearing_stardust(struct char_data *ch);

static void healthy_check(struct char_data *ch);

static void barrier_shed(struct char_data *ch);

static void check_idling(struct char_data *ch);

static void barrier_shed(struct char_data *ch) {

    if (!AFF_FLAGGED(ch, AFF_SANCTUARY)) {
        return;
    }

    if (GET_SKILL(ch, SKILL_AQUA_BARRIER) > 0) {
        return;
    }

    int chance = axion_dice(0), barrier = GET_SKILL(ch, SKILL_BARRIER), concentrate = GET_SKILL(ch,
                                                                                                SKILL_CONCENTRATION);
    double rate = 0.3;

    if (barrier >= 100) {
        rate = 0.01;
    } else if (barrier >= 95) {
        rate = 0.02;
    } else if (barrier >= 90) {
        rate = 0.04;
    } else if (barrier >= 80) {
        rate = 0.08;
    } else if (barrier >= 70) {
        rate = 0.10;
    } else if (barrier >= 60) {
        rate = 0.15;
    } else if (barrier >= 50) {
        rate = 0.20;
    } else if (barrier >= 40) {
        rate = 0.25;
    } else if (barrier >= 30) {
        rate = 0.27;
    } else if (barrier >= 20) {
        rate = 0.29;
    }

    int64_t loss = (long double) (GET_BARRIER(ch)) * rate, recharge = 0;

    if (concentrate >= chance) {
        recharge = loss * 0.5;
    }

    ch->modBaseStat<int64_t>("barrier", -loss);

    if (GET_BARRIER(ch) <= 0) {
        ch->setBaseStat<int64_t>("barrier", 0);
        act("@cYour barrier disappears.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@c$n@c's barrier disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else {
        act("@cYour barrier loses some energy.@n", true, ch, nullptr, nullptr, TO_CHAR);
        send_to_char(ch, "@D[@C%s@D]@n\r\n", add_commas(loss).c_str());
        act("@c$n@c's barrier sends some sparks into the air as it seems to get a bit weaker.@n", true, ch, nullptr,
            nullptr, TO_ROOM);
    }

    if (recharge > 0 && (ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch)) {
        ch->modCurVital(CharVital::ki, recharge);
        send_to_char(ch, "@CYou reabsorb some of the energy lost into your body!@n\r\n");
    }
}

/* If they have the Healthy trait then they have a chance to lose each of these */
static void healthy_check(struct char_data *ch) {

    if (!GET_BONUS(ch, BONUS_HEALTHY) || GET_POS(ch) != POS_SLEEPING) {
        return;
    }

    int chance = 70, roll = rand_number(1, 100), change = false;

    if (AFF_FLAGGED(ch, AFF_SHOCKED) && roll >= chance) {
        ch->affect_flags.set(AFF_SHOCKED, false);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_MBREAK) && roll >= chance) {
        ch->affect_flags.set(AFF_MBREAK, false);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_WITHER) && roll >= chance) {
        null_affect(ch, AFF_WITHER);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_CURSE) && roll >= chance) {
        ch->affect_flags.set(AFF_CURSE, false);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_POISON) && roll >= chance) {
        null_affect(ch, AFF_POISON);
        change = true;
    }
    if (IS_AFFECTED(ch, AFF_PARALYZE) && roll >= chance) {
        null_affect(ch, AFF_PARALYZE);
        change = true;
    }
    if (IS_AFFECTED(ch, AFF_PARA) && roll >= chance) {
        null_affect(ch, AFF_PARA);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_BLIND) && roll >= chance) {
        null_affect(ch, AFF_BLIND);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_HYDROZAP) && roll >= chance) {
        null_affect(ch, AFF_HYDROZAP);
        change = true;
    }
    if (AFF_FLAGGED(ch, AFF_KNOCKED) && roll >= chance) {
        ch->affect_flags.set(AFF_KNOCKED, false);
        ch->setBaseStat<int>("position", POS_SITTING);
        change = true;
    }
    if (change == true) {
        send_to_char(ch, "@CYou feel your body recover from all its ailments!@n\r\n");
    }
    return;
}

static int wearing_stardust(struct char_data *ch) {

    int count = 0, i;

    for (i = 1; i < NUM_WEARS; i++) {
        if (GET_EQ(ch, i)) {
            struct obj_data *obj = GET_EQ(ch, i);
            switch (GET_OBJ_VNUM(obj)) {
                case 1110:
                case 1111:
                case 1112:
                case 1113:
                case 1114:
                case 1115:
                case 1116:
                case 1117:
                case 1118:
                case 1119:
                    count += 1;
                    break;
            }
        }
    }

    if (count == 26)
        return (1);
    else
        return (0);

}

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
static int64_t mana_gain(struct char_data *ch) {
    int64_t gain = 0;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_MAX_MANA(ch) / 70;
    } else {
        if (ch->getRoomFlag(ROOM_REGEN) ||
            (GET_BONUS(ch, BONUS_DESTROYER) > 0 && ROOM_DAMAGE(IN_ROOM(ch)) >= 75)) {
            if (IS_KONATSU(ch)) {
                gain = GET_MAX_MANA(ch) / 12;
            }
            if (IS_MUTANT(ch)) {
                gain = GET_MAX_MANA(ch) / 11;
            }
            if (IS_ARLIAN(ch)) {
                gain = GET_MAX_MANA(ch) / 30;
            }
            if (!IS_KONATSU(ch) && !IS_MUTANT(ch)) {
                gain = GET_MAX_MANA(ch) / 10;
            }
        } else if (!ch->getRoomFlag(ROOM_REGEN)) {
            if (IS_KONATSU(ch)) {
                gain = GET_MAX_MANA(ch) / 15;
            }
            if (IS_MUTANT(ch)) {
                gain = GET_MAX_MANA(ch) / 13;
            }
            if (!IS_KONATSU(ch) && !IS_MUTANT(ch)) {
                gain = GET_MAX_MANA(ch) / 12;
            }
            if (ch->getRoomFlag(ROOM_BEDROOM)) {
                gain += gain * 0.25;
            }
            if (IS_ARLIAN(ch)) {
                gain = GET_MAX_MANA(ch) / 40;
            }
        }
        /* Position calculations    */
        switch (GET_POS(ch)) {
            case POS_STANDING:
                if (!IS_HOSHIJIN(ch) || (IS_HOSHIJIN(ch) && GET_PHASE(ch) <= 0)) {
                    gain = gain / 4;
                } else {
                    gain += (gain / 2);
                }
                break;
            case POS_FIGHTING:
                gain = gain / 4;
                break;
            case POS_SLEEPING:
                if (!SITS(ch)) {
                    gain *= 2;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090) {
                    gain *= 3;
                    gain += gain * 0.1;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19092) {
                    gain *= 3;
                    gain += gain * 0.3;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain *= 3;
                }
                break;
            case POS_RESTING:
                if (!SITS(ch)) {
                    gain += (gain / 2);
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090 && !IS_ARLIAN(ch)) {
                    gain *= 2;
                    gain += gain * 0.1;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19092 && !IS_ARLIAN(ch)) {
                    gain *= 2;
                    gain += gain * 0.3;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain *= 2;
                }
                break;
            case POS_SITTING:
                if (!SITS(ch)) {
                    gain += (gain / 4);
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090) {
                    gain += gain * 0.6;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19092) {
                    gain += gain * 0.8;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain += gain * 0.5;
                }
                break;
        }
    }

    if (IN_ROOM(ch) != NOWHERE) {
        if (cook_element(IN_ROOM(ch)) == 1) {
            gain += (gain * 0.2);
        }
    }

    if (IS_ARLIAN(ch) && IS_FEMALE(ch) && OUTSIDE(ch)) {
        gain *= 4;
    }

    if (IS_KANASSAN(ch) && weather_info.sky == SKY_RAINING && OUTSIDE(ch)) {
        gain += gain * 0.1;
    }
    if (IS_KANASSAN(ch) && ch->getLocationEnvironment(ENV_WATER) >= 100.0) {
        gain *= 16;
    }

    if (IS_HOSHIJIN(ch) && GET_PHASE(ch) > 0) {
        gain *= 2;
    }

    if (PLR_FLAGGED(ch, PLR_HEALT) && SITS(ch)) {
        gain *= 20;
    }
    if (AFF_FLAGGED(ch, AFF_POSE) && axion_dice(0) > GET_SKILL(ch, SKILL_POSE)) {
        null_affect(ch, AFF_POSE);
        send_to_char(ch, "You feel slightly less confident now.\r\n");
    }
    if (AFF_FLAGGED(ch, AFF_HYDROZAP) && rand_number(1, 4) >= 4) {
        null_affect(ch, AFF_HYDROZAP);
    }

    if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 100) {
        gain += gain / 2;
    } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 75) {
        gain += gain / 4;
    } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 50) {
        gain += gain / 6;
    } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 25) {
        gain += gain / 8;
    } else if (GET_SKILL(ch, SKILL_CONCENTRATION) < 25 && GET_SKILL(ch, SKILL_CONCENTRATION) > 0) {
        gain += gain / 10;
    }

    if (AFF_FLAGGED(ch, AFF_BLESS)) {
        gain *= 2;
    }
    if (AFF_FLAGGED(ch, AFF_CURSE)) {
        gain /= 5;
    }

    if (GET_FOODR(ch) > 0 && rand_number(1, 2) == 2) {
        ch->modBaseStat("food_rejuvenation", -1);
    }

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HINTS) && rand_number(1, 5) == 5) {
        hint_system(ch, 0);
    }

    if (AFF_FLAGGED(ch, AFF_POISON))
        gain /= 4;

    if (cook_element(IN_ROOM(ch)) == 1)
        gain *= 2;

    return (gain);
}

/* Hitpoint gain pr. game hour */
int64_t hit_gain(struct char_data *ch) {
    int64_t gain = 0;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_MAX_HIT(ch) / 70;
    } else {
        if (ch->getRoomFlag(ROOM_REGEN) || (GET_BONUS(ch, BONUS_DESTROYER) > 0 && ROOM_DAMAGE(IN_ROOM(ch)) >= 75)) {
            if (IS_HUMAN(ch)) {
                gain = GET_MAX_HIT(ch) / 20;
            }
            if (IS_ARLIAN(ch)) {
                gain = GET_MAX_HIT(ch) / 30;
            }
            if (IS_NAMEK(ch)) {
                gain = GET_MAX_HIT(ch) / 2;
            }
            if (IS_MUTANT(ch)) {
                gain = GET_MAX_HIT(ch) / 11;
            }
            if (!IS_HUMAN(ch) && !IS_NAMEK(ch) && !IS_MUTANT(ch)) {
                gain = GET_MAX_HIT(ch) / 10;
            }
        } else {
            if (IS_HUMAN(ch)) {
                gain = GET_MAX_HIT(ch) / 30;
            }
            if (IS_NAMEK(ch)) {
                gain = GET_MAX_HIT(ch) / 4;
            }
            if (IS_MUTANT(ch)) {
                gain = GET_MAX_HIT(ch) / 16;
            }
            if (IS_ARLIAN(ch)) {
                gain = GET_MAX_HIT(ch) / 40;
            }
            if (!IS_HUMAN(ch) && !IS_NAMEK(ch) && !IS_MUTANT(ch)) {
                gain = GET_MAX_HIT(ch) / 15;
            }
            if (ch->getRoomFlag(ROOM_BEDROOM)) {
                gain += gain * 0.25;
            }
        }

        /* Position calculations    */
        switch (GET_POS(ch)) {
            case POS_STANDING:
                if (IS_HOSHIJIN(ch) && GET_PHASE(ch) <= 0) {
                    gain = gain / 4;
                } else if (IS_ANDROID(ch) && ch->subrace == SubRace::android_model_absorb) {
                    gain = gain / 3;
                } else {
                    gain += (gain / 2);
                }
                break;
            case POS_FIGHTING:
                gain = gain / 4;
                break;
            case POS_SLEEPING:
                if (IS_ARLIAN(ch)) {
                    gain *= 3;
                } else if (!SITS(ch)) {
                    gain *= 2;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090) {
                    gain *= 3;
                    gain += gain * 0.1;
                } else if (SITS(ch)) {
                    gain *= 3;
                }
                break;
            case POS_RESTING:
                if (!SITS(ch)) {
                    gain += (gain / 2);
                } else if (IS_ANDROID(ch) && ch->subrace == SubRace::android_model_absorb) {
                    gain = gain * 1.5;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090 && !IS_ARLIAN(ch)) {
                    gain += gain * 1.1;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain *= 2;
                }
                break;
            case POS_SITTING:
                if (!SITS(ch)) {
                    gain += (gain / 4);
                } else if (IS_ANDROID(ch) && ch->subrace == SubRace::android_model_absorb) {
                    gain = gain * 0.5;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090 && !IS_ARLIAN(ch)) {
                    gain += gain * 0.6;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain += (gain * 0.5);
                }
        }
    }
    healthy_check(ch);

    if (IS_ARLIAN(ch) && IS_FEMALE(ch) && OUTSIDE(ch)) {
        gain *= 4;
    }

    if (IS_KANASSAN(ch) && weather_info.sky == SKY_RAINING && OUTSIDE(ch)) {
        gain += gain * 0.1;
    }
    if (IS_KANASSAN(ch) && ch->getLocationEnvironment(ENV_WATER) >= 100.0) {
        gain *= 16;
    }

    if (IS_HOSHIJIN(ch) && GET_PHASE(ch) > 0) {
        gain *= 2;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT) && SITS(ch)) {
        gain *= 20;
    }

    if (AFF_FLAGGED(ch, AFF_BLESS)) {
        gain *= 2;
    }
    if (AFF_FLAGGED(ch, AFF_CURSE)) {
        gain /= 5;
    }

    /* Fury Mode Loss for halfbreeds */

    if (PLR_FLAGGED(ch, PLR_FURY)) {
        send_to_char(ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
        ch->player_flags.set(PLR_FURY, false);
    }

    /* Fury Mode Loss for halfbreeds */

    if (AFF_FLAGGED(ch, AFF_POISON))
        gain /= 4;
    if (cook_element(IN_ROOM(ch)) == 1)
        gain *= 2;

    if (ch->subrace == SubRace::android_model_absorb) {
        gain = gain / 8;
    }

    if (auto reg = GET_REGEN(ch); reg > 0) {
        gain += (gain * 0.01) * reg;
    }

    return (gain);
}

/* move gain pr. game hour */
static int64_t move_gain(struct char_data *ch) {
    int64_t gain = 0;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_MAX_MOVE(ch) / 70;
    } else {
        if (ch->getRoomFlag(ROOM_REGEN) ||
            (GET_BONUS(ch, BONUS_DESTROYER) > 0 && ROOM_DAMAGE(IN_ROOM(ch)) >= 75)) {
            if (IS_MUTANT(ch)) {
                gain = GET_MAX_MOVE(ch) / 7;
            }
            if (IS_ARLIAN(ch)) {
                gain = GET_MAX_MOVE(ch) / 4;
            }
            if (!IS_MUTANT(ch)) {
                gain = GET_MAX_MOVE(ch) / 6;
            }
        } else if (!ch->getRoomFlag(ROOM_REGEN)) {
            if (IS_MUTANT(ch)) {
                gain = GET_MAX_MOVE(ch) / 9;
            }
            if (!IS_MUTANT(ch)) {
                gain = GET_MAX_MOVE(ch) / 8;
            }
            if (ch->getRoomFlag(ROOM_BEDROOM)) {
                gain += gain * 0.25;
            }
        }

        /* Position calculations    */
        switch (GET_POS(ch)) {
            case POS_STANDING:
                if (!IS_HOSHIJIN(ch) || (IS_HOSHIJIN(ch) && GET_PHASE(ch) <= 0)) {
                    gain = gain / 4;
                } else {
                    gain += (gain / 2);
                }
                break;
            case POS_FIGHTING:
                gain = gain / 4;
                break;
            case POS_SLEEPING:
                if (!SITS(ch)) {
                    gain *= 2;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090 && !IS_ARLIAN(ch)) {
                    gain *= 3;
                    gain += gain * 0.1;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19091 && !IS_ARLIAN(ch)) {
                    gain *= 3;
                    gain += gain * 0.3;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain *= 3;
                }
                break;
            case POS_RESTING:
                if (!SITS(ch)) {
                    gain += (gain / 2);
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090 && !IS_ARLIAN(ch)) {
                    gain += gain * 1.1;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19091 && !IS_ARLIAN(ch)) {
                    gain += gain * 1.3;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain += gain;
                }
                break;
            case POS_SITTING:
                if (!SITS(ch)) {
                    gain += (gain / 4);
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19090 && !IS_ARLIAN(ch)) {
                    gain += gain * 0.6;
                } else if (GET_OBJ_VNUM(SITS(ch)) == 19091 && !IS_ARLIAN(ch)) {
                    gain += gain * 0.8;
                } else if (SITS(ch) || IS_ARLIAN(ch)) {
                    gain += (gain / 2);
                }
        }
    }

    if (IS_ARLIAN(ch) && IS_FEMALE(ch) && OUTSIDE(ch)) {
        gain *= 2;
    }

    if (IS_NAMEK(ch)) {
        gain = gain * 0.5;
    }

    if (IS_KANASSAN(ch) && weather_info.sky == SKY_RAINING && OUTSIDE(ch)) {
        gain += gain * 0.1;
    }
    if (IS_KANASSAN(ch) && ch->getLocationEnvironment(ENV_WATER) >= 100.0) {
        gain *= 16;
    }

    if (IS_HOSHIJIN(ch) && GET_PHASE(ch) > 0) {
        gain *= 2;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT) && SITS(ch)) {
        gain *= 20;
    }

    if (AFF_FLAGGED(ch, AFF_BLESS)) {
        gain *= 2;
    }
    if (AFF_FLAGGED(ch, AFF_CURSE)) {
        gain /= 5;
    }

    if (AFF_FLAGGED(ch, AFF_POISON))
        gain /= 4;

    if (ch->getRoomFlag(ROOM_AURA)) {
        gain = GET_MAX_MOVE(ch) - (ch->getCurVital(CharVital::stamina));
    }

    if (cook_element(IN_ROOM(ch)) == 1)
        gain *= 2;

    if (auto reg = GET_REGEN(ch); reg > 0) {
        gain += (gain * 0.01) * reg;
    }

    return (gain);
}

static void update_flags(struct char_data *ch) {
    if (ch == nullptr) {
        send_to_imm("ERROR: Empty ch variable sent to update_flags.");
        return;
    }

    if (GET_BONUS(ch, BONUS_LATE) && GET_POS(ch) == POS_SLEEPING && rand_number(1, 3) == 3) {
        if (GET_HIT(ch) >= (ch->getEffectiveStat("health")) && (ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch) &&
            (ch->getCurVital(CharVital::ki)) >= GET_MAX_MANA(ch)) {
            send_to_char(ch, "You FINALLY wake up.\r\n");
            act("$n wakes up.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->setBaseStat<int>("position", POS_SITTING);
        }
    }

    if (AFF_FLAGGED(ch, AFF_KNOCKED) && !FIGHTING(ch)) {
        ch->cureStatusKnockedOut(true);
    }

    barrier_shed(ch);

    if (AFF_FLAGGED(ch, AFF_FIRESHIELD) && !FIGHTING(ch) && rand_number(1, 101) > GET_SKILL(ch, SKILL_FIRESHIELD)) {
        send_to_char(ch, "Your fireshield disappears.\r\n");
        ch->affect_flags.set(AFF_FIRESHIELD, false);
    }
    if (AFF_FLAGGED(ch, AFF_ZANZOKEN) && !FIGHTING(ch) && rand_number(1, 3) == 2) {
        send_to_char(ch, "You lose concentration and no longer are ready to zanzoken.\r\n");
        ch->affect_flags.set(AFF_ZANZOKEN, false);
    }
    if (AFF_FLAGGED(ch, AFF_ENSNARED) && rand_number(1, 3) == 2) {
        send_to_char(ch, "The silk ensnaring your arms disolves enough for you to break it!\r\n");
        ch->affect_flags.set(AFF_ENSNARED, false);
    }

    if ((IS_SAIYAN(ch) || IS_HALFBREED(ch)) && (ch->form == Form::super_saiyan_1) && !PLR_FLAGGED(ch, PLR_FPSSJ)) {
        ch->modBaseStat<int>("absorbs", 1);
        if (GET_ABSORBS(ch) >= 300) {
            send_to_char(ch,
                         "You have mastered the base Super Saiyan transformation and achieved Full Power Super Saiyan! Super Saiyan First can now be maintained effortlessly.\r\n");
            ch->player_flags.set(PLR_FPSSJ, true);
            ch->setBaseStat<int>("absorbs", 0);
        }
    }

    if(race::hasTail(ch->race) && !ch->character_flags.get(CharacterFlag::tail) && !PLR_FLAGGED(ch, PLR_NOGROW)) {
        if(auto tg = ch->modBaseStat<int>("tail_growth", 1); tg >= 10) {
            ch->gainTail(true);
            ch->setBaseStat<int>("tail_growth", 0);
        }
    }

    if (AFF_FLAGGED(ch, AFF_MBREAK) && rand_number(1, 3 + sick_fail) == 2) {
        send_to_char(ch, "@wYour mind is no longer in turmoil, you can charge ki again.@n\r\n");
        ch->affect_flags.set(AFF_MBREAK, false);
        if (GET_SKILL(ch, SKILL_TELEPATHY) <= 0) {
            bool condition1 = rand_number(1, 2) == 2;
            bool condition2 = rand_number(1, 20) == 1;
            if (condition1 || condition2) {
                send_to_char(ch, "@RYour senses are still a little addled... (-2 Int and Wis for 6 game hours.)@n\r\n");
                assign_affect(ch, AFF_MBREAK_DEBUFF, 0, 6, 0, 0, -2, 0, -2, 0);
            }
        }
    }
    if (AFF_FLAGGED(ch, AFF_SHOCKED) && rand_number(1, 4) == 4) {
        send_to_char(ch, "@wYour mind is no longer shocked.@n\r\n");
        if (GET_SKILL(ch, SKILL_TELEPATHY) > 0) {
            int skill = GET_SKILL(ch, SKILL_TELEPATHY), stop = false;
            improve_skill(ch, SKILL_TELEPATHY, 0);
            while (stop == false) {
                if (rand_number(1, 8) == 5)
                    stop = true;
                else
                    improve_skill(ch, SKILL_TELEPATHY, 0);
            }
            if (skill < GET_SKILL(ch, SKILL_TELEPATHY))
                send_to_char(ch, "Your mental damage and recovery has taught you things about your own mind.\r\n");
        }
        ch->affect_flags.set(AFF_SHOCKED, false);
    }
    if (AFF_FLAGGED(ch, AFF_FROZEN) && rand_number(1, 2) == 2) {
        send_to_char(ch, "@wYou realize you have thawed enough and break out of the ice holding you prisoner!\r\n");
        act("$n@W breaks out of the ice holding $m prisoner!", true, ch, nullptr, nullptr, TO_ROOM);
        ch->affect_flags.set(AFF_FROZEN, false);
    }
    if (AFF_FLAGGED(ch, AFF_WITHER) && rand_number(1, 6 + sick_fail) == 2) {
        send_to_char(ch, "@wYour body returns to normal and you beat the withering that plagued you.\r\n");
        act("$n@W's looks more fit now.", true, ch, nullptr, nullptr, TO_ROOM);
        null_affect(ch, AFF_WITHER);
    }
    if (wearing_stardust(ch) == 1) {
        ch->affect_flags.set(AFF_ZANZOKEN, true);
        send_to_char(ch, "The stardust armor blesses you with a free zanzoken when you next need it.\r\n");
    }

}


void set_title(struct char_data *ch, char *title) {
    if (ch) {
        send_to_char(ch,
                     "Title is disabled for the time being while Iovan works on a brand new and fancier title system.\r\n");
        return;
    }
    /*
     if (title == nullptr) {
       title = class_desc_str(ch, 2, TRUE);
     }

     if (strlen(title) > MAX_TITLE_LENGTH)
       title[MAX_TITLE_LENGTH] = '\0';

     if (GET_TITLE(ch))
       free(GET_TITLE(ch));

     GET_TITLE(ch) = strdup(title);
    */
}

void gain_level(struct char_data *ch) {
    send_to_char(ch, "Levelling no longer exists!\r\n");
    /*
    if (GET_LEVEL(ch) < 100 && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
        ch->modBaseStat<int>("Level", 1);
        advance_level(ch);
        mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true, "%s advanced level to level %d.",
               GET_NAME(ch), GET_LEVEL(ch));
        send_to_char(ch, "You rise a level!\r\n");
        ch->modExperience(-level_exp(ch, GET_LEVEL(ch)));
    }
    */
}

void run_autowiz() {

}

void gain_condition(struct char_data *ch, int condition, int value) {
    bool intoxicated;

    //Set-ups for when you cannot gain sustenance
    if (IS_NPC(ch))
        return;
    else if (IS_ANDROID(ch)) {
        return;
    } else if (GET_COND(ch, condition) < 0) {    /* No change */
        return;
    } else if (ch->getWhereFlag(WhereFlag::afterlife_hell)) {
        return;
    } else if (ch->getRoomFlag(ROOM_HELL)) {
        return;
    } else if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
        return;
    } else if (ch->getRoomVnum() <= 1) {
        return;
    }
    if (PLR_FLAGGED(ch, PLR_WRITING))
        return;

    else {
        intoxicated = (GET_COND(ch, DRUNK) > 0);

        //If there is a food value, restore that much hunger
        if (value > 0) {
            if (GET_COND(ch, condition) >= 0) {
                GET_COND(ch, condition) += value;
            }
        }

        //For food with a negative value we roll survival. On a success it cannot reduce the condition below 0
        if (!AFF_FLAGGED(ch, AFF_SPIRIT) &&
            (!GET_SKILL(ch, SKILL_SURVIVAL) || (GET_SKILL(ch, SKILL_SURVIVAL) < rand_number(1, 140)))) {
            if (value <= 0) {
                if (GET_COND(ch, condition) >= 0) {
                    if (GET_COND(ch, condition) + value < 0) {
                        GET_COND(ch, condition) = 0;
                    } else {
                        GET_COND(ch, condition) += value;
                    }
                }
            }
            //Send out hunger and thirst messages
            bool getsHungry = !(IS_NAMEK(ch));

            switch (condition) {
                case HUNGER:
                    if (getsHungry) {
                        switch (GET_COND(ch, condition)) {
                            case 0:
                                if ((ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch) / 3) {
                                    send_to_char(ch, "@RYou are starving to death!@n\r\n");
                                    ch->modCurVitalDam(CharVital::stamina, .33);
                                }
                                else if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 3) {
                                    send_to_char(ch, "@RYou are starving to death!@n\r\n");
                                    ch->modCurVitalDam(CharVital::stamina, 1);
                                    ch->modCurVitalDam(CharVital::health, .34);
                                }
                                break;
                            case 1:
                                send_to_char(ch, "You are extremely hungry!\r\n");
                                break;
                            case 2:
                                send_to_char(ch, "You are very hungry!\r\n");
                                break;
                            case 3:
                                send_to_char(ch, "You are pretty hungry!\r\n");
                                break;
                            case 4:
                                send_to_char(ch, "You are hungry!\r\n");
                                break;
                            case 5:
                            case 6:
                            case 7:
                            case 8:
                                send_to_char(ch, "Your stomach is growling!\r\n");
                                break;
                            case 9:
                            case 10:
                            case 11:
                                send_to_char(ch, "You could use something to eat.\r\n");
                                break;
                            case 12:
                            case 13:
                            case 14:
                            case 15:
                            case 16:
                            case 17:
                                send_to_char(ch, "You could use a bite to eat.\r\n");
                                break;
                            case 18:
                            case 19:
                            case 20:
                                send_to_char(ch, "You could use a snack.\r\n");
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case THIRST:
                    switch (GET_COND(ch, condition)) {
                        case 0:
                            if ((ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch) / 3) {
                                send_to_char(ch, "@RYou are dehydrated!@n\r\n");
                                ch->modCurVitalDam(CharVital::stamina, .33);
                            } else if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 3) {
                                send_to_char(ch, "@RYou are dehydrated!@n\r\n");
                                ch->modCurVitalDam(CharVital::stamina, 1);
                                ch->modCurVitalDam(CharVital::health, .34);
                            }
                            break;
                        case 1:
                            send_to_char(ch, "You are extremely thirsty!\r\n");
                            break;
                        case 2:
                            send_to_char(ch, "You are very thirsty!\r\n");
                            break;
                        case 3:
                            send_to_char(ch, "You are pretty thirsty!\r\n");
                            break;
                        case 4:
                            send_to_char(ch, "You are thirsty!\r\n");
                            break;
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                            send_to_char(ch, "Your throat is pretty dry!\r\n");
                            break;
                        case 9:
                        case 10:
                        case 11:
                            send_to_char(ch, "You could use something to drink.\r\n");
                            break;
                        case 12:
                        case 13:
                        case 14:
                        case 15:
                        case 16:
                        case 17:
                            send_to_char(ch, "Your mouth feels pretty dry.\r\n");
                            break;
                        case 18:
                        case 19:
                        case 20:
                            send_to_char(ch, "You could use a sip of water.\r\n");
                            break;
                        default:
                            break;
                    }
                    break;
                case DRUNK:
                    if (intoxicated) {
                        if (GET_COND(ch, DRUNK) <= 0) {
                            send_to_char(ch, "You are now sober.\r\n");
                        }
                    }
                    break;
                default:
                    break;
            }
            //If you starve or dehydrate, die and reset your conditions
            if (GET_HIT(ch) <= 0 && GET_COND(ch, HUNGER) == 0) {
                send_to_char(ch, "You have starved to death!\r\n");
                ch->modCurVitalDam(CharVital::stamina, 1);
                act("@W$n@W falls down dead before you...@n", false, ch, nullptr, nullptr, TO_ROOM);
                die(ch, nullptr);
                if (GET_COND(ch, HUNGER) != -1) {
                    GET_COND(ch, HUNGER) = 48;
                }
                if (GET_COND(ch, THIRST) != -1) {
                    GET_COND(ch, THIRST) = 48;
                }
            }
            if (GET_HIT(ch) <= 0 && GET_COND(ch, THIRST) == 0) {
                send_to_char(ch, "You have died of dehydration!\r\n");
                ch->modCurVitalDam(CharVital::stamina, 1);
                act("@W$n@W falls down dead before you...@n", false, ch, nullptr, nullptr, TO_ROOM);
                die(ch, nullptr);
                if (GET_COND(ch, HUNGER) != -1) {
                    GET_COND(ch, HUNGER) = 48;
                }
                GET_COND(ch, THIRST) = 48;
            }
        }
    }
}

static void check_idling(struct char_data *ch) {

    /*

    if (!dball_count(ch).empty()) {
        return;
    }
    if (++(ch->timer) > CONFIG_IDLE_VOID && IN_ROOM(ch) != 1) {
        if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
            GET_WAS_IN(ch) = IN_ROOM(ch);
            if (FIGHTING(ch)) {
                stop_fighting(FIGHTING(ch));
                stop_fighting(ch);
            }

            act("$n disappears into the void.", true, ch, nullptr, nullptr, TO_ROOM);
            send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
            char_from_room(ch);
            char_to_room(ch, 1);
        } else if (ch->timer > CONFIG_IDLE_RENT_TIME && IN_ROOM(ch) == 1) {
            send_to_char(ch, "You are idle and are extracted safely from the game.\r\n");
            mudlog(CMP, ADMLVL_GOD, true, "%s force-rented and extracted (idle).", GET_NAME(ch));
            extract_char(ch);
        }
    }

    */
}

static void heal_limb(struct char_data *ch) {
    int healrate = 0, recovered = false;

    if (PLR_FLAGGED(ch, PLR_BANDAGED)) {
        healrate += 10;
    }

    if (GET_POS(ch) == POS_SITTING) {
        healrate += 1;
    } else if (GET_POS(ch) == POS_RESTING) {
        healrate += 3;
    } else if (GET_POS(ch) == POS_SLEEPING) {
        healrate += 5;
    }

    if (healrate > 0) {
        if (GET_LIMBCOND(ch, 0) > 0 && GET_LIMBCOND(ch, 0) < 50) {
            if (GET_LIMBCOND(ch, 0) + healrate >= 50) {
                act("You realize your right arm is no longer broken.", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n starts moving $s right arm gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_LIMBCOND(ch, 0) += healrate;
                recovered = true;
            } else {
                GET_LIMBCOND(ch, 0) += healrate;
                send_to_char(ch, "Your right arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n",
                             GET_LIMBCOND(ch, 0), "%", "%");
            }
        } else if (GET_LIMBCOND(ch, 0) + healrate < 100) {
            GET_LIMBCOND(ch, 0) += healrate;
            send_to_char(ch, "Your right arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", GET_LIMBCOND(ch, 0),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 0) < 100 && GET_LIMBCOND(ch, 0) + healrate >= 100) {
            GET_LIMBCOND(ch, 0) = 100;
            send_to_char(ch, "Your right arm has fully recovered.\r\n");
        }

        if (GET_LIMBCOND(ch, 1) > 0 && GET_LIMBCOND(ch, 1) < 50) {
            if (GET_LIMBCOND(ch, 1) + healrate >= 50) {
                act("You realize your left arm is no longer broken.", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n starts moving $s left arm gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_LIMBCOND(ch, 1) += healrate;
                recovered = true;
            } else {
                GET_LIMBCOND(ch, 1) += healrate;
                send_to_char(ch, "Your left arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n",
                             GET_LIMBCOND(ch, 0), "%", "%");
            }
        } else if (GET_LIMBCOND(ch, 1) + healrate < 100) {
            GET_LIMBCOND(ch, 1) += healrate;
            send_to_char(ch, "Your left arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", GET_LIMBCOND(ch, 1),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 1) < 100 && GET_LIMBCOND(ch, 1) + healrate >= 100) {
            GET_LIMBCOND(ch, 1) = 100;
            send_to_char(ch, "Your left arm has fully recovered.\r\n");
        }

        if (GET_LIMBCOND(ch, 2) > 0 && GET_LIMBCOND(ch, 2) < 50) {
            if (GET_LIMBCOND(ch, 2) + healrate >= 50) {
                act("You realize your right leg is no longer broken.", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n starts moving $s right leg gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_LIMBCOND(ch, 2) += healrate;
                recovered = true;
            } else {
                GET_LIMBCOND(ch, 2) += healrate;
                send_to_char(ch, "Your right leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n",
                             GET_LIMBCOND(ch, 0), "%", "%");
            }
        } else if (GET_LIMBCOND(ch, 2) + healrate < 100) {
            GET_LIMBCOND(ch, 2) += healrate;
            send_to_char(ch, "Your right leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", GET_LIMBCOND(ch, 2),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 2) < 100 && GET_LIMBCOND(ch, 2) + healrate >= 100) {
            GET_LIMBCOND(ch, 2) = 100;
            send_to_char(ch, "Your right leg has fully recovered.\r\n");
        }

        if (GET_LIMBCOND(ch, 3) > 0 && GET_LIMBCOND(ch, 3) < 50) {
            if (GET_LIMBCOND(ch, 3) + healrate >= 50) {
                act("You realize your left leg is no longer broken.", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n starts moving $s left leg gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_LIMBCOND(ch, 3) += healrate;
                recovered = true;
            } else {
                GET_LIMBCOND(ch, 3) += healrate;
                send_to_char(ch, "Your left leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n",
                             GET_LIMBCOND(ch, 0), "%", "%");
            }
        } else if (GET_LIMBCOND(ch, 3) + healrate < 100) {
            GET_LIMBCOND(ch, 3) += healrate;
            send_to_char(ch, "Your left leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", GET_LIMBCOND(ch, 3),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 3) < 100 && GET_LIMBCOND(ch, 3) + healrate >= 100) {
            GET_LIMBCOND(ch, 3) = 100;
            send_to_char(ch, "Your left leg as fully recovered.\r\n");
        }

        if (!PLR_FLAGGED(ch, PLR_BANDAGED) && recovered == true) {
            if (axion_dice(-10) > GET_CON(ch)) {
                ch->modBaseStat("strength", -1);
                ch->modBaseStat("speed", -1);
                ch->modBaseStat("agility", -1);
                send_to_char(ch, "@RYou lose 1 Strength, Agility, and Speed!\r\n");
            }
        }
    }

    if (PLR_FLAGGED(ch, PLR_BANDAGED) && recovered == true) {
        ch->player_flags.set(PLR_BANDAGED, false);
        send_to_char(ch, "You remove your bandages.\r\n");
        return;
    }
}

void androidAbsorbSystem(uint64_t heartPulse, double deltaTime) {
    auto sub = characterSubscriptions.all("androidAbsorbSystem");
    for(auto ch : filter_raw(sub)) {
        
        bool unsubscribe = false;
        
        auto victim = ABSORBING(ch);
        
        if(!victim) {
            characterSubscriptions.unsubscribe("androidAbsorbSystem", ch);
        }
        
        if(IN_ROOM(ch) != IN_ROOM(victim)) {
            send_to_char(ch, "You stop absorbing %s!\r\n", GET_NAME(ABSORBING(ch)));
            ABSORBBY(ABSORBING(ch)) = nullptr;
            ABSORBING(ch) = nullptr;
            characterSubscriptions.unsubscribe("androidAbsorbSystem", ch);
            continue;
        }

        if (IS_ANDROID(ch) && victim) {
            if ((ch->absorbing)->getCurVital(CharVital::stamina) < (GET_MAX_MOVE(ch) / 15) && (ch->absorbing)->getCurVital(CharVital::ki) < (GET_MAX_MANA(ch) / 15)) {
                act("@WYou stop absorbing stamina and ki from @c$N as they don't have enough for you to take@W!@n",
                    true, ch, nullptr, victim, TO_CHAR);
                act("@C$n@W stops absorbing stamina and ki from you!@n", true, ch, nullptr,
                    victim, TO_VICT);
                act("@C$n@W stops absorbing stamina and ki from @c$N@w!@n", true, ch, nullptr,
                    victim, TO_NOTVICT);
                if (!FIGHTING(ch) || FIGHTING(ch) != victim) {
                    set_fighting(ch, ABSORBBY(victim));
                }
                if (!FIGHTING(ABSORBBY(victim)) ||
                    FIGHTING(ABSORBBY(victim)) != ch) {
                    set_fighting(ABSORBBY(victim), ch);
                }
                ABSORBBY(victim) = nullptr;
                ABSORBING(ch) = nullptr;
            }
        }
        if (IS_ANDROID(ch) && victim && rand_number(1, 9) >= 6) {
            if (((ch->absorbing)->getCurVital(CharVital::stamina)) > (GET_MAX_MOVE(ch) / 15) ||
                ((ch->absorbing)->getCurVital(CharVital::ki)) > (GET_MAX_MANA(ch) / 15)) {

                ch->modCurVital(CharVital::ki, ch->getEffectiveStat("ki") * .08);
                ch->modCurVital(CharVital::stamina, ch->getEffectiveStat("stamina") * .08);

                victim->modCurVital(CharVital::ki, -(ch->getEffectiveStat("ki") / 20));
                victim->modCurVital(CharVital::stamina, -(ch->getEffectiveStat("stamina") / 20));

                act("@WYou absorb stamina and ki from @c$N@W!@n", true, ch, nullptr, victim,
                    TO_CHAR);
                act("@C$n@W absorbs stamina and ki from you!@n", true, ch, nullptr, victim,
                    TO_VICT);
                send_to_char(victim, "@wTry 'escape'!@n\r\n");
                act("@C$n@W absorbs stamina and ki from @c$N@w!@n", true, ch, nullptr,
                    victim, TO_NOTVICT);
                if (GET_HIT(ch) < (ch->getEffectiveStat("health"))) {
                    ch->modCurVital(CharVital::health, ch->getEffectiveStat("ki") * .04);
                    send_to_char(ch,
                                 "@CYou convert a portion of the absorbed energy into refilling your powerlevel.@n\r\n");
                }

                if (ch->isFullVital(CharVital::stamina) && ch->isFullVital(CharVital::stamina)) {

                    act("@WYou stop absorbing stamina and ki from @c$N as you are full@W!@n", true, ch,
                        nullptr, victim, TO_CHAR);
                    act("@C$n@W stops absorbing stamina and ki from you!@n", true, ch, nullptr,
                        victim, TO_VICT);
                    act("@C$n@W stops absorbing stamina and ki from @c$N@w!@n", true, ch, nullptr,
                        victim, TO_NOTVICT);
                    if (!FIGHTING(ch) || FIGHTING(ch) != victim) {
                        set_fighting(ch, ABSORBBY(victim));
                    }
                    if (!FIGHTING(ABSORBBY(victim)) ||
                        FIGHTING(ABSORBBY(victim)) != ch) {
                        set_fighting(ABSORBBY(victim), ch);
                    }
                    ABSORBBY(victim) = nullptr;
                    ABSORBING(ch) = nullptr;
                }
                bool sum = !ch->is_soft_cap(0);
                bool mum = !ch->is_soft_cap(2);
                bool ium = !ch->is_soft_cap(1);
                auto leader = ch->master ? ch->master : ch;
                auto dCon = GET_CON(ch);
                auto dWis = GET_WIS(ch);
                if (sum) {
                    if (rand_number(1, 8) >= 6) {

                        int gain = rand_number(dCon / 2, dCon * 3) +
                                   (dCon * 18);
                        if (dCon > 30) {
                            gain += rand_number(dCon * 2, dCon * 4) +
                                    (dCon * 50);
                        }
                        if (dCon > 60) {
                            gain *= 2;
                        }
                        if (dCon > 80) {
                            gain *= 3;
                        }
                        if (dCon > 90) {
                            gain *= 4;
                        }
                        send_to_char(ch, "@gYou gain +@G%d@g permanent powerlevel!@n\r\n", gain);
                        if (group_bonus(ch, 2) == 7) {
                            if (leader->subrace == SubRace::android_model_sense) {
                                int gbonus = gain * 0.15;
                                gain += gbonus;
                                send_to_char(ch,
                                             "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n",
                                             add_commas(gbonus).c_str());
                            }
                        }
                        ch->gainBaseStat("health", gain);
                    }
                }
                if (mum) {
                    if (rand_number(1, 8) >= 6) {
                        int gain = rand_number(dCon / 2, dCon * 3) +
                                   (dCon * 18);
                        if (dCon > 30) {
                            gain += rand_number(dCon * 2, dCon * 4) +
                                    (dCon * 50);
                        }
                        if (dCon > 60) {
                            gain *= 2;
                        }
                        if (dCon > 80) {
                            gain *= 3;
                        }
                        if (dCon > 90) {
                            gain *= 4;
                        }
                        send_to_char(ch, "@gYou gain +@G%d@g permanent stamina!@n\r\n", gain);
                        if (group_bonus(ch, 2) == 7) {
                            if (leader->subrace == SubRace::android_model_sense) {
                                int gbonus = gain * 0.15;
                                gain += gbonus;
                                send_to_char(ch,
                                             "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n",
                                             add_commas(gbonus).c_str());
                            }
                        }
                        ch->gainBaseStat("stamina", gain);
                    }
                }
                if (ium) {
                    if (rand_number(1, 8) >= 6) {
                        int gain = rand_number(dWis / 2, dWis * 3) +
                                   (dWis * 18);
                        if (dWis > 30) {
                            gain += rand_number(dWis * 2, dWis * 4) +
                                    (dWis * 50);
                        }
                        if (dWis > 60) {
                            gain *= 2;
                        }
                        if (dWis > 80) {
                            gain *= 3;
                        }
                        if (dWis > 90) {
                            gain *= 4;
                        }
                        send_to_char(ch, "@gYou gain +@G%d@g permanent ki!@n\r\n", gain);
                        if (ch->master && group_bonus(ch, 2) == 7) {
                            if (leader->subrace == SubRace::android_model_sense) {
                                int gbonus = gain * 0.15;
                                gain += gbonus;
                                send_to_char(ch,
                                             "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n",
                                             add_commas(gbonus).c_str());
                            }
                        }
                        ch->gainBaseStat("ki", gain);
                    }
                }
                if (!sum) {
                    if (rand_number(1, 8) >= 6) {
                        int gain = 1;
                        send_to_char(ch,
                                     "@gYou gain +@G%d@g permanent health. You may need to level.@n\r\n", gain);
                        ch->gainBaseStat("health", gain);
                    }
                }
                if (!mum) {
                    if (rand_number(1, 8) >= 6) {
                        int gain = 1;
                        send_to_char(ch, "@gYou gain +@G%d@g permanent stamina. You may need to level.@n\r\n",
                                     gain);
                        ch->gainBaseStat("stamina", gain);
                    }
                }
                if (!ium) {
                    if (rand_number(1, 8) >= 6) {
                        int gain = 1;
                        send_to_char(ch, "@gYou gain +@G%d@g permanent ki. You may need to level.@n\r\n",
                                     gain);
                        ch->gainBaseStat("ki", gain);
                    }
                }
            }
        }

        if(!ABSORBING(ch)) {
            characterSubscriptions.unsubscribe("androidAbsorbSystem", ch);
        }

    }
}

void goopTimeService(uint64_t heartPulse, double deltaTime) {
    auto sub = characterSubscriptions.all("goopTimeService");
    for(auto ch : filter_raw(sub)) {

        if(!PLR_FLAGGED(ch, PLR_GOOP)) {
            characterSubscriptions.unsubscribe("goopTimeService", ch);
            continue;
        }

        if (ch->getBaseStat<int>("gooptime") == 60) {
            if (IS_BIO(ch)) {
                act("@GConciousness slowly returns to you. You realize quickly that some of your cells have survived. You take control of your regenerative processes and focus on growing a new body!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
            } else {
                act("@MSlowly you regain conciousness. The various split off chunks of your body begin to likewise stir.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@MYou think you notice the chunks of @m$n@M's moving slightly.@n", true, ch, nullptr,
                    nullptr, TO_ROOM);
            }
            ch->modBaseStat<int>("gooptime", -1);
        } else if (ch->getBaseStat<int>("gooptime") == 30) {
            if (IS_BIO(ch)) {
                act("@GFrom the collection of cells growing a crude form of your body starts to take shape!@n", true,
                    ch, nullptr, nullptr, TO_CHAR);
                act("@GYou start to notice a large mass of pulsing flesh growing before you!@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
            } else {
                act("@MYou will the various chunks of your body to return and slowly more and more of them begin to fly into you. Your body begins to grow larger and larger as this process unfolds!@n ",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@MThe various chunks of @m$n@M's body start to fly into the largest chunk! As the chunks collide they begin to form a larger and still growing blob of goo!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
            }
            ch->modBaseStat<int>("gooptime", -1);
        } else if (ch->getBaseStat<int>("gooptime") == 15) {
            if (IS_BIO(ch)) {
                act("@GYour body has almost reached its previous form! Only a little more regenerating is needed!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@GThe lump of flesh has now grown to the size where the likeness of @g$n@G can be seen of it! It appears that $e is regenerating $s body from what was only a few cells!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
            } else {
                act("@MYour body has reached half its previous size as your limbs ooze slowly out into their proper shape!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@m$n@M's body has regenerated to half its previous size! Slowly $s limbs ooze out into their proper shape! It won't be long now till $e has fully regenerated!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
            }
            ch->modBaseStat<int>("gooptime", -1);
        } else if (ch->getBaseStat<int>("gooptime") == 0) {
            if (IS_BIO(ch)) {
                ch->restoreHealth();
                act("@GYour body has fully regenerated! You flex your arms and legs outward with a rush of renewed strength!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@g$n@G's body has fully regenerated! Suddenly $e flexes $s arms and legs and a rush of power erupts from off of $s body!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
            }
                //Zenkai Boost
            else if (IS_SAIYAN(ch)) {

                int zenkaiPL, zenkaiKi, zenkaiSt;
                zenkaiPL = ch->getBaseStat("health") * 1.03;
                zenkaiKi = ch->getBaseStat("ki") * 1.015;
                zenkaiSt = ch->getBaseStat("stamina") * 1.015;

                //GET_HIT(ch) = gear_pl(ch) * .5;
                //GET_MANA(ch) = GET_MAX_MANA(ch) *.2;
                //GET_MOVE(ch) = GET_MAX_MOVE(ch) *.2;


                if (!IN_ARENA(ch)) {
                    ch->gainBaseStat("health", zenkaiPL);
                    ch->gainBaseStat("ki", zenkaiKi);
                    ch->gainBaseStat("stamina", zenkaiSt);

                    send_to_char(ch,
                                 "@D[@YZ@ye@wn@Wk@Ya@yi @YB@yo@wo@Ws@Yt@D] @WYou feel much stronger!\r\n");
                    send_to_char(ch, "@D[@RPL@Y:@n+%s@D] @D[@CKI@Y:@n+%s@D] @D[@GSTA@Y:@n+%s@D]@n\r\n",
                                 add_commas(zenkaiPL).c_str(), add_commas(zenkaiKi).c_str(), add_commas(zenkaiSt).c_str());
                }
                act("@RYou collapse to the ground, body pushed beyond the typical limits of exhaustion. The passage of time distorts and an indescribable amount of time passes as raw emotions pass through your very being. Your eyes open and focus with a newfound clarity as your unadulterated emotions and feelings revive you for a second wind!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@r$n@R collapses to the ground, seemingly dead. After a brief moment, their eyes flash open with a determined look on their face!",
                    true, ch, nullptr, nullptr, TO_ROOM);
            } else {
                ch->restoreHealth();
                act("@MYour body has fully regenerated! You scream out in triumph and a short gust of steam erupts from your pores!@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@m$n@M's body has fully regenerated! Suddenly $e screams out in gleeful triumph and short gust of steam erupts from $s skin pores!",
                    true, ch, nullptr, nullptr, TO_ROOM);
            }
            ch->player_flags.set(PLR_GOOP, false);
            characterSubscriptions.unsubscribe("goopTimeService", ch);
        } else {
            ch->modBaseStat<int>("gooptime", -1);
        }
    }
    
}

void corpseRotService(uint64_t heartPulse, double deltaTime) {
    obj_data *jj, *next_thing2;
    auto subs = objectSubscriptions.all("corpseRotService");
    for(auto j : filter_raw(subs)) {

        // how the fuck did this happen? TODO add a warning.
        if(!IS_CORPSE(j)) continue;

        /* timer count down */
        if (GET_OBJ_TIMER(j) > 0)
            j->modBaseStat("timer", -1);
        auto room = j->getRoom();
        if (!strstr(j->getName(), "android") && !strstr(j->getName(), "Android") && !OBJ_FLAGGED(j, ITEM_BURIED)) {
            if (GET_OBJ_TIMER(j) == 5) {
                if (room) {
                    send_to_room(room, "@DFlies start to gather around $p@D.@n\r\n", j->getShortDescription());
                }
            }
            if (GET_OBJ_TIMER(j) == 3) {
                if (room) {
                    send_to_room(room, "@DA cloud of flies has formed over %s@D.@n\r\n", j->getShortDescription());
                }
            }
            if (GET_OBJ_TIMER(j) == 2) {
                if (room) {
                    send_to_room(room, "@DMaggots can be seen crawling all over %s@D.@n\r\n", j->getShortDescription());
                }
            }
            if (GET_OBJ_TIMER(j) == 1) {
                if (room) {
                    send_to_room(room, "@DMaggots have nearly stripped %s of all its flesh@D.@n\r\n", j->getShortDescription());
                }
            }
        }
        if (!GET_OBJ_TIMER(j)) {

            if (j->carried_by) {
                if (!strstr(j->getName(), "android")) {
                    act("$p decays in your hands.", false, j->carried_by, j, nullptr, TO_CHAR);
                } else {
                    act("$p decays in your hands.", false, j->carried_by, j, nullptr, TO_CHAR);
                }
            }
            auto con = j->getObjects();
            for (auto jj : filter_raw(con)) {
                obj_from_obj(jj);

                if (j->in_obj)
                    obj_to_obj(jj, j->in_obj);
                else if (j->carried_by)
                    obj_to_room(jj, IN_ROOM(j->carried_by));
                else if (room)
                    obj_to_room(jj, room);
                else
                    core_dump();
            }
            extract_obj(j);
        }

    }
}

void characterVitalsRecovery(uint64_t heartPulse, double deltaTime) {
    auto subs = characterSubscriptions.all("characterVitalsRecovery");
    for(auto ch : filter_raw(subs)) {

        if(AFF_FLAGGED(ch, AFF_POISON) || ch->task != Task::nothing) {
            // Poison stops all healing. So does having a task.
            continue;
        }

        double base = 0.005;

        double universalPerc = 0.0;

        // The healing tank bonus is pretty up there.
        if(PLR_FLAGGED(ch, PLR_HEALT) && SITS(ch)) {
            universalPerc += 20.0;
        }

        // TODO: figure out how to attach this data to rooms.
        if(auto r = ch->getRoom(); r) {
            // regen rooms (or destroyed rooms, with the right bonus) grant a huge boost.
            if (ROOM_FLAGGED(r, ROOM_REGEN)
                || (GET_BONUS(ch, BONUS_DESTROYER) > 0 && r->getDamage() >= 75))
                universalPerc += 2.0;

            if(ROOM_FLAGGED(r, ROOM_BEDROOM))
                universalPerc += 0.25;

            if (cook_element(IN_ROOM(ch)) == 1)
                universalPerc += 2.0;
        }

        for(auto v : {CharVital::health, CharVital::stamina, CharVital::ki, CharVital::lifeforce}) {

            // Androids don't have Lifeforce.
            if(IS_ANDROID(ch) && v == CharVital::lifeforce) continue;

            // This will loop through most possible modifiers...
            double perc = 1.0 + universalPerc + ch->getAffectModifier(APPLY_CVIT_REGEN_MULT, static_cast<int>(v));

            if(perc <= 0.0) {
                // the healing multipliers are so low that all healing is neutralized.
                // Floor it out for sanity.
                perc = 0.05;
            }

            ch->modCurVitalDam(v, -(base * (perc)) * deltaTime);
        }
    }
}


void healTankService(uint64_t heartPulse, double deltaTime) {
    auto subs = objectSubscriptions.all("healTankService");
    for(auto o : filter_raw(subs)) {

        auto en = o->getBaseStat("energy");

        if(auto ch = SITTING(o); ch) {
            // the heal tank is occupied.

            // set this, just in case it wasn't.
            ch->player_flags.set(PLR_HEALT, true);

            bool mustLeave = false;

            en = std::max(0.0, en - deltaTime);

            if(en <= 0.0) {
                send_to_char(ch, "@wThe healing tank's energy reserves are depleted.\r\n");
                mustLeave = true;
            }

            if(ch->isFullVitals()) {
                // the occupant has reached full health.
                send_to_char(ch, "@wThe healing tank has fully restored your vigor.\r\n");
                mustLeave = true;
            }

            if(mustLeave) {
                act("You step out of the healing tank.", true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@w steps out of the healing tank.@n", true, ch, nullptr, nullptr,
                    TO_ROOM);
                ch->player_flags.set(PLR_HEALT, false);
                o->sitting.reset();
                ch->sits.reset();
            }

        } else {
            // the heal tank has no occupant.
            if(en < 200.0) {
                o->setBaseStat("energy", std::min(200.0, en + deltaTime));
            } else {
                // No need to update it further.
                objectSubscriptions.unsubscribe("healTankService", o);
            }
        }
    }
}

void hunger_update(uint64_t heartPulse, double deltaTime) {
    auto ac = characterSubscriptions.all("players");
    for(auto i : filter_raw(ac)) {
        // making it so that you don't get hungry/thirsty if you're just leisurely idling, rping, etc.
        if (!i->isFullVital(CharVital::health)) {
            if (rand_number(1, 2) == 2) {
                gain_condition(i, HUNGER, -1);
            }
            if (rand_number(1, 2) == 2) {
                gain_condition(i, THIRST, -1);
            }
        }
        if (rand_number(1, 2) == 2) {
            gain_condition(i, DRUNK, -1);
        }
    }
}

void relax_update(uint64_t heartPulse, double deltaTime) {
    auto ac = characterSubscriptions.all("players");
    for(auto i : filter_raw(ac)) {
        if (i->getRoomFlag(ROOM_HOUSE)) {
            i->modBaseStat("relax_count", 1);
        } else if (GET_RELAXCOUNT(i) >= 464) {
            i->modBaseStat("relax_count", -4);
        } else if (GET_RELAXCOUNT(i) >= 232) {
            i->modBaseStat("relax_count", -3);
        } else if (GET_RELAXCOUNT(i) > 0 && rand_number(1, 3) == 3) {
            i->modBaseStat("relax_count", -2);
        } else {
            i->modBaseStat("relax_count", -1);
        }

        if (GET_RELAXCOUNT(i) < 0) {
            i->setBaseStat("relax_count", 0);
        }
    }
}

void auralight_update(uint64_t heartPulse, double deltaTime) {
    auto ac = characterSubscriptions.all("auralight");
    for(auto i : filter_raw(ac)) {
        if (PLR_FLAGGED(i, PLR_AURALIGHT)) {
            if (GET_KI(i) > 0) {
                i->modCurVital(CharVital::ki, -1);
            } else {
                send_to_char(i, "You don't have enough energy to keep the aura active.\r\n");
                act("$n's aura slowly stops shining and fades.\r\n", true, i, nullptr, nullptr, TO_ROOM);
                i->player_flags.set(PLR_AURALIGHT, false);
            }
        } else {
            characterSubscriptions.unsubscribe("auralight", i);
        }
    }
}

void player_misc_update(uint64_t heartPulse, double deltaTime) {
    auto ac = characterSubscriptions.all("players");
    for(auto i : filter_raw(ac)) {
        i->raiseGravAcclim();
        update_char_objects(i);
        if (GET_ADMLEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
            check_idling(i);
        else
            (i->timer)++;

        auto sleeptime = GET_SLEEPT(i);
        if (sleeptime > 0 && GET_POS(i) != POS_SLEEPING) {
            i->modBaseStat("sleeptime", -1);
        }
        if (sleeptime < 8 && GET_POS(i) == POS_SLEEPING) {
            i->modBaseStat("sleeptime", rand_number(2, 4));
        }
        heal_limb(i);

        if (i->getCurVital(CharVital::ki) >= GET_MAX_MANA(i) * 0.5 && GET_CHARGE(i) < GET_MAX_MANA(i) * 0.1 && GET_PREFERENCE(i) == PREFERENCE_KI && !PLR_FLAGGED(i, PLR_AURALIGHT)) {
            i->setBaseStat<int64_t>("charge", GET_MAX_MANA(i) * 0.1);
        }

    }
}

void kaioken_update(uint64_t heartPulse, double deltaTime) {
    auto ac = characterSubscriptions.all("players");
    for(auto i : filter_raw(ac)) {
        auto kaioken = GET_KAIOKEN(i);
        int x = (kaioken * 5) + 5;
        if (kaioken > 0) {
            improve_skill(i, SKILL_KAIOKEN, -1);
            if ((GET_SKILL(i, SKILL_KAIOKEN) < rand_number(1, x) || (i->getCurVital(CharVital::stamina)) <= GET_MAX_MOVE(i) / 10))
                i->remove_kaioken(2);
        }
    }
}

void poison_update(uint64_t heartPulse, double deltaTime) {
    auto ac = characterSubscriptions.all("poisoned");
    for(auto i : filter_raw(ac)) {
        if(!AFF_FLAGGED(i, AFF_POISON)) {
            characterSubscriptions.unsubscribe("poisoned", i);
            continue;
        }

        double cost = 0.0;
        if (GET_CON(i) >= 100) {
            cost = 0.01;
        } else if (GET_CON(i) >= 80) {
            cost = 0.02;
        } else if (GET_CON(i) >= 50) {
            cost = 0.03;
        } else if (GET_CON(i) >= 30) {
            cost = 0.04;
        } else if (GET_CON(i) >= 20) {
            cost = 0.05;
        } else {
            cost = 0.06;
        }
        if (auto remaining = i->modCurVitalDam(CharVital::health, cost); remaining > 0) {
            send_to_char(i, "You puke as the poison burns through your blood.\r\n");
            act("$n shivers and then pukes.", true, i, nullptr, nullptr, TO_ROOM);
        } else {
            send_to_char(i, "The poison claims your life!\r\n");
            act("$n pukes up blood and falls down dead!", true, i, nullptr, nullptr, TO_ROOM);
            die(i, i->poisonby);
        }
    }
}

/* Update PCs, NPCs, and objects */
void point_update(uint64_t heartPulse, double deltaTime)
{
    struct obj_data *jj, *next_thing2, *vehicle = nullptr;
    /* characters */

    std::unordered_set<int> processed;

    for (auto &[vn, z] : zone_table)
    {
        if (z.playersInZone.empty())
            continue;

        for (const auto &l : {z.playersInZone, z.npcsInZone})
        {
            auto copy = l;
            for (auto i : filter_raw(copy))
            {
                if (processed.contains(i->id))
                    continue;
                processed.insert(i->id);

                int change = false;

                if (IS_NPC(i))
                {
                    i->setBaseStat("aggtimer", 0);
                }

                if (GET_POS(i) >= POS_STUNNED)
                {
                    update_flags(i);
                    if (!IS_NPC(i))
                    {
                        change = !i->isFullVitals();
                    }

                    if (i->mutations.get(Mutation::limb_regeneration))
                    {
                        mutant_limb_regen(i);
                    }

                    if (AFF_FLAGGED(i, AFF_BURNED))
                    {
                        if (rand_number(1, 5) >= 4)
                        {
                            send_to_char(i, "Your burns are healed now.\r\n");
                            act("$n@w's burns are now healed.@n", true, i, nullptr, nullptr, TO_ROOM);
                            i->affect_flags.set(AFF_BURNED, false);
                        }
                    }

                    if (i->getLocationTileType() == SECT_WATER_NOSWIM && !CARRIED_BY(i) && !IS_KANASSAN(i))
                    {
                        auto carweight = i->getEffectiveStat("weight_carried");
                        if (i->getCurVital(CharVital::stamina) >= carweight)
                        {
                            act("@bYou swim in place.@n", true, i, nullptr, nullptr, TO_CHAR);
                            act("@C$n@b swims in place.@n", true, i, nullptr, nullptr, TO_ROOM);
                            i->modCurVital(CharVital::stamina, -carweight);
                        }
                        else
                        {
                            i->modCurVital(CharVital::stamina, -carweight);
                            act("@RYou are drowning!@n", true, i, nullptr, nullptr, TO_CHAR);
                            act("@C$n@b gulps water as $e struggles to stay above the water line.@n", true, i, nullptr,
                                nullptr,
                                TO_ROOM);
                            if (i->modCurVitalDam(CharVital::health, 0.33) <= 0)
                            {
                                act("@rYou drown!@n", true, i, nullptr, nullptr, TO_CHAR);
                                act("@R$n@r drowns!@n", true, i, nullptr, nullptr, TO_ROOM);
                                die(i, nullptr);
                            }
                        }
                    }

                    if (!has_o2(i) && (i->getLocationEnvironment(ENV_WATER) >= 100.0 || i->getWhereFlag(WhereFlag::space)))
                    {
                        if (auto remKi = i->modCurVitalDam(CharVital::ki, .005); remKi < 1.0)
                        {
                            send_to_char(i, "Your ki holds an atmosphere around you.\r\n");
                        }
                        else
                        {
                            if (remKi > 0.9)
                            {
                                send_to_char(i, "You struggle trying to hold your breath!\r\n");
                            }
                            else
                            {
                                if (i->getLocationEnvironment(ENV_WATER) >= 100.0)
                                {
                                    send_to_char(i, "You have drowned!\r\n");
                                    act("@W$n@W drowns right in front of you.@n", false, i, nullptr, nullptr, TO_ROOM);
                                }
                                else
                                {
                                    send_to_char(i, "You have run out of air!\r\n");
                                    act("@W$n@W asphyxiates right in front of you.@n", false, i, nullptr, nullptr, TO_ROOM);
                                }
                                die(i, nullptr);
                            }
                        }
                    }

                    if (!AFF_FLAGGED(i, AFF_FLYING) && i->getLocationGroundEffect() == 6 && !MOB_FLAGGED(i, MOB_NOKILL) && !IS_DEMON(i))
                    {
                        act("@rYour legs are burned by the lava!@n", true, i, nullptr, nullptr, TO_CHAR);
                        act("@R$n@r's legs are burned by the lava!@n", true, i, nullptr, nullptr, TO_ROOM);
                        if (IS_NPC(i) && IS_HUMANOID(i) && rand_number(1, 2) == 2)
                        {
                            do_fly(i, nullptr, 0, 0);
                        }
                        i->modCurVitalDam(CharVital::health, .05);
                        if (GET_HIT(i) < 0)
                        {
                            act("@rYou have burned to death!@n", true, i, nullptr, nullptr, TO_CHAR);
                            act("@R$n@r has burned to death!@n", true, i, nullptr, nullptr, TO_ROOM);
                            die(i, nullptr);
                        }
                    }

                    if (change && !AFF_FLAGGED(i, AFF_POISON))
                    {
                        if (GET_POS(i) == POS_SLEEPING)
                        {
                            send_to_char(i, "@wYour sleep does you some good.@n\r\n");
                            if (!IS_ANDROID(i) && !FIGHTING(i))
                                i->restoreVital(CharVital::lifeforce);
                        }
                        else if (GET_POS(i) == POS_RESTING)
                        {
                            send_to_char(i, "@wYou feel relaxed and better.@n\r\n");
                            if (!i->isFullVital(CharVital::lifeforce))
                            {
                                if (!IS_ANDROID(i) && !FIGHTING(i) && GET_SUPPRESS(i) <= 0 &&
                                    GET_HIT(i) != (i->getEffectiveStat("health")))
                                {
                                    i->modCurVitalDam(CharVital::lifeforce, -.15);
                                    send_to_char(i, "@CYou feel more lively.@n\r\n");
                                }
                            }
                        }
                        else if (GET_POS(i) == POS_SITTING)
                            send_to_char(i, "@wYou feel rested and better.@n\r\n");
                        else
                            send_to_char(i, "You feel slightly better.\r\n");
                    }

                    if (GET_POS(i) <= POS_STUNNED)
                        update_pos(i);
                }
            }

            auto items = z.objectsInZone;
            for (auto j : filter_raw(items))
            {
                if (processed.contains(j->id))
                    continue;
                processed.insert(j->id);

                /* Let's get rid of dropped norent items. */
                if (OBJ_FLAGGED(j, ITEM_NORENT) && j->worn_by == nullptr && j->carried_by == nullptr && obj_selling != j && GET_OBJ_VNUM(j) != 7200)
                {
                    time_t diff = 0;

                    diff = time(nullptr) - GET_LAST_LOAD(j);
                    if (diff > 240 && GET_LAST_LOAD(j) > 0)
                    {
                        basic_mud_log("No rent object (%s) extracted from room (%d)", j->getShortDescription(), j->getRoomVnum());
                        extract_obj(j);
                    }
                }

                if (GET_OBJ_TYPE(j) == ITEM_HATCH)
                {
                    if ((vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(j, VAL_HATCH_DEST))))
                    {
                        SET_OBJ_VAL(j, VAL_HATCH_EXTROOM, vehicle->getRoomVnum());
                    }
                }

                if (GET_OBJ_TYPE(j) == ITEM_PORTAL)
                {
                    if (GET_OBJ_TIMER(j) > 0)
                        j->modBaseStat("timer", -1);

                    if (GET_OBJ_TIMER(j) == 0)
                    {
                        send_to_room(j->getRoom(), "A glowing portal fades from existence.\r\n");
                        extract_obj(j);
                    }
                }
                else if (GET_OBJ_VNUM(j) == 1306)
                {
                    if (GET_OBJ_TIMER(j) > 0)
                        j->modBaseStat("timer", -1);

                    if (GET_OBJ_TIMER(j) == 0)
                    {
                        send_to_room(j->getRoom(), "The %s@n settles to the ground and goes out.\r\n", j->getShortDescription());
                        extract_obj(j);
                    }
                }
                else if (OBJ_FLAGGED(j, ITEM_ICE))
                {
                    if (GET_OBJ_VNUM(j) == 79 && rand_number(1, 2) == 2)
                    {
                        if (j->getLocationGroundEffect() >= 1 && j->getLocationGroundEffect() <= 5)
                        {
                            send_to_room(IN_ROOM(j),
                                         "The heat from the lava melts a great deal of the glacial wall and the lava cools a bit in turn.\r\n");
                            j->modLocationGroundEffect(-1);
                            if (GET_OBJ_WEIGHT(j) - (5 + (GET_OBJ_WEIGHT(j) * 0.025)) > 0)
                            {
                                j->modBaseStat<weight_t>("weight", -(5 + (GET_OBJ_WEIGHT(j) * 0.025)));
                            }
                            else
                            {
                                send_to_room(IN_ROOM(j),
                                             "The glacial wall blocking off the %s direction melts completely away.\r\n",
                                             dirs[GET_OBJ_COST(j)]);
                                extract_obj(j);
                            }
                        }
                        else if (GET_OBJ_WEIGHT(j) - (5 + (GET_OBJ_WEIGHT(j) * 0.025)) > 0)
                        {
                            j->modBaseStat<weight_t>("weight", -(5 + (GET_OBJ_WEIGHT(j) * 0.025)));
                            send_to_room(IN_ROOM(j), "The glacial wall blocking off the %s direction melts some what.\r\n",
                                         dirs[GET_OBJ_COST(j)]);
                        }
                        else
                        {
                            send_to_room(IN_ROOM(j),
                                         "The glacial wall blocking off the %s direction melts completely away.\r\n",
                                         dirs[GET_OBJ_COST(j)]);
                            extract_obj(j);
                        }
                    }
                    else if (GET_OBJ_VNUM(j) != 79)
                    {
                        if (j->carried_by && !j->in_obj)
                        {
                            int melt = 5 + (GET_OBJ_WEIGHT(j) * 0.02);
                            if (GET_OBJ_WEIGHT(j) - (5 + (GET_OBJ_WEIGHT(j) * 0.02)) > 0)
                            {
                                j->modBaseStat<weight_t>("weight", -melt);
                                send_to_char(j->carried_by, "%s @wmelts a little.\r\n", j->getShortDescription());
                            }
                            else
                            {
                                send_to_char(j->carried_by, "%s @wmelts completely away.\r\n", j->getShortDescription());
                                int remainder = melt - GET_OBJ_WEIGHT(j);
                                extract_obj(j);
                            }
                        }
                        else if (IN_ROOM(j) != NOWHERE)
                        {
                            if (GET_OBJ_WEIGHT(j) - (5 + (GET_OBJ_WEIGHT(j) * 0.02)) > 0)
                            {
                                j->modBaseStat<weight_t>("weight", -(5 + (GET_OBJ_WEIGHT(j) * 0.02)));
                                send_to_room(IN_ROOM(j), "%s @wmelts a little.\r\n", j->getShortDescription());
                            }
                            else
                            {
                                send_to_room(IN_ROOM(j), "%s @wmelts completely away.\r\n", j->getShortDescription());
                                extract_obj(j);
                            }
                        }
                    }
                }

                /* If the timer is set, count it down and at 0, try the trigger */
                /* note to .rej hand-patchers: make this last in your point-update() */
                else if (GET_OBJ_TIMER(j) > 0)
                {
                    j->modBaseStat("timer", -1);
                    if (!GET_OBJ_TIMER(j))
                        timer_otrigger(j);
                }
            }
        }
    }
}
