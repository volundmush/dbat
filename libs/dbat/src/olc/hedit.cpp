/**************************************************************************
*  File: hedit.c                                        Part of CircleMUD *
*  Usage: Oasis OLC Help Editor.                                          *
* Author: Steve Wolfe, Scott Meisenholder, Rhade                          *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 2007 by Rhade                                            *
*  InfoTechMUD is based on CircleMUD, Copyright (C) 1993, 1994.           *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/
#include "dbat/act.informative.hpp"
#include "dbat/act.social.hpp"
#include "dbat/args.hpp"
#include "dbat/boards.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/handler.hpp"
#include "dbat/hedit.hpp"
#include "dbat/help.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/text_files.hpp"
#include "dbat/utils.hpp"

namespace {
  /*
   * The help editor: one help entry, found by a keyword or new. Only one builder edits help at a time, from opening
   * the editor to leaving it however they leave.
   */
  struct help_editor final : olc_session {
    enum class mode { confirm_save, confirm_edit, confirm_add, main_menu, entry, keywords, min_level };

    GameWorld *g;
    mode asking = mode::confirm_edit;
    std::string keyword;          /* what the builder asked to edit */
    int index = NOWHERE;          /* the entry in g->help_table being edited, or NOWHERE for a new one */
    help_index_element help{};    /* the entry as it's being edited */
    bool changed = false;

    help_editor(GameWorld *world, std::string_view asked) : g(world), keyword(asked) { g->HEDITS = true; }
    ~help_editor() override { g->HEDITS = false; }

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void text_done(GameWorld *g, struct descriptor_data *d, int action) override;
    void log_leaving(GameWorld *, struct descriptor_data *d) const override
    {
      mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} stops editing help files.", char_get_name(d->character));
    }

    void menu(struct descriptor_data *d);
    void start(struct descriptor_data *d, help_index_element from);
    void ask_add(struct descriptor_data *d) { write_to_output(g, d, "Do you wish to add the '{}' help file? ", keyword); }
    void ask_edit(struct descriptor_data *d) { write_to_output(g, d, "Do you wish to edit the '{}' help file? ", g->help_table[index].keywords); }
    void next_match(struct descriptor_data *d);
    void save(struct descriptor_data *d);
    bool set_keywords(struct descriptor_data *d, std::string_view line);
  };

  void save_to_disk(GameWorld *g)
  {
    if (!save_help(g))
      return; /* still listed as unsaved */

    remove_from_save_list(g, HEDIT_PERMISSION, SL_HLP);

    /* Reboot the help files. */
    free_help_table(g);
    index_boot(g, DB_BOOT_HLP);
  }

  void help_editor::menu(struct descriptor_data *d)
  {
    write_to_output(g, d,
                    "@n-- Help file editor\r\n@g1@n) Keywords    : @y{}\n@g2@n) Entry       :\r\n@y{}@g3@n) Min Level   : @y{:d}\r\n@gQ@n) Quit\r\nEnter choice : ",
                    !help.keywords.empty() ? help.keywords.c_str() : "<None>", !help.entry.empty() ? help.entry.c_str() : "<None>\r\n",
                    help.min_level);
    asking = mode::main_menu;
  }

  /* Editing begins from this copy of the entry */
  void help_editor::start(struct descriptor_data *d, help_index_element from)
  {
    help    = std::move(from);
    changed = false;
    menu(d);
  }

  /* The next entry the keyword abbreviates, if any, to offer instead; otherwise adding one */
  void help_editor::next_match(struct descriptor_data *d)
  {
    /* quirk: only the entry straight after is looked at: any other ends the search */
    index++;
    if (index < std::ssize(g->help_table) && !is_abbrev(keyword.c_str(), g->help_table[index].keywords.c_str()))
      index = g->help_table.size();

    if (index >= std::ssize(g->help_table)) {
      index = NOWHERE; /* a new entry */
      ask_add(d);
      asking = mode::confirm_add;
    } else {
      ask_edit(d);
      asking = mode::confirm_edit;
    }
  }

  /* Puts the entry into the help table and writes the table out */
  void help_editor::save(struct descriptor_data *d)
  {
    write_to_output(g, d, "Help saved to disk.\r\n");
    send_to_imm(g, "@gHedit@D: @w{}@G has just edited and saved, @Y{}@G.@n", char_get_alias(d->character), help.keywords);
    if (index == NOWHERE)
      g->help_table.push_back(help);
    else
      g->help_table[index] = help;
    add_to_save_list(g, HEDIT_PERMISSION, SL_HLP);
    save_to_disk(g);
  }

  /* Takes the keywords if they'll do, putting them at the top of the entry; otherwise says why and asks again */
  bool help_editor::set_keywords(struct descriptor_data *d, std::string_view line)
  {
    std::string text(line.size() > MAX_HELP_KEYWORDS ? line.substr(0, MAX_HELP_KEYWORDS - 1) : line);
    strip_cr(text);
    help.keywords = text.empty() ? "undefined" : text;

    std::string_view refusal;
    if (help.keywords.contains("undefined"))
      refusal = "@RYou must at least enter SOME keywords.@n\n";
    else if (help.keywords.contains("<<X<<") || help.keywords.contains("<<x<<"))
      refusal = "@RLet's not joke around with help files now.@n\n";
    if (refusal.empty()) {
      help.entry = help.keywords + "\r\n----------\r\n\r\n" + help.entry;
      return true;
    }
    clear_screen(g, d);
    write_to_output(g, d, "{}", refusal);
    write_to_output(g, d, "Keywords: ");
    return false;
  }

  void help_editor::parse(GameWorld *, struct descriptor_data *d, std::string_view line)
  {
    const char answer = first_char(line);

    switch (asking) {
      case mode::confirm_save:
        switch (answer) {
          case 'y':
          case 'Y':
            if (help.keywords.empty() || help.keywords.contains("undefined") || help.keywords.contains("<<X<<")) {
              menu(d);
              write_to_output(g, d, "\n@RYou must fill in the keywords before you save.@n\r\n");
              return;
            }
            save(d);
            leave_olc(g, d);
            return;
          case 'n':
          case 'N':
            leave_olc(g, d);
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes? : \r\n");
            return;
        }

      case mode::confirm_edit:
        switch (answer) {
          case 'y':
          case 'Y': {
            help_index_element entry = g->help_table[index];
            if (entry.keywords.empty())
              entry.keywords = "undefined";
            if (entry.entry.empty())
              entry.entry = "undefined";
            start(d, std::move(entry));
            return;
          }
          case 'q':
          case 'Q':
            leave_olc(g, d);
            return;
          case 'n':
          case 'N':
            next_match(d);
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\n");
            ask_edit(d);
            return;
        }

      case mode::confirm_add:
        switch (answer) {
          case 'y':
          case 'Y':
            start(d, {.keywords = "<<X<< Put helpfile keywords here in caps", .entry = "\r\nThis help file is unfinished.\r\n"});
            return;
          case 'n':
          case 'N':
          case 'q':
          case 'Q':
            leave_olc(g, d);
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\n");
            ask_add(d);
            return;
        }

      case mode::main_menu:
        switch (answer) {
          case 'q':
          case 'Q':
            if (changed) {
              write_to_output(g, d, "Do you wish to save your changes? : ");
              asking = mode::confirm_save;
            } else {
              write_to_output(g, d, "No changes made.\r\n");
              leave_olc(g, d);
            }
            return;
          case '1':
            asking = mode::keywords;
            clear_screen(g, d);
            write_to_output(g, d, "Enter help file keywords: ");
            return;
          case '2':
            asking = mode::entry;
            clear_screen(g, d);
            send_editor_help(g, d);
            write_to_output(g, d, "Enter help entry: (/s saves /h for help)\r\n");
            if (!help.entry.empty())
              write_to_output(g, d, help.entry);
            string_write(d, &help.entry, MAX_MESSAGE_LENGTH, 0);
            changed = true;
            return;
          case '3':
            write_to_output(g, d, "Enter min level : ");
            asking = mode::min_level;
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\n");
            menu(d);
            return;
        }

      case mode::keywords:
        if (!set_keywords(d, line))
          return;
        break;

      case mode::min_level:
        if (int number = parse_int(line); number >= 0 && number <= ADMLVL_IMPL)
          help.min_level = number;
        else {
          write_to_output(g, d, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
          return;
        }
        break;

      case mode::entry: /* the text editor has the line */
        break;
    }

    /* If we get this far, something has been changed. */
    changed = true;
    menu(d);
  }

  void help_editor::text_done(GameWorld *, struct descriptor_data *d, int)
  {
    if (asking == mode::entry)
      menu(d);
  }
} // namespace

