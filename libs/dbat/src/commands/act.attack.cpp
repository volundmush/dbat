/* ************************************************************************
*   File: act.attack.c                                  Part of DBAT      *
*  Usage: player-level commands of an offensive nature                    *
*         created because the size act.offensive.c was getting too large. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  -While this is an original file and is credited to me it is basically  *
*   just act.offensive.c part 2. So all original credit is due to the     *
*   credits found in act.offensive.c except for the commands added in-    *
*                                                   ~~Iovan               *
************************************************************************ */
#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.attack.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/techniques.hpp"
#include "dbat/combat.hpp"
#include "dbat/random.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/fight.hpp"
#include "dbat/consts/deathtype.hpp"
#include "dbat/weather.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.item.hpp"
#include "dbat/game_world.hpp"

ACMD(do_lightgrenade)
{
  int perc, dge = 2, skill;
  int64_t dmg;
  double attperc = .35, minimum = .15;
  struct char_data *targ = nullptr;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_LIGHTGRENADE)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0)) {
    return;
  }

  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  targ = nullptr;
  if (arg.empty() || !(targ = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      targ = char_get_fighting(g, ch);
    } else {
      send_to_char(g, ch, "Nobody around here by that name.\r\n");
      return;
    }
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_LIGHTGRENADE); /* Set skill value */

  perc = chance_to_hit(g, ch);

  if (skill < perc) {
    act(g, "@WYou quickly bring your hands in front of your body and cup them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as your ki is condensed between your hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Your concentration waivers however and the energy slips away from your control harmlessly!@n", true, ch, nullptr, targ, TO_CHAR);
    act(g, "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Suddenly $s concentration seems to waiver and the energy slips away from $s control harmlessly!@n", true, ch, nullptr, targ, TO_VICT);
    act(g, "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Suddenly $s concentration seems to waiver and the energy slips away from $s control harmlessly!@n", true, ch, nullptr, targ, TO_NOTVICT);
    pcost(g, ch, attperc, 0);

    improve_skill(g, ch, SKILL_LIGHTGRENADE, 0);
    return;
  }

  act(g, "@WYou quickly bring your hands in front of your body and cup them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as your ki is condensed between your hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. You shout @r'@YLIGHT GRENADE@r'@W as the orb launches from your hands at @C$N@W!@n", true, ch, nullptr, targ, TO_CHAR);
  act(g, "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. @C$n shouts @r'@YLIGHT GRENADE@r'@W as the orb launches from $s hands at YOU!@n", true, ch, nullptr, targ, TO_VICT);
  act(g, "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. @C$n shouts @r'@YLIGHT GRENADE@r'@W as the orb launches from $s hands at @c$N@W!@n", true, ch, nullptr, targ, TO_NOTVICT);

  dmg = damtype(g, ch, 57, skill, attperc);

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch) {
      continue;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_GROUP)) {
      if (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict)) {
        if (vict == targ) {
          send_to_char(g, ch, "Leave the group if you want to murder them.\r\n");
        }
        continue;
      }
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
      continue;
    }
    dge = handle_dodge(g, vict);
    if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
        (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
      act(g, "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_NOTVICT);
      char_aff_set(vict, AFF_ZANZOKEN, false);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      if (vict == targ) {
        pcost(g, ch, attperc, 0);
        handle_cooldown(g, ch, 9);
        return;
      }
      continue;
    }
    if (dge > axion_dice(g, skill * 0.5) && vict == targ) {
      send_to_char(g, ch, "DGE: %d\n", dge);
      act(g, "@c$N@W manages to dodge the light grenade!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@WYou manages to dodge the light grenade!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$N@W manages to dodge the light grenade!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      improve_skill(g, vict, SKILL_DODGE, 0);
      pcost(g, ch, attperc, 0);
      handle_cooldown(g, ch, 9);
      return;
    }
    if (dge > axion_dice(g, skill * 0.5) && vict != targ) {
      act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      improve_skill(g, vict, SKILL_DODGE, 0);
      continue;
    }
    if (vict == targ) {
      act(g, "@R$N@r is hit by the light grenade which explodes all around $m!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RYou are hit by the light grenade which explodes all around you!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@R$N@r is hit by the light grenade which explodes all around $m!@n", true, ch, nullptr, vict, TO_NOTVICT);
      if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 4) == 4) {
        handle_knockdown(g, vict);
      }
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
      continue;
    }
    act(g, "@R$N@r is caught by the light grenade's explosion!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@RYou are caught by the light grenade's explosion!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@R$N@r is caught by the light grenade's explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
    if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 4) == 4) {
      handle_knockdown(g, vict);
    }
    hurt(g, 0, 0, ch, vict, nullptr, dmg * 0.5, 1);
    continue;
  } /* Hitting targets! */
  pcost(g, ch, attperc, 0);

  improve_skill(g, ch, SKILL_LIGHTGRENADE, 0);
  handle_cooldown(g, ch, 9);
}

ACMD(do_energize)
{

  if (char_is_npc(ch))
    return;

  if (char_get_preference(ch) != PREFERENCE_THROWING) {
    send_to_char(g, ch, "You aren't dedicated to throwing!\r\n");
    return;
  }

  if (!char_get_skill(ch, SKILL_ENERGIZE)) {
    if (char_get_skill(ch, SKILL_FOCUS) >= 30) {
      int result = rand_number(g, 10, 14);
      char_set_skills_base(ch, SKILL_ENERGIZE, result);
      send_to_char(g, ch, "You learn the basics for energizing thrown weapons! Now use the energize command again.\r\n");
      return;
    }
    send_to_char(g, ch, "You need a Focus skill level of 30 to figure out the basics of this technique.\r\n");
    return;
  }
  if (char_prf_flagged(g, ch, PRF_ENERGIZE)) {
    send_to_char(g, ch, "You stop focusing ki into your fingertips.\r\n");
    char_prf_set(g, ch, PRF_ENERGIZE, false);
    return;
  }
  send_to_char(g, ch, "You start focusing your latent ki into your fingertips.\r\n");
  char_prf_set(g, ch, PRF_ENERGIZE, true);
  return;
}

