/* ************************************************************************
*   File: wizard/info.c                                  Part of CircleMUD *
*  Usage: God commands that inspect the world                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <vector>

#include <unistd.h>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/time.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/extract.hpp"
#include "dbat/affect.hpp"
#include "dbat/config.hpp"
#include "dbat/act.other.hpp"
#include "dbat/maputils.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/handler.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/weather.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/house.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/races.hpp"
#include "dbat/class.hpp"
#include "dbat/spells.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/objsave.hpp"
#include "dbat/feats.hpp"
#include "dbat/fight.hpp"
#include "dbat/genolc.hpp"
#include "dbat/screen.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void do_stat_room(GameWorld *g, struct char_data *ch);
static void do_stat_object(GameWorld *g, struct char_data *ch, struct obj_data *j);
static void do_stat_character(GameWorld *g, struct char_data *ch, struct char_data *k);
static std::string zone_summary(GameWorld *g, struct zone_data *zone, int listall);
static void mob_checkload(GameWorld *g, struct char_data *ch, mob_vnum mvnum);
static void obj_checkload(GameWorld *g, struct char_data *ch, obj_vnum ovnum);
static void trg_checkload(GameWorld *g, struct char_data *ch, trig_vnum tvnum);

namespace {
  /* when, as asctime() writes it but without its newline */
  std::string clock_text(time_t when)
  {
    std::string text = asctime(localtime(&when));
    text.pop_back();
    return text;
  }

  /* A string the getters give as null when empty, as "" */
  const char *text_of(const char *s)
  {
    return s ? s : "";
  }
} // namespace

/* do_finddoor, finds the door(s) that a key goes to */
ACMD(do_finddoor)
{
  int d, num = 0;
  obj_vnum vnum = NOTHING;
  std::string arg;
  std::string buf;
  struct char_data *tmp_char;
  struct obj_data *obj;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Format: finddoor <obj/vnum>\r\n");
  } else if (is_number(arg)) {
    vnum = parse_int(arg);
    if (!obj_proto_of(g, vnum)) {
      send_to_char(g, ch, "There is no object with that vnum.\r\n");
      return;
    }
    obj = obj_proto_of(g, vnum);
  } else {
    generic_find(g, arg,
                 FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP,
                 ch, &tmp_char, &obj);
    if (!obj)
      send_to_char(g, ch, "What key do you want to find a door for?\r\n");
    else
      vnum = obj_get_vnum(g, obj);
  }
  if (vnum != NOTHING) {
    buf = str_format("Doors unlocked by key [{:d}] {} are:\r\n",
                     vnum, obj_get_short_description(obj));
    for (room_vnum i : g->rooms.vnums()) {
      for (d = 0; d < NUM_OF_DIRS; d++) {
        if (room_get_dir_option(g, i, d) && room_get_dir_option(g, i, d)->key &&
            room_get_dir_option(g, i, d)->key == vnum) {
          buf += str_format("[{:>3d}] Room {:d}, {} ({})\r\n",
                            ++num, i,
                            dirs[d], exit_keyword(room_get_dir_option(g, i, d)));
        }
      } /* for all directions */
    } /* for all rooms */
    if (num > 0)
      send_to_char(g, ch, buf);
    else
      send_to_char(g, ch, "No doors were found for key [{:d}] {}.\r\n",
                   vnum, obj_get_short_description(obj));
  }
}

namespace {
  /* What vnum lists, by the word for it: every kind the word abbreviates is listed */
  struct vnum_kind {
    const char *name;
    int (*list)(GameWorld *, std::string_view, struct char_data *);
    const char *none;
  };
  const vnum_kind VNUM_KINDS[] = {
    {"mob", vnum_mobile, "No mobiles by that name.\r\n"},
    {"obj", vnum_object, "No objects by that name.\r\n"},
    {"mat", vnum_material, "No materials by that name.\r\n"},
    {"wtype", vnum_weapontype, "No weapon types by that name.\r\n"},
    {"atype", vnum_armortype, "No armor types by that name.\r\n"},
  };
} // namespace

ACMD(do_vnum)
{
  std::string_view rest = argument;
  std::string kind      = take_word(rest);
  std::string name(skip_spaces(rest));

  auto named = [&](const vnum_kind &k) { return is_abbrev(kind, k.name); };
  if (kind.empty() || name.empty() || std::ranges::none_of(VNUM_KINDS, named)) {
    send_to_char(g, ch, "Usage: vnum { atype | material | mob | obj | wtype } <name>\r\n");
    return;
  }
  for (const vnum_kind &k : VNUM_KINDS | std::views::filter(named))
    if (!k.list(g, name, ch))
      send_to_char(g, ch, k.none);
}

#define ZOCMD zrnum->cmd[subcmd]

