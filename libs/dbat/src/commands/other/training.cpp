/* training.c: training, exercises and growth
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <array>

#include <sys/stat.h>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.other.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/utils.hpp"
#include "dbat/flags.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/races.hpp"
#include "dbat/fileop.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/weather.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/graph.hpp"
#include "dbat/alias.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/shop.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

/* Used by do_rpp for soft-cap */
void bring_to_cap(struct char_data *ch)
{

  bool p_trans = (race_can_transform(char_get_race(ch)) && !race_can_revert(char_get_race(ch)));
  int64_t cap  = calc_soft_cap(ch);

  switch (race_soft_cap_type(ch)) {
    case SOFTCAP_FIXED:
      if (getBasePL(ch) < cap)
        gainBasePL(ch, cap - getBasePL(ch) - 1, p_trans);
      if (getBaseKI(ch) < cap)
        gainBaseKI(ch, cap - getBaseKI(ch) - 1, p_trans);
      if (getBaseST(ch) < cap)
        gainBaseST(ch, cap - getBaseST(ch) - 1, p_trans);
      break;
    default:
      break;
  }
}

namespace {

/* What one does to train a stat, to themselves and to the room */
struct drill {
  const char *to_char, *to_room;
};

/* The stats one can train, in the order the status shows them */
struct trained_stat {
  std::string_view word; /* what's typed */
  std::string_view name;
  std::string_view label; /* in the status */
  int8_t *(*slot)(struct char_data *);
  int player_special_data::*progress;
  int bonus_trait, nega_trait;
  bool uses_ki; /* rather than stamina */
  std::array<drill, 3> drills;
};

constexpr trained_stat STATS[] = {
    {"str", "strength", "Strength Progress    ", char_real_abils_str_slot, &player_special_data::trainstr, BONUS_BRAWNY, BONUS_WIMP, false,
     {{{"@WYou throw a flurry of punches into the air at an invisible opponent.@n", "@W$n throws a flurry of punches into the air.@n"},
       {"@WYou leap into the air and throw a wild kick at an invisible opponent@n", "@W$n leaps into the air and throws a wild kick at nothing.@n"},
       {"@WYou leap high into the air and unleash a flurry of punches and kicks at an invisible opponent@n", "@W$n leaps high into the air and unleashes a flurry of punches and kicks at nothing.@n"}}}},
    {"spd", "speed", "Speed Progress       ", char_real_abils_cha_slot, &player_special_data::trainspd, BONUS_QUICK, BONUS_SLOW, false,
     {{{"@WYou dash quickly around the surrounding area as fast as you can!@n", "@W$n dashes quickly around the surrounding area as fast as $e can!@n"},
       {"@WYou dodge to the side as fast as you can!@n", "@W$n dodges to the side as fast as $e can!@n"},
       {"@WYou dash backwards as fast as you can!@n", "@W$n dashes backwards as fast as $e can!@n"}}}},
    {"con", "constitution", "Constitution Progress", char_real_abils_con_slot, &player_special_data::traincon, BONUS_STURDY, BONUS_FRAIL, false,
     {{{"@WYou leap into the air and then slam into the ground with your feet outstretched!@n", "@W$n leaps into the air and then slams into the ground with $s feet outstretched!?@n"},
       {"@WYou leap into the air and then slam into the ground with your fists!@n", "@W$n leaps into the air and then slams into the ground with $s fists!?@n"},
       {"@WYou leap into the air and then slam into the ground with your body!@n", "@W$n leaps into the air and then slams into the ground with $s body!?@n"}}}},
    {"int", "intelligence", "Intelligence Progress", char_real_abils_intel_slot, &player_special_data::trainint, BONUS_SCHOLARLY, BONUS_DULL, true,
     {{{"@WConcentrating you fly high into the air as fast as you can before settling slowly back to the ground.@n", "@W$n flies high into the air as fast as $e can before settling slowly back to the ground.@n"},
       {"@WYou focus your ki at your outstretched hand and send a mild shockwave in that direction!@n", "@W$n focuses $s ki at $s outstretched hand and sends a mild shockwave in that direction!@n"},
       {"@WYou concentrate on your ki and force torrents of it to rush out from your body randomly!@n", "@W$n seems to concentrate before torrents of ki randomly blasts out from $s body!@n"}}}},
    {"wis", "wisdom", "Wisdom Progress      ", char_real_abils_wis_slot, &player_special_data::trainwis, BONUS_SAGE, BONUS_FOOLISH, true,
     {{{"@WYou close your eyes and wage a mental battle against an imaginary opponent.@n", "@W$n closes $s eyes for a moment and an expression of intensity forms on it.@n"},
       {"@WYou look around and contemplate battle tactics for an imaginary scenario.@n", "@W$n looks around and appears to be imagining things that aren't there.@n"},
       {"@WYou invent a battle plan for a battle that doesn't exist!@n", "@W$n seems to have thought of something.@n"}}}},
    {"agl", "agility", "Agility Progress     ", char_real_abils_dex_slot, &player_special_data::trainagl, BONUS_AGILE, BONUS_CLUMSY, false,
     {{{"@WYou do a series of backflips through the air, landing gracefully on one foot a moment later.@n", "@W$n does a series of backflips through the air, landing gracefully on one foot a moment later.@n"},
       {"@WYou flip forward and launch off your hands into the air. You land gracefully on one foot a moment later.@n", "@W$n flips forward and launches off $s hands into the air. Then $e lands gracefully on one foot a moment later.@n"},
       {"@WYou flip to the side off one hand and then land on your feet.@n", "@W$n flips to the side off one hand and then lands on $s feet.@n"}}}},
};
constexpr size_t N_STATS = std::size(STATS);

/* Some races need more (or less) training for some stats: shares of the usual, in STATS' order */
constexpr struct {
  bool (*is)(const struct char_data *);
  std::array<double, N_STATS> share;
} RACE_SHARES[] = {
    {char_is_human, {1, 1, 1, 0.75, 0.75, 1}},
    {char_is_kanassan, {1, 1, 1, 0.4, 0.4, 0.4}},
    {char_is_halfbreed, {0.75, 1, 1, 0.75, 1, 1}},
    {char_is_truffle, {1.5, 1, 1.5, 1, 1, 1}},
};

/* The training a stat takes to go up by one */
int training_needed(struct char_data *ch, size_t stat)
{
  int needed = 5000 + 500 * *STATS[stat].slot(ch);
  auto race  = std::ranges::find_if(RACE_SHARES, [ch](const auto &r) { return r.is(ch); });
  if (race != std::end(RACE_SHARES))
    needed = needed * race->share[stat];
  return needed;
}

void training_status(GameWorld *g, struct char_data *ch, int64_t weight)
{
  send_to_char(g, ch, "@D-------------[ @GTraining Status @D]-------------@n\r\n");
  for (size_t i = 0; i < N_STATS; i++) {
    const trained_stat &s = STATS[i];
    send_to_char(g, ch, "  @m{}@D: @R{:>6}/{:>6}@n\r\n", s.label, add_commas(char_get_player_specials(ch)->*s.progress),
                 *s.slot(ch) >= 80 ? "@rCAPPED" : add_commas(training_needed(ch, i)));
  }
  send_to_char(g, ch, "@D  -----------------------------------------  @n\r\n");
  send_to_char(g, ch, "  @CCurrent Weight Held  @D: @c{}@n\r\n", add_commas(weight));
  send_to_char(g, ch, "@D---------------------------------------------@n\r\n");
  send_to_char(g, ch, "Syntax: train (str | spd | agl | wis | int | con)\r\n");
}

/* How much more a sensei makes of training, by the student's level: the first row they reach */
constexpr struct {
  int level;
  int64_t mult;
} SENSEI_MULTS[] = {{100, 15000}, {80, 1500}, {40, 600}, {20, 300}, {10, 150}};

/* How hard the training is (its weight against the trainee's power level), from the easiest that counts */
constexpr struct {
  int gain;
  const char *feel;
  int wait;
} GAINS[] = {
    {5, "slight improvement.", PULSE_3SEC},
    {10, "some improvement.", PULSE_3SEC},
    {25, "good improvement.", PULSE_3SEC},
    {50, "great improvement!", PULSE_5SEC},
    {100, "awesome improvement!", PULSE_5SEC},
};

/* 0 for too easy to count, or 1 + the row of GAINS */
int training_difficulty(GameWorld *g, struct char_data *ch, int64_t total)
{
  int64_t pl                 = getEffMaxPL(g, ch);
  const int64_t thresholds[] = {pl / 8, pl / 4, pl / 2, pl, pl * 2};
  int bonus                  = 0;
  for (int64_t t : thresholds)
    if (total > t)
      bonus++;
  return bonus;
}

/* The stat cap for a level */
int stat_cap(struct char_data *ch)
{
  int level = char_get_level(ch);
  return level >= 61 ? 80 : level > 40 ? 60 : level > 20 ? 40 : 20;
}

} // namespace

