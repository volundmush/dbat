/************************************************************************
 * OasisOLC - Zones / zedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/config.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/oasis.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/races.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/handler.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/genshp.hpp"
#include "dbat/gengld.hpp"
#include "dbat/game_world.hpp"

static void zedit_disp_arg2(GameWorld *g, struct descriptor_data *d);
static void zedit_disp_arg5(GameWorld *g, struct descriptor_data *d);
static void zedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void zedit_new_zone(GameWorld *g, struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top);
static void zedit_save_to_disk(GameWorld *g, struct zone_data *zone);
static void zedit_setup(GameWorld *g, struct descriptor_data *d, room_rnum room_num);

/*
 * Nasty internal macros to clean up the code.
 */
#define MYCMD      (OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d) (OLC_ZONE(d)->cmd[OLC_VAL(d)])

/* Prototypes. */
static bool start_change_command(struct descriptor_data *d, int pos);

/*-------------------------------------------------------------------*/

ACMD(do_oasis_zedit)
{
  IDXTYPE number = NOWHERE;
  int save       = 0;
  room_rnum real_num;
  struct descriptor_data *d;
  char *buf3;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  buf3 = two_arguments(argument, buf1, buf2);

  /****************************************************************************/
  /** If no argument was given, use the zone the builder is standing in.     **/
  /****************************************************************************/
  if (!*buf1)
    number = rroom_get_vnum(g, char_get_in_room(ch));
  else if (!isdigit(*buf1)) {
    if (strcasecmp("save", buf1) == 0) {
      save = true;

      if (is_number(buf2))
        number = atoi(buf2);
      else if (char_get_olc_zone(g, ch) > 0) {
        struct zone_data *zlok;

        if ((zlok = find_zone(g, char_get_olc_zone(g, ch))) == nullptr)
          number = NOWHERE;
        else
          number = genolc_zone_bottom(g, zlok);
      }

      if (number == NOWHERE) {
        send_to_char(g, ch, "Save which zone?\r\n");
        return;
      }
    } else if (char_get_admlevel(ch) >= ADMLVL_IMPL) {
      if (strcasecmp("new", buf1) || !buf3 || !*buf3)
        send_to_char(g, ch, "Format: zedit new <zone number> <bottom-room> "
                         "<upper-room>\r\n");
      else {
        char sbot[MAX_INPUT_LENGTH], stop[MAX_INPUT_LENGTH];
        room_vnum bottom, top;

        skip_spaces(&buf3);
        two_arguments(buf3, sbot, stop);

        number = atoi(buf2);
        if (number < 0)
          number = NOWHERE;
        bottom = atoi(sbot);
        top    = atoi(stop);

        /**********************************************************************/
        /** Setup the new zone (displays the menu to the builder).           **/
        /**********************************************************************/
        zedit_new_zone(g, ch, number, bottom, top);
      }

      /************************************************************************/
      /** Done now, exit the function.                                       **/
      /************************************************************************/
      return;

    } else {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
  }

  /****************************************************************************/
  /** If a numeric argument was given, retrieve it.                           **/
  /****************************************************************************/
  if (number == NOWHERE)
    number = atoi(buf1);

  /****************************************************************************/
  /** Check that nobody is currently editing this zone.                      **/
  /****************************************************************************/
  for (descriptor_data *dd : each_desc(g)) {
    if (desc_get_connected(dd) == CON_ZEDIT) {
      if (dd->olc && OLC_NUM(dd) == number) {
        send_to_char(g, ch, "That zone is currently being edited by %s.\r\n",
                     char_pers(g, dd->character, ch));
        return;
      }
    }
  }

  /****************************************************************************/
  /** Store the builder's descriptor in d.                                   **/
  /****************************************************************************/
  d = char_get_desc(ch);

  /****************************************************************************/
  /** Give the builder's descriptor an OLC structure.                        **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis_zedit: Player already "
                                         "had olc structure.");
    delete d->olc;
  }

  d->olc = new oasis_olc_data();

  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  OLC_ZNUM(d) = save ? find_zone(g, number) : find_zone_by_thing(g, number);
  if (OLC_ZNUM(d) == nullptr) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_cannot_edit(g, ch, OLC_ZNUM(d)->number);
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** If we need to save, then save the zone.                                **/
  /****************************************************************************/
  if (save) {
    send_to_char(g, ch, "Saving all zone information for zone %d.\r\n",
                 OLC_ZNUM(d)->number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
           "OLC: %s saves zone information for zone %d.", char_get_name(ch),
           OLC_ZNUM(d)->number);

    /**************************************************************************/
    /** Save the zone information to the zone file.                          **/
    /**************************************************************************/
    save_zone(g, OLC_ZNUM(d));

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  if ((real_num = real_room(g, number)) == NOWHERE) {
    write_to_output(g, d, "That room does not exist.\r\n");

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  zedit_setup(g, d, real_num);
  desc_set_connected(d, CON_ZEDIT);

  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
         char_get_name(ch), OLC_ZNUM(d)->number, char_get_olc_zone(g, ch));
}

static void zedit_setup(GameWorld *g, struct descriptor_data *d, room_rnum room_num)
{
  struct zone_data *zone;
  int subcmd = 0, count = 0;
  room_rnum cmd_room = NOWHERE;

  /*
   * Allocate one scratch zone structure.  
   */
  zone = new zone_data();

  /*
   * Copy all the zone header information over.
   */
  zone->name = strdup(OLC_ZNUM(d)->name);
  if (OLC_ZNUM(d)->builders)
    zone->builders = strdup(OLC_ZNUM(d)->builders);
  zone->lifespan      = OLC_ZNUM(d)->lifespan;
  zone->bot           = OLC_ZNUM(d)->bot;
  zone->top           = OLC_ZNUM(d)->top;
  zone->reset_mode    = OLC_ZNUM(d)->reset_mode;
  zone->zone_flags[0] = OLC_ZNUM(d)->zone_flags[0];
  zone->zone_flags[1] = OLC_ZNUM(d)->zone_flags[1];
  zone->zone_flags[2] = OLC_ZNUM(d)->zone_flags[2];
  zone->zone_flags[3] = OLC_ZNUM(d)->zone_flags[3];
  zone->min_level     = OLC_ZNUM(d)->min_level;
  zone->max_level     = OLC_ZNUM(d)->max_level;
  /*
   * The remaining fields are used as a 'has been modified' flag  
   */
  zone->number = 0; /* Header information has changed.	*/
  zone->age    = 0; /* The commands have changed.		*/

  /*
   * Start the reset command list with a terminator.
   */
  zone->cmd            = new reset_com[1]();
  zone->cmd[0].command = 'S';

  /*
   * Add all entries in zone_table that relate to this room.
   */
  while (zone_get_cmd(g, OLC_ZNUM(d), subcmd)->command != 'S') {
    switch (zone_get_cmd(g, OLC_ZNUM(d), subcmd)->command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = zone_get_cmd(g, OLC_ZNUM(d), subcmd)->arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = zone_get_cmd(g, OLC_ZNUM(d), subcmd)->arg1;
        break;
      default:
        break;
    }
    if (cmd_room == room_num) {
      add_cmd_to_list(&(zone->cmd), zone_get_cmd(g, OLC_ZNUM(d), subcmd), count);
      count++;
    }
    subcmd++;
  }

  OLC_ZONE(d) = zone;
  /*
   * Display main menu.
   */
  zedit_disp_menu(g, d);
}
/*------------------------------------------------------------------*/
static void zedit_disp_flag_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];

  clear_screen(g, d);
  olc_list_columns(g, d, zone_bits, NUM_ZONE_FLAGS, 1, " ", 2);

  sprintbitarray(OLC_ZONE(d)->zone_flags, zone_bits, ZF_ARRAY_MAX, bits, sizeof(bits));
  write_to_output(g, d, "\r\nZone flags: @c%s@n\r\n"
                     "Enter Zone flags, 0 to quit : ",
                  bits);
  OLC_MODE(d) = ZEDIT_ZONE_FLAGS;
}

