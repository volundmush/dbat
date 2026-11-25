/*************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <climits>
#include <span>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/name_table.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/config.hpp"
#include "dbat/random.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/oasis_copy.hpp"
#include "dbat/relocate.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/spells.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/house.hpp"
#include "dbat/class.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void handle_fall(GameWorld *g, struct char_data *ch);
static int check_swim(GameWorld *g, struct char_data *ch);

namespace {
  /* A message to ch and one to the rest of the room */
  void tell_both(GameWorld *g, struct char_data *ch, std::string_view to_char, std::string_view to_room)
  {
    act_triple(g, {to_char, {}, to_room, true}, ch, nullptr, nullptr);
  }

  void tell_both(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view to_char, std::string_view to_room)
  {
    act_triple(g, {to_char, {}, to_room, true}, ch, obj, nullptr);
  }

  /* The last of range that pick chooses */
  template <class Range, class Pick> struct obj_data *last_of(Range &&range, Pick pick)
  {
    struct obj_data *found = nullptr;
    for (obj_data *obj : range)
      if (pick(obj))
        found = obj;
    return found;
  }

  /* ch lets go of whomever they drag */
  void stop_dragging(GameWorld *g, struct char_data *ch)
  {
    act(g, "@WYou stop dragging @C$N@W!@n", true, ch, nullptr, char_get_drag(g, ch), act_to::actor);
    act(g, "@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
    char_set_dragged(char_get_drag(g, ch), nullptr);
    char_set_drag(ch, nullptr);
  }

  /* ch puts down whomever they carry, gently if good */
  void put_down(GameWorld *g, struct char_data *carrier) { carry_drop(g, carrier, char_get_alignment(carrier) > 50 ? 0 : 1); }

  /* Whether a body dragged or carried along feels it; quirk: the roll can't fail, but it's made */
  bool feels_moved(GameWorld *g, struct char_data *body)
  {
    return !char_aff_flagged(body, aff_flag::knocked) && !char_aff_flagged(body, aff_flag::sleep) && rand_number(g, 1, 3);
  }

  /* ch, arrived, brings along someone held so, with their seat if they're on one */
  void bring(GameWorld *g, struct char_data *ch, struct char_data *held)
  {
    char_from_room(g, held);
    char_to_room(g, held, char_get_in_room(ch));
    if (char_get_sits(g, held)) {
      obj_from_room(g, char_get_sits(g, held));
      obj_to_room(g, char_get_sits(g, held), char_get_in_room(ch));
    }
  }
} // namespace

/* This handles teleporting players with instant transmission or skills like it. */
void handle_teleport(GameWorld *g, struct char_data *ch, struct char_data *tar, room_vnum location)
{
  if (location == 0 && tar == nullptr) {
    mud_log("ERROR: handle_teleport called without a destination.");
    return;
  }
  char_from_room(g, ch);
  char_to_room(g, ch, location != 0 ? location : char_get_in_room(tar));

  act(g, "@w$n@w appears in an instant out of nowhere!@n", true, ch, nullptr, nullptr, act_to::room);
  /* Players held come along; mobs are let go of */
  auto follows = [&](struct char_data *held, std::string_view how) {
    char_from_room(g, held);
    char_to_room(g, held, char_get_in_room(ch));
    act(g, how, true, held, nullptr, ch, act_to::notvict);
  };
  if (char_get_drag(g, ch) && !char_is_npc(char_get_drag(g, ch)))
    follows(char_get_drag(g, ch), "@w$n@w appears in an instant out of nowhere being dragged by $N!@n");
  if (char_get_grappling(g, ch) && !char_is_npc(char_get_grappling(g, ch)))
    follows(char_get_grappling(g, ch), "@w$n@w appears in an instant out of nowhere being grappled by $N!@n");
  if (char_get_carrying(g, ch))
    follows(char_get_carrying(g, ch), "@w$n@w appears in an instant out of nowhere being carried by $N!@n");
  if (char_get_grappled(g, ch) && !char_is_npc(char_get_grappled(g, ch)))
    follows(char_get_grappled(g, ch), "@w$n@w appears in an instant out of nowhere being grappled by $N!@n");
  if (char_get_drag(g, ch) && char_is_npc(char_get_drag(g, ch)))
    stop_dragging(g, ch);
  if (char_get_grappling(g, ch) && char_is_npc(char_get_grappling(g, ch))) {
    char_set_grap(char_get_grappling(g, ch), -1);
    char_set_grappled(char_get_grappling(g, ch), nullptr);
    char_set_grappling(ch, nullptr);
    char_set_grap(ch, -1);
  }
  if (char_get_grappled(g, ch) && char_is_npc(char_get_grappled(g, ch))) {
    char_set_grap(char_get_grappled(g, ch), -1);
    char_set_grappling(char_get_grappled(g, ch), nullptr);
    char_set_grappled(ch, nullptr);
    char_set_grap(ch, -1);
  }
}