ACMD(do_train)
{
  if (char_is_npc(ch))
    return;

  if (char_get_carry_weight(ch) > getMaxCarryWeight(g, ch)) {
    send_to_char(g, ch, "You are weighted down too much!\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  int64_t weight        = getCurCarriedWeight(ch);

  if (arg.empty()) {
    training_status(g, ch, weight);
    return;
  }

  /* Figure up the weight bonus */
  int gravity   = room_get_gravity(g, char_get_in_room(ch));
  int64_t total = weight * (gravity + 1);
  total += (gravity + 1) ^ 2; /* quirk: ^ is exclusive or, not a power */
  if (char_get_in_room(ch) >= 6100 && char_get_in_room(ch) <= 6135)
    total += total * 0.15;

  bool sensei = char_get_in_room(ch) == sensei_location_id(char_get_class(ch));
  if (sensei) {
    if (!(char_get_gold(ch) >= 8 && char_get_practice_points(g, ch) >= 1)) {
      send_to_char(g, ch, "It costs 8 Zenni and 1 PS to train with your sensei.\r\n");
      return;
    }
    total += total * 0.85;
    auto row = std::ranges::find_if(SENSEI_MULTS, [ch](const auto &m) { return char_get_level(ch) >= m.level; });
    if (row != std::end(SENSEI_MULTS))
      total *= row->mult;
    send_to_char(g, ch, "@G{} begins to instruct you in training technique.@n\r\n", char_sensei_name(ch));
  }

  int bonus    = training_difficulty(g, ch, total);
  int64_t cost = sensei ? total / 25 + getMaxST(g, ch) / 60 : total / 20 + getMaxST(g, ch) / 50;
  if (char_get_bonuses(ch, BONUS_HARDWORKER))
    cost -= cost * 0.25;
  if (char_get_relax_count(ch) >= 464)
    cost *= 10;
  else if (char_get_relax_count(ch) >= 232)
    cost *= 5;
  else if (char_get_relax_count(ch) >= 116)
    cost *= 2;

  auto found = std::ranges::find_if(STATS, [&](const trained_stat &s) { return str_eq_nocase(s.word, arg); });
  if (found == std::end(STATS)) {
    send_to_char(g, ch, "Syntax: train (str | spd | agl | wis | int | con)\r\n");
    return;
  }
  const trained_stat &stat = *found;
  int8_t *stat_val         = stat.slot(ch);

  if (*stat_val == 80) {
    send_to_char(g, ch, "Your base {} is maxed!\r\n", stat.name);
    return;
  }
  if (*stat_val >= 70 && char_get_bonuses(ch, stat.nega_trait) > 0) {
    send_to_char(g, ch, "You're not able to withstand increasing your {} beyond 70.\r\n", stat.name);
    return;
  }
  if (*stat_val >= stat_cap(ch)) {
    send_to_char(g, ch, "You have reached the stat cap for your level.\r\n");
    return;
  }

  int plus;
  if (stat.uses_ki) {
    if (getCurKI(g, ch) < cost) {
      send_to_char(g, ch, "You do not have enough ki with the current weight worn and gravity!\r\n");
      return;
    }
    plus = ((total / 20 + getMaxKI(g, ch) / 50) * 100) / getMaxKI(g, ch);
    decCurKI(g, ch, cost, 0);
  } else {
    if (getCurST(g, ch) < cost) {
      send_to_char(g, ch, "You do not have enough stamina with the current weight worn and gravity!\r\n");
      return;
    }
    plus = ((total / 20 + getMaxST(g, ch) / 50) * 100) / getMaxST(g, ch);
    decCurST(g, ch, cost, 0);
  }

  reveal_hiding(g, ch, 0);
  const drill &d = stat.drills[rand_number(g, 1, 3) - 1];
  act(g, d.to_char, true, ch, nullptr, nullptr, TO_CHAR);
  act(g, d.to_room, true, ch, nullptr, nullptr, TO_ROOM);

  plus = (plus + 75) * 2;
  if (char_get_in_room(ch) >= 19800 && char_get_in_room(ch) <= 19899)
    plus *= 4;
  if (room_flagged(g, char_get_in_room(ch), ROOM_HBTC))
    plus *= 3;
  if (char_get_bonuses(ch, BONUS_HARDWORKER))
    plus += plus * 0.25;
  if (char_get_bonuses(ch, stat.bonus_trait))
    plus += plus * 0.75;
  if (char_get_bonuses(ch, BONUS_LONER))
    plus += plus * 0.05;
  if (sensei)
    plus += plus * 0.2;

  int &progress = char_get_player_specials(ch)->*stat.progress;
  if (bonus == 0) {
    progress += 1;
    send_to_char(g, ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    char_set_wait(ch, PULSE_3SEC);
  } else {
    const auto &gain = GAINS[bonus - 1];
    progress += gain.gain + plus;
    send_to_char(g, ch, "You feel {} @D[@G+{:d}@D]@n\r\n", gain.feel, plus + gain.gain);
    char_set_wait(ch, gain.wait);
  }

  if (sensei) {
    char_set_gold(ch, char_get_gold(ch) - 8);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - 1);
  }

  int needed = training_needed(ch, &stat - STATS);
  if (progress >= needed) {
    progress -= needed;
    send_to_char(g, ch, "You feel your {} improve!@n\r\n", stat.name);
    *stat_val += 1;
    if (char_is_piccolo(ch) && char_is_namek(ch) && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0) {
      char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * 0.25));
      send_to_char(g, ch, "You gained quite a bit of experience from that!\r\n");
    }
    save_char(g, ch);
  }
}

