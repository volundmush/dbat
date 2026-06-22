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
#include "act.attack.h"
#include "consts/attacks.h"
#include "consts/maximums.h"
#include "consts/roomflags.h"
#include "consts/skills.h"

#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "combat.h"
#include "comm.h"
#include "consts/applies.h"
#include "consts/deathtype.h"
#include "consts/fightprefs.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/sex.h"
#include "dg_comm.h"
#include "extract.h"
#include "fight.h"
#include "flags.h"
#include "interpreter.h"
#include "iterate.hpp"
#include "object_impl.h"
#include "object_macros.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_api.h"
#include "search.h"
#include "spells.h"
#include "stringutils.h"
#include "techniques.h"
#include "weather_db.h"
#include <cstring>
#include <strings.h>


ACMD(do_energize) {
  return;
}

ACMD(do_breath) {
  return;
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = GET_MAX_HIT(ch) / 5000;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!IS_NPC(ch)) {
    return;
  }
  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !FIGHTING(ch)) {
    return;
  }

  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 200)) {
    return;
  }

  skill = init_skill(ch, SKILL_KNEE);

  vict = NULL;
  obj = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
      vict = FIGHTING(ch);
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL,
                                           inv_for_room(char_room_get(ch))))) {
      return;
    }
  }
  handle_cooldown(ch, 4);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = GET_DEFENDER(vict);
      vict = def;
    }
    index = check_def(vict);
    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, pry, blk, dge, prob);
    prob += 15;

    if (!tech_handle_zanzoken(ch, vict, "breath")) {
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@WYou move quickly and block @C$n's@W fiery breath!@n", TRUE, ch,
              0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W fiery breath!@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 6, skill, attperc);
          dmg = dmg * 0.8;
          if (!char_condition_has(vict, "burned") && rand_number(1, 4) == 3 &&
              !IS_DEMON(vict) && !GET_BONUS(vict, BONUS_FIREPROOF)) {
            send_to_char(vict, "@RYou are burned by the attack!@n\r\n");
            send_to_char(ch, "@RThey are burned by the attack!@n\r\n");
            char_condition_add(vict, "burned", "attack", "fiery");
          } else if (GET_BONUS(vict, BONUS_FIREPROOF) || IS_DEMON(vict)) {
            send_to_char(ch, "@RThey appear to be fireproof!@n\r\n");
          } else if (GET_BONUS(vict, BONUS_FIREPRONE)) {
            send_to_char(vict, "@RYou are extremely flammable and are burned "
                               "by the attack!@n\r\n");
            send_to_char(ch, "@RThey are easily burned!@n\r\n");
            char_condition_add(vict, "burned", "attack", "fiery");
          }
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@WYou dodge the fiery jets of flames coming from @C$n's@W "
              "mouth!@n",
              TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge the fiery jets of flames coming from "
              "@c$n's@W mouth!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@C$n@W moves to breath flames on you, but misses!@n", TRUE, ch,
              0, vict, TO_VICT);
          act("@c$n@W moves to breath flames on @C$N@W, but somehow misses!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@C$n@W moves to breath flames on you, but misses!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@c$n@W moves to breath flames on @C$N@W, but somehow misses!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, 8, skill, attperc);
      dmg += dmg * 2;
      int hitspot = 1;
      hitspot = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
      case 1:
        act("@C$n@W aims $s mouth at you and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "YOUR body!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "@c$N's@W body!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act("@C$n@W aims $s mouth at you and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "YOUR face!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "@c$N's@W face!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 0);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act("@C$n@W aims $s mouth at you and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "YOUR body!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "@c$N's@W body!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act("@C$n@W aims $s mouth at you and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "YOUR arm!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "@c$N's@W arm!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 1);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act("@C$n@W aims $s mouth at you and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "YOUR leg!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high "
            "pitched sound can be heard as the mouth opens, and as the throat "
            "is exposed a bright white flame can be seen burning there. "
            "Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto "
            "@c$N's@W leg!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 1);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      }
      pcost(ch, 0, stcost);
      if (!char_condition_has(vict, "burned") && rand_number(1, 4) == 3 &&
          !IS_DEMON(vict) && !GET_BONUS(vict, BONUS_FIREPROOF)) {
        send_to_char(vict, "@RYou are burned by the attack!@n\r\n");
        send_to_char(ch, "@RThey are burned by the attack!@n\r\n");
        char_condition_add(vict, "burned", "attack", "fiery");
      } else if (GET_BONUS(vict, BONUS_FIREPROOF) || IS_DEMON(vict)) {
        send_to_char(ch, "@RThey appear to be fireproof!@n\r\n");
      } else if (GET_BONUS(vict, BONUS_FIREPRONE)) {
        send_to_char(vict, "@RYou are extremely flammable and are burned by "
                           "the attack!@n\r\n");
        send_to_char(ch, "@RThey are easily burned!@n\r\n");
        char_condition_add(vict, "burned", "attack", "fiery");
      }
      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
    act("@C$n@W breathes flames on $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_ram) {
  return;
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = GET_MAX_HIT(ch) / 200;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!IS_NPC(ch)) {
    return;
  }
  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !FIGHTING(ch)) {
    return;
  }

  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 200)) {
    return;
  }

  skill = init_skill(ch, SKILL_KNEE);

  vict = NULL;
  obj = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
      vict = FIGHTING(ch);
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL,
                                           inv_for_room(char_room_get(ch))))) {
      return;
    }
  }
  handle_cooldown(ch, 4);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = GET_DEFENDER(vict);
      vict = def;
    }
    index = check_def(vict);
    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, pry, blk, dge, prob);

    prob -= 5;

    if (!tech_handle_zanzoken(ch, vict, "ram")) {
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@WYou move quickly and block @C$n's@W body as $e tries to ram "
              "YOU!@n",
              TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W body as $e tries to "
              "ram $M!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 6, skill, attperc);
          dmg -= dmg * 0.2;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@WYou dodge @C$n's@W attempted ram!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W attempted ram!@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@C$n@W moves to ram you, but misses!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@c$n@W moves to ram @C$N@W, but somehow misses!@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@C$n@W moves to ram you, but misses!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@c$n@W moves to ram @C$N@W, but somehow misses!@n", TRUE, ch, 0,
            vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, 8, skill, attperc);
      dmg += dmg * 1.1;
      int hitspot = 1;
      hitspot = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
      case 1:
        act("@C$n@W aims $s body at YOU and rams into YOUR body!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@c$n@W aims $s body at @C$N@W and rams into $S body!@n", TRUE, ch,
            0, vict, TO_NOTVICT);
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act("@C$n@W aims $s body at YOU and rams into YOUR face!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@c$n@W aims $s body at @C$N@W and rams into $S face!@n", TRUE, ch,
            0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 0);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act("@C$n@W aims $s body at YOU and rams into YOUR body!@n", TRUE, ch,
            0, vict, TO_VICT);
        act("@c$n@W aims $s body at @C$N@W and rams into $S body!@n", TRUE, ch,
            0, vict, TO_NOTVICT);
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act("@C$n@W aims $s body at YOU and rams into YOUR arm!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@c$n@W aims $s body at @C$N@W and rams into $S arm!@n", TRUE, ch,
            0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 1);
        hurt(0, 190, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act("@C$n@W aims $s body at YOU and rams into YOUR leg!@n", TRUE, ch, 0,
            vict, TO_VICT);
        act("@c$n@W aims $s body at @C$N@W and rams into $S leg!@n", TRUE, ch,
            0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 1);
        hurt(1, 190, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      }
      pcost(ch, 0, stcost);
      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
    act("@C$n@W rams $p@W extremely hard!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_strike) {
  return;
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg, stcost = GET_MAX_HIT(ch) / 400;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  double attperc = 0;

  one_argument(argument, arg);

  if (!IS_NPC(ch)) {
    return;
  }
  if (!can_grav(ch)) {
    return;
  }

  if (!*arg && !FIGHTING(ch)) {
    return;
  }

  if (!check_points(ch, 0, GET_MAX_HIT(ch) / 400)) {
    return;
  }

  skill = init_skill(ch, SKILL_KNEE);

  vict = NULL;
  obj = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
      vict = FIGHTING(ch);
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL,
                                           inv_for_room(char_room_get(ch))))) {
      return;
    }
  }
  handle_cooldown(ch, 4);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = GET_DEFENDER(vict);
      vict = def;
    }
    index = check_def(vict);
    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, pry, blk, dge, prob);

    prob += 5;

    if (!tech_handle_zanzoken(ch, vict, "fang strike")) {
      pcost(ch, 0, stcost / 2);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) &&
            (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_DUMMY))) {
          act("@WYou parry @C$n's@W fang strike with a punch of your own!@n",
              TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W parries @c$n's@W fang strike with a punch of $S own!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_PARRY, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(vict, -2, skill, attperc);
          dmg *= calc_critical(ch, 1);
          hurt(0, 0, vict, ch, NULL, dmg, -1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@WYou move quickly and block @C$n's@W fang strike!@n", TRUE, ch,
              0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W fang strike!@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, 0, stcost / 2);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 6, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 0);

          return;
        } else if (dge > axion_dice(10)) {
          act("@WYou dodge @C$n's@W fang strike!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W fang strike!@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        } else {
          act("@C$n@W moves to fang strike you, but misses!@n", TRUE, ch, 0,
              vict, TO_VICT);
          act("@c$n@W moves to fang strike @C$N@W, but somehow misses!@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          pcost(ch, 0, stcost / 2);
          hurt(0, 0, ch, vict, NULL, 0, 0);

          return;
        }
      } else {
        act("@C$n@W moves to fang strike you, but misses!@n", TRUE, ch, 0, vict,
            TO_VICT);
        act("@c$n@W moves to fang strike @C$N@W, but somehow misses!@n", TRUE,
            ch, 0, vict, TO_NOTVICT);
        pcost(ch, 0, stcost / 2);
      }
      hurt(0, 0, ch, vict, NULL, 0, 0);
      return;
    } else {
      dmg = damtype(ch, 8, skill, attperc);
      dmg += dmg * 0.5;
      int hitspot = 1;
      hitspot = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
      case 1:
        act("@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR "
            "body!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W launches $s body at @C$N@W and sinks $s fang strike into "
            "$S body!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 3);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 2: /* Critical */
        act("@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR "
            "face!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W launches $s body at @C$N@W and sinks $s fang strike into "
            "$S face!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 0);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 3:
        act("@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR "
            "body!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W launches $s body at @C$N@W and sinks $s fang strike into "
            "$S body!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        if (GET_BONUS(ch, BONUS_SOFT)) {
          dmg *= calc_critical(ch, 2);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 4);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 4: /* Weak */
        act("@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR "
            "arm!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W launches $s body at @C$N@W and sinks $s fang strike into "
            "$S arm!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 1);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 1);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      case 5: /* Weak 2 */
        act("@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR "
            "leg!@n",
            TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W launches $s body at @C$N@W and sinks $s fang strike into "
            "$S leg!@n",
            TRUE, ch, 0, vict, TO_NOTVICT);
        dmg *= calc_critical(ch, 1);
        hurt(0, 0, ch, vict, NULL, dmg, 0);
        dam_eq_loc(vict, 2);
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        break;
      }
      pcost(ch, 0, stcost);
      decCurST(vict, dmg * .25);
      return;
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 0)) {
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = ((GET_HIT(ch) / 10000) + (GET_STR(ch)));
    act("@C$n@W fang strikes $p@W extremely hard!@n", TRUE, ch, obj, 0,
        TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, 0, stcost);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

/* End NPC skills */

ACMD(do_combine) {

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int fire = FALSE, temp = -1, temp2 = -1;

  two_arguments(argument, arg, arg2);

  if (!has_group(ch)) {
    send_to_char(ch, "You need to be in a group!\r\n");
    return;
  } else {
    if (!*arg || (!MASTER(ch) && !*arg2)) {
      send_to_char(ch, "Follower Syntax: combine (attack)\r\n");
      send_to_char(ch, "Leader Syntax: combine (attack) (target)\r\n");
      send_to_char(ch, "Cancel Syntax: combine stop\r\n");
    } else {
      if (!strcasecmp(arg, "stop") && MASTER(ch)) {
        if (GET_COMBINE(ch) == -1) {
          send_to_char(ch, "You are not trying to combine any attacks...\r\n");
          return;
        } else {
          send_to_char(ch, "You stop your preparations to combine your attack "
                           "with a group attack.\r\n");
          send_to_char(MASTER(ch),
                       "@Y%s@C is no longer prepared to combine an attack with "
                       "the group!@n\r\n",
                       get_i_name(MASTER(ch), ch));
          char_followers_iterate(MASTER(ch), [&](struct char_data *fol) {
            if (ch != fol)
              send_to_char(fol,
                           "@Y%s@C is no longer prepared to combine an attack "
                           "with the group!@n\r\n",
                           get_i_name(fol, ch));
            return true;
          });
          GET_COMBINE(ch) = -1;
          return;
        }
      } else if (!strcasecmp(arg, "stop") && !MASTER(ch)) {
        send_to_char(
            ch,
            "You do not need to stop as you haven't prepared anything.\r\n");
        return;
      }
      int i = 0;
      for (i = 0; i < 14; i++) {
        if (strstr(arg, attack_names[i])) {
          if (i == 5) {
            if (!GET_EQ(ch, WEAR_WIELD1)) {
              send_to_char(
                  ch, "You need to wield a sword to use this technique.\r\n");
              return;
            }
            if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) !=
                TYPE_SLASH - TYPE_HIT) {
              send_to_char(ch, "You are not wielding a sword, you need one to "
                               "use this technique.\r\n");
              return;
            } else {
              temp = i;
              i = 15;
            }
          }
          temp = i;
          i = 15;
        }
      }
      if (temp == -1) {
        send_to_char(ch, "Follower Syntax: combine (attack)\r\n");
        send_to_char(ch, "Leader Syntax: combine (attack) (target)\r\n");
        send_to_char(ch, "Follower Cancel Syntax: combine stop\r\n");
        return;
      } else if (!GET_SKILL(ch, attack_skills[temp])) {
        send_to_char(ch, "You do not know that skill.\r\n");
        return;
      } else if (attack_skills[temp] == 440 && !IS_NAIL(ch)) {
        send_to_char(ch, "Only students of Nail know how to combine that "
                         "attack effectively.\r\n");
        return;
      } else if (GET_CHARGE(ch) < GET_MAX_MANA(ch) * 0.05) {
        send_to_char(
            ch,
            "You need to have the minimum of 5%s ki charged to combine.\r\n",
            "%");
      }
      if (!MASTER(ch)) {
        if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
          send_to_char(ch, "Who will your combined attack be targeting?\r\n");
          return;
        } else if (vict == ch) {
          send_to_char(ch, "No targeting yourself...\r\n");
          return;
        }
        GET_COMBINE(ch) = temp;
        char_followers_iterate(ch, [&](struct char_data *fol) {
          if (char_condition_has(fol, "group") && GET_COMBINE(fol) != -1 &&
              GET_CHARGE(fol) >= GET_MAX_MANA(fol) * 0.05)
            fire = TRUE;
          return true;
        });
        if (fire == TRUE) {
          combine_attacks(ch, vict);
          return;
        } else {
          send_to_char(ch, "You do not have any followers who have readied an "
                           "attack to combine or they do not have enough ki "
                           "anymore to combine said attack.\r\n");
          return;
        }
      } else if (MASTER(ch)) {
        if (GET_CHARGE(ch) >= GET_MAX_MANA(ch) * 0.05) {
          act("@C$n@c appears to be concentrating hard and focusing $s "
              "energy!@n\r\n",
              TRUE, ch, 0, 0, TO_ROOM);
          send_to_char(MASTER(ch),
                       "@BCOMBINE@c: @Y%s@C has prepared to combine a "
                       "@c'@G%s@c'@C with the next group attack!@n\r\n",
                       get_i_name(MASTER(ch), ch), attack_names[temp]);
          char_followers_iterate(MASTER(ch), [&](struct char_data *fol) {
            if (ch != fol)
              send_to_char(fol,
                           "@BCOMBINE@c: @Y%s@C has prepared to combine a "
                           "@c'@G%s@c'@C with the next group attack!@n\r\n",
                           get_i_name(fol, ch), attack_names[temp]);
            return true;
          });
          GET_COMBINE(ch) = temp;
        } else {
          send_to_char(ch, "You do not have the minimum 5%s ki charged.\r\n",
                       "%");
          return;
        }
      }
    }
  }
} /* End */







ACMD(do_throw) {
  return;
}
ACMD(do_selfd) {
  return;
}

ACMD(do_spiral) {
  return;
}

