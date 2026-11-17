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
      extract_script(g, slot);
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
    extract_script(g, room);
  room_proto_script(room).clear();

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
