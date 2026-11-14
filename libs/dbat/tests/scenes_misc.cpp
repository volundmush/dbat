/* Scenes for test_techniques: see scene_kit.hpp */
#include "scene_kit.hpp"

#include "dbat/act.misc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/consts/fish.hpp"
#include "dbat/consts/recipes.hpp"
#include "dbat/consts/songs.hpp"

namespace {
  /* ... knowing mystic music this well, carrying an ocarina of this vnum unless it's 0 */
  void musician(GameWorld *g, struct char_data *att, int skill = 100, obj_vnum ocarina = 8807)
  {
    char_set_skills_base(att, SKILL_MYSTICMUSIC, skill);
    if (ocarina)
      carries_as(g, att, ocarina, "ocarina");
  }

  /* ... playing this song, a verse of which the pulse plays before the command */
  void playing(GameWorld *g, struct char_data *att, int song)
  {
    char_set_powerattack(att, song);
    handle_songs(g);
  }

  /* ... the victim hurt in every limb, the right arm (whose state the left leg goes by) as asked */
  void limbs_hurt(struct char_data *victim, int right_arm = 95)
  {
    char_set_limb_condition(victim, 0, right_arm);
    char_set_limb_condition(victim, 1, 50);
    char_set_limb_condition(victim, 2, 95);
    char_set_limb_condition(victim, 3, 95);
  }

  /* ... with prototypes for these vnums, which the fixture lacks */
  void stock(GameWorld *g, std::initializer_list<obj_vnum> vnums)
  {
    for (obj_vnum vnum : vnums) {
      if (g->obj_index.find(vnum))
        continue;
      struct obj_data *spare = read_object(g, 70);
      renumber(g, spare, vnum);
      extract_obj(g, spare);
    }
  }

  /* ... with a mob prototype of this vnum, copied from the critter's */
  void mob_stock(GameWorld *g, mob_vnum vnum)
  {
    if (mob_proto_of(g, vnum))
      return;
    struct index_data *index = g->mob_index.emplace(vnum);
    index->vnum              = vnum;
    struct char_data *proto  = g->mob_proto.emplace(vnum);
    *proto                   = *mob_proto_of(g, 310);
    char_set_nr(proto, vnum);
  }

  /* ... with a bare room of this vnum, in the arena's zone */
  void room_stock(GameWorld *g, room_vnum vnum)
  {
    if (g->rooms.find(vnum))
      return;
    struct room_data *room = g->rooms.emplace(vnum);
    room_set_number(room, vnum);
    room_set_zone(room, room_get_zone(g, ARENA));
    room_set_name(room, "A built room");
  }

  void moved(GameWorld *g, struct char_data *ch, room_vnum to)
  {
    room_stock(g, to);
    char_from_room(g, ch);
    char_to_room(g, ch, to);
  }

  /* ... a token of this stat, flagged as asked */
  struct obj_data *token(GameWorld *g, struct char_data *att, int stat = APPLY_STR, int flag = ITEM_TOKEN)
  {
    struct obj_data *obj = holding(g, att, "token");
    if (flag >= 0)
      obj_extra_set(obj, flag, true);
    obj_get_affected(obj)[0].location = stat;
    obj_get_affected(obj)[0].modifier = 3;
    return obj;
  }

  /* ... a ring with slots so, and its affects at these locations */
  struct obj_data *ring(GameWorld *g, struct char_data *att, std::initializer_list<int> slots, std::initializer_list<int> stats = {})
  {
    struct obj_data *obj = holding(g, att, "ring");
    obj_wear_set(obj, ITEM_WEAR_FINGER, true);
    for (int slot : slots)
      obj_extra_set(obj, slot, true);
    int i = 0;
    for (int stat : stats) {
      obj_get_affected(obj)[i].location = stat;
      obj_get_affected(obj)[i].modifier = 1;
      i++;
    }
    return obj;
  }

  /* ... carrying as much weight as can be carried */
  void laden(GameWorld *g, struct char_data *att)
  {
    struct obj_data *rock = read_object(g, 70);
    obj_set_name(rock, "boulder");
    obj_set_weight(rock, getMaxCarryWeight(g, att) - char_get_carry_weight(att));
    obj_to_char(g, rock, att);
  }

  /* ... carrying as many things as can be carried */
  void crowded(GameWorld *g, struct char_data *att)
  {
    while (char_get_carry_items(att) < char_can_carry_n(att))
      holding(g, att, "pebble");
  }

  /* ... sitting on the chest */
  void seated(struct char_data *att)
  {
    char_set_sits(att, cast.chest);
    obj_set_sitting(cast.chest, att);
  }

  /* ... with this much more ki than usual */
  void spirited(struct char_data *ch, int64_t ki)
  {
    char_set_baseki(ch, ki);
    restoreVitals(ch, false);
  }

  void named(struct char_data *att, const char *name) { char_set_pc_name(att, name); }

  /* ... a bundle of silk of this vnum */
  struct obj_data *silk(GameWorld *g, struct char_data *att, obj_vnum vnum = 16703, bool forged = false)
  {
    struct obj_data *obj = read_object(g, vnum);
    if (forged)
      obj_extra_set(obj, ITEM_FORGED, true);
    obj_to_char(g, obj, att);
    return obj;
  }

  /* ... fishing at the arena, of this water, with a pole baited so */
  struct obj_data *angler(GameWorld *g, struct char_data *att, int bait = 5, bool fresh = true, int planet = ROOM_EARTH)
  {
    room_set_flag(g, ARENA, ROOM_FISHING, true);
    if (fresh)
      room_set_flag(g, ARENA, ROOM_FISHFRESH, true);
    if (planet >= 0)
      room_set_flag(g, ARENA, planet, true);
    struct obj_data *pole = wearing(g, att, "pole", WEAR_WIELD2, ITEM_FISHPOLE);
    obj_set_value(pole, 0, bait);
    stock(g, {1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015});
    return pole;
  }

  /* ... cast already, the fish so, and a pulse of fishing played */
  void fishing(GameWorld *g, struct char_data *att, int state, int distance, int accuracy = 50)
  {
    char_plr_set(att, PLR_FISHING, true);
    char_set_fishstate(att, state);
    char_set_accuracy_mod(att, distance);
    char_set_accuracy(att, accuracy);
    fish_update(g);
  }

  /* ... a runic writer with a brush and a bottle of this much ink */
  void runic(GameWorld *g, struct char_data *att, int ink = 24, int skill = 100)
  {
    char_set_skills_base(att, SKILL_RUNIC, skill);
    carries_as(g, att, 3427, "brush");
    if (ink >= 0)
      obj_set_value(holding_as(g, att, "bottle", 3424), 6, ink);
  }

  /* ... a cook by a campfire, or a stove, with the ingredients of these vnums and the meals in stock */
  void cook(GameWorld *g, struct char_data *att, std::initializer_list<obj_vnum> ingredients, bool stove = false, int skill = 100)
  {
    char_set_skills_base(att, SKILL_COOKING, skill);
    if (stove) {
      struct obj_data *fire = read_object(g, 70);
      renumber(g, fire, 19093);
      obj_to_room(g, fire, ARENA);
    } else {
      thing_here(g, "campfire", ITEM_CAMPFIRE);
    }
    for (obj_vnum vnum : ingredients)
      carries_as(g, att, vnum, "ingredient");
    stock(g, {1220, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235, 1236, 1237});
  }

  /* ... dressed as a demon, at a lava flow, with a blood ruby */
  void ruby_ready(GameWorld *g, struct char_data *att, bool hot = false)
  {
    char_set_race(att, RACE_DEMON);
    char_set_skills_base(att, SKILL_STYLE, 100);
    room_set_geffect(g, ARENA, 3);
    struct obj_data *ruby = holding_as(g, att, "ruby", 6600);
    if (hot)
      obj_extra_set(ruby, ITEM_HOT, true);
  }

  void tsuna(GameWorld *g, struct char_data *att, int style = 100)
  {
    char_set_class(att, CLASS_TSUNA);
    char_set_skills_base(att, SKILL_STYLE, style);
    room_set_sector_type(g, ARENA, SECT_WATER_SWIM);
    stock(g, {19056, 19057, 19058});
  }

  void at_sea(GameWorld *g, struct char_data *att, room_vnum where = 4600)
  {
    char_set_skills_base(att, SKILL_WARP, 100);
    moved(g, att, where);
  }

  void ashes(GameWorld *g, struct char_data *att)
  {
    char_set_race(att, RACE_DEMON);
    char_set_aff_abils_intel(att, 100);
    spirited(att, 10000000);
    holding_as(g, att, "ash", 1305);
  }

