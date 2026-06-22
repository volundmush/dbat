/* ************************************************************************
 *   File: act.offensive.c                               Part of CircleMUD *
 *  Usage: player-level commands of an offensive nature                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "act.offensive.h"
#include "consts/attacks.h"
#include "consts/deathtype.h"
#include "consts/maximums.h"

#include "config.h"

#include "db.h"
#include "extract.h"
#include "random.h"
#include "relocate.h"
#include "search.h"

#include "act.movement.h"

#include "combat.h"
#include "comm.h"
#include "interpreter.h"
#include "iterate.hpp"
#include "spells.h"

#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "config_db.h"
#include "consts/appearance.h"
#include "consts/applies.h"
#include "consts/fightprefs.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "fight.h"
#include "flags.h"
#include "guild.h"
#include "handler.h"
#include "object_impl.h"
#include "object_macros.h"
#include "room_api.h"

#include "stringutils.h"
#include "techniques.h"
#include <cstdlib>
#include <strings.h>

/* Combat commands below this line */







ACMD(do_geno) {

  int perc, prob;
  double attperc = 0.5, minimum = .4;
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);
  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_GENOCIDE)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !FIGHTING(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc))
    return;

  if (GET_MAX_MANA(ch) * attperc > GET_CHARGE(ch)) {
    attperc = (long double)(GET_CHARGE(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  vict = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "No one around here by that name.\r\n");
      return;
    }
  }

  if (!can_kill(ch, vict, NULL, 3)) {
    return;
  }

  if (handle_defender(vict, ch)) {
    struct char_data *def = GET_DEFENDER(vict);
    vict = def;
  }

  prob = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
  perc = rand_number(1, 115);

  if (prob < perc - 20) {
    act("@WYou raise one arm above your head and pour your charged ki there. A "
        "large swirling pink ball of energy begins to form above your raised "
        "hand. You lose concentration and the ball of energy dissipates!@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W raises one arm above $s head and pours $s charged ki there. A "
        "large swirling pink ball of energy begins to form above $s raised "
        "hand. @C$n@W loses concentration and the ball of energy dissipates!@n",
        TRUE, ch, 0, 0, TO_ROOM);
    hurt(0, 0, ch, vict, NULL, 0, 1);
    pcost(ch, attperc, 0);

    improve_skill(ch, SKILL_GENOCIDE, 2);
    return;
  }

  struct obj_data *obj;
  int dista = 15 - (GET_INT(ch) * 0.1);

  if (GET_SKILL(ch, SKILL_GENOCIDE) >= 100) {
    dista -= 3;
  } else if (GET_SKILL(ch, SKILL_GENOCIDE) >= 60) {
    dista -= 2;
  } else if (GET_SKILL(ch, SKILL_GENOCIDE) >= 40) {
    dista -= 1;
  }

  obj = read_object(83, VIRTUAL);
  obj_to_room(obj, char_room_get(vict));

  char_charge_set(ch, GET_CHARGE(ch) + (int64_t)(GET_MAX_HIT(ch) / 10));
  TARGET(obj) = vict;
  KICHARGE(obj) = damtype(ch, 41, prob, attperc);
  KITYPE(obj) = SKILL_GENOCIDE;
  USER(obj) = ch;
  KIDIST(obj) = dista;
  pcost(ch, attperc, 0);
  act("@WYou raise one arm above your head and pour your charged ki there. A "
      "large swirling pink ball of energy begins to form above your raised "
      "hand. You grin viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is "
      "complete and you toss it at @c$N@W!@n",
      TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W raises one arm above $s head and pours $s charged ki there. A "
      "large swirling pink ball of energy begins to form above $s raised hand. "
      "@C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is "
      "complete and $e tosses it at YOU!@n",
      TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W raises one arm above $s head and pours $s charged ki there. A "
      "large swirling pink ball of energy begins to form above $s raised hand. "
      "@C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is "
      "complete and $e tosses it at @c$N@W!@n",
      TRUE, ch, 0, vict, TO_NOTVICT);

  improve_skill(ch, SKILL_GENOCIDE, 2);
}

ACMD(do_genki) {
  return;
}
















/* Chimera */


