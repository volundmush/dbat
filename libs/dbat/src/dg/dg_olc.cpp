/**************************************************************************
*  File: dg_olc.c                                                         *
*                                                                         *
*  Usage: this source file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
*  Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_  *
*                                                                         *
*  $Author: Chris Jacobsen/Mark A. Heilpern/egreen/Welcor $               *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cctype>
#include <strings.h>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/modify.hpp"
#include "dbat/flags.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/genzon.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

namespace {
  /* The trigger editor: a copy of one trigger, with its script as text, compiled and put into the world on saving */
  struct trigger_editor final : olc_session {
    enum class mode { main_menu, confirm_save, name, intended, types, commands, narg, argument, copy };

    mode asking = mode::main_menu;
    trig_vnum vnum{};
    struct zone_data *zone{};
    trig_data trig;
    std::string script; /* its commands, as the text editor edits them */
    bool changed{};

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void text_done(GameWorld *g, struct descriptor_data *d, int) override
    {
      if (asking == mode::commands)
        menu(g, d);
    }
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone->number); }

    void edit_new();
    void edit_copy_of(const struct index_data *index);
    void menu(GameWorld *g, struct descriptor_data *d);
    void types_menu(GameWorld *g, struct descriptor_data *d);
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save(GameWorld *g, struct descriptor_data *d);
  };

  void trigger_editor::edit_new()
  {
    trig.name         = "new trigger";
    trig.trigger_type = MTRIG_GREET;
    trig.narg         = 100;
    script            = "%echo% This trigger commandlist is not complete!\r\n";
    changed           = false;
  }

  /* quirk: copying another trigger copies its index too, which saving then gives this trigger's prototype */
  void trigger_editor::edit_copy_of(const struct index_data *index)
  {
    trig_data_copy(&trig, index->proto);
    script  = trig.program->script(); /* read again when the trigger is saved */
    changed = false;
  }

  /* The trigger types of what it is meant for */
  const char *const *types_for(int attach_type)
  {
    switch (attach_type) {
      case WLD_TRIGGER: return wtrig_types;
      case OBJ_TRIGGER: return otrig_types;
      default: return trig_types;
    }
  }

  void trigger_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    const char *attach = trig.attach_type == OBJ_TRIGGER ? "Objects" : trig.attach_type == WLD_TRIGGER ? "Rooms" : "Mobiles";

    clear_screen(g, d);
    write_to_output(g, d,
                    "Trigger Editor [@c{:d}@n]\r\n\r\n@g1@n) Name         : @y{}\r\n@g2@n) Intended for : @y{}\r\n@g3@n) Trigger types: @y{}\r\n@g4@n) Numeric Arg  : @y{:d}\r\n@g5@n) Arguments    : @y{}\r\n@g6@n) Commands:\r\n@c{}\r\n@gW@n) Copy Trigger\r\n@gZ@n) Wiznet\r\n@gQ@n) Quit\r\nEnter Choice :",
                    vnum, trig.name, attach, sprintbit(trig.trigger_type, types_for(trig.attach_type)), trig.narg, trig.arglist, script);
    asking = mode::main_menu;
  }

  void trigger_editor::types_menu(GameWorld *g, struct descriptor_data *d)
  {
    const char *const *types = types_for(trig.attach_type);

    clear_screen(g, d);
    for (int i = 0; i < NUM_TRIG_TYPE_FLAGS; i++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}  {}", i + 1, types[i], (i + 1) % 2 ? "" : "\r\n");
    write_to_output(g, d, "\r\nCurrent types : @c{}@n\r\nEnter type (0 to quit) : ", sprintbit(trig.trigger_type, types));
    asking = mode::types;
  }

  void trigger_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    switch (tolower(first_char(line))) {
      case 'q':
        if (!changed) {
          leave_olc(g, d);
          return;
        }
        if (!trig.trigger_type)
          write_to_output(g, d, "Invalid Trigger Type! Answer a to abort quit!\r\n");
        write_to_output(g, d, "Do you wish to save your changes? : ");
        asking = mode::confirm_save;
        return;
      case '1':
        asking = mode::name;
        write_to_output(g, d, "Name: ");
        return;
      case '2':
        asking = mode::intended;
        write_to_output(g, d, "0: Mobiles, 1: Objects, 2: Rooms: ");
        return;
      case '3':
        types_menu(g, d);
        return;
      case '4':
        asking = mode::narg;
        write_to_output(g, d, "Numeric argument: ");
        return;
      case '5':
        asking = mode::argument;
        write_to_output(g, d, "Argument: ");
        return;
      case '6':
        asking = mode::commands;
        write_to_output(g, d, "Enter trigger commands: (/s saves /h for help)\r\n\r\n");
        write_to_output(g, d, script);
        string_edit_begin(d, &script, MAX_CMD_LENGTH);
        d->mail_to = 0;
        changed    = true;
        return;
      case 'w':
        write_to_output(g, d, "Copy what trigger? ");
        asking = mode::copy;
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

  void trigger_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    switch (asking) {
      case mode::main_menu:
        main_menu_choice(g, d, line);
        return;

      case mode::confirm_save:
        switch (tolower(first_char(line))) {
          case 'y':
            save(g, d);
            mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} edits trigger {:d}",
                   char_get_name(d->character), vnum);
            leave_olc(g, d);
            return;
          case 'n':
            leave_olc(g, d);
            return;
          case 'a': /* abort quitting */
            break;
          default:
            write_to_output(g, d, "Invalid choice!\r\n");
            write_to_output(g, d, "Do you wish to save the trigger? : ");
            return;
        }
        break;

      case mode::name: {
        std::string name(line);
        smash_tilde(name);
        trig.name = text_or_undefined(name);
        changed   = true;
        break;
      }

      case mode::intended: /* quirk: any number is taken */
        trig.attach_type = parse_int(line);
        changed          = true;
        break;

      case mode::narg:
        trig.narg = LIMIT(parse_int(line), 0, 100);
        changed   = true;
        break;

      case mode::argument:
        trig.arglist = line;
        smash_tilde(trig.arglist);
        changed = true;
        break;

      case mode::types:
        if (const int i = parse_int(line); i == 0)
          break;
        else if (i > 0 && i <= NUM_TRIG_TYPE_FLAGS)
          TOGGLE_BIT(trig.trigger_type, 1U << (i - 1));
        changed = true; /* quirk: a number out of range too */
        types_menu(g, d);
        return;

      case mode::commands: /* the text editor has the lines */
        break;

      case mode::copy:
        if (struct index_data *index = find_trigger(g, parse_int(line)))
          edit_copy_of(index);
        else
          write_to_output(g, d, "That trigger does not exist.\r\n");
        break;
    }
    menu(g, d);
  }

  /*
   * Puts the trigger into the index, or the prototype there, and every live copy of it, takes the edited trigger.
   * The zone's triggers are written to disk at once, not left for the builder to save: a trigger lost after being
   * given to a mobile or object makes SYSERRs at the next boot.
   */
  void trigger_editor::save(GameWorld *g, struct descriptor_data *d)
  {
    trig.program = dg::lex(script, "* No script");

    if (struct index_data *index = find_trigger(g, vnum)) {
      trig_data *proto = index->proto;
      trig_data_copy(proto, &trig);

      /* Every live copy takes the new script for its next run. One that is running or waiting finishes on the script
         it started, which its run holds (see dg::run_state), with its variables; tstat shows which those are. */
      g->triggers.visit([&](long, struct trig_data *live) {
        if (live->index == index) {
          live->arglist      = proto->arglist;
          live->name         = proto->name;
          live->program      = proto->program;
          live->trigger_type = proto->trigger_type;
          live->attach_type  = proto->attach_type;
          live->narg         = proto->narg;
          live->data_type    = proto->data_type;
        }
      });
    } else {
      struct index_data *added = g->trig_index.emplace(vnum);
      trig.index               = added;
      added->vnum              = vnum;
      added->proto             = new trig_data();
      trig_data_copy(added->proto, &trig);
    }

    report_compile_errors(g, find_trigger(g, vnum)->proto, d);
    write_to_output(g, d, save_triggers(g, zone) ? "Trigger saved to disk.\r\n" : "Could not write the trigger file; nothing was changed.\r\n");
  }

  int trigger_editor::help_context(const struct descriptor_data *) const
  {
    switch (asking) {
      case mode::main_menu: return CONTEXT_TRIGEDIT_MAIN_MENU;
      case mode::confirm_save: return CONTEXT_TRIGEDIT_CONFIRM_SAVESTRING;
      case mode::name: return CONTEXT_TRIGEDIT_NAME;
      case mode::intended: return CONTEXT_TRIGEDIT_INTENDED;
      case mode::types: return CONTEXT_TRIGEDIT_TYPES;
      case mode::commands: return CONTEXT_TRIGEDIT_COMMANDS;
      case mode::narg: return CONTEXT_TRIGEDIT_NARG;
      case mode::argument: return CONTEXT_TRIGEDIT_ARGUMENT;
      case mode::copy: return NOTHING;
    }
    return NOTHING;
  }

  /* The start of a line of script, in either case */
  bool begins(std::string_view line, std::string_view word)
  {
    return line.size() >= word.size() && strncasecmp(line.data(), word.data(), word.size()) == 0;
  }
} // namespace

