/*
 * The functions below this point is handling the context sensitive help system
 * If you add more olc options, be sure to add a case for it here. - Welcor
 */
#include <cstdlib>
#include <cerrno>

#include "dbat/descriptor_utils.hpp"
#include "dbat/context_help.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/oasis.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/fileop.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/db.hpp"
#include "dbat/log.hpp"
#include "dbat/game_world.hpp"


static const char NO_HELP[]        = "No help available (yet)!\r\n";
constexpr int FIND_HELP_CHAR = '*';

bool context_help(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int context = NOTHING;
  char actbuf[MAX_INPUT_LENGTH], *tmp;
  /* skip if context help isn't wanted */
  if (strncmp(arg, CONTEXT_HELP_STRING, strlen(CONTEXT_HELP_STRING)))
    return false;

  tmp = one_argument(arg, actbuf); /* the totally useless 'help' string.. */
  skip_spaces(&tmp);

  if (!*tmp)
    context = d->olc ? d->olc->help_context(d) : NOTHING;

  if (context != NOTHING && context < NUM_CONTEXTS && *g->context_help_list[context]) {
    if (*g->context_help_list[context] == FIND_HELP_CHAR) {
      copy_cstr(actbuf, g->context_help_list[context] + 1, sizeof(actbuf));
      do_help(g, d->character, actbuf, 0, 0);
    } else {
      write_to_output(g, d, "\r\n{}\r\n>  ", g->context_help_list[context]);
    }
    return true;
  }
  copy_cstr(actbuf, tmp, sizeof(actbuf));
  do_help(g, d->character, actbuf, 0, 0);
  return true;
}

/* The context help file, text/contexthelp: "#<context>" and its text up to a '~', for each context that has one */
void boot_context_help(GameWorld *g)
{
  /* init to 'no help' string */
  for (int i = 0; i < NUM_CONTEXTS; i++)
    g->context_help_list[i] = const_cast<char *>(NO_HELP); /* never freed: see below */

  io::loaded<io::text_reader> file = io::text_reader::open(CONTEXT_HELP_FILE);
  if (!file) {
    mud_log("No context help found : {}", io::why_unreadable(CONTEXT_HELP_FILE));
    return;
  }
  while (std::optional<std::string_view> line = file->line()) {
    int num = 0;
    if (!line->starts_with('#') || !io::fields(line->substr(1)).number(num) || num < 0 || num >= NUM_CONTEXTS)
      continue;
    io::loaded<std::string> text = file->tilde_string("Context sensitive help");
    if (!text) {
      mud_log("SYSERR: {}", text.error().text());
      exit(1);
    }
    /* an empty one is no help, as it was (it was a null, which showing it would crash on) */
    if (text->empty())
      continue;
    if (g->context_help_list[num] != NO_HELP)
      free(g->context_help_list[num]);
    g->context_help_list[num] = strdup(text->c_str());
  }
}

void free_context_help(GameWorld *g)
{
  int i;

  for (i = 0; i < NUM_CONTEXTS; i++)
    if (g->context_help_list[i] && g->context_help_list[i] != NO_HELP)
      free(g->context_help_list[i]);
}
