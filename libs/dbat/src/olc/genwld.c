/************************************************************************
 * Generic OLC Library - Rooms / genwld.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/shop.h"
#include "dbat/dg_scripts.h"

#include "dbat/genwld.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/comm.h"
#include "dbat/genolc.h"
#include "dbat/genzon.h"
#include "dbat/dg_olc.h"
#include "dbat/atomicfile.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static bool copy_room(struct room_data *to, struct room_data *from);
static bool copy_room_strings(struct room_data *dest, struct room_data *source);

/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
room_rnum add_room(GameWorld *g, struct room_data *room)
{
  struct char_data *tch;
  struct obj_data *tobj;
  int j;
  room_rnum found = false;
  room_rnum i;

  if (room == nullptr)
    return NOWHERE;

  if ((i = real_room(g, room_get_number(room))) != NOWHERE) {
    if (rroom_get_script(g, i))
      extract_script(g, &g->rooms[i], WLD_TRIGGER);
    tch  = rroom_get_people(g, i);
    tobj = rroom_get_contents(g, i);
    copy_room(&g->rooms[i], room);
    rroom_set_people(g, i, tch);
    rroom_set_contents(g, i, tobj);
    add_to_save_list(g, g->zones[room_get_zone(room)].number, SL_WLD);
    mud_log("GenOLC: add_room: Updated existing room #%d.", room_get_number(room));
    return i;
  }

  RECREATE(g->rooms, struct room_data, g->top_room + 2);
  g->top_room++;

  for (i = g->top_room; i > 0; i--) {
    if (room_get_number(room) > rroom_get_vnum(g, i - 1)) {
      g->rooms[i] = *room;
      copy_room_strings(&g->rooms[i], room);
      found = i;
      break;
    }
    /* Copy the room over now. */
    g->rooms[i] = g->rooms[i - 1];
    update_wait_events(&g->rooms[i], &g->rooms[i - 1]);

    /* People in this room must have their in_rooms moved up one. */
    for (tch = rroom_get_people(g, i); tch; tch = char_get_next_in_room(tch))
      char_set_in_room(tch, char_get_in_room(tch) + ((char_get_in_room(tch) != NOWHERE)));

    /* Move objects too. */
    for (tobj = rroom_get_contents(g, i); tobj; tobj = obj_get_next_content(tobj))
      obj_set_in_room(tobj, obj_get_in_room(tobj) + ((obj_get_in_room(tobj) != NOWHERE)));
  }
  if (!found) {
    g->rooms[0] = *room; /* Last place, in front. */
    copy_room_strings(&g->rooms[0], room);
  }

  mud_log("GenOLC: add_room: Added room %d at index #%d.", room_get_number(room), found);

  /* found is equal to the array index where we added the room. */

  /*
   * Find what zone that room was in so we can update the loading table.
   */
  for (i = room_get_zone(room); i <= g->top_zone; i++)
    for (j = 0; zone_get_cmd(g, i, j)->command != 'S'; j++)
      switch (zone_get_cmd(g, i, j)->command) {
        case 'M':
        case 'O':
        case 'T':
        case 'V':
          zone_get_cmd(g, i, j)->arg3 += (zone_get_cmd(g, i, j)->arg3 != NOWHERE && zone_get_cmd(g, i, j)->arg3 >= found);
          break;
        case 'D':
        case 'R':
          zone_get_cmd(g, i, j)->arg1 += (zone_get_cmd(g, i, j)->arg1 != NOWHERE && zone_get_cmd(g, i, j)->arg1 >= found);
        case 'G':
        case 'P':
        case 'E':
        case '*':
          /* Known zone entries we don't care about. */
          break;
        default:
          mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: add_room: Unknown zone entry found!");
      }

  /*
   * Update the loadroom table. Adds 1 or 0.
   */
  g->r_mortal_start_room += (g->r_mortal_start_room >= found);
  g->r_immort_start_room += (g->r_immort_start_room >= found);
  g->r_frozen_start_room += (g->r_frozen_start_room >= found);

  /*
   * Update world exits.
   */
  i = g->top_room + 1;
  do {
    i--;
    for (j = 0; j < NUM_OF_DIRS; j++)
      if (rroom_get_dir_option(g, i, j) && rroom_get_dir_option(g, i, j)->to_room != NOWHERE)
        rroom_get_dir_option(g, i, j)->to_room += (rroom_get_dir_option(g, i, j)->to_room >= found);
  } while (i > 0);

  add_to_save_list(g, g->zones[room_get_zone(room)].number, SL_WLD);

  /*
   * Return what array entry we placed the new room in.
   */
  return found;
}