static void list_zone_commands_room(GameWorld *g, struct char_data *ch, room_vnum rvnum)
{
  struct zone_data *zrnum = find_zone_by_thing(g, rvnum);
  room_vnum rrnum = rvnum, cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == nullptr || !room_of(g, rrnum)) {
    send_to_char(g, ch, "No zone information available.\r\n");
    return;
  }

  send_to_char(g, ch, "Zone commands in this room:@y\r\n");
  while (ZOCMD.command != 'S') {
    switch (ZOCMD.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = ZOCMD.arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = ZOCMD.arg1;
        break;
      default:
        break;
    }
    if (cmd_room == rrnum) {
      count++;
      /* start listing */
      switch (ZOCMD.command) {
        case 'M':
          send_to_char(g, ch, "{}Load {}@y [@c{:d}@y], MaxMud : {:d}, MaxR : {:d}, Chance : {:d}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       char_get_sdesc(mob_proto_of(g, ZOCMD.arg1)),
                       mob_index_of(g, ZOCMD.arg1)->vnum, ZOCMD.arg2,
                       ZOCMD.arg4, ZOCMD.arg5);
          break;
        case 'G':
          send_to_char(g, ch, "{}Give it {}@y [@c{:d}@y], Max : {:d}, Chance : {:d}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(obj_proto_of(g, ZOCMD.arg1)),
                       obj_index_of(g, ZOCMD.arg1)->vnum,
                       ZOCMD.arg2, ZOCMD.arg5);
          break;
        case 'O':
          send_to_char(g, ch, "{}Load {}@y [@c{:d}@y], Max : {:d}, MaxR : {:d}, Chance : {:d}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(obj_proto_of(g, ZOCMD.arg1)),
                       obj_index_of(g, ZOCMD.arg1)->vnum,
                       ZOCMD.arg2, ZOCMD.arg4, ZOCMD.arg5);
          break;
        case 'E':
          send_to_char(g, ch, "{}Equip with {}@y [@c{:d}@y], {}, Max : {:d}, Chance : {:d}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(obj_proto_of(g, ZOCMD.arg1)),
                       obj_index_of(g, ZOCMD.arg1)->vnum,
                       equipment_types[ZOCMD.arg3],
                       ZOCMD.arg2, ZOCMD.arg5);
          break;
        case 'P':
          send_to_char(g, ch, "{}Put {}@y [@c{:d}@y] in {}@y [@c{:d}@y], Max : {:d}, Chance : {:d}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(obj_proto_of(g, ZOCMD.arg1)),
                       obj_index_of(g, ZOCMD.arg1)->vnum,
                       obj_get_short_description(obj_proto_of(g, ZOCMD.arg3)),
                       obj_index_of(g, ZOCMD.arg3)->vnum,
                       ZOCMD.arg2, ZOCMD.arg5);
          break;
        case 'R':
          send_to_char(g, ch, "{}Remove {}@y [@c{:d}@y] from room.\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       obj_get_short_description(obj_proto_of(g, ZOCMD.arg2)),
                       obj_index_of(g, ZOCMD.arg2)->vnum);
          break;
        case 'D':
          send_to_char(g, ch, "{}Set door {} as {}.\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       dirs[ZOCMD.arg2],
                       ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open");
          break;
        case 'T':
          send_to_char(g, ch, "{}Attach trigger @c{}@y [@c{:d}@y] to {}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       find_trigger(g, ZOCMD.arg2)->proto->name,
                       ZOCMD.arg2,
                       ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" : ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))));
          break;
        case 'V':
          send_to_char(g, ch, "{}Assign global {}:{:d} to {} = {}\r\n",
                       ZOCMD.if_flag ? " then " : "",
                       ZOCMD.sarg1, ZOCMD.arg2,
                       ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" : ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))),
                       ZOCMD.sarg2);
          break;
        default:
          send_to_char(g, ch, "<Unknown Command>\r\n");
          break;
      }
    }
    subcmd++;
  }
  send_to_char(g, ch, "@n");
  if (!count)
    send_to_char(g, ch, "None!\r\n");
}
#undef ZOCMD

static void do_stat_room(GameWorld *g, struct char_data *ch)
{
  std::string buf2;
  struct room_data *rm = room_of(g, char_get_in_room(ch));
  int i, found, column;

  send_to_char(g, ch, "Room name: @c{}@n\r\n", room_get_name(rm));

  buf2 = sprinttype(room_get_sector_type(rm), sector_types);
  send_to_char(g, ch, "Zone: [{:>3d}], VNum: [@g{:>5d}@n], RNum: [{:>5d}], IDNum: [{:>5d}], Type: {}\r\n",
               room_get_zone(rm)->number, room_get_number(rm), char_get_in_room(ch),
               (long)room_get_number(rm) + ROOM_ID_BASE, buf2);

  buf2 = sprintbitarray(room_get_room_flags(rm), room_bits, RF_ARRAY_MAX);
  send_to_char(g, ch, "Room Damage: {:d}, Room Effect: {:d}\r\n", room_get_dmg(rm), room_get_geffect(rm));
  send_to_char(g, ch, "SpecProc: {}, Flags: {}\r\n", room_get_func(rm) == nullptr ? "None" : "Exists", buf2);

  send_to_char(g, ch, "Description:\r\n{}", room_get_description(rm) ? room_get_description(rm) : "  None.\r\n");

  if (!room_get_ex_description(rm).empty()) {
    send_to_char(g, ch, "Extra descs:");
    for (const extra_descr_data &desc : room_get_ex_description(rm))
      send_to_char(g, ch, " [@c{}@n]", desc.keyword);
    send_to_char(g, ch, "\r\n");
  }

  send_to_char(g, ch, "Chars present:");
  column = 14; /* ^^^ strlen ^^^ */
  found  = false;
  for (char_data *k : room_people(g, rm)) {
    if (!char_can_see(g, ch, k))
      continue;

    column += send_to_char(g, ch, "{} @y{}@n({})", found++ ? "," : "", char_get_name(k),
                           !char_is_npc(k) ? "PC" : (!char_is_mob(g, k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(g, ch, "{}\r\n", char_next_in_room(g, k) ? "," : "");
      found  = false;
      column = 0;
    }
  }

  if (room_get_contents(rm)) {
    send_to_char(g, ch, "Contents:@g");
    column = 9; /* ^^^ strlen ^^^ */

    found = 0;
    for (obj_data *j : room_contents(g, rm)) {
      if (!char_can_see_obj(g, ch, j))
        continue;

      column += send_to_char(g, ch, "{} {}", found++ ? "," : "", obj_get_short_description(j));
      if (column >= 62) {
        send_to_char(g, ch, "{}\r\n", obj_next_in_list(g, j) ? "," : "");
        found  = false;
        column = 0;
      }
    }
    send_to_char(g, ch, "@n");
  }

  for (i = 0; i < NUM_OF_DIRS; i++) {
    std::string buf1;

    if (!room_get_dir_option(rm, i))
      continue;

    if (room_get_dir_option(rm, i)->to_room == NOWHERE)
      buf1 = " @cNONE@n";
    else
      buf1 = str_format("@c{:>5d}@n", room_get_dir_option(rm, i)->to_room);

    buf2 = sprintbit(room_get_dir_option(rm, i)->exit_info, exit_bits);

    send_to_char(g, ch, "Exit @c{:<5}@n:  To: [{}], Key: [{:>5d}], Keywrd: {}, Type: {}\r\n  DC Lock: [{:>2d}], DC Hide: [{:>2d}], DC Skill: [{:>4}], DC Move: [{:>2d}]\r\n{}",
                 dirs[i], buf1,
                 room_get_dir_option(rm, i)->key == NOTHING ? -1 : room_get_dir_option(rm, i)->key,
                 exit_keyword(room_get_dir_option(rm, i)) ? exit_keyword(room_get_dir_option(rm, i)) : "None", buf2,
                 room_get_dir_option(rm, i)->dclock, room_get_dir_option(rm, i)->dchide,
                 room_get_dir_option(rm, i)->dcskill == 0 ? "None" : g->spell_info[room_get_dir_option(rm, i)->dcskill].name, room_get_dir_option(rm, i)->dcmove,
                 exit_description(room_get_dir_option(rm, i)) ? exit_description(room_get_dir_option(rm, i)) : "  No exit description.\r\n");
  }

  /* check the room for a script */
  do_sstat_room(g, ch, rm);

  list_zone_commands_room(g, ch, room_get_number(rm));
}

static void do_stat_object(GameWorld *g, struct char_data *ch, struct obj_data *j)
{
  int i, found;
  obj_vnum vnum;
  struct char_data *sitter;
  std::string buf;

  vnum = obj_get_vnum(g, j);
  if (obj_get_lload(j) > 0)
    send_to_char(g, ch, "LOADED DROPPED: [{}]\r\n", clock_text(obj_get_lload(j)));
  if (obj_get_vnum(g, j) == 65) {
    send_to_char(g, ch, "Healing Tank Charge Level: [{:d}]\r\n", obj_get_healcharge(j));
  }
  send_to_char(g, ch, "Name: '{}', Keywords: {}, Size: {}\r\n",
               obj_get_short_description(j) ? obj_get_short_description(j) : "<None>", obj_get_name(j),
               size_names[obj_get_size(j)]);

  buf = sprinttype(obj_get_type_flag(j), item_types);
  send_to_char(g, ch, "VNum: [@g{:>5d}@n], RNum: [{:>5d}], Idnum: [{:>5d}], Type: {}, SpecProc: {}\r\n",
               vnum, obj_get_item_number(j), obj_get_id(j), buf, obj_get_spec(g, j) ? "Exists" : "None");

  time_t generation = obj_get_generation(j);
  send_to_char(g, ch, "Generation time: @g{}@nUnique ID: @g{:d}@n\r\n",
               ctime(&generation), obj_get_unique_id(j));

  send_to_char(g, ch, "Object Hit Points: [ @g{:>3d}@n/@g{:>3d}@n]\r\n",
               obj_get_value(j, VAL_ALL_HEALTH), obj_get_value(j, VAL_ALL_MAXHEALTH));

  send_to_char(g, ch, "Object loaded in room: @y{:d}@n\r\n",
               obj_get_room_loaded(j));

  send_to_char(g, ch, "Object Material: @y{}@n\r\n",
               material_names[obj_get_value(j, 7)]);

  if (obj_get_sitting(g, j)) {
    sitter = obj_get_sitting(g, j);
    send_to_char(g, ch, "HOLDING: {}\r\n", char_get_name(sitter));
  }

  if (!obj_get_ex_description(j).empty()) {
    send_to_char(g, ch, "Extra descs:");
    for (const extra_descr_data &desc : obj_get_ex_description(j))
      send_to_char(g, ch, " [@g{}@n]", desc.keyword);
    send_to_char(g, ch, "\r\n");
  }

  buf = sprintbitarray(obj_get_wear_flags(j), wear_bits, TW_ARRAY_MAX);
  send_to_char(g, ch, "Can be worn on: {}\r\n", buf);

  buf = sprintbitarray(obj_get_bitvector(j), affected_bits, AF_ARRAY_MAX);
  send_to_char(g, ch, "Set char bits : {}\r\n", buf);

  buf = sprintbitarray(obj_get_extra_flags(j), extra_bits, EF_ARRAY_MAX);
  send_to_char(g, ch, "Extra flags   : {}\r\n", buf);

  send_to_char(g, ch, "Weight: {:d}, Value: {:d}, Cost/day: {:d}, Timer: {:d}, Min Level: {:d}\r\n",
               obj_get_weight(j), obj_get_cost(j), obj_get_cost_per_day(j), obj_get_timer(j), obj_get_level(j));

  send_to_char(g, ch, "In room: {:d} ({}), ", obj_get_in_room(j),
               obj_get_in_room(j) == NOWHERE ? "Nowhere" : room_get_name(g, obj_get_in_room(j)));

  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  send_to_char(g, ch, "In object: {}, ", obj_get_in_obj(j) ? obj_get_short_description(obj_get_in_obj(j)) : "None");
  send_to_char(g, ch, "Carried by: {}, ", obj_get_carried_by(j) ? char_get_name(obj_get_carried_by(j)) : "Nobody");
  send_to_char(g, ch, "Worn by: {}\r\n", obj_get_worn_by(j) ? char_get_name(obj_get_worn_by(j)) : "Nobody");

  switch (obj_get_type_flag(j)) {
    case ITEM_LIGHT:
      if (obj_get_value(j, VAL_LIGHT_HOURS) == -1)
        send_to_char(g, ch, "Hours left: Infinite\r\n");
      else
        send_to_char(g, ch, "Hours left: [{:d}]\r\n", obj_get_value(j, VAL_LIGHT_HOURS));
      break;
    case ITEM_SCROLL:
      send_to_char(g, ch, "Spell: (Level {:d}) {}\r\n", obj_get_value(j, VAL_SCROLL_LEVEL),
                   skill_name(g, obj_get_value(j, VAL_SCROLL_SPELL1)));
      break;
    case ITEM_POTION:
      send_to_char(g, ch, "Spells: (Level {:d}) {}, {}, {}\r\n", obj_get_value(j, VAL_POTION_LEVEL),
                   skill_name(g, obj_get_value(j, VAL_POTION_SPELL1)),
                   skill_name(g, obj_get_value(j, VAL_POTION_SPELL2)),
                   skill_name(g, obj_get_value(j, VAL_POTION_SPELL3)));
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      send_to_char(g, ch, "Spell: {} at level {:d}, {:d} (of {:d}) charges remaining\r\n",
                   skill_name(g, obj_get_value(j, VAL_STAFF_SPELL)), obj_get_value(j, VAL_STAFF_LEVEL),
                   obj_get_value(j, VAL_STAFF_CHARGES), obj_get_value(j, VAL_STAFF_MAXCHARGES));
      break;
    case ITEM_WEAPON:
      send_to_char(g, ch, "Weapon Type: {}, Todam: {:d}d{:d}, Message type: {:d}\r\n",
                   weapon_type[obj_get_value(j, VAL_WEAPON_SKILL)],
                   obj_get_value(j, VAL_WEAPON_DAMDICE),
                   obj_get_value(j, VAL_WEAPON_DAMSIZE),
                   obj_get_value(j, VAL_WEAPON_DAMTYPE));
      send_to_char(g, ch, "Average damage per round {:.1f}\r\n", ((obj_get_value(j, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * obj_get_value(j, VAL_WEAPON_DAMDICE));
      send_to_char(g, ch, "Crit type: {}, Crit range: {:d}-20\r\n",
                   crit_type[obj_get_value(j, 6)], 20 - obj_get_value(j, 8));
      break;
    case ITEM_ARMOR:
      send_to_char(g, ch, "Armor Type: {}, AC-apply: [{:d}]\r\n", armor_type[obj_get_value(j, VAL_ARMOR_SKILL)], obj_get_value(j, VAL_ARMOR_APPLYAC));
      send_to_char(g, ch, "Max dex bonus: {:d}, Armor penalty: {:d}, Spell failure: {:d}\r\n",
                   obj_get_value(j, VAL_ARMOR_MAXDEXMOD), obj_get_value(j, VAL_ARMOR_CHECK), obj_get_value(j, VAL_ARMOR_SPELLFAIL));
      break;
    case ITEM_TRAP:
      send_to_char(g, ch, "Spell: {:d}, - Hitpoints: {:d}\r\n", obj_get_value(j, VAL_TRAP_SPELL), obj_get_value(j, VAL_TRAP_HITPOINTS));
      break;
    case ITEM_CONTAINER:
      buf = sprintbit(obj_get_value(j, VAL_CONTAINER_FLAGS), container_bits);
      send_to_char(g, ch, "Weight capacity: {:d}, Lock Type: {}, Key Num: {:d}, Corpse: {}\r\n",
                   obj_get_value(j, VAL_CONTAINER_CAPACITY), buf, obj_get_value(j, VAL_CONTAINER_KEY),
                   YESNO(obj_get_value(j, VAL_CONTAINER_CORPSE)));
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      buf = sprinttype(obj_get_value(j, VAL_DRINKCON_LIQUID), drinks);
      send_to_char(g, ch, "Capacity: {:d}, Contains: {:d}, Poisoned: {}, Liquid: {}\r\n",
                   obj_get_value(j, VAL_DRINKCON_CAPACITY), obj_get_value(j, VAL_DRINKCON_HOWFULL), YESNO(obj_get_value(j, VAL_DRINKCON_POISON)), buf);
      break;
    case ITEM_NOTE:
      send_to_char(g, ch, "Tongue: {:d}\r\n", obj_get_value(j, VAL_NOTE_LANGUAGE));
      break;
    case ITEM_KEY:
      /* Nothing */
      break;
    case ITEM_FOOD:
      send_to_char(g, ch, "Makes full: {:d}, Poisoned: {}\r\n", obj_get_value(j, VAL_FOOD_FOODVAL), YESNO(obj_get_value(j, VAL_FOOD_POISON)));
      break;
    case ITEM_MONEY:
      send_to_char(g, ch, "Coins: {:d}\r\n", obj_get_value(j, VAL_MONEY_SIZE));
      break;
    default:
      send_to_char(g, ch, "Values 0-12: [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}] [{:d}]\r\n",
                   obj_get_value(j, 0), obj_get_value(j, 1),
                   obj_get_value(j, 2), obj_get_value(j, 3),
                   obj_get_value(j, 4), obj_get_value(j, 5),
                   obj_get_value(j, 6), obj_get_value(j, 7),
                   obj_get_value(j, 8), obj_get_value(j, 9),
                   obj_get_value(j, 10), obj_get_value(j, 11));
      break;
  }

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (obj_get_contains(j)) {
    int column;

    send_to_char(g, ch, "\r\nContents:@g");
    column = 9; /* ^^^ strlen ^^^ */

    found = 0;
    for (obj_data *j2 : obj_contents(g, j)) {
      column += send_to_char(g, ch, "{} {}", found++ ? "," : "", obj_get_short_description(j2));
      if (column >= 62) {
        send_to_char(g, ch, "{}\r\n", obj_next_in_list(g, j2) ? "," : "");
        found  = false;
        column = 0;
      }
    }
    send_to_char(g, ch, "@n");
  }

  found = false;
  send_to_char(g, ch, "Affections:");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj_get_affected(j)[i].modifier) {
      buf = sprinttype(obj_get_affected(j)[i].location, apply_types);
      send_to_char(g, ch, "{} {:+d} to {}", found++ ? "," : "", obj_get_affected(j)[i].modifier, buf);
      switch (obj_get_affected(j)[i].location) {
        case APPLY_FEAT:
          send_to_char(g, ch, " ({})", g->feat_list[obj_get_affected(j)[i].specific].name);
          break;
        case APPLY_SKILL:
          send_to_char(g, ch, " ({})", g->spell_info[obj_get_affected(j)[i].specific].name);
          break;
      }
    }
  if (!found)
    send_to_char(g, ch, " None");

  send_to_char(g, ch, "\r\n");

  /* check the object for a script */
  do_sstat_object(g, ch, j);
}

static void do_stat_character(GameWorld *g, struct char_data *ch, struct char_data *k)
{
  std::string buf;
  std::string buf2;
  int i, i2, column, found = false;
  struct obj_data *chair;
  struct affected_type *aff;

  if (char_is_npc(k))
    send_to_char(g, ch, "LOADED AT: [{}]\r\n", clock_text(char_get_lastpl(k)));
  buf = sprinttype(char_get_sex(k), genders);
  send_to_char(g, ch, "{} {} '{}'  IDNum: [{:>5d}], In room [{:>5d}], Loadroom : [{:>5d}]\r\n",
               buf, (!char_is_npc(k) ? "PC" : (!char_is_mob(g, k) ? "NPC" : "MOB")),
               char_get_name(k), char_is_npc(k) ? char_get_id(k) : char_get_idnum(k), char_get_in_room(k), char_is_npc(k) ? char_get_hometown(k) : char_get_load_room(g, k));

  send_to_char(g, ch, "DROOM: [{:>5d}]\r\n", char_get_droom(k));
  if (char_is_mob(g, k)) {
    if (char_get_master_id(k) > -1)
      buf = str_format(", Master: {}", get_name_by_id(g, char_get_master_id(k)));
    else
      buf.clear();
    send_to_char(g, ch, "Keyword: {}, VNum: [{:>5d}], RNum: [{:>5d}]{}\r\n", char_get_alias(k),
                 char_get_mob_vnum(g, k), char_get_nr(k), buf);
  } else

    send_to_char(g, ch, "Title: {}\r\n", char_get_raw_title(k) ? char_get_raw_title(k) : "<None>");

  send_to_char(g, ch, "L-Des: {}@n", char_get_ldesc(k) ? char_get_ldesc(k) : "<None>\r\n");
  if (CONFIG_ALLOW_MULTICLASS) {
    buf = class_desc_str(k, 1, 0);
  } else {
    buf = str_format("{}", char_sensei_name(k));
  }
  buf2 = str_format("{}", char_true_race_name(k));
  send_to_char(g, ch, "Class: {}, Race: {}, Lev: [@y{:>2d}({:d}HD+{:d}cl+{:d})@n], XP: [@y{:d}@n]\r\n",
               buf, buf2, char_get_level(k), char_get_race_level(k),
               char_get_class_level(k), char_get_level_adj(k), char_get_exp(k));

  if (!char_is_npc(k)) {
    char buf1[64], cmbuf2[64];
    time_t created = char_get_time_created(k), logon = char_get_time_logon(k);

    strlcpy(buf1, asctime(localtime(&created)), sizeof(buf1));
    strlcpy(cmbuf2, asctime(localtime(&logon)), sizeof(cmbuf2));
    buf1[10] = cmbuf2[10] = '\0';

    send_to_char(g, ch, "Created: [{}], Last Logon: [{}], Played [{:d}h {:d}m], Age [{:d}]\r\n",
                 buf1, cmbuf2, (int)char_get_time_played(k) / 3600,
                 (int)((char_get_time_played(k) % 3600) / 60), age(g, k)->year);

    if (char_get_desc(k) != nullptr) {
      send_to_char(g, ch, "@YOwned by User@D: [@C{}@D]@n\r\n", char_get_user(k));
    } else {
      send_to_char(g, ch, "@YOwned by User@D: [@C{}@D]@n\r\n", char_get_loguser(k));
    }
    if (!char_is_npc(k)) {
      send_to_char(g, ch, "@RCharacter Deaths@D: @r{:d}@n\r\n", char_get_dcount(k));
    }

    send_to_char(g, ch, "Hometown: [{:d}], Align: [{:>4d}]", char_get_hometown(k),
                 char_get_alignment(k));

    /*. Display OLC zone for immorts .*/
    if (char_get_admlevel(k) >= ADMLVL_BUILDER) {
      if (char_get_olc_zone(g, k) == AEDIT_PERMISSION)
        send_to_char(g, ch, ", OLC[@cActions@n]");
      else if (char_get_olc_zone(g, k) == HEDIT_PERMISSION)
        send_to_char(g, ch, ", OLC[@cHedit@n]");
      else if (char_get_olc_zone(g, k) == NOWHERE)
        send_to_char(g, ch, ", OLC[@cOFF@n]");
      else
        send_to_char(g, ch, ", OLC: [@c{:d}@n]", char_get_olc_zone(g, k));
    }
    send_to_char(g, ch, "\r\n");
  }
  send_to_char(g, ch, "Str: [@c{:d}@n]  Int: [@c{:d}@n]  Wis: [@c{:d}@n]  Dex: [@c{:d}@n]  Con: [@c{:d}@n]  Cha: [@c{:d}@n]\r\n",
               char_get_aff_abils_str(k), char_get_aff_abils_intel(k), char_get_aff_abils_wis(k), char_get_aff_abils_dex(k), char_get_aff_abils_con(k), char_get_aff_abils_cha(k));

  send_to_char(g, ch, "PL :[@g{:>12}@n]  KI :[@g{:>12}@n]  ST :[@g{:>12}@n]\r\n",
               add_commas(getCurPL(g, k)), add_commas((getCurKI(g, k))), add_commas((getCurST(g, k))));
  send_to_char(g, ch, "MPL:[@g{:>12}@n]  MKI:[@g{:>12}@n]  MST:[@g{:>12}@n]\r\n",
               add_commas(getEffMaxPL(g, k)), add_commas(getMaxKI(g, k)), add_commas(getMaxST(g, k)));
  send_to_char(g, ch, "BPL:[@g{:>12}@n]  BKI:[@g{:>12}@n]  BST:[@g{:>12}@n]\r\n",
               add_commas((getBasePL(k))), add_commas((getBaseKI(k))), add_commas((getBaseST(k))));
  send_to_char(g, ch, "LF :[@g{:>12}@n]  MLF:[@g{:>12}@n]  LFP:[@g{:>3d}@n]\r\n",
               add_commas((getCurLF(g, k))), add_commas((getMaxLF(g, k))), char_get_lifeperc(k));

  if (char_get_admlevel(k))
    send_to_char(g, ch, "Admin Level: [@y{:d} - {}@n]\r\n", char_get_admlevel(k), admin_level_names[char_get_admlevel(k)]);

  send_to_char(g, ch, "Coins: [{:>9d}], Bank: [{:>9d}] (Total: {:d})\r\n",
               char_get_gold(k), char_get_bank_gold(k), char_get_gold(k) + char_get_bank_gold(k));

  send_to_char(g, ch, "Armor: [{:d} ]\r\n", char_get_armor(k));

  buf = sprinttype(char_get_position(k), position_types);
  send_to_char(g, ch, "Pos: {}, Fighting: {}", buf, char_get_fighting(g, k) ? char_get_name(char_get_fighting(g, k)) : "Nobody");

  if (char_get_desc(k)) {
    buf = sprinttype(desc_get_connected(char_get_desc(k)), connected_types);
    send_to_char(g, ch, ", Connected: {}", buf);
  }

  if (char_is_npc(k)) {
    buf = sprinttype(char_get_mob_specials_default_pos(k), position_types);
    send_to_char(g, ch, ", Default position: {}\r\n", buf);
    buf = sprintbitarray(char_get_act(k), action_bits, PM_ARRAY_MAX);
    send_to_char(g, ch, "NPC flags: @c{}@n\r\n", buf);
  } else {
    send_to_char(g, ch, ", Idle Timer (in tics) [{:d}]\r\n", char_get_timer(k));

    buf = sprintbitarray(char_get_act(k), player_bits, PM_ARRAY_MAX);
    send_to_char(g, ch, "PLR: @c{}@n\r\n", buf);

    buf = sprintbitarray(char_get_pref(g, k), preference_bits, PR_ARRAY_MAX);
    send_to_char(g, ch, "PRF: @g{}@n\r\n", buf);
  }

  if (char_is_mob(g, k)) {
    send_to_char(g, ch, "Mob Spec-Proc: {}, NPC Bare Hand Dam: {:d}d{:d}\r\n",
                 (mob_index_of(g, char_get_nr(k))->func ? "Exists" : "None"),
                 char_get_mob_specials_damnodice(k), char_get_mob_specials_damsizedice(k));
    send_to_char(g, ch, "Average damage per round {:.1f} ({:.1f} [BHD] + {:d} [STR MOD])\r\n", (((char_get_mob_specials_damsizedice(k)) + 1) / 2.0) * (char_get_mob_specials_damnodice(k)) + ability_mod_value(char_get_aff_abils_str(k)), (((char_get_mob_specials_damsizedice(k)) + 1) / 2.0) * (char_get_mob_specials_damnodice(k)), ability_mod_value(char_get_aff_abils_str(k)));
  }

  int counts = 0, total = 0;
  i = 0;
  for (obj_data *item : char_inventory(g, k)) {
    counts += check_insidebag(g, item, 0.5);
    counts++;
    i++;
  }
  total = counts;
  total += i;
  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(k, i)) {
      i2++;
      total += check_insidebag(g, char_get_equipment(k, i), 0.5) + 1;
    }
  send_to_char(g, ch, "Carried: weight: {:d}, Total Items (includes bagged items): {:d}, EQ: {:d}\r\n", char_get_carry_weight(k), total, i2);

  if (!char_is_npc(k))
    send_to_char(g, ch, "Hunger: {:d}, Thirst: {:d}, Drunk: {:d}\r\n", char_get_conditions(g, k, HUNGER), char_get_conditions(g, k, THIRST), char_get_conditions(g, k, DRUNK));

  column = send_to_char(g, ch, "Master is: {}, Followers are:", char_get_master(g, k) ? char_get_name(char_get_master(g, k)) : "<none>");
  if (!char_get_followers(k))
    send_to_char(g, ch, " <none>\r\n");
  else {
    std::vector<struct char_data *> followers;
    for (char_data *f : char_followers(g, k))
      followers.push_back(f);
    for (size_t n = 0; n < followers.size(); n++) {
      column += send_to_char(g, ch, "{} {}", found++ ? "," : "", char_pers(g, followers[n], ch));
      if (column >= 62) {
        send_to_char(g, ch, "{}\r\n", n + 1 < followers.size() ? "," : "");
        found  = false;
        column = 0;
      }
    }
    if (column != 0)
      send_to_char(g, ch, "\r\n");
  }

  if (char_get_sits(g, k)) {
    chair = char_get_sits(g, k);
    send_to_char(g, ch, "Is on: {}@n\r\n", obj_get_short_description(chair));
  }

  /* Showing the bitvector */
  buf = sprintbitarray(char_get_affected_by(k), affected_bits, AF_ARRAY_MAX);
  send_to_char(g, ch, "AFF: @y{}@n\r\n", buf);

  /* Routine to show what spells a char is affected by */
  if (!char_affects(k).empty()) {
    for (auto aff_it = char_affects(k).begin(); aff_it != char_affects(k).end() && (aff = &*aff_it); ++aff_it) {
      send_to_char(g, ch, "SPL: ({:>3d}hr) @c{:<21}@n ", aff->duration + 1, skill_name(g, aff->type));

      if (aff->modifier)
        send_to_char(g, ch, "{:+d} to {}", aff->modifier, apply_types[(int)aff->location]);

      if (aff->bitvector) {
        if (aff->modifier)
          send_to_char(g, ch, ", ");

        buf = affected_bits[aff->bitvector];
        send_to_char(g, ch, "sets {}", buf);
      }
      send_to_char(g, ch, "\r\n");
    }
  }

  /* check mobiles for a script */
  if (char_is_npc(k)) {
    do_sstat_character(g, ch, k);
    if (char_get_memory(k)) {
      struct script_memory *mem = char_get_memory(k);
      send_to_char(g, ch, "Script memory:\r\n  Remember             Command\r\n");
      while (mem) {
        struct char_data *mc = find_char(g, mem->id);
        if (!mc)
          send_to_char(g, ch, "  ** Corrupted!\r\n");
        else {
          if (!mem->cmd.empty())
            send_to_char(g, ch, "  {:<20.20}{}\r\n", char_get_name(mc), mem->cmd);
          else
            send_to_char(g, ch, "  {:<20.20} <default>\r\n", char_get_name(mc));
        }
        mem = mem->next;
      }
    }
  } else {
    int x, track = 0;

    send_to_char(g, ch, "Bonuses/Negatives:\r\n");

    for (x = 0; x < 30; x++) {
      if (x < 15) {
        if (char_get_bonuses(k, x) > 0) {
          send_to_char(g, ch, "@c{}@n\n", list_bonus[x]);
          track += 1;
        }
      } else {
        if (char_get_bonuses(k, x) > 0) {
          send_to_char(g, ch, "@r{}@n\n", list_bonus[x]);
          track += 1;
        }
      }
    }
    if (track <= 0) {
      send_to_char(g, ch, "@wNone.@n\r\n");
    }
    send_to_char(g, ch, "To see player variables use varstat now.\r\n");
  }
}

ACMD(do_varstat)
{
  struct char_data *vict;
  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "That player is not in the game.\r\n");
    return;
  }
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "Just use stat for an NPC\r\n");
    return;
  }
  /* Display their global variables */
  if (char_get_script(vict) && char_get_script(vict)->global_vars) {
    struct trig_var_data *tv;

    send_to_char(g, ch, "{}'s Global Variables:\r\n", char_get_name(vict));

    /* currently, variable context for players is always 0, so it is */
    /* not displayed here. in the future, this might change */
    for (tv = char_get_script(vict)->global_vars; tv; tv = tv->next) {
      if (tv->value[0] == UID_CHAR) {
        send_to_char(g, ch, "    {:>10}:  [UID]: {}\r\n", tv->name, find_uid_name(g, tv->value));
      } else
        send_to_char(g, ch, "    {:>10}:  {}\r\n", tv->name, tv->value);
    }
  }
}

