/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/feats.hpp"
#include "dbat/oasis.hpp"
#include "dbat/config.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/races.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/affect.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* external variables */

/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
  int water;

  if (ch == nullptr || obj == nullptr)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (obj_get_type_flag(obj) == ITEM_DRINKCON) {
    if ((obj_get_value(obj, VAL_DRINKCON_LIQUID) != LIQ_WATER) && (obj_get_value(obj, VAL_DRINKCON_HOWFULL) != 0)) {
      name_from_drinkcon(g, obj);
      obj_set_value(obj, VAL_DRINKCON_LIQUID, LIQ_SLIME);
      name_to_drinkcon(g, obj, LIQ_SLIME);
    } else {
      water = MAX(obj_get_value(obj, VAL_DRINKCON_CAPACITY) - obj_get_value(obj, VAL_DRINKCON_HOWFULL), 0);
      if (water > 0) {
        if (obj_get_value(obj, VAL_DRINKCON_HOWFULL) >= 0)
          name_from_drinkcon(g, obj);
        obj_set_value(obj, VAL_DRINKCON_LIQUID, LIQ_WATER);
        obj_set_value(obj, VAL_DRINKCON_HOWFULL, obj_get_value(obj, VAL_DRINKCON_HOWFULL) + (water));
        name_to_drinkcon(g, obj, LIQ_WATER);
        weight_change_object(g, obj, water);
        act(g, "$p is filled.", false, ch, obj, nullptr, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_recall)
{
  if (victim == nullptr || char_is_npc(victim))
    return;

  act(g, "$n disappears.", true, victim, nullptr, nullptr, TO_ROOM);
  char_from_room(g, victim);
  char_to_room(g, victim, g->r_mortal_start_room);
  act(g, "$n appears in the middle of the room.", true, victim, nullptr, nullptr, TO_ROOM);
  look_at_room(g, char_get_in_room(victim), victim, 0);
  entry_memory_mtrigger(g, victim);
  greet_mtrigger(g, victim, -1);
  greet_memory_mtrigger(g, victim);
}

ASPELL(spell_teleport)
{
  room_vnum to_room;

  if (victim == nullptr || char_is_npc(victim))
    return;

  do {
    to_room = g->rooms.nth(rand_number(g, 0, (int)g->rooms.size() - 1));
  } while (room_flagged(g, to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM));

  act(g, "$n slowly fades out of existence and is gone.",
      false, victim, nullptr, nullptr, TO_ROOM);
  char_from_room(g, victim);
  char_to_room(g, victim, to_room);
  act(g, "$n slowly fades into existence.", false, victim, nullptr, nullptr, TO_ROOM);
  look_at_room(g, char_get_in_room(victim), victim, 0);
  entry_memory_mtrigger(g, victim);
  greet_mtrigger(g, victim, -1);
  greet_memory_mtrigger(g, victim);
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
  if (ch == nullptr || victim == nullptr)
    return;

  if (char_get_level(victim) > level + 3) {
    send_to_char(g, ch, "{}", SUMMON_FAIL);
    return;
  }

  if (!CONFIG_PK_ALLOWED) {
    if (char_mob_flagged(victim, MOB_AGGRESSIVE)) {
      act(g, "As the words escape your lips and $N travels\r\n"
          "through time and space towards you, you realize that $E is\r\n"
          "aggressive and might harm you, so you wisely send $M back.",
          false, ch, nullptr, victim, TO_CHAR);
      return;
    }
    if (!char_is_npc(victim) && !char_prf_flagged(g, victim, PRF_SUMMONABLE) &&
        !char_plr_flagged(victim, PLR_KILLER)) {
      send_to_char(g, victim, "{} just tried to summon you to: {}.\r\n{} failed because you have summon protection on.\r\nType NOSUMMON to allow other players to summon you.\r\n",
                   char_get_name(ch), room_get_name(g, char_get_in_room(ch)),
                   (char_get_sex(ch) == SEX_MALE) ? "He" : "She");

      send_to_char(g, ch, "You failed because {} has summon protection on.\r\n", char_get_name(victim));
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "{} failed summoning {} to {}.", char_get_name(ch), char_get_name(victim), room_get_name(g, char_get_in_room(ch)));
      return;
    }
  }

  if (char_mob_flagged(victim, MOB_NOSUMMON) ||
      (char_is_npc(victim) && mag_newsaves(ch, victim, SPELL_SUMMON, level, char_get_aff_abils_intel(ch)))) {
    send_to_char(g, ch, "{}", SUMMON_FAIL);
    return;
  }

  act(g, "$n disappears suddenly.", true, victim, nullptr, nullptr, TO_ROOM);

  char_from_room(g, victim);
  char_to_room(g, victim, char_get_in_room(ch));

  act(g, "$n arrives suddenly.", true, victim, nullptr, nullptr, TO_ROOM);
  act(g, "$n has summoned you!", false, ch, nullptr, victim, TO_VICT);
  look_at_room(g, char_get_in_room(victim), victim, 0);
  entry_memory_mtrigger(g, victim);
  greet_mtrigger(g, victim, -1);
  greet_memory_mtrigger(g, victim);
}

ASPELL(spell_locate_object)
{
  std::string name;
  int j;

  /*
   * FIXME: This is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */
  if (!obj) {
    send_to_char(g, ch, "You sense nothing.\r\n");
    return;
  }

  name = fname(obj_get_name(obj));
  j = level / 2;

  for (obj_data *i : each_obj(g)) {
    if (!((j > 0)))
      break;
    if (!isname(name.c_str(), obj_get_name(i)))
      continue;

    send_to_char(g, ch, "{:c}{}", as_char(toupper(*obj_get_short_description(i))), (obj_get_short_description(i)) + 1);

    if (obj_get_carried_by(i))
      send_to_char(g, ch, " is being carried by {}.\r\n", char_pers(g, obj_get_carried_by(i), ch));
    else if (obj_get_in_room(i) != NOWHERE)
      send_to_char(g, ch, " is in {}.\r\n", room_get_name(g, obj_get_in_room(i)));
    else if (obj_get_in_obj(i))
      send_to_char(g, ch, " is in {}.\r\n", obj_get_short_description(obj_get_in_obj(i)));
    else if (obj_get_worn_by(i))
      send_to_char(g, ch, " is being worn by {}.\r\n", char_pers(g, obj_get_worn_by(i), ch));
    else
      send_to_char(g, ch, "'s location is uncertain.\r\n");

    j--;
  }

  if (j == level / 2)
    send_to_char(g, ch, "You sense nothing.\r\n");
}

ASPELL(spell_charm)
{
  struct affected_type af = {};

  if (victim == nullptr || ch == nullptr)
    return;

  if (victim == ch)
    send_to_char(g, ch, "You like yourself even better!\r\n");
  else if (!char_is_npc(victim) && !char_prf_flagged(g, victim, PRF_SUMMONABLE))
    send_to_char(g, ch, "You fail because SUMMON protection is on!\r\n");
  else if (char_aff_flagged(victim, AFF_SANCTUARY))
    send_to_char(g, ch, "Your victim is protected by sanctuary!\r\n");
  else if (char_mob_flagged(victim, MOB_NOCHARM))
    send_to_char(g, ch, "Your victim resists!\r\n");
  else if (char_aff_flagged(ch, AFF_CHARM))
    send_to_char(g, ch, "You can't have any followers of your own!\r\n");
  else if (char_aff_flagged(victim, AFF_CHARM) || level < char_get_level(victim))
    send_to_char(g, ch, "You fail.\r\n");
  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !char_is_npc(victim))
    send_to_char(g, ch, "You fail - shouldn't be doing it anyway.\r\n");
  else if (char_is_saiyan(victim) && rand_number(g, 1, 100) <= 90)
    send_to_char(g, ch, "Your victim resists!\r\n");
  else if (circle_follow(g, victim, ch))
    send_to_char(g, ch, "Sorry, following in circles cannot be allowed.\r\n");
  else if (mag_newsaves(ch, victim, SPELL_CHARM, level, char_get_aff_abils_intel(ch)))
    send_to_char(g, ch, "Your victim resists!\r\n");
  else {
    if (char_get_master(g, victim))
      stop_follower(g, victim);

    add_follower(g, victim, ch);
    char_set_master_id(victim, char_get_idnum(ch));

    af.type     = SPELL_CHARM;
    af.duration = 24 * 2;
    if (char_get_aff_abils_cha(ch))
      af.duration *= char_get_aff_abils_cha(ch);
    if (char_get_aff_abils_intel(victim))
      af.duration /= char_get_aff_abils_intel(victim);
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(g, victim, &af);

    act(g, "Isn't $n just such a nice fellow?", false, ch, nullptr, victim, TO_VICT);
    if (char_is_npc(victim))
      char_mob_set(victim, MOB_SPEC, false);
  }
}

ASPELL(spell_identify)
{
  int i, found;
  size_t len;

  if (obj) {
    char bitbuf[MAX_STRING_LENGTH];
    std::string buf2;

    sprinttype(obj_get_type_flag(obj), item_types, bitbuf, sizeof(bitbuf));
    send_to_char(g, ch, "You feel informed:\r\nObject '{}', Item type: {}\r\n", obj_get_short_description(obj), bitbuf);

    if (obj_get_bitvector(obj)) {
      sprintbitarray(obj_get_bitvector(obj), affected_bits, AF_ARRAY_MAX, bitbuf, sizeof(bitbuf));
      send_to_char(g, ch, "Item will give you following abilities:  {}\r\n", bitbuf);
    }

    sprintbitarray(obj_get_extra_flags(obj), extra_bits, EF_ARRAY_MAX, bitbuf, sizeof(bitbuf));
    send_to_char(g, ch, "Item is: {}\r\n", bitbuf);

    send_to_char(g, ch, "Weight: {:d}, Value: {:d}, Rent: {:d}, Min Level: {:d}\r\n", obj_get_weight(obj), obj_get_cost(obj), obj_get_cost_per_day(obj), obj_get_level(obj));

    switch (obj_get_type_flag(obj)) {
      case ITEM_SCROLL:
      case ITEM_POTION:
        len = i = 0;

        if (obj_get_value(obj, VAL_SCROLL_SPELL1) >= 1) {
          i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(g, obj_get_value(obj, VAL_SCROLL_SPELL1)));
          if (i >= 0)
            len += i;
        }

        if (obj_get_value(obj, VAL_SCROLL_SPELL2) >= 1 && len < sizeof(bitbuf)) {
          i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(g, obj_get_value(obj, VAL_SCROLL_SPELL2)));
          if (i >= 0)
            len += i;
        }

        if (obj_get_value(obj, VAL_SCROLL_SPELL3) >= 1 && len < sizeof(bitbuf)) {
          i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(g, obj_get_value(obj, VAL_SCROLL_SPELL3)));
          if (i >= 0)
            len += i;
        }

        send_to_char(g, ch, "This {} casts: {}\r\n", item_types[(int)obj_get_type_flag(obj)], bitbuf);
        break;
      case ITEM_WAND:
      case ITEM_STAFF:
        send_to_char(g, ch, "This {} casts: {}\r\nIt has {:d} maximum charge{} and {:d} remaining.\r\n",
                     item_types[(int)obj_get_type_flag(obj)], skill_name(g, obj_get_value(obj, VAL_WAND_SPELL)),
                     obj_get_value(obj, VAL_WAND_MAXCHARGES), obj_get_value(obj, VAL_WAND_MAXCHARGES) == 1 ? "" : "s", obj_get_value(obj, VAL_WAND_CHARGES));
        break;
      case ITEM_WEAPON:
        send_to_char(g, ch, "Damage Dice is '{:d}D{:d}' for an average per-round damage of {:.1f}.\r\n",
                     obj_get_value(obj, VAL_WEAPON_DAMDICE), obj_get_value(obj, VAL_WEAPON_DAMSIZE), ((obj_get_value(obj, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * obj_get_value(obj, VAL_WEAPON_DAMDICE));
        break;
      case ITEM_ARMOR:
        send_to_char(g, ch, "AC-apply is {:.1f}\r\n", ((float)obj_get_value(obj, VAL_ARMOR_APPLYAC)) / 10);
        break;
    }
    found = false;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj_get_affected(obj)[i].location != APPLY_NONE) &&
          (obj_get_affected(obj)[i].modifier != 0)) {
        if (!found) {
          send_to_char(g, ch, "Can affect you as :\r\n");
          found = true;
        }
        sprinttype(obj_get_affected(obj)[i].location, apply_types, bitbuf, sizeof(bitbuf));
        switch (obj_get_affected(obj)[i].location) {
          case APPLY_FEAT:
            buf2 = str_format(" ({})", g->feat_list[obj_get_affected(obj)[i].specific].name);
            break;
          case APPLY_SKILL:
            buf2 = str_format(" ({})", g->spell_info[obj_get_affected(obj)[i].specific].name);
            break;
          default:
            buf2.clear();
            break;
        }
        send_to_char(g, ch, "   Affects: {}{} By {:d}\r\n", bitbuf, buf2,
                     obj_get_affected(obj)[i].modifier);
      }
    }
  }
}

