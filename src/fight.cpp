/*************************************************************************
 *   File: fight.c                                       Part of CircleMUD *
 *  Usage: Combat system                                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "fight.h"
#include "act.attack.h"
#include "act.informative.h"
#include "act.misc.h"
#include "act.movement.h"
#include "act.other.h"

#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "combat.h"
#include "comm.h"
#include "config.h"
#include "config_db.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/constates.h"
#include "consts/deathtype.h"
#include "consts/fightprefs.h"
#include "consts/itemdata.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/prefflags.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/shadowdragons.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dg_comm.h"
#include "dg_scripts.h"
#include "extract.h"
#include "flags.h"
#include "handler.h"
#include "log.h"
#include "object_db.h"
#include "object_impl.h"
#include "object_macros.h"
#include "objsave.h"
#include "races.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_api.h"
#include "room_db.h"
#include "spells.h"
#include "stringutils.h"
#include "util_macros.h"
#include "weather_db.h"
#include "zone_api.h"

#include "iterate.hpp"

#include <string.h>

/* Structures */

/* local functions */
static void perform_group_gain(struct char_data *ch, int base,
                               struct char_data *victim);
static void check_killer(struct char_data *ch, struct char_data *vict);
static void scatter_ashes(struct char_data *ch);
static struct obj_data *init_corpse_obj(struct char_data *ch, int timer);
static void handle_corpse_condition(struct obj_data *corpse,
                                    struct char_data *ch);
static void make_corpse(struct char_data *ch, struct char_data *tch);
static void make_pcorpse(struct char_data *ch);
static void change_alignment(struct char_data *ch, struct char_data *victim);
static void final_combat_resolve(struct char_data *ch);
static void shadow_dragons_live(void);
static void cleanup_arena_watch(struct char_data *ch);
static void mob_attack(struct char_data *ch, char *buf);
static int pick_n_throw(struct char_data *ch, char *buf);

int group_bonus(struct char_data *ch, int type) {
  if (!char_condition_has(ch, "group"))
    return (FALSE);

  if (char_follower_count(ch)) {
    int result = FALSE;
    bool acted = false;
    char_followers_iterate(ch, [&](struct char_data *fol) {
      if (!char_condition_has(fol, "group")) return true;
      acted = true;
      if (type == 0) {
        incCurLFPercent(fol, .25);
        send_to_char(fol, "@CIncensed by the death of your comrade "
                          "your life force swells!@n");
        result = TRUE;
      } else if (type == 1) {
        incCurLFPercent(fol, .4);
        send_to_char(fol, "@CIncensed by the death of your comrade "
                          "your life force swells!@n");
        result = TRUE;
      } else if (type == 2) {
        if (IS_ROSHI(ch)) result = 2;
        else if (IS_KRANE(ch)) result = 3;
        else if (IS_BARDOCK(ch)) result = 4;
        else if (IS_NAIL(ch)) result = 5;
        else if (IS_KABITO(ch)) result = 6;
        else if (IS_ANDSIX(ch)) result = 7;
        else if (IS_TAPION(ch)) result = 8;
        else if (IS_FRIEZA(ch)) result = 9;
        else if (IS_TSUNA(ch)) result = 10;
        else if (IS_PICCOLO(ch)) result = 11;
        else if (IS_KURZAK(ch)) result = 12;
        else if (IS_JINTO(ch)) result = 13;
        else if (IS_DABURA(ch)) result = 14;
      }
      return false; // stop on first group member
    });
    if (acted) return result;
  } else if (MASTER(ch)) {
    if (!char_condition_has(MASTER(ch), "group"))
      return (FALSE);
    else {
      if (type == 0) {
        group_bonus(MASTER(ch), 0);
      } else if (type == 2) {
        if (IS_ROSHI(MASTER(ch))) {
          return (2);
        } else if (IS_KRANE(MASTER(ch))) {
          return (3);
        } else if (IS_BARDOCK(MASTER(ch))) {
          return (4);
        } else if (IS_NAIL(MASTER(ch))) {
          return (5);
        } else if (IS_KABITO(MASTER(ch))) {
          return (6);
        } else if (IS_ANDSIX(MASTER(ch))) {
          return (7);
        } else if (IS_TAPION(MASTER(ch))) {
          return (8);
        } else if (IS_FRIEZA(MASTER(ch))) {
          return (9);
        } else if (IS_TSUNA(MASTER(ch))) {
          return (10);
        } else if (IS_PICCOLO(MASTER(ch))) {
          return (11);
        } else if (IS_KURZAK(MASTER(ch))) {
          return (12);
        } else if (IS_JINTO(MASTER(ch))) {
          return (13);
        } else if (IS_DABURA(MASTER(ch))) {
          return (14);
        }
      }
      return (TRUE);
    }
  }
  return (FALSE);
}

void mutant_limb_regen(struct char_data *ch) {
  if (GET_LIMBCOND(ch, 1) > 0 && GET_LIMBCOND(ch, 1) < 50) {
    act("The bones in your right arm have mended them selves.", TRUE, ch, 0, 0,
        TO_CHAR);
    act("$n starts moving $s right arm gingerly for a moment.", TRUE, ch, 0, 0,
        TO_ROOM);
    SET_LIMBCOND(ch, 1, 100);
  } else if (GET_LIMBCOND(ch, 1) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is "
        "whole again!",
        TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole "
        "again!.",
        TRUE, ch, 0, 0, TO_ROOM);
    SET_LIMBCOND(ch, 1, 100);
  }
  if (GET_LIMBCOND(ch, 2) > 0 && GET_LIMBCOND(ch, 2) < 50) {
    act("The bones in your left arm have mended them selves.", TRUE, ch, 0, 0,
        TO_CHAR);
    act("$n starts moving $s left arm gingerly for a moment.", TRUE, ch, 0, 0,
        TO_ROOM);
    SET_LIMBCOND(ch, 2, 100);
  } else if (GET_LIMBCOND(ch, 2) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is "
        "whole again!",
        TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole "
        "again!.",
        TRUE, ch, 0, 0, TO_ROOM);
    SET_LIMBCOND(ch, 2, 100);
  }
  if (GET_LIMBCOND(ch, 3) > 0 && GET_LIMBCOND(ch, 3) < 50) {
    act("The bones in your right leg have mended them selves.", TRUE, ch, 0, 0,
        TO_CHAR);
    act("$n starts moving $s right leg gingerly for a moment.", TRUE, ch, 0, 0,
        TO_ROOM);
    SET_LIMBCOND(ch, 3, 100);
  } else if (GET_LIMBCOND(ch, 3) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is "
        "whole again!",
        TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole "
        "again!.",
        TRUE, ch, 0, 0, TO_ROOM);
    SET_LIMBCOND(ch, 3, 100);
  }
  if (GET_LIMBCOND(ch, 4) > 0 && GET_LIMBCOND(ch, 4) < 50) {
    act("The bones in your left leg have mended them selves.", TRUE, ch, 0, 0,
        TO_CHAR);
    act("$n starts moving $s left leg gingerly for a moment.", TRUE, ch, 0, 0,
        TO_ROOM);
    SET_LIMBCOND(ch, 4, 100);
  } else if (GET_LIMBCOND(ch, 4) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is "
        "whole again!",
        TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole "
        "again!.",
        TRUE, ch, 0, 0, TO_ROOM);
    SET_LIMBCOND(ch, 4, 100);
  }
}

static int pick_n_throw(struct char_data *ch, char *buf) {
  struct obj_data *cont;
  char buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  ;

  if (rand_number(1, 20) < 18) {
    return (FALSE);
  }

  {
    bool thrown = false;
    room_contents_iterate(char_room_get(ch), [&](auto cont) {
      if (GET_OBJ_WEIGHT(cont) <= CAN_CARRY_W(ch) + IS_CARRYING_W(ch)) {
        sprintf(buf2, "%s", cont->name);
        do_get(ch, buf2, 0, 0);
        sprintf(buf3, "%s %s", buf2, buf);
        do_throw(ch, buf3, 0, 0);
        thrown = true;
        return false;
      }
      return true;
    });
    if (thrown) return (TRUE);
  }

  return (FALSE);
}