ACMD(do_breath)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = getEffMaxPL(g, ch) / 5000;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!char_is_npc(ch)) {
    return;
  }
  if (!can_grav(g, ch)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 200)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_KNEE);

  vict = nullptr;
  obj  = nullptr;
  if (arg.empty() || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(g, ch);
    } else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
      return;
    }
  }
  handle_cooldown(g, ch, 10);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
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
    prob += 15;

    if (!tech_handle_zanzoken(g, ch, vict, "breath")) {
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > axion_dice(g, 10)) {
          act(g, "@WYou move quickly and block @C$n's@W fiery breath!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W fiery breath!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 6, skill, attperc);
          dmg = dmg * 0.8;
          if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(g, 1, 4) == 3 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
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
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@WYou dodge the fiery jets of flames coming from @C$n's@W mouth!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge the fiery jets of flames coming from @c$n's@W mouth!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@C$n@W moves to breath flames on you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W moves to breath flames on @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@C$n@W moves to breath flames on you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W moves to breath flames on @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg = damtype(g, ch, 8, skill, attperc);
    dmg += dmg * 2;
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    pcost(g, ch, 0, stcost);
    if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(g, 1, 4) == 3 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
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
    act(g, "@C$n@W breathes flames on $p@W!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_ram)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = getEffMaxPL(g, ch) / 200;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!char_is_npc(ch)) {
    return;
  }
  if (!can_grav(g, ch)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 200)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_KNEE);

  vict = nullptr;
  obj  = nullptr;
  if (arg.empty() || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(g, ch);
    } else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
      return;
    }
  }
  handle_cooldown(g, ch, 4);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
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

    prob -= 5;

    if (!tech_handle_zanzoken(g, ch, vict, "ram")) {
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > axion_dice(g, 10)) {
          act(g, "@WYou move quickly and block @C$n's@W body as $e tries to ram YOU!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W body as $e tries to ram $M!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 6, skill, attperc);
          dmg -= dmg * 0.2;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@WYou dodge @C$n's@W attempted ram!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W attempted ram!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@C$n@W moves to ram you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W moves to ram @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@C$n@W moves to ram you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W moves to ram @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg = damtype(g, ch, 8, skill, attperc);
    dmg += dmg * 1.1;
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@C$n@W aims $s body at YOU and rams into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W aims $s body at @C$N@W and rams into $S body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@C$n@W aims $s body at YOU and rams into YOUR face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W aims $s body at @C$N@W and rams into $S face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@C$n@W aims $s body at YOU and rams into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W aims $s body at @C$N@W and rams into $S body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@C$n@W aims $s body at YOU and rams into YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W aims $s body at @C$N@W and rams into $S arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 190, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@C$n@W aims $s body at YOU and rams into YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W aims $s body at @C$N@W and rams into $S leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 190, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    pcost(g, ch, 0, stcost);
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
    act(g, "@C$n@W rams $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_strike)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = getEffMaxPL(g, ch) / 400;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!char_is_npc(ch)) {
    return;
  }
  if (!can_grav(g, ch)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 400)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_KNEE);

  vict = nullptr;
  obj  = nullptr;
  if (arg.empty() || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(g, ch);
    } else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
      return;
    }
  }
  handle_cooldown(g, ch, 4);
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
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

    prob += 5;

    if (!tech_handle_zanzoken(g, ch, vict, "fang strike")) {
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@WYou parry @C$n's@W fang strike with a punch of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W fang strike with a punch of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > axion_dice(g, 10)) {
          act(g, "@WYou move quickly and block @C$n's@W fang strike!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W fang strike!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 6, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > axion_dice(g, 10)) {
          act(g, "@WYou dodge @C$n's@W fang strike!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W fang strike!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@C$n@W moves to fang strike you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W moves to fang strike @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@C$n@W moves to fang strike you, but misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W moves to fang strike @C$N@W, but somehow misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    dmg = damtype(g, ch, 8, skill, attperc);
    dmg += dmg * 0.5;
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act(g, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    pcost(g, ch, 0, stcost);
    decCurST(g, vict, dmg * .25, 0);
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
    act(g, "@C$n@W fang strikes $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

/* End NPC skills */

ACMD(do_combine)
{

  std::string arg, arg2;
  struct char_data *vict;
  int fire = false, temp = -1;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  if (!has_group(g, ch)) {
    send_to_char(g, ch, "You need to be in a group!\r\n");
    return;
  }
  if (arg.empty() || (!char_get_master(g, ch) && arg2.empty())) {
    send_to_char(g, ch, "Follower Syntax: combine (attack)\r\n");
    send_to_char(g, ch, "Leader Syntax: combine (attack) (target)\r\n");
    send_to_char(g, ch, "Cancel Syntax: combine stop\r\n");
  } else {
    if (str_eq_nocase(arg, "stop") && char_get_master(g, ch)) {
      if (char_get_combine(ch) == -1) {
        send_to_char(g, ch, "You are not trying to combine any attacks...\r\n");
        return;
      }
      send_to_char(g, ch, "You stop your preparations to combine your attack with a group attack.\r\n");
      send_to_char(g, char_get_master(g, ch), "@Y%s@C is no longer prepared to combine an attack with the group!@n\r\n", get_i_name(char_get_master(g, ch), ch));
      for (char_data *fol : char_followers(g, char_get_master(g, ch))) {
        if (ch != fol)
          send_to_char(g, fol, "@Y%s@C is no longer prepared to combine an attack with the group!@n\r\n", get_i_name(fol, ch));
      }
      char_set_combine(ch, -1);
      return;
    }
    if (str_eq_nocase(arg, "stop") && !char_get_master(g, ch)) {
      send_to_char(g, ch, "You do not need to stop as you haven't prepared anything.\r\n");
      return;
    }
    int i = 0;
    for (i = 0; i < 14; i++) {
      if (arg.find(attack_names[i]) != std::string::npos) {
        if (i == 5) {
          if (!char_get_equipment(ch, WEAR_WIELD1)) {
            send_to_char(g, ch, "You need to wield a sword to use this technique.\r\n");
            return;
          }
          if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
            send_to_char(g, ch, "You are not wielding a sword, you need one to use this technique.\r\n");
            return;
          }
          temp = i;
          i    = 15;
        }
        temp = i;
        i    = 15;
      }
    }
    if (temp == -1) {
      send_to_char(g, ch, "Follower Syntax: combine (attack)\r\n");
      send_to_char(g, ch, "Leader Syntax: combine (attack) (target)\r\n");
      send_to_char(g, ch, "Follower Cancel Syntax: combine stop\r\n");
      return;
    }
    if (!char_get_skill(ch, attack_skills[temp])) {
      send_to_char(g, ch, "You do not know that skill.\r\n");
      return;
    }
    if (attack_skills[temp] == 440 && !char_is_nail(ch)) {
      send_to_char(g, ch, "Only students of Nail know how to combine that attack effectively.\r\n");
      return;
    }
    if (char_get_charge(ch) < getMaxKI(g, ch) * 0.05) {
      send_to_char(g, ch, "You need to have the minimum of 5%s ki charged to combine.\r\n", "%");
    }
    if (!char_get_master(g, ch)) {
      if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
        send_to_char(g, ch, "Who will your combined attack be targeting?\r\n");
        return;
      }
      if (vict == ch) {
        send_to_char(g, ch, "No targeting yourself...\r\n");
        return;
      }
      char_set_combine(ch, temp);
      for (char_data *foll : char_followers(g, ch)) {
        if (!char_aff_flagged(foll, AFF_GROUP)) {
          continue;
        }
        if (char_get_combine(foll) != -1 && char_get_charge(foll) >= getMaxKI(g, foll) * 0.05) {
          fire = true;
        }
      } /* End follow for statement */
      if (fire == true) {
        combine_attacks(g, ch, vict);
        return;
      }
      send_to_char(g, ch, "You do not have any followers who have readied an attack to combine or they do not have enough ki anymore to combine said attack.\r\n");
      return;
    }
    if (char_get_master(g, ch)) {
      if (char_get_charge(ch) >= getMaxKI(g, ch) * 0.05) {
        act(g, "@C$n@c appears to be concentrating hard and focusing $s energy!@n\r\n", true, ch, nullptr, nullptr, TO_ROOM);
        send_to_char(g, char_get_master(g, ch), "@BCOMBINE@c: @Y%s@C has prepared to combine a @c'@G%s@c'@C with the next group attack!@n\r\n", get_i_name(char_get_master(g, ch), ch), attack_names[temp]);
        for (char_data *f_ch : char_followers(g, char_get_master(g, ch))) {
          if (ch != f_ch)
            send_to_char(g, f_ch, "@BCOMBINE@c: @Y%s@C has prepared to combine a @c'@G%s@c'@C with the next group attack!@n\r\n", get_i_name(f_ch, ch), attack_names[temp]);
        }
        char_set_combine(ch, temp);
      } else {
        send_to_char(g, ch, "You do not have the minimum 5%s ki charged.\r\n", "%");
        return;
      }
    }
  }
} /* End */

ACMD(do_sunder)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .25, minimum = .05;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_SUNDER)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_SUNDER); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 6);

  /* There is a player/mob targeted */
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_SUNDER, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = axion_dice(g, 0);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Sundering Force")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Sundering Force, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Sundering Force, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Sundering Force, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Sundering Force misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Sundering Force at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Sundering Force at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Sundering Force misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Sundering Force at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Sundering Force at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    int chance  = rand_number(g, 2, 12);
    dmg         = damtype(g, ch, 55, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W body with the force of your energy!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR body with the force of $s energy!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W body with the force of $s energy!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        if (chance >= 10) {
          hurt(g, 0, 160, ch, vict, nullptr, dmg, 1);
        } else if (chance >= 8) {
          hurt(g, 1, 160, ch, vict, nullptr, dmg, 1);
        } else {
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        }
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W head with the force of your energy!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR head with the force of $s energy!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W head with the force of $s energy!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W body with the force of your energy!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR body with the force of $s energy!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W body with the force of $s energy!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        if (chance >= 10) {
          hurt(g, 0, 160, ch, vict, nullptr, dmg, 1);
        } else if (chance >= 8) {
          hurt(g, 1, 160, ch, vict, nullptr, dmg, 1);
        } else {
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        }
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W arm with the force of your energy!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR arm with the force of $s energy!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W arm with the force of $s energy!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 160, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W leg with the force of your energy!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR leg with the force of $s energy!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W leg with the force of $s energy!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 160, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
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
    dmg = damtype(g, ch, 10, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a Sundering Force at $p@W!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a Sundering Force at $p@W!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);
  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_zen)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .2, minimum = .1;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_ZEN)) {
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

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  if (char_get_skills_perf(ch, SKILL_ZEN) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_ZEN) == 3) {
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

  skill = init_skill(g, ch, SKILL_ZEN); /* Set skill value */

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
    improve_skill(g, ch, SKILL_ZEN, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    if (char_get_skills_perf(ch, SKILL_ZEN) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Zen Blade Strike")) {
      if (char_get_skills_perf(ch, SKILL_ZEN) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W moves quickly and blocks your Zen Blade Strike!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W Zen Blade Strike!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W Zen Blade Strike!@n", false, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_ZEN) == 3 && attperc > minimum) {
            pcost(g, ch, attperc - 0.05, 0);
          } else {
            pcost(g, ch, attperc, 0);
          }
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 54, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);

          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Zen Blade Strike, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Zen Blade Strike, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Zen Blade Strike, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(g, ch, vict, 0, 19, skill, SKILL_ZEN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
          }
          improve_skill(g, vict, SKILL_DODGE, 0);

          if (char_get_skills_perf(ch, SKILL_ZEN) == 3 && attperc > minimum) {
            pcost(g, ch, attperc - 0.05, 0);
          } else {
            pcost(g, ch, attperc, 0);
          }
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your Zen Blade Strike misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Zen Blade Strike at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Zen Blade Strike at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);

        if (char_get_skills_perf(ch, SKILL_ZEN) == 3 && attperc > minimum) {
          pcost(g, ch, attperc - 0.05, 0);
        } else {
          pcost(g, ch, attperc, 0);
        }
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your Zen Blade Strike misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Zen Blade Strike at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Zen Blade Strike at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      if (char_get_skills_perf(ch, SKILL_ZEN) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg         = damtype(g, ch, 54, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@CRaising your blade above your head, and closing your eyes, you focus ki into its edge. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as you open your eyes and instantly move past @g$N's@C body while slashing with the pure energy of your resolve! A large explosion of energy erupts across $S body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past YOUR body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past @g$N's@C body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across @g$N's@C body!@n", true, ch, nullptr, vict, TO_NOTVICT);
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
        act(g, "@CRaising your blade above your head, and closing your eyes, you focus ki into its edge. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as you open your eyes and instantly move past @g$N's@C body while slashing with the pure energy of your resolve! A large explosion of energy erupts across $S head!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past YOUR body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across YOUR head!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past @g$N's@C body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across @g$N's@C head!@n", true, ch, nullptr, vict, TO_NOTVICT);
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
            do_get(g, ch, "all.zenni corpse", 0, 0);
          }
          if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
            do_get(g, ch, "all corpse", 0, 0);
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
              do_get(g, ch, "all.zenni corpse", 0, 0);
            }
            if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
              do_get(g, ch, "all corpse", 0, 0);
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
        act(g, "@CRaising your blade above your head, and closing your eyes, you focus ki into its edge. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as you open your eyes and instantly move past @g$N's@C body while slashing with the pure energy of your resolve! A large explosion of energy erupts across $S body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past YOUR body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past @g$N's@C body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across @g$N's@C body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@CRaising your blade above your head, and closing your eyes, you focus ki into its edge. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as you open your eyes and instantly move past @g$N's@C body while slashing with the pure energy of your resolve! A large explosion of energy erupts across $S arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past YOUR body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past @g$N's@C body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across @g$N's@C arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        if (rand_number(g, 1, 100) >= 80 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
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
        act(g, "@CRaising your blade above your head, and closing your eyes, you focus ki into its edge. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as you open your eyes and instantly move past @g$N's@C body while slashing with the pure energy of your resolve! A large explosion of energy erupts across $S leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past YOUR body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@G$n @Craises $s blade above $s head, and closes $s eyes. The edge of the blade begins to glow a soft blue as the blade begins to throb with excess energy. Peels of lighting begin to arc from the blade in all directions as $e opens $s eyes and instantly moves past @g$N's@C body while slashing with the pure energy of $s resolve! A large explosion of energy erupts across @g$N's@C leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        if (rand_number(g, 1, 100) >= 80 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
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
    if (char_get_skills_perf(ch, SKILL_ZEN) == 3 && attperc > minimum) {
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
    act(g, "@WYou fire a Zen Blade Strike at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a Zen Blade Strike at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_malice)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .3, minimum = .15;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_MALICE)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_MALICE); /* Set skill value */

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
    improve_skill(g, ch, SKILL_MALICE, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (g->time_info.hours <= 15 || g->time_info.hours > 22) {
      prob += 5;
    }

    if (!tech_handle_zanzoken(g, ch, vict, "Malice Breaker")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W moves quickly and blocks your Malice Breaker!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W Malice Breaker!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W Malice Breaker!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 36, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);

          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Malice Breaker, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Malice Breaker, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Malice Breaker, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(g, ch, vict, 0, 36, skill, SKILL_MALICE); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 80) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (20));
          }
          improve_skill(g, vict, SKILL_DODGE, 0);

          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

          return;
        }
        act(g, "@WYou can't believe it but your Malice Breaker misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Malice Breaker at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Malice Breaker at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);

        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);

        return;
      }
      act(g, "@WYou can't believe it but your Malice Breaker misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Malice Breaker at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Malice Breaker at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 36, skill, attperc);
    if (g->time_info.hours <= 15) {
      dmg *= 1.25;
    } else if (g->time_info.hours <= 22) {
      dmg *= 1.4;
    }
    switch (rand_number(g, 1, 6)) {
      case 1:
        act(g, "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S chest, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's chest!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your chest, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your chest!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W chest, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
      case 3:
        act(g, "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S head, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's head!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your head, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your head!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W head, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's head!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4:
        act(g, "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S gut, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's gut!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your gut, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your gut!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W gut, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's gut!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S arm, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your arm, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W arm, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 170, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 6: /* Weak 2 */
        act(g, "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S leg, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your leg, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W leg, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 1, 170, ch, vict, nullptr, dmg, 1);
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
    dmg = damtype(g, ch, 36, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a Malice Breaker at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a Malice Breaker at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);

  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_nova)
{
  int perc, dge = 2, count = 0, skill;
  int64_t dmg;
  double attperc = .20, minimum = .1;
  std::string arg2;

  std::string_view rest = argument;
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_STARNOVA)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (char_get_skills_perf(ch, SKILL_STARNOVA) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_STARNOVA) == 3) {
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

  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_STARNOVA); /* Set skill value */
  if (char_get_skills_perf(ch, SKILL_STARNOVA) == 2) {
    skill += 5;
  }

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_GROUP) && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
      continue;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    }
    count += 1;
  }
  if (count <= 0) {
    send_to_char(g, ch, "There is no one worth targeting around.\r\n");
    return;
  } /* No one worth targeting */
  else {
    perc = chance_to_hit(g, ch);
    if (g->time_info.hours <= 15 || g->time_info.hours > 22) {
      skill += 5;
    }
    handle_cooldown(g, ch, 6);
    if (skill < perc) {
      act(g, "@WYou gather your charged energy and clench your upheld fists at either side of your body while crouching down. A hot glow of energy begins to form around your body before you lose your concentration and fail to create a @yS@Yt@Wa@wr @cN@Co@Wv@wa@W!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W gathers $s charged energy and clenches $s upheld fists at either side of $s body while crouching down. A hot glow of energy begins to form around $s body before $e seems to lose $s concentration and fail to create a @yS@Yt@Wa@wr @cN@Co@Wv@wa@W!@n", true, ch, nullptr, nullptr, TO_ROOM);
      if (char_get_skills_perf(ch, SKILL_STARNOVA) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }

      improve_skill(g, ch, SKILL_STARNOVA, 0);
      return;
    }

    act(g, "@WYou gather your charged energy and clench your upheld fists at either side of your body while crouching down. A hot glow of energy begins to form around your body in the shape of a sphere! Suddenly a shockwave of heat and energy erupts out into the surrounding area as your glorious @yS@Yt@Wa@wr @cN@Co@Wv@wa@W is born!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W gathers $s charged energy and clenches $s upheld fists at either side of $s body while crouching down. A hot glow of energy begins to form around $s body in the shape of a sphere! Suddenly a shockwave of heat and energy erupts out into the surrounding area as @C$n's@W glorious @yS@Yt@Wa@wr @cN@Co@Wv@wa@W is born!@n", true, ch, nullptr, nullptr, TO_ROOM);

    dmg = damtype(g, ch, 53, skill, attperc);

    if (g->time_info.hours <= 15) {
      dmg *= 1.25;
    } else if (g->time_info.hours <= 22) {
      dmg *= 1.4;
    }

    for (char_data *vict : room_people(g, char_get_in_room(ch))) {
      if (vict == ch) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_GROUP) && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict)) {
        continue;
      }
      if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
        continue;
      }
      if (char_mob_flagged(vict, MOB_NOKILL)) {
        continue;
      }
      dge = handle_dodge(g, vict);
      if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        act(g, "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_VICT);
        act(g, "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        continue;
      }
      if (dge + rand_number(g, -15, 5) > skill) {
        act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        improve_skill(g, vict, SKILL_DODGE, 0);
        continue;
      }
      act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
      if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING) {
        handle_knockdown(g, vict);
      }
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
      continue;
    } /* Hitting targets! */

    if (char_get_skills_perf(ch, SKILL_STARNOVA) == 3 && attperc > minimum) {
      pcost(g, ch, attperc - 0.05, 0);
    } else {
      pcost(g, ch, attperc, 0);
    }

    improve_skill(g, ch, SKILL_STARNOVA, 0);
    return;
  } /* We have targets! Attempt to kills them! */
}

