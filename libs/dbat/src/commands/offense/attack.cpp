/* attack.c: weapon attacks (attack, and the off hand's follow-up)
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc).
 *
 * Each kind of weapon (by its damage type) is a weapon_style: the skill it uses, what each hit says, and what it does
 * besides (a slash cuts limbs, a dagger backstabs, a club tires). attack is a blow with the main hand, or the off hand
 * if that's all there is; a second weapon then may follow up with a blow of its own, off_hand_attack(), which has its
 * own costs and wording. */
#include <span>
#include <array>
#include <algorithm>
#include <climits>

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
#include "dbat/game_world.hpp"

namespace {

struct weapon_style;

/* A blow being struck, as the styles' hooks see it */
struct weapon_shot {
  GameWorld *g{};
  struct char_data *ch{};
  struct char_data *vict{};
  const weapon_style *style{};
  bool main{};       /* the main blow, not the off hand's follow-up */
  int skill{};       /* init_skill()'s roll */
  int wlvl{};        /* the weapon's level, 0 to 5 */
  int64_t stcost{};  /* the stamina it costs */
  int64_t dmg{};
  int hitspot{};     /* 1 the stomach (or chest), 2 an arm, 3 a leg, 4 the face, 5 the chest (or stomach) */
  int64_t beforepl{}; /* the victim's PL before it */
};

struct weapon_style {
  int skill{}; /* SKILL_ */
  bool gun{};  /* it costs zenni, not stamina, and can't be disarmed by a parry */
  std::span<const act_msgs> main_hits{}; /* to_char, to_vict and to_room, by hit spot */
  std::span<const act_msgs> off_hits{};  /* quirk: the follow-up's own wording */
  /* each may be null */
  bool (*face_mastery)(const weapon_shot &s){}; /* a hit to the face is a critical one better */
  bool (*before_hit)(weapon_shot &s){};         /* true if that was the blow */
  void (*after_hit)(weapon_shot &s){};
};

/* What each says of a hit (generated from the old code, typos and all) */
constexpr auto SLASHING = std::to_array<act_msgs>({
    {"@WYou slash @C$N@W across the stomach!@n", "@c$n@W slashes you across the stomach!@n", "@c$n@W slashes @C$N@W across the stomach!@n", true},
    {"@WYou slash @C$N@W across the arm!@n", "@c$n@W slashes you across the arm!@n", "@c$n@W slashes @C$N@W across the arm!@n", true},
    {"@WYou slash @C$N@W across the leg!@n", "@c$n@W slashes you across the leg!@n", "@c$n@W slashes @C$N@W across the leg!@n", true},
    {"@WYou slash @C$N@W across the face!@n", "@c$n@W slashes you across the face!@n", "@c$n@W slashes @C$N@W across the face!@n", true},
    {"@WYou slash @C$N@W across the chest!@n", "@c$n@W slashes you across the chest!@n", "@c$n@W slashes @C$N@W across the chest!@n", true},
});
constexpr auto PIERCING_MAIN = std::to_array<act_msgs>({
    {"@WYou pierce @C$N's@W stomach!@n", "@c$n@W pierces your stomach!@n", "@c$n@W pierces @C$N's@W stomach!@n", true},
    {"@WYou pierce @C$N's@W arm!@n", "@c$n@W pierces your arm!@n", "@c$n@W pierces @C$N's@W arm!@n", true},
    {"@WYou pierce @C$N's@W leg!@n", "@c$n@W pierces your leg!@n", "@c$n@W pierces @C$N's@W leg!@n", true},
    {"@WYou pierce @C$N's@W face!@n", "@c$n@W pierces your face!@n", "@c$n@W pierces @C$N's@W face!@n", true},
    {"@WYou pierce @C$N's@W chest!@n", "@c$n@W pierces your chest!@n", "@c$n@W pierces @C$N's@W chest!@n", true},
});
constexpr auto PIERCING_OFF = std::to_array<act_msgs>({
    {"@WYou pierce @C$N's@W stomach!@n", "@c$n@W pierces your stomach!@n", "@c$n@W pierces @C$N's@W stomach!@n", true},
    {"@WYou pierce @C$N's@W arm!@n", "@c$n@W pierces your arm!@n", "@c$n@W pierces @C$N'@W arm!@n", true},
    {"@WYou pierce @C$N's@W leg!@n", "@c$n@W pierces your leg!@n", "@c$n@W pierces @C$N's@W leg!@n", true},
    {"@WYou pierce @C$N's@W face!@n", "@c$n@W pierces your face!@n", "@c$n@W pierces @C$N's@W face!@n", true},
    {"@WYou pierce @C$N's@W chest!@n", "@c$n@W pierces your chest!@n", "@c$n@W pierces @C$N's@W chest!@n", true},
});
constexpr auto CRUSHING_MAIN = std::to_array<act_msgs>({
    {"@WYou crush @C$N's@W chest!@n", "@c$n@W crushes your chest!@n", "@c$n@W crushes @C$N's@W chest@n", true},
    {"@WYou crush @C$N's@W arm!@n", "@c$n@W crushes your arm!@n", "@c$n@W crushes @C$N's@W arm!@n", true},
    {"@WYou crush @C$N's@W leg!@n", "@c$n@W crushes your leg!@n", "@c$n@W crushes @C$N's@W leg!@n", true},
    {"@WYou crush @C$N@W in the face!@n", "@c$n@W crushes you in the face!@n", "@c$n@W crushes @C$N@W in the face!@n", true},
    {"@WYou crush @C$N's@W stomach!@n", "@c$n@W crushes your stomach!@n", "@c$n@W crushes @C$N's@W stomach@n", true},
});
constexpr auto CRUSHING_OFF = std::to_array<act_msgs>({
    {"@WYou crush @C$N's@W stomach!@n", "@c$n@W crushes your stomach!@n", "@c$n@W crushes @C$N's@W stomach!@n", true},
    {"@WYou crush @C$N's@W arm!@n", "@c$n@W crushes your arm!@n", "@c$n@W crushes @C$N's@W arm!@n", true},
    {"@WYou crush @C$N's@W leg!@n", "@c$n@W crushes your leg!@n", "@c$n@W crushes @C$N's@W leg!@n", true},
    {"@WYou crush @C$N's@W face!@n", "@c$n@W crushes your face!@n", "@c$n@W crushes @C$N'@W face!@n", true},
    {"@WYou crush @C$N's@W chest!@n", "@c$n@W crushes your chest!@n", "@c$n@W crushes @C$N's@W chest!@n", true},
});
constexpr auto STABBING_MAIN = std::to_array<act_msgs>({
    {"@WYou stab @C$N's@W chest!@n", "@c$n@W stabs your chest!@n", "@c$n@W stabs @C$N's@W chest!@n", true},
    {"@WYou stab @C$N's@W arm!@n", "@c$n@W stabs your arm!@n", "@c$n@W stabs @C$N's@W arm!@n", true},
    {"@WYou stab @C$N's@W leg!@n", "@c$n@W stabs your leg!@n", "@c$n@W stabs @C$N's@W leg!@n", true},
    {"@WYou stab @C$N@W in the face!@n", "@c$n@W stabs you in the face!@n", "@c$n@W stabs @C$N@W in the face!@n", true},
    {"@WYou stab @C$N's@W stomach!@n", "@c$n@W stabs your stomach!@n", "@c$n@W stabs @C$N's@W stomach@n", true},
});
constexpr auto STABBING_OFF = std::to_array<act_msgs>({
    {"@WYou stab @C$N's@W stomach!@n", "@c$n@W stabs your stomach!@n", "@c$n@W stabs @C$N's@W stomach!@n", true},
    {"@WYou stab @C$N's@W arm!@n", "@c$n@W stabs your arm!@n", "@c$n@W stabs @C$N's@W arm!@n", true},
    {"@WYou stab @C$N's@W leg!@n", "@c$n@W stabs your leg!@n", "@c$n@W stabs @C$N's@W leg!@n", true},
    {"@WYou stab @C$N's@W face!@n", "@c$n@W stabs your face!@n", "@c$n@W stabs @C$N'@W face!@n", true},
    {"@WYou stab @C$N's@W chest!@n", "@c$n@W stabs your chest!@n", "@c$n@W stabs @C$N's@W chest!@n", true},
});
constexpr auto SHOOTING_MAIN = std::to_array<act_msgs>({
    {"@WYou blast @C$N's@W chest!@n", "@c$n@W blasts your chest!@n", "@c$n@W blasts @C$N's@W chest!@n", true},
    {"@WYou blast @C$N's@W arm!@n", "@c$n@W blasts your arm!@n", "@c$n@W blasts @C$N's@W arm!@n", true},
    {"@WYou blast @C$N's@W leg!@n", "@c$n@W blasts your leg!@n", "@c$n@W blasts @C$N's@W leg!@n", true},
    {"@WYou blast @C$N@W in the face!@n", "@c$n@W blasts you in the face!@n", "@c$n@W blasts @C$N@W in the face!@n", true},
    {"@WYou blast @C$N's@W stomach!@n", "@c$n@W blasts your stomach!@n", "@c$n@W blasts @C$N's@W stomach@n", true},
});
constexpr auto SHOOTING_OFF = std::to_array<act_msgs>({
    {"@WYou blast @C$N's@W stomach!@n", "@c$n@W blasts your stomach!@n", "@c$n@W blasts @C$N's@W stomach!@n", true},
    {"@WYou blast @C$N's@W arm!@n", "@c$n@W blasts your arm!@n", "@c$n@W blasts @C$N's@W arm!@n", true},
    {"@WYou blast @C$N's@W leg!@n", "@c$n@W blasts your leg!@n", "@c$n@W blasts @C$N's@W leg!@n", true},
    {"@WYou blast @C$N's@W face!@n", "@c$n@W blasts your face!@n", "@c$n@W blasts @C$N'@W face!@n", true},
    {"@WYou blast @C$N's@W chest!@n", "@c$n@W blasts your chest!@n", "@c$n@W blasts @C$N's@W chest!@n", true},
});
constexpr auto BRAWLING_MAIN = std::to_array<act_msgs>({
    {"@WYou whack @C$N's@W chest!@n", "@c$n@W whacks your chest!@n", "@c$n@W whacks @C$N's@W chest!@n", true},
    {"@WYou whack @C$N's@W arm!@n", "@c$n@W whacks your arm!@n", "@c$n@W whacks @C$N's@W arm!@n", true},
    {"@WYou whack @C$N's@W leg!@n", "@c$n@W whacks your leg!@n", "@c$n@W whacks @C$N's@W leg!@n", true},
    {"@WYou whack @C$N@W in the face!@n", "@c$n@W whacks you in the face!@n", "@c$n@W whacks @C$N@W in the face!@n", true},
    {"@WYou whack @C$N's@W stomach!@n", "@c$n@W whacks your stomach!@n", "@c$n@W whacks @C$N's@W stomach@n", true},
});
constexpr auto BRAWLING_OFF = std::to_array<act_msgs>({
    {"@WYou whack @C$N's@W stomach!@n", "@c$n@W whacks your stomach!@n", "@c$n@W whacks @C$N's@W stomach!@n", true},
    {"@WYou whack @C$N's@W arm!@n", "@c$n@W whacks your arm!@n", "@c$n@W whacks @C$N's@W arm!@n", true},
    {"@WYou whack @C$N's@W leg!@n", "@c$n@W whacks your leg!@n", "@c$n@W whacks @C$N's@W leg!@n", true},
    {"@WYou whack @C$N's@W face!@n", "@c$n@W whacks your face!@n", "@c$n@W whacks @C$N'@W face!@n", true},
    {"@WYou whack @C$N's@W chest!@n", "@c$n@W whacks your chest!@n", "@c$n@W whacks @C$N's@W chest!@n", true},
});

void off_hand_attack(GameWorld *g, struct char_data *ch);

/* A second weapon follows up a blow, as often as dual wielding allows */
void off_hand_follows(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  if (vict != nullptr && getCurPL(g, vict) > 1 && axion_dice(g, 0) < (char_get_skill(ch, SKILL_DUALWIELD)) &&
      char_get_equipment(ch, WEAR_WIELD1) && char_get_equipment(ch, WEAR_WIELD2))
    off_hand_attack(g, ch);
}

bool slash_face_mastery(const weapon_shot &s)
{
  return s.main && !char_is_npc(s.ch) && char_plr_flagged(s.ch, player_flag::thandw) && char_get_skills_base(s.ch, SKILL_TWOHAND) >= 100;
}

bool brawl_face_mastery(const weapon_shot &s)
{
  return char_get_skill(s.ch, SKILL_BRAWL) >= 100;
}

/* A slash that takes enough off may take a limb with it */
void slash_cuts(weapon_shot &s)
{
  if (s.beforepl - getCurPL(s.g, s.vict) >= (getEffMaxPL(s.g, s.vict)) * 0.025)
    cut_limb(s.g, s.ch, s.vict, s.wlvl, s.hitspot);
}

/* The main blow may be a backstab, when not yet fighting; or a dagger finds its mark by dexterity */
bool pierce_first(weapon_shot &s)
{
  if (s.main && !char_get_fighting(s.g, s.ch) && backstab(s.g, s.ch, s.vict, s.wlvl, s.dmg)) {
    off_hand_follows(s.g, s.ch, s.vict);
    pcost(s.g, s.ch, 0, s.stcost);
    return true;
  }
  s.dmg += (s.dmg * 0.01) * (char_get_aff_abils_dex(s.ch) * 0.5);
  return false;
}

void club_tires(weapon_shot &s)
{
  club_stamina(s.g, s.ch, s.vict, s.wlvl, s.dmg);
}

/* By damage type, as in STYLE_DAMTYPES, and then anything else */
enum { SLASH_STYLE, PIERCE_STYLE, CRUSH_STYLE, STAB_STYLE, GUN_STYLE, BRAWL_STYLE };
constexpr auto STYLE_DAMTYPES = std::to_array<int>({TYPE_SLASH, TYPE_PIERCE, TYPE_CRUSH, TYPE_STAB, TYPE_BLAST});
constexpr auto WEAPON_STYLES = std::to_array<weapon_style>({
    {SKILL_SWORD,  false, SLASHING, SLASHING, slash_face_mastery, nullptr, slash_cuts},
    {SKILL_DAGGER, false, PIERCING_MAIN, PIERCING_OFF, nullptr, pierce_first, nullptr},
    {SKILL_CLUB,   false, CRUSHING_MAIN, CRUSHING_OFF, nullptr, nullptr, club_tires},
    {SKILL_SPEAR,  false, STABBING_MAIN, STABBING_OFF},
    {SKILL_GUN,    true,  SHOOTING_MAIN, SHOOTING_OFF},
    {SKILL_BRAWL,  false, BRAWLING_MAIN, BRAWLING_OFF, brawl_face_mastery},
});

const weapon_style &style_of(struct obj_data *weapon)
{
  auto found = std::ranges::find(STYLE_DAMTYPES, obj_get_value(weapon, VAL_WEAPON_DAMTYPE) + TYPE_HIT);
  return WEAPON_STYLES[found - std::begin(STYLE_DAMTYPES)];
}

bool is_style(const weapon_shot &s, int style)
{
  return s.style == &WEAPON_STYLES[style];
}

/* Taking up the weapon rolls its skill, and practises it */
int take_up(GameWorld *g, struct char_data *ch, const weapon_style &style)
{
  int skill = init_skill(g, ch, style.skill);
  improve_skill(g, ch, style.skill, 1);
  return skill;
}

constexpr auto LEVEL_FLAGS = std::to_array<extra_flag>({extra_flag::weaplvl1, extra_flag::weaplvl2, extra_flag::weaplvl3, extra_flag::weaplvl4, extra_flag::weaplvl5});
constexpr auto LEVEL_SHARE = std::to_array<double>({0.05, 0.1, 0.2, 0.3, 0.5}); /* the damage a weapon's level adds */
constexpr auto GUN_COSTS = std::to_array<int>({0, 0, 2, 4, 6, 12});         /* zenni a shot, by the gun's level */
constexpr auto SPEAR_REACH = std::to_array<double>({0.04, 0.08, 0.12, 0.2, 0.25}); /* a spear's level adds this, two-handed */

int weapon_level(struct obj_data *weapon)
{
  for (int i = 0; i < 5; i++)
    if (obj_extra_flagged(weapon, LEVEL_FLAGS[i]))
      return i + 1;
  return 0;
}

constexpr act_msgs WEAPON_PARRIED = {"@C$N@W intercepts and parries your attack with $S own!@n",
                                     "@WYou intercept and parry @C$n's@W attack with one of your own!@n",
                                     "@C$N@W intercepts and parries @c$n's@W attack with one of $S own!@n", true};
constexpr act_msgs WEAPON_BLOCKED = {"@C$N@W moves quickly and blocks your attack!@n", "@WYou move quickly and block @C$n's@W attack!@n",
                                     "@C$N@W moves quickly and blocks @c$n's@W attack!@n", true};
constexpr act_msgs WEAPON_DODGED  = {"@C$N@W manages to dodge your attack!@n", "@WYou dodge @C$n's@W attack!@n",
                                     "@C$N@W manages to dodge @c$n's@W attack!@n", true};
constexpr act_msgs WEAPON_MISSED  = {"@WYou can't believe it but your attack misses!@n", "@C$n@W moves to attack you, but misses!@n",
                                     "@c$n@W moves to attack @C$N@W, but somehow misses!@n", true};

/* It misses, or the victim parries, blocks or dodges it (unless too tired to), and is paid for: or it lands (false) */
bool weapon_defended(weapon_shot &s, int prob, int perc, int pry, int blk, int dge)
{
  GameWorld *g           = s.g;
  struct char_data *ch   = s.ch;
  struct char_data *vict = s.vict;
  bool pays              = !s.style->gun;

  if (prob >= perc - 20)
    return false;
  if ((getCurST(g, vict)) > 0) {
    if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, mob_flag::dummy))) {
      act_triple(g, WEAPON_PARRIED, ch, nullptr, vict);
      if (!s.style->gun)
        handle_disarm(g, ch, vict);
      improve_skill(g, vict, SKILL_PARRY, 0);
      if (pays)
        pcost(g, ch, 0, s.stcost);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
      int64_t dmg = damtype(g, vict, -2, s.skill, 0);
      dmg *= calc_critical(g, ch, 1);
      hurt(g, 0, 0, vict, ch, nullptr, dmg, -1);
      return true;
    }
    if (blk > axion_dice(g, 10)) {
      act_triple(g, WEAPON_BLOCKED, ch, nullptr, vict);
      improve_skill(g, vict, SKILL_BLOCK, 0);
      if (pays)
        pcost(g, ch, 0, s.stcost);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
      int64_t dmg = damtype(g, ch, -1, s.skill, 0);
      dmg /= 4;
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
      return true;
    }
    if (dge > axion_dice(g, 10)) {
      act_triple(g, WEAPON_DODGED, ch, nullptr, vict);
      improve_skill(g, vict, SKILL_DODGE, 0);
      if (pays)
        pcost(g, ch, 0, s.stcost);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return true;
    }
  }
  act_triple(g, WEAPON_MISSED, ch, nullptr, vict);
  if (pays)
    pcost(g, ch, 0, s.stcost / 3);
  hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
  return true;
}

