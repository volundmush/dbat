/* ki_attack.cpp: what every ki attack does; see ki_attack.hpp */
#include "ki_attack.hpp"

#include <string>

#include "dbat/args.hpp"
#include "dbat/consts/affflags.hpp"
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

void act_at(const ki_shot &s, const act_msgs &m, bool hide)
{
  act(s.g, m.to_char, hide, s.ch, nullptr, s.vict, TO_CHAR);
  act(s.g, m.to_vict, hide, s.ch, nullptr, s.vict, TO_VICT);
  act(s.g, m.to_room, hide, s.ch, nullptr, s.vict, TO_NOTVICT);
}

int perfection(const ki_shot &s)
{
  return s.attack->perfection ? char_get_skills_perf(s.ch, s.attack->skill) : 0;
}

/* The attacker pays for the attack: perfection 3 takes 5% off, if that leaves at least the minimum. */
void pay(ki_shot &s, ki_outcome how)
{
  if (perfection(s) == 3 && s.attperc > s.minimum)
    pcost(s.g, s.ch, s.attperc - 0.05, 0);
  else
    pcost(s.g, s.ch, s.attperc, 0);
  if (s.attack->after_paying)
    s.attack->after_paying(s, how);
}

/* The victim couldn't avoid it: where it hit, and what that did. */
void land(ki_shot &s)
{
  GameWorld *g = s.g;

  s.dmg = damtype(g, s.ch, s.attack->damtype, s.skill, s.attperc);
  if (s.attack->bonus_damage)
    s.attack->bonus_damage(s);
  int hitspot = roll_hitloc(g, s.ch, s.vict, s.skill);
  if (s.attack->aim)
    hitspot = s.attack->aim(s, hitspot);
  if (hitspot < 1 || hitspot > 5)
    return;
  act_at(s, s.attack->hits[hitspot - 1], s.attack->hits_hide);
  switch (hitspot) {
    case 1:
    case 3:
      if (char_get_bonuses(s.ch, BONUS_SOFT))
        s.dmg *= calc_critical(g, s.ch, 2);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 4); /* 1 arms, 2 legs, 3 head, 4 body */
      break;
    case 2:
      if (!s.attack->powerhit_crit)
        s.dmg *= calc_critical(g, s.ch, 0);
      else
        s.dmg *= char_get_bonuses(s.ch, BONUS_POWERHIT) ? 5 : 3;
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 3);
      break;
    case 4:
      s.dmg *= calc_critical(g, s.ch, 1);
      hurt(g, 0, s.attack->limb_hurt, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 1);
      break;
    case 5:
      s.dmg *= calc_critical(g, s.ch, 1);
      hurt(g, 1, s.attack->limb_hurt, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 2);
      break;
  }
}

/* One shot at the victim, after zanzoken: avoided, or where it landed. pays: whether the attacker pays for the attack
 * now, as they do on the last shot. Returns how it ended. */
