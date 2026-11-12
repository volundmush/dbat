/* area.cpp: what every attack on everyone in the room does; see area.hpp */
#include "area.hpp"

#include <string>

#include "dbat/args.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/combat.hpp"
#include "dbat/fight.hpp"
#include "dbat/iterate.hpp"
#include "dbat/random.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/techniques.hpp"

bool area_fair_game(GameWorld *, struct char_data *, struct char_data *vict)
{
  if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict))
    return false;
  if (char_get_level(vict) <= 8 && !char_is_npc(vict))
    return false;
  return !char_mob_flagged(vict, MOB_NOKILL);
}

bool area_fair_game_ungrouped(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  if (char_aff_flagged(vict, AFF_GROUP) && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict))
    return false;
  return area_fair_game(g, ch, vict);
}

bool area_fair_game_no_group(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  if (char_aff_flagged(vict, AFF_GROUP) && char_get_master(g, vict) == char_get_master(g, ch))
    return false;
  return area_fair_game_ungrouped(g, ch, vict);
}

bool area_vanishes(GameWorld *g, struct char_data *ch, struct char_data *vict, const act_msgs &msgs)
{
  if (!((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) ||
      getCurST(g, vict) < 1 || char_get_position(vict) == POS_SLEEPING)
    return false;
  act_triple(g, msgs, ch, nullptr, vict);
  char_aff_set(vict, AFF_ZANZOKEN, false);
  pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
  return true;
}

namespace {

/* The user's side of it: to_char, then to_room */
void act_pair(GameWorld *g, const act_msgs &msgs, struct char_data *ch)
{
  act(g, msgs.to_char, true, ch, nullptr, nullptr, TO_CHAR);
  act(g, msgs.to_room, true, ch, nullptr, nullptr, TO_ROOM);
}

void pay(const area_shot &s)
{
  if (s.attack->perfections && char_get_skills_perf(s.ch, s.attack->skill) == 3 && s.attperc > s.minimum)
    pcost(s.g, s.ch, s.attperc - 0.05, 0);
  else
    pcost(s.g, s.ch, s.attperc, 0);
}

/* Each it catches vanishes, escapes or is hit */
void catch_everyone(area_shot &s)
{
  GameWorld *g            = s.g;
  struct char_data *ch    = s.ch;
  const area_attack &atk  = *s.attack;
  int caught              = 0;

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch || !atk.caught(g, ch, vict))
      continue;
    if (atk.max_hits && caught >= atk.max_hits)
      continue;
    caught++;
    int dge = handle_dodge(g, vict);
    if (area_vanishes(g, ch, vict, atk.vanish)) {
      if (atk.vanish_hurts)
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      continue;
    }
    if (dge + rand_number(g, atk.dodge_low, 5) > s.skill) {
      act_triple(g, atk.escape, ch, nullptr, vict);
      if (atk.dodge_improves_first)
        improve_skill(g, vict, SKILL_DODGE, 0);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
      if (!atk.dodge_improves_first)
        improve_skill(g, vict, SKILL_DODGE, 0);
      continue;
    }
    act_triple(g, atk.hit, ch, nullptr, vict);
    if (atk.knockdown != area_knockdown::never && !char_aff_flagged(vict, AFF_FLYING) &&
        char_get_position(vict) == POS_STANDING &&
        (atk.knockdown == area_knockdown::always || rand_number(g, 1, 4) == 4))
      handle_knockdown(g, vict);
    hurt(g, 0, 0, ch, vict, nullptr, s.dmg, 1);
  }
}

} // namespace

void perform_area_attack(GameWorld *g, struct char_data *ch, std::string_view argument, const area_attack &attack)
{
  area_shot s{.g = g, .ch = ch, .attack = &attack, .attperc = attack.attperc, .minimum = attack.minimum};
  std::string_view rest = argument;
  std::string arg       = take_arg(rest); /* how much of the charge to use */
  int perfection        = attack.perfections ? char_get_skills_perf(ch, attack.skill) : 0;

  if (!can_grav(g, ch) || !check_skill(g, ch, attack.skill) || !limb_ok(g, ch, 0))
    return;
  if (perfection == 1)
    s.attperc += 0.05;
  else if (perfection == 3) {
    s.minimum -= 0.05;
    if (s.minimum <= 0.0)
      s.minimum = 0.01;
  }
  if (!tech_handle_charge(g, ch, arg, s.minimum, &s.attperc))
    return;
  if (getMaxKI(g, ch) * s.attperc > char_get_charge(ch))
    s.attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  if (!check_points(g, ch, getMaxKI(g, ch) * s.minimum, 0))
    return;
  if (!attack.in_peace && room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  s.skill = init_skill(g, ch, attack.skill);
  if (perfection == 2)
    s.skill += 5;

  for (char_data *vict : room_people(g, char_get_in_room(ch)))
    if (vict != ch && attack.worth(g, ch, vict))
      s.count++;
  if (s.count <= 0) {
    send_to_char(g, ch, "There is no one worth targeting around.\r\n");
    return;
  }

  if (attack.cooldown_first && attack.cooldown_first_early)
    handle_cooldown(g, ch, attack.cooldown_first);
  int perc = chance_to_hit(g, ch);
  if (attack.before_roll)
    attack.before_roll(s);
  if (attack.cooldown_first && !attack.cooldown_first_early)
    handle_cooldown(g, ch, attack.cooldown_first);
  if (s.skill < perc) {
    act_pair(g, attack.fail, ch);
    pay(s);
    improve_skill(g, ch, attack.skill, 0);
    return;
  }

  act_pair(g, attack.launch, ch);
  s.dmg = damtype(g, ch, attack.damtype, s.skill, s.attperc);
  if (attack.damage)
    attack.damage(s);
  catch_everyone(s);
  if (attack.after)
    attack.after(s);
  pay(s);
  improve_skill(g, ch, attack.skill, 0);
  if (attack.cooldown_last)
    handle_cooldown(g, ch, attack.cooldown_last);
}
