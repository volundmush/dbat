/* beams.c: ki beam attacks
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

/* Combat commands below this line */

ACMD(do_galikgun)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .15, minimum = .1;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_GALIKGUN)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3) {
    minimum -= 0.05;
    if (minimum <= 0.0) {
      minimum = 0.01;
    }
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (GET_MAX_MANA(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_GALIKGUN); /* Set skill value */

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
    improve_skill(ch, SKILL_GALIKGUN, 0);
    index = check_def(vict); /* Check parry/block/dodge of vict */
    prob  = roll_accuracy(ch, skill, TRUE);

    if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 2) {
      prob += 5;
    }

    perc = chance_to_hit(ch);
    index -= handle_speed(ch, vict);
    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Galik Gun")) {
      if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your Galik Gun!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W Galik Gun!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W Galik Gun!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 16, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your Galik Gun, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W Galik Gun, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W Galik Gun, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 16, skill, SKILL_GALIKGUN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);

          if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);
          return;
        } else {
          act("@WYou can't believe it but your Galik Gun misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a Galik Gun at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a Galik Gun at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);
          return;
        }
      } else {
        act("@WYou can't believe it but your Galik Gun misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a Galik Gun at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a Galik Gun at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 16, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (char_get_skills_perf(ch, SKILL_GALIKGUN) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      handle_multihit(ch, vict);

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
    dmg = damtype(ch, 16, skill, attperc);
    dmg /= 10;
    act("@WYou fire a Galik Gun at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a Galik Gun at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);
  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_final)
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

  if (!check_skill(ch, SKILL_FINALFLASH)) {
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

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_FINALFLASH); /* Set skill value */

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
    improve_skill(ch, SKILL_FINALFLASH, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Final Flash")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your final flash!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W final flash!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W final flash!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 28, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your final flash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W final flash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W final flash, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 28, skill, SKILL_FINALFLASH); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your final flash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a final flash at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a final flash at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your final flash misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a final flash at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a final flash at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 28, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (!char_get_bonuses(ch, BONUS_POWERHIT)) {
            dmg *= 3;
          } else {
            dmg *= 5;
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
    dmg = damtype(ch, 28, skill, attperc);
    dmg /= 10;
    act("@WYou fire a final flash at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a final flash at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_tribeam)
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

  if (!check_skill(ch, SKILL_TRIBEAM)) {
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

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_TRIBEAM); /* Set skill value */

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
    improve_skill(ch, SKILL_TRIBEAM, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Tribeam")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your tribeam!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W tribeam!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W tribeam!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 26, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your tribeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W tribeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W tribeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 26, skill, SKILL_TRIBEAM); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your tribeam misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a tribeam at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a tribeam at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your tribeam misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a tribeam at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a tribeam at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 26, skill, attperc);
      if (char_get_skill(ch, SKILL_TRIBEAM) >= 100) {
        dmg += (getMaxLF(ch)) * 0.20;
      } else if (char_get_skill(ch, SKILL_TRIBEAM) >= 60) {
        dmg += (getMaxLF(ch)) * 0.10;
      } else if (char_get_skill(ch, SKILL_TRIBEAM) >= 40) {
        dmg += (getMaxLF(ch)) * 0.05;
      }

      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (!char_is_npc(vict)) {
        WAIT_STATE(vict, PULSE_2SEC);
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
    dmg = damtype(ch, 26, skill, attperc);
    dmg /= 10;
    act("@WYou fire a tribeam at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a tribeam at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_dualbeam)
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

  if (!check_skill(ch, SKILL_DUALBEAM)) {
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

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_DUALBEAM); /* Set skill value */

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
    improve_skill(ch, SKILL_DUALBEAM, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Dualbeam")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    int hits = 3;
    while (hits > 0) {
      hits -= 1;
      if (hits == 1) {
        prob -= prob / 5;
      } else if (hits <= 0) {
        return;
      } else if (!vict) {
        return;
      } else if ((char_aff_flagged(vict, AFF_SPIRIT) || GET_HIT(vict) <= 0) && !char_is_npc(vict)) {
        return;
      }
      if (prob < perc - 20) {
        if ((getCurST(vict)) > 0) {
          if (blk > axion_dice(10)) {
            act("@C$N@W moves quickly and blocks your dualbeam!@n", FALSE, ch, 0, vict, TO_CHAR);
            act("@WYou move quickly and block @C$n's@W dualbeam!@n", FALSE, ch, 0, vict, TO_VICT);
            act("@C$N@W moves quickly and blocks @c$n's@W dualbeam!@n", FALSE, ch, 0, vict, TO_NOTVICT);
            if (hits == 1) {
              pcost(ch, attperc, 0);
            }
            pcost(vict, 0, GET_MAX_HIT(vict) / 500);
            dmg = damtype(ch, 22, skill, attperc);
            dmg /= 4;
            hurt(0, 0, ch, vict, NULL, dmg, 1);

            continue;
          } else if (dge > axion_dice(10)) {
            act("@C$N@W manages to dodge your dualbeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
            act("@WYou dodge @C$n's@W dualbeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
            act("@C$N@W manages to dodge @c$n's@W dualbeam, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
            send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

            dodge_ki(ch, vict, 0, 22, skill, SKILL_DUALBEAM); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

            if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
              rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
            }
            improve_skill(vict, SKILL_DODGE, 0);
            if (hits == 1) {
              pcost(ch, attperc, 0);
            }
            hurt(0, 0, ch, vict, NULL, 0, 1);

            continue;
          } else {
            act("@WYou can't believe it but your dualbeam misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
            act("@C$n@W fires a dualbeam at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
            act("@c$n@W fires a dualbeam at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

            if (hits == 1) {
              pcost(ch, attperc, 0);
            }
            hurt(0, 0, ch, vict, NULL, 0, 1);

            continue;
          }
        } else {
          act("@WYou can't believe it but your dualbeam misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a dualbeam at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a dualbeam at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (hits == 1) {
            pcost(ch, attperc, 0);
          }
        }
        hurt(0, 0, ch, vict, NULL, 0, 1);
        continue;
      } else {
        dmg         = damtype(ch, 22, skill, attperc);
        int hitspot = 1;
        hitspot     = roll_hitloc(ch, vict, skill);
        if (char_get_skill(ch, SKILL_DUALBEAM) >= 100) {
          if (rand_number(1, 100) >= 60) {
            hitspot = 2;
          }
        } else if (char_get_skill(ch, SKILL_DUALBEAM) >= 60) {
          if (rand_number(1, 100) >= 80) {
            hitspot = 2;
          }
        } else if (char_get_skill(ch, SKILL_DUALBEAM) >= 40) {
          if (rand_number(1, 100) >= 95) {
            hitspot = 2;
          }
        }
        switch (hitspot) {
          case 1:
            act("@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S body in that instant!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR body in that instant!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S body in that instant!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            if (char_get_bonuses(ch, BONUS_SOFT)) {
              dmg *= calc_critical(ch, 2);
            }
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2: /* Critical */
            act("@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S face in that instant!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR face in that instant!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S face in that instant!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            dmg *= calc_critical(ch, 0);
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act("@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S gut in that instant!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR gut in that instant!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S gut in that instant!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            if (char_get_bonuses(ch, BONUS_SOFT)) {
              dmg *= calc_critical(ch, 2);
            }
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4: /* Weak */
            act("@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S arm in that instant!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR arm in that instant!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S arm in that instant!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            dmg *= calc_critical(ch, 1);
            hurt(0, 190, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5: /* Weak 2 */
            act("@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S leg in that instant!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR leg in that instant!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S leg in that instant!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            dmg *= calc_critical(ch, 1);
            hurt(1, 190, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        }
        if (hits == 1) {
          pcost(ch, attperc, 0);
        }
        if (GET_HIT(vict) <= 0) {
          if (hits == 2) {
            pcost(ch, attperc, 0);
          }
          hits = 0;
        }

        continue;
      }
    }
  } else if (obj) {
    if (!can_kill(ch, NULL, obj, 1)) {
      return;
    }
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      send_to_char(ch, "It is broken already!\r\n");
      return;
    }
    dmg = damtype(ch, 22, skill, attperc);
    dmg /= 10;
    act("@WYou fire a dualbeam at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a dualbeam at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_dodonpa)
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

  if (!check_skill(ch, SKILL_DODONPA)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_DODONPA) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_DODONPA) == 3) {
    minimum -= 0.05;
    if (minimum <= 0.0) {
      minimum = 0.01;
    }
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (GET_MAX_MANA(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }
  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_DODONPA); /* Set skill value */

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
    improve_skill(ch, SKILL_DODONPA, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_DODONPA) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (char_get_skills_perf(ch, SKILL_DODONPA) == 3) {
      WAIT_STATE(ch, PULSE_3SEC);
    }

    if (!tech_handle_zanzoken(ch, vict, "Dodonpa")) {
      if (char_get_skills_perf(ch, SKILL_DODONPA) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your dodonpa!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W dodonpa!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W dodonpa!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_DODONPA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 15, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your dodonpa, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W dodonpa, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W dodonpa, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 15, skill, SKILL_DODONPA); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);
          if (char_get_skills_perf(ch, SKILL_DODONPA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your dodonpa misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a dodonpa at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a dodonpa at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_DODONPA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your dodonpa misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a dodonpa at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a dodonpa at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_DODONPA) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 15, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S body and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your body and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's body and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S face and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your face and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's face and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S gut and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your gut and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's gut and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S arm and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your arm and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's arm and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S leg and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your leg and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's leg and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (rand_number(1, 3) == 2) {
        decCurKI(vict, dmg / 4, 0);
        send_to_char(vict, "@RYou feel some of your ki drained away by the attack!@n\r\n");
      }
      if (char_get_skills_perf(ch, SKILL_DODONPA) == 3 && attperc > minimum) {
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
    dmg = damtype(ch, 15, skill, attperc);
    dmg /= 10;
    act("@WYou fire a dodonpa at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a dodonpa at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_masenko)
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

  if (!check_skill(ch, SKILL_MASENKO)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_MASENKO) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_MASENKO) == 3) {
    minimum -= 0.05;
    if (minimum <= 0.0) {
      minimum = 0.01;
    }
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (GET_MAX_MANA(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_MASENKO); /* Set skill value */

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
    improve_skill(ch, SKILL_MASENKO, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_MASENKO) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Masenko")) {
      if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your masenko!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W masenko!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W masenko!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 14, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your masenko, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W masenko, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W masenko, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 14, skill, SKILL_MASENKO); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          improve_skill(vict, SKILL_DODGE, 0);
          if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }

          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your masenko misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a masenko at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a masenko at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your masenko misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a masenko at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a masenko at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 14, skill, attperc);
      if (char_get_skill(ch, SKILL_MASENKO) >= 100) {
        dmg += dmg * 0.08;
      } else if (char_get_skill(ch, SKILL_MASENKO) >= 100) {
        dmg += dmg * 0.05;
      } else if (char_get_skill(ch, SKILL_MASENKO) >= 100) {
        dmg += dmg * 0.03;
      }
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      if (char_is_piccolo(ch)) {
        if (char_get_skills_base(ch, SKILL_STYLE) >= 100)
          dmg += (GET_MAX_MANA(ch) * 0.08);
        else if (char_get_skills_base(ch, SKILL_STYLE) >= 60)
          dmg += (GET_MAX_MANA(ch) * 0.05);
        else if (char_get_skills_base(ch, SKILL_STYLE) >= 40)
          dmg += (GET_MAX_MANA(ch) * 0.03);
      }
      switch (hitspot) {
        case 1:
          act("@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (rand_number(1, 2) == 2 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
        send_to_char(vict, "@RThe attack seems to have taken a toll on your stamina!@n\r\n");
        decCurST(vict, dmg / 4, 0);
      }
      if (char_get_skills_perf(ch, SKILL_MASENKO) == 3 && attperc > minimum) {
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
    dmg = damtype(ch, 14, skill, attperc);
    dmg /= 10;
    act("@WYou fire a masenko at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a masenko at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_kamehameha)
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

  if (!check_skill(ch, SKILL_KAMEHAMEHA)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
    minimum -= 0.05;
    if (minimum <= 0.0) {
      minimum = 0.01;
    }
  }

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (GET_MAX_MANA(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_KAMEHAMEHA); /* Set skill value */

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
    improve_skill(ch, SKILL_KAMEHAMEHA, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "KameHameHa Wave")) {
      if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);

      if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
        incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
      } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
        incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
      } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
        incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
      }
      if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
        WAIT_STATE(ch, PULSE_3SEC);
      }
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your kamehameha!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W kamehameha!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W kamehameha!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
          } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
          } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
          }
          dmg = damtype(ch, 13, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your kamehameha, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W kamehameha, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W kamehameha, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 13, skill, SKILL_KAMEHAMEHA); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
          } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
          } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
          }
          improve_skill(vict, SKILL_DODGE, 0);
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your kamehameha misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a kamehameha at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a kamehameha at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
          } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
          } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
            incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
          }
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your kamehameha misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a kamehameha at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a kamehameha at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
        if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
          incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
        } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
          incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
        } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
          incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
        }
      }
      if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
        WAIT_STATE(ch, PULSE_3SEC);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 13, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S body and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your body and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S body and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S face and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your face and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S face and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S gut and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your gut and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S gut and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          break;
        case 4: /* Weak */
          act("@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S arm and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your arm and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S arm and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S leg and explodes!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your leg and explodes!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S leg and explodes!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
            WAIT_STATE(ch, PULSE_3SEC);
          }
          break;
      }
      if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
        incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
      } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
        incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
      } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
        incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
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
    dmg = damtype(ch, 13, skill, attperc);
    dmg /= 10;
    act("@WYou fire a kamehameha at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a kamehameha at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    if (char_get_skills_perf(ch, SKILL_KAMEHAMEHA) == 3) {
      WAIT_STATE(ch, PULSE_3SEC);
    }
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);
    if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 100) {
      incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.25, true);
    } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 60) {
      incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.1, true);
    } else if (char_get_skill(ch, SKILL_KAMEHAMEHA) >= 40) {
      incCurKI(ch, (GET_MAX_MANA(ch) * attperc) * 0.05, true);
    }
  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_beam)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0;
  int64_t dmg;
  double attperc = .1, minimum = .01;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_BEAM)) {
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

  if (GET_MAX_MANA(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(GET_MAX_MANA(ch));
  }

  if (!check_points(ch, GET_MAX_MANA(ch) * minimum, 0)) {
    return;
  }

  skill = init_skill(ch, SKILL_BEAM);

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  handle_cooldown(ch, 5);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 1)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_BEAM, 0);
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, TRUE);
    perc  = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "beam")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, GET_MAX_HIT(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W deflects your beam, sending it flying away!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou deflect @C$n's@W beam sending it flying away!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W deflects @c$n's@W beam sending it flying away!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(vict, 0, GET_MAX_HIT(vict) / 200);

          parry_ki(attperc, ch, vict, "beam", prob, perc, skill, 10);
          /*      User/target/skill name/skill/hurt type */
          pcost(ch, attperc, 0);

          if (rroom_get_dmg(char_get_in_room(ch)) <= 95) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (5));
          }

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks beam!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W beam!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W beam!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, attperc, 0);
          pcost(vict, 0, GET_MAX_HIT(vict) / 500);
          dmg = damtype(ch, 10, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your beam, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W beam, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W beam, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);

          dodge_ki(ch, vict, 0, 10, skill, SKILL_BEAM); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your beam misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a bright yellow beam at you, but misses!@n ", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a bright yellow beam at @C$N@W, but somehow misses!@n ", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your beam misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a bright yellow beam at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a bright yellow beam at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 10, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      pcost(ch, attperc, 0);

      int master_roll = rand_number(1, 100), master_chance = 0, master_pass = FALSE;

      if (skill >= 100)
        master_chance = 20;
      else if (skill >= 75)
        master_chance = 10;
      else if (skill >= 50)
        master_chance = 5;

      if (master_chance >= master_roll)
        master_pass = TRUE;

      if (GET_HIT(vict) > 0 && dmg > GET_MAX_HIT(vict) / 4 && master_pass == TRUE) {
        int attempt = rand_number(0, NUM_OF_DIRS); /* Select a random direction */
        int count   = 0;
        while (count < 12) {
          attempt = count;
          if (char_can_go(vict, attempt)) {
            count = 12;
          } else {
            count++;
          }
        }
        if (char_can_go(vict, attempt)) {
          act("$N@W is pushed away by the blast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou are pushed away by the blast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("$N@W is pushed away by the blast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          do_simple_move(vict, attempt, TRUE);
        } else {
          act("$N@W is pushed away by the blast, but is slammed into an obstruction!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou are pushed away by the blast, but are slammed into an obstruction!@n", TRUE, ch, 0, vict, TO_VICT);
          act("$N@W is pushed away by the blast, but is slammed into an obstruction!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= 2;
          hurt(1, 195, ch, vict, NULL, dmg, 1);
        }
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
    dmg = damtype(ch, 10, skill, attperc);
    dmg /= 10;
    act("@WYou fire a beam at $p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a beam at $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
