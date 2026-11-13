/* items.c: item crafting, repair and use
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>

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
#include "dbat/local_limits.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

namespace {

/* Worn equipment made by someone level 30 or more gets their level, rounded down to ten (at most 100), and
 * stats to match. */
void boost_obj(GameWorld *g, struct obj_data *obj, struct char_data *ch)
{
  int level = char_get_level(ch);
  if (level < 30)
    return;

  int boost = std::min(level, 100) / 10 * 10;
  obj_set_level(obj, boost);
  obj_get_affected(obj)[0].location = 17;
  obj_get_affected(obj)[0].modifier += boost * level;
  obj_get_affected(obj)[1].location = obj_get_vnum(g, obj) == 91 ? 1 : 3; /* pants */
  obj_get_affected(obj)[1].modifier = boost / 20;
}

/* What concentration takes off the ki a creation costs: the first row the skill reaches. */
constexpr struct {
  int skill;
  double discount;
} DISCOUNTS[] = {{100, 0.5}, {90, 0.6}, {80, 0.65}, {70, 0.7}, {60, 0.75}, {50, 0.8}, {40, 0.85}, {30, 0.9}, {20, 0.95}};

double concentration_discount(struct char_data *ch)
{
  int skill = char_get_skill(ch, SKILL_CONCENTRATION);
  auto row  = std::ranges::find_if(DISCOUNTS, [skill](const auto &d) { return skill >= d.skill; });
  return row == std::end(DISCOUNTS) ? 1.0 : row->discount;
}

struct form_request {
  std::string_view what; /* the creation's name */
  std::string &arg2;
  const std::string &arg3; /* the rest of the line */
  int skill;
  double discount;
};

bool has_ki_for(GameWorld *g, struct char_data *ch, const form_request &req, int64_t cost)
{
  if (getCurKI(g, ch) >= cost)
    return true;
  send_to_char(g, ch, "You do not have enough ki to create {}\r\n", req.what);
  return false;
}

struct obj_data *conjure(GameWorld *g, obj_vnum vnum)
{
  struct obj_data *obj = read_object(g, vnum);
  add_unique_id(g, obj);
  return obj;
}

/* The end of every creation: obj (the last made) is shown off and paid for. */
void created(GameWorld *g, struct char_data *ch, struct obj_data *obj, int64_t cost, bool cooldown = true)
{
  reveal_hiding(g, ch, 0);
  if (cooldown)
    char_set_con_cooldown(ch, 10);
  act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
  act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
  decCurKI(g, ch, cost, 0);
}

/* A better grade of a creation, by name, for those with the skill: 0 is the lowest, i the i-th row's. */
struct grade {
  std::string_view word;
  int skill;
};
constexpr grade GRADES[]        = {{"mid", 50}, {"high", 75}, {"highest", 100}};
constexpr grade WEAPON_GRADES[] = {{"mid", 30}, {"high", 50}, {"higher", 75}, {"highest", 100}};

