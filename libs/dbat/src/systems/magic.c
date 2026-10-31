/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/consts/aligns.h"
#include "dbat/config.h"
#include "dbat/magic.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/feats.h"
#include "dbat/mobact.h"
#include "dbat/fight.h"
#include "dbat/affect.h"

/* local functions */
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, int spellnum);

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
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
            if (char_get_speedboost(i) > 0 && af->type == SPELL_HAYASA) {
              char_set_speedboost(i, 0);
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

int mag_newsaves(struct char_data *ch, struct char_data *victim, int spellnum, int level, int cast_stat)
{
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
               int spellnum)
{
  return 0;
}

/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

#define MAX_SPELL_AFFECTS 5 /* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
                 int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;

  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type      = spellnum;
    af[i].bitvector = 0;
    af[i].modifier  = 0;
    af[i].location  = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, char_get_aff_abils_intel(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
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
      to_vict        = "You feel your strength wither!";
      break;

    case SPELL_MAGE_ARMOR:
      af[0].location = APPLY_AC;
      af[0].modifier = 40;
      af[0].duration = 1 * char_get_level(ch);
      accum_duration = FALSE;
      to_vict        = "You feel someone protecting you.";
      break;

    case SPELL_BLESS:
      af[0].location = APPLY_ACCURACY;
      af[0].modifier = 2;
      af[0].duration = 6;

      af[1].location = APPLY_WILL;
      af[1].modifier = 1;
      af[1].duration = 6;

      accum_duration = TRUE;
      to_vict        = "You feel righteous.";
      break;

    case SPELL_BLINDNESS:
      if (char_mob_flagged(victim, MOB_NOBLIND)) {
        send_to_char(ch, "You fail.\r\n");
        return;
      }

      af[0].location  = APPLY_ACCURACY;
      af[0].modifier  = -4;
      af[0].duration  = 2;
      af[0].bitvector = AFF_BLIND;

      af[1].location  = APPLY_AC;
      af[1].modifier  = -4;
      af[1].duration  = 2;
      af[1].bitvector = AFF_BLIND;

      to_room = "$n seems to be blinded!";
      to_vict = "You have been blinded!";
      break;

    case SPELL_BANE:
      af[0].location  = APPLY_ACCURACY;
      af[0].duration  = 1 + (level / 2);
      af[0].modifier  = -1;
      af[0].bitvector = AFF_CURSE;

      af[1].location  = APPLY_WILL;
      af[1].duration  = 1 + (level / 2);
      af[1].modifier  = -1;
      af[1].bitvector = AFF_CURSE;

      accum_duration = TRUE;
      accum_affect   = TRUE;
      to_room        = "$n briefly glows red!";
      to_vict        = "You feel very uncomfortable.";
      break;

    case SPELL_BESTOW_CURSE:
      af[0].location  = APPLY_STR;
      af[0].duration  = -1;
      af[0].modifier  = -6;
      af[0].bitvector = AFF_CURSE;

      af[1].location  = APPLY_ACCURACY;
      af[1].duration  = -1;
      af[1].modifier  = -4;
      af[1].bitvector = AFF_CURSE;

      accum_duration = FALSE;
      accum_affect   = FALSE;
      to_room        = "$n briefly glows red!";
      to_vict        = "You feel very uncomfortable.";
      break;

    case SPELL_DETECT_ALIGN:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_DETECT_ALIGN;
      accum_duration  = TRUE;
      to_vict         = "Your eyes tingle.";
      break;

    case SPELL_SEE_INVIS:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_DETECT_INVIS;
      accum_duration  = TRUE;
      to_vict         = "Your eyes tingle.";
      break;

    case SPELL_DETECT_MAGIC:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_DETECT_MAGIC;
      accum_duration  = TRUE;
      to_vict         = "Your eyes tingle.";
      break;

    case SPELL_FAERIE_FIRE:
      af[0].location = APPLY_AC;
      af[0].modifier = -1; /*should make target easier to hit */
      af[0].duration = 3;
      accum_duration = FALSE;
      to_vict        = "Your body flickers with a purplish light.";
      to_room        = "$n's body flickers with with a purplish light.";
      break;

    case SPELL_DARKVISION:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_INFRAVISION;
      accum_duration  = TRUE;
      to_vict         = "Your eyes glow red.";
      to_room         = "$n's eyes glow red.";
      break;

    case SPELL_INVISIBLE:
      if (!victim)
        victim = ch;

      af[0].duration  = 12 + (level / 4);
      af[0].modifier  = 4;
      af[0].location  = APPLY_AC;
      af[0].bitvector = AFF_INVISIBLE;
      accum_duration  = TRUE;
      to_vict         = "You vanish.";
      to_room         = "$n slowly fades out of existence.";
      break;

    case SPELL_POISON:

      af[0].location  = APPLY_STR;
      af[0].duration  = level;
      af[0].modifier  = -2;
      af[0].bitvector = AFF_POISON;
      to_vict         = "You feel very sick.";
      to_room         = "$n gets violently ill!";
      break;

    case SPELL_PROT_FROM_EVIL:
      af[0].duration  = 24;
      af[0].bitvector = AFF_PROTECT_GOOD;
      accum_duration  = TRUE;
      to_vict         = "You feel invulnerable!";
      break;

    case SPELL_SANCTUARY:
      af[0].duration  = 4;
      af[0].bitvector = AFF_SANCTUARY;

      accum_duration = TRUE;
      to_vict        = "A white aura momentarily surrounds you.";
      to_room        = "$n is surrounded by a white aura.";
      break;

    case SPELL_SLEEP:
      if (!CONFIG_PK_ALLOWED && !char_is_npc(ch) && !char_is_npc(victim))
        return;
      if (char_mob_flagged(victim, MOB_NOSLEEP))
        return;

      af[0].duration  = 4 + (level / 4);
      af[0].bitvector = AFF_SLEEP;

      if (char_get_position(victim) > POS_SLEEPING) {
        send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
        act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
        char_set_position(victim, POS_SLEEPING);
      }
      break;

    case SPELL_HAYASA:
      if (!CONFIG_PK_ALLOWED && !char_is_npc(ch) && !char_is_npc(victim))
        return;
      if (char_mob_flagged(victim, MOB_NOSLEEP))
        return;

      af[0].duration  = 4 + (level / 4);
      af[0].bitvector = AFF_SLEEP;

      if (char_get_position(victim) > POS_SLEEPING) {
        send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
        act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
        char_set_position(victim, POS_SLEEPING);
      }
      break;

    case SPELL_BULL_STRENGTH:
      af[0].location = APPLY_STR;
      af[0].duration = level;
      af[0].modifier = 1 + (rand_number(1, 4));
      accum_duration = FALSE;
      accum_affect   = FALSE;
      to_vict        = "You feel stronger!";
      break;

    case SPELL_SENSE_LIFE:
      to_vict         = "Your feel your awareness improve.";
      af[0].duration  = level;
      af[0].bitvector = AFF_SENSE_LIFE;
      accum_duration  = TRUE;
      break;

    case SPELL_WATERWALK:
      af[0].duration  = 24;
      af[0].bitvector = AFF_WATERWALK;
      accum_duration  = TRUE;
      to_vict         = "You feel webbing between your toes.";
      break;

    case SPELL_STONESKIN:
      af[0].duration = 1 * char_get_level(ch);
      accum_duration = FALSE;
      to_vict        = "Your skin hardens into stone!";
      break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (char_is_npc(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (char_aff_flagged(victim, af[i].bitvector)) {
        send_to_char(ch, "%s", CONFIG_NOEFFECT);
        return;
      }

  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_affect)) {
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affect_join(victim, af + i, accum_duration, FALSE, accum_affect, FALSE);

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, struct char_data *ch,
                        struct char_data *tch, int spellnum)
{
  switch (spellnum) {
    case SPELL_MASS_HEAL:
      mag_points(level, ch, tch, SPELL_HEAL);
      break;
    case SPELL_GROUP_ARMOR:
      mag_affects(level, ch, tch, SPELL_MAGE_ARMOR);
      break;
    case SPELL_GROUP_RECALL:
      spell_recall(level, ch, tch, NULL, NULL);
      break;
  }
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
void mag_groups(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!char_aff_flagged(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch    = f->follower;
    if (char_get_in_room(tch) != char_get_in_room(ch))
      continue;
    if (!char_aff_flagged(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum);
  }

  if ((k != ch) && char_aff_flagged(k, AFF_GROUP))
    perform_mag_groups(level, ch, k, spellnum);
  perform_mag_groups(level, ch, ch, spellnum);
}

/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented.
 */
void mag_masses(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *tch_next;

  for (tch = world[char_get_in_room(ch)].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}

/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;

  if (ch == NULL)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
    case SPELL_EARTHQUAKE:
      to_char = "You gesture and the earth begins to shake all around you!";
      to_room = "$n gracefully gestures and the earth begins to shake violently!";
      break;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[char_get_in_room(ch)].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     */

    if (tch == ch)
      continue;
    if (char_adm_flagged(tch, ADM_NODAMAGE))
      continue;
    if (!CONFIG_PK_ALLOWED && !char_is_npc(ch) && !char_is_npc(tch))
      continue;
    if (!char_is_npc(ch) && char_is_npc(tch) && char_aff_flagged(tch, AFF_CHARM))
      continue;

    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum);
  }
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
    {monsum_list_lg_1, monsum_list_ng_1, monsum_list_cg_1,
     monsum_list_ln_1, monsum_list_nn_1, monsum_list_cn_1,
     monsum_list_le_1, monsum_list_ne_1, monsum_list_ce_1},
    {monsum_list_lg_2, monsum_list_ng_2, monsum_list_cg_2,
     monsum_list_ln_2, monsum_list_nn_2, monsum_list_cn_2,
     monsum_list_le_2, monsum_list_ne_2, monsum_list_ce_2},
    {monsum_list_lg_3, monsum_list_ng_3, monsum_list_cg_3,
     monsum_list_ln_3, monsum_list_nn_3, monsum_list_cn_3,
     monsum_list_le_3, monsum_list_ne_3, monsum_list_ce_3},
    {monsum_list_lg_4, monsum_list_ng_4, monsum_list_cg_4,
     monsum_list_ln_4, monsum_list_nn_4, monsum_list_cn_4,
     monsum_list_le_4, monsum_list_ne_4, monsum_list_ce_4},
    {monsum_list_lg_5, monsum_list_ng_5, monsum_list_cg_5,
     monsum_list_ln_5, monsum_list_nn_5, monsum_list_cn_5,
     monsum_list_le_5, monsum_list_ne_5, monsum_list_ce_5},
    {monsum_list_lg_6, monsum_list_ng_6, monsum_list_cg_6,
     monsum_list_ln_6, monsum_list_nn_6, monsum_list_cn_6,
     monsum_list_le_6, monsum_list_ne_6, monsum_list_ce_6},
    {monsum_list_lg_7, monsum_list_ng_7, monsum_list_cg_7,
     monsum_list_ln_7, monsum_list_nn_7, monsum_list_cn_7,
     monsum_list_le_7, monsum_list_ne_7, monsum_list_ce_7},
    {monsum_list_lg_8, monsum_list_ng_8, monsum_list_cg_8,
     monsum_list_ln_8, monsum_list_nn_8, monsum_list_cn_8,
     monsum_list_le_8, monsum_list_ne_8, monsum_list_ce_8},
    {monsum_list_lg_9, monsum_list_ng_9, monsum_list_cg_9,
     monsum_list_ln_9, monsum_list_nn_9, monsum_list_cn_9,
     monsum_list_le_9, monsum_list_ne_9, monsum_list_ce_9}
};

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
    "\r\n",
    "$n animates a corpse!",
    "$n summons extraplanar assistance!"};

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE 20 /* Only one for now. */
#define MOB_ZOMBIE         11
#define MOB_AERIALSERVANT  19

void mag_summons(int level, struct char_data *ch, struct obj_data *obj, int spellnum, char *arg)
{
}

void mag_points(int level, struct char_data *ch, struct char_data *victim,
                int spellnum)
{
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
      if (char_aff_flagged(ch, AFF_CDEATH)) {
        send_to_char(ch, "Your nerves settle slightly\r\n");
      }
      break;
    case SPELL_SENSU:
      if (GET_COND(victim, HUNGER) > -1) {
        GET_COND(victim, HUNGER) = 48;
      }
      restore(victim, true);
      break;

    case ART_WHOLENESS_OF_BODY:
      healing = GET_MAX_HIT(victim) - GET_HIT(victim);
      healing = MAX(0, healing);
      tmp     = char_get_ki(ch) / 2;
      if (tmp > healing)
        tmp = healing;
      else {
        healing = tmp;
      }
      char_set_ki(ch, char_get_ki(ch) - (tmp * 2));
      break;
  }
  update_pos(victim);
}

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
                   int spellnum)
{
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
      spell   = SPELL_BLINDNESS;
      to_vict = "Your vision returns!";
      to_room = "There's a momentary gleam in $n's eyes.";
      break;
    case SPELL_NEUTRALIZE_POISON:
      spell   = SPELL_POISON;
      to_vict = "A warm feeling runs through your body!";
      to_room = "$n looks better.";
      break;
    case SPELL_REMOVE_CURSE:
      spell   = SPELL_BESTOW_CURSE;
      to_vict = "You don't feel so unlucky.";
      break;
    default:
      return;
  }

  if (!affected_by_spell(victim, spell)) {
    if (msg_not_affected)
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
    case SPELL_BLESS:
      if (!obj_extra_flagged(obj, ITEM_BLESS) &&
          (obj_get_weight(obj) <= 5 * level)) {
        obj_extra_set(obj, ITEM_BLESS, true);
        to_char = "$p glows briefly.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!obj_extra_flagged(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        obj_extra_set(obj, ITEM_INVISIBLE, true);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((obj_get_type_flag(obj) == ITEM_DRINKCON) ||
           (obj_get_type_flag(obj) == ITEM_FOUNTAIN) ||
           (obj_get_type_flag(obj) == ITEM_FOOD)) &&
          !obj_get_value(obj, VAL_FOOD_POISON)) {
        obj_set_value(obj, VAL_FOOD_POISON, 1);
        to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (obj_extra_flagged(obj, ITEM_NODROP)) {
        obj_extra_set(obj, ITEM_NODROP, false);
        if (obj_get_type_flag(obj) == ITEM_WEAPON)
          obj_set_value(obj, VAL_WEAPON_DAMSIZE, obj_get_value(obj, VAL_WEAPON_DAMSIZE) + 1);
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_NEUTRALIZE_POISON:
      if (((obj_get_type_flag(obj) == ITEM_DRINKCON) ||
           (obj_get_type_flag(obj) == ITEM_FOUNTAIN) ||
           (obj_get_type_flag(obj) == ITEM_FOOD)) &&
          obj_get_value(obj, VAL_FOOD_POISON)) {
        obj_set_value(obj, VAL_FOOD_POISON, 0);
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

void mag_creations(int level, struct char_data *ch, int spellnum)
{
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
    mud_log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
            spellnum, z);
    return;
  }
  add_unique_id(tobj);
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

void mag_affectsv(int level, struct char_data *ch, struct char_data *victim,
                  int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;

  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type      = spellnum;
    af[i].bitvector = 0;
    af[i].modifier  = 0;
    af[i].location  = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, char_get_aff_abils_intel(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  switch (spellnum) {
    case SPELL_PARALYZE:
      af[0].duration  = level / 2;
      af[0].bitvector = AFF_PARALYZE;
      accum_duration  = FALSE;
      to_vict         = "You feel your limbs freeze!";
      to_room         = "$n suddenly freezes in place!";
      break;
    case ART_STUNNING_FIST:
      af[0].duration  = 1;
      af[0].bitvector = AFF_STUNNED;
      accum_duration  = FALSE;
      to_vict         = "You are in a stunned daze!";
      to_room         = "$n is stunned.";
      break;
    case ART_EMPTY_BODY:
      af[0].duration  = char_get_ki(ch) / 10;
      af[0].bitvector = AFF_ETHEREAL;
      accum_duration  = FALSE;
      to_vict         = "You switch to the ethereal plane.";
      to_room         = "$n disappears.";
      break;
    case ART_QUIVERING_PALM:
      if (char_get_level(ch) <= char_get_level(victim)) {
        send_to_char(ch, "They are too high level for that.\r\n");
        return;
      }
      af[0].duration  = MAX(6, 20 - level);
      af[0].bitvector = AFF_CDEATH;
      accum_duration  = FALSE;
      to_vict         = "You feel death closing in.";
      break;
    case SPELL_RESISTANCE:
      af[0].duration = 12;
      af[0].location = APPLY_ALLSAVES;
      af[0].modifier = 1;
      accum_duration = FALSE;
      to_vict        = "You glow briefly with a silvery light.";
      to_room        = "$n glows briefly with a silvery light.";
      break;
    case SPELL_DAZE:
      if (char_get_race_level(victim) < 5)
        af[0].bitvector = AFF_NEXTNOACTION;
      accum_duration = FALSE;
      to_vict        = "You are struck dumb by a flash of bright light!";
      to_room        = "$n is struck dumb by a flash of bright light!";
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
      af[0].duration  = level;
      af[0].bitvector = AFF_SUMMONED;
      accum_duration  = FALSE;
      to_vict         = "You are summoned to assist $N!";
      to_room         = "$n appears, ready for action.";
      break;

    case SPELL_FLARE:
      if (char_mob_flagged(victim, MOB_NOBLIND)) {
        send_to_char(ch, "You fail.\r\n");
        return;
      }
      af[0].location  = APPLY_ACCURACY;
      af[0].modifier  = -1;
      af[0].duration  = 2;
      af[0].bitvector = AFF_BLIND;

      to_room = "$n seems to be dazzled!";
      to_vict = "You have been dazzled!";
      break;

    case SPELL_FIRE_SHIELD:
      af[0].duration  = level * 5;
      af[0].bitvector = AFF_FIRE_SHIELD;
      to_room         = "$n is engulfed in a firey shield!";
      to_vict         = "You are engulfed in a firey shield!";
      break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (char_is_npc(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (char_aff_flagged(victim, af[i].bitvector)) {
        send_to_char(ch, "%s", CONFIG_NOEFFECT);
        return;
      }
  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_affect)) {
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}
