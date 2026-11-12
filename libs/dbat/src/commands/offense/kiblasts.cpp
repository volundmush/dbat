/* kiblasts.c: ki blast attacks
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
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
#include "ki_attack.hpp"
#include "area.hpp"

namespace {

/* Where an eraser cannon's or a big bang's hits land: their second and third places are both the head */
constexpr ki_spot HEAD_TWICE[6]     = {ki_spot::body, ki_spot::head, ki_spot::head,
                                       ki_spot::arm, ki_spot::leg, ki_spot::body};
constexpr ki_spot BIG_BANG_SPOTS[6] = {ki_spot::body, ki_spot::head, ki_spot::head,
                                       ki_spot::body, ki_spot::arm, ki_spot::leg};

/* An eraser cannon takes more ki the better its user knows it */
void eraser_drain(ki_shot &s, ki_outcome)
{
  if (char_get_skill(s.ch, SKILL_ERASER) >= 100) {
    decCurKI(s.g, s.ch, (getMaxKI(s.g, s.ch) * s.attperc) * 0.15, 0);
  } else if (char_get_skill(s.ch, SKILL_ERASER) >= 60) {
    decCurKI(s.g, s.ch, (getMaxKI(s.g, s.ch) * s.attperc) * 0.1, 0);
  } else if (char_get_skill(s.ch, SKILL_ERASER) >= 40) {
    decCurKI(s.g, s.ch, (getMaxKI(s.g, s.ch) * s.attperc) * 0.05, 0);
  }
}

/* A big bang lands anywhere, whatever the user's skill */
int big_bang_locate(ki_shot &s)
{
  return rand_number(s.g, 1, 6);
}

/* Piccolo's style shortens the wait after a balefire */
int balefire_cooldown(ki_shot &s)
{
  int cool = 8;
  if (char_is_piccolo(s.ch)) {
    if (char_get_skills_base(s.ch, SKILL_STYLE) >= 100)
      cool -= 3;
    else if (char_get_skills_base(s.ch, SKILL_STYLE) >= 75)
      cool -= 2;
    else if (char_get_skills_base(s.ch, SKILL_STYLE) >= 40)
      cool -= 1;
  }
  if (cool < 1)
    cool = 1;
  return cool;
}

void balefire_steady(ki_shot &s, int &prob)
{
  prob += rand_number(s.g, 10, 20);
}

/* A hell flash wears down a barrier, though never all the way */
void hell_flash_barrier(ki_shot &s)
{
  if (char_get_barrier(s.vict) > 0) {
    char_set_barrier(s.vict, char_get_barrier(s.vict) - (s.dmg));
    if (char_get_barrier(s.vict) <= 0) {
      char_set_barrier(s.vict, 1);
    }
  }
}

/* A master's shogekiha can dissipate some of the victim's charged ki */
void shogekiha_mastery(ki_shot &s, int &)
{
  int master_roll = rand_number(s.g, 1, 100), master_chance = 0;

  if (s.skill >= 100)
    master_chance = 20;
  else if (s.skill >= 75)
    master_chance = 10;
  else if (s.skill >= 50)
    master_chance = 5;
  s.mastery = master_chance >= master_roll;
}

/* Kabito's students get some of the ki back */
void shogekiha_refund(ki_shot &s)
{
  if (char_is_kabito(s.ch))
    ki_refund(s, .15);
}

void shogekiha_dissipate(ki_shot &s)
{
  if (s.mastery) {
    act(s.g, "@CYour skillful shogekiha disipated some of @c$N's@C charged ki!@n", true, s.ch, nullptr, s.vict, TO_CHAR);
    act(s.g, "@C$n@C's skillful shogekiha disipated some of YOUR charged ki!@n", true, s.ch, nullptr, s.vict, TO_VICT);
    act(s.g, "@C$n@C's skillful shogekiha disipated some of @c$N's@C charged ki!", true, s.ch, nullptr, s.vict, TO_NOTVICT);
    char_set_charge(s.vict, char_get_charge(s.vict) - (char_get_charge(s.vict) * 0.25));
  }
}

