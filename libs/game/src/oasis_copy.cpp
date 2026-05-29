/******************************************************************************/
/** OasisOLC - InGame OLC Copying                                      v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include "dbat/game/oasis_copy.h"
#include "dbat/game/dg_comm.h"
#include "dbat/game/dg_olc.h"
#include "dbat/game/act.wizard.h"
#include "dbat/game/utils.h"
#include "dbat/game/interpreter.h"
#include "dbat/game/comm.h"
#include "dbat/game/db.h"
#include "dbat/game/config.h"
#include "dbat/game/genolc.h"
#include "dbat/game/genwld.h"
#include "dbat/game/genshp.h"
#include "dbat/game/dg_scripts.h"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/

room_vnum redit_find_new_vnum(struct zone_data* zone);


/* External Functions */



void redit_save_internally(struct descriptor_data *d);
void oedit_save_internally(struct descriptor_data *d);
void medit_save_internally(struct descriptor_data *d);
void sedit_save_internally(struct descriptor_data *d);
void trigedit_setup_existing(struct descriptor_data *d, room_vnum num);
void redit_setup_existing(struct descriptor_data *d, int rnum);
void oedit_setup_existing(struct descriptor_data *d, int rnum);
void medit_setup_existing(struct descriptor_data *d, int rnum);
void sedit_setup_existing(struct descriptor_data *d, int rnum);


