/************************************************************************
 * Generic OLC Library - Rooms / genwld.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/db/htree.h"
#include "dbat/db/shops.h"
#include "dbat/game/dg_scripts.h"

#include "dbat/game/genwld.h"
#include "dbat/game/utils.h"
#include "dbat/game/db.h"
#include "dbat/game/handler.h"
#include "dbat/game/comm.h"
#include "dbat/game/genolc.h"
#include "dbat/game/genwld.h"
#include "dbat/game/genzon.h"
#include "dbat/game/shop.h"
#include "dbat/game/dg_olc.h"
#include "dbat/db/iterate.hpp"


/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
room_vnum add_room(struct room_data *room)
{
  struct char_data *tch;
  struct obj_data *tobj;
  int j, found = FALSE;
  room_rnum i;

  if (!room)
    return NOWHERE;

  if (auto irm = room_by_id(room->number)) {
    if (SCRIPT(irm))
      extract_script(irm, WLD_TRIGGER);
    tch = irm->people; 
    tobj = irm->contents;
    copy_room(irm, room);
    irm->people = tch;
    irm->contents = tobj;
    add_to_save_list(room_zone_vnum_get(room), SL_WLD);
    log("GenOLC: add_room: Updated existing room #%d.", room->number);
    return room->number;
  }

  struct room_data *new_room;
  CREATE(new_room, struct room_data, 1);
  copy_room(new_room, room);
  room_put(room->number, new_room);

  log("GenOLC: add_room: Added room %d.", room->number);


  add_to_save_list(room_zone_vnum_get(room), SL_WLD);

  /*
   * Return what array entry we placed the new room in.
   */
  return found;
}

/* -------------------------------------------------------------------------- */

int delete_room(room_vnum vnum)
{
  room_rnum i;
  int j;
  struct char_data *ppl, *next_ppl;
  struct obj_data *obj, *next_obj;
  struct room_data *room = room_by_id(vnum);

  if (!room) /* Can't delete void yet. */
    return FALSE;

  add_to_save_list(room_zone_vnum_get(room), SL_WLD);

  /* This is something you might want to read about in the logs. */
  log("GenOLC: delete_room: Deleting room #%d (%s).", room->number, room->name);

  /*
   * Dump the contents of this room into the Void.  We could also just
   * extract the people, mobs, and objects here.
   */
  for (obj = room->contents; obj; obj = next_obj)
  {
    next_obj = obj->next_content;
    obj_from_room(obj);
    obj_to_room(obj, 0);
  }
  for (ppl = room->people; ppl; ppl = next_ppl)
  {
    next_ppl = ppl->next_in_room;
    char_from_room(ppl);
    char_to_room(ppl, 0);
  }

  free_room_strings(room);
  if (SCRIPT(room))
    extract_script(room, WLD_TRIGGER);
  free_proto_script(room, WLD_TRIGGER);

  room_iterate([&](auto other_room)
               {
    for (j = 0; j < NUM_OF_DIRS; j++) {
      auto ex = R_EXIT(other_room, j);
      if(!ex) continue;
      if(ex->to_room != vnum) continue;
      if ((!ex->keyword || !*ex->keyword) && (!ex->general_description || !*ex->general_description)) {
      /* no description, remove exit completely */
      if (ex->keyword)
        free(ex->keyword);
      if (ex->general_description)
        free(ex->general_description);
      free(ex);
      R_EXIT(other_room, j) = NULL;
    } else { 
      /* description is set, just point to nowhere */
      ex->to_room = NOWHERE;
    }
    }
    return true; });

  /*
   * Find what zone that room was in so we can update the loading table.
   */
  zone_iterate([&](auto zone) {
    for (j = 0; zone->cmd[j].command != 'S'; j++)
      switch (zone->cmd[j].command)
      {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        if (zone->cmd[j].arg3 == vnum)
          zone->cmd[j].command = '*'; /* Cancel command. */
        break;
      case 'D':
      case 'R':
        if (zone->cmd[j].arg1 == vnum)
          zone->cmd[j].command = '*'; /* Cancel command. */
      case 'G':
      case 'P':
      case 'E':
      case '*':
        /* Known zone entries we don't care about. */
        break;
      default:
        mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: GenOLC: delete_room: Unknown zone entry found!");
      }
      return true; });

  /*
   * Remove this room from all shop lists.
   */
  {
    extern int top_shop;
    for (i = 0; i < top_shop; i++)
    {
      for (j = 0; SHOP_ROOM(i, j) != NOWHERE; j++)
      {
        if (SHOP_ROOM(i, j) == vnum)
          SHOP_ROOM(i, j) = 0; /* set to the void */
      }
    }
  }

  // Remove from Zig.
  free(room);
  room_delete(vnum);

  return TRUE;
}

