/* special_beams.c: special beam attacks
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

ACMD(do_honoo)
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

  if (!check_skill(ch, SKILL_HONOO)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_HONOO) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_HONOO) == 3) {
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

  skill = init_skill(ch, SKILL_HONOO); /* Set skill value */

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
    improve_skill(ch, SKILL_HONOO, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_HONOO) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Honoo")) {
      if (char_get_skills_perf(ch, SKILL_HONOO) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      if (rroom_get_geffect(char_get_in_room(ch)) < -1) {
        send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates some!\r\n");
        rroom_set_geffect(char_get_in_room(ch), rroom_get_geffect(char_get_in_room(ch)) + (1));
      } else if (rroom_get_geffect(char_get_in_room(ch)) == -1) {
        send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates completely away!\r\n");
        rroom_set_geffect(char_get_in_room(ch), 0);
      }
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your honoo!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W honoo!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W honoo!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_HONOO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 21, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          if (rroom_get_geffect(char_get_in_room(ch)) < -1) {
            send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates some!\r\n");
            rroom_set_geffect(char_get_in_room(ch), rroom_get_geffect(char_get_in_room(ch)) + (1));
          } else if (rroom_get_geffect(char_get_in_room(ch)) == -1) {
            send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates completely away!\r\n");
            rroom_set_geffect(char_get_in_room(ch), 0);
          }
          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your honoo, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W honoo, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W honoo, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 21, skill, SKILL_HONOO); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          if (char_get_skills_perf(ch, SKILL_HONOO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          if (rroom_get_geffect(char_get_in_room(ch)) < -1) {
            send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates some!\r\n");
            rroom_set_geffect(char_get_in_room(ch), rroom_get_geffect(char_get_in_room(ch)) + (1));
          } else if (rroom_get_geffect(char_get_in_room(ch)) == -1) {
            send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates completely away!\r\n");
            rroom_set_geffect(char_get_in_room(ch), 0);
          }

          return;
        } else {
          act("@WYou can't believe it but your honoo misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a honoo at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a honoo at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          if (char_get_skills_perf(ch, SKILL_HONOO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);
          if (rroom_get_geffect(char_get_in_room(ch)) < -1) {
            send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates some!\r\n");
            rroom_set_geffect(char_get_in_room(ch), rroom_get_geffect(char_get_in_room(ch)) + (1));
          } else if (rroom_get_geffect(char_get_in_room(ch)) == -1) {
            send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates completely away!\r\n");
            rroom_set_geffect(char_get_in_room(ch), 0);
          }

          return;
        }
      } else {
        act("@WYou can't believe it but your honoo misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a honoo at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a honoo at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_HONOO) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      if (rroom_get_geffect(char_get_in_room(ch)) < -1) {
        send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates some!\r\n");
        rroom_set_geffect(char_get_in_room(ch), rroom_get_geffect(char_get_in_room(ch)) + (1));
      } else if (rroom_get_geffect(char_get_in_room(ch)) == -1) {
        send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates completely away!\r\n");
        rroom_set_geffect(char_get_in_room(ch), 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 21, skill, attperc);
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
          act("@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's body is engulfed!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR body is engulfed!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's body is engulfed!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's face is engulfed!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR face is engulfed!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's face is engulfed!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's gut is engulfed!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR gut is engulfed!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's gut is engulfed!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's arm is engulfed!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR arm is engulfed!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's arm is engulfed!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's leg is engulfed!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR leg is engulfed!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's leg is engulfed!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(1, 4) == 3 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
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
      if (char_get_skills_perf(ch, SKILL_HONOO) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      if (rroom_get_geffect(char_get_in_room(ch)) < -1) {
        send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates some!\r\n");
        rroom_set_geffect(char_get_in_room(ch), rroom_get_geffect(char_get_in_room(ch)) + (1));
      } else if (rroom_get_geffect(char_get_in_room(ch)) == -1) {
        send_to_room(char_get_in_room(ch), "The water surrounding the area evaporates completely away!\r\n");
        rroom_set_geffect(char_get_in_room(ch), 0);
      }
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
    dmg = damtype(ch, 21, skill, attperc);
    dmg /= 10;
    if (obj_get_vnum(obj) == 79)
      dmg *= 3;
    act("@WYou fire a honoo at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a honoo at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_psyblast)
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

  if (!check_skill(ch, SKILL_PSYBLAST)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3) {
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

  skill = init_skill(ch, SKILL_PSYBLAST); /* Set skill value */

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
    improve_skill(ch, SKILL_PSYBLAST, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Psychic Blast")) {
      if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your psychic blast!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W psychic blast!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W psychic blast!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 20, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your psychic blast, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W psychic blast, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W psychic blast, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 20, skill, SKILL_PSYBLAST); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your psychic blast misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a psychic blast at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a psychic blast at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your psychic blast misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a psychic blast at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a psychic blast at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 20, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's body! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR body! You scream for a moment as terrifying images sear through your mind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's body! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's head! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR head! You scream for a moment as terrifying images sear through your mind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's head! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's gut! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR gut! You scream for a moment as terrifying images sear through your mind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's gut! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's arm! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR arm! You scream for a moment as terrifying images sear through your mind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's arm! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's leg! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR leg! You scream for a moment as terrifying images sear through your mind!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's leg! $E screams for a moment as terrifying images sear through $S mind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }

      if (char_get_charge(vict) > 0 && rand_number(1, 3) == 2) {
        char_set_charge(vict, char_get_charge(vict) - (dmg / 5));
        if (char_get_charge(vict) < 0) {
          char_set_charge(vict, 0);
        }
        send_to_char(vict, "@RYou lose some of your charged ki!@n\r\n");
      }

      if (!char_aff_flagged(vict, AFF_SHOCKED) && rand_number(1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
        act("@MYour mind has been shocked!@n", TRUE, vict, 0, 0, TO_CHAR);
        act("@M$n@m's mind has been shocked!@n", TRUE, vict, 0, 0, TO_ROOM);
        char_aff_set(vict, AFF_SHOCKED, true);
      }
      if (char_get_skills_perf(ch, SKILL_PSYBLAST) == 3 && attperc > minimum) {
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
    dmg = damtype(ch, 20, skill, attperc);
    dmg /= 10;
    act("@WYou fire a psychic blast at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a psychic blast at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_sbc)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .15, minimum = .05;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_SBC)) {
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

  skill = init_skill(ch, SKILL_SBC); /* Set skill value */

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
    improve_skill(ch, SKILL_SBC, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Special Beam Cannon")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your special beam cannon, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W special beam cannon, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W special beam cannon, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 27, skill, SKILL_SBC); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 90) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (10));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your special beam cannon misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a special beam cannon at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a special beam cannon at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your special beam cannon misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a special beam cannon at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a special beam cannon at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 27, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 180, ch, vict, NULL, dmg, 1);
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
    dmg = damtype(ch, 27, skill, attperc);
    dmg /= 10;
    act("@WYou fire a special beam cannon at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a special beam cannon at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_kousengan)
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

  if (!check_skill(ch, SKILL_KOUSENGAN)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_KOUSENGAN); /* Set skill value */

  /* There is a player/mob targeted */
  handle_cooldown(ch, 5);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_KOUSENGAN, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob += 15; /* kousengan bonus */

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "kousengan")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your kousengan!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W kousengan!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W kousengan!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 42, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your kousengan, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W kousengan, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W kousengan, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 17, skill, SKILL_KOUSENGAN); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your kousengan misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a kousengan at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a kousengan at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your kousengan misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a kousengan at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a kousengan at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 42, skill, attperc);
      if (char_aff_flagged(vict, AFF_SANCTUARY)) {
        dmg *= calc_critical(ch, 1);
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          if (!char_get_bonuses(ch, BONUS_POWERHIT)) {
            dmg *= 3;
          } else {
            dmg *= 5;
          }
          act("@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(1, 4) == 3 && !char_is_demon(vict)) {
        send_to_char(vict, "@RYou are burned by the attack!@n\r\n");
        send_to_char(ch, "@RThey are burned by the attack!@n\r\n");
        char_aff_set(vict, AFF_BURNED, true);
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
    dmg = damtype(ch, 42, skill, attperc);
    dmg /= 10;
    act("@WYou fire a kousengan at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a kousengan at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_deathbeam)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .1, minimum = .05;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_DEATHBEAM)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
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

  skill = init_skill(ch, SKILL_DEATHBEAM); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 5);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_DEATHBEAM, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob += 15; /* deathbeam bonus */

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Deathbeam")) {
      if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
        char_set_wait(ch, PULSE_3SEC);
      }

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your deathbeam!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W deathbeam!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W deathbeam!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 17, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your deathbeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W deathbeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W deathbeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 17, skill, SKILL_DEATHBEAM); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }

          return;
        } else {
          act("@WYou can't believe it but your deathbeam misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a deathbeam at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a deathbeam at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }

          return;
        }
      } else {
        act("@WYou can't believe it but your deathbeam misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a deathbeam at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a deathbeam at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
        char_set_wait(ch, PULSE_3SEC);
      }
      return;
    } else {
      dmg         = damtype(ch, 17, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S body and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and point at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your body and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and point at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's body and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          break;
        case 2: /* Critical */
          act("@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S face and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your face and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's face and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          break;
        case 3:
          act("@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S gut and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your gut and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's gut and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          break;
        case 4: /* Weak */
          act("@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S arm and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your arm and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's arm and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          break;
        case 5: /* Weak 2 */
          act("@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S leg and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your leg and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's leg and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
            char_set_wait(ch, PULSE_3SEC);
          }
          break;
      }
      if (char_get_skill(ch, SKILL_DEATHBEAM) >= 100 && getCurPL(vict) >= 2) {
        decCurLF(ch, dmg * .4, -1);
      } else if (char_get_skill(ch, SKILL_DEATHBEAM) >= 60 && getCurPL(vict) >= 2) {
        decCurLF(ch, dmg * .2, -1);
      } else if (char_get_skill(ch, SKILL_DEATHBEAM) >= 40 && getCurPL(vict) >= 2) {
        decCurLF(ch, dmg * .05, -1);
      }
      if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3 && attperc > minimum) {
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
    dmg = damtype(ch, 17, skill, attperc);
    dmg /= 10;
    act("@WYou fire a deathbeam at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a deathbeam at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);
    if (char_get_skills_perf(ch, SKILL_DEATHBEAM) == 3) {
      char_set_wait(ch, PULSE_3SEC);
    }

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
