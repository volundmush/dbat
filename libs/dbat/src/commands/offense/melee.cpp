/* melee.c: basic melee attacks
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

ACMD(do_uppercut)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_UPPERCUT);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_UPPERCUT)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char_printf(g, ch, "Your hands are full!\r\n");
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char_printf(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 200)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_UPPERCUT);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  if (char_is_frieza(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(g, ch, 5);
    else
      handle_cooldown(g, ch, 7);
  } else {
    handle_cooldown(g, ch, 7);
  }
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_UPPERCUT, 1);
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

    if (!tech_handle_zanzoken(g, ch, vict, "uppercut")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your uppercut with a punch of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W uppercut with a punch of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W uppercut with a punch of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your uppercut!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W uppercut!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W uppercut!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 5, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your uppercut!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W uppercut!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W uppercut!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your uppercut misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws an uppercut at you but somehow misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws an uppercut at @C$N@W but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your uppercut misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws an uppercut at you but somehow misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws an uppercut at @C$N@W but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 5, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou leap up and launch an uppercut into @C$N's@W body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W leaps up and launches an uppercut into your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W leaps up and launches an uppercut into @C$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou smash an uppercut into @C$N's@W chin!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W smashes an uppercut into your chin!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W smashes an uppercut into @C$N's@W chin!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_KNOCKED) && (rand_number(g, 1, 8) >= 7 && (getCurPL(g, vict) > getCurPL(g, ch) / 5) && !char_aff_flagged(vict, AFF_SANCTUARY))) {
          act(g, "@C$N@W is knocked out!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou are knocked out!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W is knocked out!@n", true, ch, nullptr, vict, TO_NOTVICT);
          setStatusKnockedOut(vict);
        }
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou uppercut @C$N@W, hitting $M directly in chest!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W uppercuts you, hitting you directly in the chest!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W uppercuts @C$N@W, hitting $M directly in the chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYour poorly aimed uppercut hits @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W poorly aims an uppercut and hits you in the arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W poorly aims an uppercut and hits @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou slam an uppercut into @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams an uppercut into your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams an uppercut into @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "hand");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char_printf(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou uppercut $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W uppercuts $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char_printf(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_roundhouse)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_ROUNDHOUSE);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_ROUNDHOUSE)) {
    return;
  }

  if (!limb_ok(g, ch, 1)) {
    return;
  }
  if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50 && char_get_limb_condition(ch, (4) - 1) < 0) {
    send_to_char_printf(g, ch, "Using your broken right leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (3) - 1) < 0) {
      act(g, "@RYour right leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's right leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50 && char_get_limb_condition(ch, (3) - 1) < 0) {
    send_to_char_printf(g, ch, "Using your broken left leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (4) - 1) < 0) {
      act(g, "@RYour left leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's left leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char_printf(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 180)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_ROUNDHOUSE);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  if (char_is_nail(ch)) {
    if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
      handle_cooldown(g, ch, 5);
    else
      handle_cooldown(g, ch, 7);
  } else {
    handle_cooldown(g, ch, 7);
  }
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_ROUNDHOUSE, 1);
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

    if (!tech_handle_zanzoken(g, ch, vict, "roundhouse")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your roundhouse with a punch of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W roundhouse with a punch of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W roundhouse with a punch of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your roundhouse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W roundhouse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W roundhouse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 4, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your roundhouse!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W roundhouse!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W roundhouse!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your roundhouse misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a roundhouse at you but somehow misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your roundhouse misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws a roundhouse at you but somehow misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 4, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou spin in mid air and land a kick into @C$N's@W body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W spins in mid air and lands a kick into your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W spins in mid air and lands a kick into @C$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou spin a fierce roundhouse into @C$N's@W gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W spins a fierce roundhouse into your gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W spins a fierce roundhouse into @C$N's@W gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 8) >= 7) {
          handle_knockdown(g, vict);
        }
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou throw a roundhouse at @C$N@W, hitting $M directly in the neck!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a roundhouse at you, hitting YOU directly in the neck!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a roundhouse at @C$N@W, hitting $M directly in the face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYour poorly aimed roundhouse hits @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W poorly aims a roundhouse and hits you in the arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W poorly aims a roundhouse and hits @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 195, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou slam a roundhouse into @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams a roundhouse into your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams a roundhouse into @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 195, ch, vict, nullptr, dmg, 0);
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
      send_to_char_printf(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou roundhouse $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W roundhouses $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char_printf(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_elbow)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_ELBOW);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_ELBOW)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char_printf(g, ch, "Your hands are full!\r\n");
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char_printf(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 300)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_ELBOW);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 6);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_ELBOW, 0);

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

    if (!tech_handle_zanzoken(g, ch, vict, "elbow")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your elbow with an elbow of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W elbow with one of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W elbow with an elbow of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W blocks your elbow strike!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou block @C$n's@W elbow strike!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W blocks @c$n's@W elbow strike!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 2, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W dodges your elbow strike!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W elbow strike!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W dodges @c$n's@W elbow strike!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it, your elbow strike misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws an elbow strike at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws an elbow strike at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it, your elbow strike misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws an elbow stike at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws an elbow strike at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

      pcost(g, ch, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 2, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou slam your elbow into @C$N's@W body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s elbow into your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s elbow into @C$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou slam your elbow into @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s elbow into your face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s elbow into @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou land your elbow against @C$N's@W gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s elbow against your gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s elbow against @C$N's@W gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou land your elbow against @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s elbow against your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s elbow against @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WYou land your elbow against @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s elbow against your arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s elbow against @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "elbow");

    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou elbow stike $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W elbow strikes $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

    return;
  }
  send_to_char_printf(g, ch, "Error! Please report.\r\n");
  return;
}

ACMD(do_kick)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_KICK);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_KICK)) {
    return;
  }

  if (!limb_ok(g, ch, 1)) {
    return;
  }
  if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50 && char_get_limb_condition(ch, (4) - 1) < 0) {
    send_to_char_printf(g, ch, "Using your broken right leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (3) - 1) < 0) {
      act(g, "@RYour right leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's right leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50 && char_get_limb_condition(ch, (3) - 1) < 0) {
    send_to_char_printf(g, ch, "Using your broken left leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (4) - 1) < 0) {
      act(g, "@RYour left leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's left leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char_printf(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 400)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_KICK);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 5);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_KICK, 0);
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

    if (!tech_handle_zanzoken(g, ch, vict, "kick")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your kick with a kick of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W kick with one of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W kick with a kick of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W blocks your kick!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou block @C$n's@W kick!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W blocks @c$n's@W kick!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 1, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W dodges your kick!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W kick!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W dodges @c$n's@W kick!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it, your kick misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a kick at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a kick at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it, your kick misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws a kick at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws a kick at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

      pcost(g, ch, 0, stcost / 2);
      return;
      return;
    }
    dmg         = damtype(g, ch, 1, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou slam your foot into @C$N's@W body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s foot into your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s foot into @C$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou slam your foot into @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s foot into your face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s foot into @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou land your foot against @C$N's@W gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s foot against your gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s foot against @C$N's@W gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou land your foot against @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s foot against your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s foot against @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WYou land your foot against @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s foot against your arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s foot against @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
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
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou kick $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W kicks $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

    return;
  }
  send_to_char_printf(g, ch, "Error! Please report.\r\n");
  return;
}

ACMD(do_knee)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_KNEE);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_KNEE)) {
    return;
  }

  if (!limb_ok(g, ch, 1)) {
    return;
  }
  if (char_get_limb_condition(ch, (3) - 1) > 0 && char_get_limb_condition(ch, (3) - 1) < 50 && char_get_limb_condition(ch, (4) - 1) < 0) {
    send_to_char_printf(g, ch, "Using your broken right leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (3) - 1) < 0) {
      act(g, "@RYour right leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's right leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_get_limb_condition(ch, (4) - 1) < 50 && char_get_limb_condition(ch, (3) - 1) < 0) {
    send_to_char_printf(g, ch, "Using your broken left leg has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (4) - 1) < 0) {
      act(g, "@RYour left leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's left leg has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char_printf(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 250)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_KNEE);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 6);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_KNEE, 0);
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

    if (!tech_handle_zanzoken(g, ch, vict, "knee strike")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your knee with a knee of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W knee with one of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W knee with a knee of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W blocks your knee strike!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou block @C$n's@W knee strike!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W blocks @c$n's@W knee strike!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 3, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W dodges your knee strike!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W knee strike!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W dodges @c$n's@W knee strike!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it, your knee strike misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a knee strike at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a knee strike at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it, your  knee strike misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws a knee stike at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws a knee strike at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

      pcost(g, ch, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 3, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou slam your knee into @C$N's@W body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s knee into your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s knee into @C$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou slam your knee into @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s knee into your face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s knee into @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou land your knee against @C$N's@W gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s knee against your gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s knee against @C$N's@W gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou land your knee against @C$N's@W leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s knee against your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s knee against @C$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WYou land your knee against @C$N's@W arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W lands $s knee against your arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W lands $s knee against @C$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "knee");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou knee stike $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W knee strikes $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

    return;
  }
  send_to_char_printf(g, ch, "Error! Please report.\r\n");
  return;
}

ACMD(do_punch)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_PUNCH);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_PUNCH)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char_printf(g, ch, "Your hands are full!\r\n");
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char_printf(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 500)) {
    return;
  }
  skill = init_skill(g, ch, SKILL_PUNCH);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 4);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_PUNCH, 0);
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

    if (!tech_handle_zanzoken(g, ch, vict, "punch")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your punch with a punch of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W punch with a punch of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W punch with a punch of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@C$N@W moves quickly and blocks your punch!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W punch!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W punch!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 0, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@C$N@W manages to dodge your punch!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W punch!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W punch!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it but your punch misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a punch at you but somehow misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a punch at @C$N@W but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it but your punch misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws a punch at you but somehow misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws a punch at @C$N@W but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 0, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou slam your fist into @C$N's@W body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s fist into your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s fist into @C$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou slam your fist into @C$N's@W face!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W slams $s fist into your face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W slams $s fist into @C$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou punch @C$N@W directly in the gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W punches you directly in the gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W punches @C$N@W directly in the gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou punch @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W punches you in the arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W punches @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou punch @C$N@W in the leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W punches you in the leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W punches @C$N@W in the arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "hand");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char_printf(g, ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou punch $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W punches $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char_printf(g, ch, "Error! Please report.\r\n");
    return;
  }
}
