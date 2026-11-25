/************************************************************************
 *  OasisOLC - Rooms / redit.c					v2.0	*
 *  Original author: Levork						*
 *  Copyright 1996 Harvey Gilpin					*
 *  Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <array>
#include <optional>
#include <algorithm>
#include <cctype>

#include "dbat/act.wizard.hpp"
#include "dbat/args.hpp"
#include "dbat/boards.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genwld.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/iterate.hpp"
#include "dbat/maputils.hpp"
#include "dbat/modify.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/olc/extra_desc.hpp"
#include "dbat/olc/script_editor.hpp"
#include "dbat/relocate.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/spells.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/olc/commands.hpp"
#include "dbat/olc/limits.hpp"

namespace {
  constexpr obj_vnum REMODEL_DROID = 19094; /* the R.A.D. a mortal remodels a room with */

  /* An exit of the main menu: the key choosing it, and how the menu names it */
  struct exit_choice {
    char key;
    int dir;
    std::string_view label;
  };

  /* In the menu's two columns: the first six on the left, the others beside them */
  constexpr auto EXITS = std::to_array<exit_choice>({
      {'5', NORTH,     "north"    },
      {'6', EAST,      "east"     },
      {'7', SOUTH,     "south"    },
      {'8', WEST,      "west"     },
      {'9', UP,        "up"       },
      {'a', DOWN,      "down"     },
      {'b', NORTHWEST, "northwest"},
      {'c', NORTHEAST, "northeast"},
      {'d', SOUTHEAST, "southeast"},
      {'e', SOUTHWEST, "southwest"},
      {'f', INDIR,     "in"       },
      {'g', OUTDIR,    "out"      },
  });

  /* A number of the exit menu's, any negative one meaning none (-1) */
  struct exit_field {
    char key;
    std::string_view prompt;
    int room_direction_data::*field;
    int context;
  };

  constexpr auto EXIT_FIELDS = std::to_array<exit_field>({
      {'4', "Enter key number : ",                                         &room_direction_data::key,           CONTEXT_REDIT_EXIT_KEY},
      {'7', "Enter lock DC number : ",                                     &room_direction_data::dclock,        NOTHING               },
      {'8', "Enter door search DC number : ",                              &room_direction_data::dchide,        NOTHING               },
      {'a', "Enter DC for skill required to pass through exit : ",         &room_direction_data::dcmove,        NOTHING               },
      {'b', "Enter the SAVE TYPE for failed skill checks : ",              &room_direction_data::failsavetype,  NOTHING               },
      {'c', "Enter the DC to beat for SAVE from failed skill checks : ",   &room_direction_data::dcfailsave,    NOTHING               },
      {'d', "Enter the room to send play to for minor save failure : ",    &room_direction_data::failroom,      NOTHING               },
      {'e', "Enter the room to send play to for major save failure : ",    &room_direction_data::totalfailroom, NOTHING               },
  });

  /* The main menu's choices only builders have: a mortal remodeling with a droid has the others */
  constexpr std::string_view BUILDERS_ONLY = "3456789abcdefgwxsz";

  void show_extra_desc(GameWorld *g, struct descriptor_data *d, const extra_descr_data &desc, bool has_next)
  {
    clear_screen(g, d);
    write_to_output(g, d, "@g1@n) Keyword: @y{}\r\n@g2@n) Description:\r\n@y{}\r\n@g3@n) Goto next description: ",
                    !desc.keyword.empty() ? desc.keyword : "<NONE>", !desc.description.empty() ? desc.description : "<NONE>");
    write_to_output(g, d, has_next ? "Set.\r\n" : "Not Set.\r\n");
    write_to_output(g, d, "Enter choice (0 to quit) : ");
  }

  constexpr extra_desc_editor::style EXTRA_DESCS = {
      .show               = show_extra_desc,
      .keyword_prompt     = "Enter keywords, separated by spaces : ",
      .description_prompt = "Enter extra description:\r\n\r\n",
      .says_unfinished    = true,
      .others_leave       = true,
      .contexts           = {CONTEXT_REDIT_EXTRADESC_MENU, CONTEXT_REDIT_EXTRADESC_KEY, CONTEXT_REDIT_EXTRADESC_DESCRIPTION},
  };

  bool is_mortal(struct descriptor_data *d) { return char_get_admlevel(d->character) < 1; }

  /* The room editor: a copy of one room, its exits, extra descriptions and triggers, put into the world on saving */
  struct room_editor final : olc_session {
    enum class mode {
      main_menu,
      confirm_save,
      name,
      description,
      flags,
      sector,
      exit_menu,
      exit_number,
      exit_description,
      exit_keyword,
      exit_flags,
      exit_number_field, /* one of EXIT_FIELDS */
      exit_skill,
      extra_descs,
      script,
      confirm_delete,
    };

    mode asking = mode::main_menu;
    room_vnum vnum{};
    struct zone_data *zone{};
    room_data room;
    bool changed{};
    int exit_dir{};                    /* the exit being edited */
    const exit_field *field{};         /* the number of it being asked for */
    extra_desc_editor extra_descs{&EXTRA_DESCS};
    script_editor script{WLD_TRIGGER};

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void text_done(GameWorld *g, struct descriptor_data *d, int how) override;
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone->number); }

    void menu(GameWorld *g, struct descriptor_data *d);
    void done(GameWorld *g, struct descriptor_data *d) /* something was changed: back to the main menu */
    {
      changed = true;
      menu(g, d);
    }
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void flag_menu(GameWorld *g, struct descriptor_data *d);
    void sector_menu(GameWorld *g, struct descriptor_data *d);

    room_direction_data &exit() { return *room.dir_option[exit_dir]; }
    void exit_menu(GameWorld *g, struct descriptor_data *d);
    void exit_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void exit_flag_menu(GameWorld *g, struct descriptor_data *d);
  };

  void room_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    if (is_mortal(d))
      write_to_output(g, d,
                      "-- Room number : [@c{:d}@n]    Room zone: [@c{:d}@n]\r\n@g1@n) Location Designation (Room Name)        : @y{}\r\n@g2@n) Remodeling Routine (Room Description)   :\r\n@y{}@gH@n) Extra descriptions menu\r\n@gQ@n) Quit\r\nEnter choice : ",
                      vnum, zone->number, room.name, room.description);
    else {
      std::string exits;
      for (size_t i = 0; i < std::size(EXITS) / 2; i++) {
        const exit_choice &left = EXITS[i], &right = EXITS[i + std::size(EXITS) / 2];
        auto to = [&](const exit_choice &e) { return room.dir_option[e.dir] ? room.dir_option[e.dir]->to_room : NOWHERE; };
        exits += str_format("@g{:c}@n) Exit {:<6} : [@c{:>6d}@n],  @g{:c}@n) Exit {:<9} : [@c{:>6d}@n]\r\n", (char)toupper(left.key), left.label,
                            to(left), (char)toupper(right.key), right.label, to(right));
      }
      write_to_output(g, d,
                      "-- Room number : [@c{:d}@n]  \tRoom zone: [@c{:d}@n]\r\n@g1@n) Name        : @y{}\r\n@g2@n) Description :\r\n@y{}@g3@n) Room flags  : @c{}\r\n@g4@n) Sector type : @c{}\r\n{}@gH@n) Extra descriptions menu\r\n@gW@n) Copy Room\r\n@gX@n) Delete Room\r\n@gS@n) Script      : @c{}\r\n@gZ@n) Wiznet      :\r\n@gQ@n) Quit\r\nEnter choice : ",
                      vnum, zone->number, room.name, room.description, flag_names(room.room_flags),
                      type_name(sector_types, room.sector_type), exits, !script.triggers.empty() ? "Set." : "Not Set.");
    }
    asking = mode::main_menu;
  }

  void room_editor::flag_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, room_bits, flag_count<room_flag>, 1, "", 2);
    write_to_output(g, d, "\r\nRoom flags: @c{}@n\r\nEnter room flags, 0 to quit : ", flag_names(room.room_flags));
    asking = mode::flags;
  }

  void room_editor::sector_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, sector_types, NUM_ROOM_SECTORS, 0, "", 3);
    write_to_output(g, d, "\r\nEnter sector type : ");
    asking = mode::sector;
  }

  /* The exit being edited: made, if there wasn't one */
  void room_editor::exit_menu(GameWorld *g, struct descriptor_data *d)
  {
    if (!room.dir_option[exit_dir])
      room.dir_option[exit_dir].make();
    const room_direction_data &e = exit();

    clear_screen(g, d);
    write_to_output(g, d,
                    "@g1@n) Exit to     \t\t: @c{:d}\r\n@g2@n) Description \t\t:-\r\n@y{}\r\n@g3@n) Door name   \t\t: @y{}\r\n@g4@n) Key         \t\t: @c{:d}\r\n@g5@n) Door flags  \t\t: @c{}\r\n@g6@n) Purge exit.\r\n@g7@n) DC Lock\t\t: @c{:d}\r\n@g8@n) DC Hide     \t\t: @c{:d}\r\n@g9@n) DC Skill\t\t: @c{}\r\n@gA@n) DC Move\t\t: @c{:d}\r\n@gB@n) Skill Fail Save Type\t\t: @c{:d}\r\n@gC@n) DC Skill Save\t\t: @c{:d}\r\n@gD@n) Minor Fail Dest. Room\t: @c{:d}\r\n@gE@n) Major Fail Dest. Room\t: @c{:d}@n\r\nEnter choice, 0 to quit : ",
                    e.to_room, !e.general_description.empty() ? e.general_description : "<NONE>", !e.keyword.empty() ? e.keyword : "<NONE>", e.key,
                    flag_mask_names(e.exit_info), e.dclock, e.dchide, e.dcskill != 0 ? g->spell_info[e.dcskill].name : "<NONE>", e.dcmove,
                    e.failsavetype, e.dcfailsave, e.failroom, e.totalfailroom);
    asking = mode::exit_menu;
  }

  void room_editor::exit_flag_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, exit_bits, flag_count<exit_flag>, 1, "", 2);
    write_to_output(g, d, "\r\nExit flags: @c{}@n\r\nEnter exit flags, 0 to quit : ", flag_mask_names(exit().exit_info));
    asking = mode::exit_flags;
  }

  void room_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const int number = parse_int(line);

    switch (asking) {
      case mode::confirm_save:
        save_answer(g, d, line);
        return;

      case mode::main_menu:
        main_menu_choice(g, d, line);
        return;

      case mode::script:
        if (!script.parse(g, d, line))
          done(g, d);
        return;

      case mode::extra_descs:
        if (!extra_descs.parse(g, d, line))
          done(g, d);
        return;

      case mode::name: {
        std::string name(line);
        smash_tilde(name);
        room.name = text_or_undefined(std::string_view(name).substr(0, MAX_ROOM_NAME));
        done(g, d);
        return;
      }

      case mode::description:
      case mode::exit_description: /* the text editor has the lines while these are asked */
        done(g, d);
        return;

      case mode::flags:
        if (number < 0 || std::cmp_greater(number, flag_count<room_flag>)) {
          write_to_output(g, d, "That is not a valid choice!\r\n");
          flag_menu(g, d);
        } else if (number == 0)
          done(g, d);
        else {
          if (std::optional<room_flag> flag = flag_from_index<room_flag>(number - 1))
            room_toggle_flag(&room, *flag);
          flag_menu(g, d);
        }
        return;

      case mode::sector:
        if (number < 0 || number >= NUM_ROOM_SECTORS) {
          write_to_output(g, d, "Invalid choice!");
          sector_menu(g, d);
          return;
        }
        room.sector_type = number;
        done(g, d);
        return;

      case mode::exit_menu:
        exit_menu_choice(g, d, line);
        return;

      case mode::exit_number:
        if (number != -1 && !room_of(g, number)) {
          write_to_output(g, d, "That room does not exist, try again : ");
          return;
        }
        exit().to_room = number;
        exit_menu(g, d);
        return;

      case mode::exit_keyword:
        exit().keyword = text_or_undefined(line);
        exit_menu(g, d);
        return;

      case mode::exit_flags:
        if (number < 0 || std::cmp_greater(number, flag_count<exit_flag>)) {
          write_to_output(g, d, "That's not a valid choice!\r\n");
          exit_flag_menu(g, d);
        } else if (number == 0)
          exit_menu(g, d);
        else {
          exit().exit_info.toggle(static_cast<exit_flag>(number - 1)); /* 1 to flag_count<exit_flag> */
          exit_flag_menu(g, d);
        }
        return;

      case mode::exit_number_field:
        exit().*field->field = number < 0 ? NOTHING : number;
        exit_menu(g, d);
        return;

      case mode::exit_skill: {
        std::string name(line);
        exit().dcskill = std::max(find_skill_num(g, name, skill_kind::skill), 0);
        exit_menu(g, d);
        return;
      }

      case mode::confirm_delete:
        if (const std::optional<bool> sure = yes_or_no(line); sure == true) {
          write_to_output(g, d, delete_room(g, room.number) ? "Room deleted.\r\n" : "Couldn't delete the room!\r\n");
          leave_olc(g, d);
        } else if (sure == false)
          menu(g, d);
        else {
          write_to_output(g, d, "Please answer 'Y' or 'N': ");
          done(g, d); /* quirk: and the main menu, as though something had changed */
        }
        return;
    }
  }

  void room_editor::save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const std::optional<bool> save = yes_or_no(line);
    if (!save) {
      write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes ? : ");
      return;
    }
    struct char_data *ch = d->character;
    if (!*save) {
      leave_olc(g, d);
      if (char_get_admlevel(ch) < 1) {
        write_to_output(g, d, "@GYou close the droids interface and put it back among the rest of your things.@n\r\n");
        act(g, "@g$n@G stops typing information into the droid in $s hands and closes it back up.@n\r\n", true, ch, nullptr, nullptr, act_to::room);
        obj_to_char(g, read_object(g, REMODEL_DROID), ch);
      }
      return;
    }

    redit_save_internally(g, room, vnum, zone, script.triggers);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} edits room {:d}.", char_get_name(ch), vnum);
    if (CONFIG_OLC_SAVE) {
      save_rooms(g, find_zone_by_thing(g, vnum));
      update_space(g);
      if (is_mortal(d)) {
        write_to_output(g, d, "@GThe remodeling droid quickly launches about with remodeling the room to your specifications. It seems to finish in no time at all...@n\r\n");
        act(g, "@GThe remodeling droid quickly launches about with remodeling the room as to @g$n's@G specifications. It seems to finish in no time at all...@n", true, ch, nullptr,
            nullptr, act_to::room);
      } else
        write_to_output(g, d, "Room saved to disk.\r\n");
    } else
      write_to_output(g, d, "Room saved to memory.\r\n");
    leave_olc(g, d);
  }

  void room_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = tolower(first_char(line));
    if (key && is_mortal(d) && BUILDERS_ONLY.contains(key)) {
      write_to_output(g, d, "That options isn't available to non-builders.\r\n");
      return;
    }
    if (auto e = std::ranges::find(EXITS, key, &exit_choice::key); key && e != std::end(EXITS)) {
      exit_dir = e->dir;
      exit_menu(g, d);
      return;
    }

    switch (key) {
      case 'q':
        if (changed) {
          write_to_output(g, d, "Do you wish to save your changes? : ");
          asking = mode::confirm_save;
        } else
          leave_olc(g, d);
        return;
      case '1':
        write_to_output(g, d, is_mortal(d) ? "Enter Location Name:\r\n" : "Enter Room Name:-\r\n] ");
        asking = mode::name;
        return;
      case '2':
        asking = mode::description;
        clear_screen(g, d);
        send_editor_help(g, d);
        write_to_output(g, d, is_mortal(d) ? "Set Remodel Parameters (Enter Room Description):-\r\n] " : "Enter room description:\r\n\r\n");
        if (!room.description.empty())
          write_to_output(g, d, room.description);
        string_write(d, &room.description, MAX_ROOM_DESC, 0);
        changed = true;
        return;
      case '3':
        flag_menu(g, d);
        return;
      case '4':
        sector_menu(g, d);
        return;
      case 'h':
        asking = mode::extra_descs;
        extra_descs.open(g, d, room.ex_description);
        return;
      case 'w':
        write_to_output(g, d, "Disabled for the time being!");
        return;
      case 'x':
        write_to_output(g, d, "Are you sure you want to delete this room? ");
        asking = mode::confirm_delete;
        return;
      case 's':
        asking = mode::script;
        script.menu(g, d);
        return;
      case 'z': {
        std::string message(line);
        search_replace(message, "z ", ""); /* quirk: every "z " in it, and not "Z " */
        do_wiznet(g, d->character, message, 0, 0);
        return;
      }
      default:
        write_to_output(g, d, "Invalid choice!");
        menu(g, d);
        return;
    }
  }

  void room_editor::exit_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = tolower(first_char(line));
    if (auto f = std::ranges::find(EXIT_FIELDS, key, &exit_field::key); key && f != std::end(EXIT_FIELDS)) {
      field  = f;
      asking = mode::exit_number_field;
      write_to_output(g, d, f->prompt);
      return;
    }

    switch (key) {
      case '0':
        done(g, d);
        return;
      case '1':
        asking = mode::exit_number;
        write_to_output(g, d, "Exit to room number : ");
        return;
      case '2':
        asking = mode::exit_description;
        send_editor_help(g, d);
        write_to_output(g, d, "Enter exit description:\r\n\r\n");
        if (!exit().general_description.empty())
          write_to_output(g, d, exit().general_description);
        string_write(d, &exit().general_description, MAX_EXIT_DESC, 0);
        return;
      case '3':
        asking = mode::exit_keyword;
        write_to_output(g, d, "Enter keywords : ");
        return;
      case '5':
        exit_flag_menu(g, d);
        return;
      case '6':
        room.dir_option[exit_dir] = {};
        done(g, d);
        return;
      case '9':
        asking = mode::exit_skill;
        write_to_output(g, d, "Enter skill to be checked to pass through exit : ");
        return;
      default:
        write_to_output(g, d, "Try again : ");
        return;
    }
  }

  void room_editor::text_done(GameWorld *g, struct descriptor_data *d, int)
  {
    switch (asking) {
      case mode::description: menu(g, d); break;
      case mode::exit_description: exit_menu(g, d); break;
      case mode::extra_descs: extra_descs.text_done(g, d); break;
      default: break;
    }
  }

  int room_editor::help_context(const struct descriptor_data *) const
  {
    switch (asking) {
      case mode::main_menu: return CONTEXT_REDIT_MAIN_MENU;
      case mode::confirm_save: return CONTEXT_REDIT_CONFIRM_SAVESTRING;
      case mode::name: return CONTEXT_REDIT_NAME;
      case mode::description: return CONTEXT_REDIT_DESC;
      case mode::flags: return CONTEXT_REDIT_FLAGS;
      case mode::sector: return CONTEXT_REDIT_SECTOR;
      case mode::exit_menu: return CONTEXT_REDIT_EXIT_MENU;
      case mode::exit_number: return CONTEXT_REDIT_EXIT_NUMBER;
      case mode::exit_description: return CONTEXT_REDIT_EXIT_DESCRIPTION;
      case mode::exit_keyword: return CONTEXT_REDIT_EXIT_KEYWORD;
      case mode::exit_flags: return CONTEXT_REDIT_EXIT_DOORFLAGS;
      case mode::exit_number_field: return field->context;
      case mode::extra_descs: return extra_descs.help_context();
      case mode::script: return script.help_context();
      case mode::exit_skill:
      case mode::confirm_delete: return NOTHING;
    }
    return NOTHING;
  }
} // namespace

