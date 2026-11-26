/* Scenes for test_techniques: see scene_kit.hpp. These are act.movement.cpp's. */
#include <array>
#include "scene_kit.hpp"

#include "dbat/act.movement.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/exitflags.hpp"
#include "dbat/consts/zoneflags.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/zone_utils.hpp"

namespace {
  /* ... the way south, to room 300, a door of these flags (and the way back too), named so, opened by this key */
  struct room_direction_data *door_south(GameWorld *g, flag_set<exit_flag> flags, const char *keyword = "door", obj_vnum key = -1, bool back = true)
  {
    struct room_direction_data *exit = room_get_dir_option(g, ARENA, SOUTH);
    exit->exit_info                  = flags;
    exit->keyword                    = keyword;
    exit->key                        = key;
    if (back) {
      auto *other    = new room_direction_data();
      other->to_room = ARENA;
      other->exit_info = flags;
      other->keyword = "gate";
      room_set_dir_option(g, 300, NORTH, other);
    }
    return exit;
  }

  /* ... a room this way, of its own, stocked if it isn't there */
  struct room_data *room_to(GameWorld *g, int dir, room_vnum to, int sect = -1)
  {
    room_stock(g, to);
    exit_to(g, dir, to);
    if (sect >= 0)
      room_set_sector_type(g, to, sect);
    return room_of(g, to);
  }

  void orbiting(GameWorld *g, struct char_data *att, room_vnum above, std::initializer_list<room_vnum> landings)
  {
    for (room_vnum room : landings)
      room_stock(g, room);
    moved(g, att, above);
  }

  void asleep(struct char_data *ch) { char_set_position(ch, POS_SLEEPING); }

  void carrying(GameWorld *g, struct char_data *att, struct char_data *victim)
  {
    asleep(victim);
    char_set_carrying(g, att, victim);
    char_set_carried_by(g, victim, att);
  }

  void dragging(struct char_data *att, struct char_data *victim)
  {
    char_set_drag(att, victim);
    char_set_dragged(victim, att);
  }

  struct obj_data *thing_named(GameWorld *g, const char *name, int type, std::initializer_list<std::pair<int, int>> values = {}, obj_vnum vnum = 70)
  {
    struct obj_data *obj = thing_here(g, name, type, values);
    if (vnum != 70)
      renumber(g, obj, vnum);
    return obj;
  }

  /* ... sitting on a seat of this type */
  struct obj_data *seated_on(GameWorld *g, struct char_data *ch, int type, int position = POS_SITTING, const char *name = "seat")
  {
    struct obj_data *seat = thing_named(g, name, type);
    char_set_sits(ch, seat);
    obj_set_sitting(seat, ch);
    char_set_position(ch, position);
    return seat;
  }

  void focused(struct char_data *ch, int focus = 50) { char_set_skills_base(ch, SKILL_FOCUS, focus); }

  void flying(struct char_data *ch, int altitude = 1)
  {
    char_aff_set(ch, aff_flag::flying, true);
    char_set_altitude(ch, altitude);
  }

  void picker(GameWorld *g, struct char_data *att, int skill = 100)
  {
    char_set_skills_base(att, SKILL_OPEN_LOCK, skill);
    carries_as(g, att, 18, "kit");
  }

  /* ... carrying the key of this vnum */
  void keyed(GameWorld *g, struct char_data *att, obj_vnum key) { carries_as(g, att, key, "key"); }

  /* ... a ship of this vnum by the arena (in space if asked), with a hatch inside it in room 81, closed and locked
     as asked; att aboard by the hatch, or by the ship */
  void ship_at(GameWorld *g, struct char_data *att, obj_vnum vnum, bool aboard, bool in_space = false, bool closed = true, bool locked = false)
  {
    int flags = cont_value(container_flag::closeable) | (closed ? cont_value(container_flag::closed) : 0) | (locked ? cont_value(container_flag::locked) : 0);
    room_stock(g, 81);
    struct obj_data *ship = thing_named(g, "ship", ITEM_VEHICLE, {{VAL_PORTAL_DEST, 81}, {VAL_CONTAINER_FLAGS, flags}, {VAL_KEY_KEYCODE, 1}}, vnum);
    struct obj_data *hatch = read_object(g, 70);
    obj_set_name(hatch, "hatch");
    obj_set_type_flag(hatch, ITEM_HATCH);
    obj_set_value(hatch, VAL_HATCH_DEST, vnum);
    obj_set_value(hatch, VAL_HATCH_FLAGS, flags);
    obj_set_value(hatch, VAL_KEY_KEYCODE, 1);
    if (vnum > 19199) /* a ship's hatch is numbered as ships are */
      renumber(g, hatch, vnum + 1);
    obj_to_room(g, hatch, 81);
    if (in_space) {
      room_set_flag(g, ARENA, room_flag::space, true);
      (void)ship;
    }
    if (aboard)
      moved(g, att, 81);
  }

  void limbs(struct char_data *ch, int ra, int la, int rl, int ll)
  {
    char_set_limb_condition(ch, 0, ra);
    char_set_limb_condition(ch, 1, la);
    char_set_limb_condition(ch, 2, rl);
    char_set_limb_condition(ch, 3, ll);
  }

