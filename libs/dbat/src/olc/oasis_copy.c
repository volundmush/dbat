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
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/config.h"
#include "dbat/genolc.h"
#include "dbat/genwld.h"
#include "dbat/genshp.h"
#include "dbat/dg_scripts.h"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/

static room_vnum redit_find_new_vnum(zone_rnum zone);

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
  struct descriptor_data *d = ch->desc; /* will save us some typing */

  /* Grab the room's name (if available). */
  new_room_name = two_arguments(argument, sdir, sroom);
  skip_spaces(&new_room_name);

  /* Can't dig if we don't know where to go. */
  if (!*sdir || !*sroom) {
    send_to_char(ch, "Format: tunnel <direction> <room> - to create an exit\r\n"
                     "        tunnel <direction> -1     - to delete an exit\r\n");
    return;
  }

  rawvnum = atoi(sroom);
  if (rawvnum == -1)
    rvnum = NOWHERE;
  else
    rvnum = (room_vnum)rawvnum;
  rrnum = real_room(rvnum);
  if ((dir = search_block(sdir, abbr_dirs, FALSE)) < 0)
    dir = search_block(sdir, dirs, FALSE);
  zone = world[char_get_in_room(ch)].zone;

  if (dir < 0) {
    send_to_char(ch, "Cannot create an exit to the '%s'.\r\n", sdir);
    return;
  }
  /* Make sure that the builder has access to the zone he's in. */
  if ((zone == NOWHERE) || !can_edit_zone(ch, zone)) {
    send_cannot_edit(ch, zone);
    return;
  }
  /*
   * Lets not allow digging to limbo. 
   * After all, it'd just get us more errors on 'show errors'
   */
  if (rvnum == 0) {
    send_to_char(ch, "The target exists, but you can't dig to limbo!\r\n");
    return;
  }
  /*
   * target room == -1 removes the exit 
   */
  if (rvnum == NOTHING) {
    if (rroom_get_dir_option(char_get_in_room(ch), dir)) {
      /* free the old pointers, if any */
      if (rroom_get_dir_option(char_get_in_room(ch), dir)->general_description)
        free(rroom_get_dir_option(char_get_in_room(ch), dir)->general_description);
      if (rroom_get_dir_option(char_get_in_room(ch), dir)->keyword)
        free(rroom_get_dir_option(char_get_in_room(ch), dir)->keyword);
      free(rroom_get_dir_option(char_get_in_room(ch), dir));
      rroom_set_dir_option(char_get_in_room(ch), dir, NULL);
      add_to_save_list(zone_table[world[char_get_in_room(ch)].zone].number, SL_WLD);
      send_to_char(ch, "You remove the exit to the %s.\r\n", dirs[dir]);
      return;
    }
    send_to_char(ch, "There is no exit to the %s.\r\n"
                     "No exit removed.\r\n",
                 dirs[dir]);
    return;
  }
  /*
   * Can't dig in a direction, if it's already a door. 
   */
  if (rroom_get_dir_option(char_get_in_room(ch), dir)) {
    send_to_char(ch, "There already is an exit to the %s.\r\n", dirs[dir]);
    return;
  }

  /* Make sure that the builder has access to the zone he's linking to. */
  zone = real_zone_by_thing(rvnum);
  if (zone == NOWHERE) {
    send_to_char(ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(ch, zone)) {
    send_cannot_edit(ch, zone);
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
      mudlog(LOG_BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_dig: Player already had olc structure.");
      free(d->olc);
    }
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZNUM(d) = zone;
    OLC_NUM(d)  = rvnum;
    CREATE(OLC_ROOM(d), struct room_data, 1);

    /* Copy the room's name. */
    if (*new_room_name)
      OLC_ROOM(d)->name = strdup(new_room_name);
    else
      OLC_ROOM(d)->name = strdup("An unfinished room");

    /* Copy the room's description.*/
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    OLC_ROOM(d)->zone        = OLC_ZNUM(d);
    OLC_ROOM(d)->number      = NOWHERE;

    /*
     * Save the new room to memory.
     * redit_save_internally handles adding the room in the right place, etc.
     */
    redit_save_internally(d);
    OLC_VAL(d) = 0;

    send_to_char(ch, "New room (%d) created.\r\n", rvnum);
    cleanup_olc(d, CLEANUP_ALL);
    update_space();
    /* 
     * update rrnum to the correct room rnum after adding the room 
     */
    rrnum = real_room(rvnum);
  }

  /*
   * Now dig.
   */
  {
    struct room_direction_data *new_exit;
    CREATE(new_exit, struct room_direction_data, 1);
    rroom_set_dir_option(char_get_in_room(ch), dir, new_exit);
  }
  rroom_get_dir_option(char_get_in_room(ch), dir)->general_description = NULL;
  rroom_get_dir_option(char_get_in_room(ch), dir)->keyword             = NULL;
  rroom_get_dir_option(char_get_in_room(ch), dir)->to_room             = rrnum;
  add_to_save_list(zone_table[world[char_get_in_room(ch)].zone].number, SL_WLD);
  save_rooms(zone_table[world[rrnum].zone].number);
  send_to_char(ch, "You make an exit %s to room %d (%s).\r\n",
               dirs[dir], rvnum, world[rrnum].name);

  /* 
   * check if we can dig from there to here. 
   */
  if (rroom_get_dir_option(rrnum, rev_dir[dir]))
    send_to_char(ch, "You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                 rvnum, dirs[rev_dir[dir]]);
  else {
    {
      struct room_direction_data *new_exit;
      CREATE(new_exit, struct room_direction_data, 1);
      rroom_set_dir_option(rrnum, rev_dir[dir], new_exit);
    }
    rroom_get_dir_option(rrnum, rev_dir[dir])->general_description = NULL;
    rroom_get_dir_option(rrnum, rev_dir[dir])->keyword             = NULL;
    rroom_get_dir_option(rrnum, rev_dir[dir])->to_room             = char_get_in_room(ch);
    add_to_save_list(zone_table[world[rrnum].zone].number, SL_WLD);
    save_rooms(zone_table[world[rrnum].zone].number);
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
    send_to_char(ch, "Syntax: rclone <base rnum> <target rnum>\r\nOr be in the room you wish to be the target and provide base vnum.\r\n");
    return;
  }

  if (!*arg2) {
    tvnum = atoi(arg);
    rvnum = rroom_get_vnum(char_get_in_room(ch));
  } else {
    rvnum = atoi(arg2);
    tvnum = atoi(arg);
  }

  if (rvnum == 0 || tvnum == 0) {
    send_to_char(ch, "The void is fine as it is, try again.\r\n");
    return;
  }
  if (rvnum == tvnum) {
    send_to_char(ch, "Don't copy a room to its self!\r\n");
    return;
  }

  trnum = real_room(tvnum);
  rrnum = real_room(rvnum);

  if (trnum == NOWHERE) {
    send_to_char(ch, "Could not find base room: %d\r\n", tvnum);
    return;
  }
  if (rrnum == NOWHERE) {
    send_to_char(ch, "Could not find target room: %d\r\n", rvnum);
    return;
  }

  zone = world[rrnum].zone;
  if ((zone == NOWHERE) || !can_edit_zone(ch, zone)) {
    send_to_char(ch, "\r\n");
    send_cannot_edit(ch, zone);
    return;
  }

  /* Free descriptions. */
  if (world[rrnum].name)
    free(world[rrnum].name);
  if (world[rrnum].description)
    free(world[rrnum].description);
  if (world[rrnum].ex_description)
    free_ex_descriptions(world[rrnum].ex_description);
  world[rrnum].sector_type = world[trnum].sector_type;

  /* Copy over description name and extra descriptions */
  world[rrnum].description = str_udup(world[trnum].description);
  world[rrnum].name        = str_udup(world[trnum].name);

  /* Copy over any existings extra descriptions */
  if (world[trnum].ex_description)
    copy_ex_descriptions(&world[rrnum].ex_description, world[trnum].ex_description);
  else
    world[rrnum].ex_description = NULL;

  /* Finally copy over room flags */
  for (i = 0; i < AF_ARRAY_MAX; i++) {
    world[rrnum].room_flags[i] = world[trnum].room_flags[i];
  }
  send_to_imm("Log: %s has copied room [%d] to room [%d].", char_get_name(ch), tvnum, rvnum);
  add_to_save_list(zone_table[world[rrnum].zone].number, SL_WLD);
  save_rooms(zone_table[world[rrnum].zone].number);
  send_to_char(ch, "Room [%d] copied to room [%d].\r\n", tvnum, rvnum);
}

/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
static room_vnum redit_find_new_vnum(zone_rnum zone)
{
  room_vnum vnum = genolc_zone_bottom(zone);
  room_rnum rnum = real_room(vnum);

  if (rnum == NOWHERE)
    return NOWHERE;

  for (;;) {
    if (vnum > zone_table[zone].top)
      return (NOWHERE);
    if (rnum > top_of_world || world[rnum].number > vnum)
      break;
    rnum++;
    vnum++;
  }
  return (vnum);
}

int buildwalk(struct char_data *ch, int dir)
{
  char buf[MAX_INPUT_LENGTH];
  room_vnum vnum;
  room_rnum rnum;

  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_BUILDWALK) &&
      char_get_admlevel(ch) >= ADMLVL_IMMORT) {

    if (!can_edit_zone(ch, world[char_get_in_room(ch)].zone)) {
      send_cannot_edit(ch, world[char_get_in_room(ch)].zone);
    } else if ((vnum = redit_find_new_vnum(world[char_get_in_room(ch)].zone)) == NOWHERE)
      send_to_char(ch, "No free vnums are available in this zone!\r\n");
    else {
      struct descriptor_data *d = ch->desc;
      /*
       * Give the descriptor an olc struct.
       * This way we can let redit_save_internally handle the room adding.
       */
      if (d->olc) {
        mudlog(LOG_BRF, ADMLVL_IMMORT, TRUE, "SYSERR: buildwalk(): Player already had olc structure.");
        free(d->olc);
      }
      CREATE(d->olc, struct oasis_olc_data, 1);
      OLC_ZNUM(d) = world[char_get_in_room(ch)].zone;
      OLC_NUM(d)  = vnum;
      CREATE(OLC_ROOM(d), struct room_data, 1);

      OLC_ROOM(d)->name = strdup("New BuildWalk Room");

      sprintf(buf, "This unfinished room was created by %s.\r\n", char_get_name(ch));
      OLC_ROOM(d)->description = strdup(buf);
      OLC_ROOM(d)->zone        = OLC_ZNUM(d);
      OLC_ROOM(d)->number      = NOWHERE;

      /*
       * Save the new room to memory.
       * redit_save_internally handles adding the room in the right place, etc.
       */
      redit_save_internally(d);
      OLC_VAL(d) = 0;

      /* Link rooms */
      rnum = real_room(vnum);
      {
        struct room_direction_data *new_exit;
        CREATE(new_exit, struct room_direction_data, 1);
        char_set_exit(ch, dir, new_exit);
      }
      char_get_exit(ch, dir)->to_room = rnum;
      CREATE(world[rnum].dir_option[rev_dir[dir]], struct room_direction_data, 1);
      world[rnum].dir_option[rev_dir[dir]]->to_room = char_get_in_room(ch);

      /* Report room creation to user */
      send_to_char(ch, "@yRoom #%d created by BuildWalk.@n\r\n", vnum);
      cleanup_olc(d, CLEANUP_STRUCTS);
      update_space();
      return (1);
    }
  }

  return (0);
}
