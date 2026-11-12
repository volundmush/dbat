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
#include "offense/ki_attack.hpp"
#include "offense/strike.hpp"

namespace {

/* Kurzak's masters headbutt sooner */
int headbutt_cooldown(strike_shot &s)
{
  return char_is_kurzak(s.ch) && char_get_skill(s.ch, SKILL_HEADBUTT) >= 100 ? 5 : 6;
}

/* and Kurzak's headbutts hit harder */
void kurzak_headbutt(strike_shot &s)
{
  if (!char_is_kurzak(s.ch))
    return;
  if (char_get_skill(s.ch, SKILL_HEADBUTT) >= 60)
    s.dmg += s.dmg * 0.1;
  else if (char_get_skill(s.ch, SKILL_HEADBUTT) >= 40)
    s.dmg += s.dmg * 0.05;
}

/* A headbutt to the face can knock the victim out */
void headbutt_knockout(strike_shot &s, int where)
{
  struct char_data *vict = s.vict;

  if (where != 2)
    return;
  if (!char_aff_flagged(vict, AFF_KNOCKED) &&
      (rand_number(s.g, 1, 7) >= 4 && (getCurPL(s.g, vict) > getCurPL(s.g, s.ch) / 5) &&
       !char_aff_flagged(vict, AFF_SANCTUARY))) {
    act_triple(s.g, {"@C$N@W is knocked out!@n", "@WYou are knocked out!@n", "@C$N@W is knocked out!@n", true}, s.ch,
               nullptr, vict);
    setStatusKnockedOut(vict);
  }
}

/* A critical headbutt multiplies by a whole number, one more for Kurzak's students with style */
long double headbutt_crit(strike_shot &s)
{
  int mult = calc_critical(s.g, s.ch, 0);
  if (char_is_kurzak(s.ch) && !char_is_npc(s.ch) && char_get_skills_base(s.ch, SKILL_STYLE) >= 75)
    mult += 1;
  return mult;
}

/* A bash can bring either of them crashing down */
void bash_crashdown(strike_shot &s)
{
  if (s.vict && rand_number(s.g, 1, 5) >= 4)
    tech_handle_crashdown(s.g, s.vict, s.ch); /* quirk: the victim brings the basher down */
  if (rand_number(s.g, 1, 5) >= 5)
    tech_handle_crashdown(s.g, s.ch, s.vict);
}

int sunder_to_hit(ki_shot &s)
{
  return axion_dice(s.g, 0);
}

/* How badly a sundering force to the body tears at a limb: rolled before its damage */
void sunder_chance(ki_shot &s)
{
  s.tally = rand_number(s.g, 2, 12);
}

bool sunder_strike(ki_shot &s, int where)
{
  GameWorld *g = s.g;

  switch (where) {
    case 1:
    case 3:
      if (char_get_bonuses(s.ch, BONUS_SOFT))
        s.dmg *= calc_critical(g, s.ch, 2);
      if (s.tally >= 10)
        hurt(g, 0, 160, s.ch, s.vict, nullptr, s.dmg, 1);
      else if (s.tally >= 8)
        hurt(g, 1, 160, s.ch, s.vict, nullptr, s.dmg, 1);
      else
        hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 4);
      return true;
    case 2:
      s.dmg *= calc_critical(g, s.ch, 0);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 3);
      return true;
    case 4: /* quirk: the arm, as hurt() and the messages have it, but the legs' equipment */
      s.dmg *= calc_critical(g, s.ch, 1);
      hurt(g, 0, 160, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 2);
      return true;
    case 5: /* and the leg, but the arms' equipment */
      s.dmg *= calc_critical(g, s.ch, 1);
      hurt(g, 1, 160, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 1);
      return true;
  }
  return false;
}

/* Where a malice breaker lands: a third of the time, the head */
int malice_locate(ki_shot &s)
{
  static constexpr int spots[6] = {1, 2, 2, 3, 4, 5};
  return spots[rand_number(s.g, 1, 6) - 1];
}

/* Malice breaker is surer by night */
void malice_by_night(ki_shot &s, int &prob)
{
  if (s.g->time_info.hours <= 15 || s.g->time_info.hours > 22)
    prob += 5;
}

/* and harder late in the day */
void malice_after_dark(ki_shot &s)
{
  if (s.g->time_info.hours <= 15)
    s.dmg *= 1.25;
  else if (s.g->time_info.hours <= 22)
    s.dmg *= 1.4;
}

/* Seishou enko costs a quarter of its charge unless it hits */
double seishou_cost(const ki_shot &s, ki_outcome how)
{
  return how == ki_outcome::hit ? s.attperc : s.attperc / 4;
}