namespace {
  /* stat <kind> <name>, kind by kind */
  void stat_mob(GameWorld *g, struct char_data *ch, std::string &name)
  {
    if (name.empty()) {
      send_to_char(g, ch, "Stats on which mobile?\r\n");
      return;
    }
    auto found = find_char(g, ch, name, FIND_CHAR_WORLD, "No such mobile around.\r\n");
    if (!reject(g, ch, found))
      do_stat_character(g, ch, *found);
  }

  void stat_player(GameWorld *g, struct char_data *ch, std::string &name)
  {
    if (name.empty()) {
      send_to_char(g, ch, "Stats on which player?\r\n");
      return;
    }
    auto found = find_player(g, ch, name, FIND_CHAR_WORLD, "No such player around.\r\n");
    if (!reject(g, ch, found))
      do_stat_character(g, ch, *found);
  }

  void stat_file(GameWorld *g, struct char_data *ch, std::string &name)
  {
    if (name.empty()) {
      send_to_char(g, ch, "Stats on which player?\r\n");
      return;
    }
    if (struct char_data *victim = get_player_vis(g, ch, name, nullptr, FIND_CHAR_WORLD)) {
      do_stat_character(g, ch, victim);
      return;
    }
    struct char_data *victim = new char_data();
    clear_char(victim);
    char_make_player_specials(victim);
    if (load_char(g, name.c_str(), victim) < 0) {
      send_to_char(g, ch, "There is no such player.\r\n");
      free_char(g, victim);
      return;
    }
    char_to_room(g, victim, 0);
    if (char_get_admlevel(victim) > char_get_admlevel(ch))
      send_to_char(g, ch, "Sorry, you can't do that.\r\n");
    else
      do_stat_character(g, ch, victim);
    extract_char_final(g, victim);
  }

