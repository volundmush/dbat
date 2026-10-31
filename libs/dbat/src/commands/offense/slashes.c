/* slashes.c: ki disk and slash attacks
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

ACMD(do_tslash)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .125, minimum = .1;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_TSLASH)) {
    return;
  }

  if (!char_has_arms(ch)) {
    send_to_char(ch, "You have no available arms!\r\n");
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

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(ch, "You are not wielding a sword, you need one to use this technique.\r\n");
    return;
  }

  struct obj_data *wobj = char_get_equipment(ch, WEAR_WIELD1);
  int wlvl              = 0;
  if (obj_extra_flagged(wobj, ITEM_WEAPLVL1)) {
    wlvl = 1;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL2)) {
    wlvl = 2;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL3)) {
    wlvl = 3;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL4)) {
    wlvl = 4;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL5)) {
    wlvl = 5;
  }

  if (char_get_skills_perf(ch, SKILL_TSLASH) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_TSLASH) == 3) {
    minimum -= 0.05;
    if (minimum <= 0.0) {
      minimum = 0.01;
    }
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_TSLASH); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 6);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_TSLASH, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_TSLASH) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Twin Slash")) {
      if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your twin slash!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W twin slash!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W twin slash!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 19, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your twin slash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W twin slash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W twin slash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 19, skill, SKILL_TSLASH); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your twin slash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a twin slash at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a twin slash at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your twin slash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a twin slash at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a twin slash at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 19, skill, attperc);
      if (char_get_skill(ch, SKILL_TSLASH) >= 100) {
        dmg += (dmg * 0.05) * wlvl;
      } else if (char_get_skill(ch, SKILL_TSLASH) >= 60) {
        dmg += (dmg * 0.02) * wlvl;
      } else if (char_get_skill(ch, SKILL_TSLASH) >= 40) {
        dmg += (dmg * 0.01) * wlvl;
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's body as you fly past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR body as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's body as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_has_tail(vict)) {
            act("@rYou cut off $S tail!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@rYour tail is cut off!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@R$N@r's tail is cut off!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            char_lose_tail(vict);
          }
          break;
        case 2: /* Critical */
          act("@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's face as you fly past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR face as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's face as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          if (dmg > getEffMaxPL(vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
            act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@rYou have your head cut off by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            char_set_death_type(vict, DTYPE_HEAD);
            remove_limb(vict, 0);
            die(vict, ch);
            if (char_aff_flagged(ch, AFF_GROUP)) {
              group_gain(ch, vict);
            } else {
              solo_gain(ch, vict);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
              do_get(ch, "all.zenni corpse", 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
              do_get(ch, "all corpse", 0, 0);
            }
          } else if (dmg > getEffMaxPL(vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
            if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(1, 101) && (getCurKI(vict)) >= getMaxKI(vict) / 40) {
              act("@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYou have your head cut off by the attack but regenerate a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              decCurKI(vict, getMaxKI(vict) / 40, 0);
              hurt(0, 0, ch, vict, NULL, dmg, 1);
            } else {
              act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYou have your head cut off by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_death_type(vict, DTYPE_HEAD);
              die(vict, ch);
              if (char_aff_flagged(ch, AFF_GROUP)) {
                group_gain(ch, vict);
              } else {
                solo_gain(ch, vict);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
                do_get(ch, "all.zenni corpse", 0, 0);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
                do_get(ch, "all corpse", 0, 0);
              }
            }
          } else {
            hurt(0, 0, ch, vict, NULL, dmg, 1);
          }
          dmg *= calc_critical(ch, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's gut as you fly past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR gut as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's gut as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's arm as you fly past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR arm as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's arm as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          if (rand_number(1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
            if (char_get_limb_condition(vict, (2) - 1) > 0 && !is_sparring(ch) && rand_number(1, 2) == 2) {
              act("@RYour attack severs $N's left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severs your left arm!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's left arm is severered in the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              char_set_limb_condition(vict, (2) - 1, 0);
              remove_limb(vict, 2);
            } else if (char_get_limb_condition(vict, (1) - 1) > 0 && !is_sparring(ch)) {
              act("@RYour attack severs $N's right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severs your right arm!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's right arm is severered in the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              char_set_limb_condition(vict, (1) - 1, 0);
              remove_limb(vict, 1);
            }
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's leg as you fly past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR leg as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's leg as $e flies past, leaving a green after-image behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          if (rand_number(1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
            if (char_get_limb_condition(vict, (4) - 1) > 0 && !is_sparring(ch) && rand_number(1, 2) == 2) {
              act("@RYour attack severs $N's left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severs your left leg!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's left leg is severered in the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              char_set_limb_condition(vict, (4) - 1, 0);
              remove_limb(vict, 4);
            } else if (char_get_limb_condition(vict, (3) - 1) > 0 && !is_sparring(ch)) {
              act("@RYour attack severs $N's right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severs your right leg!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's right leg is severered in the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              char_set_limb_condition(vict, (3) - 1, 0);
              remove_limb(vict, 3);
            }
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }

      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(ch, 19, skill, attperc);
    dmg /= 10;
    act("@WYou fire a twin slash at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a twin slash at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_pslash)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .3, minimum = .15;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_PSLASH)) {
    return;
  }

  if (!char_has_arms(ch)) {
    send_to_char(ch, "You have no available arms!\r\n");
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

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }
  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(ch, "You are not wielding a sword, you need one to use this technique.\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_PSLASH); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 8);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 3)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_PSLASH, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Phoenix Slash")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your Phoenix Slash!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W Phoenix Slash!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W Phoenix Slash!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 37, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your Phoenix Slash, letting it fly harmlessly by!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W Phoenix Slash, letting it letting it fly harmlessly by!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W Phoenix Slash, letting it fly harmlessly by!@n", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your Phoenix Slash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a Phoenix Slash at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a Phoenix Slash at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your Phoenix Slash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a Phoenix Slash at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a Phoenix Slash at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 37, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      if (check_ruby(ch) == 1) {
        dmg += dmg * 0.2;
      }
      if (char_get_bonuses(vict, BONUS_FIREPROOF)) {
        dmg -= dmg * 0.4;
      } else if (char_get_bonuses(vict, BONUS_FIREPRONE)) {
        dmg += dmg * 0.4;
      }
      char_aff_set(vict, AFF_ASHED, true);
      switch (hitspot) {
        case 1:
          act("@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S body in flames a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR body in flames a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's body in flames a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S head in flames a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR head in flames a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's head in flames a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S gut in flames a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR gut in flames a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's gut in flames a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S arm in flames a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR arm in flames a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's arm in flames a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S leg in flames a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR leg in flames a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's leg in flames a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(1, 4) == 4 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
        send_to_char(vict, "@RYou are burned by the attack!@n\r\n");
        send_to_char(ch, "@RThey are burned by the attack!@n\r\n");
        char_aff_set(vict, AFF_BURNED, true);
      } else if (char_get_bonuses(vict, BONUS_FIREPROOF) || char_is_demon(vict)) {
        send_to_char(ch, "@RThey appear to be fireproof!@n\r\n");
      } else if (char_get_bonuses(vict, BONUS_FIREPRONE)) {
        send_to_char(vict, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
        send_to_char(ch, "@RThey are easily burned!@n\r\n");
        char_aff_set(vict, AFF_BURNED, true);
      }
      pcost(ch, attperc, 0);
      char_aff_set(vict, AFF_ASHED, false);
      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(ch, 37, skill, attperc);
    dmg /= 10;
    if (obj_get_vnum(obj) == 79)
      dmg *= 3;
    act("@WYou fire a Phoenix Slash at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a Phoenix Slash at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_ddslash)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .2, minimum = .10;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_DDSLASH)) {
    return;
  }

  if (!char_has_arms(ch)) {
    send_to_char(ch, "You have no available arms!\r\n");
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

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }

  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(ch, "You are not wielding a sword, you need one to use this technique.\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_DDSLASH); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 7);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_DDSLASH, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Darkness Dragon Slash")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your Darkness Dragon Slash!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W Darkness Dragon Slash!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W Darkness Dragon Slash!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 30, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your Darkness Dragon Slash, letting it fly harmlessly by!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W Darkness Dragon Slash, letting it letting it fly harmlessly by!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W Darkness Dragon Slash, letting it fly harmlessly by!@n", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your Darkness Dragon Slash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a Darkness Dragon Slash at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a Darkness Dragon Slash at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your Darkness Dragon Slash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a Darkness Dragon Slash at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a Darkness Dragon Slash at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 30, skill, attperc);
      if (char_get_skill(ch, SKILL_DDSLASH) >= 100) {
        dmg += dmg * 0.15;
      } else if (char_get_skill(ch, SKILL_DDSLASH) >= 60) {
        dmg += dmg * 0.10;
      } else if (char_get_skill(ch, SKILL_DDSLASH) >= 40) {
        dmg += dmg * 0.05;
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (rand_number(1, 3) == 3 && !char_aff_flagged(vict, AFF_BLIND)) {
        act("@mYou are struck blind temporarily!@n", TRUE, vict, 0, 0, TO_CHAR);
        act("@c$n@m is struck blind by the attack!@n", TRUE, vict, 0, 0, TO_ROOM);
        int duration = 1;
        assign_affect(vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
      }
      pcost(ch, attperc, 0);

      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(ch, 30, skill, attperc);
    dmg /= 10;
    act("@WYou fire a Darkness Dragon Slash at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a Darkness Dragon Slash at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_kienzan)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .125, minimum = .05;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_KIENZAN)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_KIENZAN); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 7);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_KIENZAN, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Kienzan")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your kienzan, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W kienzan, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W kienzan, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wThe kienzan expands, cutting through everything in its path!\r\n");

          dodge_ki(ch, vict, 2, 25, skill, SKILL_KIENZAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your kienzan misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a kienzan at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a kienzan at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          dodge_ki(ch, vict, 2, 25, skill, SKILL_KIENZAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                              Num 2: [ Number of attack for damtype ]*/

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your kienzan misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a kienzan at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a kienzan at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);

        dodge_ki(ch, vict, 2, 25, skill, SKILL_KIENZAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 25, skill, attperc);
      if (char_get_skill(ch, SKILL_KIENZAN) >= 100) {
        dmg += dmg * 0.25;
      } else if (char_get_skill(ch, SKILL_KIENZAN) >= 60) {
        dmg += dmg * 0.15;
      } else if (char_get_skill(ch, SKILL_KIENZAN) >= 40) {
        dmg += dmg * 0.05;
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (dmg > getEffMaxPL(vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
            act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@rYou are cut in half by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            die(vict, ch);
            if (char_aff_flagged(ch, AFF_GROUP)) {
              group_gain(ch, vict);
            } else {
              solo_gain(ch, vict);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
              do_get(ch, "all.zenni corpse", 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
              do_get(ch, "all corpse", 0, 0);
            }
          } else if (dmg > getEffMaxPL(vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
            if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(1, 101) && (getCurKI(vict)) >= getMaxKI(vict) / 40) {
              act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYou are cut in half by the attack but regenerate a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r is cut in half by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              decCurKI(vict, getMaxKI(vict) / 40, 0);
              hurt(0, 0, ch, vict, NULL, dmg, 1);
            } else {
              act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYou are cut in half by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r is cut in half by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              die(vict, ch);
              if (char_aff_flagged(ch, AFF_GROUP)) {
                group_gain(ch, vict);
              } else {
                solo_gain(ch, vict);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
                do_get(ch, "all.zenni corpse", 0, 0);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
                do_get(ch, "all corpse", 0, 0);
              }
            }
          } else {
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            if (char_has_tail(vict)) {
              act("@rYou cut off $S tail!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYour tail is cut off!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r's tail is cut off!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_lose_tail(vict);
            }
          }
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's neck!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR neck!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's neck!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          if (dmg > getEffMaxPL(vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
            act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@rYou have your head cut off by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            char_set_death_type(vict, DTYPE_HEAD);
            remove_limb(vict, 0);
            die(vict, ch);
            if (char_aff_flagged(ch, AFF_GROUP)) {
              group_gain(ch, vict);
            } else {
              solo_gain(ch, vict);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
              do_get(ch, "all.zenni corpse", 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
              do_get(ch, "all corpse", 0, 0);
            }
          } else if (dmg > getEffMaxPL(vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
            if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(1, 101) && (getCurKI(vict)) >= getMaxKI(vict) / 40) {
              act("@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYou have your head cut off by the attack but regenerate a moment later!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              decCurKI(vict, getMaxKI(vict) / 40, 0);
              hurt(0, 0, ch, vict, NULL, dmg, 1);
            } else {
              act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@rYou have your head cut off by the attack!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N@r has $S head cut off by the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_death_type(vict, DTYPE_HEAD);
              die(vict, ch);
              if (char_aff_flagged(ch, AFF_GROUP)) {
                group_gain(ch, vict);
              } else {
                solo_gain(ch, vict);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOGOLD)) {
                do_get(ch, "all.zenni corpse", 0, 0);
              }
              if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(ch, PRF_AUTOLOOT)) {
                do_get(ch, "all corpse", 0, 0);
              }
            }
          } else {
            hurt(0, 0, ch, vict, NULL, dmg, 1);
          }
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          if (rand_number(1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
            if (char_get_limb_condition(vict, (2) - 1) > 0 && !is_sparring(ch) && rand_number(1, 2) == 2) {
              act("@RYour attack severes $N's left arm!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severes your left arm!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's left arm is severed in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_limb_condition(vict, (2) - 1, 0);
              remove_limb(vict, 2);
            } else if (char_get_limb_condition(vict, (1) - 1) > 0 && !is_sparring(ch)) {
              act("@RYour attack severes $N's right arm!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severes your right arm!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's right arm is severed in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_limb_condition(vict, (1) - 1, 0);
              remove_limb(vict, 1);
            }
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          if (rand_number(1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
            if (char_get_limb_condition(vict, (4) - 1) > 0 && !is_sparring(ch) && rand_number(1, 2) == 2) {
              act("@RYour attack severes $N's left leg!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severes your left leg!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's left leg is severed in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_limb_condition(vict, (4) - 1, 0);
              remove_limb(vict, 4);
            } else if (char_get_limb_condition(vict, (3) - 1) > 0 && !is_sparring(ch)) {
              act("@RYour attack severes $N's right leg!@n", TRUE, ch, 0, vict, TO_CHAR);
              act("@R$n's attack severes your right leg!@n", TRUE, ch, 0, vict, TO_VICT);
              act("@R$N's right leg is severed in the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
              char_set_limb_condition(vict, (3) - 1, 0);
              remove_limb(vict, 3);
            }
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      pcost(ch, attperc, 0);

      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(ch, 25, skill, attperc);
    dmg /= 10;
    act("@WYou fire a kienzan at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a kienzan at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