void redit_save_internally(GameWorld *g, struct room_data &room, room_vnum vnum, struct zone_data *zone, const std::vector<trig_vnum> &triggers)
{
  room.number = vnum;
  room.zone   = zone;
  add_room(g, &room);
  room_set_proto_script(g, vnum, triggers);
  assign_triggers(g, room_of(g, vnum));
}

ACMD(do_oasis_redit)
{
  const bool builder = char_get_admlevel(ch) > 0;
  std::string_view rest = argument;
  std::string buf1, buf2;
  if (builder) {
    buf1 = take_arg(rest);
    buf2 = take_arg(rest);
  }

  if (!builder && !room_flagged(g, char_get_in_room(ch), room_flag::canremodel)) {
    send_to_char(g, ch, "You can not remodel this room.\r\n");
    return;
  }

  /* A mortal remodels with a droid, which is used up */
  struct obj_data *droid = nullptr;
  for (obj_data *obj : char_inventory(g, ch))
    if (obj_get_vnum(g, obj) == REMODEL_DROID) {
      droid = obj;
      break;
    }
  if (!builder) {
    if (!droid) {
      send_to_char(g, ch, "You do not have a R.A.D. Remodeling Assistance Droid.\r\n");
      return;
    }
    act(g, "@GYou open up the computer panel on the droid and begin to program its remodeling routine.@n", true, ch, nullptr, nullptr, act_to::actor);
    act(g, "@g$n@G opens up a computer panel on some kind of small spherical droid and begins to program it.@n", true, ch, nullptr, nullptr, act_to::room);
    extract_obj(g, droid);
  }

  IDXTYPE number = NOWHERE;
  bool save      = false;
  if (buf1.empty() || !builder)
    number = char_get_in_room(ch);
  else if (!isdigit(buf1[0])) {
    if (!str_eq_nocase("save", buf1)) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = true;
    if ((number = zone_to_save(g, ch, buf2, 0)) == NOWHERE)
      return;
  }
  if (number == NOWHERE)
    number = parse_int(buf1);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_REDIT && dd->olc && session_of<room_editor>(dd).vnum == number) {
      send_to_char(g, ch, "That room is currently being edited by {}.\r\n", char_pers(g, dd->character, ch));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<room_editor>(), "do_oasis_redit");
  room_editor &editor = session_of<room_editor>(d);

  editor.zone = save ? find_zone(g, number) : find_zone_by_thing(g, number);
  if (editor.zone == nullptr) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    d->olc.reset();
    return;
  }
  if (builder && !can_edit_zone(g, ch, editor.zone)) {
    send_cannot_edit(g, ch, editor.zone->number);
    d->olc.reset();
    return;
  }

  if (save) {
    save_zone_files(g, ch, editor.zone, "rooms", "room info", save_rooms);
    d->olc.reset();
    return;
  }

  editor.vnum = number;
  if (struct room_data *existing = room_of(g, number)) {
    editor.room = *existing;
    /* quirk: a room without a name or description gets "undefined" */
    editor.room.name        = text_or_undefined(editor.room.name);
    editor.room.description = text_or_undefined(editor.room.description);
    editor.script.triggers  = editor.room.proto_script;
    editor.room.proto_script.clear();
    editor.room.script = nullptr; /* the room's own, which stays with it */
  } else {
    editor.room.name        = "An unfinished room";
    editor.room.description = "You are in an unfinished room.\r\n";
    editor.room.number      = NOWHERE;
  }

  editor.menu(g, d);
  begin_editing(g, d, CON_REDIT);
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}