  void stat_object(GameWorld *g, struct char_data *ch, std::string &name)
  {
    if (name.empty()) {
      send_to_char(g, ch, "Stats on which object?\r\n");
      return;
    }
    if (struct obj_data *object = get_obj_vis(g, ch, name, nullptr))
      do_stat_object(g, ch, object);
    else
      send_to_char(g, ch, "No such object around.\r\n");
  }

  void stat_zone(GameWorld *g, struct char_data *ch, std::string &name)
  {
    if (name.empty())
      send_to_char(g, ch, "Stats on which zone?\r\n");
    else
      print_zone(g, ch, parse_int(name));
  }

  void stat_room(GameWorld *g, struct char_data *ch, std::string &)
  {
    do_stat_room(g, ch);
  }

  /* Anything by that name: worn, carried, here, then anywhere */
  void stat_anything(GameWorld *g, struct char_data *ch, std::string name)
  {
    int number = get_number(name);
    struct obj_data *object;
    struct char_data *victim;

    if ((object = get_obj_in_equip_vis(g, ch, name, &number, char_get_equipment_array(ch))) ||
        (object = get_obj_in_list_vis(g, ch, name, &number, char_get_inventory(ch))))
      do_stat_object(g, ch, object);
    else if ((victim = get_char_vis(g, ch, name, &number, FIND_CHAR_ROOM)))
      do_stat_character(g, ch, victim);
    else if ((object = get_obj_in_list_vis(g, ch, name, &number, room_get_contents(g, char_get_in_room(ch)))))
      do_stat_object(g, ch, object);
    else if ((victim = get_char_vis(g, ch, name, &number, FIND_CHAR_WORLD)))
      do_stat_character(g, ch, victim);
    else if ((object = get_obj_vis(g, ch, name, &number)))
      do_stat_object(g, ch, object);
    else
      send_to_char(g, ch, "Nothing around by that name.\r\n");
  }

  struct stat_kind {
    const char *name;
    void (*stat)(GameWorld *, struct char_data *, std::string &);
  };
  constexpr stat_kind STAT_KINDS[] = {
    {"room", stat_room}, {"mob", stat_mob}, {"player", stat_player}, {"file", stat_file}, {"object", stat_object}, {"zone", stat_zone},
  };
} // namespace

ACMD(do_stat)
{
  std::string_view rest = argument;
  std::string kind      = take_word(rest);
  std::string name(skip_spaces(rest));

  if (kind.empty()) {
    send_to_char(g, ch, "Stats on who or what or where?\r\n");
    return;
  }
  auto it = std::ranges::find_if(STAT_KINDS, [&](const stat_kind &k) { return is_abbrev(kind, k.name); });
  if (it != std::end(STAT_KINDS))
    it->stat(g, ch, name);
  else
    stat_anything(g, ch, kind);
}