ACMD(do_head)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_HEADBUTT);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;
  int mult;
  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_HEADBUTT)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 100)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_HEADBUTT);

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  if (!char_is_kurzak(ch) || char_get_skill(ch, SKILL_HEADBUTT) < 100) {
    handle_cooldown(g, ch, 6);
  } else if (char_get_skill(ch, SKILL_HEADBUTT) >= 100) {
    handle_cooldown(g, ch, 5);
  }
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_HEADBUTT, 0);
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

    if (!tech_handle_zanzoken(g, ch, vict, "headbutt")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your headbutt with an attack of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W headbutt with an attack of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W headbutt with an attack of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W blocks your headbutt!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou block @C$n's@W headbutt!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W blocks @c$n's@W headbutt!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 3, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W dodges your headbutt!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W headbutt!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W dodges @c$n's@W headbutt!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it, your headbutt misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a headbutt at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a headbutt at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it, your headbutt misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws a headbutt at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws a headbutt at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

      pcost(g, ch, 0, 0);
      return;
    }
    dmg = damtype(g, ch, 52, skill, attperc);

    if (char_is_kurzak(ch)) {
      if (char_get_skill(ch, SKILL_HEADBUTT) >= 60) {
        dmg += dmg * 0.1;
      } else if (char_get_skill(ch, SKILL_HEADBUTT) >= 40) {
        dmg += dmg * 0.05;
      }
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou grab @c$N@W by the shoulders and slam your head into $S chest!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W grabs YOU by the shoulders and slams $s head into YOUR chest!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WYou grab @c$N@W by the shoulders and slam your head into $S face!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W grabs YOU by the shoulders and slams $s head into YOUR face!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W face!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_KNOCKED) && (rand_number(g, 1, 7) >= 4 && (getCurPL(g, vict) > getCurPL(g, ch) / 5) && !char_aff_flagged(vict, AFF_SANCTUARY))) {
          act(g, "@C$N@W is knocked out!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou are knocked out!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W is knocked out!@n", true, ch, nullptr, vict, TO_NOTVICT);
          setStatusKnockedOut(vict);
        }
        mult = calc_critical(g, ch, 0);
        if (char_is_kurzak(ch) && !char_is_npc(ch)) {
          if (char_get_skills_base(ch, SKILL_STYLE) >= 75)
            mult += 1;
        }
        dmg *= mult;
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WYou grab @c$N@W by the shoulders and slam your head into $S chest!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W grabs YOU by the shoulders and slams $s head into YOUR chest!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WYou grab @c$N@W and barely manage to slam your head into $S leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W grabs YOU and barely manages to slam $s head into YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W and barely manages to slam $s head into @c$N's@W leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WYou grab @c$N@W and barely manage to slam your head into $S arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W grabs YOU and barely manages to slam $s head into YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W and barely manages to slam $s head into @c$N's@W arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "head");
    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou headbutt $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W headbutt $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

    return;
  }
  send_to_char(g, ch, "Error! Please report.\r\n");
  return;
}

