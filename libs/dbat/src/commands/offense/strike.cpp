/* strike.cpp: what every strike does; see strike.hpp */
#include "strike.hpp"

#include <string>

#include "dbat/args.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/combat.hpp"
#include "dbat/fight.hpp"
#include "dbat/techniques.hpp"

namespace {

/* A kicker's legs: false, saying so, if they can't kick; a broken leg, if the other is gone, is hurt further */
bool legs_ok(GameWorld *g, struct char_data *ch)
{
  if (!limb_ok(g, ch, 1))
    return false;
  static constexpr struct {
    int limb, other;
    const char *worse, *gone_char, *gone_room;
  } legs[] = {
      {2, 3, "Using your broken right leg has damaged it more!@n\r\n", "@RYour right leg has fallen apart!@n", "@r$n@R's right leg has fallen apart!@n"},
      {3, 2, "Using your broken left leg has damaged it more!@n\r\n",  "@RYour left leg has fallen apart!@n",  "@r$n@R's left leg has fallen apart!@n" },
  };
  for (const auto &leg : legs) {
    if (char_get_limb_condition(ch, leg.limb) > 0 && char_get_limb_condition(ch, leg.limb) < 50 &&
        char_get_limb_condition(ch, leg.other) < 0) {
      send_to_char(g, ch, leg.worse);
      char_set_limb_condition(ch, leg.limb, char_get_limb_condition(ch, leg.limb) - rand_number(g, 3, 5));
      if (char_get_limb_condition(ch, leg.limb) < 0) {
        act(g, leg.gone_char, true, ch, nullptr, nullptr, TO_CHAR);
        act(g, leg.gone_room, true, ch, nullptr, nullptr, TO_ROOM);
      }
      break;
    }
  }
  return true;
}

void pay(strike_shot &s, int64_t stamina)
{
  pcost(s.g, s.ch, 0, stamina);
}

/* The victim couldn't avoid it: where it hit, and what that did. */
void land(strike_shot &s)
{
  static constexpr strike_spot standard[5] = {strike_spot::body, strike_spot::head, strike_spot::body,
                                              strike_spot::arm, strike_spot::leg};
  GameWorld *g                             = s.g;
  const strike_attack &atk                 = *s.attack;

  s.dmg = damtype(g, s.ch, atk.damtype, s.skill, 0);
  if (atk.bonus_damage)
    atk.bonus_damage(s);
  int where = roll_hitloc(g, s.ch, s.vict, s.skill);
  if (where < 1 || where > 5)
    return;
  act_triple(g, atk.hits[where - 1], s.ch, nullptr, s.vict);
  if (atk.on_spot)
    atk.on_spot(s, where);
  int leg = 0, limb = 0, eq = 0; /* hurt()'s first two arguments; dam_eq_loc(): 1 arms, 2 legs, 3 head, 4 body */
  switch ((atk.spots ? atk.spots : standard)[where - 1]) {
    case strike_spot::body:
      if (char_get_bonuses(s.ch, BONUS_SOFT))
        s.dmg *= calc_critical(g, s.ch, 2);
      eq = 4;
      break;
    case strike_spot::head:
      s.dmg *= atk.head_crit ? atk.head_crit(s) : calc_critical(g, s.ch, 0);
      eq = 3;
      break;
    case strike_spot::arm:
      s.dmg *= calc_critical(g, s.ch, 1);
      limb = atk.limb_hurt;
      eq   = 1;
      break;
    case strike_spot::leg:
      s.dmg *= calc_critical(g, s.ch, 1);
      leg  = atk.leg_flag;
      limb = atk.limb_hurt;
      eq   = 2;
      break;
  }
  if (atk.before_damage)
    atk.before_damage(s, where);
  hurt(g, leg, limb, s.ch, s.vict, nullptr, s.dmg, 0);
  dam_eq_loc(g, s.vict, eq);
}

void at_victim(strike_shot &s)
{
  GameWorld *g             = s.g;
  const strike_attack &atk = *s.attack;
  int pry = 2, dge = 2, blk = 2;
  auto avoids = [&](int chance) { /* the victim blocks or dodges it */
                                  return chance > (atk.avoid_die ? rand_number(g, 1, atk.avoid_die) : axion_dice(g, 10));
  };

  if (!can_kill(g, s.ch, s.vict, nullptr, 0))
    return;
  if (handle_defender(g, s.vict, s.ch))
    s.vict = char_get_defender(g, s.vict);
  improve_skill(g, s.ch, atk.skill, atk.improve_mode);
  int index = check_def(g, s.vict); /* the victim's parry, block and dodge */
  int prob  = roll_accuracy(g, s.ch, s.skill, false);
  int perc  = chance_to_hit(g, s.ch);
  if (char_is_kabito(s.ch) && !char_is_npc(s.ch) && char_get_skills_base(s.ch, SKILL_STYLE) >= 75)
    perc -= perc * 0.2;
  index -= handle_speed(g, s.ch, s.vict);
  int avo = index / 4;
  if (atk.defend)
    atk.defend(s, pry, blk, dge);
  else
    handle_defense(g, s.vict, &pry, &blk, &dge);
  prob -= avo;
  tech_handle_posmodifier(s.vict, &pry, &blk, &dge, &prob);

  if (!tech_handle_zanzoken(g, s.ch, s.vict, atk.name)) {
    if (!atk.keeps_combo) {
      char_set_combo(s.ch, -1);
      char_set_combhits(s.ch, 0);
    }
    pay(s, s.stcost / 2);
    pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 200);
    return;
  }