/* -------------------------------------------------------------------------- */

bool delete_room(GameWorld *g, room_rnum rnum)
{
  room_rnum i;
  int j;
  struct char_data *ppl, *next_ppl;
  struct obj_data *obj, *next_obj;
  struct room_data *room;

  if (rnum <= 0 || rnum > g->top_room) /* Can't delete void yet. */
    return false;

  room = &g->rooms[rnum];

  /* Everyone with nowhere else to go ends up here. */
  if (rnum == g->r_mortal_start_room || room_get_number(room) == CONFIG_MORTAL_START) {
    mud_log("GenOLC: delete_room: Refusing to delete the mortal start room, #%d.", room_get_number(room));
    return false;
  }

  add_to_save_list(g, g->zones[room_get_zone(room)].number, SL_WLD);

  /* This is something you might want to read about in the logs. */
  mud_log("GenOLC: delete_room: Deleting room #%d (%s).", room_get_number(room), room_get_name(room));

  if (g->r_immort_start_room == rnum) {
    mud_log("WARNING: GenOLC: delete_room: Deleting immortal start room!");
    g->r_immort_start_room = 0; /* The Void */
  }
  if (g->r_frozen_start_room == rnum) {
    mud_log("WARNING: GenOLC: delete_room: Deleting frozen start room!");
    g->r_frozen_start_room = 0; /* The Void */
  }

  /*
   * Dump the contents of this room into the Void.  We could also just
   * extract the people, mobs, and objects here.
   */
  for (obj = rroom_get_contents(g, rnum); obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
    obj_from_room(g, obj);
    obj_to_room(g, obj, 0);
  }
  for (ppl = rroom_get_people(g, rnum); ppl; ppl = next_ppl) {
    next_ppl = char_get_next_in_room(ppl);
    char_from_room(g, ppl);
    char_to_room(g, ppl, 0);
  }

  free_room_strings(room);
  if (room_get_script(room))
    extract_script(g, room, WLD_TRIGGER);
  free_proto_script(room, WLD_TRIGGER);

  /*
   * Change any exit going to this room to go the void.
   * Also fix all the exits pointing to rooms above this.
   */
  i = g->top_room + 1;
  do {
    i--;
    for (j = 0; j < NUM_OF_DIRS; j++)
      if (rroom_get_dir_option(g, i, j) == nullptr)
        continue;
      else if (rroom_get_dir_option(g, i, j)->to_room > rnum)
        rroom_get_dir_option(g, i, j)->to_room -= (rroom_get_dir_option(g, i, j)->to_room != NOWHERE); /* with unsigned NOWHERE > any rnum */
      else if (rroom_get_dir_option(g, i, j)->to_room == rnum) {
        if ((!rroom_get_dir_option(g, i, j)->keyword || !*rroom_get_dir_option(g, i, j)->keyword) &&
            (!rroom_get_dir_option(g, i, j)->general_description || !*rroom_get_dir_option(g, i, j)->general_description)) {
          /* no description, remove exit completely */
          if (rroom_get_dir_option(g, i, j)->keyword)
            free(rroom_get_dir_option(g, i, j)->keyword);
          if (rroom_get_dir_option(g, i, j)->general_description)
            free(rroom_get_dir_option(g, i, j)->general_description);
          free(rroom_get_dir_option(g, i, j));
          rroom_set_dir_option(g, i, j, nullptr);
        } else {
          /* description is set, just point to nowhere */
          rroom_get_dir_option(g, i, j)->to_room = NOWHERE;
        }
      }
  } while (i > 0);

  /*
   * Find what zone that room was in so we can update the loading table.
   */
  for (i = 0; i <= g->top_zone; i++)
    for (j = 0; zone_get_cmd(g, i, j)->command != 'S'; j++)
      switch (zone_get_cmd(g, i, j)->command) {
        case 'M':
        case 'O':
        case 'T':
        case 'V':
          if (zone_get_cmd(g, i, j)->arg3 == rnum)
            zone_get_cmd(g, i, j)->command = '*'; /* Cancel command. */
          else if (zone_get_cmd(g, i, j)->arg3 > rnum)
            zone_get_cmd(g, i, j)->arg3 -= (zone_get_cmd(g, i, j)->arg3 != NOWHERE); /* with unsigned NOWHERE > any rnum */
          break;
        case 'D':
        case 'R':
          if (zone_get_cmd(g, i, j)->arg1 == rnum)
            zone_get_cmd(g, i, j)->command = '*'; /* Cancel command. */
          else if (zone_get_cmd(g, i, j)->arg1 > rnum)
            zone_get_cmd(g, i, j)->arg1 -= (zone_get_cmd(g, i, j)->arg1 != NOWHERE); /* with unsigned NOWHERE > any rnum */
        case 'G':
        case 'P':
        case 'E':
        case '*':
          /* Known zone entries we don't care about. */
          break;
        default:
          mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: delete_room: Unknown zone entry found!");
      }

  /*
   * Remove this room from all shop lists.
   */
  {
    for (i = 0; i < g->top_shop; i++) {
      for (j = 0; rshop_get_room(g, i, j) != NOWHERE; j++) {
        if (rshop_get_room(g, i, j) == rroom_get_vnum(g, rnum))
          rshop_set_room(g, i, j, 0); /* set to the void */
      }
    }
  }
  /*
   * Now we actually move the rooms down.
   */
  for (i = rnum; i < g->top_room; i++) {
    g->rooms[i] = g->rooms[i + 1];
    update_wait_events(&g->rooms[i], &g->rooms[i + 1]);

    for (ppl = rroom_get_people(g, i); ppl; ppl = char_get_next_in_room(ppl))
      char_set_in_room(ppl, char_get_in_room(ppl) - ((char_get_in_room(ppl) != NOWHERE))); /* Redundant check? */

    for (obj = rroom_get_contents(g, i); obj; obj = obj_get_next_content(obj))
      obj_set_in_room(obj, obj_get_in_room(obj) - ((obj_get_in_room(obj) != NOWHERE))); /* Redundant check? */
  }

  g->top_room--;
  RECREATE(g->rooms, struct room_data, g->top_room + 1);

  return true;
}

