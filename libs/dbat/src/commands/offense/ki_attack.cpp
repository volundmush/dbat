/* ki_attack.cpp: what every ki attack does; see ki_attack.hpp */
#include "ki_attack.hpp"

#include <string>

#include "dbat/args.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/fight.hpp"
#include "dbat/techniques.hpp"

namespace {

/* A sword technique needs a sword: false, saying so, if ch isn't wielding one */
bool holds_sword(GameWorld *g, struct char_data *ch)
{
  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(g, ch, "You need to wield a sword to use this.\r\n");
    return false;
  }
  if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) != TYPE_SLASH - TYPE_HIT) {
    send_to_char(g, ch, "You are not wielding a sword, you need one to use this technique.\r\n");
    return false;
  }
  return true;
}

int dodge_damtype(const ki_attack &atk)
{
  return atk.dodge_damtype ? atk.dodge_damtype : atk.damtype;
}

int perfection(const ki_shot &s)
{
  return s.attack->perfection ? char_get_skills_perf(s.ch, s.attack->skill) : 0;
}

/* The attacker pays for the attack: perfection 3 takes 5% off, if that leaves at least the minimum. */
void pay(ki_shot &s, ki_outcome how)
{
  if (s.attack->cost)
    pcost(s.g, s.ch, s.attack->cost(s), 0);
  else if (perfection(s) == 3 && s.attperc > s.minimum)
    pcost(s.g, s.ch, s.attperc - 0.05, 0);
  else
    pcost(s.g, s.ch, s.attperc, 0);
  if (s.attack->after_paying)
    s.attack->after_paying(s, how);
}

/* The victim couldn't avoid it: where it hit, and what that did. */
void land(ki_shot &s)
{
  static constexpr ki_spot standard[6] = {ki_spot::body, ki_spot::head, ki_spot::body,
                                          ki_spot::arm, ki_spot::leg, ki_spot::body};
  GameWorld *g                         = s.g;
  const ki_attack &atk                 = *s.attack;

  s.dmg = damtype(g, s.ch, atk.damtype, s.skill, s.attperc);
  if (atk.bonus_damage)
    atk.bonus_damage(s);
  s.pl_before = getCurPL(g, s.vict);
  int where   = atk.locate ? atk.locate(s) : roll_hitloc(g, s.ch, s.vict, s.skill);
  if (atk.aim)
    where = atk.aim(s, where);
  if (where < 1 || where > 6 || !atk.hits[where - 1].to_char)
    return;
  ki_act(s, atk.hits[where - 1]);
  if (atk.strike && atk.strike(s, where))
    return;
  switch ((atk.spots ? atk.spots : standard)[where - 1]) {
    case ki_spot::body:
      if (atk.soft_crit && char_get_bonuses(s.ch, BONUS_SOFT))
        s.dmg *= calc_critical(g, s.ch, 2);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 4); /* 1 arms, 2 legs, 3 head, 4 body */
      break;
    case ki_spot::head:
      if (atk.head_mult)
        s.dmg *= atk.head_mult;
      else if (!atk.powerhit_crit)
        s.dmg *= calc_critical(g, s.ch, 0);
      else
        s.dmg *= char_get_bonuses(s.ch, BONUS_POWERHIT) ? 5 : 3;
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 3);
      break;
    case ki_spot::arm:
      s.dmg *= calc_critical(g, s.ch, atk.arm_crit);
      hurt(g, 0, atk.limb_hurt, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 1);
      break;
    case ki_spot::leg:
      s.dmg *= calc_critical(g, s.ch, 1);
      hurt(g, atk.leg_flag, atk.limb_hurt, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 2);
      break;
  }
}

/* One shot at the victim, after zanzoken: avoided, or where it landed. pays: whether the attacker pays for the attack
 * now, as they do on the last shot. Returns how it ended. */