static void mob_attack(struct char_data *ch, char *buf) {
  int power = rand_number(1, 5);
  int bonus = GET_LEVEL(ch) * 0.1;
  int special = 0;
  char buf2[MAX_INPUT_LENGTH];

  power += bonus;
  if (rand_number(1, 4) == 4)
    power += 10;
  if (power > 20)
    power = 20;

  int dragonpass = TRUE;
  if (IS_DRAGON(ch)) {
    if (GET_MOB_VNUM(ch) == 81 || GET_MOB_VNUM(ch) == 82 ||
        GET_MOB_VNUM(ch) == 83 || GET_MOB_VNUM(ch) == 84 ||
        GET_MOB_VNUM(ch) == 85 || GET_MOB_VNUM(ch) == 86 ||
        GET_MOB_VNUM(ch) == 87) {
      dragonpass = TRUE;
      special = rand_number(40, 100);
    } else {
      dragonpass = FALSE;
    }
  }

  if (axion_dice(-10) > 90 && getCurHealthPercent(ch) <= .5 &&
      !MOB_FLAGGED(ch, MOB_POWERUP) && GET_MOB_VNUM(ch) != 25 &&
      !(IS_ANDROID(ch) || IS_ANIMAL(ch) || ch->chclass == CLASS_NPC_COMMONER)) {
    do_powerup(ch, nullptr, 0, 0);
    return;
  }

  if ((getCurKI(ch)) >= GET_MAX_MANA(ch) * 0.05 && IS_HUMANOID(ch) &&
      (!IS_DRAGON(ch) || dragonpass == TRUE)) {
    auto mob_charge_tick = [&]() {
      ch->mobcharge += 1;
      if (GET_LEVEL(ch) > 80)
        ch->mobcharge += 1;
    };
    if (ch->mobcharge <= 0 && rand_number(1, 10) >= 8) {
      act("@wAn aura flares up around @R$n@w!@n", TRUE, ch, 0, 0, TO_ROOM);
      mob_charge_tick();
    } else if (ch->mobcharge <= 5) {
      act("@wThe aura burns brighter around @R$n@w!@n", TRUE, ch, 0, 0, TO_ROOM);
      mob_charge_tick();
    } else if (ch->mobcharge == 6) {
      act("@wThe aura around @R$n@w flashes!@n", TRUE, ch, 0, 0, TO_ROOM);
      ch->mobcharge += 1;
      special = 100;
    }
  }

  if (IS_HUMANOID(ch) && dragonpass == TRUE) {
    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_ENSNARED))
      return;

    if (special < 100) {
      if (GET_CLASS(ch) == CLASS_SHADOWDANCER && rand_number(1, 3) == 3) {
        sprintf(buf2, "ass %s", buf);
        do_throw(ch, buf2, 0, 0);
      } else if (IS_ANDROID(ch) && MOB_FLAGGED(ch, MOB_REPAIR) &&
                 GET_HIT(ch) <= (getMaxPL(ch)) * 0.5 &&
                 rand_number(1, 20) >= 16) {
        do_srepair(ch, NULL, 0, 0);
      } else if (IS_ANDROID(ch) && MOB_FLAGGED(ch, MOB_ABSORB) &&
                 rand_number(1, 20) >= 19) {
        do_absorb(ch, buf2, 0, 0);
      } else if ((IS_BIO(ch) || IS_MAJIN(ch)) &&
                 GET_HIT(ch) <= (getMaxPL(ch)) * 0.5 &&
                 rand_number(1, 20) >= 17) {
        do_regenerate(ch, "25", 0, 0);
      } else if (IS_NAMEK(ch) && GET_HIT(ch) <= (getMaxPL(ch)) * 0.5 &&
                 rand_number(1, 20) == 20) {
        do_regenerate(ch, "25", 0, 0);
      } else if (pick_n_throw(ch, buf)) {
        /* handled */
      } else if (MOB_FLAGGED(ch, MOB_KNOWKAIO) && rand_number(1, 50) >= 46) {
        if (rand_number(1, 10) == 10)
          do_kaioken(ch, "20", 0, 0);
        else if (rand_number(1, 10) >= 8)
          do_kaioken(ch, "10", 0, 0);
        else
          do_kaioken(ch, "5", 0, 0);
      } else {
        switch (power) {
        case 1: case 2: case 3: case 4: case 5:
          if (GET_EQ(ch, WEAR_WIELD1))
            do_attack(ch, buf, 0, 0);
          else if (rand_number(1, 5) == 5)
            char_cmd_execute(ch, "kick", buf);
          else if (rand_number(1, 10) == 10)
            char_cmd_execute(ch, "elbow", buf);
          else
            char_cmd_execute(ch, "punch", buf);
          break;
        case 6: case 7: case 8:
          if (GET_EQ(ch, WEAR_WIELD1))
            do_attack(ch, buf, 0, 0);
          else if (rand_number(1, 5) == 5)
            char_cmd_execute(ch, "punch", buf);
          else if (rand_number(1, 10) == 10)
            char_cmd_execute(ch, "knee", buf);
          else
            char_cmd_execute(ch, "kick", buf);
          break;
        case 9: case 10:
          if (rand_number(1, 5) == 5)
            char_cmd_execute(ch, "knee", buf);
          else if (rand_number(1, 10) == 10)
            char_cmd_execute(ch, "uppercut", buf);
          else
            char_cmd_execute(ch, "elbow", buf);
          break;
        case 11: case 12:
          if (rand_number(1, 5) == 5)
            char_cmd_execute(ch, "elbow", buf);
          else if (rand_number(1, 10) == 10)
            char_cmd_execute(ch, "roundhouse", buf);
          else if (rand_number(1, 8) == 8)
            do_trip(ch, buf, 0, 0);
          else
            char_cmd_execute(ch, "knee", buf);
          break;
        case 13: case 14:
          if ((IS_BARDOCK(ch) || IS_KURZAK(ch)) && rand_number(1, 2) == 2)
            char_cmd_execute(ch, "headbutt", buf);
          else if ((IS_ICER(ch) || IS_BIO(ch)) && rand_number(1, 2) == 2)
            char_cmd_execute(ch, "tailwhip", buf);
          else if (rand_number(1, 8) == 8)
            do_trip(ch, buf, 0, 0);
          else
            char_cmd_execute(ch, "uppercut", buf);
          break;
        case 15: case 16:
          if ((IS_BARDOCK(ch) || IS_KURZAK(ch)) && rand_number(1, 2) == 2)
            char_cmd_execute(ch, "headbutt", buf);
          else if ((IS_ICER(ch) || IS_BIO(ch)) && rand_number(1, 2) == 2)
            char_cmd_execute(ch, "tailwhip", buf);
          else if (rand_number(1, 8) >= 7)
            do_trip(ch, buf, 0, 0);
          else
            char_cmd_execute(ch, "roundhouse", buf);
          break;
        case 17: case 18:
          char_cmd_execute(ch, "slam", buf);
          break;
        case 19: case 20:
          char_cmd_execute(ch, "heeldrop", buf);
          break;
        }
      }
    } else {
      mob_specials_used += 1;

      auto fire_charged = [&](auto fn) {
        if (ch->mobcharge == 7) {
          ch->mobcharge = 0;
          fn();
        }
      };
      auto dragon_or_charged = [&](auto fn) {
        if (IS_DRAGON(ch) && rand_number(1, 4) == 4)
          char_cmd_execute(ch, "breath", buf);
        else
          fire_charged(fn);
      };

      switch (power) {
      case 1: case 2: case 3: case 4:
        if (special > 80) do_zanzoken(ch, buf, 0, 0);
        fire_charged([&]{ char_cmd_execute(ch, "kiball", buf); });
        break;
      case 5: case 6: case 7: case 8:
        if (special > 80) do_zanzoken(ch, buf, 0, 0);
        fire_charged([&]{ char_cmd_execute(ch, "kiblast", buf); });
        break;
      case 9: case 10: case 11:
        if (special > 80) do_zanzoken(ch, buf, 0, 0);
        dragon_or_charged([&]{ char_cmd_execute(ch, "beam", buf); });
        break;
      case 12: case 13: case 14:
        if (special > 80) do_zanzoken(ch, buf, 0, 0);
        dragon_or_charged([&]{ char_cmd_execute(ch, "renzokou", buf); });
        break;
      case 15: case 16:
        dragon_or_charged([&]{ char_cmd_execute(ch, "tsuihidan", buf); });
        break;
      case 17: case 18:
        dragon_or_charged([&]{ char_cmd_execute(ch, "shogekiha", buf); });
        break;
      case 19: case 20:
        if (IS_DRAGON(ch))
          char_cmd_execute(ch, "breath", buf);
        fire_charged([&]{
          switch (GET_CLASS(ch)) {
          case CLASS_ROSHI:
            if (special >= 100) do_kakusanha(ch, buf, 0, 0);
            else if (special >= 80) char_cmd_execute(ch, "kienzan", buf);
            else if (special >= 70) char_cmd_execute(ch, "kamehameha", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_FRIEZA:
            if (special >= 100) char_cmd_execute(ch, "deathball", buf);
            else if (special >= 80) char_cmd_execute(ch, "kienzan", buf);
            else if (special >= 70) char_cmd_execute(ch, "deathbeam", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_KRANE:
            if (special >= 100) char_cmd_execute(ch, "tribeam", buf);
            else if (special >= 80) do_hass(ch, NULL, 0, 0);
            else if (special >= 70) char_cmd_execute(ch, "dodonpa", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_PICCOLO:
            if (special >= 100) char_cmd_execute(ch, "scatter", buf);
            else if (special >= 80) do_sbc(ch, buf, 0, 0);
            else if (special >= 70) char_cmd_execute(ch, "masenko", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_BARDOCK:
            if (special >= 100) char_cmd_execute(ch, "finalflash", buf);
            else if (special >= 80) char_cmd_execute(ch, "bigbang", buf);
            else if (special >= 70) char_cmd_execute(ch, "galikgun", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_ANDSIX:
            if (special >= 100) char_cmd_execute(ch, "hellflash", buf);
            else if (special >= 80) char_cmd_execute(ch, "kousengan", buf);
            else if (special >= 70) char_cmd_execute(ch, "dualbeam", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_NAIL:
            if (special >= 100) do_regenerate(ch, "50", 0, 0);
            else if (special >= 80) do_heal(ch, "self", 0, 0);
            else if (special >= 70) char_cmd_execute(ch, "masenko", buf);
            else do_zanzoken(ch, NULL, 0, 0);
            break;
          case CLASS_KURZAK:
            if (special >= 100) do_ensnare(ch, buf, 0, 0);
            else if (special >= 80) char_cmd_execute(ch, "seishou", buf);
            else if (special >= 70) char_cmd_execute(ch, "renzokou", buf);
            else if (special >= 50) do_barrier(ch, "40", 0, 0);
            else do_barrier(ch, "25", 0, 0);
            break;
          case CLASS_JINTO:
            if (special >= 100) char_cmd_execute(ch, "nova", buf);
            else if (special >= 80) char_cmd_execute(ch, "starbreaker", buf);
            else if (special >= 70) do_trip(ch, buf, 0, 0);
            else do_zanzoken(ch, "40", 0, 0);
            break;
          case CLASS_TSUNA:
            if (special >= 100) char_cmd_execute(ch, "koteiru", buf);
            else if (special >= 80) char_cmd_execute(ch, "waterrazor", buf);
            else if (special >= 70) char_cmd_execute(ch, "waterspikes", buf);
            else do_barrier(ch, "20", 0, 0);
            break;
          case CLASS_TAPION:
            if (special >= 100) char_cmd_execute(ch, "phoenix", buf);
            else if (special >= 80) char_cmd_execute(ch, "darkness", buf);
            else if (special >= 70) char_cmd_execute(ch, "twinslash", buf);
            else do_zanzoken(ch, "40", 0, 0);
            break;
          case CLASS_KABITO:
            if (special >= 100) char_cmd_execute(ch, "barrage", buf);
            else if (special >= 80) char_cmd_execute(ch, "psychic", buf);
            else if (special >= 70) do_heal(ch, buf, 0, 0);
            else do_zanzoken(ch, "40", 0, 0);
            break;
          case CLASS_DABURA:
            if (special >= 100) char_cmd_execute(ch, "hellspear", buf);
            else if (special >= 80) char_cmd_execute(ch, "honoo", buf);
            else if (special >= 70) do_fireshield(ch, buf, 0, 0);
            else do_zanzoken(ch, "40", 0, 0);
            break;
          case CLASS_GINYU:
            if (special >= 100) do_spiral(ch, buf, 0, 0);
            else if (special >= 80) char_cmd_execute(ch, "crusher", buf);
            else if (special >= 70) char_cmd_execute(ch, "eraser", buf);
            else do_zanzoken(ch, "40", 0, 0);
            break;
          }
        });
        break;
      }
    }
  } else if (!IS_HUMANOID(ch) || dragonpass == FALSE) {
    if (IS_SERPENT(ch) && rand_number(1, 5) == 5)
      char_cmd_execute(ch, "strike", buf);
    else if (IS_DRAGON(ch) && rand_number(1, 12) >= 10 &&
             GET_MOB_VNUM(ch) != 17917)
      char_cmd_execute(ch, "breath", buf);
    else if (rand_number(1, 10) >= 7 && GET_LEVEL(ch) >= 10)
      char_cmd_execute(ch, "ram", buf);
    else
      char_cmd_execute(ch, "bite", buf);
  }

  fight_mtrigger(ch);
}

static void cleanup_arena_watch(struct char_data *ch) {
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {

    if (STATE(d) != CON_PLAYING)
      continue;

    if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
      if (ARENA_IDNUM(d->character) == GET_IDNUM(ch)) {
        REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_ARENAWATCH);
        ARENA_IDNUM(d->character) = -1;
      }
    }
  }
}

static void shadow_dragons_live() {
  int value = 0;
  if (SHADOW_DRAGON1 != -1 || SHADOW_DRAGON2 != -1 || SHADOW_DRAGON3 != -1 ||
      SHADOW_DRAGON4 != -1 || SHADOW_DRAGON5 != -1 || SHADOW_DRAGON6 != -1 ||
      SHADOW_DRAGON7 != -1) {
    value = 1;
  }

  if (value == 0) {
    SELFISHMETER = 0;
    save_mud_time(&time_info);
  }
}

/* For announcing the sounds of battle to nearby rooms */
void impact_sound(struct char_data *ch, char *mssg) {
  auto room = char_room_get(ch);
  if(!room) return;
  room_exits_iterate(room, [&](auto door, auto exit) {
    if (auto dest = char_can_go_exit(ch, exit)) {
      send_to_room(dest, "%s", mssg);
    }
    return true;
  });
}

/* For removing body parts */
void remove_limb(struct char_data *vict, int num) {
  /* 0 = head, 1 = rarm, 2 = larm, 3 = rleg, 4 = lleg , 5 = tail for saiyany, 6
   * = tail for bio or icer... */

  struct obj_data *body_part;
  char part[1000];
  char buf[1000];
  char buf2[1000];

  body_part = create_obj();
  body_part->proto_id = NOTHING;
  IN_ROOM(body_part) = NOWHERE;

  switch (num) {
  case 0:
    snprintf(part, sizeof(part), "@C%s@w's bloody head@n", GET_NAME(vict));
    snprintf(buf, sizeof(buf), "%s bloody head", GET_NAME(vict));
    break;
  case 1:
    snprintf(part, sizeof(part), "@w%s right arm@n", TRUE_RACE(vict));
    snprintf(buf, sizeof(buf), "right arm");
    if (PLR_FLAGGED(vict, PLR_CRARM)) {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CRARM);
    }
    break;
  case 2:
    snprintf(part, sizeof(part), "@w%s left arm@n", TRUE_RACE(vict));
    snprintf(buf, sizeof(buf), "left arm");
    if (PLR_FLAGGED(vict, PLR_CLARM)) {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLARM);
    }
    break;
  case 3:
    snprintf(part, sizeof(part), "@w%s right leg@n", TRUE_RACE(vict));
    snprintf(buf, sizeof(buf), "right leg");
    if (PLR_FLAGGED(vict, PLR_CRLEG)) {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CRLEG);
    }
    break;
  case 4:
    snprintf(part, sizeof(part), "@w%s left leg@n", TRUE_RACE(vict));
    snprintf(buf, sizeof(buf), "left leg");
    if (PLR_FLAGGED(vict, PLR_CLLEG)) {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_CLLEG);
    }
    break;
  case 5:
  case 6:
    snprintf(part, sizeof(part), "@wA %s tail@n", TRUE_RACE(vict));
    snprintf(buf, sizeof(buf), "tail");
    break;
  default:
    snprintf(part, sizeof(part), "@w%s body part@n", TRUE_RACE(vict));
    snprintf(buf, sizeof(buf), "body part");
    break;
  }

  body_part->name = strdup(buf);
  if (num > 0) {
    snprintf(buf2, sizeof(buf2), "@wA %s is lying here@n", part);
  } else {
    snprintf(buf2, sizeof(buf2), "%s@w is lying here@n", part);
  }
  body_part->description = strdup(buf2);
  body_part->short_description = strdup(part);

  GET_OBJ_TYPE(body_part) = ITEM_OTHER;
  SET_BIT_AR(GET_OBJ_WEAR(body_part), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(body_part), ITEM_UNIQUE_SAVE);
  GET_OBJ_VAL(body_part, 4) = 1;
  GET_OBJ_VAL(body_part, 5) = 1;
  GET_OBJ_WEIGHT(body_part) = rand_number(4, 10);
  obj_to_room(body_part, char_room_get(vict));
}

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES] = {
    {"hit", "hits"}, /* 0 */
    {"sting", "stings"},   {"whip", "whips"},         {"slash", "slashes"},
    {"bite", "bites"},     {"bludgeon", "bludgeons"}, /* 5 */
    {"crush", "crushes"},  {"pound", "pounds"},       {"claw", "claws"},
    {"maul", "mauls"},     {"thrash", "thrashes"}, /* 10 */
    {"pierce", "pierces"}, {"blast", "blasts"},       {"punch", "punches"},
    {"stab", "stabs"}};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