ACMD(do_potential)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!know_skill(g, ch, SKILL_POTENTIAL))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Who's potential do you want to release?\r\nPotential Releases: {:d}\r\n", char_get_boosts(ch));
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "That target isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  const char *refusal    = nullptr;
  if (char_is_npc(vict))
    refusal = "Why would you waste your time releasing their potential?\r\n";
  else if (vict == ch)
    refusal = "You can't release your own potential.\r\n";
  else if (char_get_boosts(ch) == 0)
    refusal = "You have no potential releases to perform.\r\n";
  else if (char_plr_flagged(vict, PLR_PR))
    refusal = "Their potential has already been released\r\n";
  else if (char_is_android(vict))
    refusal = "They are a machine and have no potential to release.\r\n";
  else if (char_get_majinize(vict) > 0)
    refusal = "They are already majinized and have no potential to release.\r\n";
  else if (char_is_majin(vict))
    refusal = "They have no potential to release...\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  int boost = char_get_skill(ch, SKILL_POTENTIAL) / 2;
  char_plr_set(vict, PLR_PR, true);
  gainBasePL(vict, (getBasePL(vict) / 100) * boost, false);
  if (char_is_halfbreed(vict)) {
    gainBaseKI(vict, (getBaseKI(vict) / 100) * boost, false);
    gainBaseST(vict, (getBaseST(vict) / 100) * boost, false);
  }
  reveal_hiding(g, ch, 0);
  act_triple(g, {"You place your hand on top of $N's head. After a moment of concentrating you release their hidden potential.", "$n places $s hand on top of your head. After a moment you feel a rush of power as your hidden potential is released!", "$n places $s hand on $N's head. After a moment a rush of power explodes off of $N's body!", true}, ch, nullptr, vict);
  for (int i = 0; i < 4; i++) /* quirk: four times over */
    improve_skill(g, ch, SKILL_POTENTIAL, 0);
  char_set_boosts(ch, char_get_boosts(ch) - 1);
}

