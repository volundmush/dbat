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
#include "dbat/oasis.hpp"
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
 * Exported ACMD do_oasis function.
 *
 * This function is the OLC interface.  It deals with all the 
 * generic OLC stuff, then passes control to the sub-olc sections.
 *
 * UPDATE:
 *  I believe that yes, putting the code together that is common in all of the
 *  olc functions is good to a certain extent, but the do_oasis command was
 *  getting ridiculous.  Therefore, I have separated them into separate
 *  functions that get called from in do_oasis....yes, similar code, but it is
 *  easier to handle....   - Kip Potter
 */
ACMD(do_oasis)
{
  /*
   * No screwing around as a mobile.
   */
  if (char_is_npc(ch) || !char_get_desc(ch))
    return;

  /*
   * Prevent forcing people in OLC to edit other stuff.
   * 'force' just lets command_interpreter() handle the input,
   * regardless of the state of the victim. 
   * This can wreck havoc if people are i OLC already 
   * - ie. their input should have been redirected by nanny(), and
   * never get to command_interpreter().
   * -- Welcor 09/03 
   * - thanks to Mark Garringer (zizazat@hotmail.com) for the bug report.
   */
  if (desc_get_connected(char_get_desc(ch)) != CON_PLAYING)
    return;

  switch (subcmd) {
      /* The command to see what needs to be saved, typically 'olc'.  */

    case SCMD_OASIS_CEDIT:
      do_oasis_cedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_ZEDIT:
      do_oasis_zedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_REDIT:
      do_oasis_redit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_OEDIT:
      do_oasis_oedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_MEDIT:
      do_oasis_medit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_SEDIT:
      do_oasis_sedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_AEDIT:
      do_oasis_aedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_HEDIT:
      do_oasis_hedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_HSEDIT:
      do_oasis_hsedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_RLIST:
    case SCMD_OASIS_MLIST:
    case SCMD_OASIS_OLIST:
    case SCMD_OASIS_SLIST:
    case SCMD_OASIS_ZLIST:
    case SCMD_OASIS_TLIST:
    case SCMD_OASIS_GLIST:
      do_oasis_list(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_TRIGEDIT:
      do_oasis_trigedit(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_LINKS:
      do_oasis_links(g, ch, argument, cmd, subcmd);
      break;

    case SCMD_OASIS_GEDIT:
      do_oasis_gedit(g, ch, argument, cmd, subcmd);
      break;

    default:
      mud_log("SYSERR: (OLC) Invalid subcmd passed to do_oasis, subcmd - ({:d})", subcmd);
      return;
  }

  return;
}

/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/

/*
 * This procedure frees up the strings and/or the structures
 * attatched to a descriptor, sets all flags back to how they
 * should be.
 */
void cleanup_olc(GameWorld *g, struct descriptor_data *d, int8_t cleanup_type)
{
  /*
   * Clean up WHAT?
   */
  if (d->olc == nullptr)
    return;

  /* an editor with a session of its own frees what it holds itself */
  if (!dynamic_cast<oasis_olc_data *>(d->olc.get())) {
    leave_olc(g, d);
    return;
  }

  /* cedit's copy of the configuration */
  if (OLC_CONFIG(d)) {
    delete OLC_CONFIG(d);
    OLC_CONFIG(d) = nullptr;
  }

  /* OLC_SHOP(d) is the editor's own copy, owning all it holds. */
  delete OLC_SHOP(d);
  /* OLC_GUILD(d) is the editor's own copy too. */
  delete OLC_GUILD(d);

  /*
   * Restore descriptor playing status.
   */
  if (d->character) {
    char_plr_set(d->character, PLR_WRITING, false);
    act(g, "$n stops using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);

    if (cleanup_type == CLEANUP_CONFIG)
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing the game configuration", char_get_name(d->character));
    else
      d->olc->log_leaving(g, d);

    desc_set_connected(d, CON_PLAYING);
  }

  d->olc.reset();
}

/*
 * This function is an exact duplicate of the tag_argument function found in
 * one of the ascii patches located on the circlemud ftp website.
 */
void split_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;

  for (; *tmp; tmp++) {
    if (*tmp != ' ' && *tmp != '=')
      *(ttag++) = *tmp;
    else if (*tmp == '=')
      break;
  }

  *ttag = '\0';

  while (*tmp == '=' || *tmp == ' ')
    tmp++;

  while (*tmp)
    *(wrt++) = *(tmp++);

  *wrt = '\0';
}

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

void oasis_olc_data::log_leaving(GameWorld *g, struct descriptor_data *d) const
{
  log_stops_editing(g, d, in_zone ? in_zone->number : NOWHERE);
}
