/* slashes.c: ki disk and slash attacks
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/args.hpp"
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
#include "dbat/act.item.hpp"
#include "dbat/game_world.hpp"
#include "ki_attack.hpp"

namespace {

/* The victim is dead: what killing them earns, and the looting the killer asked for */
void slay(ki_shot &s)
{
  GameWorld *g           = s.g;
  struct char_data *ch   = s.ch;
  struct char_data *vict = s.vict;

  die(g, vict, ch);
  if (char_aff_flagged(ch, AFF_GROUP)) {
    group_gain(g, ch, vict);
  } else {
    solo_gain(g, ch, vict);
  }
  if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOGOLD)) {
    do_get(g, ch, "all.zenni corpse", 0, 0);
  }
  if (!char_is_npc(ch) && (ch != vict) && char_prf_flagged(g, ch, PRF_AUTOLOOT)) {
    do_get(g, ch, "all corpse", 0, 0);
  }
}

/* A blade's work on the head or the body: what it says when it kills, and when a majin or bio-android grows back */
struct cut_msgs {
  act_msgs kills;
  act_msgs regenerates;
  bool beheads{}; /* the head comes off: DTYPE_HEAD */
};

/* A cut that does a fifth of the victim's power level kills them outright, unless a majin or bio-android regenerates;
 * otherwise it only hurts. Returns whether the victim was only hurt. */
bool cleave(ki_shot &s, const cut_msgs &m)
{
  GameWorld *g           = s.g;
  struct char_data *vict = s.vict;

  if (s.dmg > getEffMaxPL(g, vict) / 5 && (!char_is_majin(vict) && !char_is_bio(vict))) {
    ki_act(s, m.kills);
    if (m.beheads) {
      char_set_death_type(vict, DTYPE_HEAD);
      remove_limb(g, vict, 0);
    }
    slay(s);
  } else if (s.dmg > getEffMaxPL(g, vict) / 5 && (char_is_majin(vict) || char_is_bio(vict))) {
    if (char_get_skill(vict, SKILL_REGENERATE) > rand_number(g, 1, 101) && (getCurKI(g, vict)) >= getMaxKI(g, vict) / 40) {
      ki_act(s, m.regenerates);
      decCurKI(g, vict, getMaxKI(g, vict) / 40, 0);
      hurt(g, 0, 0, s.ch, vict, nullptr, s.dmg, 1);
    } else {
      ki_act(s, m.kills);
      if (m.beheads)
        char_set_death_type(vict, DTYPE_HEAD); /* quirk: the head stays on this time */
      slay(s);
    }
  } else {
    hurt(g, 0, 0, s.ch, vict, nullptr, s.dmg, 1);
    return true;
  }
  return false;
}

/* Sometimes a cut takes off a player's left or right arm (limbs 2 and 1) or leg (4 and 3) */
struct sever_msgs {
  act_msgs left;
  act_msgs right;
  bool told_to_vict{}; /* quirk: the third message goes to the victim again rather than to the room */
};

void sever(ki_shot &s, const sever_msgs &m, int left_limb, int right_limb)
{
  GameWorld *g           = s.g;
  struct char_data *vict = s.vict;
  auto say               = [&](const act_msgs &a) {
    act(g, a.to_char, true, s.ch, nullptr, vict, TO_CHAR);
    act(g, a.to_vict, true, s.ch, nullptr, vict, TO_VICT);
    act(g, a.to_room, true, s.ch, nullptr, vict, m.told_to_vict ? TO_VICT : TO_NOTVICT);
  };

  if (rand_number(g, 1, 100) >= 70 && !char_is_npc(vict) && !char_aff_flagged(vict, AFF_SANCTUARY)) {
    if (char_get_limb_condition(vict, left_limb - 1) > 0 && !is_sparring(s.ch) && rand_number(g, 1, 2) == 2) {
      say(m.left);
      char_set_limb_condition(vict, left_limb - 1, 0);
      remove_limb(g, vict, left_limb);
    } else if (char_get_limb_condition(vict, right_limb - 1) > 0 && !is_sparring(s.ch)) {
      say(m.right);
      char_set_limb_condition(vict, right_limb - 1, 0);
      remove_limb(g, vict, right_limb);
    }
  }
}