/* A fully molted arlian's is twice as strong */
void seishou_molted(ki_shot &s)
{
  if (char_get_moltlevel(s.ch) >= 150)
    s.dmg *= 2;
}

int seishou_locate(ki_shot &s)
{
  static constexpr int spots[7] = {1, 2, 2, 2, 3, 4, 5};
  return spots[rand_number(s.g, 1, 7) - 1];
}

bool has_water(ki_shot &s)
{
  if (char_is_android(s.vict)) {
    send_to_char(s.g, s.ch, "There is not a necessary amount of water in cybernetic creatures.\r\n");
    return false;
  }
  return true;
}

/* Under an aqua barrier, some of a water attack's damage goes into the barrier */
void aqua_barrier(ki_shot &s)
{
  struct char_data *ch = s.ch;

  if (!char_aff_flagged(ch, AFF_SANCTUARY))
    return;
  if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 100)
    char_set_barrier(ch, char_get_barrier(ch) + (s.dmg * 0.1));
  else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 60)
    char_set_barrier(ch, char_get_barrier(ch) + (s.dmg * 0.05));
  else if (char_get_skill(ch, SKILL_AQUA_BARRIER) >= 40)
    char_set_barrier(ch, char_get_barrier(ch) + (s.dmg * 0.02));
  if (char_get_barrier(ch) > getMaxKI(s.g, ch))
    char_set_barrier(ch, getMaxKI(s.g, ch));
}

/* What a water razor took of the victim's power level, it also takes of their ki and stamina */
void water_razor_drain(ki_shot &s, ki_outcome how)
{
  GameWorld *g           = s.g;
  struct char_data *vict = s.vict;

  if (how != ki_outcome::hit)
    return;
  int64_t reduction = s.pl_before - getCurPL(g, vict);
  if ((!char_is_npc(vict) && !char_aff_flagged(vict, AFF_SPIRIT)) || (char_is_npc(vict) && getCurPL(g, vict) > 0)) {
    decCurKI(g, vict, reduction, 0);
    decCurST(g, vict, reduction, 0);
  }
}

/* Water spikes in an arm or a leg: quirk, with no critical roll */
bool water_spikes_limb(ki_shot &s, int where)
{
  if (where != 4 && where != 5)
    return false;
  dam_eq_loc(s.g, s.vict, where == 4 ? 1 : 2);
  hurt(s.g, where == 4 ? 0 : 1, 190, s.ch, s.vict, nullptr, s.dmg, 1);
  return true;
}

/* to the head can knock the victim out */
void water_spikes_knockout(ki_shot &s)
{
  struct char_data *vict = s.vict;

  if (s.where == 2 && !char_aff_flagged(vict, AFF_KNOCKED) &&
      (rand_number(s.g, 1, 3) >= 2 && (getCurPL(s.g, vict) > getCurPL(s.g, s.ch) / 5) &&
       !char_aff_flagged(vict, AFF_SANCTUARY))) {
    act_triple(s.g, {"@C$N@W is knocked out!@n", "@WYou are knocked out!@n", "@C$N@W is knocked out!@n", true}, s.ch,
               nullptr, vict);
    setStatusKnockedOut(vict);
  }
}

/* Some of the ki comes back, and perfection 3 makes the user wait */
void water_spikes_refund(ki_shot &s, ki_outcome how)
{
  ki_refund(s, .3);
  if (char_get_skills_perf(s.ch, SKILL_WSPIKE) == 3 && !(how == ki_outcome::hit && s.where == 4)) /* quirk */
    char_set_wait(s.ch, PULSE_3SEC);
}

/* A koteiru bakuha can freeze its victim solid */
void koteiru_freeze(ki_shot &s, ki_outcome how)
{
  struct char_data *vict = s.vict;

  if (how != ki_outcome::hit || getCurPL(s.g, vict) <= 1)
    return;
  if (rand_number(s.g, 1, 4) == 1 && !char_aff_flagged(vict, AFF_FROZEN) && !char_is_demon(vict)) {
    act(s.g, "@CYour body completely freezes!@n", true, vict, nullptr, nullptr, TO_CHAR);
    act(s.g, "@c$n's@C body completely freezes!@n", true, vict, nullptr, nullptr, TO_ROOM);
    char_aff_set(vict, AFF_FROZEN, true);
  }
}

