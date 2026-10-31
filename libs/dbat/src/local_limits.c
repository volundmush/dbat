/* ************************************************************************
 *   File: limits.c                                      Part of CircleMUD *
 *  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "dbat/config.h"
#include "dbat/local_limits.h"
#include "dbat/utils.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/dg_comm.h"
#include "dbat/act.other.h"
#include "dbat/alias.h"
#include "dbat/act.item.h"
#include "dbat/vehicles.h"
#include "dbat/act.movement.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/objsave.h"
#include "dbat/handler.h"
#include "dbat/dg_scripts.h"
#include "dbat/character_utils.h"

#include <unistd.h>

/* local defines */
#define sick_fail 2

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

static void barrier_shed(struct char_data *ch)
{

  if (!char_aff_flagged(ch, AFF_SANCTUARY))
  {
    return;
  }

  if (char_get_skill(ch, SKILL_AQUA_BARRIER) > 0)
  {
    return;
  }

  int chance = axion_dice(0), barrier = char_get_skill(ch, SKILL_BARRIER), concentrate = char_get_skill(ch, SKILL_CONCENTRATION);
  double rate = 0.3;

  if (barrier >= 100)
  {
    rate = 0.01;
  }
  else if (barrier >= 95)
  {
    rate = 0.02;
  }
  else if (barrier >= 90)
  {
    rate = 0.04;
  }
  else if (barrier >= 80)
  {
    rate = 0.08;
  }
  else if (barrier >= 70)
  {
    rate = 0.10;
  }
  else if (barrier >= 60)
  {
    rate = 0.15;
  }
  else if (barrier >= 50)
  {
    rate = 0.20;
  }
  else if (barrier >= 40)
  {
    rate = 0.25;
  }
  else if (barrier >= 30)
  {
    rate = 0.27;
  }
  else if (barrier >= 20)
  {
    rate = 0.29;
  }

  int64_t loss = (long double)(char_get_barrier(ch)) * rate, recharge = 0;

  if (concentrate >= chance)
  {
    recharge = loss * 0.5;
  }

  char_set_barrier(ch, char_get_barrier(ch) - (loss));

  if (char_get_barrier(ch) <= 0)
  {
    char_set_barrier(ch, 0);
    act("@cYour barrier disappears.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@c's barrier disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    act("@cYour barrier loses some energy.@n", TRUE, ch, 0, 0, TO_CHAR);
    send_to_char(ch, "@D[@C%s@D]@n\r\n", add_commas(loss));
    act("@c$n@c's barrier sends some sparks into the air as it seems to get a bit weaker.@n", TRUE, ch, 0, 0, TO_ROOM);
  }

  if (recharge > 0 && (getCurKI(ch)) < GET_MAX_MANA(ch))
  {
    incCurKI(ch, recharge, true);
    send_to_char(ch, "@CYou reabsorb some of the energy lost into your body!@n\r\n");
  }
}

/* If they have the Healthy trait then they have a chance to lose each of these */
static void healthy_check(struct char_data *ch)
{

  if (!char_get_bonuses(ch, BONUS_HEALTHY) || char_get_position(ch) != POS_SLEEPING)
  {
    return;
  }

  int chance = 70, roll = rand_number(1, 100), change = FALSE;

  if (char_aff_flagged(ch, AFF_SHOCKED) && roll >= chance)
  {
    char_aff_set(ch, AFF_SHOCKED, false);
    change = TRUE;
  }
  if (char_aff_flagged(ch, AFF_MBREAK) && roll >= chance)
  {
    char_aff_set(ch, AFF_MBREAK, false);
    change = TRUE;
  }
  if (is_affected(ch, AFF_WITHER) && roll >= chance)
  {
    remove_affect(ch, AFF_WITHER);
    save_char(ch);
    change = TRUE;
  }
  if (char_aff_flagged(ch, AFF_CURSE) && roll >= chance)
  {
    char_aff_set(ch, AFF_CURSE, false);
    change = TRUE;
  }
  if (char_aff_flagged(ch, AFF_POISON) && roll >= chance)
  {
    null_affect(ch, AFF_POISON);
    change = TRUE;
  }
  if (char_is_affected(ch, AFF_PARALYZE) && roll >= chance)
  {
    null_affect(ch, AFF_PARALYZE);
    change = TRUE;
  }
  if (char_is_affected(ch, AFF_PARA) && roll >= chance)
  {
    null_affect(ch, AFF_PARA);
    change = TRUE;
  }
  if (char_aff_flagged(ch, AFF_BLIND) && roll >= chance)
  {
    null_affect(ch, AFF_BLIND);
    change = TRUE;
  }
  if (is_affected(ch, AFF_HYDROZAP) && roll >= chance)
  {
    remove_affect(ch, AFF_HYDROZAP);
    save_char(ch);
    change = TRUE;
  }
  if (char_aff_flagged(ch, AFF_KNOCKED) && roll >= chance)
  {
    char_aff_set(ch, AFF_KNOCKED, false);
    char_set_position(ch, POS_SITTING);
    change = TRUE;
  }
  if (change == TRUE)
  {
    send_to_char(ch, "@CYou feel your body recover from all its ailments!@n\r\n");
  }
  return;
}

static int wearing_stardust(struct char_data *ch)
{

  int count = 0, i;

  for (i = 1; i < NUM_WEARS; i++)
  {
    if (char_get_equipment(ch, i))
    {
      struct obj_data *obj = char_get_equipment(ch, i);
      switch (obj_get_vnum(obj))
      {
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
static int64_t mana_gain(struct char_data *ch)
{
  int64_t gain = 0;

  if (char_is_npc(ch))
  {
    /* Neat and fast */
    gain = GET_MAX_MANA(ch) / 70;
  }
  else
  {
    if (rroom_flagged(char_get_in_room(ch), ROOM_REGEN) || (char_get_bonuses(ch, BONUS_DESTROYER) > 0 && rroom_get_dmg(char_get_in_room(ch)) >= 75))
    {
      if (char_is_konatsu(ch))
      {
        gain = GET_MAX_MANA(ch) / 12;
      }
      if (char_is_mutant(ch))
      {
        gain = GET_MAX_MANA(ch) / 11;
      }
      if (char_is_arlian(ch))
      {
        gain = GET_MAX_MANA(ch) / 30;
      }
      if (!char_is_konatsu(ch) && !char_is_mutant(ch))
      {
        gain = GET_MAX_MANA(ch) / 10;
      }
    }
    else if (!rroom_flagged(char_get_in_room(ch), ROOM_REGEN))
    {
      if (char_is_konatsu(ch))
      {
        gain = GET_MAX_MANA(ch) / 15;
      }
      if (char_is_mutant(ch))
      {
        gain = GET_MAX_MANA(ch) / 13;
      }
      if (!char_is_konatsu(ch) && !char_is_mutant(ch))
      {
        gain = GET_MAX_MANA(ch) / 12;
      }
      if (rroom_flagged(char_get_in_room(ch), ROOM_BEDROOM))
      {
        gain += gain * 0.25;
      }
      if (char_is_arlian(ch))
      {
        gain = GET_MAX_MANA(ch) / 40;
      }
    }
    /* Position calculations    */
    switch (char_get_position(ch))
    {
    case POS_STANDING:
      if (!char_is_hoshijin(ch) || (char_is_hoshijin(ch) && char_get_starphase(ch) <= 0))
      {
        gain = gain / 4;
      }
      else
      {
        gain += (gain / 2);
      }
      break;
    case POS_FIGHTING:
      gain = gain / 4;
      break;
    case POS_SLEEPING:
      if (!char_get_sits(ch))
      {
        gain *= 2;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090)
      {
        gain *= 3;
        gain += gain * 0.1;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19092)
      {
        gain *= 3;
        gain += gain * 0.3;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain *= 3;
      }
      break;
    case POS_RESTING:
      if (!char_get_sits(ch))
      {
        gain += (gain / 2);
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090 && !char_is_arlian(ch))
      {
        gain *= 2;
        gain += gain * 0.1;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19092 && !char_is_arlian(ch))
      {
        gain *= 2;
        gain += gain * 0.3;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain *= 2;
      }
      break;
    case POS_SITTING:
      if (!char_get_sits(ch))
      {
        gain += (gain / 4);
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090)
      {
        gain += gain * 0.6;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19092)
      {
        gain += gain * 0.8;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain += gain * 0.5;
      }
      break;
    }
  }

  if (char_get_in_room(ch) != NOWHERE)
  {
    if (cook_element(char_get_in_room(ch)) == 1)
    {
      gain += (gain * 0.2);
    }
  }

  if (char_is_arlian(ch) && char_is_female(ch) && char_is_outside(ch))
  {
    gain *= 4;
  }

  if (char_is_kanassan(ch) && weather_info.sky == SKY_RAINING && char_is_outside(ch))
  {
    gain += gain * 0.1;
  }
  if (char_is_kanassan(ch) && rroom_sunken(char_get_in_room(ch)))
  {
    gain *= 16;
  }

  if (char_is_hoshijin(ch) && char_get_starphase(ch) > 0)
  {
    gain *= 2;
  }

  if (char_plr_flagged(ch, PLR_HEALT) && char_get_sits(ch) != NULL)
  {
    gain *= 20;
  }
  if (is_affected(ch, AFF_SPECIAL_POSE) && axion_dice(0) > char_get_skill(ch, SKILL_POSE))
  {
    remove_affect(ch, AFF_SPECIAL_POSE);
    send_to_char(ch, "You feel slightly less confident now.\r\n");
    save_char(ch);
  }
  if (is_affected(ch, AFF_HYDROZAP) && rand_number(1, 4) >= 4)
  {
    remove_affect(ch, AFF_HYDROZAP);
    save_char(ch);
  }

  if (char_get_skill(ch, SKILL_CONCENTRATION) >= 100)
  {
    gain += gain / 2;
  }
  else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 75)
  {
    gain += gain / 4;
  }
  else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 50)
  {
    gain += gain / 6;
  }
  else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 25)
  {
    gain += gain / 8;
  }
  else if (char_get_skill(ch, SKILL_CONCENTRATION) < 25 && char_get_skill(ch, SKILL_CONCENTRATION) > 0)
  {
    gain += gain / 10;
  }

  if (char_aff_flagged(ch, AFF_BLESS))
  {
    gain *= 2;
  }
  if (char_aff_flagged(ch, AFF_CURSE))
  {
    gain /= 5;
  }

  if (char_get_foodr(ch) > 0 && rand_number(1, 2) == 2)
  {
    char_set_foodr(ch, char_get_foodr(ch) - (1));
  }

  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_HINTS) && rand_number(1, 5) == 5)
  {
    hint_system(ch, 0);
  }

  if (char_aff_flagged(ch, AFF_POISON))
    gain /= 4;

  if (cook_element(char_get_in_room(ch)) == 1)
    gain *= 2;

  return (gain);
}

