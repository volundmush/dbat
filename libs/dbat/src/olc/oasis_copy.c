/******************************************************************************/
/** OasisOLC - InGame OLC Copying                                      v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include "dbat/oasis_copy.h"
#include "dbat/broadcast.h"
#include "dbat/dg_olc.h"
#include "dbat/act.wizard.h"
#include "dbat/maputils.h"
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/config.h"
#include "dbat/genolc.h"
#include "dbat/genwld.h"
#include "dbat/genshp.h"
#include "dbat/dg_scripts.h"
#include "dbat/room_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/

static room_vnum redit_find_new_vnum(GameWorld *g, zone_rnum zone);

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
  room_rnum rrnum = NOWHERE;
  zone_rnum zone;
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
  rrnum = real_room(g, rvnum);
  if ((dir = search_block(sdir, abbr_dirs, false)) < 0)
    dir = search_block(sdir, dirs, false);
  zone = rroom_get_zone(g, char_get_in_room(ch));

  if (dir < 0) {
    send_to_char(g, ch, "Cannot create an exit to the '%s'.\r\n", sdir);
    return;
  }
  /* Make sure that the builder has access to the zone he's in. */
  if ((zone == NOWHERE) || !can_edit_zone(g, ch, zone)) {
    send_cannot_edit(g, ch, zone);
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
    if (rroom_get_dir_option(g, char_get_in_room(ch), dir)) {
      /* free the old pointers, if any */
      if (rroom_get_dir_option(g, char_get_in_room(ch), dir)->general_description)
        free(rroom_get_dir_option(g, char_get_in_room(ch), dir)->general_description);
      if (rroom_get_dir_option(g, char_get_in_room(ch), dir)->keyword)
        free(rroom_get_dir_option(g, char_get_in_room(ch), dir)->keyword);
      free(rroom_get_dir_option(g, char_get_in_room(ch), dir));
      rroom_set_dir_option(g, char_get_in_room(ch), dir, nullptr);
      add_to_save_list(g, g->zones[rroom_get_zone(g, char_get_in_room(ch))].number, SL_WLD);
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
  if (rroom_get_dir_option(g, char_get_in_room(ch), dir)) {
    send_to_char(g, ch, "There already is an exit to the %s.\r\n", dirs[dir]);
    return;
  }

  /* Make sure that the builder has access to the zone he's linking to. */
  zone = real_zone_by_thing(g, rvnum);
  if (zone == NOWHERE) {
    send_to_char(g, ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(g, ch, zone)) {
    send_cannot_edit(g, ch, zone);
    return;
  }
  /*
   * Now we know the builder is allowed to make the link 
   */
  /* If the room doesn't exist, create it.*/
  if (rrnum == NOWHERE) {
    /*
     * Give the descriptor an olc struct.
     * This way we can let redit_save_internally handle the room adding.
     */
    if (d->olc) {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_dig: Player already had olc structure.");
      free(d->olc);
    }
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZNUM(d) = zone;
    OLC_NUM(d)  = rvnum;
    CREATE(OLC_ROOM(d), struct room_data, 1);

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
    rrnum = real_room(g, rvnum);
  }

  /*
   * Now dig.
   */
  {
    struct room_direction_data *new_exit;
    CREATE(new_exit, struct room_direction_data, 1);
    rroom_set_dir_option(g, char_get_in_room(ch), dir, new_exit);
  }
  rroom_get_dir_option(g, char_get_in_room(ch), dir)->general_description = nullptr;
  rroom_get_dir_option(g, char_get_in_room(ch), dir)->keyword             = nullptr;
  rroom_get_dir_option(g, char_get_in_room(ch), dir)->to_room             = rrnum;
  add_to_save_list(g, g->zones[rroom_get_zone(g, char_get_in_room(ch))].number, SL_WLD);
  save_rooms(g, g->zones[rroom_get_zone(g, rrnum)].number);
  send_to_char(g, ch, "You make an exit %s to room %d (%s).\r\n",
               dirs[dir], rvnum, rroom_get_name(g, rrnum));

  /* 
   * check if we can dig from there to here. 
   */
  if (rroom_get_dir_option(g, rrnum, rev_dir[dir]))
    send_to_char(g, ch, "You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                 rvnum, dirs[rev_dir[dir]]);
  else {
    {
      struct room_direction_data *new_exit;
      CREATE(new_exit, struct room_direction_data, 1);
      rroom_set_dir_option(g, rrnum, rev_dir[dir], new_exit);
    }
    rroom_get_dir_option(g, rrnum, rev_dir[dir])->general_description = nullptr;
    rroom_get_dir_option(g, rrnum, rev_dir[dir])->keyword             = nullptr;
    rroom_get_dir_option(g, rrnum, rev_dir[dir])->to_room             = char_get_in_room(ch);
    add_to_save_list(g, g->zones[rroom_get_zone(g, rrnum)].number, SL_WLD);
    save_rooms(g, g->zones[rroom_get_zone(g, rrnum)].number);
  }
}

ACMD(do_rcopy)
{
  room_vnum rvnum;
  room_rnum rrnum;
  room_rnum trnum;
  room_rnum tvnum;
  zone_rnum zone;

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
    rvnum = rroom_get_vnum(g, char_get_in_room(ch));
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

  trnum = real_room(g, tvnum);
  rrnum = real_room(g, rvnum);

  if (trnum == NOWHERE) {
    send_to_char(g, ch, "Could not find base room: %d\r\n", tvnum);
    return;
  }
  if (rrnum == NOWHERE) {
    send_to_char(g, ch, "Could not find target room: %d\r\n", rvnum);
    return;
  }

  zone = rroom_get_zone(g, rrnum);
  if ((zone == NOWHERE) || !can_edit_zone(g, ch, zone)) {
    send_to_char(g, ch, "\r\n");
    send_cannot_edit(g, ch, zone);
    return;
  }

  /* Free descriptions. */
  if (rroom_get_name(g, rrnum))
    free(rroom_get_name(g, rrnum));
  if (rroom_get_description(g, rrnum))
    free(rroom_get_description(g, rrnum));
  if (rroom_get_ex_description(g, rrnum))
    free_ex_descriptions(rroom_get_ex_description(g, rrnum));
  rroom_set_sector_type(g, rrnum, rroom_sect(g, trnum));

  /* Copy over description name and extra descriptions */
  rroom_set_description(g, rrnum, str_udup(rroom_get_description(g, trnum)));
  rroom_set_name(g, rrnum, str_udup(rroom_get_name(g, trnum)));

  /* Copy over any existings extra descriptions */
  if (rroom_get_ex_description(g, trnum))
    copy_ex_descriptions(room_ex_description_slot(&g->rooms[rrnum]), rroom_get_ex_description(g, trnum));
  else
    rroom_set_ex_description(g, rrnum, nullptr);

  /* Finally copy over room flags */
  for (i = 0; i < AF_ARRAY_MAX; i++) {
    rroom_get_room_flags(g, rrnum)[i] = rroom_get_room_flags(g, trnum)[i];
  }
  send_to_imm(g, "Log: %s has copied room [%d] to room [%d].", char_get_name(ch), tvnum, rvnum);
  add_to_save_list(g, g->zones[rroom_get_zone(g, rrnum)].number, SL_WLD);
  save_rooms(g, g->zones[rroom_get_zone(g, rrnum)].number);
  send_to_char(g, ch, "Room [%d] copied to room [%d].\r\n", tvnum, rvnum);
}

/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
static room_vnum redit_find_new_vnum(GameWorld *g, zone_rnum zone)
{
  room_vnum vnum = genolc_zone_bottom(g, zone);
  room_rnum rnum = real_room(g, vnum);

  if (rnum == NOWHERE)
    return NOWHERE;

  for (;;) {
    if (vnum > g->zones[zone].top)
      return (NOWHERE);
    if (rnum > g->top_room || rroom_get_vnum(g, rnum) > vnum)
      break;
    rnum++;
    vnum++;
  }
  return (vnum);
}

bool buildwalk(GameWorld *g, struct char_data *ch, int dir)
{
  room_vnum vnum;
  room_rnum rnum;

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_BUILDWALK) &&
      char_get_admlevel(ch) >= ADMLVL_IMMORT) {

    if (!can_edit_zone(g, ch, rroom_get_zone(g, char_get_in_room(ch)))) {
      send_cannot_edit(g, ch, rroom_get_zone(g, char_get_in_room(ch)));
    } else if ((vnum = redit_find_new_vnum(g, rroom_get_zone(g, char_get_in_room(ch)))) == NOWHERE)
      send_to_char(g, ch, "No free vnums are available in this zone!\r\n");
    else {
      struct descriptor_data *d = char_get_desc(ch);
      /*
       * Give the descriptor an olc struct.
       * This way we can let redit_save_internally handle the room adding.
       */
      if (d->olc) {
        mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: buildwalk(): Player already had olc structure.");
        free(d->olc);
      }
      CREATE(d->olc, struct oasis_olc_data, 1);
      OLC_ZNUM(d) = rroom_get_zone(g, char_get_in_room(ch));
      OLC_NUM(d)  = vnum;
      CREATE(OLC_ROOM(d), struct room_data, 1);

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
      rnum = real_room(g, vnum);
      {
        struct room_direction_data *new_exit;
        CREATE(new_exit, struct room_direction_data, 1);
        char_set_exit(g, ch, dir, new_exit);
      }
      char_get_exit(g, ch, dir)->to_room = rnum;
      struct room_direction_data *back_exit;
      CREATE(back_exit, struct room_direction_data, 1);
      rroom_set_dir_option(g, rnum, rev_dir[dir], back_exit);
      rroom_get_dir_option(g, rnum, rev_dir[dir])->to_room = char_get_in_room(ch);

      /* Report room creation to user */
      send_to_char(g, ch, "@yRoom #%d created by BuildWalk.@n\r\n", vnum);
      cleanup_olc(g, d, CLEANUP_STRUCTS);
      update_space(g);
      return true;
    }
  }

  return false;
}
