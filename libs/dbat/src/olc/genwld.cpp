/************************************************************************
 * Generic OLC Library - Rooms / genwld.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/iterate.hpp"
#include "dbat/arrays.hpp"
#include "dbat/shop.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/genwld.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/comm.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static bool copy_room(struct room_data *to, struct room_data *from);
static bool copy_room_strings(struct room_data *dest, struct room_data *source);

/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
room_vnum add_room(GameWorld *g, struct room_data *room)
{
  struct room_data *slot;
  room_vnum vnum;

  if (room == nullptr)
    return NOWHERE;

  vnum = room_get_number(room);
  if ((slot = room_of(g, vnum))) {
    if (room_get_script(slot))
      extract_script(g, slot, WLD_TRIGGER);
    id_list people = room_get_people(slot), contents = room_get_contents(slot);
    copy_room(slot, room);
    slot->people   = people;
    slot->contents = contents;
    add_to_save_list(g, room_get_zone(room)->number, SL_WLD);
    mud_log("GenOLC: add_room: Updated existing room #{:d}.", vnum);
    return vnum;
  }

  slot  = g->rooms.emplace(vnum);
  *slot = *room;
  copy_room_strings(slot, room);
  mud_log("GenOLC: add_room: Added room {:d}.", vnum);

  add_to_save_list(g, room_get_zone(room)->number, SL_WLD);
  return vnum;
}

/* -------------------------------------------------------------------------- */

bool delete_room(GameWorld *g, room_vnum vnum)
{
  int j;
  struct room_data *room;

  if (vnum <= 0 || !(room = room_of(g, vnum))) /* Can't delete void yet. */
    return false;

  /* Everyone with nowhere else to go ends up here. */
  if (vnum == g->r_mortal_start_room || vnum == CONFIG_MORTAL_START) {
    mud_log("GenOLC: delete_room: Refusing to delete the mortal start room, #{:d}.", vnum);
    return false;
  }

  add_to_save_list(g, room_get_zone(room)->number, SL_WLD);

  /* This is something you might want to read about in the logs. */
  mud_log("GenOLC: delete_room: Deleting room #{:d} ({}).", vnum, room_get_name(room));

  if (g->r_immort_start_room == vnum) {
    mud_log("WARNING: GenOLC: delete_room: Deleting immortal start room!");
    g->r_immort_start_room = 0; /* The Void */
  }
  if (g->r_frozen_start_room == vnum) {
    mud_log("WARNING: GenOLC: delete_room: Deleting frozen start room!");
    g->r_frozen_start_room = 0; /* The Void */
  }

  /*
   * Dump the contents of this room into the Void.  We could also just
   * extract the people, mobs, and objects here.
   */
  for (obj_data *obj : room_contents(g, vnum)) {
    obj_from_room(g, obj);
    obj_to_room(g, obj, 0);
  }
  for (char_data *ppl : room_people(g, vnum)) {
    char_from_room(g, ppl);
    char_to_room(g, ppl, 0);
  }

  free_room_strings(room);
  if (room_get_script(room))
    extract_script(g, room, WLD_TRIGGER);
  free_proto_script(room, WLD_TRIGGER);

  /* Exits to this room go nowhere, or are removed if they had nothing to show. */
  for (struct room_data *other : g->rooms.all())
    for (j = 0; j < NUM_OF_DIRS; j++) {
      struct room_direction_data *exit = room_get_dir_option(other, j);

      if (!exit || exit->to_room != vnum)
        continue;
      if (exit->keyword.empty() && exit->general_description.empty()) {
        /* no description, remove exit completely */
        room_set_dir_option(other, j, nullptr);
      } else {
        /* description is set, just point to nowhere */
        exit->to_room = NOWHERE;
      }
    }

  /* Cancel the zone commands that used this room. */
  for (struct zone_data *zone : g->zones.all())
    for (j = 0; zone_get_cmd(g, zone, j)->command != 'S'; j++)
      switch (zone_get_cmd(g, zone, j)->command) {
        case 'M':
        case 'O':
        case 'T':
        case 'V':
          if (zone_get_cmd(g, zone, j)->arg3 == vnum)
            zone_get_cmd(g, zone, j)->command = '*'; /* Cancel command. */
          break;
        case 'D':
        case 'R':
          if (zone_get_cmd(g, zone, j)->arg1 == vnum)
            zone_get_cmd(g, zone, j)->command = '*'; /* Cancel command. */
          break;
        case 'G':
        case 'P':
        case 'E':
        case '*':
          /* Known zone entries we don't care about. */
          break;
        default:
          mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: delete_room: Unknown zone entry found!");
      }

  /* Remove this room from all shop lists. */
  for (struct shop_data *shop : g->shops.all())
    for (j = 0; j < (int)shop_rooms(shop).size(); j++)
      if (shop_rooms(shop)[j] == vnum)
        shop_rooms(shop)[j] = 0; /* set to the void */

  g->rooms.take(vnum);
  return true;
}