/* Hitpoint gain pr. game hour */
int64_t hit_gain(struct char_data *ch)
{
  int64_t gain = 0;

  if (char_is_npc(ch))
  {
    /* Neat and fast */
    gain = GET_MAX_HIT(ch) / 70;
  }
  else
  {
    if (rroom_flagged(char_get_in_room(ch), ROOM_REGEN) || (char_get_bonuses(ch, BONUS_DESTROYER) > 0 && rroom_get_dmg(char_get_in_room(ch)) >= 75))
    {
      if (char_is_human(ch))
      {
        gain = GET_MAX_HIT(ch) / 20;
      }
      if (char_is_arlian(ch))
      {
        gain = GET_MAX_HIT(ch) / 30;
      }
      if (char_is_namek(ch))
      {
        gain = GET_MAX_HIT(ch) / 2;
      }
      if (char_is_mutant(ch))
      {
        gain = GET_MAX_HIT(ch) / 11;
      }
      if (!char_is_human(ch) && !char_is_namek(ch) && !char_is_mutant(ch))
      {
        gain = GET_MAX_HIT(ch) / 10;
      }
    }
    else if (!rroom_flagged(char_get_in_room(ch), ROOM_REGEN))
    {
      if (char_is_human(ch))
      {
        gain = GET_MAX_HIT(ch) / 30;
      }
      if (char_is_namek(ch))
      {
        gain = GET_MAX_HIT(ch) / 4;
      }
      if (char_is_mutant(ch))
      {
        gain = GET_MAX_HIT(ch) / 16;
      }
      if (char_is_arlian(ch))
      {
        gain = GET_MAX_HIT(ch) / 40;
      }
      if (!char_is_human(ch) && !char_is_namek(ch) && !char_is_mutant(ch))
      {
        gain = GET_MAX_HIT(ch) / 15;
      }
      if (rroom_flagged(char_get_in_room(ch), ROOM_BEDROOM))
      {
        gain += gain * 0.25;
      }
    }

    /* Position calculations    */
    switch (char_get_position(ch))
    {
    case POS_STANDING:
      if (!char_is_hoshijin(ch) || (char_is_hoshijin(ch) && char_get_starphase(ch) <= 0))
      {
        gain = gain / 4;
      }
      else if (char_is_android(ch) && char_plr_flagged(ch, PLR_ABSORB))
      {
        gain = gain / 3;
      }
      else
      {
        gain += (gain / 2);
      }
      break;
    case POS_FIGHTING:
      gain = gain / 4;
      break;
    case POS_SLEEPING:
      if (char_is_arlian(ch))
      {
        gain *= 3;
      }
      else if (!char_get_sits(ch))
      {
        gain *= 2;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090)
      {
        gain *= 3;
        gain += gain * 0.1;
      }
      else if (char_get_sits(ch))
      {
        gain *= 3;
      }
      break;
    case POS_RESTING:
      if (!char_get_sits(ch))
      {
        gain += (gain / 2);
      }
      else if (char_is_android(ch) && char_plr_flagged(ch, PLR_ABSORB))
      {
        gain = gain * 1.5;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090 && !char_is_arlian(ch))
      {
        gain += gain * 1.1;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain *= 2;
      }
      break;
    case POS_SITTING:
      if (!char_get_sits(ch))
      {
        gain += (gain / 4);
      }
      else if (char_is_android(ch) && char_plr_flagged(ch, PLR_ABSORB))
      {
        gain = gain * 0.5;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090 && !char_is_arlian(ch))
      {
        gain += gain * 0.6;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain += (gain * 0.5);
      }
    }
  }
  healthy_check(ch);

  if (char_is_arlian(ch) && char_is_female(ch) && char_is_outside(ch))
  {
    gain *= 4;
  }

  if (char_is_kanassan(ch) && weather_info.sky == SKY_RAINING && char_is_outside(ch))
  {
    gain += gain * 0.1;
  }
  if (char_is_kanassan(ch) && rroom_sunken(char_get_in_room(ch)))
  {
    gain *= 16;
  }

  if (char_is_hoshijin(ch) && char_get_starphase(ch) > 0)
  {
    gain *= 2;
  }
  if (char_plr_flagged(ch, PLR_HEALT) && char_get_sits(ch) != NULL)
  {
    gain *= 20;
  }

  if (char_aff_flagged(ch, AFF_BLESS))
  {
    gain *= 2;
  }
  if (char_aff_flagged(ch, AFF_CURSE))
  {
    gain /= 5;
  }

  /* Fury Mode Loss for halfbreeds */

  if (char_plr_flagged(ch, PLR_FURY))
  {
    send_to_char(ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
    char_plr_set(ch, PLR_FURY, false);
  }

  /* Fury Mode Loss for halfbreeds */

  if (char_aff_flagged(ch, AFF_POISON))
    gain /= 4;
  if (cook_element(char_get_in_room(ch)) == 1)
    gain *= 2;

  if (!char_is_npc(ch))
  {
    if (char_plr_flagged(ch, PLR_ABSORB))
    {
      gain = gain / 8;
    }
  }

  if (char_get_regen(ch) > 0)
  {
    gain += (gain * 0.01) * char_get_regen(ch);
  }

  return (gain);
}

/* move gain pr. game hour */
static int64_t move_gain(struct char_data *ch)
{
  int64_t gain = 0;

  if (char_is_npc(ch))
  {
    /* Neat and fast */
    gain = GET_MAX_MOVE(ch) / 70;
  }
  else
  {
    if (rroom_flagged(char_get_in_room(ch), ROOM_REGEN) || (char_get_bonuses(ch, BONUS_DESTROYER) > 0 && rroom_get_dmg(char_get_in_room(ch)) >= 75))
    {
      if (char_is_mutant(ch))
      {
        gain = GET_MAX_MOVE(ch) / 7;
      }
      if (char_is_arlian(ch))
      {
        gain = GET_MAX_MOVE(ch) / 4;
      }
      if (!char_is_mutant(ch))
      {
        gain = GET_MAX_MOVE(ch) / 6;
      }
    }
    else if (!rroom_flagged(char_get_in_room(ch), ROOM_REGEN))
    {
      if (char_is_mutant(ch))
      {
        gain = GET_MAX_MOVE(ch) / 9;
      }
      if (!char_is_mutant(ch))
      {
        gain = GET_MAX_MOVE(ch) / 8;
      }
      if (rroom_flagged(char_get_in_room(ch), ROOM_BEDROOM))
      {
        gain += gain * 0.25;
      }
    }

    /* Position calculations    */
    switch (char_get_position(ch))
    {
    case POS_STANDING:
      if (!char_is_hoshijin(ch) || (char_is_hoshijin(ch) && char_get_starphase(ch) <= 0))
      {
        gain = gain / 4;
      }
      else
      {
        gain += (gain / 2);
      }
      break;
    case POS_FIGHTING:
      gain = gain / 4;
      break;
    case POS_SLEEPING:
      if (!char_get_sits(ch))
      {
        gain *= 2;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090 && !char_is_arlian(ch))
      {
        gain *= 3;
        gain += gain * 0.1;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19091 && !char_is_arlian(ch))
      {
        gain *= 3;
        gain += gain * 0.3;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain *= 3;
      }
      break;
    case POS_RESTING:
      if (!char_get_sits(ch))
      {
        gain += (gain / 2);
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090 && !char_is_arlian(ch))
      {
        gain += gain * 1.1;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19091 && !char_is_arlian(ch))
      {
        gain += gain * 1.3;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain += gain;
      }
      break;
    case POS_SITTING:
      if (!char_get_sits(ch))
      {
        gain += (gain / 4);
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19090 && !char_is_arlian(ch))
      {
        gain += gain * 0.6;
      }
      else if (obj_get_vnum(char_get_sits(ch)) == 19091 && !char_is_arlian(ch))
      {
        gain += gain * 0.8;
      }
      else if (char_get_sits(ch) || char_is_arlian(ch))
      {
        gain += (gain / 2);
      }
    }
  }

  if (char_is_arlian(ch) && char_is_female(ch) && char_is_outside(ch))
  {
    gain *= 2;
  }

  if (char_is_namek(ch))
  {
    gain = gain * 0.5;
  }

  if (char_is_kanassan(ch) && weather_info.sky == SKY_RAINING && char_is_outside(ch))
  {
    gain += gain * 0.1;
  }
  if (char_is_kanassan(ch) && rroom_sunken(char_get_in_room(ch)))
  {
    gain *= 16;
  }

  if (char_is_hoshijin(ch) && char_get_starphase(ch) > 0)
  {
    gain *= 2;
  }
  if (char_plr_flagged(ch, PLR_HEALT) && char_get_sits(ch) != NULL)
  {
    gain *= 20;
  }

  if (char_aff_flagged(ch, AFF_BLESS))
  {
    gain *= 2;
  }
  if (char_aff_flagged(ch, AFF_CURSE))
  {
    gain /= 5;
  }

  if (char_aff_flagged(ch, AFF_POISON))
    gain /= 4;

  if (!calcGravCost(ch, 0))
  {
    send_to_char(ch, "This gravity is wearing you out!\r\n");
    gain /= 4;
  }

  if (rroom_flagged(char_get_in_room(ch), ROOM_AURA))
  {
    gain = GET_MAX_MOVE(ch) - (getCurST(ch));
  }
  if (cook_element(char_get_in_room(ch)) == 1)
    gain *= 2;

  if (char_get_regen(ch) > 0)
  {
    gain += (gain * 0.01) * char_get_regen(ch);
  }

  return (gain);
}

static void update_flags(struct char_data *ch)
{
  if (ch == NULL)
  {
    send_to_imm("ERROR: Empty ch variable sent to update_flags.");
    return;
  }

  if (char_get_bonuses(ch, BONUS_LATE) && char_get_position(ch) == POS_SLEEPING && rand_number(1, 3) == 3)
  {
    if (GET_HIT(ch) >= (getEffMaxPL(ch)) && (getCurST(ch)) >= GET_MAX_MOVE(ch) && (getCurKI(ch)) >= GET_MAX_MANA(ch))
    {
      send_to_char(ch, "You FINALLY wake up.\r\n");
      act("$n wakes up.", TRUE, ch, 0, 0, TO_ROOM);
      char_set_position(ch, POS_SITTING);
    }
  }

  if (char_aff_flagged(ch, AFF_KNOCKED) && !char_get_fighting(ch))
  {
    cureStatusKnockedOut(ch, true);
  }

  barrier_shed(ch);

  if (char_aff_flagged(ch, AFF_FIRESHIELD) && !char_get_fighting(ch) && rand_number(1, 101) > char_get_skill(ch, SKILL_FIRESHIELD))
  {
    send_to_char(ch, "Your fireshield disappears.\r\n");
    char_aff_set(ch, AFF_FIRESHIELD, false);
  }
  if (char_aff_flagged(ch, AFF_ZANZOKEN) && !char_get_fighting(ch) && rand_number(1, 3) == 2)
  {
    send_to_char(ch, "You lose concentration and no longer are ready to zanzoken.\r\n");
    char_aff_set(ch, AFF_ZANZOKEN, false);
  }
  if (char_aff_flagged(ch, AFF_ENSNARED) && rand_number(1, 3) == 2)
  {
    send_to_char(ch, "The silk ensnaring your arms disolves enough for you to break it!\r\n");
    char_aff_set(ch, AFF_ENSNARED, false);
  }

  if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && char_plr_flagged(ch, PLR_TRANS1) && !char_plr_flagged(ch, PLR_FPSSJ))
  {
    char_set_absorbs(ch, char_get_absorbs(ch) + (1));
    if (char_get_absorbs(ch) >= 300)
    {
      send_to_char(ch, "You have mastered the base Super Saiyan transformation and achieved Full Power Super Saiyan! Super Saiyan First can now be maintained effortlessly.\r\n");
      char_plr_set(ch, PLR_FPSSJ, true);
      char_set_absorbs(ch, 0);
    }
  }

  if (!char_is_npc(ch) && !char_plr_flagged(ch, PLR_STAIL) && !char_plr_flagged(ch, PLR_NOGROW) && (char_is_saiyan(ch) || char_is_halfbreed(ch)))
  {
    if (RACIAL_PREF(ch) == 1 && rand_number(1, 50) >= 40)
    {
      char_set_tail_growth(ch, char_get_tail_growth(ch) + (1));
    }
    else if (RACIAL_PREF(ch) != 1 || char_is_saiyan(ch))
    {
      char_set_tail_growth(ch, char_get_tail_growth(ch) + (1));
    }
    if (char_get_tail_growth(ch) >= 10)
    {
      send_to_char(ch, "@wYour tail grows back.@n\r\n");
      act("$n@w's tail grows back.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_gain_tail(ch, true);
      char_set_tail_growth(ch, 0);
    }
  }
  if (!char_is_npc(ch) && !char_plr_flagged(ch, PLR_TAIL) && (char_is_icer(ch) || char_is_bio(ch)))
  {
    char_set_tail_growth(ch, char_get_tail_growth(ch) + (1));
    if (char_get_tail_growth(ch) >= 10)
    {
      send_to_char(ch, "@wYour tail grows back.@n\r\n");
      act("$n@w's tail grows back.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_gain_tail(ch, true);
      char_set_tail_growth(ch, 0);
    }
  }
  if (char_aff_flagged(ch, AFF_MBREAK) && rand_number(1, 3 + sick_fail) == 2)
  {
    send_to_char(ch, "@wYour mind is no longer in turmoil, you can charge ki again.@n\r\n");
    char_aff_set(ch, AFF_MBREAK, false);
    if (char_get_skill(ch, SKILL_TELEPATHY) <= 0 && rand_number(1, 2) == 2)
    {
      ch->real_abils.intel -= 1;
      ch->real_abils.wis -= 1;
      send_to_char(ch, "@RDue to the stress you've lost 1 Intelligence and Wisdom!@n\r\n");
      if (ch->real_abils.wis < 4)
        ch->real_abils.wis = 4;
      if (ch->real_abils.intel < 4)
        ch->real_abils.intel = 4;
    }
    else if (char_get_skill(ch, SKILL_TELEPATHY) <= 0 && rand_number(1, 20) == 1)
    {
      ch->real_abils.intel -= 1;
      ch->real_abils.wis -= 1;
      send_to_char(ch, "@RDue to the stress you've lost 1 Intelligence and Wisdom!@n\r\n");
      if (ch->real_abils.wis < 4)
        ch->real_abils.wis = 4;
      if (ch->real_abils.intel < 4)
        ch->real_abils.intel = 4;
    }
  }
  if (char_aff_flagged(ch, AFF_SHOCKED) && rand_number(1, 4) == 4)
  {
    send_to_char(ch, "@wYour mind is no longer shocked.@n\r\n");
    if (char_get_skill(ch, SKILL_TELEPATHY) > 0)
    {
      int skill = char_get_skill(ch, SKILL_TELEPATHY), stop = FALSE;
      improve_skill(ch, SKILL_TELEPATHY, 0);
      while (stop == FALSE)
      {
        if (rand_number(1, 8) == 5)
          stop = TRUE;
        else
          improve_skill(ch, SKILL_TELEPATHY, 0);
      }
      if (skill < char_get_skill(ch, SKILL_TELEPATHY))
        send_to_char(ch, "Your mental damage and recovery has taught you things about your own mind.\r\n");
    }
    char_aff_set(ch, AFF_SHOCKED, false);
  }
  if (char_aff_flagged(ch, AFF_FROZEN) && rand_number(1, 2) == 2)
  {
    send_to_char(ch, "@wYou realize you have thawed enough and break out of the ice holding you prisoner!\r\n");
    act("$n@W breaks out of the ice holding $m prisoner!", TRUE, ch, 0, 0, TO_ROOM);
    char_aff_set(ch, AFF_FROZEN, false);
  }
  if (is_affected(ch, AFF_WITHER) && rand_number(1, 6 + sick_fail) == 2)
  {
    send_to_char(ch, "@wYour body returns to normal and you beat the withering that plagued you.\r\n");
    act("$n@W's looks more fit now.", TRUE, ch, 0, 0, TO_ROOM);
    remove_affect(ch, AFF_WITHER);
    save_char(ch);
  }
  if (wearing_stardust(ch) == 1)
  {
    char_aff_set(ch, AFF_ZANZOKEN, true);
    send_to_char(ch, "The stardust armor blesses you with a free zanzoken when you next need it.\r\n");
  }
}

/* ki gain pr. game hour */
void set_title(struct char_data *ch, char *title)
{
  if (ch)
  {
    send_to_char(ch, "Title is disabled for the time being while Iovan works on a brand new and fancier title system.\r\n");
    return;
  }
  /*
   if (title == NULL) {
     title = class_desc_str(ch, 2, TRUE);
   }

   if (strlen(title) > MAX_TITLE_LENGTH)
     title[MAX_TITLE_LENGTH] = '\0';

   if (GET_TITLE(ch) != NULL)
     free(GET_TITLE(ch));

   GET_TITLE(ch) = strdup(title);
  */
}

void gain_level(struct char_data *ch, int whichclass)
{
  if (whichclass < 0)
    whichclass = char_get_class(ch);
  if (char_get_level(ch) < 100 && char_get_exp(ch) >= level_exp(ch, char_get_level(ch) + 1))
  {
    char_set_class_level(ch, char_get_class_level(ch) + (1));
    // GET_CLASS(ch) = whichclass; /* Now tracks latest class instead of highest */
    advance_level(ch, whichclass);
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced level to level %d.",
           char_get_name(ch), char_get_level(ch));
    send_to_char(ch, "You rise a level!\r\n");
    char_set_exp(ch, char_get_exp(ch) - (level_exp(ch, char_get_level(ch))));
    /*set_title(ch, NULL);*/
    write_aliases(ch);
    save_char(ch);
  }
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ)
  {
    size_t res;
    char buf[256];

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
                   CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT, IMMLIST_FILE, (int)getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
                   CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf))
    {
      mudlog(CMP, ADMLVL_IMMORT, FALSE, "Initiating autowiz.");
      system(buf);
      reboot_wizlists();
    }
    else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