ki_outcome shoot_once(ki_shot &s, int prob, int perc, int pry, int blk, int dge, bool pays)
{
  GameWorld *g         = s.g;
  const ki_attack &atk = *s.attack;
  auto pay_if          = [&](ki_outcome how) {
    if (pays)
      pay(s, how);
  };
  auto steer = [&]() { /* a homing or guided attack the victim got away from */
                       dodge_ki(g, s.ch, s.vict, atk.steers, dodge_damtype(atk), s.skill, atk.skill);
  };

  if (atk.through ? !atk.through(s, prob, perc) : prob < perc - 20) {
    if (getCurST(g, s.vict) <= 0) {
      ki_act(s, atk.miss_spent);
      if (atk.steers)
        steer();
      if (atk.homing) /* quirk: before paying, and again after */
        hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      pay_if(ki_outcome::spent);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      if (atk.guided) /* quirk: twice */
        hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      return ki_outcome::spent;
    }
    if (atk.absorb != ki_absorb::none && tech_handle_android_absorb(g, s.ch, s.vict)) {
      if (atk.absorb == ki_absorb::costs_one)
        pcost(g, s.ch, 1, 0);
      else
        pay(s, ki_outcome::absorbed);
      return ki_outcome::absorbed;
    }
    if (atk.parry.to_char && pry > rand_number(g, 1, 140) &&
        (!char_is_npc(s.vict) || !char_mob_flagged(s.vict, MOB_DUMMY))) {
      ki_act(s, atk.parry);
      if (atk.on_parry) {
        atk.on_parry(s);
        return ki_outcome::blocked;
      }
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 200);
      if (atk.parry_returns)
        parry_ki(g, s.attperc, s.ch, s.vict, atk.parry_name ? atk.parry_name : atk.name, prob, perc, s.skill,
                 atk.damtype);
      pay_if(ki_outcome::blocked);
      if (atk.guided) {
        hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      } else if (atk.parry_scatters) {
        room_vnum here = char_get_in_room(s.ch);
        if (room_get_dmg(g, here) <= atk.dodge_dmg_cap)
          room_set_dmg(g, here, room_get_dmg(g, here) + atk.dodge_dmg);
      }
      return ki_outcome::blocked;
    }
    if (atk.blockable && blk > axion_dice(g, 10)) {
      ki_act(s, atk.block);
      if (atk.block_improves)
        improve_skill(g, s.vict, SKILL_BLOCK, 0);
      pay_if(ki_outcome::blocked);
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 500);
      s.dmg = damtype(g, s.ch, atk.damtype, s.skill, s.attperc) / 4;
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      return ki_outcome::blocked;
    }
    if (dge > axion_dice(g, 10)) {
      ki_act(s, atk.dodge);
      if (atk.dodge_scatters) {
        if (atk.dodge_explosion)
          send_to_room(g, char_get_in_room(s.vict), atk.dodge_explosion);
        dodge_ki(g, s.ch, s.vict, atk.dodge_kind, dodge_damtype(atk), s.skill, atk.skill);
        room_vnum here = char_get_in_room(s.ch);
        if (room_get_dmg(g, here) <= atk.dodge_dmg_cap)
          room_set_dmg(g, here, room_get_dmg(g, here) + atk.dodge_dmg);
        improve_skill(g, s.vict, SKILL_DODGE, 0);
      }
      pay_if(ki_outcome::dodged);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      return ki_outcome::dodged;
    }
    ki_act(s, atk.miss);
    if (atk.steers)
      steer();
    pay_if(ki_outcome::missed);
    hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
    return ki_outcome::missed;
  }

  if (atk.before_landing)
    atk.before_landing(s);
  for (int landed = 0; landed < s.landings; landed++) {
    land(s);
    if (getCurPL(g, s.vict) <= 0)
      break;
  }
  if (atk.after_hit)
    atk.after_hit(s);
  pay_if(ki_outcome::hit);
  if (atk.multihit)
    handle_multihit(g, s.ch, s.vict);
  return ki_outcome::hit;
}

ki_outcome shoot(ki_shot &s, int prob, int perc, int pry, int blk, int dge, bool pays)
{
  ki_outcome how = shoot_once(s, prob, perc, pry, blk, dge, pays);
  if (s.attack->finish && how != ki_outcome::absorbed)
    s.attack->finish(s, how);
  return how;
}