/***********************************************************
* This function makes use of the high level OLC functions  *
* to copy most types of mud objects. The type of data is   *
* determined by the subcmd variable and the functions are  *
* looked up in a table.                                    *
***********************************************************/
ACMD(do_oasis_copy)
{
  int i, src_vnum, src_rnum, dst_vnum, dst_rnum;
  char buf1[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  struct descriptor_data *d;

  struct {
    int con_type;
    IDXTYPE (*binary_search)(IDXTYPE vnum);
    void (*save_func)(struct descriptor_data *d);
    void (*setup_existing)(struct descriptor_data *d, int rnum);
    const char *command;
    const char *text;
  } oasis_copy_info[] = {
    { CON_REDIT,  real_room,   redit_save_internally, redit_setup_existing, "rcopy", "room" },
    { CON_OEDIT,  real_object, oedit_save_internally, oedit_setup_existing, "ocopy", "object" },
    { CON_MEDIT,  mob_proto_vnum_check, medit_save_internally, medit_setup_existing, "mcopy", "mobile" },
    { CON_SEDIT,  real_shop,   sedit_save_internally, sedit_setup_existing, "scopy", "shop" },
    { CON_TRIGEDIT, real_trigger, trigedit_save,   trigedit_setup_existing, "tcopy", "trigger" },
    { -1,         NULL,        NULL,                  NULL,                 "\n", "\n" }
  };

  /* Find the given connection type in the table (passed in subcmd). */
  for (i = 0; *(oasis_copy_info[i].text) != '\n'; i++)
    if (subcmd == oasis_copy_info[i].con_type)
      break;
  /* If not found, we don't support copying that type of data. */
  if (*(oasis_copy_info[i].text) == '\n')
    return;

  /* No copying as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* We need two arguments. */
  two_arguments(argument, buf1, buf2);

  /* Both arguments are required and they must be numeric. */
  if (!*buf1 || !*buf2 || (!is_number(buf1) || !is_number(buf2))) {
    send_to_char(ch, "Syntax: %s <source vnum> <target vnum>\r\n", oasis_copy_info[i].command);
    return;
  }

  /* We can't copy non-existing data. */
  /* Note: the source data can be in any zone. It's not restricted */
  /* to the builder's designated OLC zone. */
  src_vnum = atoi(buf1);
  src_rnum = (*oasis_copy_info[i].binary_search)(src_vnum);
  if (src_rnum == NOWHERE) {
    send_to_char(ch, "The source %s (#%d) does not exist.\r\n", oasis_copy_info[i].text, src_vnum);
    return;
  }

  /* Don't copy if the target already exists. */
  dst_vnum = atoi(buf2);
  dst_rnum = (*oasis_copy_info[i].binary_search)(dst_vnum);
  if (dst_rnum != NOWHERE) {
    send_to_char(ch, "The target %s (#%d) already exists.\r\n", oasis_copy_info[i].text, dst_vnum);
    return;
  }

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == subcmd) {
      if (d->olc && OLC_NUM(d) == dst_vnum) {
       send_to_char(ch, "The target %s (#%d) is currently being edited by %s.\r\n",
       oasis_copy_info[i].text, dst_vnum, GET_NAME(d->character));
        return;
      }
    }
  }

  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_oasis_copy: Player already had olc structure.");
    free(d->olc);
  }

  /* Create the OLC structure. */
  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  if ((OLC_ZNUM(d) = virtual_zone_by_thing(dst_vnum)) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for the given vnum (#%d)!\r\n", dst_vnum);
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* Make sure the builder is allowed to modify the target zone. */
  auto zone = zone_by_id(OLC_ZNUM(d));
  if (!can_edit_zone(ch, zone)) {
    send_cannot_edit(ch, zone->number);
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* We tell the OLC functions that we want to save to the target vnum. */
  OLC_NUM(d) = dst_vnum;

  /* Perform the copy. */
  send_to_char(ch, "Copying %s: source: #%d, dest: #%d.\r\n", oasis_copy_info[i].text, src_vnum, dst_vnum);
  (*oasis_copy_info[i].setup_existing)(d, src_rnum);
  (*oasis_copy_info[i].save_func)(d);

  /* Currently CLEANUP_ALL should be used for everything. */
  cleanup_olc(d, CLEANUP_ALL);
  send_to_char(ch, "Done.\r\n");
}

/******************************************************************************/
/** Commands                                                                 **/
/******************************************************************************/

ACMD(do_dig)
{
  char sdir[MAX_INPUT_LENGTH], sroom[MAX_INPUT_LENGTH], *new_room_name;
  room_vnum rvnum = NOWHERE;
  room_rnum rrnum = NOWHERE;
  int dir = 0, rawvnum;
  struct room_data *trm = NULL, *rrm = NULL;
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
  rrm = room_by_id(rvnum);
  if ((dir = search_block(sdir, abbr_dirs, FALSE)) < 0)
  dir = search_block(sdir, dirs, FALSE);
  auto zone = char_zone_get(ch);

  if (dir < 0) {
    send_to_char(ch, "Cannot create an exit to the '%s'.\r\n", sdir);
    return;
  }
  /* Make sure that the builder has access to the zone he's in. */
  if (!can_edit_zone(ch, zone)) {
    send_cannot_edit(ch, zone->number);
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
    struct room_data *rm = char_room_get(ch);
    struct room_direction_data *exit = R_EXIT(rm, dir);
    if (exit) {
      /* free the old pointers, if any */
      if (exit->general_description)
        free(exit->general_description);
      if (exit->keyword)
        free(exit->keyword);
      free(exit);
      R_EXIT(rm, dir) = NULL;
      add_to_save_list(room_zone_vnum_get(rm), SL_WLD);
      send_to_char(ch, "You remove the exit to the %s.\r\n", dirs[dir]);
      return;
    }
    send_to_char(ch, "There is no exit to the %s.\r\n"
                     "No exit removed.\r\n", dirs[dir]);
    return;
  }  
  /*
   * Can't dig in a direction, if it's already a door. 
   */
  if (EXIT(ch, dir)) {
      send_to_char(ch, "There already is an exit to the %s.\r\n", dirs[dir]);
      return;
  }
  
  /* Make sure that the builder has access to the zone he's linking to. */
  auto z2 = room_zone_get(rrm);
  if (!z2) {
    send_to_char(ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(ch, z2)) {
    send_cannot_edit(ch, z2->number);
    return;
  }
  /*
   * Now we know the builder is allowed to make the link 
   */
  /* If the room doesn't exist, create it.*/
  if (rrm == NULL) {
    /*
     * Give the descriptor an olc struct.
     * This way we can let redit_save_internally handle the room adding.
     */
    if (d->olc) {
      mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_dig: Player already had olc structure.");
      free(d->olc);
    }
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZNUM(d) = zone->number;
    OLC_NUM(d) = rvnum;
    CREATE(OLC_ROOM(d), struct room_data, 1);
    
    
    /* Copy the room's name. */
    if (*new_room_name)
     OLC_ROOM(d)->name = strdup(new_room_name);
    else
     OLC_ROOM(d)->name = strdup("An unfinished room");
    
    /* Copy the room's description.*/
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    OLC_ROOM(d)->zone = OLC_ZNUM(d);
    OLC_ROOM(d)->number = NOWHERE;
    
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
    rrm = room_by_id(rvnum);
  }

  /*
   * Now dig.
   */
  struct room_data *rm = char_room_get(ch);
  CREATE(rm->dir_option[dir], struct room_direction_data, 1);
  struct room_direction_data *new_exit = rm->dir_option[dir];
  new_exit->to_room = rvnum;
  auto room_zone = zone_by_id(room_zone_vnum_get(rm));
  add_to_save_list(room_zone->number, SL_WLD);
  save_rooms(room_zone);
  send_to_char(ch, "You make an exit %s to room %d (%s).\r\n", 
                   dirs[dir], rvnum, rrm->name);
  
  struct room_data *dest = exit_dest_get(new_exit);

  /* 
   * check if we can dig from there to here. 
   */
  if (R_EXIT(dest, rev_dir[dir])) 
    send_to_char(ch, "You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                     rvnum, dirs[rev_dir[dir]]);
  else {
    CREATE(R_EXIT(dest, rev_dir[dir]), struct room_direction_data, 1);
    struct room_direction_data *rev_ex = R_EXIT(dest, rev_dir[dir]);
    rev_ex->to_room = char_room_vnum_get(ch);
    auto dest_zone = zone_by_id(rrm->zone);
    add_to_save_list(dest_zone->number, SL_WLD);
    save_rooms(dest_zone);
  }
}

ACMD(do_rcopy) 
{ 
  room_vnum rvnum; 
  room_rnum rrnum; 
  room_rnum trnum;
  room_rnum tvnum;

  char arg[MAX_INPUT_LENGTH]; 
  char arg2[MAX_INPUT_LENGTH];
  int i;

  two_arguments(argument, arg, arg2); 

  if (!arg || !*arg) { 
    send_to_char(ch, "Syntax: rclone <base rnum> <target rnum>\r\nOr be in the room you wish to be the target and provide base vnum.\r\n"); 
    return; 
  } 

  if (!arg2 || !*arg2) {
     tvnum = atoi(arg);
     rvnum = char_room_vnum_get(ch);
  }
  else if (arg2 || *arg2) {
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
  
  struct room_data* rrm = room_by_id(rvnum);
  struct room_data* trm = room_by_id(tvnum);

  if (trm == NULL) { 
    send_to_char(ch, "Could not find base room: %d\r\n", tvnum); 
    return; 
  }
  if (rrm == NULL) { 
    send_to_char(ch, "Could not find target room: %d\r\n", rvnum); 
    return; 
  }

  auto zone = zone_by_id(rrm->zone);
  if (!can_edit_zone(ch, zone)) {
    send_to_char(ch, "\r\n");
    send_cannot_edit(ch, zone->number);
    return;
  }


  /* Free descriptions. */ 
  if (rrm->name) 
    free(rrm->name); 
  if (rrm->description) 
    free(rrm->description); 
  if (rrm->ex_description) 
    free_ex_descriptions(rrm->ex_description); 
  rrm->sector_type = trm->sector_type;
  
  /* Copy over description name and extra descriptions */ 
  rrm->description = str_udup(trm->description); 
  rrm->name = str_udup(trm->name); 

  /* Copy over any existings extra descriptions */ 
 if (trm->ex_description) 
    copy_ex_descriptions(&rrm->ex_description, trm->ex_description); 
 else 
   rrm->ex_description = NULL; 

  /* Finally copy over room flags */ 
  for(i=0; i < AF_ARRAY_MAX; i++)  {
   rrm->room_flags[i] = trm->room_flags[i]; 
  }
  send_to_imm("Log: %s has copied room [%d] to room [%d].", GET_NAME(ch), tvnum, rvnum);
  auto room_zone = zone_by_id(rrm->zone);
  add_to_save_list(room_zone->number, SL_WLD);
  save_rooms(room_zone);
  send_to_char(ch, "Room [%d] copied to room [%d].\r\n", tvnum, rvnum); 
}

/****************************************************************************
* BuildWalk - OasisOLC Extension by D. Tyler Barnes                         *
****************************************************************************/

/* For buildwalk. Finds the next free vnum in the zone */
room_vnum redit_find_new_vnum(struct zone_data* zone) 
{

  for(room_vnum vnum = zone->bot; vnum <= zone->top; vnum++)
    if (!room_by_id(vnum))
      return vnum;
  
      return NOWHERE;
}

int buildwalk(struct char_data *ch, int dir)
{
  char buf[MAX_INPUT_LENGTH];
  room_vnum vnum;
  room_rnum rnum;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BUILDWALK) &&
      GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
    
    auto zone = char_zone_get(ch);

    if (!can_edit_zone(ch, zone)) {
      send_cannot_edit(ch, zone->number);
    } else if ((vnum = redit_find_new_vnum(zone)) == NOWHERE)
      send_to_char(ch, "No free vnums are available in this zone!\r\n");
    else {
      struct descriptor_data *d = ch->desc;
      /*
       * Give the descriptor an olc struct.
       * This way we can let redit_save_internally handle the room adding.
       */
      if (d->olc) {
        mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: buildwalk(): Player already had olc structure.");
        free(d->olc);
      }
      CREATE(d->olc, struct oasis_olc_data, 1);
      OLC_ZNUM(d) = zone->number;
      OLC_NUM(d) = vnum;
      CREATE(OLC_ROOM(d), struct room_data, 1);

      OLC_ROOM(d)->name = strdup("New BuildWalk Room");

      sprintf(buf, "This unfinished room was created by %s.\r\n", GET_NAME(ch));
      OLC_ROOM(d)->description = strdup(buf);
      OLC_ROOM(d)->zone = OLC_ZNUM(d);
      OLC_ROOM(d)->number = NOWHERE;

      /*
       * Save the new room to memory.
       * redit_save_internally handles adding the room in the right place, etc.
       */
      redit_save_internally(d);
      OLC_VAL(d) = 0;

      /* Link rooms */
      struct room_data *trm = room_by_id(vnum);
      struct room_data *rm = char_room_get(ch);
      CREATE(rm->dir_option[dir], struct room_direction_data, 1);
      rm->dir_option[dir]->to_room = vnum;
      CREATE(trm->dir_option[rev_dir[dir]], struct room_direction_data, 1);
      trm->dir_option[rev_dir[dir]]->to_room = room_vnum_get(rm);

      /* Report room creation to user */
      send_to_char(ch, "@yRoom #%d created by BuildWalk.@n\r\n", vnum);
      cleanup_olc(d, CLEANUP_STRUCTS);
      update_space();
      return (1);

    }
  }

  return(0);
}
