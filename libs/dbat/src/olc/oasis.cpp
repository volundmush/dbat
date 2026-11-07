/************************************************************************
 * OasisOLC - General / oasis.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

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
#include "dbat/gengld.hpp"
#include "dbat/hsedit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
static void free_config(GameWorld *g, struct config_data *data);

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
      mud_log("SYSERR: (OLC) Invalid subcmd passed to do_oasis, subcmd - (%d)", subcmd);
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

  /*
   * Check for a room. free_room doesn't perform
   * sanity checks, we must be careful here.
   */
  if (OLC_ROOM(d)) {
    switch (cleanup_type) {
      case CLEANUP_ALL:
        /* free(OLC_SCRIPT(d)) equivalent */
        free_proto_script(OLC_ROOM(d), WLD_TRIGGER);
        free_room(g, OLC_ROOM(d));
        break;
      case CLEANUP_STRUCTS:
        free(OLC_ROOM(d));
        break;
      default: /* The caller has screwed up. */
        mud_log("SYSERR: cleanup_olc: Unknown type!");
        break;
    }
  }

  /* cedit's copy of the configuration (it has no room, so this can't go above) */
  if (OLC_CONFIG(d)) {
    free_config(g, OLC_CONFIG(d));
    OLC_CONFIG(d) = nullptr;
  }

  /*
   * Check for an existing object in the OLC.  The strings
   * aren't part of the prototype any longer.  They get added
   * with strdup().
   */
  if (OLC_OBJ(d)) {
    free_object_strings(g, OLC_OBJ(d));
    free(OLC_OBJ(d));
  }

  /*
   * Check for a mob.  free_mobile() makes sure strings are not in
   * the prototype.
   */
  if (OLC_MOB(d))
    free_mobile(g, OLC_MOB(d));

  /*
   * Check for a zone.  cleanup_type is irrelevant here, free() everything.
   */
  if (OLC_ZONE(d)) {
    if (OLC_ZONE(d)->builders)
      free(OLC_ZONE(d)->builders);
    if (OLC_ZONE(d)->name)
      free(OLC_ZONE(d)->name);
    if (OLC_ZONE(d)->cmd)
      free(OLC_ZONE(d)->cmd);
    free(OLC_ZONE(d));
  }

  /*
   * Check for a shop.  free_shop doesn't perform sanity checks, we must
   * be careful here.
   * OLC_SHOP(d) is a _copy_ - no pointers to the original. Just go ahead
   * and free it all.
   */
  if (OLC_SHOP(d)) {
    //    switch (cleanup_type) {
    //    case CLEANUP_ALL:
    free_shop(OLC_SHOP(d));
    //      break;
    //    case CLEANUP_STRUCTS:
    //      free(OLC_SHOP(d));
    //      break;
    //    default:
    /* The caller has screwed up but we already griped above. */
    //      break;
    //    }
  }
  /*. Check for a guild . */
  if (OLC_GUILD(d)) {
    switch (cleanup_type) {
      case CLEANUP_ALL:
        free_guild(OLC_GUILD(d));
        break;
      case CLEANUP_STRUCTS:
        free(OLC_GUILD(d));
        break;
      default:
        break;
    }
  }

  if (OLC_HOUSE(d)) { /*. free_house performs no sanity checks, must be careful here .*/
    switch (cleanup_type) {
      case CLEANUP_ALL:
        free_house(OLC_HOUSE(d)); /* (its contents: the house is the editor's own copy) */
        free(OLC_HOUSE(d));
        break;
      case CLEANUP_STRUCTS:
        free(OLC_HOUSE(d));
        break;
      default:
        /*. Caller has screwed up .*/
        break;
    }
  }

  /*. Check for aedit stuff -- M. Scott */
  if (OLC_ACTION(d)) {
    switch (cleanup_type) {
      case CLEANUP_ALL:
        free_action(OLC_ACTION(d)); /* (its strings: the action is the editor's own copy) */
        free(OLC_ACTION(d));
        break;
      case CLEANUP_STRUCTS:
        free(OLC_ACTION(d));
        break;
      default:
        /* Caller has screwed up */
        break;
    }
  }

  /* free storage if allocated (for tedit and aedit) */
  /* and Triggers */
  /* 
    * this is the command list - it's been copied to disk already,
    * so just free it -- Welcor
    */
  if (OLC_STORAGE(d)) {
    free(OLC_STORAGE(d));
    OLC_STORAGE(d) = nullptr;
  }
  /*
    * Free this one regardless. If we've left olc, we've either made
    * a fresh copy of it in the trig index, or we lost connection.
    * Either way, we need to get rid of this.
    */
  if (OLC_TRIG(d)) {
    free_trigger(g, OLC_TRIG(d));
    OLC_TRIG(d) = nullptr;
  }
  /*
    * OLC_SCRIPT is always set as trig_proto of OLC_OBJ/MOB/ROOM.
    * Therefore it should not be free'd here.
    */

  /* Only one person edits help at a time, and this one has stopped, whichever way they left. */
  if (desc_get_connected(d) == CON_HEDIT)
    g->HEDITS = false;

  /*
   * Restore descriptor playing status.
   */
  if (d->character) {
    char_plr_set(d->character, PLR_WRITING, false);
    act(g, "$n stops using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);

    if (cleanup_type == CLEANUP_CONFIG)
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s stops editing the game configuration", char_get_name(d->character));
    else if (desc_get_connected(d) == CON_TEDIT)
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s stops editing text files.", char_get_name(d->character));
    else if (desc_get_connected(d) == CON_HEDIT)
      mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: %s stops editing help files.", char_get_name(d->character));
    else
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s stops editing zone %d allowed zone %d", char_get_name(d->character), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, d->character));

    desc_set_connected(d, CON_PLAYING);
  }

  free(d->olc);
  d->olc = nullptr;
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

static void free_config(GameWorld *g, struct config_data *data)
{
  /****************************************************************************/
  /** Free strings.                                                          **/
  /****************************************************************************/
  free_strings(g, data, OASIS_CFG);

  /****************************************************************************/
  /** Free the data structure.                                               **/
  /****************************************************************************/
  free(data);
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
/**     The real number of the zone attempted to be modified.                **/
/**                                                                          **/
/** Returns        : Returns TRUE if the builder has access, otherwise       **/
/**                  FALSE.                                                  **/
/**                                                                          **/
/******************************************************************************/
bool can_edit_zone(GameWorld *g, struct char_data *ch, zone_rnum rnum)
{
  /* no access if called with bad arguments */
  if (!char_get_desc(ch) || char_is_npc(ch) || rnum == NOWHERE)
    return false;

  if (rnum == HEDIT_PERMISSION)
    return true;

  /* always access if ch is high enough level */
  if (char_get_admlevel(ch) >= ADMLVL_GRGOD)
    return (true);

  /* always access if a player helped build the zone in the first place */
  if (is_name(char_get_name(ch), g->zones[rnum].builders))
    return (true);

  /* no access if you haven't been assigned a zone */
  if (char_get_olc_zone(g, ch) == NOWHERE)
    return false;

  /* no access if you're not at least LVL_BUILDER */
  if (char_get_admlevel(ch) < ADMLVL_BUILDER)
    return false;

  /* always access if you're assigned to this zone */
  if (real_zone(g, char_get_olc_zone(g, ch)) == rnum)
    return true;

  return (false);
}

void send_cannot_edit(GameWorld *g, struct char_data *ch, zone_vnum zone)
{
  send_to_char(g, ch, "You do not have permission to edit zone %d.", zone);
  if (char_get_olc_zone(g, ch) != NOWHERE)
    send_to_char(g, ch, "  Try zone %d.", char_get_olc_zone(g, ch));
  send_to_char(g, ch, "\r\n");
  mudlog(g, LOG_BRF, ADMLVL_IMPL, true, "OLC: %s tried to edit zone %d allowed zone %d",
         char_get_name(ch), zone, char_get_olc_zone(g, ch));
}
