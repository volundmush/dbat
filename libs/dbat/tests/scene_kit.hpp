#pragma once
/* What the scenes of test_techniques share: the helpers that set them up, and the scene itself. */
#include <cstdio>
#include <span>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "dbat/act.informative.hpp"
#include "dbat/affect.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/consts/fightprefs.hpp"
#include "dbat/consts/materials.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/prefflags.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/consts/sectortypes.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/relocate.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/senseis.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/db.hpp"
#include "dbat/fight.hpp"
#include "dbat/handler.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.attack.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/random.hpp"
#include "dbat/utils.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "harness.hpp"

constexpr room_vnum ARENA = 301; /* the training yard, with its dummies */

struct scene_cast {
  struct char_data *helper;  /* another attacker, to defend the victim */
  struct char_data *android; /* kept out of the arena until wanted */
  struct obj_data *chest;
};
inline scene_cast cast;

inline void clear_arena(GameWorld *g, struct char_data *a, struct char_data *b, struct char_data *c = nullptr)
{
  for (char_data *who : room_people(g, ARENA))
    if (who != a && who != b && who != c) {
      char_from_room(g, who);
      char_to_room(g, who, 300);
    }
}

/* A sword in att's hands for throwing (steel, slashing, three pounds, at full health), made into what is asked */
inline struct obj_data *missile(GameWorld *g, struct char_data *att, int damtype = TYPE_SLASH - TYPE_HIT,
                                int material = MATERIAL_STEEL, int flag = -1)
{
  struct obj_data *obj = read_object(g, 301);

  obj_set_value(obj, VAL_WEAPON_DAMTYPE, damtype);
  obj_set_value(obj, VAL_ALL_MATERIAL, material);
  if (flag >= 0)
    obj_extra_set(obj, flag, true);
  obj_to_char(g, obj, att);
  return obj;
}

/* att leads helper's group, or follows them */
inline void group(GameWorld *g, struct char_data *leader, struct char_data *follower)
{
  add_follower(g, follower, leader);
  char_aff_set(leader, AFF_GROUP, true);
  char_aff_set(follower, AFF_GROUP, true);
}

/* att wields a sword made into this damage type in pos, in place of whatever was there */
inline struct obj_data *wield(GameWorld *g, struct char_data *att, int damtype, int pos = WEAR_WIELD1, int flag = -1)
{
  if (char_get_equipment(att, pos))
    extract_obj(g, unequip_char(g, att, pos));
  struct obj_data *obj = read_object(g, 301);
  obj_set_value(obj, VAL_WEAPON_DAMTYPE, damtype);
  if (flag >= 0)
    obj_extra_set(obj, flag, true);
  equip_char(g, att, obj, pos);
  for (int skill : {SKILL_SWORD, SKILL_DAGGER, SKILL_CLUB, SKILL_SPEAR, SKILL_GUN, SKILL_BRAWL}) /* to hit with it */
    char_set_skills_base(att, skill, 100);
  return obj;
}

/* ... in the off hand alone */
inline void off_hand(GameWorld *g, struct char_data *att, int damtype)
{
  if (char_get_equipment(att, WEAR_WIELD1))
    extract_obj(g, unequip_char(g, att, WEAR_WIELD1));
  wield(g, att, damtype, WEAR_WIELD2);
}

/* ... and a sword in the main hand, and a mastery of the two */
inline void dual(GameWorld *g, struct char_data *att, int damtype, int mastery = 100)
{
  wield(g, att, TYPE_SLASH - TYPE_HIT);
  wield(g, att, damtype, WEAR_WIELD2);
  char_set_skills_base(att, SKILL_DUALWIELD, mastery);
}

inline void two_handed(struct char_data *att, int mastery)
{
  char_plr_set(att, PLR_THANDW, true);
  char_set_skills_base(att, SKILL_TWOHAND, mastery);
}

inline void defends(struct char_data *victim)
{
  char_set_skills_base(victim, SKILL_BLOCK, 100);
  char_set_skills_base(victim, SKILL_DODGE, 100);
  char_set_skills_base(victim, SKILL_PARRY, 100);
}

