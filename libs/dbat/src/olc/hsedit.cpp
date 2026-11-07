/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
*  AderonMUD by Jamdog.                                                  *
*  Based on CircleMud3.5                                                 * 
*  OasisOLC - hsedit.c                                                   * 
*                                                                        * 
*  Copyright 2007 Stefan Cole (aka Jamdog)                               * 
*  Used at AderonMUD (aderonmud.genesismuds.com 3200)                    * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "dbat/hsedit.hpp"
#include "dbat/config.hpp"
#include "dbat/comm.hpp"
#include "dbat/utils.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/oasis.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/house.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void hsedit_disp_menu(GameWorld *g, struct descriptor_data *d);

/*------------------------------------------------------------------------*/
/* internal globals */
static const char *const house_flags[] = {
    "!GUEST",
    "FREE",
    "!IMM",
    "IMP_ONLY",
    "RENTFREE",
    "SAVE_!RENT",
    "!SAVE",
    "\n"};

static const char *const house_types[] = {
    "PLAYER_OWNED",
    "IMM_OWNED",
    "CLAN_OWNED",
    "UNOWNED",
    "\n"};
/*------------------------------------------------------------------------*\ 
  Utils and exported functions. 
\*------------------------------------------------------------------------*/

static void hsedit_setup_new(GameWorld *g, struct descriptor_data *d)
{
  CREATE(OLC_HOUSE(d), struct house_control_rec, 1);
  OLC_HOUSE(d)->vnum     = OLC_NUM(d);
  OLC_HOUSE(d)->exit_num = -1;
  OLC_HOUSE(d)->built_on = game_now(g);
  OLC_HOUSE(d)->mode     = HOUSE_PRIVATE;
  OLC_VAL(d)             = 0;
  hsedit_disp_menu(g, d);
}

/*------------------------------------------------------------------------*/

static void hsedit_setup_existing(GameWorld *g, struct descriptor_data *d, int real_num)
{
  struct house_control_rec *house;
  int i;

  /*. Build a copy of the house .*/
  CREATE(house, struct house_control_rec, 1);
  *house = g->house_control[real_num];

  /* allocate space for all strings  */

  /* Copy all non-string variables */
  house->vnum          = g->house_control[real_num].vnum;
  house->atrium        = g->house_control[real_num].atrium;
  house->owner         = g->house_control[real_num].owner;
  house->exit_num      = g->house_control[real_num].exit_num;
  house->built_on      = g->house_control[real_num].built_on;
  house->mode          = g->house_control[real_num].mode;
  house->bitvector     = g->house_control[real_num].bitvector;
  house->last_payment  = g->house_control[real_num].last_payment;
  house->num_of_guests = g->house_control[real_num].num_of_guests;
  for (i = 0; i < MAX_GUESTS; i++)
    house->guests[i] = g->house_control[real_num].guests[i];

  /*. Attach room copy to players descriptor .*/
  OLC_HOUSE(d) = house;
  OLC_VAL(d)   = 0;
  hsedit_disp_menu(g, d);
}

/*-----------------------------------------------------------1-------------*/
static void hsedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  int house_rnum;

  /* this is done rather differently from the other OLCs */
  /* Houses have a pre-allocated list size, so just copy */
  /* the OLC house back into it */

  house_rnum = find_house(g, OLC_NUM(d));
  if (house_rnum != NOWHERE) {
    /* This house VNUM is already in the list */
    /* Replace the old data                   */
    free_house(g->house_control + house_rnum);
    g->house_control[house_rnum] = *OLC_HOUSE(d);
  } else {
    /*. House doesn't exist, hafta add it .*/
    house_rnum = g->num_of_houses++;
    if (house_rnum < MAX_HOUSES) {
      g->house_control[house_rnum]      = *(OLC_HOUSE(d));
      g->house_control[house_rnum].vnum = OLC_NUM(d);
    } else {
      send_to_char(g, d->character, "MAX House limit reached - Unable to save this house!");
      mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "HSEDIT: Max houses limit reached - Unable to save OLC data");
    }
  }
  /* The new house is stored - now to ensure the roomsflags are correct */
  if (real_room(g, OLC_HOUSE(d)->vnum) != NOWHERE)
    rroom_set_flag(g, real_room(g, OLC_HOUSE(d)->vnum), ROOM_HOUSE, true);

  if (real_room(g, OLC_HOUSE(d)->atrium) != NOWHERE)
    rroom_set_flag(g, real_room(g, OLC_HOUSE(d)->atrium), ROOM_ATRIUM, true);

  //  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_HOUSE);
}

