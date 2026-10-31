/* attack.c: weapon attacks (attack, attack2)
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/act.offensive.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/consts/deathtype.h"
#include "dbat/config.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/db.h"
#include "dbat/extract.h"
#include "dbat/act.movement.h"
#include "dbat/interpreter.h"
#include "dbat/combat.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/fight.h"
#include "dbat/guild.h"
#include "dbat/class.h"
#include "dbat/techniques.h"

ACMD(do_attack)
{
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0, wtype = 0, gun = FALSE, gun2 = FALSE;
  int dualwield = 0, wielded = 0, guncost = 0;
  int64_t stcost = (GET_MAX_HIT(ch) / 150);
  int64_t dmg;
  struct char_data *vict;
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  if (char_is_android(ch)) {
    stcost *= 0.25;
  }
  one_argument(argument, arg);

  if (!char_get_equipment(ch, WEAR_WIELD1) && !char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(ch, "You need to wield a weapon to use this, without one try punch, kick, or other no weapon attacks.\r\n");
    return;
  }

  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }
  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (!char_is_android(ch)) {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD1));
    } else {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD1)) * 0.25;
    }
    if (!check_points(ch, 0, stcost)) {
      return;
    }
    wielded = 1;
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (!char_is_android(ch)) {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD2));
    } else {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD2)) * 0.25;
    }
    if (!check_points(ch, 0, stcost)) {
      return;
    }
  }

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (vict) {
      if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(ch, vict, NULL, 1)) {
          return;
        }
      } else {
        if (!can_kill(ch, vict, NULL, 0)) {
          return;
        }
      }
    }
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(ch, SKILL_DAGGER);
      improve_skill(ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SWORD);
      improve_skill(ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_CLUB);
      improve_skill(ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SPEAR);
      improve_skill(ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      gun   = TRUE;
      skill = init_skill(ch, SKILL_GUN);
      improve_skill(ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(ch, SKILL_BRAWL);
      improve_skill(ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (vict) {
      if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
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
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(ch, SKILL_DAGGER);
      improve_skill(ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SWORD);
      improve_skill(ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_CLUB);
      improve_skill(ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SPEAR);
      improve_skill(ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      gun2  = TRUE;
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
    if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 100) {
      dualwield = 3;
      stcost -= stcost * 0.30;
    } else if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 75) {
      dualwield = 2;
      stcost -= stcost * 0.25;
    }
  }

  int wlvl              = 0;
  struct obj_data *weap = NULL;
  if (char_get_equipment(ch, WEAR_WIELD1)) {
    weap = char_get_equipment(ch, WEAR_WIELD1);
  } else {
    weap = char_get_equipment(ch, WEAR_WIELD2);
  }
  if (obj_extra_flagged(weap, ITEM_WEAPLVL1)) {
    wlvl = 1;
  } else if (obj_extra_flagged(weap, ITEM_WEAPLVL2)) {
    wlvl = 2;
  } else if (obj_extra_flagged(weap, ITEM_WEAPLVL3)) {
    wlvl = 3;
  } else if (obj_extra_flagged(weap, ITEM_WEAPLVL4)) {
    wlvl = 4;
  } else if (obj_extra_flagged(weap, ITEM_WEAPLVL5)) {
    wlvl = 5;
  }

  if (char_get_preference(ch) != PREFERENCE_H2H) {
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
    if (char_get_gold(ch) < guncost) {
      send_to_char(ch, "You do not have enough zenni. You need %d zenni per shot for that level of gun.\r\n", guncost);
      return;
    } else {
      char_set_gold(ch, char_get_gold(ch) - (guncost));
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
    if (char_get_gold(ch) < guncost) {
      send_to_char(ch, "You do not have enough zenni. You need %d zenni per shot for that level of gun.\r\n", guncost);
      return;
    } else {
      char_set_gold(ch, char_get_gold(ch) - (guncost));
    }
  }

  if (vict) {
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }

    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, FALSE);
    perc  = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;
    handle_defense(vict, &pry, &blk, &dge);

    if (gun == TRUE) {
      if (dualwield >= 2) {
        prob += prob * 0.1;
      }
    }

    prob -= avo;
    if (char_plr_flagged(ch, PLR_THANDW)) {
      perc += 15;
    }
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "attack")) {
      if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
        pcost(ch, 0, stcost / 3);
      pcost(vict, 0, GET_MAX_HIT(vict) / 150);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W intercepts and parries your attack with $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou intercept and parry @C$n's@W attack with one of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
          act("@C$N@W moves quickly and blocks your attack!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W attack!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, -1, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your attack!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W attack!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
            pcost(ch, 0, stcost / 3);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE))
          pcost(ch, 0, stcost / 3);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, -1, skill, attperc);
      if (obj_extra_flagged(weap, ITEM_WEAPLVL1)) {
        dmg += dmg * 0.05;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL2)) {
        dmg += dmg * 0.1;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL3)) {
        dmg += dmg * 0.2;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL4)) {
        dmg += dmg * 0.3;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL5)) {
        dmg += dmg * 0.5;
      }
      if (wtype == 5) {
        if (char_get_skill(ch, SKILL_BRAWL) >= 100) {
          dmg += dmg * 0.5;
          wlvl = 5;
        } else if (char_get_skill(ch, SKILL_BRAWL) >= 50) {
          dmg += dmg * 0.2;
          wlvl = 3;
        }
      }
      if (wtype == 0 && char_is_konatsu(ch)) {
        dmg += dmg * .25;
      }
      if (char_plr_flagged(ch, PLR_THANDW)) {
        dmg += dmg * 1.2;
      }
      if (!char_is_npc(ch)) {
        if (char_plr_flagged(ch, PLR_THANDW) && gun == FALSE && gun2 == FALSE) {
          if (char_get_skills_base(ch, SKILL_TWOHAND) >= 100) {
            dmg += dmg * 0.5;
          } else if (char_get_skills_base(ch, SKILL_TWOHAND) >= 75) {
            dmg += dmg * 0.25;
          } else if (char_get_skills_base(ch, SKILL_TWOHAND) >= 50) {
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
      hitspot          = roll_hitloc(ch, vict, skill);
      int64_t beforepl = GET_HIT(vict);
      switch (hitspot) {
        case 1:
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          break;
        case 2: /* Head */
          hitspot = 4;
          break;
        case 3: /* Body */
          hitspot = 5;
          if (char_get_bonuses(ch, BONUS_SOFT)) {
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
      if (char_plr_flagged(ch, PLR_THANDW) && gun == TRUE) {
        if (hitspot != 4 && boom_headshot(ch)) {
          hitspot = 4;
          send_to_char(ch, "@GBoom headshot!@n\r\n");
        }
      }
      switch (wtype) {
        case 0:
          switch (hitspot) {
            case 1:
              act("@WYou slash @C$N@W across the stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the stomach!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 4);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 2:
              act("@WYou slash @C$N@W across the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the arm!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 1);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 1);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 3:
              act("@WYou slash @C$N@W across the leg!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the leg!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 1);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 2);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 4:
              act("@WYou slash @C$N@W across the face!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the face!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              if (!char_is_npc(ch)) {
                if (char_plr_flagged(ch, PLR_THANDW) && gun == FALSE && gun2 == FALSE) {
                  if (char_get_skills_base(ch, SKILL_TWOHAND) >= 100) {
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
              act("@WYou slash @C$N@W across the chest!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the chest!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 4);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
          } /* end slash switch*/
          if (beforepl - GET_HIT(vict) >= (getEffMaxPL(vict)) * 0.025) {
            cut_limb(ch, vict, wlvl, hitspot);
          }
          break;
        case 1:
          if (!char_get_fighting(ch) && backstab(ch, vict, wlvl, dmg)) {
            if (vict != NULL && GET_HIT(vict) > 1 && axion_dice(0) < (char_get_skill(ch, SKILL_DUALWIELD)) && char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
              do_attack2(ch, 0, 0, 0);
            }
            pcost(ch, 0, stcost);
            return;
          }
          dmg += (dmg * 0.01) * (char_get_aff_abils_dex(ch) * 0.5);
          switch (hitspot) {
            case 1:
              act("@WYou pierce @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W pierces your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W pierces @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W pierces @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W crushes @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 0);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 3);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 5:
              act("@WYou crush @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W crushes your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W crushes @C$N's@W stomach@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W stabs @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W blasts @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 0);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 3);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 5:
              act("@WYou blast @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W blasts your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W blasts @C$N's@W stomach@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W whacks @C$N@W in the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              if (char_get_skill(ch, SKILL_BRAWL) >= 100) {
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
              act("@c$n@W whacks @C$N's@W stomach@n", TRUE, ch, 0, vict, TO_NOTVICT);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 4);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
          } /* end brawl switch*/
          break;
      } /* end switch one*/
    }
    if ((wielded == 2 && gun == FALSE) || (gun2 == FALSE && gun == FALSE)) {
      if (char_get_equipment(ch, WEAR_WIELD1)) {
        if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
          act("@c$N's@W fireshield burns your weapon!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n's@W weapon is burned by your fireshield!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
          hurt(0, 0, vict, NULL, char_get_equipment(ch, WEAR_WIELD1), damdam, 0);
        } else if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && (char_get_bonuses(ch, BONUS_FIREPROOF) || char_is_demon(ch))) {
          send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
        }
        pcost(ch, 0, stcost);
      } else {
        if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
          act("@c$N's@W fireshield burns your weapon!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n's@W weapon is burned by your fireshield!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
          hurt(0, 0, vict, NULL, char_get_equipment(ch, WEAR_WIELD2), damdam, 0);
        }
        pcost(ch, 0, stcost);
      }
    }
    if (gun == FALSE && gun2 == FALSE) {
      damage_weapon(ch, weap, vict);
    }
    if (!char_is_npc(ch)) {
      if (char_plr_flagged(ch, PLR_THANDW)) {
        if (!char_get_skill(ch, SKILL_TWOHAND) && slot_count(ch) + 1 <= char_get_skill_slots(ch)) {
          int numb = rand_number(10, 15);
          char_set_skills_base(ch, SKILL_TWOHAND, numb);
          send_to_char(ch, "@GYou learn the very basics of two-handing your weapon!@n\r\n");
        } else {
          improve_skill(ch, SKILL_TWOHAND, 0);
        }
      }
    }
    if (char_get_equipment(ch, WEAR_WIELD2)) {
      if (!char_get_skill(ch, SKILL_DUALWIELD) && slot_count(ch) + 1 <= char_get_skill_slots(ch) && (obj_get_type_flag(char_get_equipment(ch, WEAR_WIELD2)) != ITEM_LIGHT)) {
        int numb = rand_number(10, 15);
        char_set_skills_base(ch, SKILL_DUALWIELD, numb);
        send_to_char(ch, "@GYou learn the very basics of dual-wielding!@n\r\n");
      } else {
        improve_skill(ch, SKILL_DUALWIELD, 0);
      }
      if (vict != NULL && GET_HIT(vict) > 1 && axion_dice(0) < (char_get_skill(ch, SKILL_DUALWIELD)) && char_get_equipment(ch, WEAR_WIELD1)) {
        do_attack2(ch, 0, 0, 0);
      }
    }
    handle_multihit(ch, vict);
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (char_get_aff_abils_str(ch)));
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

ACMD(do_attack2)
{
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0, wtype = 0, gun2 = FALSE;
  int dualwield = 0;
  int64_t dmg;
  struct char_data *vict = NULL;
  struct obj_data *obj   = NULL;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!char_get_equipment(ch, WEAR_WIELD2)) {
    return;
  }

  int64_t stcost = ((GET_MAX_HIT(ch) / 150) + obj_get_weight(char_get_equipment(ch, WEAR_WIELD2)));

  if (char_is_android(ch)) {
    stcost *= 0.25;
  }

  if (!can_grav(ch)) {
    return;
  }

  if (!char_has_arms(ch)) {
    send_to_char(ch, "With what arms!?\r\n");
    return;
  } else if (char_get_limb_condition(ch, (1) - 1) > 0 && char_get_limb_condition(ch, (1) - 1) < 50 && char_get_limb_condition(ch, (2) - 1) < 0) {
    send_to_char(ch, "Using your broken right arm has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (1) - 1, char_get_limb_condition(ch, (1) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (1) - 1) < 0) {
      act("@RYour right arm has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's right arm has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50 && char_get_limb_condition(ch, (1) - 1) < 0) {
    send_to_char(ch, "Using your broken left arm has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (2) - 1, char_get_limb_condition(ch, (2) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (2) - 1) < 0) {
      act("@RYour left arm has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's left arm has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  if (!char_get_fighting(ch)) {
    return;
  }

  if (!check_points(ch, 0, stcost)) {
    return;
  }

  if (!char_is_npc(ch) || char_is_npc(ch)) {
    if (vict) {
      if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(ch, vict, NULL, 1)) {
          return;
        }
      } else {
        if (!can_kill(ch, vict, NULL, 0)) {
          return;
        }
      }
    }

    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(ch, SKILL_DAGGER);
      improve_skill(ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SWORD);
      improve_skill(ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(ch, SKILL_CLUB);
      improve_skill(ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      skill = init_skill(ch, SKILL_SPEAR);
      improve_skill(ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      gun2  = TRUE;
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
    if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 100) {
      dualwield = 3;
      stcost -= stcost * 0.30;
    } else if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 75) {
      dualwield = 2;
      stcost -= stcost * 0.25;
    } else if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 50) {
      dualwield = 1;
      stcost -= stcost * 0.25;
    }
  }

  if (char_is_npc(ch) && char_get_level(ch) <= 10) {
    skill = rand_number(30, 50);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 20) {
    skill = rand_number(30, 60);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 30) {
    skill = rand_number(30, 70);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 50) {
    skill = rand_number(40, 80);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 70) {
    skill = rand_number(50, 90);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 80) {
    skill = rand_number(60, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 90) {
    skill = rand_number(70, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 100) {
    skill = rand_number(80, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) > 100) {
    skill = rand_number(95, 100);
  }
  if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
    vict = char_get_fighting(ch);
  }

  if (gun2 == TRUE) {
    if (char_get_gold(ch) < 1) {
      send_to_char(ch, "You do not have enough zenni. You need 1 zenni per shot.\r\n");
      return;
    } else {
      char_set_gold(ch, char_get_gold(ch) - (1));
    }
  }

  if (char_get_preference(ch) != PREFERENCE_H2H) {
    handle_cooldown(ch, 4);
  } else {
    handle_cooldown(ch, 8);
  }

  if (vict) {
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      if (!can_kill(ch, vict, NULL, 1)) {
        return;
      }
    } else {
      if (!can_kill(ch, vict, NULL, 0)) {
        return;
      }
    }
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, FALSE);
    perc  = chance_to_hit(ch);

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
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "attack")) {
      if (gun2 == FALSE)
        pcost(ch, 0, stcost / 3);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W intercepts and parries your attack with $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou intercept and parry @C$n's@W attack with one of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
          act("@C$N@W moves quickly and blocks your attack!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W attack!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, -1, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your attack!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W attack!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (gun2 == FALSE)
            pcost(ch, 0, stcost / 3);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your attack misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W moves to attack you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W moves to attack @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        if (gun2 == FALSE)
          pcost(ch, 0, stcost / 3);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg                   = damtype(ch, -1, skill, attperc);
      int wlvl              = 0;
      struct obj_data *weap = char_get_equipment(ch, WEAR_WIELD2);
      if (obj_extra_flagged(weap, ITEM_WEAPLVL1)) {
        dmg += dmg * .05;
        wlvl = 1;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL2)) {
        dmg += dmg * .1;
        wlvl = 2;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL3)) {
        dmg += dmg * .2;
        wlvl = 3;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL4)) {
        dmg += dmg * .3;
        wlvl = 4;
      } else if (obj_extra_flagged(weap, ITEM_WEAPLVL5)) {
        dmg += dmg * .5;
        wlvl = 5;
      }
      if (wtype == 5) {
        if (char_get_skill(ch, SKILL_BRAWL) >= 100) {
          dmg += dmg * 0.5;
          wlvl = 5;
        } else if (char_get_skill(ch, SKILL_BRAWL) >= 50) {
          dmg += dmg * 0.2;
          wlvl = 3;
        }
      }
      if (wtype == 0 && char_is_konatsu(ch)) {
        dmg += dmg * .25;
      }
      int hitspot = 1;
      if (gun2 == TRUE)
        dmg = gun_dam(ch, wlvl);
      hitspot          = roll_hitloc(ch, vict, skill);
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
          if (char_get_bonuses(ch, BONUS_SOFT)) {
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
              act("@WYou slash @C$N@W across the stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the stomach!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 4);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 2:
              act("@WYou slash @C$N@W across the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the arm!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 1);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 1);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 3:
              act("@WYou slash @C$N@W across the leg!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the leg!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 1);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 2);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 4:
              act("@WYou slash @C$N@W across the face!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the face!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              dmg *= calc_critical(ch, 0);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 3);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
            case 5:
              act("@WYou slash @C$N@W across the chest!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W slashes you across the chest!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W slashes @C$N@W across the chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              hurt(0, 0, ch, vict, NULL, dmg, 0);
              dam_eq_loc(vict, 4);
              /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
              break;
          } /* end slash switch*/
          if (beforepl - GET_HIT(vict) >= (getEffMaxPL(vict)) * 0.025) {
            cut_limb(ch, vict, wlvl, hitspot);
          }
          break;
        case 1:
          dmg += (dmg * 0.01) * (char_get_aff_abils_dex(ch) * 0.5);
          switch (hitspot) {
            case 1:
              act("@WYou pierce @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@c$n@W pierces your stomach!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@c$n@W pierces @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W pierces @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W crushes @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W crushes @C$N's@W chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W stabs @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W blasts @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              act("@c$n@W whacks @C$N's@W stomach!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
              if (char_get_skill(ch, SKILL_BRAWL) >= 100) {
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
      if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
        act("@c$N's@W fireshield burns your weapon!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n's@W weapon is burned by your fireshield!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n's@W weapon is burned by @C$N's@W fireshield!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
        hurt(0, 0, vict, NULL, char_get_equipment(ch, WEAR_WIELD2), damdam, 0);
      } else if (GET_HIT(vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && (char_get_bonuses(ch, BONUS_FIREPROOF) || char_is_demon(ch))) {
        send_to_char(vict, "@RThey appear to be fireproof!@n\r\n");
      }
      pcost(ch, 0, stcost);
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (char_get_aff_abils_str(ch)));
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
