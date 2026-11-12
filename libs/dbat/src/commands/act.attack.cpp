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
#include <algorithm>
#include <span>

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
#include "offense/area.hpp"

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


/* Star nova is better known by day, and stronger by evening */
void nova_by_day(area_shot &s)
{
  if (s.g->time_info.hours <= 15 || s.g->time_info.hours > 22)
    s.skill += 5;
}

void nova_by_evening(area_shot &s)
{
  if (s.g->time_info.hours <= 15)
    s.dmg *= 1.25;
  else if (s.g->time_info.hours <= 22)
    s.dmg *= 1.4;
}

const area_attack STAR_NOVA = {
    .skill          = SKILL_STARNOVA,
    .damtype        = 53,
    .attperc        = .20,
    .minimum        = .1,
    .perfections    = true,
    .worth          = area_fair_game_ungrouped,
    .caught         = area_fair_game_ungrouped,
    .cooldown_first = 6,
    .dodge_low      = -15,
    .vanish_hurts   = true,
    .knockdown      = area_knockdown::always,
    .fail           = {"@WYou gather your charged energy and clench your upheld fists at either side of your body while crouching down. A hot glow of energy begins to form around your body before you lose your concentration and fail to create a @yS@Yt@Wa@wr @cN@Co@Wv@wa@W!@n", nullptr, "@C$n@W gathers $s charged energy and clenches $s upheld fists at either side of $s body while crouching down. A hot glow of energy begins to form around $s body before $e seems to lose $s concentration and fail to create a @yS@Yt@Wa@wr @cN@Co@Wv@wa@W!@n"},
    .launch         = {"@WYou gather your charged energy and clench your upheld fists at either side of your body while crouching down. A hot glow of energy begins to form around your body in the shape of a sphere! Suddenly a shockwave of heat and energy erupts out into the surrounding area as your glorious @yS@Yt@Wa@wr @cN@Co@Wv@wa@W is born!@n", nullptr, "@C$n@W gathers $s charged energy and clenches $s upheld fists at either side of $s body while crouching down. A hot glow of energy begins to form around $s body in the shape of a sphere! Suddenly a shockwave of heat and energy erupts out into the surrounding area as @C$n's@W glorious @yS@Yt@Wa@wr @cN@Co@Wv@wa@W is born!@n"},
    .vanish         = EXPLOSION_VANISH,
    .escape         = EXPLOSION_ESCAPE,
    .hit            = EXPLOSION_HIT,
    .before_roll    = nova_by_day,
    .damage         = nova_by_evening,
};

/*
 * What only mobs do (breath, ram and fang strike) as data, made by perform_beast(). Unlike a strike, they check no
 * skill, say nothing when they can't be made, improve no skill of the user's, and neither end a combo nor lead to a
 * multihit. The user sees none of their messages.
 */
struct beast_attack {
  const char *name{};                                                       /* as the zanzoken messages name it */
  int64_t stamina_share{};                                                  /* it costs a 1/this share of the user's max power level in stamina */
  int64_t needs_share{};                                                    /* and needs a 1/this share to be made */
  int cooldown{};                                                           /* handle_cooldown() */
  int kill_mode{};                                                          /* can_kill()'s last argument, at a victim */
  int accuracy{};                                                           /* added to the roll to hit */
  int64_t (*blocked)(int64_t dmg){};                                        /* a block's damage, from damtype() 6's */
  int64_t (*landed)(int64_t dmg){};                                         /* a hit's, from damtype() 8's, before where it lands */
  int limb_hurt{};                                                          /* hurt()'s limb argument for a hit on an arm or a leg */
  int leg_flag{};                                                           /* hurt()'s first argument for a hit on a leg */
  bool parries{};                                                           /* the victim can parry it with a counterblow */
  act_msgs parry, block, dodge, miss, hits[5];                              /* hits by where roll_hitloc() says it landed */
  const char *obj_to_room{};
  void (*on_block)(GameWorld *g, char_data *ch, char_data *vict){};        /* before a block hurts */
  void (*after_hit)(GameWorld *g, char_data *ch, char_data *vict, int64_t dmg){}; /* after the user pays for a hit */
};

void beast_at_victim(GameWorld *g, char_data *ch, char_data *vict, const beast_attack &atk, int skill, int64_t stcost)
{
  /* dam_eq_loc() for where it lands: 1 arms, 2 legs, 3 head, 4 body; quirk: a hit on the body's first place damages
     the head's equipment, and one on the face the body's */
  static constexpr int equipment[5] = {3, 4, 4, 1, 2};
  int pry = 2, dge = 2, blk = 2;

  if (!can_kill(g, ch, vict, nullptr, atk.kill_mode))
    return;
  if (handle_defender(g, vict, ch))
    vict = char_get_defender(g, vict);
  int index = check_def(g, vict);
  int prob  = roll_accuracy(g, ch, skill, true);
  int perc  = chance_to_hit(g, ch);
  index -= handle_speed(g, ch, vict);
  int avo = index / 4;
  handle_defense(g, vict, &pry, &blk, &dge);
  prob -= avo;
  tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);
  prob += atk.accuracy;

  if (!tech_handle_zanzoken(g, ch, vict, atk.name)) {
    pcost(g, ch, 0, stcost / 2);
    pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
    return;
  }

  if (prob < perc - 20) {
    if (getCurST(g, vict) > 0) { /* with no stamina left, they can only be missed */
      if (atk.parries && pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
        act_triple(g, atk.parry, ch, nullptr, vict);
        improve_skill(g, vict, SKILL_PARRY, 0);
        pcost(g, ch, 0, stcost / 2);
        pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
        int64_t dmg = damtype(g, vict, -2, skill, 0);
        dmg *= calc_critical(g, ch, 1);
        hurt(g, 0, 0, vict, ch, nullptr, dmg, -1); /* the counterblow */
        return;
      }
      if (blk > axion_dice(g, 10)) {
        act_triple(g, atk.block, ch, nullptr, vict);
        improve_skill(g, vict, SKILL_BLOCK, 0);
        pcost(g, ch, 0, stcost / 2);
        pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
        int64_t dmg = atk.blocked(damtype(g, ch, 6, skill, 0));
        if (atk.on_block)
          atk.on_block(g, ch, vict);
        hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
        return;
      }
      if (dge > axion_dice(g, 10)) {
        act_triple(g, atk.dodge, ch, nullptr, vict);
        improve_skill(g, vict, SKILL_DODGE, 0);
        pcost(g, ch, 0, stcost / 2);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
        return;
      }
    }
    act_triple(g, atk.miss, ch, nullptr, vict);
    pcost(g, ch, 0, stcost / 2);
    hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
    return;
  }

  int64_t dmg = atk.landed(damtype(g, ch, 8, skill, 0));
  int where   = roll_hitloc(g, ch, vict, skill);
  if (where >= 1 && where <= 5) {
    act_triple(g, atk.hits[where - 1], ch, nullptr, vict);
    if (where == 2)
      dmg *= calc_critical(g, ch, 0);
    else if (where >= 4)
      dmg *= calc_critical(g, ch, 1);
    else if (char_get_bonuses(ch, BONUS_SOFT))
      dmg *= calc_critical(g, ch, 2);
    hurt(g, where == 5 ? atk.leg_flag : 0, where >= 4 ? atk.limb_hurt : 0, ch, vict, nullptr, dmg, 0);
    dam_eq_loc(g, vict, equipment[where - 1]);
  }
  pcost(g, ch, 0, stcost);
  if (atk.after_hit)
    atk.after_hit(g, ch, vict, dmg);
}

