/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/utils.h"
#include "dbat/consts/deathtype.h"
#include "dbat/consts/shadowdragons.h"
#include "dbat/fight.h"
#include "dbat/dg_comm.h"
#include "dbat/act.attack.h"
#include "dbat/act.other.h"
#include "dbat/act.misc.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/config.h"
#include "dbat/races.h"
#include "dbat/handler.h"
#include "dbat/combat.h"
#include "dbat/class.h"
#include "dbat/dg_scripts.h"
#include "dbat/objsave.h"
#include "dbat/affect.h"
#include "dbat/act.offensive.h"
#include "dbat/act.item.h"
#include "dbat/local_limits.h"

/* Structures */
struct char_data *combat_list             = NULL; /* head of l-list of fighting chars */
static struct char_data *next_combat_list = NULL;

/* local functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void check_killer(struct char_data *ch, struct char_data *vict);
static void make_corpse(struct char_data *ch, struct char_data *tch);
static void handle_corpse_condition(struct obj_data *corpse, struct char_data *ch);
static void make_pcorpse(struct char_data *ch);
static void final_combat_resolve(struct char_data *ch);
static void shadow_dragons_live(void);
static void cleanup_arena_watch(struct char_data *ch);
static void mob_attack(struct char_data *ch, char *buf);
static int pick_n_throw(struct char_data *ch, char *buf);

int group_bonus(struct char_data *ch, int type)
{
  struct follow_type *k, *next;

  if (!char_aff_flagged(ch, AFF_GROUP))
    return (FALSE);

  if (ch->followers) {
    for (k = ch->followers; k; k = next) {
      next = k->next;
      if (!char_aff_flagged(k->follower, AFF_GROUP)) {
        continue;
      } else {
        if (type == 0) {
          incCurLFPercent(k->follower, .25, true);
          send_to_char(k->follower, "@CIncensed by the death of your comrade your life force swells!@n");
          return (TRUE);
        } else if (type == 1) {
          incCurLFPercent(k->follower, .4, true);
          send_to_char(k->follower, "@CIncensed by the death of your comrade your life force swells!@n");
          return (TRUE);
        } else if (type == 2) {
          if (char_is_roshi(ch)) {
            return (2);
          } else if (char_is_krane(ch)) {
            return (3);
          } else if (char_is_bardock(ch)) {
            return (4);
          } else if (char_is_nail(ch)) {
            return (5);
          } else if (char_is_kabito(ch)) {
            return (6);
          } else if (char_is_andsix(ch)) {
            return (7);
          } else if (char_is_tapion(ch)) {
            return (8);
          } else if (char_is_frieza(ch)) {
            return (9);
          } else if (char_is_tsuna(ch)) {
            return (10);
          } else if (char_is_piccolo(ch)) {
            return (11);
          } else if (char_is_kurzak(ch)) {
            return (12);
          } else if (char_is_jinto(ch)) {
            return (13);
          } else if (char_is_dabura(ch)) {
            return (14);
          }
        }
        return (FALSE);
      }
    }
  } else if (ch->master) {
    if (!char_aff_flagged(ch->master, AFF_GROUP))
      return (FALSE);
    else {
      if (type == 0) {
        group_bonus(ch->master, 0);
      } else if (type == 2) {
        if (char_is_roshi(ch->master)) {
          return (2);
        } else if (char_is_krane(ch->master)) {
          return (3);
        } else if (char_is_bardock(ch->master)) {
          return (4);
        } else if (char_is_nail(ch->master)) {
          return (5);
        } else if (char_is_kabito(ch->master)) {
          return (6);
        } else if (char_is_andsix(ch->master)) {
          return (7);
        } else if (char_is_tapion(ch->master)) {
          return (8);
        } else if (char_is_frieza(ch->master)) {
          return (9);
        } else if (char_is_tsuna(ch->master)) {
          return (10);
        } else if (char_is_piccolo(ch->master)) {
          return (11);
        } else if (char_is_kurzak(ch->master)) {
          return (12);
        } else if (char_is_jinto(ch->master)) {
          return (13);
        } else if (char_is_dabura(ch->master)) {
          return (14);
        }
      }
      return (TRUE);
    }
  }
  return (FALSE);
}

void mutant_limb_regen(struct char_data *ch)
{
  if (char_get_limb_condition(ch, (1) - 1) > 0 && char_get_limb_condition(ch, (1) - 1) < 50) {
    act("The bones in your right arm have mended them selves.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n starts moving $s right arm gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (1) - 1, 100);
  } else if (char_get_limb_condition(ch, (1) - 1) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is whole again!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole again!.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (1) - 1, 100);
  }
  if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50) {
    act("The bones in your left arm have mended them selves.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n starts moving $s left arm gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (2) - 1, 100);
  } else if (char_get_limb_condition(ch, (2) - 1) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is whole again!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole again!.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (2) - 1, 100);
  }
  if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50) {
    act("The bones in your right leg have mended them selves.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n starts moving $s right leg gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (3) - 1, 100);
  } else if (char_get_limb_condition(ch, (3) - 1) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is whole again!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole again!.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (3) - 1, 100);
  }
  if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50) {
    act("The bones in your left leg have mended them selves.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n starts moving $s left leg gingerly for a moment.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (4) - 1, 100);
  } else if (char_get_limb_condition(ch, (4) - 1) <= 0) {
    act("Your right arm begins to grow back very quickly. Within moments it is whole again!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n's right arm starts to regrow! Within moments the arm is whole again!.", TRUE, ch, 0, 0, TO_ROOM);
    char_set_limb_condition(ch, (4) - 1, 100);
  }
}

static int pick_n_throw(struct char_data *ch, char *buf)
{
  struct obj_data *cont;
  char buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  ;

  if (rand_number(1, 20) < 18) {
    return (FALSE);
  }

  for (cont = world[char_get_in_room(ch)].contents; cont; cont = cont->next_content) {
    if (obj_get_weight(cont) <= getMaxCarryWeight(ch) + char_get_carry_weight(ch)) {
      snprintf(buf2, sizeof(buf2), "%s", cont->name);
      do_get(ch, buf2, 0, 0);
      snprintf(buf3, sizeof(buf3), "%s %s", buf2, buf);
      do_throw(ch, buf3, 0, 0);
      return (TRUE);
    }
  }

  return (FALSE);
}

static void mob_attack(struct char_data *ch, char *buf)
{

  int power   = rand_number(1, 5);
  int bonus   = char_get_level(ch) * 0.1;
  int special = 0;
  char buf2[MAX_INPUT_LENGTH];

  power += bonus;

  if (rand_number(1, 4) == 4)
    power += 10;

  if (power > 20)
    power = 20;

  if (char_get_class(ch) == CLASS_NPC_COMMONER)
    special = 0;

  int dragonpass = TRUE;

  if (char_is_dragon(ch)) {
    if (char_get_mob_vnum(ch) == 81 || char_get_mob_vnum(ch) == 82 || char_get_mob_vnum(ch) == 83 || char_get_mob_vnum(ch) == 84 || char_get_mob_vnum(ch) == 85 || char_get_mob_vnum(ch) == 86 || char_get_mob_vnum(ch) == 87) {
      dragonpass = TRUE;
      special    = rand_number(40, 100);
    } else {
      dragonpass = FALSE;
    }
  }

  if (axion_dice(-10) > 90 && getCurHealthPercent(ch) <= .5 && !char_mob_flagged(ch, MOB_POWERUP) && char_get_mob_vnum(ch) != 25 &&
      !(char_is_android(ch) || char_is_animal(ch) || ch->chclass == CLASS_NPC_COMMONER)) {
    do_powerup(ch, nullptr, 0, 0);
    return;
  }

  if ((getCurKI(ch)) >= getMaxKI(ch) * 0.05 && char_is_humanoid(ch) && (!char_is_dragon(ch) || dragonpass == TRUE)) {
    if (ch->mobcharge <= 0 && rand_number(1, 10) >= 8) {
      act("@wAn aura flares up around @R$n@w!@n", TRUE, ch, 0, 0, TO_ROOM);
      ch->mobcharge += 1;
      if (char_get_level(ch) > 80) {
        ch->mobcharge += 1;
      }
    } else if (ch->mobcharge <= 5) {
      act("@wThe aura burns brighter around @R$n@w!@n", TRUE, ch, 0, 0, TO_ROOM);
      ch->mobcharge += 1;
      if (char_get_level(ch) > 80) {
        ch->mobcharge += 1;
      }
    } else if (ch->mobcharge == 6) {
      act("@wThe aura around @R$n@w flashes!@n", TRUE, ch, 0, 0, TO_ROOM);
      ch->mobcharge += 1;
      special = 100;
    }
  }

  if (char_is_humanoid(ch) && dragonpass == TRUE) { /* Is a humanoid mob */
    if (char_aff_flagged(ch, AFF_PARALYZE)) {
      return;
    } else if (char_aff_flagged(ch, AFF_ENSNARED)) {
      return;
    } else if (special < 100) { /* Normal physical attack */
      if (char_get_class(ch) == CLASS_SHADOWDANCER && rand_number(1, 3) == 3) {
        sprintf(buf2, "ass %s", buf);
        do_throw(ch, buf2, 0, 0);
      } else if (char_is_android(ch) && char_mob_flagged(ch, MOB_REPAIR) && getCurPL(ch) <= (getEffMaxPL(ch)) * 0.5 && rand_number(1, 20) >= 16) {
        do_srepair(ch, NULL, 0, 0);
      } else if (char_is_android(ch) && char_mob_flagged(ch, MOB_ABSORB) && rand_number(1, 20) >= 19) {
        do_absorb(ch, buf2, 0, 0);
      } else if ((char_is_bio(ch) || char_is_majin(ch)) && getCurPL(ch) <= (getEffMaxPL(ch)) * 0.5 && rand_number(1, 20) >= 17) {
        do_regenerate(ch, (char[]){"25"}, 0, 0);
      } else if (char_is_namek(ch) && getCurPL(ch) <= (getEffMaxPL(ch)) * 0.5 && rand_number(1, 20) == 20) {
        do_regenerate(ch, (char[]){"25"}, 0, 0);
      } else if (pick_n_throw(ch, buf)) {
        /* This determines if they throw and also handles it */
      } else if (char_mob_flagged(ch, MOB_KNOWKAIO) && rand_number(1, 50) >= 46) {
        if (rand_number(1, 10) == 10) {
          do_kaioken(ch, (char[]){"20"}, 0, 0);
        } else if (rand_number(1, 10) >= 8) {
          do_kaioken(ch, (char[]){"10"}, 0, 0);
        } else {
          do_kaioken(ch, (char[]){"5"}, 0, 0);
        }
      } else {
        switch (power) {
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
            if (char_get_equipment(ch, WEAR_WIELD1))
              do_attack(ch, buf, 0, 0);
            else if (rand_number(1, 5) == 5)
              do_kick(ch, buf, 0, 0);
            else if (rand_number(1, 10) == 10)
              do_elbow(ch, buf, 0, 0);
            else
              do_punch(ch, buf, 0, 0);
            break;
          case 6:
          case 7:
          case 8:
            if (char_get_equipment(ch, WEAR_WIELD1))
              do_attack(ch, buf, 0, 0);
            else if (rand_number(1, 5) == 5)
              do_punch(ch, buf, 0, 0);
            else if (rand_number(1, 10) == 10)
              do_knee(ch, buf, 0, 0);
            else
              do_kick(ch, buf, 0, 0);
            break;
          case 9:
          case 10:
            if (rand_number(1, 5) == 5)
              do_knee(ch, buf, 0, 0);
            else if (rand_number(1, 10) == 10)
              do_uppercut(ch, buf, 0, 0);
            else
              do_elbow(ch, buf, 0, 0);
            break;
          case 11:
          case 12:
            if (rand_number(1, 5) == 5)
              do_elbow(ch, buf, 0, 0);
            else if (rand_number(1, 10) == 10)
              do_roundhouse(ch, buf, 0, 0);
            else if (rand_number(1, 8) == 8)
              do_trip(ch, buf, 0, 0);
            else
              do_knee(ch, buf, 0, 0);
            break;
          case 13:
          case 14:
            if ((char_is_bardock(ch) || char_is_kurzak(ch)) && rand_number(1, 2) == 2)
              do_head(ch, buf, 0, 0);
            else if ((char_is_icer(ch) || char_is_bio(ch)) && rand_number(1, 2) == 2)
              do_tailwhip(ch, buf, 0, 0);
            else if (rand_number(1, 8) == 8)
              do_trip(ch, buf, 0, 0);
            else
              do_uppercut(ch, buf, 0, 0);
            break;
          case 15:
          case 16:
            if ((char_is_bardock(ch) || char_is_kurzak(ch)) && rand_number(1, 2) == 2)
              do_head(ch, buf, 0, 0);
            else if ((char_is_icer(ch) || char_is_bio(ch)) && rand_number(1, 2) == 2)
              do_tailwhip(ch, buf, 0, 0);
            else if (rand_number(1, 8) >= 7)
              do_trip(ch, buf, 0, 0);
            else
              do_roundhouse(ch, buf, 0, 0);
            break;
          case 17:
          case 18:
            do_slam(ch, buf, 0, 0);
            break;
          case 19:
          case 20:
            do_heeldrop(ch, buf, 0, 0);
            break;
        }
      }
    } else {
      mob_specials_used += 1;
      switch (power) {
        case 1:
        case 2:
        case 3:
        case 4:
          if (special > 80)
            do_zanzoken(ch, buf, 0, 0);
          if (ch->mobcharge == 7) {
            ch->mobcharge = 0;
            do_kiball(ch, buf, 0, 0);
          }
          break;
        case 5:
        case 6:
        case 7:
        case 8:
          if (special > 80)
            do_zanzoken(ch, buf, 0, 0);
          if (ch->mobcharge == 7) {
            ch->mobcharge = 0;
            do_kiblast(ch, buf, 0, 0);
          }
          break;
        case 9:
        case 10:
        case 11:
          if (special > 80)
            do_zanzoken(ch, buf, 0, 0);
          if (char_is_dragon(ch) && rand_number(1, 4) == 4) {
            do_breath(ch, buf, 0, 0);
          } else {
            if (ch->mobcharge == 7) {
              ch->mobcharge = 0;
              do_beam(ch, buf, 0, 0);
            }
          }
          break;
        case 12:
        case 13:
        case 14:
          if (special > 80)
            do_zanzoken(ch, buf, 0, 0);
          if (char_is_dragon(ch) && rand_number(1, 4) == 4) {
            do_breath(ch, buf, 0, 0);
          } else {
            if (ch->mobcharge == 7) {
              ch->mobcharge = 0;
              do_renzo(ch, buf, 0, 0);
            }
          }
          break;
        case 15:
        case 16:
          if (char_is_dragon(ch) && rand_number(1, 4) == 4) {
            do_breath(ch, buf, 0, 0);
          } else {
            if (ch->mobcharge == 7) {
              ch->mobcharge = 0;
              do_tsuihidan(ch, buf, 0, 0);
            }
          }
          break;
        case 17:
        case 18:
          if (char_is_dragon(ch) && rand_number(1, 4) == 4) {
            do_breath(ch, buf, 0, 0);
          } else {
            if (ch->mobcharge == 7) {
              ch->mobcharge = 0;
              do_shogekiha(ch, buf, 0, 0);
            }
          }
          break;
        case 19:
        case 20:
          if (char_is_dragon(ch)) {
            do_breath(ch, buf, 0, 0);
          }
          if (ch->mobcharge == 7) {
            ch->mobcharge = 0;
            switch (char_get_class(ch)) {
              case CLASS_ROSHI:
                if (special >= 100)
                  do_kakusanha(ch, buf, 0, 0);
                else if (special >= 80)
                  do_kienzan(ch, buf, 0, 0);
                else if (special >= 70)
                  do_kamehameha(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_FRIEZA:
                if (special >= 100)
                  do_deathball(ch, buf, 0, 0);
                else if (special >= 80)
                  do_kienzan(ch, buf, 0, 0);
                else if (special >= 70)
                  do_deathbeam(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_KRANE:
                if (special >= 100)
                  do_tribeam(ch, buf, 0, 0);
                else if (special >= 80)
                  do_hass(ch, NULL, 0, 0);
                else if (special >= 70)
                  do_dodonpa(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_PICCOLO:
                if (special >= 100)
                  do_scatter(ch, buf, 0, 0);
                else if (special >= 80)
                  do_sbc(ch, buf, 0, 0);
                else if (special >= 70)
                  do_masenko(ch, buf, 0, 0);
                else if (special >= 60)
                  do_balefire(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_BARDOCK:
                if (special >= 100)
                  do_final(ch, buf, 0, 0);
                else if (special >= 80)
                  do_bigbang(ch, buf, 0, 0);
                else if (special >= 70)
                  do_galikgun(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_ANDSIX:
                if (special >= 100)
                  do_hellflash(ch, buf, 0, 0);
                else if (special >= 80)
                  do_kousengan(ch, buf, 0, 0);
                else if (special >= 70)
                  do_dualbeam(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_NAIL:
                if (special >= 100)
                  do_regenerate(ch, (char[]){"50"}, 0, 0);
                else if (special >= 80)
                  do_heal(ch, (char[]){"self"}, 0, 0);
                else if (special >= 70)
                  do_masenko(ch, buf, 0, 0);
                else
                  do_zanzoken(ch, NULL, 0, 0);
                break;
              case CLASS_KURZAK:
                if (special >= 100)
                  do_ensnare(ch, buf, 0, 0);
                else if (special >= 80)
                  do_seishou(ch, buf, 0, 0);
                else if (special >= 70)
                  do_renzo(ch, buf, 0, 0);
                else if (special >= 50)
                  do_barrier(ch, (char[]){"40"}, 0, 0);
                else
                  do_barrier(ch, (char[]){"25"}, 0, 0);
                break;
              case CLASS_JINTO:
                if (special >= 100)
                  do_nova(ch, buf, 0, 0);
                else if (special >= 80)
                  do_breaker(ch, buf, 0, 0);
                else if (special >= 70)
                  do_trip(ch, buf, 0, 0);
                else
                  do_zanzoken(ch, (char[]){"40"}, 0, 0);
                break;
              case CLASS_TSUNA:
                if (special >= 100)
                  do_koteiru(ch, buf, 0, 0);
                else if (special >= 80)
                  do_razor(ch, buf, 0, 0);
                else if (special >= 70)
                  do_spike(ch, buf, 0, 0);
                else
                  do_barrier(ch, (char[]){"20"}, 0, 0);
                break;
              case CLASS_TAPION:
                if (special >= 100)
                  do_pslash(ch, buf, 0, 0);
                else if (special >= 80)
                  do_ddslash(ch, buf, 0, 0);
                else if (special >= 70)
                  do_tslash(ch, buf, 0, 0);
                else
                  do_zanzoken(ch, (char[]){"40"}, 0, 0);
                break;
              case CLASS_KABITO:
                if (special >= 100)
                  do_pbarrage(ch, buf, 0, 0);
                else if (special >= 80)
                  do_psyblast(ch, buf, 0, 0);
                else if (special >= 70)
                  do_heal(ch, buf, 0, 0);
                else
                  do_zanzoken(ch, (char[]){"40"}, 0, 0);
                break;
              case CLASS_DABURA:
                if (special >= 100)
                  do_hellspear(ch, buf, 0, 0);
                else if (special >= 80)
                  do_honoo(ch, buf, 0, 0);
                else if (special >= 70)
                  do_fireshield(ch, buf, 0, 0);
                else
                  do_zanzoken(ch, (char[]){"40"}, 0, 0);
                break;
              case CLASS_GINYU:
                if (special >= 100)
                  do_spiral(ch, buf, 0, 0);
                else if (special >= 80)
                  do_crusher(ch, buf, 0, 0);
                else if (special >= 70)
                  do_eraser(ch, buf, 0, 0);
                else
                  do_zanzoken(ch, (char[]){"40"}, 0, 0);
                break;
            }
          }
          break;
      } /* End power switch */
    } /* End special attacks */
  } else if (!char_is_humanoid(ch) || dragonpass == FALSE) { /* Is not a humanoid mob */
    if (char_is_serpent(ch) && rand_number(1, 5) == 5) {
      do_strike(ch, buf, 0, 0);
    } else if (char_is_dragon(ch) && rand_number(1, 12) >= 10 && char_get_mob_vnum(ch) != 17917) {
      do_breath(ch, buf, 0, 0);
    } else {
      if (rand_number(1, 10) >= 7 && char_get_level(ch) >= 10) {
        do_ram(ch, buf, 0, 0);
      } else {
        do_bite(ch, buf, 0, 0);
      }
    }
  }

  fight_mtrigger(ch);

} /* End mob_attack */

static void cleanup_arena_watch(struct char_data *ch)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {

    if (desc_get_connected(d) != CON_PLAYING)
      continue;

    if (char_prf_flagged(d->character, PRF_ARENAWATCH)) {
      if (char_get_arenawatch(d->character) == char_get_idnum(ch)) {
        char_prf_set(d->character, PRF_ARENAWATCH, false);
        char_set_arenawatch(d->character, -1);
      }
    }
  }
}

static void shadow_dragons_live()
{
  int value = 0;
  if (SHADOW_DRAGON1 != -1 || SHADOW_DRAGON2 != -1 || SHADOW_DRAGON3 != -1 || SHADOW_DRAGON4 != -1 || SHADOW_DRAGON5 != -1 || SHADOW_DRAGON6 != -1 || SHADOW_DRAGON7 != -1) {
    value = 1;
  }

  if (value == 0) {
    SELFISHMETER = 0;
    save_mud_time(&time_info);
  }
}

/* For announcing the sounds of battle to nearby rooms */
void impact_sound(struct char_data *ch, const char *mssg)
{
  int door;
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (char_can_go(ch, door))
      send_to_room(world[char_get_in_room(ch)].dir_option[door]->to_room, "%s", mssg);
}

/* For removing body parts */
void remove_limb(struct char_data *vict, int num)
{
  /* 0 = head, 1 = rarm, 2 = larm, 3 = rleg, 4 = lleg , 5 = tail, 6 = tail*/

  struct obj_data *body_part;
  char part[1000];
  char buf[1000];
  char buf2[1000];

  body_part              = create_obj();
  body_part->item_number = NOTHING;
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
      snprintf(buf, sizeof(buf), "tail");
      break;
    default:
      snprintf(part, sizeof(part), "@w%s body part@n", char_true_race_name(vict));
      snprintf(buf, sizeof(buf), "body part");
      break;
  }

  body_part->name = strdup(buf);
  if (num > 0) {
    snprintf(buf2, sizeof(buf2), "@wA %s is lying here@n", part);
  } else {
    snprintf(buf2, sizeof(buf2), "%s@w is lying here@n", part);
  }
  body_part->description       = strdup(buf2);
  body_part->short_description = strdup(part);

  obj_set_type_flag(body_part, ITEM_OTHER);
  obj_wear_set(body_part, ITEM_WEAR_TAKE, true);
  obj_extra_set(body_part, ITEM_UNIQUE_SAVE, true);
  obj_set_value(body_part, 0, 0);
  obj_set_value(body_part, 1, 0);
  obj_set_value(body_part, 2, 0);
  obj_set_value(body_part, 3, 0);
  obj_set_value(body_part, 4, 1);
  obj_set_value(body_part, 5, 1);
  obj_set_weight(body_part, rand_number(4, 10));
  obj_set_cost_per_day(body_part, 0);
  add_unique_id(body_part);
  obj_to_room(body_part, char_get_in_room(vict));
}

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES] =
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
void fight_stack()
{
  int perc = 0;
  struct char_data *ch;
  struct char_data *tch;
  struct char_data *wch;

  for (tch = character_list; tch; tch = tch->next) {
    ch = tch;

    if (char_get_position(ch) == POS_FIGHTING) {
      char_set_position(ch, POS_STANDING);
    }
    if (char_plr_flagged(ch, PLR_SPIRAL)) {
      handle_spiral(ch, NULL, char_get_skill(ch, SKILL_SPIRAL), FALSE);
    }
    if (char_is_npc(ch) && char_get_cooldown(ch) > 0) {
      char_set_cooldown(ch, char_get_cooldown(ch) - (1));
      if (rand_number(1, 2) == 2 && char_get_cooldown(ch) > 0) {
        char_set_cooldown(ch, char_get_cooldown(ch) - (1));
      }
      if (char_get_cooldown(ch) > 0) {
        continue;
      }
    }
    if (char_is_npc(ch) && char_mob_flagged(ch, MOB_POWERUP) && axion_dice(0) >= 90) {
      if (getCurPL(ch) >= getEffMaxPL(ch)) {
        act("@g$n@ finishes powering up as $s aura flashes brightly filling the entire area briefly with its light!@n", TRUE, ch, 0, 0, TO_ROOM);
        restoreHealth(ch, false);
        char_mob_set(ch, MOB_POWERUP, false);
      } else if (getCurPL(ch) >= getEffMaxPL(ch) / 2) {
        act("@g$n@G continues powering up as torrents of energy crackle within $s aura.@n", TRUE, ch, 0, 0, TO_ROOM);
        incCurHealthPercent(ch, .1, true);
      } else if (getCurPL(ch) > getEffMaxPL(ch) / 4) {
        act("@g$n@G powers up as a steady aura around $s body grow brighter.@n", TRUE, ch, 0, 0, TO_ROOM);
        incCurHealthPercent(ch, .125, true);
      } else if (getCurPL(ch) > 0) {
        act("@g$n@G powers up, as a weak aura flickers around $s body.@n", TRUE, ch, 0, 0, TO_ROOM);
        incCurHealthPercent(ch, .2, true);
      }
    }
    if (char_is_npc(ch) && char_is_affected(ch, AFF_FROZEN)) {
      continue;
    }
    if (!char_get_grappling(ch) && !char_get_grappled(ch) && !char_get_fighting(ch) && !char_plr_flagged(ch, PLR_CHARGE) && !char_plr_flagged(ch, PLR_POWERUP) && char_get_charge(ch) <= 0 && !char_is_transformed(ch)) {
      continue;
    }
    if (char_get_fighting(ch) && (char_get_in_room(char_get_fighting(ch)) != char_get_in_room(ch))) {
      wch = char_get_fighting(ch);
      stop_fighting(wch);
      stop_fighting(ch);
    }
    if (char_get_fighting(ch) && char_get_drag(ch)) {
      act("@WYou are forced to stop dragging @C$N@W!@n", TRUE, ch, 0, char_get_drag(ch), TO_CHAR);
      act("@C$n@W is forced to stop dragging @c$N@W!@n", TRUE, ch, 0, char_get_drag(ch), TO_ROOM);
      char_set_dragged(char_get_drag(ch), NULL);
      char_set_drag(ch, NULL);
    }

    if (char_get_lifeperc(ch) > 0 && ch->health < (double)char_get_lifeperc(ch) / 100 && (getCurLF(ch)) > 0 && !char_is_android(ch)) {
      if (rand_number(1, 15) >= 14) {
        if ((getCurLF(ch)) >= (getMaxLF(ch)) * 0.05 || char_aff_flagged(ch, AFF_HEALGLOW) || (char_is_kanassan(ch) && (getCurLF(ch)) >= (getMaxLF(ch)) * 0.03)) {
          int64_t refill = 0, lfcost = (getMaxLF(ch)) * 0.05;
          if (char_get_bonuses(ch, BONUS_DIEHARD) > 0 && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 2 && char_get_genome(ch, 1) != 2))) {
            refill = (getMaxLF(ch)) * 0.1;
          } else if (char_get_bonuses(ch, BONUS_DIEHARD) > 0 && char_is_mutant(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2)) {
            refill = (getMaxLF(ch)) * 0.17;
          } else if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 2 || char_get_genome(ch, 1) == 2)) {
            refill = (getMaxLF(ch)) * 0.12;
          } else if (char_is_kanassan(ch)) {
            lfcost = (getMaxLF(ch)) * 0.03;
            refill = (getMaxLF(ch)) * 0.03;
          } else {
            refill = (getMaxLF(ch)) * 0.05;
          }
          incCurHealth(ch, refill, true);
          if (!char_aff_flagged(ch, AFF_HEALGLOW)) {
            decCurLF(ch, lfcost, 0);
          }
        } else {
          incCurHealth(ch, (getCurLF(ch)), true);
          decCurLFPercent(ch, 2, -1);
        }

        send_to_char(ch, "@YYour life force has kept you strong@n!\r\n");
      }
    }

    if (!char_aff_flagged(ch, AFF_POSITION)) {
      if (roll_balance(ch) > axion_dice(0) && rand_number(1, 10) >= 7) {
        if (char_get_fighting(ch)) {
          if (!char_aff_flagged(char_get_fighting(ch), AFF_POSITION)) {
            act("@YYou manage to move into an advantageous position!@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@y$n@Y manages to move into an advantageous position!@n", TRUE, ch, 0, 0, TO_ROOM);
            char_aff_set(ch, AFF_POSITION, true);
          } else {
            struct char_data *vict = char_get_fighting(ch);
            if (roll_balance(ch) > roll_balance(vict)) {
              act("@YYou struggle to gain a better position than @y$N@Y and succeed!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@y$n@Y struggles to gain a better position than you and succeeds!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@y$n@Y struggles to gain a better position than @y$N@Y and succeeds!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_aff_set(vict, AFF_POSITION, false);
              char_aff_set(ch, AFF_POSITION, true);
            }
          }
        }
      }
    } else {
      if (roll_balance(ch) < axion_dice(-30) || char_get_position(ch) < POS_STANDING) {
        act("@YYou are moved out of your position!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@y$n@Y is moved out of $s position!@n", TRUE, ch, 0, 0, TO_ROOM);
        char_aff_set(ch, AFF_POSITION, false);
      }
    }
    if (char_get_grappling(ch) && char_get_grap(ch) == 2 && rand_number(1, 11) >= 8) {
      if ((getCurST((ch)->grappling)) >= getMaxST(char_get_grappling(ch)) / 8) {
        act("@WYou choke @C$N@W!@n", TRUE, ch, 0, char_get_grappling(ch), TO_CHAR);
        act("@C$n@W chokes YOU@W!@n", TRUE, ch, 0, char_get_grappling(ch), TO_VICT);
        act("@C$n@W chokes @c$N@W!@n", TRUE, ch, 0, char_get_grappling(ch), TO_NOTVICT);
        decCurST(char_get_grappling(ch), (getMaxST(char_get_grappling(ch)) / 8), 0);
      } else {
        act("@WYou choke @C$N@W, and $E passes out!@n", TRUE, ch, 0, char_get_grappling(ch), TO_CHAR);
        act("@C$n@W chokes YOU@W, and you pass out!@n", TRUE, ch, 0, char_get_grappling(ch), TO_VICT);
        act("@C$n@W chokes @c$N@W, and $E passes out!@n", TRUE, ch, 0, char_get_grappling(ch), TO_NOTVICT);
        char_aff_set(char_get_grappling(ch), AFF_KNOCKED, true);
        char_set_position(char_get_grappling(ch), POS_SLEEPING);
        char_set_grap(char_get_grappling(ch), -1);
        char_set_grappled(char_get_grappling(ch), NULL);
        char_set_grappling(ch, NULL);
        char_set_grap(ch, -1);
      }
    } else if (char_get_grappling(ch) && char_get_grap(ch) == 4 && rand_number(1, 12) >= 8) {
      act("@WYou crush @C$N@W some more!@n", TRUE, ch, 0, char_get_grappling(ch), TO_CHAR);
      act("@C$n@W crushes YOU@W some more!@n", TRUE, ch, 0, char_get_grappling(ch), TO_VICT);
      act("@C$n@W crushes @c$N@W some more!@n", TRUE, ch, 0, char_get_grappling(ch), TO_NOTVICT);
      int64_t damg = char_get_aff_abils_str(ch) * (10 + (getEffMaxPL(ch) * 0.005));
      hurt(0, 0, ch, char_get_grappling(ch), NULL, damg, 0);
    }
    if (char_get_grappled(ch) && rand_number(1, 2) == 2) {
      send_to_char(ch, "@CTry 'escape' to break free from the hold!@n\r\n");
    }
    if (char_is_halfbreed(ch) && char_plr_flagged(ch, PLR_FURY)) {
      char_set_rage_meter(ch, char_get_rage_meter(ch) + (1));
      if (char_get_rage_meter(ch) >= 1000) {

        incCurHealthPercent(ch, .15, true);
        incCurKIPercent(ch, .15, true);
        incCurSTPercent(ch, .15, true);
        send_to_char(ch, "Your fury has called forth more of your hidden power and you feel better!\r\n");
      }
    }

    if (!char_is_npc(ch) && char_is_transformed(ch) && !char_is_icer(ch) && char_is_nonptrans(ch)) {
      int tier = race_current_trans_tier(ch);

      if (getCurST(ch) < getMaxST(ch) / 60) {
        if (!(tier == 1 && char_plr_flagged(ch, PLR_FPSSJ))) {
          act("@mExhausted of stamina, your body forcibly reverts from its form.@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@C$n @wbreathing heavily, reverts from $s form, returning to normal.@n", TRUE, ch, 0, 0, TO_ROOM);
          if (char_get_kaioken(ch) < 1)
            do_kaioken(ch, (char[]){"0"}, 0, 0);
          do_transform(ch, (char[]){"revert"}, 0, 0);
        }
      }

      if (getCurST(ch) >= getMaxST(ch) / 800 && char_plr_flagged(ch, PLR_TRANS1)) {
        if (!char_plr_flagged(ch, PLR_FPSSJ)) {
          if (char_is_saiyan(ch) && (getCurLF(ch)) >= (getMaxLF(ch)) * 0.7) {
            decCurST(ch, getMaxST(ch) / 900, 0);
          } else
            decCurST(ch, getMaxST(ch) / 800, 0);
        }
      } else if (getCurST(ch) >= getMaxST(ch) / 600 && char_plr_flagged(ch, PLR_TRANS2) && !char_is_konatsu(ch) && !char_is_kai(ch) && !char_is_namek(ch)) {
        if (char_is_saiyan(ch) && (getCurLF(ch)) >= (getMaxLF(ch)) * 0.7) {
          decCurST(ch, getMaxST(ch) / 700, 0);
        } else
          decCurST(ch, getMaxST(ch) / 600, 0);
      } else if (getCurST(ch) >= getMaxST(ch) / 500 && char_plr_flagged(ch, PLR_TRANS2)) {
        decCurST(ch, getMaxST(ch) / 500, 0);
      } else if (getCurST(ch) >= getMaxST(ch) / 400 && char_plr_flagged(ch, PLR_TRANS3) && !char_is_saiyan(ch)) {
        decCurST(ch, getMaxST(ch) / 400, 0);
      } else if (getCurST(ch) >= getMaxST(ch) / 250 && char_plr_flagged(ch, PLR_TRANS3)) {
        if (char_is_saiyan(ch) && (getCurLF(ch)) >= (getMaxLF(ch)) * 0.7) {
          decCurST(ch, getMaxST(ch) / 300, 0);
        } else
          decCurST(ch, getMaxST(ch) / 250, 0);
      } else if (getCurST(ch) >= getMaxST(ch) / 200 && char_plr_flagged(ch, PLR_TRANS4) && !char_is_saiyan(ch)) {
        decCurST(ch, getMaxST(ch) / 200, 0);
      } else if (getCurST(ch) >= getMaxST(ch) / 170 && char_plr_flagged(ch, PLR_TRANS4)) {
        if (char_is_saiyan(ch) && (getCurLF(ch)) >= (getMaxLF(ch)) * 0.7) {
          decCurST(ch, getMaxST(ch) / 240, 0);
        } else
          decCurST(ch, getMaxST(ch) / 170, 0);
      }
    }

    if (!char_is_npc(ch) && char_get_wimp_level(ch) && getCurPL(ch) < char_get_wimp_level(ch) && getCurPL(ch) > 0 && char_get_fighting(ch)) {
      send_to_char(ch, "You wimp out, and attempt to flee!\r\n");
      do_flee(ch, NULL, 0, 0);
    }
    if (char_is_npc(ch) && getCurPL(ch) < getEffMaxPL(ch) / 10 && getCurPL(ch) > 0 && char_get_fighting(ch) && !char_mob_flagged(ch, MOB_SENTINEL)) {
      if (rand_number(1, 30) >= 25 && char_get_position(ch) > POS_SITTING) {
        do_flee(ch, NULL, 0, 0);
      }
    }
    if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 6 || char_get_genome(ch, 1) == 6) && rand_number(1, 200) >= 175) {
      mutant_limb_regen(ch);
    }
    if (!char_is_npc(ch) && char_plr_flagged(ch, PLR_DISGUISED) && char_get_skill(ch, SKILL_DISGUISE) < rand_number(1, 125)) {
      send_to_char(ch, "Your disguise comes off because of your swift movements!\r\n");
      char_plr_set(ch, PLR_DISGUISED, false);
      act("@W$n's@W disguise comes off because of $s swift movements!@n", FALSE, ch, 0, 0, TO_ROOM);
    }
    if (char_is_npc(ch) && char_aff_flagged(ch, AFF_BLIND) && rand_number(1, 200) >= 190) {
      act("@W$n@W is no longer blind.@n", FALSE, ch, 0, 0, TO_ROOM);
      char_aff_set(ch, AFF_BLIND, false);
    }

    if (char_aff_flagged(ch, AFF_KNOCKED) && rand_number(1, 200) >= 195) {
      cureStatusKnockedOut(ch, true);
      if (char_is_npc(ch) && rand_number(1, 20) >= 12) {
        act("@W$n@W stands up.@n", FALSE, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_STANDING);
      }
    }

    if (!char_is_npc(ch) && !(ch->desc) && char_get_position(ch) > POS_STUNNED && !char_is_affected(ch, AFF_FROZEN)) {
      if (char_get_fighting(ch)) {
        do_flee(ch, NULL, 0, 0);
      }
    }
    /* Mobile Defense System */
    if (char_is_npc(ch) && char_get_grappled(ch) && !char_mob_flagged(ch, MOB_DUMMY) && rand_number(1, 5) >= 4) {
      do_escape(ch, 0, 0, 0);
      continue;
    }
    if (char_get_fighting(ch) && char_is_npc(ch) && !char_mob_flagged(ch, MOB_DUMMY)) {
      if (char_aff_flagged(char_get_fighting(ch), AFF_FLYING) && !char_aff_flagged(ch, AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) > 10) {
        do_fly(ch, 0, 0, 0);
        continue;
      }
      if (!char_aff_flagged(char_get_fighting(ch), AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING)) {
        do_fly(ch, 0, 0, 0);
        continue;
      }
      if (char_aff_flagged(char_get_fighting(ch), AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING) && char_get_altitude(ch) < char_get_altitude(char_get_fighting(ch))) {
        do_fly(ch, (char[]){"high"}, 0, 0);
        continue;
      }
      if (char_aff_flagged(char_get_fighting(ch), AFF_FLYING) && !char_is_humanoid(ch) && !char_aff_flagged(ch, AFF_FLYING) && char_get_position(ch) > POS_RESTING) {
        if (rand_number(1, 30) >= 22 && !block_calc(ch)) {
          act("$n@G flees in terror and you lose sight of $m!", TRUE, ch, 0, 0, TO_ROOM);
          while (ch->carrying)
            extract_obj(ch->carrying);

          extract_char(ch);
          continue;
        }
      }
      if (char_aff_flagged(char_get_fighting(ch), AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) <= 10) {
        if (rand_number(1, 30) >= 22 && !block_calc(ch)) {
          act("$n@G turns and runs away. You lose sight of $m!", TRUE, ch, 0, 0, TO_ROOM);
          while (ch->carrying)
            extract_obj(ch->carrying);
          extract_char(ch);
          continue;
        }
      }
      if (char_get_position(ch) == POS_SITTING && sec_roll_check(ch) == 1) {
        do_stand(ch, 0, 0, 0);
        continue;
      }
      if (char_get_position(ch) == POS_RESTING && sec_roll_check(ch) == 1) {
        do_stand(ch, 0, 0, 0);
        continue;
      }
      if (char_is_affected(ch, AFF_PARA) && char_is_npc(ch) && char_get_aff_abils_intel(ch) + 10 < rand_number(1, 60)) {
        act("@yYou fail to overcome your paralysis!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@Y$n @ystruggles with $s paralysis!@n", TRUE, ch, 0, 0, TO_ROOM);
        continue;
      }
      if (char_get_position(ch) == POS_SLEEPING && !char_aff_flagged(ch, AFF_KNOCKED) && sec_roll_check(ch) == 1) {
        do_wake(ch, 0, 0, 0);
        do_stand(ch, 0, 0, 0);
        continue;
      }
      struct char_data *vict;
      char buf[100];

      vict = char_get_fighting(ch);
      sprintf(buf, "%s", char_get_name(vict));
      if (char_get_in_room(ch) == char_get_in_room(vict) && !char_mob_flagged(ch, MOB_DUMMY) && !char_aff_flagged(ch, AFF_KNOCKED) && char_get_position(ch) != POS_SITTING && char_get_position(ch) != POS_RESTING && char_get_position(ch) != POS_SLEEPING) {

        if (char_is_npc(ch) && rand_number(1, 30) <= 12)
          continue;

        mob_attack(ch, buf);

      } //end if
      else {
        continue;
      }
    }
    if (char_get_position(ch) <= POS_RESTING && char_plr_flagged(ch, PLR_POWERUP)) {
      char_plr_set(ch, PLR_POWERUP, false);
    }

    if (char_get_barrier(ch) > 0) {
      improve_skill(ch, SKILL_BARRIER, 0);
    }

    if (char_plr_flagged(ch, PLR_POWERUP) && rand_number(1, 3) == 3) {
      char buf3[MAX_STRING_LENGTH];
      int64_t ghit    = getCurPL(ch);
      int64_t gmaxhit = getEffMaxPL(ch);
      int64_t gki     = getCurKI(ch);
      int64_t gmaxki  = getMaxKI(ch);

      if (ghit >= gmaxhit && gki >= (gmaxki / 20) && char_get_preference(ch) != PREFERENCE_KI) {
        if (gki >= gmaxki * 0.5) {
          int64_t raise = getMaxST(ch) * 0.02;
          incCurST(ch, raise, true);
        }
        restoreHealth(ch, false);
        decCurKI(ch, getMaxKI(ch) / 20, 0);
        dispel_ash(ch);
        act("@RYou have reached your maximum!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n stops powering up in a flash of light!@n", TRUE, ch, 0, 0, TO_ROOM);
        send_to_sense(0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(ch)));
        send_to_scouter(buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      } else if (ghit >= gmaxhit && gki >= (gmaxki * 0.0375) + 1 && char_get_preference(ch) == PREFERENCE_KI) {
        if (gki >= (gmaxki * 0.0375) + 1) {
          int64_t raise = getMaxST(ch) * 0.02;
          incCurST(ch, raise, true);
        }
        restoreHealth(ch, false);
        decCurKI(ch, (gmaxki * 0.0375) + 1, 0);
        dispel_ash(ch);
        act("@RYou have reached your maximum!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n stops powering up in a flash of light!@n", TRUE, ch, 0, 0, TO_ROOM);
        send_to_sense(0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(ch)));
        send_to_scouter(buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      }

      ghit    = getCurPL(ch);
      gmaxhit = getEffMaxPL(ch);
      gki     = getCurKI(ch);
      gmaxki  = getMaxKI(ch);

      if (gki < (gmaxki / 20) && char_get_preference(ch) != PREFERENCE_KI) {
        decCurKI(ch, gmaxki / 20, 0);
        act("@RYou have run out of ki.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n stops powering up in a flash of light!@n", TRUE, ch, 0, 0, TO_ROOM);
        send_to_sense(0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(ch)));
        send_to_scouter(buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      } else if (gki < (gmaxki * 0.0375) + 1 && char_get_preference(ch) == PREFERENCE_KI) {
        decCurKI(ch, (gmaxki * 0.0375) + 1, 0);
        act("@RYou have run out of ki.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n stops powering up in a flash of light!@n", TRUE, ch, 0, 0, TO_ROOM);
        send_to_sense(0, "You sense someone stop powering up", ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Final@D: [@Y%s@D]", add_commas(getCurPL(ch)));
        send_to_scouter(buf3, ch, 1, 0);
        char_plr_set(ch, PLR_POWERUP, false);
      }

      ghit    = getCurPL(ch);
      gmaxhit = getEffMaxPL(ch);
      gki     = getCurKI(ch);
      gmaxki  = getMaxKI(ch);

      if (ghit < gmaxhit && ((char_get_preference(ch) != PREFERENCE_KI &&
                              (gki) >= gmaxki / 20) ||
                             (char_get_preference(ch) == PREFERENCE_KI &&
                              (gki) >= (gmaxki * 0.0375) + 1))) {
        incCurHealthPercent(ch, .1, true);
        if (char_get_preference(ch) != PREFERENCE_KI) {
          decCurKI(ch, gmaxki / 20, 0);
        } else {
          decCurKI(ch, gmaxki * .0375, 0);
        }
        if ((getCurKI(ch)) >= getMaxKI(ch) * 0.5) {
          int64_t raise = getMaxST(ch) * 0.02;
          incCurST(ch, raise, true);
        }

        gmaxhit = getEffMaxPL(ch);

        if (gmaxhit < 50000) {
          act("@RYou continue to powerup, as wind billows out from around you!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as wind billows out from around $m!@n", TRUE, ch, 0, 0, TO_ROOM);
        } else if (gmaxhit < 500000) {
          act("@RYou continue to powerup, as the ground splits beneath you!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as the ground splits beneath $m!@n", TRUE, ch, 0, 0, TO_ROOM);
        } else if (gmaxhit < 5000000) {
          act("@RYou continue to powerup, as the ground shudders and splits beneath you!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as the ground shudders and splits beneath $m!@n", TRUE, ch, 0, 0, TO_ROOM);
        } else if (gmaxhit < 50000000) {
          act("@RYou continue to powerup, as a huge depression forms beneath you!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as a huge depression forms beneath $m!@n", TRUE, ch, 0, 0, TO_ROOM);
        } else if (gmaxhit < 100000000) {
          act("@RYou continue to powerup, as the entire area quakes around you!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as the entire area quakes around $m!@n", TRUE, ch, 0, 0, TO_ROOM);
        } else if (gmaxhit < 300000000) {
          act("@RYou continue to powerup, as huge chunks of ground are ripped apart beneath you!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as huge chunks of ground are ripped apart beanth $m!@n", TRUE, ch, 0, 0, TO_ROOM);
        } else {
          act("@RYou continue to powerup, as the very air around you crackles and burns!@n", TRUE, ch, 0, 0, TO_CHAR);
          act("@R$n continues to powerup, as the very air around $m crackles and burns!@n", TRUE, ch, 0, 0, TO_ROOM);
        }
        send_to_sense(0, "You sense someone powering up", ch);
        send_to_worlds(ch);
        sprintf(buf3, "@D[@GBlip@D]@r Rising Powerlevel Detected@D: [@Y%s@D]", add_commas(getCurPL(ch)));
        send_to_scouter(buf3, ch, 1, 0);
        dispel_ash(ch);
      }
    }
    if ((char_get_position(ch) == POS_SLEEPING || char_get_position(ch) == POS_RESTING) && (char_plr_flagged(ch, PLR_CHARGE) || char_get_charge(ch) >= 1)) {
      send_to_char(ch, "You stop charging and release all your pent up energy!\r\n");
      switch (rand_number(1, 3)) {
        case 1:
          act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("$n@w's aura fades.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("$n@w's aura flickers brightly before disappearing.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        default:
          act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      char_plr_set(ch, PLR_CHARGE, false);
      incCurKI(ch, char_get_charge(ch), true);
      char_set_charge(ch, 0);
      char_set_chargeto(ch, 0);
    }
    if (char_plr_flagged(ch, PLR_CHARGE) && char_get_bonuses(ch, BONUS_UNFOCUSED) > 0 && rand_number(1, 80) >= 70) {
      send_to_char(ch, "You lose concentration due to your unfocused mind and release your charged energy!\r\n");
      switch (rand_number(1, 3)) {
        case 1:
          act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 2:
          act("$n@w's aura fades.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        case 3:
          act("$n@w's aura flickers brightly before disappearing.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
        default:
          act("$n@w's aura disappears.@n", TRUE, ch, 0, 0, TO_ROOM);
          break;
      }
      char_plr_set(ch, PLR_CHARGE, false);
      incCurKI(ch, char_get_charge(ch), true);
      char_set_charge(ch, 0);
      char_set_chargeto(ch, 0);
    }
    if (char_get_charge(ch) >= getMaxKI(ch) / 2) {
      improve_skill(ch, SKILL_CONCENTRATION, 1);
    }
    if (!char_plr_flagged(ch, PLR_CHARGE) && rand_number(1, 40) >= 38 && !char_get_fighting(ch) && (char_get_preference(ch) != PREFERENCE_KI || char_get_charge(ch) > getMaxKI(ch) * 0.1)) {
      if (char_get_charge(ch) >= getMaxKI(ch) / 100) {
        int64_t loss = 0;
        send_to_char(ch, "You lose some of your energy slowly.\r\n");
        switch (rand_number(1, 3)) {
          case 1:
            act("$n@w's aura flickers weakly.@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 2:
            act("$n@w's aura sheds energy.@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 3:
            act("$n@w's aura flickers brightly before growing dimmer.@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
          default:
            act("$n@w's aura shrinks some.@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
        }
        loss = char_get_charge(ch) / 20;
        char_set_charge(ch, char_get_charge(ch) - (loss));
      } else if (char_get_charge(ch) < getMaxKI(ch) / 100 && char_get_charge(ch) != 0) {
        send_to_char(ch, "Your charged energy is completely gone as your aura fades.\r\n");
        act("$n@w's aura fades away dimmly.@n", TRUE, ch, 0, 0, TO_ROOM);
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
      if ((getCurKI(ch)) <= 0) {
        send_to_char(ch, "You can not charge anymore, you have charged all your energy!\r\n");
        act("$n@w's aura grows calm.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_plr_set(ch, PLR_CHARGE, false);
      } else if (((getMaxKI(ch) * 0.01) * perc) >= (getCurKI(ch))) {
        send_to_char(ch, "You have charged the last that you can.\r\n");
        act("$n@w's aura @Yflashes@w spectacularly, rushing upwards in torrents!@n", TRUE, ch, 0, 0, TO_ROOM);
        char_set_charge(ch, char_get_charge(ch) + ((getCurKI(ch))));
        decCurKIPercent(ch, 1, 0);
        char_set_chargeto(ch, 0);
        char_plr_set(ch, PLR_CHARGE, false);
      } else {
        if (char_get_charge(ch) >= char_get_chargeto(ch)) {
          send_to_char(ch, "You have already reached the maximum that you wished to charge.\r\n");
          act("$n@w's aura burns steadily.@n", TRUE, ch, 0, 0, TO_ROOM);
          char_set_chargeto(ch, 0);
          char_plr_set(ch, PLR_CHARGE, false);
        } else if (char_get_charge(ch) + (((getMaxKI(ch) * 0.01) * perc) + 1) >= char_get_chargeto(ch)) {
          decCurKI(ch, char_get_chargeto(ch) - char_get_charge(ch), 0);
          char_set_charge(ch, char_get_chargeto(ch));
          send_to_char(ch, "You stop charging as you reach the maximum that you wished to charge.\r\n");
          act("$n@w's aura flares up brightly and then burns steadily.@n", TRUE, ch, 0, 0, TO_ROOM);
          char_set_chargeto(ch, 0);
          char_plr_set(ch, PLR_CHARGE, false);
        } else {
          decCurKI(ch, ((getMaxKI(ch) * 0.01) * perc) + 1, 0);
          char_set_charge(ch, char_get_charge(ch) + (((getMaxKI(ch) * 0.01) * perc) + 1));
          switch (rand_number(1, 3)) {
            case 1:
              act("$n@w's aura ripples magnificantly while growing brighter!@n", TRUE, ch, 0, 0, TO_ROOM);
              send_to_char(ch, "Your aura grows bright as you charge more ki.\r\n");
              break;
            case 2:
              act("$n@w's aura ripples with power as it grows larger!@n", TRUE, ch, 0, 0, TO_ROOM);
              send_to_char(ch, "Your aura ripples with power as you charge more ki.\r\n");
              break;
            case 3:
              act("$n@w's aura throws sparks off violently!.@n", TRUE, ch, 0, 0, TO_ROOM);
              send_to_char(ch, "Your aura throws sparks off violently as you charge more ki.\r\n");
              break;
            default:
              break;
          }
          if (char_get_charge(ch) >= char_get_chargeto(ch)) {
            char_set_charge(ch, char_get_chargeto(ch));
            char_set_charge(ch, char_get_charge(ch) + (char_get_level(ch)));
            send_to_char(ch, "You have finished charging!\r\n");
            act("$n@w's aura burns brightly and then evens out.@n", TRUE, ch, 0, 0, TO_ROOM);
            char_plr_set(ch, PLR_CHARGE, false);
            char_set_chargeto(ch, 0);
          }
        }
        if (char_get_skill(ch, SKILL_CONCENTRATION)) {
          improve_skill(ch, SKILL_CONCENTRATION, 1);
        }
      }
    }
  }
}

void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  if (char_aff_flagged(ch, AFF_INVISIBLE))
    char_aff_set(ch, AFF_INVISIBLE, false);

  if (char_aff_flagged(ch, AFF_HIDE))
    char_aff_set(ch, AFF_HIDE, false);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
}

void update_pos(struct char_data *victim)
{
  if (char_aff_flagged(victim, AFF_KNOCKED)) {
    return;
  }
  if ((getCurPL(victim) > 0) && (char_get_position(victim) > POS_STUNNED))
    return;
  else if (char_get_position(victim) == POS_SITTING && char_get_fighting(victim))
    return;
  else if (char_get_position(victim) == POS_SITTING && char_get_fighting(victim))
    return;
  else if (getCurPL(victim) > 0)
    char_set_position(victim, POS_STANDING);
  else if (getCurPL(victim) <= -11)
    char_set_position(victim, POS_DEAD);
  else if (getCurPL(victim) <= -6)
    char_set_position(victim, POS_MORTALLYW);
  else if (getCurPL(victim) <= -3)
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
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (char_get_fighting(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list       = ch;

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
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  if (char_is_npc(ch)) {
    char_set_combo(ch, -1);
    char_set_combhits(ch, 0);
  }
  REMOVE_FROM_LIST(ch, combat_list, next_fighting, temp);
  ch->next_fighting = NULL;
  char_set_fighting(ch, NULL);
  if (char_aff_flagged(ch, AFF_POSITION)) {
    char_aff_set(ch, AFF_POSITION, false);
  }
  update_pos(ch);
}

static void make_pcorpse(struct char_data *ch)
{

  struct obj_data *corpse;
  struct obj_data *money;
  int x, y;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  obj_set_in_room(corpse, NOWHERE);

  /* This handles how the corpse is viewed - Iovan */
  handle_corpse_condition(corpse, ch);

  if (char_aff_flagged(ch, AFF_ASHED)) {
    act("@WSome ashes fall off the corpse.@n", TRUE, ch, 0, 0, TO_ROOM);
    struct obj_data *ashes;
    if (rand_number(1, 3) == 2) {
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
    } else if (rand_number(1, 2) == 2) {
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
    } else {
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
    }
  }

  obj_set_type_flag(corpse, ITEM_CONTAINER);
  obj_set_size(corpse, get_size(ch));
  for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      obj_get_extra_flags(corpse)[x] = 0;
    if (y < TW_ARRAY_MAX)
      corpse->wear_flags[y] = 0;
  }

  obj_wear_set(corpse, ITEM_WEAR_TAKE, true);
  obj_extra_set(corpse, ITEM_NODONATE, true);
  obj_set_value(corpse, VAL_CONTAINER_CAPACITY, 0);                  /* You can't store stuff in a corpse */
  obj_set_value(corpse, VAL_CONTAINER_CORPSE, 1);                    /* corpse identifier */
  obj_set_value(corpse, VAL_CONTAINER_OWNER, char_get_pfilepos(ch)); /* corpse identifier */
  obj_set_weight(corpse, char_get_pc_weight(ch) + char_get_carry_weight(ch));
  obj_set_cost_per_day(corpse, 100000);
  obj_set_timer(corpse, CONFIG_MAX_PC_CORPSE_TIME);
  obj_extra_set(corpse, ITEM_UNIQUE_SAVE, true);

  struct obj_data *obj, *next_obj;

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;

    if (obj && obj_get_vnum(obj) < 19900 && obj_get_vnum(obj) != 17998) {
      if ((obj_get_vnum(obj) >= 18800 && obj_get_vnum(obj) <= 18999) || (obj_get_vnum(obj) >= 19100 && obj_get_vnum(obj) <= 19199)) {
        continue;
      } else {
        obj_from_char(obj);
        obj_to_obj(obj, corpse);
        continue;
      }
    } else {
      continue;
    }
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
    if (char_is_npc(ch) || ch->desc) {
      money = create_money(char_get_gold(ch));
      obj_to_obj(money, corpse);
    }
    char_set_gold(ch, 0);
  }

  obj_to_room(corpse, char_get_in_room(ch));
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
      *buf2 = '\0';
      snprintf(buf2, sizeof(buf2), "headless corpse %s", char_get_name(ch));
      corpse->name = strdup(buf2);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The headless corpse of %s is lying here", char_get_name(ch));
      corpse->description = strdup(descBuf);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The headless remains of %s's corpse", char_get_name(ch));
      corpse->short_description = strdup(descBuf);
      obj_set_value(corpse, VAL_CORPSE_HEAD, 0);
      break;
    case DTYPE_HALF:
      *buf2 = '\0';
      snprintf(buf2, sizeof(buf2), "half corpse %s", char_get_name(ch));
      corpse->name = strdup(buf2);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "Half of %s's corpse is lying here", char_get_name(ch));
      corpse->description = strdup(descBuf);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "Half of %s's corpse", char_get_name(ch));
      corpse->short_description = strdup(descBuf);
      break;
    case DTYPE_VAPOR:
      *buf2 = '\0';
      snprintf(buf2, sizeof(buf2), "burnt chunks corpse %s", char_get_name(ch));
      corpse->name = strdup(buf2);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The burnt chunks of %s's corpse are scattered here", char_get_name(ch));
      corpse->description = strdup(descBuf);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The burnt chunks of %s's corpse", char_get_name(ch));
      corpse->short_description = strdup(descBuf);
      break;
    case DTYPE_PULP:
      *buf2 = '\0';
      snprintf(buf2, sizeof(buf2), "beaten bloody corpse %s", char_get_name(ch));
      corpse->name = strdup(buf2);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The bloody and beaten corpse of %s is lying here", char_get_name(ch));
      corpse->description = strdup(descBuf);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The bloody and beaten remains of %s's corpse", char_get_name(ch));
      corpse->short_description = strdup(descBuf);
      break;
    default:
      snprintf(buf2, sizeof(buf2), "corpse %s", char_get_name(ch));
      corpse->name = strdup(buf2);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "The corpse of %s is lying here", char_get_name(ch));
      corpse->description = strdup(descBuf);

      *descBuf = '\0';
      snprintf(descBuf, sizeof(descBuf), "the remains of %s's corpse", char_get_name(ch));
      corpse->short_description = strdup(descBuf);
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

static void make_corpse(struct char_data *ch, struct char_data *tch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  struct obj_data *obj, *next_obj, *meat;
  int i, x, y;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  obj_set_in_room(corpse, NOWHERE);

  /* This handles how the corpse is viewed - Iovan */
  handle_corpse_condition(corpse, ch);

  if (char_aff_flagged(ch, AFF_ASHED)) {
    act("@WSome ashes fall off the corpse.@n", TRUE, ch, 0, 0, TO_ROOM);
    struct obj_data *ashes;
    if (rand_number(1, 3) == 2) {
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
    } else if (rand_number(1, 2) == 2) {
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
    } else {
      ashes = read_object(1305, VIRTUAL);
      obj_to_room(ashes, char_get_in_room(ch));
    }
  }

  /* Let's have a chance to give animals meat */
  if (tch != NULL) {
    if (!char_is_npc(tch) && char_get_skill(tch, SKILL_SURVIVAL)) {
      int skill = char_get_skill(tch, SKILL_SURVIVAL);
      if (!char_is_humanoid(ch) && char_prf_flagged(tch, PRF_CARVE) && axion_dice(0) < skill) {
        send_to_char(tch, "The choice edible meat is preserved because of your skill.\r\n");
        meat = read_object(1612, VIRTUAL);
        obj_to_char(meat, ch);
        char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH], nick3[MAX_INPUT_LENGTH];
        sprintf(nick, "@RRaw %s@R Steak@n", char_get_name(ch));
        sprintf(nick2, "Raw %s Steak", ch->name);
        sprintf(nick3, "@wA @Rraw %s@R steak@w is lying here@n", char_get_name(ch));
        meat->short_description = strdup(nick);
        meat->name              = strdup(nick2);
        meat->description       = strdup(nick3);
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
      corpse->wear_flags[y] = 0;
  }
  obj_wear_set(corpse, ITEM_WEAR_TAKE, true);
  obj_extra_set(corpse, ITEM_NODONATE, true);
  obj_set_value(corpse, VAL_CONTAINER_CAPACITY, 0);                  /* You can't store stuff in a corpse */
  obj_set_value(corpse, VAL_CONTAINER_CORPSE, 1);                    /* corpse identifier */
  obj_set_value(corpse, VAL_CONTAINER_OWNER, char_get_pfilepos(ch)); /* corpse identifier */
  obj_set_weight(corpse, char_get_pc_weight(ch) + char_get_carry_weight(ch));
  obj_set_cost_per_day(corpse, 100000);
  if (char_is_npc(ch))
    obj_set_timer(corpse, CONFIG_MAX_NPC_CORPSE_TIME);
  else
    obj_set_timer(corpse, rand_number(CONFIG_MAX_PC_CORPSE_TIME / 2, CONFIG_MAX_PC_CORPSE_TIME));
  obj_extra_set(corpse, ITEM_UNIQUE_SAVE, true);

  if (char_mob_flagged(ch, MOB_HUSK)) {
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      obj_from_char(obj);
      extract_obj(obj);
    }
  }

  if (!char_mob_flagged(ch, MOB_HUSK)) {
    /* transfer character's inventory to the corpse */
    corpse->contains = ch->carrying;
    for (o = corpse->contains; o != NULL; o = o->next_content) {
      o->in_obj = corpse;
    }
    object_list_new_owner(corpse, NULL);

    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++)
      if (char_get_equipment(ch, i)) {
        remove_otrigger(char_get_equipment(ch, i), ch);
        obj_to_obj(unequip_char(ch, i), corpse);
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
    if (char_is_npc(ch) || ch->desc) {
      money = create_money(char_get_gold(ch));
      obj_to_obj(money, corpse);
    }
    char_set_gold(ch, 0);
  }
  if (!char_mob_flagged(ch, MOB_HUSK)) {
    ch->carrying = NULL;
    char_set_carry_items(ch, 0);
    char_set_carry_weight(ch, 0);
  }
  obj_to_room(corpse, char_get_in_room(ch));

  if (!char_is_npc(ch))
    Crash_rentsave(ch, 0);
}

static void loadmap(struct char_data *ch)
{
  struct obj_data *obj;
  if (!char_is_npc(ch)) {
    obj = read_object(17, VIRTUAL);
    obj_to_char(obj, ch);
  }
}

/* When ch kills victim */

void death_cry(struct char_data *ch)
{
  int door;
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (char_can_go(ch, door))
      send_to_room(world[char_get_in_room(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}

/* Let's clean up necessary things after "death" */
static void final_combat_resolve(struct char_data *ch)
{
  struct obj_data *chair;

  if (char_get_sits(ch)) {
    chair = char_get_sits(ch);
    char_set_sits(ch, NULL);
    obj_set_sitting(chair, NULL);
  }
  if (!char_is_npc(ch) && char_get_clones(ch) > 0) {
    struct char_data *clone = NULL;
    for (clone = character_list; clone; clone = clone->next) {
      if (char_is_npc(clone)) {
        if (char_get_mob_vnum(clone) == 25) {
          if (char_get_original(clone) == ch) {
            handle_multi_merge(clone);
          }
        }
      }
    }
  }
  if (char_get_carrying(ch)) {
    carry_drop(ch, 2);
  }
  if (char_get_carried_by(ch)) {
    carry_drop(char_get_carried_by(ch), 2);
  }
  if (char_get_drag(ch)) {
    char_set_dragged(char_get_drag(ch), NULL);
    char_set_drag(ch, NULL);
  }
  if (char_get_dragged(ch)) {
    char_set_drag(char_get_dragged(ch), NULL);
    char_set_dragged(ch, NULL);
  }
  if (char_get_grappling(ch)) {
    char_set_grap(char_get_grappling(ch), -1);
    char_set_grappled(char_get_grappling(ch), NULL);
    char_set_grappling(ch, NULL);
    char_set_grap(ch, -1);
  }
  if (char_get_grappled(ch)) {
    char_set_grap(char_get_grappled(ch), -1);
    char_set_grappling(char_get_grappled(ch), NULL);
    char_set_grappled(ch, NULL);
    char_set_grap(ch, -1);
  }
  if (char_get_blocked(ch)) {
    char_set_blocks(char_get_blocked(ch), NULL);
    char_set_blocked(ch, NULL);
  }
  if (char_get_blocks(ch)) {
    char_set_blocked(char_get_blocks(ch), NULL);
    char_set_blocks(ch, NULL);
  }
  if (char_get_absorbing(ch)) {
    char_set_absorbby(char_get_absorbing(ch), NULL);
    char_set_absorbing(ch, NULL);
  }
  if (char_get_absorbby(ch)) {
    char_set_absorbing(char_get_absorbby(ch), NULL);
    char_set_absorbby(ch, NULL);
  }
}

typedef enum DeathType : uint8_t {
  Afterlife = 0,
  Northran  = 1,
  Past      = 2,
  Newbie    = 3
} DeathType;

void raw_kill(struct char_data *ch, struct char_data *killer)
{
  struct char_data *k, *temp;

  if (char_get_fighting(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* To make ordinary commands work in scripts.  welcor*/
  if (char_get_position(ch) != POS_SITTING && char_get_position(ch) != POS_SLEEPING && char_get_position(ch) != POS_RESTING)
    char_set_position(ch, POS_STANDING);

  if (killer && !char_is_npc(killer)) {
    if (!char_is_npc(killer) && !char_is_npc(ch)) {
      send_to_imm("[PK] %s killed %s at room [%d]\r\n", char_get_name(killer), char_get_name(ch), rroom_get_vnum(char_get_in_room(killer)));
    }
    if ((char_is_saiyan(killer) && rand_number(1, 2) == 2) || !char_is_saiyan(killer)) {
      if (rand_number(1, 6) >= 5 && (level_exp(killer, char_get_level(killer) + 1) - char_get_exp(killer) > 0 || char_get_level(killer) == 100)) {
        int psreward = char_get_aff_abils_wis(killer) * 0.35;
        if (char_get_level(killer) > char_get_level(ch) + 5) {
          psreward *= 0.2;
        } else if (char_get_level(killer) > char_get_level(ch) + 2) {
          psreward *= 0.5;
        }
        if (char_is_human(killer) || (char_is_bio(killer) && (char_get_genome(killer, 0) == 1 || char_get_genome(killer, 1) == 1))) {
          psreward *= 1.25;
        }
        if (char_is_halfbreed(ch)) {
          psreward -= psreward * 0.4;
        }
        if (char_is_npc(ch) && char_mob_flagged(ch, MOB_HUSK) && char_get_practice_points(killer) > 50 && char_is_bio(ch)) {
          psreward = 0;
          send_to_char(killer, "@D[@G+0 @BPS @cCapped at 50 for Absorb@D]@n\r\n");
        } else {
          char_set_practice_points(killer, char_get_practice_points(killer) + (psreward));
          send_to_char(killer, "@D[@G+%d @BPS@D]@n\r\n", psreward);
        }
      }
    }
    if (char_is_android(killer) && !char_is_npc(killer) && !char_plr_flagged(killer, PLR_ABSORB)) {
      if (char_plr_flagged(killer, PLR_REPAIR)) {
        if (char_get_level(killer) > char_get_level(ch) + 15) {
          send_to_char(killer, "@D[@G+0 @mUpgrade Point @r-WEAK-@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 10) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (3));
          send_to_char(killer, "@D[@G+3 @mUpgrade Point@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 8) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (6));
          send_to_char(killer, "@D[@G+6 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 4) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (12));
          send_to_char(killer, "@D[@G+12 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 2) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (16));
          send_to_char(killer, "@D[@G+16 @mUpgrade Points@D]@n\r\n");
        } else {
          char_set_upgrade(killer, char_get_upgrade(killer) + (28));
          send_to_char(killer, "@D[@G+28 @mUpgrade Points@D]@n\r\n");
        }
      } else {
        if (char_get_level(killer) > char_get_level(ch) + 15) {
          send_to_char(killer, "@D[@G+0 @mUpgrade Point @r-WEAK-@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 10) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (5));
          send_to_char(killer, "@D[@G+5 @mUpgrade Point@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 6) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (12));
          send_to_char(killer, "@D[@G+12 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 4) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (18));
          send_to_char(killer, "@D[@G+18 @mUpgrade Points@D]@n\r\n");
        } else if (char_get_level(killer) > char_get_level(ch) + 2) {
          char_set_upgrade(killer, char_get_upgrade(killer) + (28));
          send_to_char(killer, "@D[@G+28 @mUpgrade Points@D]@n\r\n");
        } else {
          char_set_upgrade(killer, char_get_upgrade(killer) + (36));
          send_to_char(killer, "@D[@G+36 @mUpgrade Points@D]@n\r\n");
        }
      }
    }
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);

  update_pos(ch);

  if (char_is_npc(ch) && !char_mob_flagged(ch, MOB_DUMMY)) {
    int shadowed = FALSE;
    decCurHealthPercent(ch, 1, 0);
    if (char_is_shadow_dragon1(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON1       = -1;
      send_to_room(char_get_in_room(ch), "@YThe one star dragon ball falls to the ground!@n\r\n");

      obj = read_object(20, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    } else if (char_is_shadow_dragon2(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON2       = -1;
      send_to_room(char_get_in_room(ch), "@YThe two star dragon ball falls to the ground!@n\r\n");

      obj = read_object(21, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    } else if (char_is_shadow_dragon3(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON3       = -1;
      send_to_room(char_get_in_room(ch), "@YThe three star dragon ball falls to the ground!@n\r\n");

      obj = read_object(22, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    } else if (char_is_shadow_dragon4(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON4       = -1;
      send_to_room(char_get_in_room(ch), "@YThe four star dragon ball falls to the ground!@n\r\n");

      obj = read_object(23, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    } else if (char_is_shadow_dragon5(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON5       = -1;
      send_to_room(char_get_in_room(ch), "@YThe five star dragon ball falls to the ground!@n\r\n");

      obj = read_object(24, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    } else if (char_is_shadow_dragon6(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON6       = -1;
      send_to_room(char_get_in_room(ch), "@YThe six star dragon ball falls to the ground!@n\r\n");

      obj = read_object(25, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    } else if (char_is_shadow_dragon7(ch)) {
      struct obj_data *obj = NULL;
      SHADOW_DRAGON7       = -1;
      send_to_room(char_get_in_room(ch), "@YThe seven star dragon ball falls to the ground!@n\r\n");

      obj = read_object(26, VIRTUAL);
      obj_to_room(obj, char_get_in_room(ch));
      shadowed = TRUE;
    }
    make_corpse(ch, killer);
    purge_homing(ch);
    extract_char(ch);
    if (shadowed == TRUE) {
      shadow_dragons_live();
    }
  } else if (char_is_npc(ch) && char_mob_flagged(ch, MOB_DUMMY)) {
    decCurHealthPercent(ch, 1, 0);
    extract_char(ch);
  } else {
    if (!char_aff_flagged(ch, AFF_SPIRIT) && !rroom_flagged(char_get_in_room(ch), ROOM_PAST) && (rroom_get_vnum(char_get_in_room(ch)) < 17900 || rroom_get_vnum(char_get_in_room(ch)) > 17999)) {
      if (!char_plr_flagged(ch, PLR_ABSORBED)) {
        make_pcorpse(ch);
        loadmap(ch);
      } else {
        char_plr_set(ch, PLR_ABSORBED, false);
      }
    }
    final_combat_resolve(ch);
    if (char_get_fighting(ch))
      stop_fighting(ch);

    for (k = combat_list; k; k = temp) {
      temp = k->next_fighting;
      if (char_get_fighting(k) == ch)
        stop_fighting(k);
    }

    bool android_lose    = true;
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
        if (char_get_level(ch) > 0 && has_group(ch)) {
          if (ch->master != NULL) {
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
        send_to_char(ch, "You wake up and realise that you didn't die, how or why are a mystery.\r\n");
        break;
      case Past:
        restore(ch, false);
        teleport_to(ch, 1561);
        android_lose = false;
        send_to_char(ch, "You wake up and realise that you died, but only in your mind.\r\n");
        break;
      case Newbie:
        restore(ch, false);
        teleport_to(ch, sensei_start_room(ch->chclass));
        send_to_char(ch, "\r\n@RYou should beware, when you reach level 9, you will actually die. So you\r\n"
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
        send_to_char(ch, "@rYou lose @R%s@r upgrade points!@n\r\n", add_commas(loss));
      }
      Crash_delete_crashfile(ch);
      save_char(ch);
    }
    char_set_wait(ch, PULSE_VIOLENCE);
  }
}

void die(struct char_data *ch, struct char_data *killer)
{
  // NPCs just die.
  if (char_is_npc(ch)) {
    raw_kill(ch, killer);
    return;
  }

  // But there's special handling for players.

  // Clear healing tank usage. How did you die while inside a healing tank though?
  if (char_plr_flagged(ch, PLR_HEALT)) {
    char_plr_set(ch, PLR_HEALT, false);
  }

  // First there are a few mechanics which can prevent death. They are checked first.

  // Saiyan Zenkai mechanic: if at 75% lifeforce or higher, 25% chance of triggering Zenkai.
  // this is implemented by setting PLR_GOOP with gooptime 0
  if (char_is_saiyan(ch) && (getCurLFPercent(ch) >= 0.75) && rand_number(1, 4) == 4) {
    char_plr_set(ch, PLR_GOOP, true);
    ch->gooptime = 0;
    decCurLFPercent(ch, 0.5, 0);
    return;
  }

  // majin and bio regen mechanic skips actually dying...
  if ((char_is_majin(ch) || char_is_bio(ch)) && ((getCurLF(ch)) >= (getMaxLF(ch)) * 0.75 || (char_plr_flagged(ch, PLR_SELFD2) &&
                                                                                             (getCurLF(ch)) >= (getMaxLF(ch)) * 0.5))) {
    decCurLFPercent(ch, 2, -1);
    decCurHealthPercent(ch, 1, 1);
    char_plr_set(ch, PLR_GOOP, true);
    ch->gooptime = 32;
    return;
  }

  // for players who used the Immortal Wish.
  if (char_plr_flagged(ch, PLR_IMMORTAL)) {
    act("@c$n@w disappears right before dying. $n appears to be immortal.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@c$n@w disappears right before dying. $n appears to be immortal.@n.", TRUE, ch, 0, 0, TO_ROOM);
    decCurHealthPercent(ch, 1, 1);
    decCurKIPercent(ch, 1, 1);
    decCurSTPercent(ch, 1, 1);
    null_affect(ch, AFF_POISON);
    if (char_get_conditions(ch, HUNGER) >= 0) {
      char_set_conditions(ch, HUNGER, 48);
    }
    if (char_get_conditions(ch, THIRST) >= 0) {
      char_set_conditions(ch, THIRST, 48);
    }
    if (char_get_fighting(ch)) {
      stop_fighting(ch);
    }
    char_set_position(ch, POS_SITTING);
    teleport_to(ch, sensei_start_room(ch->chclass));
    return;
  }

  // Removing some consequences that might happen.
  char_plr_set(ch, PLR_KILLER, false);
  char_plr_set(ch, PLR_THIEF, false);
  char_aff_set(ch, AFF_KNOCKED, false);
  char_aff_set(ch, AFF_SLEEP, false);
  char_aff_set(ch, AFF_PARALYZE, false);

  // those who die in the arena don't actually die. They get returned to the waiting room.
  if (char_in_arena(ch)) {
    cleanup_arena_watch(ch);
    if (killer != NULL) {
      cleanup_arena_watch(killer);
      send_to_all("@R%s@r manages to defeat @R%s@r in the Arena!@n\r\n", char_get_name(killer), char_get_name(ch));
      final_combat_resolve(killer);
      final_combat_resolve(ch);
      teleport_to(killer, 17875);
    } else {
      send_to_all("@R%s@r dies in the water of the Arena and is disqualified!@n\r\n", char_get_name(ch));
    }
    char_from_room(ch);
    char_to_room(ch, real_room(17875));
    decCurHealthPercent(ch, 1, 1);
    look_at_room(char_get_in_room(ch), ch, 0);
    final_combat_resolve(ch);
    return;
  }

  // For code below this, some kind of 'death' is performed.
  // it will leave a corpse.

  if (!char_aff_flagged(ch, AFF_SPIRIT) && !rroom_flagged(char_get_in_room(ch), ROOM_PAST) && char_get_level(ch) > 8) {
    if (rroom_get_vnum(char_get_in_room(ch)) >= 2002 && rroom_get_vnum(char_get_in_room(ch)) <= 2011) {
      char_set_deathtime(ch, time(0));
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_AL) || rroom_flagged(char_get_in_room(ch), ROOM_HELL)) {
      send_to_char(ch, "Your soul is saved from destruction by King Yemma. Why? Who knows.\r\n");
    } else {
      if (killer != NULL && char_is_npc(killer)) {
        char_set_deathtime(ch, time(0) + 7200);
        char_set_dcount(ch, char_get_dcount(ch) + (1));
      } else if (killer != NULL && !char_is_npc(killer)) {
        char_set_deathtime(ch, time(0) + 345600);
        char_plr_set(ch, PLR_PDEATH, true);
        char_set_dcount(ch, char_get_dcount(ch) + (1));
      } else {
        char_set_deathtime(ch, time(0) + 7200);
        char_set_dcount(ch, char_get_dcount(ch) + (1));
      }
    }
    if (char_get_conditions(ch, HUNGER) >= 0) {
      char_set_conditions(ch, HUNGER, 48);
    }
    if (char_get_conditions(ch, THIRST) >= 0) {
      char_set_conditions(ch, THIRST, 48);
    }
  }

  raw_kill(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim)
{
  int64_t share;

  if (char_in_arena(ch)) {
    return;
  }
  struct char_data *leader = ch->master ? ch->master : ch;

  /*share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base * GET_LEVEL(ch)));*/
  share = MIN(2000000, base * char_get_level(ch));
  if (!char_is_npc(ch)) {
    if (char_get_level(ch) >= 100 && getEffMaxPL(ch) * .025 >= getEffMaxPL(victim)) {
      share *= .05;
    } else if (getEffMaxPL(ch) * .025 >= getEffMaxPL(victim)) {
      share = 1;
    } else if (getEffMaxPL(ch) * .05 >= getEffMaxPL(victim)) {
      share *= .05;
    } else if (getEffMaxPL(ch) * .1 >= getEffMaxPL(victim)) {
      share *= .1;
    } else if (getEffMaxPL(ch) * .15 >= getEffMaxPL(victim)) {
      share *= .15;
    } else if (getEffMaxPL(ch) * .25 >= getEffMaxPL(victim)) {
      share *= .25;
    } else if (getEffMaxPL(ch) * .5 >= getEffMaxPL(victim)) {
      share *= .5;
    } else if (getEffMaxPL(ch) * .9 >= getEffMaxPL(victim)) {
      share *= .65;
    } else if (getEffMaxPL(ch) >= getEffMaxPL(victim)) {
      share *= .7;
    }
  }
  if (char_get_lasthit(victim) != 0 && char_get_lasthit(victim) != char_get_idnum(ch)) {
    struct follow_type *f;
    int checkit = FALSE;
    for (f = ch->followers; f; f = f->next) {
      if (char_aff_flagged(f->follower, AFF_GROUP) && char_get_lasthit(victim) == char_get_idnum(f->follower)) {
        checkit = TRUE;
      }
    }
    if (checkit == FALSE && ch->master != NULL && char_get_idnum(ch->master) == char_get_lasthit(victim)) {
      checkit = TRUE;
    }
    if (checkit == FALSE && ch->master != NULL) {
      struct char_data *master = ch->master;
      for (f = master->followers; f; f = f->next) {
        if (f->follower != ch) {
          if (char_aff_flagged(f->follower, AFF_GROUP) && char_get_lasthit(victim) == char_get_idnum(f->follower)) {
            checkit = TRUE;
          }
        }
      }
    }
    if (checkit == FALSE) {
      send_to_char(ch, "@RYou didn't do most of the work for this kill.@n\r\n");
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
  if (char_get_bonuses(ch, BONUS_LOYAL) > 0 && ch->master != NULL) {
    share += share * 0.2;
  }
  if (ch->master != NULL && ch->master != ch) {
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
  if (group_bonus(ch, 2) == 2) {
    send_to_char(ch, "You receive a bonus from your group's leader! @D[@G+2 PS!@D]@n\r\n");
    char_set_practice_points(ch, char_get_practice_points(ch) + (2));
  } else if (group_bonus(ch, 2) == 3) {
    send_to_char(ch, "You receive a bonus from your group's leader! @D[@G+5%s Exp!@D]@n\r\n", "%");
    share += share * 0.05;
  } else if (group_bonus(ch, 2) == 5) {
    incCurKIPercent(ch, .04, true);
    send_to_char(ch, "You receive a bonus from your group's leader! @D[@G4%s Ki Regenerated!@D]@n\r\n", "%");
  } else if (group_bonus(ch, 2) == 6) {
    incCurKIPercent(ch, .02, true);
    incCurSTPercent(ch, .02, true);
    incCurHealthPercent(ch, .02, true);
    send_to_char(ch, "You receive a bonus from your group's leader! @D[@G2%s PL/ST/Ki Regenerated!@D]@n\r\n", "%");
  } else if (group_bonus(ch, 2) == 7) {
    if (char_is_android(ch)) {
      if (char_plr_flagged(leader, PLR_ABSORB)) {
        incCurKIPercent(ch, .02, true);
        incCurSTPercent(ch, .02, true);
        send_to_char(ch, "You receive a bonus from your group's leader! @D[@G2%s PL/ST/Ki Recovered!@D]@n\r\n", "%");
      } else if (char_plr_flagged(leader, PLR_REPAIR)) {
        incCurHealthPercent(ch, .02, true);
        send_to_char(ch, "You receive a bonus from your group's leader! @D[@G5%s PL Repaired@D]@n\r\n", "%");
      } else if (char_plr_flagged(leader, PLR_SENSEM) && !char_plr_flagged(ch, PLR_ABSORB)) {
        char_set_upgrade(ch, char_get_upgrade(ch) + (5));
        send_to_char(ch, "You receive a bonus from your group's leader! @D[@G+5 @mUpgrade Points@D]@n\r\n");
      }
    } else {
      incCurHealthPercent(ch, .01, true);
      incCurKIPercent(ch, .01, true);
      incCurSTPercent(ch, .01, true);
    }
  } else if (group_bonus(ch, 2) == 11) {
    incCurSTPercent(ch, .04, true);
    send_to_char(ch, "You receive a bonus from your group's leader! @D[@G4%s ST Regenerated!@D]@n\r\n", "%");
  } else if (group_bonus(ch, 2) == 13) {
    if (char_get_starphase(leader) == 1) {
      share += share * 0.05;
      send_to_char(ch, "You receive a bonus from your group's leader! @D[@G+5%s Exp!@D]@n\r\n", "%");
    } else if (char_get_starphase(leader) == 2) {
      share += share * 0.1;
      send_to_char(ch, "You receive a bonus from your group's leader! @D[@G+10%s Exp!@D]@n\r\n", "%");
    }
  }
  share = gear_exp(ch, share);
  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %s points.\r\n", add_commas(share));
  else
    send_to_char(ch, "You receive your share of experience -- one measly little point!\r\n");

  gain_exp(ch, share);
  /*change_alignment(ch, victim);*/
}

void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_levels, tot_members;
  int64_t tot_gain, base;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (char_aff_flagged(k, AFF_GROUP) && (char_get_in_room(k) == char_get_in_room(ch))) {
    tot_levels  = char_get_level(k);
    tot_members = 1;
  } else {
    tot_levels  = 0;
    tot_members = 0;
  }

  for (f = k->followers; f; f = f->next)
    if (char_aff_flagged(f->follower, AFF_GROUP) && char_get_in_room(f->follower) == char_get_in_room(ch)) {
      if (!char_is_weighted(f->follower)) {
        tot_levels += char_get_level(f->follower);
        tot_members++;
      } else if (getEffMaxPL(f->follower) >= getEffMaxPL(ch) * 0.5) {
        tot_levels += char_get_level(f->follower);
        tot_members++;
      }
    }

  if (tot_members == 1 || char_in_arena(ch)) {
    solo_gain(ch, victim);
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
  perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next) {
    if (char_aff_flagged(f->follower, AFF_GROUP) && char_get_in_room(f->follower) == char_get_in_room(ch)) {
      //if ((getEffMaxPL(f->follower)()) >= GET_MAX_HIT(ch) * 0.5)
      perform_group_gain(f->follower, base, victim);
    }
  }
}

void solo_gain(struct char_data *ch, struct char_data *victim)
{

  if (char_is_npc(ch)) {
    if (char_get_original(ch)) {
      ch = char_get_original(ch);
    }
  }
  int64_t exp;

  exp = MIN(2000000, char_get_exp(victim));

  /* Calculate level-difference penalty */
  if (!char_is_npc(ch)) {
    if (char_get_level(ch) >= 100 && getEffMaxPL(ch) * .025 >= getEffMaxPL(victim)) {
      exp *= 0.04;
    } else if (getEffMaxPL(ch) * .025 >= getEffMaxPL(victim)) {
      exp = 1;
    } else if (getEffMaxPL(ch) * .05 >= getEffMaxPL(victim)) {
      exp *= 0.05;
    } else if (getEffMaxPL(ch) * .1 >= getEffMaxPL(victim)) {
      exp *= .1;
    } else if (getEffMaxPL(ch) * .15 >= getEffMaxPL(victim)) {
      exp *= .15;
    } else if (getEffMaxPL(ch) * .25 >= getEffMaxPL(victim)) {
      exp *= .25;
    } else if (getEffMaxPL(ch) * .5 >= getEffMaxPL(victim)) {
      exp *= .5;
    }
  }
  if (char_get_lasthit(victim) != 0 && char_get_lasthit(victim) != char_get_idnum(ch)) {
    send_to_char(ch, "@RYou didn't do most of the work for this victory.@n\r\n");
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
  exp = gear_exp(ch, exp);
  exp = MAX(exp, 1);

  if (exp > 1)
    send_to_char(ch, "You receive %s experience points.\r\n", add_commas(exp));
  else {
    send_to_char(ch, "You receive one lousy experience point. That fight was hardly worth it...\r\n");
  }
  if (!char_is_npc(ch)) {
    gain_exp(ch, exp);
  }
  if (char_is_npc(victim)) {
    gain_exp(victim, -exp);
  }
  if (!char_is_npc(victim)) {
    exp = exp / 5;
    gain_exp(victim, -exp);
  }
  /*change_alignment(ch, victim);*/
}