ACMD(do_bash)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = physical_cost(g, ch, SKILL_BASH);
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg;
  double attperc = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_BASH)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 70)) {
    return;
  }

  skill = init_skill(g, ch, SKILL_BASH);

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
    improve_skill(g, ch, SKILL_BASH, 0);
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

    if (!tech_handle_zanzoken(g, ch, vict, "bash")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act(g, "@C$N@W parries your bash with an attack of $S own!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou parry @C$n's@W bash with an attack of your own!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W parries @c$n's@W bash with an attack of $S own!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_PARRY, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, vict, -2, skill, attperc);
          dmg *= calc_critical(g, ch, 1);
          hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);

          return;
        }
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W blocks your bash!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou block @C$n's@W bash!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W blocks @c$n's@W bash!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_BLOCK, 0);
          pcost(g, ch, 0, stcost / 2);
          pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
          dmg = damtype(g, ch, 3, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);

          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W dodges your bash!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W bash!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W dodges @c$n's@W bash!@n", true, ch, nullptr, vict, TO_NOTVICT);
          improve_skill(g, vict, SKILL_DODGE, 0);
          pcost(g, ch, 0, stcost / 2);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

          return;
        }
        act(g, "@WYou can't believe it, your bash misses!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W throws a bash at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W throws a bash at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

        return;
      }
      act(g, "@WYou can't believe it, your bash misses!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W throws a bash at you, but thankfully misses!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W throws a bash at @C$N@W, but misses!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);

      pcost(g, ch, 0, 0);
      return;
    }
    dmg         = damtype(g, ch, 51, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S body with a crashing impact!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR body with a crashing impact!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S body with a crashing impact!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act(g, "@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S head with a crashing impact!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR head with a crashing impact!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S head with a crashing impact!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act(g, "@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S gut with a crashing impact!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR gut with a crashing impact!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S gut with a crashing impact!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act(g, "@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S leg with a crashing impact!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR leg with a crashing impact!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S leg with a crashing impact!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak */
        act(g, "@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S arm with a crashing impact!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR arm with a crashing impact!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S arm with a crashing impact!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
    }
    tech_handle_fireshield(g, ch, vict, "body");

    if (vict && rand_number(g, 1, 5) >= 4)
      tech_handle_crashdown(g, vict, ch);

    if (rand_number(g, 1, 5) >= 5)
      tech_handle_crashdown(g, ch, vict);

    pcost(g, ch, 0, stcost);
    handle_multihit(g, ch, vict);

    return;
  }
  if (obj) {
    if (!can_kill(g, ch, nullptr, obj, 0)) {
      return;
    }
    dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou bash $p@W as hard as you can!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W bash $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, 0, stcost);

    return;
  }
  send_to_char(g, ch, "Error! Please report.\r\n");
  return;
}