/* Let's carry someone! Why not? - Iovan */
ACMD(do_carry)
{
  if (char_is_npc(ch))
    return;
  if (char_get_drag(g, ch)) {
    send_to_char(g, ch, "You are busy dragging someone at the moment.\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::piloting)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_get_carrying(g, ch)) { /* Already carrying someone. Put them down. Simple and clean. */
    put_down(g, ch);
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "You want to carry who?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, find_flag::char_room, "That person isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "There's no point in carrying them.\r\n");
    return;
  }
  if (char_get_carried_by(g, vict) != nullptr) {
    send_to_char(g, ch, "Someone is already carrying them!\r\n");
    return;
  }
  if (char_get_position(vict) > POS_SLEEPING) {
    send_to_char(g, ch, "They are not unconcious.\r\n");
    return;
  }
  if (char_get_pc_weight(g, vict) + char_get_carry_weight(vict) > getMaxCarryWeight(g, ch)) {
    act_triple(g, {"@WYou try to pick up @C$N@W but have to put them down. They are too heavy for you at the moment.@n", {},
                   "@C$n@W tries to pick up @c$N@W. After struggling for a moment $e has to put $M down.@n", true},
               ch, nullptr, vict);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  act_triple(g, {"@WYou pick up @C$N@W and put $M over your shoulder.@n", {}, "@C$n@W picks up $c$N@W and puts $M over $s shoulder.@n", true}, ch, nullptr, vict);
  if (struct obj_data *chair = char_get_sits(g, vict)) {
    obj_set_sitting(chair, nullptr);
    char_set_sits(vict, nullptr);
  }
  char_set_carrying(g, ch, vict);
  char_set_carried_by(g, vict, ch);
  char_set_wait(ch, PULSE_1SEC);
}

namespace {
  /* By how one is put down: gently, hastily, knocked loose by a hit, or as the one carried leaves the game */
  constexpr act_msgs CARRY_DROPS[] = {
      {"@WYou gently set @C$N@W down on the ground.@n",                  "@C$n @Wgently sets you down on the ground.@n",                  "@C$n @Wgently sets @c$N@W down on the ground.@n",                  true},
      {"@WYou set @C$N@W hastily onto the ground.@n",                    "@C$n @Wsets you hastily onto the ground.@n",                    "@C$n @Wsets @c$N@W hastily onto the ground.@n",                    true},
      {"@WYou have @C$N@W knocked out of your arms and onto the ground!@n", "@WYou are knocked out of @C$n's@W arms and onto the ground!@n", "@C$n @Whas @c$N@W knocked out of $s arms and onto the ground!@n", true},
      {"@WYou stop carrying @C$N@W for some reason.@n",                  "@C$n @Wstops carrying you for some reason.@n",                  "@C$n @Wstops carrying @c$N@W for some reason.@n",                  true},
  };
} // namespace

/* Handles dropping someone you are carrying. */
void carry_drop(GameWorld *g, struct char_data *ch, int type)
{
  struct char_data *vict = char_get_carrying(g, ch);
  if (type >= 0 && type < (int)std::size(CARRY_DROPS))
    act_triple(g, CARRY_DROPS[type], ch, nullptr, vict);
  char_set_carrying(g, ch, nullptr);
  char_set_carried_by(g, vict, nullptr);
}

namespace {
  /* A place on a planet to land at, by name */
  struct landing {
    std::string_view name{};
    room_vnum room;
  };

  /* A planet's sky: the room above it, where it can be landed at (and where, if the place named is unknown), and
     the list of places shown */
  struct orbit {
    room_vnum above;
    std::span<const landing> places;
    room_vnum otherwise;
    std::string_view unknown{}; /* quirk: some say "You", others "you" */
    std::string_view shown{};
  };

  constexpr landing EARTH[] = {
      {"Nexus City", 300}, {"South Ocean", 800}, {"Nexus Field", 1150}, {"Cherry Blossom Mountain", 1180}, {"Sandy Desert", 1287},
      {"Northern Plains", 1428}, {"Korin's Tower", 1456}, {"Kami's Lookout", 1506}, {"Shadow Forest", 1636}, {"Decrepit Area", 1710},
      {"West City", 19510}, {"Hercule Beach", 2141}, {"Satan City", 13020}};
  constexpr landing FRIGID[] = {
      {"Ice Crown City", 4264}, {"Ice Highway", 4300}, {"Topica Snowfield", 4351}, {"Glug's Volcano", 4400}, {"Platonic Sea", 4600},
      {"Slave City", 4800}, {"Acturian Woods", 5100}, {"Desolate Demesne", 5150}, {"Chateau Ishran", 5165}, {"Wyrm Spine Mountain", 5200},
      {"Cloud Ruler Temple", 5500}, {"Koltoan Mine", 4944}};
  constexpr landing KONACK[] = {
      {"Tiranoc City", 8006}, {"Great Oroist Temple", 8300}, {"Elzthuan Forest", 8400}, {"Mazori Farm", 8447}, {"Dres", 8500},
      {"Colvian Farm", 8600}, {"St Alucia", 8700}, {"Meridius Memorial", 8800}, {"Desert of Illusion", 8900}, {"Plains of Confusion", 8954},
      {"Turlon Fair", 9200}, {"Wetlands", 9700}, {"Kerberos", 9855}, {"Shaeras Mansion", 9864}, {"Slavinus Ravine", 9900},
      {"Furian Citadel", 9949}};
  constexpr landing VEGETA[] = {
      {"Vegetos City", 2226}, {"Blood Dunes", 2600}, {"Ancestral Mountains", 2616}, {"Destopa Swamp", 2709}, {"Pride forest", 2800},
      {"Pride Tower", 2899}, {"Ruby Cave", 2615}};
  constexpr landing NAMEK[] = {
      {"Senzu Village", 11600}, {"Guru's House", 10182}, {"Crystalline Cave", 10474}, {"Elder Village", 13300}, {"Frieza's Ship", 10203},
      {"Kakureta Village", 10922}};
  constexpr landing AETHER[]  = {{"Haven City", 12010}, {"Serenity Lake", 12103}, {"Kaiju Forest", 12300}, {"Ortusian Temple", 12400}, {"Silent Glade", 12480}};
  constexpr landing YARDRAT[] = {{"Yardra City", 14008}, {"Jade Forest", 14100}, {"Jade Cliffs", 14200}, {"Mount Valaria", 14300}};
  constexpr landing CERRIA[]  = {{"Cerria Colony", 17531}, {"Crystalline Forest", 7950}, {"Fistarl Volcano", 17420}};
  constexpr landing ZENNITH[] = {{"Utatlan City", 3412}, {"Zenith Jungle", 3520}, {"Ancient Castle", 19600}};
  constexpr landing KANASSA[] = {{"Aquis City", 14904}, {"Yunkai Pirate Base", 15655}};
  constexpr landing ARLIA[]   = {{"Janacre", 16009}, {"Arlian Wasteland", 16544}, {"Arlia Mine", 16600}};

  constexpr std::string_view YOU_GUESS = "You don't know where that made up place is, but decided to land anyway.";
  constexpr std::string_view you_guess = "you don't know where that made up place is, but decided to land anyway.";

  const orbit ORBITS[] = {
      {50, EARTH, 300, YOU_GUESS,
       "@D------------------[ @GEarth@D ]------------------@c\n"
       "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n"
       "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n"
       "Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n"
       "@D---------------------------------------------@n\n"},
      {51, FRIGID, 4264, YOU_GUESS,
       "@D------------------[ @CFrigid@D ]------------------@c\n"
       "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n"
       "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n"
       "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n"
       "@D---------------------------------------------@n\n"},
      {52, KONACK, 8006, you_guess,
       "@D------------------[ @MKonack@D ]------------------@c\n"
       "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n"
       "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n"
       "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n"
       "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n"
       "@D---------------------------------------------@n\n"},
      {53, VEGETA, 2226, you_guess,
       "@D------------------[ @YVegeta@D ]------------------@c\n"
       "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n"
       "Pride Forest, Pride tower, Ruby Cave.\n"
       "@D---------------------------------------------@n\n"},
      {54, NAMEK, 11600, you_guess,
       "@D------------------[ @gNamek@D ]------------------@c\n"
       "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n"
       "Frieza's Ship, Kakureta Village.\n"
       "@D---------------------------------------------@n\n"},
      {55, AETHER, 12010, you_guess,
       "@D------------------[ @BAether@D ]-----------------@c\n"
       "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n"
       "Silent Glade.\n"
       "@D--------------------------------------------@n\n"},
      {56, YARDRAT, 14008, you_guess,
       "@D-----------------[ @mYardrat@D ]-----------------@c\n"
       "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n"
       "@D-------------------------------------------@n\n"},
      {198, CERRIA, 17531, you_guess,
       "@D------------------[ @MCerria@D ]------------------@c\n"
       "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n"
       "@D---------------------------------------------@n\n"},
      {57, ZENNITH, 3412, you_guess,
       "@D-----------------[ @CZennith@D ]-----------------@c\n"
       "Utatlan City, Zenith Jungle, Ancient Castle.\n"
       "@D-------------------------------------------@n\n"},
      {58, KANASSA, 14904, you_guess,
       "@D-----------------[ @CKanassa@D ]-----------------@c\n"
       "Aquis City, Yunkai Pirate Base.\n"
       "@D-------------------------------------------@n\n"},
      {59, ARLIA, 16009, you_guess,
       "@D------------------[ @MArlia@D ]------------------@c\n"
       "Janacre, Arlian Wasteland, Arlia Mine.\n"
       "@D---------------------------------------------@n\n"},
  };

  const orbit *orbit_at(room_vnum room)
  {
    auto o = std::ranges::find_if(ORBITS, [&](const orbit &row) { return row.above == room; });
    return o == std::end(ORBITS) ? nullptr : &*o;
  }

  /* Where ch lands from this orbit, for the place named */
  room_vnum land_location(GameWorld *g, struct char_data *ch, const orbit &o, std::string_view arg)
  {
    auto place = std::ranges::find_if(o.places, [&](const landing &l) { return str_eq_nocase(arg, l.name); });
    if (place != o.places.end())
      return place->room;
    send_to_char(g, ch, o.unknown);
    return o.otherwise;
  }
} // namespace

ACMD(do_land)
{
  argument        = skip_spaces(argument);
  const orbit *o  = orbit_at(char_get_in_room(ch));

  if (argument.empty()) {
    if (!o) {
      send_to_char(g, ch, "You are not even in the lower atmosphere of a planet!\r\n");
      return;
    }
    send_to_char(g, ch, "Land where?\n");
    send_to_char(g, ch, o->shown);
    return;
  }
  if (!o) {
    send_to_char(g, ch, "You are not above a planet!\r\n");
    return;
  }

  room_vnum landing = land_location(g, ch, *o, argument);
  room_vnum was_in  = char_get_in_room(ch);
  send_to_char(g, ch, "You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.\r\n");
  /* Where it will be, as sensed from there, told to those it leaves */
  char_from_room(g, ch);
  char_to_room(g, ch, landing);
  std::string_view where = sense_location(g, ch);
  char_from_room(g, ch);
  char_to_room(g, ch, was_in);
  act(g, str_format("@C$n@Y flies down through the atmosphere toward @G{}@Y!@n", where), true, ch, nullptr, nullptr, act_to::room);
  char_from_room(g, ch);
  char_to_room(g, ch, landing);
  if (struct zone_data *zone = find_zone_by_thing(g, char_get_in_room(ch)))
    fly_zone(g, zone, "can be seen landing from space nearby!@n\r\n", ch);
  send_to_sense(g, 1, "landing on the planet", ch);
  send_to_scouter(g, "A powerlevel signal has been detected landing on the planet", ch, 0, 1);
  act(g, "$n comes down from high above in the sky and quickly lands on the ground.", true, ch, nullptr, nullptr, act_to::room);
}

/* The stamina a step takes: more in heavy gravity, but nothing for beginners */
static int walk_cost(GameWorld *g, struct char_data *ch)
{
  int gravity = room_get_gravity(g, char_get_in_room(ch));
  if (char_get_level(ch) <= 1)
    return 0;
  if (gravity > 10 || (gravity == 10 && !char_is_bardock(ch) && !char_is_npc(ch)))
    return (1 + gravity) * gravity;
  return 1;
}

/* simple function to determine if char can fly */
static bool has_flight(GameWorld *g, struct char_data *ch)
{
  if (char_adm_flagged(ch, admin_flag::walkanywhere))
    return true;
  if (char_aff_flagged(ch, aff_flag::flying)) {
    if (char_is_android(ch) || char_is_npc(ch))
      return true;
    if (getCurKI(g, ch) >= char_get_level(ch) + (getMaxKI(g, ch) / (char_get_level(ch) * 30)))
      return true;
    tell_both(g, ch, "@WYou crash to the ground, too tired to fly anymore!@n", "@W$n@W crashes to the ground!@n");
    char_aff_set(ch, aff_flag::flying, false);
    handle_fall(g, ch);
    return false;
  }
  /* non-wearable flying items in inventory will do it */
  return find_first(char_inventory(g, ch), [&](obj_data *obj) { return obj_aff_flagged(obj, aff_flag::flying) && find_eq_pos(g, ch, obj) < 0; }) != nullptr;
}

/* simple function to determine if char can breathe non-o2 */
bool has_o2(struct char_data *ch)
{
  if (char_adm_flagged(ch, admin_flag::walkanywhere))
    return true;

  if (char_aff_flagged(ch, aff_flag::waterbreath))
    return true;

  if (char_is_kanassan(ch) || char_is_android(ch) || char_is_icer(ch) || char_is_majin(ch))
    return true;

  return false;
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
bool do_simple_move(GameWorld *g, struct char_data *ch, int dir, int need_specials_check)
{
  std::string buf2;
  std::string buf3;
  room_vnum was_in = char_get_in_room(ch);
  int need_movement;
  struct room_data *rm;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(g, ch, dir + 1, ""))
    return false;

  /* blocked by a leave trigger ? */
  if (!leave_mtrigger(g, ch, dir) || char_get_in_room(ch) != was_in) /* prevent teleport crashes */
    return false;
  if (!leave_wtrigger(g, room_of(g, char_get_in_room(ch)), ch, dir) || char_get_in_room(ch) != was_in) /* prevent teleport crashes */
    return false;
  if (!leave_otrigger(g, room_of(g, char_get_in_room(ch)), ch, dir) || char_get_in_room(ch) != was_in) /* prevent teleport crashes */
    return false;
  /* charmed? */
  if (char_aff_flagged(ch, aff_flag::charm) && char_get_master(g, ch) && char_get_in_room(ch) == char_get_in_room(char_get_master(g, ch))) {
    send_to_char(g, ch, "The thought of leaving your master makes you weep.\r\n");
    act(g, "$n bursts into tears.", false, ch, nullptr, nullptr, act_to::room);
    return false;
  }

  int willfall = false;
  /* if this room or the one we're going to needs flight, check for it */
  if ((room_sect(g, char_get_in_room(ch)) == SECT_FLYING) || (room_sect(g, char_get_exit(g, ch, dir)->to_room) == SECT_FLYING)) {
    if (!has_flight(g, ch)) {
      if (dir != 4) {
        willfall = true;
      } else {
        send_to_char(g, ch, "You need to fly to go there!\r\n");
        return false;
      }
    }
  }

  if (((room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) || (room_sect(g, char_get_exit(g, ch, dir)->to_room) == SECT_WATER_NOSWIM)) && char_is_humanoid(ch) && !has_flight(g, ch)) {
    if (char_is_kanassan(ch) || char_is_icer(ch)) {
      tell_both(g, ch, "@CYou swim swiftly.@n", "@c$n@C swims swiftly.@n");
    } else {
      if (!check_swim(g, ch))
        return false;
      tell_both(g, ch, "@CYou swim through the cold water.@n", "@c$n@C swim through the cold water.@n");
      char_set_wait(ch, PULSE_1SEC);
    }
  }

  if (room_flagged(g, char_get_in_room(ch), room_flag::space)) {
    if (!char_is_android(ch)) {
      if (!check_swim(g, ch)) {
        return false;
      }
    }
  }

  if (room_get_geffect(g, char_get_exit(g, ch, dir)->to_room) == 6 && !char_is_humanoid(ch) && char_is_npc(ch)) {
    return false;
  }

  if (char_is_npc(ch) && room_flagged(g, char_get_exit(g, ch, dir)->to_room, room_flag::nomob) && !char_get_master(g, ch)) {
    return false;
  }

  if (room_sunken(g, char_get_in_room(ch)) ||
      room_sunken(g, char_get_exit(g, ch, dir)->to_room)) {
    /* Breath is held with ki, longer in a group with the bonus for it */
    int64_t breath = getMaxKI(g, ch) / (group_bonus(g, ch, 2) == 10 ? 800 : 200);
    if (!has_o2(ch) && getCurKI(g, ch) < breath) {
      if (getCurPL(g, ch) >= getEffMaxPL(g, ch) / 20) {
        send_to_char(g, ch, "@RYou struggle to breath!@n\r\n");
        decCurHealth(g, ch, getMaxPL(g, ch) / 20, 0);
      }
      if (getCurPL(g, ch) < getEffMaxPL(g, ch) / 20) {
        send_to_char(g, ch, "@rYou drown!@n\r\n");
        die(g, ch, nullptr);
        return false;
      }
    } else if (!has_o2(ch)) {
      send_to_char(g, ch, "@CYou hold your breath!@n\r\n");
      decCurKI(g, ch, breath, 0);
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = walk_cost(g, ch);
  /* Stealth increases your move cost, less if you are good at it */
  if (char_aff_flagged(ch, aff_flag::hide))
    need_movement *= ((roll_skill(g, ch, SKILL_HIDE) > 15) ? 2 : 4);

  if (char_aff_flagged(ch, aff_flag::sneak))
    need_movement *= ((roll_skill(g, ch, SKILL_MOVE_SILENTLY) > 15) ? 1.2 : 2);

  int flight_cost = 0;

  if (char_aff_flagged(ch, aff_flag::flying) && !char_is_android(ch)) {
    if (!char_get_skill(ch, SKILL_CONCENTRATION) && !char_get_skill(ch, SKILL_FOCUS)) {
      flight_cost = getMaxKI(g, ch) / 100;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) && !char_get_skill(ch, SKILL_FOCUS)) {
      flight_cost = getMaxKI(g, ch) / (char_get_skill(ch, SKILL_CONCENTRATION) * 2);
    } else if (!char_get_skill(ch, SKILL_CONCENTRATION) && char_get_skill(ch, SKILL_FOCUS)) {
      flight_cost = getMaxKI(g, ch) / (char_get_skill(ch, SKILL_FOCUS) * 3);
    } else {
      flight_cost = getMaxKI(g, ch) / ((char_get_skill(ch, SKILL_CONCENTRATION) * 2) + (char_get_skill(ch, SKILL_FOCUS) * 3));
    }
  }

  if (char_aff_flagged(ch, aff_flag::flying) && ((getCurKI(g, ch)) < flight_cost) && !char_is_android(ch)) {
    decCurKI(g, ch, flight_cost, 0);
    tell_both(g, ch, "@WYou crash to the ground, too tired to fly anymore!@n", "@W$n@W crashes to the ground!@n");
    char_aff_set(ch, aff_flag::flying, false);
  } else if (char_aff_flagged(ch, aff_flag::flying) && !char_is_android(ch)) {
    decCurKI(g, ch, flight_cost, 0);
  }

  if ((getCurST(g, ch)) < need_movement && !char_aff_flagged(ch, aff_flag::flying) && !char_is_npc(ch)) {
    if (need_specials_check && char_get_master(g, ch)) {
      send_to_char(g, ch, "You are too exhausted to follow.\r\n");
    } else {
      send_to_char(g, ch, "You are too exhausted.\r\n");
    }

    return false;
  }

  /* Check if the character needs a skill check to go that way. */
  if (char_get_exit(g, ch, dir)->dcskill != 0) {
    if (char_get_exit(g, ch, dir)->dcmove > roll_skill(g, ch, char_get_exit(g, ch, dir)->dcskill)) {
      send_to_char(g, ch, "Your skill in {} isn't enough to move that way!\r\n", g->spell_info[char_get_exit(g, ch, dir)->dcskill].name);
      /* A failed skill check still spends the movement points! */
      if (!char_adm_flagged(ch, admin_flag::walkanywhere) && !char_is_npc(ch) && !char_aff_flagged(ch, aff_flag::flying))
        decCurST(g, ch, need_movement, 0);
      return false;
    }
    send_to_char(g, ch, "Your skill in {} aids in your movement.\r\n", g->spell_info[char_get_exit(g, ch, dir)->dcskill].name);
  }

  if (room_flagged(g, char_get_in_room(ch), room_flag::atrium)) {
    if (!House_can_enter(g, ch, char_get_exit(g, ch, dir)->to_room)) {
      send_to_char(g, ch, "That's private property -- no trespassing!\r\n");
      return false;
    }
  }
  if (room_flagged(g, char_get_exit(g, ch, dir)->to_room, room_flag::tunnel) &&
      (num_pc_in_room(g, room_of(g, char_get_exit(g, ch, dir)->to_room)) >= CONFIG_TUNNEL_SIZE)) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(g, ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(g, ch, "There isn't enough room there for more than one person!\r\n");
    return false;
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (room_flagged(g, char_get_exit(g, ch, dir)->to_room, room_flag::godroom) &&
      char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "You aren't godly enough to use that room!\r\n");
    return false;
  }

  /******* Zone flag checks *******/

  rm = room_of(g, char_get_exit(g, ch, dir)->to_room);

  if (!char_is_npc(ch) && (char_get_admlevel(ch) < ADMLVL_IMMORT) &&
      (char_get_level(ch) < zone_get_min_level(g, room_get_zone(rm))) && (zone_get_min_level(g, room_get_zone(rm)) > 0)) {
    send_to_char(g, ch, "Sorry, you are too low a level to enter this zone.\r\n");
    return false;
  }

  if ((char_get_admlevel(ch) < ADMLVL_IMMORT) && (char_get_level(ch) > zone_get_max_level(g, room_get_zone(rm))) &&
      (zone_get_max_level(g, room_get_zone(rm)) > 0)) {
    send_to_char(g, ch, "Sorry, you are too high a level to enter this zone.\r\n");
    return false;
  }

  if ((char_get_admlevel(ch) < ADMLVL_IMMORT) && zone_flagged(g, room_get_zone(rm), zone_flag::closed)) {
    send_to_char(g, ch, "This zone is currently closed to mortals.\r\n");
    return false;
  }

  if ((char_get_admlevel(ch) >= ADMLVL_IMMORT && char_get_admlevel(ch) < ADMLVL_GRGOD) && zone_flagged(g, room_get_zone(rm), zone_flag::noimmort)) {
    send_to_char(g, ch, "This zone is closed to all.\r\n");
    return false;
  }

  /* No low level immortal scouting */
  if ((char_get_admlevel(ch) >= ADMLVL_IMMORT && char_get_admlevel(ch) < ADMLVL_GOD) &&
      !can_edit_zone(g, ch, room_get_zone(rm)) && zone_flagged(g, room_get_zone(rm), zone_flag::quest)) {
    send_to_char(g, ch, "This is a Quest zone.\r\n");
    return false;
  }

  /* Now we know we're allowed to go into the room. */
  if (!char_adm_flagged(ch, admin_flag::walkanywhere) && !char_is_npc(ch) && !char_aff_flagged(ch, aff_flag::flying)) {
    decCurST(g, ch, need_movement, 0);
  }

  if (char_aff_flagged(ch, aff_flag::sneak) && !char_is_npc(ch)) {
    buf2 = str_format("$n sneaks {}.", dirs[dir]);
    if (char_get_skill(ch, SKILL_MOVE_SILENTLY)) {
      improve_skill(g, ch, SKILL_MOVE_SILENTLY, 0);
    } else if (slot_count(ch) + 1 > char_get_skill_slots(ch)) {
      send_to_char(g, ch, "@RYour skill slots are full. You can not learn Move Silently.\r\n");
      char_aff_set(ch, aff_flag::sneak, false);
    } else {
      send_to_char(g, ch, "@GYou learn the very basics of moving silently.@n\r\n");
      char_set_skills_base(ch, SKILL_MOVE_SILENTLY, rand_number(g, 5, 10));
      act(g, buf2, true, ch, nullptr, nullptr, act_to::room | act_option::sneak_resist);
      if (char_get_aff_abils_dex(ch) < rand_number(g, 1, 30)) {
        char_set_wait(ch, PULSE_1SEC);
      }
    }
  }

  if (!char_aff_flagged(ch, aff_flag::sneak) && !char_aff_flagged(ch, aff_flag::flying)) {
    buf2 = str_format("$n leaves {}.", dirs[dir]);
    act(g, buf2, true, ch, nullptr, nullptr, act_to::room);
  }
  if (!char_aff_flagged(ch, aff_flag::sneak) && char_aff_flagged(ch, aff_flag::flying)) {
    buf2 = str_format("$n flies {}.", dirs[dir]);
    act(g, buf2, true, ch, nullptr, nullptr, act_to::room);
  }

  was_in = char_get_in_room(ch);
  if (char_get_drag(g, ch)) {
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
  }
  if (char_get_carrying(g, ch)) {
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::room);
  }
  char_aff_set(ch, aff_flag::pursuit, true);
  char_from_room(g, ch);
  char_to_room(g, ch, room_get_dir_option(g, was_in, dir)->to_room);
  if ((room_get_zone(g, char_get_in_room(ch)) != room_get_zone(g, was_in)) && !char_is_npc(ch) && !char_is_android(ch)) {
    send_to_sense(g, 0, "You sense someone", ch);
    buf3 = str_format("@D[@GBlip@D]@Y {}\r\n@RSomeone has entered your scouter detection range@n.", add_commas(getCurPL(g, ch)));
    send_to_scouter(g, buf3.c_str(), ch, 0, 0);
  }
  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(g, ch) || !enter_wtrigger(g, room_of(g, char_get_in_room(ch)), ch, dir)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    char_aff_set(ch, aff_flag::pursuit, false);
    return false;
  }

  buf2 = str_format("{}{}",
                    ((dir == UP) || (dir == DOWN) ? "" : "the "),
                    (dir == UP ? "below" : (dir == DOWN) ? "above"
                                                         : dirs[rev_dir[dir]]));
  act(g, "$n arrives from $T.", true, ch, nullptr, buf2.c_str(), act_to::room | act_option::sneak_resist);
  if (char_get_fighting(g, ch)) {
    if (room_sect(g, room_get_dir_option(g, was_in, dir)->to_room) != SECT_FLYING && room_sect(g, room_get_dir_option(g, was_in, dir)->to_room) != SECT_WATER_NOSWIM && room_get_geffect(g, room_get_dir_option(g, was_in, dir)->to_room) == 0) {
      roll_pursue(g, char_get_fighting(g, ch), ch);
    }
    char_aff_set(ch, aff_flag::pursuit, false);
  }
  if (char_get_drag(g, ch)) {
    act(g, "@wYou drag @C$N@w with you.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::actor);
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
    bring(g, ch, char_get_drag(g, ch));
    if (feels_moved(g, char_get_drag(g, ch))) {
      send_to_char(g, char_get_drag(g, ch), "You feel your sleeping body being moved.\r\n");
      if (char_is_npc(char_get_drag(g, ch)) && !char_get_fighting(g, char_get_drag(g, ch)))
        set_fighting(g, char_get_drag(g, ch), ch);
    }
  }
  if (char_get_carrying(g, ch)) {
    act(g, "@wYou carry @C$N@w with you.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::actor);
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::room);
    char_from_room(g, char_get_carrying(g, ch));
    char_to_room(g, char_get_carrying(g, ch), char_get_in_room(ch));
    if (feels_moved(g, char_get_carrying(g, ch)))
      send_to_char(g, char_get_carrying(g, ch), "You feel your sleeping body being moved.\r\n");
  }

  if (char_get_desc(ch) != nullptr) {
    look_at_room(g, char_get_in_room(ch), ch, 0);
    if (char_aff_flagged(ch, aff_flag::sneak) && !char_is_npc(ch) && char_get_skill(ch, SKILL_MOVE_SILENTLY) && char_get_skill(ch, SKILL_MOVE_SILENTLY) < rand_number(g, 1, 101)) {
      send_to_char(g, ch, "@wYou make a noise as you arrive and are no longer sneaking!@n\r\n");
      act(g, "@c$n@w makes a noise revealing $s sneaking!@n", true, ch, nullptr, nullptr, act_to::room | act_option::sneak_resist);
      reveal_hiding(g, ch, 0);
      char_aff_set(ch, aff_flag::sneak, false);
    }
  }

  if (room_get_geffect(g, char_get_in_room(ch)) == 6 || room_get_geffect(g, was_in) == 6) {
    if (!char_is_demon(ch) && !char_aff_flagged(ch, aff_flag::flying) && group_bonus(g, ch, 2) != 14) {
      act(g, "@rYour legs are burned by the lava!@n", true, ch, nullptr, nullptr, act_to::actor);
      act(g, "@R$n@r's legs are burned by the lava!@n", true, ch, nullptr, nullptr, act_to::room);
      if (char_is_npc(ch) && char_is_humanoid(ch) && rand_number(g, 1, 2) == 2) {
        do_fly(g, ch, "", 0, 0);
      }
      decCurHealth(g, ch, getEffMaxPL(g, ch) / 20, 0);
      if (getCurPL(g, ch) <= 0) {
        act(g, "@rYou have burned to death!@n", true, ch, nullptr, nullptr, act_to::actor);
        act(g, "@R$n@r has burned to death!@n", true, ch, nullptr, nullptr, act_to::room);
        die(g, ch, nullptr);
      }
    }
    if (char_get_drag(g, ch) && !char_is_demon(char_get_drag(g, ch))) {
      act(g, "@R$N@r gets burned!@n", true, ch, nullptr, char_get_drag(g, ch), act_to::actor);
      act(g, "@R$N@r gets burned!@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
      decCurHealth(g, char_get_drag(g, ch), getEffMaxPL(g, char_get_drag(g, ch)) / 20, 0);
      if (getCurPL(g, char_get_drag(g, ch)) < 0) {
        act(g, "@rYou have burned to death!@n", true, char_get_drag(g, ch), nullptr, nullptr, act_to::actor);
        act(g, "@R$n@r has burned to death!@n", true, char_get_drag(g, ch), nullptr, nullptr, act_to::room);
        die(g, char_get_drag(g, ch), nullptr);
      }
    }
  }

  if (room_flagged(g, char_get_in_room(ch), room_flag::timed_dt) && !char_adm_flagged(ch, admin_flag::walkanywhere))
    timed_dt(g, nullptr);

  if (room_flagged(g, char_get_in_room(ch), room_flag::death) && !char_adm_flagged(ch, admin_flag::walkanywhere)) {
    log_death_trap(g, ch);
    death_cry(g, ch);
    extract_char(g, ch);
    return false;
  }

  entry_memory_mtrigger(g, ch);
  if (!greet_mtrigger(g, ch, dir)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    look_at_room(g, char_get_in_room(ch), ch, 0);
  } else greet_memory_mtrigger(g, ch);
  if (willfall == true) {
    handle_fall(g, ch);
    if (char_get_drag(g, ch)) {
      handle_fall(g, char_get_drag(g, ch));
    }
  }
  return true;
}

bool perform_move(GameWorld *g, struct char_data *ch, int dir, int need_specials_check)
{
  room_vnum was_in;

  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return false;
  }

  if (char_get_absorbing(g, ch) || char_get_absorbby(g, ch)) {
    send_to_char(g, ch, "You are struggling with someone!\r\n");
    return false;
  }

  if (!char_aff_flagged(ch, aff_flag::sneak) || (char_aff_flagged(ch, aff_flag::sneak) && char_get_skill(ch, SKILL_MOVE_SILENTLY) < axion_dice(g, 0))) {
    reveal_hiding(g, ch, 0);
  }

  if (ch == nullptr || dir < 0 || dir >= NUM_OF_DIRS)
    return false;
  else if ((!char_get_exit(g, ch, dir) && !buildwalk(g, ch, dir)) || char_get_exit(g, ch, dir)->to_room == NOWHERE || (room_exit_flagged(char_get_exit(g, ch, dir), exit_flag::secret) && (room_exit_flagged(char_get_exit(g, ch, dir), exit_flag::closed))))
    send_to_char(g, ch, "Alas, you cannot go that way...\r\n");
  else if (room_exit_flagged(char_get_exit(g, ch, dir), exit_flag::closed)) {
    if (!exit_keyword(char_get_exit(g, ch, dir)).empty())
      send_to_char(g, ch, "The {} seems to be closed.\r\n", fname(exit_keyword(char_get_exit(g, ch, dir))));
    else
      send_to_char(g, ch, "It seems to be closed.\r\n");
  } else if (char_get_exit(g, ch, dir)->to_room == 0 || char_get_exit(g, ch, dir)->to_room == 1) {
    send_to_char(g, ch, "Report this direction, it is illegal.\r\n");
  } else {

    for (obj_data *wall : room_contents(g, char_get_in_room(ch))) {
      if (obj_get_vnum(g, wall) == 79) {
        if (obj_get_cost(wall) == dir) {
          send_to_char(g, ch, "That direction has a glacial wall blocking it.\r\n");
          return false;
        }
      }
    }

    if (!char_get_followers(ch))
      return (do_simple_move(g, ch, dir, need_specials_check));

    was_in = char_get_in_room(ch);
    if (!do_simple_move(g, ch, dir, need_specials_check))
      return false;

    for (char_data *fol : char_followers(g, ch)) {
      if ((char_get_in_room(fol) == was_in) &&
          (char_get_position(fol) >= POS_STANDING) &&
          (!char_aff_flagged(ch, aff_flag::zanzoken) ||
           (char_aff_flagged(ch, aff_flag::group) && char_aff_flagged(fol, aff_flag::group)))) {
        act(g, "You follow $N.\r\n", false, fol, nullptr, ch, act_to::actor);
        perform_move(g, fol, dir, 1);
      } else if ((char_get_in_room(fol) == was_in) &&
                 (char_get_position(fol) >= POS_STANDING) &&
                 (char_aff_flagged(ch, aff_flag::zanzoken) && char_aff_flagged(fol, aff_flag::zanzoken)) &&
                 (!char_aff_flagged(ch, aff_flag::group) || !char_aff_flagged(fol, aff_flag::group))) {
        act(g, "$N tries to zanzoken and escape, but your zanzoken matches $S!\r\n", false, fol, nullptr, ch, act_to::actor);
        act(g, "$N tries to zanzoken and escape, but $n's zanzoken matches $S!\r\n", false, fol, nullptr, ch, act_to::notvict);
        act(g, "You zanzoken to try and escape, but $n's zanzoken matches yours!\r\n", false, fol, nullptr, ch, act_to::vict);
        char_aff_set(ch, aff_flag::zanzoken, false);
        char_aff_set(fol, aff_flag::zanzoken, false);
        perform_move(g, fol, dir, 1);
      } else if ((char_get_in_room(fol) == was_in) &&
                 (char_get_position(fol) >= POS_STANDING) &&
                 (char_aff_flagged(ch, aff_flag::zanzoken) && !char_aff_flagged(fol, aff_flag::zanzoken))) {
        act(g, "You try to follow $N, but $E disappears in a flash of movement!\r\n", false, fol, nullptr, ch, act_to::actor);
        act(g, "$n tries to follow $N, but $E disappears in a flash of movement!\r\n", false, fol, nullptr, ch, act_to::notvict);
        act(g, "$n tries to follow you, but you manage to zanzoken away!\r\n", false, fol, nullptr, ch, act_to::vict);
        char_aff_set(ch, aff_flag::zanzoken, false);
      }
    }
    return true;
  }
  return false;
}

namespace {
  /* Charged ki that keeps one from traveling: from a share of one's ki, unless concentration is this good */
  struct charge_limit {
    double from, below; /* below 0 for no limit */
    int concentration;
  };

  constexpr charge_limit CHARGE_LIMITS[] = {
      {.51, -1,  INT_MAX},
      {.5,  .51, 100    },
      {.4,  .5,  80     },
      {.3,  .4,  70     },
      {.2,  .3,  60     },
  };

  bool overcharged(GameWorld *g, struct char_data *ch)
  {
    return std::ranges::any_of(CHARGE_LIMITS, [&](const charge_limit &l) {
      return char_get_charge(ch) >= getMaxKI(g, ch) * l.from && (l.below < 0 || char_get_charge(ch) < getMaxKI(g, ch) * l.below) &&
             char_get_skill(ch, SKILL_CONCENTRATION) < l.concentration;
    });
  }

  /* Gravity that slows those of no more than so much power by so long; bardock's don't feel the least of it */
  struct heavy_gravity {
    int gravity;
    int64_t power;
    int wait;
  };

  constexpr heavy_gravity HEAVY_GRAVITIES[] = {
      {10,    10000,     PULSE_1SEC},
      {20,    30000,     PULSE_2SEC},
      {30,    100000,    PULSE_3SEC},
      {40,    200000,    PULSE_3SEC},
      {50,    300000,    PULSE_3SEC},
      {100,   500000,    PULSE_3SEC},
      {200,   1000000,   PULSE_3SEC},
      {300,   8000000,   PULSE_3SEC},
      {400,   15000000,  PULSE_3SEC},
      {500,   25000000,  PULSE_4SEC},
      {1000,  35000000,  PULSE_5SEC},
      {5000,  100000000, PULSE_5SEC},
      {10000, 200000000, PULSE_5SEC},
  };

  void slowed_by_gravity(GameWorld *g, struct char_data *ch)
  {
    int gravity = room_get_gravity(g, char_get_in_room(ch));
    auto heavy  = std::ranges::find_if(HEAVY_GRAVITIES, [&](const heavy_gravity &h) { return h.gravity == gravity; });
    if (heavy == std::end(HEAVY_GRAVITIES) || getEffMaxPL(g, ch) > heavy->power || (gravity == 10 && char_is_bardock(ch)))
      return;
    send_to_char(g, ch, "The gravity slows you down some.\r\n");
    char_set_wait(ch, heavy->wait);
  }

  /* A limb worn by being used for walking, and maybe falling apart, as `falls` says */
  void strain(GameWorld *g, struct char_data *ch, int limb, int falls, std::string_view name)
  {
    if (char_get_limb_condition(ch, limb) >= 50)
      return;
    send_to_char(g, ch, "@RYour {} is damaged by the forced use!@n\r\n", name);
    char_set_limb_condition(ch, limb, char_get_limb_condition(ch, limb) - rand_number(g, 1, 5));
    if (char_get_limb_condition(ch, falls) <= 0)
      tell_both(g, ch, str_format("@RYour {} falls apart!@n", name).c_str(), str_format("@r$n's@R {} falls apart!@n", name).c_str());
  }

  void get_off_seat(GameWorld *g, struct char_data *ch)
  {
    if (struct obj_data *chair = char_get_sits(g, ch)) {
      obj_set_sitting(chair, nullptr);
      char_set_sits(ch, nullptr);
    }
  }
} // namespace

ACMD(do_move)
{
  if (char_is_npc(ch)) {
    perform_move(g, ch, subcmd - 1, 0);
    return;
  }
  if (char_plr_flagged(ch, player_flag::selfd)) {
    send_to_char(g, ch, "You are preparing to blow up!\r\n");
    return;
  }
  if (char_aff_flagged(ch, aff_flag::liquefied)) {
    send_to_char(g, ch, "You are liquefied right now!\r\n");
    return;
  }
  if (overcharged(g, ch)) {
    send_to_char(g, ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
    return;
  }

  if (char_get_conditions(g, ch, DRUNK) > 4 && (rand_number(g, 1, 9) + char_get_conditions(g, ch, DRUNK)) >= rand_number(g, 14, 20)) {
    send_to_char(g, ch, "You wobble around and then fall on your ass.\r\n");
    act(g, "@C$n@W wobbles around before falling on $s ass@n.", true, ch, nullptr, nullptr, act_to::room);
    char_set_position(ch, POS_SITTING);
    return;
  }

  if (char_get_fighting(g, ch) && !char_is_npc(ch)) {
    std::string blah = str_format("{}", dirs[subcmd - 1]);
    do_flee(g, ch, blah, 0, 0);
    return;
  }

  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  if (char_plr_flagged(ch, player_flag::piloting)) {
    struct obj_data *vehicle = nullptr, *controls = nullptr;
    int noship = false;
    if (!(controls = find_control(g, ch)) && char_get_admlevel(ch) < 1) {
      noship = true;
    } else if (!(vehicle = find_vehicle_by_vnum(g, obj_get_value(controls, 0)))) {
      noship = true;
    }
    if (noship == true) {
      send_to_char(g, ch, "Your ship controls are not here or your ship was not found, report to Iovan!\r\n");
      return;
    }
    if (controls != nullptr && vehicle != nullptr) {
      if (obj_get_value(controls, 2) <= 0) {
        send_to_char(g, ch, "The ship is out of fuel!\r\n");
        return;
      }
      drive_in_direction(g, ch, vehicle, subcmd - 1);
      if (obj_get_value(controls, 1) == 1) {
        char_set_wait(ch, PULSE_2SEC);
      } else if (obj_get_value(controls, 1) == 2) {
        char_set_wait(ch, PULSE_1SEC);
      }
      controls = nullptr;
      vehicle  = nullptr;
      return;
    }
    return;
  }
  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (!char_is_npc(ch)) {
    int fail = false;
    for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
      if (obj_get_kicharge(obj) > 0 && obj_get_user(g, obj) == ch) {
        fail = true;
      }
    }
    if (fail == true) {
      send_to_char(g, ch, "You are too busy controlling your attack!\r\n");
      return;
    }
  }

  if (!char_is_npc(ch) && char_get_limb_condition(ch, 0) <= 0 && char_get_limb_condition(ch, 1) <= 0 && char_get_limb_condition(ch, 2) <= 0 && char_get_limb_condition(ch, 3) <= 0 && !char_aff_flagged(ch, aff_flag::flying)) {
    send_to_char(g, ch, "Unless you fly, you can't get far with no limbs.\r\n");
    return;
  }
  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return;
  }
  if (char_get_absorbing(g, ch)) {
    send_to_char(g, ch, "You are busy absorbing from {}!\r\n", char_get_name(char_get_absorbing(g, ch)));
    return;
  }
  if (char_get_absorbby(g, ch)) {
    if (axion_dice(g, 0) < char_get_skill(char_get_absorbby(g, ch), SKILL_ABSORB)) {
      send_to_char(g, ch, "You are being held by {}, they are absorbing you!\r\n", char_get_name(char_get_absorbby(g, ch)));
      send_to_char(g, char_get_absorbby(g, ch), "{} struggles in your grasp!\r\n", char_get_name(ch));
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_absorbby(g, ch), nullptr, ch, act_to::notvict);
    act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_absorbby(g, ch), nullptr, ch, act_to::vict);
    act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_absorbby(g, ch), nullptr, ch, act_to::actor);
    char_set_absorbing(char_get_absorbby(g, ch), nullptr);
    char_set_absorbby(ch, nullptr);
  }
  if (!block_calc(g, ch)) {
    return;
  }
  if (char_get_listenroom(ch) > 0) {
    send_to_char(g, ch, "You stop eavesdropping.\r\n");
    char_set_listenroom(ch, 0);
  }
  if (!char_is_npc(ch)) {
    if (char_prf_flagged(g, ch, pref_flag::arenawatch)) {
      char_prf_set(g, ch, pref_flag::arenawatch, false);
      char_set_arenawatch(ch, -1);
    }
    if (char_get_in_room(ch) != NOWHERE && char_get_in_room(ch) != 0 && char_get_in_room(ch) != 1) {
      char_set_load_room(g, ch, char_get_in_room(ch));
    }
    slowed_by_gravity(g, ch);
    if (room_flagged(g, char_get_in_room(ch), room_flag::space) && char_get_admlevel(ch) < 1) {
      send_to_char(g, ch, "You struggle to cross the vast distance.\r\n");
      char_set_wait(ch, PULSE_6SEC);
    } else if (char_get_limb_condition(ch, 2) <= 0 && char_get_limb_condition(ch, 3) <= 0 && char_get_limb_condition(ch, 0) <= 0 && !char_aff_flagged(ch, aff_flag::flying)) {
      tell_both(g, ch, "@wYou slowly pull yourself along with your arm...@n", "@C$n@w slowly pulls $mself along with one arm...@n");
      strain(g, ch, 1, 0, "left arm"); /* quirk: whether it falls apart is the right arm's state */
      char_set_wait(ch, PULSE_5SEC);
    } else if (char_get_limb_condition(ch, 2) <= 0 && char_get_limb_condition(ch, 3) <= 0 && char_get_limb_condition(ch, 1) <= 0 && !char_aff_flagged(ch, aff_flag::flying)) {
      tell_both(g, ch, "@wYou slowly pull yourself along with your arm...@n", "@C$n@w slowly pulls $mself along with one arm...@n");
      strain(g, ch, 0, 0, "right arm");
      char_set_wait(ch, PULSE_5SEC);
    } else if (char_get_limb_condition(ch, 2) <= 0 && char_get_limb_condition(ch, 3) <= 0 && !char_aff_flagged(ch, aff_flag::flying)) {
      tell_both(g, ch, "@wYou slowly pull yourself along with your arms...@n", "@C$n@w slowly pulls $mself along with one arms...@n");
      strain(g, ch, 1, 1, "left arm");
      strain(g, ch, 0, 0, "right arm");
      char_set_wait(ch, PULSE_3SEC);
    } else if (char_get_limb_condition(ch, 2) <= 0 && !char_aff_flagged(ch, aff_flag::flying)) {
      tell_both(g, ch, "@wYou hop on one leg...@n", "@C$n@w hops on one leg...@n");
      strain(g, ch, 3, 3, "left leg");
      char_set_wait(ch, PULSE_2SEC);
    } else if (char_get_limb_condition(ch, 3) <= 0 && !char_aff_flagged(ch, aff_flag::flying)) {
      tell_both(g, ch, "@wYou hop on one leg...@n", "@C$n@w hops on one leg...@n");
      strain(g, ch, 2, 2, "right leg");
      char_set_wait(ch, PULSE_2SEC);
    } else if (char_get_position(ch) == POS_RESTING) {
      tell_both(g, ch, "@wYou crawl on your hands and knees.@n", "@C$n@w crawls on $s hands and knees.@n");
      get_off_seat(g, ch);
      char_set_wait(ch, PULSE_3SEC);
    } else if (char_get_position(ch) == POS_SITTING) {
      tell_both(g, ch, "@wYou shuffle on your hands and knees.@n", "@C$n@w shuffles on $s hands and knees.@n");
      get_off_seat(g, ch);
      char_set_wait(ch, PULSE_2SEC);
    } else if (char_get_position(ch) < POS_RESTING) {
      send_to_char(g, ch, "You are in no condition to move! Try standing...\r\n");
      return;
    }
  }
  perform_move(g, ch, subcmd - 1, 0);
  if (!char_get_rdisplay(ch).empty()) {
    if (char_get_rdisplay(ch) != "Empty") {
      char_set_rdisplay(ch, "Empty");
    }
  }
}