/* What the weapon's level, the brawler's skill and a Konatsu's way with a sword add */
void weapon_bonuses(weapon_shot &s)
{
  if (s.wlvl)
    s.dmg += s.dmg * LEVEL_SHARE[s.wlvl - 1];
  if (is_style(s, BRAWL_STYLE)) {
    if (char_get_skill(s.ch, SKILL_BRAWL) >= 100) {
      s.dmg += s.dmg * 0.5;
      s.wlvl = 5;
    } else if (char_get_skill(s.ch, SKILL_BRAWL) >= 50) {
      s.dmg += s.dmg * 0.2;
      s.wlvl = 3;
    }
  }
  if (is_style(s, SLASH_STYLE) && char_is_konatsu(s.ch))
    s.dmg += s.dmg * .25;
}

/* quirk: a spear's skill adds less at 100 than at 50 */
void spear_skill(weapon_shot &s)
{
  if (!is_style(s, STAB_STYLE))
    return;
  if (s.skill >= 100)
    s.dmg += s.dmg * 0.04;
  else if (s.skill >= 50)
    s.dmg += s.dmg * 0.1;
}

/* Where roll_hitloc()'s 1 (the stomach) to 5 (a leg) lands, as a hit spot: quirk: the off hand's arm and leg hits
   land on the chest, and a soft fist's critical on the body is only the main blow's */
