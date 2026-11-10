/* ************************************************************************
*   File: modify.c                                      Part of CircleMUD *
*  Usage: Run-time modification of game variables                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/modify.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/mail.hpp"
#include "dbat/boards.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/oasis.hpp"
#include "dbat/tedit.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/config.hpp"
#include "dbat/hedit.hpp"
#include "dbat/game_world.hpp"

/* local functions */

static void smash_numb(char *str);
static void playing_string_cleanup(GameWorld *g, struct descriptor_data *d, int action);
static void exdesc_string_cleanup(GameWorld *g, struct descriptor_data *d, int action);

/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/*
 * Put '#if 1' here to erase ~, or roll your own method.  A common idea
 * is smash/show tilde to convert the tilde to another innocuous character
 * to save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP.
 *   -gg 9/9/98
 */
void smash_tilde(char *str)
{
  /*
   * Erase any _line ending_ tildes inserted in the editor.
   * The load mechanism can't handle those, yet.
   * -- Welcor 04/2003
   */

  char *p = str;
  for (; *p; p++)
    if (*p == '~' && (*(p + 1) == '\r' || *(p + 1) == '\n' || *(p + 1) == '\0'))
      *p = ' ';
#if 1
  /*
   * Erase any ~'s inserted by people in the editor.  This prevents anyone
   * using online creation from causing parse errors in the world files.
   * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
   * his tildes thank you very much.), -gg 2/20/98
   */
  while ((str = strchr(str, '~')) != nullptr)
    *str = ' ';
#endif
}

static void smash_numb(char *str)
{
  /*
   * Erase any _line ending_ tildes inserted in the editor.
   * The load mechanism can't handle those, yet.
   * -- Welcor 04/2003
   */

  char *p = str;
  for (; *p; p++)
    if (*p == '#' && (*(p + 1) == '\r' || *(p + 1) == '\n' || *(p + 1) == '\0'))
      *p = ' ';
#if 1
  /*
   * Erase any ~'s inserted by people in the editor.  This prevents anyone
   * using online creation from causing parse errors in the world files.
   * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
   * his tildes thank you very much.), -gg 2/20/98
   */
  while ((str = strchr(str, '#')) != nullptr)
    *str = ' ';
#endif
}

/* Starts the editor on target, with the text there now, which is also kept to put back if the edit is aborted. */
void string_edit_begin(struct descriptor_data *d, std::string *target, size_t len)
{
  d->str       = target;
  d->edit_text = *target;
  d->backstr   = *target;
  d->max_str   = len;
}

/* Starts writing to writeto: string_edit_begin(), as a player who is writing (mailto is the letter's recipient). */
void string_write(struct descriptor_data *d, std::string *writeto, size_t len, long mailto)
{
  if (d->character && !char_is_npc(d->character))
    char_plr_set(d->character, PLR_WRITING, true);

  string_edit_begin(d, writeto, len);
  if (!using_improved_editor) /* which can't abort */
    d->backstr.clear();
  d->mail_to = mailto;
}

/*
 * Add user input to the 'current' string (as defined by d->str).
 * This is still overly complex.
 */
