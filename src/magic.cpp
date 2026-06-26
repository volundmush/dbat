/* ************************************************************************
 *   File: magic.c                                       Part of CircleMUD *
 *  Usage: low-level functions for magic; spell template code              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "magic.h"
#include "config.h"
#include "consts/aligns.h"

#include "affected_impl.h"
#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "comm.h"
#include "config_db.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/itemdata.h"
#include "consts/magic.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/prefflags.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "consts/sex.h"
#include "db.h"
#include "dg_scripts.h"
#include "extract.h"
#include "feats.h"
#include "fight.h"
#include "flags.h"
#include "handler.h"
#include "interpreter.h"
#include "log.h"
#include "mobact.h"
#include "object_api.h"
#include "object_impl.h"
#include "iterate.hpp"
#include "object_macros.h"
#include "object_utils.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_api.h"
#include "room_db.h"
#include "room_macros.h"
#include "search.h"
#include "skills.h"
#include "spells.h"
#include "util_macros.h"
#include "zone_api.h"

/* local functions */
int mag_materials(struct char_data *ch, int item0, int item1, int item2,
                  int extract, int verbose);
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch,
                        int spellnum);

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
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  char_inventory_iterate(ch, [&](auto tobj) {
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
    return true;
  });
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
    return (FALSE);
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
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}

int mag_newsaves(struct char_data *ch, struct char_data *victim, int spellnum,
                 int level, int cast_stat) {
  return FALSE;
}

/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               int spellnum) {
  return 0;
}



#define MAX_SPELL_AFFECTS 5 /* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
                 int spellnum) {
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;

  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, GET_INT(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags,
               MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  switch (spellnum) {

  case SPELL_CHILL_TOUCH:
    af[0].location = APPLY_STR;
    af[0].duration = 24;
    af[0].modifier = -1;
    accum_duration = TRUE;
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_MAGE_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = 40;
    af[0].duration = 1 * GET_LEVEL(ch);
    accum_duration = FALSE;
    to_vict = "You feel someone protecting you.";
    break;

  case SPELL_BLESS:
    af[0].location = APPLY_ACCURACY;
    af[0].modifier = 2;
    af[0].duration = 6;

    af[1].location = APPLY_WILL;
    af[1].modifier = 1;
    af[1].duration = 6;

    accum_duration = TRUE;
    to_vict = "You feel righteous.";
    break;

  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_ACCURACY;
    af[0].modifier = -4;
    af[0].duration = 2;
    af[0].bitvector = AFF_BLIND;

    af[1].location = APPLY_AC;
    af[1].modifier = -4;
    af[1].duration = 2;
    af[1].bitvector = AFF_BLIND;

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_BANE:
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;

  case SPELL_BESTOW_CURSE:
    accum_duration = FALSE;
    accum_affect = FALSE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_SEE_INVIS:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_INVIS;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_FAERIE_FIRE:
    af[0].location = APPLY_AC;
    af[0].modifier = -1; /*should make target easier to hit */
    af[0].duration = 3;
    accum_duration = FALSE;
    to_vict = "Your body flickers with a purplish light.";
    to_room = "$n's body flickers with with a purplish light.";
    break;

  case SPELL_DARKVISION:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration = TRUE;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INVISIBLE:
    if (!victim)
      victim = ch;

    af[0].duration = 12 + (level / 4);
    af[0].modifier = 4;
    af[0].location = APPLY_AC;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration = TRUE;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_POISON:
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;

  case SPELL_PROT_FROM_EVIL:
    af[0].duration = 24;
    af[0].bitvector = AFF_PROTECT_GOOD;
    accum_duration = TRUE;
    to_vict = "You feel invulnerable!";
    break;

  case SPELL_SANCTUARY:
    af[0].duration = 4;
    af[0].bitvector = AFF_SANCTUARY;

    accum_duration = TRUE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    break;

  case SPELL_SLEEP:
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;

    af[0].duration = 4 + (level / 4);
    af[0].bitvector = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      char_position_set(victim, POS_SLEEPING);
    }
    break;

  case SPELL_HAYASA:
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;

    af[0].duration = 4 + (level / 4);
    af[0].bitvector = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      char_position_set(victim, POS_SLEEPING);
    }
    break;

  case SPELL_BULL_STRENGTH:
    af[0].location = APPLY_STR;
    af[0].duration = level;
    af[0].modifier = 1 + (rand_number(1, 4));
    accum_duration = FALSE;
    accum_affect = FALSE;
    to_vict = "You feel stronger!";
    break;

  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    af[0].duration = level;
    af[0].bitvector = AFF_SENSE_LIFE;
    accum_duration = TRUE;
    break;

  case SPELL_WATERWALK:
    af[0].duration = 24;
    af[0].bitvector = AFF_WATERWALK;
    accum_duration = TRUE;
    to_vict = "You feel webbing between your toes.";
    break;

  case SPELL_STONESKIN:
    af[0].duration = 1 * GET_LEVEL(ch);
    accum_duration = FALSE;
    to_vict = "Your skin hardens into stone!";
    break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */



  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch,
                        int spellnum) {

}