/* A shogekiha at a ki attack someone has thrown shatters it, or weakens it and takes the rest */
bool shogekiha_at_object(ki_shot &s)
{
  GameWorld *g         = s.g;
  struct char_data *ch = s.ch;
  struct obj_data *obj = s.obj;

  if (obj_get_kicharge(obj) > 0 && char_get_charge(ch) > obj_get_kicharge(obj)) {
    act(g, "@WYou leap at $p@W with your arms spread out to your sides. As you are about to make contact with $p@W you scream and shatter the attack with your ki!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W leaps out at $p@W with $s arms spead out to $s sides. As $e is about to make contact with $p@W $e screams and shatters the attack with $s ki!@n", true, ch, obj, nullptr, TO_ROOM);
    obj_set_kicharge(obj, obj_get_kicharge(obj) - (char_get_charge(ch)));
    extract_obj(g, obj);
    return true;
  }
  if (obj_get_kicharge(obj) > 0 && char_get_charge(ch) < obj_get_kicharge(obj)) {
    act(g, "@WYou leap at $p@W with your arms spread out to your sides. As you are about to make contact with $p@W you scream and weaken the attack with your ki before taking the rest of the attack in the chest!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W leaps out at $p@W with $s arms spead out to $s sides. As $e is about to make contact with $p@W $e screams and weakens the attack with $s ki before taking the rest of the attack in the chest!@n", true, ch, obj, nullptr, TO_ROOM);
    obj_set_kicharge(obj, obj_get_kicharge(obj) - (char_get_charge(ch)));
    char_set_charge(ch, 0);
    s.dmg = obj_get_kicharge(obj);
    hurt(g, 0, 0, obj_get_user(g, obj), ch, nullptr, s.dmg, 0);
    extract_obj(g, obj);
    return true;
  }
  return false;
}

/* A master's kiblast can knock out someone it hurts enough */
void kiblast_mastery(ki_shot &s, int &)
{
  int mastery = rand_number(s.g, 1, 100), chance = 0;

  if (s.skill >= 100)
    chance = 30;
  else if (s.skill >= 75)
    chance = 20;
  else if (s.skill >= 50)
    chance = 15;
  s.mastery = mastery <= chance;
}

/* Androids hit harder with it */
void kiblast_android(ki_shot &s)
{
  if (char_is_android(s.ch)) {
    if (char_get_skill(s.ch, SKILL_KIBLAST) >= 100) {
      s.dmg += s.dmg * 0.15;
    } else if (char_get_skill(s.ch, SKILL_KIBLAST) >= 60) {
      s.dmg += s.dmg * 0.10;
    } else if (char_get_skill(s.ch, SKILL_KIBLAST) >= 40) {
      s.dmg += s.dmg * 0.05;
    }
  }
}

void kiblast_knockout(ki_shot &s)
{
  GameWorld *g           = s.g;
  struct char_data *vict = s.vict;

  if (s.mastery && s.pl_before > getCurPL(g, vict) && (s.pl_before - getCurPL(g, vict) > (getEffMaxPL(g, vict)) * 0.025)) {
    if (!char_aff_flagged(vict, AFF_KNOCKED) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
      act(g, "@C$N@W is knocked out!@n", true, s.ch, nullptr, vict, TO_CHAR);
      act(g, "@WYou are knocked out!@n", true, s.ch, nullptr, vict, TO_VICT);
      act(g, "@C$N@W is knocked out!@n", true, s.ch, nullptr, vict, TO_NOTVICT);
      setStatusKnockedOut(vict);
    }
  }
}