static void reset_fighting_position(struct char_data *ch) {
  if (GET_POS(ch) == POS_FIGHTING) {
    char_position_set(ch, POS_STANDING);
  }
  if (PLR_FLAGGED(ch, PLR_SPIRAL)) {
    handle_spiral(ch, NULL, GET_SKILL(ch, SKILL_SPIRAL), FALSE);
  }
}

static bool tick_mob_cooldown(struct char_data *ch) {
  if (IS_NPC(ch) && MOB_COOLDOWN(ch) > 0) {
    MOB_COOLDOWN(ch) -= 1;
    if (rand_number(1, 2) == 2 && MOB_COOLDOWN(ch) > 0) {
      MOB_COOLDOWN(ch) -= 1;
    }
    if (MOB_COOLDOWN(ch) > 0) {
      return true;
    }
  }
  return false;
}

static void tick_mob_powerup(struct char_data *ch) {
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_POWERUP) && axion_dice(0) >= 90) {
    if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
      act("@g$n@ finishes powering up as $s aura flashes brightly filling "
          "the entire area briefly with its light!@n",
          TRUE, ch, 0, 0, TO_ROOM);
      restoreHealthAnnounced(ch, false);
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_POWERUP);
    } else if (GET_HIT(ch) >= GET_MAX_HIT(ch) / 2) {
      act("@g$n@G continues powering up as torrents of energy crackle within "
          "$s aura.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      incCurHealthPercent(ch, .1);
    } else if (GET_HIT(ch) > GET_MAX_HIT(ch) / 4) {
      act("@g$n@G powers up as a steady aura around $s body grow brighter.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      incCurHealthPercent(ch, .125);
    } else if (GET_HIT(ch) > 0) {
      act("@g$n@G powers up, as a weak aura flickers around $s body.@n", TRUE,
          ch, 0, 0, TO_ROOM);
      incCurHealthPercent(ch, .2);
    }
  }
}

static bool tick_frozen_skip(struct char_data *ch) {
  return IS_NPC(ch) && IS_AFFECTED(ch, AFF_FROZEN);
}

static bool tick_idle_skip(struct char_data *ch) {
  return !GRAPPLING(ch) && !GRAPPLED(ch) && !FIGHTING(ch) &&
         !PLR_FLAGGED(ch, PLR_CHARGE) && !PLR_FLAGGED(ch, PLR_POWERUP) &&
         GET_CHARGE(ch) <= 0 && !IS_TRANSFORMED(ch);
}

static void tick_fight_room_check(struct char_data *ch) {
  if (FIGHTING(ch) && (char_room_get(FIGHTING(ch)) != char_room_get(ch))) {
    struct char_data *wch = FIGHTING(ch);
    stop_fighting(wch);
    stop_fighting(ch);
  }
}

static void tick_dragging_interrupt(struct char_data *ch) {
  if (FIGHTING(ch) && DRAGGING(ch)) {
    act("@WYou are forced to stop dragging @C$N@W!@n", TRUE, ch, 0,
        DRAGGING(ch), TO_CHAR);
    act("@C$n@W is forced to stop dragging @c$N@W!@n", TRUE, ch, 0,
        DRAGGING(ch), TO_ROOM);
    struct char_data *dragged = DRAGGING(ch);
    char_dragging_set(ch, NULL);
    char_being_dragged_set(dragged, NULL);
  }
}

static void tick_lifeforce_heal(struct char_data *ch) {
  if (GET_LIFEPERC(ch) <= 0 || IS_ANDROID(ch))
    return;
  if (char_meter_get(ch, "powerlevel") / 1000000.0 >= (double)GET_LIFEPERC(ch) / 100)
    return;

  int64_t cur_lf = getCurLF(ch);
  if (cur_lf <= 0 || rand_number(1, 15) < 14)
    return;

  int64_t max_lf      = getMaxLF(ch);
  bool healing_glow   = char_condition_has(ch, "healing_glow");
  bool diehard        = GET_BONUS(ch, BONUS_DIEHARD) > 0;
  bool mutant_regen   = IS_MUTANT(ch) && HAS_GENOME(ch, 2);

  if (cur_lf >= max_lf * 0.05 || healing_glow || (IS_KANASSAN(ch) && cur_lf >= max_lf * 0.03)) {
    int64_t lfcost = max_lf * 0.05;
    int64_t refill;
    if      (diehard && !mutant_regen) refill = max_lf * 0.1;
    else if (diehard &&  mutant_regen) refill = max_lf * 0.17;
    else if (mutant_regen)             refill = max_lf * 0.12;
    else if (IS_KANASSAN(ch))        { refill = max_lf * 0.03; lfcost = refill; }
    else                               refill = max_lf * 0.05;

    incCurHealth(ch, refill);
    if (!healing_glow)
      decCurLF(ch, lfcost);
  } else {
    incCurHealth(ch, cur_lf);
    decCurLFPercentFloored(ch, 2, -1);
  }
  send_to_char(ch, "@YYour life force has kept you strong@n!\r\n");
}

static void tick_position_advantage(struct char_data *ch) {
  if (!AFF_FLAGGED(ch, AFF_POSITION)) {
    if (roll_balance(ch) > axion_dice(0) && rand_number(1, 10) >= 7) {
      if (FIGHTING(ch)) {
        if (!AFF_FLAGGED(FIGHTING(ch), AFF_POSITION)) {
          act("@YYou manage to move into an advantageous position!@n", TRUE,
              ch, 0, 0, TO_CHAR);
          act("@y$n@Y manages to move into an advantageous position!@n", TRUE,
              ch, 0, 0, TO_ROOM);
          SET_BIT_AR(AFF_FLAGS(ch), AFF_POSITION);
        } else {
          struct char_data *vict = FIGHTING(ch);
          if (roll_balance(ch) > roll_balance(vict)) {
            act("@YYou struggle to gain a better position than @y$N@Y and "
                "succeed!@n",
                TRUE, ch, 0, vict, TO_CHAR);
            act("@y$n@Y struggles to gain a better position than you and "
                "succeeds!@n",
                TRUE, ch, 0, vict, TO_VICT);
            act("@y$n@Y struggles to gain a better position than @y$N@Y and "
                "succeeds!@n",
                TRUE, ch, 0, vict, TO_NOTVICT);
            REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_POSITION);
            SET_BIT_AR(AFF_FLAGS(ch), AFF_POSITION);
          }
        }
      }
    }
  } else {
    if (roll_balance(ch) < axion_dice(-30) || GET_POS(ch) < POS_STANDING) {
      act("@YYou are moved out of your position!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@y$n@Y is moved out of $s position!@n", TRUE, ch, 0, 0, TO_ROOM);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POSITION);
    }
  }
}

