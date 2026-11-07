/* ************************************************************************
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/weather.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.other.hpp"
#include "dbat/config.hpp"
#include "dbat/races.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void oozaru_add(GameWorld *g);
static void oozaru_drop(GameWorld *g);

static void another_hour(GameWorld *g, int mode);
static void weather_change(GameWorld *g);
static void phase_powerup(GameWorld *g, struct char_data *ch, int phase);
static void grow_plants(GameWorld *g);

static void grow_plants(GameWorld *g)
{
  struct obj_data *k;

  for (k = g->objects; k; k = obj_get_next(k)) {
    if (obj_get_in_room(k) == NOWHERE) {
      continue;
    }
    if (obj_get_carried_by(k) || obj_get_in_obj(k)) {
      continue;
    }
    if (rroom_flagged(g, obj_get_in_room(k), ROOM_GARDEN1) || rroom_flagged(g, obj_get_in_room(k), ROOM_GARDEN2)) {
      if (obj_get_value(k, VAL_WATERLEVEL) < 0 && obj_get_value(k, VAL_WATERLEVEL) > -10) {
        obj_set_value(k, VAL_WATERLEVEL, obj_get_value(k, VAL_WATERLEVEL) - (1));
        if (obj_get_value(k, VAL_WATERLEVEL) > -10) {
          send_to_room(g, obj_get_in_room(k), "%s@y withers a bit.\r\n", obj_get_short_description(k));
        } else {
          send_to_room(g, obj_get_in_room(k), "%s@y has withered to a dried up dead husk.\r\n", obj_get_short_description(k));
        }
      } else if (obj_get_value(k, VAL_WATERLEVEL) >= 0) {
        obj_set_value(k, VAL_WATERLEVEL, obj_get_value(k, VAL_WATERLEVEL) - (1));
        if (obj_get_value(k, VAL_GROWTH) < obj_get_value(k, VAL_MATGOAL) && obj_get_value(k, VAL_MATURITY) < obj_get_value(k, VAL_MAXMATURE)) {
          obj_set_value(k, VAL_GROWTH, obj_get_value(k, VAL_GROWTH) + (1));
          if (obj_get_value(k, VAL_GROWTH) >= obj_get_value(k, VAL_MATGOAL)) {
            obj_set_value(k, VAL_GROWTH, 0);
            obj_set_value(k, VAL_MATURITY, obj_get_value(k, VAL_MATURITY) + (1));
          }
          if (obj_get_value(k, VAL_MATURITY) >= obj_get_value(k, VAL_MAXMATURE)) {
            send_to_room(g, obj_get_in_room(k), "%s@G is now fully grown!@n\r\n", obj_get_short_description(k));
          }
        }
      }
    }
  }
}

void weather_and_time(GameWorld *g, int mode)
{
  another_hour(g, mode);
  grow_plants(g);
  if (mode)
    weather_change(g);
}

static void another_hour(GameWorld *g, int mode)
{
  g->time_info.hours++;

  if (mode) {
    switch (g->time_info.hours) {
      case 4:
        if (MOON_DATE) {
          send_to_moon(g, "The full moon disappears.\r\n");
        } else if (g->time_info.day == 22) {
          send_to_moon(g, "The full moon disappears.\r\n");
        }
        break;
      case 5:
        g->weather_info.sunlight = SUN_RISE;
        send_to_outdoor(g, "The sun rises in the east.\r\n");
        if (g->time_info.day <= 14) {
          star_phase(g, nullptr, 1);
        } else if (g->time_info.day <= 21) {
          star_phase(g, nullptr, 2);
        } else {
          star_phase(g, nullptr, 0);
        }
        break;
      case 6:
        g->weather_info.sunlight = SUN_LIGHT;
        send_to_outdoor(g, "The day has begun.\r\n");
        break;
      case 19:
        g->weather_info.sunlight = SUN_SET;
        send_to_outdoor(g, "The sun slowly disappears in the west.\r\n");
        break;
      case 20:
        g->weather_info.sunlight = SUN_DARK;
        send_to_outdoor(g, "The night has begun.\r\n");
        break;
      case 21:
        if (MOON_DATE) {
          send_to_moon(g, "The full moon has risen.\r\n");
        }
        break;
      default:
        break;
    }
  }

  if (MOON_TIMECHECK(g)) {
    g->MOON_UP = true;
    oozaru_add(g);
  } else {
    g->MOON_UP = false;
    oozaru_drop(g);
  }

  if (g->time_info.hours > 23) { /* Changed by HHS due to bug ??? */
    g->time_info.hours -= 24;
    g->time_info.day++;

    if (g->time_info.day > 29) {
      g->time_info.day = 0;
      g->time_info.month++;

      if (g->time_info.month > 11) {
        g->time_info.month = 0;
        g->time_info.year++;
      }
    }
  }
}