constexpr auto MAIN_SPOTS = std::to_array<int>({1, 4, 5, 2, 3});
constexpr auto OFF_SPOTS = std::to_array<int>({1, 4, 5, 5, 5});

void aim(weapon_shot &s, int roll)
{
  if (char_get_bonuses(s.ch, BONUS_SOFT) && (roll == 1 || (s.main && roll == 3)))
    s.dmg *= calc_critical(s.g, s.ch, 2);
  s.hitspot = (s.main ? MAIN_SPOTS : OFF_SPOTS)[roll - 1];
}

constexpr auto EQ_BY_SPOT = std::to_array<int>({4, 1, 2, 3, 4}); /* dam_eq_loc()'s 1 arms, 2 legs, 3 head and 4 body */

/* It lands where aimed: false if it was something else (a backstab) */
bool weapon_lands(weapon_shot &s)
{
  if (s.style->before_hit && s.style->before_hit(s))
    return false;
  act_triple(s.g, (s.main ? s.style->main_hits : s.style->off_hits)[s.hitspot - 1], s.ch, nullptr, s.vict);
  if (s.hitspot == 2 || s.hitspot == 3)
    s.dmg *= calc_critical(s.g, s.ch, 1);
  else if (s.hitspot == 4) {
    if (s.style->face_mastery && s.style->face_mastery(s)) {
      double mult = calc_critical(s.g, s.ch, 0);
      mult += 1.0;
      s.dmg *= mult;
    } else
      s.dmg *= calc_critical(s.g, s.ch, 0);
  }
  hurt(s.g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 0);
  dam_eq_loc(s.g, s.vict, EQ_BY_SPOT[s.hitspot - 1]);
  if (s.style->after_hit)
    s.style->after_hit(s);
  return true;
}

