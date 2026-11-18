/* ***********************************************************************
*  File: alias.c				A utility to CircleMUD	 *
* Usage: writing/reading player's aliases.				 *
*									 *
* Code done by Jeremy Hess and Chad Thompson				 *
* Modifed by George Greer for inclusion into CircleMUD bpl15.		 *
*									 *
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		 *
*********************************************************************** */

#include <cerrno>
#include <cstdlib>


#include "dbat/args.hpp"
#include "dbat/alias.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/fileop.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/game_world.hpp"

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  struct alias_data *a;
  std::list<alias_data> *aliases;

  if (char_is_npc(ch) || !(aliases = char_aliases(g, ch)))
    return;

  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string repl(rest); /* the rest of the line, from the space after the alias */

  if (arg.empty()) { /* no argument specified -- list currently defined aliases */
    send_to_char(g, ch, "Currently defined aliases:\r\n");
    if (aliases->empty())
      send_to_char(g, ch, " None.\r\n");
    else
      for (const struct alias_data &each : *aliases)
        send_to_char(g, ch, "{:<15} {}\r\n", each.alias, each.replacement);
  } else { /* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    bool had = (a = find_alias(*aliases, arg)) != nullptr;
    if (had)
      aliases->remove_if([a](const alias_data &each) { return &each == a; });
    /* if no replacement string is specified, assume we want to delete */
    if (repl.empty()) {
      if (!had)
        send_to_char(g, ch, "No such alias.\r\n");
      else
        send_to_char(g, ch, "Alias deleted.\r\n");
    } else { /* otherwise, either add or redefine an alias */
      if (str_eq_nocase(arg, "alias")) {
        send_to_char(g, ch, "You can't alias 'alias'.\r\n");
        return;
      }
      struct alias_data added;
      added.alias = arg;
      delete_doubledollar(repl);
      added.replacement = repl;
      if (repl.find(ALIAS_SEP_CHAR) != std::string::npos || repl.find(ALIAS_VAR_CHAR) != std::string::npos)
        added.type = ALIAS_COMPLEX;
      else
        added.type = ALIAS_SIMPLE;
      aliases->push_front(std::move(added));
      send_to_char(g, ch, "Alias added.\r\n");
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
constexpr int NUM_TOKENS = 9;

/* Copies text to *write_point, stopping at end (the last byte of the buffer, kept for the NUL). */
static void alias_append(char **write_point, char *end, const char *text)
{
  while (*text && *write_point < end)
    *(*write_point)++ = *text++;
}

static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  const char *rep;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH]; /* raw? */
  char *end         = buf + sizeof(buf) - 1;                  /* $* and $1..$9 can expand past any buffer */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  copy_cstr(buf2, orig, sizeof(buf2));
  temp = strtok(buf2, " ");
  while (temp != nullptr && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp                    = strtok(nullptr, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue  = txt_q{};

  /* now parse the alias */
  for (rep = a->replacement.c_str(); *rep; rep++) {
    if (*rep == ALIAS_SEP_CHAR) {
      *write_point              = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*rep == ALIAS_VAR_CHAR) {
      rep++;
      if (!*rep) { /* a '$' at the very end: keep it, and don't step past the string */
        alias_append(&write_point, end, "$");
        break;
      }
      if ((num = *rep - '1') < num_of_tokens && num >= 0)
        alias_append(&write_point, end, tokens[num]);
      else if (*rep == ALIAS_GLOB_CHAR)
        alias_append(&write_point, end, orig);
      else {
        char literal[2] = {*rep, '\0'};
        alias_append(&write_point, end, literal);
        if (*rep == '$') /* redouble $ for act safety */
          alias_append(&write_point, end, "$");
      }
    } else if (write_point < end)
      *(write_point++) = *rep;
  }

  *write_point              = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  input_q->lines.insert(input_q->lines.begin(), temp_queue.lines.begin(), temp_queue.lines.end());
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
bool perform_alias(GameWorld *g, struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a;
  std::list<alias_data> *aliases;

  /* Mobs don't have alaises. */
  if (char_is_npc(d->character))
    return false;

  /* bail out immediately if the guy doesn't have any aliases */
  if (!(aliases = char_aliases(g, d->character)) || aliases->empty())
    return false;

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return false;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(*aliases, first_arg)) == nullptr)
    return false;

  if (a->type == ALIAS_SIMPLE) {
    copy_cstr(orig, a->replacement.c_str(), maxlen);
    return false;
  }
  perform_complex_alias(&d->input, ptr, a);
  return true;
}