ACMD(do_seishou)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .15, minimum = .05;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (playing_song(g, ch)) {
    return;
  }

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_SEISHOU)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_SEISHOU); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 4);

  /* There is a player/mob targeted */
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_SEISHOU, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Seishou Enko")) {
      pcost(g, ch, attperc / 4, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Seishou Enko, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Seishou Enko, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Seishou Enko, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc / 4, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Seishou Enko misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Seishou Enko at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Seishou Enko at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, attperc / 4, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Seishou Enko misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Seishou Enko at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Seishou Enko at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc / 4, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 50, skill, attperc);
    if (char_get_moltlevel(ch) >= 150) {
      dmg *= 2;
    }
    switch (rand_number(g, 1, 7)) {
      case 1:
        act(g, "@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S body with searing heat!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR body with searing heat!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S body with searing heat!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2:
      case 3:
      case 4: /* Critical */
        act(g, "@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S head with searing heat!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR head with searing heat!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S head with searing heat!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5:
        act(g, "@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S gut with searing heat!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR gut with searing heat!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S gut with searing heat!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 6: /* Weak */
        act(g, "@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S arm with searing heat!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR arm with searing heat!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S arm with searing heat!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 7: /* Weak 2 */
        act(g, "@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S leg with searing heat!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR leg with searing heat!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S leg with searing heat!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
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
    dmg = damtype(g, ch, 10, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire a Seishou Enko at $p@W!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires a Seishou Enko at $p@W!@n", true, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);
  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_throw)
{
  struct char_data *vict = nullptr, *tch = nullptr;
  struct obj_data *obj = nullptr;
  std::string arg, arg2, arg3;
  std::string chunk;
  int odam = 0, miss = true, perc = 0, prob = 0, perc2 = 0, grab = false;
  int64_t damage;

  char_set_throws(ch, rand_number(g, 1, 3)); /* (set by the interpreter before, for any "throw") */
  std::string_view rest = argument;
  arg                   = take_word(rest);
  chunk                 = skip_spaces(rest);

  if (playing_song(g, ch)) {
    return;
  }

  if (arg.empty()) {
    send_to_char(g, ch, "Throw what?\r\n");
    return;
  }

  if (is_sparring(ch)) {
    send_to_char(g, ch, "You can not spar with throw.\r\n");
    return;
  }

  if (!chunk.empty()) {
    std::string_view chunk_rest = chunk;
    arg2                        = take_arg(chunk_rest);
    arg3                        = take_arg(chunk_rest);
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    if (!(tch = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "You do not have that object or character to throw!\r\n");
      return;
    }
  }

  if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(g, ch);
    } else {
      send_to_char(g, ch, "Who do you want to target?\r\n");
      return;
    }
  }

  if (getCurPL(g, vict) <= 1) {
    return;
  }

  if (!can_kill(g, ch, vict, nullptr, 1)) {
    return;
  }

  if (handle_defender(g, vict, ch)) {
    struct char_data *def = char_get_defender(g, vict);
    vict                  = def;
  }

  /* We are throwing an object. */
  if (obj) {
    if (char_get_throws(ch) == -1) {
      char_set_throws(ch, 0);
      return;
    }

    if ((getCurST(g, ch)) < ((getEffMaxPL(g, ch) / 200) + obj_get_weight(obj))) {
      send_to_char(g, ch, "You do not have enough stamina to do it...\r\n");
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(g, ch, "That is broken and useless to throw!\r\n");
      return;
    }
    if (obj_get_weight(obj) + room_get_gravity(g, char_get_in_room(ch)) > getMaxCarryWeight(g, ch)) {
      send_to_char(g, ch, "The gravity has made that too heavy for you to throw!\r\n");
      return;
    }
    int penalty = 0, chance = axion_dice(g, 0) + axion_dice(g, 0), wtype = 0, wlvl = 1, multithrow = true;
    handle_cooldown(g, ch, 5);
    improve_skill(g, ch, SKILL_THROW, 0);
    damage = ((obj_get_weight(obj) / 3) * (char_get_aff_abils_str(ch)) * (char_get_aff_abils_cha(ch) / 3)) + (getEffMaxPL(g, ch) * 0.01);
    damage += (damage * 0.01) * (room_get_gravity(g, char_get_in_room(ch)) / 4);

    if (char_get_preference(ch) == PREFERENCE_THROWING) {
      chance -= chance * 0.25;
    }

    if (obj_extra_flagged(obj, ITEM_WEAPLVL1)) {
      damage += damage * 0.1;
      wlvl = 1;
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL2)) {
      damage += damage * 0.2;
      wlvl = 2;
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL3)) {
      damage += damage * 0.3;
      wlvl = 3;
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL4)) {
      damage += damage * 0.4;
      wlvl = 4;
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL5)) {
      damage += damage * 0.5;
      wlvl = 5;
    }
    if (obj_get_type_flag(obj) == ITEM_WEAPON) {
      if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
        wtype = 1;
      } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
        wtype = 2;
      } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
        wtype = 3;
      } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
        wtype = 4;
      } else if (obj_get_value(obj, VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        wtype  = 5;
        damage = ((obj_get_weight(obj) * char_get_aff_abils_str(ch)) * (char_get_aff_abils_cha(ch) / 3)) + (getEffMaxPL(g, ch) * 0.01);
        damage += room_get_gravity(g, char_get_in_room(ch)) * (room_get_gravity(g, char_get_in_room(ch)) / 2);
      } else {
        wtype = 6;
      }
    }
    if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL) {
      odam = rand_number(g, 5, 30);
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_IRON) {
      odam = rand_number(g, 18, 50);
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL) {
      odam = rand_number(g, 5, 15);
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_KACHIN) {
      odam = rand_number(g, 5, 15);
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
      odam = rand_number(g, 20, 50);
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_DIAMOND) {
      odam = rand_number(g, 5, 20);
    } else if (obj_get_value(obj, VAL_ALL_MATERIAL) == MATERIAL_ENERGY) {
      if (rand_number(g, 1, 2) == 2) {
        odam = 0;
      } else {
        odam = rand_number(g, 1, 3);
      }
    } else {
      odam = rand_number(g, 90, 100);
    }
    if (!obj_extra_flagged(obj, ITEM_THROW)) {
      penalty    = 15;
      multithrow = false;
      damage     = damage * 0.45;
    } else {
      odam = rand_number(g, 0, 1);
      damage += (char_get_aff_abils_str(ch)) * ((getCurPL(g, ch) * 0.00012) + rand_number(g, 1, 20));
      damage += wlvl * (damage * 0.1);
    }

    if (wlvl == 5) {
      damage += 25000;
    } else if (wlvl == 4) {
      damage += 16000;
    } else if (wlvl == 3) {
      damage += 10000;
    } else if (wlvl == 2) {
      damage += 5000;
    } else if (wlvl == 1) {
      damage += 1000;
    }

    int hot = false;
    if (obj_extra_flagged(obj, ITEM_HOT)) {
      hot = true;
    }

    if (wtype > 0 && wtype != 5 && odam > 1) {
      odam = 1;
    }
    perc  = init_skill(g, ch, SKILL_THROW);
    perc2 = init_skill(g, vict, SKILL_DODGE);
    prob  = axion_dice(g, penalty);
    if (!arg3.empty()) {
      if (str_eq_nocase(arg3, "1") || str_eq_nocase(arg3, "single")) {
        multithrow = false;
      } else {
        send_to_char(g, ch, "Syntax: throw (obj | character) (target) <-- This will multithrow if able\nSyntax: throw (obj) (target) (1 | single) <-- This will not multi throw)\r\n");
        return;
      }
    }

    if (!tech_handle_zanzoken(g, ch, vict, "$p")) {
      char_set_combo(ch, -1);
      char_set_combhits(ch, 0);
      int stcost = ((getEffMaxPL(g, ch) / 200) + obj_get_weight(obj));
      char_aff_set(vict, AFF_ZANZOKEN, false);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      obj_from_char(g, obj);
      obj_to_room(g, obj, char_get_in_room(vict));
      return;
    }

    if (perc - (perc2 / 10) < prob) {
      if (obj_extra_flagged(obj, ITEM_ICE) && char_is_demon(vict)) {
        act(g, "You throw $p at $N@n, but it melts before touching $M!", true, ch, obj, vict, TO_CHAR);
        act(g, "$n@n throws $p at $N@n, but it melts before touching $M!", true, ch, obj, vict, TO_NOTVICT);
        act(g, "$n@n throws $p at you, but it melts before touching you!", true, ch, obj, vict, TO_VICT);
        decCurST(g, ch, ((getEffMaxPL(g, ch) / 100) + obj_get_weight(obj)), 0);
        extract_obj(g, obj);
        return;
      }
      if (perc2 > 0) {
        act(g, "You throw $p at $N@n, but $E manages to dodge it easily!", true, ch, obj, vict, TO_CHAR);
        act(g, "$n@n throws $p at $N@n, but $E manages to dodge it easily!", true, ch, obj, vict, TO_NOTVICT);
        act(g, "$n@n throws $p at you, but you easily dodge it.", true, ch, obj, vict, TO_VICT);
      } else if (perc2 <= 0) {
        act(g, "You throw $p at $N@n, but unfortunatly miss!", true, ch, obj, vict, TO_CHAR);
        act(g, "$n@n throws $p at $N@n, but unfortunatly misses!", true, ch, obj, vict, TO_NOTVICT);
        act(g, "$n@n throws $p at you, but thankfully misses you.", true, ch, obj, vict, TO_VICT);
      }
      decCurST(g, ch, ((getEffMaxPL(g, ch) / 100) + obj_get_weight(obj)), 0);
      if (!obj_extra_flagged(obj, ITEM_UNBREAKABLE)) {
        obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (odam / 2));
      }
      char_set_lastattack(ch, -50);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      obj_from_char(g, obj);
      obj_to_room(g, obj, char_get_in_room(vict));
      decCurST(g, ch, ((getEffMaxPL(g, ch) / 200) + obj_get_weight(obj)), 0);
      if (!char_get_equipment(ch, WEAR_WIELD1) && !char_get_equipment(ch, WEAR_WIELD2))
        perc += 20;
      if (perc + char_get_aff_abils_cha(ch) >= chance + penalty && multithrow == true && getCurPL(g, vict) > 1 && char_get_throws(ch) > 1) {
        do_throw(g, ch, argument, 0, 0);
        char_set_throws(ch, char_get_throws(ch) - 1);
      } else if (perc + char_get_aff_abils_cha(ch) >= chance + penalty && multithrow == true && getCurPL(g, vict) > 1 && char_get_throws(ch) == 1) {
        do_throw(g, ch, argument, 0, 0);
        char_set_throws(ch, -1);
      } else {
        char_set_throws(ch, 0);
      }
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (perc - (perc2 / 10) > prob) {
      miss = false;
    }
    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_ENERGIZE) && (getCurKI(g, ch)) >= getMaxKI(g, ch) * 0.02) {
      damage += (damage * (0.0016 * char_get_skill(ch, SKILL_ENERGIZE)));
      act(g, "You charge $p with the energy in your fingertips! As it begins to @Yglow a bright hot @Rred@n you throw $p at $N@n full speed, and watch it smash into $M!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n charges $p with the energy in $s fingertips! As it begins to @Yglow a bright hot @Rred@n $e throws $p at $N@n full speed, and watches it smash into $M!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n charges $p with the energy in $s fingertips! As it begins to @Yglow a bright hot @Rred@n $e throws $p at YOU@n full speed, and watches it smash into YOU!!", true, ch, obj, vict, TO_VICT);
      if (getMaxKI(g, ch) * 0.02 > 0) {
        decCurKI(g, ch, getMaxKI(g, ch) * .02, 0);
      } else {
        decCurKI(g, ch, 1, 0);
      }
      improve_skill(g, ch, SKILL_ENERGIZE, 0);
    } else if (wtype == 0) {
      act(g, "You throw $p at $N@n full speed, and watch it smash into $M!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n throws $p at $N@n full speed, and watches it smash into $M!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n throws $p at you full speed. You reel as it smashes into your body!", true, ch, obj, vict, TO_VICT);
    } else if (wtype == 1 || wtype == 2) {
      act(g, "You pull out and throw $p at $N@n full speed, and watch it sink into $M!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n pulls out and throws $p at $N@n full speed, and watches it sink into $M!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n pulls out and throws $p at you full speed. You reel as it sink into your body!", true, ch, obj, vict, TO_VICT);
    } else if (wtype == 3) {
      act(g, "You swing $p overhead and throw it at $N@n full speed, and watch it slam into $M!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n swings $p overhead and throws it at $N@n full speed, and watches it slam into $M!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n swings $p overhead and throws it at you full speed. You reel as it slam into your body!", true, ch, obj, vict, TO_VICT);
    } else if (wtype == 4) {
      act(g, "You bring $p over your shoulder and throw it at $N@n full speed, and watch it sink into $M!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n brings $p over $s shoulder and throws it at $N@n full speed, and watches it sink into $M!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n brings $p over $s shoulder and throws $p at you full speed. You reel as it sink into your body!", true, ch, obj, vict, TO_VICT);
    } else if (wtype == 5) {
      act(g, "You pull out and throw $p at $N@n full speed, and watch it hit $M!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n pulls out and throws $p at $N@n full speed, and watches it hit $M!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n pulls out and throws $p at you full speed. You reel as it hits your body!", true, ch, obj, vict, TO_VICT);
    }
    if (!obj_extra_flagged(obj, ITEM_UNBREAKABLE)) {
      obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (odam));
    }
    char_set_lastattack(ch, -50);
    if ((obj_get_value(obj, VAL_ALL_HEALTH) - odam) <= 0 && !obj_extra_flagged(obj, ITEM_UNBREAKABLE)) {
      act(g, "You smile as $p breaks on $N's@n face!", true, ch, obj, vict, TO_CHAR);
      act(g, "$n@n smiles as $p breaks on $N's@n face!", true, ch, obj, vict, TO_NOTVICT);
      act(g, "$n@n smiles as $p breaks on your face!", true, ch, obj, vict, TO_VICT);
      obj_extra_toggle(obj, ITEM_BROKEN);
    } else if (char_get_aff_abils_dex(ch) >= axion_dice(g, 0)) {
      if (char_is_android(vict) || char_is_mechanical(vict)) {
        act(g, "@RSome pieces of metal are sent flying!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@RSome pieces of metal are sent flying!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@RSome pieces of metal are sent flying!@n", true, ch, nullptr, vict, TO_NOTVICT);
      } else if (char_is_majin(vict)) {
        act(g, "@RA wide hole is left in $S gooey flesh!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@RA wide hole is left is your gooey flesh!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@RA wide hole is left in $N@R's gooey flesh@n", true, ch, nullptr, vict, TO_NOTVICT);
      } else {
        act(g, "@RBlood flies out from the impact!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@RBlood flies out from the impact!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@RBlood flies out from the impact!@n", true, ch, nullptr, vict, TO_NOTVICT);
      }
      if (obj_extra_flagged(obj, ITEM_ICE)) {
        if (!char_is_android(vict) && !char_is_icer(vict)) {
          decCurST(g, vict, (getMaxST(g, vict) * .005) + obj_get_weight(obj), 0);
          act(g, "@mYou lose some stamina to the @ccold@m!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@m loses some stamina to the @ccold@m!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@C$N@m loses some stamina to the @ccold@m!@n", true, ch, nullptr, vict, TO_NOTVICT);
        }
      }
      damage *= calc_critical(g, ch, 0);
    }
    if (hot == true) {
      if (!char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
        act(g, "@R$N@R is burned by it!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@RYou are burned by it!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@R$N@R is burned by it!@n", true, ch, nullptr, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_BURNED, true);
        damage += damage * 0.4;
      }
    }
    if (char_get_preference(ch) == PREFERENCE_KI) {
      damage -= damage * 0.20;
    }
    if (obj_get_vnum(g, obj) == 5899 || obj_get_vnum(g, obj) == 5898) {
      damage *= 0.35;
    }
    hurt(g, 0, 0, ch, vict, nullptr, damage, 0);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(vict));

    decCurST(g, ch, ((getEffMaxPL(g, ch) / 200) + obj_get_weight(obj)), 0);
    if (!char_get_equipment(ch, WEAR_WIELD1) && !char_get_equipment(ch, WEAR_WIELD2))
      perc += 12;
    if (perc + char_get_aff_abils_cha(ch) >= chance + penalty && multithrow == true && getCurPL(g, vict) > 1 && char_get_throws(ch) > 1) {
      do_throw(g, ch, argument, 0, 0);
      char_set_throws(ch, char_get_throws(ch) - 1);
    } else if (perc + char_get_aff_abils_cha(ch) >= chance + penalty && multithrow == true && getCurPL(g, vict) > 1 && char_get_throws(ch) == 1) {
      do_throw(g, ch, argument, 0, 0);
      char_set_throws(ch, -1);
    } else {
      char_set_throws(ch, 0);
    }
    char_set_wait(ch, PULSE_3SEC);
    return;
  } /* End object section. */

  /* We are throwing a character at someone else. */
  if (tch) {
    if (tch == vict) {
      send_to_char(g, ch, "You can't throw someone at theirself.\r\n");
      return;
    }

    if (!can_kill(g, ch, tch, nullptr, 0)) {
      send_to_char(g, ch, "The one you are throwing can't be harmed.\r\n");
      return;
    }

    if (char_get_speedi(g, tch) < char_get_speedi(g, ch) && rand_number(g, 1, 106) < char_get_skill(ch, SKILL_THROW)) {
      grab = true;
    }

    if ((getCurST(g, ch)) < ((getEffMaxPL(g, ch) / 100) + char_get_pc_weight(g, tch))) {
      send_to_char(g, ch, "You do not have enough stamina to do it...\r\n");
      return;
    }
    if (char_get_pc_weight(g, tch) + room_get_gravity(g, char_get_in_room(ch)) > getMaxCarryWeight(g, ch)) {
      send_to_char(g, ch, "The gravity has made them too heavy for you to throw!\r\n");
      return;
    }
    if (grab == false) {
      act(g, "@WYou try to grab @C$N@W and throw them, but they manage to dodge your attempt!@n", true, ch, nullptr, tch, TO_CHAR);
      act(g, "@C$n@W tries to @RGRAB@W you and @RTHROW@W you, but you manage to dodge the attempt!@n", true, ch, nullptr, tch, TO_VICT);
      act(g, "@C$n@W tries to @RGRAB@W @c$N@W and @RTHROW@W $M, but $E manages to dodge the attempt!@n", true, ch, nullptr, tch, TO_NOTVICT);
      hurt(g, 0, 0, ch, tch, nullptr, 0, 0);
      handle_cooldown(g, ch, 5);
      decCurST(g, ch, (getEffMaxPL(g, ch) / 200) + char_get_pc_weight(g, tch), 0);
      return;
    }
    handle_cooldown(g, ch, 5);
    improve_skill(g, ch, SKILL_THROW, 0);
    damage = ((char_get_pc_weight(g, tch) * char_get_aff_abils_str(ch)) * (char_get_aff_abils_cha(ch) / 3)) + (getEffMaxPL(g, ch) / 100);
    damage += room_get_gravity(g, char_get_in_room(ch)) * (room_get_gravity(g, char_get_in_room(ch)) / 2);
    perc  = init_skill(g, ch, SKILL_THROW);
    perc2 = init_skill(g, vict, SKILL_DODGE);
    prob  = rand_number(g, 1, 106);
    if (perc - (perc2 / 10) < prob) {
      if (perc2 > 0) {
        act(g, "@WYou grab @C$N@W and spinning around quickly you throw $M!@n", true, ch, nullptr, tch, TO_CHAR);
        act(g, "@C$n@W grabs YOU and spinning around quickly $e throws you!@n", true, ch, nullptr, tch, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n", true, ch, nullptr, tch, TO_NOTVICT);
        act(g, "@WThrown through the air, YOU fly at @c$N@W, but $E manages to dodge and you manage recover your bearings a moment later!@n", true, tch, nullptr, vict, TO_CHAR);
        act(g, "@WThrown through the air, @C$n@W flies at YOU, but you manage to dodge and @C$n@W recovers $s bearings a moment later!@n", true, tch, nullptr, vict, TO_VICT);
        act(g, "@WThrown through the air, @C$n@W flies at @c$N@W, but $E manages to dodge and @C$n@W recovers $s bearingsa moment later!@n", true, tch, nullptr, vict, TO_NOTVICT);
      } else if (perc2 <= 0) {
        act(g, "@WYou grab @C$N@W and spinning around quickly you throw $M!@n", true, ch, nullptr, tch, TO_CHAR);
        act(g, "@C$n@W grabs YOU and spinning around quickly $e throws you!@n", true, ch, nullptr, tch, TO_VICT);
        act(g, "@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n", true, ch, nullptr, tch, TO_NOTVICT);
        act(g, "@WThrown through the air, YOU fly at @c$N@W, but the throw is a miss! You manage recover your bearings a moment later!@n", true, tch, nullptr, vict, TO_CHAR);
        act(g, "@WThrown through the air, @C$n@W flies at YOU, but the throw is a miss! @C$n@W recovers $s bearings a moment later!@n", true, tch, nullptr, vict, TO_VICT);
        act(g, "@WThrown through the air, @C$n@W flies at @c$N@W, but the throw is a miss! @C$n@W recovers $s bearingsa moment later!@n", true, tch, nullptr, vict, TO_NOTVICT);
      }
      decCurST(g, ch, ((getEffMaxPL(g, ch) / 100) + char_get_pc_weight(g, tch)), 0);
      act(g, "@W--@R$N@W--@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@W--@R$N@W--@n", true, tch, nullptr, vict, TO_CHAR);
      act(g, "@W--@RYOU@W--@n", true, vict, nullptr, nullptr, TO_CHAR);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      act(g, "@W--@R$N@W--@n", true, ch, nullptr, tch, TO_CHAR);
      act(g, "@W--@R$N@W--@n", true, vict, nullptr, tch, TO_CHAR);
      act(g, "@W--@RYOU@W--@n", true, tch, nullptr, nullptr, TO_CHAR);
      hurt(g, 0, 0, ch, tch, nullptr, 0, 0);
      return;
    }
    if (perc - (perc2 / 10) >= prob) {
      miss = false;
    }
    if (miss == false) {
      act(g, "@WYou grab @C$N@W and spinning around quickly you throw $M!@n", true, ch, nullptr, tch, TO_CHAR);
      act(g, "@C$n@W grabs YOU and spinning around quickly $e throws you!@n", true, ch, nullptr, tch, TO_VICT);
      act(g, "@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n", true, ch, nullptr, tch, TO_NOTVICT);
      act(g, "@WThrown through the air, YOU fly at @c$N@W and smash into $M!@n", true, tch, nullptr, vict, TO_CHAR);
      act(g, "@WThrown through the air, @C$n@W flies at YOU and smashes into YOU!@n", true, tch, nullptr, vict, TO_VICT);
      act(g, "@WThrown through the air, @C$n@W flies at @c$N@W and smashes into $M!@n", true, tch, nullptr, vict, TO_NOTVICT);
      act(g, "@W--@R$N@W--@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@W--@R$N@W--@n", true, tch, nullptr, vict, TO_CHAR);
      act(g, "@W--@RYOU@W--@n", true, vict, nullptr, nullptr, TO_CHAR);
      hurt(g, 0, 0, ch, vict, nullptr, damage, 0);
      act(g, "@W--@R$N@W--@n", true, ch, nullptr, tch, TO_CHAR);
      if (vict) {
        act(g, "@W--@R$N@W--@n", true, vict, nullptr, tch, TO_CHAR);
      }
      act(g, "@W--@RYOU@W--@n", true, tch, nullptr, nullptr, TO_CHAR);
      hurt(g, 0, 0, ch, tch, nullptr, damage, 0);
    }
    decCurST(g, ch, ((getEffMaxPL(g, ch) / 200) + char_get_pc_weight(g, tch)), 0);
    char_set_wait(ch, PULSE_3SEC);
  } /* End throwing character. */

  /* Whoops!? */
  if (!obj && !tch) {
    send_to_imm(g, "ERROR: Throw resolved without character or object.");
    return;
  }
}