const ki_attack ERASER = {
    .skill             = SKILL_ERASER,
    .name              = "Eraser Cannon",
    .damtype           = 18,
    .attperc           = .125,
    .minimum           = .1,
    .cooldown          = 6,
    .not_while_singing = true,
    .needs_arms        = false,
    .absorb            = ki_absorb::costs_one,
    .block             = {"@C$N@W moves quickly and blocks your eraser cannon!@n", "@WYou move quickly and block @C$n's@W eraser cannon!@n", "@C$N@W moves quickly and blocks @c$n's@W eraser cannon!@n"},
    .dodge             = {"@C$N@W manages to dodge your eraser cannon, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W eraser cannon, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W eraser cannon, letting it slam into the surroundings!@n"},
    .miss              = {"@WYou can't believe it but your eraser cannon misses, flying through the air harmlessly!@n", "@C$n@W fires a eraser cannon at you, but misses!@n ", "@c$n@W fires a eraser cannon at @C$N@W, but somehow misses!@n "},
    .miss_spent        = {"@WYou can't believe it but your eraser cannon misses, flying through the air harmlessly!@n", "@C$n@W fires a eraser cannon at you, but misses!@n", "@c$n@W fires a eraser cannon at @C$N@W, but somehow misses!@n"},
    .hits              = {{"@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's body!@n", "@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your body!@n", "@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's body!@n", true}, {"@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's face!@n", "@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your face!@n", "@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's face!@n", true}, {"@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's face!@n", "@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your face!@n", "@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's face!@n", true}, {"@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's arm!@n", "@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your arm!@n", "@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's arm!@n", true}, {"@WYou face @c$N@W quickly and open your mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in your throat, growing brighter as it rises up and out your mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's leg!@n", "@C$n @Wfaces you quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into your leg!@n", "@C$n @Wfaces @c$N@W quickly and opens $s mouth wide. A @Yg@yo@Yl@yd@Ye@yn@W glow forms from deep in $s throat, growing brighter as it rises up and out $s mouth. Suddenly a powerful Eraser Cannon erupts and slams into @c$N@W's leg!@n", true}},
    .spots             = HEAD_TWICE,
    .obj_to_char       = "@WYou fire a eraser cannon at $p@W!@n",
    .obj_to_room       = "@C$n@W fires a eraser cannon at $p@W!@n",
    .after_paying      = eraser_drain,
};

const ki_attack BIG_BANG = {
    .skill         = SKILL_BIGBANG,
    .name          = "Big Bang",
    .damtype       = 36,
    .attperc       = .3,
    .minimum       = .15,
    .cooldown      = 8,
    .kill_mode     = 3,
    .dodge_dmg_cap = 80,
    .dodge_dmg     = 20,
    .limb_hurt     = 170,
    .block         = {"@C$N@W moves quickly and blocks your Big Bang!@n", "@WYou move quickly and block @C$n's@W Big Bang!@n", "@C$N@W moves quickly and blocks @c$n's@W Big Bang!@n"},
    .dodge         = {"@C$N@W manages to dodge your Big Bang, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W Big Bang, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W Big Bang, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your Big Bang misses, flying through the air harmlessly!@n", "@C$n@W fires a Big Bang at you, but misses!@n ", "@c$n@W fires a Big Bang at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your Big Bang misses, flying through the air harmlessly!@n", "@C$n@W fires a Big Bang at you, but misses!@n", "@c$n@W fires a Big Bang at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's body, and explodes leaving behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR body, and explodes leading behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's body, and explodes leading behind a mushroom cloud shortly after!@n", true}, {"@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's head, and explodes leaving behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR head, and explodes leading behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's head, and explodes leading behind a mushroom cloud shortly after!@n", true}, {"@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's head, and explodes leaving behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR head, and explodes leading behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's head, and explodes leading behind a mushroom cloud shortly after!@n", true}, {"@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's gut, and explodes leaving behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR gut, and explodes leading behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's gut, and explodes leading behind a mushroom cloud shortly after!@n", true}, {"@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's arm, and explodes leaving behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR arm, and explodes leading behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's arm, and explodes leading behind a mushroom cloud shortly after!@n", true}, {"@WYou aim your hand at @c$N@W palm flat up at a ninety degree angle. Your charged ki pools there rapidly before a massive ball of energy explodes from your palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's leg, and explodes leaving behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at YOU palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into YOUR leg, and explodes leading behind a mushroom cloud shortly after!@n", "@C$n@W aims $s hand at @c$N@W palm flat up at a ninety degree angle. @C$n@W's charged ki pools there rapidly before a massive ball of energy explodes from $s palm! The @yB@Yi@wg @yB@Ya@wn@yg @Wattack crosses the distance rapidly, slams into @c$N@W's leg, and explodes leading behind a mushroom cloud shortly after!@n", true}},
    .spots         = BIG_BANG_SPOTS,
    .obj_to_char   = "@WYou fire a Big Bang at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a Big Bang at $p@W!@n",
    .locate        = big_bang_locate,
};

