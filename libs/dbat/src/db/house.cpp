/* ************************************************************************
*   File: house.c                                       Part of CircleMUD *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cerrno>
#include <cstdlib>

#include "dbat/affect.hpp"
#include "dbat/house.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/utils.hpp"
#include "dbat/objsave.hpp"
#include "dbat/fileop.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void House_list_guests(GameWorld *g, struct char_data *ch, int i, int quiet);

/* local globals */

/* local functions */
static bool House_get_filename(room_vnum vnum, char *filename, size_t maxlen);
static bool House_load(GameWorld *g, room_vnum vnum);
static bool House_save(GameWorld *g, const id_list &list, FILE *fp, int location);
static void House_restore_weight(GameWorld *g, const id_list &list);

static void hcontrol_build_house(GameWorld *g, struct char_data *ch, char *arg);
static void hcontrol_destroy_house(GameWorld *g, struct char_data *ch, char *arg);
static void hcontrol_pay_house(GameWorld *g, struct char_data *ch, char *arg);

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
static bool House_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
  if (vnum == NOWHERE)
    return false;

  snprintf(filename, maxlen, LIB_HOUSE "%d.house", vnum);
  return true;
}

/* Save all objects for a house (recursive; initial call must be followed
   by a call to House_restore_weight)  Assumes file is open already. */
static bool House_save(GameWorld *g, const id_list &list, FILE *fp, int location)
{
  std::vector<struct obj_data *> saved;
  std::vector<long> ids = list.ids();
  bool result           = true;

  /* Newest first, an object that isn't to be rented is passed over, but only one at a time: the one after it is
   * saved whatever it is. That is how the old recursion along the list behaved, and what the house files hold. */
  for (std::size_t i = 0; i < ids.size(); i++) {
    struct obj_data *obj = obj_by_id(g, ids[i]);
    if (obj && obj_extra_flagged(obj, ITEM_NORENT) && i + 1 < ids.size())
      obj = obj_by_id(g, ids[++i]);
    else if (obj && obj_extra_flagged(obj, ITEM_NORENT))
      obj = nullptr;
    if (obj)
      saved.push_back(obj);
  }

  /* Oldest first, each after what it contains. The result is the newest one's. */
  for (std::size_t i = saved.size(); i-- > 0;) {
    struct obj_data *obj = saved[i], *tmp;

    House_save(g, obj_get_contains(obj), fp, MIN(0, location) - 1);
    result = Obj_to_store(g, obj, fp, location);
    if (!result)
      continue;
    for (tmp = obj_get_in_obj(obj); tmp; tmp = obj_get_in_obj(tmp))
      obj_set_weight(tmp, obj_get_weight(tmp) - (obj_get_weight(obj)));
  }
  return result;
}

/* restore weight of containers after House_save has changed them for saving */
static void House_restore_weight(GameWorld *g, const id_list &list)
{
  list.each([&](long id) {
    if (struct obj_data *obj = obj_by_id(g, id)) {
      House_restore_weight(g, obj_get_contains(obj));
      if (obj_get_in_obj(obj))
        obj_set_weight(obj_get_in_obj(obj), obj_get_weight(obj_get_in_obj(obj)) + (obj_get_weight(obj)));
    }
  });
}

/* Save all objects in a house */
static void House_crashsave(GameWorld *g, room_vnum vnum)
{
  struct atomic_file af;
  room_vnum rnum;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;

  if (!room_of(g, rnum = vnum))
    return;
  if (!House_get_filename(vnum, buf, sizeof(buf)))
    return;
  if (!(fp = atomic_open(&af, buf, true))) {
    perror("SYSERR: Error saving house file");
    return;
  }
  if (!House_save(g, room_get_contents(g, rnum), fp, 0)) {
    atomic_abort(&af); /* keep the last good house file */
    return;
  }
  bool saved = atomic_commit(&af);
  House_restore_weight(g, room_get_contents(g, rnum));
  if (saved) /* otherwise leave it flagged so the next pass tries again */
    room_set_flag(g, rnum, ROOM_HOUSE_CRASH, false);
}

