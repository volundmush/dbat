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
  struct char_data *vict = NULL, *tch = NULL;
  struct obj_data *obj = NULL;
  char arg[MAX_INPUT_LENGTH];
  char arg2[1000], chunk[2000], arg3[1000];
  int odam = 0, miss = TRUE, perc = 0, prob = 0, perc2 = 0, grab = FALSE;
  int64_t damage;

  half_chop(argument, arg, chunk);

  if (char_condition_has(ch, "mystic_melody")) {
    send_to_char(ch, "You are currently playing a song! Enter the song command "
                     "in order to stop!\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Throw what?\r\n");
    return;
  }

  if (is_sparring(ch)) {
    send_to_char(ch, "You can not spar with throw.\r\n");
    return;
  }

  if (*chunk) {
    two_arguments(chunk, arg2, arg3);
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
    if (!(tch = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch,
                   "You do not have that object or character to throw!\r\n");
      return;
    }
  }

  if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Who do you want to target?\r\n");
      return;
    }
  }

  if (GET_HIT(vict) <= 1) {
    return;
  }

  if (!can_kill(ch, vict, NULL, 1)) {
    return;
  }

  if (handle_defender(vict, ch)) {
    struct char_data *def = GET_DEFENDER(vict);
    vict = def;
  }

  /* We are throwing an object. */
  if (obj) {
    if (ch->throws == -1) {
      ch->throws = 0;
      return;
    }

    if ((getCurST(ch)) < ((GET_MAX_HIT(ch) / 200) + GET_OBJ_WEIGHT(obj))) {
      send_to_char(ch, "You do not have enough stamina to do it...\r\n");
      return;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
      send_to_char(ch, "That is broken and useless to throw!\r\n");
      return;
    }
    if (GET_OBJ_WEIGHT(obj) + room_gravity_get(char_room_get(ch)) >
        CAN_CARRY_W(ch)) {
      send_to_char(ch,
                   "The gravity has made that too heavy for you to throw!\r\n");
      return;
    } else {
      int penalty = 0, chance = axion_dice(0) + axion_dice(0), wtype = 0,
          wlvl = 1, multithrow = TRUE;
      handle_cooldown(ch, 5);
      improve_skill(ch, SKILL_THROW, 0);
      damage = ((GET_OBJ_WEIGHT(obj) / 3) * (GET_STR(ch)) * (GET_CHA(ch) / 3)) +
               (GET_MAX_HIT(ch) * 0.01);
      damage += (damage * 0.01) * (room_gravity_get(char_room_get(ch)) / 4);

      if (GET_PREFERENCE(ch) == PREFERENCE_THROWING) {
        chance -= chance * 0.25;
      }

      if (OBJ_FLAGGED(obj, ITEM_WEAPLVL1)) {
        damage += damage * 0.1;
        wlvl = 1;
      } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL2)) {
        damage += damage * 0.2;
        wlvl = 2;
      } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL3)) {
        damage += damage * 0.3;
        wlvl = 3;
      } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL4)) {
        damage += damage * 0.4;
        wlvl = 4;
      } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL5)) {
        damage += damage * 0.5;
        wlvl = 5;
      }
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
        if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
          wtype = 1;
        } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                   TYPE_SLASH - TYPE_HIT) {
          wtype = 2;
        } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                   TYPE_CRUSH - TYPE_HIT) {
          wtype = 3;
        } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                   TYPE_STAB - TYPE_HIT) {
          wtype = 4;
        } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) ==
                   TYPE_BLAST - TYPE_HIT) {
          wtype = 5;
          damage = ((GET_OBJ_WEIGHT(obj) * GET_STR(ch)) * (GET_CHA(ch) / 3)) +
                   (GET_MAX_HIT(ch) * 0.01);
          damage += room_gravity_get(char_room_get(ch)) *
                    (room_gravity_get(char_room_get(ch)) / 2);
        } else {
          wtype = 6;
        }
      }
      if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL) {
        odam = rand_number(5, 30);
      } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_IRON) {
        odam = rand_number(18, 50);
      } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL) {
        odam = rand_number(5, 15);
      } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_KACHIN) {
        odam = rand_number(5, 15);
      } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
        odam = rand_number(20, 50);
      } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_DIAMOND) {
        odam = rand_number(5, 20);
      } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_ENERGY) {
        if (rand_number(1, 2) == 2) {
          odam = 0;
        } else {
          odam = rand_number(1, 3);
        }
      } else {
        odam = rand_number(90, 100);
      }
      if (!OBJ_FLAGGED(obj, ITEM_THROW)) {
        penalty = 15;
        multithrow = FALSE;
        damage = damage * 0.45;
      } else {
        odam = rand_number(0, 1);
        damage +=
            (GET_STR(ch)) * ((GET_HIT(ch) * 0.00012) + rand_number(1, 20));
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

      int hot = FALSE;
      if (OBJ_FLAGGED(obj, ITEM_HOT)) {
        hot = TRUE;
      }

      if (wtype > 0 && wtype != 5 && odam > 1) {
        odam = 1;
      }
      perc = init_skill(ch, SKILL_THROW);
      perc2 = init_skill(vict, SKILL_DODGE);
      prob = axion_dice(penalty);
      if (*arg3) {
        if (!strcasecmp(arg3, "1") || !strcasecmp(arg3, "single")) {
          multithrow = FALSE;
        } else {
          send_to_char(ch,
                       "Syntax: throw (obj | character) (target) <-- This will "
                       "multithrow if able\nSyntax: throw (obj) (target) (1 | "
                       "single) <-- This will not multi throw)\r\n");
          return;
        }
      }

      if (!tech_handle_zanzoken(ch, vict, "$p")) {
        char_condition_remove(ch, "combo", "end_combo");
        int stcost = ((GET_MAX_HIT(ch) / 200) + GET_OBJ_WEIGHT(obj));
        char_condition_remove(vict, "zanzoken", "zanzoken_over");
        pcost(ch, 0, stcost / 2);
        pcost(vict, 0, GET_MAX_HIT(vict) / 200);
        obj_from_char(obj);
        obj_to_room(obj, char_room_get(vict));
        return;
      }

      if (perc - (perc2 / 10) < prob) {
        if (OBJ_FLAGGED(obj, ITEM_ICE) && IS_DEMON(vict)) {
          act("You throw $p at $N@n, but it melts before touching $M!", TRUE,
              ch, obj, vict, TO_CHAR);
          act("$n@n throws $p at $N@n, but it melts before touching $M!", TRUE,
              ch, obj, vict, TO_NOTVICT);
          act("$n@n throws $p at you, but it melts before touching you!", TRUE,
              ch, obj, vict, TO_VICT);
          decCurST(ch, ((GET_MAX_HIT(ch) / 100) + GET_OBJ_WEIGHT(obj)));
          extract_obj(obj);
          return;
        }
        if (perc2 > 0) {
          act("You throw $p at $N@n, but $E manages to dodge it easily!", TRUE,
              ch, obj, vict, TO_CHAR);
          act("$n@n throws $p at $N@n, but $E manages to dodge it easily!",
              TRUE, ch, obj, vict, TO_NOTVICT);
          act("$n@n throws $p at you, but you easily dodge it.", TRUE, ch, obj,
              vict, TO_VICT);
        } else if (perc2 <= 0) {
          act("You throw $p at $N@n, but unfortunatly miss!", TRUE, ch, obj,
              vict, TO_CHAR);
          act("$n@n throws $p at $N@n, but unfortunatly misses!", TRUE, ch, obj,
              vict, TO_NOTVICT);
          act("$n@n throws $p at you, but thankfully misses you.", TRUE, ch,
              obj, vict, TO_VICT);
        }
        decCurST(ch, ((GET_MAX_HIT(ch) / 100) + GET_OBJ_WEIGHT(obj)));
        if (!OBJ_FLAGGED(obj, ITEM_UNBREAKABLE)) {
          GET_OBJ_VAL(obj, VAL_ALL_HEALTH) -= odam / 2;
        }
        LASTATK(ch) = -50;
        hurt(0, 0, ch, vict, NULL, 0, 0);
        obj_from_char(obj);
        obj_to_room(obj, char_room_get(vict));
        decCurST(ch, ((GET_MAX_HIT(ch) / 200) + GET_OBJ_WEIGHT(obj)));
        if (!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2))
          perc += 20;
        if (perc + GET_CHA(ch) >= chance + penalty && multithrow == TRUE &&
            GET_HIT(vict) > 1 && ch->throws > 1) {
          do_throw(ch, argument, 0, 0);
          ch->throws -= 1;
        } else if (perc + GET_CHA(ch) >= chance + penalty &&
                   multithrow == TRUE && GET_HIT(vict) > 1 && ch->throws == 1) {
          do_throw(ch, argument, 0, 0);
          ch->throws = -1;
        } else {
          ch->throws = 0;
        }
        WAIT_STATE(ch, PULSE_3SEC);
        return;
      } else if (perc - (perc2 / 10) > prob) {
        miss = FALSE;
      }
      if (!IS_NPC(ch) && char_condition_has(ch, "energize") &&
          (getCurKI(ch)) >= GET_MAX_MANA(ch) * 0.02) {
        damage += (damage * (0.0016 * GET_SKILL(ch, SKILL_ENERGIZE)));
        act("You charge $p with the energy in your fingertips! As it begins to "
            "@Yglow a bright hot @Rred@n you throw $p at $N@n full speed, and "
            "watch it smash into $M!",
            TRUE, ch, obj, vict, TO_CHAR);
        act("$n@n charges $p with the energy in $s fingertips! As it begins to "
            "@Yglow a bright hot @Rred@n $e throws $p at $N@n full speed, and "
            "watches it smash into $M!",
            TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n@n charges $p with the energy in $s fingertips! As it begins to "
            "@Yglow a bright hot @Rred@n $e throws $p at YOU@n full speed, and "
            "watches it smash into YOU!!",
            TRUE, ch, obj, vict, TO_VICT);
        if (GET_MAX_MANA(ch) * 0.02 > 0) {
          decCurKI(ch, getMaxKI(ch) * .02);
        } else {
          decCurKI(ch, 1);
        }
        improve_skill(ch, SKILL_ENERGIZE, 0);
      } else if (wtype == 0) {
        act("You throw $p at $N@n full speed, and watch it smash into $M!",
            TRUE, ch, obj, vict, TO_CHAR);
        act("$n@n throws $p at $N@n full speed, and watches it smash into $M!",
            TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n@n throws $p at you full speed. You reel as it smashes into "
            "your body!",
            TRUE, ch, obj, vict, TO_VICT);
      } else if (wtype == 1 || wtype == 2) {
        act("You pull out and throw $p at $N@n full speed, and watch it sink "
            "into $M!",
            TRUE, ch, obj, vict, TO_CHAR);
        act("$n@n pulls out and throws $p at $N@n full speed, and watches it "
            "sink into $M!",
            TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n@n pulls out and throws $p at you full speed. You reel as it "
            "sink into your body!",
            TRUE, ch, obj, vict, TO_VICT);
      } else if (wtype == 3) {
        act("You swing $p overhead and throw it at $N@n full speed, and watch "
            "it slam into $M!",
            TRUE, ch, obj, vict, TO_CHAR);
        act("$n@n swings $p overhead and throws it at $N@n full speed, and "
            "watches it slam into $M!",
            TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n@n swings $p overhead and throws it at you full speed. You reel "
            "as it slam into your body!",
            TRUE, ch, obj, vict, TO_VICT);
      } else if (wtype == 4) {
        act("You bring $p over your shoulder and throw it at $N@n full speed, "
            "and watch it sink into $M!",
            TRUE, ch, obj, vict, TO_CHAR);
        act("$n@n brings $p over $s shoulder and throws it at $N@n full speed, "
            "and watches it sink into $M!",
            TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n@n brings $p over $s shoulder and throws $p at you full speed. "
            "You reel as it sink into your body!",
            TRUE, ch, obj, vict, TO_VICT);
      } else if (wtype == 5) {
        act("You pull out and throw $p at $N@n full speed, and watch it hit "
            "$M!",
            TRUE, ch, obj, vict, TO_CHAR);
        act("$n@n pulls out and throws $p at $N@n full speed, and watches it "
            "hit $M!",
            TRUE, ch, obj, vict, TO_NOTVICT);
        act("$n@n pulls out and throws $p at you full speed. You reel as it "
            "hits your body!",
            TRUE, ch, obj, vict, TO_VICT);
      }
      if (!OBJ_FLAGGED(obj, ITEM_UNBREAKABLE)) {
        GET_OBJ_VAL(obj, VAL_ALL_HEALTH) -= odam;
      }
      LASTATK(ch) = -50;
      if ((GET_OBJ_VAL(obj, VAL_ALL_HEALTH) - odam) <= 0 &&
          !OBJ_FLAGGED(obj, ITEM_UNBREAKABLE)) {
        act("You smile as $p breaks on $N's@n face!", TRUE, ch, obj, vict,
            TO_CHAR);
        act("$n@n smiles as $p breaks on $N's@n face!", TRUE, ch, obj, vict,
            TO_NOTVICT);
        act("$n@n smiles as $p breaks on your face!", TRUE, ch, obj, vict,
            TO_VICT);
        TOGGLE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
      } else if (GET_DEX(ch) >= axion_dice(0)) {
        if (IS_ANDROID(vict) || IS_MECHANICAL(vict)) {
          act("@RSome pieces of metal are sent flying!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@RSome pieces of metal are sent flying!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@RSome pieces of metal are sent flying!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
        } else if (IS_MAJIN(vict)) {
          act("@RA wide hole is left in $S gooey flesh!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@RA wide hole is left is your gooey flesh!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@RA wide hole is left in $N@R's gooey flesh@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
        } else {
          act("@RBlood flies out from the impact!@n", TRUE, ch, 0, vict,
              TO_CHAR);
          act("@RBlood flies out from the impact!@n", TRUE, ch, 0, vict,
              TO_VICT);
          act("@RBlood flies out from the impact!@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
        }
        if (OBJ_FLAGGED(obj, ITEM_ICE)) {
          if (!IS_ANDROID(vict) && !IS_ICER(vict)) {
            decCurST(vict, (getMaxST(vict) * .005) + GET_OBJ_WEIGHT(obj));
            act("@mYou lose some stamina to the @ccold@m!@n", TRUE, ch, 0, vict,
                TO_VICT);
            act("@C$N@m loses some stamina to the @ccold@m!@n", TRUE, ch, 0,
                vict, TO_CHAR);
            act("@C$N@m loses some stamina to the @ccold@m!@n", TRUE, ch, 0,
                vict, TO_NOTVICT);
          }
        }
        damage *= calc_critical(ch, 0);
      }
      if (hot == TRUE) {
        if (!IS_DEMON(vict) && !GET_BONUS(vict, BONUS_FIREPROOF)) {
          act("@R$N@R is burned by it!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@RYou are burned by it!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@R$N@R is burned by it!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          char_condition_add(vict, "burned", "attack", "fiery");
          damage += damage * 0.4;
        }
      }
      if (GET_PREFERENCE(ch) == PREFERENCE_KI) {
        damage -= damage * 0.20;
      }
      if (GET_OBJ_VNUM(obj) == 5899 || GET_OBJ_VNUM(obj) == 5898) {
        damage *= 0.35;
      }
      hurt(0, 0, ch, vict, NULL, damage, 0);
      obj_from_char(obj);
      obj_to_room(obj, char_room_get(vict));

      decCurST(ch, ((GET_MAX_HIT(ch) / 200) + GET_OBJ_WEIGHT(obj)));
      if (!GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2))
        perc += 12;
      if (perc + GET_CHA(ch) >= chance + penalty && multithrow == TRUE &&
          GET_HIT(vict) > 1 && ch->throws > 1) {
        do_throw(ch, argument, 0, 0);
        ch->throws -= 1;
      } else if (perc + GET_CHA(ch) >= chance + penalty && multithrow == TRUE &&
                 GET_HIT(vict) > 1 && ch->throws == 1) {
        do_throw(ch, argument, 0, 0);
        ch->throws = -1;
      } else {
        ch->throws = 0;
      }
      WAIT_STATE(ch, PULSE_3SEC);
      return;
    }
  } /* End object section. */

  /* We are throwing a character at someone else. */
  if (tch) {
    if (tch == vict) {
      send_to_char(ch, "You can't throw someone at theirself.\r\n");
      return;
    }

    if (!can_kill(ch, tch, NULL, 0)) {
      send_to_char(ch, "The one you are throwing can't be harmed.\r\n");
      return;
    }

    if (GET_SPEEDI(tch) < GET_SPEEDI(ch) &&
        rand_number(1, 106) < GET_SKILL(ch, SKILL_THROW)) {
      grab = TRUE;
    }

    if ((getCurST(ch)) < ((GET_MAX_HIT(ch) / 100) + GET_PC_WEIGHT(tch))) {
      send_to_char(ch, "You do not have enough stamina to do it...\r\n");
      return;
    }
    if (GET_PC_WEIGHT(tch) + room_gravity_get(char_room_get(ch)) >
        CAN_CARRY_W(ch)) {
      send_to_char(ch,
                   "The gravity has made them too heavy for you to throw!\r\n");
      return;
    }
    if (grab == FALSE) {
      act("@WYou try to grab @C$N@W and throw them, but they manage to dodge "
          "your attempt!@n",
          TRUE, ch, 0, tch, TO_CHAR);
      act("@C$n@W tries to @RGRAB@W you and @RTHROW@W you, but you manage to "
          "dodge the attempt!@n",
          TRUE, ch, 0, tch, TO_VICT);
      act("@C$n@W tries to @RGRAB@W @c$N@W and @RTHROW@W $M, but $E manages to "
          "dodge the attempt!@n",
          TRUE, ch, 0, tch, TO_NOTVICT);
      hurt(0, 0, ch, tch, NULL, 0, 0);
      handle_cooldown(ch, 5);
      decCurST(ch, (GET_MAX_HIT(ch) / 200) + GET_PC_WEIGHT(tch));
      return;
    } else {
      handle_cooldown(ch, 5);
      improve_skill(ch, SKILL_THROW, 0);
      damage = ((GET_PC_WEIGHT(tch) * GET_STR(ch)) * (GET_CHA(ch) / 3)) +
               (GET_MAX_HIT(ch) / 100);
      damage += room_gravity_get(char_room_get(ch)) *
                (room_gravity_get(char_room_get(ch)) / 2);
      perc = init_skill(ch, SKILL_THROW);
      perc2 = init_skill(vict, SKILL_DODGE);
      prob = rand_number(1, 106);
      if (perc - (perc2 / 10) < prob) {
        if (perc2 > 0) {
          act("@WYou grab @C$N@W and spinning around quickly you throw $M!@n",
              TRUE, ch, 0, tch, TO_CHAR);
          act("@C$n@W grabs YOU and spinning around quickly $e throws you!@n",
              TRUE, ch, 0, tch, TO_VICT);
          act("@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n",
              TRUE, ch, 0, tch, TO_NOTVICT);
          act("@WThrown through the air, YOU fly at @c$N@W, but $E manages to "
              "dodge and you manage recover your bearings a moment later!@n",
              TRUE, tch, 0, vict, TO_CHAR);
          act("@WThrown through the air, @C$n@W flies at YOU, but you manage "
              "to dodge and @C$n@W recovers $s bearings a moment later!@n",
              TRUE, tch, 0, vict, TO_VICT);
          act("@WThrown through the air, @C$n@W flies at @c$N@W, but $E "
              "manages to dodge and @C$n@W recovers $s bearingsa moment "
              "later!@n",
              TRUE, tch, 0, vict, TO_NOTVICT);
        } else if (perc2 <= 0) {
          act("@WYou grab @C$N@W and spinning around quickly you throw $M!@n",
              TRUE, ch, 0, tch, TO_CHAR);
          act("@C$n@W grabs YOU and spinning around quickly $e throws you!@n",
              TRUE, ch, 0, tch, TO_VICT);
          act("@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n",
              TRUE, ch, 0, tch, TO_NOTVICT);
          act("@WThrown through the air, YOU fly at @c$N@W, but the throw is a "
              "miss! You manage recover your bearings a moment later!@n",
              TRUE, tch, 0, vict, TO_CHAR);
          act("@WThrown through the air, @C$n@W flies at YOU, but the throw is "
              "a miss! @C$n@W recovers $s bearings a moment later!@n",
              TRUE, tch, 0, vict, TO_VICT);
          act("@WThrown through the air, @C$n@W flies at @c$N@W, but the throw "
              "is a miss! @C$n@W recovers $s bearingsa moment later!@n",
              TRUE, tch, 0, vict, TO_NOTVICT);
        }
        decCurST(ch, ((GET_MAX_HIT(ch) / 100) + GET_PC_WEIGHT(tch)));
        act("@W--@R$N@W--@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@W--@R$N@W--@n", TRUE, tch, 0, vict, TO_CHAR);
        act("@W--@RYOU@W--@n", TRUE, vict, 0, 0, TO_CHAR);
        hurt(0, 0, ch, vict, NULL, 0, 0);
        act("@W--@R$N@W--@n", TRUE, ch, 0, tch, TO_CHAR);
        act("@W--@R$N@W--@n", TRUE, vict, 0, tch, TO_CHAR);
        act("@W--@RYOU@W--@n", TRUE, tch, 0, 0, TO_CHAR);
        hurt(0, 0, ch, tch, NULL, 0, 0);
        return;
      } else if (perc - (perc2 / 10) >= prob) {
        miss = FALSE;
      }
      if (miss == FALSE) {
        act("@WYou grab @C$N@W and spinning around quickly you throw $M!@n",
            TRUE, ch, 0, tch, TO_CHAR);
        act("@C$n@W grabs YOU and spinning around quickly $e throws you!@n",
            TRUE, ch, 0, tch, TO_VICT);
        act("@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n",
            TRUE, ch, 0, tch, TO_NOTVICT);
        act("@WThrown through the air, YOU fly at @c$N@W and smash into $M!@n",
            TRUE, tch, 0, vict, TO_CHAR);
        act("@WThrown through the air, @C$n@W flies at YOU and smashes into "
            "YOU!@n",
            TRUE, tch, 0, vict, TO_VICT);
        act("@WThrown through the air, @C$n@W flies at @c$N@W and smashes into "
            "$M!@n",
            TRUE, tch, 0, vict, TO_NOTVICT);
        act("@W--@R$N@W--@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@W--@R$N@W--@n", TRUE, tch, 0, vict, TO_CHAR);
        act("@W--@RYOU@W--@n", TRUE, vict, 0, 0, TO_CHAR);
        hurt(0, 0, ch, vict, NULL, damage, 0);
        act("@W--@R$N@W--@n", TRUE, ch, 0, tch, TO_CHAR);
        if (vict) {
          act("@W--@R$N@W--@n", TRUE, vict, 0, tch, TO_CHAR);
        }
        act("@W--@RYOU@W--@n", TRUE, tch, 0, 0, TO_CHAR);
        hurt(0, 0, ch, tch, NULL, damage, 0);
      }
      decCurST(ch, ((GET_MAX_HIT(ch) / 200) + GET_PC_WEIGHT(tch)));
      WAIT_STATE(ch, PULSE_3SEC);
    }
  } /* End throwing character. */

  /* Whoops!? */
  if (!obj && !tch) {
    send_to_imm("ERROR: Throw resolved without character or object.");
    return;
  }
}

ACMD(do_selfd) {
  return;
}

ACMD(do_spiral) {
  int skill;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_SPIRAL)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !FIGHTING(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * .5, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_SPIRAL); /* Set skill value */

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Nothing around here by that name.");
      return;
    }
  }

  /* There is a player/mob targeted */
  if (!can_kill(ch, vict, NULL, 3)) {
    return;
  }

  if (handle_defender(vict, ch)) {
    struct char_data *def = GET_DEFENDER(vict);
    vict = def;
  }

  SET_BIT_AR(PLR_FLAGS(ch), PLR_SPIRAL);
  improve_skill(ch, SKILL_SPIRAL, 0);
  act("@mFlying to a spot above your intended target you begin to move so fast "
      "all that can be seen of you are trails of color. You focus your "
      "movements into a vortex and prepare to attack!@n",
      TRUE, ch, 0, 0, TO_CHAR);
  act("@w$n@m flies to a spot above and begins to move so fast all that can be "
      "seen of $m are trails of color. Suddenly $e focuses $s movements into a "
      "spinning vortex and you lose track of $s movements entirely!@n",
      TRUE, ch, 0, 0, TO_ROOM);
  handle_spiral(ch, vict, skill, TRUE);
  handle_cooldown(ch, 8);
}