size_t pick_grade(GameWorld *g, struct char_data *ch, std::string_view word, int skill, std::span<const grade> grades,
                  std::string_view syntax)
{
  if (word.empty()) {
    send_to_char(g, ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) ({})\r\n", syntax);
    send_to_char(g, ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
    return 0;
  }
  auto row = std::ranges::find_if(grades, [&](const grade &gr) { return str_eq_nocase(word, gr.word) && skill >= gr.skill; });
  return row == grades.end() ? 0 : row - grades.begin() + 1;
}

/* Most creations differ only in these. */
struct creation {
  int64_t (*cost)(int64_t max_ki, int skill);
  std::array<obj_vnum, 4> vnums; /* by grade, if graded */
  bool graded;
  bool boosted;  /* worn equipment, made better by a high level */
  bool on_floor; /* or in hand */
  bool no_rent;
  bool sized;
  bool cooldown{true};
};

template <const creation &C> void make(GameWorld *g, struct char_data *ch, form_request &req)
{
  int64_t cost = C.cost(getMaxKI(g, ch), req.skill);
  cost *= req.discount;

  if (!has_ki_for(g, ch, req, cost))
    return;

  size_t grade = C.graded ? pick_grade(g, ch, req.arg2, req.skill, GRADES, "mid | high | highest") : 0;

  struct obj_data *obj = conjure(g, C.vnums[grade]);
  if (C.boosted)
    boost_obj(g, obj, ch);
  if (C.on_floor)
    obj_to_room(g, obj, char_get_in_room(ch));
  else
    obj_to_char(g, obj, ch);
  if (C.no_rent) {
    obj_extra_set(obj, ITEM_NORENT, true);
    obj_extra_set(obj, ITEM_NOSELL, true);
  }
  if (C.sized)
    obj_set_size(obj, get_size(ch));
  created(g, ch, obj, cost, C.cooldown);
}

constexpr int64_t cheap(int64_t max_ki, int skill) { return max_ki / (skill * 2); }
constexpr int64_t fifth(int64_t max_ki, int) { return max_ki / 5; }
constexpr int64_t nearly_all(int64_t max_ki, int) { return max_ki - 1; }

constexpr creation FOOD     = {.cost = [](int64_t max_ki, int skill) { return max_ki / std::max(skill / 2, 1); }, .vnums = {70, 1510, 1511, 1512}, .graded = true};
constexpr creation WATER    = {.cost = cheap, .vnums = {71, 1513, 1514, 1515}, .graded = true};
constexpr creation LIGHT    = {.cost = cheap, .vnums = {72}, .sized = true};
constexpr creation BAG      = {.cost = cheap, .vnums = {319}};
constexpr creation MATTRESS = {.cost = [](int64_t max_ki, int skill) { return max_ki / skill; }, .vnums = {16}, .cooldown = false};
constexpr creation PANTS    = {.cost = fifth, .vnums = {91}, .boosted = true, .sized = true};
constexpr creation GI       = {.cost = fifth, .vnums = {92}, .boosted = true, .sized = true};
constexpr creation WRIST    = {.cost = fifth, .vnums = {1528}, .boosted = true, .sized = true};
constexpr creation BOOTS    = {.cost = fifth, .vnums = {1532}, .boosted = true, .sized = true};
constexpr creation SHURIKEN = {.cost = [](int64_t max_ki, int) { return max_ki / 4; }, .vnums = {19053}, .no_rent = true, .sized = true};
constexpr creation KACHIN   = {.cost = nearly_all, .vnums = {87}, .on_floor = true, .sized = true};

constexpr struct {
  std::string_view name;
  std::array<obj_vnum, 5> vnums; /* by grade */
} WEAPONS[] = {
    {"sword", {90, 1516, 1517, 1518, 1519}},
    {"dagger", {1536, 1537, 1538, 1539, 1540}},
    {"club", {1541, 1542, 1543, 1544, 1545}},
    {"spear", {1546, 1547, 1548, 1549, 1550}},
    {"gun", {1551, 1552, 1553, 1554, 1555}},
};

void weapon(GameWorld *g, struct char_data *ch, form_request &req)
{
  int64_t cost = getMaxKI(g, ch) / 5;
  cost *= req.discount;

  if (!has_ki_for(g, ch, req, cost))
    return;
  if (req.arg2.empty()) {
    send_to_char(g, ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
    return;
  }

  size_t grade = pick_grade(g, ch, req.arg3, req.skill, WEAPON_GRADES, "mid | high | higher | highest");
  auto kind    = std::ranges::find_if(WEAPONS, [&](const auto &w) { return str_eq_nocase(req.arg2, w.name); });
  if (kind == std::end(WEAPONS)) {
    send_to_char(g, ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
    return;
  }

  struct obj_data *obj = conjure(g, kind->vnums[grade]);
  obj_to_char(g, obj, ch);
  obj_set_size(obj, get_size(ch));
  created(g, ch, obj, cost);
}

/* A full set of clothes, put on a follower */
void clothesbeam(GameWorld *g, struct char_data *ch, form_request &req)
{
  int64_t cost = getMaxKI(g, ch) / 2;
  cost *= req.discount;

  if (!has_ki_for(g, ch, req, cost))
    return;
  if (req.arg2.empty()) {
    send_to_char(g, ch, "Who do you want to hit with clothesbeam?\r\nSyntax: create clothesbeam (target)\r\n");
    return;
  }

  struct char_data *vict = get_char_vis(g, ch, req.arg2, nullptr, FIND_CHAR_ROOM);
  if (!vict) {
    send_to_char(g, ch, "Clothesbeam who?\r\nSyntax: create clothesbeam (target)\r\n");
    return;
  }
  if (char_get_master(g, vict) != ch) {
    send_to_char(g, ch, "They must be following you first.\r\n");
    return;
  }

  struct obj_data *obj = nullptr;
  for (obj_vnum vnum : {92, 91, 1528, 1528, 1532}) { /* gi, pants, two wristbands, boots */
    obj = conjure(g, vnum);
    boost_obj(g, obj, ch);
    obj_to_char(g, obj, vict);
    obj_set_size(obj, get_size(vict));
  }
  do_wear(g, vict, "all", 0, 0);
  created(g, ch, obj, cost);
}

void elixir(GameWorld *g, struct char_data *ch, form_request &req)
{
  int64_t cost = getMaxKI(g, ch) - 1;
  cost *= req.discount;

  if (!has_ki_for(g, ch, req, cost))
    return;
  if (getCurPL(g, ch) < getEffMaxPL(g, ch)) {
    send_to_char(g, ch, "You need to be at full powerlevel to create {}\r\n", req.what);
    return;
  }
  if (char_get_practice_points(g, ch) < 10) {
    send_to_char(g, ch, "You do not have enough PS to create {}, you need at least 10.\r\n", req.what);
    return;
  }

  struct obj_data *obj = conjure(g, 86);
  obj_to_room(g, obj, char_get_in_room(ch));
  obj_set_size(obj, get_size(ch));
  created(g, ch, obj, cost);
  decCurHealthPercent(g, ch, 1, 1);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - 10);
}

/* Costs everything, and no less with concentration */
void senzu(GameWorld *g, struct char_data *ch, form_request &req)
{
  int64_t cost  = getMaxKI(g, ch);
  int64_t cost2 = getEffMaxPL(g, ch) - 1;

  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to create {}, you need full ki.\r\n", req.what);
    return;
  }
  if (getCurPL(g, ch) <= cost2) {
    send_to_char(g, ch, "You do not have enough powerlevel to create {}, you need to be at full.\r\n", req.what);
    return;
  }
  if (getCurST(g, ch) < getMaxST(g, ch)) {
    send_to_char(g, ch, "You do not have enough stamina to create {}, you need to be at full.\r\n", req.what);
    return;
  }
  if (char_get_practice_points(g, ch) < 50) {
    send_to_char(g, ch, "You do not have enough PS to create {}, you need at least 50.\r\n", req.what);
    return;
  }

  struct obj_data *obj = conjure(g, 1);
  obj_to_char(g, obj, ch);
  created(g, ch, obj, cost);
  decCurHealth(g, ch, cost2, 0);
  decCurSTPercent(g, ch, 1, 1);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - 50);
}

/* In the menu's order. Names match exactly. */
constexpr struct form_kind {
  std::string_view name;
  int skill; /* the creation skill it takes */
  std::string_view menu;
  void (*make)(GameWorld *, struct char_data *, form_request &);
} FORMS[] = {
    {"food", 0, "create food\r\n", make<FOOD>},
    {"water", 0, "create water\r\n", make<WATER>},
    {"light", 20, "create light\r\n", make<LIGHT>},
    {"bag", 30, "create bag\r\n", make<BAG>},
    {"mattress", 40, "create mattress\r\n", make<MATTRESS>},
    {"weapon", 50, "create weapon (sword | club | dagger | spear | gun )\r\n", weapon},
    {"pants", 50, "create pants\r\n", make<PANTS>},
    {"gi", 50, "create gi\r\n", make<GI>},
    {"wristband", 50, "create wristband\r\n", make<WRIST>},
    {"boots", 50, "create boots\r\n", make<BOOTS>},
    {"clothesbeam", 60, "create clothesbeam (target)\r\n", clothesbeam},
    {"shuriken", 70, "create shuriken\r\n", make<SHURIKEN>},
    {"senzu", 80, "create senzu\r\n", senzu},
    {"kachin", 90, "create kachin\r\n", make<KACHIN>},
    {"elixir", 100, "create elixir\r\n", elixir},
};

} // namespace

ACMD(do_form)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string arg2      = take_word(rest);
  std::string arg3      = std::string(skip_spaces(rest)); /* the rest of the line, as half_chop left it */

  if (!know_skill(g, ch, SKILL_CREATE))
    return;

  int skill = char_get_skill(ch, SKILL_CREATE);

  if (arg.empty()) {
    std::string menu;
    for (const form_kind &f : FORMS)
      if (skill >= f.skill)
        menu += f.menu;
    send_to_char(g, ch, "What do you want to create?\r\n@GCreation @WMenu@n\r\n@D---------------@n\r\n@w{}\r\n", menu);
    return;
  }
  reveal_hiding(g, ch, 0);

  auto kind = std::ranges::find(FORMS, std::string_view(arg), &form_kind::name);
  if (kind == std::end(FORMS)) {
    send_to_char(g, ch, "Create what?\r\n");
    return;
  }
  if (skill < kind->skill) {
    send_to_char(g, ch, "What do you want to create?\r\n");
    return;
  }

  form_request req{kind->name, arg2, arg3, skill, concentration_discount(ch)};
  kind->make(g, ch, req);
}