/* A star breaker steals experience, the more the further below the attacker the victim is */
void star_breaker_theft(ki_shot &s)
{
  struct char_data *ch = s.ch, *vict = s.vict;
  int64_t theft;

  if (char_get_level(ch) - 30 > char_get_level(vict))
    theft = 1;
  else if (char_get_level(ch) - 20 > char_get_level(vict))
    theft = char_get_exp(vict) / 1000;
  else if (char_get_level(ch) - 10 > char_get_level(vict))
    theft = char_get_exp(vict) / 100;
  else if (char_get_level(ch) >= char_get_level(vict))
    theft = char_get_exp(vict) / 50;
  else if (char_get_level(ch) + 10 >= char_get_level(vict))
    theft = char_get_exp(vict) / 500;
  else if (char_get_level(ch) + 20 >= char_get_level(vict))
    theft = char_get_exp(vict) / 1000;
  else
    theft = char_get_exp(vict) / 2000;
  char_set_exp(vict, char_get_exp(vict) - theft);
  s.extra = theft;
}

/* and gives the attacker twice that, if they can use it */
void star_breaker_blessing(ki_shot &s)
{
  struct char_data *ch = s.ch;

  if (level_exp(ch, char_get_level(ch) + 1) - (char_get_exp(ch)) > 0 || char_get_level(ch) >= 100) {
    send_to_char(s.g, ch, "The returning Eldritch energy blesses you with some experience. @D[@G{}@D]@n\r\n",
                 add_commas(s.extra));
    char_set_exp(ch, char_get_exp(ch) + (s.extra * 2));
  }
}

const strike_attack HEADBUTT = {
    .skill             = SKILL_HEADBUTT,
    .name              = "headbutt",
    .damtype           = 52,
    .stamina_share     = 100,
    .needs             = strike_needs::nothing,
    .not_while_singing = true,
    .block_damtype     = 3,
    .avoid_die         = 130,
    .fireshield        = "head",
    .spent_hurts_first = true,
    .spent_free        = true,
    .parry             = {"@C$N@W parries your headbutt with an attack of $S own!@n", "@WYou parry @C$n's@W headbutt with an attack of your own!@n", "@C$N@W parries @c$n's@W headbutt with an attack of $S own!@n", true},
    .block             = {"@C$N@W blocks your headbutt!@n", "@WYou block @C$n's@W headbutt!@n", "@C$N@W blocks @c$n's@W headbutt!@n", true},
    .dodge             = {"@C$N@W dodges your headbutt!@n", "@WYou dodge @C$n's@W headbutt!@n", "@C$N@W dodges @c$n's@W headbutt!@n", true},
    .miss              = {"@WYou can't believe it, your headbutt misses!@n", "@C$n@W throws a headbutt at you, but thankfully misses!@n", "@c$n@W throws a headbutt at @C$N@W, but misses!@n", true},
    .miss_spent        = {"@WYou can't believe it, your headbutt misses!@n", "@C$n@W throws a headbutt at you, but thankfully misses!@n", "@c$n@W throws a headbutt at @C$N@W, but misses!@n", true},
    .hits              = {{"@WYou grab @c$N@W by the shoulders and slam your head into $S chest!@n", "@C$n@W grabs YOU by the shoulders and slams $s head into YOUR chest!@n", "@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W chest!@n", true}, {"@WYou grab @c$N@W by the shoulders and slam your head into $S face!@n", "@C$n@W grabs YOU by the shoulders and slams $s head into YOUR face!@n", "@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W face!@n", true}, {"@WYou grab @c$N@W by the shoulders and slam your head into $S chest!@n", "@C$n@W grabs YOU by the shoulders and slams $s head into YOUR chest!@n", "@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W chest!@n", true}, {"@WYou grab @c$N@W and barely manage to slam your head into $S leg!@n", "@C$n@W grabs YOU and barely manages to slam $s head into YOUR leg!@n", "@C$n@W grabs @c$N@W and barely manages to slam $s head into @c$N's@W leg!@n", true}, {"@WYou grab @c$N@W and barely manage to slam your head into $S arm!@n", "@C$n@W grabs YOU and barely manages to slam $s head into YOUR arm!@n", "@C$n@W grabs @c$N@W and barely manages to slam $s head into @c$N's@W arm!@n", true}},
    .spots             = LEG_THEN_ARM,
    .obj_to_char       = "@WYou headbutt $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W headbutt $p@W extremely hard!@n",
    .obj_hide          = true,
    .cooldown_for      = headbutt_cooldown,
    .bonus_damage      = kurzak_headbutt,
    .on_spot           = headbutt_knockout,
    .head_crit         = headbutt_crit,
};