bool save_rooms(GameWorld *g, struct zone_data *zone_num)
{
  room_vnum i;
  struct room_data *room;
  struct atomic_file af;
  FILE *sf;
  std::string filename;
  std::string buf;
  std::string buf1;
  char rbuf1[MAX_STRING_LENGTH], rbuf2[MAX_STRING_LENGTH];
  char rbuf3[MAX_STRING_LENGTH], rbuf4[MAX_STRING_LENGTH];

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  mud_log("GenOLC: save_rooms: Saving rooms in zone #{:d} ({:d}-{:d}).",
          zone_num->number, genolc_zone_bottom(g, zone_num), zone_num->top);

  filename = str_format("{}{:d}.wld", WLD_PREFIX, zone_num->number);
  if (!(sf = atomic_open(&af, filename.c_str(), false))) {
    perror("SYSERR: save_rooms");
    return false;
  }

  for (i = genolc_zone_bottom(g, zone_num); i <= zone_num->top; i++) {
    room_vnum rnum;

    if (room_of(g, rnum = i)) {
      int j;

      room = (room_of(g, rnum));

      /*
       * Copy the description and strip off trailing newlines.
       */
      buf = room_get_description(room) ? room_get_description(room) : "Empty room.";
      strip_cr(buf);

      /*
       * Save the numeric and string section of the file.
       */
      sprintascii(rbuf1, room_get_room_flags(room)[0]);
      sprintascii(rbuf2, room_get_room_flags(room)[1]);
      sprintascii(rbuf3, room_get_room_flags(room)[2]);
      sprintascii(rbuf4, room_get_room_flags(room)[3]);
      file_print(sf, "#{:d}\n"
                     "{}{:c}\n"
                     "{}{:c}\n"
                     "{:d} {} {} {} {} {:d}\n",
                 room_get_number(room),
                 room_get_name(room) ? room_get_name(room) : "Untitled", as_char(STRING_TERMINATOR),
                 buf, as_char(STRING_TERMINATOR),
                 room_get_zone(room)->number,
                 rbuf1, rbuf2, rbuf3, rbuf4, room_get_sector_type(room));

      /*
       * Now you write out the exits for the room.
       */
      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (room_get_dir_option(room, j)) {
          int dflag;
          if (exit_description(room_get_dir_option(room, j))) {
            buf = exit_description(room_get_dir_option(room, j));
            strip_cr(buf);
          } else
            buf.clear();

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

          if (exit_keyword(room_get_dir_option(room, j)))
            buf1 = exit_keyword(room_get_dir_option(room, j));
          else
            buf1.clear();

          /*
	   * Now write the exit to the file.
	   */
          file_print(sf, "D{:d}\n"
                         "{}~\n"
                         "{}~\n"
                         "{:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}\n",
                     j, buf, buf1, dflag,
                     room_get_dir_option(room, j)->key != NOTHING ? room_get_dir_option(room, j)->key : -1,
                     room_get_dir_option(room, j)->to_room != NOWHERE ? room_get_dir_option(room, j)->to_room : -1,
                     room_get_dir_option(room, j)->dclock, room_get_dir_option(room, j)->dchide,
                     room_get_dir_option(room, j)->dcskill, room_get_dir_option(room, j)->dcmove,
                     room_get_dir_option(room, j)->failsavetype, room_get_dir_option(room, j)->dcfailsave,
                     room_get_dir_option(room, j)->failroom, room_get_dir_option(room, j)->totalfailroom);
        }
      }

      for (const extra_descr_data &xdesc : room_get_ex_description(room)) {
        buf = xdesc.description;
        strip_cr(buf);
        file_print(sf, "E\n"
                       "{}~\n"
                       "{}~\n",
                   xdesc.keyword, buf);
      }
      file_print(sf, "S\n");
      script_save_to_disk(sf, room, WLD_TRIGGER);
    }
  }

  /*
   * Write the final line and close it.
   */
  file_print(sf, "$~\n");
  if (!atomic_commit(&af))
    return false;
  buf = filename;

  if (in_save_list(g, zone_num->number, SL_WLD)) {
    remove_from_save_list(g, zone_num->number, SL_WLD);
    create_world_index(g, zone_num->number, "wld");
    mud_log("GenOLC: save_rooms: Saving rooms '{}'", buf);
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
  from->people.clear();
  from->contents.clear();

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
  if (dest == nullptr || source == nullptr) {
    mud_log("SYSERR: GenOLC: copy_room_strings: NULL values passed.");
    return false;
  }

  room_set_description(dest, take_string(str_udup(room_get_description(source))));
  room_set_name(dest, take_string(str_udup(room_get_name(source))));

  /* exits and extra descriptions are copied with the room */

  return true;
}

bool free_room_strings(struct room_data *room)
{
  int i;

  /* Free descriptions. */

  /* Free exits. */
  for (i = 0; i < NUM_OF_DIRS; i++)
    room_set_dir_option(room, i, nullptr);

  return true;
}