ki_outcome shoot(ki_shot &s, int prob, int perc, int pry, int blk, int dge, bool pays)
{
  GameWorld *g         = s.g;
  const ki_attack &atk = *s.attack;
  auto pay_if          = [&](ki_outcome how) {
    if (pays)
      pay(s, how);
  };

  if (prob < perc - 20) {
    if (getCurST(g, s.vict) <= 0) {
      act_at(s, atk.miss_spent, atk.hide);
      pay_if(ki_outcome::missed);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      return ki_outcome::missed;
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
      act_at(s, atk.parry, atk.hide);
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 200);
      parry_ki(g, s.attperc, s.ch, s.vict, atk.name, prob, perc, s.skill, atk.damtype);
      pay_if(ki_outcome::blocked);
      room_vnum here = char_get_in_room(s.ch);
      if (room_get_dmg(g, here) <= 95)
        room_set_dmg(g, here, room_get_dmg(g, here) + 5);
      return ki_outcome::blocked;
    }
    if (blk > axion_dice(g, 10)) {
      act_at(s, atk.block, atk.hide);
      if (atk.block_improves)
        improve_skill(g, s.vict, SKILL_BLOCK, 0);
      pay_if(ki_outcome::blocked);
      pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 500);
      s.dmg = damtype(g, s.ch, atk.damtype, s.skill, s.attperc) / 4;
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      return ki_outcome::blocked;
    }
    if (dge > axion_dice(g, 10)) {
      act_at(s, atk.dodge, atk.hide);
      if (atk.dodge_explosion)
        send_to_room(g, char_get_in_room(s.vict), "@wA bright explosion erupts from the impact!\r\n");
      dodge_ki(g, s.ch, s.vict, 0, atk.damtype, s.skill, atk.skill); /* 0: it doesn't home in */
      room_vnum here = char_get_in_room(s.ch);
      if (room_get_dmg(g, here) <= atk.dodge_dmg_cap)
        room_set_dmg(g, here, room_get_dmg(g, here) + atk.dodge_dmg);
      improve_skill(g, s.vict, SKILL_DODGE, 0);
      pay_if(ki_outcome::dodged);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
      return ki_outcome::dodged;
    }
    act_at(s, atk.miss, atk.hide);
    pay_if(ki_outcome::missed);
    hurt(g, 0, 0, s.ch, s.vict, nullptr, 0, 1);
    return ki_outcome::missed;
  }

  land(s);
  if (atk.after_hit)
    atk.after_hit(s);
  pay_if(ki_outcome::hit);
  if (atk.multihit)
    handle_multihit(g, s.ch, s.vict);
  return ki_outcome::hit;
}

void at_victim(ki_shot &s)
{
  GameWorld *g         = s.g;
  const ki_attack &atk = *s.attack;
  int pry = 2, dge = 2, blk = 2;

  if (!can_kill(g, s.ch, s.vict, nullptr, 1))
    return;
  if (handle_defender(g, s.vict, s.ch))
    s.vict = char_get_defender(g, s.vict);
  improve_skill(g, s.ch, atk.skill, 0);
  int index = check_def(g, s.vict); /* the victim's parry, block and dodge */
  int prob  = roll_accuracy(g, s.ch, s.skill, true);
  if (perfection(s) == 2)
    prob += 5;
  int perc = chance_to_hit(g, s.ch);
  index -= handle_speed(g, s.ch, s.vict);
  int avo = index / 4;
  handle_defense(g, s.vict, &pry, &blk, &dge);
  prob -= avo;
  tech_handle_posmodifier(s.vict, &pry, &blk, &dge, &prob);

  if (!tech_handle_zanzoken(g, s.ch, s.vict, atk.name)) {
    pay(s, ki_outcome::zanzoken);
    pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 200);
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
  s.dmg = damtype(g, s.ch, s.attack->damtype, s.skill, s.attperc) / 10;
  act(g, s.attack->obj_to_char, s.attack->hide, s.ch, s.obj, nullptr, TO_CHAR);
  act(g, s.attack->obj_to_room, s.attack->hide, s.ch, s.obj, nullptr, TO_ROOM);
  hurt(g, 0, 0, s.ch, nullptr, s.obj, s.dmg, 0);
  pcost(g, s.ch, s.attperc, 0);
  if (s.attack->after_paying)
    s.attack->after_paying(s, ki_outcome::object);
}

} // namespace

void perform_ki_attack(GameWorld *g, struct char_data *ch, std::string_view argument, const ki_attack &attack)
{
  ki_shot s{.g = g, .ch = ch, .attack = &attack, .attperc = attack.attperc, .minimum = attack.minimum};
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (!can_grav(g, ch) || !check_skill(g, ch, attack.skill) || !limb_ok(g, ch, 0))
    return;
  if (attack.needs_free_hand && char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(g, ch, "Your hands are full!\r\n");
    return;
  }
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
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
  if (!check_points(g, ch, getMaxKI(g, ch) * s.minimum, 0))
    return;
  s.skill = init_skill(g, ch, attack.skill);
  if (!tech_handle_targeting(g, ch, arg, &s.vict, &s.obj))
    return;
  handle_cooldown(g, ch, attack.cooldown);
  if (s.vict)
    at_victim(s);
  else if (s.obj)
    at_object(s);
}