const ki_attack BALEFIRE = {
    .skill           = SKILL_BALEFIRE,
    .name            = "Balefire",
    .damtype         = 35,
    .attperc         = .3,
    .minimum         = .15,
    .kill_mode       = 3,
    .dodge_dmg_cap   = 80,
    .dodge_dmg       = 20,
    .limb_hurt       = 170,
    .dodge_explosion = "@wBright explosions erupts from the impact!\r\n",
    .block           = {"@C$N@W moves quickly and blocks every kiball of your scatter shot!@n", "@WYou move quickly and block every kiball of @C$n's@W scatter shot!@n", "@C$N@W moves quickly and blocks every kiball of @c$n's@W scatter shot!@n"},
    .dodge           = {"@C$N@W manages to dodge your balefire, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W balefire, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W balefire, letting it slam into the surroundings!@n"},
    .miss            = {"@WYou can't believe it but all the kiballs of your scatter shot miss, flying through the air harmlessly!@n", "@C$n@W fires scatter shot kiballs at you, but they miss!@n ", "@c$n@W fires scatter shot kiballs at @C$N@W, but somehow they miss!@n "},
    .miss_spent      = {"@WYou can't believe it but all of your balefire misses, flying through the air harmlessly!@n", "@C$n@W fires balefire at you, but they miss!@n ", "@c$n@W fires balefire at @C$N@W, but somehow they miss!@n "},
    .hits            = {{"@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's body!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR body!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's body!@n", true}, {"@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's head!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR head!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's head!@n", true}, {"@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's gut!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR gut!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's gut!@n", true}, {"@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's arm!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR arm!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's arm!@n", true}, {"@WYou open yourself to the source and begin tracing weaves, the most complex combination, with just the right amounts of each! Your innate talent finishes the final weave and a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from your hands! Your @WB@Ra@Yl@Wef@Yi@Rr@We@W crosses the distance quickly and slams into @C$N@W's leg!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into YOUR leg!@n", "@C$n@W begins to glow with a soft, white aura! Their hands begin manipulating something unseen when suddenly a giant bar of @WB@Ra@Yl@Wef@Yi@Rr@We@W lances out from $s hands, crossing the distance and slamming into @C$N@W's leg!@n", true}},
    .obj_to_char     = "@WYou fire balefire at $p@W!@n",
    .obj_to_room     = "@C$n@W fires balefire at $p@W!@n",
    .cooldown_for    = balefire_cooldown,
    .steady          = balefire_steady,
};

const ki_attack HELL_FLASH = {
    .skill        = SKILL_HELLFLASH,
    .name         = "Hell Flash",
    .damtype      = 32,
    .attperc      = .2,
    .minimum      = .10,
    .cooldown     = 7,
    .perfection   = true,
    .limb_hurt    = 170,
    .block        = {"@C$N@W moves quickly and blocks your Hell Flash!@n", "@WYou move quickly and block @C$n's@W Hell Flash!@n", "@C$N@W moves quickly and blocks @c$n's@W Hell Flash!@n"},
    .dodge        = {"@C$N@W manages to dodge your Hell Flash, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W Hell Flash, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W Hell Flash, letting it slam into the surroundings!@n"},
    .miss         = {"@WYou can't believe it but your Hell Flash misses, flying through the air harmlessly!@n", "@C$n@W fires a Hell Flash at you, but misses!@n ", "@c$n@W fires a Hell Flash at @C$N@W, but somehow misses!@n "},
    .miss_spent   = {"@WYou can't believe it but your Hell Flash misses, flying through the air harmlessly!@n", "@C$n@W fires a Hell Flash at you, but misses!@n", "@c$n@W fires a Hell Flash at @C$N@W, but somehow misses!@n"},
    .hits         = {{"@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's body!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR body!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's body!@n", true}, {"@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's head!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR head!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's head!@n", true}, {"@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's gut!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR gut!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's gut!@n", true}, {"@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's arm!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR arm!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's arm!@n", true}, {"@WYou stick one of your hands in each of your armpits and by twisting you remove them. Then you aim your exposed wrist cannons at @c$N@W as your charged energy begins to be channeled into the cannons. Shouting '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' two large blasts of energy explode from the cannons and slam into @c$N@W's leg!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at YOU as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into YOUR leg!@n", "@C$n@W sticks one of $s hands in each of $s armpits and by twisting $e removes them. Then $e aims $s exposed wrist cannons at @c$N@W as $s charged energy begins to be channeled into the cannons. @C$n@W shouts '@YH@re@Rl@Yl @rF@Rl@Ya@rs@Rh@W' as two large blasts of energy explode from the cannons and slam into @c$N@W's leg!@n", true}},
    .obj_to_char  = "@WYou fire a Hell Flash at $p@W!@n",
    .obj_to_room  = "@C$n@W fires a Hell Flash at $p@W!@n",
    .bonus_damage = hell_flash_barrier,
};