/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */
void mag_groups(int level, struct char_data *ch, int spellnum) {
  struct char_data *k;

  if (ch == NULL)
    return;

  if (!char_condition_has(ch, "group"))
    return;
  if (MASTER(ch) != NULL)
    k = MASTER(ch);
  else
    k = ch;
  char_followers_iterate(k, [&](struct char_data *tch) {
    if (char_room_get(tch) != char_room_get(ch))
      return true;
    if (!char_condition_has(tch, "group"))
      return true;
    if (ch == tch)
      return true;
    perform_mag_groups(level, ch, tch, spellnum);
    return true;
  });

  if ((k != ch) && char_condition_has(k, "group"))
    perform_mag_groups(level, ch, k, spellnum);
  perform_mag_groups(level, ch, ch, spellnum);
}

/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented.
 */
void mag_masses(int level, struct char_data *ch, int spellnum) {
  
}

/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(int level, struct char_data *ch, int spellnum) {

}

/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 */

mob_vnum monsum_list_lg_1[] = {300, 301, 302, NOBODY};
mob_vnum monsum_list_ng_1[] = {300, 301, 302, 303, 304, NOBODY};
mob_vnum monsum_list_cg_1[] = {302, 303, 304, NOBODY};
mob_vnum monsum_list_ln_1[] = {300, 301, 305, 306, NOBODY};
mob_vnum monsum_list_nn_1[] = {302, 307, 308, NOBODY};
mob_vnum monsum_list_cn_1[] = {303, 304, 309, 310, 311, NOBODY};
mob_vnum monsum_list_le_1[] = {305, 306, 307, 308, NOBODY};
mob_vnum monsum_list_ne_1[] = {305, 306, 307, 308, 309, 310, 311, NOBODY};
mob_vnum monsum_list_ce_1[] = {307, 308, 309, 310, 311, NOBODY};

mob_vnum monsum_list_lg_2[] = {312, 313, 314, NOBODY};
mob_vnum monsum_list_ng_2[] = {312, 313, 314, NOBODY};
mob_vnum monsum_list_cg_2[] = {312, 313, 314, 315, NOBODY};
mob_vnum monsum_list_ln_2[] = {312, 317, NOBODY};
mob_vnum monsum_list_nn_2[] = {315, 316, 317, NOBODY};
mob_vnum monsum_list_cn_2[] = {313, 316, 318, NOBODY};
mob_vnum monsum_list_le_2[] = {316, 317, NOBODY};
mob_vnum monsum_list_ne_2[] = {318, 319, NOBODY};
mob_vnum monsum_list_ce_2[] = {320, 321, NOBODY};

mob_vnum monsum_list_lg_3[] = {NOBODY};
mob_vnum monsum_list_ng_3[] = {NOBODY};
mob_vnum monsum_list_cg_3[] = {NOBODY};
mob_vnum monsum_list_ln_3[] = {NOBODY};
mob_vnum monsum_list_nn_3[] = {NOBODY};
mob_vnum monsum_list_cn_3[] = {NOBODY};
mob_vnum monsum_list_le_3[] = {NOBODY};
mob_vnum monsum_list_ne_3[] = {NOBODY};
mob_vnum monsum_list_ce_3[] = {NOBODY};

