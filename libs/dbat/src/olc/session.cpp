/*
 * The editors' sessions: nanny()'s way into them, and, for the editors that share oasis_olc_data, their code found by
 * the connection's state.
 */
#include <algorithm>
#include <cstring>

#include "dbat/aedit.hpp"
#include "dbat/assedit.hpp"
#include "dbat/consts/constates.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/gedit.hpp"
#include "dbat/hedit.hpp"
#include "dbat/hsedit.hpp"
#include "dbat/oasis.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/tedit.hpp"

namespace {
  /* An editor still written the old way: what answers a line, and what runs when its text editor closes */
  struct legacy_editor {
    int state{};
    void (*parse)(GameWorld *g, struct descriptor_data *d, char *arg){};
    void (*text_done)(GameWorld *g, struct descriptor_data *d, int action){};
  };

  constexpr legacy_editor LEGACY_EDITORS[] = {
      {CON_OEDIT,    oedit_parse,    oedit_string_cleanup   },
      {CON_IEDIT,    oedit_parse,    oedit_string_cleanup   },
      {CON_ZEDIT,    zedit_parse,    nullptr                },
      {CON_SEDIT,    sedit_parse,    nullptr                },
      {CON_MEDIT,    medit_parse,    medit_string_cleanup   },
      {CON_REDIT,    redit_parse,    redit_string_cleanup   },
      {CON_CEDIT,    cedit_parse,    cedit_string_cleanup   },
      {CON_AEDIT,    aedit_parse,    nullptr                },
      {CON_TRIGEDIT, trigedit_parse, trigedit_string_cleanup},
      {CON_ASSEDIT,  assedit_parse,  nullptr                },
      {CON_GEDIT,    gedit_parse,    nullptr                },
      {CON_HEDIT,    hedit_parse,    hedit_string_cleanup   },
      {CON_HSEDIT,   hsedit_parse,   nullptr                },
      {CON_TEDIT,    nullptr,        tedit_string_cleanup   },
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