void at_victim(ki_shot &s)
{
  GameWorld *g         = s.g;
  const ki_attack &atk = *s.attack;
  int pry = 2, dge = 2, blk = 2;

  if (!can_kill(g, s.ch, s.vict, nullptr, atk.kill_mode))
    return;
  if (handle_defender(g, s.vict, s.ch))
    s.vict = char_get_defender(g, s.vict);
  improve_skill(g, s.ch, atk.skill, 0);
  int index = check_def(g, s.vict); /* the victim's parry, block and dodge */
  int prob  = roll_accuracy(g, s.ch, s.skill, atk.ki_accuracy);
  if (perfection(s) == 2)
    prob += 5;
  int perc = chance_to_hit(g, s.ch);
  index -= handle_speed(g, s.ch, s.vict);
  int avo = index / 4;
  handle_defense(g, s.vict, &pry, &blk, &dge);
  prob -= avo;
  tech_handle_posmodifier(s.vict, &pry, &blk, &dge, &prob);
  if (atk.steady)
    atk.steady(s, prob);

  if (!tech_handle_zanzoken(g, s.ch, s.vict, atk.name)) {
    if (atk.homing) {
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 200);
      dodge_ki(g, s.ch, s.vict, 1, dodge_damtype(atk), s.skill, atk.skill);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      pay(s, ki_outcome::zanzoken);
    } else {
      pay(s, ki_outcome::zanzoken);
      if (atk.guided) {
        dodge_ki(g, s.ch, s.vict, 2, dodge_damtype(atk), s.skill, atk.skill);
        hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      }
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 200);
    }
    if (atk.finish)
      atk.finish(s, ki_outcome::zanzoken);
    return;
  }

  for (int shot = 1; shot <= atk.shots; shot++) {
    bool last = shot == atk.shots;
    if (shot > 1)
      prob -= prob / 5;
    else if (atk.shots > 1 && (char_aff_flagged(s.vict, AFF_SPIRIT) || getCurPL(g, s.vict) <= 0) &&
             !char_is_npc(s.vict))
      return;
    if (shoot(s, prob, perc, pry, blk, dge, last) == ki_outcome::hit && getCurPL(g, s.vict) <= 0) {
      if (!last)
        pay(s, ki_outcome::hit);
      return;
    }
  }
}

void at_object(ki_shot &s)
{
  GameWorld *g = s.g;

  /* can_kill() refuses a broken object, so it is never broken here */
  if (!can_kill(g, s.ch, nullptr, s.obj, 1))
    return;
  if (s.attack->on_object && s.attack->on_object(s))
    return;
  s.dmg = damtype(g, s.ch, s.attack->obj_damtype ? s.attack->obj_damtype : s.attack->damtype, s.skill, s.attperc) / 10;
  act(g, s.attack->obj_to_char, s.attack->obj_hide, s.ch, s.obj, nullptr, TO_CHAR);
  act(g, s.attack->obj_to_room, s.attack->obj_hide, s.ch, s.obj, nullptr, TO_ROOM);
  hurt(g, 0, 0, s.ch, nullptr, s.obj, s.dmg, 0);
  pcost(g, s.ch, s.attperc, 0);
  if (s.attack->after_paying)
    s.attack->after_paying(s, ki_outcome::object);
}

} // namespace

void ki_act(const ki_shot &s, const act_msgs &m)
{
  act_triple(s.g, m, s.ch, nullptr, s.vict);
}

void perform_ki_attack(GameWorld *g, struct char_data *ch, std::string_view argument, const ki_attack &attack)
{
  ki_shot s{.g = g, .ch = ch, .attack = &attack, .attperc = attack.attperc, .minimum = attack.minimum};
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (attack.not_while_singing && playing_song(g, ch))
    return;
  if (!can_grav(g, ch) || !check_skill(g, ch, attack.skill))
    return;
  if (attack.sword ? !tech_handle_arms(g, ch) : (attack.needs_arms && !limb_ok(g, ch, 0)))
    return;
  if (attack.needs_free_hand && char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(g, ch, "Your hands are full!\r\n");
    return;
  }
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (attack.sword && attack.sword_checked_early && !holds_sword(g, ch))
    return;
  if (attack.targets_first && !tech_handle_targeting(g, ch, arg, &s.vict, &s.obj))
    return;
  if (perfection(s) == 1)
    s.attperc += 0.05;
  else if (perfection(s) == 3) {
    s.minimum -= 0.05;
    if (s.minimum <= 0.0)
      s.minimum = 0.01;
  }
  if (!tech_handle_charge(g, ch, arg2, s.minimum, &s.attperc))
    return;
  if (getMaxKI(g, ch) * s.attperc > char_get_charge(ch))
    s.attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  if (!check_points(g, ch, getMaxKI(g, ch) * s.minimum, attack.stamina_share ? getEffMaxPL(g, ch) / attack.stamina_share : 0))
    return;
  if (attack.sword && !attack.sword_checked_early && !holds_sword(g, ch))
    return;
  s.skill = init_skill(g, ch, attack.skill);
  if (!attack.targets_first && !tech_handle_targeting(g, ch, arg, &s.vict, &s.obj))
    return;
  handle_cooldown(g, ch, attack.cooldown_for ? attack.cooldown_for(s) : attack.cooldown);
  if (s.vict)
    at_victim(s);
  else if (s.obj)
    at_object(s);
}