ACMD(do_recharge)
{
  if (char_is_npc(ch) || !char_is_android(ch)) {
    send_to_char(g, ch, "Only androids can use recharge\r\n");
    return;
  }
  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before your nanites can convert your ki.\r\n");
    return;
  }
  if (!char_plr_flagged(ch, PLR_REPAIR)) {
    send_to_char(g, ch, "You are not a repair model android.\r\n");
    return;
  }
  int64_t cost = getMaxST(g, ch) / 20;
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to recharge your stamina.\r\n");
    return;
  }
  if (getCurST(g, ch) >= getMaxST(g, ch)) {
    send_to_char(g, ch, "Your energy reserves are already full.\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  char_set_con_cooldown(ch, 10);
  act(g, "You focus your ki into your energy reserves, recharging them some.", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "$n stops and glows green briefly.", true, ch, nullptr, nullptr, TO_ROOM);
  decCurKI(g, ch, cost, 0);
  if (getCurST(g, ch) + cost * 2 < getMaxST(g, ch))
    incCurST(g, ch, cost * 2, true);
  else {
    restoreST(ch, false);
    send_to_char(g, ch, "You are fully recharged now.\r\n");
  }
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_srepair)
{
  if (!char_is_android(ch)) {
    send_to_char(g, ch, "Only androids can use repair, maybe you want 'fix' instead?\r\n");
    return;
  }
  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before your nanites can repair you.\r\n");
    return;
  }
  if (!char_is_npc(ch) && !char_plr_flagged(ch, PLR_REPAIR)) {
    send_to_char(g, ch, "You are not a repair model android.\r\n");
    return;
  }
  int64_t cost = getEffMaxPL(g, ch) / 40;
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough stamina to repair yourself.\r\n");
    return;
  }
  if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
    send_to_char(g, ch, "You are already at full functionality and do not require repairs.\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  char_set_con_cooldown(ch, 10);
  act(g, "You repair some of your outer casings and internal systems, with the small nano-robots contained in your body.", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "$n stops a moment as small glowing particles move across $s body.", true, ch, nullptr, nullptr, TO_ROOM);

  /* and a fifth of the wear off everything worn */
  bool repaired = false;
  for (int i = 0; i < NUM_WEARS && !char_is_npc(ch); i++) {
    struct obj_data *obj = char_get_equipment(ch, i);
    if (!obj || obj_get_value(obj, VAL_ALL_HEALTH) >= 100)
      continue;
    obj_set_value(obj, VAL_ALL_HEALTH, std::min(obj_get_value(obj, VAL_ALL_HEALTH) + 20, 100));
    obj_extra_set(obj, ITEM_BROKEN, false);
    repaired = true;
  }
  if (repaired)
    send_to_char(g, ch, "@GYour nano-robots also repair all of your equipment a little bit.@n\r\n");

  decCurST(g, ch, cost, 0);
  int64_t heal = cost * 2;
  if (char_get_bonuses(ch, BONUS_HEALER) > 0)
    heal += heal * .25;
  if (incCurHealth(g, ch, heal, true) == getEffMaxPL(g, ch))
    send_to_char(g, ch, "You are fully repaired now.\r\n");
  if (!char_is_npc(ch) && rand_number(g, 1, 3) == 2 && getCurKI(g, ch) < getMaxKI(g, ch)) {
    send_to_char(g, ch, "@GThe repairs have managed to relink power reserves and boost your current energy level.@n\r\n");
    incCurKI(g, ch, cost, true);
  }
  char_set_wait(ch, PULSE_2SEC);
}

namespace {
  /* What upgrade and augment can raise */
  struct upgradable {
    const char *name;
    int cost;                 /* in upgrade points, per upgrade */
    std::array<int, 7> bonus; /* per upgrade, times the level, by UPGRADE_TIERS */
    int64_t (*gain)(char_data *, int64_t, bool);
  };
  /* The levels each bonus starts at */
  constexpr std::array<int, 7> UPGRADE_TIERS = {90, 80, 70, 60, 50, 25, 0};
  constexpr upgradable UPGRADABLE[] = {
      {"powerlevel", 75, {5000, 2500, 2000, 1300, 500, 250, 150}, gainBasePL},
      {"ki", 40, {3650, 2450, 1800, 1250, 400, 200, 120}, gainBaseKI},
      {"stamina", 50, {3650, 2450, 1800, 1250, 500, 200, 120}, gainBaseST},
  };

  const upgradable *upgradable_named(std::string_view name)
  {
    auto u = std::ranges::find_if(UPGRADABLE, [&](const upgradable &each) { return str_eq_nocase(each.name, name); });
    return u == std::end(UPGRADABLE) ? nullptr : &*u;
  }