void cut_tail(ki_shot &s, const act_msgs &m)
{
  if (char_has_tail(s.vict)) {
    ki_act(s, m);
    char_lose_tail(s.g, s.vict);
  }
}

/* A twin slash bites harder with a finer sword, the better its user knows it */
void twin_slash_weapon(ki_shot &s)
{
  struct obj_data *wobj = char_get_equipment(s.ch, WEAR_WIELD1);
  int wlvl              = 0;
  if (obj_extra_flagged(wobj, ITEM_WEAPLVL1)) {
    wlvl = 1;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL2)) {
    wlvl = 2;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL3)) {
    wlvl = 3;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL4)) {
    wlvl = 4;
  } else if (obj_extra_flagged(wobj, ITEM_WEAPLVL5)) {
    wlvl = 5;
  }
  if (char_get_skill(s.ch, SKILL_TSLASH) >= 100) {
    s.dmg += (s.dmg * 0.05) * wlvl;
  } else if (char_get_skill(s.ch, SKILL_TSLASH) >= 60) {
    s.dmg += (s.dmg * 0.02) * wlvl;
  } else if (char_get_skill(s.ch, SKILL_TSLASH) >= 40) {
    s.dmg += (s.dmg * 0.01) * wlvl;
  }
}

const act_msgs TWIN_SLASH_TAIL = {"@rYou cut off $S tail!@n", "@rYour tail is cut off!@n", "@R$N@r's tail is cut off!@n", true};
const cut_msgs TWIN_SLASH_HEAD = {
    {"@R$N@r has $S head cut off by the attack!@n",                                "@rYou have your head cut off by the attack!@n",                               "@R$N@r has $S head cut off by the attack!@n",                                true},
    {"@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", "@rYou have your head cut off by the attack but regenerate a moment later!@n", "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", true},
    true
};
const sever_msgs TWIN_SLASH_ARMS = {
    {"@RYour attack severs $N's left arm!@n",  "@R$n's attack severs your left arm!@n",  "@R$N's left arm is severered in the attack!@n",  true},
    {"@RYour attack severs $N's right arm!@n", "@R$n's attack severs your right arm!@n", "@R$N's right arm is severered in the attack!@n", true},
    true
};
const sever_msgs TWIN_SLASH_LEGS = {
    {"@RYour attack severs $N's left leg!@n",  "@R$n's attack severs your left leg!@n",  "@R$N's left leg is severered in the attack!@n",  true},
    {"@RYour attack severs $N's right leg!@n", "@R$n's attack severs your right leg!@n", "@R$N's right leg is severered in the attack!@n", true},
    true
};

bool twin_slash_strike(ki_shot &s, int where)
{
  GameWorld *g = s.g;

  switch (where) {
    case 1:
      if (char_get_bonuses(s.ch, BONUS_SOFT))
        s.dmg *= calc_critical(g, s.ch, 2);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 4);
      cut_tail(s, TWIN_SLASH_TAIL);
      return true;
    case 2:
      s.dmg *= calc_critical(g, s.ch, 0);
      cleave(s, TWIN_SLASH_HEAD);
      s.dmg *= calc_critical(g, s.ch, 0); /* quirk: rolled again, for nothing */
      dam_eq_loc(g, s.vict, 3);
      return true;
    case 4:
      s.dmg *= calc_critical(g, s.ch, 1);
      sever(s, TWIN_SLASH_ARMS, 2, 1);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 1);
      return true;
    case 5:
      s.dmg *= calc_critical(g, s.ch, 1);
      sever(s, TWIN_SLASH_LEGS, 4, 3);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 2);
      return true;
  }
  return false;
}

/* Phoenix slash: a hot ruby feeds it, fireproofing resists it and the flammable suffer it, and it leaves ash */
void phoenix_slash_fire(ki_shot &s)
{
  if (check_ruby(s.g, s.ch) == 1) {
    s.dmg += s.dmg * 0.2;
  }
  if (char_get_bonuses(s.vict, BONUS_FIREPROOF)) {
    s.dmg -= s.dmg * 0.4;
  } else if (char_get_bonuses(s.vict, BONUS_FIREPRONE)) {
    s.dmg += s.dmg * 0.4;
  }
  char_aff_set(s.vict, AFF_ASHED, true);
}

