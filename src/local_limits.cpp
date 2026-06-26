/* ************************************************************************
 *   File: limits.c                                      Part of CircleMUD *
 *  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "local_limits.h"
#include "config.h"

#include "act.item.h"
#include "act.other.h"
#include "alias.h"
#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "comm.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/bonus.h"
#include "consts/constates.h"
#include "consts/fightprefs.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "consts/skills.h"
#include "consts/weather.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dg_comm.h"
#include "dg_scripts.h"
#include "extract.h"
#include "fight.h"
#include "flags.h"
#include "handler.h"
#include "log.h"
#include "object_api.h"
#include "object_impl.h"
#include "object_macros.h"
#include "objsave.h"
#include "random.h"
#include "relocate.h"
#include "room_api.h"

#include "room_utils.h"
#include "spells.h"
#include "stringutils.h"
#include "util_macros.h"
#include "vehicles.h"
#include "weather_db.h"

#include "iterate.hpp"

#include <cstring>
#include <unistd.h>

/* local defines */
#define sick_fail 2

/* local functions */
static void update_flags(struct char_data *ch);

static int wearing_stardust(struct char_data *ch);

static void check_idling(struct char_data *ch);

/* If they have the Healthy trait then they have a chance to lose each of these
 */

static int wearing_stardust(struct char_data *ch) {

  int count = 0;

  char_equipment_iterate(ch, [&](auto i, auto eq) {
    if (i == 0) return true;
    switch (GET_OBJ_VNUM(eq)) {
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
    return true;
  });

  return count == 26;
}


static void update_flags(struct char_data *ch) {
  if (ch == NULL) {
    send_to_imm("ERROR: Empty ch variable sent to update_flags.");
    return;
  }

  if (char_condition_has(ch, "knocked_out") && !FIGHTING(ch)) {
    cureStatusKnockedOutAnnounced(ch, true);
  }

  if (wearing_stardust(ch) == 1) {
    char_condition_add(ch, "zanzoken", "skill", "zanzoken");
    send_to_char(ch, "The stardust armor blesses you with a free zanzoken when "
                     "you next need it.\r\n");
  }
}

void set_title(struct char_data *ch, char *title) {
  if (ch) {
    send_to_char(ch, "Title is disabled for the time being while Iovan works "
                     "on a brand new and fancier title system.\r\n");
    return;
  }
}

void gain_level(struct char_data *ch, int whichclass) {
  if (whichclass < 0)
    whichclass = GET_CLASS(ch);
  if (GET_LEVEL(ch) < 100 && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
    char_stat_mod(ch, "level", 1);
    // GET_CLASS(ch) = whichclass; /* Now tracks latest class instead of highest
    // */
    advance_level(ch, whichclass);
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s advanced level to level %d.", GET_NAME(ch), GET_LEVEL(ch));
    send_to_char(ch, "You rise a level!\r\n");
    char_stat_mod(ch, "experience", -level_exp(ch, GET_LEVEL(ch)));
    /*set_title(ch, NULL);*/
    write_aliases(ch);
    save_char(ch);
  }
}

void run_autowiz(void) {
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ) {
    size_t res;
    char buf[256];

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
                   CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT,
                   IMMLIST_FILE, (int)getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
                   CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT,
                   IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf)) {
      mudlog(CMP, ADMLVL_IMMORT, FALSE, "Initiating autowiz.");
      system(buf);
      reboot_wizlists();
    } else
      mud_log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