void perform_beast(GameWorld *g, char_data *ch, std::string_view argument, const beast_attack &atk)
{
  int64_t stcost        = getEffMaxPL(g, ch) / atk.stamina_share;
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_is_npc(ch) || !can_grav(g, ch))
    return;
  if (arg.empty() && !char_get_fighting(g, ch))
    return;
  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / atk.needs_share))
    return;
  int skill = init_skill(g, ch, SKILL_KNEE); /* each is rolled as a knee */

  /* who they name, else who they're fighting if they're here, else an object here */
  struct char_data *vict = nullptr;
  struct obj_data *obj   = nullptr;
  if (arg.empty() || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch))
      vict = char_get_fighting(g, ch);
    else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)))))
      return;
  }
  handle_cooldown(g, ch, atk.cooldown);
  if (vict) {
    beast_at_victim(g, ch, vict, atk, skill, stcost);
    return;
  }
  if (!can_kill(g, ch, nullptr, obj, 0)) /* which refuses a broken object */
    return;
  int64_t dmg = getCurPL(g, ch) / 10000 + char_get_aff_abils_str(ch);
  act(g, atk.obj_to_room, true, ch, obj, nullptr, TO_ROOM);
  hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
  pcost(g, ch, 0, stcost);
}

/* A breath can burn its victim */
void breath_burn(GameWorld *g, char_data *ch, char_data *vict)
{
  tech_handle_burn(g, ch, vict, 3);
}

void breath_burn_after(GameWorld *g, char_data *ch, char_data *vict, int64_t)
{
  tech_handle_burn(g, ch, vict, 3);
}

/* A fang strike tires its victim by a quarter of its damage */
void fang_strike_tires(GameWorld *g, char_data *, char_data *vict, int64_t dmg)
{
  decCurST(g, vict, dmg * .25, 0);
}

const beast_attack BREATH = {
    .name          = "breath",
    .stamina_share = 5000,
    .needs_share   = 200,
    .cooldown      = 10,
    .kill_mode     = 1,
    .accuracy      = 15,
    .blocked       = [](int64_t dmg) -> int64_t { return dmg * 0.8; },
    .landed        = [](int64_t dmg) -> int64_t { return dmg + dmg * 2; },
    .block         = {nullptr, "@WYou move quickly and block @C$n's@W fiery breath!@n", "@C$N@W moves quickly and blocks @c$n's@W fiery breath!@n", true},
    .dodge         = {nullptr, "@WYou dodge the fiery jets of flames coming from @C$n's@W mouth!@n", "@C$N@W manages to dodge the fiery jets of flames coming from @c$n's@W mouth!@n", true},
    .miss          = {nullptr, "@C$n@W moves to breath flames on you, but misses!@n", "@c$n@W moves to breath flames on @C$N@W, but somehow misses!@n", true},
    .hits          = {{nullptr, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR body!@n", "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W body!@n", true}, {nullptr, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR face!@n", "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W face!@n", true}, {nullptr, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR body!@n", "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W body!@n", true}, {nullptr, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR arm!@n", "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W arm!@n", true}, {nullptr, "@C$n@W aims $s mouth at you and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto YOUR leg!@n", "@C$n@W aims $s mouth at @c$N@W and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto @c$N's@W leg!@n", true}},
    .obj_to_room   = "@C$n@W breathes flames on $p@W!@n",
    .on_block      = breath_burn,
    .after_hit     = breath_burn_after,
};

const beast_attack RAM = {
    .name          = "ram",
    .stamina_share = 200,
    .needs_share   = 200,
    .cooldown      = 4,
    .accuracy      = -5,
    .blocked       = [](int64_t dmg) -> int64_t { return dmg - dmg * 0.2; },
    .landed        = [](int64_t dmg) -> int64_t { return dmg + dmg * 1.1; },
    .limb_hurt     = 190,
    .leg_flag      = 1,
    .block         = {nullptr, "@WYou move quickly and block @C$n's@W body as $e tries to ram YOU!@n", "@C$N@W moves quickly and blocks @c$n's@W body as $e tries to ram $M!@n", true},
    .dodge         = {nullptr, "@WYou dodge @C$n's@W attempted ram!@n", "@C$N@W manages to dodge @c$n's@W attempted ram!@n", true},
    .miss          = {nullptr, "@C$n@W moves to ram you, but misses!@n", "@c$n@W moves to ram @C$N@W, but somehow misses!@n", true},
    .hits          = {{nullptr, "@C$n@W aims $s body at YOU and rams into YOUR body!@n", "@c$n@W aims $s body at @C$N@W and rams into $S body!@n", true}, {nullptr, "@C$n@W aims $s body at YOU and rams into YOUR face!@n", "@c$n@W aims $s body at @C$N@W and rams into $S face!@n", true}, {nullptr, "@C$n@W aims $s body at YOU and rams into YOUR body!@n", "@c$n@W aims $s body at @C$N@W and rams into $S body!@n", true}, {nullptr, "@C$n@W aims $s body at YOU and rams into YOUR arm!@n", "@c$n@W aims $s body at @C$N@W and rams into $S arm!@n", true}, {nullptr, "@C$n@W aims $s body at YOU and rams into YOUR leg!@n", "@c$n@W aims $s body at @C$N@W and rams into $S leg!@n", true}},
    .obj_to_room   = "@C$n@W rams $p@W extremely hard!@n",
};