ACMD(do_selfd)
{

  struct char_data *tch = nullptr;
  int64_t dmg = 0;

  if (char_is_npc(ch))
    return;

  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "You can not use self destruct in the arena.\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SPIRIT)) {
    send_to_char(g, ch, "You are already dead!\r\n");
    return;
  }

  if (char_get_level(ch) < 9) {
    send_to_char(g, ch, "You can't self destruct while protected by the newbie shield!\r\n");
    return;
  }

  /*Andros Start*/
  if (char_get_con_sdcooldown(ch) > 0) {
    send_to_char(g, ch, "Your body is still recovering from the last self destruct!\r\n");
    return;
  } /*Andros End*/

  if (!char_get_skill(ch, SKILL_SELFD)) {
    int num = rand_number(g, 10, 20);
    char_set_skills_base(ch, SKILL_SELFD, num);
  }

  if (!char_plr_flagged(ch, PLR_SELFD)) {
    act(g, "@RYour body starts to glow @wwhite@R and flash. The flashes start out slowly but steadilly increase in speed. Your aura begins to burn around your body at the same time in a violent fashion!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n's body starts to glow @wwhite@R and flash. The flashes start out slowly but steadilly increase in speed. $n's aura begins to burn around $s body at the same time in a violent fashion!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_SELFD, true);
    return;
  }
  if (!char_plr_flagged(ch, PLR_SELFD2)) {
    act(g, "@wYour body slowly stops flashing. Steam rises from your skin as you slowly let off the energy you built up in a safe manner.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@w$n's body slowly stops flashing. Steam rises from $s skin as $e slowly lets off the energy $e built up in a safe manner.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_SELFD, false);
    return;
  }
  if (char_get_grappling(g, ch) != nullptr && !can_kill(g, ch, char_get_grappling(g, ch), nullptr, 3)) {
    act(g, "@wYour body slowly stops flashing. Steam rises from your skin as you slowly let off the energy you built up in a safe manner.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@w$n's body slowly stops flashing. Steam rises from $s skin as $e slowly lets off the energy $e built up in a safe manner.@n", true, ch, nullptr, nullptr, TO_ROOM);
    send_to_char(g, ch, "You can't kill them, the immortals won't allow it!\r\n");
    char_plr_set(ch, PLR_SELFD, false);
    return;
  }
  if (char_get_grappling(g, ch) != nullptr) {
    tch = char_get_grappling(g, ch);
    dmg += char_get_charge(ch);
    char_set_charge(ch, 0);
    dmg += (getBasePL(ch)) * 0.6;
    dmg += (getBaseST(ch));
    decCurHealthPercent(g, ch, 1, 1);
    char_set_suppressed(ch, 0);
    char_set_suppression(ch, 0);
    act(g, "@RYou EXPLODE! The explosion concentrates on @r$N@R, engulfing $M in a sphere of deadly energy!@n", true, ch, nullptr, tch, TO_CHAR);
    act(g, "@R$n EXPLODES! The explosion concentrates on YOU, engulfing your body in a sphere of deadly energy!@n", true, ch, nullptr, tch, TO_VICT);
    act(g, "@R$n EXPLODES! The explosion concentrates on @r$N@R, engulfing $M in a sphere of deadly energy!@n", true, ch, nullptr, tch, TO_NOTVICT);
    hurt(g, 0, 0, ch, tch, nullptr, dmg, 1);
    char_plr_set(ch, PLR_SELFD, false);
    char_plr_set(ch, PLR_SELFD2, false);
    if (char_plr_flagged(ch, PLR_IMMORTAL)) {
      char_set_con_sdcooldown(ch, 600);
    }
    if ((char_is_majin(ch) || char_is_bio(ch)) && getCurLFPercent(ch) > 0.5) {
      decCurLFPercent(g, ch, 2, -1);
      char_plr_set(ch, PLR_GOOP, true);
      char_set_gooptime(ch, 70);
    } else {
      die(g, ch, nullptr);
    }
    int num = rand_number(g, 10, 20) + char_get_skill(ch, SKILL_SELFD);
    if (char_get_skill(ch, SKILL_SELFD) + num <= 100) {
      char_set_skills_base(ch, SKILL_SELFD, num);
    } else {
      char_set_skills_base(ch, SKILL_SELFD, 100);
    }
    return;
  }
  dmg += char_get_charge(ch);
  char_set_charge(ch, 0);
  dmg += (getBasePL(ch)) * 0.6;
  dmg += (getBaseST(ch));
  dmg *= 1.5;
  decCurHealthPercent(g, ch, 1, 1);
  char_set_suppressed(ch, 0);
  char_set_suppression(ch, 0);
  act(g, "@RYou EXPLODE! The explosion expands outward burning up all surroundings for a large distance. The explosion takes on the shape of a large energy dome with you at its center!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@R$n EXPLODES! The explosion expands outward burning up all surroundings for a large distance. The explosion takes on the shape of a large energy dome with $n at its center!@n", true, ch, nullptr, nullptr, TO_ROOM);
  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch) {
      continue;
    }
    if (!can_kill(g, ch, vict, nullptr, 3)) {
      continue;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    }
    act(g, "@r$N@R is caught in the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@RYou are caught in the explosion!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@r$N@R is caught in the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
    hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
  }
  if (char_plr_flagged(ch, PLR_IMMORTAL)) {
    char_set_con_sdcooldown(ch, 600);
  }
  die(g, ch, nullptr);
  char_plr_set(ch, PLR_SELFD, false);
  char_plr_set(ch, PLR_SELFD2, false);
  int num = rand_number(g, 10, 20) + char_get_skill(ch, SKILL_SELFD);
  if (char_get_skill(ch, SKILL_SELFD) + num <= 100) {
    char_set_skills_base(ch, SKILL_SELFD, num);
  } else {
    char_set_skills_base(ch, SKILL_SELFD, 100);
  }
  return;
}

