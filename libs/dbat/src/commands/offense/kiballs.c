/* kiballs.c: ki ball and sphere attacks
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

ACMD(do_geno)
{

  int perc, prob;
  double attperc = 0.5, minimum = .4;
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);
  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_GENOCIDE)) {
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

  vict = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(ch);
    } else {
      send_to_char(ch, "No one around here by that name.\r\n");
      return;
    }
  }

  if (!can_kill(ch, vict, NULL, 3)) {
    return;
  }

  if (handle_defender(vict, ch)) {
    struct char_data *def = char_get_defender(vict);
    vict                  = def;
  }

  prob = init_skill(ch, SKILL_GENOCIDE); /* Set skill value */
  perc = rand_number(1, 115);

  if (prob < perc - 20) {
    act("@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You lose concentration and the ball of energy dissipates!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W loses concentration and the ball of energy dissipates!@n", TRUE, ch, 0, 0, TO_ROOM);
    hurt(0, 0, ch, vict, NULL, 0, 1);
    pcost(ch, attperc, 0);

    improve_skill(ch, SKILL_GENOCIDE, 2);
    return;
  }

  struct obj_data *obj;
  int dista = 15 - (char_get_aff_abils_intel(ch) * 0.1);

  if (char_get_skill(ch, SKILL_GENOCIDE) >= 100) {
    dista -= 3;
  } else if (char_get_skill(ch, SKILL_GENOCIDE) >= 60) {
    dista -= 2;
  } else if (char_get_skill(ch, SKILL_GENOCIDE) >= 40) {
    dista -= 1;
  }

  obj = read_object(83, VIRTUAL);
  obj_to_room(obj, char_get_in_room(vict));

  char_set_charge(ch, char_get_charge(ch) + (getEffMaxPL(ch) / 10));
  obj_set_target(obj, vict);
  obj_set_kicharge(obj, damtype(ch, 41, prob, attperc));
  obj_set_kitype(obj, SKILL_GENOCIDE);
  obj_set_user(obj, ch);
  obj_set_distance(obj, dista);
  pcost(ch, attperc, 0);
  act("@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You grin viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and you toss it at @c$N@W!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at YOU!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);

  improve_skill(ch, SKILL_GENOCIDE, 2);
}

ACMD(do_genki)
{

  int perc, prob;
  double attperc = .5, minimum = .4;
  struct char_data *friend_char = NULL, *vict = NULL, *next_v = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);
  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_GENKIDAMA)) {
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

  vict = NULL;
  if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(ch);
    } else {
      send_to_char(ch, "No one around here by that name.\r\n");
      return;
    }
  }

  if (!can_kill(ch, vict, NULL, 3)) {
    return;
  }

  if (handle_defender(vict, ch)) {
    struct char_data *def = char_get_defender(vict);
    vict                  = def;
  }

  prob = init_skill(ch, SKILL_GENKIDAMA); /* Set skill value */
  perc = rand_number(1, 115);

  if (prob < perc - 20) {
    act("@WYou raise both your arms upwards and begin to pool your charged ki there. You also start calling on the ki of all living beings in the vicinity who are willing to help. Your concentration wavers though and you waste the energy you have!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. @C$n@W's concentration wavers though and $e wastes the energy $e has!@n", TRUE, ch, 0, 0, TO_ROOM);
    hurt(0, 0, ch, vict, NULL, 0, 1);
    pcost(ch, attperc, 0);
    handle_cooldown(ch, 10);
    improve_skill(ch, SKILL_GENKIDAMA, 2);
    return;
  }

  for (friend_char = world[char_get_in_room(ch)].people; friend_char; friend_char = next_v) {
    next_v = friend_char->next_in_room;
    if (friend_char == ch) {
      continue;
    }
    if (char_aff_flagged(friend_char, AFF_GROUP) && (friend_char->master == ch || ch->master == friend_char || friend_char->master == ch->master)) {
      char_set_charge(ch, char_get_charge(ch) + ((getCurKI(ch)) / 10));
      decCurKI(ch, getCurKI(ch) / 20, 0);
    }
  }

  int dista = 15 - (char_get_aff_abils_intel(ch) * 0.1);

  if (char_get_skill(ch, SKILL_GENKIDAMA) >= 100) {
    dista -= 3;
  } else if (char_get_skill(ch, SKILL_GENKIDAMA) >= 60) {
    dista -= 2;
  } else if (char_get_skill(ch, SKILL_GENKIDAMA) >= 40) {
    dista -= 1;
  }

  struct obj_data *obj;

  obj = read_object(82, VIRTUAL);
  obj_to_room(obj, char_get_in_room(vict));

  obj_set_target(obj, vict);
  obj_set_kicharge(obj, damtype(ch, 40, prob, attperc));
  obj_set_kitype(obj, SKILL_GENKIDAMA);
  obj_set_user(obj, ch);
  obj_set_distance(obj, dista);
  pcost(ch, attperc, 0);
  act("@WYou raise both your arms upwards and begin to pool your charged ki there. You also start calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above your hands, when it is finished you lob it toward @c$N@W!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above $s hands, when it is finished $e lobs it toward YOU!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above $s hands, when it is finished $e lobs it toward @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  handle_cooldown(ch, 10);
  improve_skill(ch, SKILL_GENKIDAMA, 2);
}