const beast_attack FANG_STRIKE = {
    .name          = "fang strike",
    .stamina_share = 400,
    .needs_share   = 400,
    .cooldown      = 4,
    .accuracy      = 5,
    .blocked       = [](int64_t dmg) -> int64_t { return dmg / 4; },
    .landed        = [](int64_t dmg) -> int64_t { return dmg + dmg * 0.5; },
    .parries       = true,
    .parry         = {nullptr, "@WYou parry @C$n's@W fang strike with a punch of your own!@n", "@C$N@W parries @c$n's@W fang strike with a punch of $S own!@n", true},
    .block         = {nullptr, "@WYou move quickly and block @C$n's@W fang strike!@n", "@C$N@W moves quickly and blocks @c$n's@W fang strike!@n", true},
    .dodge         = {nullptr, "@WYou dodge @C$n's@W fang strike!@n", "@C$N@W manages to dodge @c$n's@W fang strike!@n", true},
    .miss          = {nullptr, "@C$n@W moves to fang strike you, but misses!@n", "@c$n@W moves to fang strike @C$N@W, but somehow misses!@n", true},
    .hits          = {{nullptr, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR body!@n", "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S body!@n", true}, {nullptr, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR face!@n", "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S face!@n", true}, {nullptr, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR body!@n", "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S body!@n", true}, {nullptr, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR arm!@n", "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S arm!@n", true}, {nullptr, "@C$n@W launches $s body at YOU and sinks $s fang strike into YOUR leg!@n", "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S leg!@n", true}},
    .obj_to_room   = "@C$n@W fang strikes $p@W extremely hard!@n",
    .after_hit     = fang_strike_tires,
};

} // namespace

/* Light grenade is aimed at one, and its explosion catches the rest for half */
ACMD(do_lightgrenade)
{
  static constexpr act_msgs fail     = {"@WYou quickly bring your hands in front of your body and cup them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as your ki is condensed between your hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Your concentration waivers however and the energy slips away from your control harmlessly!@n", "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Suddenly $s concentration seems to waiver and the energy slips away from $s control harmlessly!@n", "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. Suddenly $s concentration seems to waiver and the energy slips away from $s control harmlessly!@n", true};
  static constexpr act_msgs launch   = {"@WYou quickly bring your hands in front of your body and cup them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as your ki is condensed between your hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. You shout @r'@YLIGHT GRENADE@r'@W as the orb launches from your hands at @C$N@W!@n", "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. @C$n shouts @r'@YLIGHT GRENADE@r'@W as the orb launches from $s hands at YOU!@n", "@C$n@W quickly brings $s hands in front of $s body and cups them a short distance from each other. A flash of @Ggreen @Ylight@W can be seen as ki is condensed between $s hands before a @Yg@yo@Yl@yd@Ye@yn@W orb of ki replaces the green light. @C$n shouts @r'@YLIGHT GRENADE@r'@W as the orb launches from $s hands at @c$N@W!@n", true};
  static constexpr act_msgs dodged   = {"@c$N@W manages to dodge the light grenade!@n", "@WYou manages to dodge the light grenade!@n", "@c$N@W manages to dodge the light grenade!@n", true};
  static constexpr act_msgs hit      = {"@R$N@r is hit by the light grenade which explodes all around $m!@n", "@RYou are hit by the light grenade which explodes all around you!@n", "@R$N@r is hit by the light grenade which explodes all around $m!@n", true};
  static constexpr act_msgs caught   = {"@R$N@r is caught by the light grenade's explosion!@n", "@RYou are caught by the light grenade's explosion!@n", "@R$N@r is caught by the light grenade's explosion!@n", true};
  double attperc                     = .35, minimum = .15;
  std::string_view rest              = argument;
  std::string arg                    = take_arg(rest);
  std::string arg2                   = take_arg(rest);
  struct char_data *targ             = nullptr;

  if (!can_grav(g, ch) || !check_skill(g, ch, SKILL_LIGHTGRENADE) || !limb_ok(g, ch, 0))
    return;
  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc))
    return;
  if (getMaxKI(g, ch) * attperc > char_get_charge(ch))
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0))
    return;
  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (arg.empty() || !(targ = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (!char_get_fighting(g, ch) || char_get_in_room(char_get_fighting(g, ch)) != char_get_in_room(ch)) {
      send_to_char(g, ch, "Nobody around here by that name.\r\n");
      return;
    }
    targ = char_get_fighting(g, ch);
  }

  int skill = init_skill(g, ch, SKILL_LIGHTGRENADE);
  if (skill < chance_to_hit(g, ch)) {
    act_triple(g, fail, ch, nullptr, targ);
    pcost(g, ch, attperc, 0);
    improve_skill(g, ch, SKILL_LIGHTGRENADE, 0);
    return;
  }
  act_triple(g, launch, ch, nullptr, targ);
  int64_t dmg = damtype(g, ch, 57, skill, attperc);

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch || char_mob_flagged(vict, MOB_NOKILL))
      continue;
    if (char_aff_flagged(vict, AFF_GROUP) &&
        (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict))) {
      if (vict == targ)
        send_to_char(g, ch, "Leave the group if you want to murder them.\r\n");
      continue;
    }
    if (!area_fair_game(g, ch, vict))
      continue;
    int dge = handle_dodge(g, vict);
    if (area_vanishes(g, ch, vict, EXPLOSION_VANISH)) {
      if (vict == targ) { /* which ends it */
        pcost(g, ch, attperc, 0);
        handle_cooldown(g, ch, 9);
        return;
      }
      continue;
    }
    /* quirk: whether they escape is rolled first for the target, then for anyone else, so twice for the rest */
    if (dge > axion_dice(g, skill * 0.5) && vict == targ) {
      send_to_char(g, ch, "DGE: {:d}\n", dge); /* quirk: left from debugging */
      act_triple(g, dodged, ch, nullptr, vict);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      improve_skill(g, vict, SKILL_DODGE, 0);
      pcost(g, ch, attperc, 0);
      handle_cooldown(g, ch, 9);
      return;
    }
    if (dge > axion_dice(g, skill * 0.5) && vict != targ) {
      act_triple(g, EXPLOSION_ESCAPE, ch, nullptr, vict);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      improve_skill(g, vict, SKILL_DODGE, 0);
      continue;
    }
    act_triple(g, vict == targ ? hit : caught, ch, nullptr, vict);
    if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 4) == 4)
      handle_knockdown(g, vict);
    if (vict == targ)
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
    else
      hurt(g, 0, 0, ch, vict, nullptr, dmg * 0.5, 1);
  }
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
    if (char_get_skill(ch, SKILL_FOCUS) < 30) {
      send_to_char(g, ch, "You need a Focus skill level of 30 to figure out the basics of this technique.\r\n");
      return;
    }
    char_set_skills_base(ch, SKILL_ENERGIZE, rand_number(g, 10, 14));
    send_to_char(g, ch, "You learn the basics for energizing thrown weapons! Now use the energize command again.\r\n");
    return;
  }
  bool on = !char_prf_flagged(g, ch, PRF_ENERGIZE);
  send_to_char(g, ch, "{}\r\n", on ? "You start focusing your latent ki into your fingertips." : "You stop focusing ki into your fingertips.");
  char_prf_set(g, ch, PRF_ENERGIZE, on);
}