static void tick_grapple_damage(struct char_data *ch) {
  if (GRAPPLING(ch) && GRAPTYPE(ch) == 2 && rand_number(1, 11) >= 8) {
    if ((getCurST(GRAPPLING(ch))) >= GET_MAX_MOVE(GRAPPLING(ch)) / 8) {
      act("@WYou choke @C$N@W!@n", TRUE, ch, 0, GRAPPLING(ch), TO_CHAR);
      act("@C$n@W chokes YOU@W!@n", TRUE, ch, 0, GRAPPLING(ch), TO_VICT);
      act("@C$n@W chokes @c$N@W!@n", TRUE, ch, 0, GRAPPLING(ch), TO_NOTVICT);
      decCurST(GRAPPLING(ch), (getMaxST(GRAPPLING(ch)) / 8));
    } else {
      act("@WYou choke @C$N@W, and $E passes out!@n", TRUE, ch, 0,
          GRAPPLING(ch), TO_CHAR);
      act("@C$n@W chokes YOU@W, and you pass out!@n", TRUE, ch, 0,
          GRAPPLING(ch), TO_VICT);
      act("@C$n@W chokes @c$N@W, and $E passes out!@n", TRUE, ch, 0,
          GRAPPLING(ch), TO_NOTVICT);
      char_condition_apply(GRAPPLING(ch), "knocked_out", "combat", "choke");
      char_position_set(GRAPPLING(ch), POS_SLEEPING);
      {
        struct char_data *other = GRAPPLING(ch);
        char_grappling_set(ch, NULL, 0);
        char_grappled_set(other, NULL, 0);
      }
    }
  } else if (GRAPPLING(ch) && GRAPTYPE(ch) == 4 && rand_number(1, 12) >= 8) {
    act("@WYou crush @C$N@W some more!@n", TRUE, ch, 0, GRAPPLING(ch),
        TO_CHAR);
    act("@C$n@W crushes YOU@W some more!@n", TRUE, ch, 0, GRAPPLING(ch),
        TO_VICT);
    act("@C$n@W crushes @c$N@W some more!@n", TRUE, ch, 0, GRAPPLING(ch),
        TO_NOTVICT);
    int64_t damg = GET_STR(ch) * (10 + (GET_MAX_HIT(ch) * 0.005));
    hurt(0, 0, ch, GRAPPLING(ch), NULL, damg, 0);
  }
}

static void tick_halfbreed_fury(struct char_data *ch) {
  if (IS_HALFBREED(ch) && PLR_FLAGGED(ch, PLR_FURY)) {
    GET_RMETER(ch) += 1;
    if (GET_RMETER(ch) >= 1000) {
      incCurHealthPercent(ch, .15);
      incCurKIPercent(ch, .15);
      incCurSTPercent(ch, .15);
      send_to_char(ch, "Your fury has called forth more of your hidden power "
                       "and you feel better!\r\n");
    }
  }
}

static void tick_transformation_drain(struct char_data *ch) {
  if (IS_NPC(ch) || !IS_TRANSFORMED(ch) || IS_ICER(ch) || !IS_NONPTRANS(ch))
    return;

  int64_t cur_st   = getCurST(ch);
  int64_t max_st   = getMaxST(ch);
  int64_t max_move = GET_MAX_MOVE(ch);
  bool saiyan_high_lf = IS_SAIYAN(ch) && getCurLF(ch) >= getMaxLF(ch) * 0.7;

  // Saiyans with high life force sustain their form at a reduced stamina cost
  auto drain = [&](int base_div, int saiyan_div) {
    decCurST(ch, max_st / (saiyan_high_lf ? saiyan_div : base_div));
  };

  if (cur_st < max_move / 60) {
    int tier = get_race(ch->race)->getCurrentTransTier(ch);
    if (!(tier == 1 && PLR_FLAGGED(ch, PLR_FPSSJ))) {
      act("@mExhausted of stamina, your body forcibly reverts from its form.@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n @wbreathing heavily, reverts from $s form, returning to normal.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (GET_KAIOKEN(ch) < 1)
        do_kaioken(ch, "0", 0, 0);
      char_cmd_execute(ch, "transform", "revert");
      return;
    }
    // FPSSJ tier 1: suppresses forced revert at low ST, but drain still applies
  }

  if (cur_st >= max_move / 800 && PLR_FLAGGED(ch, PLR_TRANS1)) {
    if (!PLR_FLAGGED(ch, PLR_FPSSJ))
      drain(800, 900);
  } else if (cur_st >= max_move / 600 && PLR_FLAGGED(ch, PLR_TRANS2) &&
             !IS_KONATSU(ch) && !IS_KAI(ch) && !IS_NAMEK(ch)) {
    drain(600, 700);
  } else if (cur_st >= max_move / 500 && PLR_FLAGGED(ch, PLR_TRANS2)) {
    decCurST(ch, max_st / 500);
  } else if (cur_st >= max_move / 400 && PLR_FLAGGED(ch, PLR_TRANS3) && !IS_SAIYAN(ch)) {
    decCurST(ch, max_st / 400);
  } else if (cur_st >= max_move / 250 && PLR_FLAGGED(ch, PLR_TRANS3)) {
    drain(250, 300);
  } else if (cur_st >= max_move / 200 && PLR_FLAGGED(ch, PLR_TRANS4) && !IS_SAIYAN(ch)) {
    decCurST(ch, max_st / 200);
  } else if (cur_st >= max_move / 170 && PLR_FLAGGED(ch, PLR_TRANS4)) {
    drain(170, 240);
  }
}

static void tick_wimp_flee(struct char_data *ch) {
  if (!IS_NPC(ch) && GET_WIMP_LEV(ch) && GET_HIT(ch) < GET_WIMP_LEV(ch) &&
      GET_HIT(ch) > 0 && FIGHTING(ch)) {
    send_to_char(ch, "You wimp out, and attempt to flee!\r\n");
    do_flee(ch, NULL, 0, 0);
  }
  if (IS_NPC(ch) && GET_HIT(ch) < GET_MAX_HIT(ch) / 10 && GET_HIT(ch) > 0 &&
      FIGHTING(ch) && !MOB_FLAGGED(ch, MOB_SENTINEL)) {
    if (rand_number(1, 30) >= 25 && GET_POS(ch) > POS_SITTING) {
      do_flee(ch, NULL, 0, 0);
    }
  }
}

static void tick_disguise_slip(struct char_data *ch) {
  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DISGUISED) && FIGHTING(ch)) {
    if (GET_SKILL(ch, SKILL_DISGUISE) < rand_number(1, 125)) {
      send_to_char(
          ch, "Your disguise comes off because of your swift movements!\r\n");
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_DISGUISED);
      act("@W$n's@W disguise comes off because of $s swift movements!@n",
          FALSE, ch, 0, 0, TO_ROOM);
    }
  }
}

static void tick_mob_blind_recovery(struct char_data *ch) {
  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_BLIND) &&
      rand_number(1, 200) >= 190) {
    act("@W$n@W is no longer blind.@n", FALSE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_BLIND);
  }
}

static void tick_knocked_recovery(struct char_data *ch) {
  if (AFF_FLAGGED(ch, AFF_KNOCKED) && rand_number(1, 200) >= 195) {
    cureStatusKnockedOutAnnounced(ch, true);
    if (IS_NPC(ch) && rand_number(1, 20) >= 12) {
      act("@W$n@W stands up.@n", FALSE, ch, 0, 0, TO_ROOM);
      char_position_set(ch, POS_STANDING);
    }
  }
}

static void tick_linkdead_flee(struct char_data *ch) {
  if (!IS_NPC(ch) && !(ch->desc) && GET_POS(ch) > POS_STUNNED &&
      !IS_AFFECTED(ch, AFF_FROZEN)) {
    if (FIGHTING(ch)) {
      do_flee(ch, NULL, 0, 0);
    }
  }
}

static bool tick_mob_grapple_escape(struct char_data *ch) {
  if (IS_NPC(ch) && GRAPPLED(ch) && !MOB_FLAGGED(ch, MOB_DUMMY) &&
      rand_number(1, 5) >= 4) {
    do_escape(ch, 0, 0, 0);
    return true;
  }
  return false;
}

static bool tick_mob_combat_ai(struct char_data *ch) {
  if (!FIGHTING(ch) || !IS_NPC(ch) || MOB_FLAGGED(ch, MOB_DUMMY))
    return false;

  struct char_data *vict = FIGHTING(ch);
  bool foe_flying = char_condition_has(vict, "flying");
  bool ch_flying  = char_condition_has(ch, "flying");

  auto flee_mob = [&](const char *msg) {
    act(msg, TRUE, ch, 0, 0, TO_ROOM);
    char_inventory_iterate(ch, [&](auto obj) { extract_obj(obj); return true; });
    extract_char(ch);
  };

  // Altitude matching: fly up to engage, or land if foe is grounded
  if (foe_flying && !ch_flying && IS_HUMANOID(ch) && GET_LEVEL(ch) > 10) {
    do_fly(ch, 0, 0, 0);
    return true;
  }
  if (!foe_flying && ch_flying) {
    do_fly(ch, 0, 0, 0);
    return true;
  }
  if (foe_flying && ch_flying && GET_ALT(ch) < GET_ALT(vict)) {
    do_fly(ch, "high", 0, 0);
    return true;
  }

  // Non-flyers facing an airborne foe may flee rather than fight
  if (foe_flying && !ch_flying && !IS_HUMANOID(ch) && GET_POS(ch) > POS_RESTING &&
      rand_number(1, 30) >= 22 && !block_calc(ch)) {
    flee_mob("$n@G flees in terror and you lose sight of $m!");
    return true;
  }
  if (foe_flying && IS_HUMANOID(ch) && GET_LEVEL(ch) <= 10 &&
      rand_number(1, 30) >= 22 && !block_calc(ch)) {
    flee_mob("$n@G turns and runs away. You lose sight of $m!");
    return true;
  }

  // Position recovery before attacking
  if ((GET_POS(ch) == POS_SITTING || GET_POS(ch) == POS_RESTING) && sec_roll_check(ch) == 1) {
    do_stand(ch, 0, 0, 0);
    return true;
  }
  if (IS_AFFECTED(ch, AFF_PARA) && GET_INT(ch) + 10 < rand_number(1, 60)) {
    act("@yYou fail to overcome your paralysis!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@Y$n @ystruggles with $s paralysis!@n", TRUE, ch, 0, 0, TO_ROOM);
    return true;
  }
  if (GET_POS(ch) == POS_SLEEPING && !AFF_FLAGGED(ch, AFF_KNOCKED) && sec_roll_check(ch) == 1) {
    do_wake(ch, 0, 0, 0);
    do_stand(ch, 0, 0, 0);
    return true;
  }

  // Can't attack if out of range, incapacitated, or not upright
  if (char_room_get(ch) != char_room_get(vict) || AFF_FLAGGED(ch, AFF_KNOCKED) ||
      GET_POS(ch) == POS_SITTING || GET_POS(ch) == POS_RESTING || GET_POS(ch) == POS_SLEEPING)
    return true;

  if (rand_number(1, 30) <= 12)
    return true;

  char buf[100];
  sprintf(buf, "%s", GET_NAME(vict));
  mob_attack(ch, buf);
  return false;
}