namespace {

/* Situps, pushups and meditation: in gravity, each costs (stamina, or ki to meditate) and gains (stamina, power level
 * or ki) more the stronger it is. */

/* What the stronger gravities add to the cost */
struct heavy_gravity {
  int gravity;
  int extra;
};
constexpr heavy_gravity HEAVY_GRAVITY[] = {{300, 1000000}, {400, 2000000}, {500, 7500000}, {1000, 15000000}, {5000, 25000000}, {10000, 50000000}};

/* What exercising costs in this gravity, with this weight lifted (by the mind). Situps and pushups count it in an
 * int, meditation in an int64_t. */
template <class T> T exercise_cost(struct char_data *ch, int gravity, int64_t weight = 0)
{
  T cost = 1;

  if (!char_is_bardock(ch) || gravity != 10)
    cost += gravity * ((gravity * 2) + char_get_level(ch));
  else
    cost += gravity * char_get_level(ch);
  cost += weight * ((gravity + 1) / 5);
  auto heavy = std::ranges::find(HEAVY_GRAVITY, gravity, &heavy_gravity::gravity);
  if (heavy != std::end(HEAVY_GRAVITY))
    cost += heavy->extra;
  if (cost < weight)
    cost = weight;

  if (cost == 1 || cost == 0)
    cost = 25;
  if (char_get_bonuses(ch, BONUS_HARDWORKER) > 0)
    cost -= cost * .25;
  else if (char_get_bonuses(ch, BONUS_SLACKER) > 0)
    cost += cost * .25;

  if (char_get_relax_count(ch) >= 464)
    cost *= 50;
  else if (char_get_relax_count(ch) >= 232)
    cost *= 15;
  else if (char_get_relax_count(ch) >= 116)
    cost *= 4;
  return cost;
}

/* The gravities one gains in, with what the body (situps and pushups) and the mind gain there, and how the body
 * feels it. Any other gains nothing. */
struct gravity_tier {
  int gravity;
  int body_lo, body_hi;
  int mind_lo, mind_hi;
  const char *to_char, *to_room; /* after "You do a situp" and "$n does a situp" */
};
constexpr gravity_tier GRAVITIES[] = {
    {0, 1, 1, 1, 1, ".", "."},
    {10, 3, 7, 2, 4, ", feeling the strain of gravity.", ", while sweating."},
    {20, 8, 14, 5, 10, ", and feel gravity's pull.", ", while sweating."},
    {30, 14, 20, 10, 15, ", and feel the burn.", ", while sweating."},
    {40, 20, 35, 15, 20, ", and feel the burn.", ", while sweating."},
    {50, 40, 60, 40, 60, ", and feel the burn.", ", while sweating."},
    {100, 180, 250, 180, 250, ", and really strain against the gravity.", ", while sweating."},
    {200, 400, 600, 400, 600, ", and really strain against the gravity.", ", while sweating."},
    {300, 800, 1200, 800, 1200, ", and really strain against the gravity.", ", while sweating."},
    {400, 2000, 3000, 2000, 3000, ", and really strain against the gravity.", ", while sweating."},
    {500, 4000, 6000, 4000, 6000, ", and really strain against the gravity.", ", while sweating."},
    {1000, 9000, 10000, 9000, 10000, ", and it was a really hard one to finish.", ", while sweating profusely."},
    {5000, 15000, 20000, 15000, 20000, ", and it was a really hard one to finish.", ", while sweating profusely."},
    {10000, 25000, 30000, 25000, 30000, ", and it was a really hard one to finish.", ", while sweating profusely."},
};

const gravity_tier *gravity_tier_of(int gravity)
{
  auto tier = std::ranges::find(GRAVITIES, gravity, &gravity_tier::gravity);
  return tier == std::end(GRAVITIES) ? nullptr : tier;
}

int roll(GameWorld *g, int lo, int hi)
{
  return lo == hi ? lo : rand_number(g, lo, hi);
}

/* A situp or a pushup (what), shown off: what it gains before the rest is reckoned */
int body_workout(GameWorld *g, struct char_data *ch, int gravity, std::string_view what)
{
  const gravity_tier *tier = gravity_tier_of(gravity);
  if (!tier)
    return 0;

  int bonus = roll(g, tier->body_lo, tier->body_hi);
  act(g, str_format("@gYou do a {}{}@n", what, tier->to_char).c_str(), true, ch, nullptr, nullptr, TO_CHAR);
  act(g, str_format("@g$n does a {}{}@n", what, tier->to_room).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
  return bonus;
}

/* How the cost measures up against the exerciser's most: the first row it fits, or else just right */
constexpr struct {
  int64_t share; /* of the most */
  bool at_least; /* or less than */
  int mult;      /* or, if negative, divide */
  const char *how;
  bool ellipsis;
} CHALLENGES[] = {
    {2, true, 10, "a great challenge for you!", false},
    {4, true, 8, "an awesome challenge for you!", false},
    {10, true, 6, "a good challenge for you!", false},
    {1000, false, -8, "so easy to you, you could do it in your sleep", true},
    {100, false, -5, "the opposite of a challenge for you", true},
    {50, false, -4, "definitely not a challenge for you", true},
    {30, false, -3, "barely a challenge for you", true},
    {20, false, -2, "hardly a challenge for you", true},
};

/* what is "This gravity is" or the like; ellipsis ends the easy ones and the just right */
template <class T>
void challenge(GameWorld *g, struct char_data *ch, T &bonus, T cost, int64_t most, std::string_view what, std::string_view ellipsis)
{
  auto row = std::ranges::find_if(CHALLENGES, [&](const auto &c) { return c.at_least ? cost >= most / c.share : cost < most / c.share; });
  if (row == std::end(CHALLENGES)) {
    send_to_char(g, ch, "This gravity is just perfect for you{}\r\n", ellipsis); /* quirk: meditating, too */
    bonus *= 4;
    return;
  }
  send_to_char(g, ch, "{} {}{}\r\n", what, row->how, row->ellipsis ? ellipsis : "");
  if (row->mult > 0)
    bonus *= row->mult;
  else
    bonus /= -row->mult;
}

bool in_rooms(struct char_data *ch, room_vnum lo, room_vnum hi)
{
  return char_get_in_room(ch) >= lo && char_get_in_room(ch) <= hi;
}

/* What the place adds: the time chamber, a workout room (better at 19100 to 19199) or 19800 to 19899's magic. The
 * least one gains is 1, 15 in the time chamber, and in a workout room 12 or 6 when the bonus is no more than
 * workout_least. */
template <class T> void place_bonus(GameWorld *g, struct char_data *ch, T &bonus, int workout_least)
{
  room_vnum room = char_get_in_room(ch);

  if (room_flagged(g, room, ROOM_HBTC)) {
    send_to_char(g, ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
    bonus *= 10;
  } else if (room_flagged(g, room, ROOM_WORKOUT)) {
    bonus *= in_rooms(ch, 19100, 19199) ? 10 : 5;
  } else if (in_rooms(ch, 19800, 19899)) {
    send_to_char(g, ch, "@rThis place feels like... Magic.@n\r\n");
    bonus *= 20;
  }
  if (bonus <= 0)
    bonus = room_flagged(g, room, ROOM_HBTC) ? 15 : 1;
  if (bonus <= workout_least && room_flagged(g, room, ROOM_WORKOUT))
    bonus = in_rooms(ch, 19100, 19199) ? 12 : 6;
}

/* How long one exercise takes, by the gravity: the first row it's within, or none past them */
constexpr struct {
  int gravity, wait;
} EXERCISE_WAITS[] = {{50, PULSE_2SEC}, {200, PULSE_3SEC}, {500, PULSE_4SEC}, {1000, PULSE_5SEC}, {5000, PULSE_6SEC}, {10000, PULSE_7SEC}};

void exercise_wait(struct char_data *ch, int gravity)
{
  auto row = std::ranges::find_if(EXERCISE_WAITS, [gravity](const auto &w) { return gravity <= w.gravity; });
  if (row != std::end(EXERCISE_WAITS))
    char_set_wait(ch, row->wait);
}

/* The races that exercise for nothing */
bool gains_nothing(struct char_data *ch)
{
  return char_is_android(ch) || char_is_bio(ch) || char_is_majin(ch) || char_is_arlian(ch);
}

/* The checks between paying and exercising, for situps and pushups; false if one fails */
bool fit_to_exercise(GameWorld *g, struct char_data *ch, int cost, int limb, std::string_view what)
{
  if (gains_nothing(ch)) {
    send_to_char(g, ch, "You will gain nothing from exercising!\r\n");
    return false;
  }
  if (!limb_ok(g, ch, limb))
    return false;
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You are too tired!\r\n");
    return false;
  }
  if (char_plr_flagged(ch, PLR_SPAR)) {
    send_to_char(g, ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
    return false;
  }
  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are fighting you moron!\r\n");
    return false;
  }
  if (char_aff_flagged(ch, AFF_FLYING)) {
    send_to_char(g, ch, "You can't do {}s in midair!\r\n", what);
    return false;
  }
  return true;
}

} // namespace

ACMD(do_situp)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You are a mob fool!\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(g, ch, "The fire makes it too hot!\r\n");
    return;
  }
  if (char_get_drag(g, ch)) {
    send_to_char(g, ch, "You are dragging someone!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_FISHING)) {
    send_to_char(g, ch, "Stop fishing first.\r\n");
    return;
  }
  if (char_get_carrying(g, ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }

  int gravity = room_get_gravity(g, char_get_in_room(ch));
  int cost    = exercise_cost<int>(ch, gravity);
  if (!fit_to_exercise(g, ch, cost, 1, "situp"))
    return;

  int bonus = body_workout(g, ch, gravity, "situp");
  challenge(g, ch, bonus, cost, getMaxST(g, ch), "This gravity is", "...");
  if (is_soft_cap(ch, 2, 1.0))
    bonus = 0;
  place_bonus(g, ch, bonus, 0); /* quirk: 0, so a workout room never lifts the least */
  bonus += char_get_level(ch) / 20;
  if (char_is_namek(ch))
    bonus -= bonus / 4;
  if (char_get_bonuses(ch, BONUS_HARDWORKER))
    bonus += bonus * 0.5;
  if (char_get_bonuses(ch, BONUS_LONER))
    bonus += bonus * 0.1;
  send_to_char(g, ch, "You feel slightly more vigorous @D[@G+{}@D]@n.\r\n", add_commas(bonus));
  gainBaseST(ch, bonus, true);
  exercise_wait(ch, gravity);
  decCurST(g, ch, cost, 0);
}

ACMD(do_pushup)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You are a mob fool!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_FISHING)) {
    send_to_char(g, ch, "Stop fishing first.\r\n");
    return;
  }
  if (char_get_drag(g, ch)) {
    send_to_char(g, ch, "You are dragging someone!\r\n");
    return;
  }
  if (char_get_carrying(g, ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }

  int gravity = room_get_gravity(g, char_get_in_room(ch));
  int cost    = exercise_cost<int>(ch, gravity);
  if (!fit_to_exercise(g, ch, cost, 0, "pushup"))
    return;

  int bonus = body_workout(g, ch, gravity, "pushup");
  challenge(g, ch, bonus, cost, getMaxPLTrans(g, ch), "This gravity is", "....");
  if (is_soft_cap(ch, 0, 1.0))
    bonus = 0;
  place_bonus(g, ch, bonus, 1);
  bonus += char_get_level(ch) / 20;
  if (char_is_namek(ch))
    bonus -= bonus / 4;
  if (char_get_bonuses(ch, BONUS_HARDWORKER))
    bonus += bonus * 0.5;
  if (char_get_bonuses(ch, BONUS_LONER))
    bonus += bonus * 0.1;
  send_to_char(g, ch, "You feel slightly stronger @D[@G+{}@D]@n.\r\n", add_commas(bonus));
  if (char_is_human(ch))
    bonus = bonus * 0.8; /* quirk: after they're told */
  gainBasePL(ch, bonus, true);
  exercise_wait(ch, gravity);
  decCurST(g, ch, cost, 0);
}

namespace {

/* meditate expand: another skill slot, for practices */
void expand_mind(GameWorld *g, struct char_data *ch)
{
  int slot_cost = char_is_saiyan(ch) ? 7000 : 3500;

  if (char_get_practice_points(g, ch) < slot_cost) {
    send_to_char(g, ch, "You do not have enough practice sessions to expand your mind and ability to remember skills.\r\n");
    send_to_char(g, ch, "{} needed.\r\n", add_commas(slot_cost));
  } else if ((char_get_skill_slots(ch) >= 60 && char_get_bonuses(ch, BONUS_GMEMORY) == 0) ||
             (char_get_skill_slots(ch) >= 65 && char_get_bonuses(ch, BONUS_GMEMORY) == 1)) {
    send_to_char(g, ch, "You can not have any more slots through this process.\r\n");
  } else {
    send_to_char(g, ch, "During your meditation you manage to expand your mind and get the feeling you could learn some new skills.\r\n");
    char_set_skill_slots(ch, char_get_skill_slots(ch) + 1);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - slot_cost);
  }
}