ACMD(do_breath)
{
  perform_beast(g, ch, argument, BREATH);
}

ACMD(do_ram)
{
  perform_beast(g, ch, argument, RAM);
}

ACMD(do_strike)
{
  perform_beast(g, ch, argument, FANG_STRIKE);
}

/* End NPC skills */

namespace {

void combine_syntax(GameWorld *g, struct char_data *ch, const char *cancel)
{
  send_to_char(g, ch, "Follower Syntax: combine (attack)\r\n");
  send_to_char(g, ch, "Leader Syntax: combine (attack) (target)\r\n");
  send_to_char(g, ch, "{} Syntax: combine stop\r\n", cancel);
}

/* say(to) to ch's leader, then to each of the leader's other followers */
template <class Say> void tell_group(GameWorld *g, struct char_data *ch, Say say)
{
  struct char_data *leader = char_get_master(g, ch);

  say(leader);
  for (char_data *fol : char_followers(g, leader))
    if (fol != ch)
      say(fol);
}

} // namespace

/* A follower readies an attack, and the leader, aiming, makes it with the attacks readied */
ACMD(do_combine)
{
  std::string_view rest    = argument;
  std::string arg          = take_arg(rest);
  std::string arg2         = take_arg(rest);
  struct char_data *leader = char_get_master(g, ch);

  if (!has_group(g, ch)) {
    send_to_char(g, ch, "You need to be in a group!\r\n");
    return;
  }
  if (arg.empty() || (!leader && arg2.empty())) {
    combine_syntax(g, ch, "Cancel");
    return;
  }
  if (str_eq_nocase(arg, "stop")) {
    if (!leader) {
      send_to_char(g, ch, "You do not need to stop as you haven't prepared anything.\r\n");
      return;
    }
    if (char_get_combine(ch) == -1) {
      send_to_char(g, ch, "You are not trying to combine any attacks...\r\n");
      return;
    }
    send_to_char(g, ch, "You stop your preparations to combine your attack with a group attack.\r\n");
    tell_group(g, ch, [&](struct char_data *to) {
      send_to_char(g, to, "@Y{}@C is no longer prepared to combine an attack with the group!@n\r\n", get_i_name(to, ch));
    });
    char_set_combine(ch, -1);
    return;
  }

  /* quirk: the attack is the first word with its name in it, so those of two words (twin slash and the
     renzokou energy dan, and with them the sword and Nail's student checks) are never found */
  std::span<const char *const> names(attack_names, 14);
  auto named = std::ranges::find_if(names, [&](const char *name) { return arg.find(name) != std::string::npos; });
  if (named == names.end()) {
    combine_syntax(g, ch, "Follower Cancel");
    return;
  }
  int attack = named - names.begin();
  if (attack == 5) { /* twin slash */
    if (!char_get_equipment(ch, WEAR_WIELD1)) {
      send_to_char(g, ch, "You need to wield a sword to use this technique.\r\n");
      return;
    }
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
      send_to_char(g, ch, "You are not wielding a sword, you need one to use this technique.\r\n");
      return;
    }
  }
  if (!char_get_skill(ch, attack_skills[attack])) {
    send_to_char(g, ch, "You do not know that skill.\r\n");
    return;
  }
  if (attack_skills[attack] == 440 && !char_is_nail(ch)) {
    send_to_char(g, ch, "Only students of Nail know how to combine that attack effectively.\r\n");
    return;
  }
  bool charged = char_get_charge(ch) >= getMaxKI(g, ch) * 0.05;
  if (!charged) /* quirk: and goes on */
    send_to_char(g, ch, "You need to have the minimum of 5{} ki charged to combine.\r\n", "%");

  if (!leader) {
    struct char_data *vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM);
    if (!vict) {
      send_to_char(g, ch, "Who will your combined attack be targeting?\r\n");
      return;
    }
    if (vict == ch) {
      send_to_char(g, ch, "No targeting yourself...\r\n");
      return;
    }
    char_set_combine(ch, attack);
    bool readied = false;
    for (char_data *fol : char_followers(g, ch))
      if (char_aff_flagged(fol, AFF_GROUP) && char_get_combine(fol) != -1 && char_get_charge(fol) >= getMaxKI(g, fol) * 0.05) {
        readied = true;
        break;
      }
    if (readied)
      combine_attacks(g, ch, vict);
    else
      send_to_char(g, ch, "You do not have any followers who have readied an attack to combine or they do not have enough ki anymore to combine said attack.\r\n");
    return;
  }

  if (!charged) {
    send_to_char(g, ch, "You do not have the minimum 5{} ki charged.\r\n", "%");
    return;
  }
  act(g, "@C$n@c appears to be concentrating hard and focusing $s energy!@n\r\n", true, ch, nullptr, nullptr, TO_ROOM);
  tell_group(g, ch, [&](struct char_data *to) {
    send_to_char(g, to, "@BCOMBINE@c: @Y{}@C has prepared to combine a @c'@G{}@c'@C with the next group attack!@n\r\n", get_i_name(to, ch), attack_names[attack]);
  });
  char_set_combine(ch, attack);
}

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
  perform_area_attack(g, ch, argument, STAR_NOVA);
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