constexpr act_msgs FIRESHIELD_BURNS = {"@c$N's@W fireshield burns your weapon!@n", "@C$n's@W weapon is burned by your fireshield!@n",
                                       "@c$n's@W weapon is burned by @C$N's@W fireshield!@n", true};

/* A fireshield burns the weapon that hit it, unless its wielder is fireproof (when, if tells, the victim hears so) */
void fireshield_burns(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *weapon, bool tells)
{
  if (getCurPL(g, vict) <= 0 || char_aff_flagged(vict, aff_flag::spirit) || !char_aff_flagged(vict, aff_flag::fireshield))
    return;
  if (!char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
    act_triple(g, FIRESHIELD_BURNS, ch, nullptr, vict);
    int damdam = char_get_skill(vict, SKILL_FIRESHIELD) / 2;
    hurt(g, 0, 0, vict, nullptr, weapon, damdam, 0);
  } else if (tells)
    send_to_char(g, vict, "@RThey appear to be fireproof!@n\r\n");
}

/* Learns skill (at 10 to 15) if it hasn't it, has a slot for it, and may; or else practises it */
void learn_or_practise(GameWorld *g, struct char_data *ch, int skill, bool may, std::string_view learnt)
{
  if (!char_get_skill(ch, skill) && slot_count(ch) + 1 <= char_get_skill_slots(ch) && may) {
    char_set_skills_base(ch, skill, rand_number(g, 10, 15));
    send_to_char(g, ch, "{}", learnt);
  } else
    improve_skill(g, ch, skill, 0);
}