static int find_door(GameWorld *g, struct char_data *ch, std::string_view type, std::string_view dir, std::string_view cmdname)
{
  int door;

  if (!dir.empty()) { /* a direction was specified */
    if ((door = search_block(dir, dirs, false)) < 0 &&
        (door = search_block(dir, abbr_dirs, false)) < 0) { /* Partial Match */
      send_to_char(g, ch, "That's not a direction.\r\n");
      return (-1);
    }
    if (char_get_exit(g, ch, door)) { /* Braces added according to indent. -gg */
      if (!exit_keyword(char_get_exit(g, ch, door)).empty()) {
        if (is_name(type, exit_keyword(char_get_exit(g, ch, door))))
          return (door);
        else {
          send_to_char(g, ch, "I see no {} there.\r\n", type);
          return (-1);
        }
      } else
        return (door);
    } else {
      send_to_char(g, ch, "I really don't see how you can {} anything there.\r\n", cmdname);
      return (-1);
    }
  } else { /* try to locate the keyword */
    if (type.empty()) {
      send_to_char(g, ch, "What is it you want to {}?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (char_get_exit(g, ch, door))
        if (!exit_keyword(char_get_exit(g, ch, door)).empty())
          if (is_name(type, exit_keyword(char_get_exit(g, ch, door))))
            return (door);

    send_to_char(g, ch, "There doesn't seem to be {} {} that could be manipulated in that way here.\r\n", AN(type), type);
    return (-1);
  }
}

static bool has_key(GameWorld *g, struct char_data *ch, obj_vnum key)
{
  int i;

  if (key == 1) {
    return true;
  }

  for (obj_data *o : char_inventory(g, ch))
    if (obj_get_vnum(g, o) == key)
      return true;

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      if (obj_get_vnum(g, char_get_equipment(ch, i)) == key)
        return true;

  return false;
}

/* What a door command needs the door to be */
struct door_needs {
  bool open = false, closed = false, unlocked = false, locked = false;
};

static constexpr std::string_view cmd_door_entries[] = {
        "open",
        "close",
        "unlock",
        "lock",
        "pick"};
static_assert(std::size(cmd_door_entries) == NUM_DOOR_CMD);
constinit const name_table cmd_door(cmd_door_entries);

static constexpr door_needs flags_door[] = {
    {.closed = true, .unlocked = true}, /* open */
    {.open = true},                     /* close */
    {.closed = true, .locked = true},   /* unlock */
    {.closed = true, .unlocked = true}, /* lock */
    {.closed = true, .locked = true},   /* pick */
};
static_assert(std::size(flags_door) == NUM_DOOR_CMD);

namespace {
  enum class change { set, clear, toggle };

  /* A container's flag, or the door's, in that room that way, changed */
  template <class Flags, class Flag> void change_flag(Flags &flags, change how, Flag flag)
  {
    if (how == change::set)
      flags.set(flag);
    else if (how == change::clear)
      flags.reset(flag);
    else
      flags.toggle(flag);
  }

  void change_door(GameWorld *g, room_vnum room, struct obj_data *obj, int door, change how, container_flag cont_bit, exit_flag ex_bit)
  {
    if (obj) {
      flag_set<container_flag> flags = obj_container_flags(obj);
      change_flag(flags, how, cont_bit);
      obj_set_container_flags(obj, flags);
      return;
    }
    change_flag(room_get_dir_option(g, room, door)->exit_info, how, ex_bit);
  }

  /* What open, close, unlock and lock do, and what's said of a ship's hatch opened or closed: from inside (seen
     inside, then outside, then of space), and as the ship from outside (the same); else, the words for a door
     and what the other side hears */
  struct door_change {
    change how;
    container_flag cont_bit;
    exit_flag ex_bit;
    bool always_say_obj;
    std::string_view inside_ship[3], outside_ship[3];
    std::string_view you, they, inside_heard, outside_heard; /* you: none for a ship's hatch */
  };

  const door_change DOOR_CHANGES[] = {
      {change::clear, container_flag::closed, exit_flag::closed, false,
       {"@wThe ship hatch opens slowly and settles onto the ground outside.\r\n", "@wThe ship hatch opens slowly and settles onto the ground.\r\n", "@wA great vortex forms as air begins to get sucked out into the void!\r\n"},
       {"@wThe ship hatch opens slowly and settles onto the ground.\r\n", "@wThe ship hatch opens slowly.\r\n", "@wThe air starts getting sucked out into space as the hatch opens!\r\n"},
       "@wYou open @c$p@w.", "@C$n@w opens @c$p@w.", "@wThe door to {}@w is opened from the other side.\r\n", "@wThe door is opened from the other side.\r\n"},
      {change::set, container_flag::closed, exit_flag::closed, false,
       {"@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n", "@wThe ship hatch slowly closes, sealing the ship.\r\n", "@wThe air stops getting sucked out into space as the hatch seals!\r\n"},
       {"@wThe ship hatch slowly closes, sealing the ship.\r\n", "@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n", "@wAir stops getting sucked out into space as the hatch seals!\r\n"},
       "@wYou close @c$p@w.", "@C$n@w closes @c$p@w.", "@wThe door to {}@w is closed from the other side.\r\n", "@wThe door to {}@w is closed from the other side.\r\n"},
      {change::clear, container_flag::locked, exit_flag::locked, true, {}, {}, {}, {}, {}, {}},
      {change::set, container_flag::locked, exit_flag::locked, true, {}, {}, {}, {}, {}, {}},
  };

  void send_to_room_of(GameWorld *g, room_vnum room, std::string_view fmt, struct obj_data *about)
  {
    std::string_view desc = obj_get_short_description(about);
    send_to_room(g, room, std::vformat(fmt, std::make_format_args(desc)));
  }
} // namespace

static void do_doorcmd(GameWorld *g, struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  room_vnum num                    = 0;
  room_vnum other_room             = NOWHERE;
  struct room_direction_data *back = nullptr;
  struct obj_data *hatch = nullptr, *vehicle = nullptr;

  /* A hatch's ship, or a ship's hatch, which ch goes in to find */
  if (obj && obj_get_type_flag(obj) == ITEM_HATCH) {
    vehicle = find_vehicle_by_vnum(g, obj_get_value(obj, VAL_HATCH_DEST));
  } else if (obj && obj_get_type_flag(obj) == ITEM_VEHICLE) {
    if (room_of(g, obj_get_value(obj, VAL_PORTAL_DEST))) {
      num = char_get_in_room(ch);
      char_from_room(g, ch);
      char_to_room(g, ch, obj_get_value(obj, VAL_PORTAL_DEST));
    }
    hatch = last_of(room_contents(g, char_get_in_room(ch)), [&](obj_data *item) { return obj_get_type_flag(item) == ITEM_HATCH; });
  }

  if (!door_mtrigger(g, ch, scmd, door) || !door_wtrigger(g, ch, scmd, door))
    return;

  std::string buf = str_format("$n {}s ", cmd_door[scmd]);
  if (!obj && ((other_room = char_get_exit(g, ch, door)->to_room) != NOWHERE)) {
    if ((back = room_get_dir_option(g, other_room, rev_dir[door])) != nullptr)
      if (back->to_room != char_get_in_room(ch))
        back = nullptr;
  }

  if (scmd == SCMD_PICK) {
    change_door(g, char_get_in_room(ch), obj, door, change::toggle, container_flag::locked, exit_flag::locked);
    if (back)
      change_door(g, other_room, obj, rev_dir[door], change::toggle, container_flag::locked, exit_flag::locked);
    send_to_char(g, ch, "The lock quickly yields to your skills.\r\n");
    buf = "$n skillfully picks the lock on ";
  } else {
    const door_change &d = DOOR_CHANGES[scmd];
    auto to              = [&](room_vnum room, struct obj_data *o) { change_door(g, room, o, door, d.how, d.cont_bit, d.ex_bit); };
    if (obj && obj_get_type_flag(obj) == ITEM_HATCH && vehicle) {
      to(char_get_in_room(ch), vehicle);
      if (d.you.empty()) {
        /* locks are silent */
      } else if (obj_get_vnum(g, obj) > 19199) {
        send_to_room(g, char_get_in_room(ch), d.inside_ship[0]);
        send_to_room(g, obj_get_in_room(vehicle), d.inside_ship[1]);
        if (room_flagged(g, obj_get_in_room(vehicle), room_flag::space))
          send_to_room(g, char_get_in_room(ch), d.inside_ship[2]);
      } else {
        tell_both(g, ch, obj, d.you, d.they);
        send_to_room_of(g, obj_get_in_room(vehicle), d.inside_heard, vehicle);
      }
      vehicle = nullptr;
    }
    if (obj && obj_get_type_flag(obj) == ITEM_VEHICLE && hatch) {
      to(char_get_in_room(ch), hatch);
      char_from_room(g, ch);
      char_to_room(g, ch, num);
      if (d.you.empty()) {
        /* locks are silent */
      } else if (obj_get_vnum(g, obj) > 19199) {
        send_to_room(g, num, d.outside_ship[0]);
        send_to_room(g, obj_get_in_room(hatch), d.outside_ship[1]);
        if (obj_get_in_room(obj) != NOWHERE && room_flagged(g, obj_get_in_room(obj), room_flag::space))
          send_to_room(g, num, d.outside_ship[2]);
      } else {
        tell_both(g, ch, obj, d.you, d.they);
        send_to_room_of(g, obj_get_in_room(hatch), d.outside_heard, hatch);
      }
      hatch = nullptr;
    }
    to(char_get_in_room(ch), obj);
    if (back)
      change_door(g, other_room, obj, rev_dir[door], d.how, d.cont_bit, d.ex_bit);
    if (!obj)
      send_to_char(g, ch, "You {} the {} that leads {}.\r\n", cmd_door[scmd], or_if_empty(exit_keyword(char_get_exit(g, ch, door)), "door"), dirs[door]);
    else if (d.always_say_obj || (obj_get_type_flag(obj) != ITEM_VEHICLE && obj_get_type_flag(obj) != ITEM_HATCH))
      send_to_char(g, ch, "You {} {}.\r\n", cmd_door[scmd], obj_get_short_description(obj));
  }

  /* Notify the room. */
  std::string dbuf(obj ? std::string_view() : dirs[door]);
  const std::string_view keyword = obj ? std::string_view() : exit_keyword(char_get_exit(g, ch, door)); /* NUL-ended: a std::string field */
  buf += str_format("{}{}{}{}.", obj ? "" : "the ", obj ? "$p" : !keyword.empty() ? "$F" : "door", obj ? "" : " that leads ", dbuf);
  if (!obj || obj_get_in_room(obj) != NOWHERE)
    act(g, buf, false, ch, obj, keyword.empty() ? nullptr : keyword.data(), act_to::room);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
    send_to_room(g, char_get_exit(g, ch, door)->to_room, "The {} that leads {} is {}{} from the other side.\r\n",
                 exit_keyword(back).empty() ? "door" : fname(exit_keyword(back)), dbuf, cmd_door[scmd], scmd == SCMD_CLOSE ? "d" : "ed");
  else if (back && (scmd == SCMD_LOCK || scmd == SCMD_UNLOCK))
    send_to_room(g, char_get_exit(g, ch, door)->to_room, "The {} that leads {} is {}ed from the other side.\r\n",
                 exit_keyword(back).empty() ? "door" : fname(exit_keyword(back)), dbuf, cmd_door[scmd]);
}

static bool ok_pick(GameWorld *g, struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, struct obj_data *hatch)
{
  if (scmd != SCMD_PICK)
    return true;
  /* PICKING_LOCKS is not an untrained skill */
  if (!char_get_skill(ch, SKILL_OPEN_LOCK)) {
    send_to_char(g, ch, "You have no idea how!\r\n");
    return false;
  }
  if (!find_first(char_inventory(g, ch), [&](obj_data *obj) { return obj_get_vnum(g, obj) == 18 && !obj_extra_flagged(obj, extra_flag::broken) && !obj_extra_flagged(obj, extra_flag::forged); })) {
    send_to_char(g, ch, "You need a lock picking kit.\r\n");
    return false;
  }
  if (hatch != nullptr && (obj_get_type_flag(hatch) == ITEM_HATCH || obj_get_type_flag(hatch) == ITEM_VEHICLE)) {
    send_to_char(g, ch, "No picking ship hatches.\r\n");
    return false;
  }
  int skill_lvl = roll_skill(g, ch, SKILL_OPEN_LOCK);
  if (dclock == 0)
    dclock = rand_number(g, 1, 101);

  if (keynum == NOTHING) {
    send_to_char(g, ch, "Odd - you can't seem to find a keyhole.\r\n");
  } else if (pickproof) {
    send_to_char(g, ch, "It resists your attempts to pick it.\r\n");
    act(g, "@c$n@w puts a set of lockpick tools away.@n", true, ch, nullptr, nullptr, act_to::room);
  } else if (getCurST(g, ch) < getMaxST(g, ch) / 30) {
    send_to_char(g, ch, "You don't have the stamina to try, it takes percision to pick locks.Not shaking tired hands.\r\n");
  } else if (dclock > skill_lvl - 2) {
    send_to_char(g, ch, "You failed to pick the lock...\r\n");
    act(g, "@c$n@w puts a set of lockpick tools away.@n", true, ch, nullptr, nullptr, act_to::room);
    decCurST(g, ch, getCurST(g, ch) / 30, 0);
  } else {
    decCurST(g, ch, getCurST(g, ch) / 30, 0);
    return true;
  }
  return false;
}

namespace {
  /* Whether obj, or else the door, has a flag: the container's, or the exit's */
  bool door_flagged(GameWorld *g, struct char_data *ch, struct obj_data *obj, int door, container_flag cont_flag, exit_flag ex_flag)
  {
    return obj ? obj_val_flagged(obj, cont_flag) : room_exit_flagged(char_get_exit(g, ch, door), ex_flag);
  }

  bool door_openable(GameWorld *g, struct char_data *ch, struct obj_data *obj, int door)
  {
    if (!obj)
      return room_exit_flagged(char_get_exit(g, ch, door), exit_flag::isdoor);
    int type = obj_get_type_flag(obj);
    return (type == ITEM_CONTAINER || type == ITEM_VEHICLE || type == ITEM_HATCH || type == ITEM_WINDOW || type == ITEM_PORTAL) && obj_val_flagged(obj, container_flag::closeable);
  }
} // namespace

ACMD(do_gen_door)
{
  int door                 = -1;
  struct obj_data *obj     = nullptr;
  struct char_data *victim = nullptr;

  argument = skip_spaces(argument);
  if (argument.empty()) {
    send_to_char(g, ch, "{:c}{} what?\r\n", as_char(toupper(cmd_door[subcmd].front())), cmd_door[subcmd].substr(1));
    return;
  }
  std::string_view rest = argument;
  std::string type      = take_arg(rest);
  std::string dir       = take_arg(rest);
  if (!generic_find(g, type, find_flag::obj_inv | find_flag::obj_room, ch, &victim, &obj))
    door = find_door(g, ch, type, dir, cmd_door[subcmd]);
  if (obj && obj_get_type_flag(obj) != ITEM_CONTAINER && obj_get_type_flag(obj) != ITEM_VEHICLE && obj_get_type_flag(obj) != ITEM_HATCH) {
    obj  = nullptr;
    door = find_door(g, ch, type, dir, cmd_door[subcmd]);
  }
  if (!obj && door < 0)
    return;

  obj_vnum keynum = obj ? obj_get_value(obj, VAL_KEY_KEYCODE) : char_get_exit(g, ch, door)->key;
  if (!(obj ? obj_get_value(obj, VAL_DOOR_DCLOCK) : char_get_exit(g, ch, door)->dclock)) {
    if (obj)
      obj_set_value(obj, VAL_DOOR_DCLOCK, 20);
    else
      char_get_exit(g, ch, door)->dclock = 20;
  }
  bool closed = door_flagged(g, ch, obj, door, container_flag::closed, exit_flag::closed), locked = door_flagged(g, ch, obj, door, container_flag::locked, exit_flag::locked);
  door_needs needs = flags_door[subcmd];

  if (!door_openable(g, ch, obj, door))
    act(g, "You can't $F that!", false, ch, nullptr, cmd_door[subcmd].data(), act_to::actor); /* a literal, ended by its '\0' */
  else if (closed && needs.open)
    send_to_char(g, ch, "But it's already closed!\r\n");
  else if (!closed && needs.closed)
    send_to_char(g, ch, "But it's currently open!\r\n");
  else if (!locked && needs.locked)
    send_to_char(g, ch, "Oh.. it wasn't locked, after all..\r\n");
  else if (locked && needs.unlocked)
    send_to_char(g, ch, "It seems to be locked.\r\n");
  else if (!has_key(g, ch, keynum) && !char_adm_flagged(ch, admin_flag::nokeys) && (subcmd == SCMD_LOCK || subcmd == SCMD_UNLOCK))
    send_to_char(g, ch, "You don't seem to have the proper key.\r\n");
  else {
    int pickproof = door_flagged(g, ch, obj, door, container_flag::pickproof, exit_flag::pickproof);
    int dclock    = obj ? obj_get_value(obj, VAL_DOOR_DCLOCK) : char_get_exit(g, ch, door)->dclock;
    if (ok_pick(g, ch, keynum, pickproof, dclock, subcmd, obj))
      do_doorcmd(g, ch, obj, door, subcmd);
    else if (!obj) /* quirk: a door that fails to be picked is tried again, to no end */
      ok_pick(g, ch, keynum, pickproof, dclock, subcmd, nullptr);
  }
}

/*
 * The checks entering and leaving an object share. Returns the movement points it costs, or -1
 * (after telling ch why) when ch can't go.
 */
static int portal_move_cost(GameWorld *g, struct char_data *ch, room_vnum dest_room, int need_specials_check)
{
  int need_movement;

  /* charmed? */
  if (char_aff_flagged(ch, aff_flag::charm) && char_get_master(g, ch) &&
      char_get_in_room(ch) == char_get_in_room(char_get_master(g, ch))) {
    send_to_char(g, ch, "The thought of leaving your master makes you weep.\r\n");
    act(g, "$n bursts into tears.", false, ch, nullptr, nullptr, act_to::room);
    return -1;
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = walk_cost(g, ch);
  if ((getCurST(g, ch)) < need_movement && !char_aff_flagged(ch, aff_flag::flying) && !char_is_npc(ch)) {
    if (need_specials_check && char_get_master(g, ch))
      send_to_char(g, ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(g, ch, "You are too exhausted.\r\n");

    return -1;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::atrium)) {
    if (!House_can_enter(g, ch, dest_room)) {
      send_to_char(g, ch, "That's private property -- no trespassing!\r\n");
      return -1;
    }
  }
  if (room_flagged(g, dest_room, room_flag::tunnel) &&
      num_pc_in_room(g, room_of(g, dest_room)) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(g, ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(g, ch, "There isn't enough room there for more than one person!\r\n");
    return -1;
  }
  return need_movement;
}

static bool do_simple_enter(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_vnum dest_room = obj_get_value(obj, VAL_PORTAL_DEST);
  room_vnum was_in    = char_get_in_room(ch);
  int need_movement   = 0;

  if ((need_movement = portal_move_cost(g, ch, dest_room, need_specials_check)) < 0)
    return false;
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (room_flagged(g, dest_room, room_flag::godroom) &&
      char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "You aren't godly enough to use that room!\r\n");
    return false;
  }
  /* Now we know we're allowed to go into the room. */
  if (!(char_is_npc(ch) || char_adm_flagged(ch, admin_flag::walkanywhere)) && !char_aff_flagged(ch, aff_flag::flying))
    decCurST(g, ch, need_movement, 0);

  act(g, "$n enters $p.", true, ch, obj, nullptr, act_to::room | act_option::sneak_resist);

  if (char_get_drag(g, ch)) {
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
  }
  if (char_get_carrying(g, ch)) {
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::room);
  }
  char_from_room(g, ch);
  char_to_room(g, ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(g, ch)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    return false;
  }

  if (obj_get_type_flag(obj) == ITEM_PORTAL)
    act(g, "$n arrives from $p.", false, ch, obj, nullptr, act_to::room | act_option::sneak_resist);
  else
    act(g, "$n arrives from outside.", false, ch, nullptr, nullptr, act_to::room | act_option::sneak_resist);
  if (char_get_drag(g, ch)) {
    act(g, "@wYou drag @C$N@w with you.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::actor);
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
    if (feels_moved(g, char_get_drag(g, ch))) {
      send_to_char(g, char_get_drag(g, ch), "You feel your sleeping body being moved.\r\n");
      if (char_is_npc(char_get_drag(g, ch)) && !char_get_fighting(g, char_get_drag(g, ch))) {
        set_fighting(g, char_get_drag(g, ch), ch);
      }
    }
    char_from_room(g, char_get_drag(g, ch));
    char_to_room(g, char_get_drag(g, ch), char_get_in_room(ch));
    if (char_get_sits(g, char_get_drag(g, ch))) {
      obj_from_room(g, char_get_sits(g, char_get_drag(g, ch)));
      obj_to_room(g, char_get_sits(g, char_get_drag(g, ch)), char_get_in_room(ch));
    }
  }
  if (char_get_carrying(g, ch)) {
    act(g, "@wYou carry @C$N@w with you.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::actor);
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::room);
    if (feels_moved(g, char_get_carrying(g, ch))) {
      send_to_char(g, char_get_carrying(g, ch), "You feel your sleeping body being moved.\r\n");
    }
    char_from_room(g, char_get_carrying(g, ch));
    char_to_room(g, char_get_carrying(g, ch), char_get_in_room(ch));
    if (char_get_sits(g, char_get_carrying(g, ch))) {
      obj_from_room(g, char_get_sits(g, char_get_carrying(g, ch)));
      obj_to_room(g, char_get_sits(g, char_get_carrying(g, ch)), char_get_in_room(ch));
    }
  }

  if (char_get_desc(ch) != nullptr)
    look_at_room(g, char_get_in_room(ch), ch, 0);

  if (room_flagged(g, char_get_in_room(ch), room_flag::death) && !char_adm_flagged(ch, admin_flag::walkanywhere)) {
    log_death_trap(g, ch);
    death_cry(g, ch);
    extract_char(g, ch);
    return false;
  }

  entry_memory_mtrigger(g, ch);
  greet_memory_mtrigger(g, ch);

  return true;
}

static int perform_enter_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_vnum was_in = char_get_in_room(ch);
  int could_move   = false;

  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (obj_get_type_flag(obj) == ITEM_VEHICLE ||
      obj_get_type_flag(obj) == ITEM_PORTAL) {
    if (obj_val_flagged(obj, container_flag::closed)) {
      send_to_char(g, ch, "But it's closed!\r\n");
    } else if ((obj_get_value(obj, VAL_PORTAL_DEST) != NOWHERE) &&
               (room_of(g, obj_get_value(obj, VAL_PORTAL_DEST)))) {
      if (obj_get_value(obj, VAL_PORTAL_DEST) >= 45000 && obj_get_value(obj, VAL_PORTAL_DEST) <= 45099) {
        int filled = false;
        for (char_data *tch : room_people(g, obj_get_value(obj, VAL_PORTAL_DEST))) {
          if (tch) {
            filled = true;
          }
        }
        if (filled == true) {
          send_to_char(g, ch, "Only one person can fit in there at a time.\r\n");
          return (0);
        }
      }
      if ((could_move = do_simple_enter(g, ch, obj, need_specials_check)))
        for (char_data *fol : char_followers(g, ch))
          if ((char_get_in_room(fol) == was_in) &&
              (char_get_position(fol) >= POS_STANDING)) {
            act(g, "You follow $N.\r\n", false, fol, nullptr, ch, act_to::actor);
            perform_enter_obj(g, fol, obj, 1);
          }
    } else {
      send_to_char(g, ch,
                   "It doesn't look like you can enter it at the moment.\r\n");
    }
  } else {
    send_to_char(g, ch, "You can't enter that!\r\n");
  }
  return could_move;
}

ACMD(do_enter)
{
  struct obj_data *obj = nullptr;
  std::string buf;
  int door, move_dir = -1;

  std::string_view rest = argument;
  buf                   = take_arg(rest);

  if (!buf.empty()) { /* an argument was supplied, search for door keyword */
    /* Is the object in the room? */
    obj = get_obj_in_list_vis(g, ch, buf, nullptr, room_get_contents(g, char_get_in_room(ch)));
    /* Is the object in the character's inventory? */
    if (!obj)
      obj = get_obj_in_list_vis(g, ch, buf, nullptr, char_get_inventory(ch));
    /* Is the character carrying the object? */
    if (!obj)
      obj = get_obj_in_equip_vis(g, ch, buf, nullptr, char_get_equipment_array(ch));
    /* We have an object to enter */
    if (obj)
      perform_enter_obj(g, ch, obj, 0);
    /* Is there a door to enter? */
    else {
      for (door = 0; door < NUM_OF_DIRS; door++)
        if (char_get_exit(g, ch, door))
          if (!exit_keyword(char_get_exit(g, ch, door)).empty())
            if (isname(buf, exit_keyword(char_get_exit(g, ch, door))))
              move_dir = door;
      /* Did we find what they wanted to enter. */
      if (move_dir > -1)
        perform_move(g, ch, move_dir, 1);
      else
        send_to_char(g, ch, "There is no {} here.\r\n", buf);
    }
  } else if (room_flagged(g, char_get_in_room(ch), room_flag::indoors)) {
    send_to_char(g, ch, "You are already indoors.\r\n");
  } else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (char_get_exit(g, ch, door))
        if (char_get_exit(g, ch, door)->to_room != NOWHERE)
          if (!room_exit_flagged(char_get_exit(g, ch, door), exit_flag::closed) &&
              room_flagged(g, char_get_exit(g, ch, door)->to_room, room_flag::indoors))
            move_dir = door;
    if (move_dir > -1)
      perform_move(g, ch, move_dir, 1);
    else
      send_to_char(g, ch, "You can't seem to find anything to enter.\r\n");
  }
}

static bool do_simple_leave(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)

{
  room_vnum was_in = char_get_in_room(ch), dest_room = NOWHERE;
  int need_movement        = 0;
  struct obj_data *vehicle = nullptr;

  if (obj_get_type_flag(obj) != ITEM_PORTAL) {
    vehicle = find_vehicle_by_vnum(g, obj_get_value(obj, VAL_HATCH_DEST));
  }

  if (vehicle == nullptr && obj_get_type_flag(obj) != ITEM_PORTAL) {
    send_to_char(g, ch, "That doesn't appear to lead anywhere.\r\n");
    return false;
  }

  if (obj_get_type_flag(obj) == ITEM_PORTAL && obj_val_flagged(obj, container_flag::closed)) {
    send_to_char(g, ch, "But it's closed!\r\n");
    return false;
  }

  if (vehicle != nullptr) {
    if ((dest_room = obj_get_in_room(vehicle)) == NOWHERE) {
      send_to_char(g, ch, "That doesn't appear to lead anywhere.\r\n");
      return false;
    }
  }
  if (vehicle == nullptr) {
    if (!room_of(g, dest_room = obj_get_value(obj, VAL_PORTAL_DEST))) {
      send_to_char(g, ch, "That doesn't appear to lead anywhere.\r\n");
      return false;
    }
  }

  if ((need_movement = portal_move_cost(g, ch, dest_room, need_specials_check)) < 0)
    return false;
  /* Now we know we're allowed to go into the room. */
  if (!(char_is_npc(ch) || char_adm_flagged(ch, admin_flag::walkanywhere)) && !char_aff_flagged(ch, aff_flag::flying))
    decCurST(g, ch, need_movement, 0);

  act(g, "$n leaves $p.", true, ch, vehicle, nullptr, act_to::room | act_option::sneak_resist);

  if (char_get_drag(g, ch)) {
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
  }
  if (char_get_carrying(g, ch)) {
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::room);
  }
  char_from_room(g, ch);
  char_to_room(g, ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(g, ch)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    return false;
  }

  if (vehicle) {
    act(g, "$n arrives from inside $p.", true, ch, vehicle, nullptr, act_to::room | act_option::sneak_resist);
  } else {
    act(g, "$n arrives from inside", true, ch, nullptr, nullptr, act_to::room | act_option::sneak_resist);
  }
  if (char_get_drag(g, ch)) {
    act(g, "@wYou drag @C$N@w with you.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::actor);
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, char_get_drag(g, ch), act_to::room);
    char_from_room(g, char_get_drag(g, ch));
    char_to_room(g, char_get_drag(g, ch), char_get_in_room(ch));
    if (char_get_sits(g, char_get_drag(g, ch))) {
      obj_from_room(g, char_get_sits(g, char_get_drag(g, ch)));
      obj_to_room(g, char_get_sits(g, char_get_drag(g, ch)), char_get_in_room(ch));
    }
    if (feels_moved(g, char_get_drag(g, ch))) {
      send_to_char(g, char_get_drag(g, ch), "You feel your sleeping body being moved.\r\n");
      if (char_is_npc(char_get_drag(g, ch)) && !char_get_fighting(g, char_get_drag(g, ch))) {
        set_fighting(g, char_get_drag(g, ch), ch);
      }
    }
  }
  if (char_get_carrying(g, ch)) {
    act(g, "@wYou carry @C$N@w with you.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::actor);
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, char_get_carrying(g, ch), act_to::room);
    char_from_room(g, char_get_carrying(g, ch));
    char_to_room(g, char_get_carrying(g, ch), char_get_in_room(ch));
    if (char_get_sits(g, char_get_carrying(g, ch))) {
      obj_from_room(g, char_get_sits(g, char_get_carrying(g, ch)));
      obj_to_room(g, char_get_sits(g, char_get_carrying(g, ch)), char_get_in_room(ch));
    }
    if (feels_moved(g, char_get_carrying(g, ch))) {
      send_to_char(g, char_get_carrying(g, ch), "You feel your sleeping body being moved.\r\n");
    }
  }

  std::string buf3;
  send_to_sense(g, 0, "You sense someone ", ch);
  buf3 = str_format("@D[@GBlip@D]@Y {}\r\n@RSomeone has entered your scouter detection range.@n", add_commas(getCurPL(g, ch)));
  send_to_scouter(g, buf3.c_str(), ch, 0, 0);

  if (char_get_desc(ch) != nullptr) {
    act(g, obj_get_action_description(obj), true, ch, obj, nullptr, act_to::actor); /* empty sends nothing */
    look_at_room(g, char_get_in_room(ch), ch, 0);
  }

  if (room_flagged(g, char_get_in_room(ch), room_flag::death) && !char_adm_flagged(ch, admin_flag::walkanywhere)) {
    log_death_trap(g, ch);
    death_cry(g, ch);
    extract_char(g, ch);
    return false;
  }

  entry_memory_mtrigger(g, ch);
  greet_memory_mtrigger(g, ch);

  return true;
}