namespace {

/* A thrown object's weapon level adds a share of its damage, and later a flat amount */
constexpr int THROW_LEVEL_FLAGS[]     = {ITEM_WEAPLVL1, ITEM_WEAPLVL2, ITEM_WEAPLVL3, ITEM_WEAPLVL4, ITEM_WEAPLVL5};
constexpr double THROW_LEVEL_SHARE[]  = {0.1, 0.2, 0.3, 0.4, 0.5};
constexpr int THROW_LEVEL_BONUS[]     = {1000, 5000, 10000, 16000, 25000};

/* What a thrown object is, by the messages it gets: anything but a weapon, a weapon by its damage type (in this
   order, from 1), or another weapon */
constexpr int THROWN_THING = 0, THROWN_BLASTING = 5, THROWN_OTHER = 6;
constexpr int THROWN_DAMTYPES[] = {TYPE_PIERCE, TYPE_SLASH, TYPE_CRUSH, TYPE_STAB, TYPE_BLAST};

int thrown_kind(struct obj_data *obj)
{
  if (obj_get_type_flag(obj) != ITEM_WEAPON)
    return THROWN_THING;
  auto found = std::ranges::find(THROWN_DAMTYPES, obj_get_value(obj, VAL_WEAPON_DAMTYPE) + TYPE_HIT);
  return found == std::end(THROWN_DAMTYPES) ? THROWN_OTHER : 1 + (found - std::begin(THROWN_DAMTYPES));
}

/* How much a throw wears it, by what it is made of */
struct throw_wear_by {
  int material, low, high;
};
constexpr throw_wear_by THROW_WEAR[] = {
    {MATERIAL_STEEL,   5,  30},
    {MATERIAL_IRON,    18, 50},
    {MATERIAL_MITHRIL, 5,  15},
    {MATERIAL_KACHIN,  5,  15},
    {MATERIAL_STONE,   20, 50},
    {MATERIAL_DIAMOND, 5,  20},
};

int throw_wear(GameWorld *g, struct obj_data *obj)
{
  int material = obj_get_value(obj, VAL_ALL_MATERIAL);

  if (material == MATERIAL_ENERGY)
    return rand_number(g, 1, 2) == 2 ? 0 : rand_number(g, 1, 3);
  auto wear = std::ranges::find(THROW_WEAR, material, &throw_wear_by::material);
  return wear == std::end(THROW_WEAR) ? rand_number(g, 90, 100) : rand_number(g, wear->low, wear->high);
}

/* An object thrown: to_char, to_vict and to_room, which act_triple_vict_last() sends to the victim last */
constexpr act_msgs THROWN[] = {
    {"You throw $p at $N@n full speed, and watch it smash into $M!",
     "$n@n throws $p at you full speed. You reel as it smashes into your body!",
     "$n@n throws $p at $N@n full speed, and watches it smash into $M!", true},
    {"You pull out and throw $p at $N@n full speed, and watch it sink into $M!",
     "$n@n pulls out and throws $p at you full speed. You reel as it sink into your body!",
     "$n@n pulls out and throws $p at $N@n full speed, and watches it sink into $M!", true},
    {"You pull out and throw $p at $N@n full speed, and watch it sink into $M!",
     "$n@n pulls out and throws $p at you full speed. You reel as it sink into your body!",
     "$n@n pulls out and throws $p at $N@n full speed, and watches it sink into $M!", true},
    {"You swing $p overhead and throw it at $N@n full speed, and watch it slam into $M!",
     "$n@n swings $p overhead and throws it at you full speed. You reel as it slam into your body!",
     "$n@n swings $p overhead and throws it at $N@n full speed, and watches it slam into $M!", true},
    {"You bring $p over your shoulder and throw it at $N@n full speed, and watch it sink into $M!",
     "$n@n brings $p over $s shoulder and throws $p at you full speed. You reel as it sink into your body!",
     "$n@n brings $p over $s shoulder and throws it at $N@n full speed, and watches it sink into $M!", true},
    {"You pull out and throw $p at $N@n full speed, and watch it hit $M!",
     "$n@n pulls out and throws $p at you full speed. You reel as it hits your body!",
     "$n@n pulls out and throws $p at $N@n full speed, and watches it hit $M!", true},
    {}, /* quirk: another weapon says nothing */
};
constexpr act_msgs THROWN_ENERGIZED = {
    "You charge $p with the energy in your fingertips! As it begins to @Yglow a bright hot @Rred@n you throw $p at $N@n full speed, and watch it smash into $M!",
    "$n@n charges $p with the energy in $s fingertips! As it begins to @Yglow a bright hot @Rred@n $e throws $p at YOU@n full speed, and watches it smash into YOU!!",
    "$n@n charges $p with the energy in $s fingertips! As it begins to @Yglow a bright hot @Rred@n $e throws $p at $N@n full speed, and watches it smash into $M!", true};
constexpr act_msgs THROWN_MELTS   = {"You throw $p at $N@n, but it melts before touching $M!",
                                     "$n@n throws $p at you, but it melts before touching you!",
                                     "$n@n throws $p at $N@n, but it melts before touching $M!", true};
constexpr act_msgs THROWN_DODGED  = {"You throw $p at $N@n, but $E manages to dodge it easily!",
                                     "$n@n throws $p at you, but you easily dodge it.",
                                     "$n@n throws $p at $N@n, but $E manages to dodge it easily!", true};
constexpr act_msgs THROWN_MISSES  = {"You throw $p at $N@n, but unfortunatly miss!",
                                     "$n@n throws $p at you, but thankfully misses you.",
                                     "$n@n throws $p at $N@n, but unfortunatly misses!", true};
constexpr act_msgs THROWN_BREAKS  = {"You smile as $p breaks on $N's@n face!", "$n@n smiles as $p breaks on your face!",
                                     "$n@n smiles as $p breaks on $N's@n face!", true};
/* and a critical hit, by what it hits (these to_char, to_vict, then to_room) */
constexpr act_msgs THROWN_METAL   = {"@RSome pieces of metal are sent flying!@n", "@RSome pieces of metal are sent flying!@n",
                                     "@RSome pieces of metal are sent flying!@n", true};
constexpr act_msgs THROWN_GOO     = {"@RA wide hole is left in $S gooey flesh!@n", "@RA wide hole is left is your gooey flesh!@n",
                                     "@RA wide hole is left in $N@R's gooey flesh@n", true};
constexpr act_msgs THROWN_BLOOD   = {"@RBlood flies out from the impact!@n", "@RBlood flies out from the impact!@n",
                                     "@RBlood flies out from the impact!@n", true};
constexpr act_msgs THROWN_BURNS   = {"@R$N@R is burned by it!@n", "@RYou are burned by it!@n", "@R$N@R is burned by it!@n", true};

/* After a throw that didn't vanish: it tires, and if it was thrown well enough (better unarmed), it is thrown
   again, at most the times rolled, by do_throw() as typed */
void throw_follow_up(GameWorld *g, struct char_data *ch, std::string_view argument, struct char_data *vict,
                     int64_t weight, int perc, int unarmed, int needed, bool multithrow)
{
  decCurST(g, ch, ((getEffMaxPL(g, ch) / 200) + weight), 0);
  if (!char_get_equipment(ch, WEAR_WIELD1) && !char_get_equipment(ch, WEAR_WIELD2))
    perc += unarmed;
  int throws = char_get_throws(ch);
  if (perc + char_get_aff_abils_cha(ch) >= needed && multithrow && getCurPL(g, vict) > 1 && throws >= 1) {
    do_throw(g, ch, argument, 0, 0);
    char_set_throws(ch, throws > 1 ? char_get_throws(ch) - 1 : -1);
  } else
    char_set_throws(ch, 0);
  char_set_wait(ch, PULSE_3SEC);
}

void throw_object(GameWorld *g, struct char_data *ch, std::string_view argument, struct obj_data *obj,
                  struct char_data *vict, const std::string &how)
{
  int64_t weight = obj_get_weight(obj);
  room_vnum here = char_get_in_room(ch);