void gain_exp(struct char_data *ch, int64_t gain)
{

  if (gain > 20000000)
  {
    gain = 20000000;
  }

  if (char_in_arena(ch))
  {
    send_to_char(ch, "EXP CANCEL: You can not gain experience from the arena.\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_WUNJO))
  {
    gain += gain * 0.15;
  }
  if (char_plr_flagged(ch, PLR_IMMORTAL))
  {
    gain = gain * 0.95;
  }

  int64_t diff = gain * 0.15;

  if (!char_is_npc(ch) && char_get_level(ch) < 1)
    return;

  if (char_is_npc(ch))
  {
    char_set_exp(ch, char_get_exp(ch) + (gain));
    return;
  }

  if (gain > 0)
  {
    gain = MIN(CONFIG_MAX_EXP_GAIN, gain); /* put a cap on the max gain per kill */
    if (char_get_equipment(ch, WEAR_SH))
    {
      struct obj_data *obj = char_get_equipment(ch, WEAR_SH);
      if (obj_get_vnum(obj) == 1127)
      {
        int64_t spar = gain;
        gain += gain * 2.5;
        spar = gain - spar;
        send_to_char(ch, "@D[@BBooster EXP@W: @G+%s@D]\r\n", add_commas(spar));
      }
    }
    if (char_get_level(ch) < 100)
    {
      if (char_get_mindlink(ch) && gain > 0 && char_get_linker(ch) == 0)
      {
        if (char_get_level(ch) + 20 < char_get_level(char_get_mindlink(ch)) || char_get_level(ch) - 20 > char_get_level(char_get_mindlink(ch)))
        {
          send_to_char(char_get_mindlink(ch), "The level difference between the two of you is too great to gain from mind read.\r\n");
        }
        else
        {
          act("@GYou've absorbed some new experiences from @W$n@G!@n", FALSE, ch, 0, char_get_mindlink(ch), TO_VICT);
          int read = gain * 0.12;
          gain -= read;
          if (read == 0)
            read = 1;
          gain_exp(char_get_mindlink(ch), read);
          act("@RYou sense that @W$N@R has stolen some of your experiences with $S mind!@n", FALSE, ch, 0, char_get_mindlink(ch), TO_CHAR);
        }
      }
      int64_t difff = level_exp(ch, char_get_level(ch) + 1) * 5;
      if (char_get_level(ch) <= 90 && (level_exp(ch, char_get_level(ch) + 1) - (char_get_exp(ch) + gain) <= (level_exp(ch, char_get_level(ch) + 1) - difff)))
      {
        send_to_char(ch, "@WYou -@RNEED@W- to @ylevel@W you can't hold any more experience.@n\r\n");
      }
      else if (char_get_level(ch) >= 91 && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) <= -1)
      {
        send_to_char(ch, "@WYou -@RNEED@W- to @ylevel@W you can't hold any more experience.@n\r\n");
      }
      else
      {
        char_set_exp(ch, char_get_exp(ch) + (gain));
      }
    }
    if (char_get_level(ch) < 100 && char_get_exp(ch) >= level_exp(ch, char_get_level(ch) + 1))
      send_to_char(ch, "@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");

    if (char_get_level(ch) == 100 && char_get_admlevel(ch) < 1)
    {
      if (char_is_kanassan(ch) || char_is_demon(ch))
      {
        diff = diff * 1.3;
      }
      if (char_is_android(ch))
      {
        diff = diff * 1.2;
      }
      if (char_get_mindlink(ch) && gain > 0 && char_get_linker(ch) == 0)
      {
        if (char_get_level(ch) + 20 < char_get_level(char_get_mindlink(ch)) || char_get_level(ch) - 20 > char_get_level(char_get_mindlink(ch)))
        {
          send_to_char(char_get_mindlink(ch), "The level difference between the two of you is too great to gain from mind read.\r\n");
        }
        else
        {
          act("@GYou've absorbed some new experiences from @W$n@G!@n", FALSE, ch, 0, char_get_mindlink(ch), TO_VICT);
          int64_t read = gain * 0.12;
          diff -= (read * 0.15);
          gain -= read;
          if (read == 0)
            read = 1;
          gain_exp(char_get_mindlink(ch), read);
          act("@RYou sense that @W$N@R has stolen some of your experiences with $S mind!@n", FALSE, ch, 0, char_get_mindlink(ch), TO_CHAR);
        }
      }
      if (rand_number(1, 5) >= 2)
      {
        if (char_is_human(ch))
        {
          gainBasePL(ch, diff * 0.8, false);
        }
        else
        {
          gainBasePL(ch, diff, false);
        }
        send_to_char(ch, "@D[@G+@Y%s @RPL@D]@n ", add_commas(diff));
      }
      if (rand_number(1, 5) >= 2)
      {
        if (char_is_halfbreed(ch))
        {
          gainBaseST(ch, diff * 0.85, false);
        }
        else
        {
          gainBaseST(ch, diff, false);
        }
        send_to_char(ch, "@D[@G+@Y%s @gSTA@D]@n ", add_commas(diff));
      }
      if (rand_number(1, 5) >= 2)
      {
        gainBaseKI(ch, diff, false);
        send_to_char(ch, "@D[@G+@Y%s @CKi@D]@n", add_commas(diff));
      }
    }
  }
  else if (gain < 0)
  {
    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain); /* Cap max exp lost per death */
    char_set_exp(ch, char_get_exp(ch) + (gain));
    if (char_get_exp(ch) < 0)
      char_set_exp(ch, 0);
  }
}