/*-------------------------------------------------------------------*/

/*
 * Create a new zone.
 */
static void zedit_new_zone(GameWorld *g, struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top)
{
  struct zone_data *result;
  const char *error;

  if ((result = create_new_zone(g, vzone_num, bottom, top, &error)) == nullptr) {
    write_to_output(g, char_get_desc(ch), "%s", error);
    return;
  }

  for (descriptor_data *dsc : each_desc(g)) {
    switch (desc_get_connected(dsc)) {
      case CON_REDIT:
        room_set_zone(OLC_ROOM(dsc), room_get_zone(OLC_ROOM(dsc)) + ((OLC_ZNUM(dsc) >= result)));
        [[fallthrough]];
      case CON_ZEDIT:
      case CON_MEDIT:
      case CON_SEDIT:
      case CON_OEDIT:
      case CON_TRIGEDIT:
      case CON_GEDIT:
        OLC_ZNUM(dsc) += (OLC_ZNUM(dsc) >= result);
        break;
      default:
        break;
    }
  }

  zedit_save_to_disk(g, result); /* save to disk .. */

  mudlog(g, LOG_BRF, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: %s creates new zone #%d", char_get_name(ch), vzone_num);
  write_to_output(g, char_get_desc(ch), "Zone created successfully.\r\n");
}

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
static void zedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  int mobloaded = false,
      objloaded = false,
      subcmd;
  room_rnum room_num = real_room(g, OLC_NUM(d));

  if (room_num == NOWHERE) {
    mud_log("SYSERR: zedit_save_internally: OLC_NUM(d) room %d not found.", OLC_NUM(d));
    return;
  }

  remove_room_zone_commands(g, OLC_ZNUM(d), room_num);

  /*
   * Now add all the entries in the players descriptor list  
   */
  for (subcmd = 0; MYCMD.command != 'S'; subcmd++) {
    /*
     * Since Circle does not keep track of what rooms the 'G', 'E', and
     * 'P' commands are exitted in, but OasisOLC groups zone commands
     * by rooms, this creates interesting problems when builders use these
     * commands without loading a mob or object first.  This fix prevents such
     * commands from being saved and 'wandering' through the zone command
     * list looking for mobs/objects to latch onto.
     * C.Raehl 4/27/99
     */
    switch (MYCMD.command) {
      /* Possible fail cases. */
      case 'G':
      case 'E':
        if (mobloaded)
          break;
        write_to_output(g, d, "Equip/Give command not saved since no mob was loaded first.\r\n");
        continue;
      case 'P':
        if (objloaded)
          break;
        write_to_output(g, d, "Put command not saved since another object was not loaded first.\r\n");
        continue;
      /* Pass cases. */
      case 'M':
        mobloaded = true;
        break;
      case 'O':
        objloaded = true;
        break;
      default:
        mobloaded = objloaded = false;
        break;
    }
    add_cmd_to_list(&OLC_ZNUM(d)->cmd, &MYCMD, subcmd);
  }

  /*
   * Finally, if zone headers have been changed, copy over  
   */
  if (OLC_ZONE(d)->number) {
    free(OLC_ZNUM(d)->name);
    free(OLC_ZNUM(d)->builders);

    OLC_ZNUM(d)->name          = strdup(OLC_ZONE(d)->name);
    OLC_ZNUM(d)->builders      = strdup(OLC_ZONE(d)->builders);
    OLC_ZNUM(d)->bot           = OLC_ZONE(d)->bot;
    OLC_ZNUM(d)->top           = OLC_ZONE(d)->top;
    OLC_ZNUM(d)->reset_mode    = OLC_ZONE(d)->reset_mode;
    OLC_ZNUM(d)->lifespan      = OLC_ZONE(d)->lifespan;
    OLC_ZNUM(d)->zone_flags[0] = OLC_ZONE(d)->zone_flags[0];
    OLC_ZNUM(d)->zone_flags[1] = OLC_ZONE(d)->zone_flags[1];
    OLC_ZNUM(d)->zone_flags[2] = OLC_ZONE(d)->zone_flags[2];
    OLC_ZNUM(d)->zone_flags[3] = OLC_ZONE(d)->zone_flags[3];
    OLC_ZNUM(d)->min_level     = OLC_ZONE(d)->min_level;
    OLC_ZNUM(d)->max_level     = OLC_ZONE(d)->max_level;
  }
  add_to_save_list(g, OLC_ZNUM(d)->number, SL_ZON);
}