/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{
  int i;

  if (ch == nullptr || obj == nullptr)
    return;

  /* Either already enchanted or not a weapon. */
  if (obj_get_type_flag(obj) != ITEM_WEAPON || obj_extra_flagged(obj, ITEM_MAGIC))
    return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj_get_affected(obj)[i].location != APPLY_NONE)
      return;

  obj_extra_set(obj, ITEM_MAGIC, true);

  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj_get_affected(obj)[i].location == APPLY_NONE) {
      obj_get_affected(obj)[i].location = APPLY_ACCURACY;
      obj_get_affected(obj)[i].modifier = 1 + (level >= 18);
      break;
    }
  }

  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj_get_affected(obj)[i].location == APPLY_NONE) {
      obj_get_affected(obj)[i].location = APPLY_DAMAGE;
      obj_get_affected(obj)[i].modifier = 1 + (level >= 20);
      break;
    }
  }

  if (char_is_good(ch)) {
    obj_extra_set(obj, ITEM_ANTI_EVIL, true);
    act(g, "$p glows blue.", false, ch, obj, nullptr, TO_CHAR);
  } else if (char_is_evil(ch)) {
    obj_extra_set(obj, ITEM_ANTI_GOOD, true);
    act(g, "$p glows red.", false, ch, obj, nullptr, TO_CHAR);
  } else
    act(g, "$p glows yellow.", false, ch, obj, nullptr, TO_CHAR);
}

ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (char_aff_flagged(victim, AFF_POISON))
        send_to_char(g, ch, "You can sense poison in your blood.\r\n");
      else
        send_to_char(g, ch, "You feel healthy.\r\n");
    } else {
      if (char_aff_flagged(victim, AFF_POISON))
        act(g, "You sense that $E is poisoned.", false, ch, nullptr, victim, TO_CHAR);
      else
        act(g, "You sense that $E is healthy.", false, ch, nullptr, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (obj_get_type_flag(obj)) {
      case ITEM_DRINKCON:
      case ITEM_FOUNTAIN:
      case ITEM_FOOD:
        if (obj_get_value(obj, VAL_FOOD_POISON))
          act(g, "You sense that $p has been contaminated.", false, ch, obj, nullptr, TO_CHAR);
        else
          act(g, "You sense that $p is safe for consumption.", false, ch, obj, nullptr,
              TO_CHAR);
        break;
      default:
        send_to_char(g, ch, "You sense that it should not be consumed.\r\n");
    }
  }
}