mob_vnum monsum_list_lg_4[] = {NOBODY};
mob_vnum monsum_list_ng_4[] = {NOBODY};
mob_vnum monsum_list_cg_4[] = {NOBODY};
mob_vnum monsum_list_ln_4[] = {NOBODY};
mob_vnum monsum_list_nn_4[] = {NOBODY};
mob_vnum monsum_list_cn_4[] = {NOBODY};
mob_vnum monsum_list_le_4[] = {NOBODY};
mob_vnum monsum_list_ne_4[] = {NOBODY};
mob_vnum monsum_list_ce_4[] = {NOBODY};

mob_vnum monsum_list_lg_5[] = {NOBODY};
mob_vnum monsum_list_ng_5[] = {NOBODY};
mob_vnum monsum_list_cg_5[] = {NOBODY};
mob_vnum monsum_list_ln_5[] = {NOBODY};
mob_vnum monsum_list_nn_5[] = {NOBODY};
mob_vnum monsum_list_cn_5[] = {NOBODY};
mob_vnum monsum_list_le_5[] = {NOBODY};
mob_vnum monsum_list_ne_5[] = {NOBODY};
mob_vnum monsum_list_ce_5[] = {NOBODY};

mob_vnum monsum_list_lg_6[] = {NOBODY};
mob_vnum monsum_list_ng_6[] = {NOBODY};
mob_vnum monsum_list_cg_6[] = {NOBODY};
mob_vnum monsum_list_ln_6[] = {NOBODY};
mob_vnum monsum_list_nn_6[] = {NOBODY};
mob_vnum monsum_list_cn_6[] = {NOBODY};
mob_vnum monsum_list_le_6[] = {NOBODY};
mob_vnum monsum_list_ne_6[] = {NOBODY};
mob_vnum monsum_list_ce_6[] = {NOBODY};

mob_vnum monsum_list_lg_7[] = {NOBODY};
mob_vnum monsum_list_ng_7[] = {NOBODY};
mob_vnum monsum_list_cg_7[] = {NOBODY};
mob_vnum monsum_list_ln_7[] = {NOBODY};
mob_vnum monsum_list_nn_7[] = {NOBODY};
mob_vnum monsum_list_cn_7[] = {NOBODY};
mob_vnum monsum_list_le_7[] = {NOBODY};
mob_vnum monsum_list_ne_7[] = {NOBODY};
mob_vnum monsum_list_ce_7[] = {NOBODY};

mob_vnum monsum_list_lg_8[] = {NOBODY};
mob_vnum monsum_list_ng_8[] = {NOBODY};
mob_vnum monsum_list_cg_8[] = {NOBODY};
mob_vnum monsum_list_ln_8[] = {NOBODY};
mob_vnum monsum_list_nn_8[] = {NOBODY};
mob_vnum monsum_list_cn_8[] = {NOBODY};
mob_vnum monsum_list_le_8[] = {NOBODY};
mob_vnum monsum_list_ne_8[] = {NOBODY};
mob_vnum monsum_list_ce_8[] = {NOBODY};

mob_vnum monsum_list_lg_9[] = {NOBODY};
mob_vnum monsum_list_ng_9[] = {NOBODY};
mob_vnum monsum_list_cg_9[] = {NOBODY};
mob_vnum monsum_list_ln_9[] = {NOBODY};
mob_vnum monsum_list_nn_9[] = {NOBODY};
mob_vnum monsum_list_cn_9[] = {NOBODY};
mob_vnum monsum_list_le_9[] = {NOBODY};
mob_vnum monsum_list_ne_9[] = {NOBODY};
mob_vnum monsum_list_ce_9[] = {NOBODY};