static void weather_change(GameWorld *g)
{
  int diff, change;
  if ((g->time_info.month >= 9) && (g->time_info.month <= 16))
    diff = (g->weather_info.pressure > 985 ? -2 : 2);
  else
    diff = (g->weather_info.pressure > 1015 ? -2 : 2);

  g->weather_info.change += (dice(g, 1, 4) * diff + dice(g, 2, 6) - dice(g, 2, 6));

  g->weather_info.change = MIN(g->weather_info.change, 12);
  g->weather_info.change = MAX(g->weather_info.change, -12);

  g->weather_info.pressure += g->weather_info.change;

  g->weather_info.pressure = MIN(g->weather_info.pressure, 1040);
  g->weather_info.pressure = MAX(g->weather_info.pressure, 960);

  change = 0;

  switch (g->weather_info.sky) {
    case SKY_CLOUDLESS:
      if (g->weather_info.pressure < 990)
        change = 1;
      else if (g->weather_info.pressure < 1010)
        if (dice(g, 1, 4) == 1)
          change = 1;
      break;
    case SKY_CLOUDY:
      if (g->weather_info.pressure < 970)
        change = 2;
      else if (g->weather_info.pressure < 990) {
        if (dice(g, 1, 4) == 1)
          change = 2;
        else
          change = 0;
      } else if (g->weather_info.pressure > 1030)
        if (dice(g, 1, 4) == 1)
          change = 3;

      break;
    case SKY_RAINING:
      if (g->weather_info.pressure < 970) {
        if (dice(g, 1, 4) == 1)
          change = 4;
        else
          change = 0;
      } else if (g->weather_info.pressure > 1030)
        change = 5;
      else if (g->weather_info.pressure > 1010)
        if (dice(g, 1, 4) == 1)
          change = 5;

      break;
    case SKY_LIGHTNING:
      if (g->weather_info.pressure > 1010)
        change = 6;
      else if (g->weather_info.pressure > 990)
        if (dice(g, 1, 4) == 1)
          change = 6;

      break;
    default:
      change           = 0;
      g->weather_info.sky = SKY_CLOUDLESS;
      break;
  }

  switch (change) {
    case 0:
      break;
    case 1:
      send_to_outdoor(g, "The sky starts to get cloudy.\r\n");
      g->weather_info.sky = SKY_CLOUDY;
      break;
    case 2:
      send_to_outdoor(g, "It starts to rain.\r\n");
      g->weather_info.sky = SKY_RAINING;
      break;
    case 3:
      send_to_outdoor(g, "The clouds disappear.\r\n");
      g->weather_info.sky = SKY_CLOUDLESS;
      break;
    case 4:
      send_to_outdoor(g, "Lightning starts to show in the sky.\r\n");
      g->weather_info.sky = SKY_LIGHTNING;
      break;
    case 5:
      send_to_outdoor(g, "The rain stops.\r\n");
      g->weather_info.sky = SKY_CLOUDY;
      break;
    case 6:
      send_to_outdoor(g, "The lightning stops.\r\n");
      g->weather_info.sky = SKY_RAINING;
      break;
    default:
      break;
  }
}