ASPELL(spell_portal)
{
  struct obj_data *portal, *tportal;
  struct room_data *rm;
  rm = room_of(g, char_get_in_room(victim));

  if (ch == nullptr || victim == nullptr)
    return;

  if (!can_edit_zone(g, ch, room_get_zone(rm)) && zone_flagged(g, room_get_zone(rm), ZONE_QUEST)) {
    send_to_char(g, ch, "That target is in a quest zone.\r\n");
    return;
  }

  if (zone_flagged(g, room_get_zone(rm), ZONE_CLOSED) && char_get_admlevel(ch) < ADMLVL_IMMORT) {
    send_to_char(g, ch, "That target is in a closed zone.\r\n");
    return;
  }

  if (zone_flagged(g, room_get_zone(rm), ZONE_NOIMMORT) && char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "That target is in a zone closed to all.\r\n");
    return;
  }

  /* create the portal */
  portal = read_object(g, portal_object);
  obj_set_value(portal, VAL_PORTAL_DEST, char_get_in_room(victim));
  obj_set_value(portal, VAL_PORTAL_HEALTH, 100);
  obj_set_value(portal, VAL_PORTAL_MAXHEALTH, 100);
  obj_set_timer(portal, level / 10);
  add_unique_id(g, portal);
  obj_to_room(g, portal, char_get_in_room(ch));
  act(g, "$n opens a portal in thin air.",
      true, ch, nullptr, nullptr, TO_ROOM);
  act(g, "You open a portal out of thin air.",
      true, ch, nullptr, nullptr, TO_CHAR);
  /* create the portal at the other end */
  tportal = read_object(g, portal_object);
  obj_set_value(tportal, VAL_PORTAL_DEST, char_get_in_room(ch));
  obj_set_value(tportal, VAL_PORTAL_HEALTH, 100);
  obj_set_value(tportal, VAL_PORTAL_MAXHEALTH, 100);
  obj_set_timer(tportal, level / 10);
  add_unique_id(g, portal);
  obj_to_room(g, tportal, char_get_in_room(victim));
  act(g, "A shimmering portal appears out of thin air.",
      true, victim, nullptr, nullptr, TO_ROOM);
  act(g, "A shimmering portal opens here for you.",
      true, victim, nullptr, nullptr, TO_CHAR);
}