ACMD(do_vstat)
{
  std::string buf, buf2;

  std::string_view rest = argument;
  buf                   = take_arg(rest);
  buf2                  = take_arg(rest);

  if (buf.empty() || buf2.empty() || !isdigit(buf2[0])) {
    send_to_char(g, ch, "Usage: vstat { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(g, ch, "That's not a valid number.\r\n");
    return;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_vnum r_num;

    if (!mob_proto_of(g, r_num = parse_int(buf2))) {
      send_to_char(g, ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(g, r_num);
    char_to_room(g, mob, 0);
    do_stat_character(g, ch, mob);
    extract_char(g, mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_vnum r_num;

    if (!obj_proto_of(g, r_num = parse_int(buf2))) {
      send_to_char(g, ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(g, r_num);
    do_stat_object(g, ch, obj);
    extract_obj(g, obj);
  } else
    send_to_char(g, ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_date)
{
  if (subcmd == SCMD_DATE) {
    send_to_char(g, ch, "Current machine time: {}\r\n", clock_text(game_now(g)));
    return;
  }
  time_t up = game_now(g) - g->boot_time;
  int d = up / 86400, h = (up / 3600) % 24, m = (up / 60) % 60;
  send_to_char(g, ch, "Up since {}: {:d} day{}, {:d}:{:02d}\r\n", clock_text(g->boot_time), d, d == 1 ? "" : "s", h, m);
}

ACMD(do_last)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "For whom do you wish to search?\r\n");
    return;
  }

  struct char_data *vict = new char_data();
  clear_char(vict);
  char_make_player_specials(vict);
  if (load_char(g, arg.c_str(), vict) < 0) {
    send_to_char(g, ch, "There is no such player.\r\n");
  } else if (char_get_admlevel(vict) > char_get_admlevel(ch) && char_get_admlevel(ch) < ADMLVL_IMPL) {
    send_to_char(g, ch, "You are not sufficiently godly for that!\r\n");
  } else {
    time_t logon = char_get_time_logon(vict);
    send_to_char(g, ch, "[{:>5d}] [{:>2d} {} {}] {:<12} : {:<18} : {:<20}\r\n", char_get_idnum(vict), char_get_level(vict), char_race_abbr(vict), char_class_abbr(vict),
                 char_get_name(vict), !char_get_player_specials(vict)->host.empty() ? char_get_player_specials(vict)->host.c_str() : "(NOHOST)", ctime(&logon));
  }
  free_char(g, vict); /* refused or not */
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

/* FIXME: overflow possible */
static std::string zone_summary(GameWorld *g, struct zone_data *zone, int listall)
{
  if (listall) {
    int j, k, l, m, n, o;
    std::string text = str_format("{:>3d} {:<30.30} By: {:<10.10} Age: {:>3d}; Reset: {:>3d} ({:>1d}); Range: {:>5d}-{:>5d}\r\n",
                                  zone->number, zone->name, zone->builders,
                                  zone->age, zone->lifespan,
                                  zone->reset_mode,
                                  zone->bot, zone->top);

    j = (int)g->rooms.vnums_in(zone->bot, zone->top).size();
    k = (int)g->obj_proto.vnums_in(zone->bot, zone->top).size();
    l = (int)g->mob_proto.vnums_in(zone->bot, zone->top).size();

    m = count_shops(g, zone->bot, zone->top);

    n = (int)g->trig_index.vnums_in(zone->bot, zone->top).size();

    o = count_guilds(g, zone->bot, zone->top);

    text += str_format("       Zone stats:\r\n       ---------------\r\n         Rooms:    {:>2d}\r\n         Objects:  {:>2d}\r\n         Mobiles:  {:>2d}\r\n         Shops:    {:>2d}\r\n         Triggers: {:>2d}\r\n         Guilds:   {:>2d}\r\n",
                       j, k, l, m, n, o);
    return text;
  }

  return str_format("{:>3d} {} By: {:<10.10} Range: {:>5d}-{:>5d}\r\n", zone->number, printf_width(zone->name.c_str(), count_color_chars(zone->name.c_str()) + 30, true),
                    zone->builders, zone->bot, zone->top);
}

namespace {
  void show_zones(GameWorld *g, struct char_data *ch, std::string &value)
  {
    std::string buf;
    if (value == ".")
      buf = zone_summary(g, room_get_zone(g, char_get_in_room(ch)), 1);
    else if (!value.empty() && is_number(value)) {
      struct zone_data *zone = find_zone(g, parse_int(value));
      if (!zone) {
        send_to_char(g, ch, "That is not a valid zone.\r\n");
        return;
      }
      buf = zone_summary(g, zone, 1);
    } else {
      for (struct zone_data *zone : g->zones.all())
        buf += zone_summary(g, zone, 0);
    }
    send_to_char(g, ch, buf);
  }

  void show_player(GameWorld *g, struct char_data *ch, std::string &value)
  {
    if (value.empty()) {
      send_to_char(g, ch, "A name would help.\r\n");
      return;
    }
    struct char_data *vict = new char_data();
    clear_char(vict);
    char_make_player_specials(vict);
    if (load_char(g, value.c_str(), vict) < 0) {
      send_to_char(g, ch, "There is no such player.\r\n");
      free_char(g, vict);
      return;
    }
    send_to_char(g, ch, "Player: {:<12} ({}) [{:>2d} {} {}]\r\n", char_get_name(vict),
                 genders[(int)char_get_sex(vict)], char_get_level(vict), char_class_abbr(vict), char_race_abbr(vict));
    send_to_char(g, ch, "Au: {:<8d}  Bal: {:<8d}  Exp: {:d}  Align: {:<5d}\r\n",
                 char_get_gold(vict), char_get_bank_gold(vict), char_get_exp(vict), char_get_alignment(vict));
    if (CONFIG_ALLOW_MULTICLASS)
      send_to_char(g, ch, "Class ranks: {}\r\n", class_desc_str(vict, 1, 0));

    /* ctime() uses static buffer: do not combine. */
    time_t created = char_get_time_created(vict), logon = char_get_time_logon(vict);
    send_to_char(g, ch, "Started: {:<20.16}  ", ctime(&created));
    send_to_char(g, ch, "Last: {:<20.16}  Played: {:>3d}h {:>2d}m\r\n", ctime(&logon),
                 (int)(char_get_time_played(vict) / 3600), (int)(char_get_time_played(vict) / 60 % 60));
    free_char(g, vict);
  }

  void show_rent(GameWorld *g, struct char_data *ch, std::string &value)
  {
    if (value.empty()) {
      send_to_char(g, ch, "A name would help.\r\n");
      return;
    }
    Crash_listrent(g, ch, value.data());
  }

  void show_stats(GameWorld *g, struct char_data *ch, std::string &)
  {
    int players = 0, connected = 0, mobiles = 0;
    for (char_data *tch : each_char(g)) {
      if (char_is_npc(tch))
        mobiles++;
      else if (char_can_see(g, ch, tch)) {
        players++;
        if (char_get_desc(tch))
          connected++;
      }
    }
    int objects = count_members(each_obj(g));
    send_to_char(g, ch,
                 "             @D---   @CCore Stats   @D---\r\n  @Y{:>5d}@W players in game  @y{:>5d}@W connected\r\n  @Y{:>5d}@W registered\r\n  @Y{:>5d}@W mobiles          @y{:>5d}@W prototypes\r\n  @Y{:>5d}@W objects          @y{:>5d}@W prototypes\r\n  @Y{:>5d}@W rooms            @y{:>5d}@W zones\r\n  @Y{:>5d}@W triggers\r\n  @Y{:>5d}@W large bufs\r\n  @Y{:>5d}@W buf switches     @y{:>5d}@W overflows\r\n             @D--- @CMiscellaneous  @D---\r\n  @Y{:>5}@W Mob ki attacks this boot\r\n  @Y{:>5}@W Asssassins Generated@n\r\n  @Y{:>5d}@W Wish Selfishness Meter@n\r\n",
                 players, connected,
                 g->top_of_p_table + 1,
                 mobiles, (int)g->mob_proto.size(),
                 objects, (int)g->obj_proto.size(),
                 (int)g->rooms.size(), (int)g->zones.size(),
                 (int)g->trig_index.size(),
                 g->buf_largecount,
                 g->buf_switches, g->buf_overflows,
                 add_commas(g->mob_specials_used),
                 add_commas(g->number_of_assassins),
                 g->SELFISHMETER);
  }

  void show_errors(GameWorld *g, struct char_data *ch, std::string &)
  {
    std::string buf = "Errant Rooms\r\n------------\r\n";
    int k           = 0;
    for (room_vnum r : g->rooms.vnums())
      for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
        auto *exit = room_get_dir_option(g, r, dir);
        if (!exit)
          continue;
        if (exit->to_room == 0)
          buf += str_format("{:>2d}: (void   ) [{:>5d}] {}{} ({})\r\n", ++k, r, printf_width(room_get_name(g, r), count_color_chars(room_get_name(g, r)) + 40, true), QNRM, dirs[dir]);
        if (exit->to_room == NOWHERE && !exit_description(exit))
          buf += str_format("{:>2d}: (Nowhere) [{:>5d}] {}{} ({})\r\n", ++k, r, printf_width(room_get_name(g, r), count_color_chars(room_get_name(g, r)) + 40, true), QNRM, dirs[dir]);
      }
    send_to_char(g, ch, buf);
  }

  template <const char *Title, int Flag>
  void show_flagged_rooms(GameWorld *g, struct char_data *ch, std::string &)
  {
    std::string buf = Title;
    int count       = 0;
    for (room_vnum r : g->rooms.vnums())
      if (room_flagged(g, r, Flag))
        buf += str_format("{:>2d}: [{:>5d}] {}\r\n", ++count, r, room_get_name(g, r));
    send_to_char(g, ch, buf);
  }

  constexpr char DEATH_TRAPS[] = "Death Traps\r\n-----------\r\n";
  constexpr char GODROOMS[]    = "Godrooms\r\n--------------------------\r\n";

  void show_shops_of(GameWorld *g, struct char_data *ch, std::string &value) { show_shops(g, ch, value.data()); }

  void show_houses(GameWorld *g, struct char_data *ch, std::string &) { hcontrol_list_houses(g, ch); }

  void show_snoop(GameWorld *g, struct char_data *ch, std::string &)
  {
    int count = 0;
    send_to_char(g, ch, "People currently snooping:\r\n--------------------------\r\n");
    for (descriptor_data *d : each_desc(g)) {
      if (d->snooping == nullptr || d->character == nullptr)
        continue;
      if (desc_get_connected(d) != CON_PLAYING || char_get_admlevel(ch) < char_get_admlevel(d->character))
        continue;
      if (!char_can_see(g, ch, d->character) || char_get_in_room(d->character) == NOWHERE)
        continue;
      count++;
      send_to_char(g, ch, "{:<10} - snooped by {}.\r\n", char_get_name(d->snooping->character), char_get_name(d->character));
    }
    if (count == 0)
      send_to_char(g, ch, "No one is currently snooping.\r\n");
  }

  void show_assemblies(GameWorld *g, struct char_data *ch, std::string &) { assemblyListToChar(g, ch); }

  void show_guilds(GameWorld *g, struct char_data *ch, std::string &value) { show_guild(g, ch, value.data()); }

  void show_levels(GameWorld *g, struct char_data *ch, std::string &) { send_to_char(g, ch, "This is not used currently.\r\n"); }

  void show_uniques(GameWorld *g, struct char_data *ch, std::string &value)
  {
    obj_vnum low = -1, high = 9999999;
    if (!value.empty() && sscanf(value.c_str(), "%d-%d", &low, &high) != 2) {
      if (sscanf(value.c_str(), "%d", &low) != 1) {
        send_to_char(g, ch, "Usage: show uniques, show uniques [vnum], or show uniques [low-high]\r\n");
        return;
      }
      high = low;
    }
    send_to_char(g, ch, sprintuniques(g, low, high));
  }

  /* Everyone affected, newest first, or the one asked about; durations in `unit`. */
  template <const char *Unit>
  void show_affects(GameWorld *g, struct char_data *ch, std::string &value)
  {
    std::vector<struct char_data *> victs;
    if (value.empty()) {
      g->affect_list.visit([&](long, struct char_data *affected) { victs.push_back(affected); });
    } else {
      struct char_data *vict = get_char_world_vis(g, ch, value, nullptr);
      if (!vict) {
        send_to_char(g, ch, "Cannot find that character.\r\n");
        return;
      }
      victs.push_back(vict);
    }
    if (victs.empty()) {
      send_to_char(g, ch, "None.\r\n");
      return;
    }
    std::string text;
    for (struct char_data *who : victs) {
      text += str_format("Name: {}\r\n", char_get_name(who));
      for (const struct affected_type &aff : char_affects(who)) {
        text += str_format("SPL: ({:>3d}{}) @c{:<21}@n ", aff.duration + 1, Unit, skill_name(g, aff.type));
        if (aff.modifier)
          text += str_format("{:+d} to {}", aff.modifier, apply_types[(int)aff.location]);
        if (aff.bitvector) {
          if (aff.modifier)
            text += ", ";
          text += str_format("sets {}", affected_bits[aff.bitvector]);
        }
        text += "\r\n";
      }
    }
    send_to_char(g, ch, text);
  }

  constexpr char HOURS[]  = "hr";
  constexpr char ROUNDS[] = "rd";

  struct show_field {
    const char *name;
    int level;
    void (*show)(GameWorld *g, struct char_data *ch, std::string &value);
  };

  /* Matched by prefix, in this order. "nothing" is there to be matched, and understood as nothing. */
  constexpr show_field SHOW_FIELDS[] = {
      {"nothing",    0,             nullptr                                    },
      {"zones",      ADMLVL_IMMORT, show_zones                                 },
      {"player",     ADMLVL_GOD,    show_player                                },
      {"rent",       ADMLVL_GRGOD,  show_rent                                  },
      {"stats",      ADMLVL_IMMORT, show_stats                                 },
      {"errors",     ADMLVL_IMPL,   show_errors                                },
      {"death",      ADMLVL_GOD,    show_flagged_rooms<DEATH_TRAPS, ROOM_DEATH>},
      {"godrooms",   ADMLVL_IMMORT, show_flagged_rooms<GODROOMS, ROOM_GODROOM> },
      {"shops",      ADMLVL_IMMORT, show_shops_of                              },
      {"houses",     ADMLVL_GOD,    show_houses                                },
      {"snoop",      ADMLVL_GRGOD,  show_snoop                                 },
      {"assemblies", ADMLVL_IMMORT, show_assemblies                            },
      {"guilds",     ADMLVL_GOD,    show_guilds                                },
      {"levels",     ADMLVL_GRGOD,  show_levels                                },
      {"uniques",    ADMLVL_GRGOD,  show_uniques                               },
      {"affect",     ADMLVL_GRGOD,  show_affects<HOURS>                        },
      {"affectv",    ADMLVL_GRGOD,  show_affects<ROUNDS>                       },
  };
} // namespace

ACMD(do_show)
{
  std::string_view rest = skip_spaces(argument);

  if (rest.empty()) {
    send_to_char(g, ch, "Game Info options:\r\n");
    int shown = 0;
    for (const show_field &f : SHOW_FIELDS | std::views::drop(1))
      if (f.level <= char_get_admlevel(ch))
        send_to_char(g, ch, "{:<15}{}", f.name, (!(++shown % 5) ? "\r\n" : ""));
    send_to_char(g, ch, "\r\n");
    return;
  }

  std::string field = take_arg(rest);
  std::string value = take_arg(rest);

  auto it = std::ranges::find_if(SHOW_FIELDS, [&](const show_field &f) { return std::string_view(f.name).starts_with(field); });
  if (it != std::end(SHOW_FIELDS) && char_get_admlevel(ch) < it->level) {
    send_to_char(g, ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (it == std::end(SHOW_FIELDS) || !it->show) {
    send_to_char(g, ch, "Sorry, I don't understand that.\r\n");
    return;
  }
  it->show(g, ch, value);
}

#define PLIST_FORMAT \
  "players [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-m]"

ACMD(do_plist)
{
  int i, count = 0;
  char mode;
  std::string buf;
  struct time_info_data time_away;
  int low = 0, high = 100, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24;

  std::string name_search;
  std::string_view rest = skip_spaces(argument);

  while (!rest.empty()) {
    std::string arg = take_word(rest);
    rest            = skip_spaces(rest);
    if (isdigit(arg[0])) {
      if (sscanf(arg.c_str(), "%d-%d", &low, &high) == 1)
        high = low;
    } else if (arg[0] == '-') {
      mode = arg[1]; /* '\0' for a bare "-" */
      switch (mode) {
        case 'l':
          arg  = take_word(rest);
          rest = skip_spaces(rest);
          sscanf(arg.c_str(), "%d-%d", &low, &high);
          break;
        case 'n':
          name_search = take_word(rest);
          rest        = skip_spaces(rest);
          break;
        case 'i':
          low = 1;
          break;
        case 'm':
          high = 100;
          break;
        case 'd':
          arg  = take_word(rest);
          rest = skip_spaces(rest);
          if (sscanf(arg.c_str(), "%d-%d", &low_day, &high_day) == 1)
            high_day = low_day;
          break;
        case 'h':
          arg  = take_word(rest);
          rest = skip_spaces(rest);
          if (sscanf(arg.c_str(), "%d-%d", &low_hr, &high_hr) == 1)
            high_hr = low_hr;
          break;
        default:
          send_to_char(g, ch, "{}\r\n", PLIST_FORMAT);
          return;
      }
    } else {
      send_to_char(g, ch, "{}\r\n", PLIST_FORMAT);
      return;
    }
  }

  buf = str_format("@W[ Id] (Lv) Name         Last@n\r\n"
                   "{}-----------------------------------------------{}\r\n",
                   CCCYN(g, ch, C_NRM), CCNRM(g, ch, C_NRM));

  for (i = 0; i <= g->top_of_p_table; i++) {
    if (IS_SET(g->player_table[i].flags, PINDEX_DELETED)) {
      buf += str_format("[{:>3d}] <DELETED> --Will be removed next boot.\r\n", g->player_table[i].id);
      continue;
    }

    if (g->player_table[i].level < low || g->player_table[i].level > high)
      continue;

    time_away = *real_time_passed(game_now(g), g->player_table[i].last);

    if (!name_search.empty() && !str_eq_nocase(name_search, g->player_table[i].name))
      continue;

    if (time_away.day > high_day || time_away.day < low_day)
      continue;
    if (time_away.hours > high_hr || time_away.hours < low_hr)
      continue;

    std::string time_str = asctime(localtime(&g->player_table[i].last));
    time_str.pop_back(); /* its newline */
    std::string name = writable_copy(g->player_table[i].name);

    buf += str_format("[{:>3d}] ({:>2d}) {:<12} {}\r\n", g->player_table[i].id, g->player_table[i].level, CAP(name), time_str);
    count++;
  }
  buf += str_format("{}-----------------------------------------------{}\r\n"
                    "{:d} players listed.\r\n",
                    CCCYN(g, ch, C_NRM), CCNRM(g, ch, C_NRM), count);
  send_to_char(g, ch, buf);
}

/******************************************************************************/
/*                         Zone Checker Code below                            */
/******************************************************************************/

/*mob limits*/
#define MAX_DAMAGE_MOD_ALLOWED MAX(char_get_level(mob) / 5, 2)
#define MAX_GOLD_ALLOWED       char_get_level(mob) * 3000
#define MAX_EXP_ALLOWED        char_get_level(mob) * char_get_level(mob) * 120
constexpr int MAX_LEVEL_ALLOWED = (100);
#define GET_OBJ_AVG_DAM(obj) (((obj_get_value(obj, 2) + 1) / 2.0) * obj_get_value(obj, 1))
/* arbitrary limit for per round dam */
constexpr int MAX_MOB_DAM_ALLOWED = 500;

#define ZCMD2 zone->cmd[cmd_no] /*from DB.C*/

/*item limits*/
constexpr int MAX_DAM_ALLOWED     = 50; /* for weapons  - avg. dam*/
constexpr int MAX_AFFECTS_ALLOWED = 3;

/* Armor class limits*/
constexpr int TOTAL_WEAR_CHECKS = (NUM_ITEM_WEARS - 2); /*minus Wield and Take*/
static const struct zcheck_armor {
  bitvector_t bitvector{}; /* from Structs.h                       */
  int ac_allowed{};        /* Max. AC allowed for this body part  */
  const char *message{};   /* phrase for error message            */
} zarmor[] = {
    {ITEM_WEAR_FINGER, 10, "Ring"         },
    {ITEM_WEAR_NECK,   10, "Necklace"     },
    {ITEM_WEAR_BODY,   10, "Body armor"   },
    {ITEM_WEAR_HEAD,   10, "Head gear"    },
    {ITEM_WEAR_LEGS,   10, "Legwear"      },
    {ITEM_WEAR_FEET,   10, "Footwear"     },
    {ITEM_WEAR_HANDS,  10, "Glove"        },
    {ITEM_WEAR_ARMS,   10, "Armwear"      },
    {ITEM_WEAR_SHIELD, 10, "Shield"       },
    {ITEM_WEAR_ABOUT,  10, "Cloak"        },
    {ITEM_WEAR_WAIST,  10, "Belt"         },
    {ITEM_WEAR_WRIST,  10, "Wristwear"    },
    {ITEM_WEAR_HOLD,   10, "Held item"    },
    {ITEM_WEAR_PACK,   10, "Backpack item"},
    {ITEM_WEAR_EAR,    10, "Earring item" },
    {ITEM_WEAR_SH,     10, "Shoulder item"},
    {ITEM_WEAR_EYE,    10, "Eye item"     }
};

/*
  Applies limits
  !! Very Important:  Keep these in the same order as in Structs.h
  To ignore an apply, set max_aff to -99.
  These will be ignored if MAX_APPLIES_LIMIT = 0
*/
static const struct zcheck_affs {
  int aff_type{};        /*from Structs.h*/
  int min_aff{};         /*min. allowed value*/
  int max_aff{};         /*max. allowed value*/
  const char *message{}; /*phrase for error message*/
} zaffs[] = {
    {APPLY_NONE,        0,   -99, "unused0"         },
    {APPLY_STR,         -6,  6,   "strength"        },
    {APPLY_DEX,         -6,  6,   "dexterity"       },
    {APPLY_INT,         -6,  6,   "intelligence"    },
    {APPLY_WIS,         -6,  6,   "wisdom"          },
    {APPLY_CON,         -6,  6,   "constitution"    },
    {APPLY_CHA,         -6,  6,   "charisma"        },
    {APPLY_CLASS,       0,   0,   "class"           },
    {APPLY_LEVEL,       0,   0,   "level"           },
    {APPLY_AGE,         -10, 10,  "age"             },
    {APPLY_CHAR_WEIGHT, -50, 50,  "character weight"},
    {APPLY_CHAR_HEIGHT, -50, 50,  "character height"},
    {APPLY_MANA,        -50, 50,  "mana"            },
    {APPLY_HIT,         -50, 50,  "hit points"      },
    {APPLY_MOVE,        -50, 50,  "movement"        },
    {APPLY_GOLD,        0,   0,   "gold"            },
    {APPLY_EXP,         0,   0,   "experience"      },
    {APPLY_AC,          -10, 10,  "magical AC"      },
    {APPLY_ACCURACY,    0,   -99, "accuracy"        },
    {APPLY_DAMAGE,      0,   -99, "damage"          },
    {APPLY_REGEN,       0,   0,   "regen"           },
    {APPLY_TRAIN,       0,   0,   "train"           },
    {APPLY_LIFEMAX,     0,   0,   "lifemax"         },
    {APPLY_UNUSED3,     0,   0,   "unused"          },
    {APPLY_UNUSED4,     0,   0,   "unused"          },
    {APPLY_RACE,        0,   0,   "race"            },
    {APPLY_TURN_LEVEL,  -6,  6,   "turn level"      },
    {APPLY_SPELL_LVL_0, 0,   0,   "spell level 0"   },
    {APPLY_SPELL_LVL_1, 0,   0,   "spell level 1"   },
    {APPLY_SPELL_LVL_2, 0,   0,   "spell level 2"   },
    {APPLY_SPELL_LVL_3, 0,   0,   "spell level 3"   },
    {APPLY_SPELL_LVL_4, 0,   0,   "spell level 4"   },
    {APPLY_SPELL_LVL_5, 0,   0,   "spell level 5"   },
    {APPLY_SPELL_LVL_6, 0,   0,   "spell level 6"   },
    {APPLY_SPELL_LVL_7, 0,   0,   "spell level 7"   },
    {APPLY_SPELL_LVL_8, 0,   0,   "spell level 8"   },
    {APPLY_SPELL_LVL_9, 0,   0,   "spell level 9"   },
    {APPLY_KI,          0,   0,   "ki"              },
    {APPLY_FORTITUDE,   -4,  4,   "fortitude"       },
    {APPLY_REFLEX,      -4,  4,   "reflex"          },
    {APPLY_WILL,        -4,  4,   "will"            },
    {APPLY_SKILL,       -10, 10,  "skill"           },
    {APPLY_FEAT,        -10, 10,  "feat"            },
    {APPLY_ALLSAVES,    -4,  4,   "all 3 save types"},
    {APPLY_RESISTANCE,  -4,  4,   "resistance"      },
    {APPLY_ALL_STATS,   -6,  6,   "all stats"       }
};

/* These are ABS() values. */
constexpr int MAX_APPLY_ACCURCY_MOD_TOTAL = 5;
constexpr int MAX_APPLY_DAMAGE_MOD_TOTAL  = 5;

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
static const int offlimit_zones[]  = {0, 12, 13, 14, -1}; /*what zones can no room connect to (virtual num) */
constexpr int MIN_ROOM_DESC_LENGTH = 80;                  /* at least one line - set to 0 to not care. */
constexpr int MAX_COLOUMN_WIDTH    = 80;                  /* at most 80 chars per line */

ACMD(do_zcheck)
{
  struct zone_data *zrnum;
  struct obj_data *obj;
  struct char_data *mob = nullptr;
  room_vnum exroom      = 0;
  int ac                = 0;
  int affs              = 0, tohit, todam, value;
  int j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
  std::string buf;
  float avg_dam;

  std::string_view rest = argument;
  std::string zarg      = take_arg(rest);

  if (zarg.empty() || zarg == ".")
    zrnum = room_get_zone(g, char_get_in_room(ch));
  else
    zrnum = find_zone(g, parse_int(zarg));

  if (zrnum == nullptr) {
    send_to_char(g, ch, "Check what zone ?\r\n");
    return;
  } else
    send_to_char(g, ch, "Checking zone {:d}!\r\n", zrnum->number);

  /************** Check mobs *****************/

  send_to_char(g, ch, "Checking Mobs for limits...\r\n");
  /*check mobs first*/
  for (mob_vnum mvnum : g->mob_proto.vnums()) {
    if (find_zone_by_thing(g, mob_index_of(g, mvnum)->vnum) == zrnum) { /*is mob in this zone?*/
      mob = mob_proto_of(g, mvnum);
      if (!strcmp(text_of(char_get_alias(mob)), "mob unfinished") && (found = 1))
        buf += "- Alias hasn't been set.\r\n";

      if (!strcmp(text_of(char_get_sdesc(mob)), "the unfinished mob") && (found = 1))
        buf += "- Short description hasn't been set.\r\n";

      if (!strncmp(text_of(char_get_ldesc(mob)), "An unfinished mob stands here.", 30) && (found = 1))
        buf += "- Long description hasn't been set.\r\n";

      if (char_get_ddesc(mob) && *char_get_ddesc(mob)) {
        if (!strncmp(char_get_ddesc(mob), "It looks unfinished.", 20) && (found = 1))
          buf += "- Description hasn't been set.\r\n";
        else if (strncmp(char_get_ddesc(mob), "   ", 3) && (found = 1))
          buf += "- Description hasn't been formatted. (/fi)\r\n";
      }

      if (char_get_level(mob) > MAX_LEVEL_ALLOWED && (found = 1))
        buf += str_format("- Is level {:d} (limit: 1-{:d})\r\n",
                          char_get_level(mob), MAX_LEVEL_ALLOWED);

      /* avg. dam per round of combat */
      avg_dam = ((char_get_mob_specials_damsizedice(mob) / 2.0) * char_get_mob_specials_damnodice(mob));
      if (avg_dam > MAX_MOB_DAM_ALLOWED && (found = 1))
        buf += str_format("- average damage of {:>4.1f} is too high (limit: {:d})\r\n",
                          avg_dam, MAX_MOB_DAM_ALLOWED);

      if (char_get_mob_specials_damsizedice(mob) == 1 &&
          char_get_mob_specials_damnodice(mob) == 1 &&
          char_get_level(mob) == 0 &&
          (found = 1))
        buf += str_format("- Needs to be fixed - {}Autogenerate!{}\r\n", CCYEL(g, ch, C_NRM), CCNRM(g, ch, C_NRM));

      if (char_mob_flagged(mob, MOB_AGGRESSIVE) &&
          char_mob_flagged(mob, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL) && (found = 1))
        buf += "- Both aggresive and agressive to align.\r\n";

      if ((char_get_gold(mob) > MAX_GOLD_ALLOWED) && (found = 1))
        buf += str_format("- Set to {:d} Gold (limit : {:d}).\r\n",
                          char_get_gold(mob),
                          MAX_GOLD_ALLOWED);

      if (char_get_exp(mob) > MAX_EXP_ALLOWED && (found = 1))
        buf += str_format("- Has {:d} experience (limit: {:d})\r\n",
                          char_get_exp(mob), MAX_EXP_ALLOWED);
      if (char_aff_flagged(mob, AFF_GROUP | AFF_CHARM | AFF_POISON) && (found = 1))
        buf += str_format("- Has illegal affection bits set ({} {} {})\r\n",
                          char_aff_flagged(mob, AFF_GROUP) ? "GROUP" : "",
                          char_aff_flagged(mob, AFF_CHARM) ? "CHARM" : "",
                          char_aff_flagged(mob, AFF_POISON) ? "POISON" : "");

      /*if (!MOB_FLAGGED(mob, MOB_SENTINEL) && !MOB_FLAGGED(mob, MOB_STAY_ZONE) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                            "- Neither SENTINEL nor STAY_ZONE bits set.\r\n");*/

      if (char_mob_flagged(mob, MOB_SPEC) && (found = 1))
        buf += "- SPEC flag needs to be removed.\r\n";

      /*****ADDITIONAL MOB CHECKS HERE*****/
      if (found) {
        send_to_char(g, ch,
                     "{}[{:>5d}]{} {:<30}: {}\r\n{}",
                     CCCYN(g, ch, C_NRM), char_get_mob_vnum(g, mob),
                     CCYEL(g, ch, C_NRM), char_get_name(mob),
                     CCNRM(g, ch, C_NRM), buf);
      }
      buf.clear();
      found = 0;
    } /*mob is in zone*/
  } /*check mobs*/

  /************** Check objects *****************/
  send_to_char(g, ch, "\r\nChecking Objects for limits...\r\n");
  for (obj_vnum ovnum : g->obj_proto.vnums()) {
    if (find_zone_by_thing(g, obj_index_of(g, ovnum)->vnum) == zrnum) { /*is object in this zone?*/
      obj = obj_proto_of(g, ovnum);
      switch (obj_get_type_flag(obj)) {
        case ITEM_MONEY:
          if ((value = obj_get_value(obj, 1)) > MAX_GOLD_ALLOWED && (found = 1))
            buf += str_format("- Is worth {:d} (money limit {:d} coins).\r\n",
                              value, MAX_GOLD_ALLOWED);
          break;
        case ITEM_WEAPON:
          if (obj_get_value(obj, 3) >= NUM_ATTACK_TYPES && (found = 1))
            buf += str_format("- has out of range attack type {:d}.\r\n",
                              obj_get_value(obj, 3));

          if (GET_OBJ_AVG_DAM(obj) > MAX_DAM_ALLOWED && (found = 1))
            buf += str_format("- Damroll is {:>2.1f} (limit {:d})\r\n",
                              GET_OBJ_AVG_DAM(obj), MAX_DAM_ALLOWED);
          break;
        case ITEM_ARMOR:
          ac = obj_get_value(obj, 0);
          for (j = 0; j < TOTAL_WEAR_CHECKS; j++) {
            if (obj_wear_flagged(obj, zarmor[j].bitvector) && (ac > zarmor[j].ac_allowed) && (found = 1))
              buf += str_format("- Has AC {:d} ({} limit is {:d})\r\n",
                                ac, zarmor[j].message, zarmor[j].ac_allowed);
          }
          break;

      } /*switch on Item_Type*/

      if (!obj_wear_flagged(obj, ITEM_WEAR_TAKE)) {
        if ((obj_get_cost(obj) || (obj_get_weight(obj) && obj_get_type_flag(obj) != ITEM_FOUNTAIN) ||
             obj_get_cost_per_day(obj)) &&
            (found = 1))
          buf += str_format("- is NO_TAKE, but has cost ({:d}) weight ({:d}) or rent ({:d}) set.\r\n",
                            obj_get_cost(obj), obj_get_weight(obj), obj_get_cost_per_day(obj));
      } else {
        if (obj_get_cost(obj) == 0 && (found = 1))
          buf += "- has 0 cost (min. 1).\r\n";

        if (obj_get_weight(obj) == 0 && (found = 1))
          buf += "- has 0 weight (min. 1).\r\n";

        if (obj_get_weight(obj) > MAX_OBJ_WEIGHT && (found = 1))
          buf += str_format("  Weight is too high: {:d} (limit  {:d}).\r\n",
                            obj_get_weight(obj), MAX_OBJ_WEIGHT);

        if (obj_get_cost(obj) > MAX_OBJ_COST && (found = 1))
          buf += str_format("- has {:d} cost (max {:d}).\r\n",
                            obj_get_cost(obj), MAX_OBJ_COST);
      }

      if (obj_get_level(obj) > ADMLVL_IMMORT - 1 && (found = 1))
        buf += str_format("- has min level set to {:d} (max {:d}).\r\n",
                          obj_get_level(obj), ADMLVL_IMMORT - 1);

      if (obj_get_action_description(obj) && *obj_get_action_description(obj) &&
          obj_get_type_flag(obj) != ITEM_STAFF &&
          obj_get_type_flag(obj) != ITEM_WAND &&
          obj_get_type_flag(obj) != ITEM_SCROLL &&
          obj_get_type_flag(obj) != ITEM_NOTE && (found = 1))
        buf += "- has action_description set, but is inappropriate type.\r\n";

      /*first check for over-all affections*/
      for (affs = 0, j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj_get_affected(obj)[j].modifier) affs++;

      if (affs > MAX_AFFECTS_ALLOWED && (found = 1))
        buf += str_format("- has {:d} affects (limit {:d}).\r\n",
                          affs, MAX_AFFECTS_ALLOWED);

      /*check for out of range affections. */
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        if (zaffs[(int)obj_get_affected(obj)[j].location].max_aff != -99 && /* only care if a range is set */
            (obj_get_affected(obj)[j].modifier > zaffs[(int)obj_get_affected(obj)[j].location].max_aff ||
             obj_get_affected(obj)[j].modifier < zaffs[(int)obj_get_affected(obj)[j].location].min_aff ||
             zaffs[(int)obj_get_affected(obj)[j].location].min_aff == zaffs[(int)obj_get_affected(obj)[j].location].max_aff) &&
            (found = 1))
          buf += str_format("- apply to {} is {:d} (limit {:d} - {:d}).\r\n",
                            zaffs[(int)obj_get_affected(obj)[j].location].message,
                            obj_get_affected(obj)[j].modifier,
                            zaffs[(int)obj_get_affected(obj)[j].location].min_aff,
                            zaffs[(int)obj_get_affected(obj)[j].location].max_aff);

      /* special handling of +hit and +dam because of +hit_n_dam */
      for (todam = 0, tohit = 0, j = 0; j < MAX_OBJ_AFFECT; j++) {
        if (obj_get_affected(obj)[j].location == APPLY_DAMAGE)
          todam += obj_get_affected(obj)[j].modifier;
      }
      if (abs(todam) > MAX_APPLY_DAMAGE_MOD_TOTAL && (found = 1))
        buf += str_format("- total damage mod {:d} out of range (limit +/-{:d}.\r\n",
                          todam, MAX_APPLY_DAMAGE_MOD_TOTAL);
      if (abs(tohit) > MAX_APPLY_ACCURCY_MOD_TOTAL && (found = 1))
        buf += str_format("- total accurcy mod {:d} out of range (limit +/-{:d}).\r\n",
                          tohit, MAX_APPLY_ACCURCY_MOD_TOTAL);

      bool unformatted = false;
      for (const extra_descr_data &ext : obj_get_ex_description(obj))
        if (strncmp(ext.description.c_str(), "   ", 3))
          unformatted = true;

      if (unformatted && (found = 1))
        buf += "- has unformatted extra description\r\n";
      /*****ADDITIONAL OBJ CHECKS HERE*****/
      if (found) {
        send_to_char(g, ch, "[{:>5d}] {:<30}: \r\n{}", obj_get_vnum(g, obj), obj_get_short_description(obj), buf);
      }
      buf.clear();
      found = 0;
    } /*object is in zone*/
  } /*check objects*/

  /************** Check rooms *****************/
  send_to_char(g, ch, "\r\nChecking Rooms for limits...\r\n");
  for (room_vnum i : g->rooms.vnums()) {
    if (room_get_zone(g, i) == zrnum) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
        /*check for exit, but ignore off limits if you're in an offlimit zone*/
        if (!room_get_dir_option(g, i, j))
          continue;
        exroom = room_get_dir_option(g, i, j)->to_room;
        if (exroom == NOWHERE)
          continue;
        if (room_get_zone(g, exroom) == zrnum)
          continue;
        if (room_get_zone(g, exroom) == room_get_zone(g, i))
          continue;

        for (k = 0; offlimit_zones[k] != -1; k++) {
          if (room_get_zone(g, exroom) == find_zone(g, offlimit_zones[k]) && (found = 1))
            buf += str_format("- Exit {} cannot connect to {:d} (zone off limits).\r\n",
                              dirs[j], exroom);
        } /* for (k.. */
      } /* cycle directions */

      if (room_flagged(g, i, ROOM_ATRIUM | ROOM_HOUSE | ROOM_HOUSE_CRASH | ROOM_OLC | ROOM_BFS_MARK))
        buf += str_format("- Has illegal affection bits set ({} {} {} {} {})\r\n",
                          room_flagged(g, i, ROOM_ATRIUM) ? "ATRIUM" : "",
                          room_flagged(g, i, ROOM_HOUSE) ? "HOUSE" : "",
                          room_flagged(g, i, ROOM_HOUSE_CRASH) ? "HCRSH" : "",
                          room_flagged(g, i, ROOM_OLC) ? "OLC" : "",
                          room_flagged(g, i, ROOM_BFS_MARK) ? "*" : "");

      if ((MIN_ROOM_DESC_LENGTH) && strlen(text_of(room_get_description(g, i))) < MIN_ROOM_DESC_LENGTH && (found = 1))
        buf += str_format("- Room description is too short. ({:>4} of min. {:d} characters).\r\n",
                          printf_digits(as_unsigned(strlen(text_of(room_get_description(g, i)))), 4), MIN_ROOM_DESC_LENGTH);

      if (strncmp(text_of(room_get_description(g, i)), "   ", 3) && (found = 1))
        buf += "- Room description not formatted with indent (/fi in the editor).\r\n";

      /* strcspan = size of text in first arg before any character in second arg */
      if ((strcspn(text_of(room_get_description(g, i)), "\r\n") > MAX_COLOUMN_WIDTH) && (found = 1))
        buf += str_format("- Room description not wrapped at {:d} chars (/fi in the editor).\r\n",
                          MAX_COLOUMN_WIDTH);

      bool unformatted = false;
      for (const extra_descr_data &ext : room_get_ex_description(g, i))
        if (strncmp(ext.description.c_str(), "   ", 3))
          unformatted = true;

      if (unformatted && (found = 1))
        buf += "- has unformatted extra description\r\n";

      if (found) {
        send_to_char(g, ch, "[{:>5d}] {:<30}: \r\n{}",
                     i, room_get_name(g, i) ? room_get_name(g, i) : "An unnamed room", buf);
        buf.clear();
        found = 0;
      }
    } /*is room in this zone?*/
  } /*checking rooms*/

  for (room_vnum i : g->rooms.vnums()) {
    if (room_get_zone(g, i) == zrnum) {
      m++;
      for (j = 0, k = 0; j < NUM_OF_DIRS; j++)
        if (!room_get_dir_option(g, i, j))
          k++;

      if (k == NUM_OF_DIRS)
        l++;
    }
  }
  if (l * 3 > m)
    send_to_char(g, ch, "More than 1/3 of the rooms are not linked.\r\n");
}
/**********************************************************************************/

static void mob_checkload(GameWorld *g, struct char_data *ch, mob_vnum mvnum)
{
  int cmd_no, count = 0;
  mob_vnum mrnum = mvnum;

  if (!mob_proto_of(g, mrnum)) {
    send_to_char(g, ch, "That mob does not exist.\r\n");
    return;
  }

  send_to_char(g, ch, "Checking load info for the mob [{:d}] {}...\r\n",
               mvnum, char_get_sdesc(mob_proto_of(g, mrnum)));

  for (struct zone_data *zone : g->zones.all()) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      if (ZCMD2.command != 'M')
        continue;

      /* read a mobile */
      if (ZCMD2.arg1 == mrnum) {
        send_to_char(g, ch, "  [{:>5d}] {} ({:d} MAX)\r\n",
                     ZCMD2.arg3,
                     room_get_name(g, ZCMD2.arg3),
                     ZCMD2.arg2);
        count += 1;
      }
    }
  }
  if (count > 0)
    send_to_char(g, ch, "@D[@nTotal counted: {}.@D]@n\r\n", add_commas(count));
}

static void obj_checkload(GameWorld *g, struct char_data *ch, obj_vnum ovnum)
{
  int cmd_no, count = 0;
  obj_vnum ornum       = ovnum;
  room_vnum lastroom_v = 0;
  room_vnum lastroom_r = 0;
  mob_vnum lastmob_r   = 0;

  if (!obj_proto_of(g, ornum)) {
    send_to_char(g, ch, "That object does not exist.\r\n");
    return;
  }

  send_to_char(g, ch, "Checking load info for the obj [{:d}] {}...\r\n",
               ovnum, obj_get_short_description(obj_proto_of(g, ornum)));

  for (struct zone_data *zone : g->zones.all()) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = ZCMD2.arg3;
          lastroom_r = ZCMD2.arg3;
          lastmob_r  = ZCMD2.arg1;
          break;
        case 'O': /* read an object */
          lastroom_v = ZCMD2.arg3;
          lastroom_r = ZCMD2.arg3;
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [{:>5d}] {} ({:d} Max)\r\n",
                         lastroom_v,
                         room_get_name(g, lastroom_r),
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'P': /* object to object */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [{:>5d}] {} (Put in another object [{:d} Max])\r\n",
                         lastroom_v,
                         room_get_name(g, lastroom_r),
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'G': /* obj_to_char */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [{:>5d}] {} (Given to {} [{:d}][{:d} Max])\r\n",
                         lastroom_v,
                         room_get_name(g, lastroom_r),
                         char_get_sdesc(mob_proto_of(g, lastmob_r)),
                         mob_index_of(g, lastmob_r)->vnum,
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'E': /* object to equipment list */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(g, ch, "  [{:>5d}] {} (Equipped to {} [{:d}][{:d} Max])\r\n",
                         lastroom_v,
                         room_get_name(g, lastroom_r),
                         char_get_sdesc(mob_proto_of(g, lastmob_r)),
                         mob_index_of(g, lastmob_r)->vnum,
                         ZCMD2.arg2);
            count += 1;
          }
          break;
        case 'R': /* rem obj from room */
          lastroom_v = ZCMD2.arg1;
          lastroom_r = ZCMD2.arg1;
          if (ZCMD2.arg2 == ornum) {
            send_to_char(g, ch, "  [{:>5d}] {} (Removed from room)\r\n",
                         lastroom_v,
                         room_get_name(g, lastroom_r));
            count += 1;
          }
          break;
      } /* switch */
    } /*for cmd_no......*/
  } /*for zone...*/

  if (count > 0)
    send_to_char(g, ch, "@D[@nTotal counted: {}.@D]@n\r\n", add_commas(count));
}