const strike_attack BASH = {
    .skill             = SKILL_BASH,
    .name              = "bash",
    .damtype           = 51,
    .cooldown          = 6,
    .stamina_share     = 70,
    .needs             = strike_needs::nothing,
    .not_while_singing = true,
    .block_damtype     = 3,
    .avoid_die         = 130,
    .fireshield        = "body",
    .spent_hurts_first = true,
    .spent_free        = true,
    .parry             = {"@C$N@W parries your bash with an attack of $S own!@n", "@WYou parry @C$n's@W bash with an attack of your own!@n", "@C$N@W parries @c$n's@W bash with an attack of $S own!@n", true},
    .block             = {"@C$N@W blocks your bash!@n", "@WYou block @C$n's@W bash!@n", "@C$N@W blocks @c$n's@W bash!@n", true},
    .dodge             = {"@C$N@W dodges your bash!@n", "@WYou dodge @C$n's@W bash!@n", "@C$N@W dodges @c$n's@W bash!@n", true},
    .miss              = {"@WYou can't believe it, your bash misses!@n", "@C$n@W throws a bash at you, but thankfully misses!@n", "@c$n@W throws a bash at @C$N@W, but misses!@n", true},
    .miss_spent        = {"@WYou can't believe it, your bash misses!@n", "@C$n@W throws a bash at you, but thankfully misses!@n", "@c$n@W throws a bash at @C$N@W, but misses!@n", true},
    .hits              = {{"@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S body with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR body with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S body with a crashing impact!@n", true}, {"@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S head with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR head with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S head with a crashing impact!@n", true}, {"@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S gut with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR gut with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S gut with a crashing impact!@n", true}, {"@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S leg with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR leg with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S leg with a crashing impact!@n", true}, {"@WBending over slightly you aim your body at @c$N@W and instantly launch yourself toward $M at full speed! You slam into $S arm with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at YOU and then instantly launches $mself toward YOU at full speed! @C$n@W slams into YOUR arm with a crashing impact!@n", "@C$n@W bends over slightly aiming $s body at @c$N@W and then instantly launches $mself toward $M at full speed! @C$n@W slams into $S arm with a crashing impact!@n", true}},
    .spots             = LEG_THEN_ARM,
    .obj_to_char       = "@WYou bash $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W bash $p@W extremely hard!@n",
    .obj_hide          = true,
    .after_hit         = bash_crashdown,
};

const ki_attack SUNDER = {
    .skill          = SKILL_SUNDER,
    .name           = "Sundering Force",
    .damtype        = 55,
    .attperc        = .25,
    .minimum        = .05,
    .cooldown       = 6,
    .blockable      = false,
    .avoid_die      = 130,
    .dodge_scatters = false,
    .obj_damtype    = 10,
    .dodge          = {"@C$N@W manages to dodge your Sundering Force, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Sundering Force, letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Sundering Force, letting it fly harmlessly by!@n"},
    .miss           = {"@WYou can't believe it but your Sundering Force misses, flying through the air harmlessly!@n", "@C$n@W fires a Sundering Force at you, but misses!@n ", "@c$n@W fires a Sundering Force at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your Sundering Force misses, flying through the air harmlessly!@n", "@C$n@W fires a Sundering Force at you, but misses!@n", "@c$n@W fires a Sundering Force at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W body with the force of your energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR body with the force of $s energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W body with the force of $s energy!@n", true}, {"@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W head with the force of your energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR head with the force of $s energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W head with the force of $s energy!@n", true}, {"@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W body with the force of your energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR body with the force of $s energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W body with the force of $s energy!@n", true}, {"@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W arm with the force of your energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR arm with the force of $s energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W arm with the force of $s energy!@n", true}, {"@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W leg with the force of your energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR leg with the force of $s energy!@n", "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W leg with the force of $s energy!@n", true}},
    .obj_to_char    = "@WYou fire a Sundering Force at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a Sundering Force at $p@W!@n",
    .obj_hide       = true,
    .to_hit         = sunder_to_hit,
    .before_landing = sunder_chance,
    .strike         = sunder_strike,
};