void phoenix_slash_burn(ki_shot &s)
{
  GameWorld *g         = s.g;
  struct char_data *ch = s.ch, *vict = s.vict;

  if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(g, 1, 4) == 4 && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
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
}

void phoenix_slash_settle(ki_shot &s, ki_outcome how)
{
  if (how == ki_outcome::hit)
    char_aff_set(s.vict, AFF_ASHED, false);
}

/* It does three times the damage to object 79 */
bool phoenix_slash_at_object(ki_shot &s)
{
  if (obj_get_vnum(s.g, s.obj) != 79)
    return false;
  s.dmg = damtype(s.g, s.ch, s.attack->damtype, s.skill, s.attperc);
  s.dmg /= 10;
  s.dmg *= 3;
  act(s.g, s.attack->obj_to_char, s.attack->obj_hide, s.ch, s.obj, nullptr, TO_CHAR);
  act(s.g, s.attack->obj_to_room, s.attack->obj_hide, s.ch, s.obj, nullptr, TO_ROOM);
  hurt(s.g, 0, 0, s.ch, nullptr, s.obj, s.dmg, 0);
  pcost(s.g, s.ch, s.attperc, 0);
  return true;
}

void darkness_dragon_slash_bonus(ki_shot &s)
{
  if (char_get_skill(s.ch, SKILL_DDSLASH) >= 100) {
    s.dmg += s.dmg * 0.15;
  } else if (char_get_skill(s.ch, SKILL_DDSLASH) >= 60) {
    s.dmg += s.dmg * 0.10;
  } else if (char_get_skill(s.ch, SKILL_DDSLASH) >= 40) {
    s.dmg += s.dmg * 0.05;
  }
}