  /* upgrade augment (what): an augmentation kit, for androids of level 80 or more */
  void augment(GameWorld *g, struct char_data *ch, std::string &what)
  {
    if (char_get_level(ch) < 80) {
      send_to_char(g, ch, "You need to be at least level 80 to use these kits.\r\n");
      return;
    }
    std::string augmentation = "Augmentation";
    struct obj_data *kit     = get_obj_in_list_vis(g, ch, augmentation, nullptr, char_get_inventory(ch));
    if (!kit) {
      send_to_char(g, ch, "You don't have a Circuit Augmentation Kit.\r\n");
      return;
    }
    int64_t gain = 0;
    if (char_get_level(ch) <= 100)
      gain = getEffMaxPL(g, ch) * 0.005;
    if (char_get_level(ch) == 100)
      gain = std::min<int64_t>(gain, 10000000);
    const upgradable *u = upgradable_named(what);
    if (!u) {
      send_to_char(g, ch, "What do you want to augment? Powerlevel, ki, or stamina?\r\n");
      return;
    }
    obj_from_char(g, kit);
    extract_obj(g, kit);
    act(g, str_format("@WYou install the circuits and upgrade your maximum {}.@n", u->name).c_str(), true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
    u->gain(ch, gain, true);
    send_to_char(g, ch, "@gGain @D[@G+{}@D]\r\n", add_commas(gain));
  }
} // namespace

ACMD(do_upgrade)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (char_is_npc(ch) || !char_is_android(ch)) {
    send_to_char(g, ch, "You are not an android!\r\n");
    return;
  }
  if (arg.empty()) {
    if (!char_plr_flagged(ch, PLR_ABSORB))
      send_to_char(g, ch, "@c--------@D[@rUpgrade Menu@D]@c--------\r\n@cUpgrade @RPowerlevel@D: @Y75 @WPoints\r\n@cUpgrade @CKi        @D: @Y40 @WPoints\r\n@cUpgrade @GStamina   @D: @Y50 @WPoints\r\n@D            -----------\r\n");
    send_to_char(g, ch, "@cAugment @RPowerlevel\r\n@cAugment @CKi\r\n@cAugment @GStamina\r\n@WCurrent Upgrade Points @D[@y{}@D]@n\r\n", add_commas(char_get_upgrade(ch)));
    return;
  }
  if (str_eq_nocase("augment", arg)) {
    augment(g, ch, arg2);
    return;
  }
  if (char_plr_flagged(ch, PLR_ABSORB)) {
    send_to_char(g, ch, "You are an absorb model and can only upgrade with augmentation kits.\r\n");
    return;
  }
  if (is_soft_cap(ch, 0, 1.0)) {
    send_to_char(g, ch, "@mYou are unable to spend anymore UGP right now (Softcap)@n\r\n");
    return;
  }
  const upgradable *u = upgradable_named(arg);
  if (!u) {
    send_to_char(g, ch, "That is not a valid upgrade option.\r\n");
    return;
  }
  if (arg2.empty()) {
    send_to_char(g, ch, "How many times do you want to increase {}?", arg);
    return;
  }
  int count = parse_int(arg2);
  if (count <= 0 || count > 1000) {
    send_to_char(g, ch, "It needs to be between 1-1000\r\n");
    return;
  }
  size_t tier = std::ranges::find_if(UPGRADE_TIERS, [&](int from) { return char_get_level(ch) >= from; }) - UPGRADE_TIERS.begin();
  int bonus   = count * char_get_level(ch) * u->bonus[tier];
  int cost    = count * u->cost;
  if (cost > char_get_upgrade(ch)) {
    send_to_char(g, ch, "You need {} upgrade points, and only have {}.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
    return;
  }
  if (is_soft_cap(ch, bonus, 1.0)) {
    send_to_char(g, ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
    return;
  }
  char_set_upgrade(ch, char_get_upgrade(ch) - cost);
  send_to_char(g, ch, "You upgrade your system and gain {} {}!", add_commas(bonus), arg);
  u->gain(ch, bonus, true);
}

namespace {
  /* Why a majin of ch's level and appetite can't ingest vict, if they can't */
  const char *cannot_ingest(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    int level = char_get_level(ch), absorbs = char_get_absorbs(ch);
    if (absorbs > 3)
      return "You already have already ingested 4 people.\r\n";
    if (level < 25)
      return "You can't ingest yet.\r\n";
    if ((level < 100 && level >= 75 && absorbs == 3) || (level < 75 && level >= 50 && absorbs == 2) || (level < 50 && level >= 25 && absorbs == 1))
      return "You already have ingested as much as you can. You'll have to get more experienced.\r\n";
    if (getEffMaxPL(g, vict) >= getBasePL(ch) * 3)
      return "You are too weak to ingest them into your body!\r\n";
    if (char_aff_flagged(vict, AFF_SANCTUARY))
      return "You can't ingest them, they have a barrier!\r\n";
    return nullptr;
  }

  /* What ingesting vict changes about ch's looks: one thing, each in turn a one in three chance */
  void takes_after(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (rand_number(g, 1, 3) == 3) {
      send_to_char(g, ch, "You get {}'s eye color.\r\n", char_get_name(vict));
      char_set_eye(ch, char_get_eye(vict));
    } else if (rand_number(g, 1, 3) == 3) {
      send_to_char(g, ch, "{} changes your height.\r\n", char_get_name(vict));
      int mine = char_get_pc_height(g, ch), theirs = char_get_pc_height(g, vict);
      if (mine > theirs)
        char_set_height(ch, char_get_height(ch) - (mine - theirs) / 2);
      else if (mine < theirs)
        char_set_height(ch, char_get_height(ch) + (theirs - mine) / 2);
      else
        char_set_height(ch, theirs);
    } else if (rand_number(g, 1, 3) == 3) {
      send_to_char(g, ch, "{} changes your weight.\r\n", char_get_name(vict));
      int mine = char_get_pc_weight(g, ch), theirs = char_get_pc_weight(g, vict);
      if (mine > theirs)
        char_set_weight(ch, char_get_weight(ch) - (mine - theirs) / 2);
      else if (mine < theirs)
        char_set_weight(ch, char_get_weight(ch) + (theirs - mine) / 2);
      else
        char_set_weight(ch, theirs);
    } else {
      send_to_char(g, ch, "Your forelock length changes because of {}.\r\n", char_get_name(vict));
      char_set_hairl(ch, char_get_hairl(vict));
    }
  }
} // namespace

ACMD(do_ingest)
{
  if (!char_is_majin(ch)) {
    send_to_char(g, ch, "You are not a majin, you can not ingest.\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Who do you want to ingest?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Ingest who?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  if (char_get_absorbby(g, vict)) {
    send_to_char(g, ch, "{} is already absorbing from them!", char_get_name(char_get_absorbby(g, vict)));
    return;
  }
  if (const char *why = cannot_ingest(g, ch, vict)) {
    send_to_char(g, ch, why);
    return;
  }
  reveal_hiding(g, ch, 0);
  if (char_aff_flagged(vict, AFF_ZANZOKEN) && getCurST(g, vict) >= 1 && char_get_position(vict) != POS_SLEEPING) {
    act_triple(g, {"@C$N@c disappears, avoiding your attempted ingestion!@n", "@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", "@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n"}, ch, nullptr, vict);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    char_set_wait(ch, PULSE_3SEC);
    return;
  }
  /* quirk: ch's speed is matched against ch's own */
  if (char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, ch) + rand_number(g, 1, 5)) {
    act_triple(g, {"@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E manages to avoid your blob of goo though!@n", "@C$n@W flings a piece of goo at you, you manage to avoid it though!@n", "@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n", true}, ch, nullptr, vict);
    char_set_wait(ch, PULSE_3SEC);
    return;
  }
  act_triple(g, {"@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then returns to your body!@n", "@C$n@W flings a piece of goo at you! The goo engulfs your body and then returns to @C$n@W!@n", "@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then return to @C$n@W!@n", true}, ch, nullptr, vict);
  char_set_absorbs(ch, char_get_absorbs(ch) + 1);
  int64_t pl = getBasePL(vict) / 6, stam = getBaseST(vict) / 6, ki = getBaseKI(vict) / 6;
  gainBasePL(ch, pl, true);
  gainBaseST(ch, stam, true);
  gainBaseKI(ch, ki, true);
  if (!char_is_npc(vict) && !char_is_npc(ch)) {
    send_to_imm(g, "[PK] {} killed {} at room [{:d}]\r\n", char_get_name(ch), char_get_name(vict), char_get_in_room(vict));
    char_plr_set(vict, PLR_ABSORBED, true);
  }
  send_to_char(g, ch, "@D[@mINGEST@D] @rPL@W: @D(@y{}@D) @cKi@W: @D(@y{}@D) @gSt@W: @D(@y{}@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
  takes_after(g, ch, vict);
  handle_ingest_learn(g, ch, vict);
  die(g, vict, nullptr);
}

ACMD(do_plant)
{
  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string obj_name  = take_arg(rest);
  std::string vict_name = take_arg(rest);

  auto found = find_char(g, ch, vict_name, FIND_CHAR_ROOM, "Plant what on who?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (vict == ch) {
    send_to_char(g, ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (char_mob_flagged(vict, MOB_NOKILL) && char_get_admlevel(ch) == ADMLVL_NONE) {
    send_to_char(g, ch, "That isn't such a good idea...\r\n");
    return;
  }

  int roll   = roll_skill(g, ch, SKILL_SLEIGHT_OF_HAND) + rand_number(g, 1, 3);
  int fail   = rand_number(g, 1, 105);
  int detect = char_get_position(vict) < POS_SLEEPING ? 0 : roll_skill(g, vict, SKILL_SPOT) + rand_number(g, 1, 3);
  /* NO NO With Imp's and Shopkeepers, and if player planting is not allowed */
  if ((char_adm_flagged(vict, ADM_NOSTEAL) || char_get_mob_spec(g, vict) == shop_keeper) && char_get_admlevel(ch) < 5)
    roll = -10;

  auto obj = find_carried(g, ch, obj_name, "You don't have that to plant on them.\r\n");
  if (reject(g, ch, obj))
    return;
  constexpr const char *TRIES_ON_YOU = "@C$n@w tries to plant $p@w on you!@n", *TRIES_ON_THEM = "@C$n@w tries to plant $p@w on @c$N@w!@n";
  if (roll <= detect && roll <= fail) {
    reveal_hiding(g, ch, 0);
    act(g, TRIES_ON_YOU, true, ch, *obj, vict, TO_VICT);
    act(g, TRIES_ON_THEM, true, ch, *obj, vict, TO_NOTVICT);
    act(g, "@wYou try and fail to plant $p@w on @c$N@w, and $E notices!@n", true, ch, *obj, vict, TO_CHAR);
  } else if (roll <= fail)
    act(g, "@wYou try and fail to plant $p@w on @c$N@w! However no one seemed to notice.@n", true, ch, *obj, vict, TO_CHAR);
  else if (obj_get_weight(*obj) + getCurCarriedWeight(vict) > getMaxCarryWeight(g, vict)) {
    reveal_hiding(g, ch, 0);
    act(g, TRIES_ON_YOU, true, ch, *obj, vict, TO_VICT);
    act(g, TRIES_ON_THEM, true, ch, *obj, vict, TO_NOTVICT);
    act(g, "@wYou try and fail to plant $p@w on @c$N@w because $E can't carry the weight. It seems $E noticed the attempt!@n", true, ch, *obj, vict, TO_CHAR);
  } else {
    if (roll <= detect) {
      act(g, "@cYou feel like the weight of your inventory has changed.@n", true, ch, *obj, vict, TO_VICT);
      act(g, "@c$N@w looks around after feeling $S pockets.@n", true, ch, *obj, vict, TO_NOTVICT);
      act(g, "@wYou plant $p@w on @c$N@w! @c$N @wseems to notice the change in weight in their inventory.@n", true, ch, *obj, vict, TO_CHAR);
    } else
      act(g, "@wYou plant $p@w on @c$N@w! No one noticed, whew....@n", true, ch, *obj, vict, TO_CHAR);
    obj_from_char(g, *obj);
    obj_to_char(g, *obj, vict);
  }
  char_set_wait(ch, PULSE_2SEC);
}

namespace {
  /* Why obj can't be forged, if it can't */
  const char *unforgeable(GameWorld *g, struct obj_data *obj)
  {
    obj_vnum vnum = obj_get_vnum(g, obj);
    if (vnum >= 60000 || vnum == 0)
      return "You can not make a forgery of that! It's far too squishy....";
    if ((vnum >= 18800 && vnum <= 18999) || (vnum >= 19080 && vnum <= 19199) || (vnum >= 4 && vnum <= 6))
      return "You can not make a forgery of that!\r\n";
    if (obj_extra_flagged(obj, ITEM_PROTECTED))
      return "You don't know where to begin with this work of ART.\r\n";
    return nullptr;
  }

  /* The first of ch's kits of this number that's neither broken nor fake */
  struct obj_data *usable_kit(GameWorld *g, struct char_data *ch, obj_vnum vnum)
  {
    return find_first(char_inventory(g, ch), [&](obj_data *obj) {
      return obj_get_vnum(g, obj) == vnum && !obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED);
    });
  }
} // namespace

ACMD(do_forgery)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch) || !know_skill(g, ch, SKILL_FORGERY))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Okay, make a forgery of what?\r\n");
    return;
  }
  auto found = find_carried(g, ch, arg, "You want to make a fake copy of what?\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *original = *found;
  struct obj_data *kit      = usable_kit(g, ch, 19);
  if (!kit) {
    send_to_char(g, ch, "You need a forgery kit.\r\n");
    return;
  }
  if (obj_get_vnum(g, original) == 19) {
    send_to_char(g, ch, "You can't duplicate a forgery kit.\r\n");
    return;
  }
  if (obj_extra_flagged(original, ITEM_FORGED) || obj_extra_flagged(original, ITEM_BROKEN)) {
    send_to_char(g, ch, "{} is {}\r\n", obj_get_short_description(original), obj_extra_flagged(original, ITEM_FORGED) ? "forgery, there is no reason to make a fake of a fake!" : "broken, there is no reason to make a fake of this mess!");
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  if (const char *why = unforgeable(g, original)) {
    send_to_char(g, ch, why);
    return;
  }

  reveal_hiding(g, ch, 0);
  act(g, "@c$n@w looks at $p, begins to work on forging a fake copy of it.@n", true, ch, original, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_FORGERY, 1);
  if (char_get_skill(ch, SKILL_FORGERY) < axion_dice(g, 0)) {
    if (rand_number(g, 1, 10) >= 9) { /* Uh oh */
      send_to_char(g, ch, "In the middle of creating a forgery of {} you screw up. The fabrication unit built into the forgery kit melts and bonds with the original. You clumsy mistake with the Estex Titanium drill has broken both.\r\n", obj_get_short_description(original));
      extract_obj(g, kit);
      extract_obj(g, original);
      return;
    }
    send_to_char(g, ch, "You start to make a forgery of {} but screw up and waste your forgery kit..\r\n", obj_get_short_description(original));
    act(g, "@c$n@w tried to duplicate $p but screws up somehow.@n", true, ch, original, nullptr, TO_ROOM);
    obj_from_char(g, kit);
    extract_obj(g, kit);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  struct obj_data *fake = read_object(g, obj_get_vnum(g, original));
  add_unique_id(g, fake);
  obj_extra_set(fake, ITEM_FORGED, true);
  /* lighter than the real thing; weighed before it's carried, so the carrier counts what it really weighs */
  obj_set_weight(fake, rand_number(g, obj_get_weight(fake) / 2, obj_get_weight(fake)));
  obj_to_char(g, fake, ch);
  obj_from_char(g, kit);
  extract_obj(g, kit);
  send_to_char(g, ch, "You make an excellent forgery of {}@n!\r\n", obj_get_short_description(original));
  act(g, "@c$n@w makes a perfect forgery of $p.@n", true, ch, original, nullptr, TO_ROOM);
  char_set_wait(ch, PULSE_2SEC);
}

namespace {
  constexpr struct {
    int flag;
    const char *line;
  } WEAPON_LEVELS[] = {
      {ITEM_WEAPLVL1, "Weapon Level: 1\nDamage Bonus: 5%\r\n"},  {ITEM_WEAPLVL2, "Weapon Level: 2\nDamage Bonus: 10%\r\n"},
      {ITEM_WEAPLVL3, "Weapon Level: 3\nDamage Bonus: 20%\r\n"}, {ITEM_WEAPLVL4, "Weapon Level: 4\nDamage Bonus: 30%\r\n"},
      {ITEM_WEAPLVL5, "Weapon Level: 5\nDamage Bonus: 50%\r\n"},
  };

  /* How full obj's token slots are, if it has any; quirk: two filled slots show nothing */
  const char *token_slots(struct obj_data *obj)
  {
    bool filled = obj_extra_flagged(obj, ITEM_SLOTS_FILLED);
    if (obj_extra_flagged(obj, ITEM_SLOT1))
      return filled ? "Token Slots  : @m1/1@n\n" : "Token Slots  : @m0/1@n\n";
    if (obj_extra_flagged(obj, ITEM_SLOT2) && !filled)
      return obj_extra_flagged(obj, ITEM_SLOT_ONE) ? "Token Slots  : @m1/2@n\n" : "Token Slots  : @m0/2@n\n";
    return nullptr;
  }

  /* The bonuses obj gives, as appraise lists them after "Bonuses:" */
  void appraise_bonuses(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    bool found = false;
    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
      const auto &aff = obj_get_affected(obj)[i];
      if (!aff.modifier)
        continue;
      bool percent = aff.location == APPLY_REGEN || aff.location == APPLY_TRAIN || aff.location == APPLY_LIFEMAX;
      send_to_char(g, ch, "{} {:+d}{} to {}", found ? "," : "", aff.modifier, percent ? "%" : "", sprinttype(aff.location, apply_types));
      found = true;
      if (aff.location == APPLY_FEAT)
        send_to_char(g, ch, " ({})", g->feat_list[aff.specific].name);
      else if (aff.location == APPLY_SKILL)
        send_to_char(g, ch, " ({})", g->spell_info[aff.specific].name);
    }
    if (!found)
      send_to_char(g, ch, " None");
    send_to_char(g, ch, "\nSpecial: {}\r\n", sprintbitarray(obj_get_bitvector(obj), affected_bits, AF_ARRAY_MAX));
  }
} // namespace

ACMD(do_appraise)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch) || !know_skill(g, ch, SKILL_APPRAISE))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Okay, appraise what?\r\n");
    return;
  }
  auto found = find_carried(g, ch, arg, "You want to appraise what?\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *obj = *found;

  reveal_hiding(g, ch, 0);
  act(g, "@c$n@w looks at $p, turning it over in $s hands.@n", true, ch, obj, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_APPRAISE, 1);
  /* what's said of it instead, either side of its name, and how ch looks */
  const char *before = "", *after = nullptr, *look = nullptr;
  if (char_get_skill(ch, SKILL_APPRAISE) < axion_dice(g, -10)) {
    before = "You fail to perceive the worth of ";
    after  = "..";
    look   = "@c$n@w looks stumped about $p.@n";
  } else if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    after = " is broken!";
    look  = "@c$n@w looks at $p and frowns.@n";
  } else if (obj_extra_flagged(obj, ITEM_FORGED)) {
    after = " is fake and worthless!";
    look  = "@c$n@w looks at $p with an angry face.@n";
  }
  if (after) {
    send_to_char(g, ch, "{}{}{}\r\n", before, obj_get_short_description(obj), after);
    act(g, look, true, ch, obj, nullptr, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  bool weapon   = obj_get_type_flag(obj) == ITEM_WEAPON;
  int displevel = weapon && obj_extra_flagged(obj, ITEM_CUSTOM) ? 20 : obj_get_level(obj);
  send_to_char(g, ch, "{} is worth: {}\r\nMin Lvl: {:d}\r\n", obj_get_short_description(obj), add_commas(obj_get_cost(obj)), displevel);
  if (weapon) {
    auto level = std::ranges::find_if(WEAPON_LEVELS, [&](const auto &l) { return obj_extra_flagged(obj, l.flag); });
    if (level != std::end(WEAPON_LEVELS))
      send_to_char(g, ch, level->line);
  }
  send_to_char(g, ch, "Size: {}\r\n", size_names[obj_get_size(obj)]);
  if (const char *slots = token_slots(obj))
    send_to_char(g, ch, slots);
  send_to_char(g, ch, "Bonuses:");
  act(g, "@c$n@w looks at $p and nods, a satisfied look on $s face.@n", true, ch, obj, nullptr, TO_ROOM);
  appraise_bonuses(g, ch, obj);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_disguise)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You forgot your disguise off in mobland.\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_DISGUISED)) {
    send_to_char(g, ch, "You stop disguising yourself.\r\n");
    char_plr_set(ch, PLR_DISGUISED, false);
    act(g, "@C$n @wpulls off $s disguise and reveals $mself!", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  if (!know_skill(g, ch, SKILL_DISGUISE))
    return;
  if (!char_get_equipment(ch, WEAR_HEAD)) {
    send_to_char(g, ch, "You can't disguise your identity without anything on your head.\r\n");
    return;
  }
  if (getCurST(g, ch) < getCurST(g, ch) / 50) { /* quirk: never so, as it's measured against itself */
    send_to_char(g, ch, "You are too tired to try that right now.\r\n");
    return;
  }
  if (char_get_skill(ch, SKILL_DISGUISE) > axion_dice(g, -10)) {
    send_to_char(g, ch, "You managed to disguise yourself with some skilled manipulation of your headwear.\r\n");
    act(g, "@C$n @wmanages to disguise $mself with some skilled manipulation of $s headwear.", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_DISGUISED, true);
    return;
  }
  send_to_char(g, ch, "You finish attempting to disguise yourself, but realize you failed and need to try again.\r\n");
  act(g, "@C$n @wattempts and fails to disguise $mself properly and must try again.", true, ch, nullptr, nullptr, TO_ROOM);
  decCurST(g, ch, getMaxST(g, ch) / 50, 0);
}

namespace {
  /* use (medical supplies): each of these does its own thing; anything else isn't to be used */
  void use_supply(GameWorld *g, struct char_data *ch, struct obj_data *item)
  {
    switch (obj_get_vnum(g, item)) {
      case 381: /* stamina injector */
        if (getCurST(g, ch) >= getMaxST(g, ch)) {
          send_to_char(g, ch, "Your stamina is full.\r\n");
          return;
        }
        act(g, "@WYou place the $p@W against your chest and feel a rush of stamina as it automatically administers the dose.@n", true, ch, item, nullptr, TO_CHAR);
        act(g, "@C$n@W places an $p@W against $s chest and a loud click is heard.@n", true, ch, item, nullptr, TO_ROOM);
        if (char_get_skill(ch, SKILL_FIRST_AID) > 0) {
          send_to_char(g, ch, "@CYour skill in First Aid has helped increase the use of the injector. You gain more stamina as a result.@n\r\n");
          incCurST(g, ch, getMaxST(g, ch) * .25, true);
        } else
          incCurST(g, ch, getMaxST(g, ch) * .1, true);
        extract_obj(g, item);
        return;
      case 382: /* burn salve */
        if (!char_aff_flagged(ch, AFF_BURNED)) {
          send_to_char(g, ch, "You are not burned.\r\n");
          return;
        }
        act(g, "@WYou gently apply the salve to your burns.@n", true, ch, item, nullptr, TO_CHAR);
        act(g, "@C$n@W gently applies a burn salve to $s burns.@n", true, ch, item, nullptr, TO_ROOM);
        char_aff_set(ch, AFF_BURNED, false);
        extract_obj(g, item);
        return;
      case 383: /* antitoxin */
        if (!char_aff_flagged(ch, AFF_POISON)) {
          send_to_char(g, ch, "You are not poisoned.\r\n");
          return;
        }
        act(g, "@WYou place the $p@W against your neck and feel a rush of relief as the antitoxiin enters your bloodstream.@n", true, ch, item, nullptr, TO_CHAR);
        act(g, "@C$n@W places an $p@W against $s neck and a loud click is heard.@n", true, ch, item, nullptr, TO_ROOM);
        null_affect(g, ch, AFF_POISON);
        extract_obj(g, item);
        return;
      case 385: { /* restorative vial */
        act(g, "@WYou drink the contents of the vial before disposing of it.@n", true, ch, item, nullptr, TO_CHAR);
        act(g, "@C$n@W dinks a $p and then disposes of it.@n", true, ch, item, nullptr, TO_ROOM);
        if (char_aff_flagged(ch, AFF_BLIND)) {
          act(g, "@WYour eyesight has returned!@n", true, ch, item, nullptr, TO_CHAR);
          act(g, "@C$n@W eyesight seems to have returned.@n", true, ch, item, nullptr, TO_ROOM);
          null_affect(g, ch, AFF_BLIND);
        }
        bool refreshed = true;
        if (getCurPL(g, ch) <= getEffMaxPL(g, ch) * 0.99)
          incCurHealth(g, ch, large_rand(g, getEffMaxPL(g, ch) * 0.08, getEffMaxPL(g, ch) * 0.16), true);
        else if (getCurKI(g, ch) <= getEffMaxPL(g, ch) * 0.99) /* quirk: ki measured against powerlevel */
          incCurKI(g, ch, large_rand(g, getMaxKI(g, ch) * 0.08, getMaxKI(g, ch) * 0.16), true);
        else if (getCurST(g, ch) <= getMaxST(g, ch) * 0.99)
          incCurST(g, ch, large_rand(g, getMaxST(g, ch) * 0.08, getMaxST(g, ch) * 0.16), true);
        else
          refreshed = false;
        if (refreshed)
          send_to_char(g, ch, "@CYou feel refreshed!\r\n");
        extract_obj(g, item);
        return;
      }
      default:
        send_to_char(g, ch, "That is not something you can apparently use.\r\n");
        return;
    }
  }
} // namespace

ACMD(do_use)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string buf(skip_spaces(rest));

  if (arg.empty()) {
    send_to_char(g, ch, "What do you want to {}?\r\n", CMD_NAME);
    return;
  }
  if (subcmd != SCMD_RECITE && subcmd != SCMD_QUAFF && subcmd != SCMD_USE) {
    mud_log("SYSERR: Unknown subcmd {:d} passed to do_use.", subcmd);
    return;
  }
  struct obj_data *mag_item = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!mag_item) {
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg.c_str()), arg);
    return;
  }

  const char *refusal = nullptr;
  if (subcmd == SCMD_QUAFF) {
    if (obj_get_type_flag(mag_item) != ITEM_POTION)
      refusal = "You can only swallow beans.\r\n";
    else if (char_is_android(ch))
      refusal = "You can't swallow beans, you are an android.\r\n";
    else if (obj_extra_flagged(mag_item, ITEM_FORGED))
      refusal = "You can't swallow that, it is fake!\r\n";
    else if (obj_extra_flagged(mag_item, ITEM_BROKEN))
      refusal = "You can't swallow that, it is broken!\r\n";
  } else if (subcmd == SCMD_RECITE) {
    if (obj_get_type_flag(mag_item) != ITEM_SCROLL)
      refusal = "You can only recite scrolls.\r\n";
  } else {
    if (char_is_android(ch))
      refusal = "You are not biological enough to use these, Tincan.\r\n";
    else {
      use_supply(g, ch, mag_item);
      return;
    }
  }
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  mag_objectmagic(g, ch, mag_item, buf);
}

