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
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/help.hpp"
#include "dbat/utils.hpp"
#include "dbat/hedit.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/boards.hpp"
#include "dbat/oasis.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/config.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.social.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* external functions */

/* local functions */
static void hedit_setup_new(GameWorld *g, struct descriptor_data *);
static void hedit_setup_existing(GameWorld *g, struct descriptor_data *, int);
static void hedit_save_to_disk(GameWorld *g, struct descriptor_data *);
static void hedit_save_internally(GameWorld *g, struct descriptor_data *);

static void hedit_disp_menu(GameWorld *g, struct descriptor_data *);

ACMD(do_oasis_hedit)
{
  std::string arg;
  struct descriptor_data *d;

  /* No building as a mob or while being forced. */
  if (char_is_npc(ch) || !char_get_desc(ch) || desc_get_connected(char_get_desc(ch)) != CON_PLAYING)
    return;

  if (g->HEDITS == true) {
    send_to_char(g, ch, "Sorry, only one person can edit help files at a time.\r\n");
    return;
  }

  if (char_get_admlevel(ch) < 4 && (!strcasecmp("Tepsih", char_get_name(ch)) && !strcasecmp("Rogoshen", char_get_name(ch)))) {
    send_to_char(g, ch, "Sorry you are incapable of editing help files at this time.\r\n");
    return;
  }

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Please specify a help entry to edit.\r\n");
    return;
  }

  d = char_get_desc(ch);

  if (str_eq_nocase("save", arg)) {
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves help files.",
           char_get_name(ch));
    hedit_save_to_disk(g, d);
    send_to_char(g, ch, "Saving help files.\r\n");
    return;
  }

  /* Give descriptor an OLC structure. */
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis: Player already had olc structure.");
    delete d->olc;
  }

  d->olc         = new oasis_olc_data();
  OLC_NUM(d)     = 0;
  OLC_STORAGE(d) = arg;
  OLC_INDEX(d)   = search_help(g, OLC_STORAGE(d).c_str(), ADMLVL_IMPL);

  if (OLC_INDEX(d) == NOWHERE) {
    send_to_char(g, ch, "Do you wish to add the '{}' help file? ", OLC_STORAGE(d).c_str());
    OLC_MODE(d) = HEDIT_CONFIRM_ADD;
  } else {
    send_to_char(g, ch, "Do you wish to edit the '{}' help file? ", g->help_table[OLC_INDEX(d)].keywords.c_str());
    OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
  }

  desc_set_connected(d, CON_HEDIT);
  /*send_to_room(IN_ROOM(ch), "%s starts using OLC.\r\n", GET_NAME(ch));*/
  act(g, "$n starts using OLC.", true, ch, nullptr, nullptr, TO_ROOM);
  g->HEDITS = true;
  char_plr_set(ch, PLR_WRITING, true);
  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: {} starts editing help files.", char_get_name(ch));
}

static void hedit_setup_new(GameWorld *g, struct descriptor_data *d)
{
  OLC_HELP(d) = new help_index_element();

  char buf3[MAX_INPUT_LENGTH];
  sprintf(buf3, "<<X<< Put helpfile keywords here in caps");

  OLC_HELP(d)->keywords = buf3;
  OLC_HELP(d)->entry    = "\r\nThis help file is unfinished.\r\n";
  OLC_VAL(d)         = 0;

  hedit_disp_menu(g, d);
}

static void hedit_setup_existing(GameWorld *g, struct descriptor_data *d, int rnum)
{
  OLC_HELP(d) = new help_index_element();

  *OLC_HELP(d) = g->help_table[rnum];
  if (OLC_HELP(d)->keywords.empty())
    OLC_HELP(d)->keywords = "undefined"; /* as str_udup() */
  if (OLC_HELP(d)->entry.empty())
    OLC_HELP(d)->entry = "undefined";
  OLC_VAL(d)             = 0;

  hedit_disp_menu(g, d);
}

