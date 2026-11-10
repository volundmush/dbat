/* attack.c: weapon attacks (attack, attack2)
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/args.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/consts/deathtype.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/db.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/fight.hpp"
#include "dbat/guild.hpp"
#include "dbat/class.hpp"
#include "dbat/techniques.hpp"
#include "dbat/game_world.hpp"

static ACMD(do_attack2);

ACMD(do_attack)
{
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0, wtype = 0, gun = false, gun2 = false;
  int dualwield = 0, wielded = 0, guncost = 0;
  int64_t stcost = (getEffMaxPL(g, ch) / 150);
  int64_t dmg;
  struct char_data *vict;
  struct obj_data *obj = nullptr;
  std::string arg;
  double attperc = 0;

  if (char_is_android(ch)) {
    stcost *= 0.25;
  }
  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (playing_song(g, ch)) {
    return;
  }

  if (!char_get_equipment(ch, WEAR_WIELD1) && !char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(g, ch, "You need to wield a weapon to use this, without one try punch, kick, or other no weapon attacks.\r\n");
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (!char_is_android(ch)) {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD1));
    } else {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD1)) * 0.25;
    }
    if (!check_points(g, ch, 0, stcost)) {
      return;
    }
    wielded = 1;
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (!char_is_android(ch)) {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD2));
    } else {
      stcost += obj_get_weight(char_get_equipment(ch, WEAR_WIELD2)) * 0.25;
    }
    if (!check_points(g, ch, 0, stcost)) {
      return;
    }
  }

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (vict) {
      if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(g, ch, vict, nullptr, 1)) {
          return;
        }
      } else {
        if (!can_kill(g, ch, vict, nullptr, 0)) {
          return;
        }
      }
    }
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_DAGGER);
      improve_skill(g, ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_SWORD);
      improve_skill(g, ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_CLUB);
      improve_skill(g, ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_SPEAR);
      improve_skill(g, ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      gun   = true;
      skill = init_skill(g, ch, SKILL_GUN);
      improve_skill(g, ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(g, ch, SKILL_BRAWL);
      improve_skill(g, ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (vict) {
      if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(g, ch, vict, nullptr, 1)) {
          return;
        }
      } else {
        if (!can_kill(g, ch, vict, nullptr, 0)) {
          return;
        }
      }
    }
    if (wielded == 1)
      wielded = 2;
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_DAGGER);
      improve_skill(g, ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_SWORD);
      improve_skill(g, ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_CLUB);
      improve_skill(g, ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_SPEAR);
      improve_skill(g, ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      gun2  = true;
      skill = init_skill(g, ch, SKILL_GUN);
      improve_skill(g, ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(g, ch, SKILL_BRAWL);
      improve_skill(g, ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  }
  if (wielded == 2 && gun == false) {
    if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 100) {
      dualwield = 3;
      stcost -= stcost * 0.30;
    } else if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 75) {
      dualwield = 2;
      stcost -= stcost * 0.25;
    }
  }

  int wlvl              = 0;
  struct obj_data *weap = nullptr;
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
    handle_cooldown(g, ch, 4);
  } else {
    handle_cooldown(g, ch, 8);
  }

  if (wielded == 1 && (gun == true || gun2 == true)) {
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
      send_to_char(g, ch, "You do not have enough zenni. You need {:d} zenni per shot for that level of gun.\r\n", guncost);
      return;
    }
    char_set_gold(ch, char_get_gold(ch) - (guncost));
  } else if (wielded == 2 && gun == true) {
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
      send_to_char(g, ch, "You do not have enough zenni. You need {:d} zenni per shot for that level of gun.\r\n", guncost);
      return;
    }
    char_set_gold(ch, char_get_gold(ch) - (guncost));
  }

  if (vict) {
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }

    index = check_def(g, vict);
    prob  = roll_accuracy(g, ch, skill, false);
    perc  = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;
    handle_defense(g, vict, &pry, &blk, &dge);

    if (gun == true) {
      if (dualwield >= 2) {
        prob += prob * 0.1;
      }
    }

    prob -= avo;
    if (char_plr_flagged(ch, PLR_THANDW)) {
      perc += 15;
    }
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "attack")) {
      if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
        pcost(g, ch, 0, stcost / 3);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 150);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W intercepts and parries your attack with $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou intercept and parry @C$n's@W attack with one of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (wtype != 4)
            handle_disarm(g, ch, vict);
          improve_skill(g, vict, SKILL_PARRY, 0);
          if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
            pcost(g, ch, 0, stcost);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
            pcost(g, ch, 0, stcost);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, -1, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
            pcost(g, ch, 0, stcost);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
          pcost(g, ch, 0, stcost / 3);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
        pcost(g, ch, 0, stcost / 3);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg = damtype(g, ch, -1, skill, attperc);
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
      if (char_plr_flagged(ch, PLR_THANDW) && gun == false && gun2 == false) {
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
    if (gun == true)
      dmg = gun_dam(g, ch, wlvl);
    hitspot          = roll_hitloc(g, ch, vict, skill);
    int64_t beforepl = getCurPL(g, vict);
    switch (hitspot) {
      case 1:
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        break;
      case 2: /* Head */
        hitspot = 4;
        break;
      case 3: /* Body */
        hitspot = 5;
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        break;
      case 4: /* Arm */
        hitspot = 2;
        break;
      case 5: /* Leg */
        hitspot = 3;
        break;
    }
    if (char_plr_flagged(ch, PLR_THANDW) && gun == true) {
      if (hitspot != 4 && boom_headshot(g, ch)) {
        hitspot = 4;
        send_to_char(g, ch, "@GBoom headshot!@n\r\n");
      }
    }
    switch (wtype) {
      case 0:
        switch (hitspot) {
          case 1:
            act(g, "@WYou slash @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou slash @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou slash @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou slash @C$N@W across the face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            if (!char_is_npc(ch)) {
              if (char_plr_flagged(ch, PLR_THANDW) && gun == false && gun2 == false) {
                if (char_get_skills_base(ch, SKILL_TWOHAND) >= 100) {
                  double mult = calc_critical(g, ch, 0);
                  mult += 1.0;
                  dmg *= mult;
                } else {
                  dmg *= calc_critical(g, ch, 0);
                }
              } else {
                dmg *= calc_critical(g, ch, 0);
              }
            } else {
              dmg *= calc_critical(g, ch, 0);
            }
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou slash @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end slash switch*/
        if (beforepl - getCurPL(g, vict) >= (getEffMaxPL(g, vict)) * 0.025) {
          cut_limb(g, ch, vict, wlvl, hitspot);
        }
        break;
      case 1:
        if (!char_get_fighting(g, ch) && backstab(g, ch, vict, wlvl, dmg)) {
          if (vict != nullptr && getCurPL(g, vict) > 1 && axion_dice(g, 0) < (char_get_skill(ch, SKILL_DUALWIELD)) && char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
            do_attack2(g, ch, "", 0, 0);
          }
          pcost(g, ch, 0, stcost);
          return;
        }
        dmg += (dmg * 0.01) * (char_get_aff_abils_dex(ch) * 0.5);
        switch (hitspot) {
          case 1:
            act(g, "@WYou pierce @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou pierce @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou pierce @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou pierce @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou pierce @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end pierce switch*/
        break;
      case 2:
        switch (hitspot) {
          case 1:
            act(g, "@WYou crush @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W chest@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou crush @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou crush @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou crush @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes you in the face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou crush @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end crush switch*/
        club_stamina(g, ch, vict, wlvl, dmg);
        break;
      case 3:
        switch (hitspot) {
          case 1:
            act(g, "@WYou stab @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou stab @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou stab @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou stab @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs you in the face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou stab @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end stab switch*/
        break;
      case 4:
        switch (hitspot) {
          case 1:
            act(g, "@WYou blast @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou blast @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou blast @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou blast @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts you in the face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou blast @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end blast switch*/
        break;
      case 5:
        switch (hitspot) {
          case 1:
            act(g, "@WYou whack @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou whack @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou whack @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou whack @C$N@W in the face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks you in the face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N@W in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            if (char_get_skill(ch, SKILL_BRAWL) >= 100) {
              double mult = calc_critical(g, ch, 0);
              mult += 1.0;
              dmg *= mult;
            } else {
              dmg *= calc_critical(g, ch, 0);
            }
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou whack @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W stomach@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end brawl switch*/
        break;
    } /* end switch one*/
    if ((wielded == 2 && gun == false) || (gun2 == false && gun == false)) {
      if (char_get_equipment(ch, WEAR_WIELD1)) {
        if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
          act(g, "@c$N's@W fireshield burns your weapon!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@C$n's@W weapon is burned by your fireshield!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true, ch, nullptr, vict, TO_NOTVICT);
          int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
          hurt(g, 0, 0, vict, nullptr, char_get_equipment(ch, WEAR_WIELD1), damdam, 0);
        } else if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && (char_get_bonuses(ch, BONUS_FIREPROOF) || char_is_demon(ch))) {
          send_to_char(g, vict, "@RThey appear to be fireproof!@n\r\n");
        }
        pcost(g, ch, 0, stcost);
      } else {
        if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
          act(g, "@c$N's@W fireshield burns your weapon!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@C$n's@W weapon is burned by your fireshield!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true, ch, nullptr, vict, TO_NOTVICT);
          int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
          hurt(g, 0, 0, vict, nullptr, char_get_equipment(ch, WEAR_WIELD2), damdam, 0);
        }
        pcost(g, ch, 0, stcost);
      }
    }
    if (gun == false && gun2 == false) {
      damage_weapon(g, ch, weap, vict);
    }
    if (!char_is_npc(ch)) {
      if (char_plr_flagged(ch, PLR_THANDW)) {
        if (!char_get_skill(ch, SKILL_TWOHAND) && slot_count(ch) + 1 <= char_get_skill_slots(ch)) {
          int numb = rand_number(g, 10, 15);
          char_set_skills_base(ch, SKILL_TWOHAND, numb);
          send_to_char(g, ch, "@GYou learn the very basics of two-handing your weapon!@n\r\n");
        } else {
          improve_skill(g, ch, SKILL_TWOHAND, 0);
        }
      }
    }
    if (char_get_equipment(ch, WEAR_WIELD2)) {
      if (!char_get_skill(ch, SKILL_DUALWIELD) && slot_count(ch) + 1 <= char_get_skill_slots(ch) && (obj_get_type_flag(char_get_equipment(ch, WEAR_WIELD2)) != ITEM_LIGHT)) {
        int numb = rand_number(g, 10, 15);
        char_set_skills_base(ch, SKILL_DUALWIELD, numb);
        send_to_char(g, ch, "@GYou learn the very basics of dual-wielding!@n\r\n");
      } else {
        improve_skill(g, ch, SKILL_DUALWIELD, 0);
      }
      if (vict != nullptr && getCurPL(g, vict) > 1 && axion_dice(g, 0) < (char_get_skill(ch, SKILL_DUALWIELD)) && char_get_equipment(ch, WEAR_WIELD1)) {
        do_attack2(g, ch, "", 0, 0);
      }
    }
    handle_multihit(g, ch, vict);
  } else if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou attack $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W attacks $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    if ((wielded == 2 && gun == false) || (gun2 == false && gun == false))
      pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

static ACMD(do_attack2)
{
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0, wtype = 0, gun2 = false;
  int dualwield = 0;
  int64_t dmg;
  struct char_data *vict = nullptr;
  struct obj_data *obj   = nullptr;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!char_get_equipment(ch, WEAR_WIELD2)) {
    return;
  }

  int64_t stcost = ((getEffMaxPL(g, ch) / 150) + obj_get_weight(char_get_equipment(ch, WEAR_WIELD2)));

  if (char_is_android(ch)) {
    stcost *= 0.25;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "With what arms!?\r\n");
    return;
  }
  if (char_get_limb_condition(ch, (1) - 1) > 0 && char_get_limb_condition(ch, (1) - 1) < 50 && char_get_limb_condition(ch, (2) - 1) < 0) {
    send_to_char(g, ch, "Using your broken right arm has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (1) - 1, char_get_limb_condition(ch, (1) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (1) - 1) < 0) {
      act(g, "@RYour right arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's right arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50 && char_get_limb_condition(ch, (1) - 1) < 0) {
    send_to_char(g, ch, "Using your broken left arm has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (2) - 1, char_get_limb_condition(ch, (2) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (2) - 1) < 0) {
      act(g, "@RYour left arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's left arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  }

  if (!char_get_fighting(g, ch)) {
    return;
  }

  if (!check_points(g, ch, 0, stcost)) {
    return;
  }

  if (!char_is_npc(ch) || char_is_npc(ch)) {
    if (vict) {
      if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        if (!can_kill(g, ch, vict, nullptr, 1)) {
          return;
        }
      } else {
        if (!can_kill(g, ch, vict, nullptr, 0)) {
          return;
        }
      }
    }

    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_DAGGER);
      improve_skill(g, ch, SKILL_DAGGER, 1);
      wtype = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_SWORD);
      improve_skill(g, ch, SKILL_SWORD, 1);
      wtype = 0;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_CLUB);
      improve_skill(g, ch, SKILL_CLUB, 1);
      wtype = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      skill = init_skill(g, ch, SKILL_SPEAR);
      improve_skill(g, ch, SKILL_SPEAR, 1);
      wtype = 3;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      gun2  = true;
      skill = init_skill(g, ch, SKILL_GUN);
      improve_skill(g, ch, SKILL_GUN, 1);
      wtype = 4;
    } else {
      skill = init_skill(g, ch, SKILL_BRAWL);
      improve_skill(g, ch, SKILL_BRAWL, 1);
      wtype = 5;
    }
  }

  if (gun2 == false) {
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
    skill = rand_number(g, 30, 50);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 20) {
    skill = rand_number(g, 30, 60);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 30) {
    skill = rand_number(g, 30, 70);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 50) {
    skill = rand_number(g, 40, 80);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 70) {
    skill = rand_number(g, 50, 90);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 80) {
    skill = rand_number(g, 60, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 90) {
    skill = rand_number(g, 70, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) <= 100) {
    skill = rand_number(g, 80, 100);
  } else if (char_is_npc(ch) && char_get_level(ch) > 100) {
    skill = rand_number(g, 95, 100);
  }
  if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
    vict = char_get_fighting(g, ch);
  }

  if (gun2 == true) {
    if (char_get_gold(ch) < 1) {
      send_to_char(g, ch, "You do not have enough zenni. You need 1 zenni per shot.\r\n");
      return;
    }
    char_set_gold(ch, char_get_gold(ch) - (1));
  }

  if (char_get_preference(ch) != PREFERENCE_H2H) {
    handle_cooldown(g, ch, 4);
  } else {
    handle_cooldown(g, ch, 8);
  }

  if (vict) {
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
      if (!can_kill(g, ch, vict, nullptr, 1)) {
        return;
      }
    } else {
      if (!can_kill(g, ch, vict, nullptr, 0)) {
        return;
      }
    }
    index = check_def(g, vict);
    prob  = roll_accuracy(g, ch, skill, false);
    perc  = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;
    handle_defense(g, vict, &pry, &blk, &dge);
    if (dualwield == 3) {
      pry -= pry * 0.1;
      blk -= pry * 0.1;
      dge -= pry * 0.1; /* pry, This is intentional */
    }

    if (gun2 == true) {
      if (dualwield >= 1) {
        prob += prob * 0.1;
      }
    }

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "attack")) {
      if (gun2 == false)
        pcost(g, ch, 0, stcost / 3);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W intercepts and parries your attack with $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou intercept and parry @C$n's@W attack with one of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          if (wtype != 4)
            handle_disarm(g, ch, vict);
          improve_skill(g, vict, SKILL_PARRY, 0);
          if (gun2 == false)
            pcost(g, ch, 0, stcost);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          if (gun2 == false)
            pcost(g, ch, 0, stcost);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, -1, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          if (gun2 == false)
            pcost(g, ch, 0, stcost);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (gun2 == false)
          pcost(g, ch, 0, stcost / 3);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your attack misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W moves to attack you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W moves to attack @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      if (gun2 == false)
        pcost(g, ch, 0, stcost / 3);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg                   = damtype(g, ch, -1, skill, attperc);
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
    if (gun2 == true)
      dmg = gun_dam(g, ch, wlvl);
    hitspot          = roll_hitloc(g, ch, vict, skill);
    int64_t beforepl = getCurPL(g, vict);
    if (wtype == 3) {
      if (skill >= 100)
        dmg += dmg * 0.04;
      else if (skill >= 50)
        dmg += dmg * 0.1;
    }
    if (gun2 == true) {
      if (dualwield == 3 && rand_number(g, 1, 3) == 3) {
        send_to_char(g, ch, "@GYour masterful aim scores a critical!@n\r\n");
        hitspot = 2;
      }
    }
    switch (hitspot) {
      case 1:
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
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
            act(g, "@WYou slash @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou slash @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou slash @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou slash @C$N@W across the face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou slash @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W slashes you across the chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W slashes @C$N@W across the chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end slash switch*/
        if (beforepl - getCurPL(g, vict) >= (getEffMaxPL(g, vict)) * 0.025) {
          cut_limb(g, ch, vict, wlvl, hitspot);
        }
        break;
      case 1:
        dmg += (dmg * 0.01) * (char_get_aff_abils_dex(ch) * 0.5);
        switch (hitspot) {
          case 1:
            act(g, "@WYou pierce @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou pierce @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N'@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou pierce @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou pierce @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou pierce @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W pierces your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W pierces @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end pierce switch*/
        break;
      case 2:
        switch (hitspot) {
          case 1:
            act(g, "@WYou crush @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou crush @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou crush @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou crush @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou crush @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W crushes your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W crushes @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end crush switch*/
        club_stamina(g, ch, vict, wlvl, dmg);
        break;
      case 3:
        switch (hitspot) {
          case 1:
            act(g, "@WYou stab @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou stab @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou stab @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou stab @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou stab @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W stabs your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W stabs @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end stab switch*/
        break;
      case 4:
        switch (hitspot) {
          case 1:
            act(g, "@WYou blast @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou blast @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou blast @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou blast @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou blast @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W blasts your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W blasts @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end blast switch*/
        break;
      case 5:
        switch (hitspot) {
          case 1:
            act(g, "@WYou whack @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your stomach!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W stomach!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2:
            act(g, "@WYou whack @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act(g, "@WYou whack @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
            dmg *= calc_critical(g, ch, 1);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4:
            act(g, "@WYou whack @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your face!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N'@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
            if (char_get_skill(ch, SKILL_BRAWL) >= 100) {
              double mult = calc_critical(g, ch, 0);
              mult += 1.0;
              dmg *= mult;
            } else {
              dmg *= calc_critical(g, ch, 0);
            }
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5:
            act(g, "@WYou whack @C$N's@W chest!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@c$n@W whacks your chest!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@c$n@W whacks @C$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
            dam_eq_loc(g, vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        } /* end whacks switch*/
        break;
    } /* end switch one*/
    if (gun2 == false) {
      if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
        act(g, "@c$N's@W fireshield burns your weapon!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n's@W weapon is burned by your fireshield!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true, ch, nullptr, vict, TO_NOTVICT);
        int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
        hurt(g, 0, 0, vict, nullptr, char_get_equipment(ch, WEAR_WIELD2), damdam, 0);
      } else if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && (char_get_bonuses(ch, BONUS_FIREPROOF) || char_is_demon(ch))) {
        send_to_char(g, vict, "@RThey appear to be fireproof!@n\r\n");
      }
      pcost(g, ch, 0, stcost);
    }
  } else if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou attack $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W attacks $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    if (gun2 == false)
      pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}