void gain_exp_regardless(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  gain = (gain * CONFIG_EXP_MULTIPLIER);

  char_set_exp(ch, char_get_exp(ch) + (gain));
  if (char_get_exp(ch) < 0)
    char_set_exp(ch, 0);

  if (!char_is_npc(ch))
  {
    while (char_get_level(ch) < CONFIG_LEVEL_CAP - 1 && char_get_exp(ch) >= level_exp(ch, char_get_level(ch) + 1))
    {
      char_set_class_level(ch, char_get_class_level(ch) + (1));
      num_levels++;
      advance_level(ch, char_get_class(ch));
      is_altered = TRUE;
    }

    if (is_altered)
    {
      mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
             "%s advanced %d level%s to level %d.", char_get_name(ch), num_levels,
             num_levels == 1 ? "" : "s", char_get_level(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
        send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      /*set_title(ch, NULL);*/
    }
  }
}

void gain_condition(struct char_data *ch, int condition, int value)
{
  bool intoxicated;

  if (char_is_npc(ch))
    return;

  if (char_is_android(ch))
  {
    return;
  }

  if (GET_COND(ch, condition) < 0)
  { /* No change */
    return;
  }

  if (rroom_get_vnum(char_get_in_room(ch)) <= 1)
  {
    return;
  }

  if (char_plr_flagged(ch, PLR_WRITING))
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);
  if (value > 0)
  {
    if (GET_COND(ch, condition) >= 0)
    {
      if (GET_COND(ch, condition) + value > 48)
      {
        int prior = GET_COND(ch, condition);
        GET_COND(ch, condition) = 48;
        if (condition != DRUNK && prior >= 48 && !char_is_majin(ch))
        {
          int ocond = condition;
          if (condition == HUNGER)
            ocond = THIRST;
          else if (condition == THIRST)
            ocond = HUNGER;
        }
      }
      else
      {
        GET_COND(ch, condition) += value;
      }
    }
  } 
  else
  {
    if (GET_COND(ch, condition) >= 0)
    {
      if (GET_COND(ch, condition) + value < 0)
      {
        GET_COND(ch, condition) = 0;
      }
      else
      {
        GET_COND(ch, condition) += value;
      }
    }
  }
  switch (condition)
  {
  case HUNGER:
    switch (GET_COND(ch, condition))
    {
    case 0:
      //send_to_char(ch, "@RYou are feeling ravenous!@n\r\n");
      break;
    case 1:
    case 2:
    case 3:
      send_to_char(ch, "You are extremely hungry!\r\n");
      break;
    case 9:
    case 10:
    case 11:
      send_to_char(ch, "You are hungry!\r\n");
      break;
    case 19:
    case 20:
    case 21:
      send_to_char(ch, "You could use something to eat.\r\n");
      break;
    default:
      break;
    }
    break;
  case THIRST:
    switch (GET_COND(ch, condition))
    {
    case 0:
      //send_to_char(ch, "@RYou are dehydrated!@n\r\n");
      break;
    case 1:
    case 2:
    case 3:
      send_to_char(ch, "You are extremely thirsty!\r\n");
      break;
    case 9:
    case 10:
    case 11:
      send_to_char(ch, "Your throat is pretty dry!\r\n");
      break;
    case 19:
    case 20:
    case 21:
      send_to_char(ch, "You could use something to drink.\r\n");
      break;
    default:
      break;
    }
    break;
  case DRUNK:
    if (intoxicated)
    {
      if (GET_COND(ch, DRUNK) <= 0)
      {
        send_to_char(ch, "You are now sober.\r\n");
      }
    }
    break;
  default:
    break;
  }
}