/* and can blind */
void darkness_dragon_slash_blind(ki_shot &s)
{
  if (rand_number(s.g, 1, 3) == 3 && !char_aff_flagged(s.vict, AFF_BLIND)) {
    act(s.g, "@mYou are struck blind temporarily!@n", true, s.vict, nullptr, nullptr, TO_CHAR);
    act(s.g, "@c$n@m is struck blind by the attack!@n", true, s.vict, nullptr, nullptr, TO_ROOM);
    int duration = 1;
    assign_affect(s.g, s.vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
  }
}

void kienzan_bonus(ki_shot &s)
{
  if (char_get_skill(s.ch, SKILL_KIENZAN) >= 100) {
    s.dmg += s.dmg * 0.25;
  } else if (char_get_skill(s.ch, SKILL_KIENZAN) >= 60) {
    s.dmg += s.dmg * 0.15;
  } else if (char_get_skill(s.ch, SKILL_KIENZAN) >= 40) {
    s.dmg += s.dmg * 0.05;
  }
}

const cut_msgs KIENZAN_BODY = {
    {"@R$N@r is cut in half by the attack!@n",                                "@rYou are cut in half by the attack!@n",                               "@R$N@r is cut in half by the attack!@n",                                true},
    {"@R$N@r is cut in half by the attack but regenerates a moment later!@n", "@rYou are cut in half by the attack but regenerate a moment later!@n", "@R$N@r is cut in half by the attack but regenerates a moment later!@n", true},
    false
};
const act_msgs KIENZAN_TAIL = {"@rYou cut off $S tail!@n", "@rYour tail is cut off!@n", "@R$N@r's tail is cut off!@n", true};
const cut_msgs KIENZAN_HEAD = {
    {"@R$N@r has $S head cut off by the attack!@n",                                "@rYou have your head cut off by the attack!@n",                               "@R$N@r has $S head cut off by the attack!@n",                                true},
    {"@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", "@rYou have your head cut off by the attack but regenerate a moment later!@n", "@R$N@r has $S head cut off by the attack but regenerates a moment later!@n", true},
    true
};
const sever_msgs KIENZAN_ARMS = {
    {"@RYour attack severes $N's left arm!@n",  "@R$n's attack severes your left arm!@n",  "@R$N's left arm is severed in the attack!@n",  true},
    {"@RYour attack severes $N's right arm!@n", "@R$n's attack severes your right arm!@n", "@R$N's right arm is severed in the attack!@n", true},
    false
};
const sever_msgs KIENZAN_LEGS = {
    {"@RYour attack severes $N's left leg!@n",  "@R$n's attack severes your left leg!@n",  "@R$N's left leg is severed in the attack!@n",  true},
    {"@RYour attack severes $N's right leg!@n", "@R$n's attack severes your right leg!@n", "@R$N's right leg is severed in the attack!@n", true},
    false
};

/* The kienzan's disc cuts through whatever it hits */
bool kienzan_strike(ki_shot &s, int where)
{
  GameWorld *g = s.g;

  switch (where) {
    case 1:
      if (cleave(s, KIENZAN_BODY))
        cut_tail(s, KIENZAN_TAIL);
      dam_eq_loc(g, s.vict, 4);
      return true;
    case 2:
      s.dmg *= calc_critical(g, s.ch, 0);
      cleave(s, KIENZAN_HEAD);
      dam_eq_loc(g, s.vict, 3);
      return true;
    case 4:
      s.dmg *= calc_critical(g, s.ch, 1);
      sever(s, KIENZAN_ARMS, 2, 1);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 1);
      return true;
    case 5:
      s.dmg *= calc_critical(g, s.ch, 1);
      sever(s, KIENZAN_LEGS, 4, 3);
      hurt(g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
      dam_eq_loc(g, s.vict, 2);
      return true;
  }
  return false;
}

const ki_attack TWIN_SLASH = {
    .skill               = SKILL_TSLASH,
    .name                = "Twin Slash",
    .damtype             = 19,
    .attperc             = .125,
    .minimum             = .1,
    .cooldown            = 6,
    .not_while_singing   = true,
    .sword               = true,
    .sword_checked_early = true,
    .perfection          = true,
    .block               = {"@C$N@W moves quickly and blocks your twin slash!@n", "@WYou move quickly and block @C$n's@W twin slash!@n", "@C$N@W moves quickly and blocks @c$n's@W twin slash!@n"},
    .dodge               = {"@C$N@W manages to dodge your twin slash, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W twin slash, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W twin slash, letting it slam into the surroundings!@n"},
    .miss                = {"@WYou can't believe it but your twin slash misses, flying through the air harmlessly!@n", "@C$n@W fires a twin slash at you, but misses!@n ", "@c$n@W fires a twin slash at @C$N@W, but somehow misses!@n "},
    .miss_spent          = {"@WYou can't believe it but your twin slash misses, flying through the air harmlessly!@n", "@C$n@W fires a twin slash at you, but misses!@n", "@c$n@W fires a twin slash at @C$N@W, but somehow misses!@n"},
    .hits                = {{"@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's body as you fly past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR body as $e flies past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's body as $e flies past, leaving a green after-image behind!@n", true}, {"@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's face as you fly past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR face as $e flies past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's face as $e flies past, leaving a green after-image behind!@n", true}, {"@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's gut as you fly past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR gut as $e flies past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's gut as $e flies past, leaving a green after-image behind!@n", true}, {"@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's arm as you fly past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR arm as $e flies past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's arm as $e flies past, leaving a green after-image behind!@n", true}, {"@WYou channel your charged ki into the blade of your sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as you draw it up to attack. Two blindingly quick slashes connect with @c$N@W's leg as you fly past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with YOUR leg as $e flies past, leaving a green after-image behind!@n", "@C$n @Wchannels $s charged ki into the blade of $s sword. @rF@Rl@Ya@rm@Ri@Yn@rg @gg@Gre@wen@W energy burns around the blade as $e draws it up to attack. Two blindingly quick slashes connect with @c$N@W's leg as $e flies past, leaving a green after-image behind!@n", true}},
    .obj_to_char         = "@WYou fire a twin slash at $p@W!@n",
    .obj_to_room         = "@C$n@W fires a twin slash at $p@W!@n",
    .bonus_damage        = twin_slash_weapon,
    .strike              = twin_slash_strike,
};

const ki_attack PHOENIX_SLASH = {
    .skill             = SKILL_PSLASH,
    .name              = "Phoenix Slash",
    .damtype           = 37,
    .attperc           = .3,
    .minimum           = .15,
    .cooldown          = 8,
    .kill_mode         = 3,
    .not_while_singing = true,
    .sword             = true,
    .limb_hurt         = 170,
    .dodge_scatters    = false,
    .block             = {"@C$N@W moves quickly and blocks your Phoenix Slash!@n", "@WYou move quickly and block @C$n's@W Phoenix Slash!@n", "@C$N@W moves quickly and blocks @c$n's@W Phoenix Slash!@n"},
    .dodge             = {"@C$N@W manages to dodge your Phoenix Slash, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Phoenix Slash, letting it letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Phoenix Slash, letting it fly harmlessly by!@n"},
    .miss              = {"@WYou can't believe it but your Phoenix Slash misses, flying through the air harmlessly!@n", "@C$n@W fires a Phoenix Slash at you, but misses!@n ", "@c$n@W fires a Phoenix Slash at @C$N@W, but somehow misses!@n "},
    .miss_spent        = {"@WYou can't believe it but your Phoenix Slash misses, flying through the air harmlessly!@n", "@C$n@W fires a Phoenix Slash at you, but misses!@n", "@c$n@W fires a Phoenix Slash at @C$N@W, but somehow misses!@n"},
    .hits              = {{"@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S body in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR body in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's body in flames a moment later!@n", true}, {"@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S head in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR head in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's head in flames a moment later!@n", true}, {"@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S gut in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR gut in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's gut in flames a moment later!@n", true}, {"@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S arm in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR arm in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's arm in flames a moment later!@n", true}, {"@WYou pour your charged ki into your sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surround the entire sword in the same instant that you pull the blade back and extend it behind your body. Suddenly you swing the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs $S leg in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward YOU, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards YOU! The Phoenix Slash engulfs YOUR leg in flames a moment later!@n", "@C$n@W pours $s charged ki into $s sword's blade. @YF@ri@Re@Yr@ry @Rf@Yl@ra@Rm@Ye@rs@W surrounds the entire sword in the same instant that $e pulls the blade back and extends it behind $s body. Suddenly $e swings the blade forward toward @c$N@W, unleashing a large wave of flames! The flames take the shape of a large phoenix soaring towards @c$N@W! The Phoenix Slash engulfs @c$N@W's leg in flames a moment later!@n", true}},
    .obj_to_char       = "@WYou fire a Phoenix Slash at $p@W!@n",
    .obj_to_room       = "@C$n@W fires a Phoenix Slash at $p@W!@n",
    .bonus_damage      = phoenix_slash_fire,
    .after_hit         = phoenix_slash_burn,
    .after_paying      = phoenix_slash_settle,
    .on_object         = phoenix_slash_at_object,
};

const ki_attack DARKNESS_DRAGON_SLASH = {
    .skill             = SKILL_DDSLASH,
    .name              = "Darkness Dragon Slash",
    .damtype           = 30,
    .attperc           = .2,
    .minimum           = .10,
    .cooldown          = 7,
    .not_while_singing = true,
    .sword             = true,
    .limb_hurt         = 180,
    .dodge_scatters    = false,
    .block             = {"@C$N@W moves quickly and blocks your Darkness Dragon Slash!@n", "@WYou move quickly and block @C$n's@W Darkness Dragon Slash!@n", "@C$N@W moves quickly and blocks @c$n's@W Darkness Dragon Slash!@n"},
    .dodge             = {"@C$N@W manages to dodge your Darkness Dragon Slash, letting it fly harmlessly by!@n", "@WYou dodge @C$n's@W Darkness Dragon Slash, letting it letting it fly harmlessly by!@n", "@C$N@W manages to dodge @c$n's@W Darkness Dragon Slash, letting it fly harmlessly by!@n"},
    .miss              = {"@WYou can't believe it but your Darkness Dragon Slash misses, flying through the air harmlessly!@n", "@C$n@W fires a Darkness Dragon Slash at you, but misses!@n ", "@c$n@W fires a Darkness Dragon Slash at @C$N@W, but somehow misses!@n "},
    .miss_spent        = {"@WYou can't believe it but your Darkness Dragon Slash misses, flying through the air harmlessly!@n", "@C$n@W fires a Darkness Dragon Slash at you, but misses!@n", "@c$n@W fires a Darkness Dragon Slash at @C$N@W, but somehow misses!@n"},
    .hits              = {{"@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's body!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR body!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's body!@n", true}, {"@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's head!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR head!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's head!@n", true}, {"@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's gut!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR gut!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's gut!@n", true}, {"@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's arm!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR arm!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's arm!@n", true}, {"@WYou channel your charged ki into the blade of your sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging your sword at @c$N@W you unleash a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's leg!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at YOU $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into YOUR leg!@n", "@C$n@W channels $s charged ki into the blade of $s sword. The energy takes the form of @md@Ma@Wr@wk@W @mf@Ml@Wa@wmes@W burning along the very blade's edge. Swinging $s sword at @c$N@W $e unleashes a serpentine dragon of @md@Ma@Wr@wk @mf@Ml@Wa@wmes@W! The fiery dragon slams into @c$N@W's leg!@n", true}},
    .obj_to_char       = "@WYou fire a Darkness Dragon Slash at $p@W!@n",
    .obj_to_room       = "@C$n@W fires a Darkness Dragon Slash at $p@W!@n",
    .bonus_damage      = darkness_dragon_slash_bonus,
    .after_hit         = darkness_dragon_slash_blind,
};

const ki_attack KIENZAN = {
    .skill           = SKILL_KIENZAN,
    .name            = "Kienzan",
    .damtype         = 25,
    .attperc         = .125,
    .minimum         = .05,
    .cooldown        = 7,
    .blockable       = false,
    .dodge_kind      = 2,
    .steers          = 2,
    .dodge_explosion = "@wThe kienzan expands, cutting through everything in its path!\r\n",
    .dodge           = {"@C$N@W manages to dodge your kienzan, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W kienzan, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W kienzan, letting it slam into the surroundings!@n"},
    .miss            = {"@WYou can't believe it but your kienzan misses, flying through the air harmlessly!@n", "@C$n@W fires a kienzan at you, but misses!@n ", "@c$n@W fires a kienzan at @C$N@W, but somehow misses!@n "},
    .miss_spent      = {"@WYou can't believe it but your kienzan misses, flying through the air harmlessly!@n", "@C$n@W fires a kienzan at you, but misses!@n", "@c$n@W fires a kienzan at @C$N@W, but somehow misses!@n"},
    .hits            = {{"@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's body!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR body!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's body!@n", true}, {"@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's neck!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR neck!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's neck!@n", true}, {"@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's gut!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR gut!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's gut!@n", true}, {"@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's arm!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR arm!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's arm!@n", true}, {"@WYou raise your hand above your head and pool your charged ki above your flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete you shout '@yK@Yi@we@yn@Yz@wa@yn@W!' and throw it! You watch as it slices into @c$N@W's leg!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into YOUR leg!@n", "@C$n@W raises $s hand above $s head and pools $s charged ki above $s flattened palm. Slowly a golden spinning disk of energy grows from the ki. With the attack complete $e shouts '@yK@Yi@we@yn@Yz@wa@yn@W!' and throws it! @C$n@W watches as it slices into @c$N@W's leg!@n", true}},
    .obj_to_char     = "@WYou fire a kienzan at $p@W!@n",
    .obj_to_room     = "@C$n@W fires a kienzan at $p@W!@n",
    .bonus_damage    = kienzan_bonus,
    .strike          = kienzan_strike,
};

} // namespace

ACMD(do_tslash) { perform_ki_attack(g, ch, argument, TWIN_SLASH); }

ACMD(do_pslash) { perform_ki_attack(g, ch, argument, PHOENIX_SLASH); }

ACMD(do_ddslash) { perform_ki_attack(g, ch, argument, DARKNESS_DRAGON_SLASH); }

ACMD(do_kienzan) { perform_ki_attack(g, ch, argument, KIENZAN); }