ACMD(do_razor)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .14, minimum = .05;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_WRAZOR)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_WRAZOR); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 6);
  /* There is a player/mob targeted */
  if (vict) {
    if (char_is_android(vict)) {
      send_to_char(g, ch, "There is not a necessary amount of water in cybernetic creatures.\r\n");
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_WRAZOR, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Water Razor")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Water Razor, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Water Razor, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Water Razor, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Water Razor misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Water Razor at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Water Razor at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Water Razor misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Water Razor at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Water Razor at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    int64_t reduction = getCurPL(g, vict);
    dmg               = damtype(g, ch, 47, skill, attperc);
    if (char_aff_flagged(ch, AFF_SANCTUARY)) {
      if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 100) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.1));
      } else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 60) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.05));
      } else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 40) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.02));
      }
      if (char_get_barrier(ch) > getMaxKI(g, ch)) {
        char_set_barrier(ch, getMaxKI(g, ch));
      }
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        break;
      case 2: /* Critical */
        act(g, "@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out in a mist from every pore of $S body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides! Blood sprays out of your pores into a fine mist!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out of $S pores into a fine mist!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        break;
      case 3:
        act(g, "@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out in a mist from every pore of $S body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides! Blood sprays out of your pores into a fine mist!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out of $S pores into a fine mist!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        break;
      case 4: /* Weak */
        act(g, "@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        break;
    }
    pcost(g, ch, attperc, 0);
    if (vict) {
      reduction = reduction - getCurPL(g, vict);

      if ((!char_is_npc(vict) && !char_aff_flagged(vict, AFF_SPIRIT)) || (char_is_npc(vict) && getCurPL(g, vict) > 0)) {
        decCurKI(g, vict, reduction, 0);
        decCurST(g, vict, reduction, 0);
      }
    }
    return;
  }
  if (obj) {
    send_to_char(g, ch, "You can not hurt an inanimate object with this technique.\r\n");
    return;
  }
  send_to_char(g, ch, "Error! Please report.\r\n");
  return;
}

ACMD(do_spike)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;
  int64_t dmg;
  double attperc = .14, minimum = .05;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_WSPIKE)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_WSPIKE) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
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

  skill = init_skill(g, ch, SKILL_WSPIKE); /* Set skill value */

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
    improve_skill(g, ch, SKILL_WSPIKE, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    if (char_get_skills_perf(ch, SKILL_WSPIKE) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "attack")) {
      if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }
      if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
        incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
      } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
        incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
      } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
        incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
      }
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
        char_set_wait(ch, PULSE_3SEC);
      }
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W moves quickly and blocks your water spikes!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W water spikes!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W water spikes!@n", false, ch, nullptr, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3 && attperc > minimum) {
            pcost(g, ch, attperc - 0.05, 0);
          } else {
            pcost(g, ch, attperc, 0);
          }
          if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
            incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
          } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
            incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
          } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
            incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
          }
          dmg = damtype(g, ch, 10, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your water spikes, letting them slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W water spikes, letting them slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W water spikes, letting them slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(g, ch, vict, 0, 43, skill, SKILL_WSPIKE); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
          }
          if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3 && attperc > minimum) {
            pcost(g, ch, attperc - 0.05, 0);
          } else {
            pcost(g, ch, attperc, 0);
          }
          if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
            incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
          } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
            incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
          } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
            incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
          }
          if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your water spikes miss, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fire water spikes at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fire water spikes at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3 && attperc > minimum) {
          pcost(g, ch, attperc - 0.05, 0);
        } else {
          pcost(g, ch, attperc, 0);
        }
        if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
          incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
        } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
          incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
        } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
          incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
        }
        if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
          char_set_wait(ch, PULSE_3SEC);
        }
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your water spikes miss, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires water spikes at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires water spikes at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3 && attperc > minimum) {
        pcost(g, ch, attperc - 0.05, 0);
      } else {
        pcost(g, ch, attperc, 0);
      }
      if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
        incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
      } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
        incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
      } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
        incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
      }
      if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
        char_set_wait(ch, PULSE_3SEC);
      }
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 43, skill, attperc);
    if (char_aff_flagged(ch, AFF_SANCTUARY)) {
      if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 100) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.1));
      } else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 60) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.05));
      } else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 40) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.02));
      }
      if (char_get_barrier(ch) > getMaxKI(g, ch)) {
        char_set_barrier(ch, getMaxKI(g, ch));
      }
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S chest!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR chest!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
          char_set_wait(ch, PULSE_3SEC);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 2: /* Critical */
        act(g, "@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S head!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR head!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s head!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
          char_set_wait(ch, PULSE_3SEC);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        if (!char_aff_flagged(vict, AFF_KNOCKED) && (rand_number(g, 1, 3) >= 2 && (getCurPL(g, vict) > getCurPL(g, ch) / 5) && !char_aff_flagged(vict, AFF_SANCTUARY))) {
          act(g, "@C$N@W is knocked out!@n", true, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou are knocked out!@n", true, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W is knocked out!@n", true, ch, nullptr, vict, TO_NOTVICT);
          setStatusKnockedOut(vict);
        }
        break;
      case 3:
        act(g, "@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
          char_set_wait(ch, PULSE_3SEC);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 4: /* Weak */
        act(g, "@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dam_eq_loc(g, vict, 1);
        hurt(g, 0, 190, ch, vict, nullptr, dmg, 1);
        break;
      case 5: /* Weak 2 */
        act(g, "@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dam_eq_loc(g, vict, 2);
        if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
          char_set_wait(ch, PULSE_3SEC);
        }
        hurt(g, 1, 190, ch, vict, nullptr, dmg, 1);
        break;
    }
    if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3 && attperc > minimum) {
      pcost(g, ch, attperc - 0.05, 0);
    } else {
      pcost(g, ch, attperc, 0);
    }
    if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
      incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
    } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
      incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
    } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
      incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
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
    dmg = damtype(g, ch, 43, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire water spikes at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires water spikes at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    if (char_get_skills_perf(ch, SKILL_WSPIKE) == 3) {
      char_set_wait(ch, PULSE_3SEC);
    }
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);
    if (char_get_skill(ch, SKILL_WSPIKE) >= 100) {
      incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .3, true);
    } else if (char_get_skill(ch, SKILL_WSPIKE) >= 60) {
      incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .1, true);
    } else if (char_get_skill(ch, SKILL_WSPIKE) >= 40) {
      incCurKI(g, ch, (getMaxKI(g, ch) * attperc) * .05, true);
    }
  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_koteiru)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;
  double attperc = .3, minimum = .1;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_KOTEIRU)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_KOTEIRU); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 7);
  /* There is a player/mob targeted */
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_KOTEIRU, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Koteiru Bakuha")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Koteiru Bakuha, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Koteiru Bakuha, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Koteiru Bakuha, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Koteiru Bakuha misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Koteiru Bakuha at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Koteiru Bakuha at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Koteiru Bakuha misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Koteiru Bakuha at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Koteiru Bakuha at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg = damtype(g, ch, 48, skill, attperc);
    if (char_aff_flagged(ch, AFF_SANCTUARY)) {
      if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 100) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.1));
      } else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 60) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.05));
      } else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 40) {
        char_set_barrier(ch, char_get_barrier(ch) + (dmg * 0.02));
      }
      if (char_get_barrier(ch) > getMaxKI(g, ch)) {
        char_set_barrier(ch, getMaxKI(g, ch));
      }
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S chest it freezes solid around $M!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR chest it freezes solid around YOU!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S chest it freezes solid around @W$N@C!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 2: /* Critical */
        act(g, "@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S head it freezes solid around $M!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR head it freezes solid around YOU!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S head it freezes solid around @W$N@C!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        break;
      case 3:
        act(g, "@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S gut it freezes solid around $M!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR gut it freezes solid around YOU!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S gut it freezes solid around @W$N@C!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 4: /* Weak */
        act(g, "@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S arm it freezes solid around $M!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR arm it freezes solid around YOU!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S arm it freezes solid around @W$N@C!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        dam_eq_loc(g, vict, 1);
        hurt(g, 0, 190, ch, vict, nullptr, dmg, 1);
        break;
      case 5: /* Weak 2 */
        act(g, "@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S leg it freezes solid around $M!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR leg it freezes solid around YOU!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S leg it freezes solid around @W$N@C!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        dam_eq_loc(g, vict, 2);
        hurt(g, 1, 190, ch, vict, nullptr, dmg, 1);
        break;
    }
    pcost(g, ch, attperc, 0);
    if (vict) {
      if (getCurPL(g, vict) > 1) {
        if (rand_number(g, 1, 4) == 1 && !char_aff_flagged(vict, AFF_FROZEN) && !char_is_demon(vict)) {
          act(g, "@CYour body completely freezes!@n", true, vict, nullptr, nullptr, TO_CHAR);
          act(g, "@c$n's@C body completely freezes!@n", true, vict, nullptr, nullptr, TO_ROOM);
          char_aff_set(vict, AFF_FROZEN, true);
        }
      }
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
    dmg = damtype(g, ch, 48, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire Koteiru Bakuha at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires Koteiru Bakuha at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);
  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_hspiral)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;
  double attperc = .3, minimum = .15;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_HSPIRAL)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_HSPIRAL); /* Set skill value */

  if (!tech_handle_targeting(g, ch, arg, &vict, &obj)) return;

  handle_cooldown(g, ch, 6);
  /* There is a player/mob targeted */
  if (vict) {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      return;
    }
    if (handle_defender(g, vict, ch)) {
      struct char_data *def = char_get_defender(g, vict);
      vict                  = def;
    }
    improve_skill(g, ch, SKILL_HSPIRAL, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Hell Spiral")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Hell Spiral, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Hell Spiral, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Hell Spiral, letting it fly harmlessly by!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Hell Spiral misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Hell Spiral at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Hell Spiral at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Hell Spiral misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Hell Spiral at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Hell Spiral at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg         = damtype(g, ch, 49, skill, attperc);
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 2: /* Critical */
        act(g, "@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        break;
      case 3:
        act(g, "@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W body where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR body where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W body where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 4: /* Weak */
        act(g, "@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        dam_eq_loc(g, vict, 1);
        hurt(g, 0, 120, ch, vict, nullptr, dmg, 1);
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        dam_eq_loc(g, vict, 2);
        hurt(g, 1, 120, ch, vict, nullptr, dmg, 1);
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
    dmg = damtype(g, ch, 49, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire Hell Spiral at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires Hell Spiral at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);
  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_spiral)
{
  int skill;
  struct char_data *vict;
  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_SPIRAL)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * .5, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_SPIRAL); /* Set skill value */

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch)) {
      vict = char_get_fighting(g, ch);
    } else {
      send_to_char(g, ch, "Nothing around here by that name.");
      return;
    }
  }

  /* There is a player/mob targeted */
  if (!can_kill(g, ch, vict, nullptr, 3)) {
    return;
  }

  if (handle_defender(g, vict, ch)) {
    struct char_data *def = char_get_defender(g, vict);
    vict                  = def;
  }

  char_plr_set(ch, PLR_SPIRAL, true);
  improve_skill(g, ch, SKILL_SPIRAL, 0);
  act(g, "@mFlying to a spot above your intended target you begin to move so fast all that can be seen of you are trails of color. You focus your movements into a vortex and prepare to attack!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@w$n@m flies to a spot above and begins to move so fast all that can be seen of $m are trails of color. Suddenly $e focuses $s movements into a spinning vortex and you lose track of $s movements entirely!@n", true, ch, nullptr, nullptr, TO_ROOM);
  handle_spiral(g, ch, vict, skill, true);
  handle_cooldown(g, ch, 8);
}