const ki_attack MALICE = {
    .skill             = SKILL_MALICE,
    .name              = "Malice Breaker",
    .damtype           = 36,
    .attperc           = .3,
    .minimum           = .15,
    .cooldown          = 8,
    .kill_mode         = 3,
    .not_while_singing = true,
    .dodge_dmg_cap     = 80,
    .dodge_dmg         = 20,
    .limb_hurt         = 170,
    .avoid_die         = 130,
    .block             = {"@C$N@W moves quickly and blocks your Malice Breaker!@n", "@WYou move quickly and block @C$n's@W Malice Breaker!@n", "@C$N@W moves quickly and blocks @c$n's@W Malice Breaker!@n"},
    .dodge             = {"@C$N@W manages to dodge your Malice Breaker, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W Malice Breaker, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W Malice Breaker, letting it slam into the surroundings!@n"},
    .miss              = {"@WYou can't believe it but your Malice Breaker misses, flying through the air harmlessly!@n", "@C$n@W fires a Malice Breaker at you, but misses!@n ", "@c$n@W fires a Malice Breaker at @C$N@W, but somehow misses!@n "},
    .miss_spent        = {"@WYou can't believe it but your Malice Breaker misses, flying through the air harmlessly!@n", "@C$n@W fires a Malice Breaker at you, but misses!@n", "@c$n@W fires a Malice Breaker at @C$N@W, but somehow misses!@n"},
    .hits              = {{"@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S chest, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's chest!@n", "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your chest, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your chest!@n", "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W chest, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's chest!@n", true}, {"@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S head, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's head!@n", "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your head, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your head!@n", "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W head, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's head!@n", true}, {"@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S gut, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's gut!@n", "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your gut, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your gut!@n", "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W gut, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's gut!@n", true}, {"@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S arm, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's arm!@n", "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your arm, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your arm!@n", "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W arm, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's arm!@n", true}, {"@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S leg, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's leg!@n", "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your leg, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your leg!@n", "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slam an open palm into @c$N's@W leg, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's leg!@n", true}},
    .obj_to_char       = "@WYou fire a Malice Breaker at $p@W!@n",
    .obj_to_room       = "@C$n@W fires a Malice Breaker at $p@W!@n",
    .steady            = malice_by_night,
    .bonus_damage      = malice_after_dark,
    .locate            = malice_locate,
};

const ki_attack SEISHOU = {
    .skill             = SKILL_SEISHOU,
    .name              = "Seishou Enko",
    .damtype           = 50,
    .attperc           = .15,
    .minimum           = .05,
    .cooldown          = 4,
    .not_while_singing = true,
    .needs_arms        = false,
    .limb_hurt         = 0,
    .blockable         = false,
    .avoid_die         = 130,
    .leg_flag          = 0,
    .dodge_scatters    = false,
    .obj_damtype       = 10,
    .dodge             = {"@C$N@W manages to dodge your Seishou Enko, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Seishou Enko, letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Seishou Enko, letting it fly harmlessly by!@n"},
    .miss              = {"@WYou can't believe it but your Seishou Enko misses, flying through the air harmlessly!@n", "@C$n@W fires a Seishou Enko at you, but misses!@n ", "@c$n@W fires a Seishou Enko at @C$N@W, but somehow misses!@n "},
    .miss_spent        = {"@WYou can't believe it but your Seishou Enko misses, flying through the air harmlessly!@n", "@C$n@W fires a Seishou Enko at you, but misses!@n", "@c$n@W fires a Seishou Enko at @C$N@W, but somehow misses!@n"},
    .hits              = {{"@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S body with searing heat!@n", "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR body with searing heat!@n", "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S body with searing heat!@n", true}, {"@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S head with searing heat!@n", "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR head with searing heat!@n", "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S head with searing heat!@n", true}, {"@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S gut with searing heat!@n", "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR gut with searing heat!@n", "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S gut with searing heat!@n", true}, {"@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S arm with searing heat!@n", "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR arm with searing heat!@n", "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S arm with searing heat!@n", true}, {"@WYou aim your mouth at @C$N@W and focus your charged ki. In an instant you fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S leg with searing heat!@n", "@C$n @Waims $s mouth at YOU and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! Almost instantly it blasts into YOUR leg with searing heat!@n", "@C$n @Waims $s mouth at @c$N@W and seems to focus $s ki. In an instant $e fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! Almost instantly it blasts into $S leg with searing heat!@n", true}},
    .obj_to_char       = "@WYou fire a Seishou Enko at $p@W!@n",
    .obj_to_room       = "@C$n@W fires a Seishou Enko at $p@W!@n",
    .obj_hide          = true,
    .cost              = seishou_cost,
    .bonus_damage      = seishou_molted,
    .locate            = seishou_locate,
};