/*------------------------------------------------------------------------*/

static void hsedit_save_to_disk(GameWorld *g)
{
  /* Why bother writing a new function when there is already one that does the job */
  House_save_control(g);
}

/*------------------------------------------------------------------------*/

void free_house(struct house_control_rec *house)
{
  /* House structure has no strings to free                */
  /* This function is here in case someone adds some later */
}

/*------------------------------------------------------------------------*/

static void hedit_delete_house(GameWorld *g, struct descriptor_data *d, room_vnum house_vnum)
{
  int i, j;
  room_rnum real_house;

  if ((i = find_house(g, house_vnum)) == NOWHERE) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: hsedit: Invalid house vnum in hedit_delete_house\r\n");
    cleanup_olc(g, d, CLEANUP_STRUCTS);
    return;
  }

  if ((real_house = real_room(g, g->house_control[i].vnum)) == NOWHERE)
    mud_log("SYSERR: House %d had invalid vnum %d!", house_vnum, g->house_control[i].vnum);
  else
    rroom_set_flag(g, real_house, ROOM_HOUSE | ROOM_HOUSE_CRASH, false);

  House_delete_file(g->house_control[i].vnum);

  for (j = i; j < g->num_of_houses - 1; j++)
    g->house_control[j] = g->house_control[j + 1];

  g->num_of_houses--;

  send_to_char(g, d->character, "House deleted.\r\n");
  House_save_control(g);

  cleanup_olc(g, d, CLEANUP_ALL);
}

/************************************************************************** 
 Menu functions 
 
**************************************************************************/

static void hsedit_disp_flags_menu(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;
  char buf1[MAX_STRING_LENGTH];

  clear_screen(g, d);
  for (counter = 0; counter < HOUSE_NUM_FLAGS; counter++) {
    send_to_char(g, d->character, "@g%2d@D)@y %-20.20s @n", counter + 1, house_flags[counter]);
    if (!(++columns % 2))
      send_to_char(g, d->character, "\r\n");
  }
  sprintbit(OLC_HOUSE(d)->bitvector, house_flags, buf1, sizeof(buf1));
  send_to_char(g, d->character,
               "\r\nHouse flags: @g%s@n\r\n"
               "Enter house flags, 0 to quit : ",
               buf1);
  OLC_MODE(d) = HSEDIT_FLAGS;
}

static void hsedit_owner_menu(GameWorld *g, struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH];
  struct house_control_rec *house;

  house = OLC_HOUSE(d);

  sprintf(buf,
          "@g1@D)@g Owner Name : @c%s@n\r\n"
          "@g2@D)@g Owner ID   : @c%ld\r\n"
          "@gQ@D)@g Back to main menu\r\n"
          "@gEnter choice : @n",
          get_name_by_id(g, house->owner), house->owner);
  send_to_char(g, d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_OWNER_MENU;
}