ACMD(do_oasis_hedit)
{
  struct descriptor_data *d = char_get_desc(ch);

  /* No building as a mob or while being forced. */
  if (char_is_npc(ch) || !d || desc_get_connected(d) != CON_PLAYING)
    return;

  if (g->HEDITS) {
    send_to_char(g, ch, "Sorry, only one person can edit help files at a time.\r\n");
    return;
  }

  /* quirk: no one is both, so no one is refused */
  if (char_get_admlevel(ch) < 4 && str_eq_nocase("Tepsih", char_get_name(ch)) && str_eq_nocase("Rogoshen", char_get_name(ch))) {
    send_to_char(g, ch, "Sorry you are incapable of editing help files at this time.\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  if (str_eq_nocase("save", arg)) {
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves help files.", char_get_name(ch));
    save_to_disk(g);
    send_to_char(g, ch, "Saving help files.\r\n");
    return;
  }

  open_session(g, d, std::make_unique<help_editor>(g, arg), "do_oasis");
  help_editor &editor = session_of<help_editor>(d);
  editor.index        = search_help(g, arg.c_str(), ADMLVL_IMPL);
  if (editor.index == NOWHERE) {
    editor.ask_add(d);
    editor.asking = help_editor::mode::confirm_add;
  } else {
    editor.ask_edit(d);
    editor.asking = help_editor::mode::confirm_edit;
  }
  begin_editing(g, d, CON_HEDIT);
  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} starts editing help files.", char_get_name(ch));
}

