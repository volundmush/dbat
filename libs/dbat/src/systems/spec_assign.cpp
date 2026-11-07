/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/config.hpp"
#include "dbat/spec_assign.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/utils.hpp"
#include "dbat/spec_procs.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void ASSIGNROOM(GameWorld *g, room_vnum room, SPECIAL(fname));
static void ASSIGNMOB(GameWorld *g, mob_vnum mob, SPECIAL(fname));
static void ASSIGNOBJ(GameWorld *g, obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

static void ASSIGNMOB(GameWorld *g, mob_vnum mob, SPECIAL(fn))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(g, mob)) != NOBODY)
    g->mob_index[rnum].func = fn;
  else if (!g->mini_mud)
    mud_log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

static void ASSIGNOBJ(GameWorld *g, obj_vnum obj, SPECIAL(fn))
{
  obj_rnum rnum;

  if ((rnum = real_object(g, obj)) != NOTHING)
    g->obj_index[rnum].func = fn;
  else if (!g->mini_mud)
    mud_log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

static void ASSIGNROOM(GameWorld *g, room_vnum room, SPECIAL(fn))
{
  room_rnum rnum;

  if ((rnum = real_room(g, room)) != NOWHERE)
    rroom_set_func(g, rnum, fn);
  else if (!g->mini_mud)
    mud_log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(GameWorld *g)
{
  ASSIGNMOB(g, 3010, postmaster);
}

/* assign special procedures to objects */
void assign_objects(GameWorld *g)
{
  ASSIGNOBJ(g, 3034, bank);   /* atm */
  ASSIGNOBJ(g, 3036, bank);   /* cashcard */
  ASSIGNOBJ(g, 11, gravity);  /* gravity generator */
  ASSIGNOBJ(g, 65, healtank); /* Healing Tank */
  ASSIGNOBJ(g, 3, augmenter); /* Augmenter 9001 */
}

/* assign special procedures to rooms */
void assign_rooms(GameWorld *g)
{
  room_rnum i;

  ASSIGNROOM(g, 5, dump);
  ASSIGNROOM(g, 3, pet_shops);
  ASSIGNROOM(g, 4, pet_shops);
  ASSIGNROOM(g, 81, auction);
  ASSIGNROOM(g, 82, auction);
  ASSIGNROOM(g, 83, auction);
  ASSIGNROOM(g, 84, auction);
  ASSIGNROOM(g, 85, auction);
  ASSIGNROOM(g, 86, auction);
  /* Gauntlet rooms track how far a player progressed into zone  Jamdog - 13th Feb 2006 */

  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= g->top_room; i++)
      if (rroom_flagged(g, i, ROOM_DEATH))
        rroom_set_func(g, i, dump);
}