static void hedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  if (OLC_INDEX(d) == NOWHERE)
    g->help_table.push_back(*OLC_HELP(d));
  else
    g->help_table[OLC_INDEX(d)] = *OLC_HELP(d);

  add_to_save_list(g, HEDIT_PERMISSION, SL_HLP);
  hedit_save_to_disk(g, d);
}

static void hedit_save_to_disk(GameWorld *g, struct descriptor_data *d)
{
  struct atomic_file af;
  FILE *fp;
  char buf1[MAX_STRING_LENGTH], index_name[READ_SIZE];
  int i;

  snprintf(index_name, sizeof(index_name), "%s%s", HLP_PREFIX, HELP_FILE);
  if (!(fp = atomic_open(&af, index_name, false))) {
    mud_log("SYSERR: Could not write help index file");
    return;
  }

  for (i = 0; i < (int)g->help_table.size(); i++) {
    if (g->help_table[i].duplicate)
      continue;
    strlcpy(buf1, !g->help_table[i].entry.empty() ? g->help_table[i].entry.c_str() : "Empty\r\n", sizeof(buf1));
    strip_cr(buf1);

    /* Forget making a buffer, lets just write the thing now. */
    fprintf(fp, "%s#%d\n", buf1, g->help_table[i].min_level);
  }
  /* Write final line and close. */
  fprintf(fp, "$~\n");
  if (!atomic_commit(&af))
    return; /* still listed as unsaved */

  remove_from_save_list(g, HEDIT_PERMISSION, SL_HLP);

  /* Reboot the help files. */
  free_help_table(g);
  index_boot(g, DB_BOOT_HLP);
}

/* The main menu. */
static void hedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d,
                  "@n-- Help file editor\r\n@g1@n) Keywords    : @y{}\n@g2@n) Entry       :\r\n@y{}@g3@n) Min Level   : @y{:d}\r\n@gQ@n) Quit\r\nEnter choice : ",
                  !OLC_HELP(d)->keywords.empty() ? OLC_HELP(d)->keywords.c_str() : "<None>",
                  !OLC_HELP(d)->entry.empty() ? OLC_HELP(d)->entry.c_str() : "<None>\r\n",
                  OLC_HELP(d)->min_level);
  OLC_MODE(d) = HEDIT_MAIN_MENU;
}

void hedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int number, change = true;

  switch (OLC_MODE(d)) {
    case HEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          if (OLC_HELP(d)->keywords.empty()) {
            hedit_disp_menu(g, d);
            write_to_output(g, d, "\n@RYou must fill in the keywords before you save.@n\r\n");
          } else if (OLC_HELP(d)->keywords.find("undefined") != std::string::npos) {
            hedit_disp_menu(g, d);
            write_to_output(g, d, "\n@RYou must fill in the keywords before you save.@n\r\n");
          } else if (OLC_HELP(d)->keywords.find("<<X<<") != std::string::npos) {
            hedit_disp_menu(g, d);
            write_to_output(g, d, "\n@RYou must fill in the keywords before you save.@n\r\n");
          } else {
            write_to_output(g, d, "Help saved to disk.\r\n");
            char buf[MAX_INPUT_LENGTH];
            *buf = '\0';
            snprintf(buf, sizeof(buf), "%s", OLC_HELP(d)->keywords.c_str());
            send_to_imm(g, "@gHedit@D: @w{}@G has just edited and saved, @Y{}@G.@n", char_get_alias(d->character), buf);
            hedit_save_internally(g, d);
            cleanup_olc(g, d, CLEANUP_STRUCTS);
            g->HEDITS = false;
          }
          break;
        case 'n':
        case 'N':
          /* Free everything up, including strings, etc. */
          cleanup_olc(g, d, CLEANUP_ALL);
          g->HEDITS = false;
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes? : \r\n");
          break;
      }
      return;

    case HEDIT_CONFIRM_EDIT:
      switch (*arg) {
        case 'y':
        case 'Y':
          hedit_setup_existing(g, d, OLC_INDEX(d));
          break;
        case 'q':
        case 'Q':
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        case 'n':
        case 'N':
          OLC_INDEX(d)
          ++;
          for (; OLC_INDEX(d) < (int)g->help_table.size(); OLC_INDEX(d)++)
            if (is_abbrev(OLC_STORAGE(d).c_str(), g->help_table[OLC_INDEX(d)].keywords.c_str()))
              break;
            else
              OLC_INDEX(d) = g->help_table.size();

          if (OLC_INDEX(d) >= (int)g->help_table.size()) {
            OLC_INDEX(d) = NOWHERE; /* what hedit_save_internally() takes to mean a new entry */
            write_to_output(g, d, "Do you wish to add the '{}' help file? ",
                            OLC_STORAGE(d).c_str());
            OLC_MODE(d) = HEDIT_CONFIRM_ADD;
          } else {
            write_to_output(g, d, "Do you wish to edit the '{}' help file? ",
                            g->help_table[OLC_INDEX(d)].keywords.c_str());
            OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
          }
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to edit the '{}' help file? ",
                          g->help_table[OLC_INDEX(d)].keywords.c_str());
          break;
      }
      return;

    case HEDIT_CONFIRM_ADD:
      switch (*arg) {
        case 'y':
        case 'Y':
          hedit_setup_new(g, d);
          break;
        case 'n':
        case 'N':
        case 'q':
        case 'Q':
          cleanup_olc(g, d, CLEANUP_ALL);
          g->HEDITS = false;
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to add the '{}' help file? ",
                          OLC_STORAGE(d).c_str());
          break;
      }
      return;

    case HEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) {
            /* Something has been modified. */
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
          } else {
            write_to_output(g, d, "No changes made.\r\n");
            cleanup_olc(g, d, CLEANUP_ALL);
            g->HEDITS = false;
          }
          break;
        case '1':
          OLC_MODE(d) = HEDIT_KEYWORDS;
          clear_screen(g, d);
          write_to_output(g, d, "Enter help file keywords: ");
          break;
        case '2':
          OLC_MODE(d) = HEDIT_ENTRY;
          clear_screen(g, d);
          send_editor_help(g, d);
          write_to_output(g, d, "Enter help entry: (/s saves /h for help)\r\n");
          if (!OLC_HELP(d)->entry.empty()) {
            write_to_output(g, d, "{}", OLC_HELP(d)->entry.c_str());
          }
          string_write(d, &OLC_HELP(d)->entry, MAX_MESSAGE_LENGTH, 0);
          OLC_VAL(d) = 1;
          break;
        case '3':
          write_to_output(g, d, "Enter min level : ");
          OLC_MODE(d) = HEDIT_MIN_LEVEL;
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\n");
          hedit_disp_menu(g, d);
          break;
      }
      return;

    case HEDIT_KEYWORDS:
      if (strlen(arg) > MAX_HELP_KEYWORDS)
        arg[MAX_HELP_KEYWORDS - 1] = '\0';
      strip_cr(arg);
      OLC_HELP(d)->keywords = *arg ? arg : "undefined"; /* as str_udup() */
      if (OLC_HELP(d)->keywords.find("undefined") != std::string::npos) {
        OLC_MODE(d) = HEDIT_KEYWORDS;
        clear_screen(g, d);
        write_to_output(g, d, "@RYou must at least enter SOME keywords.@n\n");
        write_to_output(g, d, "Keywords: ");
        change = false;
      } else if (OLC_HELP(d)->keywords.find("<<X<<") != std::string::npos) {
        OLC_MODE(d) = HEDIT_KEYWORDS;
        clear_screen(g, d);
        write_to_output(g, d, "@RLet's not joke around with help files now.@n\n");
        write_to_output(g, d, "Keywords: ");
        change = false;
      } else if (OLC_HELP(d)->keywords.find("<<x<<") != std::string::npos) {
        OLC_MODE(d) = HEDIT_KEYWORDS;
        clear_screen(g, d);
        write_to_output(g, d, "@RLet's not joke around with help files now.@n\n");
        write_to_output(g, d, "Keywords: ");
        change = false;
      } else {
        OLC_HELP(d)->entry = OLC_HELP(d)->keywords + "\r\n----------\r\n\r\n" + OLC_HELP(d)->entry;
      }
      break;

    case HEDIT_ENTRY:
      /* We will NEVER get here, we hope. */
      mudlog(g, true, ADMLVL_BUILDER, LOG_BRF, "SYSERR: Reached HEDIT_ENTRY case in parse_hedit");
      break;

    case HEDIT_MIN_LEVEL:
      number = atoi(arg);
      if ((number < 0) || (number > ADMLVL_IMPL))
        write_to_output(g, d, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
      else {
        OLC_HELP(d)->min_level = number;
        break;
      }
      return;

    default:
      /* We should never get here. */
      mudlog(g, true, ADMLVL_BUILDER, LOG_BRF, "SYSERR: Reached default case in parse_hedit");
      break;
  }

  /* If we get this far, something has been changed. */
  if (change == true) {
    OLC_VAL(d) = 1;
    hedit_disp_menu(g, d);
  }
}

void hedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    case HEDIT_ENTRY:
      hedit_disp_menu(g, d);
      break;
  }
}

ACMD(do_helpcheck)
{
  char buf[MAX_STRING_LENGTH];
  int i, count = 0;
  size_t len = 0, nlen;

  send_to_char(g, ch, "Commands without help entries:\r\n");

  for (i = 1; *(g->complete_cmd_info[i].command) != '\n'; i++) {
    if (g->complete_cmd_info[i].command_pointer != do_action && g->complete_cmd_info[i].minimum_level >= 0) {
      if (search_help(g, g->complete_cmd_info[i].command, ADMLVL_IMPL) == NOWHERE) {
        /*if (search_help(complete_cmd_info[i].command, ADMLVL_IMPL) == 
NOWHERE) {*/
        nlen = snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", g->complete_cmd_info[i].command,
                        (++count % 3 ? "" : "\r\n"));
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    }
  }
  if (count % 3 && len < sizeof(buf))
    nlen = snprintf(buf + len, sizeof(buf) - len, "\r\n");

  if (char_get_desc(ch))
    send_to_char(g, ch, "{}", buf);

  *buf = '\0';
}

ACMD(do_hindex)
{
  int len, count = 0, i, num = 0;
  char buf[MAX_STRING_LENGTH];

  std::string text(skip_spaces(argument));

  if (text.empty()) {
    send_to_char(g, ch, "Usage: hindex <string>\r\n");
    for (i = 0; i < (int)g->help_table.size(); i++) {
      num++;
    }
    if (num > 0 && char_get_admlevel(ch) > 0) {
      send_to_char(g, ch, "\r\n@D[@Y{:d}@y Help files in index.@D]@n\r\n", num);
    }
    return;
  }

  len = sprintf(buf, "Help index entries based on '%s':\r\n", text.c_str());
  for (i = 0; i < (int)g->help_table.size(); i++) {
    num++;
    if (is_abbrev(text, g->help_table[i].keywords.c_str()) && (char_get_admlevel(ch) >= g->help_table[i].min_level))
      len +=
          snprintf(buf + len, sizeof(buf) - len, "%-20.20s%s", g->help_table[i].keywords.c_str(),
                   (++count % 3 ? "" : "\r\n"));
  }
  if (count % 3)
    len += snprintf(buf + len, sizeof(buf) - len, "\r\n");

  if (!count)
    len += snprintf(buf + len, sizeof(buf) - len, "  None.\r\n");

  if (count > 0 && char_get_admlevel(ch) > 0) {
    len += snprintf(buf + len, sizeof(buf) - len, "  %d Help files in index.\r\n", count);
  }
  send_to_char(g, ch, "{}", buf);
}