ACMD(do_spiritball)
{
  int prob, perc, avo, index, pry = 2, dge = 2, blk = 2, skill;
  int64_t dmg;
  double attperc = .25, minimum = .20;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_SPIRITBALL)) {
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

  skill = init_skill(ch, SKILL_SPIRITBALL); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 8);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 3)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }

    improve_skill(ch, SKILL_SPIRITBALL, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Spirit Ball")) {
      pcost(ch, attperc, 0);
      dodge_ki(ch, vict, 2, 39, skill, SKILL_SPIRITBALL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/
      hurt(0, 0, ch, vict, NULL, 0, 1);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W deflects your Spirit Ball, sending it flying away!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou deflect @C$n's@W Spirit Ball sending it flying away!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W deflects @c$n's@W Spirit Ball sending it flying away!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(vict, 0, getEffMaxPL(vict) / 200);

          parry_ki(attperc, ch, vict, "spiritball", prob, perc, skill, 39);
          /*      User/target/skill name/skill/hurt type */
          pcost(ch, attperc, 0);

          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks the Spirit Ball!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W Spirit Ball!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W Spirit Ball!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 39, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your Spirit Ball, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W Spirit Ball, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W Spirit Ball, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your spirit ball misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a spirit ball at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a spirit ball at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          dodge_ki(ch, vict, 2, 39, skill, SKILL_SPIRITBALL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your spirit ball misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a spirit ball at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a spirit ball at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        dodge_ki(ch, vict, 2, 39, skill, SKILL_SPIRITBALL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/
        pcost(ch, attperc, 0);
        hurt(0, 0, ch, vict, NULL, 0, 1);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 39, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S body, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR body, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's body, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S head, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR head, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's head, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S gut, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR gut, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's gut, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S arm, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR arm, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's arm, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S leg, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR leg, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's leg, and explodes with a load roar!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
    dmg = damtype(ch, 39, skill, attperc);
    dmg /= 10;
    act("@WYou fire a spirit ball at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a spirit ball at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_deathball)
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

  if (!check_skill(ch, SKILL_DEATHBALL)) {
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

  skill = init_skill(ch, SKILL_DEATHBALL); /* Set skill value */

  if (!tech_handle_targeting(ch, arg, &vict, &obj)) return;

  /* There is a player/mob targeted */
  handle_cooldown(ch, 8);
  if (vict) {
    if (!can_kill(ch, vict, NULL, 3)) {
      return;
    }
    if (handle_defender(vict, ch)) {
      struct char_data *def = char_get_defender(vict);
      vict                  = def;
    }
    improve_skill(ch, SKILL_DEATHBALL, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);
    prob -= rand_number(8, 10);

    if (!tech_handle_zanzoken(ch, vict, "Deathball")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your deathball, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W deathball, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W deathball, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 38, skill, SKILL_DEATHBALL); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your deathball misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a deathball at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a deathball at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your deathball misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a deathball at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a deathball at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 38, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S body before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR body before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S body before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S head before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR head before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S head before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S gut before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR gut before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S gut before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S arm before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR arm before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S arm before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 170, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S leg before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR leg before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S leg before exploding into a massive blast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
    dmg = damtype(ch, 38, skill, attperc);
    dmg /= 10;
    act("@WYou fire a deathball at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a deathball at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_hellspear)
{
  int perc, dge = 2, count = 0, skill;
  int64_t dmg;
  double attperc = .3, minimum = .1;
  struct char_data *vict = NULL, *next_v = NULL;
  char arg2[MAX_INPUT_LENGTH];

  one_argument(argument, arg2);

  /* Can they do the technique? */

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_HELLSPEAR)) {
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

  if (rroom_flagged(char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(ch, SKILL_HELLSPEAR); /* Set skill value */

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
      continue;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
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
    perc = chance_to_hit(ch);

    if (skill < perc) {
      act("@WYou fly up higher in the air while holding your hand up above your head. Your charged ki is condensed and materialized in the grasp of your raised hand forming a spear of energy. The spear disolves as you screw up the technique!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W flies up higher in the air while holding $s hand up above $s head. @C$n@W's charged ki is condensed and materialized in the grasp of $s raised hand forming a spear of energy. The spear disolves as $e screws up the technique!@n", TRUE, ch, 0, 0, TO_ROOM);
      pcost(ch, attperc, 0);

      improve_skill(ch, SKILL_HELLSPEAR, 0);
      return;
    }

    act("@WYou fly up higher in the air while holding your hand above your head. Your charged ki is condensed and materialized in the grasp of your raised hand forming a spear of energy. Grinning evily you aim the spear at the ground below and throw it. As the red spear of energy slams into the ground your laughter rings throughout the area, and the @rH@Re@Dl@Wl @rS@Rp@De@Wa@wr B@rl@Ra@Ds@wt@W erupts with a roar!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W flies up higher in the air while holding $s hand above $s head. @C$n@W's charged ki is condensed and materialized in the grasp of $s raised hand forming a spear of energy. Grinning evily $e aims the spear at the ground below and throws it. As the red spear of energy slams into the ground $s laughter rings throughout the area, and the @rH@Re@Dl@Wl @rS@Rp@De@Wa@wr B@rl@Ra@Ds@wt@W erupts with a roar!@n", TRUE, ch, 0, 0, TO_ROOM);

    dmg = damtype(ch, 33, skill, attperc);

    for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (vict == ch) {
        continue;
      }
      if (char_mob_flagged(vict, MOB_NOKILL)) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
        continue;
      }
      if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
        continue;
      }
      dge = handle_dodge(vict);

      if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        pcost(vict, 0, getEffMaxPL(vict) / 200);
        continue;
      } else if (dge + rand_number(-10, 5) > skill) {
        act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@WYou manage to escape the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$N@W manages to escape the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        hurt(0, 0, ch, vict, NULL, 0, 1);
        improve_skill(vict, SKILL_DODGE, 0);
        continue;
      } else {
        act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@RYou are caught by the explosion!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@R$N@r is caught by the explosion!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(1, 4) == 4) {
          handle_knockdown(vict);
        }
        hurt(0, 0, ch, vict, NULL, dmg, 1);
        continue;
      }
    } /* Hitting targets! */

    pcost(ch, attperc, 0);

    improve_skill(ch, SKILL_HELLSPEAR, 0);
    handle_cooldown(ch, 5);
    return;
  } /* We have targets! Attempt to kills them! */
}

ACMD(do_crusher)
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

  if (!check_skill(ch, SKILL_CRUSHER)) {
    return;
  }

  if (!limb_ok(ch, 0)) {
    return;
  }

  if (!*arg && !char_get_fighting(ch)) {
    send_to_char(ch, "Direct it at who?\r\n");
    return;
  }

  if (char_get_skills_perf(ch, SKILL_CRUSHER) == 1) {
    attperc += 0.05;
  } else if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3) {
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

  skill = init_skill(ch, SKILL_CRUSHER); /* Set skill value */

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
    improve_skill(ch, SKILL_CRUSHER, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    if (char_get_skills_perf(ch, SKILL_CRUSHER) == 2) {
      prob += 5;
    }
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "Crusher Ball")) {
      if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3 && attperc > minimum) {
        pcost(ch, attperc - 0.05, 0);
      } else {
        pcost(ch, attperc, 0);
      }
      pcost(vict, 0, getEffMaxPL(vict) / 200);

      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your crusher ball!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W crusher ball!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W crusher ball!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 29, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your crusher ball, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W crusher ball, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W crusher ball, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 29, skill, SKILL_CRUSHER); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 90) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (10));
          }
          improve_skill(vict, SKILL_DODGE, 0);
          if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your crusher ball misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a crusher ball at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a crusher ball at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);
          if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3 && attperc > minimum) {
            pcost(ch, attperc - 0.05, 0);
          } else {
            pcost(ch, attperc, 0);
          }
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your crusher ball misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a crusher ball at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a crusher ball at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
        if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3 && attperc > minimum) {
          pcost(ch, attperc - 0.05, 0);
        } else {
          pcost(ch, attperc, 0);
        }
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      dmg         = damtype(ch, 29, skill, attperc);
      int hitspot = 1;
      hitspot     = roll_hitloc(ch, vict, skill);
      switch (hitspot) {
        case 1:
          act("@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S body!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR body!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's body!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 2: /* Critical */
          act("@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 0);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(1, 180, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 2);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
      }
      if (rand_number(1, 3) == 3) {
        tech_handle_crashdown(ch, vict);
      }

      if (char_get_skills_perf(ch, SKILL_CRUSHER) == 3 && attperc > minimum) {
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
    dmg = damtype(ch, 29, skill, attperc);
    dmg /= 10;
    act("@WYou fire a crusher ball at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a crusher ball at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

/* Chimera */
ACMD(do_blessedhammer)
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

  if (!check_skill(ch, SKILL_BLESSEDHAMMER)) {
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

  skill = init_skill(ch, SKILL_BLESSEDHAMMER); /* Set skill value */

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
    improve_skill(ch, SKILL_BLESSEDHAMMER, 0);

    index = check_def(vict); /* Check parry/block/dodge of vict */

    prob = roll_accuracy(ch, skill, TRUE);
    perc = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    prob += 15; /* kousengan bonus */

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "@WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@C")) {
      pcost(ch, attperc, 0);
      pcost(vict, 0, getEffMaxPL(vict) / 200);
      return;
    }

    if (prob < perc - 20) {
      if ((getCurST(vict)) > 0) {
        if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 42, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n", FALSE, ch, 0, vict, TO_NOTVICT);
          send_to_room(char_get_in_room(vict), "@wA bright explosion erupts from the impact!\r\n");

          dodge_ki(ch, vict, 0, 17, skill, SKILL_BLESSEDHAMMER); /* Effects on the room from dodging a ki attack
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
          act("@WYou can't believe it but your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at you, but misses!@n ", FALSE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at @C$N@W, but somehow misses!@n ", FALSE, ch, 0, vict, TO_NOTVICT);

          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W misses, flying through the air harmlessly!@n", FALSE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at you, but misses!@n", FALSE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at @C$N@W, but somehow misses!@n", FALSE, ch, 0, vict, TO_NOTVICT);
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
          act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their chest!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR chest!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's chest!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
          act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their face!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR face!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's face!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 3);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 3:
          act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their gut!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR gut!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's gut!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          if (char_get_bonuses(ch, BONUS_SOFT)) {
            dmg *= calc_critical(ch, 2);
          }
          hurt(0, 0, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 4);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 4: /* Weak */
          act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their arm!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR arm!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's arm!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          dmg *= calc_critical(ch, 1);
          hurt(0, 190, ch, vict, NULL, dmg, 1);
          dam_eq_loc(vict, 1);
          /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
          break;
        case 5: /* Weak 2 */
          act("@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their leg!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR leg!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's leg!@n", TRUE, ch, 0, vict, TO_NOTVICT);
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
    act("@WYou fire a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr at $p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr at $p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}

ACMD(do_kiball)
{
  int prob, perc, avo, index = 0, pry = 2, dge = 2, blk = 2, skill = 0, frompool = FALSE;
  int64_t dmg;
  double attperc = .05, minimum = .01;
  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!can_grav(ch)) {
    return;
  }

  if (!check_skill(ch, SKILL_KIBALL)) {
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
  /*Rillao: Trying to make conc/focus no-charge for t1 ki */
  /* if ((GET_SKILL (ch, SKILL_CONCENTRATION) == 100) && (GET_SKILL (ch, SKILL_FOCUS) == 100)) {
     minimum -=0.01;
     if (GET_CHARGE(ch) <= 100) {
      GET_CHARGE(ch) = GET_MAX_MANA(ch) * 0.01;
      attperc = 0.01;      
      if (frompool == TRUE) {
       GET_MANA(ch) -= GET_MAX_MANA(ch) * attperc;
       } else {
       pcost(ch, 0, GET_MAX_MANA(ch) * 0.01);
       }
     } */
  // log("Log 1 - attperc: %f, minimum: %f, charge: %" I64T "", attperc, minimum, GET_CHARGE(ch));
  //}

  if (!tech_handle_charge(ch, arg2, minimum, &attperc)) return;

  if (!frompool && getMaxKI(ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(ch));
  }

  // log("Log 2 - attperc: %f, minimum: %f, charge: %" I64T "", attperc, minimum, GET_CHARGE(ch));
  if (!check_points(ch, getMaxKI(ch) * minimum, 0)) {
    return;
  }
  // log("Log 3 - attperc: %f, minimum: %f, charge: %" I64T "", attperc, minimum, GET_CHARGE(ch));
  skill = init_skill(ch, SKILL_KIBALL);

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
    improve_skill(ch, SKILL_KIBALL, 0);
    index = check_def(vict);
    prob  = roll_accuracy(ch, skill, TRUE);
    perc  = chance_to_hit(ch);

    index -= handle_speed(ch, vict);

    avo = index / 4;

    handle_defense(vict, &pry, &blk, &dge);

    /* Kiballs skill roll for multi-shot */
    int mult_roll = rand_number(1, 100), mult_count = 1, mult_chance = 0;

    if (skill >= 100) {
      mult_chance = 30;
    } else if (skill >= 75) {
      mult_chance = 15;
    } else if (skill >= 50) {
      mult_chance = 10;
    }

    if (mult_roll <= mult_chance)
      mult_count = rand_number(2, 3);

    /*-----------------------------------*/

    prob -= avo;
    tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

    if (!tech_handle_zanzoken(ch, vict, "kiball")) {
      if (frompool == TRUE) {
        decCurKI(ch, getMaxKI(ch) * attperc, 0);
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
        } else if (pry > rand_number(1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
          act("@C$N@W deflects your kiball, sending it flying away!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou deflect @C$n's@W kiball sending it flying away!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W deflects @c$n's@W kiball sending it flying away!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(vict, 0, getEffMaxPL(vict) / 200);

          parry_ki(attperc, ch, vict, "kiball", prob, perc, skill, 7);
          /*      User/target/skill name/skill/hurt type */
          pcost(ch, attperc, 0);

          if (rroom_get_dmg(char_get_in_room(ch)) <= 98) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (2));
          }

          return;
        } else if (blk > axion_dice(10)) {
          act("@C$N@W moves quickly and blocks kiball!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou move quickly and block @C$n's@W kiball!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W moves quickly and blocks @c$n's@W kiball!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          improve_skill(vict, SKILL_BLOCK, 0);
          pcost(ch, attperc, 0);
          pcost(vict, 0, getEffMaxPL(vict) / 500);
          dmg = damtype(ch, 7, skill, attperc);
          dmg /= 4;
          hurt(0, 0, ch, vict, NULL, dmg, 1);

          return;
        } else if (dge > axion_dice(10)) {
          act("@C$N@W manages to dodge your kiball, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@WYou dodge @C$n's@W kiball, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$N@W manages to dodge @c$n's@W kiball, letting it slam into the surroundings!@n", TRUE, ch, 0, vict, TO_NOTVICT);

          dodge_ki(ch, vict, 0, 7, skill, SKILL_KIBALL); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/

          if (rroom_get_dmg(char_get_in_room(ch)) <= 98) {
            rroom_set_dmg(char_get_in_room(ch), rroom_get_dmg(char_get_in_room(ch)) + (2));
          }
          improve_skill(vict, SKILL_DODGE, 0);
          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        } else {
          act("@WYou can't believe it but your kiball misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
          act("@C$n@W fires a bright yellow kiball at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@c$n@W fires a bright yellow kiball at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          pcost(ch, attperc, 0);
          hurt(0, 0, ch, vict, NULL, 0, 1);

          return;
        }
      } else {
        act("@WYou can't believe it but your kiball misses, flying through the air harmlessly!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n@W fires a bright yellow kiball at you, but misses!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@c$n@W fires a bright yellow kiball at @C$N@W, but somehow misses!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        pcost(ch, attperc, 0);
      }
      hurt(0, 0, ch, vict, NULL, 0, 1);
      return;
    } else {
      if (mult_count > 1) {
        act("@CYour expertise has allowed you to fire multiple shots in a row!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@C$n's@C expertise has allowed $m to fire multiple shots in a row!@n", TRUE, ch, 0, vict, TO_ROOM);
      }
      while (mult_count > 0) {
        mult_count -= 1;
        dmg         = damtype(ch, 7, skill, attperc);
        int hitspot = 1;
        hitspot     = roll_hitloc(ch, vict, skill);
        switch (hitspot) {
          case 1:
            act("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $M quickly and explodes with roaring light!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into you quickly and explodes with roaring light!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $M quickly and explodes with roaring light!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            if (char_get_bonuses(ch, BONUS_SOFT)) {
              dmg *= calc_critical(ch, 2);
            }
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 2: /* Critical */
            act("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $S face and explodes, shrouding $S head with smoke!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into your face and explodes, leaving you choking on smoke!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $S face and explodes, shrouding $S head with smoke!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            dmg *= calc_critical(ch, 0);
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 3);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 3:
            act("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $S body and explodes with a loud roar!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into your body and explodes with a loud roar!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $S body and explodes with a loud roar!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            if (char_get_bonuses(ch, BONUS_SOFT)) {
              dmg *= calc_critical(ch, 2);
            }
            hurt(0, 0, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 4);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 4: /* Weak */
            act("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball grazes $S arm and explodes shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball grazes your arm and explodes shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball grazes $S arm and explodes shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            dmg *= calc_critical(ch, 1);
            hurt(0, 195, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 1);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
          case 5: /* Weak 2 */
            act("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball grazes $S leg and explodes shortly after!@n", TRUE, ch, 0, vict, TO_CHAR);
            act("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball grazes your leg and explodes shortly after!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball grazes $S leg and explodes shortly after!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            dmg *= calc_critical(ch, 1);
            hurt(1, 195, ch, vict, NULL, dmg, 1);
            dam_eq_loc(vict, 2);
            /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
            break;
        }
        if (getCurPL(vict) <= 0) {
          mult_count = 0;
        }
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
    dmg = damtype(ch, 7, skill, attperc);
    dmg /= 10;
    act("@WYou fire a kiball at $p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@C$n@W fires a kiball at $p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    hurt(0, 0, ch, NULL, obj, dmg, 0);
    pcost(ch, attperc, 0);

  } else {
    send_to_char(ch, "Error! Please report.\r\n");
    return;
  }
}
