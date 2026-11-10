/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/consts/deathtype.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/fight.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.attack.hpp"
#include "dbat/act.other.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/spells.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/config.hpp"
#include "dbat/races.hpp"
#include "dbat/handler.hpp"
#include "dbat/combat.hpp"
#include "dbat/class.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/affect.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/act.item.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* Structures */

/* local functions */
static void perform_group_gain(GameWorld *g, struct char_data *ch, int base, struct char_data *victim);
static void check_killer(struct char_data *ch, struct char_data *vict);
static void make_corpse(GameWorld *g, struct char_data *ch, struct char_data *tch);
static void handle_corpse_condition(struct obj_data *corpse, struct char_data *ch);
static void make_pcorpse(GameWorld *g, struct char_data *ch);
static void final_combat_resolve(GameWorld *g, struct char_data *ch);
static void shadow_dragons_live(GameWorld *g);
static void cleanup_arena_watch(GameWorld *g, struct char_data *ch);
static void mob_attack(GameWorld *g, struct char_data *ch, char *buf);
static bool pick_n_throw(GameWorld *g, struct char_data *ch, char *buf);

int group_bonus(GameWorld *g, struct char_data *ch, int type)
{

  if (!char_aff_flagged(ch, AFF_GROUP))
    return (false);

  if (char_get_followers(ch)) {
    for (char_data *fol : char_followers(g, ch)) {
      if (!char_aff_flagged(fol, AFF_GROUP)) {
        continue;
      }
      if (type == 0) {
        incCurLFPercent(g, fol, .25, true);
        send_to_char(g, fol, "@CIncensed by the death of your comrade your life force swells!@n");
        return (true);
      }
      if (type == 1) {
        incCurLFPercent(g, fol, .4, true);
        send_to_char(g, fol, "@CIncensed by the death of your comrade your life force swells!@n");
        return (true);
      }
      if (type == 2) {
        if (char_is_roshi(ch)) {
          return (2);
        }
        if (char_is_krane(ch)) {
          return (3);
        }
        if (char_is_bardock(ch)) {
          return (4);
        }
        if (char_is_nail(ch)) {
          return (5);
        }
        if (char_is_kabito(ch)) {
          return (6);
        }
        if (char_is_andsix(ch)) {
          return (7);
        }
        if (char_is_tapion(ch)) {
          return (8);
        }
        if (char_is_frieza(ch)) {
          return (9);
        }
        if (char_is_tsuna(ch)) {
          return (10);
        }
        if (char_is_piccolo(ch)) {
          return (11);
        }
        if (char_is_kurzak(ch)) {
          return (12);
        }
        if (char_is_jinto(ch)) {
          return (13);
        }
        if (char_is_dabura(ch)) {
          return (14);
        }
      }
      return (false);
    }
  } else if (char_get_master(g, ch)) {
    if (!char_aff_flagged(char_get_master(g, ch), AFF_GROUP))
      return (false);
    else {
      if (type == 0) {
        group_bonus(g, char_get_master(g, ch), 0);
      } else if (type == 2) {
        if (char_is_roshi(char_get_master(g, ch))) {
          return (2);
        }
        if (char_is_krane(char_get_master(g, ch))) {
          return (3);
        }
        if (char_is_bardock(char_get_master(g, ch))) {
          return (4);
        }
        if (char_is_nail(char_get_master(g, ch))) {
          return (5);
        }
        if (char_is_kabito(char_get_master(g, ch))) {
          return (6);
        }
        if (char_is_andsix(char_get_master(g, ch))) {
          return (7);
        }
        if (char_is_tapion(char_get_master(g, ch))) {
          return (8);
        }
        if (char_is_frieza(char_get_master(g, ch))) {
          return (9);
        }
        if (char_is_tsuna(char_get_master(g, ch))) {
          return (10);
        }
        if (char_is_piccolo(char_get_master(g, ch))) {
          return (11);
        }
        if (char_is_kurzak(char_get_master(g, ch))) {
          return (12);
        }
        if (char_is_jinto(char_get_master(g, ch))) {
          return (13);
        }
        if (char_is_dabura(char_get_master(g, ch))) {
          return (14);
        }
      }
      return (true);
    }
  }
  return (false);
}