const ki_attack WATER_RAZOR = {
    .skill          = SKILL_WRAZOR,
    .name           = "Water Razor",
    .damtype        = 47,
    .attperc        = .14,
    .minimum        = .05,
    .cooldown       = 6,
    .limb_hurt      = 0,
    .blockable      = false,
    .avoid_die      = 130,
    .leg_flag       = 0,
    .dodge_scatters = false,
    .damages_eq     = false,
    .no_objects     = "You can not hurt an inanimate object with this technique.\r\n",
    .dodge          = {"@C$N@W manages to dodge your Water Razor, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Water Razor, letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Water Razor, letting it fly harmlessly by!@n"},
    .miss           = {"@WYou can't believe it but your Water Razor misses, flying through the air harmlessly!@n", "@C$n@W fires a Water Razor at you, but misses!@n ", "@c$n@W fires a Water Razor at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your Water Razor misses, flying through the air harmlessly!@n", "@C$n@W fires a Water Razor at you, but misses!@n", "@c$n@W fires a Water Razor at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n", "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true}, {"@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out in a mist from every pore of $S body!@n", "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides! Blood sprays out of your pores into a fine mist!@n", "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out of $S pores into a fine mist!@n", true}, {"@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out in a mist from every pore of $S body!@n", "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides! Blood sprays out of your pores into a fine mist!@n", "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides! Blood sprays out of $S pores into a fine mist!@n", true}, {"@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n", "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true}, {"@WYou raise your hand toward @c$N@W with the palm open as if you are ready to grab the air between the two of you. You then focus your ki into $S body and close your hand in an instant! The @Bwater@W in $S body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", "@C$n@W raises $s hand toward YOU with the palm open as if $e is ready to grab the air between the two of you. $e then focus $s ki into YOUR body and close $s hand in an instant! The @Bwater@W in your body instantly takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n", "@C$n@W raises $s hand toward @c$N@W with the palm open as if $e is ready to grab the air between the two of them. $e then focus $s ki into @c$N's@W body and close $s hand in an instant! The @Bwater@W in @c$N's@W body instantly takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n", true}},
    .victim_ok      = has_water,
    .bonus_damage   = aqua_barrier,
    .after_paying   = water_razor_drain,
};

const ki_attack WATER_SPIKES = {
    .skill          = SKILL_WSPIKE,
    .name           = "attack",
    .damtype        = 43,
    .attperc        = .14,
    .minimum        = .05,
    .cooldown       = 6,
    .perfection     = true,
    .limb_hurt      = 190,
    .avoid_die      = 130,
    .block_damtype  = 10,
    .block_tires    = false,
    .dodge_improves = false,
    .limb_eq_first  = true,
    .block          = {"@C$N@W moves quickly and blocks your water spikes!@n", "@WYou move quickly and block @C$n's@W water spikes!@n", "@C$N@W moves quickly and blocks @c$n's@W water spikes!@n"},
    .dodge          = {"@C$N@W manages to dodge your water spikes, letting them slam into the surroundings!@n", "@WYou dodge @C$n's@W water spikes, letting them slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W water spikes, letting them slam into the surroundings!@n"},
    .miss           = {"@WYou can't believe it but your water spikes miss, flying through the air harmlessly!@n", "@C$n@W fire water spikes at you, but misses!@n ", "@c$n@W fire water spikes at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your water spikes miss, flying through the air harmlessly!@n", "@C$n@W fires water spikes at you, but misses!@n", "@c$n@W fires water spikes at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S chest!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR chest!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s chest!@n", true}, {"@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S head!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR head!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s head!@n", true}, {"@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S body!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR body!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s body!@n", true}, {"@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S arm!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR arm!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s arm!@n", true}, {"@CYou slam both your hands together in front of you, palms flat. As you pull them apart ki flows from your palms and forms into a giant ball of water. You raise your hand above your head with the ball of water. You command the water to form several sharp spikes which freeze upon forming. The spikes then launch at @R$N@C and slam into $S leg!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @RYOU@C and slam into YOUR leg!@n", "@c$n@C slams both $s hands together in front of $mself, palms flat. As $e pulls them apart ki flows from the palms and forms into a giant ball of water. Then $e raises $s hand above $s head with the ball of water. The water forms several sharp spikes which freeze instantly as they take shape. The spikes then launch at @R$N@C and slam into $s leg!@n", true}},
    .obj_to_char    = "@WYou fire water spikes at $p@W!@n",
    .obj_to_room    = "@C$n@W fires water spikes at $p@W!@n",
    .bonus_damage   = aqua_barrier,
    .strike         = water_spikes_limb,
    .after_hit      = water_spikes_knockout,
    .after_paying   = water_spikes_refund,
};