bool save_rooms(GameWorld *g, zone_rnum zone_num)
{
  room_vnum i;
  struct room_data *room;
  struct atomic_file af;
  FILE *sf;
  char filename[128];
  char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
  char rbuf1[MAX_STRING_LENGTH], rbuf2[MAX_STRING_LENGTH];
  char rbuf3[MAX_STRING_LENGTH], rbuf4[MAX_STRING_LENGTH];

  if (zone_num < 0 || zone_num > g->top_zone) {
    mud_log("SYSERR: GenOLC: save_rooms: Invalid zone number %d passed! (0-%d)", zone_num, g->top_zone);
    return false;
  }

  mud_log("GenOLC: save_rooms: Saving rooms in zone #%d (%d-%d).",
          g->zones[zone_num].number, genolc_zone_bottom(g, zone_num), g->zones[zone_num].top);

  snprintf(filename, sizeof(filename), "%s%d.wld", WLD_PREFIX, g->zones[zone_num].number);
  if (!(sf = atomic_open(&af, filename, false))) {
    perror("SYSERR: save_rooms");
    return false;
  }

  for (i = genolc_zone_bottom(g, zone_num); i <= g->zones[zone_num].top; i++) {
    room_rnum rnum;

    if ((rnum = real_room(g, i)) != NOWHERE) {
      int j;

      room = (g->rooms + rnum);

      /*
       * Copy the description and strip off trailing newlines.
       */
      strlcpy(buf, room_get_description(room) ? room_get_description(room) : "Empty room.", sizeof(buf));
      strip_cr(buf);

      /*
       * Save the numeric and string section of the file.
       */
      sprintascii(rbuf1, room_get_room_flags(room)[0]);
      sprintascii(rbuf2, room_get_room_flags(room)[1]);
      sprintascii(rbuf3, room_get_room_flags(room)[2]);
      sprintascii(rbuf4, room_get_room_flags(room)[3]);
      fprintf(sf, "#%d\n"
                  "%s%c\n"
                  "%s%c\n"
                  "%d %s %s %s %s %d\n",
              room_get_number(room),
              room_get_name(room) ? room_get_name(room) : "Untitled", STRING_TERMINATOR,
              buf, STRING_TERMINATOR,
              g->zones[room_get_zone(room)].number,
              rbuf1, rbuf2, rbuf3, rbuf4, room_get_sector_type(room));

      /*
       * Now you write out the exits for the room.
       */
      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (room_get_dir_option(room, j)) {
          int dflag;
          if (room_get_dir_option(room, j)->general_description) {
            strlcpy(buf, room_get_dir_option(room, j)->general_description, sizeof(buf));
            strip_cr(buf);
          } else
            *buf = '\0';

          /*
	   * Figure out door flag.
	   */
          if (IS_SET(room_get_dir_option(room, j)->exit_info, EX_ISDOOR)) {
            if (IS_SET(room_get_dir_option(room, j)->exit_info, EX_SECRET) &&
                IS_SET(room_get_dir_option(room, j)->exit_info, EX_PICKPROOF))
              dflag = 4;
            else if (IS_SET(room_get_dir_option(room, j)->exit_info, EX_SECRET))
              dflag = 3;
            else if (IS_SET(room_get_dir_option(room, j)->exit_info, EX_PICKPROOF))
              dflag = 2;
            else
              dflag = 1;
          } else
            dflag = 0;

          if (room_get_dir_option(room, j)->keyword)
            strlcpy(buf1, room_get_dir_option(room, j)->keyword, sizeof(buf1));
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
                  room_get_dir_option(room, j)->key != NOTHING ? room_get_dir_option(room, j)->key : -1,
                  room_get_dir_option(room, j)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, j)->to_room) : -1,
                  room_get_dir_option(room, j)->dclock, room_get_dir_option(room, j)->dchide,
                  room_get_dir_option(room, j)->dcskill, room_get_dir_option(room, j)->dcmove,
                  room_get_dir_option(room, j)->failsavetype, room_get_dir_option(room, j)->dcfailsave,
                  room_get_dir_option(room, j)->failroom, room_get_dir_option(room, j)->totalfailroom);
        }
      }

      if (room_get_ex_description(room)) {
        struct extra_descr_data *xdesc;

        for (xdesc = room_get_ex_description(room); xdesc; xdesc = xdesc->next) {
          strlcpy(buf, xdesc->description, sizeof(buf));
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
  }

  /*
   * Write the final line and close it.
   */
  fprintf(sf, "$~\n");
  if (!atomic_commit(&af))
    return false;
  snprintf(buf, sizeof(buf), "%s", filename);

  if (in_save_list(g, g->zones[zone_num].number, SL_WLD)) {
    remove_from_save_list(g, g->zones[zone_num].number, SL_WLD);
    create_world_index(g, g->zones[zone_num].number, "wld");
    mud_log("GenOLC: save_rooms: Saving rooms '%s'", buf);
  }
  return true;
}