void mutant_limb_regen(GameWorld *g, struct char_data *ch)
{
  if (char_get_limb_condition(ch, (1) - 1) > 0 && char_get_limb_condition(ch, (1) - 1) < 50) {
    act(g, "The bones in your right arm have mended them selves.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n starts moving $s right arm gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (1) - 1, 100);
  } else if (char_get_limb_condition(ch, (1) - 1) <= 0) {
    act(g, "Your right arm begins to grow back very quickly. Within moments it is whole again!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n's right arm starts to regrow! Within moments the arm is whole again!.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (1) - 1, 100);
  }
  if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50) {
    act(g, "The bones in your left arm have mended them selves.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n starts moving $s left arm gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (2) - 1, 100);
  } else if (char_get_limb_condition(ch, (2) - 1) <= 0) {
    act(g, "Your right arm begins to grow back very quickly. Within moments it is whole again!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n's right arm starts to regrow! Within moments the arm is whole again!.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (2) - 1, 100);
  }
  if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50) {
    act(g, "The bones in your right leg have mended them selves.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n starts moving $s right leg gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (3) - 1, 100);
  } else if (char_get_limb_condition(ch, (3) - 1) <= 0) {
    act(g, "Your right arm begins to grow back very quickly. Within moments it is whole again!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n's right arm starts to regrow! Within moments the arm is whole again!.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (3) - 1, 100);
  }
  if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50) {
    act(g, "The bones in your left leg have mended them selves.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n starts moving $s left leg gingerly for a moment.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (4) - 1, 100);
  } else if (char_get_limb_condition(ch, (4) - 1) <= 0) {
    act(g, "Your right arm begins to grow back very quickly. Within moments it is whole again!", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n's right arm starts to regrow! Within moments the arm is whole again!.", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_limb_condition(ch, (4) - 1, 100);
  }
}

static bool pick_n_throw(GameWorld *g, struct char_data *ch, char *buf)
{
  char buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  ;

  if (rand_number(g, 1, 20) < 18) {
    return (false);
  }

  for (obj_data *cont : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_weight(cont) <= getMaxCarryWeight(g, ch) + char_get_carry_weight(ch)) {
      snprintf(buf2, sizeof(buf2), "%s", obj_get_name(cont));
      do_get(g, ch, buf2, 0, 0);
      snprintf(buf3, sizeof(buf3), "%s %s", buf2, buf);
      do_throw(g, ch, buf3, 0, 0);
      return (true);
    }
  }

  return (false);
}

static void mob_attack(GameWorld *g, struct char_data *ch, char *buf)
{

  int power   = rand_number(g, 1, 5);
  int bonus   = char_get_level(ch) * 0.1;
  int special = 0;
  char buf2[MAX_INPUT_LENGTH];

  power += bonus;

  if (rand_number(g, 1, 4) == 4)
    power += 10;

  if (power > 20)
    power = 20;

  if (char_get_class(ch) == CLASS_NPC_COMMONER)
    special = 0;

  int dragonpass = true;

  if (char_is_dragon(ch)) {
    if (char_get_mob_vnum(g, ch) == 81 || char_get_mob_vnum(g, ch) == 82 || char_get_mob_vnum(g, ch) == 83 || char_get_mob_vnum(g, ch) == 84 || char_get_mob_vnum(g, ch) == 85 || char_get_mob_vnum(g, ch) == 86 || char_get_mob_vnum(g, ch) == 87) {
      dragonpass = true;
      special    = rand_number(g, 40, 100);
    } else {
      dragonpass = false;
    }
  }

  if (axion_dice(g, -10) > 90 && getCurHealthPercent(g, ch) <= .5 && !char_mob_flagged(ch, MOB_POWERUP) && char_get_mob_vnum(g, ch) != 25 &&
      !(char_is_android(ch) || char_is_animal(ch) || char_get_class(ch) == CLASS_NPC_COMMONER)) {
    do_powerup(g, ch, "", 0, 0);
    return;
  }

  if ((getCurKI(g, ch)) >= getMaxKI(g, ch) * 0.05 && char_is_humanoid(ch) && (!char_is_dragon(ch) || dragonpass == true)) {
    if (char_get_mobcharge(ch) <= 0 && rand_number(g, 1, 10) >= 8) {
      act(g, "@wAn aura flares up around @R$n@w!@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_mobcharge(ch, char_get_mobcharge(ch) + 1);
      if (char_get_level(ch) > 80) {
        char_set_mobcharge(ch, char_get_mobcharge(ch) + 1);
      }
    } else if (char_get_mobcharge(ch) <= 5) {
      act(g, "@wThe aura burns brighter around @R$n@w!@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_mobcharge(ch, char_get_mobcharge(ch) + 1);
      if (char_get_level(ch) > 80) {
        char_set_mobcharge(ch, char_get_mobcharge(ch) + 1);
      }
    } else if (char_get_mobcharge(ch) == 6) {
      act(g, "@wThe aura around @R$n@w flashes!@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_mobcharge(ch, char_get_mobcharge(ch) + 1);
      special = 100;
    }
  }

  if (char_is_humanoid(ch) && dragonpass == true) { /* Is a humanoid mob */
    if (char_aff_flagged(ch, AFF_PARALYZE)) {
      return;
    }
    if (char_aff_flagged(ch, AFF_ENSNARED)) {
      return;
    } else if (special < 100) { /* Normal physical attack */
      if (char_get_class(ch) == CLASS_SHADOWDANCER && rand_number(g, 1, 3) == 3) {
        sprintf(buf2, "ass %s", buf);
        do_throw(g, ch, buf2, 0, 0);
      } else if (char_is_android(ch) && char_mob_flagged(ch, MOB_REPAIR) && getCurPL(g, ch) <= (getEffMaxPL(g, ch)) * 0.5 && rand_number(g, 1, 20) >= 16) {
        do_srepair(g, ch, "", 0, 0);
      } else if (char_is_android(ch) && char_mob_flagged(ch, MOB_ABSORB) && rand_number(g, 1, 20) >= 19) {
        do_absorb(g, ch, buf2, 0, 0);
      } else if ((char_is_bio(ch) || char_is_majin(ch)) && getCurPL(g, ch) <= (getEffMaxPL(g, ch)) * 0.5 && rand_number(g, 1, 20) >= 17) {
        do_regenerate(g, ch, "25", 0, 0);
      } else if (char_is_namek(ch) && getCurPL(g, ch) <= (getEffMaxPL(g, ch)) * 0.5 && rand_number(g, 1, 20) == 20) {
        do_regenerate(g, ch, "25", 0, 0);
      } else if (pick_n_throw(g, ch, buf)) {
        /* This determines if they throw and also handles it */
      } else if (char_mob_flagged(ch, MOB_KNOWKAIO) && rand_number(g, 1, 50) >= 46) {
        if (rand_number(g, 1, 10) == 10) {
          do_kaioken(g, ch, "20", 0, 0);
        } else if (rand_number(g, 1, 10) >= 8) {
          do_kaioken(g, ch, "10", 0, 0);
        } else {
          do_kaioken(g, ch, "5", 0, 0);
        }
      } else {
        switch (power) {
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
            if (char_get_equipment(ch, WEAR_WIELD1))
              do_attack(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 5) == 5)
              do_kick(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 10) == 10)
              do_elbow(g, ch, buf, 0, 0);
            else
              do_punch(g, ch, buf, 0, 0);
            break;
          case 6:
          case 7:
          case 8:
            if (char_get_equipment(ch, WEAR_WIELD1))
              do_attack(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 5) == 5)
              do_punch(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 10) == 10)
              do_knee(g, ch, buf, 0, 0);
            else
              do_kick(g, ch, buf, 0, 0);
            break;
          case 9:
          case 10:
            if (rand_number(g, 1, 5) == 5)
              do_knee(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 10) == 10)
              do_uppercut(g, ch, buf, 0, 0);
            else
              do_elbow(g, ch, buf, 0, 0);
            break;
          case 11:
          case 12:
            if (rand_number(g, 1, 5) == 5)
              do_elbow(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 10) == 10)
              do_roundhouse(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 8) == 8)
              do_trip(g, ch, buf, 0, 0);
            else
              do_knee(g, ch, buf, 0, 0);
            break;
          case 13:
          case 14:
            if ((char_is_bardock(ch) || char_is_kurzak(ch)) && rand_number(g, 1, 2) == 2)
              do_head(g, ch, buf, 0, 0);
            else if ((char_is_icer(ch) || char_is_bio(ch)) && rand_number(g, 1, 2) == 2)
              do_tailwhip(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 8) == 8)
              do_trip(g, ch, buf, 0, 0);
            else
              do_uppercut(g, ch, buf, 0, 0);
            break;
          case 15:
          case 16:
            if ((char_is_bardock(ch) || char_is_kurzak(ch)) && rand_number(g, 1, 2) == 2)
              do_head(g, ch, buf, 0, 0);
            else if ((char_is_icer(ch) || char_is_bio(ch)) && rand_number(g, 1, 2) == 2)
              do_tailwhip(g, ch, buf, 0, 0);
            else if (rand_number(g, 1, 8) >= 7)
              do_trip(g, ch, buf, 0, 0);
            else
              do_roundhouse(g, ch, buf, 0, 0);
            break;
          case 17:
          case 18:
            do_slam(g, ch, buf, 0, 0);
            break;
          case 19:
          case 20:
            do_heeldrop(g, ch, buf, 0, 0);
            break;
        }
      }
    } else {
      g->mob_specials_used += 1;
      switch (power) {
        case 1:
        case 2:
        case 3:
        case 4:
          if (special > 80)
            do_zanzoken(g, ch, buf, 0, 0);
          if (char_get_mobcharge(ch) == 7) {
            char_set_mobcharge(ch, 0);
            do_kiball(g, ch, buf, 0, 0);
          }
          break;
        case 5:
        case 6:
        case 7:
        case 8:
          if (special > 80)
            do_zanzoken(g, ch, buf, 0, 0);
          if (char_get_mobcharge(ch) == 7) {
            char_set_mobcharge(ch, 0);
            do_kiblast(g, ch, buf, 0, 0);
          }
          break;
        case 9:
        case 10:
        case 11:
          if (special > 80)
            do_zanzoken(g, ch, buf, 0, 0);
          if (char_is_dragon(ch) && rand_number(g, 1, 4) == 4) {
            do_breath(g, ch, buf, 0, 0);
          } else {
            if (char_get_mobcharge(ch) == 7) {
              char_set_mobcharge(ch, 0);
              do_beam(g, ch, buf, 0, 0);
            }
          }
          break;
        case 12:
        case 13:
        case 14:
          if (special > 80)
            do_zanzoken(g, ch, buf, 0, 0);
          if (char_is_dragon(ch) && rand_number(g, 1, 4) == 4) {
            do_breath(g, ch, buf, 0, 0);
          } else {
            if (char_get_mobcharge(ch) == 7) {
              char_set_mobcharge(ch, 0);
              do_renzo(g, ch, buf, 0, 0);
            }
          }
          break;
        case 15:
        case 16:
          if (char_is_dragon(ch) && rand_number(g, 1, 4) == 4) {
            do_breath(g, ch, buf, 0, 0);
          } else {
            if (char_get_mobcharge(ch) == 7) {
              char_set_mobcharge(ch, 0);
              do_tsuihidan(g, ch, buf, 0, 0);
            }
          }
          break;
        case 17:
        case 18:
          if (char_is_dragon(ch) && rand_number(g, 1, 4) == 4) {
            do_breath(g, ch, buf, 0, 0);
          } else {
            if (char_get_mobcharge(ch) == 7) {
              char_set_mobcharge(ch, 0);
              do_shogekiha(g, ch, buf, 0, 0);
            }
          }
          break;
        case 19:
        case 20:
          if (char_is_dragon(ch)) {
            do_breath(g, ch, buf, 0, 0);
          }
          if (char_get_mobcharge(ch) == 7) {
            char_set_mobcharge(ch, 0);
            switch (char_get_class(ch)) {
              case CLASS_ROSHI:
                if (special >= 100)
                  do_kakusanha(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_kienzan(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_kamehameha(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_FRIEZA:
                if (special >= 100)
                  do_deathball(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_kienzan(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_deathbeam(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_KRANE:
                if (special >= 100)
                  do_tribeam(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_hass(g, ch, "", 0, 0);
                else if (special >= 70)
                  do_dodonpa(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_PICCOLO:
                if (special >= 100)
                  do_scatter(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_sbc(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_masenko(g, ch, buf, 0, 0);
                else if (special >= 60)
                  do_balefire(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_BARDOCK:
                if (special >= 100)
                  do_final(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_bigbang(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_galikgun(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_ANDSIX:
                if (special >= 100)
                  do_hellflash(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_kousengan(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_dualbeam(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_NAIL:
                if (special >= 100)
                  do_regenerate(g, ch, "50", 0, 0);
                else if (special >= 80)
                  do_heal(g, ch, "self", 0, 0);
                else if (special >= 70)
                  do_masenko(g, ch, buf, 0, 0);
                else
                  do_zanzoken(g, ch, "", 0, 0);
                break;
              case CLASS_KURZAK:
                if (special >= 100)
                  do_ensnare(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_seishou(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_renzo(g, ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(g, ch, "40", 0, 0);
                else
                  do_barrier(g, ch, "25", 0, 0);
                break;
              case CLASS_JINTO:
                if (special >= 100)
                  do_nova(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_breaker(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_trip(g, ch, buf, 0, 0);
                else
                  do_zanzoken(g, ch, "40", 0, 0);
                break;
              case CLASS_TSUNA:
                if (special >= 100)
                  do_koteiru(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_razor(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_spike(g, ch, buf, 0, 0);
                else
                  do_barrier(g, ch, "20", 0, 0);
                break;
              case CLASS_TAPION:
                if (special >= 100)
                  do_pslash(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_ddslash(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_tslash(g, ch, buf, 0, 0);
                else
                  do_zanzoken(g, ch, "40", 0, 0);
                break;
              case CLASS_KABITO:
                if (special >= 100)
                  do_pbarrage(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_psyblast(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_heal(g, ch, buf, 0, 0);
                else
                  do_zanzoken(g, ch, "40", 0, 0);
                break;
              case CLASS_DABURA:
                if (special >= 100)
                  do_hellspear(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_honoo(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_fireshield(g, ch, buf, 0, 0);
                else
                  do_zanzoken(g, ch, "40", 0, 0);
                break;
              case CLASS_GINYU:
                if (special >= 100)
                  do_spiral(g, ch, buf, 0, 0);
                else if (special >= 80)
                  do_crusher(g, ch, buf, 0, 0);
                else if (special >= 70)
                  do_eraser(g, ch, buf, 0, 0);
                else
                  do_zanzoken(g, ch, "40", 0, 0);
                break;
            }
          }
          break;
      } /* End power switch */
    } /* End special attacks */
  } else if (!char_is_humanoid(ch) || dragonpass == false) { /* Is not a humanoid mob */
    if (char_is_serpent(ch) && rand_number(g, 1, 5) == 5) {
      do_strike(g, ch, buf, 0, 0);
    } else if (char_is_dragon(ch) && rand_number(g, 1, 12) >= 10 && char_get_mob_vnum(g, ch) != 17917) {
      do_breath(g, ch, buf, 0, 0);
    } else {
      if (rand_number(g, 1, 10) >= 7 && char_get_level(ch) >= 10) {
        do_ram(g, ch, buf, 0, 0);
      } else {
        do_bite(g, ch, buf, 0, 0);
      }
    }
  }

  fight_mtrigger(g, ch);

} /* End mob_attack */

static void cleanup_arena_watch(GameWorld *g, struct char_data *ch)
{

  for (descriptor_data *d : each_desc(g)) {

    if (desc_get_connected(d) != CON_PLAYING)
      continue;

    if (char_prf_flagged(g, d->character, PRF_ARENAWATCH)) {
      if (char_get_arenawatch(d->character) == char_get_idnum(ch)) {
        char_prf_set(g, d->character, PRF_ARENAWATCH, false);
        char_set_arenawatch(d->character, -1);
      }
    }
  }
}

static void shadow_dragons_live(GameWorld *g)
{
  int value = 0;
  if (g->SHADOW_DRAGON1 != -1 || g->SHADOW_DRAGON2 != -1 || g->SHADOW_DRAGON3 != -1 || g->SHADOW_DRAGON4 != -1 || g->SHADOW_DRAGON5 != -1 || g->SHADOW_DRAGON6 != -1 || g->SHADOW_DRAGON7 != -1) {
    value = 1;
  }

  if (value == 0) {
    g->SELFISHMETER = 0;
    save_mud_time(g, &g->time_info);
  }
}

/* For announcing the sounds of battle to nearby rooms */
void impact_sound(GameWorld *g, struct char_data *ch, const char *mssg)
{
  int door;
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (char_can_go(g, ch, door))
      send_to_room(g, room_get_dir_option(g, char_get_in_room(ch), door)->to_room, "%s", mssg);
}

/* For removing body parts */
void remove_limb(GameWorld *g, struct char_data *vict, int num)
{
  /* 0 = head, 1 = rarm, 2 = larm, 3 = rleg, 4 = lleg , 5 = tail, 6 = tail*/

  struct obj_data *body_part;
  char part[1000];
  char buf[1000];
  char buf2[1000];

  body_part              = create_obj(g);
  obj_set_item_number(body_part, NOTHING);
  obj_set_in_room(body_part, NOWHERE);

  switch (num) {
    case 0:
      snprintf(part, sizeof(part), "@C%s@w's bloody head@n", char_get_name(vict));
      snprintf(buf, sizeof(buf), "%s bloody head", char_get_name(vict));
      break;
    case 1:
      snprintf(part, sizeof(part), "@w%s right arm@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "right arm");
      if (char_plr_flagged(vict, PLR_CRARM)) {
        char_plr_set(vict, PLR_CRARM, false);
      }
      break;
    case 2:
      snprintf(part, sizeof(part), "@w%s left arm@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "left arm");
      if (char_plr_flagged(vict, PLR_CLARM)) {
        char_plr_set(vict, PLR_CLARM, false);
      }
      break;
    case 3:
      snprintf(part, sizeof(part), "@w%s right leg@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "right leg");
      if (char_plr_flagged(vict, PLR_CRLEG)) {
        char_plr_set(vict, PLR_CRLEG, false);
      }
      break;
    case 4:
      snprintf(part, sizeof(part), "@w%s left leg@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "left leg");
      if (char_plr_flagged(vict, PLR_CLLEG)) {
        char_plr_set(vict, PLR_CLLEG, false);
      }
      break;
    case 5:
      snprintf(part, sizeof(part), "@wA %s tail@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "tail");
      break;
    case 6:
      snprintf(part, sizeof(part), "@wA %s tail@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "tail");
      break;
    default:
      snprintf(part, sizeof(part), "@w%s body part@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "body part");
      break;
  }

  obj_set_name(body_part, buf);
  if (num > 0) {
    snprintf(buf2, sizeof(buf2), "@wA %s is lying here@n", part);
  } else {
    snprintf(buf2, sizeof(buf2), "%s@w is lying here@n", part);
  }
  obj_set_description(body_part, buf2);
  obj_set_short_description(body_part, part);

  obj_set_type_flag(body_part, ITEM_OTHER);
  obj_wear_set(body_part, ITEM_WEAR_TAKE, true);
  obj_extra_set(body_part, ITEM_UNIQUE_SAVE, true);
  obj_set_value(body_part, 0, 0);
  obj_set_value(body_part, 1, 0);
  obj_set_value(body_part, 2, 0);
  obj_set_value(body_part, 3, 0);
  obj_set_value(body_part, 4, 1);
  obj_set_value(body_part, 5, 1);
  obj_set_weight(body_part, rand_number(g, 4, 10));
  obj_set_cost_per_day(body_part, 0);
  add_unique_id(g, body_part);
  obj_to_room(g, body_part, char_get_in_room(vict));
}

/* Weapon attack texts */
const struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES] =
    {
        {"hit",      "hits"     }, /* 0 */
        {"sting",    "stings"   },
        {"whip",     "whips"    },
        {"slash",    "slashes"  },
        {"bite",     "bites"    },
        {"bludgeon", "bludgeons"}, /* 5 */
        {"crush",    "crushes"  },
        {"pound",    "pounds"   },
        {"claw",     "claws"    },
        {"maul",     "mauls"    },
        {"thrash",   "thrashes" }, /* 10 */
        {"pierce",   "pierces"  },
        {"blast",    "blasts"   },
        {"punch",    "punches"  },
        {"stab",     "stabs"    }
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */
void fight_stack(GameWorld *g)
{
  int perc = 0;
  struct char_data *ch;
  struct char_data *wch;

  for (char_data *tch : each_char(g)) {
    ch = tch;

    if (char_get_position(ch) == POS_FIGHTING) {
      char_set_position(ch, POS_STANDING);
    }
    if (char_plr_flagged(ch, PLR_SPIRAL)) {
      handle_spiral(g, ch, nullptr, char_get_skill(ch, SKILL_SPIRAL), false);
    }
    if (char_is_npc(ch) && char_get_cooldown(ch) > 0) {
      char_set_cooldown(ch, char_get_cooldown(ch) - (1));
      if (rand_number(g, 1, 2) == 2 && char_get_cooldown(ch) > 0) {
        char_set_cooldown(ch, char_get_cooldown(ch) - (1));
      }
      if (char_get_cooldown(ch) > 0) {
        continue;
      }
    }
    if (char_is_npc(ch) && char_mob_flagged(ch, MOB_POWERUP) && axion_dice(g, 0) >= 90) {
      if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
        act(g, "@g$n@ finishes powering up as $s aura flashes brightly filling the entire area briefly with its light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        restoreHealth(ch, false);
        char_mob_set(ch, MOB_POWERUP, false);
      } else if (getCurPL(g, ch) >= getEffMaxPL(g, ch) / 2) {
        act(g, "@g$n@G continues powering up as torrents of energy crackle within $s aura.@n", true, ch, nullptr, nullptr, TO_ROOM);
        incCurHealthPercent(g, ch, .1, true);
      } else if (getCurPL(g, ch) > getEffMaxPL(g, ch) / 4) {
        act(g, "@g$n@G powers up as a steady aura around $s body grow brighter.@n", true, ch, nullptr, nullptr, TO_ROOM);
        incCurHealthPercent(g, ch, .125, true);
      } else if (getCurPL(g, ch) > 0) {
        act(g, "@g$n@G powers up, as a weak aura flickers around $s body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        incCurHealthPercent(g, ch, .2, true);
      }
    }
    if (char_is_npc(ch) && char_is_affected(ch, AFF_FROZEN)) {
      continue;
    }
    if (!char_get_grappling(g, ch) && !char_get_grappled(g, ch) && !char_get_fighting(g, ch) && !char_plr_flagged(ch, PLR_CHARGE) && !char_plr_flagged(ch, PLR_POWERUP) && char_get_charge(ch) <= 0 && !char_is_transformed(ch)) {
      continue;
    }
    if (char_get_fighting(g, ch) && (char_get_in_room(char_get_fighting(g, ch)) != char_get_in_room(ch))) {
      wch = char_get_fighting(g, ch);
      stop_fighting(g, wch);
      stop_fighting(g, ch);
    }
    if (char_get_fighting(g, ch) && char_get_drag(g, ch)) {
      act(g, "@WYou are forced to stop dragging @C$N@W!@n", true, ch, nullptr, char_get_drag(g, ch), TO_CHAR);
      act(g, "@C$n@W is forced to stop dragging @c$N@W!@n", true, ch, nullptr, char_get_drag(g, ch), TO_ROOM);
      char_set_dragged(char_get_drag(g, ch), nullptr);
      char_set_drag(ch, nullptr);
    }

    if (char_get_lifeperc(ch) > 0 && char_get_health(ch) < (double)char_get_lifeperc(ch) / 100 && (getCurLF(g, ch)) > 0 && !char_is_android(ch)) {
      if (rand_number(g, 1, 15) >= 14) {
        if ((getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.05 || char_aff_flagged(ch, AFF_HEALGLOW) || (char_is_kanassan(ch) && (getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.03)) {
          int64_t refill = 0, lfcost = (getMaxLF(g, ch)) * 0.05;
          if (char_get_bonuses(ch, BONUS_DIEHARD) > 0 && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 2 && char_get_genome(ch, 1) != 2))) {
            refill = (getMaxLF(g, ch)) * 0.1;
          } else if (char_get_bonuses(ch, BONUS_DIEHARD) > 0 && char_is_mutant(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2)) {
            refill = (getMaxLF(g, ch)) * 0.17;
          } else if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2)) {
            refill = (getMaxLF(g, ch)) * 0.12;
          } else if (char_is_kanassan(ch)) {
            lfcost = (getMaxLF(g, ch)) * 0.03;
            refill = (getMaxLF(g, ch)) * 0.03;
          } else {
            refill = (getMaxLF(g, ch)) * 0.05;
          }
          incCurHealth(g, ch, refill, true);
          if (!char_aff_flagged(ch, AFF_HEALGLOW)) {
            decCurLF(g, ch, lfcost, 0);
          }
        } else {
          incCurHealth(g, ch, (getCurLF(g, ch)), true);
          decCurLFPercent(g, ch, 2, -1);
        }

        send_to_char(g, ch, "@YYour life force has kept you strong@n!\r\n");
      }
    }

    if (!char_aff_flagged(ch, AFF_POSITION)) {
      if (roll_balance(g, ch) > axion_dice(g, 0) && rand_number(g, 1, 10) >= 7) {
        if (char_get_fighting(g, ch)) {
          if (!char_aff_flagged(char_get_fighting(g, ch), AFF_POSITION)) {
            act(g, "@YYou manage to move into an advantageous position!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "@y$n@Y manages to move into an advantageous position!@n", true, ch, nullptr, nullptr, TO_ROOM);
            char_aff_set(ch, AFF_POSITION, true);
          } else {
            struct char_data *vict = char_get_fighting(g, ch);
            if (roll_balance(g, ch) > roll_balance(g, vict)) {
              act(g, "@YYou struggle to gain a better position than @y$N@Y and succeed!@n", true, ch, nullptr, vict, TO_CHAR);
              act(g, "@y$n@Y struggles to gain a better position than you and succeeds!@n", true, ch, nullptr, vict, TO_VICT);
              act(g, "@y$n@Y struggles to gain a better position than @y$N@Y and succeeds!@n", true, ch, nullptr, vict, TO_NOTVICT);
              char_aff_set(vict, AFF_POSITION, false);
              char_aff_set(ch, AFF_POSITION, true);
            }
          }
        }
      }
    } else {
      if (roll_balance(g, ch) < axion_dice(g, -30) || char_get_position(ch) < POS_STANDING) {
        act(g, "@YYou are moved out of your position!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@y$n@Y is moved out of $s position!@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_aff_set(ch, AFF_POSITION, false);
      }
    }
    if (char_get_grappling(g, ch) && char_get_grap(ch) == 2 && rand_number(g, 1, 11) >= 8) {
      if ((getCurST(g, char_get_grappling(g, ch))) >= getMaxST(g, char_get_grappling(g, ch)) / 8) {
        act(g, "@WYou choke @C$N@W!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_CHAR);
        act(g, "@C$n@W chokes YOU@W!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_VICT);
        act(g, "@C$n@W chokes @c$N@W!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_NOTVICT);
        decCurST(g, char_get_grappling(g, ch), (getMaxST(g, char_get_grappling(g, ch)) / 8), 0);
      } else {
        act(g, "@WYou choke @C$N@W, and $E passes out!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_CHAR);
        act(g, "@C$n@W chokes YOU@W, and you pass out!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_VICT);
        act(g, "@C$n@W chokes @c$N@W, and $E passes out!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_NOTVICT);
        char_aff_set(char_get_grappling(g, ch), AFF_KNOCKED, true);
        char_set_position(char_get_grappling(g, ch), POS_SLEEPING);
        char_set_grap(char_get_grappling(g, ch), -1);
        char_set_grappled(char_get_grappling(g, ch), nullptr);
        char_set_grappling(ch, nullptr);
        char_set_grap(ch, -1);
      }
    } else if (char_get_grappling(g, ch) && char_get_grap(ch) == 4 && rand_number(g, 1, 12) >= 8) {
      act(g, "@WYou crush @C$N@W some more!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_CHAR);
      act(g, "@C$n@W crushes YOU@W some more!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_VICT);
      act(g, "@C$n@W crushes @c$N@W some more!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_NOTVICT);
      int64_t damg = char_get_aff_abils_str(ch) * (10 + (getEffMaxPL(g, ch) * 0.005));
      hurt(g, 0, 0, ch, char_get_grappling(g, ch), nullptr, damg, 0);
    }
    if (char_get_grappled(g, ch) && rand_number(g, 1, 2) == 2) {
      send_to_char(g, ch, "@CTry 'escape' to break free from the hold!@n\r\n");
    }
    if (char_is_halfbreed(ch) && char_plr_flagged(ch, PLR_FURY)) {
      char_set_rage_meter(ch, char_get_rage_meter(ch) + (1));
      if (char_get_rage_meter(ch) >= 1000) {

        incCurHealthPercent(g, ch, .15, true);
        incCurKIPercent(g, ch, .15, true);
        incCurSTPercent(g, ch, .15, true);
        send_to_char(g, ch, "Your fury has called forth more of your hidden power and you feel better!\r\n");
      }
    }

    if (!char_is_npc(ch) && char_is_transformed(ch) && !char_is_icer(ch) && char_is_nonptrans(ch)) {
      int tier = race_current_trans_tier(ch);

      if (getCurST(g, ch) < getMaxST(g, ch) / 60) {
        if (!(tier == 1 && char_plr_flagged(ch, PLR_FPSSJ))) {
          act(g, "@mExhausted of stamina, your body forcibly reverts from its form.@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@C$n @wbreathing heavily, reverts from $s form, returning to normal.@n", true, ch, nullptr, nullptr, TO_ROOM);
          if (char_get_kaioken(ch) < 1)
            do_kaioken(g, ch, "0", 0, 0);
          do_transform(g, ch, "revert", 0, 0);
        }
      }

      if (getCurST(g, ch) >= getMaxST(g, ch) / 800 && char_plr_flagged(ch, PLR_TRANS1)) {
        if (!char_plr_flagged(ch, PLR_FPSSJ)) {
          if (char_is_saiyan(ch) && (getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.7) {
            decCurST(g, ch, getMaxST(g, ch) / 900, 0);
          } else
            decCurST(g, ch, getMaxST(g, ch) / 800, 0);
        }
      } else if (getCurST(g, ch) >= getMaxST(g, ch) / 600 && char_plr_flagged(ch, PLR_TRANS2) && !char_is_konatsu(ch) && !char_is_kai(ch) && !char_is_namek(ch)) {
        if (char_is_saiyan(ch) && (getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.7) {
          decCurST(g, ch, getMaxST(g, ch) / 700, 0);
        } else
          decCurST(g, ch, getMaxST(g, ch) / 600, 0);
      } else if (getCurST(g, ch) >= getMaxST(g, ch) / 500 && char_plr_flagged(ch, PLR_TRANS2)) {
        decCurST(g, ch, getMaxST(g, ch) / 500, 0);
      } else if (getCurST(g, ch) >= getMaxST(g, ch) / 400 && char_plr_flagged(ch, PLR_TRANS3) && !char_is_saiyan(ch)) {
        decCurST(g, ch, getMaxST(g, ch) / 400, 0);
      } else if (getCurST(g, ch) >= getMaxST(g, ch) / 250 && char_plr_flagged(ch, PLR_TRANS3)) {
        if (char_is_saiyan(ch) && (getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.7) {
          decCurST(g, ch, getMaxST(g, ch) / 300, 0);
        } else
          decCurST(g, ch, getMaxST(g, ch) / 250, 0);
      } else if (getCurST(g, ch) >= getMaxST(g, ch) / 200 && char_plr_flagged(ch, PLR_TRANS4) && !char_is_saiyan(ch)) {
        decCurST(g, ch, getMaxST(g, ch) / 200, 0);
      } else if (getCurST(g, ch) >= getMaxST(g, ch) / 170 && char_plr_flagged(ch, PLR_TRANS4)) {
        if (char_is_saiyan(ch) && (getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.7) {
          decCurST(g, ch, getMaxST(g, ch) / 240, 0);
        } else
          decCurST(g, ch, getMaxST(g, ch) / 170, 0);
      }
    }

    if (!char_is_npc(ch) && char_get_wimp_level(g, ch) && getCurPL(g, ch) < char_get_wimp_level(g, ch) && getCurPL(g, ch) > 0 && char_get_fighting(g, ch)) {
      send_to_char(g, ch, "You wimp out, and attempt to flee!\r\n");
      do_flee(g, ch, "", 0, 0);
    }
    if (char_is_npc(ch) && getCurPL(g, ch) < getEffMaxPL(g, ch) / 10 && getCurPL(g, ch) > 0 && char_get_fighting(g, ch) && !char_mob_flagged(ch, MOB_SENTINEL)) {
      if (rand_number(g, 1, 30) >= 25 && char_get_position(ch) > POS_SITTING) {
        do_flee(g, ch, "", 0, 0);
      }
    }
    if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 6 || char_get_genome(ch, 1) == 6) && rand_number(g, 1, 200) >= 175) {
      mutant_limb_regen(g, ch);
    }
    if (!char_is_npc(ch) && char_plr_flagged(ch, PLR_DISGUISED) && char_get_fighting(g, ch)) {
      if (char_get_skill(ch, SKILL_DISGUISE) < rand_number(g, 1, 125)) {
        send_to_char(g, ch, "Your disguise comes off because of your swift movements!\r\n");
        char_plr_set(ch, PLR_DISGUISED, false);
        act(g, "@W$n's@W disguise comes off because of $s swift movements!@n", false, ch, nullptr, nullptr, TO_ROOM);
      }
    }
    if (char_is_npc(ch) && char_aff_flagged(ch, AFF_BLIND) && rand_number(g, 1, 200) >= 190) {
      act(g, "@W$n@W is no longer blind.@n", false, ch, nullptr, nullptr, TO_ROOM);
      char_aff_set(ch, AFF_BLIND, false);
    }

    if (char_aff_flagged(ch, AFF_KNOCKED) && rand_number(g, 1, 200) >= 195) {
      cureStatusKnockedOut(g, ch, true);
      if (char_is_npc(ch) && rand_number(g, 1, 20) >= 12) {
        act(g, "@W$n@W stands up.@n", false, ch, nullptr, nullptr, TO_ROOM);
        char_set_position(ch, POS_STANDING);
      }
    }

    if (!char_is_npc(ch) && !(char_get_desc(ch)) && char_get_position(ch) > POS_STUNNED && !char_is_affected(ch, AFF_FROZEN)) {
      if (char_get_fighting(g, ch)) {
        do_flee(g, ch, "", 0, 0);
      }
    }
    /* Mobile Defense System */
    if (char_is_npc(ch) && char_get_grappled(g, ch) && !char_mob_flagged(ch, MOB_DUMMY) && rand_number(g, 1, 5) >= 4) {
      do_escape(g, ch, "", 0, 0);
      continue;
    }
    if (char_get_fighting(g, ch) && char_is_npc(ch) && !char_mob_flagged(ch, MOB_DUMMY)) {
      if (char_aff_flagged(char_get_fighting(g, ch), AFF_FLYING) && !char_aff_flagged(ch, AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) > 10) {
        do_fly(g, ch, "", 0, 0);
        continue;
      }
      if (!char_aff_flagged(char_get_fighting(g, ch), AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING)) {
        do_fly(g, ch, "", 0, 0);
        continue;
      }
      if (char_aff_flagged(char_get_fighting(g, ch), AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING) && char_get_altitude(ch) < char_get_altitude(char_get_fighting(g, ch))) {
        do_fly(g, ch, "high", 0, 0);
        continue;
      }
      if (char_aff_flagged(char_get_fighting(g, ch), AFF_FLYING) && !char_is_humanoid(ch) && !char_aff_flagged(ch, AFF_FLYING) && char_get_position(ch) > POS_RESTING) {
        if (rand_number(g, 1, 30) >= 22 && !block_calc(g, ch)) {
          act(g, "$n@G flees in terror and you lose sight of $m!", true, ch, nullptr, nullptr, TO_ROOM);
          while (char_get_inventory(ch))
            extract_obj(g, first_obj(g, char_get_inventory(ch)));

          extract_char(g, ch);
          continue;
        }
      }
      if (char_aff_flagged(char_get_fighting(g, ch), AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) <= 10) {
        if (rand_number(g, 1, 30) >= 22 && !block_calc(g, ch)) {
          act(g, "$n@G turns and runs away. You lose sight of $m!", true, ch, nullptr, nullptr, TO_ROOM);
          while (char_get_inventory(ch))
            extract_obj(g, first_obj(g, char_get_inventory(ch)));
          extract_char(g, ch);
          continue;
        }
      }
      if (char_get_position(ch) == POS_SITTING && sec_roll_check(g, ch) == 1) {
        do_stand(g, ch, "", 0, 0);
        continue;
      }
      if (char_get_position(ch) == POS_RESTING && sec_roll_check(g, ch) == 1) {
        do_stand(g, ch, "", 0, 0);
        continue;
      }
      if (char_is_affected(ch, AFF_PARA) && char_is_npc(ch) && char_get_aff_abils_intel(ch) + 10 < rand_number(g, 1, 60)) {
        act(g, "@yYou fail to overcome your paralysis!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@Y$n @ystruggles with $s paralysis!@n", true, ch, nullptr, nullptr, TO_ROOM);
        continue;
      }
      if (char_get_position(ch) == POS_SLEEPING && !char_aff_flagged(ch, AFF_KNOCKED) && sec_roll_check(g, ch) == 1) {
        do_wake(g, ch, "", 0, 0);
        do_stand(g, ch, "", 0, 0);
        continue;
      }
      struct char_data *vict;
      char buf[100];

      vict = char_get_fighting(g, ch);
      sprintf(buf, "%s", char_get_name(vict));
      if (char_get_in_room(ch) == char_get_in_room(vict) && !char_mob_flagged(ch, MOB_DUMMY) && !char_aff_flagged(ch, AFF_KNOCKED) && char_get_position(ch) != POS_SITTING && char_get_position(ch) != POS_RESTING && char_get_position(ch) != POS_SLEEPING) {

        if (char_is_npc(ch) && rand_number(g, 1, 30) <= 12)
          continue;

        mob_attack(g, ch, buf);

      } //end if
      else {
        continue;
      }
    }
    if (char_get_position(ch) <= POS_RESTING && char_plr_flagged(ch, PLR_POWERUP)) {
      char_plr_set(ch, PLR_POWERUP, false);
    }

    if (char_get_barrier(ch) > 0) {
      improve_skill(g, ch, SKILL_BARRIER, 0);
    }

    if (char_plr_flagged(ch, PLR_POWERUP) && rand_number(g, 1, 3) == 3) {
      char buf3[MAX_STRING_LENGTH];
      int64_t ghit    = getCurPL(g, ch);
      int64_t gmaxhit = getEffMaxPL(g, ch);
      int64_t gki     = getCurKI(g, ch);
      int64_t gmaxki  = getMaxKI(g, ch);

      if (ghit >= gmaxhit && gki >= (gmaxki / 20) && char_get_preference(ch) != PREFERENCE_KI) {
        if (gki >= gmaxki * 0.5) {
          int64_t raise = getMaxST(g, ch) * 0.02;
          incCurST(g, ch, raise, true);
        }
        restoreHealth(ch, false);
        decCurKI(g, ch, getMaxKI(g, ch) / 20, 0);
        dispel_ash(g, ch);
        act(g, "@RYou have reached your maximum!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@R$n stops powering up in a flash of light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        send_to_sense(g, 0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(g, ch)));
        send_to_scouter(g, buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      } else if (ghit >= gmaxhit && gki >= (gmaxki * 0.0375) + 1 && char_get_preference(ch) == PREFERENCE_KI) {
        if (gki >= (gmaxki * 0.0375) + 1) {
          int64_t raise = getMaxST(g, ch) * 0.02;
          incCurST(g, ch, raise, true);
        }
        restoreHealth(ch, false);
        decCurKI(g, ch, (gmaxki * 0.0375) + 1, 0);
        dispel_ash(g, ch);
        act(g, "@RYou have reached your maximum!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@R$n stops powering up in a flash of light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        send_to_sense(g, 0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(g, ch)));
        send_to_scouter(g, buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      }

      ghit    = getCurPL(g, ch);
      gmaxhit = getEffMaxPL(g, ch);
      gki     = getCurKI(g, ch);
      gmaxki  = getMaxKI(g, ch);

      if (gki < (gmaxki / 20) && char_get_preference(ch) != PREFERENCE_KI) {
        decCurKI(g, ch, gmaxki / 20, 0);
        act(g, "@RYou have run out of ki.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@R$n stops powering up in a flash of light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        send_to_sense(g, 0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(g, ch)));
        send_to_scouter(g, buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      } else if (gki < (gmaxki * 0.0375) + 1 && char_get_preference(ch) == PREFERENCE_KI) {
        decCurKI(g, ch, (gmaxki * 0.0375) + 1, 0);
        act(g, "@RYou have run out of ki.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@R$n stops powering up in a flash of light!@n", true, ch, nullptr, nullptr, TO_ROOM);
        send_to_sense(g, 0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(g, ch)));
        send_to_scouter(g, buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      }

      ghit    = getCurPL(g, ch);
      gmaxhit = getEffMaxPL(g, ch);
      gki     = getCurKI(g, ch);
      gmaxki  = getMaxKI(g, ch);

      if (ghit < gmaxhit && ((char_get_preference(ch) != PREFERENCE_KI &&
                              (gki) >= gmaxki / 20) ||
                             (char_get_preference(ch) == PREFERENCE_KI &&
                              (gki) >= (gmaxki * 0.0375) + 1))) {
        incCurHealthPercent(g, ch, .1, true);
        if (char_get_preference(ch) != PREFERENCE_KI) {
          decCurKI(g, ch, gmaxki / 20, 0);
        } else {
          decCurKI(g, ch, gmaxki * .0375, 0);
        }
        if ((getCurKI(g, ch)) >= getMaxKI(g, ch) * 0.5) {
          int64_t raise = getMaxST(g, ch) * 0.02;
          incCurST(g, ch, raise, true);
        }

        gmaxhit = getEffMaxPL(g, ch);

        if (gmaxhit < 50000) {
          act(g, "@RYou continue to powerup, as wind billows out from around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as wind billows out from around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (gmaxhit < 500000) {
          act(g, "@RYou continue to powerup, as the ground splits beneath you!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as the ground splits beneath $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (gmaxhit < 5000000) {
          act(g, "@RYou continue to powerup, as the ground shudders and splits beneath you!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as the ground shudders and splits beneath $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (gmaxhit < 50000000) {
          act(g, "@RYou continue to powerup, as a huge depression forms beneath you!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as a huge depression forms beneath $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (gmaxhit < 100000000) {
          act(g, "@RYou continue to powerup, as the entire area quakes around you!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as the entire area quakes around $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (gmaxhit < 300000000) {
          act(g, "@RYou continue to powerup, as huge chunks of ground are ripped apart beneath you!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as huge chunks of ground are ripped apart beanth $m!@n", true, ch, nullptr, nullptr, TO_ROOM);
        } else {
          act(g, "@RYou continue to powerup, as the very air around you crackles and burns!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n continues to powerup, as the very air around $m crackles and burns!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
        send_to_sense(g, 0, "You sense someone powering up", ch);
        send_to_worlds(g, ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Detected@D: [@Y%s@D]", add_commas(getCurPL(g, ch)));
        send_to_scouter(g, buf3, ch, 1, 0);
        dispel_ash(g, ch);
      }
    }
    if ((char_get_position(ch) == POS_SLEEPING || char_get_position(ch) == POS_RESTING) && (char_plr_flagged(ch, PLR_CHARGE) || char_get_charge(ch) >= 1)) {
      send_to_char(g, ch, "You stop charging and release all your pent up energy!\r\n");
      switch (rand_number(g, 1, 3)) {
        case 1:
          act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
        case 2:
          act(g, "$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
        case 3:
          act(g, "$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
        default:
          act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
      }
      char_plr_set(ch, PLR_CHARGE, false);
      incCurKI(g, ch, char_get_charge(ch), true);
      char_set_charge(ch, 0);
      char_set_chargeto(ch, 0);
    }
    if (char_plr_flagged(ch, PLR_CHARGE) && char_get_bonuses(ch, BONUS_UNFOCUSED) > 0 && rand_number(g, 1, 80) >= 70) {
      send_to_char(g, ch, "You lose concentration due to your unfocused mind and release your charged energy!\r\n");
      switch (rand_number(g, 1, 3)) {
        case 1:
          act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
        case 2:
          act(g, "$n@w's aura fades.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
        case 3:
          act(g, "$n@w's aura flickers brightly before disappearing.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
        default:
          act(g, "$n@w's aura disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
          break;
      }
      char_plr_set(ch, PLR_CHARGE, false);
      incCurKI(g, ch, char_get_charge(ch), true);
      char_set_charge(ch, 0);
      char_set_chargeto(ch, 0);
    }
    if (char_get_charge(ch) >= getMaxKI(g, ch) / 2) {
      improve_skill(g, ch, SKILL_CONCENTRATION, 1);
    }
    if (!char_plr_flagged(ch, PLR_CHARGE) && rand_number(g, 1, 40) >= 38 && !char_get_fighting(g, ch) && (char_get_preference(ch) != PREFERENCE_KI || char_get_charge(ch) > getMaxKI(g, ch) * 0.1)) {
      if (char_get_charge(ch) >= getMaxKI(g, ch) / 100) {
        int64_t loss = 0;
        send_to_char(g, ch, "You lose some of your energy slowly.\r\n");
        switch (rand_number(g, 1, 3)) {
          case 1:
            act(g, "$n@w's aura flickers weakly.@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 2:
            act(g, "$n@w's aura sheds energy.@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 3:
            act(g, "$n@w's aura flickers brightly before growing dimmer.@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          default:
            act(g, "$n@w's aura shrinks some.@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        }
        loss = char_get_charge(ch) / 20;
        char_set_charge(ch, char_get_charge(ch) - (loss));
      } else if (char_get_charge(ch) < getMaxKI(g, ch) / 100 && char_get_charge(ch) != 0) {
        send_to_char(g, ch, "Your charged energy is completely gone as your aura fades.\r\n");
        act(g, "$n@w's aura fades away dimmly.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_charge(ch, 0);
      }
    }
    if (char_plr_flagged(ch, PLR_CHARGE)) {
      if ((char_get_skill(ch, SKILL_CONCENTRATION) > 74)) {
        perc = 10;
      } else if ((char_get_skill(ch, SKILL_CONCENTRATION) > 49)) {
        perc = 5;
      } else if ((char_get_skill(ch, SKILL_CONCENTRATION) > 24)) {
        perc = 2;
      } else {
        perc = 1;
      }
      if (char_is_truffle(ch) && perc == 10) {
        perc += 10;
      }
      if (char_is_truffle(ch) && perc == 5) {
        perc += 5;
      }
      if (char_is_truffle(ch) && perc == 2) {
        perc += 3;
      }
      if (char_is_truffle(ch) && perc == 1) {
        perc += 1;
      }
      if (perc > 1 && char_get_preference(ch) == PREFERENCE_H2H) {
        perc = perc * 0.5;
      }
    }
    if (char_plr_flagged(ch, PLR_CHARGE)) {
      if ((char_get_skill(ch, SKILL_CONCENTRATION) > 74)) {
        perc = 10;
      } else if ((char_get_skill(ch, SKILL_CONCENTRATION) > 49)) {
        perc = 5;
      } else if ((char_get_skill(ch, SKILL_CONCENTRATION) > 24)) {
        perc = 2;
      } else {
        perc = 1;
      }
      if (char_is_mutant(ch) && perc == 10) {
        perc -= 1;
      }
      if (char_is_mutant(ch) && perc == 5) {
        perc -= 1;
      }
      if (char_is_mutant(ch) && perc == 2) {
        perc -= 1;
      }
      if (perc > 1 && char_get_preference(ch) == PREFERENCE_H2H) {
        perc = perc * 0.5;
      }
      if ((getCurKI(g, ch)) <= 0) {
        send_to_char(g, ch, "You can not charge anymore, you have charged all your energy!\r\n");
        act(g, "$n@w's aura grows calm.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_plr_set(ch, PLR_CHARGE, false);
      } else if (((getMaxKI(g, ch) * 0.01) * perc) >= (getCurKI(g, ch))) {
        send_to_char(g, ch, "You have charged the last that you can.\r\n");
        act(g, "$n@w's aura @Yflashes@w spectacularly, rushing upwards in torrents!@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_set_charge(ch, char_get_charge(ch) + ((getCurKI(g, ch))));
        decCurKIPercent(g, ch, 1, 0);
        char_set_chargeto(ch, 0);
        char_plr_set(ch, PLR_CHARGE, false);
      } else {
        if (char_get_charge(ch) >= char_get_chargeto(ch)) {
          send_to_char(g, ch, "You have already reached the maximum that you wished to charge.\r\n");
          act(g, "$n@w's aura burns steadily.@n", true, ch, nullptr, nullptr, TO_ROOM);
          char_set_chargeto(ch, 0);
          char_plr_set(ch, PLR_CHARGE, false);
        } else if (char_get_charge(ch) + (((getMaxKI(g, ch) * 0.01) * perc) + 1) >= char_get_chargeto(ch)) {
          decCurKI(g, ch, char_get_chargeto(ch) - char_get_charge(ch), 0);
          char_set_charge(ch, char_get_chargeto(ch));
          send_to_char(g, ch, "You stop charging as you reach the maximum that you wished to charge.\r\n");
          act(g, "$n@w's aura flares up brightly and then burns steadily.@n", true, ch, nullptr, nullptr, TO_ROOM);
          char_set_chargeto(ch, 0);
          char_plr_set(ch, PLR_CHARGE, false);
        } else {
          decCurKI(g, ch, ((getMaxKI(g, ch) * 0.01) * perc) + 1, 0);
          char_set_charge(ch, char_get_charge(ch) + (((getMaxKI(g, ch) * 0.01) * perc) + 1));
          switch (rand_number(g, 1, 3)) {
            case 1:
              act(g, "$n@w's aura ripples magnificantly while growing brighter!@n", true, ch, nullptr, nullptr, TO_ROOM);
              send_to_char(g, ch, "Your aura grows bright as you charge more ki.\r\n");
              break;
            case 2:
              act(g, "$n@w's aura ripples with power as it grows larger!@n", true, ch, nullptr, nullptr, TO_ROOM);
              send_to_char(g, ch, "Your aura ripples with power as you charge more ki.\r\n");
              break;
            case 3:
              act(g, "$n@w's aura throws sparks off violently!.@n", true, ch, nullptr, nullptr, TO_ROOM);
              send_to_char(g, ch, "Your aura throws sparks off violently as you charge more ki.\r\n");
              break;
            default:
              break;
          }
          if (char_get_charge(ch) >= char_get_chargeto(ch)) {
            char_set_charge(ch, char_get_chargeto(ch));
            char_set_charge(ch, char_get_charge(ch) + (char_get_level(ch)));
            send_to_char(g, ch, "You have finished charging!\r\n");
            act(g, "$n@w's aura burns brightly and then evens out.@n", true, ch, nullptr, nullptr, TO_ROOM);
            char_plr_set(ch, PLR_CHARGE, false);
            char_set_chargeto(ch, 0);
          }
        }
        if (char_get_skill(ch, SKILL_CONCENTRATION)) {
          improve_skill(g, ch, SKILL_CONCENTRATION, 1);
        }
      }
    }
  }
}

void appear(GameWorld *g, struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(g, ch, SPELL_INVISIBLE);

  if (char_aff_flagged(ch, AFF_INVISIBLE))
    char_aff_set(ch, AFF_INVISIBLE, false);

  if (char_aff_flagged(ch, AFF_HIDE))
    char_aff_set(ch, AFF_HIDE, false);

  act(g, "$n slowly fades into existence.", false, ch, nullptr, nullptr, TO_ROOM);
}

void update_pos(GameWorld *g, struct char_data *victim)
{
  if (char_aff_flagged(victim, AFF_KNOCKED)) {
    return;
  }
  if ((getCurPL(g, victim) > 0) && (char_get_position(victim) > POS_STUNNED))
    return;
  else if (char_get_position(victim) == POS_SITTING && char_get_fighting(g, victim))
    return;
  else if (char_get_position(victim) == POS_SITTING && char_get_fighting(g, victim))
    return;
  else if (getCurPL(g, victim) > 0)
    char_set_position(victim, POS_STANDING);
  else if (getCurPL(g, victim) <= -11)
    char_set_position(victim, POS_DEAD);
  else if (getCurPL(g, victim) <= -6)
    char_set_position(victim, POS_MORTALLYW);
  else if (getCurPL(g, victim) <= -3)
    char_set_position(victim, POS_INCAP);
  else
    char_set_position(victim, POS_STUNNED);
}

static void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (char_plr_flagged(vict, PLR_KILLER) || char_plr_flagged(vict, PLR_THIEF))
    return;
  if (char_plr_flagged(ch, PLR_KILLER) || char_is_npc(ch) || char_is_npc(vict) || ch == vict)
    return;
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (char_get_fighting(g, ch)) /* they carry on with whoever they were already fighting */
    return;

  g->combat_list.add(char_get_id(ch), ch);

  char_set_fighting(ch, vict);

  if (char_get_position(ch) == POS_SITTING) {
    char_set_position(ch, POS_SITTING);
  } else if (char_get_position(ch) == POS_SLEEPING) {
    char_set_position(ch, POS_SLEEPING);
  }

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(GameWorld *g, struct char_data *ch)
{

  if (char_is_npc(ch)) {
    char_set_combo(ch, -1);
    char_set_combhits(ch, 0);
  }
  char_unlink_from_combat_list(g, ch);
  char_set_fighting(ch, nullptr);
  if (char_aff_flagged(ch, AFF_POSITION)) {
    char_aff_set(ch, AFF_POSITION, false);
  }
  update_pos(g, ch);
}

static void make_pcorpse(GameWorld *g, struct char_data *ch)
{

  struct obj_data *corpse;
  struct obj_data *money;
  int x, y;

  corpse = create_obj(g);

  obj_set_item_number(corpse, NOTHING);
  obj_set_in_room(corpse, NOWHERE);

  /* This handles how the corpse is viewed - Iovan */
  handle_corpse_condition(corpse, ch);

  if (char_aff_flagged(ch, AFF_ASHED)) {
    act(g, "@WSome ashes fall off the corpse.@n", true, ch, nullptr, nullptr, TO_ROOM);
    struct obj_data *ashes;
    if (rand_number(g, 1, 3) == 2) {
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
    } else if (rand_number(g, 1, 2) == 2) {
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
    } else {
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
    }
  }

  obj_set_type_flag(corpse, ITEM_CONTAINER);
  obj_set_size(corpse, get_size(ch));
  for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      obj_get_extra_flags(corpse)[x] = 0;
    if (y < TW_ARRAY_MAX)
      obj_get_wear_flags(corpse)[y] = 0;
  }

  obj_wear_set(corpse, ITEM_WEAR_TAKE, true);
  obj_extra_set(corpse, ITEM_NODONATE, true);
  obj_set_value(corpse, VAL_CONTAINER_CAPACITY, 0);                  /* You can't store stuff in a corpse */
  obj_set_value(corpse, VAL_CONTAINER_CORPSE, 1);                    /* corpse identifier */
  obj_set_value(corpse, VAL_CONTAINER_OWNER, char_get_pfilepos(ch)); /* corpse identifier */
  obj_set_weight(corpse, char_get_pc_weight(g, ch) + char_get_carry_weight(ch));
  obj_set_cost_per_day(corpse, 100000);
  obj_set_timer(corpse, CONFIG_MAX_PC_CORPSE_TIME);
  obj_extra_set(corpse, ITEM_UNIQUE_SAVE, true);

  for (obj_data *obj : char_inventory(g, ch)) {

    if (obj && obj_get_vnum(g, obj) < 19900 && obj_get_vnum(g, obj) != 17998) {
      if ((obj_get_vnum(g, obj) >= 18800 && obj_get_vnum(g, obj) <= 18999) || (obj_get_vnum(g, obj) >= 19100 && obj_get_vnum(g, obj) <= 19199)) {
        continue;
      }
      obj_from_char(g, obj);
      obj_to_obj(g, obj, corpse);
      continue;
    }
    continue;
  }

  /* transfer gold */
  if (char_get_gold(ch) > 0) {
    /*
     * following 'if' clause added to fix gold duplication loophole
     * The above line apparently refers to the old "partially log in,
     * kill the game character, then finish login sequence" duping
     * bug. The duplication has been fixed (knock on wood) but the
     * test below shall live on, for a while. -gg 3/3/2002
     */
    if (char_is_npc(ch) || char_get_desc(ch)) {
      money = create_money(g, char_get_gold(ch));
      obj_to_obj(g, money, corpse);
    }
    char_set_gold(ch, 0);
  }

  obj_to_room(g, corpse, char_get_in_room(ch));
}

/* This handles how corpses are viewed. How many limbs they have. If they were *
 * disintergrated, blown in half, beat to a pulp, etc.        - Iovan 3/2/2011 */
static void handle_corpse_condition(struct obj_data *corpse, struct char_data *ch)
{

  char buf2[MAX_NAME_LENGTH + 128];
  char descBuf[512];

  obj_set_value(corpse, VAL_CORPSE_HEAD, 1);
  obj_set_value(corpse, VAL_CORPSE_RARM, 1);
  obj_set_value(corpse, VAL_CORPSE_LARM, 1);
  obj_set_value(corpse, VAL_CORPSE_RLEG, 1);
  obj_set_value(corpse, VAL_CORPSE_LLEG, 1);

  switch (char_get_death_type(ch)) {
    case DTYPE_HEAD:
      *buf2        = '\0';
      obj_set_name(corpse, str_format("headless corpse %s", char_get_name(ch)));

      *descBuf            = '\0';
      obj_set_description(corpse, str_format("The headless corpse of %s is lying here", char_get_name(ch)));

      *descBuf                  = '\0';
      obj_set_short_description(corpse, str_format("The headless remains of %s's corpse", char_get_name(ch)));
      obj_set_value(corpse, VAL_CORPSE_HEAD, 0);
      break;
    case DTYPE_HALF:
      *buf2        = '\0';
      obj_set_name(corpse, str_format("half corpse %s", char_get_name(ch)));

      *descBuf            = '\0';
      obj_set_description(corpse, str_format("Half of %s's corpse is lying here", char_get_name(ch)));

      *descBuf                  = '\0';
      obj_set_short_description(corpse, str_format("Half of %s's corpse", char_get_name(ch)));
      break;
    case DTYPE_VAPOR:
      *buf2        = '\0';
      obj_set_name(corpse, str_format("burnt chunks corpse %s", char_get_name(ch)));

      *descBuf            = '\0';
      obj_set_description(corpse, str_format("The burnt chunks of %s's corpse are scattered here", char_get_name(ch)));

      *descBuf                  = '\0';
      obj_set_short_description(corpse, str_format("The burnt chunks of %s's corpse", char_get_name(ch)));
      break;
    case DTYPE_PULP:
      *buf2        = '\0';
      obj_set_name(corpse, str_format("beaten bloody corpse %s", char_get_name(ch)));

      *descBuf            = '\0';
      obj_set_description(corpse, str_format("The bloody and beaten corpse of %s is lying here", char_get_name(ch)));

      *descBuf                  = '\0';
      obj_set_short_description(corpse, str_format("The bloody and beaten remains of %s's corpse", char_get_name(ch)));
      break;
    default:
      obj_set_name(corpse, str_format("corpse %s", char_get_name(ch)));

      *descBuf            = '\0';
      obj_set_description(corpse, str_format("The corpse of %s is lying here", char_get_name(ch)));

      *descBuf                  = '\0';
      obj_set_short_description(corpse, str_format("the remains of %s's corpse", char_get_name(ch)));
      break;
  }

  if (!char_is_npc(ch)) { /* Let's set the corpse's limbs */
    if (char_get_limb_condition(ch, (1) - 1) <= 0) {
      obj_set_value(corpse, VAL_CORPSE_RARM, 0);
    } else if (char_get_limb_condition(ch, (1) - 1) > 0 && char_get_limb_condition(ch, (1) - 1) < 50) {
      obj_set_value(corpse, VAL_CORPSE_RARM, 2);
    }
    if (char_get_limb_condition(ch, (2) - 1) <= 0) {
      obj_set_value(corpse, VAL_CORPSE_LARM, 0);
    } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50) {
      obj_set_value(corpse, VAL_CORPSE_LARM, 2);
    }
    if (char_get_limb_condition(ch, (3) - 1) <= 0) {
      obj_set_value(corpse, VAL_CORPSE_RLEG, 0);
    } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50) {
      obj_set_value(corpse, VAL_CORPSE_RLEG, 2);
    }
    if (char_get_limb_condition(ch, (4) - 1) <= 0) {
      obj_set_value(corpse, VAL_CORPSE_LLEG, 0);
    } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50) {
      obj_set_value(corpse, VAL_CORPSE_LLEG, 2);
    }
    return;
  } else { /* Do nothing else! */
    return;
  }
}

static void make_corpse(GameWorld *g, struct char_data *ch, struct char_data *tch)
{
  struct obj_data *corpse;
  struct obj_data *money;
  struct obj_data *meat;
  int i, x, y;

  corpse = create_obj(g);

  obj_set_item_number(corpse, NOTHING);
  obj_set_in_room(corpse, NOWHERE);

  /* This handles how the corpse is viewed - Iovan */
  handle_corpse_condition(corpse, ch);

  if (char_aff_flagged(ch, AFF_ASHED)) {
    act(g, "@WSome ashes fall off the corpse.@n", true, ch, nullptr, nullptr, TO_ROOM);
    struct obj_data *ashes;
    if (rand_number(g, 1, 3) == 2) {
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
    } else if (rand_number(g, 1, 2) == 2) {
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
    } else {
      ashes = read_object(g, 1305);
      obj_to_room(g, ashes, char_get_in_room(ch));
    }
  }

  /* Let's have a chance to give animals meat */
  if (tch != nullptr) {
    if (!char_is_npc(tch) && char_get_skill(tch, SKILL_SURVIVAL)) {
      int skill = char_get_skill(tch, SKILL_SURVIVAL);
      if (!char_is_humanoid(ch) && char_prf_flagged(g, tch, PRF_CARVE) && axion_dice(g, 0) < skill) {
        send_to_char(g, tch, "The choice edible meat is preserved because of your skill.\r\n");
        meat = read_object(g, 1612);
        obj_to_char(g, meat, ch);
        char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH], nick3[MAX_INPUT_LENGTH];
        sprintf(nick, "@RRaw %s@R Steak@n", char_get_name(ch));
        sprintf(nick2, "Raw %s Steak", char_get_alias(ch));
        sprintf(nick3, "@wA @Rraw %s@R steak@w is lying here@n", char_get_name(ch));
        obj_set_short_description(meat, nick);
        obj_set_name(meat, nick2);
        obj_set_description(meat, nick3);
        obj_set_value(meat, 7, 14);
      }
    }
  }

  obj_set_type_flag(corpse, ITEM_CONTAINER);
  obj_set_size(corpse, get_size(ch));
  for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      obj_get_extra_flags(corpse)[x] = 0;
    if (y < TW_ARRAY_MAX)
      obj_get_wear_flags(corpse)[y] = 0;
  }
  obj_wear_set(corpse, ITEM_WEAR_TAKE, true);
  obj_extra_set(corpse, ITEM_NODONATE, true);
  obj_set_value(corpse, VAL_CONTAINER_CAPACITY, 0);                  /* You can't store stuff in a corpse */
  obj_set_value(corpse, VAL_CONTAINER_CORPSE, 1);                    /* corpse identifier */
  obj_set_value(corpse, VAL_CONTAINER_OWNER, char_get_pfilepos(ch)); /* corpse identifier */
  obj_set_weight(corpse, char_get_pc_weight(g, ch) + char_get_carry_weight(ch));
  obj_set_cost_per_day(corpse, 100000);
  if (char_is_npc(ch))
    obj_set_timer(corpse, CONFIG_MAX_NPC_CORPSE_TIME);
  else
    obj_set_timer(corpse, rand_number(g, CONFIG_MAX_PC_CORPSE_TIME / 2, CONFIG_MAX_PC_CORPSE_TIME));
  obj_extra_set(corpse, ITEM_UNIQUE_SAVE, true);

  if (char_mob_flagged(ch, MOB_HUSK)) {
    for (obj_data *obj : char_inventory(g, ch)) {
      obj_from_char(g, obj);
      extract_obj(g, obj);
    }
  }

  if (!char_mob_flagged(ch, MOB_HUSK)) {
    /* transfer character's inventory to the corpse */
    *obj_contains_slot(corpse) = char_get_inventory(ch);
    for (obj_data *o : obj_contents(g, corpse)) {
      obj_set_in_obj(o, corpse);
    }
    object_list_new_owner(g, obj_get_contains(corpse), nullptr);
    obj_set_carried_by(corpse, nullptr);

    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++)
      if (char_get_equipment(ch, i)) {
        remove_otrigger(g, char_get_equipment(ch, i), ch);
        if (char_get_equipment(ch, i)) /* its script can purge it */
          obj_to_obj(g, unequip_char(g, ch, i), corpse);
      }
  }
  /* transfer gold */
  if (char_get_gold(ch) > 0 && !char_mob_flagged(ch, MOB_HUSK)) {
    /*
     * following 'if' clause added to fix gold duplication loophole
     * The above line apparently refers to the old "partially log in,
     * kill the game character, then finish login sequence" duping
     * bug. The duplication has been fixed (knock on wood) but the
     * test below shall live on, for a while. -gg 3/3/2002
     */
    if (char_is_npc(ch) || char_get_desc(ch)) {
      money = create_money(g, char_get_gold(ch));
      obj_to_obj(g, money, corpse);
    }
    char_set_gold(ch, 0);
  }
  if (!char_mob_flagged(ch, MOB_HUSK)) {
    char_inventory_slot(ch)->clear();
    char_set_carry_items(ch, 0);
    char_set_carry_weight(ch, 0);
  }
  obj_to_room(g, corpse, char_get_in_room(ch));

  if (!char_is_npc(ch))
    Crash_rentsave(g, ch, 0);
}

static void loadmap(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj;
  if (!char_is_npc(ch)) {
    obj = read_object(g, 17);
    obj_to_char(g, obj, ch);
  }
}

/* When ch kills victim */

void death_cry(GameWorld *g, struct char_data *ch)
{
  int door;
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (char_can_go(g, ch, door))
      send_to_room(g, room_get_dir_option(g, char_get_in_room(ch), door)->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}

/* Let's clean up necessary things after "death" */
static void final_combat_resolve(GameWorld *g, struct char_data *ch)
{
  struct obj_data *chair;

  if (char_get_sits(g, ch)) {
    chair = char_get_sits(g, ch);
    char_set_sits(ch, nullptr);
    obj_set_sitting(chair, nullptr);
  }
  if (!char_is_npc(ch) && char_get_clones(ch) > 0) {
    for (char_data *clone : each_char(g)) {
      if (char_is_npc(clone)) {
        if (char_get_mob_vnum(g, clone) == 25) {
          if (char_get_original(g, clone) == ch) {
            handle_multi_merge(g, clone);
          }
        }
      }
    }
  }
  if (char_get_carrying(g, ch)) {
    carry_drop(g, ch, 2);
  }
  if (char_get_carried_by(g, ch)) {
    carry_drop(g, char_get_carried_by(g, ch), 2);
  }
  if (char_get_drag(g, ch)) {
    char_set_dragged(char_get_drag(g, ch), nullptr);
    char_set_drag(ch, nullptr);
  }
  if (char_get_dragged(g, ch)) {
    char_set_drag(char_get_dragged(g, ch), nullptr);
    char_set_dragged(ch, nullptr);
  }
  if (char_get_grappling(g, ch)) {
    char_set_grap(char_get_grappling(g, ch), -1);
    char_set_grappled(char_get_grappling(g, ch), nullptr);
    char_set_grappling(ch, nullptr);
    char_set_grap(ch, -1);
  }
  if (char_get_grappled(g, ch)) {
    char_set_grap(char_get_grappled(g, ch), -1);
    char_set_grappling(char_get_grappled(g, ch), nullptr);
    char_set_grappled(ch, nullptr);
    char_set_grap(ch, -1);
  }
  if (char_get_blocked(g, ch)) {
    char_set_blocks(char_get_blocked(g, ch), nullptr);
    char_set_blocked(ch, nullptr);
  }
  if (char_get_blocks(g, ch)) {
    char_set_blocked(char_get_blocks(g, ch), nullptr);
    char_set_blocks(ch, nullptr);
  }
  if (char_get_absorbing(g, ch)) {
    char_set_absorbby(char_get_absorbing(g, ch), nullptr);
    char_set_absorbing(ch, nullptr);
  }
  if (char_get_absorbby(g, ch)) {
    char_set_absorbing(char_get_absorbby(g, ch), nullptr);
    char_set_absorbby(ch, nullptr);
  }
}

typedef enum DeathType : uint8_t {
  Afterlife = 0,
  Northran  = 1,
  Past      = 2,
  Newbie    = 3
} DeathType;

void raw_kill(GameWorld *g, struct char_data *ch, struct char_data *killer)
{

  if (char_get_fighting(g, ch))
    stop_fighting(g, ch);

  while (!char_affects(ch).empty())
    affect_remove(g, ch, &char_affects(ch).front());

  /* To make ordinary commands work in scripts.  welcor*/
  if (char_get_position(ch) != POS_SITTING && char_get_position(ch) != POS_SLEEPING && char_get_position(ch) != POS_RESTING)
    char_set_position(ch, POS_STANDING);

  if (killer && !char_is_npc(killer)) {
    if (!char_is_npc(killer) && !char_is_npc(ch)) {
      send_to_imm(g, "[PK] %s killed %s at room [%d]\r\n", char_get_name(killer), char_get_name(ch), char_get_in_room(killer));
    }
    if ((char_is_saiyan(killer) && rand_number(g, 1, 2) == 2) || !char_is_saiyan(killer)) {
      if (rand_number(g, 1, 6) >= 5 && (level_exp(killer, char_get_level(killer) + 1) - char_get_exp(killer) > 0 || char_get_level(killer) == 100)) {
        int psreward = char_get_aff_abils_wis(killer) * 0.35;
        if (char_get_level(killer) > char_get_level(ch) + 5) {
          psreward *= 0.2;
        } else if (char_get_level(killer) > char_get_level(ch) + 2) {
          psreward *= 0.5;
        }
        if (char_is_human(killer) || (char_is_bio(killer) && (char_get_genome(killer, 0) == 1 || char_get_genome(killer, 1) == 1))) {
          psreward *= 1.25;
        }
        if (char_is_halfbreed(killer)) {
          psreward *= 0.6;
        }
        if (char_is_npc(ch) && char_mob_flagged(ch, MOB_HUSK) && char_get_practice_points(g, killer) > 50 && char_is_bio(ch)) {
          psreward = 0;
          send_to_char(g, killer, "@D[@G+0 @BPS @cCapped at 50 for Absorb@D]@n\r\n");
        } else {
          char_set_practice_points(g, killer, char_get_practice_points(g, killer) + (psreward));
          send_to_char(g, killer, "@D[@G+%d @BPS@D]@n\r\n", psreward);
        }
      }
    }
    if (char_is_android(killer) && !char_is_npc(killer) && !char_plr_flagged(killer, PLR_ABSORB)) {
      if (char_plr_flagged(killer, PLR_REPAIR)) {
        if (char_get_level(killer) > char_get_level(ch) + 15) {
          send_to_char(g, killer, "@D[@G+0 @mUpgrade Point @r-WEAK-@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 10) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (3));
          send_to_char(g, killer, "@D[@G+3 @mUpgrade Point@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 8) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (6));
          send_to_char(g, killer, "@D[@G+6 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 4) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (12));
          send_to_char(g, killer, "@D[@G+12 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 2) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (16));
          send_to_char(g, killer, "@D[@G+16 @mUpgrade Points@D]@n\r\n");
        } else {
          char_set_upgrade(killer, char_get_upgrade(killer) + (28));
          send_to_char(g, killer, "@D[@G+28 @mUpgrade Points@D]@n\r\n");
        }
      } else {
        if (char_get_level(killer) > char_get_level(ch) + 15) {
          send_to_char(g, killer, "@D[@G+0 @mUpgrade Point @r-WEAK-@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 10) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (5));
          send_to_char(g, killer, "@D[@G+5 @mUpgrade Point@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 6) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (12));
          send_to_char(g, killer, "@D[@G+12 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 4) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (18));
          send_to_char(g, killer, "@D[@G+18 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 2) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (28));
          send_to_char(g, killer, "@D[@G+28 @mUpgrade Points@D]@n\r\n");
        } else {
          char_set_upgrade(killer, char_get_upgrade(killer) + (36));
          send_to_char(g, killer, "@D[@G+36 @mUpgrade Points@D]@n\r\n");
        }
      }
    }
    if (death_mtrigger(g, ch, killer))
      death_cry(g, ch);
  } else
    death_cry(g, ch);

  update_pos(g, ch);

  if (char_is_npc(ch) && !char_mob_flagged(ch, MOB_DUMMY)) {
    int shadowed = false;
    decCurHealthPercent(g, ch, 1, 0);
    if (char_is_shadow_dragon1(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON1       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe one star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 20);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    } else if (char_is_shadow_dragon2(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON2       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe two star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 21);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    } else if (char_is_shadow_dragon3(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON3       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe three star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 22);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    } else if (char_is_shadow_dragon4(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON4       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe four star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 23);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    } else if (char_is_shadow_dragon5(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON5       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe five star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 24);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    } else if (char_is_shadow_dragon6(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON6       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe six star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 25);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    } else if (char_is_shadow_dragon7(g, ch)) {
      struct obj_data *obj = nullptr;
      g->SHADOW_DRAGON7       = -1;
      send_to_room(g, char_get_in_room(ch), "@YThe seven star dragon ball falls to the ground!@n\r\n");

      obj = read_object(g, 26);
      obj_to_room(g, obj, char_get_in_room(ch));
      shadowed = true;
    }
    make_corpse(g, ch, killer);
    purge_homing(g, ch);
    extract_char(g, ch);
    if (shadowed == true) {
      shadow_dragons_live(g);
    }
  } else if (char_is_npc(ch) && char_mob_flagged(ch, MOB_DUMMY)) {
    decCurHealthPercent(g, ch, 1, 0);
    extract_char(g, ch);
  } else {
    if (!char_aff_flagged(ch, AFF_SPIRIT) && !room_flagged(g, char_get_in_room(ch), ROOM_PAST) && (char_get_in_room(ch) < 17900 || char_get_in_room(ch) > 17999)) {
      if (!char_plr_flagged(ch, PLR_ABSORBED)) {
        make_pcorpse(g, ch);
        loadmap(g, ch);
      } else {
        char_plr_set(ch, PLR_ABSORBED, false);
      }
    }
    final_combat_resolve(g, ch);
    if (char_get_fighting(g, ch))
      stop_fighting(g, ch);

    for (char_data *k : each_fighting(g))
      if (char_get_fighting(g, k) == ch)
        stop_fighting(g, k);

    bool android_lose    = true;
    DeathType death_type = Afterlife;
    if (in_past(g, ch))
      death_type = Past;
    else if (in_northran(g, ch))
      death_type = Northran;
    else if (is_newbie(ch))
      death_type = Newbie;

    switch (death_type) {
      case Afterlife:
        ghostify(g, ch);
        purge_homing(g, ch);
        if (char_get_level(ch) > 0 && has_group(g, ch)) {
          if (char_get_master(g, ch) != nullptr) {
            group_bonus(g, ch, 1);
          } else {
            group_bonus(g, ch, 0);
          }
        }
        teleport_to(g, ch, 6000);
        break;
      case Northran:
        restore(g, ch, false);
        teleport_to(g, ch, 17900);
        android_lose = false;
        send_to_char(g, ch, "You wake up and realise that you didn't die, how or why are a mystery.\r\n");
        break;
      case Past:
        restore(g, ch, false);
        teleport_to(g, ch, 1561);
        android_lose = false;
        send_to_char(g, ch, "You wake up and realise that you died, but only in your mind.\r\n");
        break;
      case Newbie:
        restore(g, ch, false);
        teleport_to(g, ch, sensei_start_room(char_get_class(ch)));
        send_to_char(g, ch, "\r\n@RYou should beware, when you reach level 9, you will actually die. So you\r\n"
                         "should learn to be more careful. Since when you die past that point and\r\n"
                         "actually reach the afterlife you need to realise that being revived will\r\n"
                         "not be very easy. So treat your character's dying with as much care as\r\n"
                         "possible.@n\r\n");
        break;
    }

    if (!char_is_npc(ch)) {
      if (char_is_android(ch) && !char_plr_flagged(ch, PLR_ABSORB) && android_lose && char_get_upgrade(ch) > 5) {
        int loss = char_get_upgrade(ch) / 5;
        char_set_upgrade(ch, char_get_upgrade(ch) - (loss));
        send_to_char(g, ch, "@rYou lose @R%s@r upgrade points!@n\r\n", add_commas(loss));
      }
      Crash_delete_crashfile(ch);
      save_char(g, ch);
    }
    char_set_wait(ch, PULSE_VIOLENCE);
  }
}

void die(GameWorld *g, struct char_data *ch, struct char_data *killer)
{
  // NPCs just die.
  if (char_is_npc(ch)) {
    raw_kill(g, ch, killer);
    return;
  }

  // But there's special handling for players.

  // Clear healing tank usage. How did you die while inside a healing tank though?
  if (char_plr_flagged(ch, PLR_HEALT)) {
    char_plr_set(ch, PLR_HEALT, false);
  }

  // First there are a few mechanics which can prevent death. They are checked first.

  // Saiyan Zenkai mechanic: if at 75% lifeforce or higher, 25% chance of triggering Zenkai.
  // Added check for character having AFF_SPIRIT to prevent triggering in Afterlife
  // this is implemented by setting PLR_GOOP with gooptime 0
  if (char_is_saiyan(ch) && !char_aff_flagged(ch, AFF_SPIRIT) && (getCurLFPercent(ch) >= 0.75) && rand_number(g, 1, 4) == 4) {
    char_plr_set(ch, PLR_GOOP, true);
    char_set_gooptime(ch, 0);
    decCurLFPercent(g, ch, 0.5, 0);
    return;
  }

  // majin and bio regen mechanic skips actually dying...
  if ((char_is_majin(ch) || char_is_bio(ch)) && ((getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.75 || (char_plr_flagged(ch, PLR_SELFD2) &&
                                                                                             (getCurLF(g, ch)) >= (getMaxLF(g, ch)) * 0.5))) {
    decCurLFPercent(g, ch, 2, -1);
    decCurHealthPercent(g, ch, 1, 1);
    char_plr_set(ch, PLR_GOOP, true);
    char_set_gooptime(ch, 32);
    return;
  }

  // for players who used the Immortal Wish.
  if (char_plr_flagged(ch, PLR_IMMORTAL)) {
    act(g, "@c$n@w disappears right before dying. $n appears to be immortal.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@w disappears right before dying. $n appears to be immortal.@n.", true, ch, nullptr, nullptr, TO_ROOM);
    decCurHealthPercent(g, ch, 1, 1);
    decCurKIPercent(g, ch, 1, 1);
    decCurSTPercent(g, ch, 1, 1);
    null_affect(g, ch, AFF_POISON);
    if (char_get_conditions(g, ch, HUNGER) >= 0) {
      char_set_conditions(g, ch, HUNGER, 48);
    }
    if (char_get_conditions(g, ch, THIRST) >= 0) {
      char_set_conditions(g, ch, THIRST, 48);
    }
    if (char_get_fighting(g, ch)) {
      stop_fighting(g, ch);
    }
    char_set_position(ch, POS_SITTING);
    teleport_to(g, ch, sensei_start_room(char_get_class(ch)));
    return;
  }

  // Removing some consequences that might happen.
  char_plr_set(ch, PLR_KILLER, false);
  char_plr_set(ch, PLR_THIEF, false);
  char_aff_set(ch, AFF_KNOCKED, false);
  char_aff_set(ch, AFF_SLEEP, false);
  char_aff_set(ch, AFF_PARALYZE, false);

  // those who die in the arena don't actually die. They get returned to the waiting room.
  if (char_in_arena(g, ch)) {
    cleanup_arena_watch(g, ch);
    if (killer != nullptr) {
      cleanup_arena_watch(g, killer);
      send_to_all(g, "@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n", char_get_name(killer), char_get_name(ch));
      final_combat_resolve(g, killer);
      final_combat_resolve(g, ch);
      teleport_to(g, killer, 17875);
    } else {
      send_to_all(g, "@R%s@r dies in the water of the Arena and is disqualified!@n\r\n", char_get_name(ch));
    }
    char_from_room(g, ch);
    char_to_room(g, ch, 17875);
    decCurHealthPercent(g, ch, 1, 1);
    look_at_room(g, char_get_in_room(ch), ch, 0);
    final_combat_resolve(g, ch);
    return;
  }

  // For code below this, some kind of 'death' is performed.
  // it will leave a corpse.

  if (!char_aff_flagged(ch, AFF_SPIRIT) && !room_flagged(g, char_get_in_room(ch), ROOM_PAST) && char_get_level(ch) > 8) {
    if (char_get_in_room(ch) >= 2002 && char_get_in_room(ch) <= 2011) {
      char_set_deathtime(ch, game_now(g));
    } else if (room_flagged(g, char_get_in_room(ch), ROOM_AL) || room_flagged(g, char_get_in_room(ch), ROOM_HELL)) {
      send_to_char(g, ch, "Your soul is saved from destruction by King Yemma. Why? Who knows.\r\n");
    } else {
      if (killer != nullptr && char_is_npc(killer)) {
        char_set_deathtime(ch, game_now(g) + 7200);
        char_set_dcount(ch, char_get_dcount(ch) + (1));
      } else if (killer != nullptr && !char_is_npc(killer)) {
        char_set_deathtime(ch, game_now(g) + 345600);
        char_plr_set(ch, PLR_PDEATH, true);
        char_set_dcount(ch, char_get_dcount(ch) + (1));
      } else {
        char_set_deathtime(ch, game_now(g) + 7200);
        char_set_dcount(ch, char_get_dcount(ch) + (1));
      }
    }
    if (char_get_conditions(g, ch, HUNGER) >= 0) {
      char_set_conditions(g, ch, HUNGER, 48);
    }
    if (char_get_conditions(g, ch, THIRST) >= 0) {
      char_set_conditions(g, ch, THIRST, 48);
    }
  }

  raw_kill(g, ch, killer);
}

static void perform_group_gain(GameWorld *g, struct char_data *ch, int base, struct char_data *victim)
{
  int64_t share;

  if (char_in_arena(g, ch)) {
    return;
  }
  struct char_data *leader = char_get_master(g, ch) ? char_get_master(g, ch) : ch;

  /*share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base * GET_LEVEL(ch)));*/
  share = MIN(2000000, base * char_get_level(ch));
  if (!char_is_npc(ch)) {
    if (char_get_level(ch) >= 100 && getEffMaxPL(g, ch) * .025 >= getEffMaxPL(g, victim)) {
      share *= .05;
    } else if (getEffMaxPL(g, ch) * .025 >= getEffMaxPL(g, victim)) {
      share = 1;
    } else if (getEffMaxPL(g, ch) * .05 >= getEffMaxPL(g, victim)) {
      share *= .05;
    } else if (getEffMaxPL(g, ch) * .1 >= getEffMaxPL(g, victim)) {
      share *= .1;
    } else if (getEffMaxPL(g, ch) * .15 >= getEffMaxPL(g, victim)) {
      share *= .15;
    } else if (getEffMaxPL(g, ch) * .25 >= getEffMaxPL(g, victim)) {
      share *= .25;
    } else if (getEffMaxPL(g, ch) * .5 >= getEffMaxPL(g, victim)) {
      share *= .5;
    } else if (getEffMaxPL(g, ch) * .9 >= getEffMaxPL(g, victim)) {
      share *= .65;
    } else if (getEffMaxPL(g, ch) >= getEffMaxPL(g, victim)) {
      share *= .7;
    }
  }
  if (char_get_lasthit(victim) != 0 && char_get_lasthit(victim) != char_get_idnum(ch)) {
    int checkit = false;
    for (char_data *fol : char_followers(g, ch)) {
      if (char_aff_flagged(fol, AFF_GROUP) && char_get_lasthit(victim) == char_get_idnum(fol)) {
        checkit = true;
      }
    }
    if (checkit == false && char_get_master(g, ch) != nullptr && char_get_idnum(char_get_master(g, ch)) == char_get_lasthit(victim)) {
      checkit = true;
    }
    if (checkit == false && char_get_master(g, ch) != nullptr) {
      struct char_data *master = char_get_master(g, ch);
      for (char_data *foll : char_followers(g, master)) {
        if (foll != ch) {
          if (char_aff_flagged(foll, AFF_GROUP) && char_get_lasthit(victim) == char_get_idnum(foll)) {
            checkit = true;
          }
        }
      }
    }
    if (checkit == false) {
      send_to_char(g, ch, "@RYou didn't do most of the work for this kill.@n\r\n");
      share = 1;
    }
  }
  if (char_is_npc(victim) && char_mob_flagged(victim, MOB_HUSK)) {
    share /= 10;
  }
  if (char_get_bonuses(ch, BONUS_PRODIGY) > 0) {
    share = share + (share * .25);
  }
  if (char_is_saiyan(ch)) {
    share = share + (share * .50);
  }
  if (char_is_halfbreed(ch)) {
    share = share + (share * .40);
  }
  if (char_is_icer(ch)) {
    share = share - (share * .20);
  }
  if (char_get_bonuses(ch, BONUS_LOYAL) > 0 && char_get_master(g, ch) != nullptr) {
    share += share * 0.2;
  }
  if (char_get_master(g, ch) != nullptr && char_get_master(g, ch) != ch) {
    share += share * 0.15;
  }
  if (char_mob_flagged(victim, MOB_KNOWKAIO)) {
    share += share * .25;
  }
  char_set_combatexpertise(ch, char_get_combatexpertise(ch) + (1));
  if ((char_get_combatexpertise(ch) + 1) / 20 > share * 0.16) {
    share += share * 0.16;
  } else {
    share += (share * 0.02) * ((char_get_combatexpertise(ch) + 1) / 20);
  }
  if (group_bonus(g, ch, 2) == 2) {
    send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G+2 PS!@D]@n\r\n");
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) + (2));
  } else if (group_bonus(g, ch, 2) == 3) {
    send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G+5%s Exp!@D]@n\r\n", "%");
    share += share * 0.05;
  } else if (group_bonus(g, ch, 2) == 5) {
    incCurKIPercent(g, ch, .04, true);
    send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G4%s Ki Regenerated!@D]@n\r\n", "%");
  } else if (group_bonus(g, ch, 2) == 6) {
    incCurKIPercent(g, ch, .02, true);
    incCurSTPercent(g, ch, .02, true);
    incCurHealthPercent(g, ch, .02, true);
    send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G2%s PL/ST/Ki Regenerated!@D]@n\r\n", "%");
  } else if (group_bonus(g, ch, 2) == 7) {
    if (char_is_android(ch)) {
      if (char_plr_flagged(leader, PLR_ABSORB)) {
        incCurKIPercent(g, ch, .02, true);
        incCurSTPercent(g, ch, .02, true);
        send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G2%s PL/ST/Ki Recovered!@D]@n\r\n", "%");
      } else if (char_plr_flagged(leader, PLR_REPAIR)) {
        incCurHealthPercent(g, ch, .02, true);
        send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G5%s PL Repaired@D]@n\r\n", "%");
      } else if (char_plr_flagged(leader, PLR_SENSEM) && !char_plr_flagged(ch, PLR_ABSORB)) {
        char_set_upgrade(ch, char_get_upgrade(ch) + (5));
        send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G+5 @mUpgrade Points@D]@n\r\n");
      }
    } else {
      incCurHealthPercent(g, ch, .01, true);
      incCurKIPercent(g, ch, .01, true);
      incCurSTPercent(g, ch, .01, true);
    }
  } else if (group_bonus(g, ch, 2) == 11) {
    incCurSTPercent(g, ch, .04, true);
    send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G4%s ST Regenerated!@D]@n\r\n", "%");
  } else if (group_bonus(g, ch, 2) == 13) {
    if (char_get_starphase(leader) == 1) {
      share += share * 0.05;
      send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G+5%s Exp!@D]@n\r\n", "%");
    } else if (char_get_starphase(leader) == 2) {
      share += share * 0.1;
      send_to_char(g, ch, "You receive a bonus from your group's leader! @D[@G+10%s Exp!@D]@n\r\n", "%");
    }
  }
  share = gear_exp(g, ch, share);
  if (share > 1)
    send_to_char(g, ch, "You receive your share of experience -- %s points.\r\n", add_commas(share));
  else
    send_to_char(g, ch, "You receive your share of experience -- one measly little point!\r\n");

  gain_exp(g, ch, share);
  /*change_alignment(ch, victim);*/
}

void group_gain(GameWorld *g, struct char_data *ch, struct char_data *victim)
{
  int tot_levels, tot_members;
  int64_t tot_gain, base;
  struct char_data *k;

  if (!(k = char_get_master(g, ch)))
    k = ch;

  if (char_aff_flagged(k, AFF_GROUP) && (char_get_in_room(k) == char_get_in_room(ch))) {
    tot_levels  = char_get_level(k);
    tot_members = 1;
  } else {
    tot_levels  = 0;
    tot_members = 0;
  }

  for (char_data *fol : char_followers(g, k))
    if (char_aff_flagged(fol, AFF_GROUP) && char_get_in_room(fol) == char_get_in_room(ch)) {
      if (!char_is_weighted(g, fol)) {
        tot_levels += char_get_level(fol);
        tot_members++;
      } else if (getEffMaxPL(g, fol) >= getEffMaxPL(g, ch) * 0.5) {
        tot_levels += char_get_level(fol);
        tot_members++;
      }
    }

  if (tot_members == 1 || char_in_arena(g, ch)) {
    solo_gain(g, ch, victim);
    return;
  }

  /* round up to the next highest tot_members */
  tot_gain = (char_get_exp(victim)) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!char_is_npc(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2, tot_gain);

  if (tot_levels >= 1) {
    base     = MAX(1, tot_gain / tot_levels);
    int perc = 20 * tot_members;
    if (perc >= 80) {
      perc = 60;
    }
    base += (base / 100) * perc;
  } else
    base = 0;

  /*
  if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch)) {
   if (!IS_WEIGHTED(k)) {
    perform_group_gain(k, base, victim);
   } else if (k != ch && (getEffMaxPL(k)()) >= (getEffMaxPL(ch)) * 0.5) {
    perform_group_gain(k, base, victim);
   } else if (k == ch && (getEffMaxPL(k)()) >= GET_MAX_HIT(ch) * 0.5) {
    perform_group_gain(k, base, victim);
   } else {
    if (k == ch) {
     send_to_char(ch, "You can not group gain while your powerlevel is weighted down more than half of your max.\r\n");
    } else {
     send_to_char(ch, "You can not group gain while your powerlevel is weighted down more than half of the leader's adjusted powerlevel.\r\n");
    }
   }
  }
   */
  perform_group_gain(g, k, base, victim);

  for (char_data *foll : char_followers(g, k)) {
    if (char_aff_flagged(foll, AFF_GROUP) && char_get_in_room(foll) == char_get_in_room(ch)) {
      //if ((getEffMaxPL(foll)()) >= GET_MAX_HIT(ch) * 0.5)
      perform_group_gain(g, foll, base, victim);
    }
  }
}

void solo_gain(GameWorld *g, struct char_data *ch, struct char_data *victim)
{

  if (char_is_npc(ch)) {
    if (char_get_original(g, ch)) {
      ch = char_get_original(g, ch);
    }
  }
  int64_t exp;

  exp = MIN(2000000, char_get_exp(victim));

  /* Calculate level-difference penalty */
  if (!char_is_npc(ch)) {
    if (char_get_level(ch) >= 100 && getEffMaxPL(g, ch) * .025 >= getEffMaxPL(g, victim)) {
      exp *= 0.04;
    } else if (getEffMaxPL(g, ch) * .025 >= getEffMaxPL(g, victim)) {
      exp = 1;
    } else if (getEffMaxPL(g, ch) * .05 >= getEffMaxPL(g, victim)) {
      exp *= 0.05;
    } else if (getEffMaxPL(g, ch) * .1 >= getEffMaxPL(g, victim)) {
      exp *= .1;
    } else if (getEffMaxPL(g, ch) * .15 >= getEffMaxPL(g, victim)) {
      exp *= .15;
    } else if (getEffMaxPL(g, ch) * .25 >= getEffMaxPL(g, victim)) {
      exp *= .25;
    } else if (getEffMaxPL(g, ch) * .5 >= getEffMaxPL(g, victim)) {
      exp *= .5;
    }
  }
  if (char_get_lasthit(victim) != 0 && char_get_lasthit(victim) != char_get_idnum(ch)) {
    send_to_char(g, ch, "@RYou didn't do most of the work for this victory.@n\r\n");
    exp = 1;
  }
  if (char_is_npc(victim) && char_mob_flagged(victim, MOB_HUSK)) {
    exp /= 10;
  }
  if (char_get_bonuses(ch, BONUS_PRODIGY) > 0) {
    exp = exp + (exp * .25);
  }
  if (char_is_saiyan(ch)) {
    exp = exp + (exp * .50);
  }
  if (char_is_halfbreed(ch)) {
    exp = exp + (exp * .40);
  }
  if (char_is_icer(ch)) {
    exp = exp - (exp * .20);
  }
  if (char_mob_flagged(victim, MOB_KNOWKAIO)) {
    exp += exp * .25;
  }
  exp = gear_exp(g, ch, exp);
  exp = MAX(exp, 1);

  if (exp > 1)
    send_to_char(g, ch, "You receive %s experience points.\r\n", add_commas(exp));
  else {
    send_to_char(g, ch, "You receive one lousy experience point. That fight was hardly worth it...\r\n");
  }
  if (!char_is_npc(ch)) {
    gain_exp(g, ch, exp);
  }
  if (char_is_npc(victim)) {
    gain_exp(g, victim, -exp);
  }
  if (!char_is_npc(victim)) {
    exp = exp / 5;
    gain_exp(g, victim, -exp);
  }
  /*change_alignment(ch, victim);*/
}