static void check_idling(struct char_data *ch)
{
  if (dball_count(ch))
  {
    return;
  }
  if (++(ch->timer) > CONFIG_IDLE_VOID)
  {
    if (char_get_was_in_room(ch) == NOWHERE && char_get_in_room(ch) != NOWHERE)
    {
      char_set_was_in_room(ch, char_get_in_room(ch));
      if (char_get_fighting(ch))
      {
        stop_fighting(char_get_fighting(ch));
        stop_fighting(ch);
      }
      if (char_get_in_room(ch) == 0 || char_get_in_room(ch) == 1)
      {
        GET_LOADROOM(ch) = GET_LOADROOM(ch);
      }
      if (!rroom_flagged(char_get_in_room(ch), ROOM_PAST) && (rroom_get_vnum(char_get_in_room(ch)) < 19800 || rroom_get_vnum(char_get_in_room(ch)) > 19899))
      {
        GET_LOADROOM(ch) = rroom_get_vnum(char_get_in_room(ch));
      }
      if (rroom_flagged(char_get_in_room(ch), ROOM_PAST))
      {
        GET_LOADROOM(ch) = rroom_get_vnum(real_room(1561));
      }
      if (rroom_get_vnum(char_get_in_room(ch)) >= 2002 && rroom_get_vnum(char_get_in_room(ch)) <= 2011)
      {
        GET_LOADROOM(ch) = rroom_get_vnum(real_room(1960));
      }
      if (rroom_get_vnum(char_get_in_room(ch)) == 2069)
      {
        GET_LOADROOM(ch) = rroom_get_vnum(real_room(2017));
      }
      if (rroom_get_vnum(char_get_in_room(ch)) == 2070)
      {
        GET_LOADROOM(ch) = rroom_get_vnum(real_room(2046));
      }
      if (rroom_get_vnum(char_get_in_room(ch)) >= 101 && rroom_get_vnum(char_get_in_room(ch)) <= 139)
      {
        if (char_get_level(ch) == 1)
        {
          GET_LOADROOM(ch) = rroom_get_vnum(real_room(100));
          char_set_exp(ch, 0);
        }
        else
        {
          if (char_is_roshi(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(1130));
          }
          if (char_is_kabito(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(12098));
          }
          if (char_is_nail(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(11683));
          }
          if (char_is_bardock(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(2268));
          }
          if (char_is_krane(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(13009));
          }
          if (char_is_tapion(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(8231));
          }
          if (char_is_piccolo(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(1659));
          }
          if (char_is_andsix(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(1713));
          }
          if (char_is_dabura(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(6486));
          }
          if (char_is_frieza(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(4282));
          }
          if (char_is_ginyu(ch))
          {
            GET_LOADROOM(ch) = rroom_get_vnum(real_room(4289));
          }
        }
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    }
    else if (ch->timer > CONFIG_IDLE_RENT_TIME)
    {
      if (char_get_in_room(ch) != NOWHERE)
      {
        char_from_room(ch);
        char_to_room(ch, 3);
      }
      if (ch->desc)
      {
        send_to_char(ch, "You are idle and are extracted safely from the game.\r\n");
        desc_set_connected(ch->desc, CON_DISCONNECT);
        /*
         * For the 'if (d->character)' test in close().
         * -gg 3/1/98 (Happy anniversary.)
         */
        ch->desc->character = NULL;
        ch->desc = NULL;
      }
      Crash_rentsave(ch, 0);
      mudlog(CMP, ADMLVL_GOD, TRUE, "%s force-rented and extracted (idle).", char_get_name(ch));
      extract_char(ch);
    }
  }
}

static void heal_limb(struct char_data *ch)
{
  int healrate = 0, recovered = FALSE;

  if (char_plr_flagged(ch, PLR_BANDAGED))
  {
    healrate += 10;
  }

  if (char_get_position(ch) == POS_SITTING)
  {
    healrate += 1;
  }
  else if (char_get_position(ch) == POS_RESTING)
  {
    healrate += 3;
  }
  else if (char_get_position(ch) == POS_SLEEPING)
  {
    healrate += 5;
  }

  if (healrate > 0)
  {
    if (char_get_limb_condition(ch, (1)-1) > 0 && char_get_limb_condition(ch, (1)-1) < 50)
    {
      if (char_get_limb_condition(ch, (1)-1) + healrate >= 50)
      {
        act("You realize your right arm is no longer broken.", TRUE, ch, 0, 0, TO_CHAR);
        act("$n starts moving $s right arm gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
        char_set_limb_condition(ch, (1)-1, char_get_limb_condition(ch, (1)-1) + (healrate));
        recovered = TRUE;
      }
      else
      {
        char_set_limb_condition(ch, (1)-1, char_get_limb_condition(ch, (1)-1) + (healrate));
        send_to_char(ch, "Your right arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
      }
    }
    else if (char_get_limb_condition(ch, (1)-1) + healrate < 100)
    {
      char_set_limb_condition(ch, (1)-1, char_get_limb_condition(ch, (1)-1) + (healrate));
      send_to_char(ch, "Your right arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
    }
    else if (char_get_limb_condition(ch, (1)-1) < 100 && char_get_limb_condition(ch, (1)-1) + healrate >= 100)
    {
      char_set_limb_condition(ch, (1)-1, 100);
      send_to_char(ch, "Your right arm has fully recovered.\r\n");
    }

    if (char_get_limb_condition(ch, (2)-1) > 0 && char_get_limb_condition(ch, (2)-1) < 50)
    {
      if (char_get_limb_condition(ch, (2)-1) + healrate >= 50)
      {
        act("You realize your left arm is no longer broken.", TRUE, ch, 0, 0, TO_CHAR);
        act("$n starts moving $s left arm gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
        char_set_limb_condition(ch, (2)-1, char_get_limb_condition(ch, (2)-1) + (healrate));
        recovered = TRUE;
      }
      else
      {
        char_set_limb_condition(ch, (2)-1, char_get_limb_condition(ch, (2)-1) + (healrate));
        send_to_char(ch, "Your left arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
      }
    }
    else if (char_get_limb_condition(ch, (2)-1) + healrate < 100)
    {
      char_set_limb_condition(ch, (2)-1, char_get_limb_condition(ch, (2)-1) + (healrate));
      send_to_char(ch, "Your left arm feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (2)-1), "%", "%");
    }
    else if (char_get_limb_condition(ch, (2)-1) < 100 && char_get_limb_condition(ch, (2)-1) + healrate >= 100)
    {
      char_set_limb_condition(ch, (2)-1, 100);
      send_to_char(ch, "Your left arm has fully recovered.\r\n");
    }

    if (char_get_limb_condition(ch, (3)-1) > 0 && char_get_limb_condition(ch, (3)-1) < 50)
    {
      if (char_get_limb_condition(ch, (3)-1) + healrate >= 50)
      {
        act("You realize your right leg is no longer broken.", TRUE, ch, 0, 0, TO_CHAR);
        act("$n starts moving $s right leg gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
        char_set_limb_condition(ch, (3)-1, char_get_limb_condition(ch, (3)-1) + (healrate));
        recovered = TRUE;
      }
      else
      {
        char_set_limb_condition(ch, (3)-1, char_get_limb_condition(ch, (3)-1) + (healrate));
        send_to_char(ch, "Your right leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
      }
    }
    else if (char_get_limb_condition(ch, (3)-1) + healrate < 100)
    {
      char_set_limb_condition(ch, (3)-1, char_get_limb_condition(ch, (3)-1) + (healrate));
      send_to_char(ch, "Your right leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (3)-1), "%", "%");
    }
    else if (char_get_limb_condition(ch, (3)-1) < 100 && char_get_limb_condition(ch, (3)-1) + healrate >= 100)
    {
      char_set_limb_condition(ch, (3)-1, 100);
      send_to_char(ch, "Your right leg has fully recovered.\r\n");
    }

    if (char_get_limb_condition(ch, (4)-1) > 0 && char_get_limb_condition(ch, (4)-1) < 50)
    {
      if (char_get_limb_condition(ch, (4)-1) + healrate >= 50)
      {
        act("You realize your left leg is no longer broken.", TRUE, ch, 0, 0, TO_CHAR);
        act("$n starts moving $s left leg gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
        char_set_limb_condition(ch, (4)-1, char_get_limb_condition(ch, (4)-1) + (healrate));
        recovered = TRUE;
      }
      else
      {
        char_set_limb_condition(ch, (4)-1, char_get_limb_condition(ch, (4)-1) + (healrate));
        send_to_char(ch, "Your left leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (1)-1), "%", "%");
      }
    }
    else if (char_get_limb_condition(ch, (4)-1) + healrate < 100)
    {
      char_set_limb_condition(ch, (4)-1, char_get_limb_condition(ch, (4)-1) + (healrate));
      send_to_char(ch, "Your left leg feels a little better @D[@G%d%s@D/@g100%s@D]@n.\r\n", char_get_limb_condition(ch, (4)-1), "%", "%");
    }
    else if (char_get_limb_condition(ch, (4)-1) < 100 && char_get_limb_condition(ch, (4)-1) + healrate >= 100)
    {
      char_set_limb_condition(ch, (4)-1, 100);
      send_to_char(ch, "Your left leg as fully recovered.\r\n");
    }

    if (!char_plr_flagged(ch, PLR_BANDAGED) && recovered == TRUE)
    {
      if (axion_dice(-10) > char_get_aff_abils_con(ch))
      {
        ch->real_abils.str -= 1;
        ch->real_abils.dex -= 1;
        ch->real_abils.cha -= 1;
        send_to_char(ch, "@RYou lose 1 Strength, Agility, and Speed!\r\n");
        if (ch->real_abils.str < 4)
        {
          ch->real_abils.str = 4;
        }
        if (ch->real_abils.con < 4)
        {
          ch->real_abils.con = 4;
        }
        if (ch->real_abils.dex < 4)
        {
          ch->real_abils.dex = 4;
        }
        if (ch->real_abils.cha < 4)
        {
          ch->real_abils.cha = 4;
        }
        save_char(ch);
      }
    }
  }

  if (char_plr_flagged(ch, PLR_BANDAGED) && recovered == TRUE)
  {
    char_plr_set(ch, PLR_BANDAGED, false);
    send_to_char(ch, "You remove your bandages.\r\n");
    return;
  }
}

static void point_update_characters(void)
{
  struct char_data *i, *next_char;
  int change = FALSE;
  for (i = character_list; i; i = next_char)
  {
    next_char = i->next;

    if (!char_is_npc(i) && char_get_in_room(i) != NOWHERE)
    {
      if (rroom_flagged(char_get_in_room(i), ROOM_HOUSE))
      {
        char_set_relax_count(i, char_get_relax_count(i) + (1));
      }
      else if (char_get_relax_count(i) >= 464)
      {
        char_set_relax_count(i, char_get_relax_count(i) - (4));
      }
      else if (char_get_relax_count(i) >= 232)
      {
        char_set_relax_count(i, char_get_relax_count(i) - (3));
      }
      else if (char_get_relax_count(i) > 0 && rand_number(1, 3) == 3)
      {
        char_set_relax_count(i, char_get_relax_count(i) - (2));
      }
      else
      {
        char_set_relax_count(i, char_get_relax_count(i) - (1));
      }

      if (char_get_relax_count(i) < 0)
      {
        char_set_relax_count(i, 0);
      }
    }
    if (rand_number(1, 2) == 2)
    {
      gain_condition(i, HUNGER, -1);
    }
    if (rand_number(1, 2) == 2)
    {
      gain_condition(i, THIRST, -1);
    }
    if (rand_number(1, 2) == 2)
    {
      gain_condition(i, DRUNK, -1);
    }
    if (char_is_npc(i))
    {
      i->aggtimer = 0;
    }

    if (char_get_position(i) >= POS_STUNNED)
    {
      change = FALSE;
      update_flags(i);
      if (!char_is_npc(i))
      {
        if (!isFullVitals(i))
        {
          change = TRUE;
        }
      }

      if (char_plr_flagged(i, PLR_AURALIGHT))
      {
        if ((getCurKI(i)) > (mana_gain(i) + getPercentOfMaxKI(i, .05)))
        {
          send_to_char(i, "You send more energy into your aura to keep the light active.\r\n");
          decCurKI(i, mana_gain(i) + getPercentOfMaxKI(i, .05), 0);
        }
        else
        {
          send_to_char(i, "You don't have enough energy to keep the aura active.\r\n");
          act("$n's aura slowly stops shining and fades.\r\n", TRUE, i, nullptr, nullptr, TO_ROOM);
          char_plr_set(i, PLR_AURALIGHT, false);
          world[char_get_in_room(i)].light--;
        }
      }
      if (char_is_mutant(i) && (char_get_genome(i, 0) == 6 || char_get_genome(i, 1) == 6))
      {
        mutant_limb_regen(i);
      }

      int x = (char_get_kaioken(i) * 5) + 5;

      if (char_get_sleeptime(i) > 0 && char_get_position(i) != POS_SLEEPING)
      {
        char_set_sleeptime(i, char_get_sleeptime(i) - (1));
      }
      if (char_get_sleeptime(i) < 8 && char_get_position(i) == POS_SLEEPING)
      {
        char_set_sleeptime(i, char_get_sleeptime(i) + (rand_number(2, 4)));
        if (char_get_sleeptime(i) > 8)
        {
          char_set_sleeptime(i, 8);
        }
      }

      if (char_get_kaioken(i) > 0)
      {
        improve_skill(i, SKILL_KAIOKEN, -1);
        if ((char_get_skill(i, SKILL_KAIOKEN) < rand_number(1, x) || (getCurST(i)) <= GET_MAX_MOVE(i) / 10))
          remove_kaioken(i, 2);
      }

      if (char_aff_flagged(i, AFF_BURNED))
      {
        if (rand_number(1, 5) >= 4)
        {
          send_to_char(i, "Your burns are healed now.\r\n");
          act("$n@w's burns are now healed.@n", TRUE, i, 0, 0, TO_ROOM);
          char_aff_set(i, AFF_BURNED, false);
        }
      }

      incCurHealth(i, hit_gain(i), true);
      incCurST(i, move_gain(i), true);
      incCurKI(i, mana_gain(i), true);

      if (!char_is_npc(i))
      {
        heal_limb(i);
      }

      if (rroom_sect(char_get_in_room(i)) == SECT_WATER_NOSWIM && !CARRIED_BY(i) && !char_is_kanassan(i))
      {
        if ((getCurST(i)) >= (getCurCarriedWeight(i)))
        {
          act("@bYou swim in place.@n", TRUE, i, 0, 0, TO_CHAR);
          act("@C$n@b swims in place.@n", TRUE, i, 0, 0, TO_ROOM);
          decCurST(i, getCurCarriedWeight(i), 0);
        }
        else
        {
          decCurST(i, getCurCarriedWeight(i), 0);
          act("@RYou are drowning!@n", TRUE, i, 0, 0, TO_CHAR);
          act("@C$n@b gulps water as $e struggles to stay above the water line.@n", TRUE, i, 0, 0, TO_ROOM);
          if (GET_HIT(i) - ((getEffMaxPL(i)) / 3) <= 0)
          {
            act("@rYou drown!@n", TRUE, i, 0, 0, TO_CHAR);
            act("@R$n@r drowns!@n", TRUE, i, 0, 0, TO_ROOM);
            die(i, NULL);
          }
          else
          {
            decCurHealth(i, (getEffMaxPL(i)) / 3, 0);
          }
        }
      }
      if (!has_o2(i) && rroom_sunken(char_get_in_room(i)) && !rroom_flagged(char_get_in_room(i), ROOM_SPACE))
      {
        if (((getCurKI(i)) - mana_gain(i)) > GET_MAX_MANA(i) / 200)
        {
          send_to_char(i, "Your ki holds an atmosphere around you.\r\n");
          decCurKI(i, mana_gain(i) + getPercentOfMaxKI(i, .005), 0);
        }
        else
        {
          if ((GET_HIT(i) - hit_gain(i)) > (getEffMaxPL(i)) * 0.05)
          {
            send_to_char(i, "You struggle trying to hold your breath!\r\n");
            decCurHealth(i, hit_gain(i) + getPercentOfMaxHealth(i, .05), 0);
          }
          else if (GET_HIT(i) <= GET_MAX_HIT(i) / 20)
          {
            send_to_char(i, "You have drowned!\r\n");
            act("@W$n@W drowns right in front of you.@n", FALSE, i, 0, 0, TO_ROOM);
            die(i, NULL);
          }
        }
      }
      if (!has_o2(i) && rroom_flagged(char_get_in_room(i), ROOM_SPACE))
      {
        if (((getCurKI(i)) - mana_gain(i)) > GET_MAX_MANA(i) * 0.005)
        {
          send_to_char(i, "Your ki holds an atmosphere around you.\r\n");
          decCurKI(i, mana_gain(i) + getPercentOfMaxKI(i, .005), 0);
        }
        else
        {
          if ((GET_HIT(i) - hit_gain(i)) > (getEffMaxPL(i)) * 0.05)
          {
            send_to_char(i, "You struggle trying to hold your breath!\r\n");
            decCurHealth(i, hit_gain(i) + getPercentOfMaxHealth(i, .05), 0);
          }
          else if (GET_HIT(i) <= GET_MAX_HIT(i) / 20)
          {
            send_to_char(i, "You have drowned!\r\n");
            decCurHealthPercent(i, 1, 1);
            act("@W$n@W drowns right in front of you.@n", FALSE, i, 0, 0, TO_ROOM);
            die(i, NULL);
          }
        }
      }
      if (!char_aff_flagged(i, AFF_FLYING) && rroom_get_geffect(char_get_in_room(i)) == 6 && !char_mob_flagged(i, MOB_NOKILL) && !char_is_demon(i))
      {
        act("@rYour legs are burned by the lava!@n", TRUE, i, 0, 0, TO_CHAR);
        act("@R$n@r's legs are burned by the lava!@n", TRUE, i, 0, 0, TO_ROOM);
        if (char_is_npc(i) && char_is_humanoid(i) && rand_number(1, 2) == 2)
        {
          do_fly(i, 0, 0, 0);
        }
        decCurHealthPercent(i, .05, 0);
        if (GET_HIT(i) < 0)
        {
          act("@rYou have burned to death!@n", TRUE, i, 0, 0, TO_CHAR);
          act("@R$n@r has burned to death!@n", TRUE, i, 0, 0, TO_ROOM);
          die(i, NULL);
        }
      }
      if (change && !char_aff_flagged(i, AFF_POISON))
      {
        if (char_plr_flagged(i, PLR_HEALT) && char_get_sits(i) != NULL)
        {
          send_to_char(i, "@wThe healing tank works wonders on your injuries.@n\r\n");
          obj_set_healcharge(char_get_sits(i), obj_get_healcharge(char_get_sits(i)) - (rand_number(1, 2)));
          if (obj_get_healcharge(char_get_sits(i)) == 0)
          {
            send_to_char(i, "@wThe healing tank is now too low on energy to heal you.\r\n");
            act("You step out of the now empty healing tank.", TRUE, i, 0, 0, TO_CHAR);
            act("@C$n@w steps out of the now empty healing tank.@n", TRUE, i, 0, 0, TO_ROOM);
            char_plr_set(i, PLR_HEALT, false);
            obj_set_sitting(char_get_sits(i), NULL);
            char_set_sits(i, NULL);
          }
          else if (isFullVitals(i))
          {
            send_to_char(i, "@wYou are fully recovered now.\r\n");
            act("You step out of the now empty healing tank.", TRUE, i, 0, 0, TO_CHAR);
            act("@C$n@w steps out of the now empty healing tank.@n", TRUE, i, 0, 0, TO_ROOM);
            char_plr_set(i, PLR_HEALT, false);
            obj_set_sitting(char_get_sits(i), NULL);
            char_set_sits(i, NULL);
          }
        }
        else if (char_plr_flagged(i, PLR_HEALT) && char_get_sits(i) == NULL)
        {
          char_plr_set(i, PLR_HEALT, false);
        }
        else if (char_get_position(i) == POS_SLEEPING)
        {
          send_to_char(i, "@wYour sleep does you some good.@n\r\n");
          if (!char_is_android(i) && !char_get_fighting(i))
            restoreLF(i, false);
        }
        else if (char_get_position(i) == POS_RESTING)
        {
          send_to_char(i, "@wYou feel relaxed and better.@n\r\n");
          if (!isFullLF(i))
          {
            if (!char_is_android(i) && !char_get_fighting(i) && char_get_suppression(i) <= 0 && GET_HIT(i) != (getEffMaxPL(i)))
            {
              incCurLFPercent(i, .15, true);
              send_to_char(i, "@CYou feel more lively.@n\r\n");
            }
          }
        }
        else if (char_get_position(i) == POS_SITTING)
          send_to_char(i, "@wYou feel rested and better.@n\r\n");
        else
          send_to_char(i, "You feel slightly better.\r\n");
      }
      if (char_aff_flagged(i, AFF_POISON))
      {
        double cost = 0.0;
        if (char_get_aff_abils_con(i) >= 100)
        {
          cost = 0.01;
        }
        else if (char_get_aff_abils_con(i) >= 80)
        {
          cost = 0.02;
        }
        else if (char_get_aff_abils_con(i) >= 50)
        {
          cost = 0.03;
        }
        else if (char_get_aff_abils_con(i) >= 30)
        {
          cost = 0.04;
        }
        else if (char_get_aff_abils_con(i) >= 20)
        {
          cost = 0.05;
        }
        else
        {
          cost = 0.06;
        }
        if (GET_HIT(i) - GET_MAX_HIT(i) * cost > 0)
        {
          send_to_char(i, "You puke as the poison burns through your blood.\r\n");
          act("$n shivers and then pukes.", TRUE, i, 0, 0, TO_ROOM);
          decCurHealth(i, getEffMaxPL(i) * cost, 0);
        }
        else
        {
          send_to_char(i, "The poison claims your life!\r\n");
          act("$n pukes up blood and falls down dead!", TRUE, i, 0, 0, TO_ROOM);
          if (i->poisonby)
          {
            if (char_aff_flagged(i->poisonby, AFF_GROUP))
            {
              group_gain(i->poisonby, i);
            }
            else
            {
              solo_gain(i->poisonby, i);
            }
            die(i, i->poisonby);
          }
          else
          {
            die(i, NULL);
          }
        }
      }
      if (char_get_position(i) <= POS_STUNNED)
        update_pos(i);
    }
    else if (char_get_position(i) == POS_INCAP)
    {
      continue;
    }
    else if (char_get_position(i) == POS_MORTALLYW)
    {
      continue;
    }
    if ((getCurKI(i)) >= GET_MAX_MANA(i) * 0.5 && char_get_charge(i) < GET_MAX_MANA(i) * 0.1 && char_get_preference(i) == PREFERENCE_KI && !char_plr_flagged(i, PLR_AURALIGHT))
    {
      char_set_charge(i, GET_MAX_MANA(i) * 0.1);
    }
    if (!char_is_npc(i))
    {
      update_char_objects(i);
      update_innate(i);
      if (char_get_admlevel(i) < CONFIG_IDLE_MAX_LEVEL)
        check_idling(i);
      else
        (i->timer)++;
    }
  }
}

static void point_update_objects(void)
{
  struct obj_data *j, *next_thing, *jj, *next_thing2, *vehicle = NULL;

  /* objects */
  for (j = object_list; j; j = next_thing)
  {
    next_thing = j->next; /* Next in object list */

    /* Let's get rid of dropped norent items. */
    if (obj_extra_flagged(j, ITEM_NORENT) && j->worn_by == NULL && j->carried_by == NULL && obj_selling != j && obj_get_vnum(j) != 7200)
    {
      time_t diff = 0;

      diff = time(0) - obj_get_lload(j);
      if (diff > 240 && obj_get_lload(j) > 0)
      {
        log("No rent object (%s) extracted from room (%d)", j->short_description, rroom_get_vnum(obj_get_in_room(j)));
        extract_obj(j);
        continue;
      }
    }

    if (obj_get_type_flag(j) == ITEM_HATCH)
    {
      if ((vehicle = find_vehicle_by_vnum(obj_get_value(j, VAL_HATCH_DEST))))
      {
        obj_set_value(j, 3, rroom_get_vnum(obj_get_in_room(vehicle)));
      }
    }

    /* If this is a corpse */
    if (obj_is_corpse(j))
    {
      /* timer count down */
      if (obj_get_timer(j) > 0)
        obj_set_timer(j, obj_get_timer(j) - 1);
      if (!strstr(j->name, "android") && !strstr(j->name, "Android") && !obj_extra_flagged(j, ITEM_BURIED))
      {
        if (obj_get_timer(j) == 5)
        {
          if ((obj_get_in_room(j) != NOWHERE) && (world[obj_get_in_room(j)].people))
          {
            act("@DFlies start to gather around $p@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
            act("@DFlies start to gather around $p@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
          }
        }
        if (obj_get_timer(j) == 3)
        {
          if ((obj_get_in_room(j) != NOWHERE) && (world[obj_get_in_room(j)].people))
          {
            act("@DA cloud of flies has formed over $p@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
            act("@DA cloud of flies has formed over $p@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
          }
        }
        if (obj_get_timer(j) == 2)
        {
          if ((obj_get_in_room(j) != NOWHERE) && (world[obj_get_in_room(j)].people))
          {
            act("@DMaggots can be seen crawling all over $p@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
            act("@DMaggots can be seen crawling all over $p@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
          }
        }
        if (obj_get_timer(j) == 1)
        {
          if ((obj_get_in_room(j) != NOWHERE) && (world[obj_get_in_room(j)].people))
          {
            act("@DMaggots have nearly stripped $p of all its flesh@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
            act("@DMaggots have nearly stripped $p of all its flesh@D.@n", TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
          }
        }
      }
      if (!obj_get_timer(j))
      {

        if (j->carried_by)
        {
          if (!strstr(j->name, "android"))
          {
            act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
            if ((obj_get_in_room(j) != NOWHERE) && (world[obj_get_in_room(j)].people))
            {
              act("A quivering horde of maggots consumes $p.",
                  TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
              act("A quivering horde of maggots consumes $p.",
                  TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
            }
          }
          else
          {
            act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
            if ((obj_get_in_room(j) != NOWHERE) && (world[obj_get_in_room(j)].people))
            {
              act("$p breaks down completely into a pile of junk.",
                  TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
              act("$p breaks down completely into a pile of junk.",
                  TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
            }
          }
        }
        for (jj = j->contains; jj; jj = next_thing2)
        {
          next_thing2 = jj->next_content; /* Next in inventory */
          obj_from_obj(jj);

          if (j->in_obj)
            obj_to_obj(jj, j->in_obj);
          else if (j->carried_by)
            obj_to_room(jj, char_get_in_room(j->carried_by));
          else if (obj_get_in_room(j) != NOWHERE)
            obj_to_room(jj, obj_get_in_room(j));
          else
            core_dump();
        }
        extract_obj(j);
        continue;
      }
    }

    if (obj_get_vnum(j) == 65)
    {
      if (obj_get_healcharge(j) < 20 && !obj_get_sitting(j))
      {
        obj_set_healcharge(j, obj_get_healcharge(j) + (rand_number(0, 1)));
      }
    }
    if (obj_get_type_flag(j) == ITEM_PORTAL)
    {
      if (obj_get_timer(j) > 0)
        obj_set_timer(j, obj_get_timer(j) - 1);

      if (obj_get_timer(j) == 0)
      {
        act("A glowing portal fades from existence.",
            TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
        act("A glowing portal fades from existence.",
            TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
        extract_obj(j);
        continue;
      }
    }
    else if (obj_get_vnum(j) == 1306)
    {
      if (obj_get_timer(j) > 0)
        obj_set_timer(j, obj_get_timer(j) - 1);

      if (obj_get_timer(j) == 0)
      {
        act("The $p@n settles to the ground and goes out.",
            TRUE, world[obj_get_in_room(j)].people, j, 0, TO_ROOM);
        act("A $p@n settles to the ground and goes out.",
            TRUE, world[obj_get_in_room(j)].people, j, 0, TO_CHAR);
        extract_obj(j);
        continue;
      }
    }
    else if (obj_extra_flagged(j, ITEM_ICE))
    {
      if (obj_get_vnum(j) == 79 && rand_number(1, 2) == 2)
      {
        if (rroom_get_geffect(obj_get_in_room(j)) >= 1 && rroom_get_geffect(obj_get_in_room(j)) <= 5)
        {
          send_to_room(obj_get_in_room(j), "The heat from the lava melts a great deal of the glacial wall and the lava cools a bit in turn.\r\n");
          rroom_set_geffect(obj_get_in_room(j), rroom_get_geffect(obj_get_in_room(j)) - (1));
          if (obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.025)) > 0)
          {
            obj_set_weight(j, obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.025)));
          }
          else
          {
            send_to_room(obj_get_in_room(j), "The glacial wall blocking off the %s direction melts completely away.\r\n", dirs[obj_get_cost(j)]);
            extract_obj(j);
            continue;
          }
        }
        else if (obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.025)) > 0)
        {
          obj_set_weight(j, obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.025)));
          send_to_room(obj_get_in_room(j), "The glacial wall blocking off the %s direction melts some what.\r\n", dirs[obj_get_cost(j)]);
        }
        else
        {
          send_to_room(obj_get_in_room(j), "The glacial wall blocking off the %s direction melts completely away.\r\n", dirs[obj_get_cost(j)]);
          extract_obj(j);
          continue;
        }
      }
      else if (obj_get_vnum(j) != 79)
      {
        if (j->carried_by && !j->in_obj)
        {
          int melt = 5 + (obj_get_weight(j) * 0.02);
          if (obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.02)) > 0)
          {
            obj_set_weight(j, obj_get_weight(j) - (melt));
            send_to_char(j->carried_by, "%s @wmelts a little.\r\n", j->short_description);
            char_set_carry_weight(j->carried_by, char_get_carry_weight(j->carried_by) - (melt));
          }
          else
          {
            send_to_char(j->carried_by, "%s @wmelts completely away.\r\n", j->short_description);
            int remainder = melt - obj_get_weight(j);
            char_set_carry_weight(j->carried_by, char_get_carry_weight(j->carried_by) - ((melt - remainder)));
            extract_obj(j);
            continue;
          }
        }
        else if (obj_get_in_room(j) != NOWHERE)
        {
          if (obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.02)) > 0)
          {
            obj_set_weight(j, obj_get_weight(j) - (5 + (obj_get_weight(j) * 0.02)));
            send_to_room(obj_get_in_room(j), "%s @wmelts a little.\r\n", j->short_description);
          }
          else
          {
            send_to_room(obj_get_in_room(j), "%s @wmelts completely away.\r\n", j->short_description);
            extract_obj(j);
            continue;
          }
        }
      }
    }

    /* If the timer is set, count it down and at 0, try the trigger */
    /* note to .rej hand-patchers: make this last in your point-update() */
    else if (obj_get_timer(j) > 0)
    {
      obj_set_timer(j, obj_get_timer(j) - 1);
      if (!obj_get_timer(j))
        timer_otrigger(j);
    }
  }
}

void point_update(void)
{
  point_update_characters();
  point_update_objects();
}

void timed_dt(struct char_data *ch)
{
  struct char_data *vict;
  room_rnum rrnum;

  if (ch == NULL)
  {
    /* BY -WELCOR
      first make sure all rooms in the world have thier 'timed'
      value decreased if its not -1.
      */

    for (rrnum = 0; rrnum < top_of_world; rrnum++)
      world[rrnum].timed -= (world[rrnum].timed != -1);

    for (vict = character_list; vict; vict = vict->next)
    {
      if (char_is_npc(vict))
        continue;

      if (char_get_in_room(vict) == NOWHERE)
        continue;

      if (!rroom_flagged(char_get_in_room(vict), ROOM_TIMED_DT))
        continue;

      timed_dt(vict);
    }
    return;
  }

  /*Called with a non-null ch. let's check the room. */

  /*if the room wasn't triggered (i.e timed wasn't set), just set it
    and return again.
  */

  if (world[char_get_in_room(ch)].timed < 0)
  {
    world[char_get_in_room(ch)].timed = rand_number(2, 5);
    return;
  }

  /* We know ch is in a dt room with timed >= 0 - see if its the end.
   *
   */
  if (world[char_get_in_room(ch)].timed == 0)
  {
    for (vict = world[char_get_in_room(ch)].people; vict; vict = vict->next_in_room)
    {
      if (char_is_npc(vict))
        continue;
      if (char_get_admlevel(vict) >= ADMLVL_IMMORT)
        continue;

      /* Skip those alread dead people */
      /* extract char() jest sets the bit*/
      if (char_plr_flagged(vict, PLR_NOTDEADYET))
        continue;

      log_death_trap(vict);
      death_cry(vict);
      extract_char(vict);
    }
  }
}
