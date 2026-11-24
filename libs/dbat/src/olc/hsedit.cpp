/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  AderonMUD by Jamdog.                                                  *
*  Based on CircleMud3.5                                                 *
*  OasisOLC - hsedit.c                                                   *
*                                                                        *
*  Copyright 2007 Stefan Cole (aka Jamdog)                               *
*  Used at AderonMUD (aderonmud.genesismuds.com 3200)                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <algorithm>
#include "dbat/name_table.hpp"
#include <cstdlib>

#include "dbat/act.informative.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/house.hpp"
#include "dbat/hsedit.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/utils.hpp"
#include "dbat/olc/commands.hpp"

namespace {
  static constexpr std::string_view house_flags_entries[] = {"!GUEST", "FREE", "!IMM", "IMP_ONLY", "RENTFREE", "SAVE_!RENT", "!SAVE"};
  static constexpr name_table house_flags(house_flags_entries);

  static constexpr std::string_view house_types_entries[] = {"PLAYER_OWNED", "IMM_OWNED", "CLAN_OWNED", "UNOWNED"};
  static constexpr name_table house_types(house_types_entries);

  /* The atrium directions' menu lines, in the order of the directions */
  static constexpr std::string_view atrium_directions[] = {"North  ", "East   ", "South  ", "West   ", "Up     ", "Down   ",
                                           "NorthW ", "NorthE ", "SouthE ", "SouthW ", "Inside ", "Outside"};

  /* The house editor: the house in one room, as the house control list has it, or new */
  struct house_editor final : olc_session {
    /* numbered as hsedit_parse() logs them */
    enum class mode {
      main_menu     = 0,
      confirm_save  = 1,
      owner_menu    = 2,
      owner_name    = 3,
      owner_id      = 4,
      atrium        = 6,
      dir_menu      = 7,
      guest_menu    = 8,
      guest_add     = 9,
      guest_delete  = 10,
      guest_clear   = 11,
      flags         = 12,
      build_date    = 13,
      payment       = 14,
      type          = 15,
      confirm_delete = 16,
      no_vnum       = 21,
    };

    mode asking = mode::main_menu;
    struct zone_data *zone{};   /* the zone the house's room is in */
    room_vnum number = NOWHERE; /* the house's room */
    house_control_rec house{};  /* the house as it's being edited */
    bool changed = false;

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override
    {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing zone {:d} allowed zone {:d}", char_get_name(d->character),
             zone ? zone->number : NOWHERE, char_get_olc_zone(g, d->character));
    }