static int perform_leave_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_vnum was_in = char_get_in_room(ch);
  int could_move   = false;

  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (obj_val_flagged(obj, container_flag::closed)) {
    send_to_char(g, ch, "But the way out is closed.\r\n");
  } else {
    if (obj_get_value(obj, VAL_HATCH_DEST) != NOWHERE)
      if ((could_move = do_simple_leave(g, ch, obj, need_specials_check)))
        for (char_data *fol : char_followers(g, ch))
          if ((char_get_in_room(fol) == was_in) &&
              (char_get_position(fol) >= POS_STANDING)) {
            act(g, "You follow $N.\r\n", false, fol, nullptr, ch, act_to::actor);
            perform_leave_obj(g, fol, obj, 1);
          }
  }
  return could_move;
}

ACMD(do_leave)
{
  int door;

  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }

  for (obj_data *obj : room_contents(g, char_get_in_room(ch)))
    if (char_can_see_obj(g, ch, obj))
      if (obj_get_type_flag(obj) == ITEM_HATCH || obj_get_type_flag(obj) == ITEM_PORTAL) {
        perform_leave_obj(g, ch, obj, 0);
        return;
      }

  if (char_is_outside(g, ch))
    send_to_char(g, ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (char_get_exit(g, ch, door))
        if (char_get_exit(g, ch, door)->to_room != NOWHERE)
          if (!room_exit_flagged(char_get_exit(g, ch, door), exit_flag::closed) &&
              !room_flagged(g, char_get_exit(g, ch, door)->to_room, room_flag::indoors)) {
            perform_move(g, ch, door, 1);
            return;
          }
    send_to_char(g, ch, "I see no obvious exits to the outside.\r\n");
  }
}