static void hsedit_dir_menu(GameWorld *g, struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH];
  struct house_control_rec *house;
  room_rnum house_rnum, newroom[12];
  int i;

  house = OLC_HOUSE(d);

  mudlog(g, LOG_CMP, ADMLVL_BUILDER, true, "(LOG) hsedit_dir_menu: house vnum = %d", house->vnum);
  house_rnum = real_room(g, house->vnum);

  if ((house_rnum < 0) || (house_rnum == NOWHERE)) {
    sprintf(buf, "WARNING: You cannot set an atium direction before selecting a valid room vnum\r\n"
                 "(Press Enter)\r\n");
    OLC_MODE(d) = HSEDIT_NOVNUM;
  } else {
    /* Grab exit rooms */
    for (i = 0; i < 12; i++) {
      if (rroom_get_dir_option(g, house_rnum, i))
        newroom[i] = rroom_get_dir_option(g, house_rnum, i)->to_room;
      else
        newroom[i] = NOWHERE;
    }

    sprintf(buf,
            "@g1@D)@g North  : @D(@c%s@D)\r\n"
            "@g2@D)@g East   : @D(@c%s@D)\r\n"
            "@g3@D)@g South  : @D(@c%s@D)\r\n"
            "@g4@D)@g West   : @D(@c%s@D)\r\n"
            "@g5@D)@g Up     : @D(@c%s@D)\r\n"
            "@g6@D)@g Down   : @D(@c%s@D)\r\n"
            "@g7@D)@g NorthW : @D(@c%s@D)\r\n"
            "@g8@D)@g NorthE : @D(@c%s@D)\r\n"
            "@g9@D)@g SouthE : @D(@c%s@D)\r\n"
            "@g10@D)@g SouthW : @D(@c%s@D)\r\n"
            "@g11@D)@g Inside : @D(@c%s@D)\r\n"
            "@g12@D)@g Outside: @D(@c%s@D)\r\n"
            "@gQ@D)@g Back to main menu\r\n"
            "@gEnter atrium direction : @n",

            newroom[0] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[0])),
            newroom[1] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[1])),
            newroom[2] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[2])),
            newroom[3] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[3])),
            newroom[4] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[4])),
            newroom[5] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[5])),
            newroom[6] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[6])),
            newroom[7] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[7])),
            newroom[8] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[8])),
            newroom[9] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[9])),
            newroom[10] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[10])),
            newroom[11] == NOWHERE ? "NO ROOM" : rroom_get_name(g, (newroom[11])));
    OLC_MODE(d) = HSEDIT_DIR_MENU;
  }
  send_to_char(g, d->character, "%s", buf);
}

static void hsedit_disp_type_menu(GameWorld *g, struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(g, d);
  for (counter = 0; counter < NUM_HOUSE_TYPES; counter++) {
    send_to_char(g, d->character, "@g%2d@D)@y %-20.20s @n", counter, house_types[counter]);
    if (!(++columns % 2))
      send_to_char(g, d->character, "\r\n");
  }
  send_to_char(g, d->character, "\r\nEnter house type : ");
  OLC_MODE(d) = HSEDIT_TYPE;
}

static void hsedit_disp_guest_menu(GameWorld *g, struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH];
  char not_set[128];
  struct house_control_rec *house;

  house = OLC_HOUSE(d);

  sprintf(not_set, "@D(@yNOT SET@D)@n");

  sprintf(buf,
          " @g1@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g2@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g3@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g4@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g5@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g6@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g7@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g8@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          " @g9@D)@c %s @D(@gID: @c%ld@D)@n\r\n"
          "@g10@D)@c %s @D(@gID: @c%ld@D)@n\r\n\r\n"
          "@gA@D)@g Add a guest\r\n"
          "@gD@D)@g Delete a guest\r\n"
          "@gC@D)@g Clear guest list\r\n"
          "@gQ@D)@g Back to main menu\r\n"
          "@gEnter selection @D(@gA@D/@gD@D/@gC@D/@gQ@D)@n: ",

          get_name_by_id(g, house->guests[0]) == nullptr ? not_set : get_name_by_id(g, house->guests[0]),
          house->guests[0] < 1 ? 0 : house->guests[0],
          get_name_by_id(g, house->guests[1]) == nullptr ? not_set : get_name_by_id(g, house->guests[1]),
          house->guests[1] < 1 ? 0 : house->guests[1],
          get_name_by_id(g, house->guests[2]) == nullptr ? not_set : get_name_by_id(g, house->guests[2]),
          house->guests[2] < 1 ? 0 : house->guests[2], get_name_by_id(g, house->guests[3]) == nullptr ? not_set : get_name_by_id(g, house->guests[3]),
          house->guests[3] < 1 ? 0 : house->guests[3],
          get_name_by_id(g, house->guests[4]) == nullptr ? not_set : get_name_by_id(g, house->guests[4]),
          house->guests[4] < 1 ? 0 : house->guests[4],
          get_name_by_id(g, house->guests[5]) == nullptr ? not_set : get_name_by_id(g, house->guests[5]),
          house->guests[5] < 1 ? 0 : house->guests[5],
          get_name_by_id(g, house->guests[6]) == nullptr ? not_set : get_name_by_id(g, house->guests[6]),
          house->guests[6] < 1 ? 0 : house->guests[6],
          get_name_by_id(g, house->guests[7]) == nullptr ? not_set : get_name_by_id(g, house->guests[7]),
          house->guests[7] < 1 ? 0 : house->guests[7],
          get_name_by_id(g, house->guests[8]) == nullptr ? not_set : get_name_by_id(g, house->guests[8]),
          house->guests[8] < 1 ? 0 : house->guests[8],
          get_name_by_id(g, house->guests[9]) == nullptr ? not_set : get_name_by_id(g, house->guests[9]),
          house->guests[9] < 1 ? 0 : house->guests[9]);
  send_to_char(g, d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_GUEST_MENU;
}