mob_vnum *monsum_list[9][9] = {
    {monsum_list_lg_1, monsum_list_ng_1, monsum_list_cg_1, monsum_list_ln_1,
     monsum_list_nn_1, monsum_list_cn_1, monsum_list_le_1, monsum_list_ne_1,
     monsum_list_ce_1},
    {monsum_list_lg_2, monsum_list_ng_2, monsum_list_cg_2, monsum_list_ln_2,
     monsum_list_nn_2, monsum_list_cn_2, monsum_list_le_2, monsum_list_ne_2,
     monsum_list_ce_2},
    {monsum_list_lg_3, monsum_list_ng_3, monsum_list_cg_3, monsum_list_ln_3,
     monsum_list_nn_3, monsum_list_cn_3, monsum_list_le_3, monsum_list_ne_3,
     monsum_list_ce_3},
    {monsum_list_lg_4, monsum_list_ng_4, monsum_list_cg_4, monsum_list_ln_4,
     monsum_list_nn_4, monsum_list_cn_4, monsum_list_le_4, monsum_list_ne_4,
     monsum_list_ce_4},
    {monsum_list_lg_5, monsum_list_ng_5, monsum_list_cg_5, monsum_list_ln_5,
     monsum_list_nn_5, monsum_list_cn_5, monsum_list_le_5, monsum_list_ne_5,
     monsum_list_ce_5},
    {monsum_list_lg_6, monsum_list_ng_6, monsum_list_cg_6, monsum_list_ln_6,
     monsum_list_nn_6, monsum_list_cn_6, monsum_list_le_6, monsum_list_ne_6,
     monsum_list_ce_6},
    {monsum_list_lg_7, monsum_list_ng_7, monsum_list_cg_7, monsum_list_ln_7,
     monsum_list_nn_7, monsum_list_cn_7, monsum_list_le_7, monsum_list_ne_7,
     monsum_list_ce_7},
    {monsum_list_lg_8, monsum_list_ng_8, monsum_list_cg_8, monsum_list_ln_8,
     monsum_list_nn_8, monsum_list_cn_8, monsum_list_le_8, monsum_list_ne_8,
     monsum_list_ce_8},
    {monsum_list_lg_9, monsum_list_ng_9, monsum_list_cg_9, monsum_list_ln_9,
     monsum_list_nn_9, monsum_list_cn_9, monsum_list_le_9, monsum_list_ne_9,
     monsum_list_ce_9}};

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {"\r\n", "$n animates a corpse!",
                                 "$n summons extraplanar assistance!"};

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE 20 /* Only one for now. */
#define MOB_ZOMBIE 11
#define MOB_AERIALSERVANT 19

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
                 int spellnum, char *arg) {
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int msg = 0, num = 1, handle_corpse = FALSE, affs = 0, affvs = 0, assist = 0,
      i, j, count;
  char *buf = NULL;
  char buf2[MAX_INPUT_LENGTH];
  int lev;
  mob_vnum mob_num;

  if (ch == NULL)
    return;

  lev = spell_info[spellnum].spell_level;

  switch (spellnum) {
  case SPELL_ANIMATE_DEAD:
    if (obj == NULL) {
      send_to_char(ch, "With what corpse?\r\n");
      return;
    }
    if (!IS_CORPSE(obj)) {
      send_to_char(ch, "That's not a corpse!\r\n");
      return;
    }
    handle_corpse = TRUE;
    msg = 11;
    mob_num = MOB_ZOMBIE;
    break;

  case SPELL_SUMMON_MONSTER_I:
  case SPELL_SUMMON_MONSTER_II:
  case SPELL_SUMMON_MONSTER_III:
  case SPELL_SUMMON_MONSTER_IV:
  case SPELL_SUMMON_MONSTER_V:
  case SPELL_SUMMON_MONSTER_VI:
  case SPELL_SUMMON_MONSTER_VII:
  case SPELL_SUMMON_MONSTER_VIII:
  case SPELL_SUMMON_MONSTER_IX:
    mob_num = NOBODY;
    affvs = 1;
    assist = 1;
    if (arg) {
      buf = arg;
      skip_spaces(&buf);
      if (!*buf)
        buf = NULL;
    }
    j = ALIGN_TYPE(ch);
    if (buf) {
      buf = any_one_arg(buf, buf2);
      for (i = lev - 1; i >= 0; i--) {
        for (count = 0; monsum_list[i][j][count] != NOBODY; count++) {
          mob_num = monsum_list[i][j][count];
          if (auto proto = mob_proto_by_id(mob_num); !proto)
            mob_num = NOBODY;
          else if (!is_name(buf2, proto->name))
            mob_num = NOBODY;
          else
            break;
        }
        if (mob_num != NOBODY)
          break;
      }
      if (mob_num == NOBODY) {
        send_to_char(ch, "That's not a name for a monster you can summon. "
                         "Summoning something else.\r\n");
      } else {
        mud_log("lev=%d, i=%d, ngen=%d", lev, i, lev - i);
        switch (lev - i) {
        case 1:
          num = 1;
          break;
        case 2:
          num = rand_number(1, 3);
          break;
        default:
          num = rand_number(1, 4) + 1;
          break;
        }
      }
    }
    if (mob_num == NOBODY) {
      num = 1;
      for (count = 0; monsum_list[lev - 1][j][count] != NOBODY; count++)
        ;
      if (!count) {
        mud_log("No monsums for spell level %d align %s", lev, alignments[j]);
        return;
      }
      count--;
      mob_num = monsum_list[lev - 1][j][rand_number(0, count)];
    }
    break;

  default:
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }
  for (i = 0; i < num; i++) {
    if (!(mob = read_mobile(mob_num, VIRTUAL))) {
      send_to_char(ch,
                   "You don't quite remember how to summon that creature.\r\n");
      return;
    }
    char_to_room(mob, char_room_get(ch));
    if (affs)
      mag_affects(level, ch, mob, spellnum);
    if (affvs)
      mag_affectsv(level, ch, mob, spellnum);
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    load_mtrigger(mob);
    add_follower(mob, ch);
    if (assist && FIGHTING(ch)) {
      set_fighting(mob, FIGHTING(ch));
    }
    mob->master_id = GET_IDNUM(ch);
  }
  if (handle_corpse) {
    obj_contents_iterate(obj, [&](struct obj_data *tobj) {
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
      return true;
    });
    extract_obj(obj);
  }
}