void gain_exp(struct char_data *ch, int64_t gain) {

  if (gain > 20000000) {
    gain = 20000000;
  }

  if (IN_ARENA(ch)) {
    send_to_char(ch,
                 "EXP CANCEL: You can not gain experience from the arena.\r\n");
    return;
  }

  if (char_condition_has(ch, "rune_wunjo")) {
    gain += gain * 0.15;
  }
  if (PLR_FLAGGED(ch, PLR_IMMORTAL)) {
    gain = gain * 0.95;
  }

  int64_t diff = gain * 0.15;

  if (!IS_NPC(ch) && GET_LEVEL(ch) < 1)
    return;

  if (IS_NPC(ch)) {
    char_stat_mod(ch, "experience", gain);
    return;
  }

  if (gain > 0) {
    gain =
        MIN(CONFIG_MAX_EXP_GAIN, gain); /* put a cap on the max gain per kill */
    if (GET_EQ(ch, WEAR_SH)) {
      struct obj_data *obj = GET_EQ(ch, WEAR_SH);
      if (GET_OBJ_VNUM(obj) == 1127) {
        int64_t spar = gain;
        gain += gain * 2.5;
        spar = gain - spar;
        send_to_char(ch, "@D[@BBooster EXP@W: @G+%s@D]\r\n", add_commas(spar));
      }
    }
    if (GET_LEVEL(ch) < 100) {
      if (MINDLINK(ch) && gain > 0 && LINKER(ch) == 0) {
        if (GET_LEVEL(ch) + 20 < GET_LEVEL(MINDLINK(ch)) ||
            GET_LEVEL(ch) - 20 > GET_LEVEL(MINDLINK(ch))) {
          send_to_char(MINDLINK(ch),
                       "The level difference between the two of you is too "
                       "great to gain from mind read.\r\n");
        } else {
          act("@GYou've absorbed some new experiences from @W$n@G!@n", FALSE,
              ch, 0, MINDLINK(ch), TO_VICT);
          int read = gain * 0.12;
          gain -= read;
          if (read == 0)
            read = 1;
          gain_exp(MINDLINK(ch), read);
          act("@RYou sense that @W$N@R has stolen some of your experiences "
              "with $S mind!@n",
              FALSE, ch, 0, MINDLINK(ch), TO_CHAR);
        }
      }
      int64_t difff = level_exp(ch, GET_LEVEL(ch) + 1) * 5;
      if (GET_LEVEL(ch) <= 90 &&
          (level_exp(ch, GET_LEVEL(ch) + 1) - (GET_EXP(ch) + gain) <=
           (level_exp(ch, GET_LEVEL(ch) + 1) - difff))) {
        send_to_char(ch, "@WYou -@RNEED@W- to @ylevel@W you can't hold any "
                         "more experience.@n\r\n");
      } else if (GET_LEVEL(ch) >= 91 &&
                 level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) <= -1) {
        send_to_char(ch, "@WYou -@RNEED@W- to @ylevel@W you can't hold any "
                         "more experience.@n\r\n");
      } else {
        char_stat_mod(ch, "experience", gain);
      }
    }
    if (GET_LEVEL(ch) < 100 && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
      send_to_char(
          ch, "@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");

    if (GET_LEVEL(ch) == 100 && GET_ADMLEVEL(ch) < 1) {
      if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
        diff = diff * 1.3;
      }
      if (IS_ANDROID(ch)) {
        diff = diff * 1.2;
      }
      if (MINDLINK(ch) && gain > 0 && LINKER(ch) == 0) {
        if (GET_LEVEL(ch) + 20 < GET_LEVEL(MINDLINK(ch)) ||
            GET_LEVEL(ch) - 20 > GET_LEVEL(MINDLINK(ch))) {
          send_to_char(MINDLINK(ch),
                       "The level difference between the two of you is too "
                       "great to gain from mind read.\r\n");
        } else {
          act("@GYou've absorbed some new experiences from @W$n@G!@n", FALSE,
              ch, 0, MINDLINK(ch), TO_VICT);
          int64_t read = gain * 0.12;
          diff -= (read * 0.15);
          gain -= read;
          if (read == 0)
            read = 1;
          gain_exp(MINDLINK(ch), read);
          act("@RYou sense that @W$N@R has stolen some of your experiences "
              "with $S mind!@n",
              FALSE, ch, 0, MINDLINK(ch), TO_CHAR);
        }
      }
      if (rand_number(1, 5) >= 2) {
        if (IS_HUMAN(ch)) {
          gainBasePL(ch, diff * 0.8);
        } else {
          gainBasePL(ch, diff);
        }
        send_to_char(ch, "@D[@G+@Y%s @RPL@D]@n ", add_commas(diff));
      }
      if (rand_number(1, 5) >= 2) {
        if (IS_HALFBREED(ch)) {
          gainBaseST(ch, diff * 0.85);
        } else {
          gainBaseST(ch, diff);
        }
        send_to_char(ch, "@D[@G+@Y%s @gSTA@D]@n ", add_commas(diff));
      }
      if (rand_number(1, 5) >= 2) {
        gainBaseKI(ch, diff);
        send_to_char(ch, "@D[@G+@Y%s @CKi@D]@n", add_commas(diff));
      }
    }
  } else if (gain < 0) {
    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain); /* Cap max exp lost per death */
    char_stat_mod(ch, "experience", gain);
    if (GET_EXP(ch) < 0)
      char_stat_set(ch, "experience", 0);
  }
}

