/**************************************************************************
*  File: dg_handler.c                                                     *
*                                                                         *
*  Usage: contains functions to handle memory for scripts.                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
***************************************************************************/
#include <cstdlib>
#include <cerrno>

#include <linux/limits.h>

#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/fileop.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

/* returns *slot, first creating an empty script there if there is none */
struct script_data *dg_ensure_script(struct script_data **slot)
{
  if (!*slot)
    *slot = new script_data();
  return *slot;
}

/* where go keeps its script pointer */
struct script_data **script_slot(owner_ptr go)
{
  return std::visit(overloaded{[](char_data *ch) { return char_script_slot(ch); },
                               [](obj_data *obj) { return obj_script_slot(obj); },
                               [](room_data *room) { return room_script_slot(room); }},
                    go);
}

/*
 * Return memory used by a trigger
 * The command list is free'd when changed and when
 * shutting down.
 */
void free_trigger(GameWorld *g, struct trig_data *trig)
{
  trig->name.clear();
  trig->arglist.clear();
  if (trig->wait_event)
    event_cancel(g, trig->wait_event);

  delete trig;
}

/* remove a single trigger from a mob/obj/room */
void extract_trigger(GameWorld *g, struct trig_data *trig)
{
  if (trig->wait_event) {
    event_cancel(g, trig->wait_event);
    trig->wait_event = nullptr;
  }

  trig->index->number--;

  g->triggers.remove(trig->id, trig);

  /* A running script may be this trigger, or be about to read it, so it can't go yet. */
  if (g->script_depth > 0) {
    trig->extracted = true;
    g->extracted_triggers.push_back(trig);
    return;
  }
  free_trigger(g, trig);
}

/* Frees the triggers extract_trigger() left, once no script can be using them. */
void free_extracted_triggers(GameWorld *g)
{
  if (g->script_depth > 0)
    return;
  std::vector<struct trig_data *> extracted;
  extracted.swap(g->extracted_triggers);
  for (struct trig_data *trig : extracted)
    free_trigger(g, trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(GameWorld *g, owner_ptr thing)
{
  struct script_data **slot = script_slot(thing);
  struct script_data *sc    = *slot;
  struct trig_data *trig, *next_trig;

  if (!sc)
    return;
  *slot = nullptr;

  for (trig = sc->trig_list; trig; trig = next_trig) {
    next_trig = trig->next;
    trig->owner_gone = true;
    extract_trigger(g, trig);
  }
  sc->trig_list = nullptr;

  delete sc;
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
  struct script_memory *next;
  while (sc) {
    next = sc->next;
    delete sc;
    sc = next;
  }
}

/* Where thing keeps the vnums of the triggers it starts with */
std::vector<trig_vnum> &proto_script_of(owner_ptr thing)
{
  return std::visit(overloaded{[](char_data *mob) -> std::vector<trig_vnum> & { return char_proto_script(mob); },
                               [](obj_data *obj) -> std::vector<trig_vnum> & { return obj_proto_script(obj); },
                               [](room_data *room) -> std::vector<trig_vnum> & { return room_proto_script(room); }},
                    thing);
}

/* source and dest are of the same kind */
void copy_proto_script(owner_ptr source, owner_ptr dest)
{
  proto_script_of(dest) = proto_script_of(source);
}

void delete_variables(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), SCRIPT_VARS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    mud_log("SYSERR: deleting variable file {}: {}", filename, strerror(errno));
}