static void tick_barrier_skill(struct char_data *ch) {
  if (GET_BARRIER(ch) > 0) {
    improve_skill(ch, SKILL_BARRIER, 0);
  }
}

static void tick_player_powerup(struct char_data *ch) {
  if (PLR_FLAGGED(ch, PLR_POWERUP) && GET_POS(ch) <= POS_RESTING) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_POWERUP);
    return;
  }
  if (!PLR_FLAGGED(ch, PLR_POWERUP) || rand_number(1, 3) != 3)
    return;

  bool ki_pref   = (GET_PREFERENCE(ch) == PREFERENCE_KI);
  int64_t gmaxki = GET_MAX_MANA(ch);
  // ki_threshold: minimum ki needed to tick; ki_cost: ki consumed per tick
  int64_t ki_threshold = ki_pref ? (int64_t)(gmaxki * 0.0375) + 1 : gmaxki / 20;
  int64_t ki_cost      = ki_pref ? (int64_t)(gmaxki * 0.0375)     : gmaxki / 20;

  char buf3[MAX_STRING_LENGTH];

  auto st_boost = [&]() {
    incCurST(ch, (int64_t)(GET_MAX_MOVE(ch) * 0.02));
  };

  auto stop_powerup = [&](const char *reason) {
    act(reason, TRUE, ch, 0, 0, TO_CHAR);
    act("@R$n stops powering up in a flash of light!@n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_sense(0, "You sense someone stop powering up", ch);
    sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
    send_to_scouter(buf3, ch, 1, 0);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_POWERUP);
  };

  int64_t ghit    = GET_HIT(ch);
  int64_t gmaxhit = getMaxPL(ch);
  int64_t gki     = getCurKI(ch);

  if (ghit >= gmaxhit && gki >= ki_threshold) {
    if (ki_pref || gki >= gmaxki * 0.5) st_boost();
    restoreHealthAnnounced(ch, false);
    decCurKI(ch, ki_pref ? ki_threshold : getMaxKI(ch) / 20);
    dispel_ash(ch);
    stop_powerup("@RYou have reached your maximum!@n");
    return;
  }

  if (gki < ki_threshold) {
    decCurKI(ch, ki_threshold);
    stop_powerup("@RYou have run out of ki.@n");
    return;
  }

  // Active tick: ghit < gmaxhit && gki >= ki_threshold
  incCurHealthPercent(ch, .1);
  decCurKI(ch, ki_cost);
  if (getCurKI(ch) >= gmaxki * 0.5) st_boost();

  static const struct {
    int64_t threshold;
    const char *self_msg;
    const char *room_msg;
  } tiers[] = {
    {      50000, "@RYou continue to powerup, as wind billows out from around you!@n",                   "@R$n continues to powerup, as wind billows out from around $m!@n"                  },
    {     500000, "@RYou continue to powerup, as the ground splits beneath you!@n",                      "@R$n continues to powerup, as the ground splits beneath $m!@n"                     },
    {    5000000, "@RYou continue to powerup, as the ground shudders and splits beneath you!@n",         "@R$n continues to powerup, as the ground shudders and splits beneath $m!@n"         },
    {   50000000, "@RYou continue to powerup, as a huge depression forms beneath you!@n",                "@R$n continues to powerup, as a huge depression forms beneath $m!@n"               },
    {  100000000, "@RYou continue to powerup, as the entire area quakes around you!@n",                  "@R$n continues to powerup, as the entire area quakes around $m!@n"                 },
    {  300000000, "@RYou continue to powerup, as huge chunks of ground are ripped apart beneath you!@n", "@R$n continues to powerup, as huge chunks of ground are ripped apart beanth $m!@n" },
  };

  gmaxhit = getMaxPL(ch);
  const char *self_msg = "@RYou continue to powerup, as the very air around you crackles and burns!@n";
  const char *room_msg = "@R$n continues to powerup, as the very air around $m crackles and burns!@n";
  for (const auto &tier : tiers) {
    if (gmaxhit < tier.threshold) {
      self_msg = tier.self_msg;
      room_msg = tier.room_msg;
      break;
    }
  }
  act(self_msg, TRUE, ch, 0, 0, TO_CHAR);
  act(room_msg, TRUE, ch, 0, 0, TO_ROOM);

  send_to_sense(0, "You sense someone powering up", ch);
  send_to_worlds(ch);
  sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Detected@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
  send_to_scouter(buf3, ch, 1, 0);
  dispel_ash(ch);
}

static void fight_stack_one(struct char_data *ch) {
  reset_fighting_position(ch);

  if (tick_mob_cooldown(ch)) return;

  tick_mob_powerup(ch);

  if (tick_frozen_skip(ch)) return;
  if (tick_idle_skip(ch)) return;

  tick_fight_room_check(ch);
  tick_dragging_interrupt(ch);
  tick_lifeforce_heal(ch);
  tick_position_advantage(ch);
  tick_grapple_damage(ch);

  if (GRAPPLED(ch) && rand_number(1, 2) == 2)
    send_to_char(ch, "@CTry 'escape' to break free from the hold!@n\r\n");

  tick_halfbreed_fury(ch);
  tick_transformation_drain(ch);
  tick_wimp_flee(ch);

  if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 6 || GET_GENOME(ch, 1) == 6) &&
      rand_number(1, 200) >= 175)
    mutant_limb_regen(ch);

  tick_disguise_slip(ch);
  tick_mob_blind_recovery(ch);
  tick_knocked_recovery(ch);
  tick_linkdead_flee(ch);

  if (tick_mob_grapple_escape(ch)) return;
  if (tick_mob_combat_ai(ch)) return;

  tick_barrier_skill(ch);
  tick_player_powerup(ch);
}

void fight_stack() {
  char_iterate_all([](struct char_data *tch) {
    if (zone_player_count_get(char_zone_vnum_get(tch)))
      fight_stack_one(tch);
    return true;
  });
}

void appear(struct char_data *ch) {

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
}

void update_pos(struct char_data *victim) {
  if (AFF_FLAGGED(victim, AFF_KNOCKED)) {
    return;
  }
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_POS(victim) == POS_SITTING && FIGHTING(victim))
    return;
  else if (GET_POS(victim) == POS_SITTING && FIGHTING(victim))
    return;
  else if (GET_HIT(victim) > 0)
    char_position_set(victim, POS_STANDING);
  else if (GET_HIT(victim) <= -11)
    char_position_set(victim, POS_DEAD);
  else if (GET_HIT(victim) <= -6)
    char_position_set(victim, POS_MORTALLYW);
  else if (GET_HIT(victim) <= -3)
    char_position_set(victim, POS_INCAP);
  else
    char_position_set(victim, POS_STUNNED);
}

static void check_killer(struct char_data *ch, struct char_data *vict) {
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict) {
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  char_subscribe_add(ch, "combat");

  char_fighting_set(ch, vict);

  if (GET_POS(ch) == POS_SITTING) {
    char_position_set(ch, POS_SITTING);
  } else if (GET_POS(ch) == POS_SLEEPING) {
    char_position_set(ch, POS_SLEEPING);
  }

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch) {

  char_subscribe_remove(ch, "combat");

  char_condition_remove(ch, "combo", "end_combo");
  
  char_fighting_set(ch, NULL);
  if (AFF_FLAGGED(ch, AFF_POSITION)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POSITION);
  }
  update_pos(ch);
}

static void make_pcorpse(struct char_data *ch) {
  auto *corpse = init_corpse_obj(ch, CONFIG_MAX_PC_CORPSE_TIME);

  char_inventory_iterate(ch, [&](auto obj) {
    int vnum = GET_OBJ_VNUM(obj);
    if (vnum < 19900 && vnum != 17998 &&
        !(vnum >= 18800 && vnum <= 18999) &&
        !(vnum >= 19100 && vnum <= 19199)) {
      obj_from_char(obj);
      obj_to_obj(obj, corpse);
    }
    return true;
  });

  /* guard against gold duplication (desc check preserves CircleMUD gg 3/3/2002 invariant) */
  if (GET_GOLD(ch) > 0) {
    if (ch->desc)
      obj_to_obj(create_money(GET_GOLD(ch)), corpse);
    char_stat_set(ch, "money", 0);
  }

  obj_subscribe_add(corpse, "obj_corpse");
  obj_to_room(corpse, char_room_get(ch));
}

/* This handles how corpses are viewed. How many limbs they have. If they were *
 * disintergrated, blown in half, beat to a pulp, etc.        - Iovan 3/2/2011
 */