  if ((getCurST(g, ch)) < ((getEffMaxPL(g, ch) / 200) + weight)) {
    send_to_char(g, ch, "You do not have enough stamina to do it...\r\n");
    return;
  }
  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(g, ch, "That is broken and useless to throw!\r\n");
    return;
  }
  if (weight + room_get_gravity(g, here) > getMaxCarryWeight(g, ch)) {
    send_to_char(g, ch, "The gravity has made that too heavy for you to throw!\r\n");
    return;
  }
  int penalty = 0, chance = axion_dice(g, 0) + axion_dice(g, 0);
  bool multithrow = true;
  handle_cooldown(g, ch, 5);
  improve_skill(g, ch, SKILL_THROW, 0);

  int64_t damage = ((weight / 3) * (char_get_aff_abils_str(ch)) * (char_get_aff_abils_cha(ch) / 3)) + (getEffMaxPL(g, ch) * 0.01);
  damage += (damage * 0.01) * (room_get_gravity(g, here) / 4);
  if (char_get_preference(ch) == PREFERENCE_THROWING)
    chance -= chance * 0.25;
  int level = 1; /* quirk: one of no level counts as level 1 below, though it gets no share for it here */
  for (int i = 0; i < 5; i++)
    if (obj_extra_flagged(obj, THROW_LEVEL_FLAGS[i])) {
      damage += damage * THROW_LEVEL_SHARE[i];
      level = i + 1;
      break;
    }
  int kind = thrown_kind(obj);
  if (kind == THROWN_BLASTING) {
    damage = ((weight * char_get_aff_abils_str(ch)) * (char_get_aff_abils_cha(ch) / 3)) + (getEffMaxPL(g, ch) * 0.01);
    damage += room_get_gravity(g, here) * (room_get_gravity(g, here) / 2);
  }
  int odam = throw_wear(g, obj);
  if (!obj_extra_flagged(obj, ITEM_THROW)) {
    penalty    = 15;
    multithrow = false;
    damage     = damage * 0.45;
  } else {
    odam = rand_number(g, 0, 1);
    damage += (char_get_aff_abils_str(ch)) * ((getCurPL(g, ch) * 0.00012) + rand_number(g, 1, 20));
    damage += level * (damage * 0.1);
  }
  damage += THROW_LEVEL_BONUS[level - 1];
  if (kind != THROWN_THING && kind != THROWN_BLASTING && odam > 1)
    odam = 1;

  int perc  = init_skill(g, ch, SKILL_THROW);
  int perc2 = init_skill(g, vict, SKILL_DODGE);
  int prob  = axion_dice(g, penalty);
  if (!how.empty()) {
    if (!str_eq_nocase(how, "1") && !str_eq_nocase(how, "single")) {
      send_to_char(g, ch, "Syntax: throw (obj | character) (target) <-- This will multithrow if able\nSyntax: throw (obj) (target) (1 | single) <-- This will not multi throw)\r\n");
      return;
    }
    multithrow = false;
  }

  if (!tech_handle_zanzoken(g, ch, vict, "$p")) {
    char_set_combo(ch, -1);
    char_set_combhits(ch, 0);
    int stcost = ((getEffMaxPL(g, ch) / 200) + weight);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    pcost(g, ch, 0, stcost / 2);
    pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(vict));
    return;
  }

  if (perc - (perc2 / 10) < prob) {
    if (obj_extra_flagged(obj, ITEM_ICE) && char_is_demon(vict)) {
      act_triple_vict_last(g, THROWN_MELTS, ch, obj, vict);
      decCurST(g, ch, ((getEffMaxPL(g, ch) / 100) + weight), 0);
      extract_obj(g, obj);
      return;
    }
    act_triple_vict_last(g, perc2 > 0 ? THROWN_DODGED : THROWN_MISSES, ch, obj, vict);
    decCurST(g, ch, ((getEffMaxPL(g, ch) / 100) + weight), 0);
    if (!obj_extra_flagged(obj, ITEM_UNBREAKABLE))
      obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (odam / 2));
    char_set_lastattack(ch, -50);
    hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(vict));
    throw_follow_up(g, ch, argument, vict, weight, perc, 20, chance + penalty, multithrow);
    return;
  }

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_ENERGIZE) && (getCurKI(g, ch)) >= getMaxKI(g, ch) * 0.02) {
    damage += (damage * (0.0016 * char_get_skill(ch, SKILL_ENERGIZE)));
    act_triple_vict_last(g, THROWN_ENERGIZED, ch, obj, vict);
    if (getMaxKI(g, ch) * 0.02 > 0)
      decCurKI(g, ch, getMaxKI(g, ch) * .02, 0);
    else
      decCurKI(g, ch, 1, 0);
    improve_skill(g, ch, SKILL_ENERGIZE, 0);
  } else
    act_triple_vict_last(g, THROWN[kind], ch, obj, vict);
  if (!obj_extra_flagged(obj, ITEM_UNBREAKABLE))
    obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) - (odam));
  char_set_lastattack(ch, -50);
  /* quirk: the wear counts twice here */
  if ((obj_get_value(obj, VAL_ALL_HEALTH) - odam) <= 0 && !obj_extra_flagged(obj, ITEM_UNBREAKABLE)) {
    act_triple_vict_last(g, THROWN_BREAKS, ch, obj, vict);
    obj_extra_toggle(obj, ITEM_BROKEN);
  } else if (char_get_aff_abils_dex(ch) >= axion_dice(g, 0)) {
    act_triple(g, char_is_android(vict) || char_is_mechanical(vict) ? THROWN_METAL : char_is_majin(vict) ? THROWN_GOO : THROWN_BLOOD,
               ch, nullptr, vict);
    if (obj_extra_flagged(obj, ITEM_ICE) && !char_is_android(vict) && !char_is_icer(vict)) {
      decCurST(g, vict, (getMaxST(g, vict) * .005) + weight, 0);
      act(g, "@mYou lose some stamina to the @ccold@m!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@C$N@m loses some stamina to the @ccold@m!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$N@m loses some stamina to the @ccold@m!@n", true, ch, nullptr, vict, TO_NOTVICT);
    }
    damage *= calc_critical(g, ch, 0);
  }
  if (obj_extra_flagged(obj, ITEM_HOT) && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
    act_triple(g, THROWN_BURNS, ch, nullptr, vict);
    char_aff_set(vict, AFF_BURNED, true);
    damage += damage * 0.4;
  }
  if (char_get_preference(ch) == PREFERENCE_KI)
    damage -= damage * 0.20;
  if (obj_get_vnum(g, obj) == 5899 || obj_get_vnum(g, obj) == 5898)
    damage *= 0.35;
  hurt(g, 0, 0, ch, vict, nullptr, damage, 0);
  obj_from_char(g, obj);
  obj_to_room(g, obj, char_get_in_room(vict));
  throw_follow_up(g, ch, argument, vict, weight, perc, 12, chance + penalty, multithrow);
}