static void trg_checkload(GameWorld *g, struct char_data *ch, trig_vnum tvnum)
{
  int cmd_no, found = 0;
  struct index_data *trnum = find_trigger(g, tvnum);
  room_vnum lastroom_v = 0;
  room_vnum lastroom_r     = 0;
  mob_vnum lastmob_r       = 0;
  obj_vnum lastobj_r       = 0;
  struct trig_proto_list *tpl;

  if (trnum == nullptr) {
    send_to_char(g, ch, "That trigger does not exist.\r\n");
    return;
  }

  send_to_char(g, ch, "Checking load info for the {} trigger [{:d}] '{}':\r\n",
               trnum->proto->attach_type == MOB_TRIGGER ? "mobile" : (trnum->proto->attach_type == OBJ_TRIGGER ? "object" : "room"),
               tvnum, trnum->proto->name);

  for (struct zone_data *zone : g->zones.all()) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = ZCMD2.arg3;
          lastroom_r = ZCMD2.arg3;
          lastmob_r  = ZCMD2.arg1;
          break;
        case 'O': /* read an object */
          lastroom_v = ZCMD2.arg3;
          lastroom_r = ZCMD2.arg3;
          lastobj_r  = ZCMD2.arg1;
          break;
        case 'P': /* object to object */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'G': /* obj_to_char */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'E': /* object to equipment list */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'R': /* rem obj from room */
          lastroom_v = 0;
          lastroom_r = 0;
          lastobj_r  = 0;
          lastmob_r  = 0;
          break;
        case 'T': /* trigger to something */
          if (ZCMD2.arg2 != tvnum)
            break;
          if (ZCMD2.arg1 == MOB_TRIGGER) {
            send_to_char(g, ch, "mob [{:>5d}] {:<60} (zedit room {:>5d})\r\n",
                         mob_index_of(g, lastmob_r)->vnum,
                         char_get_sdesc(mob_proto_of(g, lastmob_r)),
                         lastroom_v);
            found = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER) {
            send_to_char(g, ch, "obj [{:>5d}] {:<60}  (zedit room {:d})\r\n",
                         obj_index_of(g, lastobj_r)->vnum,
                         obj_get_short_description(obj_proto_of(g, lastobj_r)),
                         lastroom_v);
            found = 1;
          } else if (ZCMD2.arg1 == WLD_TRIGGER) {
            send_to_char(g, ch, "room [{:>5d}] {:<60} (zedit)\r\n",
                         lastroom_v,
                         room_get_name(g, lastroom_r));
            found = 1;
          }
          break;
      } /* switch */
    } /*for cmd_no......*/
  } /*for zone...*/

  for (mob_vnum i : g->mob_proto.vnums()) {
    if (!char_get_proto_script(mob_proto_of(g, i)))
      continue;

    for (tpl = char_get_proto_script(mob_proto_of(g, i)); tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(g, ch, "mob [{:>5d}] {}\r\n",
                     mob_index_of(g, i)->vnum,
                     char_get_sdesc(mob_proto_of(g, i)));
        found = 1;
      }
  }

  for (obj_vnum j : g->obj_proto.vnums()) {
    if (!obj_get_proto_script(obj_proto_of(g, j)))
      continue;

    for (tpl = obj_get_proto_script(obj_proto_of(g, j)); tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(g, ch, "obj [{:>5d}] {}\r\n",
                     obj_index_of(g, j)->vnum,
                     obj_get_short_description(obj_proto_of(g, j)));
        found = 1;
      }
  }

  for (room_vnum k : g->rooms.vnums()) {
    if (!room_get_proto_script(g, k))
      continue;

    for (tpl = room_get_proto_script(g, k); tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(g, ch, "room[{:>5d}] {}\r\n",
                     k,
                     room_get_name(g, k));
        found = 1;
      }
  }

  if (!found)
    send_to_char(g, ch, "This trigger is not attached to anything.\r\n");
}