static void handle_corpse_condition(struct obj_data *corpse,
                                    struct char_data *ch) {
  GET_OBJ_VAL(corpse, VAL_CORPSE_HEAD) = 1;
  GET_OBJ_VAL(corpse, VAL_CORPSE_RARM) = 1;
  GET_OBJ_VAL(corpse, VAL_CORPSE_LARM) = 1;
  GET_OBJ_VAL(corpse, VAL_CORPSE_RLEG) = 1;
  GET_OBJ_VAL(corpse, VAL_CORPSE_LLEG) = 1;

  auto set_strings = [&](const char *name_fmt, const char *desc_fmt,
                         const char *short_fmt) {
    char buf[512];
    snprintf(buf, sizeof(buf), name_fmt, GET_NAME(ch));
    corpse->name = strdup(buf);
    snprintf(buf, sizeof(buf), desc_fmt, GET_NAME(ch));
    corpse->description = strdup(buf);
    snprintf(buf, sizeof(buf), short_fmt, GET_NAME(ch));
    corpse->short_description = strdup(buf);
  };

  switch (GET_DEATH_TYPE(ch)) {
  case DTYPE_HEAD:
    set_strings("headless corpse %s",
                "The headless corpse of %s is lying here",
                "The headless remains of %s's corpse");
    GET_OBJ_VAL(corpse, VAL_CORPSE_HEAD) = 0;
    break;
  case DTYPE_HALF:
    set_strings("half corpse %s",
                "Half of %s's corpse is lying here",
                "Half of %s's corpse");
    break;
  case DTYPE_VAPOR:
    set_strings("burnt chunks corpse %s",
                "The burnt chunks of %s's corpse are scattered here",
                "The burnt chunks of %s's corpse");
    break;
  case DTYPE_PULP:
    set_strings("beaten bloody corpse %s",
                "The bloody and beaten corpse of %s is lying here",
                "The bloody and beaten remains of %s's corpse");
    break;
  default:
    set_strings("corpse %s",
                "The corpse of %s is lying here",
                "the remains of %s's corpse");
    break;
  }

  if (IS_NPC(ch))
    return;

  auto set_limb = [&](int limb_idx, int corpse_slot) {
    int cond = GET_LIMBCOND(ch, limb_idx);
    if (cond <= 0)
      GET_OBJ_VAL(corpse, corpse_slot) = 0;
    else if (cond < 50)
      GET_OBJ_VAL(corpse, corpse_slot) = 2;
  };
  set_limb(1, VAL_CORPSE_RARM);
  set_limb(2, VAL_CORPSE_LARM);
  set_limb(3, VAL_CORPSE_RLEG);
  set_limb(4, VAL_CORPSE_LLEG);
}

static void scatter_ashes(struct char_data *ch) {
  act("@WSome ashes fall off the corpse.@n", TRUE, ch, 0, 0, TO_ROOM);
  int n = rand_number(1, 3);
  for (int i = 0; i < n; ++i) {
    auto *ashes = read_object(1305, VIRTUAL);
    obj_to_room(ashes, char_room_get(ch));
  }
}

static struct obj_data *init_corpse_obj(struct char_data *ch, int timer) {
  auto *corpse = create_obj();
  corpse->proto_id = NOTHING;
  IN_ROOM(corpse) = NOWHERE;

  handle_corpse_condition(corpse, ch);

  if (AFF_FLAGGED(ch, AFF_ASHED))
    scatter_ashes(ch);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_SIZE(corpse) = get_size(ch);
  for (int x = 0, y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; ++x, ++y) {
    if (x < EF_ARRAY_MAX) GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX) corpse->wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CAPACITY) = 0;
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE) = 1;
  GET_OBJ_VAL(corpse, VAL_CONTAINER_OWNER) = GET_PFILEPOS(ch);
  GET_OBJ_WEIGHT(corpse) = GET_PC_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_TIMER(corpse) = timer;
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_UNIQUE_SAVE);
  return corpse;
}

static void make_corpse(struct char_data *ch, struct char_data *tch) {
  int timer = IS_NPC(ch) ? CONFIG_MAX_NPC_CORPSE_TIME
                         : rand_number(CONFIG_MAX_PC_CORPSE_TIME / 2,
                                       CONFIG_MAX_PC_CORPSE_TIME);
  auto *corpse = init_corpse_obj(ch, timer);

  if (tch && !IS_NPC(tch)) {
    int skill = GET_SKILL(tch, SKILL_SURVIVAL);
    if (skill && !IS_HUMANOID(ch) && PRF_FLAGGED(tch, PRF_CARVE) &&
        axion_dice(0) < skill) {
      send_to_char(tch,
                   "The choice edible meat is preserved because of your skill.\r\n");
      auto *meat = read_object(1612, VIRTUAL);
      obj_to_char(meat, ch);
      char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH],
          nick3[MAX_INPUT_LENGTH];
      sprintf(nick, "@RRaw %s@R Steak@n", GET_NAME(ch));
      sprintf(nick2, "Raw %s Steak", ch->name);
      sprintf(nick3, "@wA @Rraw %s@R steak@w is lying here@n", GET_NAME(ch));
      meat->short_description = strdup(nick);
      meat->name = strdup(nick2);
      meat->description = strdup(nick3);
      GET_OBJ_MATERIAL(meat) = 14;
    }
  }

  if (MOB_FLAGGED(ch, MOB_HUSK)) {
    char_inventory_iterate(ch, [&](auto obj) {
      obj_from_char(obj);
      extract_obj(obj);
      return true;
    });
  } else {
    char_inventory_iterate(ch, [&](struct obj_data *o) {
      obj_from_char(o);
      obj_to_obj(o, corpse);
      return true;
    });

    char_equipment_iterate(ch, [&](auto i, auto eq) {
      remove_otrigger(eq, ch);
      obj_to_obj(unequip_char(ch, i), corpse);
      return true;
    });

    /* guard against gold duplication (desc check preserves CircleMUD gg 3/3/2002 invariant) */
    if (GET_GOLD(ch) > 0) {
      if (IS_NPC(ch) || ch->desc)
        obj_to_obj(create_money(GET_GOLD(ch)), corpse);
      char_stat_set(ch, "money", 0);
    }
  }

  obj_subscribe_add(corpse, "obj_corpse");
  obj_to_room(corpse, char_room_get(ch));
}

void loadmap(struct char_data *ch) {
  struct obj_data *obj;
  if (!IS_NPC(ch)) {
    obj = read_object(17, VIRTUAL);
    obj_to_char(obj, ch);
  }
}

/* When ch kills victim */
static void change_alignment(struct char_data *ch, struct char_data *victim) {
  /*
   * If you kill a monster with alignment A, you move 1/20th of the way to
   * having alignment -A.
   * Ethical alignments of killer and victim make this faster or slower.
   */

  /*if (GET_ALIGNMENT(ch) < -1000) {
    GET_ALIGNMENT(ch) = -1000;
   }
   if (GET_ALIGNMENT(ch) > 1000) {
    GET_ALIGNMENT(ch) = 1000;
   }*/
}

void death_cry(struct char_data *ch) {
  auto room = char_room_get(ch);
  if(!room) return;

  room_exits_iterate(room, [&](auto dir, auto exit) {
    if (auto dest = char_can_go_exit(ch, exit)) {
      send_to_room(dest,
                   "Your blood freezes as you hear someone's death cry.\r\n");
    }
    return true;
  });
}

/* Let's clean up necessary things after "death" */
static void final_combat_resolve(struct char_data *ch) {
  struct obj_data *chair;

  if (SITS(ch)) {
    chair = SITS(ch);
    SITS(ch) = NULL;
    SITTING(chair) = NULL;
  }
  if (!IS_NPC(ch) && char_condition_has(ch, "multiform_original")) {
    char_iterate_all([&](struct char_data *clone) {
      if (IS_NPC(clone) && GET_MOB_VNUM(clone) == 25 &&
          GET_ORIGINAL(clone) == ch) {
        handle_multi_merge(clone);
      }
      return true;
    });
  }
  if (CARRYING(ch)) {
    carry_drop(ch, 2);
  }
  if (CARRIED_BY(ch)) {
    carry_drop(CARRIED_BY(ch), 2);
  }
  if (DRAGGING(ch)) {
    char_being_dragged_set(DRAGGING(ch), NULL);
    char_dragging_set(ch, NULL);
  }
  if (DRAGGED(ch)) {
    char_dragging_set(DRAGGED(ch), NULL);
    char_being_dragged_set(ch, NULL);
  }
  if (GRAPPLING(ch)) {
    struct char_data *other = GRAPPLING(ch);
    char_grappling_set(ch, NULL, 0);
    char_grappled_set(other, NULL, 0);
  }
  if (GRAPPLED(ch)) {
    struct char_data *other = GRAPPLED(ch);
    char_grappled_set(ch, NULL, 0);
    char_grappling_set(other, NULL, 0);
  }
  if (BLOCKED(ch)) {
    char_blocking_set(BLOCKED(ch), NULL);
    char_blocked_by_set(ch, NULL);
  }
  if (BLOCKS(ch)) {
    char_blocked_by_set(BLOCKS(ch), NULL);
    char_blocking_set(ch, NULL);
  }
  if (ABSORBING(ch)) {
    char_absorbed_by_set(ABSORBING(ch), NULL);
    char_absorbing_set(ch, NULL);
  }
  if (ABSORBBY(ch)) {
    char_absorbing_set(ABSORBBY(ch), NULL);
    char_absorbed_by_set(ch, NULL);
  }
}

enum DeathType : uint8_t { Afterlife = 0, Northran = 1, Past = 2, Newbie = 3 };

