/* melee.c: basic melee attacks
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

ACMD(do_uppercut)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_UPPERCUT);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_UPPERCUT)) {
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

  if (!check_points(ch, 0, getEffMaxPL(ch) / 200)) {
    return;
  }

  skill = init_skill(ch, SKILL_UPPERCUT);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (char_is_frieza(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(ch, 5);
    else
      handle_cooldown(ch, 7);
  } else {
    handle_cooldown(ch, 7);
  }
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_UPPERCUT, 1);
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

    if (!tech_handle_zanzoken(ch, vict, "uppercut")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your uppercut with a punch of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W uppercut with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W uppercut with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your uppercut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W uppercut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W uppercut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 5, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your uppercut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W uppercut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W uppercut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your uppercut misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws an uppercut at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws an uppercut at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your uppercut misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws an uppercut at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws an uppercut at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg         = damtype(ch, 5, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou leap up and launch an uppercut into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W leaps up and launches an uppercut into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W leaps up and launches an uppercut into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou smash an uppercut into @C$N's@W chin!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W smashes an uppercut into your chin!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W smashes an uppercut into @C$N's@W chin!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (!char_aff_flagged(vict, AFF_KNOCKED) && (rand_number(1, 8) >= 7 && (getCurPL(vict) > getCurPL(ch) / 5) && !char_aff_flagged(vict, AFF_SANCTUARY))) {
            act("@C$N@W is knocked out!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@WYou are knocked out!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$N@W is knocked out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            setStatusKnockedOut(vict);
          }
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou uppercut @C$N@W, hitting $M directly in chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W uppercuts you, hitting you directly in the chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W uppercuts @C$N@W, hitting $M directly in the chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYour poorly aimed uppercut hits @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W poorly aims an uppercut and hits you in the arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W poorly aims an uppercut and hits @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou slam an uppercut into @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams an uppercut into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams an uppercut into @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "hand");
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
    dmg = ((getCurPL(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@WYou uppercut $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W uppercuts $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_roundhouse)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_ROUNDHOUSE);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_ROUNDHOUSE)) {
    return;
  }

  if (!limb_ok(ch, 1)) {
    return;
  } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50 && char_get_limb_condition(ch, (4) - 1) < 0) {
    send_to_char(ch, "Using your broken right leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (3) - 1) < 0) {
      act("@RYour right leg has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's right leg has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50 && char_get_limb_condition(ch, (3) - 1) < 0) {
    send_to_char(ch, "Using your broken left leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (4) - 1) < 0) {
      act("@RYour left leg has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's left leg has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(ch, 0, getEffMaxPL(ch) / 180)) {
    return;
  }

  skill = init_skill(ch, SKILL_ROUNDHOUSE);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (char_is_nail(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(ch, 5);
    else
      handle_cooldown(ch, 7);
  } else {
    handle_cooldown(ch, 7);
  }
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_ROUNDHOUSE, 1);
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

    if (!tech_handle_zanzoken(ch, vict, "roundhouse")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your roundhouse with a punch of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W roundhouse with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W roundhouse with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your roundhouse!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W roundhouse!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W roundhouse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 4, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your roundhouse!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W roundhouse!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W roundhouse!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your roundhouse misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws a roundhouse at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your roundhouse misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws a roundhouse at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg         = damtype(ch, 4, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou spin in mid air and land a kick into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W spins in mid air and lands a kick into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W spins in mid air and lands a kick into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou spin a fierce roundhouse into @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W spins a fierce roundhouse into your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W spins a fierce roundhouse into @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(1, 8) >= 7) {
            handle_knockdown(vict);
          }
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou throw a roundhouse at @C$N@W, hitting $M directly in the neck!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws a roundhouse at you, hitting YOU directly in the neck!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws a roundhouse at @C$N@W, hitting $M directly in the face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYour poorly aimed roundhouse hits @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W poorly aims a roundhouse and hits you in the arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W poorly aims a roundhouse and hits @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou slam a roundhouse into @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams a roundhouse into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams a roundhouse into @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 0);
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
    dmg = ((getCurPL(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@WYou roundhouse $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W roundhouses $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_elbow)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_ELBOW);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_ELBOW)) {
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

  if (!check_points(ch, 0, getEffMaxPL(ch) / 300)) {
    return;
  }

  skill = init_skill(ch, SKILL_ELBOW);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  handle_cooldown(ch, 6);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_ELBOW, 0);

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

    if (!tech_handle_zanzoken(ch, vict, "elbow")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your elbow with an elbow of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W elbow with one of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W elbow with an elbow of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W blocks your elbow strike!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou block @C$n's@W elbow strike!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W blocks @c$n's@W elbow strike!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 2, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W dodges your elbow strike!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W elbow strike!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W dodges @c$n's@W elbow strike!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it, your elbow strike misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws an elbow strike at you, but thankfully misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws an elbow strike at @C$N@W, but misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it, your elbow strike misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws an elbow stike at you, but thankfully misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws an elbow strike at @C$N@W, but misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, 0, 0);

        pcost(ch, 0, 0);
      }
      return;
    } else {
      dmg         = damtype(ch, 2, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou slam your elbow into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s elbow into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s elbow into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou slam your elbow into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s elbow into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s elbow into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou land your elbow against @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s elbow against your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s elbow against @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou land your elbow against @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s elbow against your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s elbow against @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak */
          act("@WYou land your elbow against @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s elbow against your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s elbow against @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "elbow");

      pcost(ch, 0, stcost);
      handle_multihit(ch, vict);

      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    dmg = ((getCurPL(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@WYou elbow stike $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W elbow strikes $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

    return;
  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_kick)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_KICK);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_KICK)) {
    return;
  }

  if (!limb_ok(ch, 1)) {
    return;
  } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50 && char_get_limb_condition(ch, (4) - 1) < 0) {
    send_to_char(ch, "Using your broken right leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (3) - 1) < 0) {
      act("@RYour right leg has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's right leg has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50 && char_get_limb_condition(ch, (3) - 1) < 0) {
    send_to_char(ch, "Using your broken left leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (4) - 1) < 0) {
      act("@RYour left leg has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's left leg has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(ch, 0, getEffMaxPL(ch) / 400)) {
    return;
  }

  skill = init_skill(ch, SKILL_KICK);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  handle_cooldown(ch, 5);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_KICK, 0);
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

    if (!tech_handle_zanzoken(ch, vict, "kick")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your kick with a kick of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W kick with one of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W kick with a kick of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W blocks your kick!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou block @C$n's@W kick!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W blocks @c$n's@W kick!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 1, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W dodges your kick!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W kick!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W dodges @c$n's@W kick!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it, your kick misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws a kick at you, but thankfully misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws a kick at @C$N@W, but misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it, your kick misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws a kick at you, but thankfully misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws a kick at @C$N@W, but misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, 0, 0);

        pcost(ch, 0, stcost / 2);
        return;
      }
      return;
    } else {
      dmg         = damtype(ch, 1, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou slam your foot into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s foot into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s foot into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou slam your foot into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s foot into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s foot into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou land your foot against @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s foot against your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s foot against @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou land your foot against @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s foot against your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s foot against @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak */
          act("@WYou land your foot against @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s foot against your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s foot against @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
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
    dmg = ((getCurPL(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@WYou kick $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W kicks $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

    return;
  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_knee)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_KNEE);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_KNEE)) {
    return;
  }

  if (!limb_ok(ch, 1)) {
    return;
  } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50 && char_get_limb_condition(ch, (4) - 1) < 0) {
    send_to_char(ch, "Using your broken right leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (3) - 1) < 0) {
      act("@RYour right leg has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's right leg has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50 && char_get_limb_condition(ch, (3) - 1) < 0) {
    send_to_char(ch, "Using your broken left leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(3, 5)));
    if (char_get_limb_condition(ch, (4) - 1) < 0) {
      act("@RYour left leg has fallen apart!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@r$n@R's left leg has fallen apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(ch, 0, getEffMaxPL(ch) / 250)) {
    return;
  }

  skill = init_skill(ch, SKILL_KNEE);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  handle_cooldown(ch, 6);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_KNEE, 0);
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

    if (!tech_handle_zanzoken(ch, vict, "knee strike")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your knee with a knee of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W knee with one of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W knee with a knee of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W blocks your knee strike!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou block @C$n's@W knee strike!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W blocks @c$n's@W knee strike!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 3, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W dodges your knee strike!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W knee strike!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W dodges @c$n's@W knee strike!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it, your knee strike misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws a knee strike at you, but thankfully misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws a knee strike at @C$N@W, but misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it, your  knee strike misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws a knee stike at you, but thankfully misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws a knee strike at @C$N@W, but misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, 0, 0);

        pcost(ch, 0, 0);
      }
      return;
    } else {
      dmg         = damtype(ch, 3, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou slam your knee into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s knee into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s knee into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou slam your knee into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s knee into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s knee into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou land your knee against @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s knee against your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s knee against @C$N's@W gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou land your knee against @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s knee against your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s knee against @C$N's@W leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak */
          act("@WYou land your knee against @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W lands $s knee against your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W lands $s knee against @C$N's@W arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "knee");
      pcost(ch, 0, stcost);
      handle_multihit(ch, vict);

      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    dmg = ((getCurPL(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@WYou knee stike $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W knee strikes $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

    return;
  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_punch)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(ch, SKILL_PUNCH);
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_PUNCH)) {
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

  if (!check_points(ch, 0, getEffMaxPL(ch) / 500)) {
    return;
  }
  skill = init_skill(ch, SKILL_PUNCH);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  handle_cooldown(ch, 4);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_PUNCH, 0);
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

    if (!tech_handle_zanzoken(ch, vict, "punch")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W parries your punch with a punch of $S own!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou parry @C$n's@W punch with a punch of your own!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W punch with a punch of $S own!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your punch!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W punch!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W punch!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 0, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your punch!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W punch!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W punch!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@WYou can't believe it but your punch misses!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W throws a punch at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W throws a punch at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@WYou can't believe it but your punch misses!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W throws a punch at you but somehow misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W throws a punch at @C$N@W but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg         = damtype(ch, 0, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou slam your fist into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s fist into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s fist into @C$N's@W body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou slam your fist into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W slams $s fist into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W slams $s fist into @C$N's@W face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou punch @C$N@W directly in the gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W punches you directly in the gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W punches @C$N@W directly in the gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou punch @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W punches you in the arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W punches @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou punch @C$N@W in the leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W punches you in the leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W punches @C$N@W in the arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, ch, vict, NULL, dmg, 0);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      tech_handle_fireshield(ch, vict, "hand");
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
    dmg = ((getCurPL(ch) / 10000) + (char_get_aff_abils_str(ch)));
    act("@WYou punch $p@W as hard as you can!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W punches $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
