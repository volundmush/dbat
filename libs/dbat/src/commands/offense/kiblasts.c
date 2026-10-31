/* kiblasts.c: ki blast attacks
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

ACMD(do_eraser)
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

  if (!check_skill(ch, SKILL_ERASER)) {
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

  skill = init_skill(ch, SKILL_ERASER); /* Set skill value */

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
    improve_skill(ch, SKILL_ERASER, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Eraser Cannon")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      if (char_get_skill(ch, SKILL_ERASER) >= 100) {
        decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
      } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
        decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
      } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
        decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
      }

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your eraser cannon!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W eraser cannon!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W eraser cannon!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          if (char_get_skill(ch, SKILL_ERASER) >= 100) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
          } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
          } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
          }
          dmg = damtype(ch, 18, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your eraser cannon, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W eraser cannon, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W eraser cannon, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 18, skill, SKILL_ERASER); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          pcost(ch, attperc, 0);
          if (char_get_skill(ch, SKILL_ERASER) >= 100) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
          } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
          } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your eraser cannon misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a eraser cannon at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a eraser cannon at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          if (char_get_skill(ch, SKILL_ERASER) >= 100) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
          } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
          } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
            decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your eraser cannon misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a eraser cannon at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a eraser cannon at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
        if (char_get_skill(ch, SKILL_ERASER) >= 100) {
          decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
        } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
          decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
        } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
          decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 18, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
        case 3:
          act("@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      pcost(ch, attperc, 0);
      if (char_get_skill(ch, SKILL_ERASER) >= 100) {
        decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
      } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
        decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
      } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
        decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
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
    dmg = damtype(ch, 18, skill, attperc);
    dmg /= 10;
    act("@WYou fire a eraser cannon at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a eraser cannon at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);
    if (char_get_skill(ch, SKILL_ERASER) >= 100) {
      decCurKI(ch, (getMaxKI(ch) * attperc) * 0.15, 0);
    } else if (char_get_skill(ch, SKILL_ERASER) >= 60) {
      decCurKI(ch, (getMaxKI(ch) * attperc) * 0.1, 0);
    } else if (char_get_skill(ch, SKILL_ERASER) >= 40) {
      decCurKI(ch, (getMaxKI(ch) * attperc) * 0.05, 0);
    }

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_bigbang)
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

  if (!check_skill(ch, SKILL_BIGBANG)) {
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

  skill = init_skill(ch, SKILL_BIGBANG); /* Set skill value */

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
    improve_skill(ch, SKILL_BIGBANG, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Big Bang")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your Big Bang!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W Big Bang!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W Big Bang!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 36, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your Big Bang, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W Big Bang, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W Big Bang, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 36, skill, SKILL_BIGBANG); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 80) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (20));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your Big Bang misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a Big Bang at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a Big Bang at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your Big Bang misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a Big Bang at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a Big Bang at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 36, skill, attperc);
      switch (rand_number(1, 6)) {
        case 1:
          act("@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's body, and explodes leaving behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR body, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's body, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
        case 3:
          act("@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's head, and explodes leaving behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR head, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's head, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4:
          act("@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's gut, and explodes leaving behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR gut, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's gut, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak */
          act("@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's arm, and explodes leaving behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR arm, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's arm, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 6: /* Weak 2 */
          act("@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's leg, and explodes leaving behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR leg, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's leg, and explodes leading behind a mushroom cloud shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 170, ch, vict, NULL, dmg, 1);
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
    dmg = damtype(ch, 36, skill, attperc);
    dmg /= 10;
    act("@WYou fire a Big Bang at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a Big Bang at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_balefire)
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

  if (!check_skill(ch, SKILL_BALEFIRE)) {
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

  skill = init_skill(ch, SKILL_BALEFIRE); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  int cool = 8;

  if (char_is_piccolo(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 100)
      cool -= 3;
    else if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      cool -= 2;
    else if (char_get_skills_base(ch, SKILL_STYLE) >= 40)
      cool -= 1;
  }

  if (cool < 1)
    cool = 1;

  handle_cooldown(ch, cool);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 3)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_BALEFIRE, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    prob += rand_number(10, 20);

    if (!tech_handle_zanzoken(ch, vict, "Balefire")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks every kiball of your scatter shot!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block every kiball of @C$n's@W scatter shot!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks every kiball of @c$n's@W scatter shot!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 35, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your balefire, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W balefire, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W balefire, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wBright explosions erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 35, skill, SKILL_BALEFIRE); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 80) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (20));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but all the kiballs of your scatter shot miss, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires scatter shot kiballs at you, but they miss!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires scatter shot kiballs at @C$N@W, but somehow they miss!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but all of your balefire misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires balefire at you, but they miss!@n ", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires balefire at @C$N@W, but somehow they miss!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 35, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 170, ch, vict, NULL, dmg, 1);
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
    dmg = damtype(ch, 35, skill, attperc);
    dmg /= 10;
    act("@WYou fire balefire at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires balefire at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_hellflash)
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

  if (!check_skill(ch, SKILL_HELLFLASH)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3) {
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

  skill = init_skill(ch, SKILL_HELLFLASH); /* Set skill value */

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
    improve_skill(ch, SKILL_HELLFLASH, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Hell Flash")) {
      if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3 && attperc > minimum) {
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
          act("@C$N@W moves quickly and blocks your Hell Flash!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W Hell Flash!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W Hell Flash!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 32, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your Hell Flash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W Hell Flash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W Hell Flash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 32, skill, SKILL_HELLFLASH); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);
          if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }

          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your Hell Flash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a Hell Flash at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a Hell Flash at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your Hell Flash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a Hell Flash at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a Hell Flash at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 32, skill, attperc);
      if (char_get_barrier(vict) > 0) {
        char_set_barrier(vict, char_get_barrier(vict) - (dmg));
        if (char_get_barrier(vict) <= 0) {
          char_set_barrier(vict, 1);
        }
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (char_get_skills_perf(ch, SKILL_HELLFLASH) == 3 && attperc > minimum) {
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
    dmg = damtype(ch, 32, skill, attperc);
    dmg /= 10;
    act("@WYou fire a Hell Flash at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a Hell Flash at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_baku)
{
  int perc, dge = 2, count = 0, skill;
  int64_t dmg;
  double attperc = .15, minimum = .05;
  struct char_data *vict = NULL, *next_v = NULL;
  char arg2[MAX_INPUT_LENGTH];

  one_argument(argument, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_BAKUHATSUHA)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }

  if (rroom_flagged(char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_BAKUHATSUHA); /* Set skill value */

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
      continue;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    } else {
      count += 1;
    }
  }
  if (count <= 0) {
    send_to_char(ch, "There is no one worth targeting around.\r\n");
    return;
  } /* No one worth targeting */
  else {
    perc = chance_to_hit(ch);
    handle_cooldown(ch, 6);
    if (skill < perc) {
      act("@WYou raise your hand with index and middle fingers extended upwards. You try releasing your charged ki in a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W but mess up and waste the ki!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W raises $s hand with index and middle fingers extended upwards. @C$n@W tries releasing $s charged ki in a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W but messes up and wastes the ki!@n", TRUE, ch, 0, 0, TO_ROOM);
      pcost(ch, attperc, 0);

      improve_skill(ch, SKILL_BAKUHATSUHA, 0);
      return;
    }

    act("@WYou raise your hand with index and middle fingers extended upwards. A sudden burst rushes up from the ground as your charged ki explodes in the form of a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W raises $s hand with index and middle fingers extended upwards. A sudden burst rushes up from the ground as $s charged ki explodes in the form of a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W!@n", TRUE, ch, 0, 0, TO_ROOM);

    dmg = damtype(ch, 24, skill, attperc);

    if (char_get_skill(ch, SKILL_BAKUHATSUHA) >= 100) {
      dmg += getEffMaxPL(ch) * 0.08;
    } else if (char_get_skill(ch, SKILL_BAKUHATSUHA) >= 60) {
      dmg += getEffMaxPL(ch) * 0.04;
    } else if (char_get_skill(ch, SKILL_BAKUHATSUHA) >= 40) {
      dmg += getEffMaxPL(ch) * 0.02;
    }

    switch (count) {
      case 1: /* full damage */
        break;
      case 2:
        dmg = (dmg / 100) * 75;
        break;
      case 3:
        dmg = (dmg / 100) * 50;
        break;
      default:
        dmg = (dmg / 100) * 25;
        break;
    }
    for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (vict == ch) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_GROUP) && (vict->master == ch || ch->master == vict || vict->master == ch->master)) {
        continue;
      }
      if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
        continue;
      }
      if (char_mob_flagged(vict, MOB_NOKILL)) {
        continue;
      }
      dge = handle_dodge(vict);
      if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        pcost(vict, 0, getEffMaxPL(vict) / 200);
        hurt(0, 0, ch, vict, NULL, 0, 1);
        continue;
      } else if (dge + rand_number(-15, 5) > skill) {
        act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@WYou manage to escape the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, 0, 1);
        improve_skill(vict, SKILL_DODGE, 0);
        continue;
      } else {
        act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@RYou are caught by the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING) {
          handle_knockdown(vict);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 1);
        continue;
      }
    } /* Hitting targets! */

    pcost(ch, attperc, 0);

    improve_skill(ch, SKILL_BAKUHATSUHA, 0);
    return;
  } /* We have targets! Attempt to kills them! */
}

ACMD(do_shogekiha)
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

  if (!check_skill(ch, SKILL_SHOGEKIHA)) {
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

  skill = init_skill(ch, SKILL_SHOGEKIHA); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  if (!char_is_kabito(ch)) {
    handle_cooldown(ch, 5);
  } else {
    if (char_get_skill(ch, SKILL_SHOGEKIHA) < 100) {
      handle_cooldown(ch, 5);
    }
  }
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }

    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_SHOGEKIHA, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    /* Shogekiha Mastery Bonus Area of Funstuff */
    int master_roll = rand_number(1, 100), master_chance = 0, master_pass = FALSE;

    if (skill >= 100)
      master_chance = 20;
    else if (skill >= 75)
      master_chance = 10;
    else if (skill >= 50)
      master_chance = 5;

    if (master_chance >= master_roll)
      master_pass = TRUE;

    /*==========================================*/

    if (!tech_handle_zanzoken(ch, vict, "Shogekiha")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks shogekiha!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W shogekiha!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W shogekiha!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 10, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your shogekiha, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W shogekiha, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W shogekiha, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your shogekiha misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W aims $s hand and releases a shogekiha at you, but misses!@n ", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W aims $s hand and releases a shogekiha at @C$N@W, but somehow misses!@n ", TRUE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your shogekiha misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W aims $s hand and releases a shogekiha at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W aims $s hand and releases a shogekiha at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 10, skill, attperc);
      if (char_is_kabito(ch)) {
        if (char_get_skill(ch, SKILL_SHOGEKIHA) >= 100) {
          incCurKI(ch, (getMaxKI(ch) * attperc) * .15, true);
        } else if (char_get_skill(ch, SKILL_SHOGEKIHA) >= 60) {
          incCurKI(ch, (getMaxKI(ch) * attperc) * .1, true);
        } else if (char_get_skill(ch, SKILL_SHOGEKIHA) >= 40) {
          incCurKI(ch, (getMaxKI(ch) * attperc) * .05, true);
        }
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (master_pass == TRUE) {
        act("@CYour skillful shogekiha disipated some of @c$N's@C charged ki!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@C's skillful shogekiha disipated some of YOUR charged ki!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@C's skillful shogekiha disipated some of @c$N's@C charged ki!", TRUE, ch, 0, vict, TO_NOTVICT);
        char_set_charge(vict, char_get_charge(vict) - (char_get_charge(vict) * 0.25));
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
    if (obj_get_kicharge(obj) > 0 && char_get_charge(ch) > obj_get_kicharge(obj)) {
      act("@WYou leap at $p@W with your arms spread out to your sides. As you are about to make contact with $p@W you scream and shatter the attack with your ki!@n", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@W leaps out at $p@W with $s arms spead out to $s sides. As $e is about to make contact with $p@W $e screams and shatters the attack with $s ki!@n", TRUE, ch, obj, 0, TO_ROOM);
      obj_set_kicharge(obj, obj_get_kicharge(obj) - (char_get_charge(ch)));
      extract_obj(obj);
    } else if (obj_get_kicharge(obj) > 0 && char_get_charge(ch) < obj_get_kicharge(obj)) {
      act("@WYou leap at $p@W with your arms spread out to your sides. As you are about to make contact with $p@W you scream and weaken the attack with your ki before taking the rest of the attack in the chest!@n", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@W leaps out at $p@W with $s arms spead out to $s sides. As $e is about to make contact with $p@W $e screams and weakens the attack with $s ki before taking the rest of the attack in the chest!@n", TRUE, ch, obj, 0, TO_ROOM);
      obj_set_kicharge(obj, obj_get_kicharge(obj) - (char_get_charge(ch)));
      char_set_charge(ch, 0);
      dmg = obj_get_kicharge(obj);
      hurt(0, 0, obj_get_user(obj), ch, NULL, dmg, 0);
      extract_obj(obj);
    } else {
      dmg = damtype(ch, 10, skill, attperc);
      dmg /= 10;
      act("@WYou fire a shogekiha at $p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@W fires a shogekiha at $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
      hurt(0, 0, ch, NULL, obj, dmg, 0);
      pcost(ch, attperc, 0);
    }

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_kiblast)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg;
  double attperc = .075, minimum = .01;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_KIBLAST)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(ch, "Your hands are full!\r\n");
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

  skill = init_skill(ch, SKILL_KIBLAST);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (!char_is_android(ch) || char_get_skill(ch, SKILL_KIBLAST) < 100) {
    handle_cooldown(ch, 5);
  }
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_KIBLAST, 0);
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, TRUE);
    perc  = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    /* Kiblast mastery Area */

    int mastery = rand_number(1, 100), master_pass = FALSE, chance = 0;

    if (skill >= 100)
      chance = 30;
    else if (skill >= 75)
      chance = 20;
    else if (skill >= 50)
      chance = 15;

    if (mastery <= chance)
      master_pass = TRUE;

    /*======================*/

    if (!tech_handle_zanzoken(ch, vict, "kiblast")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W deflects your kiblast, sending it flying away!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou deflect @C$n's@W kiblast sending it flying away!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W deflects @c$n's@W kiblast sending it flying away!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(vict, 0, getEffMaxPL(vict) / 200);

          parry_ki(attperc, ch, vict, "kiblast", prob, perc, skill, 9);
          /*      User/target/skill name/skill/hurt type */
          pcost(ch, attperc, 0);

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks kiblast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W kiblast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W kiblast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 9, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your kiblast, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W kiblast, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W kiblast, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);

          dodge_ki(ch, vict, 0, 9, skill, SKILL_KIBLAST); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your kiblast misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a bright yellow kiblast at you, but misses!@n ", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a bright yellow kiblast at @C$N@W, but somehow misses!@n ", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your kiblast misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a bright yellow kiblast at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a bright yellow kiblast at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 9, skill, attperc);
      if (char_is_android(ch)) {
        if (char_get_skill(ch, SKILL_KIBLAST) >= 100) {
          dmg += dmg * 0.15;
        } else if (char_get_skill(ch, SKILL_KIBLAST) >= 60) {
          dmg += dmg * 0.10;
        } else if (char_get_skill(ch, SKILL_KIBLAST) >= 40) {
          dmg += dmg * 0.05;
        }
      }
      int64_t record = getCurPL(vict);
      int hitspot    = 1;
      hitspot        = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (master_pass == TRUE && record > getCurPL(vict) && (record - getCurPL(vict) > (getEffMaxPL(vict)) * 0.025)) {
        if (!char_aff_flagged(vict, AFF_KNOCKED) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
          act("@C$N@W is knocked out!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou are knocked out!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W is knocked out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          setStatusKnockedOut(vict);
        }
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
    dmg = damtype(ch, 10, skill, attperc);
    dmg /= 10;
    act("@WYou fire a kiblast at $p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a kiblast at $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