void raw_kill(struct char_data *ch, struct char_data *killer) {
  struct char_data *k, *temp;

  if (FIGHTING(ch))
    stop_fighting(ch);

  /* To make ordinary commands work in scripts.  welcor*/
  if (GET_POS(ch) != POS_SITTING && GET_POS(ch) != POS_SLEEPING &&
      GET_POS(ch) != POS_RESTING)
    char_position_set(ch, POS_STANDING);

  if (killer && !IS_NPC(killer)) {
    if (!IS_NPC(killer) && !IS_NPC(ch)) {
      send_to_imm("[PK] %s killed %s at room [%d]\r\n", GET_NAME(killer),
                  GET_NAME(ch), char_room_vnum_get(killer));
    }
    if ((IS_SAIYAN(killer) && rand_number(1, 2) == 2) || !IS_SAIYAN(killer)) {
      if (rand_number(1, 6) >= 5 &&
          (level_exp(killer, GET_LEVEL(killer) + 1) - GET_EXP(killer) > 0 ||
           GET_LEVEL(killer) == 100)) {
        int psreward = GET_WIS(killer) * 0.35;
        if (GET_LEVEL(killer) > GET_LEVEL(ch) + 5) {
          psreward *= 0.2;
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 2) {
          psreward *= 0.5;
        }
        if (IS_HUMAN(killer) ||
            (IS_BIO(killer) &&
             (GET_GENOME(killer, 0) == 1 || GET_GENOME(killer, 1) == 1))) {
          psreward *= 1.25;
        }
        if (IS_HALFBREED(killer)) {
          psreward *= 0.6;
        }
        if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_HUSK) &&
            GET_PRACTICES(killer, GET_CLASS(killer)) > 50 && IS_BIO(ch)) {
          psreward = 0;
          send_to_char(killer,
                       "@D[@G+0 @BPS @cCapped at 50 for Absorb@D]@n\r\n");
        } else {
          char_stat_mod(killer, "practices", psreward);
          send_to_char(killer, "@D[@G+%d @BPS@D]@n\r\n", psreward);
        }
      }
    }
    if (IS_ANDROID(killer) && !IS_NPC(killer) &&
        !PLR_FLAGGED(killer, PLR_ABSORB)) {
      if (PLR_FLAGGED(killer, PLR_REPAIR)) {
        if (GET_LEVEL(killer) > GET_LEVEL(ch) + 15) {
          send_to_char(killer, "@D[@G+0 @mUpgrade Point @r-WEAK-@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 10) {
          char_stat_mod(killer, "upgrades", 3);
          send_to_char(killer, "@D[@G+3 @mUpgrade Point@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 8) {
          char_stat_mod(killer, "upgrades", 6);
          send_to_char(killer, "@D[@G+6 @mUpgrade Points@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 4) {
          char_stat_mod(killer, "upgrades", 12);
          send_to_char(killer, "@D[@G+12 @mUpgrade Points@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 2) {
          char_stat_mod(killer, "upgrades", 16);
          send_to_char(killer, "@D[@G+16 @mUpgrade Points@D]@n\r\n");
        } else {
          char_stat_mod(killer, "upgrades", 28);
          send_to_char(killer, "@D[@G+28 @mUpgrade Points@D]@n\r\n");
        }
      } else {
        if (GET_LEVEL(killer) > GET_LEVEL(ch) + 15) {
          send_to_char(killer, "@D[@G+0 @mUpgrade Point @r-WEAK-@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 10) {
          char_stat_mod(killer, "upgrades", 5);
          send_to_char(killer, "@D[@G+5 @mUpgrade Point@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 6) {
          char_stat_mod(killer, "upgrades", 12);
          send_to_char(killer, "@D[@G+12 @mUpgrade Points@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 4) {
          char_stat_mod(killer, "upgrades", 18);
          send_to_char(killer, "@D[@G+18 @mUpgrade Points@D]@n\r\n");
        } else if (GET_LEVEL(killer) > GET_LEVEL(ch) + 2) {
          char_stat_mod(killer, "upgrades", 28);
          send_to_char(killer, "@D[@G+28 @mUpgrade Points@D]@n\r\n");
        } else {
          char_stat_mod(killer, "upgrades", 36);
          send_to_char(killer, "@D[@G+36 @mUpgrade Points@D]@n\r\n");
        }
      }
    }
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);

  update_pos(ch);

  struct room_data *room = char_room_get(ch);

  if (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_DUMMY)) {
    int shadowed = FALSE;
    decCurHealthPercent(ch, 1);
    if (IS_SHADOW_DRAGON1(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON1 = -1;
      send_to_room(room,
                   "@YThe one star dragon ball falls to the ground!@n\r\n");

      obj = read_object(20, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    } else if (IS_SHADOW_DRAGON2(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON2 = -1;
      send_to_room(room,
                   "@YThe two star dragon ball falls to the ground!@n\r\n");

      obj = read_object(21, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    } else if (IS_SHADOW_DRAGON3(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON3 = -1;
      send_to_room(room,
                   "@YThe three star dragon ball falls to the ground!@n\r\n");

      obj = read_object(22, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    } else if (IS_SHADOW_DRAGON4(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON4 = -1;
      send_to_room(room,
                   "@YThe four star dragon ball falls to the ground!@n\r\n");

      obj = read_object(23, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    } else if (IS_SHADOW_DRAGON5(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON5 = -1;
      send_to_room(room,
                   "@YThe five star dragon ball falls to the ground!@n\r\n");

      obj = read_object(24, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    } else if (IS_SHADOW_DRAGON6(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON6 = -1;
      send_to_room(room,
                   "@YThe six star dragon ball falls to the ground!@n\r\n");

      obj = read_object(25, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    } else if (IS_SHADOW_DRAGON7(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON7 = -1;
      send_to_room(room,
                   "@YThe seven star dragon ball falls to the ground!@n\r\n");

      obj = read_object(26, VIRTUAL);
      obj_to_room(obj, char_room_get(ch));
      shadowed = TRUE;
    }
    make_corpse(ch, killer);
    purge_homing(ch);
    extract_char(ch);
    if (shadowed == TRUE) {
      shadow_dragons_live();
    }
  } else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_DUMMY)) {
    decCurHealthPercent(ch, 1);
    extract_char(ch);
  } else {
    if (!AFF_FLAGGED(ch, AFF_SPIRIT) &&
        !room_flagged(char_room_get(ch), ROOM_PAST) &&
        (char_room_vnum_get(ch) < 17900 || char_room_vnum_get(ch) > 17999)) {
      if (!PLR_FLAGGED(ch, PLR_ABSORBED)) {
        make_pcorpse(ch);
        loadmap(ch);
      } else {
        REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_ABSORBED);
      }
    }
    final_combat_resolve(ch);
    if (FIGHTING(ch))
      stop_fighting(ch);

    char_iterate_subscriptions("combat", [&](auto k) {
      if (FIGHTING(k) == ch)
        stop_fighting(k);
      return true;
    });

    bool android_lose = true;
    DeathType death_type = Afterlife;
    if (in_past(ch))
      death_type = Past;
    else if (in_northran(ch))
      death_type = Northran;
    else if (is_newbie(ch))
      death_type = Newbie;

    switch (death_type) {
    case Afterlife:
      ghostify(ch);
      purge_homing(ch);
      if (GET_LEVEL(ch) > 0 && has_group(ch)) {
        if (MASTER(ch) != NULL) {
          group_bonus(ch, 1);
        } else {
          group_bonus(ch, 0);
        }
      }
      teleport_to(ch, 6000);
      break;
    case Northran:
      restore(ch, false);
      teleport_to(ch, 17900);
      android_lose = false;
      send_to_char(ch, "You wake up and realise that you didn't die, how or "
                       "why are a mystery.\r\n");
      break;
    case Past:
      restore(ch, false);
      teleport_to(ch, 1561);
      android_lose = false;
      send_to_char(
          ch,
          "You wake up and realise that you died, but only in your mind.\r\n");
      break;
    case Newbie:
      restore(ch, false);
      teleport_to(ch, sensei_start_room(ch->chclass));
      send_to_char(ch, "\r\n@RYou should beware, when you reach level 9, you "
                       "will actually die. So you\r\n"
                       "should learn to be more careful. Since when you die "
                       "past that point and\r\n"
                       "actually reach the afterlife you need to realise that "
                       "being revived will\r\n"
                       "not be very easy. So treat your character's dying with "
                       "as much care as\r\n"
                       "possible.@n\r\n");
      break;
    }

    if (!IS_NPC(ch)) {
      if (IS_ANDROID(ch) && !PLR_FLAGGED(ch, PLR_ABSORB) && android_lose &&
          GET_UP(ch) > 5) {
        int loss = GET_UP(ch) / 5;
        char_stat_mod(ch, "upgrades", -loss);
        send_to_char(ch, "@rYou lose @R%s@r upgrade points!@n\r\n",
                     add_commas(loss));
      }
      Crash_delete_crashfile(ch);
      save_char(ch);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }
}

void die(struct char_data *ch, struct char_data *killer) {
  // NPCs just die.
  if (IS_NPC(ch)) {
    raw_kill(ch, killer);
    return;
  }

  // But there's special handling for players.

  // Clear healing tank usage. How did you die while inside a healing tank
  // though?
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_HEALT);
  }

  // First there are a few mechanics which can prevent death. They are checked
  // first.

  // Saiyan Zenkai mechanic: if at 75% lifeforce or higher, 25% chance of
  // triggering Zenkai. Added check for character having AFF_SPIRIT to prevent
  // triggering in Afterlife this is implemented by setting PLR_GOOP with
  // gooptime 0
  if (IS_SAIYAN(ch) && !AFF_FLAGGED(ch, AFF_SPIRIT) &&
      (getCurLFPercent(ch) >= 0.75) && rand_number(1, 4) == 4) {
    SET_BIT_AR(PLR_FLAGS(ch), PLR_GOOP);
    ch->gooptime = 0;
    decCurLFPercent(ch, 0.5);
    return;
  }

  // majin and bio regen mechanic skips actually dying...
  if ((IS_MAJIN(ch) || IS_BIO(ch)) &&
      ((getCurLF(ch)) >= (getMaxLF(ch)) * 0.75 ||
       (PLR_FLAGGED(ch, PLR_SELFD2) &&
        (getCurLF(ch)) >= (getMaxLF(ch)) * 0.5))) {
    decCurLFPercentFloored(ch, 2, -1);
    decCurHealthPercentFloored(ch, 1, 1);
    SET_BIT_AR(PLR_FLAGS(ch), PLR_GOOP);
    ch->gooptime = 32;
    return;
  }

  // for players who used the Immortal Wish.
  if (PLR_FLAGGED(ch, PLR_IMMORTAL)) {
    act("@c$n@w disappears right before dying. $n appears to be immortal.@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@w disappears right before dying. $n appears to be immortal.@n.",
        TRUE, ch, 0, 0, TO_ROOM);
    decCurHealthPercentFloored(ch, 1, 1);
    decCurKIPercentFloored(ch, 1, 1);
    decCurSTPercentFloored(ch, 1, 1);
    char_condition_remove(ch, "poison", "immortal_wish");
    if (char_stat_get(ch, "hunger") >= 0) {
      char_stat_set(ch, "hunger", 48);
    }
    if (char_stat_get(ch, "thirst") >= 0) {
      char_stat_set(ch, "thirst", 48);
    }
    if (FIGHTING(ch)) {
      stop_fighting(ch);
    }
    char_position_set(ch, POS_SITTING);
    teleport_to(ch, sensei_start_room(ch->chclass));
    return;
  }

  // Removing some consequences that might happen.
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);

  char_condition_remove_tag(ch, "remove_on_death", "death");

  // those who die in the arena don't actually die. They get returned to the
  // waiting room.
  if (IN_ARENA(ch)) {
    cleanup_arena_watch(ch);
    if (killer != NULL) {
      cleanup_arena_watch(killer);
      send_to_all("@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n",
                  GET_NAME(killer), GET_NAME(ch));
      final_combat_resolve(killer);
      final_combat_resolve(ch);
      teleport_to(killer, 17875);
    } else {
      send_to_all(
          "@R%s@r dies in the water of the Arena and is disqualified!@n\r\n",
          GET_NAME(ch));
    }
    char_from_room(ch);
    char_to_room(ch, room_by_id(17875));
    decCurHealthPercentFloored(ch, 1, 1);
    look_at_room(char_room_get(ch), ch, 0);
    final_combat_resolve(ch);
    return;
  }

  // For code below this, some kind of 'death' is performed.
  // it will leave a corpse.

  if (!AFF_FLAGGED(ch, AFF_SPIRIT) &&
      !room_flagged(char_room_get(ch), ROOM_PAST) && GET_LEVEL(ch) > 8) {
    if (char_room_vnum_get(ch) >= 2002 && char_room_vnum_get(ch) <= 2011) {
      GET_DTIME(ch) = time(0);
    } else if (room_flagged(char_room_get(ch), ROOM_AL) ||
               room_flagged(char_room_get(ch), ROOM_HELL)) {
      send_to_char(ch, "Your soul is saved from destruction by King Yemma. "
                       "Why? Who knows.\r\n");
    } else {
      if (killer != NULL && IS_NPC(killer)) {
        GET_DTIME(ch) = time(0) + 7200;
        char_stat_mod(ch, "death_count", 1);
      } else if (killer != NULL && !IS_NPC(killer)) {
        GET_DTIME(ch) = time(0) + 345600;
        SET_BIT_AR(PLR_FLAGS(ch), PLR_PDEATH);
        char_stat_mod(ch, "death_count", 1);
      } else {
        GET_DTIME(ch) = time(0) + 7200;
        char_stat_mod(ch, "death_count", 1);
      }
    }
    if (char_stat_get(ch, "hunger") >= 0) {
      char_stat_set(ch, "hunger", 48);
    }
    if (char_stat_get(ch, "thirst") >= 0) {
      char_stat_set(ch, "thirst", 48);
    }
  }

  raw_kill(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base,
                               struct char_data *victim) {
  int64_t share;

  if (IN_ARENA(ch)) {
    return;
  }
  struct char_data *leader = MASTER(ch) ? MASTER(ch) : ch;

  /*share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base * GET_LEVEL(ch)));*/
  share = MIN(2000000, base * GET_LEVEL(ch));
  if (!IS_NPC(ch)) {
    if (GET_LEVEL(ch) >= 100 && GET_MAX_HIT(ch) * .025 >= GET_MAX_HIT(victim)) {
      share *= .05;
    } else if (GET_MAX_HIT(ch) * .025 >= GET_MAX_HIT(victim)) {
      share = 1;
    } else if (GET_MAX_HIT(ch) * .05 >= GET_MAX_HIT(victim)) {
      share *= .05;
    } else if (GET_MAX_HIT(ch) * .1 >= GET_MAX_HIT(victim)) {
      share *= .1;
    } else if (GET_MAX_HIT(ch) * .15 >= GET_MAX_HIT(victim)) {
      share *= .15;
    } else if (GET_MAX_HIT(ch) * .25 >= GET_MAX_HIT(victim)) {
      share *= .25;
    } else if (GET_MAX_HIT(ch) * .5 >= GET_MAX_HIT(victim)) {
      share *= .5;
    } else if (GET_MAX_HIT(ch) * .9 >= GET_MAX_HIT(victim)) {
      share *= .65;
    } else if (GET_MAX_HIT(ch) >= GET_MAX_HIT(victim)) {
      share *= .7;
    }
  }
  if (LASTHIT(victim) != 0 && LASTHIT(victim) != GET_IDNUM(ch)) {
    int checkit = FALSE;
    char_followers_iterate(ch, [&](struct char_data *fol) {
      if (char_condition_has(fol, "group") &&
          LASTHIT(victim) == GET_IDNUM(fol))
        checkit = TRUE;
      return true;
    });
    if (checkit == FALSE && MASTER(ch) != NULL &&
        GET_IDNUM(MASTER(ch)) == LASTHIT(victim)) {
      checkit = TRUE;
    }
    if (checkit == FALSE && MASTER(ch) != NULL) {
      struct char_data *master = MASTER(ch);
      char_followers_iterate(master, [&](struct char_data *fol) {
        if (fol != ch && char_condition_has(fol, "group") &&
            LASTHIT(victim) == GET_IDNUM(fol))
          checkit = TRUE;
        return true;
      });
    }
    if (checkit == FALSE) {
      send_to_char(ch, "@RYou didn't do most of the work for this kill.@n\r\n");
      share = 1;
    }
  }
  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_HUSK)) {
    share /= 10;
  }
  if (GET_BONUS(ch, BONUS_PRODIGY) > 0) {
    share = share + (share * .25);
  }
  if (IS_SAIYAN(ch)) {
    share = share + (share * .50);
  }
  if (IS_HALFBREED(ch)) {
    share = share + (share * .40);
  }
  if (IS_ICER(ch)) {
    share = share - (share * .20);
  }
  if (GET_BONUS(ch, BONUS_LOYAL) > 0 && MASTER(ch) != NULL) {
    share += share * 0.2;
  }
  if (MASTER(ch) != NULL && MASTER(ch) != ch) {
    share += share * 0.15;
  }
  if (MOB_FLAGGED(victim, MOB_KNOWKAIO)) {
    share += share * .25;
  }
  auto group_kills = GET_GROUPKILLS(ch);
  group_kills += 1;
  char_condition_number_set(ch, "group", "kills", group_kills);
  if (group_kills / 20 > share * 0.16) {
    share += share * 0.16;
  } else {
    share += (share * 0.02) * (group_kills / 20);
  }
  if (group_bonus(ch, 2) == 2) {
    send_to_char(
        ch,
        "You receive a bonus from your group's leader! @D[@G+2 PS!@D]@n\r\n");
    char_stat_mod(ch, "practices", 2);
  } else if (group_bonus(ch, 2) == 3) {
    send_to_char(
        ch,
        "You receive a bonus from your group's leader! @D[@G+5%s Exp!@D]@n\r\n",
        "%");
    share += share * 0.05;
  } else if (group_bonus(ch, 2) == 5) {
    incCurKIPercent(ch, .04);
    send_to_char(ch,
                 "You receive a bonus from your group's leader! @D[@G4%s Ki "
                 "Regenerated!@D]@n\r\n",
                 "%");
  } else if (group_bonus(ch, 2) == 6) {
    incCurKIPercent(ch, .02);
    incCurSTPercent(ch, .02);
    incCurHealthPercent(ch, .02);
    send_to_char(ch,
                 "You receive a bonus from your group's leader! @D[@G2%s "
                 "PL/ST/Ki Regenerated!@D]@n\r\n",
                 "%");
  } else if (group_bonus(ch, 2) == 7) {
    if (IS_ANDROID(ch)) {
      if (PLR_FLAGGED(leader, PLR_ABSORB)) {
        incCurKIPercent(ch, .02);
        incCurSTPercent(ch, .02);
        send_to_char(ch,
                     "You receive a bonus from your group's leader! @D[@G2%s "
                     "PL/ST/Ki Recovered!@D]@n\r\n",
                     "%");
      } else if (PLR_FLAGGED(leader, PLR_REPAIR)) {
        incCurHealthPercent(ch, .02);
        send_to_char(ch,
                     "You receive a bonus from your group's leader! @D[@G5%s "
                     "PL Repaired@D]@n\r\n",
                     "%");
      } else if (PLR_FLAGGED(leader, PLR_SENSEM) &&
                 !PLR_FLAGGED(ch, PLR_ABSORB)) {
        char_stat_mod(ch, "upgrades", 5);
        send_to_char(ch, "You receive a bonus from your group's leader! "
                         "@D[@G+5 @mUpgrade Points@D]@n\r\n");
      }
    } else {
      incCurHealthPercent(ch, .01);
      incCurKIPercent(ch, .01);
      incCurSTPercent(ch, .01);
    }
  } else if (group_bonus(ch, 2) == 11) {
    incCurSTPercent(ch, .04);
    send_to_char(ch,
                 "You receive a bonus from your group's leader! @D[@G4%s ST "
                 "Regenerated!@D]@n\r\n",
                 "%");
  } else if (group_bonus(ch, 2) == 13) {
    if (GET_PHASE(leader) == 1) {
      share += share * 0.05;
      send_to_char(ch,
                   "You receive a bonus from your group's leader! @D[@G+5%s "
                   "Exp!@D]@n\r\n",
                   "%");
    } else if (GET_PHASE(leader) == 2) {
      share += share * 0.1;
      send_to_char(ch,
                   "You receive a bonus from your group's leader! @D[@G+10%s "
                   "Exp!@D]@n\r\n",
                   "%");
    }
  }
  share = gear_exp(ch, share);
  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %s points.\r\n",
                 add_commas(share));
  else
    send_to_char(
        ch,
        "You receive your share of experience -- one measly little point!\r\n");

  gain_exp(ch, share);
  /*change_alignment(ch, victim);*/
}

void group_gain(struct char_data *ch, struct char_data *victim) {
  int tot_levels, tot_members;
  int64_t tot_gain, base;
  struct char_data *k;

  if (!(k = MASTER(ch)))
    k = ch;

  if (char_condition_has(k, "group") && (char_room_get(k) == char_room_get(ch))) {
    tot_levels = GET_LEVEL(k);
    tot_members = 1;
  } else {
    tot_levels = 0;
    tot_members = 0;
  }

  char_followers_iterate(k, [&](struct char_data *fol) {
    if (char_condition_has(fol, "group") &&
        char_room_get(fol) == char_room_get(ch)) {
      if (!IS_WEIGHTED(fol)) {
        tot_levels += GET_LEVEL(fol);
        tot_members++;
      } else if (getMaxPL(fol) >= getMaxPL(ch) * 0.5) {
        tot_levels += GET_LEVEL(fol);
        tot_members++;
      }
    }
    return true;
  });

  /* round up to the next highest tot_members */
  tot_gain = (GET_EXP(victim)) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2, tot_gain);

  if (tot_levels >= 1) {
    base = MAX(1, tot_gain / tot_levels);
    int perc = 20 * tot_members;
    if (perc >= 80) {
      perc = 60;
    }
    base += (base / 100) * perc;
  } else
    base = 0;

  /*
  if (char_condition_has(k, "group") && char_room_get(k) == char_room_get(ch)) {
   if (!IS_WEIGHTED(k)) {
    perform_group_gain(k, base, victim);
   } else if (k != ch && (getMaxPL(k)()) >= (getMaxPL(ch)) * 0.5) {
    perform_group_gain(k, base, victim);
   } else if (k == ch && (getMaxPL(k)()) >= GET_MAX_HIT(ch) * 0.5) {
    perform_group_gain(k, base, victim);
   } else {
    if (k == ch) {
     send_to_char(ch, "You can not group gain while your powerlevel is weighted
  down more than half of your max.\r\n"); } else { send_to_char(ch, "You can not
  group gain while your powerlevel is weighted down more than half of the
  leader's adjusted powerlevel.\r\n");
    }
   }
  }
   */
  // perform_group_gain(k, base, victim);

  char_followers_iterate(k, [&](struct char_data *fol) {
    if (char_condition_has(fol, "group") &&
        char_room_get(fol) == char_room_get(ch)) {
      // if (getMaxPL(fol) >= GET_MAX_HIT(ch) * 0.5)
      // perform_group_gain(fol, base, victim);
    }
    return true;
  });
}