  if (prob < perc - 20) {
    if (getCurST(g, s.vict) <= 0) {
      act_triple(g, atk.miss_spent, s.ch, nullptr, s.vict);
      if (atk.spent_hurts_first) {
        hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 0);
        pay(s, atk.spent_free ? 0 : s.stcost / 2);
      } else {
        pay(s, s.stcost / 2);
        hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 0);
      }
      return;
    }
    if (pry > rand_number(g, 1, 140) && (!char_is_npc(s.vict) || !char_mob_flagged(s.vict, MOB_DUMMY))) {
      act_triple(g, atk.parry, s.ch, nullptr, s.vict);
      improve_skill(g, s.vict, SKILL_PARRY, 0);
      pay(s, s.stcost / 2);
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 500);
      s.dmg = damtype(g, s.vict, -2, s.skill, 0);
      s.dmg *= calc_critical(g, s.ch, atk.parry_crit);
      hurt(g, 0, 0, s.vict, s.ch, nullptr, s.dmg, -1); /* the counterblow */
      return;
    }
    if (avoids(blk)) {
      act_triple(g, atk.block, s.ch, nullptr, s.vict);
      improve_skill(g, s.vict, SKILL_BLOCK, 0);
      pay(s, s.stcost / 2);
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 500);
      s.dmg = damtype(g, s.ch, atk.block_damtype.value_or(atk.damtype), s.skill, 0) / 4;
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 0);
      return;
    }
    if (avoids(dge)) {
      act_triple(g, atk.dodge, s.ch, nullptr, s.vict);
      improve_skill(g, s.vict, SKILL_DODGE, 0);
      pay(s, s.stcost / 2);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 0);
      return;
    }
    act_triple(g, atk.miss, s.ch, nullptr, s.vict);
    pay(s, s.stcost / 2);
    hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 0);
    return;
  }

  land(s);
  tech_handle_fireshield(g, s.ch, s.vict, atk.fireshield);
  if (atk.after_hit)
    atk.after_hit(s);
  pay(s, s.stcost);
  handle_multihit(g, s.ch, s.vict);
}

void at_object(strike_shot &s)
{
  GameWorld *g = s.g;

  /* can_kill() refuses a broken object, so it is never broken here */
  if (!can_kill(g, s.ch, nullptr, s.obj, 0))
    return;
  s.dmg = getCurPL(g, s.ch) / 10000 + char_get_aff_abils_str(s.ch);
  act(g, s.attack->obj_to_char, s.attack->obj_hide, s.ch, s.obj, nullptr, TO_CHAR);
  act(g, s.attack->obj_to_room, s.attack->obj_hide, s.ch, s.obj, nullptr, TO_ROOM);
  hurt(g, 0, 0, s.ch, nullptr, s.obj, s.dmg, 0);
  pay(s, s.stcost);
}

} // namespace

void perform_strike(GameWorld *g, struct char_data *ch, std::string_view argument, const strike_attack &attack)
{
  strike_shot s{.g = g, .ch = ch, .attack = &attack, .stcost = physical_cost(g, ch, attack.skill)};
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (attack.not_while_singing && playing_song(g, ch))
    return;
  if (attack.skill_before_grav ? !check_skill(g, ch, attack.skill) || !can_grav(g, ch)
                               : !can_grav(g, ch) || !check_skill(g, ch, attack.skill))
    return;
  switch (attack.needs) {
    case strike_needs::hands:
      if (!limb_ok(g, ch, 0))
        return;
      if (char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
        send_to_char(g, ch, "Your hands are full!\r\n");
        return;
      }
      break;
    case strike_needs::legs:
      if (!legs_ok(g, ch))
        return;
      break;
    case strike_needs::tail:
      if (!char_plr_flagged(ch, PLR_TAIL) && !char_is_npc(ch)) {
        send_to_char(g, ch, "You have no tail!\r\n");
        return;
      }
      break;
    case strike_needs::nothing:
      break;
  }
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / attack.stamina_share))
    return;
  s.skill = init_skill(g, ch, attack.skill);
  if (!tech_handle_targeting(g, ch, arg, &s.vict, &s.obj))
    return;
  handle_cooldown(g, ch, attack.cooldown_for ? attack.cooldown_for(s) : attack.cooldown);
  if (s.vict)
    at_victim(s);
  else if (s.obj)
    at_object(s);
}