/* Using an arm broken (above 0 and below 50) when the other is gone hurts it more */
void strain_broken_arm(GameWorld *g, struct char_data *ch)
{
  struct arms_entry {
    int arm, other;
    std::string_view strained, to_char, to_room;
  };
  static constexpr auto ARMS = std::to_array<arms_entry>({
      {0, 1, "Using your broken right arm has damaged it more!@n\r\n", "@RYour right arm has fallen apart!@n", "@r$n@R's right arm has fallen apart!@n"},
      {1, 0, "Using your broken left arm has damaged it more!@n\r\n", "@RYour left arm has fallen apart!@n", "@r$n@R's left arm has fallen apart!@n"},
  });

  for (const auto &a : ARMS) {
    if (char_get_limb_condition(ch, a.arm) > 0 && char_get_limb_condition(ch, a.arm) < 50 && char_get_limb_condition(ch, a.other) < 0) {
      send_to_char(g, ch, "{}", a.strained);
      char_set_limb_condition(ch, a.arm, char_get_limb_condition(ch, a.arm) - (rand_number(g, 3, 5)));
      if (char_get_limb_condition(ch, a.arm) < 0) {
        act(g, a.to_char, true, ch, nullptr, nullptr, act_to::actor);
        act(g, a.to_room, true, ch, nullptr, nullptr, act_to::room);
      }
      return;
    }
  }
}