ACMD(do_checkloadstatus)
{
  std::string_view rest = argument;
  std::string kind      = take_arg(rest);
  std::string number    = take_arg(rest);

  if (kind.empty() || number.empty() || !isdigit(number[0])) {
    send_to_char(g, ch, "Checkload <M | O | T> <vnum>\r\n");
    return;
  }
  switch (tolower(kind[0])) {
    case 'm': mob_checkload(g, ch, parse_int(number)); break;
    case 'o': obj_checkload(g, ch, parse_int(number)); break;
    case 't': trg_checkload(g, ch, parse_int(number)); break;
    default: break;
  }
}
/**************************************************************************************/
/*                         Zone Checker code above                                    */
/**************************************************************************************/

/* do_findkey, finds where the key to a door loads to using do_checkloadstatus() */
ACMD(do_findkey)
{
  int dir;
  obj_vnum key;
  std::string buf;

  std::string_view rest = argument;
  std::string arg       = take_word(rest);          /* not take_arg(): "in" is a valid direction */

  if (arg.empty()) {
    send_to_char(g, ch, "Format: findkey <dir>\r\n");
  } else if ((dir = search_block(arg, dirs, false)) >= 0 ||
             (dir = search_block(arg, abbr_dirs, false)) >= 0) {
    if (!char_get_exit(g, ch, dir)) {
      send_to_char(g, ch, "There's no exit in that direction!\r\n");
    } else if ((key = char_get_exit(g, ch, dir)->key) == NOTHING || key == 0) {
      send_to_char(g, ch, "There's no key for that exit.\r\n");
    } else {
      buf = str_format("obj {:d}", key);
      do_checkloadstatus(g, ch, buf, 0, 0);
    }
  } else {
    send_to_char(g, ch, "What direction is that?!?\r\n");
  }
}

ACMD(do_spells)
{
  int i, qend;

  send_to_char(g, ch, "The following spells are in the game:\r\n");

  for (qend = 0, i = 0; i < MAX_SPELLS; i++) {
    if (g->spell_info[i].name == unused_spellname) /* This is valid. */
      continue;
    send_to_char(g, ch, "{:>18}", g->spell_info[i].name);
    if (qend++ % 4 == 3)
      send_to_char(g, ch, "\r\n");
  }
  if (qend % 4 != 0)
    send_to_char(g, ch, "\r\n");
  return;
}

struct audit_reader {
  GameWorld *g{};
  struct char_data *ch{};
};

static void audit_report_to_char(void *context, const char *problem)
{
  struct audit_reader *reader = static_cast<struct audit_reader *>(context);

  send_to_char(reader->g, reader->ch, "{}\r\n", problem);
}

ACMD(do_audit)
{
  struct audit_reader reader = {g, ch};
  int problems               = world_audit(g, audit_report_to_char, &reader);

  send_to_char(g, ch, "World audit: {:d} problem{} found.\r\n", problems, problems == 1 ? "" : "s");
}