ACMD(do_oasis_trigedit)
{
  argument = skip_spaces(argument);
  if (argument.empty() || !isdigit(argument[0])) {
    send_to_char(g, ch, "Specify a trigger VNUM to edit.\r\n");
    return;
  }
  const trig_vnum number = parse_int(argument);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_TRIGEDIT && dd->olc && session_of<trigger_editor>(dd).vnum == number) {
      send_to_char(g, ch, "That trigger is currently being edited by {}.\r\n", char_get_name(dd->character));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<trigger_editor>(), "do_oasis_trigedit");
  trigger_editor &editor = session_of<trigger_editor>(d);

  if (!(editor.zone = zone_to_edit(g, ch, number, false))) {
    d->olc.reset();
    return;
  }

  editor.vnum = number;
  if (struct index_data *index = find_trigger(g, number))
    editor.edit_copy_of(index);
  else
    editor.edit_new();

  editor.menu(g, d);
  begin_editing(g, d, CON_TRIGEDIT);
  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} [trigger](allowed zone {:d})", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}

/* Indents the script in the text editor by its blocks: false, leaving it as it was, for one that doesn't nest */
bool format_script(GameWorld *g, struct descriptor_data *d)
{
  if (d->edit_text.empty())
    return false;

  const std::string_view text = d->edit_text;
  std::string formatted;
  int indent = 0, line_num = 0;
  bool indent_next = false, found_case = false;

  /* the lines that aren't empty, as strtok() found them */
  for (size_t pos = 0; (pos = text.find_first_not_of("\r\n", pos)) != std::string_view::npos;) {
    const size_t end = text.find_first_of("\r\n", pos);
    const std::string_view t = skip_spaces(text.substr(pos, end - pos));
    pos = end;
    line_num++;

    if (begins(t, "if ") || begins(t, "switch ")) {
      indent_next = true;
    } else if (begins(t, "while ")) {
      found_case  = true; /* so you can 'break' a loop without complains */
      indent_next = true;
    } else if (begins(t, "end") || begins(t, "done")) {
      if (!indent) {
        write_to_output(g, d, "Unmatched 'end' or 'done' (line {:d})!\r\n", line_num);
        return false;
      }
      indent--;
      indent_next = false;
    } else if (begins(t, "else")) {
      if (!indent) {
        write_to_output(g, d, "Unmatched 'else' (line {:d})!\r\n", line_num);
        return false;
      }
      indent--;
      indent_next = true;
    } else if (begins(t, "case") || begins(t, "default")) {
      if (!indent) {
        write_to_output(g, d, "Case/default outside switch (line {:d})!\r\n", line_num);
        return false;
      }
      if (!found_case) /* so we don't indent multiple case statements without a break */
        indent_next = true;
      found_case = true;
    } else if (begins(t, "break")) {
      if (!found_case || !indent) {
        write_to_output(g, d, "Break not in case (line {:d})!\r\n", line_num);
        return false;
      }
      found_case = false;
      indent--;
    }

    std::string line = std::string(2 * indent, ' ') + std::string(t) + "\r\n";
    if (formatted.size() + line.size() > d->max_str - 1) {
      write_to_output(g, d, "String too long, formatting aborted\r\n");
      return false;
    }
    formatted += line;

    if (indent_next) {
      indent++;
      indent_next = false;
    }
  }

  if (indent)
    write_to_output(g, d, "Unmatched if, while or switch ignored.\r\n");

  d->edit_text = formatted;
  return true;
}