ACMD(do_helpcheck)
{
  std::string buf;
  int count = 0;

  send_to_char(g, ch, "Commands without help entries:\r\n");

  for (int i = 1; g->complete_cmd_info[i].command != "\n"; i++) {
    if (g->complete_cmd_info[i].command_pointer != do_action && g->complete_cmd_info[i].minimum_level >= 0 &&
        search_help(g, g->complete_cmd_info[i].command, ADMLVL_IMPL) == NOWHERE)
      buf += str_format("{:<20.20}{}", g->complete_cmd_info[i].command, (++count % 3 ? "" : "\r\n"));
  }
  if (count % 3)
    buf += "\r\n";

  if (char_get_desc(ch))
    send_to_char(g, ch, buf);
}

ACMD(do_hindex)
{
  std::string text(skip_spaces(argument));

  if (text.empty()) {
    send_to_char(g, ch, "Usage: hindex <string>\r\n");
    if (!g->help_table.empty() && char_get_admlevel(ch) > 0)
      send_to_char(g, ch, "\r\n@D[@Y{:d}@y Help files in index.@D]@n\r\n", (int)g->help_table.size());
    return;
  }

  int count       = 0;
  std::string buf = str_format("Help index entries based on '{}':\r\n", text);
  for (const help_index_element &entry : g->help_table)
    if (is_abbrev(text, entry.keywords.c_str()) && char_get_admlevel(ch) >= entry.min_level)
      buf += str_format("{:<20.20}{}", entry.keywords, (++count % 3 ? "" : "\r\n"));
  if (count % 3)
    buf += "\r\n";

  if (!count)
    buf += "  None.\r\n";

  if (count > 0 && char_get_admlevel(ch) > 0)
    buf += str_format("  {:d} Help files in index.\r\n", count);
  send_to_char(g, ch, buf);
}