/*-------------------------------------------------------------------*/

static void zedit_save_to_disk(GameWorld *g, struct zone_data *zone)
{
  save_zone(g, zone);
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then setup change  
 */
static bool start_change_command(struct descriptor_data *d, int pos)
{
  if (pos < 0 || pos >= count_commands(OLC_ZONE(d)->cmd))
    return false;

  /*
   * Ok, let's get editing.
   */
  OLC_VAL(d) = pos;
  return true;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * the main menu 
 */
static void zedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  int subcmd = 0, counter = 0;
  char buf1[MAX_STRING_LENGTH];

  clear_screen(g, d);
  sprintbitarray(OLC_ZONE(d)->zone_flags, zone_bits, ZF_ARRAY_MAX, buf1, sizeof(buf1));

  /*
   * Menu header  
   */
  send_to_char(g, d->character,
               "Room number: [@c%d@n]		Room zone: @c%d\r\n"
               "@g1@n) Builders       : @y%s\r\n"
               "@gA@n) Zone name      : @y%s\r\n"
               "@gL@n) Lifespan       : @y%d minutes\r\n"
               "@gB@n) Bottom of zone : @y%d\r\n"
               "@gT@n) Top of zone    : @y%d\r\n"
               "@gR@n) Reset Mode     : @y%s@n\r\n"
               "@gF@n) Zone Flags     : @y%s@n\r\n"
               "@gM@n) Min Level      : @y%d@n\r\n"
               "@gX@n) Max Level      : @y%d@n\r\n"
               "@gZ@n) Wiznet         :\r\n"
               "[Command list]\r\n",

               OLC_NUM(d),
               OLC_ZNUM(d)->number,
               OLC_ZONE(d)->builders ? OLC_ZONE(d)->builders : "None.",
               OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>",
               OLC_ZONE(d)->lifespan,
               OLC_ZONE(d)->bot,
               OLC_ZONE(d)->top,
               OLC_ZONE(d)->reset_mode ? ((OLC_ZONE(d)->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
               buf1, OLC_ZONE(d)->min_level, OLC_ZONE(d)->max_level);

  /*
   * Print the commands for this room into display buffer.
   */
  while (MYCMD.command != 'S') {
    /*
     * Translate what the command means.
     */
    write_to_output(g, d, "@n%d - @y", counter++);
    switch (MYCMD.command) {
      case 'M':
        write_to_output(g, d, "%sLoad %s@y [@c%d@y], Max : %d, MaxR %d, Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        char_get_sdesc(&g->mob_proto[MYCMD.arg1]),
                        g->mob_index[MYCMD.arg1].vnum, MYCMD.arg2, MYCMD.arg4, MYCMD.arg5);
        break;
      case 'G':
        write_to_output(g, d, "%sGive it %s@y [@c%d@y], Max : %d, Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        obj_get_short_description(&g->obj_proto[MYCMD.arg1]),
                        g->obj_index[MYCMD.arg1].vnum,
                        MYCMD.arg2, MYCMD.arg5);
        break;
      case 'O':
        write_to_output(g, d, "%sLoad %s@y [@c%d@y], Max : %d, MaxR %d, Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        obj_get_short_description(&g->obj_proto[MYCMD.arg1]),
                        g->obj_index[MYCMD.arg1].vnum,
                        MYCMD.arg2, MYCMD.arg4, MYCMD.arg5);
        break;
      case 'E':
        write_to_output(g, d, "%sEquip with %s@y [@c%d@n], %s, Max : %d, Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        obj_get_short_description(&g->obj_proto[MYCMD.arg1]),
                        g->obj_index[MYCMD.arg1].vnum,
                        equipment_types[MYCMD.arg3],
                        MYCMD.arg2, MYCMD.arg5);
        break;
      case 'P':
        write_to_output(g, d, "%sPut %s@y [@c%d@n] in %s [@c%d@n], Max : %d, %% Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        obj_get_short_description(&g->obj_proto[MYCMD.arg1]),
                        g->obj_index[MYCMD.arg1].vnum,
                        obj_get_short_description(&g->obj_proto[MYCMD.arg3]),
                        g->obj_index[MYCMD.arg3].vnum,
                        MYCMD.arg2, MYCMD.arg5);
        break;
      case 'R':
        write_to_output(g, d, "%sRemove %s@y [@c%d@n] from room.",
                        MYCMD.if_flag ? " then " : "",
                        obj_get_short_description(&g->obj_proto[MYCMD.arg2]),
                        g->obj_index[MYCMD.arg2].vnum);
        break;
      case 'D':
        write_to_output(g, d, "%sSet door %s@y as %s.",
                        MYCMD.if_flag ? " then " : "",
                        dirs[MYCMD.arg2],
                        MYCMD.arg3 ? ((MYCMD.arg3 == 1) ? "closed" : "locked") : "open");
        break;
      case 'T':
        write_to_output(g, d, "%sAttach trigger @c%s@y [@c%d@y] to %s, %% Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        g->trig_index[MYCMD.arg2]->proto->name,
                        g->trig_index[MYCMD.arg2]->vnum,
                        ((MYCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((MYCMD.arg1 == OBJ_TRIGGER) ? "object" : ((MYCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))), MYCMD.arg5);
        break;
      case 'V':
        write_to_output(g, d, "%sAssign global %s:%d to %s = %s, %% Chance %d",
                        MYCMD.if_flag ? " then " : "",
                        MYCMD.sarg1, MYCMD.arg2,
                        ((MYCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((MYCMD.arg1 == OBJ_TRIGGER) ? "object" : ((MYCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))),
                        MYCMD.sarg2, MYCMD.arg5);
        break;
      default:
        write_to_output(g, d, "<Unknown Command>");
        break;
    }
    write_to_output(g, d, "\r\n");
    subcmd++;
  }
  /*
   * Finish off menu  
   */
  write_to_output(g, d,
                  "@n%d - <END OF LIST>\r\n"
                  "@gN@n) Insert new command.\r\n"
                  "@gE@n) Edit a command.\r\n"
                  "@gD@n) Delete a command.\r\n"
                  "@gQ@n) Quit\r\nEnter your choice : ",
                  counter);

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Print the command type menu and setup response catch. 
 */
static void zedit_disp_comtype(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);
  write_to_output(g, d,
                  "\r\n"
                  "@gM@n) Load Mobile to room             @gO@n) Load Object to room\r\n"
                  "@gE@n) Equip mobile with object        @gG@n) Give an object to a mobile\r\n"
                  "@gP@n) Put object in another object    @gD@n) Open/Close/Lock a Door\r\n"
                  "@gR@n) Remove an object from the room\r\n"
                  "@gT@n) Assign a trigger                @gV@n) Set a global variable\r\n"
                  "\r\n"
                  "What sort of command will this be? : ");
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg1 and set
 * up the input catch clause  
 */
static void zedit_disp_arg1(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "\r\n");

  switch (OLC_CMD(d).command) {
    case 'M':
      write_to_output(g, d, "Input mob's vnum : ");
      OLC_MODE(d) = ZEDIT_ARG1;
      break;
    case 'O':
    case 'E':
    case 'P':
    case 'G':
      write_to_output(g, d, "Input object vnum : ");
      OLC_MODE(d) = ZEDIT_ARG1;
      break;
    case 'D':
    case 'R':
      /*
     * Arg1 for these is the room number, skip to arg2  
     */
      OLC_CMD(d).arg1 = real_room(g, OLC_NUM(d));
      zedit_disp_arg2(g, d);
      break;
    case 'T':
    case 'V':
      write_to_output(g, d, "Input trigger type (0:mob, 1:obj, 2:room) :");
      OLC_MODE(d) = ZEDIT_ARG1;
      break;
    default:
      /*
     * We should never get here.
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_disp_arg1(): Help!");
      write_to_output(g, d, "Oops...\r\n");
      return;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg2 and set
 * up the input catch clause.
 */
static void zedit_disp_arg2(GameWorld *g, struct descriptor_data *d)
{
  int i;

  switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
    case 'E':
    case 'P':
    case 'G':
      write_to_output(g, d, "Input the maximum number that can exist on the mud : ");
      break;
    case 'D':
      for (i = 0; *dirs[i] != '\n'; i++) {
        write_to_output(g, d, "%d) Exit %s.\r\n", i, dirs[i]);
      }
      write_to_output(g, d, "Enter exit number for door : ");
      break;
    case 'R':
      write_to_output(g, d, "Input object's vnum : ");
      break;
    case 'T':
      write_to_output(g, d, "Enter the trigger VNum : ");
      break;
    case 'V':
      write_to_output(g, d, "Global's context (0 for none) : ");
      break;
    default:
      /*
     * We should never get here, but just in case...
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_disp_arg2(): Help!");
      write_to_output(g, d, "Oops...\r\n");
      return;
  }
  OLC_MODE(d) = ZEDIT_ARG2;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg3 and set
 * up the input catch clause.
 */
static void zedit_disp_arg3(GameWorld *g, struct descriptor_data *d)
{
  int i = 0;

  write_to_output(g, d, "\r\n");

  switch (OLC_CMD(d).command) {
    case 'E':
      while (*equipment_types[i] != '\n') {
        write_to_output(g, d, "%2d) %26.26s %2d) %26.26s\r\n", i,
                        equipment_types[i], i + 1, (*equipment_types[i + 1] != '\n') ? equipment_types[i + 1] : "");
        if (*equipment_types[i + 1] != '\n')
          i += 2;
        else
          break;
      }
      write_to_output(g, d, "Location to equip : ");
      break;
    case 'P':
      write_to_output(g, d, "Virtual number of the container : ");
      break;
    case 'D':
      write_to_output(g, d, "0)  Door open\r\n"
                         "1)  Door closed\r\n"
                         "2)  Door locked\r\n"
                         "Enter state of the door : ");
      break;
    case 'V':
    case 'T':
    case 'M':
    case 'O':
    case 'R':
    case 'G':
    default:
      /*
     * We should never get here, just in case.
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_disp_arg3(): Help!");
      write_to_output(g, d, "Oops...\r\n");
      return;
  }
  OLC_MODE(d) = ZEDIT_ARG3;
}
/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg4 and set
 * up the input catch clause.
 */
static void zedit_disp_arg4(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "\r\n");

  switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
      write_to_output(g, d, "Input the max allowed to load from this room (Pressing enter == 0(ignore)) : ");
      break;
    case 'E':
    case 'P':
    case 'G':
    case 'T':
    case 'V':
      zedit_disp_arg5(g, d);
      break;
    default:
      /*
     * We should never get here, but just in case...
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_disp_arg4(): Help!");
      write_to_output(g, d, "Oops...\r\n");
      return;
  }
  OLC_MODE(d) = ZEDIT_ARG4;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg5 and set
 * up the input catch clause.
 */
static void zedit_disp_arg5(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "\r\n");

  switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
    case 'E':
    case 'P':
    case 'G':
      write_to_output(g, d, "Input the percentage chance of the load NOT occurring : ");
      break;
    case 'T':
    case 'V':
    default:
      /*
     * We should never get here, but just in case...
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_disp_arg5(): Help!");
      write_to_output(g, d, "Oops...\r\n");
      return;
  }
  OLC_MODE(d) = ZEDIT_ARG5;
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

/*
 * Saving a zone only writes what's inside its range, so anything left outside would be gone for good
 * after the next save, along with every copy of it that players have. Says so if a new range would do that.
 */
static bool zone_range_loses_something(GameWorld *g, struct descriptor_data *d, int bot, int top)
{
  struct zone_data *zone = OLC_ZNUM(d);

  for (int vnum = genolc_zonep_bottom(zone); vnum <= zone->top; vnum++) {
    const char *what = nullptr;

    if (vnum >= bot && vnum <= top)
      continue;
    if (real_room(g, vnum) != NOWHERE)
      what = "room";
    else if (real_mobile(g, vnum) != NOBODY)
      what = "mob";
    else if (real_object(g, vnum) != NOTHING)
      what = "object";
    else if (real_trigger(g, vnum) != NOTHING)
      what = "trigger";
    else if (find_shop(g, vnum))
      what = "shop";
    else if (find_guild(g, vnum))
      what = "guild";
    if (what) {
      write_to_output(g, d, "That would leave %s %d outside the zone, and the next save would destroy it. "
                            "Range not changed.\r\n", what, vnum);
      return true;
    }
  }
  return false;
}

void zedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int pos;
  IDXTYPE rnum;
  int i = 0;
  int number;

  switch (OLC_MODE(d)) {
      /*-------------------------------------------------------------------*/
    case ZEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          /*
       * Save the zone in memory, hiding invisible people.
       */
          zedit_save_internally(g, d);
          if (CONFIG_OLC_SAVE) {
            write_to_output(g, d, "Saving zone info to disk.\r\n");
            zedit_save_to_disk(g, OLC_ZNUM(d));
          } else
            write_to_output(g, d, "Saving zone info in memory.\r\n");

          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: %s edits zone info for room %d.", char_get_name(d->character), OLC_NUM(d));
          [[fallthrough]];
        case 'n':
        case 'N':
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        default:
          write_to_output(g, d, "Invalid choice!\r\n");
          write_to_output(g, d, "Do you wish to save your changes? : ");
          break;
      }
      break;
      /* End of ZEDIT_CONFIRM_SAVESTRING */

      /*-------------------------------------------------------------------*/
    case ZEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_ZONE(d)->age || OLC_ZONE(d)->number) {
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
          } else {
            write_to_output(g, d, "No changes made.\r\n");
            cleanup_olc(g, d, CLEANUP_ALL);
          }
          break;
        case 'n':
        case 'N':
          /*
       * New entry.
       */
          if (OLC_ZONE(d)->cmd[0].command == 'S') {
            /* first command */
            if (new_command(OLC_ZONE(d), 0) && start_change_command(d, 0)) {
              zedit_disp_comtype(g, d);
              OLC_ZONE(d)->age = 1;
              break;
            }
          }
          write_to_output(g, d, "What number in the list should the new command be? : ");
          OLC_MODE(d) = ZEDIT_NEW_ENTRY;
          break;
        case 'e':
        case 'E':
          /*
       * Change an entry.
       */
          write_to_output(g, d, "Which command do you wish to change? : ");
          OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
          break;
        case 'd':
        case 'D':
          /*
       * Delete an entry.
       */
          write_to_output(g, d, "Which command do you wish to delete? : ");
          OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
          break;
        case 'a':
        case 'A':
          /*
       * Edit zone name.
       */
          write_to_output(g, d, "Enter new zone name : ");
          OLC_MODE(d) = ZEDIT_ZONE_NAME;
          break;
        case '1':
          /*
       * Edit zone builders.
       * We do not want to allow baby builders to tweak this.
       */
          if (char_get_admlevel(d->character) <= ADMLVL_BUILDER) {
            OLC_MODE(d) = ZEDIT_MAIN_MENU;
            write_to_output(g, d, "Access Denied.\r\n");
          } else {
            write_to_output(g, d, "Enter new builders list : ");
            OLC_MODE(d) = ZEDIT_ZONE_BUILDERS;
          }
          break;
        case 'b':
        case 'B':
          /*
       * Edit bottom of zone.
       */
          if (char_get_admlevel(d->character) < ADMLVL_IMPL)
            zedit_disp_menu(g, d);
          else {
            write_to_output(g, d, "Enter new bottom of zone : ");
            OLC_MODE(d) = ZEDIT_ZONE_BOT;
          }
          break;
        case 't':
        case 'T':
          /*
       * Edit top of zone.
       */
          if (char_get_admlevel(d->character) < ADMLVL_IMPL)
            zedit_disp_menu(g, d);
          else {
            write_to_output(g, d, "Enter new top of zone : ");
            OLC_MODE(d) = ZEDIT_ZONE_TOP;
          }
          break;
        case 'l':
        case 'L':
          /*
       * Edit zone lifespan.
       */
          write_to_output(g, d, "Enter new zone lifespan : ");
          OLC_MODE(d) = ZEDIT_ZONE_LIFE;
          break;
        case 'r':
        case 'R':
          /*
       * Edit zone reset mode.
       */
          write_to_output(g, d, "\r\n"
                             "0) Never reset\r\n"
                             "1) Reset only when no players in zone\r\n"
                             "2) Normal reset\r\n"
                             "Enter new zone reset type : ");
          OLC_MODE(d) = ZEDIT_ZONE_RESET;
          break;
        case 'm':
        case 'M':
          /*** Edit zone min level. ***/
          write_to_output(g, d, "Enter Minimum level to enter zone : \r\n");
          OLC_MODE(d) = ZEDIT_MIN_LEVEL;
          break;
        case 'f':
        case 'F':
          zedit_disp_flag_menu(g, d);
          break;
        case 'Z':
        case 'z':
          search_replace(arg, "z ", "");
          do_wiznet(g, d->character, arg, 0, 0);
          break;
        case 'x':
        case 'X':
          /*** Edit zone max level. ***/
          write_to_output(g, d, "Enter Maximum level to enter zone : \r\n");
          OLC_MODE(d) = ZEDIT_MAX_LEVEL;
          break;
        default:
          zedit_disp_menu(g, d);
          break;
      }
      break;
      /* End of ZEDIT_MAIN_MENU */

      /*-------------------------------------------------------------------*/
    case ZEDIT_NEW_ENTRY:
      /*
     * Get the line number and insert the new line.
     */
      pos = atoi(arg);
      if (isdigit(*arg) && new_command(OLC_ZONE(d), pos)) {
        if (start_change_command(d, pos)) {
          zedit_disp_comtype(g, d);
          OLC_ZONE(d)->age = 1;
        }
      } else
        zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_DELETE_ENTRY:
      /*
     * Get the line number and delete the line.
     */
      pos = atoi(arg);
      if (isdigit(*arg)) {
        delete_zone_command(OLC_ZONE(d), pos);
        OLC_ZONE(d)->age = 1;
      }
      zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_CHANGE_ENTRY:
      /*
     * Parse the input for which line to edit, and goto next quiz.
     */
      /*
     *  Abort edit, and return to main menu 
     * - idea from Mark Garringer zizazat@hotmail.com
     */
      if (toupper(*arg) == 'A') {
        if (OLC_CMD(d).command == 'N') {
          OLC_CMD(d).command = '*';
        }
        zedit_disp_menu(g, d);
        break;
      }

      pos = atoi(arg);
      if (isdigit(*arg) && start_change_command(d, pos)) {
        zedit_disp_comtype(g, d);
        OLC_ZONE(d)->age = 1;
      } else
        zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_COMMAND_TYPE:
      /*
     * Parse the input for which type of command this is, and goto next
     * quiz.
     */
      OLC_CMD(d).command = toupper(*arg);
      if (!OLC_CMD(d).command || (strchr("MOPEDGRTV", OLC_CMD(d).command) == nullptr)) {
        write_to_output(g, d, "Invalid choice, try again : ");
      } else {
        if (OLC_VAL(d)) { /* If there was a previous command. */
          if (OLC_CMD(d).command == 'T' || OLC_CMD(d).command == 'V') {
            OLC_CMD(d).if_flag = 1;
            zedit_disp_arg1(g, d);
          } else {
            write_to_output(g, d, "Is this command dependent on the success of the previous one? (y/n)\r\n");
            OLC_MODE(d) = ZEDIT_IF_FLAG;
          }
        } else { /* 'if-flag' not appropriate. */
          OLC_CMD(d).if_flag = 0;
          zedit_disp_arg1(g, d);
        }
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_IF_FLAG:
      /*
     * Parse the input for the if flag, and goto next quiz.
     */
      switch (*arg) {
        case 'y':
        case 'Y':
          OLC_CMD(d).if_flag = 1;
          break;
        case 'n':
        case 'N':
          OLC_CMD(d).if_flag = 0;
          break;
        default:
          write_to_output(g, d, "Try again : ");
          return;
      }
      zedit_disp_arg1(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ARG1:
      /*
     * Parse the input for arg1, and goto next quiz.
     */
      if (!isdigit(*arg)) {
        write_to_output(g, d, "Must be a numeric value, try again : ");
        return;
      }
      switch (OLC_CMD(d).command) {
        case 'M':
          if ((rnum = real_mobile(g, atoi(arg))) != NOBODY) {
            OLC_CMD(d).arg1 = rnum;
            zedit_disp_arg2(g, d);
          } else
            write_to_output(g, d, "That mobile does not exist, try again : ");
          break;
        case 'O':
        case 'P':
        case 'E':
        case 'G':
          if ((rnum = real_object(g, atoi(arg))) != NOTHING) {
            OLC_CMD(d).arg1 = rnum;
            zedit_disp_arg2(g, d);
          } else
            write_to_output(g, d, "That object does not exist, try again : ");
          break;
        case 'T':
        case 'V':
          if (atoi(arg) < MOB_TRIGGER || atoi(arg) > WLD_TRIGGER)
            write_to_output(g, d, "Invalid input.");
          else {
            OLC_CMD(d).arg1 = atoi(arg);
            zedit_disp_arg2(g, d);
          }
          break;
        case 'D':
        case 'R':
        default:
          /*
       * We should never get here.
       */
          cleanup_olc(g, d, CLEANUP_ALL);
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): case ARG1: Ack!");
          write_to_output(g, d, "Oops...\r\n");
          break;
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ARG2:
      /*
     * Parse the input for arg2, and goto next quiz.
     */
      if (!isdigit(*arg)) {
        write_to_output(g, d, "Must be a numeric value, try again : ");
        return;
      }
      switch (OLC_CMD(d).command) {
        case 'M':
        case 'O':
          OLC_CMD(d).arg2 = MIN(MAX_DUPLICATES, atoi(arg));
          OLC_CMD(d).arg3 = real_room(g, OLC_NUM(d));
          zedit_disp_arg4(g, d);
          break;
        case 'G':
          OLC_CMD(d).arg2 = MIN(MAX_DUPLICATES, atoi(arg));
          zedit_disp_arg5(g, d);
          break;
        case 'P':
        case 'E':
          OLC_CMD(d).arg2 = MIN(MAX_DUPLICATES, atoi(arg));
          zedit_disp_arg3(g, d);
          break;
        case 'V':
          OLC_CMD(d).arg2 = atoi(arg); /* context */
          OLC_CMD(d).arg3 = real_room(g, OLC_NUM(d));
          write_to_output(g, d, "Enter the global name : ");
          OLC_MODE(d) = ZEDIT_SARG1;
          break;
        case 'T':
          if (real_trigger(g, atoi(arg)) != NOTHING) {
            OLC_CMD(d).arg2 = real_trigger(g, atoi(arg)); /* trigger */
            OLC_CMD(d).arg3 = real_room(g, OLC_NUM(d));
            zedit_disp_menu(g, d);
          } else
            write_to_output(g, d, "That trigger does not exist, try again : ");
          break;
        case 'D':
          pos = atoi(arg);
          /*
       * Count directions.
       */
          if (pos < 0 || pos > NUM_OF_DIRS)
            write_to_output(g, d, "Try again : ");
          else {
            OLC_CMD(d).arg2 = pos;
            zedit_disp_arg3(g, d);
          }
          break;
        case 'R':
          if ((rnum = real_object(g, atoi(arg))) != NOTHING) {
            OLC_CMD(d).arg2 = rnum;
            zedit_disp_menu(g, d);
          } else
            write_to_output(g, d, "That object does not exist, try again : ");
          break;
        default:
          /*
       * We should never get here, but just in case...
       */
          cleanup_olc(g, d, CLEANUP_ALL);
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): case ARG2: Ack!");
          write_to_output(g, d, "Oops...\r\n");
          break;
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ARG3:
      /*
     * Parse the input for arg3, and go back to main menu.
     */
      if (!isdigit(*arg)) {
        write_to_output(g, d, "Must be a numeric value, try again : ");
        return;
      }
      switch (OLC_CMD(d).command) {
        case 'E':
          pos = atoi(arg);
          /*
       * Count number of wear positions.  We could use NUM_WEARS, this is
       * more reliable.
       */
          while (*equipment_types[i] != '\n')
            i++;
          if (pos < 0 || pos > i)
            write_to_output(g, d, "Try again : ");
          else {
            OLC_CMD(d).arg3 = pos;
            zedit_disp_arg5(g, d);
          }
          break;
        case 'P':
          if ((rnum = real_object(g, atoi(arg))) != NOTHING) {
            OLC_CMD(d).arg3 = rnum;
            zedit_disp_arg5(g, d);
          } else
            write_to_output(g, d, "That object does not exist, try again : ");
          break;
        case 'D':
          pos = atoi(arg);
          if (pos < 0 || pos > 2)
            write_to_output(g, d, "Try again : ");
          else {
            OLC_CMD(d).arg3 = pos;
            zedit_disp_menu(g, d);
          }
          break;
        case 'M':
        case 'O':
        case 'G':
        case 'R':
        case 'T':
        case 'V':
        default:
          /*
       * We should never get here, but just in case...
       */
          cleanup_olc(g, d, CLEANUP_ALL);
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): case ARG3: Ack!");
          write_to_output(g, d, "Oops...\r\n");
          break;
      }
      break;
      /*-------------------------------------------------------------------*/
    case ZEDIT_ARG4:
      /*
     * Parse the input for arg4, and goto next quiz.
     */
      if (!*arg) {
        static char zero[] = "0";
        arg                = zero;
      }

      if (!isdigit(*arg)) {
        write_to_output(g, d, "Must be a numeric value, try again : ");
        return;
      }
      switch (OLC_CMD(d).command) {
        case 'M':
        case 'O':
          OLC_CMD(d).arg4 = MIN(MAX_FROM_ROOM, atoi(arg));
          zedit_disp_arg5(g, d);
          break;
        case 'G':
        case 'P':
        case 'E':
        case 'V':
        case 'T':
        default:
          /*
       * We should never get here, but just in case...
       */
          cleanup_olc(g, d, CLEANUP_ALL);
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): case ARG4: Ack!");
          write_to_output(g, d, "Oops...\r\n");
          break;
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ARG5:
      /*
     * Parse the input for arg5, and goto next quiz.
     */
      if (!isdigit(*arg)) {
        write_to_output(g, d, "Must be a numeric value, try again : ");
        return;
      }
      if (atoi(arg) < 0 || atoi(arg) > 100) {
        write_to_output(g, d, "Value must be from 0 to 100");
        return;
      }
      switch (OLC_CMD(d).command) {
        case 'M':
        case 'O':
        case 'G':
        case 'P':
        case 'E':
          OLC_CMD(d).arg5 = atoi(arg);
          zedit_disp_menu(g, d);
          break;
        case 'V':
        case 'T':
        default:
          /*
       * We should never get here, but just in case...
       */
          cleanup_olc(g, d, CLEANUP_ALL);
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): case ARG5: Ack!");
          write_to_output(g, d, "Oops...\r\n");
          break;
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_SARG1:
      if (strlen(arg)) {
        if (OLC_CMD(d).sarg1)
          free(OLC_CMD(d).sarg1);
        OLC_CMD(d).sarg1 = strdup(arg);
        OLC_MODE(d)      = ZEDIT_SARG2;
        write_to_output(g, d, "Enter the global value : ");
      } else
        write_to_output(g, d, "Must have some name to assign : ");
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_SARG2:
      if (strlen(arg)) {
        free(OLC_CMD(d).sarg2);
        OLC_CMD(d).sarg2 = strdup(arg);
        zedit_disp_menu(g, d); /* a V command is complete: it has no arg4/arg5 (they would end the session) */
      } else
        write_to_output(g, d, "Must have some value to set it to :");
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_NAME:
      /*
     * Add new name and return to main menu.
     */
      if (genolc_checkstring(d, arg)) {
        if (OLC_ZONE(d)->name)
          free(OLC_ZONE(d)->name);
        else
          mud_log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
        OLC_ZONE(d)->name   = strdup(arg);
        OLC_ZONE(d)->number = 1;
      }
      zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_BUILDERS:
      /*
     * Add new builders list and return to main menu.
     */
      if (genolc_checkstring(d, arg)) {
        if (OLC_ZONE(d)->builders)
          free(OLC_ZONE(d)->builders);
        else
          mud_log("SYSERR: OLC: ZEDIT_ZONE_BUILDERS: no builders list to free!");
        OLC_ZONE(d)->builders = strdup(arg);
        OLC_ZONE(d)->number   = 1;
      }
      zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_RESET:
      /*
     * Parse and add new reset_mode and return to main menu.
     */
      pos = atoi(arg);
      if (!isdigit(*arg) || pos < 0 || pos > 2)
        write_to_output(g, d, "Try again (0-2) : ");
      else {
        OLC_ZONE(d)->reset_mode = pos;
        OLC_ZONE(d)->number     = 1;
        zedit_disp_menu(g, d);
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_LIFE:
      /*
     * Parse and add new lifespan and return to main menu.
     */
      pos = atoi(arg);
      if (!isdigit(*arg) || pos < 0 || pos > 240)
        write_to_output(g, d, "Try again (0-240) : ");
      else {
        OLC_ZONE(d)->lifespan = pos;
        OLC_ZONE(d)->number   = 1;
        zedit_disp_menu(g, d);
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_BOT:
      /*
     * Parse and add new bottom room in zone and return to main menu.
     */
      if (struct zone_data *previous = g->zones.before(OLC_ZNUM(d)->number))
        number = LIMIT(atoi(arg), previous->top + 1, OLC_ZONE(d)->top);
      else
        number = LIMIT(atoi(arg), 0, OLC_ZONE(d)->top);
      if (!zone_range_loses_something(g, d, number, OLC_ZONE(d)->top)) {
        OLC_ZONE(d)->bot    = number;
        OLC_ZONE(d)->number = 1;
      }
      zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_TOP:
      /*
     * Parse and add new top room in zone and return to main menu.
     */
      if (struct zone_data *next = g->zones.after(OLC_ZNUM(d)->number))
        number = LIMIT(atoi(arg), genolc_zonep_bottom(OLC_ZONE(d)), genolc_zone_bottom(g, next) - 1);
      else
        number = LIMIT(atoi(arg), genolc_zonep_bottom(OLC_ZONE(d)), 65000);
      if (!zone_range_loses_something(g, d, genolc_zonep_bottom(OLC_ZONE(d)), number)) {
        OLC_ZONE(d)->top    = number;
        OLC_ZONE(d)->number = 1;
      }
      zedit_disp_menu(g, d);
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_FLAGS:
      number = atoi(arg);
      if (number < 0 || number > NUM_ZONE_FLAGS) {
        write_to_output(g, d, "That is not a valid choice!\r\n");
        zedit_disp_flag_menu(g, d);
      } else if (number == 0) {
        zedit_disp_menu(g, d);
        break;
      } else {
        /*
       * Toggle the bit.
       */
        zone_data_toggle_flag(OLC_ZONE(d), number - 1);
        OLC_ZONE(d)->number = 1;
        zedit_disp_flag_menu(g, d);
      }
      return;

      /*-------------------------------------------------------------------*/
    case ZEDIT_MIN_LEVEL:
      /* Parse new min level of zone. */
      pos = atoi(arg);
      if (!isdigit(*arg) || pos < 0 || pos > CONFIG_LEVEL_CAP)
        write_to_output(g, d, "Try again (0 - CONFIG_LEVEL_CAP) : \r\n");
      else {
        OLC_ZONE(d)->min_level = pos;
        OLC_ZONE(d)->number    = 1;
        zedit_disp_menu(g, d);
      }
      break;

      /*-------------------------------------------------------------------*/
    case ZEDIT_MAX_LEVEL:
      /* Parse new max level of zone. */
      pos = atoi(arg);
      if (!isdigit(*arg) || pos < 0 || pos > CONFIG_LEVEL_CAP)
        write_to_output(g, d, "Try again (0 - CONFIG_LEVEL_CAP) : \r\n");
      else if (OLC_ZONE(d)->min_level > pos)
        write_to_output(g, d, "Max level can not be lower the Min level! :\r\n");
      else {
        OLC_ZONE(d)->max_level = pos;
        OLC_ZONE(d)->number    = 1;
        zedit_disp_menu(g, d);
      }
      break;

    default:
      /*
     * We should never get here, but just in case...
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): Reached default case!");
      write_to_output(g, d, "Oops...\r\n");
      break;
  }
}

/******************************************************************************/
/** End of parse_zedit()                                                     **/
/******************************************************************************/
