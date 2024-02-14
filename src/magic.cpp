/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "dbat/magic.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/feats.h"
#include "dbat/mobact.h"
#include "dbat/fight.h"

/* local functions */
/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(uint64_t heartPulse, double deltaTime) {
    struct affected_type *af, *next;
    struct char_data *i;

    for (i = affect_list; i; i = i->next_affect) {
        for (af = i->affected; af; af = next) {
            next = af->next;
            if (af->duration >= 1)
                af->duration--;
            else if (af->duration == 0) {
                if (af->type > 0)
                    if (!af->next || (af->next->type != af->type) || (af->next->duration > 0)) {
                        if (spell_info[af->type].wear_off_msg)
                            send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
                        if (GET_SPEEDBOOST(i) > 0 && af->type == SPELL_HAYASA) {
                            GET_SPEEDBOOST(i) = 0;
                        }
                    }
                affect_remove(i, af);
            }
        }
    }
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data *ch, int item0, int item1, int item2,
                  int extract, int verbose) {
    struct obj_data *tobj;
    struct obj_data *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr;

    for (tobj = ch->contents; tobj; tobj = tobj->next_content) {
        if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
            obj0 = tobj;
            item0 = -1;
        } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
            obj1 = tobj;
            item1 = -1;
        } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
            obj2 = tobj;
            item2 = -1;
        }
    }
    if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
        if (verbose) {
            switch (rand_number(0, 2)) {
                case 0:
                    send_to_char(ch, "A wart sprouts on your nose.\r\n");
                    break;
                case 1:
                    send_to_char(ch, "Your hair falls out in clumps.\r\n");
                    break;
                case 2:
                    send_to_char(ch, "A huge corn develops on your big toe.\r\n");
                    break;
            }
        }
        return (false);
    }
    if (extract) {
        if (item0 < 0)
            extract_obj(obj0);
        if (item1 < 0)
            extract_obj(obj1);
        if (item2 < 0)
            extract_obj(obj2);
    }
    if (verbose) {
        send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
        act("A puff of smoke rises from $n's pack.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    return (true);
}


#define MAX_SPELL_AFFECTS 5    /* change if more needed */

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE    20    /* Only one for now. */
#define MOB_ZOMBIE        11
#define MOB_AERIALSERVANT    19

/* affect_update_violence: called from fight.c (causes spells to wear off) */
void affect_update_violence(uint64_t heartPulse, double deltaTime) {
    struct affected_type *af, *next;
    struct char_data *i;
    int dam;
    int maxdam;

    for (i = affectv_list; i; i = i->next_affectv) {
        for (af = i->affectedv; af; af = next) {
            next = af->next;
            if (af->duration >= 1) {
                af->duration--;
                switch (af->type) {
                    case ART_EMPTY_BODY:
                        break;
                }
            } else if (af->duration == -1) {     /* No action */
                continue;
            }
            if (!af->duration) {
                if ((af->type > 0) && (af->type < SKILL_TABLE_SIZE))
                    if (!af->next || (af->next->type != af->type) ||
                        (af->next->duration > 0))
                        if (spell_info[af->type].wear_off_msg)
                            send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
                if (af->bitvector == AFF_SUMMONED) {
                    stop_follower(i);
                    if (!DEAD(i))
                        extract_char(i);
                }
                if (af->type == ART_QUIVERING_PALM) {
                    maxdam = GET_HIT(i) + 8;
                    dam = GET_MAX_HIT(i) * 3 / 4;
                    dam = MIN(dam, maxdam);
                    dam = MAX(0, dam);
                    basic_mud_log("Creeping death strike doing %d dam", dam);
                }
                affectv_remove(i, af);
            }
        }
    }
}

