/*
 * The editors' sessions: nanny()'s way into them, and, for the editors that share oasis_olc_data, their code found by
 * the connection's state.
 */
#include <algorithm>
#include <cstring>

#include "dbat/consts/constates.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/admlevel.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/handler.hpp"
#include "dbat/log.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/gedit.hpp"
#include "dbat/oasis.hpp"
#include "dbat/olc/session.hpp"

namespace {
  /* An editor still written the old way: what answers a line, and what runs when its text editor closes */
  struct legacy_editor {
    int state{};
    void (*parse)(GameWorld *g, struct descriptor_data *d, char *arg){};
    void (*text_done)(GameWorld *g, struct descriptor_data *d, int action){};
  };

  constexpr legacy_editor LEGACY_EDITORS[] = {
      {CON_ZEDIT,    zedit_parse,    nullptr                },
      {CON_SEDIT,    sedit_parse,    nullptr                },
      {CON_CEDIT,    cedit_parse,    cedit_string_cleanup   },
      {CON_GEDIT,    gedit_parse,    nullptr                },
  };

  const legacy_editor *legacy_editor_for(const struct descriptor_data *d)
  {
    auto found = std::ranges::find(LEGACY_EDITORS, desc_get_connected(d), &legacy_editor::state);
    return found == std::end(LEGACY_EDITORS) ? nullptr : found;
  }

  /* The old editors take a C string they may change, in a buffer the size of nanny()'s */
  void legacy_parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const legacy_editor *editor = legacy_editor_for(d);
    if (!editor || !editor->parse)
      return;
    char arg[MAX_INPUT_LENGTH];
    line = line.substr(0, sizeof(arg) - 1);
    line.copy(arg, line.size());
    arg[line.size()] = '\0';
    editor->parse(g, d, arg);
  }
} // namespace

int olc_session::help_context(const struct descriptor_data *) const { return NOTHING; }

void open_session(GameWorld *g, struct descriptor_data *d, std::unique_ptr<olc_session> session, std::string_view who)
{
  if (d->olc && !who.empty())
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: {}: Player already had olc structure.", who);
  d->olc = std::move(session);
}

void begin_editing(GameWorld *g, struct descriptor_data *d, int state)
{
  desc_set_connected(d, state);
  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(d->character, PLR_WRITING, true);
}

void leave_olc(GameWorld *g, struct descriptor_data *d)
{
  if (!d->olc)
    return;
  if (d->character) {
    char_plr_set(d->character, PLR_WRITING, false);
    act(g, "$n stops using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
    d->olc->log_leaving(g, d);
    desc_set_connected(d, CON_PLAYING);
  }
  d->olc.reset();
}

void log_stops_editing(GameWorld *g, struct descriptor_data *d, zone_vnum zone)
{
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing zone {:d} allowed zone {:d}", char_get_name(d->character), zone,
         char_get_olc_zone(g, d->character));
}

void olc_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  if (d->olc)
    d->olc->parse(g, d, arg);
  else
    legacy_parse(g, d, arg); /* as nanny() would have called it */
}

void oasis_olc_data::parse(GameWorld *g, struct descriptor_data *d, std::string_view line) { legacy_parse(g, d, line); }

void oasis_olc_data::text_done(GameWorld *g, struct descriptor_data *d, int how)
{
  if (const legacy_editor *editor = legacy_editor_for(d); editor && editor->text_done)
    editor->text_done(g, d, how);
}

int oasis_olc_data::help_context(const struct descriptor_data *d) const { return legacy_help_context(d); }