void gain_exp_regardless(struct char_data *ch, int gain) {
  int is_altered = FALSE;
  int num_levels = 0;

  gain = (gain * CONFIG_EXP_MULTIPLIER);

  char_stat_mod(ch, "experience", gain);
  if (GET_EXP(ch) < 0)
    char_stat_set(ch, "experience", 0);

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 1 &&
           GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
      char_stat_mod(ch, "level", 1);
      num_levels++;
      advance_level(ch, GET_CLASS(ch));
      is_altered = TRUE;
    }

    if (is_altered) {
      mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
             "%s advanced %d level%s to level %d.", GET_NAME(ch), num_levels,
             num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
        send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      /*set_title(ch, NULL);*/
    }
  }
}

void gain_condition(struct char_data *ch, int condition, int value) {
  const char *condition_name;
  bool intoxicated;

  switch (condition) {
  case DRUNK:
    condition_name = "drunk";
    break;
  case HUNGER:
    condition_name = "hunger";
    break;
  case THIRST:
    condition_name = "thirst";
    break;
  default:
    return;
  }

  if (IS_NPC(ch))
    return;

  if (IS_ANDROID(ch)) {
    return;
  }

  if (char_stat_get(ch, condition_name) < 0) { /* No change */
    return;
  }

  if (char_room_vnum_get(ch) <= 1) {
    return;
  }

  if (PLR_FLAGGED(ch, PLR_WRITING))
    return;

  intoxicated = (char_stat_get(ch, "drunk") > 0);
  if (value > 0) {
    if (char_stat_get(ch, condition_name) >= 0) {
      if (char_stat_get(ch, condition_name) + value > 48) {
        int prior = char_stat_get(ch, condition_name);
        char_stat_set(ch, condition_name, 48);
        if (condition != DRUNK && prior >= 48 && !IS_MAJIN(ch)) {
          int ocond = condition;
          if (condition == HUNGER)
            ocond = THIRST;
          else if (condition == THIRST)
            ocond = HUNGER;
        }
      } else {
        char_stat_mod(ch, condition_name, value);
      }
    }
  } else {
    if (char_stat_get(ch, condition_name) >= 0) {
      if (char_stat_get(ch, condition_name) + value < 0) {
        char_stat_set(ch, condition_name, 0);
      } else {
        char_stat_mod(ch, condition_name, value);
      }
    }
  }
  switch (condition) {
  case HUNGER:
    switch (char_stat_get(ch, condition_name)) {
    case 0:
      // send_to_char(ch, "@RYou are feeling ravenous!@n\r\n");
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
    switch (char_stat_get(ch, condition_name)) {
    case 0:
      // send_to_char(ch, "@RYou are dehydrated!@n\r\n");
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
    if (intoxicated) {
      if (char_stat_get(ch, "drunk") <= 0) {
        send_to_char(ch, "You are now sober.\r\n");
      }
    }
    break;
  default:
    break;
  }
}

static void check_idling(struct char_data *ch) {
  if (dball_count(ch)) {
    return;
  }

  struct room_data *room = char_room_get(ch);

  if (++(ch->timer) > CONFIG_IDLE_VOID) {
    if (GET_WAS_IN(ch) == NOWHERE && room) {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch)) {
        stop_fighting(FIGHTING(ch));
        stop_fighting(ch);
      }

      room_vnum v = room_vnum_get(room);

      if (!room_flagged(room, ROOM_PAST) && (v < 19800 || v > 19899)) {
        GET_LOADROOM(ch) = v;
      }
      if (room_flagged(room, ROOM_PAST)) {
        GET_LOADROOM(ch) = room_vnum_check(1561);
      }
      if (v >= 2002 && v <= 2011) {
        GET_LOADROOM(ch) = room_vnum_check(1960);
      }
      if (v == 2069) {
        GET_LOADROOM(ch) = room_vnum_check(2017);
      }
      if (v == 2070) {
        GET_LOADROOM(ch) = room_vnum_check(2046);
      }
      if (v >= 101 && v <= 139) {
        if (GET_LEVEL(ch) == 1) {
          GET_LOADROOM(ch) = room_vnum_check(100);
          char_stat_set(ch, "experience", 0);
        } else {
          if (IS_ROSHI(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(1130);
          }
          if (IS_KABITO(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(12098);
          }
          if (IS_NAIL(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(11683);
          }
          if (IS_BARDOCK(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(2268);
          }
          if (IS_KRANE(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(13009);
          }
          if (IS_TAPION(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(8231);
          }
          if (IS_PICCOLO(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(1659);
          }
          if (IS_ANDSIX(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(1713);
          }
          if (IS_DABURA(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(6486);
          }
          if (IS_FRIEZA(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(4282);
          }
          if (IS_GINYU(ch)) {
            GET_LOADROOM(ch) = room_vnum_check(4289);
          }
        }
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch);
      char_from_room(ch);
      char_to_room(ch, room_by_id(1));
    } else if (ch->timer > CONFIG_IDLE_RENT_TIME) {
      if (char_room_get(ch) != NULL) {
        char_from_room(ch);
        char_to_room(ch, room_by_id(3));
      }
      if (ch->desc) {
        send_to_char(
            ch, "You are idle and are extracted safely from the game.\r\n");
        STATE(ch->desc) = CON_DISCONNECT;
        /*
         * For the 'if (d->character)' test in close().
         * -gg 3/1/98 (Happy anniversary.)
         */
        ch->desc->character = NULL;
        ch->desc = NULL;
      }
      Crash_rentsave(ch, 0);
      mudlog(CMP, ADMLVL_GOD, TRUE, "%s force-rented and extracted (idle).",
             GET_NAME(ch));
      extract_char(ch);
    }
  }
}



static void tick_char_sleep(struct char_data *i) {
  if (GET_SLEEPT(i) > 0 && GET_POS(i) != POS_SLEEPING)
    GET_SLEEPT(i) -= 1;
  if (GET_SLEEPT(i) < 8 && GET_POS(i) == POS_SLEEPING) {
    GET_SLEEPT(i) += rand_number(2, 4);
    if (GET_SLEEPT(i) > 8) GET_SLEEPT(i) = 8;
  }
}


static void tick_char_heal_messages(struct char_data *i, bool change) {
  if (!change || char_condition_has(i, "poison")) return;
  if (PLR_FLAGGED(i, PLR_HEALT) && SITS(i) != NULL) {
    send_to_char(i, "@wThe healing tank works wonders on your injuries.@n\r\n");
    HCHARGE(SITS(i)) -= rand_number(1, 2);
    if (HCHARGE(SITS(i)) == 0) {
      send_to_char(i, "@wThe healing tank is now too low on energy to heal you.\r\n");
      act("You step out of the now empty healing tank.", TRUE, i, 0, 0, TO_CHAR);
      act("@C$n@w steps out of the now empty healing tank.@n", TRUE, i, 0, 0, TO_ROOM);
      REMOVE_BIT_AR(PLR_FLAGS(i), PLR_HEALT);
      SITTING(SITS(i)) = NULL;
      SITS(i) = NULL;
    } else if (isFullVitals(i)) {
      send_to_char(i, "@wYou are fully recovered now.\r\n");
      act("You step out of the now empty healing tank.", TRUE, i, 0, 0, TO_CHAR);
      act("@C$n@w steps out of the now empty healing tank.@n", TRUE, i, 0, 0, TO_ROOM);
      REMOVE_BIT_AR(PLR_FLAGS(i), PLR_HEALT);
      SITTING(SITS(i)) = NULL;
      SITS(i) = NULL;
    }
  } else if (PLR_FLAGGED(i, PLR_HEALT) && SITS(i) == NULL) {
    REMOVE_BIT_AR(PLR_FLAGS(i), PLR_HEALT);
  } else if (GET_POS(i) == POS_SLEEPING) {
    send_to_char(i, "@wYour sleep does you some good.@n\r\n");
    if (!IS_ANDROID(i) && !FIGHTING(i))
      restoreLFAnnounced(i, false);
  } else if (GET_POS(i) == POS_RESTING) {
    send_to_char(i, "@wYou feel relaxed and better.@n\r\n");
    if (!isFullLF(i) && !IS_ANDROID(i) && !FIGHTING(i) &&
        GET_SUPPRESS(i) <= 0 && GET_HIT(i) != getMaxPL(i)) {
      incCurLFPercent(i, .15);
      send_to_char(i, "@CYou feel more lively.@n\r\n");
    }
  } else if (GET_POS(i) == POS_SITTING) {
    send_to_char(i, "@wYou feel rested and better.@n\r\n");
  } else {
    send_to_char(i, "You feel slightly better.\r\n");
  }
}

namespace {
  inline struct timespec pu_now() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts;
  }
  inline double pu_elapsed(const struct timespec &a, const struct timespec &b) {
    return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1.0e6;
  }
  struct PUTimings {
    double relax = 0, flags = 0, vitals = 0, heal = 0,
           obj_upd = 0, innate = 0, idle = 0;
    int count = 0;
  };
}

static void process_char_point_update(struct char_data *i, PUTimings &t) {
  ++t.count;

  struct timespec tp = pu_now();
  if (IS_NPC(i)) i->aggtimer = 0;
  t.relax += pu_elapsed(tp, pu_now());

  if (GET_POS(i) >= POS_STUNNED) {
    bool change = false;

    tp = pu_now();
    update_flags(i);
    if (!IS_NPC(i) && !isFullVitals(i)) change = true;
    tick_char_sleep(i);
    t.flags += pu_elapsed(tp, pu_now());

    tp = pu_now();
    tick_char_heal_messages(i, change);
    if (GET_POS(i) <= POS_STUNNED) update_pos(i);
    t.heal += pu_elapsed(tp, pu_now());

  } else if (GET_POS(i) == POS_INCAP || GET_POS(i) == POS_MORTALLYW) {
    return;
  }

  if (getCurKI(i) >= GET_MAX_MANA(i) * 0.5 &&
      GET_CHARGE(i) < GET_MAX_MANA(i) * 0.1 &&
      GET_PREFERENCE(i) == PREFERENCE_KI && !PLR_FLAGGED(i, PLR_AURALIGHT))
    char_charge_set(i, (int64_t)(GET_MAX_MANA(i) * 0.1));

  if (!IS_NPC(i)) {
    tp = pu_now();
    update_char_objects(i);
    t.obj_upd += pu_elapsed(tp, pu_now());

    tp = pu_now();
    if (GET_ADMLEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
      check_idling(i);
    else
      (i->timer)++;
    t.idle += pu_elapsed(tp, pu_now());
  }
}

static void point_update_characters(void) {
  PUTimings t;
  const struct timespec t_total = pu_now();

  zone_iterate_active([&](struct zone_data *zone) {
    auto vnum = zone_id_get(zone);
    zone_players_iterate(vnum, [&](struct char_data *i) {
      process_char_point_update(i, t);
      return true;
    });
    zone_mobs_iterate(vnum, [&](struct char_data *i) {
      process_char_point_update(i, t);
      return true;
    });
    return true;
  });

  const double total_ms = pu_elapsed(t_total, pu_now());
  if (total_ms >= 100.0) {
    mud_log("SLOW point_update_chars %.0fms (%d ents): "
            "relax=%.0fms flags=%.0fms vitals=%.0fms heal=%.0fms "
            "char_objs=%.0fms innate=%.0fms idle=%.0fms",
            total_ms, t.count,
            t.relax, t.flags, t.vitals, t.heal,
            t.obj_upd, t.innate, t.idle);
  }
}

/* Returns true if the object was extracted. */
static bool tick_obj_norent(struct obj_data *j) {
  if (!OBJ_FLAGGED(j, ITEM_NORENT) || j->worn_by || j->carried_by ||
      obj_selling == j || GET_OBJ_VNUM(j) == 7200)
    return false;
  time_t diff = time(0) - GET_LAST_LOAD(j);
  if (diff > 240 && GET_LAST_LOAD(j) > 0) {
    mud_log("No rent object (%s) extracted from room (%d)",
        j->short_description, obj_room_vnum_get(j));
    extract_obj(j);
    return true;
  }
  return false;
}

static void tick_obj_healing_tank(struct obj_data *j) {
  if (GET_OBJ_VNUM(j) != 65) return;
  if (HCHARGE(j) < 20 && !SITTING(j))
    HCHARGE(j) += rand_number(0, 1);
}

/* Handles the mutually exclusive portal/vnum1306/generic-timer chain.
   Returns true if the object was extracted. */
static bool tick_obj_timed(struct obj_data *j) {
  if (GET_OBJ_TYPE(j) == ITEM_PORTAL) {
    if (GET_OBJ_TIMER(j) > 0) GET_OBJ_TIMER(j)--;
    if (GET_OBJ_TIMER(j) == 0) {
      act("A glowing portal fades from existence.", TRUE,
          room_people_get(obj_room_get(j)), j, 0, TO_ROOM);
      act("A glowing portal fades from existence.", TRUE,
          room_people_get(obj_room_get(j)), j, 0, TO_CHAR);
      extract_obj(j);
      return true;
    }
  /* vnum 1306 (ashcloud) lifecycle moved to lua/objects/scripts/ashcloud.lua */
  } else if (GET_OBJ_TIMER(j) > 0) {
    GET_OBJ_TIMER(j)--;
    if (!GET_OBJ_TIMER(j))
      timer_otrigger(j);
  }
  return false;
}

static void tick_obj_corpse(struct obj_data *j) {
  if (GET_OBJ_TIMER(j) > 0) GET_OBJ_TIMER(j)--;

  if (!strstr(j->name, "android") && !strstr(j->name, "Android") &&
      !OBJ_FLAGGED(j, ITEM_BURIED)) {
    auto oroom = obj_room_get(j);
    static const struct { int timer; const char *msg; } decay_msgs[] = {
      {5, "@DFlies start to gather around $p@D.@n"},
      {3, "@DA cloud of flies has formed over $p@D.@n"},
      {2, "@DMaggots can be seen crawling all over $p@D.@n"},
      {1, "@DMaggots have nearly stripped $p of all its flesh@D.@n"},
    };
    for (auto &dm : decay_msgs) {
      if (GET_OBJ_TIMER(j) == dm.timer && oroom && room_people_get(oroom)) {
        act(dm.msg, TRUE, room_people_get(oroom), j, 0, TO_CHAR);
        act(dm.msg, TRUE, room_people_get(oroom), j, 0, TO_ROOM);
        break;
      }
    }
  }

  if (GET_OBJ_TIMER(j) != 0) return;

  auto oroom = obj_room_get(j);
  if (j->carried_by) {
    if (!strstr(j->name, "android")) {
      act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
      if (oroom && room_people_get(oroom)) {
        act("A quivering horde of maggots consumes $p.", TRUE,
            room_people_get(oroom), j, 0, TO_ROOM);
        act("A quivering horde of maggots consumes $p.", TRUE,
            room_people_get(oroom), j, 0, TO_CHAR);
      }
    } else {
      act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
      if (oroom && room_people_get(oroom)) {
        act("$p breaks down completely into a pile of junk.", TRUE,
            room_people_get(oroom), j, 0, TO_ROOM);
        act("$p breaks down completely into a pile of junk.", TRUE,
            room_people_get(oroom), j, 0, TO_CHAR);
      }
    }
  }
  obj_contents_iterate(j, [&](struct obj_data *jj) {
    obj_from_obj(jj);
    if (j->in_obj)            obj_to_obj(jj, j->in_obj);
    else if (j->carried_by)   obj_to_room(jj, char_room_get(j->carried_by));
    else if (obj_room_get(j)) obj_to_room(jj, obj_room_get(j));
    else                      core_dump();
    return true;
  });
  extract_obj(j);
}

static void tick_obj_ice(struct obj_data *j) {
  auto oroom = obj_room_get(j);
  if (GET_OBJ_VNUM(j) == 79 && rand_number(1, 2) == 2) {
    if (room_geffect_get(oroom) >= 1 && room_geffect_get(oroom) <= 5) {
      send_to_room(oroom,
                   "The heat from the lava melts a great deal of the "
                   "glacial wall and the lava cools a bit in turn.\r\n");
      room_geffect_mod(oroom, -1);
      if (GET_OBJ_WEIGHT(j) - (5 + GET_OBJ_WEIGHT(j) * 0.025) > 0) {
        GET_OBJ_WEIGHT(j) -= 5 + (GET_OBJ_WEIGHT(j) * 0.025);
      } else {
        send_to_room(oroom, "The glacial wall blocking off the %s direction "
                            "melts completely away.\r\n", dirs[GET_OBJ_COST(j)]);
        extract_obj(j);
      }
    } else if (GET_OBJ_WEIGHT(j) - (5 + GET_OBJ_WEIGHT(j) * 0.025) > 0) {
      GET_OBJ_WEIGHT(j) -= 5 + (GET_OBJ_WEIGHT(j) * 0.025);
      send_to_room(oroom, "The glacial wall blocking off the %s direction "
                          "melts some what.\r\n", dirs[GET_OBJ_COST(j)]);
    } else {
      send_to_room(oroom, "The glacial wall blocking off the %s direction "
                          "melts completely away.\r\n", dirs[GET_OBJ_COST(j)]);
      extract_obj(j);
    }
  } else if (GET_OBJ_VNUM(j) != 79) {
    if (j->carried_by && !j->in_obj) {
      int melt = 5 + (GET_OBJ_WEIGHT(j) * 0.02);
      if (GET_OBJ_WEIGHT(j) - melt > 0) {
        GET_OBJ_WEIGHT(j) -= melt;
        send_to_char(j->carried_by, "%s @wmelts a little.\r\n",
                     j->short_description);
      } else {
        send_to_char(j->carried_by, "%s @wmelts completely away.\r\n",
                     j->short_description);
        extract_obj(j);
      }
    } else if (oroom) {
      if (GET_OBJ_WEIGHT(j) - (5 + GET_OBJ_WEIGHT(j) * 0.02) > 0) {
        GET_OBJ_WEIGHT(j) -= 5 + (GET_OBJ_WEIGHT(j) * 0.02);
        send_to_room(oroom, "%s @wmelts a little.\r\n", j->short_description);
      } else {
        send_to_room(oroom, "%s @wmelts completely away.\r\n",
                     j->short_description);
        extract_obj(j);
      }
    }
  }
}

static void point_update_objects(void) {
  auto msnow = []() -> double {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
  };
#define TIMED_SUBS(tag, fn) do { \
  double _t0 = msnow(); \
  obj_iterate_subscriptions(tag, [](struct obj_data *j) { fn(j); return true; }); \
  double _ms = msnow() - _t0; \
  if (_ms > 1.0) mud_log("point_update_objects: %s=%.1fms", tag, _ms); \
} while(0)
  TIMED_SUBS("obj_norent",      tick_obj_norent);
  TIMED_SUBS("obj_healing_tank",tick_obj_healing_tank);
  TIMED_SUBS("obj_timed",       tick_obj_timed);
  TIMED_SUBS("obj_corpse",      tick_obj_corpse);
  TIMED_SUBS("obj_ice",         tick_obj_ice);
#undef TIMED_SUBS
}

void point_update(void) {
  auto mono_now = []() -> struct timespec {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts;
  };
  auto elapsed_ms = [](const struct timespec &a, const struct timespec &b) -> double {
    return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1.0e6;
  };
  const struct timespec t0 = mono_now();
  point_update_characters();
  const struct timespec t1 = mono_now();
  point_update_objects();
  const struct timespec t2 = mono_now();
  const double chars_ms = elapsed_ms(t0, t1), objs_ms = elapsed_ms(t1, t2);
  if (chars_ms + objs_ms >= 100.0)
    mud_log("point_update %.0fms: chars=%.0fms objs=%.0fms",
            chars_ms + objs_ms, chars_ms, objs_ms);
}