/* quirk: Kabito's students were to have a different wait, but both waits are 5 */
const ki_attack SHOGEKIHA = {
    .skill          = SKILL_SHOGEKIHA,
    .name           = "Shogekiha",
    .damtype        = 10,
    .attperc        = .125,
    .minimum        = .05,
    .cooldown       = 5,
    .limb_hurt      = 0,
    .soft_crit      = false,
    .leg_flag       = 0,
    .dodge_scatters = false,
    .block          = {"@C$N@W moves quickly and blocks shogekiha!@n", "@WYou move quickly and block @C$n's@W shogekiha!@n", "@C$N@W moves quickly and blocks @c$n's@W shogekiha!@n", true},
    .dodge          = {"@C$N@W manages to dodge your shogekiha, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W shogekiha, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W shogekiha, letting it slam into the surroundings!@n", true},
    .miss           = {"@WYou can't believe it but your shogekiha misses, flying through the air harmlessly!@n", "@C$n@W aims $s hand and releases a shogekiha at you, but misses!@n ", "@c$n@W aims $s hand and releases a shogekiha at @C$N@W, but somehow misses!@n ", true},
    .miss_spent     = {"@WYou can't believe it but your shogekiha misses, flying through the air harmlessly!@n", "@C$n@W aims $s hand and releases a shogekiha at you, but misses!@n", "@c$n@W aims $s hand and releases a shogekiha at @C$N@W, but somehow misses!@n", true},
    .hits           = {{"@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S chest!@n", "@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your chest!@n", "@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's chest!@n", true}, {"@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S face!@n", "@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your face!@n", "@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's face!@n", true}, {"@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S gut!@n", "@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your gut!@n", "@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's gut!@n", true}, {"@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S arm!@n", "@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your arm!@n", "@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's arm!@n", true}, {"@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S leg!@n", "@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your leg!@n", "@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's leg!@n", true}},
    .obj_to_char    = "@WYou fire a shogekiha at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a shogekiha at $p@W!@n",
    .obj_hide       = true,
    .steady         = shogekiha_mastery,
    .bonus_damage   = shogekiha_refund,
    .after_hit      = shogekiha_dissipate,
    .on_object      = shogekiha_at_object,
};

const ki_attack KIBLAST = {
    .skill           = SKILL_KIBLAST,
    .name            = "kiblast",
    .damtype         = 9,
    .attperc         = .075,
    .minimum         = .01,
    .cooldown        = 5,
    .absorb          = ki_absorb::costs_one,
    .block_improves  = true,
    .limb_hurt       = 195,
    .obj_damtype     = 10,
    .needs_free_hand = true,
    .dodge_explosion = nullptr,
    .parry           = {"@C$N@W deflects your kiblast, sending it flying away!@n", "@WYou deflect @C$n's@W kiblast sending it flying away!@n", "@C$N@W deflects @c$n's@W kiblast sending it flying away!@n", true},
    .block           = {"@C$N@W moves quickly and blocks kiblast!@n", "@WYou move quickly and block @C$n's@W kiblast!@n", "@C$N@W moves quickly and blocks @c$n's@W kiblast!@n", true},
    .dodge           = {"@C$N@W manages to dodge your kiblast, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W kiblast, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W kiblast, letting it slam into the surroundings!@n", true},
    .miss            = {"@WYou can't believe it but your kiblast misses, flying through the air harmlessly!@n", "@C$n@W fires a bright yellow kiblast at you, but misses!@n ", "@c$n@W fires a bright yellow kiblast at @C$N@W, but somehow misses!@n ", true},
    .miss_spent      = {"@WYou can't believe it but your kiblast misses, flying through the air harmlessly!@n", "@C$n@W fires a bright yellow kiblast at you, but misses!@n", "@c$n@W fires a bright yellow kiblast at @C$N@W, but somehow misses!@n", true},
    .hits            = {{"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S chest!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your chest!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's chest!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S face!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your face!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's face!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S gut!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your gut!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's gut!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S arm!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your arm!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's arm!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S leg!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your leg!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's leg!@n", true}},
    .obj_to_char     = "@WYou fire a kiblast at $p@W!@n",
    .obj_to_room     = "@C$n@W fires a kiblast at $p@W!@n",
    .obj_hide        = true,
    .steady          = kiblast_mastery,
    .bonus_damage    = kiblast_android,
    .after_hit       = kiblast_knockout,
};

} // namespace