ASPELL(art_abundant_step)
{
  int steps, i = 0, j, rep, max;
  room_vnum r, nextroom;
  char buf[MAX_INPUT_LENGTH], tc;
  const char *p;

  steps = 0;
  r     = char_get_in_room(ch);
  p     = arg;
  max   = 10 + char_get_class_ranks(ch, CLASS_KABITO) / 2;

  while (p && *p && !isdigit(*p) && !isalpha(*p)) p++;

  if (!p || !*p) {
    send_to_char(g, ch, "You must give directions from your current location. Examples:\r\n  w w nw n e\r\n  2w nw n e\r\n");
    return;
  }

  while (*p) {
    while (*p && !isdigit(*p) && !isalpha(*p)) p++;
    if (isdigit(*p)) {
      rep = atoi(p);
      while (isdigit(*p)) p++;
    } else
      rep = 1;
    if (isalpha(*p)) {
      for (i = 0; isalpha(*p); i++, p++) buf[i] = tolower(*p);
      j      = i;
      tc     = buf[i];
      buf[i] = 0;
      for (i = 1; g->complete_cmd_info[i].command_pointer == do_move && strcmp(g->complete_cmd_info[i].sort_as, buf); i++);
      if (g->complete_cmd_info[i].command_pointer == do_move) {
        i = g->complete_cmd_info[i].subcmd - 1;
      } else
        i = -1;
      buf[j] = tc;
    }
    if (i > -1)
      while (rep--) {
        if (++steps > max)
          break;
        if (!room_get_dir_option(g, r, i)) {
          send_to_char(g, ch, "Invalid step. Skipping.\r\n");
          break;
        }
        nextroom = room_get_dir_option(g, r, i)->to_room;
        if (nextroom == NOWHERE)
          break;
        r = nextroom;
      }
    if (steps > max)
      break;
  }
  send_to_char(g, ch, "Your will bends reality as you travel through the ethereal plane.\r\n");
  act(g, "$n is suddenly absent.", true, ch, nullptr, nullptr, TO_ROOM);

  char_from_room(g, ch);
  char_to_room(g, ch, r);

  act(g, "$n is suddenly present.", true, ch, nullptr, nullptr, TO_ROOM);

  look_at_room(g, char_get_in_room(ch), ch, 0);

  return;
}