    void menu(GameWorld *g, struct descriptor_data *d);
    void owner_menu(GameWorld *g, struct descriptor_data *d);
    void dir_menu(GameWorld *g, struct descriptor_data *d);
    void type_menu(GameWorld *g, struct descriptor_data *d);
    void guest_menu(GameWorld *g, struct descriptor_data *d);
    void flags_menu(GameWorld *g, struct descriptor_data *d);
    void save(GameWorld *g, struct descriptor_data *d);
    void delete_house(GameWorld *g, struct descriptor_data *d);
    bool parse_guests(GameWorld *g, struct descriptor_data *d, std::string_view line);
  };

  /* The guests' names, as the main menu lists them */
  std::string list_guests(GameWorld *g, const house_control_rec &house)
  {
    if (house.num_of_guests == 0)
      return "NONE";
    std::string list;
    int printed = 0;
    for (int j = 0; j < house.num_of_guests; j++) {
      const std::string *name = get_name_by_id(g, house.guests[j]);
      if (!name) /* Avoid <UNDEF>. -gg 6/21/98 */
        continue;
      printed++;
      list += str_format("{} ", upper_first(*name));
    }
    return printed ? list : "all dead";
  }

  /* A date as the menu shows it: the day, without the time */
  std::string menu_date(time_t when, std::string_view never)
  {
    if (!when)
      return std::string(never);
    return asctime_text(when).substr(0, 10);
  }

  void house_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    send_to_char(g, d->character, "{}",
                 str_format("@D-- @RJamdog's House OLC Editor @D--\r\n@D--@g House number : @D[@c{:d}@D]     @gHouse zone: @D[@c{:d}@D]\r\n@g1@D)@g Owner       : @c{:d} -- {}\r\n@g2@D)@g Atrium      : @c{:d}\r\n@g3@D)@g Direction   : @c{}\r\n@g4@D)@g House Type  : @c{}\r\n@g5@D)@g Built on    : @c{}\r\n@g6@D)@g Payment     : @c{}\r\n@g7@D)@g Guests      : @c{}\r\n@g8@D)@g Flags       : @c{}\r\n@gX@D)@g Delete this house\r\n@gQ@D)@g Quit\r\n@gEnter choice : @n",
                            number, zone->number, house.owner, player_name_or(g, house.owner, "(NOBODY)"), house.atrium,
                            house.exit_num >= 0 && house.exit_num <= 11 ? dirs[house.exit_num] : "NONE", house_types[house.mode],
                            menu_date(house.built_on, "Unknown"), menu_date(house.last_payment, "None"), list_guests(g, house),
                            sprintbit(house.bitvector, house_flags)));
    asking = mode::main_menu;
  }

  void house_editor::owner_menu(GameWorld *g, struct descriptor_data *d)
  {
    send_to_char(g, d->character, str_format("@g1@D)@g Owner Name : @c{}@n\r\n@g2@D)@g Owner ID   : @c{:d}\r\n@gQ@D)@g Back to main menu\r\n@gEnter choice : @n",
                                             player_name_or(g, house.owner, ""), house.owner)); /* no owner: "(null)", once */
    asking = mode::owner_menu;
  }

  void house_editor::dir_menu(GameWorld *g, struct descriptor_data *d)
  {
    mudlog(g, LOG_CMP, ADMLVL_BUILDER, true, "(LOG) hsedit_dir_menu: house vnum = {:d}", house.vnum);
    if (house.vnum < 0 || !room_of(g, house.vnum)) {
      send_to_char(g, d->character, "WARNING: You cannot set an atium direction before selecting a valid room vnum\r\n(Press Enter)\r\n");
      asking = mode::no_vnum;
      return;
    }
    std::string menu;
    for (int dir = 0; dir < 12; dir++) {
      struct room_direction_data *exit = room_get_dir_option(g, house.vnum, dir);
      room_vnum to                     = exit ? exit->to_room : NOWHERE;
      menu += str_format("@g{:d}@D)@g {}: @D(@c{}@D)\r\n", dir + 1, atrium_directions[dir], to == NOWHERE ? "NO ROOM" : room_get_name(g, to));
    }
    menu += "@gQ@D)@g Back to main menu\r\n@gEnter atrium direction : @n";
    asking = mode::dir_menu;
    send_to_char(g, d->character, menu);
  }

  void house_editor::type_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int type = 0; type < NUM_HOUSE_TYPES; type++) {
      send_to_char(g, d->character, "@g{:>2d}@D)@y {:<20.20} @n", type, house_types[type]);
      if (type % 2)
        send_to_char(g, d->character, "\r\n");
    }
    send_to_char(g, d->character, "\r\nEnter house type : ");
    asking = mode::type;
  }

  void house_editor::guest_menu(GameWorld *g, struct descriptor_data *d)
  {
    std::string menu;
    for (int i = 0; i < MAX_GUESTS; i++) {
      menu += str_format("{}@g{:d}@D)@c {} @D(@gID: @c{:d}@D)@n\r\n", i < 9 ? " " : "", i + 1, player_name_or(g, house.guests[i], "@D(@yNOT SET@D)@n"),
                         house.guests[i] < 1 ? 0 : house.guests[i]);
    }
    menu += "\r\n@gA@D)@g Add a guest\r\n@gD@D)@g Delete a guest\r\n@gC@D)@g Clear guest list\r\n@gQ@D)@g Back to main menu\r\n@gEnter selection @D(@gA@D/@gD@D/@gC@D/@gQ@D)@n: ";
    send_to_char(g, d->character, menu);
    asking = mode::guest_menu;
  }

  void house_editor::flags_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int flag = 0; flag < HOUSE_NUM_FLAGS; flag++) {
      send_to_char(g, d->character, "@g{:>2d}@D)@y {:<20.20} @n", flag + 1, house_flags[flag]);
      if (flag % 2)
        send_to_char(g, d->character, "\r\n");
    }
    send_to_char(g, d->character, "\r\nHouse flags: @g{}@n\r\nEnter house flags, 0 to quit : ", sprintbit(house.bitvector, house_flags));
    asking = mode::flags;
  }

  /* Puts the house into the house control list, and flags its rooms */
  void house_editor::save(GameWorld *g, struct descriptor_data *d)
  {
    if (int existing = find_house(g, number); existing != NOWHERE)
      g->house_control[existing] = house;
    else if (int added = g->num_of_houses++; added < MAX_HOUSES) {
      g->house_control[added]      = house;
      g->house_control[added].vnum = number;
    } else {
      send_to_char(g, d->character, "MAX House limit reached - Unable to save this house!");
      mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "HSEDIT: Max houses limit reached - Unable to save OLC data");
    }
    if (room_of(g, house.vnum))
      room_set_flag(g, house.vnum, ROOM_HOUSE, true);
    if (room_of(g, house.atrium))
      room_set_flag(g, house.atrium, ROOM_ATRIUM, true);
  }

  void house_editor::delete_house(GameWorld *g, struct descriptor_data *d)
  {
    int i = find_house(g, house.vnum);
    if (i == NOWHERE) {
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: hsedit: Invalid house vnum in hedit_delete_house\r\n");
      leave_olc(g, d);
      return;
    }
    if (room_vnum room = g->house_control[i].vnum; !room_of(g, room))
      mud_log("SYSERR: House {:d} had invalid vnum {:d}!", house.vnum, room);
    else
      room_set_flag(g, room, ROOM_BFS_MARK, false); /* quirk: clears ROOM_BFS_MARK, meant ROOM_HOUSE and ROOM_HOUSE_CRASH:
                                                         their numbers or-ed as bits, 11 | 12 is 15 */
    House_delete_file(g->house_control[i].vnum);
    std::copy(g->house_control + i + 1, g->house_control + g->num_of_houses, g->house_control + i);
    g->num_of_houses--;
    send_to_char(g, d->character, "House deleted.\r\n");
    House_save_control(g);
    leave_olc(g, d);
  }

  /* The guest list's menu and the prompts it leads to; false if the line is for the main menu's end */
  bool house_editor::parse_guests(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char answer = first_char(line);
    switch (asking) {
      case mode::guest_menu:
        switch (answer) {
          case 'a':
          case 'A':
            if (house.num_of_guests > MAX_GUESTS - 1)
              send_to_char(g, d->character, "Guest List Full! - delete some before adding more\r\nEnter selection (A/D/C/Q) : ");
            else {
              send_to_char(g, d->character, "Name of guest to add: ");
              asking = mode::guest_add;
            }
            break;
          case 'd':
          case 'D':
            if (house.num_of_guests < 1)
              send_to_char(g, d->character, "Guest List Empty! - add a guest before trying to delete one\r\nEnter selection (A/D/C/Q) : ");
            else {
              send_to_char(g, d->character, "Name of guest to delete : ");
              asking = mode::guest_delete;
            }
            break;
          case 'c':
          case 'C':
            send_to_char(g, d->character, "Clear guest list? (Y/N) : ");
            asking = mode::guest_clear;
            break;
          case 'q':
          case 'Q':
            menu(g, d);
            break;
          default:
            send_to_char(g, d->character, "Invalid choice!\r\n\r\n");
            guest_menu(g, d);
            break;
        }
        return true;

      case mode::guest_add:
      case mode::guest_delete: {
        long id = get_id_by_name(g, line);
        long *end = house.guests + house.num_of_guests;
        long *at  = std::find(house.guests, end, id);
        if (id < 0)
          send_to_char(g, d->character, "No such player.\r\n");
        else if (id == char_get_idnum(d->character))
          send_to_char(g, d->character, "House owner should not be in the guest list!\r\n");
        else if (asking == mode::guest_add && at != end)
          send_to_char(g, d->character, "That player is already in the guest list!.\r\n");
        else if (asking == mode::guest_add) {
          house.guests[house.num_of_guests++] = id;
          send_to_char(g, d->character, "Guest added.\r\n");
        } else if (at == end)
          send_to_char(g, d->character, "That player isn't in the guest list!\r\n");
        else {
          std::copy(at + 1, end, at);
          end[-1] = end == house.guests + MAX_GUESTS ? 0 : *end; /* what was after the list moves up, as it always did */
          house.num_of_guests--;
          send_to_char(g, d->character, "Guest deleted.\r\n");
          changed = true;
        }
        guest_menu(g, d);
        return true;
      }

      case mode::guest_clear:
        switch (answer) {
          case 'n':
          case 'N':
            send_to_char(g, d->character, "Invalid choice!"); /* quirk: that's no answer */
            return true;
          case 'y':
          case 'Y':
            house.num_of_guests = 0;
            std::fill(std::begin(house.guests), std::end(house.guests), 0);
            send_to_char(g, d->character, "Guest List Cleared!");
            changed = true;
            guest_menu(g, d);
            return false; /* quirk: and the main menu after it */
          default:
            send_to_char(g, d->character, "Invalid choice!\r\nClear Guest List? (Y/N) : ");
            return true;
        }

      default:
        return false;
    }
  }

  void house_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char answer = first_char(line);
    const int number_given = parse_int(line);

    mudlog(g, LOG_CMP, ADMLVL_BUILDER, false, "(LOG) hsedit_parse: OLC mode {:d}", (int)asking);
    switch (asking) {
      case mode::confirm_save:
        switch (answer) {
          case 'y':
          case 'Y':
            save(g, d);
            mudlog(g, LOG_CMP, ADMLVL_BUILDER, true, "OLC: {} edits house {:d}", char_get_name(d->character), number);
            if (CONFIG_OLC_SAVE) {
              House_save_control(g);
              write_to_output(g, d, "House saved to disk.\r\n");
            } else
              write_to_output(g, d, "House saved to memory.\r\n");
            leave_olc(g, d);
            return;
          case 'n':
          case 'N':
            leave_olc(g, d);
            return;
          default:
            send_to_char(g, d->character, "Invalid choice!\r\n");
            send_to_char(g, d->character, "Do you wish to save this house internally? : ");
            return;
        }

      case mode::main_menu:
        switch (answer) {
          case 'q':
          case 'Q':
            if (changed) {
              send_to_char(g, d->character, "Do you wish to save this house internally? : ");
              asking = mode::confirm_save;
            } else
              leave_olc(g, d);
            return;
          case '1':
            owner_menu(g, d);
            return;
          case '2':
          case '3':
            if (house.vnum == NOWHERE || !room_of(g, house.vnum)) {
              send_to_char(g, d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
              mudlog(g, LOG_NRM, answer == '2' ? ADMLVL_GRGOD : ADMLVL_BUILDER, true, "SYSERR: Invalid house VNUM in hsedit");
            } else if (answer == '2') {
              send_to_char(g, d->character, "Enter atrium room vnum:");
              asking = mode::atrium;
            } else
              dir_menu(g, d);
            return;
          case '4':
            type_menu(g, d);
            return;
          case '5':
            send_to_char(g, d->character, "Set build date to now? (Y/N):");
            asking = mode::build_date;
            return;
          case '6':
            send_to_char(g, d->character, "Set last payment as now? (Y/N) : ");
            asking = mode::payment;
            return;
          case '7':
            guest_menu(g, d);
            return;
          case '8':
            flags_menu(g, d);
            return;
          case 'x':
          case 'X':
            send_to_char(g, d->character, "Are you sure you want to delete this house? (Y/N) : ");
            asking = mode::confirm_delete;
            return;
          default:
            send_to_char(g, d->character, "Invalid choice!\r\n");
            menu(g, d);
            return;
        }

      case mode::owner_menu:
        switch (answer) {
          case '1':
            send_to_char(g, d->character, "Enter the name of the owner : ");
            asking = mode::owner_name;
            break;
          case '2':
            send_to_char(g, d->character, "Enter the user id of the owner : ");
            asking = mode::owner_id;
            break;
          case 'Q': /* quirk: not 'q' */
            menu(g, d);
            break;
        }
        return;

      case mode::owner_name:
      case mode::owner_id: {
        long id = asking == mode::owner_name ? get_id_by_name(g, line) : number_given;
        if (asking == mode::owner_name ? id < 0 : get_name_by_id(g, id) == nullptr) {
          send_to_char(g, d->character, "There is no such player.\r\n");
          owner_menu(g, d);
          return;
        }
        house.owner = id;
        break;
      }

      case mode::atrium: {
        if (number_given == 0) {
          menu(g, d);
          return;
        }
        if (!room_of(g, number_given)) {
          send_to_char(g, d->character, "Room VNUM does not exist.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
          return;
        }
        int exit_to = -1; /* the last direction leading there */
        for (int dir = 0; dir < 12; dir++)
          if (struct room_direction_data *exit = room_get_dir_option(g, house.vnum, dir); exit && exit->to_room == number_given)
            exit_to = dir;
        if (exit_to < 0) {
          send_to_char(g, d->character, "Atrium MUST be an adjoining room.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
          return;
        }
        house.atrium   = number_given;
        house.exit_num = exit_to;
        break;
      }

      case mode::dir_menu: {
        int dir = number_given - 1;
        if (answer == 'q' || answer == 'Q' || dir == -1) {
          menu(g, d);
          return;
        }
        if (dir < 0 || dir >= NUM_OF_DIRS) {
          send_to_char(g, d->character, "Invalid choice, Please select a direction (1-12, Q to quit) : ");
          return;
        }
        struct room_direction_data *exit = room_get_dir_option(g, house.vnum, dir);
        if (!exit || exit->to_room == NOWHERE) {
          send_to_char(g, d->character, !exit ? "You cannot set the atrium to a room that doesn't exist!\r\n" : "You cannot set the atrium to nowhere!\r\n");
          dir_menu(g, d);
          return;
        }
        house.exit_num = dir;
        house.atrium   = exit->to_room;
        break;
      }

      case mode::no_vnum: /* Just an 'enter' keypress - don't do anything */
        break;

      /* quirk: any answer but 'n' counts as a change, though only 'y' makes one */
      case mode::build_date:
      case mode::payment:
        if (answer == 'y' || answer == 'Y')
          (asking == mode::build_date ? house.built_on : house.last_payment) = game_now(g);
        else if (answer == 'n' || answer == 'N') {
          send_to_char(g, d->character, asking == mode::build_date ? "Build Date not changed\r\n" : "Last Payment Date not changed\r\n");
          menu(g, d);
          return;
        }
        break;

      case mode::confirm_delete:
        if (answer == 'y' || answer == 'Y') {
          delete_house(g, d);
          return;
        }
        if (answer == 'n' || answer == 'N') {
          send_to_char(g, d->character, "House not deleted!\r\n");
          menu(g, d);
          return;
        }
        break;

      case mode::guest_menu:
      case mode::guest_add:
      case mode::guest_delete:
      case mode::guest_clear:
        if (parse_guests(g, d, line))
          return;
        break;

      case mode::type:
        if (number_given < 0 || number_given >= NUM_HOUSE_TYPES) {
          send_to_char(g, d->character, "Invalid choice!");
          type_menu(g, d);
          return;
        }
        house.mode = number_given;
        break;

      case mode::flags:
        if (number_given < 0 || number_given > HOUSE_NUM_FLAGS) {
          send_to_char(g, d->character, "That's not a valid choice!\r\n");
          flags_menu(g, d);
          return;
        }
        if (number_given == 0)
          break;
        house.bitvector ^= 1L << (number_given - 1);
        flags_menu(g, d);
        return;
    }
    /*. If we get this far, something has been changed .*/
    changed = true;
    menu(g, d);
  }
} // namespace

ACMD(do_oasis_hsedit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);
  std::string buf2      = take_arg(rest);
  room_vnum number      = NOWHERE;
  bool save             = false;

  /* If there aren't any arguments...grab the number of the current room... */
  if (buf1.empty())
    number = char_get_in_room(ch);
  else if (!isdigit(buf1[0])) {
    if (!str_eq_nocase("save", buf1)) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = true;
    if ((number = zone_to_save(g, ch, buf2)) == NOWHERE)
      return;
  }
  if (number == NOWHERE)
    number = parse_int(buf1);

  /* Check that whatever it is isn't already being edited. */
  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_HSEDIT && dd->olc && session_of<house_editor>(dd).number == number) {
      send_to_char(g, ch, "That house is currently being edited by {}.\r\n", char_pers(g, dd->character, ch));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<house_editor>(), "do_oasis_hsedit");
  house_editor &editor = session_of<house_editor>(d);

  editor.zone = save ? find_zone(g, number) : find_zone_by_thing(g, number);
  if (!editor.zone) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    d->olc.reset();
    return;
  }

  /* Everyone but IMPLs can only edit zones they have been assigned. */
  if (!can_edit_zone(g, ch, editor.zone)) {
    send_to_char(g, ch, " You do not have permission to edit zone {:d}. Try zone {:d}.\r\n", editor.zone->number, char_get_olc_zone(g, ch));
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "OLC: {} tried to edit zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
           char_get_olc_zone(g, ch));
    d->olc.reset();
    return;
  }

  if (save) {
    send_to_char(g, ch, "Saving all houses in zone {:d}.\r\n", editor.zone->number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves house info for zone {:d}.", char_get_name(ch),
           editor.zone->number);
    House_save_control(g);
    d->olc.reset();
    return;
  }

  editor.number = number;
  if (int existing = find_house(g, number); existing != NOWHERE)
    editor.house = g->house_control[existing];
  else if (g->num_of_houses >= MAX_HOUSES) {
    /* quirk: the session is left behind, though the builder isn't editing */
    send_to_char(g, ch, "MAX houses limit reached ({:d}) - Unable to create more.\r\n", MAX_HOUSES);
    mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "HSEDIT: MAX houses limit reached ({:d})\r\n", MAX_HOUSES);
    return;
  } else
    editor.house = {.vnum = number, .exit_num = -1, .built_on = game_now(g), .mode = HOUSE_PRIVATE};
  editor.menu(g, d);
  begin_editing(g, d, CON_HSEDIT);
  mudlog(g, LOG_CMP, ADMLVL_BUILDER, true, "OLC: (hsedit) {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}