static bool copy_room(struct room_data *to, struct room_data *from)
{
  free_room_strings(to);
  *to = *from;
  copy_room_strings(to, from);

  /* Don't put people and objects in two locations.
     Am thinking this shouldn't be done here... */
  room_set_people(from, nullptr);
  room_set_contents(from, nullptr);

  return true;
}

/* -------------------------------------------------------------------------- */

/*
 * Copy strings over so bad things don't happen.  We do not free the
 * existing strings here because copy_room() did a shallow copy previously
 * and we'd be freeing the very strings we're copying.  If this function
 * is used elsewhere, be sure to free_room_strings() the 'dest' room first.
 */
static bool copy_room_strings(struct room_data *dest, struct room_data *source)
{
  int i;

  if (dest == nullptr || source == nullptr) {
    mud_log("SYSERR: GenOLC: copy_room_strings: NULL values passed.");
    return false;
  }

  room_set_description(dest, str_udup(room_get_description(source)));
  room_set_name(dest, str_udup(room_get_name(source)));

  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (!room_get_dir_option(source, i))
      continue;

    {
      struct room_direction_data *new_exit;
      CREATE(new_exit, struct room_direction_data, 1);
      room_set_dir_option(dest, i, new_exit);
    }
    *room_get_dir_option(dest, i) = *room_get_dir_option(source, i);
    if (room_get_dir_option(source, i)->general_description)
      room_get_dir_option(dest, i)->general_description = strdup(room_get_dir_option(source, i)->general_description);
    if (room_get_dir_option(source, i)->keyword)
      room_get_dir_option(dest, i)->keyword = strdup(room_get_dir_option(source, i)->keyword);
  }

  if (room_get_ex_description(source))
    copy_ex_descriptions(room_ex_description_slot(dest), room_get_ex_description(source));

  return true;
}

bool free_room_strings(struct room_data *room)
{
  int i;

  /* Free descriptions. */
  if (room_get_name(room))
    free(room_get_name(room));
  if (room_get_description(room))
    free(room_get_description(room));
  if (room_get_ex_description(room))
    free_ex_descriptions(room_get_ex_description(room));

  /* Free exits. */
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (room_get_dir_option(room, i)) {
      if (room_get_dir_option(room, i)->general_description)
        free(room_get_dir_option(room, i)->general_description);
      if (room_get_dir_option(room, i)->keyword)
        free(room_get_dir_option(room, i)->keyword);
      free(room_get_dir_option(room, i));
      room_set_dir_option(room, i, nullptr);
    }
  }

  return true;
}