void string_add(GameWorld *g, struct descriptor_data *d, char *str)
{
  int action;

  delete_doubledollar(str);
  smash_tilde(str);
  smash_numb(str);

  /* determine if this is the terminal string, and truncate if so */
  /* changed to only accept '@' at the beginning of line - J. Elson 1/17/94 */
  /* changed to only accept '@' if it's by itself - fnord 10/15/2004 */
  if ((action = (*str == '@' && !str[1])))
    *str = '\0';
  else if ((action = improved_editor_execute(g, d, str)) == STRINGADD_ACTION)
    return;

  std::string &text = d->edit_text;

  if (action != STRINGADD_OK)
    /* Do nothing. */;
  else if (text.empty()) {
    if (strlen(str) + 3 > d->max_str) { /* \r\n\0 */
      send_to_char(g, d->character, "String too long - Truncated.\r\n");
      strcpy(str + (d->max_str - 3), "\r\n");
      text = str;
      if (!using_improved_editor)
        action = STRINGADD_SAVE;
    } else
      text = str;
  } else {
    if (strlen(str) + text.size() + 3 > d->max_str) { /* \r\n\0 */
      send_to_char(g, d->character, "String too long.  Last line skipped.\r\n");
      if (!using_improved_editor)
        action = STRINGADD_SAVE;
      else if (action == STRINGADD_OK)
        action = STRINGADD_ACTION; /* No appending \r\n\0, but still let them save. */
    } else
      text += str;
  }

  /*
   * Common cleanup code.
   */
  switch (action) {
    case STRINGADD_ABORT:
      switch (desc_get_connected(d)) {
        case CON_CEDIT:
        case CON_TEDIT:
        case CON_NEWSEDIT:
        case CON_REDIT:
        case CON_MEDIT:
        case CON_OEDIT:
        case CON_IEDIT:
        case CON_EXDESC:
        case CON_TRIGEDIT:
        case CON_HEDIT:
          *d->str = d->backstr; /* what was there before */
          d->backstr.clear();
          d->str = nullptr;
          break;
        case CON_PLAYING:
          /* all CON_PLAYING are handled below in playing_string_cleanup */
          break;

        default:
          mud_log("SYSERR: string_add: Aborting write from unknown origin.");
          break;
      }
      break;
    case STRINGADD_SAVE:
      if (text.empty())
        text = "Nothing.\r\n";
      *d->str = text; /* the cleanup below may read it from there */
      d->backstr.clear();
      break;
    case STRINGADD_ACTION:
      break;
  }

  /* Ok, now final cleanup. */

  if (action == STRINGADD_SAVE || action == STRINGADD_ABORT) {
    int i;
    struct {
      int mode{};
      void (*func)(GameWorld *g, struct descriptor_data *dsc, int todo){};
    } cleanup_modes[] = {
        {CON_CEDIT,    cedit_string_cleanup   },
        {CON_MEDIT,    medit_string_cleanup   },
        {CON_OEDIT,    oedit_string_cleanup   },
        {CON_REDIT,    redit_string_cleanup   },
        {CON_TEDIT,    tedit_string_cleanup   },
        {CON_TRIGEDIT, trigedit_string_cleanup},
        {CON_EXDESC,   exdesc_string_cleanup  },
        {CON_PLAYING,  playing_string_cleanup },
        {CON_IEDIT,    oedit_string_cleanup   },
        {CON_HEDIT,    hedit_string_cleanup   },
        {CON_NEWSEDIT, news_string_cleanup    },
        {-1,           nullptr                }
    };

    for (i = 0; cleanup_modes[i].func; i++)
      if (desc_get_connected(d) == cleanup_modes[i].mode)
        (*cleanup_modes[i].func)(g, d, action);

    /* Common post cleanup code. */
    d->str = nullptr;
    d->edit_text.clear();
    d->mail_to = 0;
    d->max_str = 0;
    if (d->character && !char_is_npc(d->character)) {
      char_plr_set(d->character, PLR_MAILING, false);
      char_plr_set(d->character, PLR_WRITING, false);
    }
  } else if (action != STRINGADD_ACTION && text.size() + 3 <= d->max_str) /* 3 = \r\n\0 */
    text += "\r\n";
}

static void playing_string_cleanup(GameWorld *g, struct descriptor_data *d, int action)
{
  if (char_plr_flagged(d->character, PLR_MAILING)) {
    if (action == STRINGADD_SAVE && !d->edit_text.empty()) {
      store_mail(g, d->mail_to, char_get_idnum(d->character), d->edit_text.data());
      write_to_output(g, d, "Message sent!\r\n");
      notify_if_playing(g, d->character, d->mail_to);
    } else {
      write_to_output(g, d, "Mail aborted.\r\n");
    }
    d->draft.clear(); /* store_mail() keeps its own copy */
  }

  if (char_plr_flagged(d->character, PLR_WRITING)) {
    if (d->mail_to >= BOARD_MAGIC) {
      struct board_info *board = locate_board(g, d->mail_to - BOARD_MAGIC);

      if (action == STRINGADD_ABORT) {
        if (board_discard_message(board, d->str))
          write_to_output(g, d, "Post aborted.\r\n");
        else
          write_to_output(g, d, "Unable to find your message to delete it!\r\n");
      } else {
        write_to_output(g, d, "\r\nPost saved.\r\n");
        save_board(g, board);
      }
    }

    /* hm... I wonder what happens when you can't finish writing a note */
  }
}

