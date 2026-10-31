/* melee_special.c: special melee attacks
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

ACMD(do_rogafufuken)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .05, minimum = .01;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_ROGAFUFUKEN)) {
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

  if (GET_MAX_MANA(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, GET_MAX_HIT(ch) / 50)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_ROGAFUFUKEN); /* Set skill value */

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
    improve_skill(ch, SKILL_ROGAFUFUKEN, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, FALSE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Rogafufuken")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your rogafufuken with a punch of $s own!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W rogafufuken with a punch of your own!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W rogafufuken with a punch of $s own!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, GET_MAX_HIT(vict) / 300);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your rogafufuken!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W rogafufuken!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W rogafufuken!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 23, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your rogafufuken!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W rogafufuken!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W rogafufuken!@n", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your rogafufuken misses!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a rogafufuken at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a rogafufuken at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your rogafufuken misses!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a rogafufuken at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a rogafufuken at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 23, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "hands and feet");
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
    dmg = damtype(ch, 23, skill, attperc);
    dmg /= 10;
    act("@WYou fire a rogafufuken at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a rogafufuken at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_heeldrop)
{
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_HEELDROP);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!check_skill(ch, SKILL_HEELDROP)) {
    return;
  }
  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 90)) {
    return;
  }

  skill = init_skill(ch, SKILL_HEELDROP);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (char_is_piccolo(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(ch, 7);
    else
      handle_cooldown(ch, 9);
  } else {
    handle_cooldown(ch, 9);
  }
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_HEELDROP, 1);
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, FALSE);
    perc  = chance_to_hit(ch);

    if (char_is_kabito(ch) && !char_is_npc(ch)) {
      if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
        perc -= perc * 0.2;
    }

    index -= handle_speed(ch, vict);

    avo = index / 4;
    if (!char_is_npc(vict)) {
      pry = handle_parry(vict);
      blk = char_get_skill(vict, SKILL_BLOCK);
      dge = char_get_skill(vict, SKILL_DODGE);
    }
    if (char_is_npc(vict) && char_get_level(ch) <= 10) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(20, 40);
        blk = rand_number(20, 40);
      }
      dge = rand_number(20, 40);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 20) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(20, 60);
        blk = rand_number(20, 60);
      }
      dge = rand_number(20, 60);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 30) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(20, 70);
        blk = rand_number(20, 70);
      }
      dge = rand_number(20, 70);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 40) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(30, 70);
        blk = rand_number(30, 70);
      }
      dge = rand_number(30, 70);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 40) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(40, 70);
        blk = rand_number(40, 70);
      }
      dge = rand_number(40, 70);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 60) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(40, 80);
        blk = rand_number(40, 80);
      }
      dge = rand_number(40, 80);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 70) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(50, 80);
        blk = rand_number(50, 80);
      }
      dge = rand_number(50, 80);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 80) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(50, 90);
        blk = rand_number(50, 90);
      }
      dge = rand_number(50, 90);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 90) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(60, 90);
        blk = rand_number(60, 90);
      }
      dge = rand_number(60, 90);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 100) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(70, 100);
        blk = rand_number(70, 100);
      }
      dge = rand_number(70, 100);
    }
    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Heeldrop")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your heeldrop with a punch of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W heeldrop with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W heeldrop with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your heeldrop!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W heeldrop!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W heeldrop!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 8, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your heeldrop!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W heeldrop!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W heeldrop!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your heeldrop misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves to heeldrop you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W moves to heeldrop @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your heeldrop misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W moves to heeldrop you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W moves to heeldrop @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg         = damtype(ch, 8, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou disappear, appearing above @C$N@W you spin and heeldrop $M in the face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W disappears, only to appear above you, spinning quickly and heeldropping you in the face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W disappears, only to appear above @C$N@W, spinning quickly and heeldropping $M in the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          tech_handle_crashdown(ch, vict);

          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou disappear, reappearing in front of @C$N@W, you flip upside down and slam your heel into the top of $S head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W disappears, reappearing in front of you, $e flips upside down and slams $s heel into the top of your head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W disappears, reappearing in front of @C$N@W, $e flips upside down and slams $s heel into the top of @C$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          tech_handle_crashdown(ch, vict);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou fly at @C$N@W, heeldropping $S gut as you fly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W flies at you, heeldropping your gut as $e flies!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W flies at @C$N@W, heeldropping $S gut as $e flies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou heeldrop @C$N@W, hitting $M in the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W heeldrops you, hitting you in the arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W heeldrops @C$N@W, hitting $M in the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 180, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou heeldrop @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W heeldrops your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W heeldrops @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 180, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "leg");
      pcost(ch, 0, stcost);
      handle_multihit(ch, vict);

      return;
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
    act("@WYou heeldrop $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W heeldrops $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_bite)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = 20 + (GET_MAX_HIT(ch) / 500);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!char_is_npc(ch) && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 7 && char_get_genome(ch, 1) != 7))) {
    send_to_char(ch, "You don't want to put that in your mouth, you don't know where it has been!\r\n");
    return;
  }
  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    return;
  }

  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 200)) {
    return;
  }

  skill = init_skill(ch, SKILL_PUNCH);

  if (skill <= 0) {
    skill = 60;
  }

  vict = NULL;
  obj  = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(ch);
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[char_get_in_room(ch)].contents))) {
      return;
    }
  }
  handle_cooldown(ch, 4);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, TRUE);
    perc  = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "bite")) {
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your bite with a punch of their own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W bite with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W bite with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -1, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your bite!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W bite!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W bite!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 6, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your bite!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W bite!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W bite!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou move to bite @C$N@W, but miss!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves to bite you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W moves to bite @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@C$n@W moves to bite you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W moves to bite @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, 8, skill, attperc);
      if (!char_is_npc(ch)) {
        dmg = damtype(ch, 0, skill, attperc);
      }
      dmg += dmg * 0.25;
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou bite @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W bites your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W bites into $N's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou bite @C$N@W!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W bites you!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W bites @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou bite @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W bites you on the body, sending blood flying!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W bites @c$N@W on the body, sending blood flying!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou bite @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W bites you on the arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W bites @c$N@W on the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou bite @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W bites into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W bites into @c$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (!char_is_npc(ch)) {
        if (axion_dice(0) > char_get_aff_abils_con(vict) && rand_number(1, 5) == 5) {
          act("@R$N@r was poisoned by your bite!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@rYou were poisoned by the bite!@n", TRUE, ch, 0, vict, TO_VICT);
          vict->poisonby = ch;
          int duration   = (char_get_aff_abils_intel(ch) / 50) + 1;
          assign_affect(vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
        }
      }

      pcost(ch, 0, stcost);

      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@C$n@W bites $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_slam)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_SLAM);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_SLAM)) {
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
  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 100)) {
    return;
  }

  skill = init_skill(ch, SKILL_SLAM);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (char_is_bardock(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(ch, 7);
    else
      handle_cooldown(ch, 9);
  } else {
    handle_cooldown(ch, 9);
  }

  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_SLAM, 1);
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, FALSE);
    perc  = chance_to_hit(ch);

    if (char_is_kabito(ch) && !char_is_npc(ch)) {
      if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
        perc -= perc * 0.2;
    }

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);
    if (!tech_handle_zanzoken(ch, vict, "slam")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your slam with a punch of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W slam with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W slam with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your slam!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W slam!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W slam!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 6, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your slam!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W slam!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W slam!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your slam misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves to slam you with both $s fists, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W moves to slam @C$N@W with both $s fists, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your slam misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W moves to slam you with both $s fists, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W moves to slam @C$N@W with both $s fists, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg         = damtype(ch, 6, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou disappear, appearing above @C$N@W and slam a double fisted blow into $M!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W disappears, only to appear above you, slamming a double fisted blow into you!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W disappears, only to appear above @C$N@W, slamming a double fisted blow into $M!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou disappear, reappearing in front of @C$N@W, you grab $M! Spinning you send $M flying into the ground!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W disappears, reappearing in front of you, and $e grabs you! Spinning quickly $e sends you flying into the ground!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W disappears, reappearing in front of @C$N@W, and grabs $M! Spinning quickly $e sends $M flying into the ground!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          if (!char_aff_flagged(vict, AFF_KNOCKED) && (rand_number(1, 4) >= 3 && (GET_HIT(vict) > GET_HIT(ch) / 5) && !char_aff_flagged(vict, AFF_SANCTUARY))) {
            act("@C$N@W is knocked out!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@WYou are knocked out!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$N@W is knocked out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            setStatusKnockedOut(vict);
          } else if ((char_get_position(vict) == POS_STANDING || char_get_position(vict) == POS_FIGHTING) && !char_aff_flagged(vict, AFF_KNOCKED)) {
            char_set_position(vict, POS_SITTING);
          }
          if (rroom_get_dmg(char_get_in_room(vict)) <= 95 && !rroom_flagged(char_get_in_room(vict), ROOM_SPACE)) {
            act("@W$N@W slams into the ground forming a large crater with $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@WYou slam into the ground forming a large crater with your body!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@W$N@W slams into the ground forming a large crater with $S body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            if (rroom_sect(char_get_in_room(vict)) != SECT_INSIDE && rroom_sect(char_get_in_room(vict)) != SECT_UNDERWATER && rroom_sect(char_get_in_room(vict)) != SECT_WATER_SWIM && rroom_sect(char_get_in_room(vict)) != SECT_WATER_NOSWIM) {
              impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
              switch (rand_number(1, 8)) {
                case 1:
                  act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_CHAR);
                  act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 2:
                  if (rand_number(1, 4) == 4 && rroom_get_geffect(char_get_in_room(vict)) == 0) {
                    rroom_set_geffect(char_get_in_room(vict), 1);
                    act("Lava leaks up through cracks in the crater!", TRUE, ch, 0, vict, TO_CHAR);
                    act("Lava leaks up through cracks in the crater!", TRUE, ch, 0, vict, TO_ROOM);
                  }
                  break;
                case 3:
                  act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, vict, TO_CHAR);
                  act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 4:
                  act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 5:
                  act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 6:
                  /* One less message */
                  break;
                default:
                  /* we want no message for the default */
                  break;
              }
            }
            if (rroom_sect(char_get_in_room(vict)) == SECT_UNDERWATER) {
              switch (rand_number(1, 3)) {
                case 1:
                  act("The water churns violently!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The water churns violently!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 2:
                  act("Large bubbles rise from the movement!", TRUE, ch, 0, vict, TO_CHAR);
                  act("Large bubbles rise from the movement!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 3:
                  act("The water collapses in on the hole created!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The water collapses in on the hole create!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
              }
            }
            if (rroom_sect(char_get_in_room(vict)) == SECT_WATER_SWIM || rroom_sect(char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
              switch (rand_number(1, 3)) {
                case 1:
                  act("A huge column of water erupts from the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("A huge column of water erupts from the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 2:
                  act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 3:
                  act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
              }
            }
            if (rroom_sect(char_get_in_room(vict)) == SECT_INSIDE) {
              impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
              switch (rand_number(1, 8)) {
                case 1:
                  act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_CHAR);
                  act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 2:
                  act("The structure of the surrounding room cracks and quakes from the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The structure of the surrounding room cracks and quakes from the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 3:
                  act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, vict, TO_CHAR);
                  act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 4:
                  act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 5:
                  act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                case 6:
                  act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, vict, TO_CHAR);
                  act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, vict, TO_ROOM);
                  break;
                default:
                  /* we want no message for the default */
                  break;
              }
            }
            rroom_set_dmg(char_get_in_room(vict), rroom_get_dmg(char_get_in_room(vict)) + (5));
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou fly at @C$N@W, slamming both your fists into $S gut as you fly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W flies at you, slamming both $s fists into your gut as $e flies!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W flies at @C$N@W, slamming both $s fists into $S gut as $e flies!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou slam both your fists into @C$N@W, hitting $M in the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams both $s fists into you, hitting you in the arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams both $s fists into @C$N@W, hitting $M in the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou slam both your fists into @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams both $s fists into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams both $s fists into @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "hands");
      pcost(ch, 0, stcost);
      handle_multihit(ch, vict);

      return;
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
    act("@WYou slam $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W slams $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_tailwhip)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_TAILWHIP);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_TAILWHIP)) {
    return;
  }
  if (!char_plr_flagged(ch, PLR_TAIL) && !char_is_npc(ch)) {
    send_to_char(ch, "You have no tail!\r\n");
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 120)) {
    return;
  }

  skill = init_skill(ch, SKILL_TAILWHIP);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  handle_cooldown(ch, 7);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_TAILWHIP, 1);
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, FALSE);
    perc  = chance_to_hit(ch);

    if (char_is_kabito(ch) && !char_is_npc(ch)) {
      if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
        perc -= perc * 0.2;
    }

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "tailwhip")) {
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your tailwhip with a punch of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W tailwhip with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W tailwhip with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your tailwhip!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W tailwhip!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W tailwhip!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 3, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your tailwhip!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W tailwhip!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W tailwhip!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your tailwhip misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws a tailwhip at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws a tailwhip at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your tailwhip misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws a tailwhip at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws a tailwhip at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg         = damtype(ch, 56, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou spin to swing your tail and slam it into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W spins to swing $s tail and slams it into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W spins to swing $s tail and slams it into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(1, 8) >= 7) {
            handle_knockdown(vict);
          }
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou flip forward and slam your tail into the top of @c$N@W's head brutally!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W flips forward and slams $s tail into the top of YOUR head brutally!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W flips forward and slams $s tail into the top of @c$N@W's head brutally!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(1, 8) >= 6) {
            handle_knockdown(vict);
          }
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou swing your tail and manage to slam it into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W swings $s tail and manages to slam it into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W swings $s tail and manages to slam it into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(1, 8) >= 7) {
            handle_knockdown(vict);
          }
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou swing your tail and manage to slam it into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W swings $s tail and manages to slam it into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W swings $s tail and manages to slam it into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou swing your tail and manage to slam it into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W swings $s tail and manages to slam it into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W swings $s tail and manages to slam it into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "tail");
      pcost(ch, 0, stcost);
      handle_multihit(ch, vict);

      return;
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
    act("@WYou tailwhip $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W tailwhips $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