  const auto list = std::to_array<scene>({
    /* carry */
    {"dragging someone", "carry namek", false, SETUP(dragging(att, victim);), false, 1},
    {"piloting", "carry namek", false, SETUP(char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"a good soul, carrying someone", "carry", false, SETUP(carrying(g, att, victim); char_set_alignment(att, 100);), false, 1},
    {"carrying someone", "carry", false, SETUP(carrying(g, att, victim); char_set_alignment(att, 0);), false, 1},
    {"", "carry", false, SETUP(), false, 1},
    {"", "carry nobody", false, SETUP(), false, 1},
    {"", "carry dummy", false, SETUP(), false, 1},
    {"by someone carried", "carry namek", false, SETUP(carrying(g, cast.helper, victim);), false, 1},
    {"", "carry namek", false, SETUP(), false, 1},
    {"by a heavy sleeper", "carry namek", false, SETUP(asleep(victim); laden(g, victim);), false, 1},
    {"by a sleeper", "carry namek", false, SETUP(asleep(victim);), false, 1},
    {"by a sleeper on a bed", "carry namek", false, SETUP(seated_on(g, victim, ITEM_BED, POS_SLEEPING);), false, 1},

    {"carrying someone, knocked loose", "look", false, SETUP(carrying(g, att, victim); carry_drop(g, att, 2);), false, 1},
    {"carrying someone, who leaves", "look", false, SETUP(carrying(g, att, victim); carry_drop(g, att, 3);), false, 1},
    {"carrying someone, put down oddly", "look", false, SETUP(carrying(g, att, victim); carry_drop(g, att, 7);), false, 1},

    /* teleporting, called as the techniques that use it do */
    {"teleporting nowhere", "look", false, SETUP(handle_teleport(g, att, nullptr, 0);), false, 1},
    {"teleporting to a room, dragging and grappling players", "look", false, SETUP(dragging(att, victim); grappled_by(cast.helper, att, 3); handle_teleport(g, att, nullptr, 300);), false, 1},
    {"teleporting to someone, carrying a player", "look", false, SETUP(carrying(g, att, victim); away(g, cast.helper); handle_teleport(g, att, cast.helper, 0);), false, 1},
    {"teleporting, grappled by a player", "look", false, SETUP(grappled_by(att, cast.helper, 3); handle_teleport(g, att, nullptr, 300);), false, 1},
    {"teleporting, dragging and grappling mobs", "look", false, SETUP({ char_data *d = nullptr; for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c)) { d = c; break; } if (d) { dragging(att, d); grappled_by(d, att, 3); } handle_teleport(g, att, nullptr, 300); }), false, 1},
    {"teleporting, grappled by a mob", "look", false, SETUP({ char_data *d = nullptr; for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c)) { d = c; break; } if (d) grappled_by(att, d, 3); handle_teleport(g, att, nullptr, 300); }), false, 1},

    /* land */
    {"", "land", false, SETUP(), false, 1},
    {"", "land Nexus City", false, SETUP(), false, 1},
    {"above Earth", "land", false, SETUP(orbiting(g, att, 50, {});), false, 1},
    {"above Earth", "land Nexus City", false, SETUP(orbiting(g, att, 50, {});), false, 1},
    {"above Earth", "land kami's lookout", false, SETUP(orbiting(g, att, 50, {1506});), false, 1},
    {"above Earth", "land Atlantis", false, SETUP(orbiting(g, att, 50, {});), false, 1},
    {"above Frigid", "land", false, SETUP(orbiting(g, att, 51, {});), false, 1},
    {"above Frigid", "land Koltoan Mine", false, SETUP(orbiting(g, att, 51, {4944});), false, 1},
    {"above Frigid", "land Atlantis", false, SETUP(orbiting(g, att, 51, {4264});), false, 1},
    {"above Konack", "land", false, SETUP(orbiting(g, att, 52, {});), false, 1},
    {"above Konack", "land Furian Citadel", false, SETUP(orbiting(g, att, 52, {9949});), false, 1},
    {"above Konack", "land Atlantis", false, SETUP(orbiting(g, att, 52, {8006});), false, 1},
    {"above Vegeta", "land", false, SETUP(orbiting(g, att, 53, {});), false, 1},
    {"above Vegeta", "land Pride Forest", false, SETUP(orbiting(g, att, 53, {2800});), false, 1},
    {"above Vegeta", "land Atlantis", false, SETUP(orbiting(g, att, 53, {2226});), false, 1},
    {"above Namek", "land", false, SETUP(orbiting(g, att, 54, {});), false, 1},
    {"above Namek", "land Kakureta Village", false, SETUP(orbiting(g, att, 54, {10922});), false, 1},
    {"above Namek", "land Atlantis", false, SETUP(orbiting(g, att, 54, {11600});), false, 1},
    {"above Aether", "land", false, SETUP(orbiting(g, att, 55, {});), false, 1},
    {"above Aether", "land Silent Glade", false, SETUP(orbiting(g, att, 55, {12480});), false, 1},
    {"above Aether", "land Atlantis", false, SETUP(orbiting(g, att, 55, {12010});), false, 1},
    {"above Yardrat", "land", false, SETUP(orbiting(g, att, 56, {});), false, 1},
    {"above Yardrat", "land Mount Valaria", false, SETUP(orbiting(g, att, 56, {14300});), false, 1},
    {"above Yardrat", "land Atlantis", false, SETUP(orbiting(g, att, 56, {14008});), false, 1},
    {"above Zennith", "land", false, SETUP(orbiting(g, att, 57, {});), false, 1},
    {"above Zennith", "land Ancient Castle", false, SETUP(orbiting(g, att, 57, {19600});), false, 1},
    {"above Zennith", "land Atlantis", false, SETUP(orbiting(g, att, 57, {3412});), false, 1},
    {"above Kanassa", "land", false, SETUP(orbiting(g, att, 58, {});), false, 1},
    {"above Kanassa", "land Yunkai Pirate Base", false, SETUP(orbiting(g, att, 58, {15655});), false, 1},
    {"above Kanassa", "land Atlantis", false, SETUP(orbiting(g, att, 58, {14904});), false, 1},
    {"above Arlia", "land", false, SETUP(orbiting(g, att, 59, {});), false, 1},
    {"above Arlia", "land Arlia Mine", false, SETUP(orbiting(g, att, 59, {16600});), false, 1},
    {"above Arlia", "land Atlantis", false, SETUP(orbiting(g, att, 59, {16009});), false, 1},
    {"above Cerria", "land", false, SETUP(orbiting(g, att, 198, {});), false, 1},
    {"above Cerria", "land Fistarl Volcano", false, SETUP(orbiting(g, att, 198, {17420});), false, 1},
    {"above Cerria", "land Atlantis", false, SETUP(orbiting(g, att, 198, {17531});), false, 1},

    {"landing everywhere from 50", "look", false, SETUP(for (room_vnum r : {300, 800, 1150, 1180, 1287, 1428, 1456, 1506, 1636, 1710, 19510, 2141, 13020}) room_stock(g, r); moved(g, att, 50); do_land(g, att, "Nexus City", 0, 0); moved(g, att, 50); do_land(g, att, "South Ocean", 0, 0); moved(g, att, 50); do_land(g, att, "Nexus Field", 0, 0); moved(g, att, 50); do_land(g, att, "Cherry Blossom Mountain", 0, 0); moved(g, att, 50); do_land(g, att, "Sandy Desert", 0, 0); moved(g, att, 50); do_land(g, att, "Northern Plains", 0, 0); moved(g, att, 50); do_land(g, att, "Korin's Tower", 0, 0); moved(g, att, 50); do_land(g, att, "Kami's Lookout", 0, 0); moved(g, att, 50); do_land(g, att, "Shadow Forest", 0, 0); moved(g, att, 50); do_land(g, att, "Decrepit Area", 0, 0); moved(g, att, 50); do_land(g, att, "West City", 0, 0); moved(g, att, 50); do_land(g, att, "Hercule Beach", 0, 0); moved(g, att, 50); do_land(g, att, "Satan City", 0, 0);), false, 1},
    {"landing everywhere from 51", "look", false, SETUP(for (room_vnum r : {4264, 4300, 4351, 4400, 4600, 4800, 5100, 5150, 5165, 5200, 5500, 4944}) room_stock(g, r); moved(g, att, 51); do_land(g, att, "Ice Crown City", 0, 0); moved(g, att, 51); do_land(g, att, "Ice Highway", 0, 0); moved(g, att, 51); do_land(g, att, "Topica Snowfield", 0, 0); moved(g, att, 51); do_land(g, att, "Glug's Volcano", 0, 0); moved(g, att, 51); do_land(g, att, "Platonic Sea", 0, 0); moved(g, att, 51); do_land(g, att, "Slave City", 0, 0); moved(g, att, 51); do_land(g, att, "Acturian Woods", 0, 0); moved(g, att, 51); do_land(g, att, "Desolate Demesne", 0, 0); moved(g, att, 51); do_land(g, att, "Chateau Ishran", 0, 0); moved(g, att, 51); do_land(g, att, "Wyrm Spine Mountain", 0, 0); moved(g, att, 51); do_land(g, att, "Cloud Ruler Temple", 0, 0); moved(g, att, 51); do_land(g, att, "Koltoan Mine", 0, 0);), false, 1},
    {"landing everywhere from 52", "look", false, SETUP(for (room_vnum r : {8006, 8300, 8400, 8447, 8500, 8600, 8700, 8800, 8900, 8954, 9200, 9700, 9855, 9864, 9900, 9949}) room_stock(g, r); moved(g, att, 52); do_land(g, att, "Tiranoc City", 0, 0); moved(g, att, 52); do_land(g, att, "Great Oroist Temple", 0, 0); moved(g, att, 52); do_land(g, att, "Elzthuan Forest", 0, 0); moved(g, att, 52); do_land(g, att, "Mazori Farm", 0, 0); moved(g, att, 52); do_land(g, att, "Dres", 0, 0); moved(g, att, 52); do_land(g, att, "Colvian Farm", 0, 0); moved(g, att, 52); do_land(g, att, "St Alucia", 0, 0); moved(g, att, 52); do_land(g, att, "Meridius Memorial", 0, 0); moved(g, att, 52); do_land(g, att, "Desert of Illusion", 0, 0); moved(g, att, 52); do_land(g, att, "Plains of Confusion", 0, 0); moved(g, att, 52); do_land(g, att, "Turlon Fair", 0, 0); moved(g, att, 52); do_land(g, att, "Wetlands", 0, 0); moved(g, att, 52); do_land(g, att, "Kerberos", 0, 0); moved(g, att, 52); do_land(g, att, "Shaeras Mansion", 0, 0); moved(g, att, 52); do_land(g, att, "Slavinus Ravine", 0, 0); moved(g, att, 52); do_land(g, att, "Furian Citadel", 0, 0);), false, 1},
    {"landing everywhere from 53", "look", false, SETUP(for (room_vnum r : {2226, 2600, 2616, 2709, 2800, 2899, 2615}) room_stock(g, r); moved(g, att, 53); do_land(g, att, "Vegetos City", 0, 0); moved(g, att, 53); do_land(g, att, "Blood Dunes", 0, 0); moved(g, att, 53); do_land(g, att, "Ancestral Mountains", 0, 0); moved(g, att, 53); do_land(g, att, "Destopa Swamp", 0, 0); moved(g, att, 53); do_land(g, att, "Pride forest", 0, 0); moved(g, att, 53); do_land(g, att, "Pride Tower", 0, 0); moved(g, att, 53); do_land(g, att, "Ruby Cave", 0, 0);), false, 1},
    {"landing everywhere from 54", "look", false, SETUP(for (room_vnum r : {11600, 10182, 10474, 13300, 10203, 10922}) room_stock(g, r); moved(g, att, 54); do_land(g, att, "Senzu Village", 0, 0); moved(g, att, 54); do_land(g, att, "Guru's House", 0, 0); moved(g, att, 54); do_land(g, att, "Crystalline Cave", 0, 0); moved(g, att, 54); do_land(g, att, "Elder Village", 0, 0); moved(g, att, 54); do_land(g, att, "Frieza's Ship", 0, 0); moved(g, att, 54); do_land(g, att, "Kakureta Village", 0, 0);), false, 1},
    {"landing everywhere from 55", "look", false, SETUP(for (room_vnum r : {12010, 12103, 12300, 12400, 12480}) room_stock(g, r); moved(g, att, 55); do_land(g, att, "Haven City", 0, 0); moved(g, att, 55); do_land(g, att, "Serenity Lake", 0, 0); moved(g, att, 55); do_land(g, att, "Kaiju Forest", 0, 0); moved(g, att, 55); do_land(g, att, "Ortusian Temple", 0, 0); moved(g, att, 55); do_land(g, att, "Silent Glade", 0, 0);), false, 1},
    {"landing everywhere from 56", "look", false, SETUP(for (room_vnum r : {14008, 14100, 14200, 14300}) room_stock(g, r); moved(g, att, 56); do_land(g, att, "Yardra City", 0, 0); moved(g, att, 56); do_land(g, att, "Jade Forest", 0, 0); moved(g, att, 56); do_land(g, att, "Jade Cliffs", 0, 0); moved(g, att, 56); do_land(g, att, "Mount Valaria", 0, 0);), false, 1},
    {"landing everywhere from 198", "look", false, SETUP(for (room_vnum r : {17531, 7950, 17420}) room_stock(g, r); moved(g, att, 198); do_land(g, att, "Cerria Colony", 0, 0); moved(g, att, 198); do_land(g, att, "Crystalline Forest", 0, 0); moved(g, att, 198); do_land(g, att, "Fistarl Volcano", 0, 0);), false, 1},
    {"landing everywhere from 57", "look", false, SETUP(for (room_vnum r : {3412, 3520, 19600}) room_stock(g, r); moved(g, att, 57); do_land(g, att, "Utatlan City", 0, 0); moved(g, att, 57); do_land(g, att, "Zenith Jungle", 0, 0); moved(g, att, 57); do_land(g, att, "Ancient Castle", 0, 0);), false, 1},
    {"landing everywhere from 58", "look", false, SETUP(for (room_vnum r : {14904, 15655}) room_stock(g, r); moved(g, att, 58); do_land(g, att, "Aquis City", 0, 0); moved(g, att, 58); do_land(g, att, "Yunkai Pirate Base", 0, 0);), false, 1},
    {"landing everywhere from 59", "look", false, SETUP(for (room_vnum r : {16009, 16544, 16600}) room_stock(g, r); moved(g, att, 59); do_land(g, att, "Janacre", 0, 0); moved(g, att, 59); do_land(g, att, "Arlian Wasteland", 0, 0); moved(g, att, 59); do_land(g, att, "Arlia Mine", 0, 0);), false, 1},

    /* walking: do_move */
    {"about to blow up", "south", false, SETUP(char_plr_set(att, player_flag::selfd, true);), false, 1},
    {"liquefied", "south", false, SETUP(char_aff_set(att, aff_flag::liquefied, true);), false, 1},
    {"fully charged", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att));), false, 1},
    {"half charged", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .505);), false, 1},
    {"half charged, concentrating", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .505); char_set_skills_base(att, SKILL_CONCENTRATION, 100);), false, 1},
    {"well charged", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .45);), false, 1},
    {"well charged, concentrating", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .45); char_set_skills_base(att, SKILL_CONCENTRATION, 80);), false, 1},
    {"a third charged", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .35);), false, 1},
    {"a quarter charged", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .25);), false, 1},
    {"a quarter charged, concentrating", "south", false, SETUP(char_set_charge(att, getMaxKI(g, att) * .25); char_set_skills_base(att, SKILL_CONCENTRATION, 60);), false, 1},
    {"uncharged", "south", false, SETUP(char_set_charge(att, 0);), false, 1},
    {"drunk", "south", false, SETUP(char_set_charge(att, 0); char_set_conditions(g, att, DRUNK, 12);), false, 4},
    {"fighting", "south", true, SETUP(char_set_charge(att, 0);), false, 2},
    {"piloting nothing", "south", false, SETUP(char_set_charge(att, 0); char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"in a healing tank", "south", false, SETUP(char_set_charge(att, 0); char_plr_set(att, player_flag::healt, true);), false, 1},
    {"controlling an attack", "south", false, SETUP(char_set_charge(att, 0); { obj_data *ball = thing_named(g, "ball", ITEM_OTHER); obj_set_kicharge(ball, 10); obj_set_user(ball, att); }), false, 1},
    {"limbless", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 0, 0, 0, 0);), false, 1},
    {"absorbing", "south", false, SETUP(char_set_charge(att, 0); absorbed_by(victim, att);), false, 1},
    {"being absorbed", "south", false, SETUP(char_set_charge(att, 0); absorbed_by(att, victim); char_set_skills_base(victim, SKILL_ABSORB, 50);), false, 6},
    {"eavesdropping", "south", false, SETUP(char_set_charge(att, 0); char_set_listenroom(att, 300);), false, 1},
    {"watching the arena", "south", false, SETUP(char_set_charge(att, 0); char_prf_set(g, att, pref_flag::arenawatch, true);), false, 1},
    {"in the space between", "south", false, SETUP(char_set_charge(att, 0); room_set_flag(g, ARENA, room_flag::space, true);), false, 1},
    {"in gravity 10", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 10);), false, 1},
    {"a bardock pupil in gravity 10", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 10); char_set_class(att, CLASS_BARDOCK);), false, 1},
    {"in gravity 20", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 20);), false, 1},
    {"in gravity 30", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 30);), false, 1},
    {"in gravity 40", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 40);), false, 1},
    {"in gravity 50", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 50);), false, 1},
    {"in gravity 100", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 100);), false, 1},
    {"in gravity 200", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 200);), false, 1},
    {"in gravity 300", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 300);), false, 1},
    {"in gravity 400", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 400);), false, 1},
    {"in gravity 500", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 500);), false, 1},
    {"in gravity 1000", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 1000);), false, 1},
    {"in gravity 5000", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 5000);), false, 1},
    {"in gravity 10000", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 10000);), false, 1},
    {"in gravity 15", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 15);), false, 1},
    {"legless, with a worn left arm", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 0, 30, 0, 0);), false, 2},
    {"legless, with a ruined left arm", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 0, 3, 0, 0);), false, 2},
    {"legless, with a worn right arm", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 30, 0, 0, 0);), false, 2},
    {"legless, with a ruined right arm", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 3, 0, 0, 0);), false, 2},
    {"legless, with worn arms", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 30, 30, 0, 0);), false, 2},
    {"legless, with ruined arms", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 3, 3, 0, 0);), false, 2},
    {"legless, with good arms", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 0, 0);), false, 1},
    {"on a worn left leg", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 0, 30);), false, 2},
    {"on a ruined left leg", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 0, 3);), false, 2},
    {"on a worn right leg", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 30, 0);), false, 2},
    {"on a ruined right leg", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 3, 0);), false, 2},
    {"on a good right leg", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 90, 0);), false, 1},
    {"flying, legless", "south", false, SETUP(char_set_charge(att, 0); limbs(att, 90, 90, 0, 0); flying(att);), false, 1},
    {"resting", "south", false, SETUP(char_set_charge(att, 0); char_set_position(att, POS_RESTING);), false, 1},
    {"resting on a bed", "south", false, SETUP(char_set_charge(att, 0); seated_on(g, att, ITEM_BED, POS_RESTING);), false, 1},
    {"sitting", "south", false, SETUP(char_set_charge(att, 0); char_set_position(att, POS_SITTING);), false, 1},
    {"sitting on a chair", "south", false, SETUP(char_set_charge(att, 0); seated_on(g, att, ITEM_CHAIR);), false, 1},
    {"in character", "south", false, SETUP(char_set_charge(att, 0); char_set_rdisplay(att, "is here, smiling");), false, 1},
    {"in no character", "south", false, SETUP(char_set_charge(att, 0); char_set_rdisplay(att, "Empty");), false, 1},
    {"watching the dummy go", "look", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { do_move(g, c, "", 0, SCMD_SOUTH); break; }), false, 1},

    /* walking: perform_move */
    {"", "north", false, SETUP(char_set_charge(att, 0);), false, 1},
    {"by a hidden door", "south", false, SETUP(char_set_charge(att, 0); door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::secret);), false, 1},
    {"by a closed door", "south", false, SETUP(char_set_charge(att, 0); door_south(g, exit_flag::isdoor | exit_flag::closed);), false, 1},
    {"by a closed nameless door", "south", false, SETUP(char_set_charge(att, 0); door_south(g, exit_flag::isdoor | exit_flag::closed, "");), false, 1},
    {"by a way to the void", "east", false, SETUP(char_set_charge(att, 0); exit_to(g, EAST, 0);), false, 1},
    {"by a way to limbo", "east", false, SETUP(char_set_charge(att, 0); exit_to(g, EAST, 1);), false, 1},
    {"by a way nowhere", "east", false, SETUP(char_set_charge(att, 0); exit_to(g, EAST, NOWHERE);), false, 1},
    {"by a glacial wall", "south", false, SETUP(char_set_charge(att, 0); obj_set_cost(thing_named(g, "wall", ITEM_OTHER, {}, 79), SOUTH);), false, 1},
    {"by a glacial wall the other way", "south", false, SETUP(char_set_charge(att, 0); obj_set_cost(thing_named(g, "wall", ITEM_OTHER, {}, 79), NORTH);), false, 1},
    {"followed", "south", false, SETUP(char_set_charge(att, 0); group(g, att, cast.helper);), false, 1},
    {"followed by a sleeper", "south", false, SETUP(char_set_charge(att, 0); group(g, att, cast.helper); asleep(cast.helper);), false, 1},
    {"in a flash, followed", "south", false, SETUP(char_set_charge(att, 0); add_follower(g, cast.helper, att); char_aff_set(att, aff_flag::zanzoken, true);), false, 1},
    {"in a flash, followed in a flash", "south", false, SETUP(char_set_charge(att, 0); add_follower(g, cast.helper, att); char_aff_set(att, aff_flag::zanzoken, true); char_aff_set(cast.helper, aff_flag::zanzoken, true);), false, 1},
    {"in a flash, followed by the group", "south", false, SETUP(char_set_charge(att, 0); group(g, att, cast.helper); char_aff_set(att, aff_flag::zanzoken, true);), false, 1},
    {"sneaking", "south", false, SETUP(char_set_charge(att, 0); char_aff_set(att, aff_flag::sneak, true); char_set_skills_base(att, SKILL_MOVE_SILENTLY, 50);), false, 4},
    {"sneaking clumsily", "south", false, SETUP(char_set_charge(att, 0); char_aff_set(att, aff_flag::sneak, true);), false, 4},
    {"sneaking clumsily, without slots", "south", false, SETUP(char_set_charge(att, 0); char_aff_set(att, aff_flag::sneak, true); char_set_skill_slots(att, 0);), false, 1},
    {"hiding", "south", false, SETUP(char_set_charge(att, 0); char_aff_set(att, aff_flag::hide, true); char_set_skills_base(att, SKILL_HIDE, 50);), false, 2},

    /* walking: do_simple_move */
    {"charmed by the dummy", "south", false, SETUP(char_set_charge(att, 0); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { add_follower(g, att, c); break; } char_aff_set(att, aff_flag::charm, true);), false, 1},
    {"in the sky, flightless", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_sector_type(g, ARENA, SECT_FLYING); exit_to(g, DOWN, 300);), false, 1},
    {"flightless, below the sky", "up", false, SETUP(char_set_charge(att, 0); room_to(g, UP, 81, SECT_FLYING);), false, 1},
    {"flying, below the sky", "up", false, SETUP(char_set_charge(att, 0); room_to(g, UP, 81, SECT_FLYING); flying(att); focused(att);), false, 1},
    {"flying spent, below the sky", "up", false, SETUP(char_set_charge(att, 0); room_to(g, UP, 81, SECT_FLYING); flying(att); drained(g, att);), false, 1},
    {"an android flying, below the sky", "up", false, SETUP(char_set_charge(att, 0); room_to(g, UP, 81, SECT_FLYING); flying(att); char_set_race(att, RACE_ANDROID);), false, 1},
    {"with a flying charm, below the sky", "up", false, SETUP(char_set_charge(att, 0); room_to(g, UP, 81, SECT_FLYING); obj_aff_set(holding(g, att, "feather"), aff_flag::flying, true);), false, 1},
    {"walking anywhere, below the sky", "up", false, SETUP(char_set_charge(att, 0); room_to(g, UP, 81, SECT_FLYING); admin_set(g, att, ADMLVL_IMPL);), false, 1},
    {"by the sea", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_WATER_NOSWIM);), false, 1},
    {"tired, by the sea", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_WATER_NOSWIM); laden(g, att); tired(g, att, 0);), false, 1},
    {"a kanassan, by the sea", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_WATER_NOSWIM); char_set_race(att, RACE_KANASSAN);), false, 1},
    {"an icer, by the sea", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_WATER_NOSWIM); char_set_race(att, RACE_ICER);), false, 1},
    {"flying, by the sea", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_WATER_NOSWIM); flying(att); focused(att);), false, 1},
    {"in space", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_flag(g, ARENA, room_flag::space, true); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"spent, in space", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_flag(g, ARENA, room_flag::space, true); admin_set(g, att, ADMLVL_IMMORT); drained(g, att);), false, 1},
    {"an android in space", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_flag(g, ARENA, room_flag::space, true); char_set_race(att, RACE_ANDROID); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"by deep water", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_UNDERWATER);), false, 1},
    {"breathless, by deep water", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_UNDERWATER); drained(g, att);), false, 1},
    {"breathless and hurt, by deep water", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_UNDERWATER); drained(g, att); decCurHealthPercent(g, att, 0.97, 1);), false, 1},
    {"a gilled majin, by deep water", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_UNDERWATER); char_set_race(att, RACE_MAJIN); drained(g, att);), false, 1},
    {"a tsuna's student, by deep water", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81, SECT_UNDERWATER); char_set_class(att, CLASS_TSUNA); group(g, att, cast.helper);), false, 1},
    {"a beginner", "south", false, SETUP(char_set_charge(att, 0); char_set_class_level(att, 1);), false, 1},
    {"exhausted", "south", false, SETUP(char_set_charge(att, 0); room_set_gravity(g, ARENA, 15); tired(g, att, 0);), false, 1},
    {"exhausted, following", "south", false, SETUP(char_set_charge(cast.helper, 0); room_set_gravity(g, ARENA, 15); add_follower(g, att, cast.helper); tired(g, att, 0); command_interpreter(g, cast.helper, "south");), false, 1},
    {"flying unskilled", "south", false, SETUP(char_set_charge(att, 0); flying(att); focused(att, 0);), false, 1},
    {"flying, concentrating", "south", false, SETUP(char_set_charge(att, 0); flying(att); char_set_skills_base(att, SKILL_CONCENTRATION, 50);), false, 1},
    {"flying, focused", "south", false, SETUP(char_set_charge(att, 0); flying(att); focused(att);), false, 1},
    {"flying, focused and concentrating", "south", false, SETUP(char_set_charge(att, 0); flying(att); focused(att); char_set_skills_base(att, SKILL_CONCENTRATION, 50);), false, 1},
    {"flying, spent", "south", false, SETUP(char_set_charge(att, 0); flying(att); drained(g, att);), false, 1},
    {"by a hard way", "south", false, SETUP(char_set_charge(att, 0); { auto *e = room_get_dir_option(g, ARENA, SOUTH); e->dcskill = SKILL_SURVIVAL; e->dcmove = 500; }), false, 1},
    {"by a hard way, flying", "south", false, SETUP(char_set_charge(att, 0); flying(att); focused(att); { auto *e = room_get_dir_option(g, ARENA, SOUTH); e->dcskill = SKILL_SURVIVAL; e->dcmove = 500; }), false, 1},
    {"by a way a skill eases", "south", false, SETUP(char_set_charge(att, 0); { auto *e = room_get_dir_option(g, ARENA, SOUTH); e->dcskill = SKILL_SURVIVAL; e->dcmove = -50; }), false, 1},
    {"in an atrium", "south", false, SETUP(char_set_charge(att, 0); room_set_flag(g, ARENA, room_flag::atrium, true);), false, 1},
    {"by a tunnel", "south", false, SETUP(char_set_charge(att, 0); room_set_flag(g, 300, room_flag::tunnel, true); CONFIG_TUNNEL_SIZE = 1; away(g, victim);), false, 1},
    {"by a wide tunnel", "south", false, SETUP(char_set_charge(att, 0); room_set_flag(g, 300, room_flag::tunnel, true); CONFIG_TUNNEL_SIZE = 1; away(g, victim); CONFIG_TUNNEL_SIZE = 2; away(g, cast.helper);), false, 1},
    {"by a god's room", "south", false, SETUP(char_set_charge(att, 0); room_set_flag(g, 300, room_flag::godroom, true);), false, 1},
    {"by a zone for the seasoned", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 1960); room_get_zone(g, 1960)->min_level = 50;), false, 1},
    {"by a zone for beginners", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 1960); room_get_zone(g, 1960)->max_level = 10;), false, 1},
    {"by a closed zone", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 1960); zone_set_flag(room_get_zone(g, 1960), zone_flag::closed, true);), false, 1},
    {"an immortal, by a zone closed to all", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 1960); zone_set_flag(room_get_zone(g, 1960), zone_flag::noimmort, true); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"an immortal, by a quest zone", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 1960); zone_set_flag(room_get_zone(g, 1960), zone_flag::quest, true); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"flying", "south", false, SETUP(char_set_charge(att, 0); flying(att); focused(att);), false, 1},
    {"dragging someone", "south", false, SETUP(char_set_charge(att, 0); dragging(att, victim); asleep(victim);), false, 1},
    {"dragging someone on a bed", "south", false, SETUP(char_set_charge(att, 0); dragging(att, victim); seated_on(g, victim, ITEM_BED, POS_SLEEPING);), false, 1},
    {"dragging someone knocked out", "south", false, SETUP(char_set_charge(att, 0); dragging(att, victim); char_aff_set(victim, aff_flag::knocked, true);), false, 1},
    {"dragging the dummy", "south", false, SETUP(char_set_charge(att, 0); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { dragging(att, c); break; }), false, 1},
    {"carrying someone", "south", false, SETUP(char_set_charge(att, 0); carrying(g, att, victim);), false, 1},
    {"carrying someone knocked out", "south", false, SETUP(char_set_charge(att, 0); carrying(g, att, victim); char_aff_set(victim, aff_flag::knocked, true);), false, 1},
    {"into another zone", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 1960);), false, 1},
    {"by lava", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_geffect(g, 81, 6);), false, 1},
    {"hurt, by lava", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_geffect(g, 81, 6); decCurHealthPercent(g, att, 0.97, 1);), false, 1},
    {"a demon, by lava", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_geffect(g, 81, 6); char_set_race(att, RACE_DEMON);), false, 1},
    {"dragging someone, by lava", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_geffect(g, 81, 6); dragging(att, victim); asleep(victim);), false, 1},
    {"dragging someone hurt, by lava", "east", false, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); room_set_geffect(g, 81, 6); dragging(att, victim); asleep(victim); decCurHealthPercent(g, victim, 1, 0);), false, 1},
    {"fighting, by a way", "east", true, SETUP(char_set_charge(att, 0); room_to(g, EAST, 81); char_set_class_level(att, 100);), false, 4},

    /* doors */
    {"", "open", false, SETUP(), false, 1},
    {"", "open door", false, SETUP(), false, 1},
    {"", "open door sideways", false, SETUP(), false, 1},
    {"", "open door east", false, SETUP(), false, 1},
    {"by a gate", "open door south", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed, "gate");), false, 1},
    {"by a nameless door", "open door south", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed, "");), false, 1},
    {"by a closed door", "open door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed);), false, 1},
    {"by a closed door", "open door s", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed);), false, 1},
    {"by a closed door alone", "open door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed, "door", -1, false);), false, 1},
    {"by an open door", "open door", false, SETUP(door_south(g, exit_flag::isdoor);), false, 1},
    {"by an open door", "close door", false, SETUP(door_south(g, exit_flag::isdoor);), false, 1},
    {"by a closed door", "close door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed);), false, 1},
    {"by a locked door", "open door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked);), false, 1},
    {"by a locked door", "unlock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72);), false, 1},
    {"with the key, by a locked door", "unlock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); keyed(g, att, 72);), false, 1},
    {"with the key worn, by a locked door", "unlock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); scouter_on(g, att); renumber(g, char_get_equipment(att, WEAR_EYE), 72);), false, 1},
    {"by a door that needs no key", "unlock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 1);), false, 1},
    {"keyless, an implementor, by a locked door", "unlock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); admin_set(g, att, ADMLVL_IMPL);), false, 1},
    {"by a closed door", "unlock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed, "door", 72);), false, 1},
    {"with the key, by a closed door", "lock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed, "door", 72); keyed(g, att, 72);), false, 1},
    {"with the key, by a locked door", "lock door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); keyed(g, att, 72);), false, 1},
    {"by a wall", "open wall", false, SETUP(door_south(g, {}, "wall");), false, 1},
    {"unskilled, by a locked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72);), false, 1},
    {"skilled without a kit, by a locked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); char_set_skills_base(att, SKILL_OPEN_LOCK, 100);), false, 1},
    {"with a broken kit, by a locked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); char_set_skills_base(att, SKILL_OPEN_LOCK, 100); obj_extra_set(carries_as(g, att, 18, "kit"), extra_flag::broken, true);), false, 1},
    {"a picker, by a keyless lock", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", NOTHING); picker(g, att);), false, 1},
    {"a picker, by a pickproof lock", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked | exit_flag::pickproof, "door", 72); picker(g, att);), false, 1},
    {"a tired picker, by a locked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); picker(g, att); tired(g, att, 0);), false, 1},
    {"a picker, by a locked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); picker(g, att);), false, 4},
    {"a poor picker, by a locked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72); picker(g, att, 1);), false, 2},
    {"a picker, by a hard lock", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed | exit_flag::locked, "door", 72)->dclock = 1; picker(g, att);), false, 2},
    {"a picker, by an unlocked door", "pick door", false, SETUP(door_south(g, exit_flag::isdoor | exit_flag::closed, "door", 72); picker(g, att);), false, 1},
    {"with a closed bag", "open bag", false, SETUP(obj_set_value(holding(g, att, "bag", ITEM_CONTAINER), VAL_CONTAINER_FLAGS, cont_value(container_flag::closeable, container_flag::closed));), false, 1},
    {"with an open bag", "close bag", false, SETUP(obj_set_value(holding(g, att, "bag", ITEM_CONTAINER), VAL_CONTAINER_FLAGS, cont_value(container_flag::closeable));), false, 1},
    {"with a locked bag", "unlock bag", false, SETUP(obj_set_value(holding(g, att, "bag", ITEM_CONTAINER), VAL_CONTAINER_FLAGS, cont_value(container_flag::closeable, container_flag::closed, container_flag::locked)); keyed(g, att, 0);), false, 1},
    {"with a closed bag and its key", "lock bag", false, SETUP({ obj_data *b = holding(g, att, "bag", ITEM_CONTAINER); obj_set_value(b, VAL_CONTAINER_FLAGS, cont_value(container_flag::closeable, container_flag::closed)); obj_set_value(b, VAL_KEY_KEYCODE, 72); } keyed(g, att, 72);), false, 1},
    {"a picker, with a locked bag", "pick bag", false, SETUP({ obj_data *b = holding(g, att, "bag", ITEM_CONTAINER); obj_set_value(b, VAL_CONTAINER_FLAGS, cont_value(container_flag::closeable, container_flag::closed, container_flag::locked)); obj_set_value(b, VAL_KEY_KEYCODE, 72); } picker(g, att);), false, 4},
    {"by a closed chest", "open chest", false, SETUP(obj_set_value(cast.chest, VAL_CONTAINER_FLAGS, cont_value(container_flag::closeable, container_flag::closed));), false, 1},
    {"with a rock", "open rock", false, SETUP(holding(g, att, "rock");), false, 1},
    {"with an unclosable bag", "close bag", false, SETUP(holding(g, att, "bag", ITEM_CONTAINER);), false, 1},

    /* ship hatches and the ships they open from */
    {"aboard a ship, by its hatch", "open hatch", false, SETUP(ship_at(g, att, 19250, true);), false, 1},
    {"aboard a ship in space, by its hatch", "open hatch", false, SETUP(ship_at(g, att, 19250, true, true);), false, 1},
    {"aboard a car, by its hatch", "open hatch", false, SETUP(ship_at(g, att, 19150, true);), false, 1},
    {"aboard a ship, by its open hatch", "close hatch", false, SETUP(ship_at(g, att, 19250, true, false, false);), false, 1},
    {"aboard a ship in space, by its open hatch", "close hatch", false, SETUP(ship_at(g, att, 19250, true, true, false);), false, 1},
    {"aboard a car, by its open hatch", "close hatch", false, SETUP(ship_at(g, att, 19150, true, false, false);), false, 1},
    {"aboard a ship, by its hatch", "lock hatch", false, SETUP(ship_at(g, att, 19250, true);), false, 1},
    {"aboard a ship, by its locked hatch", "unlock hatch", false, SETUP(ship_at(g, att, 19250, true, false, true, true);), false, 1},
    {"a picker aboard a ship, by its locked hatch", "pick hatch", false, SETUP(ship_at(g, att, 19250, true, false, true, true); picker(g, att);), false, 1},
    {"by a ship", "open ship", false, SETUP(ship_at(g, att, 19250, false);), false, 1},
    {"by a ship in space", "open ship", false, SETUP(ship_at(g, att, 19250, false, true);), false, 1},
    {"by a car", "open ship", false, SETUP(ship_at(g, att, 19150, false);), false, 1},
    {"by an open ship", "close ship", false, SETUP(ship_at(g, att, 19250, false, false, false);), false, 1},
    {"by an open ship in space", "close ship", false, SETUP(ship_at(g, att, 19250, false, true, false);), false, 1},
    {"by an open car", "close ship", false, SETUP(ship_at(g, att, 19150, false, false, false);), false, 1},
    {"by a ship", "lock ship", false, SETUP(ship_at(g, att, 19250, false);), false, 1},
    {"by a locked ship", "unlock ship", false, SETUP(ship_at(g, att, 19250, false, false, true, true);), false, 1},
    {"a picker by a locked ship", "pick ship", false, SETUP(ship_at(g, att, 19250, false, false, true, true); picker(g, att);), false, 1},
    {"aboard a ship, by its open hatch", "leave", false, SETUP(ship_at(g, att, 19250, true, false, false);), false, 1},
    {"aboard a ship, by its closed hatch", "leave", false, SETUP(ship_at(g, att, 19250, true);), false, 1},
    {"by an open ship", "enter ship", false, SETUP(ship_at(g, att, 19250, false, false, false);), false, 1},

    /* enter */
    {"", "enter", false, SETUP(), false, 1},
    {"indoors", "enter", false, SETUP(room_set_flag(g, ARENA, room_flag::indoors, true);), false, 1},
    {"by a way in", "enter", false, SETUP(room_set_flag(g, 300, room_flag::indoors, true);), false, 1},
    {"by a closed way in", "enter", false, SETUP(room_set_flag(g, 300, room_flag::indoors, true); door_south(g, exit_flag::isdoor | exit_flag::closed);), false, 1},
    {"", "enter hut", false, SETUP(), false, 1},
    {"by a hut's door", "enter hut", false, SETUP(door_south(g, exit_flag::isdoor, "hut door");), false, 1},
    {"by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}});), false, 1},
    {"with a portal", "enter portal", false, SETUP(obj_set_value(holding(g, att, "portal", ITEM_PORTAL), VAL_PORTAL_DEST, 300);), false, 1},
    {"wearing a portal", "enter portal", false, SETUP(obj_set_type_flag(scouter_on(g, att), ITEM_PORTAL); obj_set_name(char_get_equipment(att, WEAR_EYE), "portal"); obj_set_value(char_get_equipment(att, WEAR_EYE), VAL_PORTAL_DEST, 300);), false, 1},
    {"by a closed portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}, {VAL_CONTAINER_FLAGS, cont_value(container_flag::closed)}});), false, 1},
    {"by a portal to nowhere", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, NOWHERE}});), false, 1},
    {"by a portal to a gone room", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 31999}});), false, 1},
    {"by a rock", "enter rock", false, SETUP(thing_named(g, "rock", ITEM_OTHER);), false, 1},
    {"by a pod", "enter pod", false, SETUP(room_stock(g, 45001); thing_named(g, "pod", ITEM_VEHICLE, {{VAL_PORTAL_DEST, 45001}});), false, 1},
    {"by a full pod", "enter pod", false, SETUP(room_stock(g, 45001); thing_named(g, "pod", ITEM_VEHICLE, {{VAL_PORTAL_DEST, 45001}}); moved(g, victim, 45001);), false, 1},
    {"followed, by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); group(g, att, cast.helper);), false, 1},
    {"dragging and carrying, by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); dragging(att, victim); asleep(victim); carrying(g, att, cast.helper); seated_on(g, victim, ITEM_BED, POS_SLEEPING);), false, 1},
    {"dragging the dummy, by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { dragging(att, c); break; }), false, 1},
    {"exhausted, by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); room_set_gravity(g, ARENA, 15); tired(g, att, 0);), false, 1},
    {"charmed, by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); add_follower(g, att, victim); char_aff_set(att, aff_flag::charm, true);), false, 1},
    {"by a portal to a god's room", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); room_set_flag(g, 300, room_flag::godroom, true);), false, 1},
    {"by a portal to a tunnel", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); room_set_flag(g, 300, room_flag::tunnel, true); CONFIG_TUNNEL_SIZE = 1; away(g, victim);), false, 1},
    {"in an atrium, by a portal", "enter portal", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); room_set_flag(g, ARENA, room_flag::atrium, true);), false, 1},

    /* leave */
    {"in a healing tank", "leave", false, SETUP(char_plr_set(att, player_flag::healt, true);), false, 1},
    {"outside", "leave", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD);), false, 1},
    {"indoors", "leave", false, SETUP(room_set_flag(g, ARENA, room_flag::indoors, true); room_set_flag(g, 300, room_flag::indoors, true);), false, 1},
    {"indoors, by a way out", "leave", false, SETUP(room_set_flag(g, ARENA, room_flag::indoors, true);), false, 1},
    {"by a portal out", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}});), false, 1},
    {"by a closed portal out", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}, {VAL_CONTAINER_FLAGS, cont_value(container_flag::closed)}});), false, 1},
    {"by a portal out to nowhere", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, NOWHERE}});), false, 1},
    {"by a portal out to a gone room", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 31999}});), false, 1},
    {"by a hatch to no ship", "leave", false, SETUP(thing_named(g, "hatch", ITEM_HATCH, {{VAL_HATCH_DEST, 72}});), false, 1},
    {"by a hatch", "leave", false, SETUP(thing_named(g, "hatch", ITEM_HATCH, {{VAL_HATCH_DEST, 71}}); obj_to_room(g, read_object(g, 71), 300); obj_set_action_description(first_obj(g, room_get_contents(g, ARENA)), "You climb out.");), false, 1},
    {"by a hatch of a ship nowhere", "leave", false, SETUP(thing_named(g, "hatch", ITEM_HATCH, {{VAL_HATCH_DEST, 71}}); obj_to_char(g, read_object(g, 71), cast.android);), false, 1},
    {"followed, dragging and carrying, by a portal out", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); group(g, att, cast.helper); dragging(att, victim); seated_on(g, victim, ITEM_BED, POS_SLEEPING);), false, 1},
    {"carrying someone on a bed, by a portal out", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); carrying(g, att, victim); seated_on(g, victim, ITEM_BED, POS_SLEEPING);), false, 1},
    {"dragging the dummy, by a portal out", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { dragging(att, c); break; }), false, 1},
    {"exhausted, by a portal out", "leave", false, SETUP(thing_named(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_DEST, 300}}); room_set_gravity(g, ARENA, 15); tired(g, att, 0);), false, 1},

    /* fly */
    {"absorbing", "fly", false, SETUP(absorbed_by(victim, att);), false, 1},
    {"in a healing tank", "fly", false, SETUP(char_plr_set(att, player_flag::healt, true);), false, 1},
    {"piloting", "fly", false, SETUP(char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"unfocused", "fly", false, SETUP(focused(att, 10);), false, 1},
    {"an android", "fly", false, SETUP(char_set_race(att, RACE_ANDROID);), false, 1},
    {"focused, flying", "fly", false, SETUP(focused(att); flying(att);), false, 1},
    {"focused, flying in the sky", "fly", false, SETUP(focused(att); flying(att); room_set_sector_type(g, ARENA, SECT_FLYING); exit_to(g, DOWN, 300);), false, 1},
    {"focused, flying in the sky above the sea", "fly", false, SETUP(focused(att); flying(att); room_set_sector_type(g, ARENA, SECT_FLYING); room_to(g, DOWN, 81, SECT_WATER_NOSWIM); laden(g, att);), false, 1},
    {"focused, flying in the sky above the sea, hurt", "fly", false, SETUP(focused(att); flying(att); room_set_sector_type(g, ARENA, SECT_FLYING); room_to(g, DOWN, 81, SECT_WATER_NOSWIM); decCurHealthPercent(g, att, 0.8, 1);), false, 1},
    {"focused, flying in the sky, carrying someone", "fly", false, SETUP(focused(att); flying(att); room_set_sector_type(g, ARENA, SECT_FLYING); exit_to(g, DOWN, 300); carrying(g, att, victim);), false, 1},
    {"focused, flying in space", "fly", false, SETUP(focused(att); flying(att); room_set_sector_type(g, ARENA, SECT_SPACE);), false, 1},
    {"focused, spent", "fly", false, SETUP(focused(att); drained(g, att);), false, 1},
    {"focused", "fly", false, SETUP(focused(att);), false, 1},
    {"focused, seated", "fly", false, SETUP(focused(att); seated_on(g, att, ITEM_CHAIR);), false, 1},
    {"focused", "fly high", false, SETUP(focused(att);), false, 1},
    {"focused, spent", "fly high", false, SETUP(focused(att); drained(g, att);), false, 1},
    {"focused, resting", "fly high", false, SETUP(focused(att); seated_on(g, att, ITEM_BED, POS_RESTING);), false, 1},
    {"focused", "fly sideways", false, SETUP(focused(att);), false, 1},
    {"focused, indoors", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::indoors, true);), false, 1},
    {"focused, weary", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); decCurKI(g, att, getMaxKI(g, att) * 0.95, 0);), false, 1},
    {"focused, fighting", "fly space", true, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att);), false, 1},
    {"focused, off any planet", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att);), false, 1},
    {"focused, on Earth", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::earth, true);), false, 1},
    {"focused, on Earth, blocked", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::earth, true); char_set_blocked(att, victim); char_set_blocks(victim, att);), false, 1},
    {"an android, on Earth", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); char_set_race(att, RACE_ANDROID); room_set_flag(g, ARENA, room_flag::earth, true);), false, 1},
    {"focused, on Cerria", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::cerria, true);), false, 1},
    {"focused, on Vegeta", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::vegeta, true);), false, 1},
    {"focused, on Kanassa", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::kanassa, true);), false, 1},
    {"focused, at Aquis's launchpad", "fly space", false, SETUP(focused(att); moved(g, att, 14904); room_set_sector_type(g, 14904, SECT_FIELD); room_set_flag(g, 14904, room_flag::kanassa, true);), false, 1},
    {"focused, on Frigid", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::frigid, true);), false, 1},
    {"focused, on Konack", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::konack, true);), false, 1},
    {"focused, on Namek", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::namek, true);), false, 1},
    {"focused, on Aether", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::aether, true);), false, 1},
    {"focused, on Yardrat", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::yardrat, true);), false, 1},
    {"focused, on Arlia", "fly space", false, SETUP(room_set_sector_type(g, ARENA, SECT_FIELD); focused(att); room_set_flag(g, ARENA, room_flag::arlia, true);), false, 1},
    {"focused, on Zennith", "fly space", false, SETUP(focused(att); moved(g, att, 3412); room_set_sector_type(g, 3412, SECT_FIELD);), false, 1},

    /* stand */
    {"knocked out", "stand", false, SETUP(char_aff_set(att, aff_flag::knocked, true);), false, 1},
    {"legless", "stand", false, SETUP(limbs(att, 90, 90, 0, 0);), false, 1},
    {"piloting", "stand", false, SETUP(char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"", "stand", false, SETUP(), false, 1},
    {"sitting", "stand", false, SETUP(char_set_position(att, POS_SITTING);), false, 1},
    {"sitting, fighting", "stand", true, SETUP(char_set_position(att, POS_SITTING);), false, 1},
    {"sitting on a chair", "stand", false, SETUP(seated_on(g, att, ITEM_CHAIR);), false, 1},
    {"sitting on a cushion", "stand", false, SETUP(seated_on(g, att, ITEM_OTHER, POS_SITTING, "cushion");), false, 1},
    {"laden, sitting on a cushion", "stand", false, SETUP(laden(g, att); seated_on(g, att, ITEM_OTHER, POS_SITTING, "cushion");), false, 1},
    {"resting", "stand", false, SETUP(char_set_position(att, POS_RESTING);), false, 1},
    {"resting on a chair", "stand", false, SETUP(seated_on(g, att, ITEM_CHAIR, POS_RESTING);), false, 1},
    {"resting on a fixed bed", "stand", false, SETUP(obj_wear_set(seated_on(g, att, ITEM_BED, POS_RESTING), wear_flag::take, false);), false, 1},
    {"floating", "stand", false, SETUP(char_set_position(att, POS_FIGHTING);), false, 1},
    {"watching the dummy stand", "stand", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { char_set_position(c, POS_SLEEPING); do_stand(g, c, "", 0, 0); break; }), false, 1},

    /* sit */
    {"piloting", "sit", false, SETUP(char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"in a healing tank", "sit", false, SETUP(char_plr_set(att, player_flag::healt, true);), false, 1},
    {"dragging someone", "sit", false, SETUP(dragging(att, victim); asleep(victim);), false, 1},
    {"carrying someone", "sit", false, SETUP(carrying(g, att, victim);), false, 1},
    {"flying", "sit", false, SETUP(flying(att); focused(att);), false, 1},
    {"", "sit", false, SETUP(), false, 1},
    {"resting", "sit", false, SETUP(char_set_position(att, POS_RESTING);), false, 1},
    {"fighting", "sit", true, SETUP(char_set_position(att, POS_FIGHTING);), false, 1},
    {"", "sit chair", false, SETUP(), false, 1},
    {"by a chair", "sit chair", false, SETUP(thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"by a bed", "sit bed", false, SETUP(thing_named(g, "bed", ITEM_BED);), false, 1},
    {"by a rock", "sit rock", false, SETUP(thing_named(g, "rock", ITEM_OTHER);), false, 1},
    {"by a spike", "sit spike", false, SETUP(thing_named(g, "spike", ITEM_CHAIR, {}, 65);), false, 1},
    {"by a chair taken", "sit chair", false, SETUP({ obj_data *c = thing_named(g, "chair", ITEM_CHAIR); obj_set_sitting(c, victim); char_set_sits(victim, c); }), false, 1},
    {"by a tiny chair", "sit chair", false, SETUP(obj_set_size(thing_named(g, "chair", ITEM_CHAIR), SIZE_FINE); char_set_size(att, SIZE_HUGE);), false, 1},
    {"on a chair, by another", "sit chair", false, SETUP(seated_on(g, att, ITEM_CHAIR, POS_SITTING, "stool"); thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"resting, by a chair", "sit chair", false, SETUP(char_set_position(att, POS_RESTING); thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"fighting, by a chair", "sit chair", true, SETUP(char_set_position(att, POS_FIGHTING); thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"sitting, by a chair", "sit chair", false, SETUP(char_set_position(att, POS_SITTING); thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"watching the dummy sit", "sit", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { char_set_position(c, POS_SLEEPING); do_sit(g, c, "", 0, 0); char_set_position(c, POS_STUNNED); do_sit(g, c, "", 0, 0); thing_named(g, "chair", ITEM_CHAIR); char_set_position(c, POS_SLEEPING); do_sit(g, c, "chair", 0, 0); char_set_position(c, POS_STUNNED); do_sit(g, c, "chair", 0, 0); break; }), false, 1},

    /* rest */
    {"piloting", "rest", false, SETUP(char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"at sea", "rest", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM);), false, 1},
    {"in a healing tank", "rest", false, SETUP(char_plr_set(att, player_flag::healt, true);), false, 1},
    {"behind a barrier", "rest", false, SETUP(char_aff_set(att, aff_flag::sanctuary, true); char_set_skills_base(att, SKILL_BARRIER, 50);), false, 1},
    {"sanctified", "rest", false, SETUP(char_aff_set(att, aff_flag::sanctuary, true);), false, 1},
    {"fighting", "rest", true, SETUP(), false, 1},
    {"fighting, unfought", "rest", false, SETUP(char_set_position(att, POS_FIGHTING);), false, 1},
    {"fighting, unfought, by a bed", "rest bed", false, SETUP(char_set_position(att, POS_FIGHTING); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"in kaioken", "rest", false, SETUP(char_set_kaioken(att, 2);), false, 1},
    {"dragging someone", "rest", false, SETUP(dragging(att, victim); asleep(victim);), false, 1},
    {"carrying someone", "rest", false, SETUP(carrying(g, att, victim);), false, 1},
    {"flying", "rest", false, SETUP(flying(att); focused(att);), false, 1},
    {"", "rest", false, SETUP(), false, 1},
    {"sitting", "rest", false, SETUP(char_set_position(att, POS_SITTING);), false, 1},
    {"resting", "rest", false, SETUP(char_set_position(att, POS_RESTING);), false, 1},
    {"on a chair", "rest", false, SETUP(seated_on(g, att, ITEM_CHAIR);), false, 1},
    {"on a bed", "rest", false, SETUP(seated_on(g, att, ITEM_BED);), false, 1},
    {"", "rest bed", false, SETUP(), false, 1},
    {"by a bed", "rest bed", false, SETUP(thing_named(g, "bed", ITEM_BED);), false, 1},
    {"by a chair", "rest chair", false, SETUP(thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"by a spike", "rest spike", false, SETUP(thing_named(g, "spike", ITEM_BED, {}, 65);), false, 1},
    {"by a bed taken", "rest bed", false, SETUP({ obj_data *c = thing_named(g, "bed", ITEM_BED); obj_set_sitting(c, victim); char_set_sits(victim, c); }), false, 1},
    {"by a tiny bed", "rest bed", false, SETUP(obj_set_size(thing_named(g, "bed", ITEM_BED), SIZE_FINE); char_set_size(att, SIZE_HUGE);), false, 1},
    {"on a chair, by a bed", "rest bed", false, SETUP(seated_on(g, att, ITEM_CHAIR); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"sitting, by a bed", "rest bed", false, SETUP(char_set_position(att, POS_SITTING); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"resting, by a bed", "rest bed", false, SETUP(char_set_position(att, POS_RESTING); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"watching the dummy rest", "rest", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { thing_named(g, "bed", ITEM_BED); for (int pos : {POS_SLEEPING, POS_STUNNED}) { char_set_position(c, pos); do_rest(g, c, "", 0, 0); char_set_position(c, pos); do_rest(g, c, "bed", 0, 0); } char_set_position(c, POS_FIGHTING); do_rest(g, c, "bed", 0, 0); break; }), false, 1},

    /* sleep */
    {"watching the arena", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_prf_set(g, att, pref_flag::arenawatch, true);), false, 1},
    {"an insomniac", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_set_bonuses(att, BONUS_INSOMNIAC, 1);), false, 1},
    {"at sea", "sleep", false, SETUP(char_set_sleeptime(att, 0); room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM);), false, 1},
    {"piloting", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_plr_set(att, player_flag::piloting, true);), false, 1},
    {"fighting", "sleep", true, SETUP(char_set_sleeptime(att, 0); ), false, 1},
    {"fighting, unfought", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_set_position(att, POS_FIGHTING);), false, 1},
    {"fighting, unfought, by a bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); char_set_position(att, POS_FIGHTING); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"asleep", "sleep", false, SETUP(char_set_sleeptime(att, 0); asleep(att);), false, 1},
    {"asleep, by a bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); asleep(att); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"in a healing tank", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_plr_set(att, player_flag::healt, true);), false, 1},
    {"powering up", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_plr_set(att, player_flag::powerup, true);), false, 1},
    {"behind a barrier", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_aff_set(att, aff_flag::sanctuary, true); char_set_skills_base(att, SKILL_BARRIER, 50);), false, 1},
    {"sanctified", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_aff_set(att, aff_flag::sanctuary, true);), false, 1},
    {"in kaioken", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_set_kaioken(att, 2);), false, 1},
    {"rested", "sleep", false, SETUP(char_set_sleeptime(att, 5);), false, 1},
    {"dragging someone", "sleep", false, SETUP(char_set_sleeptime(att, 0); dragging(att, victim); asleep(victim);), false, 1},
    {"carrying someone", "sleep", false, SETUP(char_set_sleeptime(att, 0); carrying(g, att, victim);), false, 1},
    {"flying", "sleep", false, SETUP(char_set_sleeptime(att, 0); flying(att); focused(att);), false, 1},
    {"", "sleep", false, SETUP(char_set_sleeptime(att, 0); ), false, 1},
    {"furious", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_plr_set(att, player_flag::fury, true);), false, 1},
    {"resting", "sleep", false, SETUP(char_set_sleeptime(att, 0); char_set_position(att, POS_RESTING);), false, 1},
    {"on a chair", "sleep", false, SETUP(char_set_sleeptime(att, 0); seated_on(g, att, ITEM_CHAIR);), false, 1},
    {"on a bed", "sleep", false, SETUP(char_set_sleeptime(att, 0); seated_on(g, att, ITEM_BED);), false, 1},
    {"", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); ), false, 1},
    {"by a bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"furious, by a bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); thing_named(g, "bed", ITEM_BED); char_plr_set(att, player_flag::fury, true);), false, 1},
    {"by a chair", "sleep chair", false, SETUP(char_set_sleeptime(att, 0); thing_named(g, "chair", ITEM_CHAIR);), false, 1},
    {"by a spike", "sleep spike", false, SETUP(char_set_sleeptime(att, 0); thing_named(g, "spike", ITEM_BED, {}, 65);), false, 1},
    {"by a bed taken", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); { obj_data *c = thing_named(g, "bed", ITEM_BED); obj_set_sitting(c, victim); char_set_sits(victim, c); }), false, 1},
    {"by a tiny bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); obj_set_size(thing_named(g, "bed", ITEM_BED), SIZE_FINE); char_set_size(att, SIZE_HUGE);), false, 1},
    {"on a chair, by a bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); seated_on(g, att, ITEM_CHAIR); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"sitting, by a bed", "sleep bed", false, SETUP(char_set_sleeptime(att, 0); char_set_position(att, POS_SITTING); thing_named(g, "bed", ITEM_BED);), false, 1},
    {"watching the dummy sleep", "sleep", false, SETUP(char_set_sleeptime(att, 0); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { char_set_sleeptime(c, 0); thing_named(g, "bed", ITEM_BED); for (int pos : {POS_SLEEPING, POS_STUNNED, POS_FIGHTING}) { char_set_position(c, pos); do_sleep(g, c, "", 0, 0); char_set_position(c, pos); do_sleep(g, c, "bed", 0, 0); } break; }), false, 1},

    /* wake */
    {"knocked out", "wake", false, SETUP(char_aff_set(att, aff_flag::knocked, true);), false, 1},
    {"a late sleeper, asleep", "wake", false, SETUP(char_set_bonuses(att, BONUS_LATE, 1); asleep(att);), false, 1},
    {"asleep", "wake namek", false, SETUP(asleep(att);), false, 1},
    {"", "wake nobody", false, SETUP(), false, 1},
    {"", "wake self", false, SETUP(), false, 1},
    {"", "wake namek", false, SETUP(), false, 1},
    {"by a sleeper spelled", "wake namek", false, SETUP(asleep(victim); char_aff_set(victim, aff_flag::sleep, true);), false, 1},
    {"by someone dying", "wake namek", false, SETUP(char_set_position(victim, POS_STUNNED);), false, 1},
    {"by someone knocked out", "wake namek", false, SETUP(asleep(victim); char_aff_set(victim, aff_flag::knocked, true);), false, 1},
    {"a late sleeper, by a sleeper", "wake namek", false, SETUP(asleep(victim); char_set_bonuses(att, BONUS_LATE, 1);), false, 1},
    {"by a sleeper", "wake namek", false, SETUP(asleep(victim);), false, 1},
    {"by a sleeper dragged", "wake namek", false, SETUP(dragging(cast.helper, victim); asleep(victim);), false, 1},
    {"by a sleeper carried gently", "wake namek", false, SETUP(carrying(g, cast.helper, victim); char_set_alignment(cast.helper, 100);), false, 1},
    {"by a sleeper carried roughly", "wake namek", false, SETUP(carrying(g, cast.helper, victim); char_set_alignment(cast.helper, -100);), false, 1},
    {"spelled asleep", "wake", false, SETUP(asleep(att); char_aff_set(att, aff_flag::sleep, true);), false, 1},
    {"asleep", "wake", false, SETUP(asleep(att);), false, 1},
    {"asleep, dragged", "wake", false, SETUP(asleep(att); dragging(victim, att);), false, 1},
    {"asleep, carried gently", "wake", false, SETUP(carrying(g, victim, att); char_set_alignment(victim, 100);), false, 1},
    {"asleep, carried roughly", "wake", false, SETUP(carrying(g, victim, att); char_set_alignment(victim, -100);), false, 1},

    /* follow */
    {"in a healing tank", "follow namek", false, SETUP(char_plr_set(att, player_flag::healt, true);), false, 1},
    {"", "follow", false, SETUP(), false, 1},
    {"", "follow nobody", false, SETUP(), false, 1},
    {"", "follow namek", false, SETUP(), false, 1},
    {"following", "follow namek", false, SETUP(add_follower(g, att, victim);), false, 1},
    {"charmed", "follow dummy", false, SETUP(add_follower(g, att, victim); char_aff_set(att, aff_flag::charm, true);), false, 1},
    {"", "follow self", false, SETUP(), false, 1},
    {"following", "follow self", false, SETUP(add_follower(g, att, victim);), false, 1},
    {"followed", "follow namek", false, SETUP(add_follower(g, victim, att);), false, 1},
    {"following another", "follow namek", false, SETUP(add_follower(g, att, cast.helper);), false, 1},
  });
} // namespace

const std::span<const scene> SCENES_MOVEMENT = list;
