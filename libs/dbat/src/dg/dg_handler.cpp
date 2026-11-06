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
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/stringutils.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/fileop.h"
#include "dg_internal.h"

#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>
#include "dbat/game_world.h"

/* frees memory associated with var */
static void free_var_el(struct trig_var_data *var)
{
  if (var->name)
    free(var->name);
  if (var->value)
    free(var->value);
  free(var);
}

/* returns *slot, first creating an empty script there if there is none */
struct script_data *dg_ensure_script(struct script_data **slot)
{
  if (!*slot)
    CREATE(*slot, struct script_data, 1);
  return *slot;
}

/* where go keeps its script pointer: go is a mob, object or room according to type */
struct script_data **script_slot(void *go, int type)
{
  switch (type) {
    case MOB_TRIGGER: return char_script_slot((char_data *)go);
    case OBJ_TRIGGER: return obj_script_slot((obj_data *)go);
    case WLD_TRIGGER: return room_script_slot((room_data *)go);
  }
  return nullptr;
}

/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd)
{
  struct trig_var_data *i, *j;

  for (i = vd; i;) {
    j = i;
    i = i->next;
    free_var_el(j);
  }
}

/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
bool remove_var(struct trig_var_data **var_list, char *name)
{
  struct trig_var_data *i, *j;

  for (j = nullptr, i = *var_list; i && strcasecmp(name, i->name);
       j = i, i = i->next);

  if (i) {
    if (j) {
      j->next = i->next;
      free_var_el(i);
    } else {
      *var_list = i->next;
      free_var_el(i);
    }

    return true;
  }

  return false;
}

/*
 * Return memory used by a trigger
 * The command list is free'd when changed and when
 * shutting down.
 */
void free_trigger(GameWorld *g, struct trig_data *trig)
{
  free(trig->name);
  trig->name = nullptr;

  if (trig->arglist) {
    free(trig->arglist);
    trig->arglist = nullptr;
  }
  if (trig->var_list) {
    free_varlist(trig->var_list);
    trig->var_list = nullptr;
  }
  if (trig->wait_event)
    event_cancel(g, trig->wait_event);

  free(trig);
}

/* remove a single trigger from a mob/obj/room */
void extract_trigger(GameWorld *g, struct trig_data *trig)
{
  struct trig_data *temp;

  if (trig->wait_event) {
    event_cancel(g, trig->wait_event);
    trig->wait_event = nullptr;
  }

  g->trig_index[trig->nr]->number--;

  /* walk the trigger list and remove this one */
  REMOVE_FROM_LIST(trig, g->trigger_list, next_in_world, temp);

  /* A running script may be this trigger, or be about to read it, so it can't go yet. */
  if (g->script_depth > 0) {
    trig->extracted     = true;
    trig->next_in_world = g->extracted_triggers;
    g->extracted_triggers = trig;
    return;
  }
  free_trigger(g, trig);
}

/* Frees the triggers extract_trigger() left, once no script can be using them. */
void free_extracted_triggers(GameWorld *g)
{
  struct trig_data *next;

  if (g->script_depth > 0)
    return;
  for (struct trig_data *trig = g->extracted_triggers; trig; trig = next) {
    next = trig->next_in_world;
    free_trigger(g, trig);
  }
  g->extracted_triggers = nullptr;
}

/* remove all triggers from a mob/obj/room */
void extract_script(GameWorld *g, void *thing, int type)
{
  struct script_data **slot = script_slot(thing, type);
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

  /* Thanks to James Long for tracking down this memory leak */
  free_varlist(sc->global_vars);

  free(sc);
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
  struct script_memory *next;
  while (sc) {
    next = sc->next;
    if (sc->cmd) free(sc->cmd);
    free(sc);
    sc = next;
  }
}

void free_proto_script(void *thing, int type)
{
  struct trig_proto_list *proto = nullptr, *fproto;
  char_data *mob;
  obj_data *obj;
  room_data *room;

  switch (type) {
    case MOB_TRIGGER:
      mob               = (struct char_data *)thing;
      proto             = char_get_proto_script(mob);
      char_set_proto_script(mob, nullptr);
      break;
    case OBJ_TRIGGER:
      obj               = (struct obj_data *)thing;
      proto             = obj_get_proto_script(obj);
      obj_set_proto_script(obj, nullptr);
      break;
    case WLD_TRIGGER:
      room               = (struct room_data *)thing;
      proto              = room_get_proto_script(room);
      room_set_proto_script(room, nullptr);
      break;
  }

  while (proto) {
    fproto = proto;
    proto  = proto->next;
    free(fproto);
  }
}

void copy_proto_script(void *source, void *dest, int type)
{
  struct trig_proto_list *tp_src = nullptr, *tp_dst = nullptr;

  switch (type) {
    case MOB_TRIGGER:
      tp_src = char_get_proto_script((char_data *)source);
      break;
    case OBJ_TRIGGER:
      tp_src = obj_get_proto_script((obj_data *)source);
      break;
    case WLD_TRIGGER:
      tp_src = room_get_proto_script((room_data *)source);
      break;
  }

  if (tp_src) {
    CREATE(tp_dst, struct trig_proto_list, 1);
    switch (type) {
      case MOB_TRIGGER:
        char_set_proto_script((char_data *)dest, tp_dst);
        break;
      case OBJ_TRIGGER:
        obj_set_proto_script((obj_data *)dest, tp_dst);
        break;
      case WLD_TRIGGER:
        room_set_proto_script((room_data *)dest, tp_dst);
        break;
    }

    while (tp_src) {
      tp_dst->vnum = tp_src->vnum;
      tp_src       = tp_src->next;
      if (tp_src)
        CREATE(tp_dst->next, struct trig_proto_list, 1);
      tp_dst = tp_dst->next;
    }
  }
}

void delete_variables(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), SCRIPT_VARS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    mud_log("SYSERR: deleting variable file %s: %s", filename, strerror(errno));
}

void update_wait_events(struct room_data *to, struct room_data *from)
{
  struct trig_data *trig;

  if (!room_get_script(from))
    return;

  for (trig = room_get_script(from)->trig_list; trig; trig = trig->next) {
    if (!trig->wait_event)
      continue;

    ((struct wait_event_data *)trig->wait_event->event_obj)->go = to;
  }
}