/* meditate break: pushing out the one mind linked with them */
void break_mind_link(GameWorld *g, struct char_data *ch)
{
  struct char_data *linked = char_get_mindlink(g, ch);

  if (!linked) {
    send_to_char(g, ch, "You are not mind linked with anyone.\r\n");
    return;
  }
  if (char_get_linker(ch) == 1) {
    send_to_char(g, ch, "This is not how you break YOUR mind link.\r\n");
    return;
  }
  if (getCurKI(g, ch) < getMaxKI(g, linked) * 0.05) {
    send_to_char(g, ch, "You do not have enough ki to manage a break.\r\n");
    return;
  }
  if (char_get_aff_abils_intel(ch) + rand_number(g, -5, 10) >= char_get_aff_abils_intel(linked) + (char_get_skill(linked, SKILL_TELEPATHY) * 0.1)) {
    act(g, "@rYou manage to break the mind link between you and @R$N@r!@n", false, ch, nullptr, linked, TO_CHAR);
    act(g, "$n closes their eyes for a few seconds.", false, ch, nullptr, linked, TO_ROOM);
    send_to_char(g, linked, "@rYour mind linked target manages to push you out!@n\r\n");
    if (char_get_aff_abils_intel(linked) < axion_dice(g, -10) && !char_aff_flagged(linked, AFF_SHOCKED)) {
      send_to_char(g, linked, "Your mind is shocked by the flood of mental energy that pushed it out!@n\r\n");
      char_aff_set(linked, AFF_SHOCKED, false); /* quirk: they aren't, though */
    }
    char_set_linker(linked, 0);
    char_set_mindlink(linked, nullptr);
    char_set_mindlink(ch, nullptr);
    return;
  }
  act(g, "@rYou struggle to free your mind of @R$N's@r link, but fail!@n", false, ch, nullptr, linked, TO_CHAR);
  act(g, "$n closes their eyes for a few seconds, and appears to struggle quite a bit.", false, ch, nullptr, linked, TO_ROOM);
  send_to_char(g, linked, "@rYour mind linked target struggles to free their mind, but fails!@n\r\n");
  decCurKI(g, ch, getMaxKI(g, linked) * .05, 0);
}

} // namespace

ACMD(do_meditate)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You are a mob fool!\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), ROOM_HELL)) {
    send_to_char(g, ch, "The fire makes it too hot!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_FISHING)) {
    send_to_char(g, ch, "Stop fishing first.\r\n");
    return;
  }
  if (gains_nothing(ch)) {
    send_to_char(g, ch, "You will gain nothing from exercising!\r\n");
    return;
  }
  if (char_get_carrying(g, ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }
  if (char_get_drag(g, ch)) {
    send_to_char(g, ch, "You are dragging someone!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_SPAR)) {
    send_to_char(g, ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
    return;
  }
  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are fighting you moron!\r\n");
    return;
  }
  if (char_get_position(ch) != POS_SITTING) {
    send_to_char(g, ch, "You need to be sitting to meditate.\r\n");
    return;
  }

  constexpr const char *syntax = "Syntax: meditate (object)\nSyntax: meditate expand\nSyntax: meditate break\r\n";
  if (arg.empty()) {
    send_to_char(g, ch, syntax);
    return;
  }
  if (str_eq_nocase(arg, "expand")) {
    expand_mind(g, ch);
    return;
  }
  if (str_eq_nocase(arg, "break")) {
    break_mind_link(g, ch);
    return;
  }

  struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)));
  if (!obj) {
    send_to_char(g, ch, syntax);
    return;
  }
  if (obj_get_vnum(g, obj) == 79) {
    send_to_char(g, ch, "It's frozen to the surface.\r\n");
    return;
  }

  int64_t weight = obj_get_weight(obj);
  if (obj_get_sitting(g, obj))
    weight += char_get_weight(obj_get_sitting(g, obj));

  int gravity  = room_get_gravity(g, char_get_in_room(ch));
  int64_t cost = exercise_cost<int64_t>(ch, gravity, weight);
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You don't have enough ki!\r\n");
    return;
  }

  act(g, "@cYou close your eyes and concentrate, lifting $p@c with your ki.@n", true, ch, obj, nullptr, TO_CHAR);
  act(g, "@c$n closes $s eyes and lifts $p@c with $s ki.@n", true, ch, obj, nullptr, TO_ROOM);

  const gravity_tier *tier = gravity_tier_of(gravity);
  int64_t bonus            = tier ? roll(g, tier->mind_lo, tier->mind_hi) : 0;
  bonus += ((weight + 1) / 500) + 1;
  challenge(g, ch, bonus, cost, getMaxKI(g, ch), "The object weight and gravity are", "....");
  if (is_soft_cap(ch, 1, 1.0))
    bonus = 0;
  place_bonus(g, ch, bonus, 1);
  if (bonus != 1 && char_is_demon(ch) && rand_number(g, 1, 100) >= 80) {
    send_to_char(g, ch, "Your spirit magnifies the strength of your body! @D[@G+{}@D]@n\r\n", add_commas(bonus / 2));
    gainBasePL(ch, bonus / 2, false);
  }
  bonus += char_get_level(ch) / 20;
  if (char_is_namek(ch))
    bonus += bonus / 2;
  if (char_get_bonuses(ch, BONUS_LONER))
    bonus += bonus * 0.1;
  send_to_char(g, ch, "You feel your spirit grow stronger @D[@G+{}@D]@n.\r\n", add_commas(bonus));
  gainBaseKI(ch, bonus, true);
  exercise_wait(ch, gravity);
  decCurKI(g, ch, cost, 0);
}

