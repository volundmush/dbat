/******************************************************************************/
/** OasisOLC - InGame OLC Copying                                      v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include <cstdlib>

#include "dbat/oasis_copy.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/maputils.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/config.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genwld.hpp"
#include "dbat/genshp.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/

static room_vnum redit_find_new_vnum(GameWorld *g, struct zone_data *zone);

/* External Functions */

/***********************************************************
* This function makes use of the high level OLC functions  *
* to copy most types of mud objects. The type of data is   *
* determined by the subcmd variable and the functions are  *
* looked up in a table.                                    *
***********************************************************/
/******************************************************************************/
/** Commands                                                                 **/
/******************************************************************************/

ACMD(do_dig)
{
  char sdir[MAX_INPUT_LENGTH], sroom[MAX_INPUT_LENGTH], *new_room_name;
  room_vnum rvnum = NOWHERE;
  room_vnum rrnum = NOWHERE;
  struct zone_data *zone;
  int dir                   = 0, rawvnum;
  struct descriptor_data *d = char_get_desc(ch); /* will save us some typing */

  /* Grab the room's name (if available). */
  new_room_name = two_arguments(argument, sdir, sroom);
  skip_spaces(&new_room_name);

  /* Can't dig if we don't know where to go. */
  if (!*sdir || !*sroom) {
    send_to_char(g, ch, "Format: tunnel <direction> <room> - to create an exit\r\n"
                     "        tunnel <direction> -1     - to delete an exit\r\n");
    return;
  }

  rawvnum = atoi(sroom);
  if (rawvnum == -1)
    rvnum = NOWHERE;
  else
    rvnum = (room_vnum)rawvnum;
  rrnum = rvnum;
  if ((dir = search_block(sdir, abbr_dirs, false)) < 0)
    dir = search_block(sdir, dirs, false);
  zone = room_get_zone(g, char_get_in_room(ch));

  if (dir < 0) {
    send_to_char(g, ch, "Cannot create an exit to the '%s'.\r\n", sdir);
    return;
  }
  /* Make sure that the builder has access to the zone he's in. */
  if ((zone == nullptr) || !can_edit_zone(g, ch, zone)) {
    send_cannot_edit(g, ch, zone ? zone->number : NOWHERE);
    return;
  }
  /*
   * Lets not allow digging to limbo. 
   * After all, it'd just get us more errors on 'show errors'
   */
  if (rvnum == 0) {
    send_to_char(g, ch, "The target exists, but you can't dig to limbo!\r\n");
    return;
  }
  /*
   * target room == -1 removes the exit 
   */
  if (rvnum == NOTHING) {
    if (room_get_dir_option(g, char_get_in_room(ch), dir)) {
      /* free the old pointers, if any */
      if (room_get_dir_option(g, char_get_in_room(ch), dir)->general_description)
        free(room_get_dir_option(g, char_get_in_room(ch), dir)->general_description);
      if (room_get_dir_option(g, char_get_in_room(ch), dir)->keyword)
        free(room_get_dir_option(g, char_get_in_room(ch), dir)->keyword);
      delete room_get_dir_option(g, char_get_in_room(ch), dir);
      room_set_dir_option(g, char_get_in_room(ch), dir, nullptr);
      add_to_save_list(g, room_get_zone(g, char_get_in_room(ch))->number, SL_WLD);
      send_to_char(g, ch, "You remove the exit to the %s.\r\n", dirs[dir]);
      return;
    }
    send_to_char(g, ch, "There is no exit to the %s.\r\n"
                     "No exit removed.\r\n",
                 dirs[dir]);
    return;
  }
  /*
   * Can't dig in a direction, if it's already a door. 
   */
  if (room_get_dir_option(g, char_get_in_room(ch), dir)) {
    send_to_char(g, ch, "There already is an exit to the %s.\r\n", dirs[dir]);
    return;
  }

  /* Make sure that the builder has access to the zone he's linking to. */
  zone = find_zone_by_thing(g, rvnum);
  if (zone == nullptr) {
    send_to_char(g, ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(g, ch, zone)) {
    send_cannot_edit(g, ch, zone ? zone->number : NOWHERE);
    return;
  }
  /*
   * Now we know the builder is allowed to make the link 
   */
  /* If the room doesn't exist, create it.*/
  if (!room_of(g, rrnum)) {
    /*
     * Give the descriptor an olc struct.
     * This way we can let redit_save_internally handle the room adding.
     */
    if (d->olc) {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_dig: Player already had olc structure.");
      delete d->olc;
    }
    d->olc      = new oasis_olc_data();
    OLC_ZNUM(d) = zone;
    OLC_NUM(d)  = rvnum;
    OLC_ROOM(d) = new room_data();

    /* Copy the room's name. */
    if (*new_room_name)
      room_set_name(OLC_ROOM(d), strdup(new_room_name));
    else
      room_set_name(OLC_ROOM(d), strdup("An unfinished room"));

    /* Copy the room's description.*/
    room_set_description(OLC_ROOM(d), strdup("You are in an unfinished room.\r\n"));
    room_set_zone(OLC_ROOM(d), OLC_ZNUM(d));
    room_set_number(OLC_ROOM(d), NOWHERE);

    /*
     * Save the new room to memory.
     * redit_save_internally handles adding the room in the right place, etc.
     */
    redit_save_internally(g, d);
    OLC_VAL(d) = 0;

    send_to_char(g, ch, "New room (%d) created.\r\n", rvnum);
    cleanup_olc(g, d, CLEANUP_ALL);
    update_space(g);
    /* 
     * update rrnum to the correct room rnum after adding the room 
     */
    rrnum = rvnum;
  }

  /*
   * Now dig.
   */
  {
    struct room_direction_data *new_exit;
    new_exit = new room_direction_data();
    room_set_dir_option(g, char_get_in_room(ch), dir, new_exit);
  }
  room_get_dir_option(g, char_get_in_room(ch), dir)->general_description = nullptr;
  room_get_dir_option(g, char_get_in_room(ch), dir)->keyword             = nullptr;
  room_get_dir_option(g, char_get_in_room(ch), dir)->to_room             = rrnum;
  add_to_save_list(g, room_get_zone(g, char_get_in_room(ch))->number, SL_WLD);
  save_rooms(g, room_get_zone(g, rrnum));
  send_to_char(g, ch, "You make an exit %s to room %d (%s).\r\n",
               dirs[dir], rvnum, room_get_name(g, rrnum));

  /* 
   * check if we can dig from there to here. 
   */
  if (room_get_dir_option(g, rrnum, rev_dir[dir]))
    send_to_char(g, ch, "You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                 rvnum, dirs[rev_dir[dir]]);
  else {
    {
      struct room_direction_data *new_exit;
      new_exit = new room_direction_data();
      room_set_dir_option(g, rrnum, rev_dir[dir], new_exit);
    }
    room_get_dir_option(g, rrnum, rev_dir[dir])->general_description = nullptr;
    room_get_dir_option(g, rrnum, rev_dir[dir])->keyword             = nullptr;
    room_get_dir_option(g, rrnum, rev_dir[dir])->to_room             = char_get_in_room(ch);
    add_to_save_list(g, room_get_zone(g, rrnum)->number, SL_WLD);
    save_rooms(g, room_get_zone(g, rrnum));
  }
}

ACMD(do_rcopy)
{
  room_vnum rvnum;
  room_vnum rrnum;
  room_vnum trnum;
  room_vnum tvnum;
  struct zone_data *zone;

  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int i;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: rclone <base rnum> <target rnum>\r\nOr be in the room you wish to be the target and provide base vnum.\r\n");
    return;
  }

  if (!*arg2) {
    tvnum = atoi(arg);
    rvnum = char_get_in_room(ch);
  } else {
    rvnum = atoi(arg2);
    tvnum = atoi(arg);
  }

  if (rvnum == 0 || tvnum == 0) {
    send_to_char(g, ch, "The void is fine as it is, try again.\r\n");
    return;
  }
  if (rvnum == tvnum) {
    send_to_char(g, ch, "Don't copy a room to its self!\r\n");
    return;
  }

  trnum = tvnum;
  rrnum = rvnum;

  if (!room_of(g, trnum)) {
    send_to_char(g, ch, "Could not find base room: %d\r\n", tvnum);
    return;
  }
  if (!room_of(g, rrnum)) {
    send_to_char(g, ch, "Could not find target room: %d\r\n", rvnum);
    return;
  }

  zone = room_get_zone(g, rrnum);
  if ((zone == nullptr) || !can_edit_zone(g, ch, zone)) {
    send_to_char(g, ch, "\r\n");
    send_cannot_edit(g, ch, zone ? zone->number : NOWHERE);
    return;
  }

  /* Free descriptions. */
  if (room_get_name(g, rrnum))
    free(room_get_name(g, rrnum));
  if (room_get_description(g, rrnum))
    free(room_get_description(g, rrnum));
  if (room_get_ex_description(g, rrnum))
    free_ex_descriptions(room_get_ex_description(g, rrnum));
  room_set_sector_type(g, rrnum, room_sect(g, trnum));

  /* Copy over description name and extra descriptions */
  room_set_description(g, rrnum, str_udup(room_get_description(g, trnum)));
  room_set_name(g, rrnum, str_udup(room_get_name(g, trnum)));

  /* Copy over any existings extra descriptions */
  if (room_get_ex_description(g, trnum))
    copy_ex_descriptions(room_ex_description_slot(room_of(g, rrnum)), room_get_ex_description(g, trnum));
  else
    room_set_ex_description(g, rrnum, nullptr);

  /* Finally copy over room flags */
  for (i = 0; i < AF_ARRAY_MAX; i++) {
    room_get_room_flags(g, rrnum)[i] = room_get_room_flags(g, trnum)[i];
  }
  send_to_imm(g, "Log: %s has copied room [%d] to room [%d].", char_get_name(ch), tvnum, rvnum);
  add_to_save_list(g, room_get_zone(g, rrnum)->number, SL_WLD);
  save_rooms(g, room_get_zone(g, rrnum));
  send_to_char(g, ch, "Room [%d] copied to room [%d].\r\n", tvnum, rvnum);
}

/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
static room_vnum redit_find_new_vnum(GameWorld *g, struct zone_data *zone)
{
  room_vnum vnum = genolc_zone_bottom(g, zone);

  if (!room_of(g, vnum))
    return NOWHERE;

  while (room_of(g, vnum))
    vnum++;
  return vnum > zone->top ? NOWHERE : vnum;
}

bool buildwalk(GameWorld *g, struct char_data *ch, int dir)
{
  room_vnum vnum;
  room_vnum rnum;

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_BUILDWALK) &&
      char_get_admlevel(ch) >= ADMLVL_IMMORT) {

    if (!can_edit_zone(g, ch, room_get_zone(g, char_get_in_room(ch)))) {
      send_cannot_edit(g, ch, room_get_zone(g, char_get_in_room(ch))->number);
    } else if ((vnum = redit_find_new_vnum(g, room_get_zone(g, char_get_in_room(ch)))) == NOWHERE)
      send_to_char(g, ch, "No free vnums are available in this zone!\r\n");
    else {
      struct descriptor_data *d = char_get_desc(ch);
      /*
       * Give the descriptor an olc struct.
       * This way we can let redit_save_internally handle the room adding.
       */
      if (d->olc) {
        mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: buildwalk(): Player already had olc structure.");
        delete d->olc;
      }
      d->olc      = new oasis_olc_data();
      OLC_ZNUM(d) = room_get_zone(g, char_get_in_room(ch));
      OLC_NUM(d)  = vnum;
      OLC_ROOM(d) = new room_data();

      room_set_name(OLC_ROOM(d), strdup("New BuildWalk Room"));

      room_set_description(OLC_ROOM(d), str_printf("This unfinished room was created by %s.\r\n", char_get_name(ch)));
      room_set_zone(OLC_ROOM(d), OLC_ZNUM(d));
      room_set_number(OLC_ROOM(d), NOWHERE);

      /*
       * Save the new room to memory.
       * redit_save_internally handles adding the room in the right place, etc.
       */
      redit_save_internally(g, d);
      OLC_VAL(d) = 0;

      /* Link rooms */
      rnum = vnum;
      {
        struct room_direction_data *new_exit;
        new_exit = new room_direction_data();
        char_set_exit(g, ch, dir, new_exit);
      }
      char_get_exit(g, ch, dir)->to_room = rnum;
      struct room_direction_data *back_exit;
      back_exit = new room_direction_data();
      room_set_dir_option(g, rnum, rev_dir[dir], back_exit);
      room_get_dir_option(g, rnum, rev_dir[dir])->to_room = char_get_in_room(ch);

      /* Report room creation to user */
      send_to_char(g, ch, "@yRoom #%d created by BuildWalk.@n\r\n", vnum);
      cleanup_olc(g, d, CLEANUP_STRUCTS);
      update_space(g);
      return true;
    }
  }

  return false;
}
