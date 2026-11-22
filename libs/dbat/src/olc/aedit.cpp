/************************************************************************
 * OasisOLC - Socials / aedit.c                                         *
 *                                                                      *
 * Copyright 1996 Harvey Gilpin                                         *
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)             *
 ************************************************************************/
#include <algorithm>
#include <cstdlib>
#include <string_view>

#include "dbat/aedit.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.social.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/social.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/text_files.hpp"
#include "dbat/utils.hpp"

namespace {
  /* A social's messages, in the order the menu and astat list them */
  struct message_row {
    char key;
    const char *label;  /* as the menu shows it, before the ": " */
    const char *prompt; /* asking for a new one, before the old one is shown */
    social_message social_messg::*text;
  };

  constexpr message_row MESSAGES[] = {
      {'a', "Char    [NO ARG]", "Enter social shown to the Character when there is no argument supplied.", &social_messg::char_no_arg},
      {'b', "Others  [NO ARG]", "Enter social shown to Others when there is no argument supplied.", &social_messg::others_no_arg},
      {'c', "Char [NOT FOUND]", "Enter text shown to the Character when his victim isnt found.", &social_messg::not_found},
      {'d', "Char  [ARG SELF]", "Enter social shown to the Character when it is its own victim.", &social_messg::char_auto},
      {'e', "Others[ARG SELF]", "Enter social shown to Others when the Char is its own victim.", &social_messg::others_auto},
      {'f', "Char      [VICT]", "Enter normal social shown to the Character when the victim is found.", &social_messg::char_found},
      {'g', "Others    [VICT]", "Enter normal social shown to Others when the victim is found.", &social_messg::others_found},
      {'h', "Victim    [VICT]", "Enter normal social shown to the Victim when the victim is found.", &social_messg::vict_found},
      {'i', "Char  [BODY PRT]", "Enter 'body part' social shown to the Character when the victim is found.", &social_messg::char_body_found},
      {'j', "Others[BODY PRT]", "Enter 'body part' social shown to Others when the victim is found.", &social_messg::others_body_found},
      {'k', "Victim[BODY PRT]", "Enter 'body part' social shown to the Victim when the victim is found.", &social_messg::vict_body_found},
      {'l', "Char       [OBJ]", "Enter 'object' social shown to the Character when the object is found.", &social_messg::char_obj_found},
      {'m', "Others     [OBJ]", "Enter 'object' social shown to the Room when the object is found.", &social_messg::others_obj_found},
  };

  /* The command (other than a social) the text names, by abbreviating what it's sorted as or being it; -1 for none */
  int find_named_command(GameWorld *g, std::string_view txt)
  {
    for (int cmd = 1; g->complete_cmd_info[cmd].command != "\n"; cmd++)
      if (g->complete_cmd_info[cmd].sort_as.starts_with(txt) || g->complete_cmd_info[cmd].command == txt)
        return cmd;
    return -1;
  }

  void save_to_disk(GameWorld *g)
  {
    if (save_socials(g)) /* otherwise still listed as unsaved */
      remove_from_save_list(g, AEDIT_PERMISSION, SL_ACT);
  }

  /* The social editor: one social, found by what it abbreviates or new */
  struct social_editor final : olc_session {
    enum class mode { confirm_save, confirm_edit, confirm_add, main_menu, name, sort_as, min_char_pos, min_vict_pos, min_level, message };

    mode asking = mode::confirm_edit;
    std::string keyword;          /* what the builder asked to edit */
    int index = 0;                /* the social in g->soc_mess_list, or past its end for a new one */
    social_messg action{};        /* the social as it's being edited */
    const message_row *message{}; /* the message being asked for */
    bool changed = false;

    explicit social_editor(std::string_view asked) : keyword(asked) {}

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override
    {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing zone {:d} allowed zone {:d}", char_get_name(d->character), NOWHERE,
             char_get_olc_zone(g, d->character));
    }