/* A mob's skill with its off hand, by its level (quirk: never used, as mobs keep no dual wield skill to follow up
   with) */
namespace {
struct mob_off_hand_skill_entry {
  int level, low, high;
};
}
constexpr auto MOB_OFF_HAND_SKILL = std::to_array<mob_off_hand_skill_entry>({
    {10, 30, 50}, {20, 30, 60}, {30, 30, 70}, {50, 40, 80}, {70, 50, 90}, {80, 60, 100}, {90, 70, 100}, {100, 80, 100}, {INT_MAX, 95, 100},
});

/* The off hand's follow-up, at whoever ch is fighting here */
void off_hand_attack(GameWorld *g, struct char_data *ch)
{
  struct obj_data *weapon = char_get_equipment(ch, WEAR_WIELD2);
  if (!weapon)
    return;
  int64_t stcost = ((getEffMaxPL(g, ch) / 150) + obj_get_weight(weapon));
  if (char_is_android(ch))
    stcost *= 0.25;

  if (!can_grav(g, ch))
    return;
  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "With what arms!?\r\n");
    return;
  }
  strain_broken_arm(g, ch);
  if (!char_get_fighting(g, ch) || !check_points(g, ch, 0, stcost))
    return;

  const weapon_style &style = style_of(weapon);
  int skill                 = take_up(g, ch, style);
  int dualwield             = 0;
  if (!style.gun) {
    if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 100) {
      dualwield = 3;
      stcost -= stcost * 0.30;
    } else if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 75) {
      dualwield = 2;
      stcost -= stcost * 0.25;
    } else if (char_get_skills_base(ch, SKILL_DUALWIELD) >= 50) {
      dualwield = 1;
      stcost -= stcost * 0.25;
    }
  }
  if (char_is_npc(ch)) {
    auto band = std::ranges::find_if(MOB_OFF_HAND_SKILL, [&](const auto &b) { return char_get_level(ch) <= b.level; });
    skill     = rand_number(g, band->low, band->high);
  }
  struct char_data *vict = nullptr;
  if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch))
    vict = char_get_fighting(g, ch);
  if (style.gun) {
    if (char_get_gold(ch) < 1) {
      send_to_char(g, ch, "You do not have enough zenni. You need 1 zenni per shot.\r\n");
      return;
    }
    char_set_gold(ch, char_get_gold(ch) - (1));
  }
  handle_cooldown(g, ch, char_get_preference(ch) != PREFERENCE_H2H ? 4 : 8);
  if (!vict) { /* fighting someone elsewhere */
    send_to_char(g, ch, "Error! Please report.\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, style.gun ? 1 : 0))
    return;

  int pry = 0, blk = 0, dge = 0;
  int index = check_def(g, vict);
  int prob  = roll_accuracy(g, ch, skill, false);
  int perc  = chance_to_hit(g, ch);
  index -= handle_speed(g, ch, vict);
  int avo = index / 4;
  handle_defense(g, vict, &pry, &blk, &dge);
  if (dualwield == 3) {
    pry -= pry * 0.1;
    blk -= pry * 0.1;
    dge -= pry * 0.1; /* pry, This is intentional */
  }
  prob -= avo;
  tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);
  if (!tech_handle_zanzoken(g, ch, vict, "attack")) {
    if (!style.gun)
      pcost(g, ch, 0, stcost / 3);
    pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
    return;
  }

  weapon_shot s{.g = g, .ch = ch, .vict = vict, .style = &style, .main = false, .skill = skill, .wlvl = weapon_level(weapon), .stcost = stcost};
  if (weapon_defended(s, prob, perc, pry, blk, dge))
    return;
  s.dmg = damtype(g, ch, -1, skill, 0);
  weapon_bonuses(s);
  if (style.gun)
    s.dmg = gun_dam(g, ch, s.wlvl);
  int roll   = roll_hitloc(g, ch, vict, skill);
  s.beforepl = getCurPL(g, vict);
  spear_skill(s);
  aim(s, roll);
  weapon_lands(s);
  if (!style.gun) {
    fireshield_burns(g, ch, vict, char_get_equipment(ch, WEAR_WIELD2), true);
    pcost(g, ch, 0, stcost);
  }
}

} // namespace