ACMD(do_attack) {
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0,
                       wtype = 0, gun = FALSE, gun2 = FALSE;
  int dualwield = 0, wielded = 0, guncost = 0;
  int64_t stcost = (GET_MAX_HIT(ch) / 150);
  int64_t dmg;
  struct char_data *vict;
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  if (IS_ANDROID(ch)) {
    stcost *= 0.25;
  }
  one_argument(argument, arg);

  if (char_condition_has(ch, "mystic_melody")) {
    send_to_char(ch, "You are currently playing a song! Enter the song command "
                     "in order to stop!\r\n");
    return;
  }

  if (!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) {
    send_to_char(ch, "You need to wield a weapon to use this, without one try "
                     "punch, kick, or other no weapon attacks.\r\n");
    return;
  }

  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !FIGHTING(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }
  if (GET_EQ(ch, WEAR_WIELD1)) {
    if (!IS_ANDROID(ch)) {
      stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD1));
    } else {
      stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD1)) * 0.25;
    }
    if (!check_points(ch, 0, stcost)) {
      return;
    }
    wielded = 1;
  } else if (GET_EQ(ch, WEAR_WIELD2)) {
    if (!IS_ANDROID(ch)) {
      stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2));
    } else {
      stcost += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2)) * 0.25;
    }
    if (!check_points(ch, 0, stcost)) {
      return;
    }
  }

  if (!tech_handle_targeting(ch, arg, &vict, &obj))
    return;

  if (GET_EQ(ch, WEAR_WIELD1)) {
    if (vict) {
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
          TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(ch, vict, NULL, 1)) {
          return;
        }
      } else {
        if (!can_kill(ch, vict, NULL, 0)) {
          return;
        }
      }
    }
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
        TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(ch, SKILL_DAGGER);
      improve_skill(ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
               TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SWORD);
      improve_skill(ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
               TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_CLUB);
      improve_skill(ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
               TYPE_STAB - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SPEAR);
      improve_skill(ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) ==
               TYPE_BLAST - TYPE_HIT) {
      gun = TRUE;
      skill = init_skill(ch, SKILL_GUN);
      improve_skill(ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(ch, SKILL_BRAWL);
      improve_skill(ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  } else if (GET_EQ(ch, WEAR_WIELD2)) {
    if (vict) {
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
          TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(ch, vict, NULL, 1)) {
          return;
        }
      } else {
        if (!can_kill(ch, vict, NULL, 0)) {
          return;
        }
      }
    }
    if (wielded == 1)
      wielded = 2;
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
        TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(ch, SKILL_DAGGER);
      improve_skill(ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SWORD);
      improve_skill(ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_CLUB);
      improve_skill(ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_STAB - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SPEAR);
      improve_skill(ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_BLAST - TYPE_HIT) {
      gun2 = TRUE;
      skill = init_skill(ch, SKILL_GUN);
      improve_skill(ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(ch, SKILL_BRAWL);
      improve_skill(ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  }
  if (wielded == 2 && gun == FALSE) {
    if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 100) {
      dualwield = 3;
      stcost -= stcost * 0.30;
    } else if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 75) {
      dualwield = 2;
      stcost -= stcost * 0.25;
    }
  }

  int wlvl = 0;
  struct obj_data *weap = NULL;
  if (GET_EQ(ch, WEAR_WIELD1)) {
    weap = GET_EQ(ch, WEAR_WIELD1);
  } else {
    weap = GET_EQ(ch, WEAR_WIELD2);
  }
  if (OBJ_FLAGGED(weap, ITEM_WEAPLVL1)) {
    wlvl = 1;
  } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL2)) {
    wlvl = 2;
  } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL3)) {
    wlvl = 3;
  } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL4)) {
    wlvl = 4;
  } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL5)) {
    wlvl = 5;
  }

  if (GET_PREFERENCE(ch) != PREFERENCE_H2H) {
    handle_cooldown(ch, 4);
  } else {
    handle_cooldown(ch, 8);
  }

  if (wielded == 1 && (gun == TRUE || gun2 == TRUE)) {
    if (wlvl == 5) {
      guncost = 12;
    } else if (wlvl == 4) {
      guncost = 6;
    } else if (wlvl == 3) {
      guncost = 4;
    } else if (wlvl == 2) {
      guncost = 2;
    }
    if (GET_GOLD(ch) < guncost) {
      send_to_char(ch,
                   "You do not have enough zenni. You need %d zenni per shot "
                   "for that level of gun.\r\n",
                   guncost);
      return;
    } else {
      char_stat_mod(ch, "money", -guncost);
    }
  } else if (wielded == 2 && gun == TRUE) {
    if (wlvl == 5) {
      guncost = 12;
    } else if (wlvl == 4) {
      guncost = 6;
    } else if (wlvl == 3) {
      guncost = 4;
    } else if (wlvl == 2) {
      guncost = 2;
    }
    if (GET_GOLD(ch) < guncost) {
      send_to_char(ch,
                   "You do not have enough zenni. You need %d zenni per shot "
                   "for that level of gun.\r\n",
                   guncost);
      return;
    } else {
      char_stat_mod(ch, "money", -guncost);
    }
  }

  if (vict) {
    if (handle_defender(vict, ch)) {
      struct char_data *def = GET_DEFENDER(vict);
      vict = def;
    }

    index = check_def(vict);
    prob = roll_accuracy(ch, skill, FALSE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;
    handle_defense(vict, &pry, &blk, &dge);

    if (gun == TRUE) {
      if (dualwield >= 2) {
        prob += prob * 0.1;
      }
    }

    prob -= avo;
    if (PLR_FLAGGED(ch, PLR_THANDW)) {
      perc += 15;
    }
    tech_handle_posmodifier(vict, pry, blk, dge, prob);

    if (!tech_handle_zanzoken(ch, vict, "attack")) {
      if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
        pcost(ch, 0, stcost / 3);
      pcost(vict, 0, GET_MAX_HIT(vict) / 150);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) &&
            (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY))) {
          act("@C$N@W intercepts and parries your attack with $S own!@n", TRUE,
              ch, 0, vict, TO_CHAR);
          act("@WYou intercept and parry @C$n's@W attack with one of your "
              "own!@n",
              TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W intercepts and parries @c$n's@W attack with one of $S "
              "own!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          if (wtype != 4)
            handle_disarm(ch, vict);
          improve_skill(vict, SKILL_PARRY, 0);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your attack!@n", TRUE, ch, 0,
              vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W attack!@n", TRUE, ch, 0,
              vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W attack!@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, -1, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your attack!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@WYou dodge @C$n's@W attack!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W attack!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0,
              vict, TO_CHAR);
          act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch,
              0, vict, TO_NOTVICT);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost / 3);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
          pcost(ch, 0, stcost / 3);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, -1, skill, attperc);
      if (OBJ_FLAGGED(weap, ITEM_WEAPLVL1)) {
        dmg += dmg * 0.05;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL2)) {
        dmg += dmg * 0.1;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL3)) {
        dmg += dmg * 0.2;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL4)) {
        dmg += dmg * 0.3;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL5)) {
        dmg += dmg * 0.5;
      }
      if (wtype == 5) {
        if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
          dmg += dmg * 0.5;
          wlvl = 5;
        } else if (GET_SKILL(ch, SKILL_BRAWL) >= 50) {
          dmg += dmg * 0.2;
          wlvl = 3;
        }
      }
      if (wtype == 0 && IS_KONATSU(ch)) {
        dmg += dmg * .25;
      }
      if (PLR_FLAGGED(ch, PLR_THANDW)) {
        dmg += dmg * 1.2;
      }
      if (!IS_NPC(ch)) {
        if (PLR_FLAGGED(ch, PLR_THANDW) && gun == FALSE && gun2 == FALSE) {
          if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 100) {
            dmg += dmg * 0.5;
          } else if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 75) {
            dmg += dmg * 0.25;
          } else if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 50) {
            dmg += dmg * 0.1;
          }
          if (wtype == 3) {
            switch (wlvl) {
            case 1:
              dmg += dmg * 0.04;
              break;
            case 2:
              dmg += dmg * 0.08;
              break;
            case 3:
              dmg += dmg * 0.12;
              break;
            case 4:
              dmg += dmg * 0.2;
              break;
            case 5:
              dmg += dmg * 0.25;
              break;
            }
          }
        }
      }
      if (wtype == 3) {
        if (skill >= 100)
          dmg += dmg * 0.04;
        else if (skill >= 50)
          dmg += dmg * 0.1;
      }
      int hitspot = 1;
      if (gun == TRUE)
        dmg = gun_dam(ch, wlvl);
      hitspot = roll_hitloc(ch, vict, skill);
      int64_t beforepl = GET_HIT(vict);
      switch (hitspot) {
      case 1:
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        break;
      case 2: /* Head */
        hitspot = 4;
        break;
      case 3: /* Body */
        hitspot = 5;
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        break;
      case 4: /* Arm */
        hitspot = 2;
        break;
      case 5: /* Leg */
        hitspot = 3;
        break;
      }
      if (PLR_FLAGGED(ch, PLR_THANDW) && gun == TRUE) {
        if (hitspot != 4 && boom_headshot(ch)) {
          hitspot = 4;
          send_to_char(ch, "@GBoom headshot!@n\r\n");
        }
      }
      switch (wtype) {
      case 0:
        switch (hitspot) {
        case 1:
          act("@WYou slash @C$N@W across the stomach!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the stomach!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou slash @C$N@W across the arm!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the arm!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the arm!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou slash @C$N@W across the leg!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the leg!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the leg!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou slash @C$N@W across the face!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the face!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the face!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          if (!IS_NPC(ch)) {
            if (PLR_FLAGGED(ch, PLR_THANDW) && gun == FALSE && gun2 == FALSE) {
              if (GET_SKILL_BASE(ch, SKILL_TWOHAND) >= 100) {
                double mult = calc_critical(ch, 0);
                mult += 1.0;
                dmg *= mult;
              } else {
                dmg *= calc_critical(ch, 0);
              }
            } else {
              dmg *= calc_critical(ch, 0);
            }
          } else {
            dmg *= calc_critical(ch, 0);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou slash @C$N@W across the chest!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the chest!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the chest!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end slash switch*/
        if (beforepl - GET_HIT(vict) >= (getMaxPL(vict)) * 0.025) {
          cut_limb(ch, vict, wlvl, hitspot);
        }
        break;
      case 1:
        if (!FIGHTING(ch) && backstab(ch, vict, wlvl, dmg)) {
          if (vict != NULL && GET_HIT(vict) > 1 &&
              axion_dice(0) < (GET_SKILL(ch, SKILL_DUALWIELD)) &&
              GET_EQ(ch, WEAR_WIELD1) && GET_EQ(ch, WEAR_WIELD2)) {
            do_attack2(ch, 0, 0, 0);
          }
          pcost(ch, 0, stcost);
          return;
        }
        dmg += (dmg * 0.01) * (GET_DEX(ch) * 0.5);
        switch (hitspot) {
        case 1:
          act("@WYou pierce @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou pierce @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou pierce @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou pierce @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou pierce @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W chest!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end pierce switch*/
        break;
      case 2:
        switch (hitspot) {
        case 1:
          act("@WYou crush @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W chest@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou crush @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou crush @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou crush @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes you in the face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N@W in the face!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou crush @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W stomach@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end crush switch*/
        club_stamina(ch, vict, wlvl, dmg);
        break;
      case 3:
        switch (hitspot) {
        case 1:
          act("@WYou stab @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou stab @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou stab @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou stab @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs you in the face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N@W in the face!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou stab @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W stomach@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end stab switch*/
        break;
      case 4:
        switch (hitspot) {
        case 1:
          act("@WYou blast @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou blast @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou blast @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou blast @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts you in the face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N@W in the face!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou blast @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W stomach@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end blast switch*/
        break;
      case 5:
        switch (hitspot) {
        case 1:
          act("@WYou whack @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou whack @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou whack @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou whack @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks you in the face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N@W in the face!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
            double mult = calc_critical(ch, 0);
            mult += 1.0;
            dmg *= mult;
          } else {
            dmg *= calc_critical(ch, 0);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou whack @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W stomach@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end brawl switch*/
        break;
      } /* end switch one*/
    }
    if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE)) {
      if (GET_EQ(ch, WEAR_WIELD1)) {
        if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) &&
            AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
            !GET_BONUS(ch, BONUS_FIREPROOF) && !IS_DEMON(ch)) {
          act("@c$N's@W fireshield burns your weapon!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@C$n's@W weapon is burned by your fireshield!@n", TRUE, ch, 0,
              vict, TO_VICT);
          act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", TRUE, ch,
              0, vict, TO_NOTVICT);
          int damdam = GET_SKILL(vict, SKILL_FIRESHIELD) / 2;
          hurt(0, 0, vict, NULL, GET_EQ(ch, WEAR_WIELD1), damdam, 0);
        } else if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) &&
                   AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                   (GET_BONUS(ch, BONUS_FIREPROOF) || IS_DEMON(ch))) {
          send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
        }
        pcost(ch, 0, stcost);
      } else {
        if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) &&
            AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
            !GET_BONUS(ch, BONUS_FIREPROOF) && !IS_DEMON(ch)) {
          act("@c$N's@W fireshield burns your weapon!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@C$n's@W weapon is burned by your fireshield!@n", TRUE, ch, 0,
              vict, TO_VICT);
          act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", TRUE, ch,
              0, vict, TO_NOTVICT);
          int damdam = GET_SKILL(vict, SKILL_FIRESHIELD) / 2;
          hurt(0, 0, vict, NULL, GET_EQ(ch, WEAR_WIELD2), damdam, 0);
        }
        pcost(ch, 0, stcost);
      }
    }
    if (gun == FALSE && gun2 == FALSE) {
      damage_weapon(ch, weap, vict);
    }
    if (!IS_NPC(ch)) {
      if (PLR_FLAGGED(ch, PLR_THANDW)) {
        if (!GET_SKILL(ch, SKILL_TWOHAND) &&
            slot_count(ch) + 1 <= GET_SLOTS(ch)) {
          int numb = rand_number(10, 15);
          SET_SKILL(ch, SKILL_TWOHAND, numb);
          send_to_char(
              ch,
              "@GYou learn the very basics of two-handing your weapon!@n\r\n");
        } else {
          improve_skill(ch, SKILL_TWOHAND, 0);
        }
      }
    }
    if (GET_EQ(ch, WEAR_WIELD2)) {
      if (!GET_SKILL(ch, SKILL_DUALWIELD) &&
          slot_count(ch) + 1 <= GET_SLOTS(ch) &&
          (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) != ITEM_LIGHT)) {
        int numb = rand_number(10, 15);
        SET_SKILL(ch, SKILL_DUALWIELD, numb);
        send_to_char(ch, "@GYou learn the very basics of dual-wielding!@n\r\n");
      } else {
        improve_skill(ch, SKILL_DUALWIELD, 0);
      }
      if (vict != NULL && GET_HIT(vict) > 1 &&
          axion_dice(0) < (GET_SKILL(ch, SKILL_DUALWIELD)) &&
          GET_EQ(ch, WEAR_WIELD1)) {
        do_attack2(ch, 0, 0, 0);
      }
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
    act("@WYou attack $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W attacks $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
      pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}


ACMD(do_attack2) {
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0,
                       wtype = 0, gun2 = FALSE;
  int dualwield = 0;
  int64_t dmg;
  struct char_data *vict = NULL;
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!GET_EQ(ch, WEAR_WIELD2)) {
    return;
  }

  int64_t stcost =
      ((GET_MAX_HIT(ch) / 150) + GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2)));
  int64_t kicost =
      ((GET_MAX_HIT(ch) / 150) + GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD2)));

  if (IS_ANDROID(ch)) {
    stcost *= 0.25;
  }

  if (IS_ANDROID(ch) && gun2 == TRUE) {
    kicost *= 0.25;
  }

  if (!can_grav(ch)) {
    return;
  }

  if (!HAS_ARMS(ch)) {
    send_to_char(ch, "With what arms!?\r\n");
    return;
  } else if (GET_LIMBCOND(ch, 1) > 0 && GET_LIMBCOND(ch, 1) < 50 &&
             GET_LIMBCOND(ch, 2) < 0) {
    send_to_char(ch, "Using your broken right arm has damaged it more!@n\r\n");
    SET_LIMBCOND(ch, 1, GET_LIMBCOND(ch, 1) - (rand_number(3, 5)));
    if (GET_LIMBCOND(ch, 1) < 0) {
      act("@RYour right arm has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's right arm has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  } else if (GET_LIMBCOND(ch, 2) > 0 && GET_LIMBCOND(ch, 2) < 50 &&
             GET_LIMBCOND(ch, 1) < 0) {
    send_to_char(ch, "Using your broken left arm has damaged it more!@n\r\n");
    SET_LIMBCOND(ch, 2, GET_LIMBCOND(ch, 2) - (rand_number(3, 5)));
    if (GET_LIMBCOND(ch, 2) < 0) {
      act("@RYour left arm has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's left arm has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  if (!FIGHTING(ch)) {
    return;
  }

  if (!check_points(ch, 0, stcost)) {
    return;
  }

  if (!IS_NPC(ch) || IS_NPC(ch)) {
    if (vict) {
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
          TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(ch, vict, NULL, 1)) {
          return;
        }
      } else {
        if (!can_kill(ch, vict, NULL, 0)) {
          return;
        }
      }
    }

    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
        TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(ch, SKILL_DAGGER);
      improve_skill(ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SWORD);
      improve_skill(ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_CLUB);
      improve_skill(ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_STAB - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SPEAR);
      improve_skill(ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
               TYPE_BLAST - TYPE_HIT) {
      gun2 = TRUE;
      skill = init_skill(ch, SKILL_GUN);
      improve_skill(ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(ch, SKILL_BRAWL);
      improve_skill(ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  }

  if (gun2 == FALSE) {
    if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 100) {
      dualwield = 3;
      stcost -= stcost * 0.30;
    } else if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 75) {
      dualwield = 2;
      stcost -= stcost * 0.25;
    } else if (GET_SKILL_BASE(ch, SKILL_DUALWIELD) >= 50) {
      dualwield = 1;
      stcost -= stcost * 0.25;
    }
  }

  if (IS_NPC(ch) && GET_LEVEL(ch) <= 10) {
    skill = rand_number(30, 50);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 20) {
    skill = rand_number(30, 60);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 30) {
    skill = rand_number(30, 70);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 50) {
    skill = rand_number(40, 80);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 70) {
    skill = rand_number(50, 90);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 80) {
    skill = rand_number(60, 100);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 90) {
    skill = rand_number(70, 100);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) <= 100) {
    skill = rand_number(80, 100);
  } else if (IS_NPC(ch) && GET_LEVEL(ch) > 100) {
    skill = rand_number(95, 100);
  }
  if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
    vict = FIGHTING(ch);
  }

  if (gun2 == TRUE) {
    if (GET_GOLD(ch) < 1) {
      send_to_char(
          ch, "You do not have enough zenni. You need 1 zenni per shot.\r\n");
      return;
    } else {
      char_stat_mod(ch, "money", -1);
    }
  }

  if (GET_PREFERENCE(ch) != PREFERENCE_H2H) {
    handle_cooldown(ch, 4);
  } else {
    handle_cooldown(ch, 8);
  }

  if (vict) {
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) ==
        TYPE_BLAST - TYPE_HIT) {
      if (!can_kill(ch, vict, NULL, 1)) {
        return;
      }
    } else {
      if (!can_kill(ch, vict, NULL, 0)) {
        return;
      }
    }
    index = check_def(vict);
    prob = roll_accuracy(ch, skill, FALSE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;
    handle_defense(vict, &pry, &blk, &dge);
    if (dualwield == 3) {
      pry -= pry * 0.1;
      blk -= pry * 0.1;
      dge -= pry * 0.1; /* pry, This is intentional */
    }

    if (gun2 == TRUE) {
      if (dualwield >= 1) {
        prob += prob * 0.1;
      }
    }

    prob -= avo;
    tech_handle_posmodifier(vict, pry, blk, dge, prob);

    if (!tech_handle_zanzoken(ch, vict, "attack")) {
      if (gun2 == FALSE)
        pcost(ch, 0, stcost / 3);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) &&
            (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY))) {
          act("@C$N@W intercepts and parries your attack with $S own!@n", TRUE,
              ch, 0, vict, TO_CHAR);
          act("@WYou intercept and parry @C$n's@W attack with one of your "
              "own!@n",
              TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W intercepts and parries @c$n's@W attack with one of $S "
              "own!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          if (wtype != 4)
            handle_disarm(ch, vict);
          improve_skill(vict, SKILL_PARRY, 0);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your attack!@n", TRUE, ch, 0,
              vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W attack!@n", TRUE, ch, 0,
              vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W attack!@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, -1, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your attack!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@WYou dodge @C$n's@W attack!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W attack!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0,
              vict, TO_CHAR);
          act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch,
              0, vict, TO_NOTVICT);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost / 3);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0,
            vict, TO_CHAR);
        act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        if (gun2 == FALSE)
          pcost(ch, 0, stcost / 3);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, -1, skill, attperc);
      int wlvl = 0;
      struct obj_data *weap = GET_EQ(ch, WEAR_WIELD2);
      if (OBJ_FLAGGED(weap, ITEM_WEAPLVL1)) {
        dmg += dmg * .05;
        wlvl = 1;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL2)) {
        dmg += dmg * .1;
        wlvl = 2;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL3)) {
        dmg += dmg * .2;
        wlvl = 3;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL4)) {
        dmg += dmg * .3;
        wlvl = 4;
      } else if (OBJ_FLAGGED(weap, ITEM_WEAPLVL5)) {
        dmg += dmg * .5;
        wlvl = 5;
      }
      if (wtype == 5) {
        if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
          dmg += dmg * 0.5;
          wlvl = 5;
        } else if (GET_SKILL(ch, SKILL_BRAWL) >= 50) {
          dmg += dmg * 0.2;
          wlvl = 3;
        }
      }
      if (wtype == 0 && IS_KONATSU(ch)) {
        dmg += dmg * .25;
      }
      int hitspot = 1;
      if (gun2 == TRUE)
        dmg = gun_dam(ch, wlvl);
      hitspot = roll_hitloc(ch, vict, skill);
      int64_t beforepl = GET_HIT(vict);
      if (wtype == 3) {
        if (skill >= 100)
          dmg += dmg * 0.04;
        else if (skill >= 50)
          dmg += dmg * 0.1;
      }
      if (gun2 == TRUE) {
        if (dualwield == 3 && rand_number(1, 3) == 3) {
          send_to_char(ch, "@GYour masterful aim scores a critical!@n\r\n");
          hitspot = 2;
        }
      }
      switch (hitspot) {
      case 1:
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        break;
      case 2: /* Head */
        hitspot = 4;
        break;
      case 3: /* Body */
        hitspot = 5;
        break;
      case 4: /* Arm */
        hitspot = 5;
        break;
      case 5: /* Leg */
        hitspot = 5;
        break;
      }
      switch (wtype) {
      case 0:
        switch (hitspot) {
        case 1:
          act("@WYou slash @C$N@W across the stomach!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the stomach!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou slash @C$N@W across the arm!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the arm!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the arm!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou slash @C$N@W across the leg!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the leg!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the leg!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou slash @C$N@W across the face!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the face!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the face!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou slash @C$N@W across the chest!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@c$n@W slashes you across the chest!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W slashes @C$N@W across the chest!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end slash switch*/
        if (beforepl - GET_HIT(vict) >= (getMaxPL(vict)) * 0.025) {
          cut_limb(ch, vict, wlvl, hitspot);
        }
        break;
      case 1:
        dmg += (dmg * 0.01) * (GET_DEX(ch) * 0.5);
        switch (hitspot) {
        case 1:
          act("@WYou pierce @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou pierce @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N'@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou pierce @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou pierce @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou pierce @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W pierces your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W pierces @C$N's@W chest!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end pierce switch*/
        break;
      case 2:
        switch (hitspot) {
        case 1:
          act("@WYou crush @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou crush @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou crush @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou crush @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N'@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou crush @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W crushes your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W crushes @C$N's@W chest!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end crush switch*/
        club_stamina(ch, vict, wlvl, dmg);
        break;
      case 3:
        switch (hitspot) {
        case 1:
          act("@WYou stab @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou stab @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou stab @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou stab @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N'@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou stab @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W stabs your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W stabs @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end stab switch*/
        break;
      case 4:
        switch (hitspot) {
        case 1:
          act("@WYou blast @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou blast @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou blast @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou blast @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N'@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou blast @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W blasts your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W blasts @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end blast switch*/
        break;
      case 5:
        switch (hitspot) {
        case 1:
          act("@WYou whack @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W stomach!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2:
          act("@WYou whack @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou whack @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou whack @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N'@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (GET_SKILL(ch, SKILL_BRAWL) >= 100) {
            double mult = calc_critical(ch, 0);
            mult += 1.0;
            dmg *= mult;
          } else {
            dmg *= calc_critical(ch, 0);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5:
          act("@WYou whack @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@c$n@W whacks your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W whacks @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        } /* end whacks switch*/
        break;
      } /* end switch one*/
    }
    if (gun2 == FALSE) {
      if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) &&
          AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
          !GET_BONUS(ch, BONUS_FIREPROOF) && !IS_DEMON(ch)) {
        act("@c$N's@W fireshield burns your weapon!@n", TRUE, ch, 0, vict,
            TO_CHAR);
        act("@C$n's@W weapon is burned by your fireshield!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        int damdam = GET_SKILL(vict, SKILL_FIRESHIELD) / 2;
        hurt(0, 0, vict, NULL, GET_EQ(ch, WEAR_WIELD2), damdam, 0);
      } else if (GET_HIT(vict) > 0 && !AFF_FLAGGED(vict, AFF_SPIRIT) &&
                 AFF_FLAGGED(vict, AFF_FIRESHIELD) &&
                 (GET_BONUS(ch, BONUS_FIREPROOF) || IS_DEMON(ch))) {
        send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
      }
      pcost(ch, 0, stcost);
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
    act("@WYou attack $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W attacks $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    if (gun2 == FALSE)
      pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}













/* do_charge moved to lua/characters/commands/misc/charge.lua */

ACMD(do_powerup) {
  if (IS_NPC(ch)) {
    SET_BIT_AR(MOB_FLAGS(ch), MOB_POWERUP);
    if (GET_MAX_HIT(ch) < 50000) {
      act("@RYou begin to powerup, and air billows outward around you!@n", TRUE,
          ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and air billows outward around $m!@n", TRUE,
          ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 500000) {
      act("@RYou begin to powerup, and loose objects are lifted into the "
          "air!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and loose objects are lifted into the "
          "air!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 5000000) {
      act("@RYou begin to powerup, and torrents of energy crackle around "
          "you!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and torrents of energy crackle around $m!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 50000000) {
      act("@RYou begin to powerup, and the entire area begins to shudder!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the entire area begins to shudder!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 100000000) {
      act("@RYou begin to powerup, and massive cracks begin to form beneath "
          "you!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and massive cracks begin to form beneath "
          "$m!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 300000000) {
      act("@RYou begin to powerup, and everything around you shudders from the "
          "power!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and everything around $m shudders from the "
          "power!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else {
      act("@RYou begin to powerup, and the very air around you begins to "
          "burn!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the very air around $m begins to "
          "burn!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    }
    return;
  }
  if (PLR_FLAGGED(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "@WYou are concentrating too much on your aura to be able "
                     "to power up.");
    return;
  }
  if (IS_ANDROID(ch)) {
    send_to_char(ch, "@WYou are an android, you do not powerup.@n");
    return;
  }
  if (GET_SUPPRESS(ch) > 0) {
    send_to_char(ch,
                 "@WYou currently have your powerlevel suppressed to %" I64T
                 " percent.@n",
                 GET_SUPPRESS(ch));
    return;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP)) {
    send_to_char(ch, "@WYou stop powering up.@n");
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_POWERUP);
    return;
  }
  if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
    send_to_char(ch, "@WYou are already at max!@n");
    return;
  }
  if ((getCurKI(ch)) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "@WYou do not have enough ki to powerup!@n");
    return;
  } else {
    reveal_hiding(ch, 0);
    if (GET_MAX_HIT(ch) < 50000) {
      act("@RYou begin to powerup, and air billows outward around you!@n", TRUE,
          ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and air billows outward around $m!@n", TRUE,
          ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 500000) {
      act("@RYou begin to powerup, and loose objects are lifted into the "
          "air!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and loose objects are lifted into the "
          "air!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 5000000) {
      act("@RYou begin to powerup, and torrents of energy crackle around "
          "you!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and torrents of energy crackle around $m!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 50000000) {
      act("@RYou begin to powerup, and the entire area begins to shudder!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the entire area begins to shudder!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 100000000) {
      act("@RYou begin to powerup, and massive cracks begin to form beneath "
          "you!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and massive cracks begin to form beneath "
          "$m!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else if (GET_MAX_HIT(ch) < 300000000) {
      act("@RYou begin to powerup, and everything around you shudders from the "
          "power!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and everything around $m shudders from the "
          "power!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    } else {
      act("@RYou begin to powerup, and the very air around you begins to "
          "burn!@n",
          TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n begins to powerup, and the very air around $m begins to "
          "burn!@n",
          TRUE, ch, 0, 0, TO_ROOM);
    }
    SET_BIT_AR(PLR_FLAGS(ch), PLR_POWERUP);
    return;
  }
}

ACMD(do_rescue) {

  char arg[100];
  struct char_data *helpee, *opponent;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to rescue?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else if (!FIGHTING(helpee))
    send_to_char(ch, "They are not fighting anyone!\r\n");
  else if (FIGHTING(ch) && !IS_NPC(ch))
    send_to_char(ch, "You are a little too busy fighting for yourself!\r\n");
  else {
    opponent = FIGHTING(helpee);
    int mobbonus = 0;

    if (IS_NPC(ch)) {
      mobbonus = GET_SPEEDI(ch) * 0.2;
    }
    if (GET_SPEEDI(ch) + mobbonus < GET_SPEEDI(opponent) &&
        rand_number(1, 3) != 3) {
      act("@GYou leap towards @g$N@G and try to rescue $M but are too slow!@n",
          TRUE, ch, 0, helpee, TO_CHAR);
      act("@g$n@G leaps towards you! $n is too slow and fails to rescue you!@n",
          TRUE, ch, 0, helpee, TO_VICT);
      act("@g$n@G leaps towards @g$N@G and tries to rescue $M but is too "
          "slow!@n",
          TRUE, ch, 0, helpee, TO_NOTVICT);
      return;
    }
    act("@GYou leap in front of @g$N@G and rescue $M!@n", TRUE, ch, 0, helpee,
        TO_CHAR);
    act("@g$n@G leaps in front of you! You are rescued!@n", TRUE, ch, 0, helpee,
        TO_VICT);
    act("@g$n@G leaps in front of @g$N@G and rescues $M!@n", TRUE, ch, 0,
        helpee, TO_NOTVICT);
    stop_fighting(opponent);
    hurt(0, 0, ch, opponent, NULL, rand_number(1, GET_LEVEL(ch)), 1);
    hurt(0, 0, opponent, ch, NULL, rand_number(1, GET_LEVEL(ch)), 1);
    return;
  }
}

ACMD(do_assist) {
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(
        ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else {
      opponent = NULL;
      room_people_iterate(char_room_get(ch), [&](auto vict) {
        if (FIGHTING(vict) == helpee) {
          opponent = vict;
          return false;
        }
        return true;
      });
    }

    if (!opponent)
      act("But nobody is fighting $M!", TRUE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", TRUE, ch, 0, helpee, TO_CHAR);
    /* prevent accidental pkill */
    else {
      reveal_hiding(ch, 0);
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", TRUE, ch, 0, helpee, TO_NOTVICT);
      if (!FIGHTING(ch)) {
        set_fighting(ch, opponent);
      }
      if (!FIGHTING(opponent)) {
        set_fighting(opponent, ch);
      }
    }
  }
}

ACMD(do_kill) {
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_INSTANTKILL)) {
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
    else {
      act("You chop $M to pieces!  Ah!  The blood!", TRUE, ch, 0, vict,
          TO_CHAR);
      act("$N chops you to pieces!", TRUE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", TRUE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict, ch);
    }
  }
}

ACMD(do_flee) {
  int i, attempt;
  struct char_data *was_fighting;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (GET_POS(ch) < POS_RESTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (GRAPPLING(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return;
  }

  if (GRAPPLED(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return;
  }

  if (ABSORBING(ch)) {
    send_to_char(ch, "You are absorbing from someone!\r\n");
    return;
  }

  if (ABSORBBY(ch)) {
    send_to_char(ch, "You are being absorbed from by someone!\r\n");
    return;
  }

  if (!IS_NPC(ch)) {
    int fail = FALSE;
    room_contents_iterate(char_room_get(ch), [&](auto obj) {
      if (KICHARGE(obj) > 0 && USER(obj) == ch) {
        fail = TRUE;
      }
      return true;
    });
    if (fail == TRUE) {
      send_to_char(ch, "You are too busy controlling your attack!\r\n");
      return;
    }
  }

  for (i = 0; i < 12; i++) {
    if (*arg) {
      if ((attempt = search_block(arg, dirs, FALSE) > -1)) {
        attempt = search_block(arg, dirs, FALSE);
      } else if ((attempt = search_block(arg, abbr_dirs, FALSE) > -1)) {
        attempt = search_block(arg, abbr_dirs, FALSE);
      } else {
        attempt =
            rand_number(0, NUM_OF_DIRS - 1); /* Select a random direction */
      }
    }
    if (!*arg) {
      attempt = rand_number(0, NUM_OF_DIRS - 1); /* Select a random direction */
    }
    if (CAN_GO(ch, attempt) &&
        !room_flagged(exit_dest_get(EXIT(ch, attempt)), ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (IS_NPC(ch) &&
          room_flagged(exit_dest_get(EXIT(ch, attempt)), ROOM_NOMOB)) {
        return;
      }
      was_fighting = FIGHTING(ch);

      {
        bool hit_wall = false;
        room_contents_iterate(char_room_get(ch), [&](auto wall) {
          if (GET_OBJ_VNUM(wall) == 79) {
            if (GET_OBJ_COST(wall) == attempt) {
              hit_wall = true;
              return false;
            }
          }
          return true;
        });
        if (hit_wall)
          return;
      }

      if (!block_calc(ch)) {
        return;
      }

      if (ABSORBING(ch)) {
        send_to_char(ch, "You are busy absorbing from %s!\r\n",
                     GET_NAME(ABSORBING(ch)));
        return;
      }
      if (ABSORBBY(ch)) {
        if (axion_dice(0) < GET_SKILL(ABSORBING(ch), SKILL_ABSORB)) {
          send_to_char(ch,
                       "You are being held by %s, they are absorbing you!\r\n",
                       GET_NAME(ABSORBBY(ch)));
          send_to_char(ABSORBBY(ch), "%s struggles in your grasp!\r\n",
                       GET_NAME(ch));
          WAIT_STATE(ch, PULSE_2SEC);
          return;
        } else {
          act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE,
              ABSORBBY(ch), 0, ch, TO_NOTVICT);
          act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE,
              ABSORBBY(ch), 0, ch, TO_VICT);
          act("@c$N@W manages to break loose of your hold!@n", TRUE,
              ABSORBBY(ch), 0, ch, TO_CHAR);
          struct char_data *absorber = ABSORBBY(ch);
          char_absorbed_by_set(ch, NULL);
          char_absorbing_set(absorber, NULL);
        }
      }
      if (do_simple_move(ch, attempt, TRUE)) {
        send_to_char(ch, "You flee head over heels.\r\n");
        WAIT_STATE(ch, PULSE_2SEC);
      } else {
        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}