    void menu(GameWorld *g, struct descriptor_data *d);
    bool find_from(GameWorld *g, int first);
    void ask(GameWorld *g, struct descriptor_data *d);
    void setup_existing(GameWorld *g);
    void setup_new();
    void save(GameWorld *g);
    void ask_position(GameWorld *g, struct descriptor_data *d, const char *whose, mode next);
  };

  void social_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    std::string messages;
    for (const message_row &row : MESSAGES)
      messages += str_format("@g{:c}@n) {}: @c{}\r\n", row.key, row.label, (action.*row.text).value_or("<Null>"));
    write_to_output(g, d,
                    "@n-- Action editor\r\n@gn@n) Command         : @y{:<15.15}@n @g1@n) Sort as Command  : @y{:<15.15}@n\r\n@g2@n) Min Position[CH]: @c{:<8.8}        @g3@n) Min Position [VT]: @c{:<8.8}\r\n@g4@n) Min Level   [CH]: @c{:<3d}             @g5@n) Show if Invisible: @c{}\r\n{}@gq@n) Quit\r\nEnter Choice:",
                    action.command, action.sort_as, position_types[action.min_char_position], position_types[action.min_victim_position],
                    action.min_level_char, action.hide ? "HIDDEN" : "NOT HIDDEN", messages);
    asking = mode::main_menu;
  }

  /* Finds the first social from `first` on that the keyword abbreviates: true if there is one */
  bool social_editor::find_from(GameWorld *g, int first)
  {
    for (index = first; index <= g->top_of_socialt; index++)
      if (is_abbrev(keyword, g->soc_mess_list[index].command))
        return true;
    return false;
  }

  /* Asks whether to edit the social found, or to add a new one */
  void social_editor::ask(GameWorld *g, struct descriptor_data *d)
  {
    if (index > g->top_of_socialt) {
      write_to_output(g, d, "Do you wish to add the '{}' action? ", keyword);
      asking = mode::confirm_add;
    } else {
      write_to_output(g, d, "Do you wish to edit the '{}' action? ", g->soc_mess_list[index].command);
      asking = mode::confirm_edit;
    }
  }

  void social_editor::setup_existing(GameWorld *g)
  {
    action = g->soc_mess_list[index];
  }

  void social_editor::setup_new()
  {
    action.command             = keyword;
    action.sort_as             = keyword;
    action.min_victim_position = POS_STANDING;
    action.min_char_position   = POS_STANDING;
    action.char_no_arg         = "This action is unfinished.";
    action.others_no_arg       = "This action is unfinished.";
  }

  /* Puts the social into the table and writes the socials out */
  void social_editor::save(GameWorld *g)
  {
    if (index > g->top_of_socialt) {
      auto *grown = new social_messg[g->top_of_socialt + 2]();
      std::move(g->soc_mess_list, g->soc_mess_list + g->top_of_socialt + 1, grown);
      grown[++g->top_of_socialt] = std::move(action);
      delete[] g->soc_mess_list;
      g->soc_mess_list = grown;
    } else {
      action.act_nr           = g->soc_mess_list[index].act_nr;
      g->soc_mess_list[index] = std::move(action);
    }
    action = {}; /* the table's now */

    create_command_list(g);
    sort_commands(g);

    add_to_save_list(g, AEDIT_PERMISSION, SL_ACT);
    save_to_disk(g); /* autosave by Rumble */
  }

  void social_editor::ask_position(GameWorld *g, struct descriptor_data *d, const char *whose, mode next)
  {
    write_to_output(g, d, "Enter the minimum position the {} has to be in to activate social:\r\n", whose);
    for (int i = POS_DEAD; i <= POS_STANDING; i++)
      write_to_output(g, d, "   {:d}) {}\r\n", i, position_types[i]);
    write_to_output(g, d, "Enter choice: ");
    asking = next;
  }

  void social_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char answer = first_char(line);
    std::string text(line);

    switch (asking) {
      case mode::confirm_save:
        switch (answer) {
          case 'y':
          case 'Y': {
            std::string command = action.command;
            save(g);
            mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "OLC: {} edits action {}", char_get_name(d->character), command);
            leave_olc(g, d);
            write_to_output(g, d, "Action saved to disk.\r\n");
            return;
          }
          case 'n':
          case 'N':
            leave_olc(g, d);
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes? : ");
            return;
        }

      case mode::confirm_edit:
        switch (answer) {
          case 'y':
          case 'Y':
            setup_existing(g);
            changed = false;
            menu(g, d);
            return;
          case 'q':
          case 'Q':
            leave_olc(g, d);
            return;
          case 'n':
          case 'N':
            if (!find_from(g, index + 1) && find_named_command(g, keyword) != -1) {
              leave_olc(g, d);
              return;
            }
            ask(g, d);
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\nDo you wish to edit the '{}' action? ", g->soc_mess_list[index].command);
            return;
        }

      case mode::confirm_add:
        switch (answer) {
          case 'y':
          case 'Y':
            setup_new();
            menu(g, d);
            changed = false;
            return;
          case 'n':
          case 'N':
          case 'q':
          case 'Q':
            leave_olc(g, d);
            return;
          default:
            write_to_output(g, d, "Invalid choice!\r\nDo you wish to add the '{}' action? ", keyword);
            return;
        }

      case mode::main_menu:
        switch (answer) {
          case 'q':
          case 'Q':
            if (changed) {
              write_to_output(g, d, "Do you wish to save your changes? : ");
              asking = mode::confirm_save;
            } else
              leave_olc(g, d);
            return;
          case 'n': /* quirk: not 'N' */
            write_to_output(g, d, "Enter action name: ");
            asking = mode::name;
            return;
          case '1':
            write_to_output(g, d, "Enter sort info for this action (for the command listing): ");
            asking = mode::sort_as;
            return;
          case '2':
            ask_position(g, d, "Character", mode::min_char_pos);
            return;
          case '3':
            ask_position(g, d, "Victim", mode::min_vict_pos);
            return;
          case '4':
            write_to_output(g, d, "Enter new minimum level for social: ");
            asking = mode::min_level;
            return;
          case '5':
            action.hide = !action.hide;
            menu(g, d);
            changed = true;
            return;
          default:
            if (auto row = std::ranges::find(MESSAGES, tolower(answer), &message_row::key); answer && row != std::end(MESSAGES)) {
              write_to_output(g, d, "{}\r\n[OLD]: {}\r\n[NEW]: ", row->prompt, (action.*row->text).value_or("NULL"));
              message = row;
              asking  = mode::message;
              return;
            }
            menu(g, d);
            return;
        }

      case mode::name:
      case mode::sort_as:
        if (text.empty() || text.contains(' ')) {
          menu(g, d);
          return;
        }
        if (asking == mode::name) {
          action.command = text;
        } else {
          action.sort_as = text;
        }
        break;

      case mode::min_char_pos:
      case mode::min_vict_pos: {
        int position = atoi(text.c_str());
        if (text.empty() || position < POS_DEAD || position > POS_STANDING) {
          menu(g, d);
          return;
        }
        (asking == mode::min_char_pos ? action.min_char_position : action.min_victim_position) = position;
        break;
      }

      case mode::min_level: {
        int level = atoi(text.c_str());
        if (text.empty() || level < 0) {
          menu(g, d);
          return;
        }
        action.min_level_char = level;
        break;
      }

      case mode::message:
        if (!text.empty()) {
          delete_doubledollar(text);
          action.*message->text = text;
        } else
          (action.*message->text).reset();
        break;
    }
    changed = true;
    menu(g, d);
  }
} // namespace

