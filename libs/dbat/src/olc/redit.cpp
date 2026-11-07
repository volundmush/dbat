/************************************************************************
 *  OasisOLC - Rooms / redit.c					v2.0	*
 *  Original author: Levork						*
 *  Copyright 1996 Harvey Gilpin					*
 *  Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <cstdlib>

#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/config.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/boards.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genwld.hpp"
#include "dbat/genzon.hpp"
#include "dbat/oasis.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/races.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/maputils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void redit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void redit_setup_existing(GameWorld *g, struct descriptor_data *d, room_rnum real_num);
static void redit_setup_new(struct descriptor_data *d);

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */

/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

ACMD(do_oasis_redit)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  IDXTYPE number = NOWHERE;
  int save       = 0;
  room_rnum real_num;
  struct descriptor_data *d;

  /* Parse any arguments. */
  if (char_get_admlevel(ch) > 0) {
    two_arguments(argument, buf1, buf2);
  }

  if (char_get_admlevel(ch) < 1 && !rroom_flagged(g, char_get_in_room(ch), ROOM_CANREMODEL)) {
    send_to_char(g, ch, "You can not remodel this room.\r\n");
    return;
  }

  struct obj_data *capsule = nullptr, *next_obj = nullptr, *remove = nullptr;
  int remodeling = false;

  for (capsule = char_get_inventory(ch); capsule; capsule = next_obj) {
    next_obj = obj_get_next_content(capsule);
    if (remove != nullptr) {
      continue;
    }
    if (obj_get_vnum(g, capsule) == 19094) {
      remove = capsule;
    }
  }

  if (remove == nullptr && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "You do not have a R.A.D. Remodeling Assistance Droid.\r\n");
    return;
  }
  if (char_get_admlevel(ch) < 1) {
    act(g, "@GYou open up the computer panel on the droid and begin to program its remodeling routine.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@g$n@G opens up a computer panel on some kind of small spherical droid and begins to program it.@n", true, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, remove);
    remodeling = true;
  }

  if (!*buf1 || char_get_admlevel(ch) < 1)
    number = rroom_get_vnum(g, char_get_in_room(ch));
  else if (!isdigit(*buf1)) {
    if (strcasecmp("save", buf1) != 0) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = true;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (char_get_olc_zone(g, ch) >= 0) {
      zone_rnum zlok;

      if ((zlok = real_zone(g, char_get_olc_zone(g, ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(g, zlok);
    }

    if (number == NOWHERE) {
      send_to_char(g, ch, "Save which zone?\r\n");
      return;
    }
  }

  /*
   * If a numeric argument was given (like a room number), get it.
   */
  if (number == NOWHERE)
    number = atoi(buf1);

  /* Check to make sure the room isn't already being edited. */
  for (d = g->descriptors; d; d = d->next) {
    if (desc_get_connected(d) == CON_REDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(g, ch, "That room is currently being edited by %s.\r\n",
                     char_pers(g, d->character, ch));
        return;
      }
    }
  }

  /* Retrieve the player's descriptor. */
  d = char_get_desc(ch);

  /* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis_redit: Player already had olc structure.");
    delete d->olc;
  }

  /* Create the OLC structure. */
  d->olc = new oasis_olc_data();

  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(g, number) : real_zone_by_thing(g, number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  /* Make sure the builder is allowed to modify this zone. */
  if (!can_edit_zone(g, ch, OLC_ZNUM(d)) && remodeling == false) {
    send_cannot_edit(g, ch, g->zones[OLC_ZNUM(d)].number);
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  if (save) {
    send_to_char(g, ch, "Saving all rooms in zone %d.\r\n", g->zones[OLC_ZNUM(d)].number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: %s saves room info for zone %d.", char_get_name(ch), g->zones[OLC_ZNUM(d)].number);

    /* Save the rooms. */
    save_rooms(g, OLC_ZNUM(d));

    /* Free the olc data from the descriptor. */
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  if ((real_num = real_room(g, number)) != NOWHERE)
    redit_setup_existing(g, d, real_num);
  else
    redit_setup_new(d);

  redit_disp_menu(g, d);
  desc_set_connected(d, CON_REDIT);
  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
         char_get_name(ch), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));
}

static void redit_setup_new(struct descriptor_data *d)
{
  OLC_ROOM(d) = new room_data();

  room_set_name(OLC_ROOM(d), strdup("An unfinished room"));
  room_set_description(OLC_ROOM(d), strdup("You are in an unfinished room.\r\n"));
  room_set_number(OLC_ROOM(d), NOWHERE);
  OLC_ITEM_TYPE(d)         = WLD_TRIGGER;
  OLC_SCRIPT(d)            = nullptr;

  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

static void redit_setup_existing(GameWorld *g, struct descriptor_data *d, room_rnum real_num)
{
  struct room_data *room;
  int counter;

  /*
   * Build a copy of the room for editing.
   */
  room = new room_data();

  *room = g->rooms[real_num];
  /*
   * Allocate space for all strings.
   */
  room_set_name(room, str_udup(rroom_get_name(g, real_num)));
  room_set_description(room, str_udup(rroom_get_description(g, real_num)));

  /*
   * Exits - We allocate only if necessary.
   */
  for (counter = 0; counter < NUM_OF_DIRS; counter++) {
    if (rroom_get_dir_option(g, real_num, counter)) {
      struct room_direction_data *new_exit;
      new_exit = new room_direction_data();
      room_set_dir_option(room, counter, new_exit);

      /*
       * Copy the numbers over.
       */
      *room_get_dir_option(room, counter) = *rroom_get_dir_option(g, real_num, counter);
      /*
       * Allocate the strings.
       */
      if (rroom_get_dir_option(g, real_num, counter)->general_description)
        room_get_dir_option(room, counter)->general_description = strdup(rroom_get_dir_option(g, real_num, counter)->general_description);
      if (rroom_get_dir_option(g, real_num, counter)->keyword)
        room_get_dir_option(room, counter)->keyword = strdup(rroom_get_dir_option(g, real_num, counter)->keyword);
    }
  }

  /*
   * Extra descriptions, if necessary.
   */
  if (rroom_get_ex_description(g, real_num)) {
    struct extra_descr_data *tdesc, *temp, *temp2;
    temp = new extra_descr_data();

    room_set_ex_description(room, temp);
    for (tdesc = rroom_get_ex_description(g, real_num); tdesc; tdesc = tdesc->next) {
      temp->keyword     = strdup(tdesc->keyword);
      temp->description = strdup(tdesc->description);
      if (tdesc->next) {
        temp2      = new extra_descr_data();
        temp->next = temp2;
        temp       = temp2;
      }
    }
  }
  /*
   * Attach copy of room to player's descriptor.
   */
  OLC_ROOM(d)      = room;
  OLC_VAL(d)       = 0;
  OLC_ITEM_TYPE(d) = WLD_TRIGGER;

  dg_olc_script_copy(d);
  room_set_proto_script(room, nullptr);
  room_set_script(room, nullptr);
}

/*------------------------------------------------------------------------*/

void redit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  int j, new_room = false;
  room_rnum room_num;
  struct descriptor_data *dsc;

  if (room_get_number(OLC_ROOM(d)) == NOWHERE) {
    new_room = true;
  }
  room_set_number(OLC_ROOM(d), OLC_NUM(d));
  /* FIXME: Why is this not set elsewhere? */
  room_set_zone(OLC_ROOM(d), OLC_ZNUM(d));

  if ((room_num = add_room(g, OLC_ROOM(d))) == NOWHERE) {
    write_to_output(g, d, "Something went wrong...\r\n");
    mud_log("SYSERR: redit_save_internally: Something failed! (%d)", room_num);
    return;
  }

  /* Update triggers */
  /* Free old proto list */
  if (rroom_get_proto_script(g, room_num) &&
      rroom_get_proto_script(g, room_num) != OLC_SCRIPT(d))
    free_proto_script(&g->rooms[room_num], WLD_TRIGGER);

  rroom_set_proto_script(g, room_num, OLC_SCRIPT(d));
  assign_triggers(g, &g->rooms[room_num], WLD_TRIGGER);
  /* end trigger update */

  /* Don't adjust numbers on a room update. */
  if (!new_room)
    return;

  /* Idea contributed by C.Raehl 4/27/99 */
  for (dsc = g->descriptors; dsc; dsc = dsc->next) {
    if (dsc == d)
      continue;

    if (desc_get_connected(dsc) == CON_ZEDIT) {
      for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
        switch (OLC_ZONE(dsc)->cmd[j].command) {
          case 'O':
          case 'M':
          case 'T':
          case 'V':
            OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
            break;
          case 'D':
            OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
            [[fallthrough]];
          case 'R':
            OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
            break;
        }
    } else if (desc_get_connected(dsc) == CON_REDIT) {
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (room_get_dir_option(OLC_ROOM(dsc), j))
          if (room_get_dir_option(OLC_ROOM(dsc), j)->to_room >= room_num)
            room_get_dir_option(OLC_ROOM(dsc), j)->to_room++;
    }
  }
}

/*------------------------------------------------------------------------*/

static void redit_save_to_disk(GameWorld *g, zone_vnum zone_num)
{
  save_rooms(g, zone_num); /* :) */
  update_space(g);
}

/*------------------------------------------------------------------------*/

void free_room(GameWorld *g, struct room_data *room)
{
  /* Free the strings (Mythran). */
  free_room_strings(room);

  if (room_get_script(room))
    extract_script(g, room, WLD_TRIGGER);
  free_proto_script(room, WLD_TRIGGER);

  /* Free the room. */
  delete room; /* XXX ? */
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
static void redit_disp_extradesc_menu(GameWorld *g, struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  clear_screen(g, d);
  write_to_output(g, d,
                  "@g1@n) Keyword: @y%s\r\n"
                  "@g2@n) Description:\r\n@y%s\r\n"
                  "@g3@n) Goto next description: ",

                  extra_desc->keyword ? extra_desc->keyword : "<NONE>",
                  extra_desc->description ? extra_desc->description : "<NONE>");

  write_to_output(g, d, !extra_desc->next ? "Not Set.\r\n" : "Set.\r\n");
  write_to_output(g, d, "Enter choice (0 to quit) : ");
  OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
static void redit_disp_exit_menu(GameWorld *g, struct descriptor_data *d)
{
  char door_buf[40];
  /*
   * if exit doesn't exist, alloc/create it 
   */
  if (OLC_EXIT(d) == nullptr) {
    struct room_direction_data *new_exit;
    new_exit = new room_direction_data();
    room_set_dir_option(OLC_ROOM(d), OLC_VAL(d), new_exit);
  }

  sprintbit(OLC_EXIT(d)->exit_info, exit_bits, door_buf, sizeof(door_buf));

  clear_screen(g, d);
  write_to_output(g, d,
                  "@g1@n) Exit to     		: @c%d\r\n"
                  "@g2@n) Description 		:-\r\n@y%s\r\n"
                  "@g3@n) Door name   		: @y%s\r\n"
                  "@g4@n) Key         		: @c%d\r\n"
                  "@g5@n) Door flags  		: @c%s\r\n"
                  "@g6@n) Purge exit.\r\n"
                  "@g7@n) DC Lock		: @c%d\r\n"
                  "@g8@n) DC Hide     		: @c%d\r\n"
                  "@g9@n) DC Skill		: @c%s\r\n"
                  "@gA@n) DC Move		: @c%d\r\n"
                  "@gB@n) Skill Fail Save Type		: @c%d\r\n"
                  "@gC@n) DC Skill Save		: @c%d\r\n"
                  "@gD@n) Minor Fail Dest. Room	: @c%d\r\n"
                  "@gE@n) Major Fail Dest. Room	: @c%d@n\r\n"
                  "Enter choice, 0 to quit : ",

                  OLC_EXIT(d)->to_room != NOWHERE ? rroom_get_vnum(g, OLC_EXIT(d)->to_room) : -1,
                  OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>",
                  OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
                  OLC_EXIT(d)->key != NOTHING ? OLC_EXIT(d)->key : -1,
                  door_buf, OLC_EXIT(d)->dclock, OLC_EXIT(d)->dchide,
                  OLC_EXIT(d)->dcskill != 0 ? g->spell_info[OLC_EXIT(d)->dcskill].name : "<NONE>",
                  OLC_EXIT(d)->dcmove, OLC_EXIT(d)->failsavetype,
                  OLC_EXIT(d)->dcfailsave, OLC_EXIT(d)->failroom,
                  OLC_EXIT(d)->totalfailroom);

  OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
static void redit_disp_exit_flag_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_INPUT_LENGTH];

  clear_screen(g, d);
  olc_list_columns(g, d, exit_bits, NUM_EXIT_FLAGS, 1, "", 2);
  sprintbit(OLC_EXIT(d)->exit_info, exit_bits, bits, sizeof(bits));
  write_to_output(g, d, "\r\nExit flags: @c%s@n\r\n"
                     "Enter exit flags, 0 to quit : ",
                  bits);
  OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
}

/*
 * For room flags.
 */
static void redit_disp_flag_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];

  clear_screen(g, d);
  olc_list_columns(g, d, room_bits, NUM_ROOM_FLAGS, 1, "", 2);
  sprintbitarray(room_get_room_flags(OLC_ROOM(d)), room_bits, RF_ARRAY_MAX, bits, sizeof(bits));
  write_to_output(g, d, "\r\nRoom flags: @c%s@n\r\n"
                     "Enter room flags, 0 to quit : ",
                  bits);
  OLC_MODE(d) = REDIT_FLAGS;
}

/*
 * For sector type.
 */
static void redit_disp_sector_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);
  olc_list_columns(g, d, sector_types, NUM_ROOM_SECTORS, 0, "", 3);
  write_to_output(g, d, "\r\nEnter sector type : ");
  OLC_MODE(d) = REDIT_SECTOR;
}

/*
 * The main menu.
 */
static void redit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct room_data *room;

  clear_screen(g, d);
  room = OLC_ROOM(d);

  sprintbitarray(room_get_room_flags(room), room_bits, RF_ARRAY_MAX, buf1, sizeof(buf1));
  sprinttype(room_get_sector_type(room), sector_types, buf2, sizeof(buf2));
  if (char_get_admlevel(d->character) > 0) {
    write_to_output(g, d,
                    "-- Room number : [@c%d@n]  	Room zone: [@c%d@n]\r\n"
                    "@g1@n) Name        : @y%s\r\n"
                    "@g2@n) Description :\r\n@y%s"
                    "@g3@n) Room flags  : @c%s\r\n"
                    "@g4@n) Sector type : @c%s\r\n"
                    "@g5@n) Exit north  : [@c%6d@n],  @gB@n) Exit northwest : [@c%6d@n]\r\n"
                    "@g6@n) Exit east   : [@c%6d@n],  @gC@n) Exit northeast : [@c%6d@n]\r\n"
                    "@g7@n) Exit south  : [@c%6d@n],  @gD@n) Exit southeast : [@c%6d@n]\r\n"
                    "@g8@n) Exit west   : [@c%6d@n],  @gE@n) Exit southwest : [@c%6d@n]\r\n"
                    "@g9@n) Exit up     : [@c%6d@n],  @gF@n) Exit in        : [@c%6d@n]\r\n"
                    "@gA@n) Exit down   : [@c%6d@n],  @gG@n) Exit out       : [@c%6d@n]\r\n"
                    "@gH@n) Extra descriptions menu\r\n"
                    "@gW@n) Copy Room\r\n"
                    "@gX@n) Delete Room\r\n"
                    "@gS@n) Script      : @c%s\r\n"
                    "@gZ@n) Wiznet      :\r\n"
                    "@gQ@n) Quit\r\n"
                    "Enter choice : ",

                    OLC_NUM(d), g->zones[OLC_ZNUM(d)].number, room_get_name(room),
                    room_get_description(room), buf1, buf2,
                    room_get_dir_option(room, NORTH) && room_get_dir_option(room, NORTH)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, NORTH)->to_room) : -1,
                    room_get_dir_option(room, NORTHWEST) && room_get_dir_option(room, NORTHWEST)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, NORTHWEST)->to_room) : -1,
                    room_get_dir_option(room, EAST) && room_get_dir_option(room, EAST)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, EAST)->to_room) : -1,
                    room_get_dir_option(room, NORTHEAST) && room_get_dir_option(room, NORTHEAST)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, NORTHEAST)->to_room) : -1,
                    room_get_dir_option(room, SOUTH) && room_get_dir_option(room, SOUTH)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, SOUTH)->to_room) : -1,
                    room_get_dir_option(room, SOUTHEAST) && room_get_dir_option(room, SOUTHEAST)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, SOUTHEAST)->to_room) : -1,
                    room_get_dir_option(room, WEST) && room_get_dir_option(room, WEST)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, WEST)->to_room) : -1,
                    room_get_dir_option(room, SOUTHWEST) && room_get_dir_option(room, SOUTHWEST)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, SOUTHWEST)->to_room) : -1,
                    room_get_dir_option(room, UP) && room_get_dir_option(room, UP)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, UP)->to_room) : -1,
                    room_get_dir_option(room, INDIR) && room_get_dir_option(room, INDIR)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, INDIR)->to_room) : -1,
                    room_get_dir_option(room, DOWN) && room_get_dir_option(room, DOWN)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, DOWN)->to_room) : -1,
                    room_get_dir_option(room, OUTDIR) && room_get_dir_option(room, OUTDIR)->to_room != NOWHERE ? rroom_get_vnum(g, room_get_dir_option(room, OUTDIR)->to_room) : -1,
                    OLC_SCRIPT(d) ? "Set." : "Not Set.");
  } else {
    write_to_output(g, d,
                    "-- Room number : [@c%d@n]    Room zone: [@c%d@n]\r\n"
                    "@g1@n) Location Designation (Room Name)        : @y%s\r\n"
                    "@g2@n) Remodeling Routine (Room Description)   :\r\n@y%s"
                    "@gH@n) Extra descriptions menu\r\n"
                    "@gQ@n) Quit\r\n"
                    "Enter choice : ",

                    OLC_NUM(d), g->zones[OLC_ZNUM(d)].number, room_get_name(room),
                    room_get_description(room));
  }

  OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int number;
  room_rnum rnum;
  char *oldtext = nullptr;

  switch (OLC_MODE(d)) {
    case REDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          redit_save_internally(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                 "OLC: %s edits room %d.", char_get_name(d->character), OLC_NUM(d));
          if (CONFIG_OLC_SAVE) {
            redit_save_to_disk(g, real_zone_by_thing(g, OLC_NUM(d)));
            if (char_get_admlevel(d->character) < 1) {
              write_to_output(g, d, "@GThe remodeling droid quickly launches about with remodeling the room to your specifications. It seems to finish in no time at all...@n\r\n");
              act(g, "@GThe remodeling droid quickly launches about with remodeling the room as to @g$n's@G specifications. It seems to finish in no time at all...@n", true, d->character, nullptr, nullptr, TO_ROOM);
            } else {
              write_to_output(g, d, "Room saved to disk.\r\n");
            }
          } else
            write_to_output(g, d, "Room saved to memory.\r\n");
          /* Free everything. */
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        case 'n':
        case 'N':
          /* If not saving, we must free the script_proto list. We do so by
       * assigning it to the edited room and letting free_room in
       * cleanup_olc handle it. */
          room_set_proto_script(OLC_ROOM(d), OLC_SCRIPT(d));
          cleanup_olc(g, d, CLEANUP_ALL);
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "@GYou close the droids interface and put it back among the rest of your things.@n\r\n");
            act(g, "@g$n@G stops typing information into the droid in $s hands and closes it back up.@n\r\n", true, d->character, nullptr, nullptr, TO_ROOM);
            struct obj_data *obj;
            obj = read_object(g, 19094, VIRTUAL);
            obj_to_char(g, obj, d->character);
          }
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes ? : ");
          break;
      }
      return;

    case REDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /* Something has been modified. */
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case '1':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "Enter Location Name:\r\n");
          } else {
            write_to_output(g, d, "Enter Room Name:-\r\n] ");
          }
          OLC_MODE(d) = REDIT_NAME;
          break;
        case '2':
          OLC_MODE(d) = REDIT_DESC;
          clear_screen(g, d);
          send_editor_help(g, d);
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "Set Remodel Parameters (Enter Room Description):-\r\n] ");
          } else {
            write_to_output(g, d, "Enter room description:\r\n\r\n");
          }

          if (room_get_description(OLC_ROOM(d))) {
            write_to_output(g, d, "%s", room_get_description(OLC_ROOM(d)));
            oldtext = strdup(room_get_description(OLC_ROOM(d)));
          }
          string_write(d, room_description_slot(OLC_ROOM(d)), MAX_ROOM_DESC, 0, oldtext);
          OLC_VAL(d) = 1;
          break;
        case '3':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          redit_disp_flag_menu(g, d);
          break;
        case '4':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          redit_disp_sector_menu(g, d);
          break;
        case '5':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = NORTH;
          redit_disp_exit_menu(g, d);
          break;
        case '6':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = EAST;
          redit_disp_exit_menu(g, d);
          break;
        case '7':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = SOUTH;
          redit_disp_exit_menu(g, d);
          break;
        case '8':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = WEST;
          redit_disp_exit_menu(g, d);
          break;
        case '9':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = UP;
          redit_disp_exit_menu(g, d);
          break;
        case 'a':
        case 'A':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = DOWN;
          redit_disp_exit_menu(g, d);
          break;
        case 'b':
        case 'B':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = NORTHWEST;
          redit_disp_exit_menu(g, d);
          break;
        case 'c':
        case 'C':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = NORTHEAST;
          redit_disp_exit_menu(g, d);
          break;
        case 'd':
        case 'D':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = SOUTHEAST;
          redit_disp_exit_menu(g, d);
          break;
        case 'e':
        case 'E':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = SOUTHWEST;
          redit_disp_exit_menu(g, d);
          break;
        case 'f':
        case 'F':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = INDIR;
          redit_disp_exit_menu(g, d);
          break;
        case 'g':
        case 'G':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_VAL(d) = OUTDIR;
          redit_disp_exit_menu(g, d);
          break;
        case 'h':
        case 'H':
          /*
       * If the extra description doesn't exist.
       */
          if (!room_get_ex_description(OLC_ROOM(d)))
            *room_ex_description_slot(OLC_ROOM(d)) = new extra_descr_data();
          OLC_DESC(d) = room_get_ex_description(OLC_ROOM(d));
          redit_disp_extradesc_menu(g, d);
          break;
        case 'w':
        case 'W':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          write_to_output(g, d, "Disabled for the time being!");
          /*OLC_MODE(d) = REDIT_COPY;*/
          break;
        case 'x':
        case 'X':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          /* Delete the room, prompt first. */
          write_to_output(g, d, "Are you sure you want to delete this room? ");
          OLC_MODE(d) = REDIT_DELETE;
          break;
        case 's':
        case 'S':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
          dg_script_menu(g, d);
          return;
        case 'Z':
        case 'z':
          if (char_get_admlevel(d->character) < 1) {
            write_to_output(g, d, "That options isn't available to non-builders.\r\n");
            break;
          }
          search_replace(arg, "z ", "");
          do_wiznet(g, d->character, arg, 0, 0);
          break;
        default:
          write_to_output(g, d, "Invalid choice!");
          redit_disp_menu(g, d);
          break;
      }
      return;

    case OLC_SCRIPT_EDIT:
      if (dg_script_edit_parse(g, d, arg)) return;
      break;

    case REDIT_NAME:
      if (!genolc_checkstring(d, arg))
        break;
      if (room_get_name(OLC_ROOM(d)))
        free(room_get_name(OLC_ROOM(d)));
      arg[MAX_ROOM_NAME] = '\0';
      room_set_name(OLC_ROOM(d), str_udup(arg));
      break;

    case REDIT_DESC:
      /*
     * We will NEVER get here, we hope.
     */
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: Reached REDIT_DESC case in parse_redit().");
      write_to_output(g, d, "Oops, in REDIT_DESC.\r\n");
      break;

    case REDIT_FLAGS:
      number = atoi(arg);
      if (number < 0 || number > NUM_ROOM_FLAGS) {
        write_to_output(g, d, "That is not a valid choice!\r\n");
        redit_disp_flag_menu(g, d);
      } else if (number == 0)
        break;
      else {
        /*
       * Toggle the bit.
       */
        room_toggle_flag(OLC_ROOM(d), number - 1);
        redit_disp_flag_menu(g, d);
      }
      return;

    case REDIT_SECTOR:
      number = atoi(arg);
      if (number < 0 || number >= NUM_ROOM_SECTORS) {
        write_to_output(g, d, "Invalid choice!");
        redit_disp_sector_menu(g, d);
        return;
      }
      room_set_sector_type(OLC_ROOM(d), number);
      break;

    case REDIT_EXIT_MENU:
      switch (*arg) {
        case '0':
          break;
        case '1':
          OLC_MODE(d) = REDIT_EXIT_NUMBER;
          write_to_output(g, d, "Exit to room number : ");
          return;
        case '2':
          OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
          send_editor_help(g, d);
          write_to_output(g, d, "Enter exit description:\r\n\r\n");
          if (OLC_EXIT(d)->general_description) {
            write_to_output(g, d, "%s", OLC_EXIT(d)->general_description);
            oldtext = strdup(OLC_EXIT(d)->general_description);
          }
          string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC, 0, oldtext);
          return;
        case '3':
          OLC_MODE(d) = REDIT_EXIT_KEYWORD;
          write_to_output(g, d, "Enter keywords : ");
          return;
        case '4':
          OLC_MODE(d) = REDIT_EXIT_KEY;
          write_to_output(g, d, "Enter key number : ");
          return;
        case '5':
          OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
          redit_disp_exit_flag_menu(g, d);
          return;
        case '6':
          /*
       * Delete an exit.
       */
          if (OLC_EXIT(d)->keyword)
            free(OLC_EXIT(d)->keyword);
          if (OLC_EXIT(d)->general_description)
            free(OLC_EXIT(d)->general_description);
          if (OLC_EXIT(d))
            delete OLC_EXIT(d);
          room_set_dir_option(OLC_ROOM(d), OLC_VAL(d), nullptr);
          break;
        case '7':
          OLC_MODE(d) = REDIT_EXIT_DCLOCK;
          write_to_output(g, d, "Enter lock DC number : ");
          return;
        case '8':
          OLC_MODE(d) = REDIT_EXIT_DCHIDE;
          write_to_output(g, d, "Enter door search DC number : ");
          return;
        case '9':
          OLC_MODE(d) = REDIT_EXIT_DCSKILL;
          write_to_output(g, d, "Enter skill to be checked to pass through exit : ");
          return;
        case 'a':
        case 'A':
          OLC_MODE(d) = REDIT_EXIT_DCMOVE;
          write_to_output(g, d, "Enter DC for skill required to pass through exit : ");
          return;
        case 'b':
        case 'B':
          OLC_MODE(d) = REDIT_EXIT_SAVETYPE;
          write_to_output(g, d, "Enter the SAVE TYPE for failed skill checks : ");
          return;
        case 'c':
        case 'C':
          OLC_MODE(d) = REDIT_EXIT_DCSAVE;
          write_to_output(g, d, "Enter the DC to beat for SAVE from failed skill checks : ");
          return;
        case 'd':
        case 'D':
          OLC_MODE(d) = REDIT_EXIT_FAILROOM;
          write_to_output(g, d, "Enter the room to send play to for minor save failure : ");
          return;
        case 'e':
        case 'E':
          OLC_MODE(d) = REDIT_EXIT_TOTALFAILROOM;
          write_to_output(g, d, "Enter the room to send play to for major save failure : ");
          return;
        default:
          write_to_output(g, d, "Try again : ");
          return;
      }
      break;

    case REDIT_EXIT_NUMBER:
      rnum = NOWHERE;
      if ((number = atoi(arg)) != -1)
        if ((rnum = real_room(g, number)) == NOWHERE) {
          write_to_output(g, d, "That room does not exist, try again : ");
          return;
        }
      OLC_EXIT(d)->to_room = rnum;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_DESCRIPTION:
      /*
     * We should NEVER get here, hopefully.
     */
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: Reached REDIT_EXIT_DESC case in parse_redit");
      write_to_output(g, d, "Oops, in REDIT_EXIT_DESCRIPTION.\r\n");
      break;

    case REDIT_EXIT_KEYWORD:
      if (OLC_EXIT(d)->keyword)
        free(OLC_EXIT(d)->keyword);
      OLC_EXIT(d)->keyword = str_udup(arg);
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_KEY:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->key = NOTHING;
      else
        OLC_EXIT(d)->key = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_DOORFLAGS:
      number = atoi(arg);
      if (number < 0 || number > NUM_EXIT_FLAGS) {
        write_to_output(g, d, "That's not a valid choice!\r\n");
        redit_disp_exit_flag_menu(g, d);
      } else if (number == 0) {
        redit_disp_exit_menu(g, d);
      } else {
        /* Toggle the bit. */
        TOGGLE_BIT(OLC_EXIT(d)->exit_info, 1U << (number - 1));
        redit_disp_exit_flag_menu(g, d);
      }
      return;

    case REDIT_EXTRADESC_KEY:
      if (genolc_checkstring(d, arg)) {
        if (OLC_DESC(d)->keyword)
          free(OLC_DESC(d)->keyword);
        OLC_DESC(d)->keyword = str_udup(arg);
      }
      redit_disp_extradesc_menu(g, d);
      return;

    case REDIT_EXTRADESC_MENU:
      switch ((number = atoi(arg))) {
        case 0:
          /*
       * If something got left out, delete the extra description
       * when backing out to the menu.
       */
          if (OLC_DESC(d)->keyword == nullptr || OLC_DESC(d)->description == nullptr) {
            struct extra_descr_data *temp;
            if (OLC_DESC(d)->keyword)
              free(OLC_DESC(d)->keyword);
            if (OLC_DESC(d)->description)
              free(OLC_DESC(d)->description);

            /*
	 * Clean up pointers.
	 */
            REMOVE_FROM_LIST(OLC_DESC(d), *room_ex_description_slot(OLC_ROOM(d)), next, temp);
            delete OLC_DESC(d);
          }
          break;
        case 1:
          OLC_MODE(d) = REDIT_EXTRADESC_KEY;
          write_to_output(g, d, "Enter keywords, separated by spaces : ");
          return;
        case 2:
          OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
          send_editor_help(g, d);
          write_to_output(g, d, "Enter extra description:\r\n\r\n");
          if (OLC_DESC(d)->description) {
            write_to_output(g, d, "%s", OLC_DESC(d)->description);
            oldtext = strdup(OLC_DESC(d)->description);
          }
          string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
          return;
        case 3:
          if (OLC_DESC(d)->keyword == nullptr || OLC_DESC(d)->description == nullptr) {
            write_to_output(g, d, "You can't edit the next extra description without completing this one.\r\n");
            redit_disp_extradesc_menu(g, d);
          } else {
            struct extra_descr_data *new_extra;

            if (OLC_DESC(d)->next)
              OLC_DESC(d) = OLC_DESC(d)->next;
            else {
              /*
	   * Make new extra description and attach at end.
	   */
              new_extra         = new extra_descr_data();
              OLC_DESC(d)->next = new_extra;
              OLC_DESC(d)       = new_extra;
            }
            redit_disp_extradesc_menu(g, d);
          }
          return;
      }
      break;

    case REDIT_COPY:
      if ((rnum = real_room(g, atoi(arg))) != NOWHERE) {
        redit_setup_existing(g, d, rnum);
      } else
        write_to_output(g, d, "That room does not exist.\r\n");
      break;

    case REDIT_DELETE:
      if (*arg == 'y' || *arg == 'Y') {
        if (delete_room(g, real_room(g, room_get_number(OLC_ROOM(d)))))
          write_to_output(g, d, "Room deleted.\r\n");
        else
          write_to_output(g, d, "Couldn't delete the room!\r\n");

        cleanup_olc(g, d, CLEANUP_ALL);
        return;
      }
      if (*arg == 'n' || *arg == 'N') {
        redit_disp_menu(g, d);
        OLC_MODE(d) = REDIT_MAIN_MENU;
        return;
      } else
        write_to_output(g, d, "Please answer 'Y' or 'N': ");

      break;

    case REDIT_EXIT_DCLOCK:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->dclock = NOTHING;
      else
        OLC_EXIT(d)->dclock = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_DCHIDE:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->dchide = NOTHING;
      else
        OLC_EXIT(d)->dchide = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_DCSKILL:
      number = find_skill_num(g, arg, SKTYPE_SKILL);
      if (number < 1)
        OLC_EXIT(d)->dcskill = 0;
      else
        OLC_EXIT(d)->dcskill = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_DCMOVE:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->dcmove = NOTHING;
      else
        OLC_EXIT(d)->dcmove = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_SAVETYPE:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->failsavetype = NOTHING;
      else
        OLC_EXIT(d)->failsavetype = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_DCSAVE:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->dcfailsave = NOTHING;
      else
        OLC_EXIT(d)->dcfailsave = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_FAILROOM:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->failroom = NOWHERE;
      else
        OLC_EXIT(d)->failroom = number;
      redit_disp_exit_menu(g, d);
      return;

    case REDIT_EXIT_TOTALFAILROOM:
      number = atoi(arg);
      if (number < 0)
        OLC_EXIT(d)->totalfailroom = NOWHERE;
      else
        OLC_EXIT(d)->totalfailroom = number;
      redit_disp_exit_menu(g, d);
      return;

    default:
      /*
     * We should never get here.
     */
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: Reached default case in parse_redit");
      break;
  }
  /*
   * If we get this far, something has been changed.
   */
  OLC_VAL(d) = 1;
  redit_disp_menu(g, d);
}

void redit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    case REDIT_DESC:
      redit_disp_menu(g, d);
      break;
    case REDIT_EXIT_DESCRIPTION:
      redit_disp_exit_menu(g, d);
      break;
    case REDIT_EXTRADESC_DESCRIPTION:
      redit_disp_extradesc_menu(g, d);
      break;
  }
}