static void handle_fall(GameWorld *g, struct char_data *ch)
{
  room_vnum room = -1;
  while (char_get_exit(g, ch, 5) && room_sect(g, char_get_in_room(ch)) == SECT_FLYING) {
    room = char_get_exit(g, ch, 5)->to_room;
    char_from_room(g, ch);
    char_to_room(g, ch, room);
    if (char_get_carrying(g, ch)) {
      char_from_room(g, char_get_carrying(g, ch));
      char_to_room(g, char_get_carrying(g, ch), room);
    }
    if (!char_get_exit(g, ch, 5) || room_sect(g, char_get_in_room(ch)) != SECT_FLYING) {
      act(g, "@r$n slams into the ground!@n", true, ch, nullptr, nullptr, act_to::room);
      decCurHealth(g, ch, getEffMaxPL(g, ch) / 20, 1);

      act(g, "@rYou slam into the ground!@n", true, ch, nullptr, nullptr, act_to::actor);
      look_at_room(g, char_get_in_room(ch), ch, 0);
    } else {
      act(g, "@r$n pummets down toward the ground below!@n", true, ch, nullptr, nullptr, act_to::room);
    }
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM && !char_get_carried_by(g, ch) && !char_is_kanassan(ch)) {
    if ((getCurST(g, ch)) >= (getCurCarriedWeight(ch))) {
      act(g, "@bYou swim in place.@n", true, ch, nullptr, nullptr, act_to::actor);
      act(g, "@C$n@b swims in place.@n", true, ch, nullptr, nullptr, act_to::room);
      decCurST(g, ch, getCurCarriedWeight(ch), 0);
      act(g, "@RYou are drowning!@n", true, ch, nullptr, nullptr, act_to::actor);
      act(g, "@C$n@b gulps water as $e struggles to stay above the water line.@n", true, ch, nullptr, nullptr, act_to::room);
      if (getCurPL(g, ch) - ((getEffMaxPL(g, ch)) / 3) <= 0) {
        act(g, "@rYou drown!@n", true, ch, nullptr, nullptr, act_to::actor);
        act(g, "@R$n@r drowns!@n", true, ch, nullptr, nullptr, act_to::room);
        die(g, ch, nullptr);
        decCurHealthPercent(g, ch, 1, 1);
      } else {
        decCurHealthPercent(g, ch, .33, 0);
      }
    }
  }
}