ACMD(do_breaker)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  struct char_data *vict;
  struct obj_data *obj;
  std::string arg, arg2;
  double attperc = .14, minimum = .05;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_BREAKER)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (arg.empty() && !char_get_fighting(g, ch)) {
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

  skill = init_skill(g, ch, SKILL_BREAKER); /* Set skill value */

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
    improve_skill(g, ch, SKILL_BREAKER, 0);

    index = check_def(g, vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(g, ch, skill, true);
    perc = chance_to_hit(g, ch);

    index -= handle_speed(g, ch, vict);

    avo = index / 4;

    handle_defense(g, vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(g, ch, vict, "Star Breaker")) {
      pcost(g, ch, attperc, 0);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(g, vict)) > 0) {
        if (blk > rand_number(g, 1, 130)) {
          act(g, "@C$N@W moves quickly and blocks your Star Breaker!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou move quickly and block @C$n's@W Star Breaker!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W moves quickly and blocks @c$n's@W Star Breaker!@n", false, ch, nullptr, vict, TO_NOTVICT);
          pcost(g, ch, attperc, 0);
          dmg = damtype(g, ch, 10, skill, attperc);
          dmg /= 4;
          hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
          return;
        }
        if (dge > rand_number(g, 1, 130)) {
          act(g, "@C$N@W manages to dodge your Star Breaker, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_CHAR);
          act(g, "@WYou dodge @C$n's@W Star Breaker, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_VICT);
          act(g, "@C$N@W manages to dodge @c$n's@W Star Breaker, letting it slam into the surroundings!@n", false, ch, nullptr, vict, TO_NOTVICT);
          send_to_room(g, char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(g, ch, vict, 0, 46, skill, SKILL_BREAKER); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (room_get_dmg(g, char_get_in_room(ch)) <= 95) {
            room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + (5));
          }

          pcost(g, ch, attperc, 0);
          hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
          return;
        }
        act(g, "@WYou can't believe it but your Star Breaker misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W fires a Star Breaker at you, but misses!@n ", false, ch, nullptr, vict, TO_VICT);
        act(g, "@c$n@W fires a Star Breaker at @C$N@W, but somehow misses!@n ", false, ch, nullptr, vict, TO_NOTVICT);
        pcost(g, ch, attperc, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        return;
      }
      act(g, "@WYou can't believe it but your Star Breaker misses, flying through the air harmlessly!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W fires a Star Breaker at you, but misses!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@c$n@W fires a Star Breaker at @C$N@W, but somehow misses!@n", false, ch, nullptr, vict, TO_NOTVICT);
      pcost(g, ch, attperc, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      return;
    }
    dmg           = damtype(g, ch, 46, skill, attperc);
    int64_t theft = 0;
    if (char_get_level(ch) - 30 > char_get_level(vict)) {
      theft = 1;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    } else if (char_get_level(ch) - 20 > char_get_level(vict)) {
      theft = char_get_exp(vict) / 1000;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    } else if (char_get_level(ch) - 10 > char_get_level(vict)) {
      theft = char_get_exp(vict) / 100;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    } else if (char_get_level(ch) >= char_get_level(vict)) {
      theft = char_get_exp(vict) / 50;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    } else if (char_get_level(ch) + 10 >= char_get_level(vict)) {
      theft = char_get_exp(vict) / 500;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    } else if (char_get_level(ch) + 20 >= char_get_level(vict)) {
      theft = char_get_exp(vict) / 1000;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    } else {
      theft = char_get_exp(vict) / 2000;
      char_set_exp(vict, char_get_exp(vict) - (theft));
    }
    int hitspot = 1;
    hitspot     = roll_hitloc(g, ch, vict, skill);
    switch (hitspot) {
      case 1:
        act(g, "@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S chest!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your chest!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S chest!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 2: /* Critical */
        act(g, "@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S head!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your head!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S head!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 0);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 3);
        break;
      case 3:
        act(g, "@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S body!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your body!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S body!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (char_get_bonuses(ch, BONUS_SOFT)) {
          dmg *= calc_critical(g, ch, 2);
        }
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
        dam_eq_loc(g, vict, 4);
        break;
      case 4: /* Weak */
        act(g, "@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S arm!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your arm!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S arm!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        dam_eq_loc(g, vict, 1);
        hurt(g, 0, 180, ch, vict, nullptr, dmg, 1);
        break;
      case 5: /* Weak 2 */
        act(g, "@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S leg!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your leg!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S leg!@n", true, ch, nullptr, vict, TO_NOTVICT);
        dmg *= calc_critical(g, ch, 1);
        dam_eq_loc(g, vict, 2);
        hurt(g, 1, 180, ch, vict, nullptr, dmg, 1);
        break;
    }

    if (level_exp(ch, char_get_level(ch) + 1) - (char_get_exp(ch)) > 0 || char_get_level(ch) >= 100) {
      send_to_char(g, ch, "The returning Eldritch energy blesses you with some experience. @D[@G%s@D]@n\r\n", add_commas(theft));
      char_set_exp(ch, char_get_exp(ch) + (theft * 2));
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
    dmg = damtype(g, ch, 46, skill, attperc);
    dmg /= 10;
    act(g, "@WYou fire Star Breaker at $p@W!@n", false, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W fires Star Breaker at $p@W!@n", false, ch, obj, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    pcost(g, ch, attperc, 0);
  } else {
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
}