int save_rooms(struct zone_data *zone)
{
  int i;
  FILE *sf;
  char filename[128];
  char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
  char rbuf1[MAX_STRING_LENGTH], rbuf2[MAX_STRING_LENGTH];
  char rbuf3[MAX_STRING_LENGTH], rbuf4[MAX_STRING_LENGTH];

 if(!zone) {
    log("SYSERR: GenOLC: save_rooms: Invalid zone!");
    return FALSE;
  }

  log("GenOLC: save_rooms: Saving rooms in zone #%d (%d-%d).",
      zone->number, zone->bot, zone->top);

  snprintf(filename, sizeof(filename), "%s%d.new", WLD_PREFIX, zone->number);
  if (!(sf = fopen(filename, "w")))
  {
    perror("SYSERR: save_rooms");
    return FALSE;
  }

  for (i = zone->bot; i <= zone->top; i++)
  {
    auto room = room_by_id(i);
    if (!room)
      continue;

    int j;

    /*
     * Copy the description and strip off trailing newlines.
     */
    strncpy(buf, room->description ? room->description : "Empty room.", sizeof(buf) - 1);
    strip_cr(buf);

    /*
     * Save the numeric and string section of the file.
     */
    sprintascii(rbuf1, room->room_flags[0]);
    sprintascii(rbuf2, room->room_flags[1]);
    sprintascii(rbuf3, room->room_flags[2]);
    sprintascii(rbuf4, room->room_flags[3]);
    fprintf(sf, "#%d\n"
                "%s%c\n"
                "%s%c\n"
                "%d %s %s %s %s %d\n",
            room->number,
            room->name ? room->name : "Untitled", STRING_TERMINATOR,
            buf, STRING_TERMINATOR,
            room->zone,
            rbuf1, rbuf2, rbuf3, rbuf4, room->sector_type);

    /*
     * Now you write out the exits for the room.
     */
    for (j = 0; j < NUM_OF_DIRS; j++)
    {
      auto ex = R_EXIT(room, j);
      if (!ex)
        continue;

      int dflag;
      if (ex->general_description)
      {
        strncpy(buf, ex->general_description, sizeof(buf) - 1);
        strip_cr(buf);
      }
      else
        *buf = '\0';

      /*
       * Figure out door flag.
       */
      if (IS_SET(ex->exit_info, EX_ISDOOR))
      {
        if (IS_SET(ex->exit_info, EX_SECRET) &&
            IS_SET(ex->exit_info, EX_PICKPROOF))
          dflag = 4;
        else if (IS_SET(ex->exit_info, EX_SECRET))
          dflag = 3;
        else if (IS_SET(ex->exit_info, EX_PICKPROOF))
          dflag = 2;
        else
          dflag = 1;
      }
      else
        dflag = 0;

      if (ex->keyword)
        strncpy(buf1, ex->keyword, sizeof(buf1) - 1);
      else
        *buf1 = '\0';

      /*
       * Now write the exit to the file.
       */
      fprintf(sf, "D%d\n"
                  "%s~\n"
                  "%s~\n"
                  "%d %d %d %d %d %d %d %d %d %d %d\n",
              j, buf, buf1, dflag,
              ex->key,
              ex->to_room,
              ex->dclock, ex->dchide,
              ex->dcskill, ex->dcmove,
              ex->failsavetype, ex->dcfailsave,
              ex->failroom, ex->totalfailroom);
    }

    if (room->ex_description)
    {
      struct extra_descr_data *xdesc;

      for (xdesc = room->ex_description; xdesc; xdesc = xdesc->next)
      {
        strncpy(buf, xdesc->description, sizeof(buf));
        strip_cr(buf);
        fprintf(sf, "E\n"
                    "%s~\n"
                    "%s~\n",
                xdesc->keyword, buf);
      }
    }
    fprintf(sf, "S\n");
    script_save_to_disk(sf, room, WLD_TRIGGER);
  }

  /*
   * Write the final line and close it.
   */
  fprintf(sf, "$~\n");
  fclose(sf);

  /* Old file we're replacing. */
  snprintf(buf, sizeof(buf), "%s%d.wld", WLD_PREFIX, zone->number);

  remove(buf);
  rename(filename, buf);

  if (in_save_list(zone->number, SL_WLD))
  {
    remove_from_save_list(zone->number, SL_WLD);
    create_world_index(zone->number, "wld");
    log("GenOLC: save_rooms: Saving rooms '%s'", buf);
  }
  return TRUE;
}

int copy_room(struct room_data *to, struct room_data *from)
{
  free_room_strings(to);
  *to = *from;
  copy_room_strings(to, from);

  /* Don't put people and objects in two locations.
     Am thinking this shouldn't be done here... */
  from->people = NULL;
  from->contents = NULL;

  return TRUE;
}

/* -------------------------------------------------------------------------- */

/*
 * Copy strings over so bad things don't happen.  We do not free the
 * existing strings here because copy_room() did a shallow copy previously
 * and we'd be freeing the very strings we're copying.  If this function
 * is used elsewhere, be sure to free_room_strings() the 'dest' room first.
 */
int copy_room_strings(struct room_data *dest, struct room_data *source)
{
  int i;

  if (dest == NULL || source == NULL) {
    log("SYSERR: GenOLC: copy_room_strings: NULL values passed.");
    return FALSE;
  }

  dest->description = str_udup(source->description);
  dest->name = str_udup(source->name);

  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (!R_EXIT(source, i))
      continue;

    CREATE(R_EXIT(dest, i), struct room_direction_data, 1);
    *R_EXIT(dest, i) = *R_EXIT(source, i);
    if (R_EXIT(source, i)->general_description)
      R_EXIT(dest, i)->general_description = strdup(R_EXIT(source, i)->general_description);
    if (R_EXIT(source, i)->keyword)
      R_EXIT(dest, i)->keyword = strdup(R_EXIT(source, i)->keyword);
  }

  if (source->ex_description)
    copy_ex_descriptions(&dest->ex_description, source->ex_description);

  return TRUE;
}

int free_room_strings(struct room_data *room)
{
  int i;

  /* Free descriptions. */
  if (room->name)
    free(room->name);
  if (room->description)
    free(room->description);
  if (room->ex_description)
    free_ex_descriptions(room->ex_description);

  /* Free exits. */
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (room->dir_option[i]) {
      if (room->dir_option[i]->general_description)
        free(room->dir_option[i]->general_description);
      if (room->dir_option[i]->keyword)
        free(room->dir_option[i]->keyword);
      free(room->dir_option[i]);
      room->dir_option[i] = NULL;
    }
  }

  return TRUE;
}
