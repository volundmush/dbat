/************************************************************************
 * OasisOLC - Zones / zedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include <algorithm>
#include <cctype>
#include <span>

#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/gengld.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/modify.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/races.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/zones.hpp"
#include "dbat/olc/commands.hpp"
#include "dbat/olc/limits.hpp"

namespace {
  /* ---- the arguments of a reset command, as zedit asks for them ---- */

  /* One kind of argument: how it's asked for, why a number won't do, and what of it is kept */
  struct arg_kind {
    void (*ask)(GameWorld *g, struct descriptor_data *d);
    std::string_view (*refuse)(GameWorld *g, int n); /* none, or the refusal: empty for none */
    int (*keep)(int n);                         /* null to keep it as it is */
  };

  void say(GameWorld *g, struct descriptor_data *d, std::string_view text) { write_to_output(g, d, text); }

  constexpr arg_kind MOB_VNUM = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input mob's vnum : "); },
      [](GameWorld *g, int n) -> std::string_view { return mob_proto_of(g, n) ? std::string_view() : "That mobile does not exist, try again : "; },
      nullptr};
  constexpr arg_kind OBJ_VNUM = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input object vnum : "); },
      [](GameWorld *g, int n) -> std::string_view { return obj_proto_of(g, n) ? std::string_view() : "That object does not exist, try again : "; },
      nullptr};
  constexpr arg_kind TRIGGER_TYPE = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input trigger type (0:mob, 1:obj, 2:room) :"); },
      [](GameWorld *, int n) -> std::string_view { return n < MOB_TRIGGER || n > WLD_TRIGGER ? "Invalid input." : std::string_view(); },
      nullptr};
  constexpr arg_kind MAX_LOADED = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input the maximum number that can exist on the mud : "); },
      nullptr,
      [](int n) { return MIN(MAX_DUPLICATES, n); }};
  constexpr arg_kind DOOR = {
      [](GameWorld *g, descriptor_data *d) {
        for (int i = 0; i < NUM_OF_DIRS; i++)
          write_to_output(g, d, "{:d}) Exit {}.\r\n", i, dirs[i]);
        say(g, d, "Enter exit number for door : ");
      },
      [](GameWorld *, int n) -> std::string_view { return n < 0 || n > NUM_OF_DIRS ? "Try again : " : std::string_view(); }, /* quirk: one past the last */
      nullptr};
  constexpr arg_kind REMOVED_VNUM = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input object's vnum : "); },
      [](GameWorld *g, int n) -> std::string_view { return obj_proto_of(g, n) ? std::string_view() : "That object does not exist, try again : "; },
      nullptr};
  constexpr arg_kind TRIGGER_VNUM = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Enter the trigger VNum : "); },
      [](GameWorld *g, int n) -> std::string_view { return find_trigger(g, n) ? std::string_view() : "That trigger does not exist, try again : "; },
      nullptr};
  constexpr arg_kind CONTEXT = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Global's context (0 for none) : "); },
      nullptr,
      nullptr};
  constexpr arg_kind WEAR_POSITION = {
      [](GameWorld *g, descriptor_data *d) {
        for (int i = 0; i < NUM_WEARS; i += 2) {
          const bool pair = i + 1 < NUM_WEARS;
          write_to_output(g, d, "{:>2d}) {:>26.26} {:>2d}) {:>26.26}\r\n", i, equipment_types[i], i + 1, pair ? equipment_types[i + 1] : "");
          if (!pair)
            break;
        }
        say(g, d, "Location to equip : ");
      },
      [](GameWorld *, int n) -> std::string_view {
        return n < 0 || n > NUM_WEARS ? "Try again : " : std::string_view(); /* quirk: one past the last */
      },
      nullptr};
  constexpr arg_kind CONTAINER = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Virtual number of the container : "); },
      [](GameWorld *g, int n) -> std::string_view { return obj_proto_of(g, n) ? std::string_view() : "That object does not exist, try again : "; },
      nullptr};
  constexpr arg_kind DOOR_STATE = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "0)  Door open\r\n1)  Door closed\r\n2)  Door locked\r\nEnter state of the door : "); },
      [](GameWorld *, int n) -> std::string_view { return n < 0 || n > 2 ? "Try again : " : std::string_view(); },
      nullptr};
  constexpr arg_kind ROOM_MAX = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input the max allowed to load from this room (Pressing enter == 0(ignore)) : "); },
      nullptr,
      [](int n) { return MIN(MAX_FROM_ROOM, n); }};
  constexpr arg_kind CHANCE = {
      [](GameWorld *g, descriptor_data *d) { say(g, d, "Input the percentage chance of the load NOT occurring : "); },
      [](GameWorld *, int n) -> std::string_view { return n < 0 || n > 100 ? "Value must be from 0 to 100" : std::string_view(); },
      nullptr};

  /* An argument a command asks for: which of its five, and of what kind */
  struct arg_step {
    int number; /* 1 to 5 */
    const arg_kind *kind;
  };

  /* A reset command zedit can make: what it asks for, in order, and where the room goes */
  struct command_kind {
    char letter;
    std::span<const arg_step> steps;
    bool room_first;   /* arg1 is the room, not asked for */
    bool room_third;   /* arg3 is the room, once arg2 is in */
    bool names_global; /* then the global's name and value (V) */
  };

  constexpr arg_step LOAD_MOB[]   = {{1, &MOB_VNUM}, {2, &MAX_LOADED}, {4, &ROOM_MAX}, {5, &CHANCE}};
  constexpr arg_step LOAD_OBJ[]   = {{1, &OBJ_VNUM}, {2, &MAX_LOADED}, {4, &ROOM_MAX}, {5, &CHANCE}};
  constexpr arg_step GIVE[]       = {{1, &OBJ_VNUM}, {2, &MAX_LOADED}, {5, &CHANCE}};
  constexpr arg_step EQUIP[]      = {{1, &OBJ_VNUM}, {2, &MAX_LOADED}, {3, &WEAR_POSITION}, {5, &CHANCE}};
  constexpr arg_step PUT[]        = {{1, &OBJ_VNUM}, {2, &MAX_LOADED}, {3, &CONTAINER}, {5, &CHANCE}};
  constexpr arg_step SET_DOOR[]   = {{2, &DOOR}, {3, &DOOR_STATE}};
  constexpr arg_step REMOVE[]     = {{2, &REMOVED_VNUM}};
  constexpr arg_step TRIGGER[]    = {{1, &TRIGGER_TYPE}, {2, &TRIGGER_VNUM}};
  constexpr arg_step VARIABLE[]   = {{1, &TRIGGER_TYPE}, {2, &CONTEXT}};

  constexpr command_kind COMMANDS[] = {
      {'M', LOAD_MOB, false, true,  false},
      {'O', LOAD_OBJ, false, true,  false},
      {'P', PUT,      false, false, false},
      {'E', EQUIP,    false, false, false},
      {'D', SET_DOOR, true,  false, false},
      {'G', GIVE,     false, false, false},
      {'R', REMOVE,   true,  false, false},
      {'T', TRIGGER,  false, true,  false},
      {'V', VARIABLE, false, true,  true },
  };

  int reset_com::*const ARGS[] = {nullptr, &reset_com::arg1, &reset_com::arg2, &reset_com::arg3, &reset_com::arg4, &reset_com::arg5};

  /* What a reset command's arg1 says the thing is: a mobile, object or room */
  std::string_view attached_to(int type)
  {
    return type == MOB_TRIGGER ? "mobile" : type == OBJ_TRIGGER ? "object" : type == WLD_TRIGGER ? "room" : "????";
  }

  /* One of the room's reset commands, as the menu lists it */
  void describe(GameWorld *g, struct descriptor_data *d, const reset_com &c)
  {
    std::string_view then = c.if_flag ? " then " : "";
    switch (c.command) {
      case 'M':
        write_to_output(g, d, "{}Load {}@y [@c{:d}@y], Max : {:d}, MaxR {:d}, Chance {:d}", then, char_get_sdesc(mob_proto_of(g, c.arg1)),
                        mob_index_of(g, c.arg1)->vnum, c.arg2, c.arg4, c.arg5);
        break;
      case 'G':
        write_to_output(g, d, "{}Give it {}@y [@c{:d}@y], Max : {:d}, Chance {:d}", then, obj_get_short_description(obj_proto_of(g, c.arg1)),
                        obj_index_of(g, c.arg1)->vnum, c.arg2, c.arg5);
        break;
      case 'O':
        write_to_output(g, d, "{}Load {}@y [@c{:d}@y], Max : {:d}, MaxR {:d}, Chance {:d}", then, obj_get_short_description(obj_proto_of(g, c.arg1)),
                        obj_index_of(g, c.arg1)->vnum, c.arg2, c.arg4, c.arg5);
        break;
      case 'E':
        write_to_output(g, d, "{}Equip with {}@y [@c{:d}@n], {}, Max : {:d}, Chance {:d}", then, obj_get_short_description(obj_proto_of(g, c.arg1)),
                        obj_index_of(g, c.arg1)->vnum, equipment_types[c.arg3], c.arg2, c.arg5);
        break;
      case 'P':
        write_to_output(g, d, "{}Put {}@y [@c{:d}@n] in {} [@c{:d}@n], Max : {:d}, % Chance {:d}", then,
                        obj_get_short_description(obj_proto_of(g, c.arg1)), obj_index_of(g, c.arg1)->vnum,
                        obj_get_short_description(obj_proto_of(g, c.arg3)), obj_index_of(g, c.arg3)->vnum, c.arg2, c.arg5);
        break;
      case 'R':
        write_to_output(g, d, "{}Remove {}@y [@c{:d}@n] from room.", then, obj_get_short_description(obj_proto_of(g, c.arg2)),
                        obj_index_of(g, c.arg2)->vnum);
        break;
      case 'D':
        write_to_output(g, d, "{}Set door {}@y as {}.", then, dirs[c.arg2], c.arg3 ? ((c.arg3 == 1) ? "closed" : "locked") : "open");
        break;
      case 'T':
        write_to_output(g, d, "{}Attach trigger @c{}@y [@c{:d}@y] to {}, % Chance {:d}", then, find_trigger(g, c.arg2)->proto->name, c.arg2,
                        attached_to(c.arg1), c.arg5);
        break;
      case 'V':
        write_to_output(g, d, "{}Assign global {}:{:d} to {} = {}, % Chance {:d}", then, c.sarg1, c.arg2, attached_to(c.arg1), c.sarg2, c.arg5);
        break;
      default:
        write_to_output(g, d, "<Unknown Command>");
        break;
    }
  }

  /* The room a zone command acts on, or before (G, E, P) the one of the command before it */
  room_vnum command_room(const reset_com &c, room_vnum before)
  {
    switch (c.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V': return c.arg3;
      case 'D':
      case 'R': return c.arg1;
      default: return before;
    }
  }

  /* The zone editor: the reset commands of one room of a zone, and the zone's own settings */
  struct zone_editor final : olc_session {
    enum class mode {
      main_menu,
      confirm_save,
      new_entry,
      delete_entry,
      change_entry,
      command_type,
      if_flag,
      argument, /* step of the command's kind */
      global_name,
      global_value,
      zone_name,
      zone_builders,
      zone_reset,
      zone_life,
      zone_bot,
      zone_top,
      zone_flags,
      min_level,
      max_level,
    };

    mode asking = mode::main_menu;
    room_vnum room{};
    struct zone_data *zone{}; /* the zone in the game */
    zone_data edit;           /* its settings, and the room's commands, as edited */
    bool settings_changed{};
    bool commands_changed{};
    int editing{}; /* the command being edited */
    const command_kind *kind{};
    size_t step{};

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone->number); }

    reset_com &command() { return edit.cmd[editing]; }
    void setup(GameWorld *g);
    void menu(GameWorld *g, struct descriptor_data *d);
    void flag_menu(GameWorld *g, struct descriptor_data *d);
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    bool start_change(int pos);
    void command_types(GameWorld *g, struct descriptor_data *d);
    void command_type_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void start_arguments(GameWorld *g, struct descriptor_data *d);
    void ask_step(GameWorld *g, struct descriptor_data *d);
    void argument_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save(GameWorld *g, struct descriptor_data *d);
    bool range_loses_something(GameWorld *g, struct descriptor_data *d, int bot, int top);
  };

  /* The zone's settings, and the commands acting on the room, unchanged so far */
  void zone_editor::setup(GameWorld *g)
  {
    edit.name          = zone->name;
    edit.builders      = zone->builders;
    edit.lifespan      = zone->lifespan;
    edit.bot           = zone->bot;
    edit.top           = zone->top;
    edit.reset_mode    = zone->reset_mode;
    std::ranges::copy(zone->zone_flags, edit.zone_flags);
    edit.min_level = zone->min_level;
    edit.max_level = zone->max_level;
    edit.cmd       = empty_cmd_list();

    room_vnum at = NOWHERE;
    int count    = 0;
    for (int subcmd = 0; zone_get_cmd(g, zone, subcmd)->command != 'S'; subcmd++) {
      const reset_com &c = *zone_get_cmd(g, zone, subcmd);
      if ((at = command_room(c, at)) == room)
        add_cmd_to_list(&edit.cmd, &c, count++);
    }
  }

  void zone_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    send_to_char(g, d->character,
                 "Room number: [@c{:d}@n]		Room zone: @c{:d}\r\n@g1@n) Builders       : @y{}\r\n@gA@n) Zone name      : @y{}\r\n@gL@n) Lifespan       : @y{:d} minutes\r\n@gB@n) Bottom of zone : @y{:d}\r\n@gT@n) Top of zone    : @y{:d}\r\n@gR@n) Reset Mode     : @y{}@n\r\n@gF@n) Zone Flags     : @y{}@n\r\n@gM@n) Min Level      : @y{:d}@n\r\n@gX@n) Max Level      : @y{:d}@n\r\n@gZ@n) Wiznet         :\r\n[Command list]\r\n",
                 room, zone->number, edit.builders, edit.name, edit.lifespan, edit.bot, edit.top,
                 edit.reset_mode ? ((edit.reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
                 sprintbitarray(edit.zone_flags, zone_bits, ZF_ARRAY_MAX), edit.min_level, edit.max_level);

    int counter = 0;
    for (const reset_com &c : edit.cmd) {
      if (c.command == 'S')
        break;
      write_to_output(g, d, "@n{:d} - @y", counter++);
      describe(g, d, c);
      write_to_output(g, d, "\r\n");
    }
    write_to_output(g, d,
                    "@n{:d} - <END OF LIST>\r\n@gN@n) Insert new command.\r\n@gE@n) Edit a command.\r\n@gD@n) Delete a command.\r\n@gQ@n) Quit\r\nEnter your choice : ",
                    counter);
    asking = mode::main_menu;
  }

  void zone_editor::flag_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, zone_bits, NUM_ZONE_FLAGS, 1, " ", 2);
    write_to_output(g, d, "\r\nZone flags: @c{}@n\r\nEnter Zone flags, 0 to quit : ", sprintbitarray(edit.zone_flags, zone_bits, ZF_ARRAY_MAX));
    asking = mode::zone_flags;
  }

  bool zone_editor::start_change(int pos)
  {
    if (pos < 0 || pos >= count_commands(edit.cmd))
      return false;
    editing = pos;
    return true;
  }

  void zone_editor::command_types(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    write_to_output(g, d,
                    "\r\n@gM@n) Load Mobile to room             @gO@n) Load Object to room\r\n@gE@n) Equip mobile with object        @gG@n) Give an object to a mobile\r\n@gP@n) Put object in another object    @gD@n) Open/Close/Lock a Door\r\n@gR@n) Remove an object from the room\r\n@gT@n) Assign a trigger                @gV@n) Set a global variable\r\n\r\nWhat sort of command will this be? : ");
    asking           = mode::command_type;
    commands_changed = true;
  }

  void zone_editor::command_type_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    command().command = toupper(first_char(line)); /* quirk: kept even when refused */
    kind              = std::ranges::find(COMMANDS, command().command, &command_kind::letter);
    if (!command().command || kind == std::end(COMMANDS)) {
      write_to_output(g, d, "Invalid choice, try again : ");
      return;
    }
    if (editing == 0) { /* 'if-flag' not appropriate. */
      command().if_flag = false;
      start_arguments(g, d);
    } else if (command().command == 'T' || command().command == 'V') {
      command().if_flag = true;
      start_arguments(g, d);
    } else {
      write_to_output(g, d, "Is this command dependent on the success of the previous one? (y/n)\r\n");
      asking = mode::if_flag;
    }
  }

  void zone_editor::start_arguments(GameWorld *g, struct descriptor_data *d)
  {
    write_to_output(g, d, "\r\n");
    if (kind->room_first)
      command().arg1 = room;
    step = 0;
    ask_step(g, d);
  }

  void zone_editor::ask_step(GameWorld *g, struct descriptor_data *d)
  {
    if (step == kind->steps.size()) {
      if (kind->names_global) {
        write_to_output(g, d, "Enter the global name : ");
        asking = mode::global_name;
      } else
        menu(g, d);
      return;
    }
    const arg_step &s = kind->steps[step];
    if (s.number > 2) /* arg1's blank line came before the first prompt; arg2 has none */
      write_to_output(g, d, "\r\n");
    s.kind->ask(g, d);
    asking = mode::argument;
  }

  void zone_editor::argument_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const arg_step &s = kind->steps[step];
    if (line.empty() && s.number == 4)
      line = "0";
    if (!isdigit(first_char(line))) {
      write_to_output(g, d, "Must be a numeric value, try again : ");
      return;
    }
    const int n = parse_int(line);
    if (std::string_view refused = s.kind->refuse ? s.kind->refuse(g, n) : std::string_view(); !refused.empty()) {
      write_to_output(g, d, refused);
      return;
    }
    command().*ARGS[s.number] = s.kind->keep ? s.kind->keep(n) : n;
    if (s.number == 2 && kind->room_third)
      command().arg3 = room;
    step++;
    ask_step(g, d);
  }

  /*
   * Puts the room's commands into the zone, in place of those it had, and the settings too if they were changed.
   * A G, E or P with no M or O before it isn't kept: the commands are grouped by room, and it would latch onto
   * whatever was loaded before it in the zone. C.Raehl 4/27/99
   */
  void zone_editor::save(GameWorld *g, struct descriptor_data *d)
  {
    if (!room_of(g, room)) {
      mud_log("SYSERR: zedit_save_internally: OLC_NUM(d) room {:d} not found.", room);
      return;
    }
    remove_room_zone_commands(g, zone, room);

    bool mob_loaded = false, obj_loaded = false;
    for (int subcmd = 0; edit.cmd[subcmd].command != 'S'; subcmd++) {
      const reset_com &c = edit.cmd[subcmd];
      switch (c.command) {
        case 'G':
        case 'E':
          if (!mob_loaded) {
            write_to_output(g, d, "Equip/Give command not saved since no mob was loaded first.\r\n");
            continue;
          }
          break;
        case 'P':
          if (!obj_loaded) {
            write_to_output(g, d, "Put command not saved since another object was not loaded first.\r\n");
            continue;
          }
          break;
        case 'M': mob_loaded = true; break;
        case 'O': obj_loaded = true; break;
        default: mob_loaded = obj_loaded = false; break;
      }
      add_cmd_to_list(&zone->cmd, &c, subcmd); /* quirk: at its place among the room's, skipped ones and all */
    }

    if (settings_changed) {
      zone->name       = edit.name;
      zone->builders   = edit.builders;
      zone->bot        = edit.bot;
      zone->top        = edit.top;
      zone->reset_mode = edit.reset_mode;
      zone->lifespan   = edit.lifespan;
      std::ranges::copy(edit.zone_flags, zone->zone_flags);
      zone->min_level = edit.min_level;
      zone->max_level = edit.max_level;
    }
    add_to_save_list(g, zone->number, SL_ZON);
  }

  /*
   * Saving a zone only writes what's inside its range, so anything left outside would be gone for good
   * after the next save, along with every copy of it that players have. Says so if a new range would do that.
   */
  bool zone_editor::range_loses_something(GameWorld *g, struct descriptor_data *d, int bot, int top)
  {
    for (int vnum = genolc_zonep_bottom(zone); vnum <= zone->top; vnum++) {
      if (vnum >= bot && vnum <= top)
        continue;
      std::string_view what = room_of(g, vnum)                ? "room"
                         : mob_proto_of(g, vnum)         ? "mob"
                         : obj_proto_of(g, vnum)         ? "object"
                         : find_trigger(g, vnum)         ? "trigger"
                         : find_shop(g, vnum)            ? "shop"
                         : find_guild(g, vnum)           ? "guild"
                                                         : std::string_view();
      if (!what.empty()) {
        write_to_output(g, d, "That would leave {} {:d} outside the zone, and the next save would destroy it. Range not changed.\r\n", what, vnum);
        return true;
      }
    }
    return false;
  }

  void zone_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    switch (tolower(first_char(line))) {
      case 'q':
        if (commands_changed || settings_changed) {
          write_to_output(g, d, "Do you wish to save your changes? : ");
          asking = mode::confirm_save;
        } else {
          write_to_output(g, d, "No changes made.\r\n");
          leave_olc(g, d);
        }
        return;
      case 'n':
        if (edit.cmd[0].command == 'S' && new_command(&edit, 0) && start_change(0)) { /* the first command */
          command_types(g, d);
          return;
        }
        write_to_output(g, d, "What number in the list should the new command be? : ");
        asking = mode::new_entry;
        return;
      case 'e':
        write_to_output(g, d, "Which command do you wish to change? : ");
        asking = mode::change_entry;
        return;
      case 'd':
        write_to_output(g, d, "Which command do you wish to delete? : ");
        asking = mode::delete_entry;
        return;
      case 'a':
        write_to_output(g, d, "Enter new zone name : ");
        asking = mode::zone_name;
        return;
      case '1': /* We do not want to allow baby builders to tweak this. */
        if (char_get_admlevel(d->character) <= ADMLVL_BUILDER)
          write_to_output(g, d, "Access Denied.\r\n");
        else {
          write_to_output(g, d, "Enter new builders list : ");
          asking = mode::zone_builders;
        }
        return;
      case 'b':
      case 't':
        if (char_get_admlevel(d->character) < ADMLVL_IMPL)
          menu(g, d);
        else if (tolower(first_char(line)) == 'b') {
          write_to_output(g, d, "Enter new bottom of zone : ");
          asking = mode::zone_bot;
        } else {
          write_to_output(g, d, "Enter new top of zone : ");
          asking = mode::zone_top;
        }
        return;
      case 'l':
        write_to_output(g, d, "Enter new zone lifespan : ");
        asking = mode::zone_life;
        return;
      case 'r':
        write_to_output(g, d, "\r\n0) Never reset\r\n1) Reset only when no players in zone\r\n2) Normal reset\r\nEnter new zone reset type : ");
        asking = mode::zone_reset;
        return;
      case 'm':
        write_to_output(g, d, "Enter Minimum level to enter zone : \r\n");
        asking = mode::min_level;
        return;
      case 'x':
        write_to_output(g, d, "Enter Maximum level to enter zone : \r\n");
        asking = mode::max_level;
        return;
      case 'f':
        flag_menu(g, d);
        return;
      case 'z': {
        std::string message(line);
        search_replace(message, "z ", ""); /* quirk: every "z " in it, and not "Z " */
        do_wiznet(g, d->character, message, 0, 0);
        return;
      }
      default:
        menu(g, d);
        return;
    }
  }

  void zone_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const int n          = parse_int(line);
    const bool a_number  = isdigit(first_char(line));

    switch (asking) {
      case mode::confirm_save:
        if (const std::optional<bool> answer = yes_or_no(line); !answer) {
          write_to_output(g, d, "Invalid choice!\r\n");
          write_to_output(g, d, "Do you wish to save your changes? : ");
        } else {
          if (*answer) {
            save(g, d);
            if (CONFIG_OLC_SAVE) {
              write_to_output(g, d, "Saving zone info to disk.\r\n");
              save_zone(g, zone);
            } else
              write_to_output(g, d, "Saving zone info in memory.\r\n");
            mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} edits zone info for room {:d}.",
                   char_get_name(d->character), room);
          }
          leave_olc(g, d);
        }
        return;

      case mode::main_menu:
        main_menu_choice(g, d, line);
        return;

      case mode::new_entry:
        if (a_number && new_command(&edit, n) && start_change(n))
          command_types(g, d);
        else
          menu(g, d);
        return;

      case mode::delete_entry:
        if (a_number) {
          delete_zone_command(&edit, n);
          commands_changed = true;
        }
        menu(g, d);
        return;

      case mode::change_entry:
        if (toupper(first_char(line)) == 'A') { /* Abort edit - idea from Mark Garringer zizazat@hotmail.com */
          /* the command last edited, if it's still there: deleting can have shortened the list since */
          if (editing < count_commands(edit.cmd) && command().command == 'N')
            command().command = '*';
          menu(g, d);
        } else if (a_number && start_change(n))
          command_types(g, d);
        else
          menu(g, d);
        return;

      case mode::command_type:
        command_type_answer(g, d, line);
        return;

      case mode::if_flag:
        if (const std::optional<bool> dependent = yes_or_no(line)) {
          command().if_flag = *dependent;
          start_arguments(g, d);
        } else
          write_to_output(g, d, "Try again : ");
        return;

      case mode::argument:
        argument_answer(g, d, line);
        return;

      case mode::global_name:
        if (line.empty())
          write_to_output(g, d, "Must have some name to assign : ");
        else {
          command().sarg1 = line;
          write_to_output(g, d, "Enter the global value : ");
          asking = mode::global_value;
        }
        return;

      case mode::global_value:
        if (line.empty())
          write_to_output(g, d, "Must have some value to set it to :");
        else {
          command().sarg2 = line;
          menu(g, d);
        }
        return;

      case mode::zone_name:
      case mode::zone_builders: {
        std::string text(line);
        smash_tilde(text);
        (asking == mode::zone_name ? edit.name : edit.builders) = text;
        settings_changed                                         = true;
        menu(g, d);
        return;
      }

      case mode::zone_reset:
        if (!a_number || n < 0 || n > 2)
          write_to_output(g, d, "Try again (0-2) : ");
        else {
          edit.reset_mode  = n;
          settings_changed = true;
          menu(g, d);
        }
        return;

      case mode::zone_life:
        if (!a_number || n < 0 || n > 240)
          write_to_output(g, d, "Try again (0-240) : ");
        else {
          edit.lifespan    = n;
          settings_changed = true;
          menu(g, d);
        }
        return;

      case mode::zone_bot: {
        struct zone_data *previous = g->zones.before(zone->number);
        const int bot                    = LIMIT(n, previous ? previous->top + 1 : 0, edit.top);
        if (!range_loses_something(g, d, bot, edit.top)) {
          edit.bot         = bot;
          settings_changed = true;
        }
        menu(g, d);
        return;
      }

      case mode::zone_top: {
        struct zone_data *next = g->zones.after(zone->number);
        const int top          = LIMIT(n, genolc_zonep_bottom(&edit), next ? genolc_zone_bottom(g, next) - 1 : 65000);
        if (!range_loses_something(g, d, genolc_zonep_bottom(&edit), top)) {
          edit.top         = top;
          settings_changed = true;
        }
        menu(g, d);
        return;
      }

      case mode::zone_flags:
        if (n < 0 || n > NUM_ZONE_FLAGS) {
          write_to_output(g, d, "That is not a valid choice!\r\n");
          flag_menu(g, d);
        } else if (n == 0)
          menu(g, d);
        else {
          zone_data_toggle_flag(&edit, n - 1);
          settings_changed = true;
          flag_menu(g, d);
        }
        return;

      case mode::min_level:
      case mode::max_level:
        if (!a_number || n < 0 || n > CONFIG_LEVEL_CAP)
          write_to_output(g, d, "Try again (0 - CONFIG_LEVEL_CAP) : \r\n");
        else if (asking == mode::max_level && edit.min_level > n)
          write_to_output(g, d, "Max level can not be lower the Min level! :\r\n");
        else {
          (asking == mode::min_level ? edit.min_level : edit.max_level) = n;
          settings_changed                                              = true;
          menu(g, d);
        }
        return;
    }
  }

  int zone_editor::help_context(const struct descriptor_data *) const
  {
    switch (asking) {
      case mode::main_menu: return CONTEXT_ZEDIT_MAIN_MENU;
      case mode::delete_entry: return CONTEXT_ZEDIT_DELETE_ENTRY;
      case mode::new_entry: return CONTEXT_ZEDIT_NEW_ENTRY;
      case mode::change_entry: return CONTEXT_ZEDIT_CHANGE_ENTRY;
      case mode::command_type: return CONTEXT_ZEDIT_COMMAND_TYPE;
      case mode::if_flag: return CONTEXT_ZEDIT_IF_FLAG;
      case mode::argument:
        switch (kind->steps[step].number) {
          case 1: return CONTEXT_ZEDIT_ARG1;
          case 2: return CONTEXT_ZEDIT_ARG2;
          case 3: return CONTEXT_ZEDIT_ARG3;
          default: return NOTHING;
        }
      case mode::zone_name: return CONTEXT_ZEDIT_ZONE_NAME;
      case mode::zone_life: return CONTEXT_ZEDIT_ZONE_LIFE;
      case mode::zone_bot: return CONTEXT_ZEDIT_ZONE_BOT;
      case mode::zone_top: return CONTEXT_ZEDIT_ZONE_TOP;
      case mode::zone_reset: return CONTEXT_ZEDIT_ZONE_RESET;
      case mode::confirm_save: return CONTEXT_ZEDIT_CONFIRM_SAVESTRING;
      case mode::global_name: return CONTEXT_ZEDIT_SARG1;
      case mode::global_value: return CONTEXT_ZEDIT_SARG2;
      default: return NOTHING;
    }
  }

  void new_zone(GameWorld *g, struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top)
  {
    std::expected<zone_data *, std::string_view> result = create_new_zone(g, vzone_num, bottom, top);
    if (!result) {
      write_to_output(g, char_get_desc(ch), result.error());
      return;
    }
    save_zone(g, *result);
    mudlog(g, LOG_BRF, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} creates new zone #{:d}", char_get_name(ch), vzone_num);
    write_to_output(g, char_get_desc(ch), "Zone created successfully.\r\n");
  }
} // namespace