void mag_points(int level, struct char_data *ch, struct char_data *victim,
                int spellnum) {
  int healing = 0;
  int tmp;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_LIGHT:
    healing = dice(1, 8) + MIN(level, 5);
    send_to_char(victim, "You feel better.\r\n");
    break;
  case SPELL_CURE_CRITIC:
    healing = dice(4, 8) + MIN(level, 20);
    send_to_char(victim, "You feel a lot better!\r\n");
    break;
  case SPELL_HEAL:
    healing = 100 + dice(3, 8);
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    break;
  case SPELL_SENSU:
    if (char_stat_get(victim, "hunger") > -1) {
      char_stat_set(victim, "hunger", 48);
    }
    restore(victim, true);
    break;

  case ART_WHOLENESS_OF_BODY:
    healing = GET_MAX_HIT(victim) - GET_HIT(victim);
    healing = MAX(0, healing);
    tmp = 0;
    if (tmp > healing)
      tmp = healing;
    else {
      healing = tmp;
    }
    break;
  }
  update_pos(victim);
}

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
                   int spellnum) {
  int spell = 0, msg_not_affected = TRUE;
  const char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_HEAL:
    /*
     * Heal also restores health, so don't give the "no effect" message
     * if the target isn't afflicted by the 'blindness' spell.
     */
    msg_not_affected = FALSE;
    /* fall-through */
  case SPELL_REMOVE_BLINDNESS:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    to_room = "There's a momentary gleam in $n's eyes.";
    break;
  case SPELL_NEUTRALIZE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_BESTOW_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  default:
    return;
  }


  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum) {
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
  case SPELL_BLESS:
    if (!OBJ_FLAGGED(obj, ITEM_BLESS) && (GET_OBJ_WEIGHT(obj) <= 5 * level)) {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
      to_char = "$p glows briefly.";
    }
    break;
  case SPELL_INVISIBLE:
    if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
      to_char = "$p vanishes.";
    }
    break;
  case SPELL_POISON:
    if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
        !GET_OBJ_VAL(obj, VAL_FOOD_POISON)) {
      GET_OBJ_VAL(obj, VAL_FOOD_POISON) = 1;
      to_char = "$p steams briefly.";
    }
    break;
  case SPELL_REMOVE_CURSE:
    if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
      REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE)++;
      to_char = "$p briefly glows blue.";
    }
    break;
  case SPELL_NEUTRALIZE_POISON:
    if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
        GET_OBJ_VAL(obj, VAL_FOOD_POISON)) {
      GET_OBJ_VAL(obj, VAL_FOOD_POISON) = 0;
      to_char = "$p steams briefly.";
    }
    break;
  }

  if (to_char == NULL)
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);
}