/* This handles many player specific routines. It may be a bit too bloated though. */
void base_update(GameWorld *g)
{
  int cash = false, inc = 0;
  int countch = false, pcoun = 0;

  if (g->INTERESTTIME != 0 && g->INTERESTTIME <= game_now(g) && game_now(g) != 0) {
    g->INTERESTTIME = game_now(g) + 86400;
    g->LASTINTEREST = game_now(g);
    save_mud_time(g, &g->time_info);
    cash    = true;
    countch = true;
  }

  if (g->TOPCOUNTDOWN > 0) {
    g->TOPCOUNTDOWN -= 4;
  }

  for (descriptor_data *d : each_desc(g)) {
    if (!desc_is_playing(d))
      continue;
    if (char_is_npc(d->character)) {
      if (char_get_absorbing(g, d->character) && char_get_in_room(d->character) != char_get_in_room(char_get_absorbing(g, d->character))) {
        send_to_char(g, d->character, "You stop absorbing {}!\r\n", char_get_name(char_get_absorbing(g, d->character)));
        char_set_absorbby(char_get_absorbing(g, d->character), nullptr);
        char_set_absorbing(d->character, nullptr);
      }
      if (char_is_android(d->character) && char_get_absorbing(g, d->character) && rand_number(g, 1, 10) >= 7) {
        int64_t drain1 = getMaxKI(g, d->character) * 0.01, drain2 = getMaxST(g, d->character) * 0.01;
        struct char_data *drained = char_get_absorbing(g, d->character);
        if ((getCurST(g, drained)) - drain2 < 0) {
          drain2 = (getCurST(g, drained));
        }
        if ((getCurKI(g, drained)) - drain1 < 0) {
          drain1 = (getCurKI(g, drained));
        }
        incCurST(g, d->character, drain2, true);
        incCurKI(g, d->character, drain1, true);
        incCurHealth(g, d->character, drain1 * .5, true);

        if (isFullKI(d->character) && isFullST(d->character)) {
          do_absorb(g, d->character, "", 0, 0);
        }
      }
      continue;
    }
    if (countch == true) {
      pcoun += 1;
    }
    if (!char_is_npc(d->character) && rand_number(g, 1, 15) >= 14) {
      ash_burn(g, d->character);
    }
    int64_t forty_lf = getMaxLF(g, d->character) * 0.4;
    if (char_aff_flagged(d->character, AFF_CURSE) && getCurLF(g, d->character) > forty_lf) {
      decCurLFPercent(g, d->character, .01, 0);
      demon_refill_lf(g, d->character, getMaxLF(g, d->character) * 0.01);

      if (getCurLF(g, d->character) < forty_lf) {
        incCurLF(g, d->character, forty_lf - getCurLF(g, d->character), true);
      }
    }
    if (char_get_backstabcool(d->character) > 0) {
      char_set_backstabcool(d->character, char_get_backstabcool(d->character) - (1));
    }
    if (char_plr_flagged(d->character, PLR_GOOP) && char_get_gooptime(d->character) == 60) {
      if (char_is_bio(d->character)) {
        act(g, "@GConciousness slowly returns to you. You realize quickly that some of your cells have survived. You take control of your regenerative processes and focus on growing a new body!@n", true, d->character, nullptr, nullptr, TO_CHAR);
      } else {
        act(g, "@MSlowly you regain conciousness. The various split off chunks of your body begin to likewise stir.@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@MYou think you notice the chunks of @m$n@M's moving slightly.@n", true, d->character, nullptr, nullptr, TO_ROOM);
      }
      char_set_gooptime(d->character, char_get_gooptime(d->character) - 1);
    } else if (char_plr_flagged(d->character, PLR_GOOP) && char_get_gooptime(d->character) == 30) {
      if (char_is_bio(d->character)) {
        act(g, "@GFrom the collection of cells growing a crude form of your body starts to take shape!@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@GYou start to notice a large mass of pulsing flesh growing before you!@n", true, d->character, nullptr, nullptr, TO_ROOM);
      } else {
        act(g, "@MYou will the various chunks of your body to return and slowly more and more of them begin to fly into you. Your body begins to grow larger and larger as this process unfolds!@n ", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@MThe various chunks of @m$n@M's body start to fly into the largest chunk! As the chunks collide they begin to form a larger and still growing blob of goo!@n", true, d->character, nullptr, nullptr, TO_ROOM);
      }
      char_set_gooptime(d->character, char_get_gooptime(d->character) - 1);
    } else if (char_plr_flagged(d->character, PLR_GOOP) && char_get_gooptime(d->character) == 15) {
      if (char_is_bio(d->character)) {
        act(g, "@GYour body has almost reached its previous form! Only a little more regenerating is needed!@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@GThe lump of flesh has now grown to the size where the likeness of @g$n@G can be seen of it! It appears that $e is regenerating $s body from what was only a few cells!@n", true, d->character, nullptr, nullptr, TO_ROOM);
      } else {
        act(g, "@MYour body has reached half its previous size as your limbs ooze slowly out into their proper shape!@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@m$n@M's body has regenerated to half its previous size! Slowly $s limbs ooze out into their proper shape! It won't be long now till $e has fully regenerated!@n", true, d->character, nullptr, nullptr, TO_ROOM);
      }
      char_set_gooptime(d->character, char_get_gooptime(d->character) - 1);
    } else if (char_plr_flagged(d->character, PLR_GOOP) && char_get_gooptime(d->character) == 0) {
      if (char_is_bio(d->character)) {
        restoreHealth(d->character, true);
        act(g, "@GYour body has fully regenerated! You flex your arms and legs outward with a rush of renewed strength!@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@g$n@G's body has fully regenerated! Suddenly $e flexes $s arms and legs and a rush of power erupts from off of $s body!@n", true, d->character, nullptr, nullptr, TO_ROOM);
      }
      //Zenkai Boost
      else if (char_is_saiyan(d->character)) {

        setCurHealthPercent(g, d->character, 0.5);
        double curKiPerc = getCurKIPercent(g, d->character);
        // if less than 25%, restore up to 25%
        if (curKiPerc < 0.25) {
          setCurKIPercent(g, d->character, 0.25);
        }
        double curSTPerc = getCurSTPercent(g, d->character);
        // if less than 25%, restore up to 25%
        if (curSTPerc < 0.25) {
          setCurSTPercent(g, d->character, 0.25);
        }

        if (!char_in_arena(g, d->character)) {
          int64_t zenkaiPL, zenkaiKi, zenkaiSt;
          zenkaiPL = getBasePL(d->character) * 0.03;
          zenkaiKi = getBaseKI(d->character) * 0.015;
          zenkaiSt = getBaseST(d->character) * 0.015;

          gainBasePL(d->character, zenkaiPL, false);
          gainBaseKI(d->character, zenkaiKi, false);
          gainBaseST(d->character, zenkaiSt, false);

          send_to_char(g, d->character, "@D[@YZ@ye@wn@Wk@Ya@yi @YB@yo@wo@Ws@Yt@D] @WYou feel much stronger!\r\n");
          send_to_char(g, d->character, "@D[@RPL@Y:@n+{}@D] @D[@CKI@Y:@n+{}@D] @D[@GSTA@Y:@n+{}@D]@n\r\n", add_commas(zenkaiPL), add_commas(zenkaiKi), add_commas(zenkaiSt));
        }
        act(g, "@RYou collapse to the ground, body pushed beyond the typical limits of exhaustion. The passage of time distorts and an indescribable amount of time passes as raw emotions pass through your very being. Your eyes open and focus with a newfound clarity as your unadulterated emotions and feelings revive you for a second wind!@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@r$n@R collapses to the ground, seemingly dead. After a brief moment, their eyes flash open with a determined look on their face!", true, d->character, nullptr, nullptr, TO_ROOM);
      } else {
        restoreHealth(d->character, true);
        act(g, "@MYour body has fully regenerated! You scream out in triumph and a short gust of steam erupts from your pores!@n", true, d->character, nullptr, nullptr, TO_CHAR);
        act(g, "@m$n@M's body has fully regenerated! Suddenly $e screams out in gleeful triumph and short gust of steam erupts from $s skin pores!", true, d->character, nullptr, nullptr, TO_ROOM);
      }
      char_plr_set(d->character, PLR_GOOP, false);
    } else {
      char_set_gooptime(d->character, char_get_gooptime(d->character) - 1);
    }
    if (char_get_con_cooldown(d->character) > 0) {
      char_set_con_cooldown(d->character, char_get_con_cooldown(d->character) - (2));
      if (char_get_con_cooldown(d->character) <= 0) {
        char_set_con_cooldown(d->character, 0);
        send_to_char(g, d->character, "You can concentrate again.\r\n");
      }
    }
    /* Andros Start */
    if (char_get_con_sdcooldown(d->character) > 0) {
      char_set_con_sdcooldown(d->character, char_get_con_sdcooldown(d->character) - (10));
      if (char_get_con_sdcooldown(d->character) <= 0) {
        char_set_con_sdcooldown(d->character, 0);
        send_to_char(g, d->character, "Your body has recovered from your last selfdestruct.\r\n");
      }
    } /* Andros End */
    if (char_get_carrying(g, d->character)) {
      if (char_get_in_room(char_get_carrying(g, d->character)) != char_get_in_room(d->character)) {
        carry_drop(g, d->character, 3);
      }
    }
    if (char_get_defender(g, d->character)) {
      if (char_get_in_room(d->character) != char_get_in_room(char_get_defender(g, d->character))) {
        char_set_defending(char_get_defender(g, d->character), nullptr);
        char_set_defender(d->character, nullptr);
      }
    }
    if (char_get_defending(g, d->character)) {
      if (char_get_in_room(d->character) != char_get_in_room(char_get_defending(g, d->character))) {
        char_set_defender(char_get_defending(g, d->character), nullptr);
        char_set_defending(d->character, nullptr);
      }
    }
    if (char_plr_flagged(d->character, PLR_TRANSMISSION)) {
      char_plr_set(d->character, PLR_TRANSMISSION, false);
    }
    if (!char_get_fighting(g, d->character) && char_aff_flagged(d->character, AFF_POSITION)) {
      char_aff_set(d->character, AFF_POSITION, false);
    }
    if (char_get_sits(g, d->character)) {
      if (char_get_in_room(d->character) != obj_get_in_room(char_get_sits(g, d->character))) {
        struct obj_data *chair = char_get_sits(g, d->character);
        obj_set_sitting(chair, nullptr);
        char_set_sits(d->character, nullptr);
      }
    }
    if (char_get_ping(d->character) >= 1) {
      char_set_ping(d->character, char_get_ping(d->character) - (1));
      if (char_plr_flagged(d->character, PLR_PILOTING) && char_get_ping(d->character) == 0) {
        send_to_char(g, d->character, "Your radar is ready to calculate the direction of another destination.\r\n");
      }
    }
    if (char_get_admlevel(d->character) < 1 && g->TOPCOUNTDOWN <= 0 && char_get_level(d->character) > 0) {
      topWrite(g, d->character);
    }
    if (char_plr_flagged(d->character, PLR_SELFD) && !char_plr_flagged(d->character, PLR_SELFD2)) {
      if (rand_number(g, 4, 100) < char_get_skill(d->character, SKILL_SELFD)) {
        send_to_char(g, d->character, "You feel you are ready to self destruct!\r\n");
        char_plr_set(d->character, PLR_SELFD2, true);
      }
    }
    if (!char_get_fighting(g, d->character) && char_get_combo(d->character) > -1) {
      char_set_combo(d->character, -1);
      char_set_combhits(d->character, 0);
    }
    if (char_moon_ok(g, d->character)) {
      oozaru_transform(g, d->character);
    }
    if (cash == true && char_get_bank_gold(d->character) > 0) {
      inc = char_get_bank_interest(d->character);
      char_set_lastint(d->character, g->LASTINTEREST);
      char_set_bank_gold(d->character, char_get_bank_gold(d->character) + (inc));
      send_to_char(g, d->character, "@cBank Interest@D: @Y{}@n\r\n", add_commas(inc));
    }
    if (!char_is_npc(d->character)) {
      check_eq(g, d->character);
    }
    if (!char_is_npc(d->character) && room_get_geffect(g, char_get_in_room(d->character)) >= 1 && rand_number(g, 1, 100) >= 96) {
      if (room_get_geffect(g, char_get_in_room(d->character)) <= 4) {
        switch (rand_number(g, 1, 4)) {
          case 1:
            act(g, "@RLava spews up violently from the cracks in the ground!@n", false, d->character, nullptr, nullptr, TO_ROOM);
            act(g, "@RLava spews up violently from the cracks in the ground!@n", false, d->character, nullptr, nullptr, TO_CHAR);
            break;
          case 2:
            act(g, "@RThe lava bubbles and gives off tremendous heat!@n", false, d->character, nullptr, nullptr, TO_ROOM);
            act(g, "@RThe lava bubbles and gives off tremendous heat!@n", false, d->character, nullptr, nullptr, TO_CHAR);
            break;
          case 3:
            act(g, "@RNoxious fumes rise from the bubbling lava!@n", false, d->character, nullptr, nullptr, TO_ROOM);
            act(g, "@RNoxious fumes rise from the bubbling lava!@n", false, d->character, nullptr, nullptr, TO_CHAR);
            break;
          case 4:
            act(g, "@RSome of the lava cools as it spreads further from the source!@n", false, d->character, nullptr, nullptr, TO_ROOM);
            act(g, "@RSome of the lava cools as it spreads further from the source!@n", false, d->character, nullptr, nullptr, TO_CHAR);
            break;
        }
        room_set_geffect(g, char_get_in_room(d->character), room_get_geffect(g, char_get_in_room(d->character)) + (1));
      } else if (room_get_geffect(g, char_get_in_room(d->character)) == 5) {
        act(g, "@RLava covers the entire area now!@n", false, d->character, nullptr, nullptr, TO_ROOM);
        act(g, "@RLava covers the entire area now!@n", false, d->character, nullptr, nullptr, TO_CHAR);
        room_set_geffect(g, char_get_in_room(d->character), room_get_geffect(g, char_get_in_room(d->character)) + (1));
      }
    }
    if (char_get_absorbing(g, d->character) && char_get_in_room(d->character) != char_get_in_room(char_get_absorbing(g, d->character))) {
      send_to_char(g, d->character, "You stop absorbing {}!\r\n", char_get_name(char_get_absorbing(g, d->character)));
      char_set_absorbby(char_get_absorbing(g, d->character), nullptr);
      char_set_absorbing(d->character, nullptr);
    }
    if (char_is_android(d->character) && char_get_absorbing(g, d->character)) {
      if (getCurST(g, char_get_absorbing(g, d->character)) < (getMaxST(g, d->character) / 15) &&
          getCurKI(g, char_get_absorbing(g, d->character)) < (getMaxKI(g, d->character) / 15)) {
        act(g, "@WYou stop absorbing stamina and ki from @c$N as they don't have enough for you to take@W!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_CHAR);
        act(g, "@C$n@W stops absorbing stamina and ki from you!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_VICT);
        act(g, "@C$n@W stops absorbing stamina and ki from @c$N@w!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_NOTVICT);
        if (!char_get_fighting(g, d->character) || char_get_fighting(g, d->character) != char_get_absorbing(g, d->character)) {
          set_fighting(g, d->character, char_get_absorbby(g, char_get_absorbing(g, d->character)));
        }
        if (!char_get_fighting(g, char_get_absorbby(g, char_get_absorbing(g, d->character))) || char_get_fighting(g, char_get_absorbby(g, char_get_absorbing(g, d->character))) != d->character) {
          set_fighting(g, char_get_absorbby(g, char_get_absorbing(g, d->character)), d->character);
        }
        char_set_absorbby(char_get_absorbing(g, d->character), nullptr);
        char_set_absorbing(d->character, nullptr);
      }
    }
    if (char_is_android(d->character) && char_get_absorbing(g, d->character) && rand_number(g, 1, 9) >= 6) {
      if (getCurST(g, char_get_absorbing(g, d->character)) > (getMaxST(g, d->character) / 15) ||
          getCurKI(g, char_get_absorbing(g, d->character)) > (getMaxKI(g, d->character) / 15)) {

        incCurKI(g, d->character, getMaxKI(g, d->character) * .08, true);
        incCurST(g, d->character, getMaxST(g, d->character) * .08, true);

        decCurKI(g, char_get_absorbing(g, d->character), getMaxKI(g, d->character) / 20, 1);
        decCurST(g, char_get_absorbing(g, d->character), getMaxST(g, d->character) / 20, 1);

        act(g, "@WYou absorb stamina and ki from @c$N@W!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_CHAR);
        act(g, "@C$n@W absorbs stamina and ki from you!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_VICT);
        send_to_char(g, char_get_absorbing(g, d->character), "@wTry 'escape'!@n\r\n");
        act(g, "@C$n@W absorbs stamina and ki from @c$N@w!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_NOTVICT);
        if (getCurPL(g, d->character) < getEffMaxPL(g, d->character)) {
          incCurHealth(g, d->character, getMaxKI(g, d->character) * .04, true);
          send_to_char(g, d->character, "@CYou convert a portion of the absorbed energy into refilling your powerlevel.@n\r\n");
        }

        if (isFullST(d->character) && isFullKI(d->character)) {

          act(g, "@WYou stop absorbing stamina and ki from @c$N as you are full@W!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_CHAR);
          act(g, "@C$n@W stops absorbing stamina and ki from you!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_VICT);
          act(g, "@C$n@W stops absorbing stamina and ki from @c$N@w!@n", true, d->character, nullptr, char_get_absorbing(g, d->character), TO_NOTVICT);
          if (!char_get_fighting(g, d->character) || char_get_fighting(g, d->character) != char_get_absorbing(g, d->character)) {
            set_fighting(g, d->character, char_get_absorbby(g, char_get_absorbing(g, d->character)));
          }
          if (!char_get_fighting(g, char_get_absorbby(g, char_get_absorbing(g, d->character))) || char_get_fighting(g, char_get_absorbby(g, char_get_absorbing(g, d->character))) != d->character) {
            set_fighting(g, char_get_absorbby(g, char_get_absorbing(g, d->character)), d->character);
          }
          char_set_absorbby(char_get_absorbing(g, d->character), nullptr);
          char_set_absorbing(d->character, nullptr);
        }
        bool sum                 = !is_soft_cap(d->character, 0, 1.0);
        bool mum                 = !is_soft_cap(d->character, 2, 1.0);
        bool ium                 = !is_soft_cap(d->character, 1, 1.0);
        struct char_data *leader = char_get_master(g, d->character) ? char_get_master(g, d->character) : d->character;
        if (sum) {
          if (rand_number(g, 1, 8) >= 6) {
            int gain = rand_number(g, char_get_level(d->character) / 2, char_get_level(d->character) * 3) + (char_get_level(d->character) * 18);
            if (char_get_level(d->character) > 30) {
              gain += rand_number(g, char_get_level(d->character) * 2, char_get_level(d->character) * 4) + (char_get_level(d->character) * 50);
            }
            if (char_get_level(d->character) > 60) {
              gain *= 2;
            }
            if (char_get_level(d->character) > 80) {
              gain *= 3;
            }
            if (char_get_level(d->character) > 90) {
              gain *= 4;
            }
            send_to_char(g, d->character, "@gYou gain +@G{:d}@g permanent powerlevel!@n\r\n", gain);
            if (group_bonus(g, d->character, 2) == 7) {
              if (char_plr_flagged(leader, PLR_SENSEM)) {
                int gbonus = gain * 0.15;
                gain += gbonus;
                send_to_char(g, d->character, "The leader of your group conveys an extra bonus! @D[@G+{}@D]@n \r\n", add_commas(gbonus));
              }
            }
            gainBasePL(d->character, gain, false);
          }
        }
        if (mum) {
          if (rand_number(g, 1, 8) >= 6) {
            int gain = rand_number(g, char_get_level(d->character) / 2, char_get_level(d->character) * 3) + (char_get_level(d->character) * 18);
            if (char_get_level(d->character) > 30) {
              gain += rand_number(g, char_get_level(d->character) * 2, char_get_level(d->character) * 4) + (char_get_level(d->character) * 50);
            }
            if (char_get_level(d->character) > 60) {
              gain *= 2;
            }
            if (char_get_level(d->character) > 80) {
              gain *= 3;
            }
            if (char_get_level(d->character) > 90) {
              gain *= 4;
            }
            send_to_char(g, d->character, "@gYou gain +@G{:d}@g permanent stamina!@n\r\n", gain);
            if (group_bonus(g, d->character, 2) == 7) {
              if (char_plr_flagged(leader, PLR_SENSEM)) {
                int gbonus = gain * 0.15;
                gain += gbonus;
                send_to_char(g, d->character, "The leader of your group conveys an extra bonus! @D[@G+{}@D]@n \r\n", add_commas(gbonus));
              }
            }
            gainBaseST(d->character, gain, false);
          }
        }
        if (ium) {
          if (rand_number(g, 1, 8) >= 6) {
            int gain = rand_number(g, char_get_level(d->character) / 2, char_get_level(d->character) * 3) + (char_get_level(d->character) * 18);
            if (char_get_level(d->character) > 30) {
              gain += rand_number(g, char_get_level(d->character) * 2, char_get_level(d->character) * 4) + (char_get_level(d->character) * 50);
            }
            if (char_get_level(d->character) > 60) {
              gain *= 2;
            }
            if (char_get_level(d->character) > 80) {
              gain *= 3;
            }
            if (char_get_level(d->character) > 90) {
              gain *= 4;
            }
            send_to_char(g, d->character, "@gYou gain +@G{:d}@g permanent ki!@n\r\n", gain);
            if (char_get_master(g, d->character) && group_bonus(g, d->character, 2) == 7) {
              if (char_plr_flagged(leader, PLR_SENSEM)) {
                int gbonus = gain * 0.15;
                gain += gbonus;
                send_to_char(g, d->character, "The leader of your group conveys an extra bonus! @D[@G+{}@D]@n \r\n", add_commas(gbonus));
              }
            }
            gainBaseKI(d->character, gain, false);
          }
        }
        if (!sum) {
          if (rand_number(g, 1, 8) >= 6) {
            int gain = 1;
            send_to_char(g, d->character, "@gYou gain +@G{:d}@g permanent powerlevel. You may need to level.@n\r\n", gain);
            gainBasePL(d->character, gain, false);
          }
        }
        if (!mum) {
          if (rand_number(g, 1, 8) >= 6) {
            int gain = 1;
            send_to_char(g, d->character, "@gYou gain +@G{:d}@g permanent stamina. You may need to level.@n\r\n", gain);
            gainBaseST(d->character, gain, false);
          }
        }
        if (!ium) {
          if (rand_number(g, 1, 8) >= 6) {
            int gain = 1;
            send_to_char(g, d->character, "@gYou gain +@G{:d}@g permanent ki. You may need to level.@n\r\n", gain);
            gainBaseKI(d->character, gain, false);
          }
        }
      }
    }
    if (char_get_blocks(g, d->character)) {
      struct char_data *vict = char_get_blocks(g, d->character);
      if (char_get_in_room(vict) != char_get_in_room(d->character)) {
        char_set_blocked(vict, nullptr);
        char_set_blocks(d->character, nullptr);
      }
    }
    if (char_get_overf(d->character) == true) {
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "OVERFLOW: {} has caused an overflow, check for illegal activity.", char_get_name(d->character));
      char_set_overf(d->character, false);
    }
    if (char_get_spam(d->character) > 0) {
      char_set_spam(d->character, 0);
    } else
      continue;
  }

  if (countch == true) {
    g->PCOUNT    = pcoun;
    g->PCOUNTDAY = game_now(g);
  }

  if (g->TOPCOUNTDOWN <= 0) {
    g->TOPCOUNTDOWN = 60;
  }
}
