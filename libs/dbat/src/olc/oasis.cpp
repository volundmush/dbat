/************************************************************************
 * OasisOLC - General / oasis.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <cstdlib>

#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/shop.hpp"
#include "dbat/genmob.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/genobj.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/handler.hpp"
#include "dbat/guild.hpp"
#include "dbat/aedit.hpp"
#include "dbat/hedit.hpp"
#include "dbat/medit.hpp"
#include "dbat/sedit.hpp"
#include "dbat/act.social.hpp"
#include "dbat/social.hpp"
#include "dbat/gengld.hpp"
#include "dbat/hsedit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/olc/commands.hpp"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/

/* -------------------------------------------------------------------------- */

/*
 * Only player characters should be using OLC anyway.
 */
void clear_screen(GameWorld *g, struct descriptor_data *d)
{
  if (char_prf_flagged(g, d->character, PRF_CLS))
    write_to_output(g, d, "[H[J");
}

/* -------------------------------------------------------------------------- */

/*
 * The OLC commands, as their subcommands reach do_oasis(). Each editor's command does its own work; this only keeps a
 * mob, or someone not playing, from starting one.
 */
namespace {
  constexpr std::pair<int, void (*)(GameWorld *, struct char_data *, std::string_view, int, int)> OASIS_COMMANDS[] = {
      {SCMD_OASIS_CEDIT,    do_oasis_cedit   },
      {SCMD_OASIS_ZEDIT,    do_oasis_zedit   },
      {SCMD_OASIS_REDIT,    do_oasis_redit   },
      {SCMD_OASIS_OEDIT,    do_oasis_oedit   },
      {SCMD_OASIS_MEDIT,    do_oasis_medit   },
      {SCMD_OASIS_SEDIT,    do_oasis_sedit   },
      {SCMD_OASIS_AEDIT,    do_oasis_aedit   },
      {SCMD_OASIS_HEDIT,    do_oasis_hedit   },
      {SCMD_OASIS_HSEDIT,   do_oasis_hsedit  },
      {SCMD_OASIS_RLIST,    do_oasis_list    },
      {SCMD_OASIS_MLIST,    do_oasis_list    },
      {SCMD_OASIS_OLIST,    do_oasis_list    },
      {SCMD_OASIS_SLIST,    do_oasis_list    },
      {SCMD_OASIS_ZLIST,    do_oasis_list    },
      {SCMD_OASIS_TLIST,    do_oasis_list    },
      {SCMD_OASIS_GLIST,    do_oasis_list    },
      {SCMD_OASIS_TRIGEDIT, do_oasis_trigedit},
      {SCMD_OASIS_LINKS,    do_oasis_links   },
      {SCMD_OASIS_GEDIT,    do_oasis_gedit   },
  };
} // namespace

ACMD(do_oasis)
{
  /* No screwing around as a mobile. */
  if (char_is_npc(ch) || !char_get_desc(ch))
    return;

  /*
   * Prevent forcing people in OLC to edit other stuff: 'force' hands the line to command_interpreter() whatever the
   * victim's state, and someone in OLC has their input go to nanny() instead. -- Welcor 09/03 (thanks to Mark Garringer
   * for the bug report)
   */
  if (desc_get_connected(char_get_desc(ch)) != CON_PLAYING)
    return;

  auto command = std::ranges::find(OASIS_COMMANDS, subcmd, &std::pair<int, void (*)(GameWorld *, struct char_data *, std::string_view, int, int)>::first);
  if (command == std::end(OASIS_COMMANDS)) {
    mud_log("SYSERR: (OLC) Invalid subcmd passed to do_oasis, subcmd - ({:d})", subcmd);
    return;
  }
  command->second(g, ch, argument, cmd, subcmd);
}

/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/

/******************************************************************************/
/**                                                                          **/
/** Function       : can_edit_zone()                                         **/
/**                                                                          **/
/** Description    : Checks to see if a builder can modify the specified     **/
/**                  zone.                                                   **/
/**                                                                          **/
/** Arguments      :                                                         **/
/**   ch                                                                     **/
/**     The character requesting access to modify this zone.                 **/
/**   rnum                                                                   **/
/**     The zone attempted to be modified.                                   **/
/**                                                                          **/
/** Returns        : Returns TRUE if the builder has access, otherwise       **/
/**                  FALSE.                                                  **/
/**                                                                          **/
/******************************************************************************/
bool can_edit_zone(GameWorld *g, struct char_data *ch, struct zone_data *rnum)
{
  /* no access if called with bad arguments */
  if (!char_get_desc(ch) || char_is_npc(ch) || rnum == nullptr)
    return false;

  /* always access if ch is high enough level */
  if (char_get_admlevel(ch) >= ADMLVL_GRGOD)
    return (true);

  /* always access if a player helped build the zone in the first place */
  if (is_name(char_get_name(ch), rnum->builders.c_str()))
    return (true);

  /* no access if you haven't been assigned a zone */
  if (char_get_olc_zone(g, ch) == NOWHERE)
    return false;

  /* no access if you're not at least LVL_BUILDER */
  if (char_get_admlevel(ch) < ADMLVL_BUILDER)
    return false;

  /* always access if you're assigned to this zone */
  if (find_zone(g, char_get_olc_zone(g, ch)) == rnum)
    return true;

  return (false);
}

void send_cannot_edit(GameWorld *g, struct char_data *ch, zone_vnum zone)
{
  send_to_char(g, ch, "You do not have permission to edit zone {:d}.", zone);
  if (char_get_olc_zone(g, ch) != NOWHERE)
    send_to_char(g, ch, "  Try zone {:d}.", char_get_olc_zone(g, ch));
  send_to_char(g, ch, "\r\n");
  mudlog(g, LOG_BRF, ADMLVL_IMPL, true, "OLC: {} tried to edit zone {:d} allowed zone {:d}",
         char_get_name(ch), zone, char_get_olc_zone(g, ch));
}