int roll_skill(GameWorld *g, const struct char_data *ch, int snum)
{
  int roll, skval, i;
  if (!char_is_npc(ch)) {
    skval = char_get_skill(ch, snum);
    if (SKILL_SPOT == snum) {
      if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4)) {
        skval += 5;
      }
    } else if (SKILL_HIDE == snum) {
      if (char_aff_flagged(ch, AFF_LIQUEFIED)) {
        skval += 5;
      } else if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 5 || char_get_genome(ch, 1) == 5)) {
        skval += 10;
      }
    }
  } else {
    int numb = 0;
    if (char_get_level(ch) <= 10) {
      numb = rand_number(g, 15, 30);
    }
    if (char_get_level(ch) <= 20) {
      numb = rand_number(g, 20, 40);
    }
    if (char_get_level(ch) <= 30) {
      numb = rand_number(g, 40, 60);
    }
    if (char_get_level(ch) <= 60) {
      numb = rand_number(g, 60, 80);
    }
    if (char_get_level(ch) <= 80) {
      numb = rand_number(g, 70, 90);
    }
    if (char_get_level(ch) <= 90) {
      numb = rand_number(g, 80, 95);
    }
    if (char_get_level(ch) <= 100) {
      numb = rand_number(g, 90, 100);
    }
    skval = numb;
  }
  if (snum == SKILL_SPOT && char_get_skill(ch, SKILL_LISTEN)) {
    skval += char_get_skill(ch, SKILL_LISTEN) / 10;
  }
  if (snum < 0 || snum >= SKILL_TABLE_SIZE)
    return 0;
  if (IS_SET(g->spell_info[snum].skilltype, SKTYPE_SPELL)) {
    /*
     * There's no real roll for a spell to succeed, so instead we will
     * return the spell resistance roll; the defender must have resistance
     * higher than this roll to avoid it. Most spells should also have some
     * kind of save called after roll_skill.
     */
    for (i = 0, roll = 0; i < NUM_CLASSES; i++)
      if (char_get_class_ranks(ch, i) &&
          (g->spell_info[snum].min_level[i] < char_get_class_ranks(ch, i)))
        roll += char_get_class_ranks(ch, i); /* Caster level for eligable classes */
    return roll + rand_number(g, 1, 20);
  }
  if (IS_SET(g->spell_info[snum].skilltype, SKTYPE_SKILL)) {
    if (!skval && IS_SET(g->spell_info[snum].flags, SKFLAG_NEEDTRAIN)) {
      return -1;
    }
    roll = skval;
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_STRMOD))
      roll += ability_mod_value(char_get_aff_abils_str(ch));
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_DEXMOD))
      roll += dex_mod_capped(ch);
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_CONMOD))
      roll += ability_mod_value(char_get_aff_abils_con(ch));
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_INTMOD))
      roll += ability_mod_value(char_get_aff_abils_intel(ch));
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_WISMOD))
      roll += ability_mod_value(char_get_aff_abils_wis(ch));
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_CHAMOD))
      roll += ability_mod_value(char_get_aff_abils_cha(ch));
    if (IS_SET(g->spell_info[snum].flags, SKFLAG_ARMORALL))
      roll -= 0;
    else if (IS_SET(g->spell_info[snum].flags, SKFLAG_ARMORBAD))
      roll -= 0;
    return roll + rand_number(g, 1, 20);
  }
  mud_log("Trying to roll uncategorized skill/spell #{:d} for {}", snum, char_get_name(ch));
  return 0;
}

bool roll_resisted(GameWorld *g, const struct char_data *actor, int sact, const struct char_data *resistor, int sres)
{
  return roll_skill(g, actor, sact) >= roll_skill(g, resistor, sres);
}