static int check_swim(GameWorld *g, struct char_data *ch)
{
  bool can = false;

  if (room_flagged(g, char_get_in_room(ch), room_flag::space)) {
    int64_t space_cost = (getMaxKI(g, ch) / 1000) + ((getCurCarriedWeight(ch)) / 2);
    if (getCurKI(g, ch) >= space_cost)
      can = true;
    decCurKI(g, ch, space_cost, 0);
    if (!can) send_to_char(g, ch, "You do not have enough ki to fly through space. You are drifting helplessly.\r\n");
    return can;
  }
  int64_t swim_cost = (getCurCarriedWeight(ch)) - 1;
  if (getCurST(g, ch) >= swim_cost)
    can = true;
  decCurST(g, ch, swim_cost, 0);
  if (!can) send_to_char(g, ch, "You are too tired to swim!\r\n");
  return can;
}

/* Takes off from the planet into the space room. */
static void fly_to_space(GameWorld *g, struct char_data *ch, room_vnum space)
{
  reveal_hiding(g, ch, 0);
  char_set_altitude(ch, 2);
  char_aff_set(ch, aff_flag::flying, true);
  if (!block_calc(g, ch)) {
    return;
  }
  char_set_altitude(ch, 0);
  char_aff_set(ch, aff_flag::flying, false);
  if (struct zone_data *zone = find_zone_by_thing(g, char_get_in_room(ch)))
    fly_zone(g, zone, "can be seen blasting off into space!@n\r\n", ch);
  send_to_sense(g, 1, "leaving the planet", ch);
  send_to_scouter(g, "A powerlevel signal has left the planet", ch, 0, 2);
  tell_both(g, ch, "@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n",
            "@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n");
  char_from_room(g, ch);
  char_to_room(g, ch, space);
  act(g, "@C$n blasts up from the atmosphere below and then comes to a stop.@n", true, ch, nullptr, nullptr, act_to::room);
  send_to_char(g, ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
  if (!char_is_android(ch)) {
    decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
  }
  char_set_wait(ch, PULSE_3SEC);
}

namespace {
  /* By the planet's flag, the space above it, and the one room it can be left from if there is one */
  struct launch {
    room_flag planet;
    room_vnum space, only_from;
  };

  constexpr launch LAUNCHES[] = {
      {room_flag::earth,   50,  NOWHERE},
      {room_flag::cerria,  198, NOWHERE},
      {room_flag::vegeta,  53,  NOWHERE},
      {room_flag::kanassa, 58,  14904  },
      {room_flag::frigid,  51,  NOWHERE},
      {room_flag::konack,  52,  NOWHERE},
      {room_flag::namek,   54,  NOWHERE},
      {room_flag::aether,  55,  NOWHERE},
      {room_flag::yardrat, 56,  NOWHERE},
      {room_flag::arlia,   59,  NOWHERE},
  };

  void fly_space(GameWorld *g, struct char_data *ch)
  {
    if (!char_is_outside(g, ch)) {
      send_to_char(g, ch, "You are not outside!");
      return;
    }
    if (getCurKI(g, ch) < getMaxKI(g, ch) / 10 && !char_is_android(ch)) {
      send_to_char(g, ch, "You do not have the ki to fly to space.");
      return;
    }
    if (char_get_fighting(g, ch)) {
      send_to_char(g, ch, "You are too busy fighting!");
      return;
    }
    room_vnum here = char_get_in_room(ch);
    auto from      = std::ranges::find_if(LAUNCHES, [&](const launch &l) { return room_flagged(g, here, l.planet); });
    if (from != std::end(LAUNCHES)) {
      if (from->only_from != NOWHERE && here != from->only_from)
        send_to_char(g, ch, "You can only fly off the planet from the launchpad of Aquis.\r\n");
      else
        fly_to_space(g, ch, from->space);
      return;
    }
    if (room_planet_zenith(g, here)) {
      fly_to_space(g, ch, 57);
      return;
    }
    send_to_char(g, ch, "You are not on a planet.\r\n");
  }

  /* Taking off, to this altitude */
  void take_off(GameWorld *g, struct char_data *ch, int altitude, std::string_view to_char, std::string_view to_room)
  {
    if (getCurKI(g, ch) < getMaxKI(g, ch) / 100 && !char_is_android(ch)) {
      send_to_char(g, ch, "You do not have the ki to fly.");
      return;
    }
    reveal_hiding(g, ch, 0);
    tell_both(g, ch, to_char, to_room);
    if (char_get_sits(g, ch)) {
      obj_set_sitting(char_get_sits(g, ch), nullptr);
      char_set_sits(ch, nullptr);
    }
    if (char_get_position(ch) < POS_STANDING)
      char_set_position(ch, POS_STANDING);
    char_aff_set(ch, aff_flag::flying, true);
    char_set_altitude(ch, altitude);
    decCurKI(g, ch, getMaxKI(g, ch) / 100, 0);
  }

  void stop_flying(struct char_data *ch)
  {
    char_aff_set(ch, aff_flag::flying, false);
    char_set_altitude(ch, 0);
  }
} // namespace

ACMD(do_fly)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_get_absorbing(g, ch) || char_get_absorbby(g, ch) || char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You can't fly, you are struggling with someone right now!");
    return;
  }
  if (!char_is_npc(ch) && char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (!char_is_npc(ch) && char_plr_flagged(ch, player_flag::piloting)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (!char_is_npc(ch) && char_get_skill(ch, SKILL_FOCUS) < 30 && !char_is_android(ch)) {
    send_to_char(g, ch, "You do not have enough focus to hold yourself aloft.\r\n");
    send_to_char(g, ch, "@wOOC@D: @WYou need the skill Focus at @m30@W.@n\r\n");
    return;
  }

  if (arg.empty()) {
    int sect = room_sect(g, char_get_in_room(ch));
    if (char_aff_flagged(ch, aff_flag::flying) && sect == SECT_FLYING) {
      tell_both(g, ch, "@WYou begin to plummet to the ground!@n", "@W$n starts to pummet to the ground below!@n");
      stop_flying(ch);
      handle_fall(g, ch);
      return;
    }
    if (char_aff_flagged(ch, aff_flag::flying)) {
      if (sect == SECT_SPACE)
        tell_both(g, ch, "@WYou let yourself drift aimlessly through space.@n", "@W$n starts to drift slowly.!@n");
      else
        tell_both(g, ch, "@WYou slowly settle down to the ground.@n", "@W$n slowly settles down to the ground.@n");
      stop_flying(ch);
      return;
    }
    take_off(g, ch, 1, "@WYou slowly take off into the sky.@n", "@W$n slowly takes off into the sky.@n");
    return;
  }
  /* quirk: any other word does nothing */
  if (str_eq_nocase("high", arg))
    take_off(g, ch, 2, "@WYou rocket high into the sky.@n", "@W$n rockets high into the sky.@n");
  else if (str_eq_nocase("space", arg))
    fly_space(g, ch);
}

namespace {
  /* ch getting up, taking along what they sat on if it can be carried (and isn't a chair, if `but_chairs`) */
  void leave_seat(GameWorld *g, struct char_data *ch, bool but_chairs)
  {
    struct obj_data *seat = char_get_sits(g, ch);
    if (!seat)
      return;
    if (obj_wear_flagged(seat, wear_flag::take) && !(but_chairs && obj_get_type_flag(seat) == ITEM_CHAIR) &&
        char_get_carry_weight(ch) + obj_get_weight(seat) <= getMaxCarryWeight(g, ch)) {
      obj_from_room(g, seat);
      obj_to_char(g, seat, ch);
      act_triple(g, {"You pick up $p.", {}, "$n picks up $p.", true}, ch, seat, nullptr);
    }
    obj_set_sitting(seat, nullptr);
    char_set_sits(ch, nullptr);
  }

  /* The seat named to sit, rest or sleep on, or null having said why not: a bed, or a chair too if allowed */
  struct obj_data *seat_named(GameWorld *g, struct char_data *ch, std::string &arg, bool chairs_too, std::string_view not_that)
  {
    if (char_get_sits(g, ch)) {
      send_to_char(g, ch, "You are already on something!\r\n");
      return nullptr;
    }
    struct obj_data *seat = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)));
    if (!seat) {
      send_to_char(g, ch, "That isn't here.\r\n");
      return nullptr;
    }
    if (obj_get_vnum(g, seat) == 65) {
      send_to_char(g, ch, "You can't get on that!\r\n");
      return nullptr;
    }
    if (obj_get_sitting(g, seat)) {
      send_to_char(g, ch, "Someone is already on that one!\r\n");
      return nullptr;
    }
    if (obj_get_type_flag(seat) != ITEM_BED && !(chairs_too && obj_get_type_flag(seat) == ITEM_CHAIR)) {
      send_to_char(g, ch, not_that);
      return nullptr;
    }
    if (obj_get_size(seat) + 1 < get_size(ch)) {
      send_to_char(g, ch, "You are too large for it!\r\n");
      return nullptr;
    }
    return seat;
  }

  void sit_on(struct char_data *ch, struct obj_data *seat, int position)
  {
    char_set_position(ch, position);
    char_set_sits(ch, seat);
    obj_set_sitting(seat, ch);
  }

  /* What sit, rest and sleep check first: whether ch can, letting go of whomever they drag and settling if flying */
  bool can_settle(GameWorld *g, struct char_data *ch)
  {
    if (char_get_drag(g, ch))
      stop_dragging(g, ch);
    if (char_get_carrying(g, ch))
      return false;
    if (char_aff_flagged(ch, aff_flag::flying))
      do_fly(g, ch, "", 0, 0);
    return true;
  }

  void fury_subsides(GameWorld *g, struct char_data *ch)
  {
    if (!char_plr_flagged(ch, player_flag::fury))
      return;
    send_to_char(g, ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
    char_plr_set(ch, player_flag::fury, false);
  }
} // namespace