void oozaru_transform(GameWorld *g, char_data *ch)
{
  if (char_plr_flagged(ch, PLR_OOZARU))
    return;

  act(g, "@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Hair sprouts  all over your body and your teeth become sharp as your body takes on the Oozaru form!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant ape!@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, oozaru.flag, true);
}

static void oozaru_add(GameWorld *g)
{
  for (descriptor_data *d = g->descriptors; d; d = d->next) {
    if (!desc_is_playing(d)) {
      continue;
    }
    if (char_moon_ok(g, d->character) && !char_plr_flagged(d->character, PLR_OOZARU)) {
      oozaru_transform(g, d->character);
    }
  }
}

void oozaru_revert(GameWorld *g, char_data *ch)
{
  if (!char_plr_flagged(ch, PLR_OOZARU))
    return;

  act(g, "@CYour body begins to shrink back to its normal form as the power of the Oozaru leaves you. You fall asleep shortly after returning to normal!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@c$n@C's body begins to shrink and return to normal. Their giant ape features fading back into humanoid features until $e is left normal and asleep.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_position(ch, POS_SLEEPING);

  char_plr_set(ch, PLR_OOZARU, false);
}

static void oozaru_drop(GameWorld *g)
{
  for (descriptor_data *d = g->descriptors; d; d = d->next) {
    if (!desc_is_playing(d))
      continue;
    oozaru_revert(g, d->character);
  }
}

static void check_hoshi_phase(GameWorld *g, struct char_data *ch, int type)
{
  if (char_is_npc(ch)) return;
  if (char_get_level(ch) < 2) return;
  if (!char_is_hoshijin(ch)) return;
  switch (type) {
    case 0:
      if (char_get_starphase(ch) > 0) {
        act(g, "@WYour eyes and the glyphs on your skin slowly start to lose their glow. You feel the power received from the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W drain away from your body. It has apparently entered the @rDeath Phase@W of its cycle...@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@c$n@W's eyes and the glyphs on $s skin slowly start to lose their glow. You notice that $e seems weaker now for some reason.@n", true, ch, nullptr, nullptr, TO_ROOM);
        phase_powerup(g, ch, 0);
      }
      break; // Drop Powerup
    case 1:
      if (char_get_starphase(ch) != 1) {
        act(g, "@WYou suddenly feel a @RSURGE@W of power through your body. You feel the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W come into its @CBirth Phase@W and its power is flowing into your body! Finally your eyes and the glyphs on your skin begin to glow an electric @bb@Bl@Cue@W!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@c$n@W suddenly seems to grow stronger for some reason. You notice $s eyes begin to glow an electric @bb@Bl@Cue@W. Suddenly glyphs start to appear all over $s skin and glow with the same light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        phase_powerup(g, ch, 1);
      }
      break; // Powerup Phase 1
    case 2:
      if (char_get_starphase(ch) != 2) {
        act(g, "@WYou suddenly feel a @RSURGE@W of power through your body. You feel the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W come into its @GLife Phase@W and its power is flowing into your body! Finally your eyes and the glyphs on your skin begin to glow an fiery @Rr@re@Rd@W!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@c$n@W suddenly seems to grow stronger for some reason. You notice $s eyes begin to glow a fiery @rR@Re@rd@W. Suddenly glyphs start to appear all over $s skin and glow with the same light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        phase_powerup(g, ch, 2);
      }
      break; // Powerup Phase 2
    default:
      send_to_imm(g, "Strange Error in star_phase by: %s", char_get_name(ch));
      break; // Error
  }
}

/* This controls the powering up of Hoshi-jin from their Eldritch Star */
void star_phase(GameWorld *g, struct char_data *ch, int type)
{
  struct descriptor_data *d;

  if (ch != nullptr) {
    check_hoshi_phase(g, ch, type);
    return;
  }

  for (d = g->descriptors; d; d = d->next) {
    if (!desc_is_playing(d)) {
      continue;
    }
    check_hoshi_phase(g, d->character, type);
  }
}

static void phase_powerup(GameWorld *g, struct char_data *ch, int phase)
{
  if (char_is_npc(ch)) {
    return;
  }

  // clear existing bonus...
  remove_affect(g, ch, AFF_STARPHASE);

  int bonus = 0;

  switch (phase) {
    case 0:
      return;
    case 1:
      bonus = 5;
      break;
    case 2:
      bonus = 8;
      break;
    default:
      send_to_imm(g, "Error: phase_powerup called with GET_PHASE equal to zero by: %s", char_get_name(ch));
      return;
  }

  if (bonus > 0) {
    assign_affect(g, ch, AFF_STARPHASE, 0, -1, bonus, 0, 0, 0, 0, bonus);
  }
  char_set_starphase(ch, phase);
  save_char(g, ch);
}