/* Delete a house save file */
void House_delete_file(room_vnum vnum)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;
  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT)
      mud_log("SYSERR: Error deleting house file #%d. (1): %s", vnum, strerror(errno));
    return;
  }
  fclose(fl);
  if (remove(filename) < 0)
    mud_log("SYSERR: Error deleting house file #%d. (2): %s", vnum, strerror(errno));
}

/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/

int find_house(GameWorld *g, room_vnum vnum)
{
  int i;

  for (i = 0; i < g->num_of_houses; i++)
    if (g->house_control[i].vnum == vnum)
      return (i);

  return (NOWHERE);
}

/* Save the house control information */
void House_save_control(GameWorld *g)
{
  struct atomic_file af;
  FILE *fl;

  if (!(fl = atomic_open(&af, HCONTROL_FILE, true))) {
    perror("SYSERR: Unable to open house control file.");
    return;
  }
  /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
  fwrite(g->house_control, sizeof(struct house_control_rec), g->num_of_houses, fl);

  atomic_commit(&af);
}

/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(GameWorld *g)
{
  struct house_control_rec temp_house;
  room_vnum real_house;
  FILE *fl;

  memset((char *)g->house_control, 0, sizeof(struct house_control_rec) * MAX_HOUSES);

  if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
    if (errno == ENOENT)
      mud_log("   No houses to load. File '%s' does not exist.", HCONTROL_FILE);
    else
      perror("SYSERR: " HCONTROL_FILE);
    return;
  }
  while (!feof(fl) && g->num_of_houses < MAX_HOUSES) {
    fread(&temp_house, sizeof(struct house_control_rec), 1, fl);

    if (feof(fl))
      break;

    if (get_name_by_id(g, temp_house.owner) == nullptr)
      continue; /* owner no longer exists -- skip */

    if (!room_of(g, real_house = temp_house.vnum))
      continue; /* this vnum doesn't exist -- skip */

    if (find_house(g, temp_house.vnum) != NOWHERE)
      continue; /* this vnum is already a house -- skip */

    g->house_control[g->num_of_houses++] = temp_house;

    room_set_flag(g, real_house, ROOM_HOUSE, true);
    House_load(g, temp_house.vnum);
  }

  fclose(fl);
  House_save_control(g);
}

/* "House Control" functions */

static const char *const HCONTROL_FORMAT =
    "Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
    "       hcontrol destroy <house vnum>\r\n"
    "       hcontrol pay <house vnum>\r\n"
    "       hcontrol show\r\n";