/* A blow with the main hand's weapon, or the off hand's if that's all there is, at a victim or an object */
ACMD(do_attack)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  int64_t stcost        = (getEffMaxPL(g, ch) / 150);

  if (char_is_android(ch))
    stcost *= 0.25;
  if (playing_song(g, ch))
    return;
  struct obj_data *weapon = char_get_equipment(ch, WEAR_WIELD1);
  bool in_main            = weapon != nullptr;
  if (!in_main)
    weapon = char_get_equipment(ch, WEAR_WIELD2);
  if (!weapon) {
    send_to_char(g, ch, "You need to wield a weapon to use this, without one try punch, kick, or other no weapon attacks.\r\n");
    return;
  }
  if (!can_grav(g, ch))
    return;
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (!char_is_android(ch))
    stcost += obj_get_weight(weapon);
  else
    stcost += obj_get_weight(weapon) * 0.25;
  if (!check_points(g, ch, 0, stcost))
    return;

  struct char_data *vict = nullptr;
  struct obj_data *obj   = nullptr;
  if (!tech_handle_targeting(g, ch, arg, &vict, &obj))
    return;
  const weapon_style &style = style_of(weapon);
  if (vict && !can_kill(g, ch, vict, nullptr, style.gun ? 1 : 0))
    return;
  int skill = take_up(g, ch, style);
  int wlvl  = weapon_level(weapon);
  /* quirk: a gun in the off hand alone is free, and does a weapon's damage, not a gun's */
  bool gun = style.gun && in_main;

  handle_cooldown(g, ch, char_get_preference(ch) != PREFERENCE_H2H ? 4 : 8);
  if (gun) {
    if (char_get_gold(ch) < GUN_COSTS[wlvl]) {
      send_to_char(g, ch, "You do not have enough zenni. You need {:d} zenni per shot for that level of gun.\r\n", GUN_COSTS[wlvl]);
      return;
    }
    char_set_gold(ch, char_get_gold(ch) - (GUN_COSTS[wlvl]));
  }

  if (!vict) {
    if (!obj) {
      send_to_char(g, ch, "Error! Please report.\r\n");
      return;
    }
    if (!can_kill(g, ch, nullptr, obj, 0))
      return;
    if (obj_extra_flagged(obj, extra_flag::broken)) {
      send_to_char(g, ch, "It is broken already!\r\n");
      return;
    }
    int64_t dmg = ((getCurPL(g, ch) / 10000) + (char_get_aff_abils_str(ch)));
    act(g, "@WYou attack $p@W as hard as you can!@n", true, ch, obj, nullptr, act_to::actor);
    act(g, "@C$n@W attacks $p@W extremely hard!@n", true, ch, obj, nullptr, act_to::room);
    hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
    if (!style.gun)
      pcost(g, ch, 0, stcost);
    return;
  }

  if (handle_defender(g, vict, ch))
    vict = char_get_defender(g, vict);
  int pry = 0, blk = 0, dge = 0;
  int index = check_def(g, vict);
  int prob  = roll_accuracy(g, ch, skill, false);
  int perc  = chance_to_hit(g, ch);
  index -= handle_speed(g, ch, vict);
  int avo = index / 4;
  handle_defense(g, vict, &pry, &blk, &dge);
  prob -= avo;
  if (char_plr_flagged(ch, player_flag::thandw))
    perc += 15;
  tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);
  if (!tech_handle_zanzoken(g, ch, vict, "attack")) {
    if (!style.gun)
      pcost(g, ch, 0, stcost / 3);
    pcost(g, vict, 0, getEffMaxPL(g, vict) / 150);
    return;
  }

  weapon_shot s{.g = g, .ch = ch, .vict = vict, .style = &style, .main = true, .skill = skill, .wlvl = wlvl, .stcost = stcost};
  if (weapon_defended(s, prob, perc, pry, blk, dge))
    return;
  s.dmg = damtype(g, ch, -1, skill, 0);
  weapon_bonuses(s);
  if (char_plr_flagged(ch, player_flag::thandw))
    s.dmg += s.dmg * 1.2;
  if (!char_is_npc(ch) && char_plr_flagged(ch, player_flag::thandw) && !style.gun) {
    if (char_get_skills_base(ch, SKILL_TWOHAND) >= 100)
      s.dmg += s.dmg * 0.5;
    else if (char_get_skills_base(ch, SKILL_TWOHAND) >= 75)
      s.dmg += s.dmg * 0.25;
    else if (char_get_skills_base(ch, SKILL_TWOHAND) >= 50)
      s.dmg += s.dmg * 0.1;
    if (is_style(s, STAB_STYLE) && s.wlvl)
      s.dmg += s.dmg * SPEAR_REACH[s.wlvl - 1];
  }
  spear_skill(s);
  if (gun)
    s.dmg = gun_dam(g, ch, s.wlvl);
  int roll   = roll_hitloc(g, ch, vict, skill);
  s.beforepl = getCurPL(g, vict);
  aim(s, roll);
  if (char_plr_flagged(ch, player_flag::thandw) && gun && s.hitspot != 4 && boom_headshot(g, ch)) {
    s.hitspot = 4;
    send_to_char(g, ch, "@GBoom headshot!@n\r\n");
  }
  if (!weapon_lands(s))
    return;

  if (!style.gun) {
    if (char_get_equipment(ch, WEAR_WIELD1))
      fireshield_burns(g, ch, vict, char_get_equipment(ch, WEAR_WIELD1), true);
    else /* quirk: the victim isn't told when an off hand alone is fireproof */
      fireshield_burns(g, ch, vict, char_get_equipment(ch, WEAR_WIELD2), false);
    pcost(g, ch, 0, stcost);
    damage_weapon(g, ch, weapon, vict);
  }
  if (!char_is_npc(ch) && char_plr_flagged(ch, player_flag::thandw))
    learn_or_practise(g, ch, SKILL_TWOHAND, true, "@GYou learn the very basics of two-handing your weapon!@n\r\n");
  if (struct obj_data *off = char_get_equipment(ch, WEAR_WIELD2)) {
    learn_or_practise(g, ch, SKILL_DUALWIELD, obj_get_type_flag(off) != ITEM_LIGHT, "@GYou learn the very basics of dual-wielding!@n\r\n");
    off_hand_follows(g, ch, vict);
  }
  handle_multihit(g, ch, vict);
}
