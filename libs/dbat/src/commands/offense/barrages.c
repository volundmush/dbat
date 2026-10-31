/* barrages.c: multi-hit ki barrages
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

ACMD(do_pbarrage)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .2, minimum = .15;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_PBARRAGE)) {
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

  skill = init_skill(ch, SKILL_PBARRAGE); /* Set skill value */

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
    improve_skill(ch, SKILL_PBARRAGE, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Psychic Barrage")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your psychic barrage!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W psychic barrage!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W psychic barrage!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 31, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your psychic barrage, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W psychic barrage, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W psychic barrage, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 31, skill, SKILL_PBARRAGE); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your psychic barrage misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a psychic barrage at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a psychic barrage at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your psychic barrage misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a psychic barrage at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a psychic barrage at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 31, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (!char_aff_flagged(vict, AFF_MBREAK) && rand_number(1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
        act("@mYour mind's eye has been shattered, you can't charge ki until you recover!@n", TRUE, vict, 0, 0, TO_CHAR);
        act("@M$n@m's mind has been damaged by the attack!@n", TRUE, vict, 0, 0, TO_ROOM);
        char_aff_set(vict, AFF_MBREAK, true);
      } else if (!char_aff_flagged(vict, AFF_SHOCKED) && rand_number(1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
        act("@MYour mind has been shocked!@n", TRUE, vict, 0, 0, TO_CHAR);
        act("@M$n@m's mind has been shocked!@n", TRUE, vict, 0, 0, TO_ROOM);
        char_aff_set(vict, AFF_SHOCKED, true);
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
    dmg = damtype(ch, 31, skill, attperc);
    dmg /= 10;
    act("@WYou fire a psychic barrage at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a psychic barrage at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_scatter)
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

  if (!check_skill(ch, SKILL_SCATTER)) {
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

  skill = init_skill(ch, SKILL_SCATTER); /* Set skill value */

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
    improve_skill(ch, SKILL_SCATTER, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    prob += rand_number(10, 20);

    if (!tech_handle_zanzoken(ch, vict, "Scatter Shot")) {
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
          act("@C$N@W manages to dodge your scatter shot kiballs, letting them slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W scatter shot kiballs, letting them slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W scatter shot kiballs, letting them slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wBright explosions erupts from the impacts!\r\n");

          dodge_ki(ch, vict, 0, 35, skill, SKILL_SCATTER); /* Effects on the room from dodging a ki attack
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
        act("@WYou can't believe it but all the kiballs of your scatter shot miss, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires scatter shot kiballs at you, but they miss!@n ", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires scatter shot kiballs at @C$N@W, but somehow they miss!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
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
          act("@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's head!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR head!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's head!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
    act("@WYou fire a scatter shot at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a scatter shot at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
/* Rillao: End Balefire */

ACMD(do_kakusanha)
{
  int perc, dge = 2, count = 0, skill;
  int64_t dmg;
  double attperc = .3, minimum = .10;
  struct char_data *vict = NULL, *next_v = NULL;
  char arg2[MAX_INPUT_LENGTH];

  one_argument(argument, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_KAKUSANHA)) {
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

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_KAKUSANHA); /* Set skill value */

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (getCurPL(vict) <= 0) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_GROUP) && !char_is_npc(vict)) {
      if (vict->master == ch) {
        continue;
      } else if (ch->master == vict) {
        continue;
      } else if (vict->master == ch->master) {
        continue;
      }
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
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
    handle_cooldown(ch, 5);
    int hits = 0;
    perc     = chance_to_hit(ch);

    if (skill < perc) {
      act("@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. The beam flies off harmlessly through the air as you lose control over it!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. The beam flies off harmlessly through the air as $e loses control over it!@n", TRUE, ch, 0, 0, TO_ROOM);
      pcost(ch, attperc, 0);

      improve_skill(ch, SKILL_KAKUSANHA, 0);
      return;
    }

    act("@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. Above your targets the beam breaks apart into five seperate pieces that follow their victims!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. Above you the beam breaks apart into five seperate pieces that follow their victims!@n", TRUE, ch, 0, 0, TO_ROOM);

    dmg = damtype(ch, 34, skill, attperc);
    if (count >= 3) {
      dmg = (dmg / 100) * 40;
    } else if (count > 1) {
      dmg = (dmg / 100) * 60;
    }

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
      if (char_mob_flagged(vict, MOB_NOKILL)) {
        continue;
      }
      if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
        continue;
      }
      if (getCurPL(vict) <= 0) {
        continue;
      }
      if (hits >= 5) {
        continue;
      }
      dge = handle_dodge(vict);
      if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        hits++;
        act("@C$N@c disappears, avoiding the beam chasing $M!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@cYou disappear, avoiding the beam chasing you!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@C$N@c disappears, avoiding the beam chasing $M!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        pcost(vict, 0, getEffMaxPL(vict) / 200);
        hurt(0, 0, ch, vict, NULL, 0, 1);
        continue;
      } else if (dge + rand_number(-10, 5) > skill) {
        hits++;
        act("@c$N@W manages to escape the attack!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@WYou manage to escape the attack!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N@W manages to escape the attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        improve_skill(vict, SKILL_DODGE, 0);
        hurt(0, 0, ch, vict, NULL, 0, 1);
        continue;
      } else {
        hits++;
        act("@R$N@r is slammed by one of the beams!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@RYou are slammed by one of the beams!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r is slammed by one of the beams!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
        continue;
      }
    } /* Hitting targets! */

    if (count < 5 && !rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) {
      send_to_room(char_get_in_room(ch), "The rest of the beams slam into the ground!@n\r\n");
      send_to_room(char_get_in_room(ch), "@wBright explosions erupt from the impacts!\r\n");

      if (rroom_sect(char_get_in_room(ch)) != SECT_INSIDE) {
        impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
        switch (rand_number(1, 8)) {
          case 1:
            act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, 0, TO_CHAR);
            act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 2:
            if (rand_number(1, 4) == 4 && rroom_get_geffect(char_get_in_room(ch)) == 0) {
              rroom_set_geffect(char_get_in_room(ch), 5);
              act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", TRUE, ch, 0, 0, TO_CHAR);
              act("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", TRUE, ch, 0, 0, TO_ROOM);
            }
            break;
          case 3:
            act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, 0, TO_CHAR);
            act("A cloud of dust envelopes the entire area!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 4:
            act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, 0, TO_CHAR);
            act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 5:
            act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, 0, TO_CHAR);
            act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 6:
            act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.", TRUE, ch, 0, 0, TO_CHAR);
            act("The explosion continues to burn spreading out and devouring some more of the ground before dying out.", TRUE, ch, 0, 0, TO_ROOM);
            break;
          default:
            /* we want no message for the default */
            break;
        }
      }
      if (rroom_sect(char_get_in_room(ch)) == SECT_UNDERWATER) {
        switch (rand_number(1, 3)) {
          case 1:
            act("The water churns violently!", TRUE, ch, 0, 0, TO_CHAR);
            act("The water churns violently!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 2:
            act("Large bubbles rise from the movement!", TRUE, ch, 0, 0, TO_CHAR);
            act("Large bubbles rise from the movement!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 3:
            act("The water collapses in on the hole created!", TRUE, ch, 0, 0, TO_CHAR);
            act("The water collapses in on the hole create!", TRUE, ch, 0, 0, TO_ROOM);
            break;
        }
      }
      if (rroom_sect(char_get_in_room(ch)) == SECT_WATER_SWIM || rroom_sect(char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
        switch (rand_number(1, 3)) {
          case 1:
            act("A huge column of water erupts from the impact!", TRUE, ch, 0, 0, TO_CHAR);
            act("A huge column of water erupts from the impact!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 2:
            act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, 0, TO_CHAR);
            act("The impact briefly causes a swirling vortex of water!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 3:
            act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, 0, TO_CHAR);
            act("A huge depression forms in the water and erupts into a wave from the impact!", TRUE, ch, 0, 0, TO_ROOM);
            break;
        }
      }
      if (rroom_sect(char_get_in_room(ch)) == SECT_INSIDE) {
        impact_sound(ch, "@wA loud roar is heard nearby!@n\r\n");
        switch (rand_number(1, 8)) {
          case 1:
            act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, 0, TO_CHAR);
            act("Debris is thrown into the air and showers down thunderously!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 2:
            act("The structure of the surrounding room cracks and quakes from the blast!", TRUE, ch, 0, 0, TO_CHAR);
            act("The structure of the surrounding room cracks and quakes from the blast!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 3:
            act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, 0, TO_CHAR);
            act("Parts of the ceiling collapse, crushing into the floor!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 4:
            act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, 0, TO_CHAR);
            act("The surrounding area roars and shudders from the impact!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 5:
            act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, 0, TO_CHAR);
            act("The ground shatters apart from the stress of the impact!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          case 6:
            act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, 0, TO_CHAR);
            act("The walls of the surrounding room crack in the same instant!", TRUE, ch, 0, 0, TO_ROOM);
            break;
          default:
            /* we want no message for the default */
            break;
        }
      }

      if (rroom_get_dmg(char_get_in_room(ch)) <= (100 - ((5 - count) * 5))) {
        rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + ((5 - count) * 5));
      }
    }

    pcost(ch, attperc, 0);

    improve_skill(ch, SKILL_KAKUSANHA, 0);
    handle_cooldown(ch, 5);
    return;
  } /* We have targets! Attempt to kills them! */
}

ACMD(do_renzo)
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

  if (!check_skill(ch, SKILL_RENZO)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
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

  skill = init_skill(ch, SKILL_RENZO); /* Set skill value */

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
    improve_skill(ch, SKILL_RENZO, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    /* Renzokou Energy Dan Mastery */

    int master_roll = rand_number(1, 100), master_chance = 0, half_chance = 0, master_pass = 0;

    if (skill >= 100) {
      master_chance = 10;
      half_chance   = 20;
    } else if (skill >= 75) {
      master_chance = 5;
      half_chance   = 10;
    } else if (skill >= 50) {
      master_chance = 5;
      half_chance   = 5;
    }

    if (master_chance >= master_roll)
      master_pass = 1;
    else if (half_chance >= master_roll)
      master_pass = 2;

    if (master_pass == 1)
      send_to_char(ch, "@GYour mastery of the technique has made your use of energy more efficient!@n\r\n");
    else if (master_pass == 2)
      send_to_char(ch, "@GYour mastery of the technique has made your use of energy as efficient as possible!@n\r\n");

    /*=============================*/

    if (!tech_handle_zanzoken(ch, vict, "Renzokou Energy Dan")) {
      if (master_pass == 1)
        pcost(ch, attperc * 0.25, 0);
      else if (master_pass == 2)
        pcost(ch, attperc * 0.5, 0);
      else
        pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      return;
    }

    int count = 0;
    if (prob + 20 < perc) {
      count = 0;
    } else if (prob + 15 < perc) {
      count = 10;
    } else if (prob + 10 < perc) {
      count = 25;
    } else if (prob + 5 < perc) {
      count = 50;
    } else if (prob < perc - 20) {
      count = 75;
    } else if (prob > perc) {
      count = 100;
    }
    if (count > 0) {
      if (char_is_nail(ch)) {
        if (char_get_skill(ch, SKILL_RENZO) >= 100) {
          count += 200;
        } else if (char_get_skill(ch, SKILL_RENZO) >= 60) {
          count += 100;
        } else if (char_get_skill(ch, SKILL_RENZO) >= 40) {
          count += 40;
        }
      }
      if (rand_number(1, 5) >= 5) { /* Random boost or neg for everyone */
        count += rand_number(-15, 25);
      }
    }
    if (count == 0) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W deflects every shot of your renzokou energy dan, sending them flying away!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou deflect every shot @C$n's@W renzokou energy dan sending them flying away!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W deflects every shot of @c$n's@W renzokou energy dan sending them flying away!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (master_pass == 1)
            pcost(ch, attperc * 0.25, 0);
          else if (master_pass == 2)
            pcost(ch, attperc * 0.5, 0);
          else
            pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 200);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks every renzokou energy dan shot with $S arms!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W renzokou energy dan!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W renzokou energy dan with $S arms!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (master_pass == 1)
            pcost(ch, attperc * 0.25, 0);
          else if (master_pass == 2)
            pcost(ch, attperc * 0.5, 0);
          else
            pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 12, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge all your renzokou energy dan shots, letting them slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge all of @C$n's @Wrenzokou energy dan shots, letting them slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge all @c$n's@W renzokou energy dan shots, letting them slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (master_pass == 1)
            pcost(ch, attperc * 0.25, 0);
          else if (master_pass == 2)
            pcost(ch, attperc * 0.5, 0);
          else
            pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but all your renzokou energy dan shots miss, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires hundreds of renzokou energy dan shots at you, but misses!@n ", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires hundreds of renzokou energy dan shots at @C$N@W, but somehow misses!@n ", TRUE, ch, 0, vict, TO_NOTVICT);
          if (master_pass == 1)
            pcost(ch, attperc * 0.25, 0);
          else if (master_pass == 2)
            pcost(ch, attperc * 0.5, 0);
          else
            pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but all your renzokou energy dan shots miss, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires hundreds of renzokou energy dan shots at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires hundreds of renzokou energy dan shots at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        if (master_pass == 1)
          pcost(ch, attperc * 0.25, 0);
        else if (master_pass == 2)
          pcost(ch, attperc * 0.5, 0);
        else
          pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg = damtype(ch, 12, skill, attperc);
      if (count >= 100) {
        dmg = dmg * 0.01;
        dmg *= count;
        act("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! All of the shots hit!", TRUE, ch, 0, vict, TO_CHAR);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! All of the shots hit!", TRUE, ch, 0, vict, TO_VICT);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! All of the shots hit!", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
      }
      if (count >= 75 && count < 100) {
        dmg = dmg * 0.01;
        dmg *= count;
        act("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Most of the shots hit, but some of them are avoided by $M!", TRUE, ch, 0, vict, TO_CHAR);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Most of the shots hit, but some of them you manage to avoid!", TRUE, ch, 0, vict, TO_VICT);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Most of the shots hit, but some of them are avoided by $M!", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
      }
      if (count >= 50 && count < 75) {
        dmg = dmg * 0.01;
        dmg *= count;
        act("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! About half of the shots hit, the rest are avoided by $M!", TRUE, ch, 0, vict, TO_CHAR);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! About half of the shots hit, the rest you manage to avoid!", TRUE, ch, 0, vict, TO_VICT);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! About half of the shots hit, the rest are avoided by $M!", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
      }
      if (count >= 25 && count < 50) {
        dmg = dmg * 0.01;
        dmg *= count;
        act("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Few of the shots hit, the rest are avoided by $M!", TRUE, ch, 0, vict, TO_CHAR);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Few of the shots hit, the rest you manage to avoid!", TRUE, ch, 0, vict, TO_VICT);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Few of the shots hit, the rest are avoided by $M!", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
      }
      if (count >= 10 && count < 25) {
        dmg /= 100;
        dmg *= count;
        act("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Very few of the shots hit, the rest are avoided by $M!", TRUE, ch, 0, vict, TO_CHAR);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Very few of the shots hit, the rest you manage to avoid!", TRUE, ch, 0, vict, TO_VICT);
        act("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Very few of the shots hit, the rest are avoided by $M!", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, dmg, 1);
      }
      if (master_pass == 1)
        pcost(ch, attperc * 0.25, 0);
      else if (master_pass == 2)
        pcost(ch, attperc * 0.5, 0);
      else
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
    dmg = damtype(ch, 12, skill, attperc);
    dmg /= 10;
    act("@WYou fire hundreds of renzokou energy dan shots at $p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires hundreds of renzokou energy dan shots at $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_tsuihidan)
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

  if (!check_skill(ch, SKILL_TSUIHIDAN)) {
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

  skill = init_skill(ch, SKILL_TSUIHIDAN); /* Set skill value */

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
    improve_skill(ch, SKILL_TSUIHIDAN, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    /* Tsuihidan Mastery */

    int master_roll = rand_number(1, 100), master_chance = 0, master_pass = FALSE;

    if (skill >= 100)
      master_chance = 20;
    else if (skill >= 75)
      master_chance = 10;
    else if (skill >= 50)
      master_chance = 5;

    if (master_chance >= master_roll)
      master_pass = TRUE;

    /*===================*/

    if (!tech_handle_zanzoken(ch, vict, "Tsuihidan")) {
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      dodge_ki(ch, vict, 1, 11, skill, SKILL_TSUIHIDAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/
      hurt(0, 0, ch, vict, NULL, 0, 1);
      pcost(ch, attperc, 0);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (tech_handle_android_absorb(ch, vict)) {
          pcost(ch, 1, 0);
          return;
        } else if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W deflects your tsuihidan, sending it flying away!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou deflect @C$n's@W tsuihidan sending it flying away!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W deflects @c$n's@W tsuihidan sending it flying away!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(vict, 0, getEffMaxPL(vict) / 200);

          parry_ki(attperc, ch, vict, "tsuihidan", prob, perc, skill, 11);
          /*      User/target/skill name/skill/hurt type */
          pcost(ch, attperc, 0);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks tsuihidan!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W tsuihidan!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W tsuihidan!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 11, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your tsuihidan, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W tsuihidan, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W tsuihidan, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your tsuihidan misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a tsuihidan at you, but misses!@n ", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a tsuihidan at @C$N@W, but somehow misses!@n ", TRUE, ch, 0, vict, TO_NOTVICT);

          dodge_ki(ch, vict, 1, 11, skill, SKILL_TSUIHIDAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/
          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your tsuihidan misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a tsuihidan at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a tsuihidan at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        dodge_ki(ch, vict, 1, 11, skill, SKILL_TSUIHIDAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

        hurt(0, 0, ch, vict, NULL, 0, 1);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 11, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 195, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 195, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (master_pass == TRUE) {
        decCurST(vict, dmg, 0);
        act("@CYour tsuihidan hits a vital spot and seems to sap some of @c$N's@C stamina!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n's@C tsuihidan hits a vital spot and saps some of your stamina!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@C$n's@C tsuihidan hits a vital spot and saps some of @c$N's@C stamina!", TRUE, ch, 0, vict, TO_NOTVICT);
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
    dmg = damtype(ch, 11, skill, attperc);
    dmg /= 10;
    act("@WYou fire a tsuihidan at $p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a tsuihidan at $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