void hcontrol_list_houses(GameWorld *g, struct char_data *ch)
{
  int i;
  char *timestr, *temp;
  char built_on[128], last_pay[128], own_name[MAX_NAME_LENGTH + 1];

  if (!g->num_of_houses) {
    send_to_char(g, ch, "No houses have been defined.\r\n");
    return;
  }
  send_to_char(g, ch,
               "Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n"
               "-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < g->num_of_houses; i++) {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(g, g->house_control[i].owner)) == nullptr)
      continue;

    if (g->house_control[i].built_on) {
      timestr         = asctime(localtime(&(g->house_control[i].built_on)));
      *(timestr + 10) = '\0';
      strlcpy(built_on, timestr, sizeof(built_on));
    } else
      strcpy(built_on, "Unknown"); /* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (g->house_control[i].last_payment) {
      timestr         = asctime(localtime(&(g->house_control[i].last_payment)));
      *(timestr + 10) = '\0';
      strlcpy(last_pay, timestr, sizeof(last_pay));
    } else
      strcpy(last_pay, "None"); /* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strcpy(own_name, temp); /* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    send_to_char(g, ch, "%7d %-10s    %2d    %-12s %s\r\n",
                 g->house_control[i].vnum, built_on,
                 g->house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(g, ch, i, true);
  }
}

static void hcontrol_build_house(GameWorld *g, struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH];
  struct house_control_rec temp_house;
  room_vnum virt_house;
  room_vnum real_house;
  int16_t exit_num;
  long owner;

  if (g->num_of_houses >= MAX_HOUSES) {
    send_to_char(g, ch, "Max houses already defined.\r\n");
    return;
  }

  /* first arg: house's vnum */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(g, ch, "%s", HCONTROL_FORMAT);
    return;
  }
  virt_house = atoi(arg1);
  if (!room_of(g, real_house = virt_house)) {
    send_to_char(g, ch, "No such room exists.\r\n");
    return;
  }
  if ((find_house(g, virt_house)) != NOWHERE) {
    send_to_char(g, ch, "House already exists.\r\n");
    return;
  }

  /* second arg: direction of house's exit */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(g, ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((exit_num = search_block(arg1, dirs, false)) < 0 &&
      (exit_num = search_block(arg1, abbr_dirs, false)) < 0) {
    send_to_char(g, ch, "'%s' is not a valid direction.\r\n", arg1);
    return;
  }
  if (room_get_exit_to_room(g, real_house, exit_num) == NOWHERE) {
    send_to_char(g, ch, "There is no exit %s from room %d.\r\n", dirs[exit_num], virt_house);
    return;
  }

  /* third arg: player's name */
  one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(g, ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((owner = get_id_by_name(g, arg1)) < 0) {
    send_to_char(g, ch, "Unknown player '%s'.\r\n", arg1);
    return;
  }

  temp_house.mode          = HOUSE_PRIVATE;
  temp_house.vnum          = virt_house;
  temp_house.exit_num      = exit_num;
  temp_house.built_on      = game_now(g);
  temp_house.last_payment  = 0;
  temp_house.owner         = owner;
  temp_house.num_of_guests = 0;

  g->house_control[g->num_of_houses++] = temp_house;

  room_set_flag(g, real_house, ROOM_HOUSE, true);
  House_crashsave(g, virt_house);

  send_to_char(g, ch, "House built.  Mazel tov!\r\n");
  House_save_control(g);
}

static void hcontrol_destroy_house(GameWorld *g, struct char_data *ch, char *arg)
{
  int i, j;
  room_vnum real_atrium, real_house;

  if (!*arg) {
    send_to_char(g, ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((i = find_house(g, atoi(arg))) == NOWHERE) {
    send_to_char(g, ch, "Unknown house.\r\n");
    return;
  }
  if (!room_of(g, real_atrium = g->house_control[i].atrium))
    mud_log("SYSERR: House %d had invalid atrium %d!", atoi(arg), g->house_control[i].atrium);
  else
    room_set_flag(g, real_atrium, ROOM_ATRIUM, false);

  if (!room_of(g, real_house = g->house_control[i].vnum))
    mud_log("SYSERR: House %d had invalid vnum %d!", atoi(arg), g->house_control[i].vnum);
  else {
    room_set_flag(g, real_house, ROOM_HOUSE, false);
    room_set_flag(g, real_house, ROOM_HOUSE_CRASH, false);
  }

  House_delete_file(g->house_control[i].vnum);

  for (j = i; j < g->num_of_houses - 1; j++)
    g->house_control[j] = g->house_control[j + 1];

  g->num_of_houses--;

  send_to_char(g, ch, "House deleted.\r\n");
  House_save_control(g);

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (i = 0; i < g->num_of_houses; i++)
    if (room_of(g, real_atrium = g->house_control[i].atrium))
      room_set_flag(g, real_atrium, ROOM_ATRIUM, true);
}

static void hcontrol_pay_house(GameWorld *g, struct char_data *ch, char *arg)
{
  int i;

  if (!*arg)
    send_to_char(g, ch, "%s", HCONTROL_FORMAT);
  else if ((i = find_house(g, atoi(arg))) == NOWHERE)
    send_to_char(g, ch, "Unknown house.\r\n");
  else {
    mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "Payment for house %s collected by %s.", arg, char_get_name(ch));

    g->house_control[i].last_payment = game_now(g);
    House_save_control(g);
    send_to_char(g, ch, "Payment recorded.\r\n");
  }
}

/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "build"))
    hcontrol_build_house(g, ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(g, ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(g, ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(g, ch);
  else
    send_to_char(g, ch, "%s", HCONTROL_FORMAT);
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  char arg[MAX_INPUT_LENGTH];
  int i, j, id;

  one_argument(argument, arg);

  if (!room_flagged(g, char_get_in_room(ch), ROOM_HOUSE))
    send_to_char(g, ch, "You must be in your house to set guests.\r\n");
  else if ((i = find_house(g, char_get_in_room(ch))) == NOWHERE)
    send_to_char(g, ch, "Um.. this house seems to be screwed up.\r\n");
  else if (char_get_idnum(ch) != g->house_control[i].owner)
    send_to_char(g, ch, "Only the primary owner can set guests.\r\n");
  else if (!*arg)
    House_list_guests(g, ch, i, false);
  else if ((id = get_id_by_name(g, arg)) < 0)
    send_to_char(g, ch, "No such player.\r\n");
  else if (id == char_get_idnum(ch))
    send_to_char(g, ch, "It's your house!\r\n");
  else {
    for (j = 0; j < g->house_control[i].num_of_guests; j++)
      if (g->house_control[i].guests[j] == id) {
        for (; j < g->house_control[i].num_of_guests; j++)
          g->house_control[i].guests[j] = g->house_control[i].guests[j + 1];
        g->house_control[i].num_of_guests--;
        House_save_control(g);
        send_to_char(g, ch, "Guest deleted.\r\n");
        return;
      }
    if (g->house_control[i].num_of_guests == MAX_GUESTS) {
      send_to_char(g, ch, "You have too many guests.\r\n");
      return;
    }
    j                          = g->house_control[i].num_of_guests++;
    g->house_control[i].guests[j] = id;
    House_save_control(g);
    send_to_char(g, ch, "Guest added.\r\n");
  }
}

/* Misc. administrative functions */

/* crash-save all the houses */
void House_save_all(GameWorld *g)
{
  int i;
  room_vnum real_house;

  for (i = 0; i < g->num_of_houses; i++)
    if (room_of(g, real_house = g->house_control[i].vnum))
      House_crashsave(g, g->house_control[i].vnum);
}

/* note: arg passed must be house vnum, so there. */
bool House_can_enter(GameWorld *g, struct char_data *ch, room_vnum house)
{
  int i, j;

  if (char_adm_flagged(ch, ADM_ALLHOUSES) || (i = find_house(g, house)) == NOWHERE)
    return true;

  switch (g->house_control[i].mode) {
    case HOUSE_CLAN:
    case HOUSE_UNOWNED:
      return true;
    case HOUSE_PRIVATE:
      if (char_get_idnum(ch) == g->house_control[i].owner)
        return true;
      for (j = 0; j < g->house_control[i].num_of_guests; j++)
        if (char_get_idnum(ch) == g->house_control[i].guests[j])
          return true;
  }

  return false;
}

static void House_list_guests(GameWorld *g, struct char_data *ch, int i, int quiet)
{
  int j, num_printed;
  char *temp;

  if (g->house_control[i].num_of_guests == 0) {
    if (!quiet)
      send_to_char(g, ch, "  Guests: None\r\n");
    return;
  }

  send_to_char(g, ch, "  Guests: ");

  for (num_printed = j = 0; j < g->house_control[i].num_of_guests; j++) {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(g, g->house_control[i].guests[j])) == nullptr)
      continue;

    num_printed++;
    send_to_char(g, ch, "%c%s ", toupper(*temp), temp + 1);
  }

  if (num_printed == 0)
    send_to_char(g, ch, "all dead");

  send_to_char(g, ch, "\r\n");
}

static bool House_load(GameWorld *g, room_vnum rvnum)
{
  FILE *fl;
  char f1[READ_SIZE] = "", f2[READ_SIZE] = "", f3[READ_SIZE] = "", f4[READ_SIZE] = "";
  char cmfname[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char line[256] = "";
  int t[21] = {}, zwei = 0; /* a damaged line leaves some of these unread */
  struct obj_data *temp;
  int locate = 0, j;
  obj_vnum nr;
  std::vector<struct obj_data *> cont_row[MAX_BAG_ROWS]; /* each list in file order */
  room_vnum rrnum;

  if (!room_of(g, rrnum = rvnum))
    return false;

  if (!House_get_filename(rvnum, cmfname, sizeof(cmfname)))
    return false;

  if (!(fl = fopen(cmfname, "rb"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      snprintf(buf1, sizeof(buf1), "SYSERR: READING HOUSE FILE %s (5)", cmfname);
      perror(buf1);
    }
    return false;
  }

  for (j = 0; j < MAX_BAG_ROWS; j++)
    cont_row[j].clear(); /* empty all cont lists (you never know ...) */

  if (!feof(fl))
    get_line(fl, line);
  while (!feof(fl)) {
    temp = nullptr;
    /* first, we get the number. Not too hard. */
    if (*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        get_line(fl, line); /* skip it, or the loop never moves on */
        continue;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) { /* then it is unique */
        temp              = create_obj(g);
        obj_set_item_number(temp, NOTHING);
      } else if (nr < 0 || nr >= 999999) {
        get_line(fl, line); /* skip it, or the loop never moves on */
        continue;
      } else {
        temp = read_object(g, nr);
        if (!temp) {
          get_line(fl, line);
          continue;
        }
      }

      get_line(fl, line);
      sscanf(line, "%d %d %d %d %d %d %d %d %d %s %s %s %s %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, f1, f2, f3, f4, t + 13, t + 14, t + 15, t + 16, t + 17, t + 18, t + 19, t + 20);
      locate = t[0];
      obj_set_value(temp, 0, t[1]);
      obj_set_value(temp, 1, t[2]);
      obj_set_value(temp, 2, t[3]);
      obj_set_value(temp, 3, t[4]);
      obj_set_value(temp, 4, t[5]);
      obj_set_value(temp, 5, t[6]);
      obj_set_value(temp, 6, t[7]);
      obj_set_value(temp, 7, t[8]);
      obj_get_extra_flags(temp)[0] = asciiflag_conv(f1);
      obj_get_extra_flags(temp)[1] = asciiflag_conv(f2);
      obj_get_extra_flags(temp)[2] = asciiflag_conv(f3);
      obj_get_extra_flags(temp)[3] = asciiflag_conv(f3);
      obj_set_value(temp, 8, t[13]);
      obj_set_value(temp, 9, t[14]);
      obj_set_value(temp, 10, t[15]);
      obj_set_value(temp, 11, t[16]);
      obj_set_value(temp, 12, t[17]);
      obj_set_value(temp, 13, t[18]);
      obj_set_value(temp, 14, t[19]);
      obj_set_value(temp, 15, t[20]);
      obj_set_posted_to(temp, nullptr);
      obj_set_posttype(temp, 0);

      get_line(fl, line);
      /* read line check for xap. */
      if (!strcmp("XAP", line)) { /* then this is a Xap Obj, requires
                                       special care */
        obj_set_name(temp, take_string(fread_string(fl, buf2)));
        if (obj_get_name(temp) == nullptr) {
          obj_set_name(temp, "undefined");
        }

        obj_set_short_description(temp, take_string(fread_string(fl, buf2)));
        if (obj_get_short_description(temp) == nullptr) {
          obj_set_short_description(temp, "undefined");
        }

        obj_set_description(temp, take_string(fread_string(fl, buf2)));
        if (obj_get_description(temp) == nullptr) {
          obj_set_description(temp, "undefined");
        }

        obj_set_action_description(temp, take_string(fread_string(fl, buf2)));
        if (obj_get_action_description(temp) == nullptr) {
          obj_set_action_description(temp, nullptr);
        }

        if (!get_line(fl, line) ||
            (sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7) != 8)) {
          fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
          return false;
        }
        obj_set_type_flag(temp, t[0]);
        obj_get_wear_flags(temp)[0] = t[1];
        obj_get_wear_flags(temp)[1] = t[2];
        obj_get_wear_flags(temp)[2] = t[3];
        obj_get_wear_flags(temp)[3] = t[4];
        obj_set_weight(temp, t[5]);
        obj_set_cost(temp, t[6]);
        obj_set_cost_per_day(temp, t[7]);

        /* buf2 is error codes pretty much */
        //strcat(buf2, ", after numeric constants (expecting E/#xxx)");

        /*add_unique_id(temp);*/
        /* we're clearing these for good luck */

        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          obj_get_affected(temp)[j].location = APPLY_NONE;
          obj_get_affected(temp)[j].modifier = 0;
          obj_get_affected(temp)[j].specific = 0;
        }

        obj_ex_description_slot(temp)->clear();

        get_line(fl, line);
        for (j = zwei = 0; !zwei && !feof(fl);) {
          switch (*line) {
            case 'E': {
              extra_descr_data descr;
              descr.keyword     = take_string(fread_string(fl, buf2));
              descr.description = take_string(fread_string(fl, buf2));
              obj_ex_description_slot(temp)->push_front(std::move(descr));
              get_line(fl, line);
              break;
            }
            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                mud_log("SYSERR: Too many object affectations in loading house file");
              }
              get_line(fl, line);
              sscanf(line, "%d %d %d", t, t + 1, t + 2);
              if (j < MAX_OBJ_AFFECT) {
                obj_get_affected(temp)[j].location = t[0];
                obj_get_affected(temp)[j].modifier = t[1];
                obj_get_affected(temp)[j].specific = affect_checked_specific(t[0], t[2]);
                j++;
              }
              get_line(fl, line);
              break;

            case 'G': {
              long long generation;

              get_line(fl, line);
              if (sscanf(line, "%lld", &generation) == 1)
                obj_set_generation(temp, generation);
              get_line(fl, line);
              break;
            }
            case 'U':
              get_line(fl, line);
              {
                int64_t uid = obj_get_unique_id(temp);
                sscanf(line, "%" SCNd64, &uid);
                /* -1 is an object not yet given one; no real one is negative. */
                if (uid < -1) {
                  mud_log("SYSERR: Object #%d has unique id %" PRId64 ", which can't be; giving it a new one.",
                          obj_get_vnum(g, temp), uid);
                  uid = -1;
                }
                obj_set_unique_id(temp, uid);
              }
              get_line(fl, line);
              break;
            case 'S':
              if (j >= SPELLBOOK_SIZE) {
                mud_log("SYSERR: Too many spells in spellbook loading rent file");
              }
              get_line(fl, line);
              sscanf(line, "%d %d", t, t + 1);

              if (!obj_get_sbinfo(temp)) {
                *obj_sbinfo_slot(temp) = new obj_spellbook_spell[SPELLBOOK_SIZE]();
              }
              obj_get_sbinfo(temp)[j].spellname = t[0];
              obj_get_sbinfo(temp)[j].pages     = t[1];
              j++;
              get_line(fl, line);
              break;
            case 'Z':
              get_line(fl, line);
              {
                int size = obj_get_size(temp);
                sscanf(line, "%d", &size);
                obj_set_size(temp, size);
              }
              get_line(fl, line);
              break;
            case '$':
            case '#':
              zwei = 1;
              break;
            default:
              zwei = 1;
              break;
          }
        } /* exit our for loop */
      } /* exit our xap loop */
      if (temp != nullptr) {
        obj_to_room(g, temp, rrnum);
      } else {
        continue;
      }

      /*No need to check if its equipped since rooms can't equip things --firebird_223*/
      for (j = MAX_BAG_ROWS - 1; j > -locate; j--)
        if (!cont_row[j].empty()) { /* no container -> back to ch's inventory */
          for (struct obj_data *row_obj : cont_row[j])
            obj_to_room(g, row_obj, rrnum);
          cont_row[j].clear();
          cont_row[j].clear();
        }

      if (j == -locate && !cont_row[j].empty()) { /* content list existing */
        if (obj_get_type_flag(temp) == ITEM_CONTAINER) {
          /* take item ; fill ; give to char again */
          obj_from_room(g, temp);
          obj_contains_slot(temp)->clear();
          for (struct obj_data *row_obj : cont_row[j])
            obj_to_obj(g, row_obj, temp);
          cont_row[j].clear();
          obj_to_room(g, temp, rrnum); /* add to inv first ... */
        } else {                    /* object isn't container -> empty content list */
          for (struct obj_data *row_obj : cont_row[j])
            obj_to_room(g, row_obj, rrnum);
          cont_row[j].clear();
          cont_row[j].clear();
        }
      }

      if (locate < 0 && locate >= -MAX_BAG_ROWS) {
        /* let obj be part of content list
                  but put it at the list's end thus having the items
                  in the same order as before renting */
        obj_from_room(g, temp);
        cont_row[-locate - 1].push_back(temp);
      }
    } else {
      get_line(fl, line);
    }
  }

  fclose(fl);

  return true;
}