static void exdesc_string_cleanup(GameWorld *g, struct descriptor_data *d, int action)
{
  if (action == STRINGADD_ABORT)
    write_to_output(g, d, "Description aborted.\r\n");

  write_to_output(g, d, "{}", CONFIG_MENU.c_str());
  desc_set_connected(d, CON_MENU);
}

/* *********************************************************************
   *  Modification of character skills                                 *
   ********************************************************************* */

/* The length of a buffer of size bytes after a write that wanted to add wrote bytes, as snprintf() reports it.
 * snprintf() returns what it would have written, so adding that up can pass the end of the buffer. */
static size_t text_used(size_t len, int wrote, size_t size)
{
  len += wrote > 0 ? (size_t)wrote : 0;
  return len < size ? len : size - 1;
}

ACMD(do_skillset)
{
  struct char_data *vict;
  char help_text[MAX_STRING_LENGTH];
  int skill, value, i = 0, qend;

  std::string_view rest = argument;
  std::string name      = take_arg(rest);

  if (name.empty()) { /* no arguments. print an informative text */
    send_to_char(g, ch, "Syntax: skillset <name> '<skill>' <value>\r\nSkill being one of the following:\r\n");
    for (qend = 0, i = 0; i < SKILL_TABLE_SIZE; i++) {
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

  if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "{}", CONFIG_NOPERSON.c_str());
    return;
  }
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "NPCs don't have skills.\r\n");
    return;
  }
  rest = skip_spaces(rest);

  /* If there is no chars in argument */
  if (rest.empty()) {
    size_t len = 0;

    len = text_used(len, snprintf(help_text, sizeof(help_text), "\r\nSkills:\r\n"), sizeof(help_text));
    len = text_used(len, print_skills_by_type(g, vict, help_text + len, sizeof(help_text) - len, SKTYPE_SKILL, ""), sizeof(help_text));
    len = text_used(len, snprintf(help_text + len, sizeof(help_text) - len, "\r\nSpells:\r\n"), sizeof(help_text));
    len = text_used(len, print_skills_by_type(g, vict, help_text + len, sizeof(help_text) - len, SKTYPE_SPELL, ""), sizeof(help_text));
    if (CONFIG_ENABLE_LANGUAGES) {
      len = text_used(len, snprintf(help_text + len, sizeof(help_text) - len, "\r\nLanguages:\r\n"), sizeof(help_text));
      len = text_used(len, print_skills_by_type(g, vict, help_text + len, sizeof(help_text) - len, SKTYPE_SKILL | SKTYPE_LANG, ""), sizeof(help_text));
    }
    if (len == sizeof(help_text) - 1)
      strcpy(help_text + sizeof(help_text) - strlen("** OVERFLOW **") - 1, "** OVERFLOW **"); /* strcpy: OK */
    send_to_char(g, ch, "{}", help_text);
    return;
  }

  if (rest[0] != '\'') {
    send_to_char(g, ch, "Skill must be enclosed in: ''\r\n");
    return;
  }
  /* Locate the closing quote; the magic words between are lowercased */
  size_t close = rest.find('\'', 1);
  if (close == std::string_view::npos) {
    send_to_char(g, ch, "Skill must be enclosed in: ''\r\n");
    return;
  }
  std::string skillname(rest.substr(1, close - 1));
  for (char &c : skillname)
    c = tolower(c);
  if ((skill = find_skill_num(g, skillname.data(), SKTYPE_SKILL)) <= 0) {
    send_to_char(g, ch, "Unrecognized skill.\r\n");
    return;
  }
  rest.remove_prefix(close + 1); /* skip to next parameter */
  std::string buf = take_arg(rest);

  if (buf.empty()) {
    send_to_char(g, ch, "Learned value expected.\r\n");
    return;
  }
  value = parse_int(buf);
  if (value < 0) {
    send_to_char(g, ch, "Minimum value for learned is 0.\r\n");
    return;
  }

  /*
   * find_skill_num() guarantees a valid spell_info[] index, or -1, and we
   * checked for the -1 above so we are safe here.
   */
  char_set_skills_base(vict, skill, value);
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "skillset: {} changed {}'s '{}' to {:d}.", char_get_name(ch), char_get_name(vict), g->spell_info[skill].name, value);
  send_to_char(g, ch, "You change {}'s {} to {:d}.\r\n", char_get_name(vict), g->spell_info[skill].name, value);
}
