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
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>


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

/* One line of a complex alias as it is built: at most MAX_RAW_INPUT_LENGTH - 1 long, then queued cut to a command's
   length */
struct alias_line {
  std::string text;

  void append(std::string_view more)
  {
    text.append(more.substr(0, std::min(more.size(), (size_t)MAX_RAW_INPUT_LENGTH - 1 - text.size())));
  }
  void queue(struct txt_q *q) const { write_to_q(std::string_view(text).substr(0, MAX_INPUT_LENGTH - 1), q, true); }
};

static void perform_complex_alias(struct txt_q *input_q, std::string_view orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  std::vector<std::string_view> tokens;
  alias_line line;

  /* First, parse the original string: its words, split at spaces */
  std::string_view words = orig.substr(0, std::min(orig.size(), (size_t)MAX_RAW_INPUT_LENGTH - 1));
  for (size_t at = 0; tokens.size() < NUM_TOKENS;) {
    at = words.find_first_not_of(' ', at);
    if (at == std::string_view::npos)
      break;
    size_t end = std::min(words.find(' ', at), words.size());
    tokens.push_back(words.substr(at, end - at));
    at = end;
  }

  /* now parse the alias */
  const std::string &rep = a->replacement;
  for (size_t i = 0; i < rep.size(); i++) {
    if (rep[i] == ALIAS_SEP_CHAR) {
      line.queue(&temp_queue);
      line.text.clear();
    } else if (rep[i] == ALIAS_VAR_CHAR) {
      if (++i == rep.size()) { /* a '$' at the very end: keep it */
        line.append("$");
        break;
      }
      if (rep[i] >= '1' && (size_t)(rep[i] - '1') < tokens.size())
        line.append(tokens[rep[i] - '1']);
      else if (rep[i] == ALIAS_GLOB_CHAR)
        line.append(orig);
      else {
        line.append(std::string_view(&rep[i], 1));
        if (rep[i] == '$') /* redouble $ for act safety */
          line.append("$");
      }
    } else
      line.append(std::string_view(&rep[i], 1));
  }
  line.queue(&temp_queue);

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
bool perform_alias(GameWorld *g, struct descriptor_data *d, std::string &line)
{
  struct alias_data *a;
  std::list<alias_data> *aliases;

  /* Mobs don't have alaises. */
  if (char_is_npc(d->character))
    return false;

  /* bail out immediately if the guy doesn't have any aliases */
  if (!(aliases = char_aliases(g, d->character)) || aliases->empty())
    return false;

  /* find the alias we're supposed to match */
  std::string_view rest = line;
  std::string first_arg = take_word(rest);

  /* bail out if it's null */
  if (first_arg.empty())
    return false;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(*aliases, first_arg)) == nullptr)
    return false;

  if (a->type == ALIAS_SIMPLE) {
    line = a->replacement.substr(0, MAX_INPUT_LENGTH - 1);
    return false;
  }
  perform_complex_alias(&d->input, rest, a);
  return true;
}