const ki_attack KOTEIRU = {
    .skill          = SKILL_KOTEIRU,
    .name           = "Koteiru Bakuha",
    .damtype        = 48,
    .attperc        = .3,
    .minimum        = .1,
    .cooldown       = 7,
    .blockable      = false,
    .avoid_die      = 130,
    .dodge_scatters = false,
    .limb_eq_first  = true,
    .dodge          = {"@C$N@W manages to dodge your Koteiru Bakuha, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Koteiru Bakuha, letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Koteiru Bakuha, letting it fly harmlessly by!@n"},
    .miss           = {"@WYou can't believe it but your Koteiru Bakuha misses, flying through the air harmlessly!@n", "@C$n@W fires a Koteiru Bakuha at you, but misses!@n ", "@c$n@W fires a Koteiru Bakuha at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your Koteiru Bakuha misses, flying through the air harmlessly!@n", "@C$n@W fires a Koteiru Bakuha at you, but misses!@n", "@c$n@W fires a Koteiru Bakuha at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S chest it freezes solid around $M!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR chest it freezes solid around YOU!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S chest it freezes solid around @W$N@C!@n", true}, {"@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S head it freezes solid around $M!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR head it freezes solid around YOU!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S head it freezes solid around @W$N@C!@n", true}, {"@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S gut it freezes solid around $M!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR gut it freezes solid around YOU!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S gut it freezes solid around @W$N@C!@n", true}, {"@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S arm it freezes solid around $M!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR arm it freezes solid around YOU!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S arm it freezes solid around @W$N@C!@n", true}, {"@CYou hold your hands outstretched in front of your body and with your ki begin to create turbulant waters that hover around your body. You begin a sort of dance as you control the water more and more until you have created a huge floating wave. In an instant you swing the wave at @W$N@C! As the wave slams into $S leg it freezes solid around $M!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at YOU! As the wave slams into YOUR leg it freezes solid around YOU!@n", "@c$n@C holds $s hands outstretched in front of $s body and with $s ki begins to create turbulant waters that hover around $s body. @c$n@C begins a sort of dance as $e controls the water more and more until $e has created a huge floating wave. In an instant $e swings the wave at @W$N@C! As the wave slams into $S leg it freezes solid around @W$N@C!@n", true}},
    .obj_to_char    = "@WYou fire Koteiru Bakuha at $p@W!@n",
    .obj_to_room    = "@C$n@W fires Koteiru Bakuha at $p@W!@n",
    .after_paying   = koteiru_freeze,
};

const ki_attack HELL_SPIRAL = {
    .skill          = SKILL_HSPIRAL,
    .name           = "Hell Spiral",
    .damtype        = 49,
    .attperc        = .3,
    .minimum        = .15,
    .cooldown       = 6,
    .limb_hurt      = 120,
    .blockable      = false,
    .avoid_die      = 130,
    .dodge_scatters = false,
    .limb_eq_first  = true,
    .dodge          = {"@C$N@W manages to dodge your Hell Spiral, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Hell Spiral, letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Hell Spiral, letting it fly harmlessly by!@n"},
    .miss           = {"@WYou can't believe it but your Hell Spiral misses, flying through the air harmlessly!@n", "@C$n@W fires a Hell Spiral at you, but misses!@n ", "@c$n@W fires a Hell Spiral at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your Hell Spiral misses, flying through the air harmlessly!@n", "@C$n@W fires a Hell Spiral at you, but misses!@n", "@c$n@W fires a Hell Spiral at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W chest where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true}, {"@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W head where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true}, {"@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W body where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR body where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W body where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true}, {"@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W arm where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true}, {"@WYou hold out your hand, palm upward, while looking toward @C$N@W and grinning as @Rred@W energy begins to pool in the center of your palm. As the large orb of energy swells to about three feet in diameter you move your hand away! You quickly punch the ball of energy and send it flying into @C$N's@W leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward YOU and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into YOUR leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", "@C$n @Wholds out $s hand, palm upward, while looking toward @c$N@W and grinning as @Rred@W energy begins to pool in the center of $s palm. As the large orb of energy swells to about three feet in diameter $e moves $s hand away! Then $e quickly punches the ball of energy and sends it flying into @c$N's@W leg where it @re@Rx@Dp@rl@Ro@Dd@re@Rs@W in a flash of light!@n", true}},
    .obj_to_char    = "@WYou fire Hell Spiral at $p@W!@n",
    .obj_to_room    = "@C$n@W fires Hell Spiral at $p@W!@n",
};