/* att knows every focus at this level */
inline void focuses(struct char_data *att, int level)
{
  for (int skill : {SKILL_TSKIN, SKILL_MIGHT, SKILL_WITHER, SKILL_ENLIGHTEN, SKILL_GENIUS, SKILL_FLEX, SKILL_BLESS,
                    SKILL_CURSE, SKILL_YOIK, SKILL_VIGOR, SKILL_CURE, SKILL_POISON})
    char_set_skills_base(att, skill, level);
}

inline void drained(GameWorld *g, struct char_data *ch)
{
  decCurKI(g, ch, getMaxKI(g, ch), 0);
}

/* att knows how to create things, at this level, with this much concentration */
inline void creates(GameWorld *g, struct char_data *att, int skill, int concentration = 0, int practices = 0)
{
  char_set_skills_base(att, SKILL_CREATE, skill);
  char_set_skills_base(att, SKILL_CONCENTRATION, concentration);
  char_set_practice_points(g, att, practices);
}

/* att trains at level 70 with every stat at 30, carrying 1 in gravity that makes the training about this share of
   their power level */
inline void trains(GameWorld *g, struct char_data *att, double share = 0.3)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_weight(obj, 1);
  obj_to_char(g, obj, att);
  char_set_class_level(att, 70);
  for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel,
                   char_set_real_abils_wis, char_set_real_abils_dex})
    set(att, 30);
  room_set_gravity(g, char_get_in_room(att), (int)(getEffMaxPL(g, att) * share));
}

/* ... with their sensei (an assassin's is in the town square), who is paid */
inline void with_sensei(GameWorld *g, struct char_data *att, bool paid = true)
{
  char_set_class(att, CLASS_ASSASSIN);
  char_from_room(g, att);
  char_to_room(g, att, 300);
  char_set_gold(att, paid ? 8 : 7);
  char_set_practice_points(g, att, 1);
}

/* att exercises at level 50 in this gravity, with this much more stamina, ki and power */
inline void exercises(GameWorld *g, struct char_data *att, int gravity, int64_t vigor = 0)
{
  char_set_class_level(att, 50);
  room_set_gravity(g, char_get_in_room(att), gravity);
  gainBaseST(att, vigor, true);
  gainBaseKI(att, vigor, true);
  gainBasePL(att, vigor, true);
  restoreVitals(att, false);
}

/* ... sitting by item70, of this weight, to meditate on */
inline struct obj_data *to_lift(GameWorld *g, struct char_data *att, int64_t weight = 1)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_weight(obj, weight);
  obj_to_room(g, obj, char_get_in_room(att));
  char_set_position(att, POS_SITTING);
  return obj;
}

/* att is under a lasting effect of the skill of this name */
inline void lasting(GameWorld *g, struct char_data *att, const char *skill)
{
  for (int i = 0; i < SKILL_TABLE_SIZE; i++)
    if (g->spell_info[i].name && !strcasecmp(skill_name(g, i), skill)) {
      assign_affect(g, att, AFF_INFUSE, i, 5, 0, 0, 0, 0, 0, 0);
      return;
    }
}

/* a scene's setup, with what it doesn't use put aside */
/* ... with Shenron summoned where they stand, played by the victim */
inline void before_shenron(GameWorld *g, struct char_data *att, struct char_data *dragon, int wishes = 0)
{
  g->SHENRON      = true;
  g->DRAGONR      = char_get_in_room(att);
  g->EDRAGON_uid  = char_get_id(dragon);
  g->WISH[0]      = wishes > 0;
  g->WISH[1]      = wishes > 1;
}

/* ... and every player there dead, but `alive` */
inline void the_dead(GameWorld *g, struct char_data *att, const char *alive = "")
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (!char_is_npc(c))
      char_aff_set(c, AFF_SPIRIT, strcmp(char_get_name(c), alive) != 0);
}

/* ... with the first other player there snooping them, and `then` done to that snooper */
template <class Then>
inline void snooped(GameWorld *g, struct char_data *att, Then then)
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (c != att && char_get_desc(c) && char_get_desc(att) && !char_get_desc(att)->snoop_by) {
      char_get_desc(c)->snooping   = char_get_desc(att);
      char_get_desc(att)->snoop_by = char_get_desc(c);
      then(c);
    }
}