/* wimpy, the only value it sets now */
ACMD(do_value)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (subcmd != SCMD_WIMPY) {
    mud_log("Unknown subcmd to do_value {:d} called by {}", subcmd, char_get_name(ch));
    return;
  }
  if (arg.empty()) {
    if (char_get_wimp_level(g, ch))
      send_to_char(g, ch, "Your current wimp level is {:d} powerlevel.\r\n", char_get_wimp_level(g, ch));
    else
      send_to_char(g, ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
    return;
  }
  if (!isdigit(arg[0])) {
    send_to_char(g, ch, "Specify a value.  (0 to disable)\r\n");
    return;
  }
  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (char_is_npc(ch))
    return;
  int value_lev = parse_int(arg);
  if (value_lev == 0) {
    send_to_char(g, ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
    char_set_wimp_level(g, ch, 0);
  } else if (value_lev < 0)
    send_to_char(g, ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
  else if (value_lev > getEffMaxPL(g, ch))
    send_to_char(g, ch, "That doesn't make much sense, now does it?\r\n");
  else if (value_lev > getEffMaxPL(g, ch) * 0.5)
    send_to_char(g, ch, "You can't set your wimp level above half your powerlevel.\r\n");
  else {
    send_to_char(g, ch, "Okay, you'll wimp out if you drop below {:d} powerlevel.\r\n", value_lev);
    char_set_wimp_level(g, ch, value_lev);
  }
}

namespace {
  /* What compare goes by, for the kinds of things it can compare */
  int comparable_worth(struct obj_data *obj)
  {
    if (obj_get_type_flag(obj) == ITEM_ARMOR)
      return obj_get_value(obj, VAL_ARMOR_APPLYAC);
    return (1 + obj_get_value(obj, VAL_WEAPON_DAMSIZE)) * obj_get_value(obj, VAL_WEAPON_DAMDICE);
  }
} // namespace

ACMD(do_compare)
{
  std::string_view rest = argument;
  std::string arg1      = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg1.empty() || arg2.empty()) {
    send_to_char(g, ch, "Compare what to what?\n\r");
    return;
  }
  struct char_data *tchar;
  struct obj_data *obj1, *obj2;
  int o1 = generic_find(g, arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj1);
  int o2 = generic_find(g, arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj2);
  if (!o1 || !o2) {
    send_to_char(g, ch, "You do not have that item.\r\n");
    return;
  }
  const char *msg;
  if (obj1 == obj2)
    msg = "You compare $p to itself.  It looks about the same.";
  else if (obj_get_type_flag(obj1) != obj_get_type_flag(obj2) || (obj_get_type_flag(obj1) != ITEM_ARMOR && obj_get_type_flag(obj1) != ITEM_WEAPON))
    msg = "You can't compare $p and $P.";
  else if (int value1 = comparable_worth(obj1), value2 = comparable_worth(obj2); value1 == value2)
    msg = "$p and $P look about the same.";
  else
    msg = value1 > value2 ? "$p looks better than $P." : "$p looks worse than $P.";
  act(g, msg, false, ch, obj1, obj2, TO_CHAR);
}

ACMD(do_break)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Usually you break SOMETHING.\r\n");
    return;
  }
  struct char_data *dummy = nullptr;
  struct obj_data *obj;
  if (!generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &obj)) {
    send_to_char(g, ch, "Can't seem to find what you want to break!\r\n");
    return;
  }
  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(g, ch, "Seems like it's already broken!\r\n");
    return;
  }
  send_to_char(g, ch, "You ruin {}.\r\n", obj_get_short_description(obj));
  act(g, "$n ruins $p.", false, ch, obj, nullptr, TO_ROOM);
  obj_set_value(obj, VAL_ALL_HEALTH, 0);
  obj_extra_toggle(obj, ITEM_BROKEN);
}