  const scene list[] = {
    /* misc/songs.cpp: song, and the verses the pulse plays */
    {"unskilled", "song", false, SETUP(), false, 1},
    {"a musician without an instrument", "song", false, SETUP(musician(g, att, 100, 0);), false, 1},
    {"a musician", "song", false, SETUP(musician(g, att);), false, 1},
    {"a beginner", "song", false, SETUP(musician(g, att, 30);), false, 1},
    {"a middling musician", "song", false, SETUP(musician(g, att, 60);), false, 1},
    {"an able musician", "song", false, SETUP(musician(g, att, 85);), false, 1},
    {"a musician", "song nonsense", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song safety", false, SETUP(musician(g, att);), false, 1},
    {"a musician with the great ocarina", "song safety", false, SETUP(musician(g, att, 100, 8802);), false, 1},
    {"a drained musician", "song safety", false, SETUP(musician(g, att); drained(g, att);), false, 1},
    {"a musician", "song shadow", false, SETUP(musician(g, att);), false, 1},
    {"a beginner", "song shadow", false, SETUP(musician(g, att, 30);), false, 1},
    {"a drained musician", "song shadow", false, SETUP(musician(g, att); drained(g, att);), false, 1},
    {"a musician", "song shielding", false, SETUP(musician(g, att);), false, 1},
    {"an able musician", "song shielding", false, SETUP(musician(g, att, 85);), false, 1},
    {"a drained musician", "song shielding", false, SETUP(musician(g, att); drained(g, att);), false, 1},
    {"a musician", "song teleport", false, SETUP(musician(g, att);), false, 1},
    {"a middling musician", "song teleport earth", false, SETUP(musician(g, att, 60);), false, 1},
    {"a drained musician", "song teleport earth", false, SETUP(musician(g, att); drained(g, att);), false, 1},
    {"a dead musician", "song teleport earth", false, SETUP(musician(g, att); char_aff_set(att, AFF_SPIRIT, true);), false, 1},
    {"a musician", "song teleport mars", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport earth", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport frigid", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport vegeta", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport namek", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport arlia", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport kanassa", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport konack", false, SETUP(musician(g, att);), false, 1},
    {"a musician", "song teleport aether", false, SETUP(musician(g, att);), false, 1},
    {"a musician playing", "song", false, SETUP(musician(g, att); char_set_powerattack(att, SONG_SAFETY);), false, 1},
    {"playing, the instrument lost", "song", false, SETUP(musician(g, att, 100, 0); playing(g, att, SONG_SAFETY);), false, 1},
    {"a beginner playing", "song", false, SETUP(musician(g, att, 10); playing(g, att, SONG_SAFETY);), false, 2},
    {"playing the Song of Safety alone", "song", false, SETUP(musician(g, att); limbs_hurt(att); playing(g, att, SONG_SAFETY);), false, 2},
    {"playing the Song of Safety to the group", "song", false, SETUP(musician(g, att); group(g, att, victim); limbs_hurt(victim); playing(g, att, SONG_SAFETY);), false, 2},
    {"playing the Song of Safety, the right arm whole", "song", false, SETUP(musician(g, att); group(g, att, victim); limbs_hurt(victim, 100); playing(g, att, SONG_SAFETY);), false, 1},
    {"playing the Song of Safety, the right arm badly hurt", "song", false, SETUP(musician(g, att); group(g, att, victim); limbs_hurt(victim, 50); char_set_limb_condition(victim, 1, 95); playing(g, att, SONG_SAFETY);), false, 1},
    {"playing the Song of Safety to a leader", "song", false, SETUP(musician(g, att); group(g, victim, att); playing(g, att, SONG_SAFETY);), false, 1},
    {"playing the Song of Safety, drained", "song", false, SETUP(musician(g, att); group(g, att, victim); drained(g, att); playing(g, att, SONG_SAFETY);), false, 1},
    {"playing the Shadow Stitch Minuet", "song", false, SETUP(musician(g, att); playing(g, att, SONG_SHADOW_STITCH);), false, 2},
    {"playing the Shadow Stitch Minuet among followers", "song", false, SETUP(musician(g, att); group(g, cast.helper, att); group(g, cast.helper, victim); playing(g, att, SONG_SHADOW_STITCH);), false, 2},
    {"playing the Shadow Stitch Minuet, following ungrouped", "song", false, SETUP(musician(g, att); add_follower(g, att, cast.helper); add_follower(g, victim, cast.helper); playing(g, att, SONG_SHADOW_STITCH);), false, 1},
    {"playing the Shadow Stitch Minuet, grouped apart", "song", false, SETUP(musician(g, att); group(g, cast.helper, att); group(g, victim, cast.android); playing(g, att, SONG_SHADOW_STITCH);), false, 1},
    {"playing the Shadow Stitch Minuet, following another group", "song", false, SETUP(musician(g, att); group(g, cast.helper, att); group(g, cast.android, victim); playing(g, att, SONG_SHADOW_STITCH);), false, 2},
    {"playing the Shadow Stitch Minuet, drained", "song", false, SETUP(musician(g, att); drained(g, att); playing(g, att, SONG_SHADOW_STITCH);), false, 1},
    {"playing the Song of Shielding", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_SHIELDING);), false, 2},
    {"playing the Song of Shielding, the victim shielded", "song", false, SETUP(musician(g, att); group(g, att, victim); char_set_barrier(victim, getMaxKI(g, victim)); char_aff_set(victim, AFF_SANCTUARY, true); playing(g, att, SONG_SHIELDING);), false, 1},
    {"playing the Song of Shielding, drained", "song", false, SETUP(musician(g, att); group(g, att, victim); drained(g, att); playing(g, att, SONG_SHIELDING);), false, 1},
    {"playing the melody to Earth", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_EARTH);), false, 2},
    {"playing the melody to Vegeta", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_VEGETA);), false, 1},
    {"playing the melody to Frigid", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_FRIGID);), false, 1},
    {"playing the melody to Konack", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_KONACK);), false, 1},
    {"playing the melody to Namek", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_NAMEK);), false, 1},
    {"playing the melody to Arlia", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_ARLIA);), false, 1},
    {"playing the melody to Aether", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_AETHER);), false, 1},
    {"playing the melody to Kanassa", "song", false, SETUP(musician(g, att); group(g, att, victim); playing(g, att, SONG_TELEPORT_KANASSA);), false, 1},
    {"playing the melody to Earth, ungrouped", "song", false, SETUP(musician(g, att); playing(g, att, SONG_TELEPORT_EARTH);), false, 1},
    {"playing an unknown song", "song", false, SETUP(musician(g, att); playing(g, att, 12);), false, 1},

    /* misc/rpp.cpp: preference, the RPP store and feature, instill, arena */
    {"choosing", "preference", false, SETUP(), false, 1},
    {"choosing", "preference throw", false, SETUP(), false, 1},
    {"choosing, a thrower", "preference throw", false, SETUP(char_set_skills_base(att, SKILL_THROW, 95);), false, 1},
    {"choosing, a master thrower", "preference throw", false, SETUP(char_set_skills_base(att, SKILL_THROW, 100);), false, 1},
    {"choosing", "preference hand", false, SETUP(), false, 1},
    {"choosing", "preference ki", false, SETUP(), false, 1},
    {"choosing", "preference weapon", false, SETUP(), false, 1},
    {"choosing", "preference magic", false, SETUP(), false, 1},
    {"chosen already", "preference ki", false, SETUP(char_set_preference(att, PREFERENCE_H2H);), false, 1},
    {"rich, at level 50", "rpp 12 1", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); stock(g, {1110, 1111, 1112, 1113, 1114, 1115, 1116, 1117, 1118, 1119});), false, 1},
    {"rich", "rpp 12 2", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50);), false, 1},
    {"rich", "rpp 12 3", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50);), false, 1},
    {"rich", "rpp 12 4", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50);), false, 1},
    {"rich", "rpp 12 5", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50);), false, 1},
    {"rich", "rpp 12 6", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50);), false, 1},
    {"rich", "rpp 12 7", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50);), false, 1},
    {"rich", "rpp 12 8", false, SETUP(char_set_rp(att, 10000);), false, 1},
    {"rich and mighty", "rpp 12 8", false, SETUP(char_set_rp(att, 10000); empowered(att, 100000000000LL);), false, 1},
    {"rich", "rpp 12 9", false, SETUP(char_set_rp(att, 10000);), false, 1},
    {"rich", "rpp 12 10", false, SETUP(char_set_rp(att, 10000);), false, 1},
    {"poor", "rpp 12 9", false, SETUP(char_set_rp(att, 3);), false, 1},
    {"rich, at level 30", "rpp 12 1", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, at level 30", "rpp 12 4", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, laden", "rpp 12 1", false, SETUP(char_set_rp(att, 10000); laden(g, att);), false, 1},
    {"rich, laden", "rpp 12 9", false, SETUP(char_set_rp(att, 10000); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 1", false, SETUP(char_set_rp(att, 10000); crowded(g, att);), false, 1},
    {"rich, crowded", "rpp 12 5", false, SETUP(char_set_rp(att, 10000); crowded(g, att);), false, 1},
    {"rich, crowded", "rpp 12 6", false, SETUP(char_set_rp(att, 10000); crowded(g, att);), false, 1},
    {"rich, crowded", "rpp 12 8", false, SETUP(char_set_rp(att, 10000); crowded(g, att); empowered(att, 100000000000LL);), false, 1},
    {"rich, crowded", "rpp 12 9", false, SETUP(char_set_rp(att, 10000); crowded(g, att);), false, 1},
    {"rich, laden", "rpp 12 2", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 2", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); crowded(g, att);), false, 1},
    {"rich, at level 30", "rpp 12 2", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, laden", "rpp 12 3", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 3", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); crowded(g, att);), false, 1},
    {"rich, at level 30", "rpp 12 3", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, laden", "rpp 12 4", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 4", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); crowded(g, att);), false, 1},
    {"rich, at level 30", "rpp 12 4", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, laden", "rpp 12 5", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 5", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); crowded(g, att);), false, 1},
    {"rich, at level 30", "rpp 12 5", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, laden", "rpp 12 6", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 6", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); crowded(g, att);), false, 1},
    {"rich, at level 30", "rpp 12 6", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"rich, laden", "rpp 12 7", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); laden(g, att);), false, 1},
    {"rich, crowded", "rpp 12 7", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 50); crowded(g, att);), false, 1},
    {"rich, at level 30", "rpp 12 7", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 30);), false, 1},
    {"banked", "rpp 13 a long red scar", false, SETUP(char_set_rbank(att, 10);), false, 1},
    {"banked, the feature set", "rpp 13 a long red scar", false, SETUP(char_set_rbank(att, 10); char_set_feature(att, "...He has a mole.");), false, 1},
    {"banked, a woman", "rpp 13 a long red scar", false, SETUP(char_set_rbank(att, 10); char_set_sex(att, SEX_FEMALE);), false, 1},
    {"banked, neither", "rpp 13 a long red scar", false, SETUP(char_set_rbank(att, 10); char_set_sex(att, SEX_NEUTRAL);), false, 1},
    {"unbanked", "rpp 13 a long red scar", false, SETUP(char_set_rbank(att, 1);), false, 1},
    {"banked", "rpp 13 a very long description that goes on and on and on well past sixty letters", false, SETUP(char_set_rbank(att, 10);), false, 1},
    {"instilling", "instill", false, SETUP(), false, 1},
    {"instilling", "instill token", false, SETUP(token(g, att);), false, 1},
    {"instilling without the token", "instill token ring", false, SETUP(ring(g, att, {ITEM_SLOT1});), false, 1},
    {"instilling into nothing", "instill token ring", false, SETUP(token(g, att);), false, 1},
    {"instilling a plain thing", "instill token ring", false, SETUP(token(g, att, APPLY_STR, -1); ring(g, att, {ITEM_SLOT1});), false, 1},
    {"instilling a forgery", "instill token ring", false, SETUP(obj_extra_set(token(g, att), ITEM_FORGED, true); ring(g, att, {ITEM_SLOT1});), false, 1},
    {"instilling into a plain thing", "instill token ring", false, SETUP(token(g, att); holding(g, att, "ring");), false, 1},
    {"instilling into a ring without slots", "instill token ring", false, SETUP(token(g, att); ring(g, att, {});), false, 1},
    {"instilling into a filled ring", "instill token ring", false, SETUP(token(g, att); ring(g, att, {ITEM_SLOT1, ITEM_SLOTS_FILLED});), false, 1},
    {"instilling into a full ring", "instill token ring", false, SETUP(token(g, att, APPLY_STR); ring(g, att, {ITEM_SLOT1}, {APPLY_DEX, APPLY_CON, APPLY_INT, APPLY_WIS, APPLY_CHA, APPLY_AGE});), false, 1},
    {"instilling into a full ring of the stat", "instill token ring", false, SETUP(token(g, att, APPLY_CON); ring(g, att, {ITEM_SLOT1}, {APPLY_DEX, APPLY_STR, APPLY_INT, APPLY_WIS, APPLY_CHA, APPLY_CON});), false, 1},
    {"instilling into a ring of one slot", "instill token ring", false, SETUP(token(g, att); ring(g, att, {ITEM_SLOT1}, {APPLY_DEX});), false, 1},
    {"instilling into a ring of two slots", "instill token ring", false, SETUP(token(g, att); ring(g, att, {ITEM_SLOT2}, {APPLY_DEX, APPLY_STR});), false, 1},
    {"instilling into a ring of two slots, one filled", "instill token ring", false, SETUP(token(g, att); ring(g, att, {ITEM_SLOT2, ITEM_SLOT_ONE}, {APPLY_DEX, APPLY_INT, APPLY_WIS});), false, 1},
    {"instilling into a ring of the stat", "instill token ring", false, SETUP(token(g, att, APPLY_DEX); ring(g, att, {ITEM_SLOT1}, {APPLY_DEX});), false, 1},
    {"instilling into a bare ring", "instill token ring", false, SETUP(token(g, att); ring(g, att, {ITEM_SLOT1});), false, 1},
    {"instilling into a ring of five stats", "instill token ring", false, SETUP(token(g, att); ring(g, att, {ITEM_SLOT1}, {APPLY_DEX, APPLY_CON, APPLY_INT, APPLY_WIS, APPLY_CHA});), false, 1},
    {"instilling into a ring of five stats, the last its", "instill token ring", false, SETUP(token(g, att, APPLY_CHA); ring(g, att, {ITEM_SLOT1}, {APPLY_DEX, APPLY_CON, APPLY_INT, APPLY_WIS, APPLY_CHA});), false, 1},
    {"a fighter", "arena", false, SETUP(moved(g, att, 17850);), false, 1},
    {"a spectator", "arena", false, SETUP(), false, 1},
    {"a spectator", "arena stop", false, SETUP(char_prf_set(g, att, PRF_ARENAWATCH, true);), false, 1},
    {"far from the arena", "arena look", false, SETUP(), false, 1},
    {"at the arena", "arena look", false, SETUP(moved(g, att, 17875);), false, 1},
    {"at the arena, watching no one", "arena look", false, SETUP(moved(g, att, 17875); char_prf_set(g, att, PRF_ARENAWATCH, true); char_set_arenawatch(att, -5);), false, 1},
    {"at the arena, watching a fighter", "arena look", false, SETUP(moved(g, att, 17875); moved(g, victim, 17850); char_prf_set(g, att, PRF_ARENAWATCH, true); char_set_arenawatch(att, char_get_idnum(victim));), false, 1},
    {"at the empty arena", "arena scan", false, SETUP(moved(g, att, 17875);), false, 1},
    {"at the arena, a fighter in it", "arena scan", false, SETUP(moved(g, att, 17875); moved(g, victim, 17850);), false, 1},
    {"at the arena", "arena -3", false, SETUP(moved(g, att, 17875);), false, 1},
    {"at the arena", "arena 99999", false, SETUP(moved(g, att, 17875); moved(g, victim, 17850);), false, 1},
    {"at the arena, a fighter in it", "arena 1", false, SETUP(moved(g, att, 17875); moved(g, victim, 17850); char_set_idnum(victim, 1);), false, 1},

    /* misc/techniques.cpp */
    {"unskilled", "spiritcontrol", false, SETUP(), false, 1},
    {"in control", "spiritcontrol", false, SETUP(char_set_skills_base(att, SKILL_SPIRITCONTROL, 50); char_aff_set(att, AFF_SPIRITCONTROL, true);), false, 1},
    {"tired", "spiritcontrol", false, SETUP(char_set_skills_base(att, SKILL_SPIRITCONTROL, 50); tired(g, att, 0);), false, 1},
    {"able", "spiritcontrol", false, SETUP(char_set_skills_base(att, SKILL_SPIRITCONTROL, 50);), false, 1},
    {"hiding a tail", "tailhide", false, SETUP(), false, 1},
    {"a saiyan, hiding a tail", "tailhide", false, SETUP(char_set_race(att, RACE_SAIYAN);), false, 1},
    {"a halfbreed, showing a tail", "tailhide", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TAILHIDE, true);), false, 1},
    {"stopping a tail", "nogrow", false, SETUP(), false, 1},
    {"a saiyan, stopping a tail", "nogrow", false, SETUP(char_set_race(att, RACE_SAIYAN);), false, 1},
    {"a saiyan, growing a tail", "nogrow", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_NOGROW, true);), false, 1},
    {"unskilled", "multiform split", false, SETUP(), false, 1},
    {"able", "multiform", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100);), false, 1},
    {"able", "multiform merge", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100);), false, 1},
    {"able, a clone by", "multiform merge", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100); mob_stock(g, 25); struct char_data *clone = read_mobile(g, 25); char_set_original(clone, att); char_to_room(g, clone, ARENA);), false, 1},
    {"able, without the clone", "multiform split", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100);), false, 2},
    {"able", "multiform split", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100); mob_stock(g, 25);), false, 4},
    {"able, described", "multiform split", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100); mob_stock(g, 25); char_set_ddesc(att, "A tall one.");), false, 2},
    {"able, fighting", "multiform split", true, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100); mob_stock(g, 25);), false, 2},
    {"clumsy", "multiform split", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 5); mob_stock(g, 25);), false, 2},
    {"able, drained", "multiform split", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100); drained(g, att);), false, 1},
    {"able, tired", "multiform split", false, SETUP(char_set_skills_base(att, SKILL_MULTIFORM, 100); tired(g, att, 0);), false, 1},
    {"a male", "moondust", false, SETUP(), false, 1},
    {"an arlian woman alone", "moondust", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE);), false, 1},
    {"an arlian woman, tired", "moondust", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); group(g, att, victim); tired(g, att, 0);), false, 1},
    {"an arlian woman, grouped", "moondust", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); group(g, att, victim); group(g, att, cast.helper); char_set_aff_abils_wis(att, 100);), false, 2},
    {"an arlian woman, following", "moondust", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); group(g, victim, att); char_set_aff_abils_wis(att, 100);), false, 1},
    {"a dull arlian woman, hurt", "moondust", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); group(g, att, victim); char_set_aff_abils_wis(att, 1); decCurHealth(g, att, getMaxPL(g, att) / 2, 1);), false, 4},
    {"a human", "shell", false, SETUP(), false, 1},
    {"an arlian woman", "shell", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE);), false, 1},
    {"an arlian, shelled", "shell", false, SETUP(char_set_race(att, RACE_ARLIAN); char_aff_set(att, AFF_SHELL, true);), false, 1},
    {"an arlian, tired", "shell", false, SETUP(char_set_race(att, RACE_ARLIAN); tired(g, att, 1);), false, 1},
    {"an arlian", "shell", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_aff_abils_con(att, 100);), false, 1},
    {"a weak arlian", "shell", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_aff_abils_con(att, 1);), false, 2},
    {"a human", "liquefy", false, SETUP(), false, 1},
    {"a majin, liquid", "liquefy", false, SETUP(char_set_race(att, RACE_MAJIN); char_aff_set(att, AFF_LIQUEFIED, true);), false, 1},
    {"a majin", "liquefy", false, SETUP(char_set_race(att, RACE_MAJIN);), false, 1},
    {"a drained majin", "liquefy hide", false, SETUP(char_set_race(att, RACE_MAJIN); drained(g, att);), false, 1},
    {"a majin", "liquefy melt", false, SETUP(char_set_race(att, RACE_MAJIN);), false, 1},
    {"a majin, holding on", "liquefy hide", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); grappled_by(victim, att); char_set_drag(att, cast.helper); char_set_dragged(cast.helper, att);), false, 2},
    {"a majin, held", "liquefy hide", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); grappled_by(att, victim);), false, 1},
    {"a novice majin", "liquefy hide", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 1);), false, 2},
    {"a majin", "liquefy explode", false, SETUP(char_set_race(att, RACE_MAJIN); grappled_by(victim, att);), false, 1},
    {"a majin, short of ki", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); decCurKI(g, att, getMaxKI(g, att) * 0.95, 0);), false, 1},
    {"a mighty majin, short of ki", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); spirited(att, 10000000); decCurKI(g, att, getMaxKI(g, att) * 0.95, 0);), false, 1},
    {"a majin, held and dragging", "liquefy explode nobody", false, SETUP(char_set_race(att, RACE_MAJIN); grappled_by(att, victim); char_set_drag(att, cast.helper); char_set_dragged(cast.helper, att);), false, 1},
    {"a majin", "liquefy explode nobody", false, SETUP(char_set_race(att, RACE_MAJIN);), false, 1},
    {"a majin, the victim a newbie", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(victim, 1);), false, 1},
    {"a novice majin", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 1);), false, 2},
    {"a majin, slower", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); char_set_aff_abils_dex(victim, 100); char_set_aff_abils_dex(att, 1);), false, 1},
    {"a majin, weaker", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); char_set_aff_abils_dex(att, 100); char_set_aff_abils_dex(victim, 1); empowered(victim, 100000000LL);), false, 1},
    {"a majin, mightier", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); char_set_aff_abils_dex(att, 100); char_set_aff_abils_dex(victim, 1); empowered(att, 100000000LL);), false, 1},
    {"a majin, mightier, grouped", "liquefy explode namek", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); char_set_aff_abils_dex(att, 100); char_set_aff_abils_dex(victim, 1); empowered(att, 100000000LL); group(g, att, cast.helper);), false, 1},
    {"setting it", "life", false, SETUP(), false, 1},
    {"setting it", "life 50", false, SETUP(), false, 1},
    {"setting it", "life 150", false, SETUP(), false, 1},
    {"setting it", "life 0", false, SETUP(), false, 1},
    {"defending no one", "defend", false, SETUP(), false, 1},
    {"defending the victim", "defend", false, SETUP(char_set_defending(att, victim); char_set_defender(victim, att);), false, 1},
    {"defending", "defend nobody", false, SETUP(), false, 1},
    {"defending", "defend self", false, SETUP(), false, 1},
    {"defending", "defend namek", false, SETUP(), false, 1},
    {"defending the victim already", "defend namek", false, SETUP(char_set_defending(att, victim); char_set_defender(victim, att);), false, 1},
    {"defending another, the victim defended", "defend namek", false, SETUP(char_set_defending(att, cast.helper); char_set_defender(cast.helper, att); char_set_defending(cast.android, victim); char_set_defender(victim, cast.android);), false, 1},
    {"not Galeos", "scry namek", false, SETUP(), false, 1},
    {"Galeos", "scry", false, SETUP(named(att, "Galeos");), false, 1},
    {"Galeos", "scry nobody", false, SETUP(named(att, "Galeos");), false, 1},
    {"Galeos", "scry self", false, SETUP(named(att, "Galeos");), false, 1},
    {"Galeos", "scry dummy", false, SETUP(named(att, "Galeos");), false, 1},
    {"Galeos, unpractised", "scry namek", false, SETUP(named(att, "Galeos");), false, 1},
    {"Galeos, practised", "scry namek", false, SETUP(named(att, "Galeos"); char_set_practice_points(g, att, 3000);), false, 1},
    {"Galeos, practised, ready to level", "scry namek", false, SETUP(named(att, "Galeos"); char_set_practice_points(g, att, 3000); char_set_exp(att, level_exp(att, char_get_level(att) + 1) + 5);), false, 1},
    {"Galeos, practised, at level 100", "scry namek", false, SETUP(named(att, "Galeos"); char_set_practice_points(g, att, 3000); char_set_class_level(att, 100);), false, 1},
    {"a human", "ashcloud 1", false, SETUP(), false, 1},
    {"a demon", "ashcloud", false, SETUP(ashes(g, att);), false, 1},
    {"a demon, by ash", "ashcloud 1", false, SETUP(ashes(g, att); obj_to_room(g, read_object(g, 1306), ARENA);), false, 1},
    {"a demon without ash", "ashcloud 1", false, SETUP(char_set_race(att, RACE_DEMON);), false, 1},
    {"a demon", "ashcloud 4", false, SETUP(ashes(g, att);), false, 1},
    {"a drained demon", "ashcloud 1", false, SETUP(ashes(g, att); drained(g, att);), false, 1},
    {"a demon, in water", "ashcloud 1", false, SETUP(ashes(g, att); room_set_sector_type(g, ARENA, SECT_UNDERWATER);), false, 1},
    {"a demon, in space", "ashcloud 1", false, SETUP(ashes(g, att); room_set_sector_type(g, ARENA, SECT_SPACE);), false, 1},
    {"a dull demon", "ashcloud 1", false, SETUP(ashes(g, att); char_set_aff_abils_intel(att, 1);), false, 2},
    {"a demon", "ashcloud 1", false, SETUP(ashes(g, att);), false, 2},
    {"a demon", "ashcloud 2", false, SETUP(ashes(g, att);), false, 2},
    {"a demon", "ashcloud 3", false, SETUP(ashes(g, att); char_set_aff_abils_con(att, 1);), false, 2},
    {"a demon, by the frail", "ashcloud 3", false, SETUP(ashes(g, att); char_set_aff_abils_con(victim, 1); char_set_aff_abils_con(cast.helper, 1);), false, 4},
    {"in burning ash", "ashcloud", false, SETUP(struct obj_data *cloud = read_object(g, 1306); obj_set_cost(cloud, 2); obj_to_room(g, cloud, ARENA); char_set_aff_abils_con(att, 1); ash_burn(g, att);), false, 4},
    {"in burning ash, eyes shut", "ashcloud", false, SETUP(struct obj_data *cloud = read_object(g, 1306); obj_to_room(g, cloud, ARENA); char_set_aff_abils_con(att, 1); char_plr_set(att, PLR_EYEC, true); ash_burn(g, att);), false, 1},
    {"an icer in burning ash", "ashcloud", false, SETUP(struct obj_data *cloud = read_object(g, 1306); obj_to_room(g, cloud, ARENA); char_set_race(att, RACE_ICER); char_set_aff_abils_con(att, 1); ash_burn(g, att);), false, 1},
    {"unskilled", "healingglow", false, SETUP(), false, 1},
    {"a healer", "healingglow namek", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 50);), false, 1},
    {"a master healer", "healingglow nobody", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 100);), false, 1},
    {"a healer, glowing", "healingglow", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 50); char_aff_set(att, AFF_HEALGLOW, true);), false, 1},
    {"a master healer, the victim glowing", "healingglow namek", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 100); char_aff_set(victim, AFF_HEALGLOW, true);), false, 1},
    {"a healer, fighting", "healingglow", true, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 50);), false, 1},
    {"a master healer, the victim fighting", "healingglow namek", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 100); set_fighting(g, victim, cast.helper);), false, 1},
    {"a drained healer", "healingglow", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 50); drained(g, att);), false, 1},
    {"a healer", "healingglow", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 50);), false, 1},
    {"a poor healer", "healingglow", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 5);), false, 1},
    {"a master healer", "healingglow namek", false, SETUP(char_set_skills_base(att, SKILL_HEALGLOW, 100);), false, 4},
    {"unskilled", "metamorph", false, SETUP(), false, 1},
    {"a saint", "metamorph", false, SETUP(char_set_skills_base(att, SKILL_METAMORPH, 50); char_set_alignment(att, 500);), false, 1},
    {"dark already", "metamorph", false, SETUP(char_set_skills_base(att, SKILL_METAMORPH, 50); char_aff_set(att, AFF_METAMORPH, true);), false, 1},
    {"drained", "metamorph", false, SETUP(char_set_skills_base(att, SKILL_METAMORPH, 50); drained(g, att);), false, 1},
    {"wise", "metamorph", false, SETUP(char_set_skills_base(att, SKILL_METAMORPH, 50); char_set_aff_abils_wis(att, 40);), false, 2},
    {"very wise", "metamorph", false, SETUP(char_set_skills_base(att, SKILL_METAMORPH, 50); char_set_aff_abils_wis(att, 60);), false, 1},
    {"foolish", "metamorph", false, SETUP(char_set_skills_base(att, SKILL_METAMORPH, 50); char_set_aff_abils_wis(att, 5);), false, 4},
    {"not Anubis, watching the arena", "shimmer earth", false, SETUP(char_prf_set(g, att, PRF_ARENAWATCH, true);), false, 1},
    {"Anubis, piloting", "shimmer namek", false, SETUP(named(att, "Anubis"); char_plr_set(att, PLR_PILOTING, true);), false, 1},
    {"Anubis, in a tank", "shimmer namek", false, SETUP(named(att, "Anubis"); char_plr_set(att, PLR_HEALT, true);), false, 1},
    {"Anubis, in a pocket dimension", "shimmer namek", false, SETUP(named(att, "Anubis"); moved(g, att, 19810);), false, 1},
    {"Anubis", "shimmer", false, SETUP(named(att, "Anubis");), false, 1},
    {"Anubis, drained", "shimmer namek", false, SETUP(named(att, "Anubis"); drained(g, att);), false, 1},
    {"Anubis", "shimmer nobody", false, SETUP(named(att, "Anubis");), false, 1},
    {"Anubis", "shimmer self", false, SETUP(named(att, "Anubis");), false, 2},
    {"Anubis", "shimmer planet-earth", false, SETUP(named(att, "Anubis");), false, 1},
    {"Anubis", "shimmer planet-konack", false, SETUP(named(att, "Anubis");), false, 1},
    {"Anubis", "shimmer afterlife", false, SETUP(named(att, "Anubis");), false, 1},
    {"Anubis", "shimmer planet-namek", false, SETUP(named(att, "Anubis"); room_stock(g, 10222);), false, 1},
    {"Anubis", "shimmer planet-frigid", false, SETUP(named(att, "Anubis"); room_stock(g, 4017);), false, 1},
    {"Anubis", "shimmer planet-vegeta", false, SETUP(named(att, "Anubis"); room_stock(g, 2200);), false, 1},
    {"Anubis", "shimmer planet-aether", false, SETUP(named(att, "Anubis"); room_stock(g, 12024);), false, 1},
    {"Anubis, fighting", "shimmer self", true, SETUP(named(att, "Anubis");), false, 4},
    {"Anubis, fighting", "shimmer planet-earth", true, SETUP(named(att, "Anubis");), false, 4},
    {"Anubis, fighting", "shimmer namek", true, SETUP(named(att, "Anubis"); away(g, victim);), false, 4},
    {"Anubis, the victim away", "shimmer namek", false, SETUP(named(att, "Anubis"); away(g, victim);), false, 1},
    {"Anubis, grappling the victim", "shimmer namek", false, SETUP(named(att, "Anubis"); grappled_by(victim, att);), false, 1},
    {"Anubis, the victim immortal", "shimmer namek", false, SETUP(named(att, "Anubis"); away(g, victim); char_set_admlevel(victim, 1);), false, 1},
    {"Anubis, the victim protected", "shimmer namek", false, SETUP(named(att, "Anubis"); away(g, victim); room_set_flag(g, 300, ROOM_NOINSTANT, true);), false, 1},
    {"Anubis, grappling the dead", "shimmer namek", false, SETUP(named(att, "Anubis"); away(g, victim); grappled_by(cast.helper, att); char_aff_set(cast.helper, AFF_SPIRIT, true);), false, 1},
    {"Anubis, dragging the dead", "shimmer namek", false, SETUP(named(att, "Anubis"); away(g, victim); char_set_drag(att, cast.helper); char_set_dragged(cast.helper, att); char_aff_set(cast.helper, AFF_SPIRIT, true);), false, 1},
    {"Anubis, held by the dead", "shimmer namek", false, SETUP(named(att, "Anubis"); away(g, victim); grappled_by(att, cast.helper); char_aff_set(cast.helper, AFF_SPIRIT, true);), false, 1},
    {"a human", "channel", false, SETUP(), false, 1},
    {"a demon, drained", "channel", false, SETUP(ruby_ready(g, att); drained(g, att);), false, 1},
    {"a demon without a ruby", "channel", false, SETUP(char_set_race(att, RACE_DEMON); char_set_skills_base(att, SKILL_STYLE, 100);), false, 1},
    {"a demon with a hot ruby", "channel", false, SETUP(ruby_ready(g, att, true);), false, 1},
    {"a demon, away from lava", "channel", false, SETUP(ruby_ready(g, att); room_set_geffect(g, ARENA, 0);), false, 1},
    {"a demon", "channel", false, SETUP(ruby_ready(g, att);), false, 2},
    {"a clumsy demon", "channel", false, SETUP(ruby_ready(g, att); char_set_skills_base(att, SKILL_STYLE, 40);), false, 4},
    {"a human", "hydromancy", false, SETUP(), false, 1},
    {"a tsuna, on land", "hydromancy", false, SETUP(tsuna(g, att); room_set_sector_type(g, ARENA, SECT_FIELD);), false, 1},
    {"a tsuna, underwater", "hydromancy", false, SETUP(tsuna(g, att); room_set_sector_type(g, ARENA, SECT_UNDERWATER);), false, 1},
    {"a tsuna, flooded", "hydromancy", false, SETUP(tsuna(g, att); room_set_sector_type(g, ARENA, SECT_FIELD); room_set_geffect(g, ARENA, -3);), false, 1},
    {"a drained tsuna", "hydromancy spike", false, SETUP(tsuna(g, att); drained(g, att);), false, 1},
    {"a clever tsuna, drained", "hydromancy spike", false, SETUP(tsuna(g, att); char_set_aff_abils_intel(att, 100); char_set_class_level(att, 100); drained(g, att);), false, 1},
    {"a tsuna, concentrating", "hydromancy spike", false, SETUP(tsuna(g, att); char_set_con_cooldown(att, 5);), false, 1},
    {"a tsuna", "hydromancy", false, SETUP(tsuna(g, att);), false, 1},
    {"a tsuna", "hydromancy rain", false, SETUP(tsuna(g, att);), false, 1},
    {"a tsuna", "hydromancy spike", false, SETUP(tsuna(g, att);), false, 2},
    {"a tsuna, laden", "hydromancy spike", false, SETUP(tsuna(g, att); laden(g, att);), false, 1},
    {"a middling tsuna", "hydromancy spike", false, SETUP(tsuna(g, att, 60);), false, 2},
    {"a poor tsuna", "hydromancy spike", false, SETUP(tsuna(g, att, 10);), false, 4},
    {"a tsuna", "hydromancy flood", false, SETUP(tsuna(g, att);), false, 1},
    {"a tsuna", "hydromancy flood north", false, SETUP(tsuna(g, att);), false, 1},
    {"a tsuna, by rooms", "hydromancy flood north", false, SETUP(tsuna(g, att); surrounded(g, SECT_FIELD); char_set_race(cast.helper, RACE_KANASSAN); char_aff_set(att, AFF_FLYING, true);), false, 4},
    {"a tsuna, by rooms, the victim balanced", "hydromancy flood north", false, SETUP(tsuna(g, att); surrounded(g, SECT_FIELD); char_set_skills_base(victim, SKILL_BALANCE, 100);), false, 2},
    {"a tsuna, by rooms, the victim a newbie", "hydromancy flood north", false, SETUP(tsuna(g, att); surrounded(g, SECT_FIELD); char_set_class_level(victim, 1);), false, 1},
    {"a poor tsuna, by rooms", "hydromancy flood north", false, SETUP(tsuna(g, att, 10); surrounded(g, SECT_FIELD);), false, 4},
    {"not Levanthoth", "kanso namek", false, SETUP(), false, 1},
    {"Levanthoth", "kanso nobody", false, SETUP(named(att, "Levanthoth");), false, 1},
    {"Levanthoth, an android by", "kanso android", false, SETUP(named(att, "Levanthoth"); char_from_room(g, cast.android); char_to_room(g, cast.android, ARENA);), false, 1},
    {"Levanthoth, the victim a newbie", "kanso namek", false, SETUP(named(att, "Levanthoth"); char_set_class_level(victim, 1);), false, 1},
    {"Levanthoth, drained", "kanso namek", false, SETUP(named(att, "Levanthoth"); char_set_aff_abils_intel(att, 50); drained(g, att);), false, 1},
    {"Levanthoth, dull", "kanso namek", false, SETUP(named(att, "Levanthoth"); char_set_aff_abils_intel(att, 5);), false, 2},
    {"Levanthoth, clever", "kanso namek", false, SETUP(named(att, "Levanthoth"); char_set_aff_abils_intel(att, 100); char_set_aff_abils_wis(att, 100); char_set_conditions(g, att, THIRST, 47);), false, 4},
    {"Levanthoth, clever, both a little thirsty", "kanso namek", false, SETUP(named(att, "Levanthoth"); char_set_aff_abils_intel(att, 100); char_set_conditions(g, att, THIRST, 10); char_set_conditions(g, victim, THIRST, 24);), false, 2},
    {"Levanthoth, clever, the victim parched", "kanso namek", false, SETUP(named(att, "Levanthoth"); char_set_aff_abils_intel(att, 100); char_set_conditions(g, victim, THIRST, 0); char_aff_set(victim, AFF_PARA, true);), false, 2},
    {"unskilled", "hayasa", false, SETUP(), false, 1},
    {"fast already", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 50); char_aff_set(att, AFF_HAYASA, true);), false, 1},
    {"drained", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 50); drained(g, att);), false, 1},
    {"a master", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 100);), false, 2},
    {"an expert", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 85);), false, 2},
    {"able", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 60);), false, 2},
    {"middling", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 30);), false, 2},
    {"a novice", "hayasa", false, SETUP(char_set_skills_base(att, SKILL_HAYASA, 10);), false, 2},
    {"unskilled", "ensnare namek", false, SETUP(), false, 1},
    {"without silk", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100);), false, 1},
    {"with forged silk", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att, 16703, true);), false, 1},
    {"with silk", "ensnare", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att);), false, 1},
    {"with silk", "ensnare nobody", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att);), false, 1},
    {"with silk, the victim ensnared", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att); char_aff_set(victim, AFF_ENSNARED, true);), false, 1},
    {"with silk, the victim armless", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att); char_set_limb_condition(victim, 0, 0); char_set_limb_condition(victim, 1, 0);), false, 1},
    {"clumsy, with silk", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 1); silk(g, att);), false, 2},
    {"with silk", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att);), false, 4},
    {"with silk, the victim zanzoken", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att); char_aff_set(victim, AFF_ZANZOKEN, true);), false, 1},
    {"with silk, both zanzoken", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att); char_aff_set(victim, AFF_ZANZOKEN, true); char_aff_set(att, AFF_ZANZOKEN, true);), false, 4},
    {"with silk, zanzoken", "ensnare namek", false, SETUP(char_set_skills_base(att, SKILL_ENSNARE, 100); silk(g, att); char_aff_set(att, AFF_ZANZOKEN, true);), false, 1},
    {"a human", "adrenaline pl 10", false, SETUP(), false, 1},
    {"a bio of arlian genes", "adrenaline", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 1, 6);), false, 1},
    {"a bio of other genes", "adrenaline pl 10", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 2); char_set_genome(att, 1, 3);), false, 1},
    {"an arlian", "adrenaline pl 200", false, SETUP(char_set_race(att, RACE_ARLIAN);), false, 1},
    {"an arlian, tired", "adrenaline pl 50", false, SETUP(char_set_race(att, RACE_ARLIAN); tired(g, att, 0);), false, 1},
    {"an arlian, hurt", "adrenaline pl 10", false, SETUP(char_set_race(att, RACE_ARLIAN); decCurHealth(g, att, getMaxPL(g, att) / 2, 1);), false, 1},
    {"an arlian", "adrenaline pl 10", false, SETUP(char_set_race(att, RACE_ARLIAN);), false, 1},
    {"an arlian, spent", "adrenaline ki 10", false, SETUP(char_set_race(att, RACE_ARLIAN); decCurKI(g, att, getMaxKI(g, att) / 2, 0);), false, 1},
    {"an arlian", "adrenaline ki 10", false, SETUP(char_set_race(att, RACE_ARLIAN);), false, 1},
    {"an arlian", "adrenaline st 10", false, SETUP(char_set_race(att, RACE_ARLIAN);), false, 1},
    {"unskilled", "fireshield", false, SETUP(), false, 1},
    {"shielded", "fireshield", false, SETUP(fireshielded(att);), false, 1},
    {"in a barrier", "fireshield", false, SETUP(char_set_skills_base(att, SKILL_FIRESHIELD, 80); char_aff_set(att, AFF_SANCTUARY, true);), false, 1},
    {"underwater", "fireshield", false, SETUP(char_set_skills_base(att, SKILL_FIRESHIELD, 80); room_set_sector_type(g, ARENA, SECT_UNDERWATER);), false, 1},
    {"drained", "fireshield", false, SETUP(char_set_skills_base(att, SKILL_FIRESHIELD, 80); drained(g, att);), false, 1},
    {"able", "fireshield", false, SETUP(char_set_skills_base(att, SKILL_FIRESHIELD, 100);), false, 2},
    {"clumsy", "fireshield", false, SETUP(char_set_skills_base(att, SKILL_FIRESHIELD, 5);), false, 2},
    {"unskilled", "warppool earth", false, SETUP(), false, 1},
    {"grappling", "warppool earth", false, SETUP(char_set_skills_base(att, SKILL_WARP, 100); grappled_by(victim, att);), false, 1},
    {"absorbing", "warppool earth", false, SETUP(char_set_skills_base(att, SKILL_WARP, 100); absorbed_by(victim, att);), false, 1},
    {"sitting", "warppool earth", false, SETUP(char_set_skills_base(att, SKILL_WARP, 100); seated(att);), false, 1},
    {"able", "warppool", false, SETUP(char_set_skills_base(att, SKILL_WARP, 100);), false, 1},
    {"drained", "warppool earth", false, SETUP(char_set_skills_base(att, SKILL_WARP, 100); drained(g, att);), false, 1},
    {"on land", "warppool earth", false, SETUP(char_set_skills_base(att, SKILL_WARP, 100);), false, 1},
    {"at sea by Frigid", "warppool earth", false, SETUP(at_sea(g, att, 4600);), false, 2},
    {"at sea by Earth", "warppool earth", false, SETUP(at_sea(g, att, 850); room_set_flag(g, 850, ROOM_EARTH, true);), false, 1},
    {"at sea by Earth", "warppool frigid", false, SETUP(at_sea(g, att, 850); room_set_flag(g, 4609, ROOM_FRIGID, true);), false, 1},
    {"at sea by Kanassa", "warppool kanassa", false, SETUP(at_sea(g, att, 15100); room_set_flag(g, 15100, ROOM_KANASSA, true);), false, 1},
    {"at sea by Kanassa", "warppool namek", false, SETUP(at_sea(g, att, 15100);), false, 1},
    {"at sea by Namek", "warppool namek", false, SETUP(at_sea(g, att, ARENA); room_set_flag(g, ARENA, ROOM_NAMEK, true); room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM);), false, 1},
    {"at sea by Aether", "warppool aether", false, SETUP(at_sea(g, att, 12252); room_set_flag(g, 12252, ROOM_AETHER, true);), false, 1},
    {"at sea by Aether", "warppool kanassa", false, SETUP(at_sea(g, att, 12252);), false, 1},
    {"at sea by Frigid", "warppool frigid", false, SETUP(at_sea(g, att, 4609); room_set_flag(g, 4609, ROOM_FRIGID, true);), false, 1},
    {"at sea by Frigid", "warppool aether", false, SETUP(at_sea(g, att, 4609);), false, 1},
    {"at sea", "warppool mars", false, SETUP(at_sea(g, att, 13155);), false, 1},
    {"clumsy at sea", "warppool namek", false, SETUP(at_sea(g, att, 4600); char_set_skills_base(att, SKILL_WARP, 1);), false, 2},
    {"unskilled", "hyoga north", false, SETUP(), false, 1},
    {"able, in peace", "hyoga north", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); room_set_flag(g, ARENA, ROOM_PEACEFUL, true);), false, 1},
    {"able, in space", "hyoga north", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); room_set_sector_type(g, ARENA, SECT_SPACE);), false, 1},
    {"able, in the air", "hyoga north", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); room_set_sector_type(g, ARENA, SECT_FLYING);), false, 1},
    {"able", "hyoga", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100);), false, 1},
    {"able, drained", "hyoga north", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); drained(g, att);), false, 1},
    {"able", "hyoga sideways", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100);), false, 1},
    {"able", "hyoga N", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100);), false, 1},
    {"able, by rooms", "hyoga n", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD);), false, 2},
    {"able, by rooms", "hyoga sw", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); char_set_aff_abils_intel(att, 100); char_set_aff_abils_wis(att, 100);), false, 1},
    {"able, by rooms", "hyoga i", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); char_set_aff_abils_intel(att, 30); char_set_aff_abils_wis(att, 30);), false, 1},
    {"able, by rooms", "hyoga O", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); char_set_aff_abils_intel(att, 20); char_set_aff_abils_wis(att, 15);), false, 1},
    {"able, by rooms", "hyoga se", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); char_set_aff_abils_intel(att, 12); char_set_aff_abils_wis(att, 10);), false, 1},
    {"able, by rooms", "hyoga ne", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); char_set_aff_abils_intel(att, 8); char_set_aff_abils_wis(att, 8);), false, 1},
    {"able, by rooms", "hyoga nw", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); char_set_aff_abils_intel(att, 1); char_set_aff_abils_wis(att, 1);), false, 1},
    {"able, by rooms", "hyoga e", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD);), false, 1},
    {"able, by rooms", "hyoga s", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD);), false, 1},
    {"able, by rooms", "hyoga w", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD);), false, 1},
    {"able, by rooms", "hyoga u", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD);), false, 1},
    {"able, by rooms", "hyoga d", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD);), false, 1},
    {"able, by a peaceful room", "hyoga n", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); room_set_flag(g, 51, ROOM_PEACEFUL, true);), false, 1},
    {"able, by a wall", "hyoga n", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); struct obj_data *wall = read_object(g, 79); obj_set_cost(wall, SOUTH); obj_to_room(g, wall, 51);), false, 1},
    {"able, by a wall the other way", "hyoga n", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 100); surrounded(g, SECT_FIELD); struct obj_data *wall = read_object(g, 79); obj_set_cost(wall, EAST); obj_to_room(g, wall, 51);), false, 1},
    {"clumsy, by rooms", "hyoga n", false, SETUP(char_set_skills_base(att, SKILL_HYOGA_KABE, 20); spirited(att, 10000000); surrounded(g, SECT_FIELD);), false, 4},
    {"unskilled", "dimizu", false, SETUP(), false, 1},
    {"able, flooded", "dimizu", false, SETUP(char_set_skills_base(att, SKILL_DIMIZU, 100); room_set_geffect(g, ARENA, -3);), false, 1},
    {"able, underwater", "dimizu", false, SETUP(char_set_skills_base(att, SKILL_DIMIZU, 100); room_set_sector_type(g, ARENA, SECT_UNDERWATER);), false, 1},
    {"able, in space", "dimizu", false, SETUP(char_set_skills_base(att, SKILL_DIMIZU, 100); room_set_flag(g, ARENA, ROOM_SPACE, true);), false, 1},
    {"able, drained", "dimizu", false, SETUP(char_set_skills_base(att, SKILL_DIMIZU, 100); drained(g, att);), false, 1},
    {"able", "dimizu", false, SETUP(char_set_skills_base(att, SKILL_DIMIZU, 100);), false, 2},
    {"clumsy", "dimizu", false, SETUP(char_set_skills_base(att, SKILL_DIMIZU, 1);), false, 2},
    {"dead", "stake", false, SETUP(char_aff_set(att, AFF_SPIRIT, true);), false, 1},
    {"in an immortal room", "stake", false, SETUP(moved(g, att, 5);), false, 1},
    {"alive", "stake", false, SETUP(), false, 1},

    /* misc/crafting.cpp */
    {"away from the shop", "restring item70", false, SETUP(), false, 1},
    {"at the shop, poor", "restring item70", false, SETUP(moved(g, att, 180);), false, 1},
    {"at the shop", "restring nothing", false, SETUP(moved(g, att, 180); char_set_gold(att, 6000);), false, 1},
    {"at the shop", "restring item70", false, SETUP(moved(g, att, 180); char_set_gold(att, 6000); holding(g, att, "item70");), false, 1},
    {"at the shop, with a custom piece", "restring item70", false, SETUP(moved(g, att, 180); char_set_gold(att, 6000); obj_extra_set(holding(g, att, "item70"), ITEM_CUSTOM, true);), false, 1},
    {"a mob at the shop restringing", "restring", false, SETUP(room_stock(g, 180); struct char_data *mob = read_mobile(g, 310); char_to_room(g, mob, 180); char_set_gold(mob, 6000); obj_to_char(g, read_object(g, 70), mob); command_interpreter(g, mob, "restring item70");), false, 1},
    {"fishing", "fish", false, SETUP(), false, 1},
    {"fishing", "fish reel", false, SETUP(), false, 1},
    {"fishing already", "fish cast", false, SETUP(char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"away from water", "fish cast", false, SETUP(), false, 1},
    {"by water, flying", "fish cast", false, SETUP(angler(g, att); char_aff_set(att, AFF_FLYING, true);), false, 1},
    {"by water, fighting", "fish cast", true, SETUP(angler(g, att);), false, 1},
    {"by water without a pole", "fish cast", false, SETUP(room_set_flag(g, ARENA, ROOM_FISHING, true);), false, 1},
    {"by water with a stick", "fish cast", false, SETUP(room_set_flag(g, ARENA, ROOM_FISHING, true); wearing(g, att, "stick", WEAR_WIELD2, ITEM_OTHER);), false, 1},
    {"by water, unbaited", "fish cast", false, SETUP(angler(g, att, 0);), false, 1},
    {"by water", "fish cast", false, SETUP(angler(g, att);), false, 1},
    {"not fishing", "fish hook", false, SETUP(), false, 1},
    {"fishing, nothing biting", "fish hook", false, SETUP(char_plr_set(att, PLR_FISHING, true); char_set_fishstate(att, FISH_NOFISH);), false, 1},
    {"fishing, reeling", "fish hook", false, SETUP(char_plr_set(att, PLR_FISHING, true); char_set_fishstate(att, FISH_REELING);), false, 1},
    {"fishing, hooked", "fish hook", false, SETUP(char_plr_set(att, PLR_FISHING, true); char_set_fishstate(att, FISH_HOOKED);), false, 1},
    {"fishing, a bite", "fish hook", false, SETUP(char_plr_set(att, PLR_FISHING, true); char_set_fishstate(att, FISH_BITE); char_set_accuracy(att, 60);), false, 4},
    {"without a pole", "fish apply worm", false, SETUP(), false, 1},
    {"with a stick", "fish apply worm", false, SETUP(wearing(g, att, "stick", WEAR_WIELD2, ITEM_OTHER);), false, 1},
    {"with a baited pole", "fish apply worm", false, SETUP(angler(g, att);), false, 1},
    {"with a pole", "fish apply", false, SETUP(angler(g, att, 0);), false, 1},
    {"with a pole", "fish apply worm", false, SETUP(angler(g, att, 0);), false, 1},
    {"with a pole and a stone", "fish apply worm", false, SETUP(angler(g, att, 0); holding(g, att, "worm");), false, 1},
    {"with a pole and bait", "fish apply worm", false, SETUP(angler(g, att, 0); obj_set_cost(holding(g, att, "worm", ITEM_FISHBAIT), 7);), false, 1},
    {"not fishing", "fish stop", false, SETUP(), false, 1},
    {"fishing", "fish stop", false, SETUP(char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"fishing out of water", "fish stop", false, SETUP(fishing(g, att, FISH_NOFISH, 40);), false, 1},
    {"fishing without a pole", "fish stop", false, SETUP(room_set_flag(g, ARENA, ROOM_FISHING, true); fishing(g, att, FISH_NOFISH, 40);), false, 1},
    {"fishing, waiting", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_NOFISH, 40);), false, 8},
    {"fishing in the sea, waiting", "fish stop", false, SETUP(angler(g, att, 5, false); fishing(g, att, FISH_NOFISH, 40);), false, 8},
    {"fishing, a bite", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_BITE, 40);), false, 6},
    {"fishing, hooked", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_HOOKED, 40);), false, 6},
    {"fishing, reeling", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_REELING, 40, 90);), false, 8},
    {"fishing, reeling, middling", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_REELING, 2, 50);), false, 4},
    {"fishing, reeling, poor", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_REELING, 2, 10);), false, 4},
    {"fishing, reeling, a long while", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_REELING, 40, 90);), false, 200},
    {"fishing in earth's fresh water, landing it, unstocked", "fish stop", false, SETUP(room_set_flag(g, ARENA, ROOM_FISHING, true); room_set_flag(g, ARENA, ROOM_FISHFRESH, true); room_set_flag(g, ARENA, ROOM_EARTH, true); obj_set_value(wearing(g, att, "pole", WEAR_WIELD2, ITEM_FISHPOLE), 0, 5); fishing(g, att, FISH_REELING, 0, 100);), false, 1},
    {"fishing in earth's fresh water, landing it", "fish stop", false, SETUP(angler(g, att); fishing(g, att, FISH_REELING, 0, 100);), false, 6},
    {"fishing in earth's fresh water, landing it clumsily", "fish stop", false, SETUP(angler(g, att, 0); fishing(g, att, FISH_REELING, 0, 50);), false, 6},
    {"fishing in earth's fresh water, landing it badly", "fish stop", false, SETUP(angler(g, att, 0); fishing(g, att, FISH_REELING, 0, 1);), false, 6},
    {"fishing in aether's fresh water, landing it", "fish stop", false, SETUP(angler(g, att, 200, true, ROOM_AETHER); fishing(g, att, FISH_REELING, 0, 100);), false, 4},
    {"fishing in earth's sea, landing it", "fish stop", false, SETUP(angler(g, att, 100, false); fishing(g, att, FISH_REELING, 0, 100);), false, 4},
    {"fishing in namek's sea, landing it", "fish stop", false, SETUP(angler(g, att, 100, false, ROOM_NAMEK); fishing(g, att, FISH_REELING, 0, 100);), false, 4},
    {"fishing nowhere in particular, landing it", "fish stop", false, SETUP(angler(g, att, 5, false, -1); fishing(g, att, FISH_REELING, 0, 100);), false, 2},
    {"unskilled", "extract plant", false, SETUP(), false, 1},
    {"an extractor", "extract", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100);), false, 1},
    {"an extractor", "extract combine", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100);), false, 1},
    {"an extractor with one bottle", "extract combine bottle bottle2", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding_as(g, att, "bottle", 3424);), false, 1},
    {"an extractor with two things", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding(g, att, "bottle"); holding_as(g, att, "jar", 3424);), false, 1},
    {"an extractor with a bottle and a thing", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding_as(g, att, "bottle", 3424); holding(g, att, "jar");), false, 1},
    {"an extractor with an empty bottle", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding_as(g, att, "bottle", 3424); obj_set_value(holding_as(g, att, "jar", 3424), 6, 5);), false, 1},
    {"an extractor with an empty jar", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); obj_set_value(holding_as(g, att, "bottle", 3424), 6, 5); holding_as(g, att, "jar", 3424);), false, 1},
    {"an extractor, the bottle fuller", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); obj_set_value(holding_as(g, att, "bottle", 3424), 6, 20); obj_set_value(holding_as(g, att, "jar", 3424), 6, 10);), false, 1},
    {"an extractor, the jar fuller", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); obj_set_value(holding_as(g, att, "bottle", 3424), 6, 5); obj_set_value(holding_as(g, att, "jar", 3424), 6, 10);), false, 1},
    {"an extractor, the jar much fuller", "extract combine bottle jar", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); obj_set_value(holding_as(g, att, "bottle", 3424), 6, 10); obj_set_value(holding_as(g, att, "jar", 3424), 6, 20);), false, 1},
    {"an extractor", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100);), false, 1},
    {"an extractor with a stone", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding(g, att, "plant");), false, 1},
    {"an extractor with an unripe plant", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); struct obj_data *p = holding_as(g, att, "plant", 3425); obj_set_value(p, VAL_MAXMATURE, 5); obj_set_value(p, VAL_MATURITY, 2);), false, 1},
    {"an extractor with a plant", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding_as(g, att, "plant", 3425);), false, 1},
    {"a drained extractor with a plant", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); holding_as(g, att, "plant", 3425); holding_as(g, att, "bottle", 3423); drained(g, att);), false, 1},
    {"an extractor with a plant", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); spirited(att, 10000000); holding_as(g, att, "plant", 3425); holding_as(g, att, "bottle", 3423);), false, 2},
    {"an extractor with a plant, nearly full", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 100); spirited(att, 10000000); holding_as(g, att, "plant", 3425); obj_set_value(holding_as(g, att, "bottle", 3423), 6, 22);), false, 2},
    {"a clumsy extractor with a plant", "extract plant", false, SETUP(char_set_skills_base(att, SKILL_EXTRACT, 1); spirited(att, 10000000); holding_as(g, att, "plant", 3425); holding_as(g, att, "bottle", 3423);), false, 2},
    {"unskilled", "runic namek kenaz", false, SETUP(), false, 1},
    {"a writer, fighting", "runic namek kenaz", true, SETUP(runic(g, att);), false, 1},
    {"a writer", "runic", false, SETUP(runic(g, att);), false, 1},
    {"a novice writer", "runic namek", false, SETUP(runic(g, att, 24, 10);), false, 1},
    {"a writer without ink", "runic namek kenaz", false, SETUP(runic(g, att, -1);), false, 1},
    {"a writer without a brush", "runic namek kenaz", false, SETUP(char_set_skills_base(att, SKILL_RUNIC, 100); obj_set_value(holding_as(g, att, "bottle", 3424), 6, 10);), false, 1},
    {"a writer", "runic nobody kenaz", false, SETUP(runic(g, att);), false, 1},
    {"a drained writer", "runic namek kenaz", false, SETUP(runic(g, att); drained(g, att);), false, 1},
    {"a clumsy writer", "runic namek kenaz", false, SETUP(runic(g, att, 24, 1);), false, 8},
    {"a writer", "runic namek nauthiz", false, SETUP(runic(g, att);), false, 1},
    {"a novice writer", "runic namek nauthiz", false, SETUP(runic(g, att, 24, 30);), false, 1},
    {"a hoshijin writer", "runic self kenaz", false, SETUP(runic(g, att); char_set_race(att, RACE_HOSHIJIN);), false, 1},
    {"a writer", "runic self kenaz", false, SETUP(runic(g, att);), false, 1},
    {"a writer, short of ink", "runic namek kenaz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer", "runic namek Kenaz", false, SETUP(runic(g, att);), false, 1},
    {"a writer, short of ink", "runic self algiz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer", "runic self algiz", false, SETUP(runic(g, att);), false, 1},
    {"a writer", "runic namek algiz", false, SETUP(runic(g, att, 4);), false, 1},
    {"a writer, short of ink", "runic self oagaz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer", "runic self oagaz", false, SETUP(runic(g, att, 5);), false, 1},
    {"a writer", "runic namek oagaz", false, SETUP(runic(g, att);), false, 1},
    {"a writer, short of ink", "runic self laguz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer", "runic self laguz", false, SETUP(runic(g, att);), false, 1},
    {"a writer", "runic namek laguz", false, SETUP(runic(g, att);), false, 1},
    {"a writer, short of ink", "runic self wunjo", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer", "runic self wunjo", false, SETUP(runic(g, att);), false, 1},
    {"a writer", "runic namek wunjo", false, SETUP(runic(g, att);), false, 1},
    {"a writer, short of ink", "runic self purisaz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer", "runic self purisaz", false, SETUP(runic(g, att);), false, 1},
    {"a writer", "runic namek purisaz", false, SETUP(runic(g, att);), false, 1},
    {"a novice writer", "runic namek purisaz", false, SETUP(runic(g, att, 24, 10); char_set_race(att, RACE_HOSHIJIN);), false, 4},
    {"a novice writer", "runic self purisaz", false, SETUP(runic(g, att, 24, 10); char_set_race(att, RACE_HOSHIJIN);), false, 4},
    {"a writer, short of ink", "runic self gebo", false, SETUP(runic(g, att, 5);), false, 1},
    {"a writer", "runic self gebo", false, SETUP(runic(g, att, 12);), false, 1},
    {"a writer", "runic namek gebo", false, SETUP(runic(g, att);), false, 1},
    {"a writer, the bottle emptied", "runic self kenaz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer, the bottle emptied", "runic namek kenaz", false, SETUP(runic(g, att, 3);), false, 1},
    {"a writer, the bottle emptied", "runic self algiz", false, SETUP(runic(g, att, 4);), false, 1},
    {"a writer, the bottle emptied", "runic namek algiz", false, SETUP(runic(g, att, 4);), false, 1},
    {"a writer, the bottle emptied", "runic self oagaz", false, SETUP(runic(g, att, 5);), false, 1},
    {"a writer, the bottle emptied", "runic namek oagaz", false, SETUP(runic(g, att, 5);), false, 1},
    {"a writer, the bottle emptied", "runic self laguz", false, SETUP(runic(g, att, 6);), false, 1},
    {"a writer, the bottle emptied", "runic namek laguz", false, SETUP(runic(g, att, 6);), false, 1},
    {"a writer, the bottle emptied", "runic self wunjo", false, SETUP(runic(g, att, 6);), false, 1},
    {"a writer, the bottle emptied", "runic namek wunjo", false, SETUP(runic(g, att, 6);), false, 1},
    {"a writer, the bottle emptied", "runic self purisaz", false, SETUP(runic(g, att, 6);), false, 1},
    {"a writer, the bottle emptied", "runic namek purisaz", false, SETUP(runic(g, att, 6);), false, 1},
    {"a writer, the bottle emptied", "runic self gebo", false, SETUP(runic(g, att, 12);), false, 1},
    {"a writer, the bottle emptied", "runic namek gebo", false, SETUP(runic(g, att, 12);), false, 1},
    {"a clumsy writer, nearly out of ink", "runic namek kenaz", false, SETUP(runic(g, att, 1, 1);), false, 8},
    {"unskilled", "resize item70 small", false, SETUP(), false, 1},
    {"a builder", "resize item70 small", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 50);), false, 1},
    {"a master builder", "resize item70", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100);), false, 1},
    {"a master builder", "resize ring small", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100);), false, 1},
    {"a master builder", "resize ring small", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); holding(g, att, "ring");), false, 1},
    {"a tired master builder", "resize ring small", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); ring(g, att, {}); tired(g, att, 0);), false, 1},
    {"a master builder", "resize ring small", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); obj_set_size(ring(g, att, {}), SIZE_MEDIUM);), false, 1},
    {"a master builder", "resize ring small", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); obj_set_size(ring(g, att, {}), SIZE_SMALL);), false, 1},
    {"a master builder", "resize ring medium", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); obj_set_size(ring(g, att, {}), SIZE_SMALL);), false, 1},
    {"a master builder", "resize ring medium", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); obj_set_size(ring(g, att, {}), SIZE_MEDIUM);), false, 1},
    {"a master builder", "resize ring large", false, SETUP(char_set_skills_base(att, SKILL_BUILD, 100); ring(g, att, {});), false, 1},
    {"armless", "dig", false, SETUP(char_set_limb_condition(att, 0, 0); char_set_limb_condition(att, 1, 0);), false, 1},
    {"grappling", "dig", false, SETUP(grappled_by(victim, att);), false, 1},
    {"absorbing", "dig", false, SETUP(absorbed_by(victim, att);), false, 1},
    {"digging", "dig", false, SETUP(), false, 1},
    {"on stone", "dig bury item70", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY);), false, 1},
    {"on a field", "dig bury", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD);), false, 1},
    {"on a field", "dig bury stone", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD);), false, 1},
    {"on a field", "dig bury stone", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); holding(g, att, "stone");), false, 1},
    {"in a desert", "dig bury stone", false, SETUP(room_set_sector_type(g, ARENA, SECT_DESERT); holding(g, att, "stone");), false, 1},
    {"on a hill, by a buried thing", "dig bury stone", false, SETUP(room_set_sector_type(g, ARENA, SECT_HILLS); holding(g, att, "stone"); obj_extra_set(thing_here(g, "box", ITEM_OTHER), ITEM_BURIED, true);), false, 1},
    {"in a forest", "dig uncover", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST);), false, 1},
    {"in a forest, by a buried thing", "dig uncover", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); obj_extra_set(thing_here(g, "box", ITEM_OTHER), ITEM_BURIED, true);), false, 1},
    {"in a desert, by a buried thing", "dig uncover", false, SETUP(room_set_sector_type(g, ARENA, SECT_DESERT); obj_extra_set(thing_here(g, "box", ITEM_OTHER), ITEM_BURIED, true);), false, 1},
    {"on a mountain", "dig around", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN);), false, 1},
    {"unskilled", "silk", false, SETUP(), false, 1},
    {"a spinner", "silk", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100);), false, 1},
    {"a spinner", "silk knit", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100);), false, 1},
    {"a spinner", "silk weave", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100);), false, 1},
    {"a spinner without silk", "silk weave head", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100);), false, 1},
    {"a spinner with forged silk", "silk weave head", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16703, true);), false, 1},
    {"a spinner", "silk weave scarf", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att);), false, 1},
    {"a clumsy spinner", "silk weave head", false, SETUP(char_set_skills_base(att, SKILL_SILK, 1); silk(g, att);), false, 2},
    {"a spinner, the best silk", "silk weave head", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16708);), false, 4},
    {"a spinner, fine silk", "silk weave wrist", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16700);), false, 4},
    {"a spinner, great silk", "silk weave belt", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16701);), false, 4},
    {"a spinner, good silk", "silk weave head", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16702);), false, 4},
    {"a spinner, decent silk", "silk weave wrist", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16703);), false, 4},
    {"a spinner, poor silk", "silk weave belt", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); silk(g, att, 16704);), false, 4},
    {"a drained spinner", "silk bundle", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100); drained(g, att);), false, 1},
    {"a master spinner", "silk bundle", false, SETUP(char_set_skills_base(att, SKILL_SILK, 100);), false, 4},
    {"a fine spinner", "silk bundle", false, SETUP(char_set_skills_base(att, SKILL_SILK, 95);), false, 4},
    {"a good spinner", "silk bundle", false, SETUP(char_set_skills_base(att, SKILL_SILK, 85);), false, 4},
    {"a decent spinner", "silk bundle", false, SETUP(char_set_skills_base(att, SKILL_SILK, 60);), false, 4},
    {"a poor spinner", "silk bundle", false, SETUP(char_set_skills_base(att, SKILL_SILK, 30);), false, 4},
    {"a kurzak master spinner", "silk bundle", false, SETUP(char_set_class(att, CLASS_KURZAK); char_set_skills_base(att, SKILL_SILK, 100);), false, 16},
    {"a kurzak good spinner", "silk bundle", false, SETUP(char_set_class(att, CLASS_KURZAK); char_set_skills_base(att, SKILL_SILK, 70);), false, 16},
    {"a kurzak decent spinner", "silk bundle", false, SETUP(char_set_class(att, CLASS_KURZAK); char_set_skills_base(att, SKILL_SILK, 45);), false, 16},
    {"a kurzak poor spinner", "silk bundle", false, SETUP(char_set_class(att, CLASS_KURZAK); char_set_skills_base(att, SKILL_SILK, 20);), false, 2},
    {"away from a fire", "cook", false, SETUP(), false, 1},
    {"unskilled, by a fire", "cook", false, SETUP(thing_here(g, "campfire", ITEM_CAMPFIRE);), false, 1},
    {"a cook", "cook", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook", "cook 0", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook", "cook 18", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 4", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 5", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 6", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 7", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 8", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 9", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 10", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 12", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 14", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 15", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 16", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook without anything", "cook 17", false, SETUP(cook(g, att, {});), false, 1},
    {"a cook with half of it", "cook 6", false, SETUP(cook(g, att, {RCP_CHILIPEPPER, RCP_TOMATO, RCP_TOMATO});), false, 1},
    {"a cook with too much", "cook 2", false, SETUP(cook(g, att, {RCP_TOMATO, RCP_TOMATO, RCP_TOMATO, RCP_POTATO});), false, 1},
    {"a cook", "cook 1", false, SETUP(cook(g, att, {RCP_NORMAL_MEAT});), false, 2},
    {"a cook", "cook 2", false, SETUP(cook(g, att, {RCP_TOMATO, RCP_TOMATO}, true);), false, 1},
    {"a cook over a fire", "cook 2", false, SETUP(cook(g, att, {RCP_TOMATO, RCP_TOMATO});), false, 1},
    {"a cook", "cook 3", false, SETUP(cook(g, att, {RCP_POTATO, RCP_POTATO}, true);), false, 1},
    {"a cook", "cook 4", false, SETUP(cook(g, att, {RCP_POTATO, RCP_TOMATO, RCP_CARROT, RCP_GREENBEAN, RCP_ONION}, true);), false, 1},
    {"a cook", "cook 5", false, SETUP(cook(g, att, {RCP_NORMAL_MEAT, RCP_POTATO, RCP_TOMATO, RCP_GARLIC}, true);), false, 1},
    {"a cook", "cook 6", false, SETUP(cook(g, att, {RCP_NORMAL_MEAT, RCP_CHILIPEPPER, RCP_CHILIPEPPER, RCP_CHILIPEPPER, RCP_CHILIPEPPER, RCP_TOMATO, RCP_TOMATO}, true);), false, 1},
    {"a cook", "cook 7", false, SETUP(cook(g, att, {RCP_FLOUNDER});), false, 1},
    {"a cook", "cook 8", false, SETUP(cook(g, att, {RCP_VALBISH});), false, 1},
    {"a cook", "cook 9", false, SETUP(cook(g, att, {RCP_SHADOWFISH});), false, 1},
    {"a cook", "cook 10", false, SETUP(cook(g, att, {RCP_TAMBOR});), false, 1},
    {"a cook", "cook 11", false, SETUP(cook(g, att, {RCP_RICE}, true);), false, 1},
    {"a cook", "cook 12", false, SETUP(cook(g, att, {RCP_RICE, RCP_NARRI}, true);), false, 1},
    {"a cook", "cook 13", false, SETUP(cook(g, att, {RCP_FLOUR}, true);), false, 1},
    {"a cook", "cook 14", false, SETUP(cook(g, att, {RCP_TOMATO, RCP_CUCUMBER, RCP_CARROT, RCP_LETTUCE}, true);), false, 1},
    {"a cook", "cook 15", false, SETUP(cook(g, att, {RCP_FLOUR, RCP_APPLEPLUM}, true);), false, 1},
    {"a cook", "cook 16", false, SETUP(cook(g, att, {RCP_FLOUR, RCP_FROZENBERRY}, true);), false, 1},
    {"a cook", "cook 17", false, SETUP(cook(g, att, {RCP_FLOUR, RCP_CARAMBOLA}, true);), false, 1},
    {"a cook of recipes", "cook 1", false, SETUP(cook(g, att, {RCP_NORMAL_MEAT}); char_set_bonuses(att, BONUS_RECIPE, 1);), false, 1},
    {"a clumsy cook", "cook 1", false, SETUP(cook(g, att, {RCP_NORMAL_MEAT}, false, 1);), false, 2},
    {"feeding", "feed", false, SETUP(), false, 1},
    {"feeding", "feed nobody senzu", false, SETUP(), false, 1},
    {"feeding, an android by", "feed android senzu", false, SETUP(char_from_room(g, cast.android); char_to_room(g, cast.android, ARENA);), false, 1},
    {"feeding without a bean", "feed namek senzu", false, SETUP(), false, 1},
    {"feeding a stone", "feed namek senzu", false, SETUP(holding(g, att, "senzu");), false, 1},
    {"feeding a fake bean", "feed namek senzu", false, SETUP(obj_extra_set(holding(g, att, "senzu", ITEM_POTION), ITEM_FORGED, true);), false, 1},
    {"feeding a broken bean", "feed namek senzu", false, SETUP(obj_extra_set(holding(g, att, "senzu", ITEM_POTION), ITEM_BROKEN, true);), false, 1},
    {"feeding the fighting", "feed namek senzu", false, SETUP(holding(g, att, "senzu", ITEM_POTION); set_fighting(g, victim, cast.helper);), false, 1},
    {"feeding a stranger", "feed namek senzu", false, SETUP(holding(g, att, "senzu", ITEM_POTION);), false, 1},
    {"feeding a follower, ungrouped", "feed namek senzu", false, SETUP(holding(g, att, "senzu", ITEM_POTION); add_follower(g, victim, att);), false, 1},
    {"feeding another's follower", "feed namek senzu", false, SETUP(holding(g, att, "senzu", ITEM_POTION); group(g, cast.helper, victim); char_aff_set(att, AFF_GROUP, true);), false, 1},
    {"feeding the group", "feed namek senzu", false, SETUP(holding(g, att, "senzu", ITEM_POTION); group(g, att, victim);), false, 1},
    {"decapitating", "decapitate", false, SETUP(), false, 1},
    {"decapitating", "decapitate corpse", false, SETUP(), false, 1},
    {"decapitating a headless corpse", "decapitate corpse", false, SETUP(thing_here(g, "headless corpse", ITEM_CONTAINER, {{VAL_CORPSE_HEAD, 0}});), false, 1},
    {"decapitating by hand", "decapitate corpse", false, SETUP(thing_here(g, "corpse burnt bloody Bob", ITEM_CONTAINER, {{VAL_CORPSE_HEAD, 1}});), false, 1},
    {"decapitating with a blade", "decapitate corpse", false, SETUP(wield(g, att, TYPE_PIERCE - TYPE_HIT); thing_here(g, "corpse Bob", ITEM_CONTAINER, {{VAL_CORPSE_HEAD, 1}});), false, 1},
    {"decapitating with a club", "decapitate corpse", false, SETUP(wield(g, att, TYPE_BLUDGEON - TYPE_HIT); thing_here(g, "corpse Bob", ITEM_CONTAINER, {{VAL_CORPSE_HEAD, 1}});), false, 1},
    {"decapitating with a dagger in the other hand", "decapitate corpse", false, SETUP(off_hand(g, att, TYPE_STAB - TYPE_HIT); thing_here(g, "corpse Bob", ITEM_CONTAINER, {{VAL_CORPSE_HEAD, 1}});), false, 1},
    {"decapitating with a club in the other hand", "decapitate corpse", false, SETUP(off_hand(g, att, TYPE_BLUDGEON - TYPE_HIT); thing_here(g, "corpse Bob", ITEM_CONTAINER, {{VAL_CORPSE_HEAD, 1}});), false, 1},
  };
} // namespace

extern const std::span<const scene> SCENES_MISC = list;