/* Someone thrown at someone else: to_char, to_vict and to_room, as the thrower grabs (or the one thrown), then as
   the one thrown flies (or the target) */
constexpr act_msgs THROW_SLIPS   = {"@WYou try to grab @C$N@W and throw them, but they manage to dodge your attempt!@n",
                                    "@C$n@W tries to @RGRAB@W you and @RTHROW@W you, but you manage to dodge the attempt!@n",
                                    "@C$n@W tries to @RGRAB@W @c$N@W and @RTHROW@W $M, but $E manages to dodge the attempt!@n", true};
constexpr act_msgs THROW_GRABS   = {"@WYou grab @C$N@W and spinning around quickly you throw $M!@n",
                                    "@C$n@W grabs YOU and spinning around quickly $e throws you!@n",
                                    "@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n", true};
constexpr act_msgs FLUNG_DODGED  = {"@WThrown through the air, YOU fly at @c$N@W, but $E manages to dodge and you manage recover your bearings a moment later!@n",
                                    "@WThrown through the air, @C$n@W flies at YOU, but you manage to dodge and @C$n@W recovers $s bearings a moment later!@n",
                                    "@WThrown through the air, @C$n@W flies at @c$N@W, but $E manages to dodge and @C$n@W recovers $s bearingsa moment later!@n", true};
constexpr act_msgs FLUNG_MISSES  = {"@WThrown through the air, YOU fly at @c$N@W, but the throw is a miss! You manage recover your bearings a moment later!@n",
                                    "@WThrown through the air, @C$n@W flies at YOU, but the throw is a miss! @C$n@W recovers $s bearings a moment later!@n",
                                    "@WThrown through the air, @C$n@W flies at @c$N@W, but the throw is a miss! @C$n@W recovers $s bearingsa moment later!@n", true};
constexpr act_msgs FLUNG_HITS    = {"@WThrown through the air, YOU fly at @c$N@W and smash into $M!@n",
                                    "@WThrown through the air, @C$n@W flies at YOU and smashes into YOU!@n",
                                    "@WThrown through the air, @C$n@W flies at @c$N@W and smashes into $M!@n", true};

/* Names who is hurt next, to a and b, and to them */
void throw_banner(GameWorld *g, struct char_data *a, struct char_data *b, struct char_data *who)
{
  act(g, "@W--@R$N@W--@n", true, a, nullptr, who, TO_CHAR);
  act(g, "@W--@R$N@W--@n", true, b, nullptr, who, TO_CHAR);
  act(g, "@W--@RYOU@W--@n", true, who, nullptr, nullptr, TO_CHAR);
}

void throw_character(GameWorld *g, struct char_data *ch, struct char_data *tch, struct char_data *vict)
{
  if (tch == vict) {
    send_to_char(g, ch, "You can't throw someone at theirself.\r\n");
    return;
  }
  if (!can_kill(g, ch, tch, nullptr, 0)) {
    send_to_char(g, ch, "The one you are throwing can't be harmed.\r\n");
    return;
  }
  bool grab = char_get_speedi(g, tch) < char_get_speedi(g, ch) && rand_number(g, 1, 106) < char_get_skill(ch, SKILL_THROW);
  if ((getCurST(g, ch)) < ((getEffMaxPL(g, ch) / 100) + char_get_pc_weight(g, tch))) {
    send_to_char(g, ch, "You do not have enough stamina to do it...\r\n");
    return;
  }
  if (char_get_pc_weight(g, tch) + room_get_gravity(g, char_get_in_room(ch)) > getMaxCarryWeight(g, ch)) {
    send_to_char(g, ch, "The gravity has made them too heavy for you to throw!\r\n");
    return;
  }
  if (!grab) {
    act_triple(g, THROW_SLIPS, ch, nullptr, tch);
    hurt(g, 0, 0, ch, tch, nullptr, 0, 0);
    handle_cooldown(g, ch, 5);
    decCurST(g, ch, (getEffMaxPL(g, ch) / 200) + char_get_pc_weight(g, tch), 0);
    return;
  }
  handle_cooldown(g, ch, 5);
  improve_skill(g, ch, SKILL_THROW, 0);
  int64_t damage = ((char_get_pc_weight(g, tch) * char_get_aff_abils_str(ch)) * (char_get_aff_abils_cha(ch) / 3)) + (getEffMaxPL(g, ch) / 100);
  damage += room_get_gravity(g, char_get_in_room(ch)) * (room_get_gravity(g, char_get_in_room(ch)) / 2);
  int perc  = init_skill(g, ch, SKILL_THROW);
  int perc2 = init_skill(g, vict, SKILL_DODGE);
  int prob  = rand_number(g, 1, 106);
  bool hits = perc - (perc2 / 10) >= prob;

  act_triple(g, THROW_GRABS, ch, nullptr, tch);
  act_triple(g, hits ? FLUNG_HITS : perc2 > 0 ? FLUNG_DODGED : FLUNG_MISSES, tch, nullptr, vict);
  if (!hits)
    decCurST(g, ch, ((getEffMaxPL(g, ch) / 100) + char_get_pc_weight(g, tch)), 0);
  throw_banner(g, ch, tch, vict);
  hurt(g, 0, 0, ch, vict, nullptr, hits ? damage : 0, 0);
  throw_banner(g, ch, vict, tch);
  hurt(g, 0, 0, ch, tch, nullptr, hits ? damage : 0, 0);
  if (!hits)
    return;
  decCurST(g, ch, ((getEffMaxPL(g, ch) / 200) + char_get_pc_weight(g, tch)), 0);
  char_set_wait(ch, PULSE_3SEC);
}

} // namespace

/* Throws something carried, or someone in the room, at someone */
ACMD(do_throw)
{
  char_set_throws(ch, rand_number(g, 1, 3)); /* (set by the interpreter before, for any "throw") */
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string arg2      = take_arg(rest);
  std::string arg3      = take_arg(rest);

  if (playing_song(g, ch))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Throw what?\r\n");
    return;
  }
  if (is_sparring(ch)) {
    send_to_char(g, ch, "You can not spar with throw.\r\n");
    return;
  }
  struct obj_data *obj  = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  struct char_data *tch = obj ? nullptr : get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!obj && !tch) {
    send_to_char(g, ch, "You do not have that object or character to throw!\r\n");
    return;
  }
  struct char_data *vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM);
  if (!vict) {
    struct char_data *foe = char_get_fighting(g, ch);
    if (!foe || char_get_in_room(foe) != char_get_in_room(ch)) {
      send_to_char(g, ch, "Who do you want to target?\r\n");
      return;
    }
    vict = foe;
  }
  if (getCurPL(g, vict) <= 1 || !can_kill(g, ch, vict, nullptr, 1))
    return;
  if (handle_defender(g, vict, ch))
    vict = char_get_defender(g, vict);

  if (obj)
    throw_object(g, ch, argument, obj, vict, arg3);
  else
    throw_character(g, ch, tch, vict);
}

