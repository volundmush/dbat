/* slashes.c: ki disk and slash attacks
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
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
#include "dbat/act.item.hpp"
#include "dbat/game_world.hpp"

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

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_TSLASH)) {
    return;
  }

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
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

  if (!*arg && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(g, ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(g, ch, "You are not wielding a sword, you need one to use this technique.\r\n");
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

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_TSLASH); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(g, ch, 6);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_TSLASH, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    if (char_get_skills_perf(ch, SKILL_TSLASH) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Twin Slash")) {
      if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your twin slash!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W twin slash!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W twin slash!@n", false, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
            pcost(g, ch, attperc - 0.05, 0);
          } else {
            pcost(g, ch, attperc, 0);
          }
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 19, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your twin slash, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W twin slash, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W twin slash, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(g, ch, vict, 0, 19, skill, SKILL_TSLASH); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
          }
          improve_skill(g, vict, SKILL_DODGE, 0);

          if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
            pcost(g, ch, attperc - 0.05, 0);
          } else {
            pcost(g, ch, attperc, 0);
          }
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your twin slash misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a twin slash at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a twin slash at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);

        if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
          pcost(g, ch, attperc - 0.05, 0);
        } else {
          pcost(g, ch, attperc, 0);
        }
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your twin slash misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a twin slash at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a twin slash at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 19, skill, attperc);
    if (char_get_skill(ch, SKILL_TSLASH) >= 100) {
      dmg += (dmg * 0.05) * wlvl;
    } else if (char_get_skill(ch, SKILL_TSLASH) >= 60) {
      dmg += (dmg * 0.02) * wlvl;
    } else if (char_get_skill(ch, SKILL_TSLASH) >= 40) {
      dmg += (dmg * 0.01) * wlvl;
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's body as you fly past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR body as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's body as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        if (char_has_tail(vict)) {
          act(g, "@rYou cut off $S tail!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYour tail is cut off!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r's tail is cut off!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_lose_tail(g, vict);
        }
        break;
      case 2: /* Critical */
        act(g, "@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's face as you fly past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR face as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's face as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        if (dmg > getEffMaxPL(g, vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
          act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou have your head cut off by the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HEAD);
          remove_limb(g, vict, 0);
          die(g, vict, ch);
          if (char_aff_flagged(ch, AFF_GROUP)) {
            group_gain(g, ch, vict);
          } else {
            solo_gain(g, ch, vict);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
            do_get(g, ch, writable{"all.zenni corpse"}, 0, 0);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
            do_get(g, ch, writable{"all corpse"}, 0, 0);
          }
        } else if (dmg > getEffMaxPL(g, vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
          if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(g, 1, 101) && (getCurKI(g, vict)) >= getMaxKI(g, vict) / 40) {
            act(g, "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYou have your head cut off by the attack but regenerate a moment later!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
            decCurKI(g, vict, getMaxKI(g, vict) / 40, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          } else {
            act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYou have your head cut off by the attack!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_set_death_type(vict, DTYPE_HEAD);
            die(g, vict, ch);
            if (char_aff_flagged(ch, AFF_GROUP)) {
              group_gain(g, ch, vict);
            } else {
              solo_gain(g, ch, vict);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
              do_get(g, ch, writable{"all.zenni corpse"}, 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
              do_get(g, ch, writable{"all corpse"}, 0, 0);
            }
          }
        } else {
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        }
        dmg *= calc_critical(g, ch, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's gut as you fly past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR gut as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's gut as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's arm as you fly past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR arm as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's arm as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        if (rand_number(g, 1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
          if (char_get_limb_condition(vict, (2) - 1) > 0 && !is_sparring(ch) && rand_number(g, 1, 2) == 2) {
            act(g, "@RYour attack severs $N's left arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severs your left arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's left arm is severered in the attack!@n", true, ch, nullptr, vict, TO_VICT);
            char_set_limb_condition(vict, (2) - 1, 0);
            remove_limb(g, vict, 2);
          } else if (char_get_limb_condition(vict, (1) - 1) > 0 && !is_sparring(ch)) {
            act(g, "@RYour attack severs $N's right arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severs your right arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's right arm is severered in the attack!@n", true, ch, nullptr, vict, TO_VICT);
            char_set_limb_condition(vict, (1) - 1, 0);
            remove_limb(g, vict, 1);
          }
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's leg as you fly past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR leg as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's leg as $e flies past, leaving a green after-image behind!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        if (rand_number(g, 1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
          if (char_get_limb_condition(vict, (4) - 1) > 0 && !is_sparring(ch) && rand_number(g, 1, 2) == 2) {
            act(g, "@RYour attack severs $N's left leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severs your left leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's left leg is severered in the attack!@n", true, ch, nullptr, vict, TO_VICT);
            char_set_limb_condition(vict, (4) - 1, 0);
            remove_limb(g, vict, 4);
          } else if (char_get_limb_condition(vict, (3) - 1) > 0 && !is_sparring(ch)) {
            act(g, "@RYour attack severs $N's right leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severs your right leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's right leg is severered in the attack!@n", true, ch, nullptr, vict, TO_VICT);
            char_set_limb_condition(vict, (3) - 1, 0);
            remove_limb(g, vict, 3);
          }
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    if (char_get_skills_perf(ch, SKILL_TSLASH) == 3 && attperc > minimum) {
      pcost(g, ch, attperc - 0.05, 0);
    } else {
      pcost(g, ch, attperc, 0);
    }

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(g, ch, 19, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a twin slash at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a twin slash at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
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

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_PSLASH)) {
    return;
  }

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
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

  if (!*arg && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0)) {
    return;
  }
  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(g, ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(g, ch, "You are not wielding a sword, you need one to use this technique.\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_PSLASH); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(g, ch, 8);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 3)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_PSLASH, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Phoenix Slash")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your Phoenix Slash!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W Phoenix Slash!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W Phoenix Slash!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 37, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your Phoenix Slash, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Phoenix Slash, letting it letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Phoenix Slash, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);

          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your Phoenix Slash misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Phoenix Slash at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Phoenix Slash at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);

        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your Phoenix Slash misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Phoenix Slash at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Phoenix Slash at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg         = damtype(g, ch, 37, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    if (check_ruby(g, ch) == 1) {
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
        act(g, "@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S body in flames a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR body in flames a moment later!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's body in flames a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S head in flames a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR head in flames a moment later!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's head in flames a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S gut in flames a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR gut in flames a moment later!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's gut in flames a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S arm in flames a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR arm in flames a moment later!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's arm in flames a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 170, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S leg in flames a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR leg in flames a moment later!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's leg in flames a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 170, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(g, 1, 4) == 4 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
      send_to_char(g, vict, "@RYou are burned by the attack!@n\r\n");
      send_to_char(g, ch, "@RThey are burned by the attack!@n\r\n");
      char_aff_set(vict, AFF_BURNED, true);
    } else if (char_get_bonuses(vict, BONUS_FIREPROOF) || char_is_demon(vict)) {
      send_to_char(g, ch, "@RThey appear to be fireproof!@n\r\n");
    } else if (char_get_bonuses(vict, BONUS_FIREPRONE)) {
      send_to_char(g, vict, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
      send_to_char(g, ch, "@RThey are easily burned!@n\r\n");
      char_aff_set(vict, AFF_BURNED, true);
    }
    pcost(g, ch, attperc, 0);
    char_aff_set(vict, AFF_ASHED, false);
    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(g, ch, 37, skill, attperc);
    dmg /= 10;
    if (obj_get_vnum(g, obj) == 79)
      dmg *= 3;
    act(g, "@WYou fire a Phoenix Slash at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a Phoenix Slash at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
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

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_DDSLASH)) {
    return;
  }

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
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

  if (!*arg && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0)) {
    return;
  }

  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(g, ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(g, ch, "You are not wielding a sword, you need one to use this technique.\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_DDSLASH); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(g, ch, 7);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_DDSLASH, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Darkness Dragon Slash")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your Darkness Dragon Slash!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W Darkness Dragon Slash!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W Darkness Dragon Slash!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 30, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your Darkness Dragon Slash, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Darkness Dragon Slash, letting it letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Darkness Dragon Slash, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);

          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your Darkness Dragon Slash misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Darkness Dragon Slash at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Darkness Dragon Slash at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);

        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your Darkness Dragon Slash misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Darkness Dragon Slash at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Darkness Dragon Slash at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 30, skill, attperc);
    if (char_get_skill(ch, SKILL_DDSLASH) >= 100) {
      dmg += dmg * 0.15;
    } else if (char_get_skill(ch, SKILL_DDSLASH) >= 60) {
      dmg += dmg * 0.10;
    } else if (char_get_skill(ch, SKILL_DDSLASH) >= 40) {
      dmg += dmg * 0.05;
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's head!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR head!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's head!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 180, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 180, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    if (rand_number(g, 1, 3) == 3 && !char_aff_flagged(vict, AFF_BLIND)) {
      act(g, "@mYou are struck blind temporarily!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@m is struck blind by the attack!@n", true, vict, nullptr, nullptr, TO_ROOM);
      int duration = 1;
      assign_affect(g, vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
    }
    pcost(g, ch, attperc, 0);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(g, ch, 30, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a Darkness Dragon Slash at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a Darkness Dragon Slash at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
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

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_KIENZAN)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_KIENZAN); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(g, ch, 7);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_KIENZAN, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Kienzan")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your kienzan, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W kienzan, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W kienzan, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wThe kienzan expands, cutting through everything in its path!\r\n");

          dodge_ki(g, ch, vict, 2, 25, skill, SKILL_KIENZAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
          }
          improve_skill(g, vict, SKILL_DODGE, 0);

          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your kienzan misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a kienzan at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a kienzan at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);

        dodge_ki(g, ch, vict, 2, 25, skill, SKILL_KIENZAN); /* Effects on the room from dodging a ki attack
                             Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                            Num 2: [ Number of attack for damtype ]*/

        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your kienzan misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a kienzan at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a kienzan at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);

      dodge_ki(g, ch, vict, 2, 25, skill, SKILL_KIENZAN); /* Effects on the room from dodging a ki attack
                             Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                             Num 2: [ Number of attack for damtype ]*/

      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 25, skill, attperc);
    if (char_get_skill(ch, SKILL_KIENZAN) >= 100) {
      dmg += dmg * 0.25;
    } else if (char_get_skill(ch, SKILL_KIENZAN) >= 60) {
      dmg += dmg * 0.15;
    } else if (char_get_skill(ch, SKILL_KIENZAN) >= 40) {
      dmg += dmg * 0.05;
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (dmg > getEffMaxPL(g, vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
          act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou are cut in half by the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          die(g, vict, ch);
          if (char_aff_flagged(ch, AFF_GROUP)) {
            group_gain(g, ch, vict);
          } else {
            solo_gain(g, ch, vict);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
            do_get(g, ch, writable{"all.zenni corpse"}, 0, 0);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
            do_get(g, ch, writable{"all corpse"}, 0, 0);
          }
        } else if (dmg > getEffMaxPL(g, vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
          if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(g, 1, 101) && (getCurKI(g, vict)) >= getMaxKI(g, vict) / 40) {
            act(g, "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYou are cut in half by the attack but regenerate a moment later!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
            decCurKI(g, vict, getMaxKI(g, vict) / 40, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          } else {
            act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYou are cut in half by the attack!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r is cut in half by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            die(g, vict, ch);
            if (char_aff_flagged(ch, AFF_GROUP)) {
              group_gain(g, ch, vict);
            } else {
              solo_gain(g, ch, vict);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
              do_get(g, ch, writable{"all.zenni corpse"}, 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
              do_get(g, ch, writable{"all corpse"}, 0, 0);
            }
          }
        } else {
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          if (char_has_tail(vict)) {
            act(g, "@rYou cut off $S tail!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYour tail is cut off!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r's tail is cut off!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_lose_tail(g, vict);
          }
        }
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's neck!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR neck!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's neck!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        if (dmg > getEffMaxPL(g, vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
          act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@rYou have your head cut off by the attack!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
          char_set_death_type(vict, DTYPE_HEAD);
          remove_limb(g, vict, 0);
          die(g, vict, ch);
          if (char_aff_flagged(ch, AFF_GROUP)) {
            group_gain(g, ch, vict);
          } else {
            solo_gain(g, ch, vict);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
            do_get(g, ch, writable{"all.zenni corpse"}, 0, 0);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
            do_get(g, ch, writable{"all corpse"}, 0, 0);
          }
        } else if (dmg > getEffMaxPL(g, vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
          if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(g, 1, 101) && (getCurKI(g, vict)) >= getMaxKI(g, vict) / 40) {
            act(g, "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYou have your head cut off by the attack but regenerate a moment later!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", true, ch, nullptr, vict, TO_NOTVICT);
            decCurKI(g, vict, getMaxKI(g, vict) / 40, 0);
            hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          } else {
            act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@rYou have your head cut off by the attack!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N@r has $S head cut off by the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_set_death_type(vict, DTYPE_HEAD);
            die(g, vict, ch);
            if (char_aff_flagged(ch, AFF_GROUP)) {
              group_gain(g, ch, vict);
            } else {
              solo_gain(g, ch, vict);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
              do_get(g, ch, writable{"all.zenni corpse"}, 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
              do_get(g, ch, writable{"all corpse"}, 0, 0);
            }
          }
        } else {
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        }
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        if (rand_number(g, 1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
          if (char_get_limb_condition(vict, (2) - 1) > 0 && !is_sparring(ch) && rand_number(g, 1, 2) == 2) {
            act(g, "@RYour attack severes $N's left arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severes your left arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's left arm is severed in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_set_limb_condition(vict, (2) - 1, 0);
            remove_limb(g, vict, 2);
          } else if (char_get_limb_condition(vict, (1) - 1) > 0 && !is_sparring(ch)) {
            act(g, "@RYour attack severes $N's right arm!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severes your right arm!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's right arm is severed in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_set_limb_condition(vict, (1) - 1, 0);
            remove_limb(g, vict, 1);
          }
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        if (rand_number(g, 1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
          if (char_get_limb_condition(vict, (4) - 1) > 0 && !is_sparring(ch) && rand_number(g, 1, 2) == 2) {
            act(g, "@RYour attack severes $N's left leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severes your left leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's left leg is severed in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_set_limb_condition(vict, (4) - 1, 0);
            remove_limb(g, vict, 4);
          } else if (char_get_limb_condition(vict, (3) - 1) > 0 && !is_sparring(ch)) {
            act(g, "@RYour attack severes $N's right leg!@n", true, ch, nullptr, vict, TO_CHAR);
            act(g, "@R$n's attack severes your right leg!@n", true, ch, nullptr, vict, TO_VICT);
            act(g, "@R$N's right leg is severed in the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
            char_set_limb_condition(vict, (3) - 1, 0);
            remove_limb(g, vict, 3);
          }
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    pcost(g, ch, attperc, 0);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(g, ch, 25, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a kienzan at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a kienzan at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}
