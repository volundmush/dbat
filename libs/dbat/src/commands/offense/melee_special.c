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
#include "dbat/game_world.h"

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

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_ROGAFUFUKEN)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, getEffMaxPL(g, ch) / 50)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_ROGAFUFUKEN); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(g, ch, 7);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_ROGAFUFUKEN, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, false);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Rogafufuken")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your rogafufuken with a punch of $s own!@n", false, ch, 0, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W rogafufuken with a punch of your own!@n", false, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W rogafufuken with a punch of $s own!@n", false, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, getEffMaxPL(g, vict) / 300);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your rogafufuken!@n", false, ch, 0, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W rogafufuken!@n", false, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W rogafufuken!@n", false, ch, 0, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 23, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your rogafufuken!@n", false, ch, 0, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W rogafufuken!@n", false, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W rogafufuken!@n", false, ch, 0, vict, TO_NOTVICT);

          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your rogafufuken misses!@n", false, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W fires a rogafufuken at you, but misses!@n ", false, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W fires a rogafufuken at @C$N@W, but somehow misses!@n ", false, ch, 0, vict, TO_NOTVICT);

        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your rogafufuken misses!@n", false, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W fires a rogafufuken at you, but misses!@n", false, ch, 0, vict, TO_VICT);
      act(g, "@c$n@W fires a rogafufuken at @C$N@W, but somehow misses!@n", false, ch, 0, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg         = damtype(g, ch, 23, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's body!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR body!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's body!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's face!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR face!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's face!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's gut!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR gut!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's gut!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's arm!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR arm!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's arm!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 170, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's leg!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR leg!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's leg!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 170, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "hands and feet");
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
    dmg = damtype(g, ch, 23, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a rogafufuken at $p@W!@n", false, ch, obj, 0, TO_CHAR);
    act(g, "@C$n@W fires a rogafufuken at $p@W!@n", false, ch, obj, 0, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_heeldrop)
{
  int prob, perc, avo, index = 0, pry = 0, dge = 0, blk = 0, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_HEELDROP);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!check_skill(g, ch, SKILL_HEELDROP)) {
    return;
  }
  if (!can_grav(g, ch)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 90)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_HEELDROP);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  if (char_is_piccolo(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(g, ch, 7);
    else
      handle_cooldown(g, ch, 9);
  } else {
    handle_cooldown(g, ch, 9);
  }
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_HEELDROP, 1);
    index = check_def(g, vict);
    prob  = roll_accuracy(g, ch, skill, false);
    perc  = chance_to_hit(g, ch);

    if (char_is_kabito(ch) && !char_is_npc(ch)) {
      if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
        perc -= perc * 0.2;
    }

    index -= handle_speed(g, ch, vict);

    avo = index / 4;
    if (!char_is_npc(vict)) {
      pry = handle_parry(g, vict);
      blk = char_get_skill(vict, SKILL_BLOCK);
      dge = char_get_skill(vict, SKILL_DODGE);
    }
    if (char_is_npc(vict) && char_get_level(ch) <= 10) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 20, 40);
        blk = rand_number(g, 20, 40);
      }
      dge = rand_number(g, 20, 40);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 20) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 20, 60);
        blk = rand_number(g, 20, 60);
      }
      dge = rand_number(g, 20, 60);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 30) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 20, 70);
        blk = rand_number(g, 20, 70);
      }
      dge = rand_number(g, 20, 70);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 40) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 30, 70);
        blk = rand_number(g, 30, 70);
      }
      dge = rand_number(g, 30, 70);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 40) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 40, 70);
        blk = rand_number(g, 40, 70);
      }
      dge = rand_number(g, 40, 70);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 60) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 40, 80);
        blk = rand_number(g, 40, 80);
      }
      dge = rand_number(g, 40, 80);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 70) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 50, 80);
        blk = rand_number(g, 50, 80);
      }
      dge = rand_number(g, 50, 80);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 80) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 50, 90);
        blk = rand_number(g, 50, 90);
      }
      dge = rand_number(g, 50, 90);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 90) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 60, 90);
        blk = rand_number(g, 60, 90);
      }
      dge = rand_number(g, 60, 90);
    } else if (char_is_npc(vict) && char_get_level(ch) <= 100) {
      if (char_is_humanoid(vict)) {
        pry = rand_number(g, 70, 100);
        blk = rand_number(g, 70, 100);
      }
      dge = rand_number(g, 70, 100);
    }
    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Heeldrop")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your heeldrop with a punch of $S own!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W heeldrop with a punch of your own!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W heeldrop with a punch of $S own!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 0);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your heeldrop!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W heeldrop!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W heeldrop!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 8, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your heeldrop!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W heeldrop!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W heeldrop!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your heeldrop misses!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W moves to heeldrop you, but misses!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W moves to heeldrop @C$N@W, but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your heeldrop misses!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W moves to heeldrop you, but misses!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@c$n@W moves to heeldrop @C$N@W, but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 8, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou disappear, appearing above @C$N@W you spin and heeldrop $M in the face!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W disappears, only to appear above you, spinning quickly and heeldropping you in the face!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W disappears, only to appear above @C$N@W, spinning quickly and heeldropping $M in the face!@n", true, ch, 0, vict, TO_NOTVICT);
        tech_handle_crashdown(g, ch, vict);

        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou disappear, reappearing in front of @C$N@W, you flip upside down and slam your heel into the top of $S head!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W disappears, reappearing in front of you, $e flips upside down and slams $s heel into the top of your head!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W disappears, reappearing in front of @C$N@W, $e flips upside down and slams $s heel into the top of @C$N@W's head!@n", true, ch, 0, vict, TO_NOTVICT);
        tech_handle_crashdown(g, ch, vict);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou fly at @C$N@W, heeldropping $S gut as you fly!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W flies at you, heeldropping your gut as $e flies!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W flies at @C$N@W, heeldropping $S gut as $e flies!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou heeldrop @C$N@W, hitting $M in the arm!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W heeldrops you, hitting you in the arm!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W heeldrops @C$N@W, hitting $M in the arm!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 180, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou heeldrop @C$N's@W leg!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W heeldrops your leg!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W heeldrops @C$N's@W leg!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 180, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "leg");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou heeldrop $p@W as hard as you can!@n", true, ch, obj, 0, TO_CHAR);
    act(g, "@C$n@W heeldrops $p@W extremely hard!@n", true, ch, obj, 0, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_bite)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = 20 + (getEffMaxPL(g, ch) / 500);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!char_is_npc(ch) && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 7 && char_get_genome(ch, 1) != 7))) {
    send_to_char(g, ch, "You don't want to put that in your mouth, you don't know where it has been!\r\n");
    return;
  }
  if (!can_grav(g, ch)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 200)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_PUNCH);

  if (skill <= 0) {
    skill = 60;
  }

  vict = nullptr;
  obj  = nullptr;
  if (!*arg || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(ch);
    } else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      return;
    }
  }
  handle_cooldown(g, ch, 4);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    index = check_def(g, vict);
    prob  = roll_accuracy(g, ch, skill, true);
    perc  = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "bite")) {
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your bite with a punch of their own!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W bite with a punch of your own!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W bite with a punch of $S own!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -1, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your bite!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W bite!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W bite!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 6, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your bite!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W bite!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W bite!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou move to bite @C$N@W, but miss!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W moves to bite you, but misses!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W moves to bite @C$N@W, but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@C$n@W moves to bite you, but misses!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@c$n@W moves to bite @C$N@W, but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg = damtype(g, ch, 8, skill, attperc);
    if (!char_is_npc(ch)) {
      dmg = damtype(g, ch, 0, skill, attperc);
    }
    dmg += dmg * 0.25;
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou bite @C$N's@W face!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W bites your face!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W bites into $N's face!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou bite @C$N@W!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W bites you!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W bites @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou bite @C$N's@W body!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W bites you on the body, sending blood flying!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W bites @c$N@W on the body, sending blood flying!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou bite @C$N's@W arm!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W bites you on the arm!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W bites @c$N@W on the arm!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou bite @C$N's@W leg!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W bites into your leg!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W bites into @c$N's@W leg!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    if (!char_is_npc(ch)) {
      if (axion_dice(g, 0) > char_get_aff_abils_con(vict) && rand_number(g, 1, 5) == 5) {
        act(g, "@R$N@r was poisoned by your bite!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@rYou were poisoned by the bite!@n", true, ch, 0, vict, TO_VICT);
        char_set_poisonby(vict, ch);
        int duration   = (char_get_aff_abils_intel(ch) / 50) + 1;
        assign_affect(g, vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
      }
    }

    pcost(g, ch, 0, stcost);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@C$n@W bites $p@W extremely hard!@n", true, ch, obj, 0, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_slam)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_SLAM);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_SLAM)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(g, ch, "Your hands are full!\r\n");
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 100)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_SLAM);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  if (char_is_bardock(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(g, ch, 7);
    else
      handle_cooldown(g, ch, 9);
  } else {
    handle_cooldown(g, ch, 9);
  }

  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_SLAM, 1);
    index = check_def(g, vict);
    prob  = roll_accuracy(g, ch, skill, false);
    perc  = chance_to_hit(g, ch);

    if (char_is_kabito(ch) && !char_is_npc(ch)) {
      if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
        perc -= perc * 0.2;
    }

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);
    if (!tech_handle_zanzoken(g, ch, vict, "slam")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your slam with a punch of $S own!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W slam with a punch of your own!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W slam with a punch of $S own!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your slam!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W slam!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W slam!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 6, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your slam!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W slam!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W slam!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your slam misses!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W moves to slam you with both $s fists, but misses!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W moves to slam @C$N@W with both $s fists, but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your slam misses!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W moves to slam you with both $s fists, but misses!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@c$n@W moves to slam @C$N@W with both $s fists, but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 6, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou disappear, appearing above @C$N@W and slam a double fisted blow into $M!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W disappears, only to appear above you, slamming a double fisted blow into you!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W disappears, only to appear above @C$N@W, slamming a double fisted blow into $M!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou disappear, reappearing in front of @C$N@W, you grab $M! Spinning you send $M flying into the ground!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W disappears, reappearing in front of you, and $e grabs you! Spinning quickly $e sends you flying into the ground!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W disappears, reappearing in front of @C$N@W, and grabs $M! Spinning quickly $e sends $M flying into the ground!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        if (!char_aff_flagged(vict, AFF_KNOCKED) && (rand_number(g, 1, 4) >= 3 && (getCurPL(g, vict) > getCurPL(g, ch) / 5) && !char_aff_flagged(vict, AFF_SANCTUARY))) {
          act(g, "@C$N@W is knocked out!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou are knocked out!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W is knocked out!@n", true, ch, 0, vict, TO_NOTVICT);
          setStatusKnockedOut(vict);
        } else if ((char_get_position(vict) == POS_STANDING || char_get_position(vict) == POS_FIGHTING) && !char_aff_flagged(vict, AFF_KNOCKED)) {
          char_set_position(vict, POS_SITTING);
        }
        if (rroom_get_dmg(g, char_get_in_room(vict)) <= 95 && !rroom_flagged(g, char_get_in_room(vict), ROOM_SPACE)) {
          act(g, "@W$N@W slams into the ground forming a large crater with $S body!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou slam into the ground forming a large crater with your body!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@W$N@W slams into the ground forming a large crater with $S body!@n", true, ch, 0, vict, TO_NOTVICT);
          if (rroom_sect(g, char_get_in_room(vict)) != SECT_INSIDE && rroom_sect(g, char_get_in_room(vict)) != SECT_UNDERWATER && rroom_sect(g, char_get_in_room(vict)) != SECT_WATER_SWIM && rroom_sect(g, char_get_in_room(vict)) != SECT_WATER_NOSWIM) {
            impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
            switch (rand_number(g, 1, 8)) {
              case 1:
                act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, 0, vict, TO_CHAR);
                act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, 0, vict, TO_ROOM);
                break;
              case 2:
                if (rand_number(g, 1, 4) == 4 && rroom_get_geffect(g, char_get_in_room(vict)) == 0) {
                  rroom_set_geffect(g, char_get_in_room(vict), 1);
                  act(g, "Lava leaks up through cracks in the crater!", true, ch, 0, vict, TO_CHAR);
                  act(g, "Lava leaks up through cracks in the crater!", true, ch, 0, vict, TO_ROOM);
                }
                break;
              case 3:
                act(g, "A cloud of dust envelopes the entire area!", true, ch, 0, vict, TO_CHAR);
                act(g, "A cloud of dust envelopes the entire area!", true, ch, 0, vict, TO_ROOM);
                break;
              case 4:
                act(g, "The surrounding area roars and shudders from the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "The surrounding area roars and shudders from the impact!", true, ch, 0, vict, TO_ROOM);
                break;
              case 5:
                act(g, "The ground shatters apart from the stress of the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "The ground shatters apart from the stress of the impact!", true, ch, 0, vict, TO_ROOM);
                break;
              case 6:
                /* One less message */
                break;
              default:
                /* we want no message for the default */
                break;
            }
          }
          if (rroom_sect(g, char_get_in_room(vict)) == SECT_UNDERWATER) {
            switch (rand_number(g, 1, 3)) {
              case 1:
                act(g, "The water churns violently!", true, ch, 0, vict, TO_CHAR);
                act(g, "The water churns violently!", true, ch, 0, vict, TO_ROOM);
                break;
              case 2:
                act(g, "Large bubbles rise from the movement!", true, ch, 0, vict, TO_CHAR);
                act(g, "Large bubbles rise from the movement!", true, ch, 0, vict, TO_ROOM);
                break;
              case 3:
                act(g, "The water collapses in on the hole created!", true, ch, 0, vict, TO_CHAR);
                act(g, "The water collapses in on the hole create!", true, ch, 0, vict, TO_ROOM);
                break;
            }
          }
          if (rroom_sect(g, char_get_in_room(vict)) == SECT_WATER_SWIM || rroom_sect(g, char_get_in_room(vict)) == SECT_WATER_NOSWIM) {
            switch (rand_number(g, 1, 3)) {
              case 1:
                act(g, "A huge column of water erupts from the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "A huge column of water erupts from the impact!", true, ch, 0, vict, TO_ROOM);
                break;
              case 2:
                act(g, "The impact briefly causes a swirling vortex of water!", true, ch, 0, vict, TO_CHAR);
                act(g, "The impact briefly causes a swirling vortex of water!", true, ch, 0, vict, TO_ROOM);
                break;
              case 3:
                act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, 0, vict, TO_ROOM);
                break;
            }
          }
          if (rroom_sect(g, char_get_in_room(vict)) == SECT_INSIDE) {
            impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
            switch (rand_number(g, 1, 8)) {
              case 1:
                act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, 0, vict, TO_CHAR);
                act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, 0, vict, TO_ROOM);
                break;
              case 2:
                act(g, "The structure of the surrounding room cracks and quakes from the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "The structure of the surrounding room cracks and quakes from the impact!", true, ch, 0, vict, TO_ROOM);
                break;
              case 3:
                act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, 0, vict, TO_CHAR);
                act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, 0, vict, TO_ROOM);
                break;
              case 4:
                act(g, "The surrounding area roars and shudders from the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "The surrounding area roars and shudders from the impact!", true, ch, 0, vict, TO_ROOM);
                break;
              case 5:
                act(g, "The ground shatters apart from the stress of the impact!", true, ch, 0, vict, TO_CHAR);
                act(g, "The ground shatters apart from the stress of the impact!", true, ch, 0, vict, TO_ROOM);
                break;
              case 6:
                act(g, "The walls of the surrounding room crack in the same instant!", true, ch, 0, vict, TO_CHAR);
                act(g, "The walls of the surrounding room crack in the same instant!", true, ch, 0, vict, TO_ROOM);
                break;
              default:
                /* we want no message for the default */
                break;
            }
          }
          rroom_set_dmg(g, char_get_in_room(vict), rroom_get_dmg(g, char_get_in_room(vict)) + (5));
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou fly at @C$N@W, slamming both your fists into $S gut as you fly!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W flies at you, slamming both $s fists into your gut as $e flies!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W flies at @C$N@W, slamming both $s fists into $S gut as $e flies!@n", true, ch, 0, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou slam both your fists into @C$N@W, hitting $M in the arm!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W slams both $s fists into you, hitting you in the arm!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W slams both $s fists into @C$N@W, hitting $M in the arm!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou slam both your fists into @C$N's@W leg!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W slams both $s fists into your leg!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W slams both $s fists into @C$N's@W leg!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "hands");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou slam $p@W as hard as you can!@n", true, ch, obj, 0, TO_CHAR);
    act(g, "@C$n@W slams $p@W extremely hard!@n", true, ch, obj, 0, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_tailwhip)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_TAILWHIP);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_TAILWHIP)) {
    return;
  }
  if (!char_plr_flagged(ch, PLR_TAIL) && !char_is_npc(ch)) {
    send_to_char(g, ch, "You have no tail!\r\n");
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 120)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_TAILWHIP);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 7);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_TAILWHIP, 1);
    index = check_def(g, vict);
    prob  = roll_accuracy(g, ch, skill, false);
    perc  = chance_to_hit(g, ch);

    if (char_is_kabito(ch) && !char_is_npc(ch)) {
      if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
        perc -= perc * 0.2;
    }

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "tailwhip")) {
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your tailwhip with a punch of $S own!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W tailwhip with a punch of your own!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W tailwhip with a punch of $S own!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your tailwhip!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W tailwhip!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W tailwhip!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 3, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your tailwhip!@n", true, ch, 0, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W tailwhip!@n", true, ch, 0, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W tailwhip!@n", true, ch, 0, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your tailwhip misses!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W throws a tailwhip at you but somehow misses!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@c$n@W throws a tailwhip at @C$N@W but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your tailwhip misses!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W throws a tailwhip at you but somehow misses!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@c$n@W throws a tailwhip at @C$N@W but somehow misses!@n", true, ch, 0, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 56, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou spin to swing your tail and slam it into @c$N@W's body!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W spins to swing $s tail and slams it into YOUR body!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W spins to swing $s tail and slams it into @c$N@W's body!@n", true, ch, 0, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 8) >= 7) {
          handle_knockdown(g, vict);
        }
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou flip forward and slam your tail into the top of @c$N@W's head brutally!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W flips forward and slams $s tail into the top of YOUR head brutally!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W flips forward and slams $s tail into the top of @c$N@W's head brutally!@n", true, ch, 0, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 8) >= 6) {
          handle_knockdown(g, vict);
        }
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou swing your tail and manage to slam it into @c$N@W's gut!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W swings $s tail and manages to slam it into YOUR gut!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W swings $s tail and manages to slam it into @c$N@W's gut!@n", true, ch, 0, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 8) >= 7) {
          handle_knockdown(g, vict);
        }
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou swing your tail and manage to slam it into @c$N@W's arm!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W swings $s tail and manages to slam it into YOUR arm!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W swings $s tail and manages to slam it into @c$N@W's arm!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou swing your tail and manage to slam it into @c$N@W's leg!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@C$n@W swings $s tail and manages to slam it into YOUR leg!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@C$n@W swings $s tail and manages to slam it into @c$N@W's leg!@n", true, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "tail");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou tailwhip $p@W as hard as you can!@n", true, ch, obj, 0, TO_CHAR);
    act(g, "@C$n@W tailwhips $p@W extremely hard!@n", true, ch, obj, 0, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}