namespace {
  /* Why obj can't be fixed, if it can't */
  const char *unfixable(struct obj_data *obj)
  {
    if (obj_get_value(obj, VAL_ALL_HEALTH) >= 100)
      return "But it isn't even damaged!\r\n";
    if (obj_extra_flagged(obj, ITEM_FORGED))
      return "That is fake, why bother fixing it?\r\n";
    switch (obj_get_value(obj, VAL_ALL_MATERIAL)) {
      case MATERIAL_ORGANIC:
      case MATERIAL_FOOD:
      case MATERIAL_PAPER:
      case MATERIAL_LIQUID:
        return "You can't repair that.\r\n";
    }
    return nullptr;
  }

  /* fix (obj), with a repair kit */
  void fix_thing(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *kit)
  {
    if (char_get_skill(ch, SKILL_REPAIR) < axion_dice(g, 0)) {
      act(g, "You try to repair $p but screw up..", true, ch, obj, nullptr, TO_CHAR);
      act(g, "$n tries to repair $p but screws up..", true, ch, obj, nullptr, TO_ROOM);
      extract_obj(g, kit);
      improve_skill(g, ch, SKILL_REPAIR, 1);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    if (obj_get_value(obj, VAL_ALL_HEALTH) + char_get_skill(ch, SKILL_REPAIR) < 100) {
      send_to_char(g, ch, "You repair {} a bit.\r\n", obj_get_short_description(obj));
      act(g, "$n repairs $p a bit.", false, ch, obj, nullptr, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) + char_get_skill(ch, SKILL_REPAIR));
    } else {
      send_to_char(g, ch, "You repair {} completely.\r\n", obj_get_short_description(obj));
      act(g, "$n repairs $p completely.", false, ch, obj, nullptr, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, 100);
    }
    obj_extra_set(obj, ITEM_BROKEN, false);
    /* Something not carried teaches a little, once, until a lucky fix makes it teach again */
    if (obj_get_carried_by(obj) == nullptr && !char_plr_flagged(ch, PLR_REPLEARN) && (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 || char_get_level(ch) >= 100)) {
      int64_t gain = (level_exp(ch, char_get_level(ch) + 1) * 0.0003) * char_get_skill(ch, SKILL_REPAIR);
      send_to_char(g, ch, "@mYou've learned a bit from repairing it. @D[@gEXP@W: @G+{}@D]@n\r\n", add_commas(gain));
      char_plr_set(ch, PLR_REPLEARN, true);
      gain_exp(g, ch, gain);
    } else if (rand_number(g, 2, 12) >= 10 && char_plr_flagged(ch, PLR_REPLEARN)) {
      char_plr_set(ch, PLR_REPLEARN, false);
      send_to_char(g, ch, "@mYou think you might be on to something...@n\r\n");
    }
    improve_skill(g, ch, SKILL_REPAIR, 1);
    extract_obj(g, kit);
    char_set_wait(ch, PULSE_2SEC);
  }