static char *hsedit_list_guests(GameWorld *g, struct house_control_rec *thishouse, char *guestlist)
{
  int j, num_printed;
  char *temp;

  if (thishouse->num_of_guests == 0) {
    sprintf(guestlist, "NONE");
    return (guestlist);
  }

  for (num_printed = j = 0; j < thishouse->num_of_guests; j++) {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(g, thishouse->guests[j])) == nullptr)
      continue;

    num_printed++;
    sprintf(guestlist + strlen(guestlist), "%c%s ", toupper(*temp), temp + 1);
  }

  if (num_printed == 0)
    sprintf(guestlist, "all dead");

  return (guestlist);
}

/* the main menu */
static void hsedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], built_on[128],
      last_pay[128], buf2[MAX_STRING_LENGTH];
  char *timestr, no_name[128];
  struct house_control_rec *house;

  clear_screen(g, d);
  house = OLC_HOUSE(d);

  if (house->built_on) {
    timestr         = asctime(localtime(&(house->built_on)));
    *(timestr + 10) = '\0';
    strlcpy(built_on, timestr, sizeof(built_on));
  } else
    strcpy(built_on, "Unknown"); /* strcpy: OK (for 'strlen("Unknown") < 128') */

  if (house->last_payment) {
    timestr         = asctime(localtime(&(house->last_payment)));
    *(timestr + 10) = '\0';
    strlcpy(last_pay, timestr, sizeof(last_pay));
  } else
    strcpy(last_pay, "None"); /* strcpy: OK (for 'strlen("None") < 128') */

  *buf2 = '\0';
  sprintbit(house->bitvector, house_flags, buf1, sizeof(buf1));
  //  sprintf(buf2, "%d %d %d %d", house->value[0], house->value[1], house->value[2], house->value[3]);
  sprintf(no_name, "(NOBODY)");
  snprintf(buf, sizeof(buf),
           "@D-- @RJamdog's House OLC Editor @D--\r\n"
           "@D--@g House number : @D[@c%d@D]     @gHouse zone: @D[@c%d@D]\r\n"
           "@g1@D)@g Owner       : @c%ld -- %s\r\n"
           "@g2@D)@g Atrium      : @c%d\r\n"
           "@g3@D)@g Direction   : @c%s\r\n"
           "@g4@D)@g House Type  : @c%s\r\n"
           "@g5@D)@g Built on    : @c%s\r\n"
           "@g6@D)@g Payment     : @c%s\r\n"
           "@g7@D)@g Guests      : @c%s\r\n"
           "@g8@D)@g Flags       : @c%s\r\n"
           // "9) Values      : %s\r\n"
           "@gX@D)@g Delete this house\r\n"
           "@gQ@D)@g Quit\r\n"
           "@gEnter choice : @n",

           OLC_NUM(d), g->zones[OLC_ZNUM(d)].number,
           house->owner, get_name_by_id(g, house->owner) == nullptr ? no_name : get_name_by_id(g, house->owner),
           house->atrium, ((house->exit_num >= 0) && (house->exit_num <= 11)) ? dirs[(house->exit_num)] : "NONE",
           house_types[(house->mode)], built_on, last_pay, hsedit_list_guests(g, house, buf2), buf1);
  send_to_char(g, d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_MAIN_MENU;
}

/************************************************************************** 
  The main loop 
 
**************************************************************************/

void hsedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int number = 0, id = 0, i;
  room_rnum house, rnum;
  char *tmp;
  bool found = false;

  mudlog(g, LOG_CMP, ADMLVL_BUILDER, false, "(LOG) hsedit_parse: OLC mode %d", OLC_MODE(d));

  switch (OLC_MODE(d)) {
    case HSEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          hsedit_save_internally(g, d);
          mudlog(g, LOG_CMP, ADMLVL_BUILDER, true, "OLC: %s edits house %d", char_get_name(d->character), OLC_NUM(d));
          if (CONFIG_OLC_SAVE) {
            hsedit_save_to_disk(g);
            write_to_output(g, d, "House saved to disk.\r\n");
          } else
            write_to_output(g, d, "House saved to memory.\r\n");
          /*. Do NOT free strings! just the room structure .*/
          cleanup_olc(g, d, CLEANUP_STRUCTS);
          break;
        case 'n':
        case 'N':
          /* free everything up, including strings etc */
          cleanup_olc(g, d, CLEANUP_ALL);
          break;
        default:
          send_to_char(g, d->character, "Invalid choice!\r\n");
          send_to_char(g, d->character, "Do you wish to save this house internally? : ");
          break;
      }
      return;

    case HSEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /*. Something has been modified .*/
            send_to_char(g, d->character, "Do you wish to save this house internally? : ");
            OLC_MODE(d) = HSEDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case '1':
          hsedit_owner_menu(g, d);
          break;

        case '2':
          if ((OLC_HOUSE(d)->vnum == NOWHERE) || (real_room(g, OLC_HOUSE(d)->vnum) == NOWHERE)) {
            send_to_char(g, d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
            mudlog(g, LOG_NRM, ADMLVL_GRGOD, true, "SYSERR: Invalid house VNUM in hsedit");
          } else {
            send_to_char(g, d->character, "Enter atrium room vnum:");
            OLC_MODE(d) = HSEDIT_ATRIUM;
          }
          break;

        case '3':
          if ((OLC_HOUSE(d)->vnum == NOWHERE) || (real_room(g, OLC_HOUSE(d)->vnum) == NOWHERE)) {
            send_to_char(g, d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
            mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: Invalid house VNUM in hsedit");
          } else {
            hsedit_dir_menu(g, d);
          }
          break;

        case '4':
          hsedit_disp_type_menu(g, d);
          break;

        case '5':
          send_to_char(g, d->character, "Set build date to now? (Y/N):");
          OLC_MODE(d) = HSEDIT_BUILD_DATE;
          break;

        case '6':
          send_to_char(g, d->character, "Set last payment as now? (Y/N) : ");
          OLC_MODE(d) = HSEDIT_PAYMENT;
          break;

        case '7':
          hsedit_disp_guest_menu(g, d);
          break;

        case '8':
          hsedit_disp_flags_menu(g, d);
          break;

          //  case '9':
          //    hsedit_disp_val0_menu(d);
          //    break;

        case 'x':
        case 'X':
          send_to_char(g, d->character, "Are you sure you want to delete this house? (Y/N) : ");
          OLC_MODE(d) = HSEDIT_DELETE;
          break;

        default:
          send_to_char(g, d->character, "Invalid choice!\r\n");
          hsedit_disp_menu(g, d);
          break;
      }
      return;

    case HSEDIT_OWNER_MENU:
      switch (*arg) {
        case '1':
          send_to_char(g, d->character, "Enter the name of the owner : ");
          OLC_MODE(d) = HSEDIT_OWNER_NAME;
          break;

        case '2':
          send_to_char(g, d->character, "Enter the user id of the owner : ");
          OLC_MODE(d) = HSEDIT_OWNER_ID;
          break;

        case 'Q':
          hsedit_disp_menu(g, d);
          break;
      }
      return;
      break;

    case HSEDIT_OWNER_NAME:
      if ((id = get_id_by_name(g, arg)) < 0) {
        send_to_char(g, d->character, "There is no such player.\r\n");
        hsedit_owner_menu(g, d);
        return;
      }
      OLC_HOUSE(d)->owner = id;
      break;

    case HSEDIT_OWNER_ID:
      id = atoi(arg);
      if ((tmp = get_name_by_id(g, id)) == nullptr) {
        send_to_char(g, d->character, "There is no such player.\r\n");
        hsedit_owner_menu(g, d);
        return;
      }
      OLC_HOUSE(d)->owner = id;
      break;

    case HSEDIT_ATRIUM:
      number = atoi(arg);
      if (number == 0) {
        /* '0' chosen - go back to main menu */
        hsedit_disp_menu(g, d);
        return;
      }
      rnum = real_room(g, OLC_HOUSE(d)->vnum);
      if (real_room(g, number) == NOWHERE) {
        send_to_char(g, d->character, "Room VNUM does not exist.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
        return;
      }
      for (i = 0; i < 12; i++) {
        if (rroom_get_dir_option(g, rnum, i)) {
          if (rroom_get_dir_option(g, rnum, i)->to_room == real_room(g, number)) {
            found = true;
            id    = i;
          }
        }
      }

      if (found == false) {
        send_to_char(g, d->character, "Atrium MUST be an adjoining room.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
        return;
      }
      OLC_HOUSE(d)->atrium   = number;
      OLC_HOUSE(d)->exit_num = id;
      break;

    case HSEDIT_DIR_MENU:

      number = atoi(arg) - 1;

      if ((*arg == 'q') || (*arg == 'Q') || (number == -1)) {
        hsedit_disp_menu(g, d);
        return;
      }
      if ((number < 0) || (number >= NUM_OF_DIRS)) {
        send_to_char(g, d->character, "Invalid choice, Please select a direction (1-12, Q to quit) : ");
        return;
      }
      house = real_room(g, OLC_HOUSE(d)->vnum);
      if (!(rroom_get_dir_option(g, house, number))) {
        send_to_char(g, d->character, "You cannot set the atrium to a room that doesn't exist!\r\n");
        hsedit_dir_menu(g, d);
        return;
      }
      if ((rroom_get_dir_option(g, house, number)->to_room) == NOWHERE) {
        send_to_char(g, d->character, "You cannot set the atrium to nowhere!\r\n");
        hsedit_dir_menu(g, d);
        return;
      }
      OLC_HOUSE(d)->exit_num = number;

      rnum                 = rroom_get_dir_option(g, house, number)->to_room;
      OLC_HOUSE(d)->atrium = rroom_get_vnum(g, rnum);
      break;

    case HSEDIT_NOVNUM:
      /* Just an 'enter' keypress - don't do anything */
      break;

    case HSEDIT_BUILD_DATE:
      switch (*arg) {
        case 'y':
        case 'Y':
          OLC_HOUSE(d)->built_on = game_now(g);
          break;
        case 'n':
        case 'N':
          send_to_char(g, d->character, "Build Date not changed\r\n");
          hsedit_disp_menu(g, d);
          return;
          break;
      }
      break;

    case HSEDIT_DELETE:
      switch (*arg) {
        case 'y':
        case 'Y':
          hedit_delete_house(g, d, OLC_HOUSE(d)->vnum);
          return;
        case 'n':
        case 'N':
          send_to_char(g, d->character, "House not deleted!\r\n");
          hsedit_disp_menu(g, d);
          return;
          break;
      }
      break;

    case HSEDIT_BUILDER:
      if ((id = get_id_by_name(g, arg)) < 0) {
        send_to_char(g, d->character, "No such player.\r\n");
        return;
      }
      OLC_HOUSE(d)->builtby = id;
      send_to_char(g, d->character, "Builder changed.\r\n");
      break;

    case HSEDIT_PAYMENT:
      switch (*arg) {
        case 'y':
        case 'Y':
          OLC_HOUSE(d)->last_payment = game_now(g);
          break;
        case 'n':
        case 'N':
          send_to_char(g, d->character, "Last Payment Date not changed\r\n");
          hsedit_disp_menu(g, d);
          return;
          break;
      }
      break;

    case HSEDIT_GUEST_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          if (OLC_HOUSE(d)->num_of_guests > (MAX_GUESTS - 1)) {
            send_to_char(g, d->character, "Guest List Full! - delete some before adding more\r\nEnter selection (A/D/C/Q) : ");
          } else {
            send_to_char(g, d->character, "Name of guest to add: ");
            OLC_MODE(d) = HSEDIT_GUEST_ADD;
          }
          break;

        case 'd':
        case 'D':
          if (OLC_HOUSE(d)->num_of_guests < 1) {
            send_to_char(g, d->character, "Guest List Empty! - add a guest before trying to delete one\r\nEnter selection (A/D/C/Q) : ");
          } else {
            send_to_char(g, d->character, "Name of guest to delete : ");
            OLC_MODE(d) = HSEDIT_GUEST_DELETE;
          }
          break;

        case 'c':
        case 'C':
          send_to_char(g, d->character, "Clear guest list? (Y/N) : ");
          OLC_MODE(d) = HSEDIT_GUEST_CLEAR;
          break;

        case 'q':
        case 'Q':
          hsedit_disp_menu(g, d);
          break;

        default:
          send_to_char(g, d->character, "Invalid choice!\r\n\r\n");
          hsedit_disp_guest_menu(g, d);
          break;
      }
      return;
      break;

    case HSEDIT_GUEST_ADD:
      if ((id = get_id_by_name(g, arg)) < 0) {
        send_to_char(g, d->character, "No such player.\r\n");
        hsedit_disp_guest_menu(g, d);
        return;
      }
      if (id == char_get_idnum(d->character)) {
        send_to_char(g, d->character, "House owner should not be in the guest list!\r\n");
        hsedit_disp_guest_menu(g, d);
        return;
      }
      for (i = 0; i < OLC_HOUSE(d)->num_of_guests; i++) {
        if (OLC_HOUSE(d)->guests[i] == id) {
          send_to_char(g, d->character, "That player is already in the guest list!.\r\n");
          hsedit_disp_guest_menu(g, d);
          return;
        }
      }
      i                       = OLC_HOUSE(d)->num_of_guests++;
      OLC_HOUSE(d)->guests[i] = id;
      send_to_char(g, d->character, "Guest added.\r\n");
      hsedit_disp_guest_menu(g, d);
      return;
      break;

    case HSEDIT_GUEST_DELETE:
      if ((id = get_id_by_name(g, arg)) < 0) {
        send_to_char(g, d->character, "No such player.\r\n");
        hsedit_disp_guest_menu(g, d);
        return;
      }
      if (id == char_get_idnum(d->character)) {
        send_to_char(g, d->character, "House owner should not be in the guest list!\r\n");
        hsedit_disp_guest_menu(g, d);
        return;
      }
      for (i = 0; i < OLC_HOUSE(d)->num_of_guests; i++) {
        if (OLC_HOUSE(d)->guests[i] == id) {
          for (; i < OLC_HOUSE(d)->num_of_guests; i++)
            OLC_HOUSE(d)->guests[i] = OLC_HOUSE(d)->guests[i + 1];
          OLC_HOUSE(d)->num_of_guests--;
          send_to_char(g, d->character, "Guest deleted.\r\n");
          OLC_VAL(d) = 1;
          hsedit_disp_guest_menu(g, d);
          return;
        }
      }
      send_to_char(g, d->character, "That player isn't in the guest list!\r\n");
      hsedit_disp_guest_menu(g, d);
      return;
      break;

    case HSEDIT_GUEST_CLEAR:
      switch (*arg) {
        case 'n':
        case 'N':
          send_to_char(g, d->character, "Invalid choice!");
          return;

        case 'y':
        case 'Y':
          OLC_HOUSE(d)->num_of_guests = 0;
          for (i = 0; i < MAX_GUESTS; i++)
            OLC_HOUSE(d)->guests[i] = 0;

          send_to_char(g, d->character, "Guest List Cleared!");
          OLC_VAL(d) = 1;
          hsedit_disp_guest_menu(g, d);
          break;

        default:
          send_to_char(g, d->character, "Invalid choice!\r\nClear Guest List? (Y/N) : ");
          return;
      }
      break;

    case HSEDIT_TYPE:
      number = atoi(arg);
      if (number < 0 || number >= NUM_HOUSE_TYPES) {
        send_to_char(g, d->character, "Invalid choice!");
        hsedit_disp_type_menu(g, d);
        return;
      } else
        OLC_HOUSE(d)->mode = number;
      break;

    case HSEDIT_FLAGS:
      number = atoi(arg);
      if ((number < 0) || (number > HOUSE_NUM_FLAGS)) {
        send_to_char(g, d->character, "That's not a valid choice!\r\n");
        hsedit_disp_flags_menu(g, d);
      } else {
        if (number == 0)
          break;
        else {
          /* toggle bits */
          if (IS_SET(OLC_HOUSE(d)->bitvector, 1U << (number - 1)))
            REMOVE_BIT(OLC_HOUSE(d)->bitvector, 1U << (number - 1));
          else
            SET_BIT(OLC_HOUSE(d)->bitvector, 1U << (number - 1));
          hsedit_disp_flags_menu(g, d);
        }
      }
      return;

      /* Houses have no 'values', but this is left commented out for future 
expansion    
  case HSEDIT_VALUE_0: 
    number = atoi(arg); 
    OLC_HOUSE(d)->value[0] = number; 
    hsedit_disp_val1_menu(d); 
    return; 
    
  case HSEDIT_VALUE_1: 
    OLC_HOUSE(d)->value[1] = atoi(arg); 
    hsedit_disp_val2_menu(d); 
    return;  
    
  case HSEDIT_VALUE_2: 
    OLC_HOUSE(d)->value[2] = atoi(arg); 
    hsedit_disp_val3_menu(d); 
    return;  
    
  case HSEDIT_VALUE_3: 
    OLC_HOUSE(d)->value[3] = atoi(arg); 
    break;    
*/

    default:
      /* we should never get here */
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: Reached default case in parse_hsedit");
      break;
  }
  /*. If we get this far, something has been changed .*/
  OLC_VAL(d) = 1;
  hsedit_disp_menu(g, d);
}