namespace {

void selfd_calms(GameWorld *g, struct char_data *ch)
{
  act(g, "@wYour body slowly stops flashing. Steam rises from your skin as you slowly let off the energy you built up in a safe manner.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@w$n's body slowly stops flashing. Steam rises from $s skin as $e slowly lets off the energy $e built up in a safe manner.@n", true, ch, nullptr, nullptr, TO_ROOM);
}

/* The blast: the charge, 60% of the base PL and the base ST (half again for a wide one), all spent */
int64_t selfd_blast(GameWorld *g, struct char_data *ch, bool wide)
{
  int64_t dmg = char_get_charge(ch);

  char_set_charge(ch, 0);
  dmg += (getBasePL(ch)) * 0.6;
  dmg += (getBaseST(ch));
  if (wide)
    dmg *= 1.5;
  decCurHealthPercent(g, ch, 1, 1);
  char_set_suppressed(ch, 0);
  char_set_suppression(ch, 0);
  return dmg;
}

/* Each self destruct teaches it: quirk: it becomes the skill and a roll, unless twice the skill and the roll is
   more than 100, when it becomes 100 */
void selfd_learns(GameWorld *g, struct char_data *ch)
{
  int num = rand_number(g, 10, 20) + char_get_skill(ch, SKILL_SELFD);
  char_set_skills_base(ch, SKILL_SELFD, char_get_skill(ch, SKILL_SELFD) + num <= 100 ? num : 100);
}

constexpr act_msgs SELFD_ON_ONE = {"@RYou EXPLODE! The explosion concentrates on @r$N@R, engulfing $M in a sphere of deadly energy!@n",
                                   "@R$n EXPLODES! The explosion concentrates on YOU, engulfing your body in a sphere of deadly energy!@n",
                                   "@R$n EXPLODES! The explosion concentrates on @r$N@R, engulfing $M in a sphere of deadly energy!@n", true};
constexpr act_msgs SELFD_CAUGHT = {"@r$N@R is caught in the explosion!@n", "@RYou are caught in the explosion!@n",
                                   "@r$N@R is caught in the explosion!@n", true};

} // namespace

/* The first time, starts to glow; the next, calms down, unless primed (PLR_SELFD2), when it explodes on whoever is
   grappled, or else on everyone around */
ACMD(do_selfd)
{
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
  if (char_get_con_sdcooldown(ch) > 0) {
    send_to_char(g, ch, "Your body is still recovering from the last self destruct!\r\n");
    return;
  }
  if (!char_get_skill(ch, SKILL_SELFD))
    char_set_skills_base(ch, SKILL_SELFD, rand_number(g, 10, 20));

  if (!char_plr_flagged(ch, PLR_SELFD)) {
    act(g, "@RYour body starts to glow @wwhite@R and flash. The flashes start out slowly but steadilly increase in speed. Your aura begins to burn around your body at the same time in a violent fashion!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@R$n's body starts to glow @wwhite@R and flash. The flashes start out slowly but steadilly increase in speed. $n's aura begins to burn around $s body at the same time in a violent fashion!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_SELFD, true);
    return;
  }
  if (!char_plr_flagged(ch, PLR_SELFD2)) {
    selfd_calms(g, ch);
    char_plr_set(ch, PLR_SELFD, false);
    return;
  }
  if (char_get_grappling(g, ch) != nullptr && !can_kill(g, ch, char_get_grappling(g, ch), nullptr, 3)) {
    selfd_calms(g, ch);
    send_to_char(g, ch, "You can't kill them, the immortals won't allow it!\r\n");
    char_plr_set(ch, PLR_SELFD, false);
    return;
  }

  if (struct char_data *tch = char_get_grappling(g, ch)) {
    int64_t dmg = selfd_blast(g, ch, false);
    act_triple(g, SELFD_ON_ONE, ch, nullptr, tch);
    hurt(g, 0, 0, ch, tch, nullptr, dmg, 1);
    char_plr_set(ch, PLR_SELFD, false);
    char_plr_set(ch, PLR_SELFD2, false);
    if (char_plr_flagged(ch, PLR_IMMORTAL))
      char_set_con_sdcooldown(ch, 600);
    if ((char_is_majin(ch) || char_is_bio(ch)) && getCurLFPercent(ch) > 0.5) {
      decCurLFPercent(g, ch, 2, -1);
      char_plr_set(ch, PLR_GOOP, true);
      char_set_gooptime(ch, 70);
    } else
      die(g, ch, nullptr);
    selfd_learns(g, ch);
    return;
  }

  int64_t dmg = selfd_blast(g, ch, true);
  act(g, "@RYou EXPLODE! The explosion expands outward burning up all surroundings for a large distance. The explosion takes on the shape of a large energy dome with you at its center!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@R$n EXPLODES! The explosion expands outward burning up all surroundings for a large distance. The explosion takes on the shape of a large energy dome with $n at its center!@n", true, ch, nullptr, nullptr, TO_ROOM);
  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch || !can_kill(g, ch, vict, nullptr, 3) || char_mob_flagged(vict, MOB_NOKILL))
      continue;
    act_triple(g, SELFD_CAUGHT, ch, nullptr, vict);
    hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
  }
  if (char_plr_flagged(ch, PLR_IMMORTAL))
    char_set_con_sdcooldown(ch, 600);
  die(g, ch, nullptr); /* quirk: before the flags are cleared, unlike above */
  char_plr_set(ch, PLR_SELFD, false);
  char_plr_set(ch, PLR_SELFD2, false);
  selfd_learns(g, ch);
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
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!can_grav(g, ch) || !check_skill(g, ch, SKILL_SPIRAL) || !limb_ok(g, ch, 0))
    return;
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (!check_points(g, ch, getMaxKI(g, ch) * .5, 0))
    return;
  int skill = init_skill(g, ch, SKILL_SPIRAL);

  struct char_data *vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!vict && !(vict = char_get_fighting(g, ch))) {
    send_to_char(g, ch, "Nothing around here by that name.");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 3))
    return;
  if (handle_defender(g, vict, ch))
    vict = char_get_defender(g, vict);

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