  /* fix self, an android's body, with a repair kit */
  void fix_self(GameWorld *g, struct char_data *ch, struct obj_data *kit)
  {
    if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
      send_to_char(g, ch, "Your body is already in peak condition.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_REPAIR) < axion_dice(g, 0)) {
      act(g, "You try to repair your body but screw up..", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n tries to repair $s body but screws up..", true, ch, nullptr, nullptr, TO_ROOM);
      extract_obj(g, kit);
      improve_skill(g, ch, SKILL_REPAIR, 1);
      char_set_wait(ch, PULSE_5SEC);
      return;
    }
    act(g, "You use the repair kit to fix part of your body...", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n works on their body with a repair kit.", true, ch, nullptr, nullptr, TO_ROOM);
    int64_t add = (getEffMaxPL(g, ch) * 0.005 + 10) * char_get_skill(ch, SKILL_REPAIR);
    extract_obj(g, kit);
    if (incCurHealth(g, ch, add, true) == getEffMaxPL(g, ch))
      send_to_char(g, ch, "Your body has been totally repaired.\r\n");
    else
      send_to_char(g, ch, "Your body still needs some work done to it.\r\n");
    char_set_wait(ch, PULSE_5SEC);
  }
} // namespace

ACMD(do_fix)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!know_skill(g, ch, SKILL_REPAIR))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Usually you fix SOMETHING.\r\n");
    return;
  }
  bool self = str_eq_nocase("self", arg);
  if (self && !char_is_android(ch)) {
    send_to_char(g, ch, "Only androids can fix their bodies with repair kits.\r\n");
    return;
  }
  struct obj_data *obj = nullptr;
  if (!self) {
    struct char_data *dummy = nullptr;
    if (!generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy, &obj)) {
      send_to_char(g, ch, "Can't seem to find what you want to fix!\r\n");
      return;
    }
    if (const char *why = unfixable(obj)) {
      send_to_char(g, ch, why);
      return;
    }
  }
  /* custom gear takes an orb */
  bool custom           = obj && (obj_get_vnum(g, obj) == 20099 || obj_get_vnum(g, obj) == 20098);
  struct obj_data *kit  = usable_kit(g, ch, custom ? 13593 : 48);
  if (!kit) {
    send_to_char(g, ch, custom ? "You do not even have a Nano-tech Repair Orb.\r\n" : "You do not even have a repair kit.\r\n");
    return;
  }
  if (self)
    fix_self(g, ch, kit);
  else
    fix_thing(g, ch, obj, kit);
}