void mag_creations(int level, struct char_data *ch, int spellnum) {
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
  default:
    send_to_char(ch, "Spell unimplemented, it would seem.\r\n");
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char(ch, "I seem to have goofed.\r\n");
    mud_log("SYSERR: spell_creations, spell %d, obj %d: obj not found", spellnum,
        z);
    return;
  }
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

void mag_affectsv(int level, struct char_data *ch, struct char_data *victim,
                  int spellnum) {
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;

  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, GET_INT(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags,
               MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  switch (spellnum) {
  case SPELL_PARALYZE:
    af[0].duration = level / 2;
    af[0].bitvector = AFF_PARALYZE;
    accum_duration = FALSE;
    to_vict = "You feel your limbs freeze!";
    to_room = "$n suddenly freezes in place!";
    break;
  case ART_STUNNING_FIST:
    af[0].duration = 1;
    af[0].bitvector = AFF_STUNNED;
    accum_duration = FALSE;
    to_vict = "You are in a stunned daze!";
    to_room = "$n is stunned.";
    break;
  case ART_EMPTY_BODY:
    accum_duration = FALSE;
    to_vict = "You switch to the ethereal plane.";
    to_room = "$n disappears.";
    break;
  case ART_QUIVERING_PALM:
    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
      send_to_char(ch, "They are too high level for that.\r\n");
      return;
    }
    af[0].duration = MAX(6, 20 - level);
    af[0].bitvector = AFF_CDEATH;
    accum_duration = FALSE;
    to_vict = "You feel death closing in.";
    break;
  case SPELL_RESISTANCE:
    af[0].duration = 12;
    af[0].location = APPLY_ALLSAVES;
    af[0].modifier = 1;
    accum_duration = FALSE;
    to_vict = "You glow briefly with a silvery light.";
    to_room = "$n glows briefly with a silvery light.";
    break;
  case SPELL_DAZE:
    break;
  case SPELL_SUMMON_MONSTER_I:
  case SPELL_SUMMON_MONSTER_II:
  case SPELL_SUMMON_MONSTER_III:
  case SPELL_SUMMON_MONSTER_IV:
  case SPELL_SUMMON_MONSTER_V:
  case SPELL_SUMMON_MONSTER_VI:
  case SPELL_SUMMON_MONSTER_VII:
  case SPELL_SUMMON_MONSTER_VIII:
  case SPELL_SUMMON_MONSTER_IX:
    af[0].duration = level;
    af[0].bitvector = AFF_SUMMONED;
    accum_duration = FALSE;
    to_vict = "You are summoned to assist $N!";
    to_room = "$n appears, ready for action.";
    break;

  case SPELL_FLARE:
    if (MOB_FLAGGED(victim, MOB_NOBLIND)) {
      send_to_char(ch, "You fail.\r\n");
      return;
    }
    af[0].location = APPLY_ACCURACY;
    af[0].modifier = -1;
    af[0].duration = 2;
    af[0].bitvector = AFF_BLIND;

    to_room = "$n seems to be dazzled!";
    to_vict = "You have been dazzled!";
    break;

  case SPELL_FIRE_SHIELD:
    af[0].duration = level * 5;
    af[0].bitvector = AFF_FIRE_SHIELD;
    to_room = "$n is engulfed in a firey shield!";
    to_vict = "You are engulfed in a firey shield!";
    break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}