const ki_attack STAR_BREAKER = {
    .skill          = SKILL_BREAKER,
    .name           = "Star Breaker",
    .damtype        = 46,
    .attperc        = .14,
    .minimum        = .05,
    .cooldown       = 6,
    .limb_hurt      = 180,
    .avoid_die      = 130,
    .block_damtype  = 10,
    .block_tires    = false,
    .dodge_improves = false,
    .limb_eq_first  = true,
    .block          = {"@C$N@W moves quickly and blocks your Star Breaker!@n", "@WYou move quickly and block @C$n's@W Star Breaker!@n", "@C$N@W moves quickly and blocks @c$n's@W Star Breaker!@n"},
    .dodge          = {"@C$N@W manages to dodge your Star Breaker, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W Star Breaker, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W Star Breaker, letting it slam into the surroundings!@n"},
    .miss           = {"@WYou can't believe it but your Star Breaker misses, flying through the air harmlessly!@n", "@C$n@W fires a Star Breaker at you, but misses!@n ", "@c$n@W fires a Star Breaker at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your Star Breaker misses, flying through the air harmlessly!@n", "@C$n@W fires a Star Breaker at you, but misses!@n", "@c$n@W fires a Star Breaker at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S chest!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your chest!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S chest!@n", true}, {"@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S head!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your head!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S head!@n", true}, {"@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S body!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your body!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S body!@n", true}, {"@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S arm!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your arm!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S arm!@n", true}, {"@WYou raise your right hand above your head with it slightly cupped. Dark @rred@W energy from the Eldritch Star begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by your ki flows up your left arm as you raise it. You slam both of your hands together, combining the energy, and then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S leg!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then toss a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs your leg!@n", "@C$n@W raises $s right hand above $s head with it slightly cupped. Dark @rred@W energy begins to pool there and form a growing orb of energy. At the same time @mpurple@W arcs of electricity formed by $s ki flows up $s left arm as $e raises it. Suddenly $e slams both of $s hands together, combining the energy, and then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S leg!@n", true}},
    .obj_to_char    = "@WYou fire Star Breaker at $p@W!@n",
    .obj_to_room    = "@C$n@W fires Star Breaker at $p@W!@n",
    .bonus_damage   = star_breaker_theft,
    .after_hit      = star_breaker_blessing,
};

} // namespace

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
      send_to_char(g, ch, "DGE: {:d}\n", dge);
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
      send_to_char(g, char_get_master(g, ch), "@Y{}@C is no longer prepared to combine an attack with the group!@n\r\n", get_i_name(char_get_master(g, ch), ch));
      for (char_data *fol : char_followers(g, char_get_master(g, ch))) {
        if (ch != fol)
          send_to_char(g, fol, "@Y{}@C is no longer prepared to combine an attack with the group!@n\r\n", get_i_name(fol, ch));
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
      send_to_char(g, ch, "You need to have the minimum of 5{} ki charged to combine.\r\n", "%");
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
        send_to_char(g, char_get_master(g, ch), "@BCOMBINE@c: @Y{}@C has prepared to combine a @c'@G{}@c'@C with the next group attack!@n\r\n", get_i_name(char_get_master(g, ch), ch), attack_names[temp]);
        for (char_data *f_ch : char_followers(g, char_get_master(g, ch))) {
          if (ch != f_ch)
            send_to_char(g, f_ch, "@BCOMBINE@c: @Y{}@C has prepared to combine a @c'@G{}@c'@C with the next group attack!@n\r\n", get_i_name(f_ch, ch), attack_names[temp]);
        }
        char_set_combine(ch, temp);
      } else {
        send_to_char(g, ch, "You do not have the minimum 5{} ki charged.\r\n", "%");
        return;
      }
    }
  }
} /* End */

ACMD(do_sunder)
{
  perform_ki_attack(g, ch, argument, SUNDER);
}

ACMD(do_malice)
{
  perform_ki_attack(g, ch, argument, MALICE);
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
  perform_strike(g, ch, argument, HEADBUTT);
}

ACMD(do_bash)
{
  perform_strike(g, ch, argument, BASH);
}

ACMD(do_seishou)
{
  perform_ki_attack(g, ch, argument, SEISHOU);
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
  perform_ki_attack(g, ch, argument, WATER_RAZOR);
}

ACMD(do_spike)
{
  perform_ki_attack(g, ch, argument, WATER_SPIKES);
}

ACMD(do_koteiru)
{
  perform_ki_attack(g, ch, argument, KOTEIRU);
}

ACMD(do_hspiral)
{
  perform_ki_attack(g, ch, argument, HELL_SPIRAL);
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
  perform_ki_attack(g, ch, argument, STAR_BREAKER);
}
