/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/constants.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/feats.h"
#include "dbat/oasis.h"
#include "dbat/config.h"
#include "dbat/act.item.h"
#include "dbat/act.movement.h"
#include "dbat/races.h"
#include "dbat/act.informative.h"
#include "dbat/class.h"

/* external variables */

/*
 * Special spells appear below.
 */

int roll_skill(struct char_data *ch, int snum) {
    int roll, skval, i;
    if (!IS_NPC(ch)) {
        skval = GET_SKILL(ch, snum);
        if (SKILL_SPOT == snum) {
            if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4)) {
                skval += 5;
            }
        } else if (SKILL_HIDE == snum) {
            if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
                skval += 5;
            } else if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 5 || GET_GENOME(ch, 1) == 5)) {
                skval += 10;
            }
        }
    } else if (IS_NPC(ch)) {
        int numb = 0;
        if (GET_LEVEL(ch) <= 10) {
            numb = rand_number(15, 30);
        }
        if (GET_LEVEL(ch) <= 20) {
            numb = rand_number(20, 40);
        }
        if (GET_LEVEL(ch) <= 30) {
            numb = rand_number(40, 60);
        }
        if (GET_LEVEL(ch) <= 60) {
            numb = rand_number(60, 80);
        }
        if (GET_LEVEL(ch) <= 80) {
            numb = rand_number(70, 90);
        }
        if (GET_LEVEL(ch) <= 90) {
            numb = rand_number(80, 95);
        }
        if (GET_LEVEL(ch) <= 100) {
            numb = rand_number(90, 100);
        }
        skval = numb;
    }
    if (snum == SKILL_SPOT && GET_SKILL(ch, SKILL_LISTEN)) {
        skval += GET_SKILL(ch, SKILL_LISTEN) / 10;
    }
    if (snum < 0 || snum >= SKILL_TABLE_SIZE)
        return 0;
    if (IS_SET(spell_info[snum].skilltype, SKTYPE_SPELL)) {
        /*
         * There's no real roll for a spell to succeed, so instead we will
         * return the spell resistance roll; the defender must have resistance
         * higher than this roll to avoid it. Most spells should also have some
         * kind of save called after roll_skill.
         */

        return roll + rand_number(1, 20);
    } else if (IS_SET(spell_info[snum].skilltype, SKTYPE_SKILL)) {
        if (!skval && IS_SET(spell_info[snum].flags, SKFLAG_NEEDTRAIN)) {
            return -1;
        } else {
            roll = skval;
            if (IS_SET(spell_info[snum].flags, SKFLAG_STRMOD))
                roll += ability_mod_value(GET_STR(ch));
            if (IS_SET(spell_info[snum].flags, SKFLAG_DEXMOD))
                roll += dex_mod_capped(ch);
            if (IS_SET(spell_info[snum].flags, SKFLAG_CONMOD))
                roll += ability_mod_value(GET_CON(ch));
            if (IS_SET(spell_info[snum].flags, SKFLAG_INTMOD))
                roll += ability_mod_value(GET_INT(ch));
            if (IS_SET(spell_info[snum].flags, SKFLAG_WISMOD))
                roll += ability_mod_value(GET_WIS(ch));
            if (IS_SET(spell_info[snum].flags, SKFLAG_CHAMOD))
                roll += ability_mod_value(GET_CHA(ch));
            if (IS_SET(spell_info[snum].flags, SKFLAG_ARMORALL))
                roll -= GET_ARMORCHECKALL(ch);
            else if (IS_SET(spell_info[snum].flags, SKFLAG_ARMORBAD))
                roll -= GET_ARMORCHECK(ch);
            return roll + rand_number(1, 20);
        }
    } else {
        basic_mud_log("Trying to roll uncategorized skill/spell #%d for %s", snum, GET_NAME(ch));
        return 0;
    }
}

int roll_resisted(struct char_data *actor, int sact, struct char_data *resistor, int sres) {
    return roll_skill(actor, sact) >= roll_skill(resistor, sres);
}