/* ... with an exit from room 301 in `dir` to `to` */
inline void exit_to(GameWorld *g, int dir, room_vnum to, const char *description = "")
{
  auto *exit                = new room_direction_data();
  exit->to_room             = to;
  exit->general_description = description;
  room_set_dir_option(g, ARENA, dir, exit);
}

/* ... carrying something standing in for gardening gear (or a plant) of this vnum */
/* Gives obj the vnum of something the fixture world doesn't have, with an index and a bare prototype made for it. */
inline void renumber(GameWorld *g, struct obj_data *obj, obj_vnum vnum)
{
  struct index_data *index = g->obj_index.find(vnum);
  if (!index) {
    index       = g->obj_index.emplace(vnum);
    index->vnum = vnum;
    obj_set_item_number(g->obj_proto.emplace(vnum), vnum);
  }
  obj_index_of(g, obj_get_item_number(obj))->number--;
  index->number++;
  obj_set_item_number(obj, vnum);
}

inline struct obj_data *carries_as(GameWorld *g, struct char_data *att, obj_vnum vnum, const char *name = "item70 tool")
{
  struct obj_data *obj = read_object(g, 70);
  renumber(g, obj, vnum);
  obj_set_name(obj, name);
  obj_set_weight(obj, 1);
  obj_to_char(g, obj, att);
  return obj;
}

/* ... a gardener of this skill, in a garden of the arena, by a seedling of this vnum planted there */
inline struct obj_data *gardening(GameWorld *g, struct char_data *att, int skill, obj_vnum seedling = 250)
{
  char_set_skills_base(att, SKILL_GARDENING, skill);
  room_set_flag(g, ARENA, ROOM_GARDEN1, true);
  if (seedling < 0)
    return nullptr;
  struct obj_data *plant = read_object(g, 70);
  renumber(g, plant, seedling);
  obj_set_name(plant, "seedling");
  obj_set_type_flag(plant, ITEM_PLANT);
  obj_to_room(g, plant, ARENA);
  return plant;
}

/* ... with `count` more plants growing there */
inline void planters(GameWorld *g, int count)
{
  for (int i = 0; i < count; i++) {
    struct obj_data *plant = read_object(g, 70);
    obj_set_type_flag(plant, ITEM_PLANT);
    obj_to_room(g, plant, ARENA);
  }
}

/* ... with a toplist file of this many places, or none */
inline void toplist_of(int places)
{
  remove("intro/P-T/toplist.itr");
  if (places < 0)
    return;
  FILE *fl = fopen("intro/P-T/toplist.itr", "w");
  for (int x = 0; fl && x < places; x++)
    fprintf(fl, "Name%d %d\n", x, (x + 1) * 100);
  if (fl)
    fclose(fl);
}

/* ... another player there, by name */
inline struct char_data *player_named(GameWorld *g, struct char_data *att, const char *name)
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (!char_is_npc(c) && !strcmp(char_get_name(c), name))
      return c;
  return nullptr;
}

/* ... and everyone else there, players and not */
template <class Each>
inline void others(GameWorld *g, struct char_data *att, Each each)
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (c != att)
      each(c);
}

/* ... holding item70 under another name, of another type */
inline struct obj_data *holding(GameWorld *g, struct char_data *att, const char *name, int type = -1)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_name(obj, name);
  obj_set_weight(obj, 1);
  if (type >= 0)
    obj_set_type_flag(obj, type);
  obj_to_char(g, obj, att);
  return obj;
}