ACMD(do_oasis_hsedit)
{
  IDXTYPE number = NOWHERE;
  int save = 0, real_num;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                  **/

  /****************************************************************************/
  two_arguments(argument, buf1, buf2);

  /****************************************************************************/
  /** If there aren't any arguments...grab the number of the current room... **/

  /****************************************************************************/
  if (!*buf1) {
    number = rroom_get_vnum(g, char_get_in_room(ch));
  } else if (!isdigit(*buf1)) {
    if (strcasecmp("save", buf1) != 0) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = true;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (char_get_olc_zone(g, ch) > 0) {
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

  /****************************************************************************/
  /** If a numeric argument was given, get it.                              **/

  /****************************************************************************/
  if (number == NOWHERE)
    number = atoi(buf1);

  /****************************************************************************/
  /** Check that whatever it is isn't already being edited.                **/

  /****************************************************************************/
  for (d = g->descriptors; d; d = d->next) {
    if (desc_get_connected(d) == CON_HSEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(g, ch, "That house is currently being edited by %s.\r\n",
                     char_pers(g, d->character, ch));
        return;
      }
    }
  }

  /****************************************************************************/
  /** Point d to the builder's descriptor (for easier typing later).         **/

  /****************************************************************************/
  d = char_get_desc(ch);

  /****************************************************************************/
  /** Give the descriptor an OLC structure.                                 **/

  /****************************************************************************/
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: do_oasis_hsedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /****************************************************************************/
  /** Find the zone.                                                         **/

  /****************************************************************************/
  OLC_ZNUM(d) = save ? real_zone(g, number) : real_zone_by_thing(g, number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/

    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/

  /****************************************************************************/
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_to_char(g, ch, " You do not have permission to edit zone %d. Try zone %d.\r\n", g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "OLC: %s tried to edit zone %d allowed zone %d",
           char_get_name(ch), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                **/

    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** If we need to save, save the houses.                                  **/

  /****************************************************************************/
  if (save) {
    send_to_char(g, ch, "Saving all houses in zone %d.\r\n",
                 g->zones[OLC_ZNUM(d)].number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
           "OLC: %s saves house info for zone %d.", char_get_name(ch),
           g->zones[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Save the houses in this zone.                                       **/

    /**************************************************************************/
    hsedit_save_to_disk(g);

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                **/

    /**************************************************************************/
    free(d->olc);
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  /****************************************************************************/
  /** If this is a new house, setup a new house, otherwise setup the       **/
  /** existing house.                                                    **/

  /****************************************************************************/
  real_num = find_house(g, number);
  if (real_num == NOWHERE) {
    /* Do a quick check to ensure there is room for more */
    if (g->num_of_houses >= MAX_HOUSES) {
      send_to_char(g, ch, "MAX houses limit reached (%d) - Unable to create more.\r\n", MAX_HOUSES);
      mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "HSEDIT: MAX houses limit reached (%d)\r\n", MAX_HOUSES);
      return;
    }
    hsedit_setup_new(g, d);
  } else {
    hsedit_setup_existing(g, d, real_num);
  }

  desc_set_connected(d, CON_HSEDIT);

  /****************************************************************************/
  /** Send the OLC message to the players in the same room as the builder.   **/

  /****************************************************************************/
  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  /****************************************************************************/
  /** Log the OLC message.                                                   **/

  /****************************************************************************/
  mudlog(g, LOG_CMP, ADMLVL_BUILDER, true, "OLC: (hsedit) %s starts editing zone %d allowed zone %d",
         char_get_name(ch), g->zones[OLC_ZNUM(d)].number, char_get_olc_zone(g, ch));
}