ACMD(do_eraser) { perform_ki_attack(g, ch, argument, ERASER); }

ACMD(do_bigbang) { perform_ki_attack(g, ch, argument, BIG_BANG); }

ACMD(do_balefire) { perform_ki_attack(g, ch, argument, BALEFIRE); }

ACMD(do_hellflash) { perform_ki_attack(g, ch, argument, HELL_FLASH); }

namespace {

/* A bakuhatsuha hits harder the better it is known, and spreads thinner the more it catches */
void bakuhatsuha_damage(area_shot &s)
{
  if (char_get_skill(s.ch, SKILL_BAKUHATSUHA) >= 100)
    s.dmg += getEffMaxPL(s.g, s.ch) * 0.08;
  else if (char_get_skill(s.ch, SKILL_BAKUHATSUHA) >= 60)
    s.dmg += getEffMaxPL(s.g, s.ch) * 0.04;
  else if (char_get_skill(s.ch, SKILL_BAKUHATSUHA) >= 40)
    s.dmg += getEffMaxPL(s.g, s.ch) * 0.02;
  if (s.count == 2)
    s.dmg = (s.dmg / 100) * 75;
  else if (s.count == 3)
    s.dmg = (s.dmg / 100) * 50;
  else if (s.count > 3)
    s.dmg = (s.dmg / 100) * 25;
}

const area_attack BAKUHATSUHA = {
    .skill          = SKILL_BAKUHATSUHA,
    .damtype        = 24,
    .attperc        = .15,
    .minimum        = .05,
    .worth          = area_fair_game_ungrouped,
    .caught         = area_fair_game_no_group, /* quirk: it isn't worth making for those it then spares */
    .cooldown_first = 6,
    .dodge_low      = -15,
    .vanish_hurts   = true,
    .knockdown      = area_knockdown::always,
    .fail           = {"@WYou raise your hand with index and middle fingers extended upwards. You try releasing your charged ki in a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W but mess up and waste the ki!@n", nullptr, "@C$n@W raises $s hand with index and middle fingers extended upwards. @C$n@W tries releasing $s charged ki in a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W but messes up and wastes the ki!@n"},
    .launch         = {"@WYou raise your hand with index and middle fingers extended upwards. A sudden burst rushes up from the ground as your charged ki explodes in the form of a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W!@n", nullptr, "@C$n@W raises $s hand with index and middle fingers extended upwards. A sudden burst rushes up from the ground as $s charged ki explodes in the form of a @yB@Ya@Wk@wuh@ya@Yt@Ws@wuh@ya@W!@n"},
    .vanish         = EXPLOSION_VANISH,
    .escape         = EXPLOSION_ESCAPE,
    .hit            = EXPLOSION_HIT,
    .damage         = bakuhatsuha_damage,
};

} // namespace

ACMD(do_baku)
{
  perform_area_attack(g, ch, argument, BAKUHATSUHA);
}

ACMD(do_shogekiha) { perform_ki_attack(g, ch, argument, SHOGEKIHA); }

ACMD(do_kiblast) { perform_ki_attack(g, ch, argument, KIBLAST); }