ACMD(do_oasis_zedit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);
  std::string buf2      = take_arg(rest);

  IDXTYPE number = NOWHERE;
  bool save      = false;
  if (buf1.empty()) /* the zone the builder is standing in */
    number = char_get_in_room(ch);
  else if (!isdigit(buf1[0])) {
    if (str_eq_nocase("save", buf1)) {
      save = true;
      if ((number = zone_to_save(g, ch, buf2)) == NOWHERE)
        return;
    } else if (char_get_admlevel(ch) >= ADMLVL_IMPL) {
      if (!str_eq_nocase("new", buf1) || skip_spaces(rest).empty())
        send_to_char(g, ch, "Format: zedit new <zone number> <bottom-room> <upper-room>\r\n");
      else {
        std::string bottom = take_arg(rest);
        std::string top    = take_arg(rest);
        number             = parse_int(buf2);
        new_zone(g, ch, number < 0 ? NOWHERE : number, parse_int(bottom), parse_int(top));
      }
      return;
    } else {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
  }
  if (number == NOWHERE)
    number = parse_int(buf1);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_ZEDIT && dd->olc && session_of<zone_editor>(dd).room == number) {
      send_to_char(g, ch, "That zone is currently being edited by {}.\r\n", char_pers(g, dd->character, ch));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<zone_editor>(), "do_oasis_zedit");
  zone_editor &editor = session_of<zone_editor>(d);

  if (!(editor.zone = zone_to_edit(g, ch, number, save))) {
    d->olc.reset();
    return;
  }

  if (save) {
    send_to_char(g, ch, "Saving all zone information for zone {:d}.\r\n", editor.zone->number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves zone information for zone {:d}.", char_get_name(ch),
           editor.zone->number);
    save_zone(g, editor.zone);
    d->olc.reset();
    return;
  }

  editor.room = number;
  if (!room_of(g, number)) {
    write_to_output(g, d, "That room does not exist.\r\n");
    d->olc.reset();
    return;
  }

  editor.setup(g);
  editor.menu(g, d);
  begin_editing(g, d, CON_ZEDIT);
  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}