ACMD(do_oasis_aedit)
{
  if (CONFIG_NEW_SOCIALS == 0) {
    send_to_char(g, ch, "Socials cannot be edited at the moment.\r\n");
    return;
  }

  if (char_get_olc_zone(g, ch) != AEDIT_PERMISSION && char_get_admlevel(ch) < ADMLVL_BUILDER) {
    send_to_char(g, ch, "You don't have access to editing socials.\r\n");
    return;
  }

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_AEDIT) {
      send_to_char(g, ch, "Sorry, only one can edit socials at a time.\r\n");
      return;
    }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Please specify a social to edit.\r\n");
    return;
  }

  struct descriptor_data *d = char_get_desc(ch);

  if (str_eq_nocase("save", arg)) {
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves socials.", char_get_name(ch));
    send_to_char(g, ch, "Writing social file..\r\n");
    save_to_disk(g);
    send_to_char(g, ch, "Done.\r\n");
    return;
  }

  open_session(g, d, std::make_unique<social_editor>(arg), "do_oasis");
  social_editor &editor = session_of<social_editor>(d);

  if (!editor.find_from(g, 0)) {
    if (int i = find_named_command(g, arg); i != -1) {
      send_to_char(g, ch, "The '{}' command already exists ({}).\r\n", arg, g->complete_cmd_info[i].command);
      leave_olc(g, d);
      return;
    }
  }
  editor.ask(g, d);
  begin_editing(g, d, CON_AEDIT);
  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} starts editing actions.", char_get_name(ch));
}

ACMD(do_astat)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Astat which social?\r\n");
    return;
  }

  int i = 0;
  while (i <= g->top_of_socialt && !is_abbrev(arg, g->soc_mess_list[i].command))
    i++;
  if (i > g->top_of_socialt) {
    send_to_char(g, ch, "No such social.\r\n");
    return;
  }

  const social_messg &social = g->soc_mess_list[i];
  std::string messages;
  for (const message_row &row : MESSAGES)
    messages += str_format("{:c}) {}: @c{}@n\r\n", row.key, row.label, message_text(social.*row.text));
  send_to_char(g, ch,
               "n) Command         : @y{:<15.15}@n 1) Sort as Command : @y{:<15.15}@n\r\n2) Min Position[CH]: @c{:<8.8}@n        3) Min Position[VT]: @c{:<8.8}@n\r\n4) Min Level   [CH]: @c{:<3d}@n             5) Show if Invis   : @c{}@n\r\n{}",
               social.command, social.sort_as, position_types[social.min_char_position], position_types[social.min_victim_position],
               social.min_level_char, social.hide ? "HIDDEN" : "NOT HIDDEN", messages);
}