ACMD(do_stand)
{
  if (char_aff_flagged(ch, aff_flag::knocked)) {
    send_to_char(g, ch, "You are knocked out cold for right now!\r\n");
    return;
  }
  if (!char_is_npc(ch) && char_get_limb_condition(ch, 2) <= 0 && char_get_limb_condition(ch, 3) <= 0) {
    send_to_char(g, ch, "With what legs will you be standing up on?\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::piloting)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  switch (char_get_position(ch)) {
    case POS_STANDING:
      send_to_char(g, ch, "You are already standing.\r\n");
      break;
    case POS_SITTING:
      reveal_hiding(g, ch, 0);
      send_to_char(g, ch, "You stand up.\r\n");
      act(g, "$n clambers to $s feet.", true, ch, nullptr, nullptr, act_to::room);
      leave_seat(g, ch, true);
      /* May be sitting for some reason and may still be fighting. */
      char_set_position(ch, char_get_fighting(g, ch) ? POS_FIGHTING : POS_STANDING);
      break;
    case POS_RESTING:
      send_to_char(g, ch, "You stop resting, and stand up.\r\n");
      act(g, "$n stops resting, and clambers to $s feet.", true, ch, nullptr, nullptr, act_to::room);
      leave_seat(g, ch, false); /* quirk: a chair rested on is picked up, one sat on isn't */
      char_set_position(ch, POS_STANDING);
      break;
    case POS_SLEEPING:
      send_to_char(g, ch, "You have to wake up first!\r\n");
      break;
    default:
      send_to_char(g, ch, "You stop floating around, and put your feet on the ground.\r\n");
      act(g, "$n stops floating around, and puts $s feet on the ground.", true, ch, nullptr, nullptr, act_to::room);
      char_set_position(ch, POS_STANDING);
      break;
  }
}

ACMD(do_sit)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_plr_flagged(ch, player_flag::piloting)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (!can_settle(g, ch)) {
    send_to_char(g, ch, "You are busy carrying someone!\r\n");
    return;
  }

  struct obj_data *chair = nullptr;
  if (!arg.empty() && !(chair = seat_named(g, ch, arg, true, "You can't sit on that!\r\n")))
    return;
  switch (char_get_position(ch)) {
    case POS_STANDING:
      reveal_hiding(g, ch, 0);
      if (chair) {
        act_triple(g, {"You sit down on $p.", {}, "$n sits down on $p."}, ch, chair, nullptr);
        sit_on(ch, chair, POS_SITTING);
      } else {
        send_to_char(g, ch, "You sit down.\r\n");
        act(g, "$n sits down.", false, ch, nullptr, nullptr, act_to::room);
        char_set_position(ch, POS_SITTING);
      }
      break;
    case POS_SITTING:
      send_to_char(g, ch, chair ? "You should stand up first.\r\n" : "You're sitting already.\r\n");
      break;
    case POS_RESTING:
      if (chair) {
        send_to_char(g, ch, "You should stand up first.\r\n");
        break;
      }
      send_to_char(g, ch, "You stop resting, and sit up.\r\n");
      act(g, "$n stops resting.", true, ch, nullptr, nullptr, act_to::room);
      char_set_position(ch, POS_SITTING);
      break;
    case POS_SLEEPING:
      send_to_char(g, ch, "You have to wake up first.\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(g, ch, "Sit down while fighting? Are you MAD?\r\n");
      break;
    default:
      send_to_char(g, ch, "You stop floating around, and sit down.\r\n");
      act(g, "$n stops floating around, and sits down.", true, ch, nullptr, nullptr, act_to::room);
      char_set_position(ch, POS_SITTING);
      break;
  }
}

ACMD(do_rest)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_plr_flagged(ch, player_flag::piloting)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
    send_to_char(g, ch, "You can't rest here!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_aff_flagged(ch, aff_flag::sanctuary)) {
    if (char_get_skill(ch, SKILL_BARRIER)) {
      send_to_char(g, ch, "You have a barrier around you and can't rest.\r\n");
      return;
    }
    char_set_barrier(ch, 0);
    char_aff_set(ch, aff_flag::sanctuary, false);
  }
  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are a bit busy at the moment!\r\n");
    return;
  }
  if (char_get_kaioken(ch) > 0) {
    send_to_char(g, ch, "You are utilizing kaioken and can't rest!\r\n");
    return;
  }
  if (!can_settle(g, ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }

  struct obj_data *bed = nullptr;
  if (arg.empty()) {
    if (char_get_sits(g, ch) && obj_get_type_flag(char_get_sits(g, ch)) != ITEM_BED) {
      send_to_char(g, ch, "You can't lay on that!\r\n");
      return;
    }
  } else if (!(bed = seat_named(g, ch, arg, false, "You can't lay on that!\r\n"))) {
    return;
  }
  switch (char_get_position(ch)) {
    case POS_STANDING:
      reveal_hiding(g, ch, 0);
      if (bed) {
        tell_both(g, ch, bed, "You lay down and rest on $p.", "$n lays down and rests on $p.");
        sit_on(ch, bed, POS_RESTING);
      } else {
        send_to_char(g, ch, "You lay down and rest your tired bones.\r\n");
        act(g, "$n lays down and rests.", true, ch, nullptr, nullptr, act_to::room);
        char_set_position(ch, POS_RESTING);
      }
      break;
    case POS_SITTING:
      if (bed) {
        send_to_char(g, ch, "You should get up first.\r\n");
        break;
      }
      send_to_char(g, ch, "You rest your tired bones.\r\n");
      act(g, "$n rests.", true, ch, nullptr, nullptr, act_to::room);
      char_set_position(ch, POS_RESTING);
      break;
    case POS_RESTING:
      send_to_char(g, ch, "You are already resting.\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(g, ch, "You have to wake up first.\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(g, ch, "Rest while fighting?  Are you MAD?\r\n");
      break;
    default:
      send_to_char(g, ch, "You stop floating around, and stop to rest your tired bones.\r\n");
      act(g, "$n stops floating around, and rests.", false, ch, nullptr, nullptr, act_to::room);
      char_set_position(ch, POS_RESTING);
      break;
  }
}

ACMD(do_sleep)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::arenawatch)) {
    char_prf_set(g, ch, pref_flag::arenawatch, false);
    char_set_arenawatch(ch, -1);
    send_to_char(g, ch, "You stop watching the arena action.\r\n");
  }
  if (char_get_bonuses(ch, BONUS_INSOMNIAC)) {
    send_to_char(g, ch, "You don't feel the least bit tired.\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
    send_to_char(g, ch, "You can't rest here!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::piloting)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are a bit busy at the moment!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::powerup)) {
    send_to_char(g, ch, "You are busy powering up!\r\n");
    return;
  }
  if (char_aff_flagged(ch, aff_flag::sanctuary)) {
    if (char_get_skill(ch, SKILL_BARRIER) > 0) {
      send_to_char(g, ch, "You have a barrier around you and can't sleep.\r\n");
      return;
    }
    char_set_barrier(ch, 0);
    char_aff_set(ch, aff_flag::sanctuary, false);
  }
  if (char_get_kaioken(ch) > 0) {
    send_to_char(g, ch, "You are utilizing kaioken and can't sleep!\r\n");
    return;
  }
  if (char_get_sleeptime(ch) > 0) {
    send_to_char(g, ch, "You aren't sleepy enough.\r\n");
    return;
  }
  if (!can_settle(g, ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }

  struct obj_data *bed = nullptr;
  if (arg.empty()) {
    if (char_get_sits(g, ch) && obj_get_type_flag(char_get_sits(g, ch)) != ITEM_BED) {
      send_to_char(g, ch, "You can't sleep on {}.\r\n", obj_get_short_description(char_get_sits(g, ch)));
      return;
    }
  } else if (!(bed = seat_named(g, ch, arg, false, "You can't sleep on that!\r\n"))) {
    return;
  }
  switch (char_get_position(ch)) {
    case POS_SITTING:
    case POS_RESTING:
      if (bed) {
        send_to_char(g, ch, "You need to get up first!\r\n");
        break;
      }
      [[fallthrough]];
    case POS_STANDING:
      reveal_hiding(g, ch, 0);
      if (bed) {
        act_triple(g, {"You lay down on $p and sleep.", {}, "$n lays down on $p and sleeps."}, ch, bed, nullptr);
        fury_subsides(g, ch);
        sit_on(ch, bed, POS_SLEEPING);
      } else {
        send_to_char(g, ch, "You go to sleep.\r\n");
        act(g, "$n lies down and falls asleep.", true, ch, nullptr, nullptr, act_to::room);
        char_set_position(ch, POS_SLEEPING);
        fury_subsides(g, ch);
      }
      break;
    case POS_SLEEPING:
      send_to_char(g, ch, "You are already sound asleep.\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(g, ch, "Sleep while fighting?  Are you MAD?\r\n");
      break;
    default:
      send_to_char(g, ch, "You stop floating around, and lie down to sleep.\r\n");
      act(g, "$n stops floating around, and lie down to sleep.", true, ch, nullptr, nullptr, act_to::room);
      char_set_position(ch, POS_SLEEPING);
      break;
  }
}

namespace {
  /* Someone woken is let go of by whoever drags or carries them */
  void woken_and_let_go(GameWorld *g, struct char_data *vict)
  {
    if (char_get_dragged(g, vict)) {
      struct char_data *dragger = char_get_dragged(g, vict);
      act(g, "@WYou stop dragging @C$N@W!@n", true, dragger, nullptr, vict, act_to::actor);
      act(g, "@C$n@W stops dragging @c$N@W!@n", true, dragger, nullptr, vict, act_to::room);
      char_set_drag(dragger, nullptr);
      char_set_dragged(vict, nullptr);
    }
    if (char_get_carried_by(g, vict))
      put_down(g, char_get_carried_by(g, vict));
  }

  void wake_someone(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (char_is_awake(vict))
      act(g, "$E is already awake.", false, ch, nullptr, vict, act_to::actor);
    else if (char_aff_flagged(vict, aff_flag::sleep))
      act(g, "You can't wake $M up!", false, ch, nullptr, vict, act_to::actor);
    else if (char_get_position(vict) < POS_SLEEPING)
      act(g, "$E's in pretty bad shape!", false, ch, nullptr, vict, act_to::actor);
    else if (char_aff_flagged(vict, aff_flag::knocked))
      send_to_char(g, ch, "They are knocked out cold for right now!\r\n");
    else if (char_get_bonuses(ch, BONUS_LATE)) /* quirk: it's the waker's lateness, not theirs */
      send_to_char(g, ch, "They say 'Yeah yeah...' and then roll back over.\r\n");
    else {
      act(g, "You wake $M up.", false, ch, nullptr, vict, act_to::actor);
      act(g, "You are awakened by $n.", false, ch, nullptr, vict, act_to::vict | act_option::even_asleep);
      char_set_position(vict, POS_SITTING);
      woken_and_let_go(g, vict);
    }
  }
} // namespace

ACMD(do_wake)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_aff_flagged(ch, aff_flag::knocked)) {
    send_to_char(g, ch, "You are knocked out cold for right now!\r\n");
    return;
  }
  if (char_get_bonuses(ch, BONUS_LATE) && char_get_position(ch) == POS_SLEEPING) {
    send_to_char(g, ch, "Nah you're enjoying sleeping too much.\r\n");
    return;
  }
  if (!arg.empty()) {
    struct char_data *vict;
    if (char_get_position(ch) == POS_SLEEPING) {
      send_to_char(g, ch, "Maybe you should wake yourself up first.\r\n");
      return;
    }
    if (!(vict = get_char_vis(g, ch, arg, nullptr, find_flag::char_room))) {
      send_to_char(g, ch, CONFIG_NOPERSON);
      return;
    }
    if (vict != ch) {
      wake_someone(g, ch, vict);
      return;
    }
  }
  if (char_aff_flagged(ch, aff_flag::sleep)) {
    send_to_char(g, ch, "You can't wake up!\r\n");
    return;
  }
  if (char_get_position(ch) > POS_SLEEPING) {
    send_to_char(g, ch, "You are already awake...\r\n");
    return;
  }
  send_to_char(g, ch, "You awaken, and sit up.\r\n");
  act(g, "$n awakens.", true, ch, nullptr, nullptr, act_to::room);
  if (char_get_dragged(g, ch)) {
    act_triple(g, {"@WYou stop dragging @C$N@W!@n", "@C$n@W stops dragging you!@n", "@C$n@W stops dragging @c$N@W!@n", true}, char_get_dragged(g, ch), nullptr, ch);
    char_set_drag(char_get_dragged(g, ch), nullptr);
    char_set_dragged(ch, nullptr);
  }
  if (char_get_carried_by(g, ch))
    put_down(g, char_get_carried_by(g, ch));
  char_set_position(ch, POS_SITTING);
}

ACMD(do_follow)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (buf.empty()) {
    send_to_char(g, ch, "Whom do you wish to follow?\r\n");
    return;
  }
  struct char_data *leader = get_char_vis(g, ch, buf, nullptr, find_flag::char_room);
  if (!leader) {
    send_to_char(g, ch, CONFIG_NOPERSON);
    return;
  }
  if (char_get_master(g, ch) == leader) {
    act(g, "You are already following $M.", false, ch, nullptr, leader, act_to::actor);
    return;
  }
  if (char_aff_flagged(ch, aff_flag::charm) && char_get_master(g, ch)) {
    act(g, "But you only feel like following $N!", false, ch, nullptr, char_get_master(g, ch), act_to::actor);
    return;
  }
  if (leader == ch) {
    if (!char_get_master(g, ch))
      send_to_char(g, ch, "You are already following yourself.\r\n");
    else
      stop_follower(g, ch);
    return;
  }
  if (circle_follow(g, ch, leader)) {
    send_to_char(g, ch, "Sorry, but following in loops is not allowed.\r\n");
    return;
  }
  if (char_get_master(g, ch))
    stop_follower(g, ch);
  char_aff_set(ch, aff_flag::group, false);
  reveal_hiding(g, ch, 0);
  add_follower(g, ch, leader);
}