/* ... sitting at an official duel table (vnum 604), its own table (600) in the room, with a case of cards */
inline struct obj_data *at_duel_table(GameWorld *g, struct char_data *att, int cards)
{
  struct obj_data *seat = read_object(g, 70);
  renumber(g, seat, 604);
  obj_to_room(g, seat, char_get_in_room(att));
  char_set_sits(att, seat);
  obj_set_sitting(seat, att);
  char_set_position(att, POS_SITTING);
  struct obj_data *table = read_object(g, 70);
  renumber(g, table, 600);
  obj_set_name(table, "table");
  obj_to_room(g, table, char_get_in_room(att));
  struct obj_data *box = holding(g, att, "case");
  for (int i = 0; i < cards; i++) {
    struct obj_data *card = read_object(g, 70);
    obj_set_name(card, "card");
    obj_set_weight(card, 0);
    obj_extra_set(card, ITEM_ANTI_HIEROPHANT, true);
    obj_to_obj(g, card, box);
  }
  return table;
}

/* ... with a help table of five entries, the first "fixture" one for level 5 */
inline void restricted_help_table(GameWorld *g)
{
  g->help_table = {{"QQQ", "q\n", 0, 5}};
}

inline void longer_help_table(GameWorld *g)
{
  g->help_table = {{"AAA", "a\n", 0, 0}, {"FIXTURE", "f\n", 0, 5}, {"FIXTUREB", "fb\n", 0, 0}, {"FIXTUREC", "fc\n", 0, 0}, {"ZZZ", "z\n", 0, 0}};
}

/* ... with a room in every direction from the arena, of this sector and ground effect, through exits like so */
inline void surrounded(GameWorld *g, int sect, int geffect = 0, int exit_info = 0, const char *keyword = "door")
{
  static constexpr room_vnum around[NUM_OF_DIRS] = {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86};
  for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
    exit_to(g, dir, around[dir]);
    struct room_direction_data *exit = room_get_dir_option(g, ARENA, dir);
    exit->exit_info                  = exit_info;
    exit->keyword                    = keyword;
    room_set_sector_type(g, around[dir], sect);
    room_set_geffect(g, around[dir], geffect);
    /* and onward, for scanning */
    auto *far    = new room_direction_data();
    far->to_room = 300;
    room_set_dir_option(g, around[dir], dir, far);
  }
}

/* ... and a thing in the room by that name, of this type, with these values */
inline struct obj_data *thing_here(GameWorld *g, const char *name, int type, std::initializer_list<std::pair<int, int>> values = {})
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_name(obj, name);
  obj_to_room(g, obj, ARENA);
  obj_set_type_flag(obj, type);
  for (auto [i, v] : values)
    obj_set_value(obj, i, v);
  return obj;
}

inline void described(ex_desc_list *list, const char *keyword) { list->push_back({keyword, "You see it closely.\r\n"}); }

/* ... a scouter on ch's eye (item70), flagged as asked and tuned to freq */
inline struct obj_data *scouter_on(GameWorld *g, struct char_data *ch, int flag = -1, int freq = 0)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_name(obj, "scouter");
  if (flag >= 0)
    obj_extra_set(obj, flag, true);
  obj_set_scoutfreq(obj, freq);
  equip_char(g, ch, obj, WEAR_EYE);
  return obj;
}

/* ... moved to room 300, by the arena, wearing a scouter on freq unless it's -1 */
inline void away(GameWorld *g, struct char_data *ch, int freq = -1)
{
  char_from_room(g, ch);
  char_to_room(g, ch, 300);
  if (freq >= 0)
    scouter_on(g, ch, -1, freq);
}

/* ... a telepath, the victim away by the arena unless asked */
inline void telepath(GameWorld *g, struct char_data *att, struct char_data *victim, bool victim_away = true)
{
  char_set_skills_base(att, SKILL_TELEPATHY, 100);
  if (victim_away)
    away(g, victim);
}

/* ... with the victim at sixteenths/16 of their stamina */
inline void tired(GameWorld *g, struct char_data *victim, int sixteenths)
{
  decCurST(g, victim, getCurST(g, victim) - getMaxST(g, victim) * sixteenths / 16, 0);
}

/* ... the victim tied up with someone: grappling, grappled, absorbing or absorbed by them, with grap as the hold */
inline void grappled_by(struct char_data *victim, struct char_data *by, int grap = 1)
{
  char_set_grappled(victim, by);
  char_set_grappling(by, victim);
  char_set_grap(victim, grap);
  char_set_grap(by, grap);
}

inline void absorbed_by(struct char_data *victim, struct char_data *by)
{
  char_set_absorbby(victim, by);
  char_set_absorbing(by, victim);
}

/* ... ch with this much more power than usual */
inline void empowered(struct char_data *ch, int64_t pl)
{
  char_set_basepl(ch, pl);
  restoreVitals(ch, false);
}

/* ... an android of the repair model */
inline void repair_model(struct char_data *att)
{
  char_set_race(att, RACE_ANDROID);
  char_plr_set(att, PLR_REPAIR, true);
}

/* ... carrying item70 as an item of another number, named so */
inline struct obj_data *holding_as(GameWorld *g, struct char_data *att, const char *name, obj_vnum vnum, int type = -1)
{
  struct obj_data *obj = holding(g, att, name, type);
  renumber(g, obj, vnum);
  return obj;
}

/* ... wearing item70, named so, where asked */
inline struct obj_data *wearing(GameWorld *g, struct char_data *att, const char *name, int where, int type = -1)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_name(obj, name);
  if (type >= 0)
    obj_set_type_flag(obj, type);
  equip_char(g, att, obj, where);
  return obj;
}

/* ... and enchanted, with affects of every kind appraise names differently */
inline void enchanted(struct obj_data *obj)
{
  constexpr std::pair<int, int> affects[] = {{APPLY_STR, 2}, {APPLY_REGEN, 5}, {APPLY_SKILL, 3}, {APPLY_FEAT, 1}, {APPLY_TRAIN, -2}, {APPLY_LIFEMAX, 4}};
  for (int i = 0; i < 6; i++) {
    obj_get_affected(obj)[i].location = affects[i].first;
    obj_get_affected(obj)[i].modifier = affects[i].second;
  }
  obj_get_affected(obj)[2].specific = SKILL_PUNCH;
  obj_aff_set(obj, AFF_INFRAVISION, true);
}

/* ... with the medical supplies a construction kit makes in the world, which the fixture lacks */
inline void medical_stock(GameWorld *g)
{
  for (obj_vnum vnum : {381, 382, 383, 385}) {
    struct obj_data *spare = read_object(g, 70);
    renumber(g, spare, vnum);
    extract_obj(g, spare);
  }
}

/* ... carrying dragon balls: those of these stars, forged where asked */
inline void dragon_balls(GameWorld *g, struct char_data *att, std::initializer_list<int> stars, int forged = 0)
{
  for (int star : stars) {
    struct obj_data *ball = holding_as(g, att, "ball", 19 + star);
    if (star == forged)
      obj_extra_set(ball, ITEM_FORGED, true);
  }
}

#define SETUP(body) [](GameWorld *g, char_data *att, char_data *victim) { (void)g; (void)att; (void)victim; body }

inline void fireshielded(struct char_data *victim)
{
  char_aff_set(victim, AFF_FIRESHIELD, true);
  char_set_skills_base(victim, SKILL_FIRESHIELD, 80);
}

inline void throw_ready(struct char_data *att)
{
  char_set_skills_base(att, SKILL_THROW, 100);
}

/*
 * The commands the grid can't set up, each in the ways it needs: with something to throw, in a group, ready to
 * self destruct. Each is tried by every attacker, with a few seeds; cast.helper is another attacker.
 */
struct scene {
  const char *name;
  const char *line; /* what the attacker types; "namek" is the victim */
  bool fighting;    /* fighting the dummy when it starts */
  void (*setup)(GameWorld *g, struct char_data *att, struct char_data *victim);
  bool by_mob;      /* a critter makes it in the attacker's place, by do_attack() (mobs only fight so) */
  int seeds;        /* if not SCENE_SEEDS */
};

/* The scenes, a file of them for each part of the commands, in the order they run */
extern const std::span<const scene> SCENES_OFFENSE;
extern const std::span<const scene> SCENES_DISPATCHERS;
extern const std::span<const scene> SCENES_INFORMATIVE;
extern const std::span<const scene> SCENES_ITEMS_WIZARD;
extern const std::span<const scene> SCENES_OTHER;
extern const std::span<const scene> SCENES_MISC;
