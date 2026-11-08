/**************************************************************************
*  File: dg_wldcmd.c                                                      *
*  Usage: contains the command_interpreter for rooms,                     *
*         room commands.                                                  *
*                                                                         *
*                                                                         *
*  $Author: galion/Mark A. Heilpern/egreen/Welcor $                       *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/*
 * Local functions
 */

#define WCMD(name) \
  void(name)(GameWorld * g, room_data * room, char *argument, int cmd, int subcmd)

static void wld_log(GameWorld *g, room_data *room, const char *format, ...) __attribute__((format(printf, 3, 4)));
static void act_to_room(GameWorld *g, char *str, room_data *room);
static WCMD(do_wasound);
static WCMD(do_wecho);
static WCMD(do_wsend);
static WCMD(do_wzoneecho);
static WCMD(do_wrecho);
static WCMD(do_wdoor);
static WCMD(do_wteleport);
static WCMD(do_wforce);
static WCMD(do_wpurge);
static WCMD(do_wload);
static WCMD(do_wdamage);
static WCMD(do_wat);
static WCMD(do_weffect);

struct wld_command_info {
  const char *command{};
  void (*command_pointer)(GameWorld *g, room_data *room, char *argument, int cmd, int subcmd){};
  int subcmd{};
};

/* do_wsend */
constexpr int SCMD_WSEND       = 0;
constexpr int SCMD_WECHOAROUND = 1;

/* attaches room vnum to msg and sends it to script_log */
static void wld_log(GameWorld *g, room_data *room, const char *format, ...)
{
  va_list args;
  char output[MAX_STRING_LENGTH];

  va_start(args, format);
  vsnprintf(output, sizeof(output), format, args);
  va_end(args);

  script_log(g, "Room %d :: %s", room_get_number(room), output);
}

/* sends str to room */
static void act_to_room(GameWorld *g, char *str, room_data *room)
{
  /* no one is in the room */
  if (!room_get_people(room))
    return;

  /*
     * since you can't use act(..., TO_ROOM) for an room, send it
     * TO_ROOM and TO_CHAR for some char in the room.
     * (just dont use $n or you might get strange results)
     */
  act(g, str, false, first_char(g, room_get_people(room)), nullptr, nullptr, TO_ROOM);
  act(g, str, false, first_char(g, room_get_people(room)), nullptr, nullptr, TO_CHAR);
}

/* World commands */

/* changes gravity, light, or lava levels of a room. */
static WCMD(do_weffect)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int num = 0;
  room_vnum nr;
  room_vnum target;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    wld_log(g, room, "weffect called without type argument");
    return;
  }

  if (!*arg2) {
    wld_log(g, room, "weffect called without setting argument");
    return;
  }

  num = atoi(arg2);

  nr     = num;
  target = nr;

  if (!strcasecmp(arg, "gravity")) { /* Set gravity */
    if (num < 0 || num > 10000) {
      wld_log(g, room, "weffect setting out of bounds, 0 - 10000 only.");
      return;
    }
    room_set_gravity(g, room_get_number(room), num);
  } else if (!strcasecmp(arg, "light")) {
    if (!room_of(g, target)) {
      wld_log(g, room, "weffect target is NOWHERE.");
      return;
    }
    if (!room_flagged(g, target, ROOM_INDOORS)) {
      room_set_flag(g, target, ROOM_INDOORS, true);
    } else {
      room_set_flag(g, target, ROOM_INDOORS, false);
    }
  } else if (!strcasecmp(arg, "lava")) {
    if (!room_of(g, target)) {
      wld_log(g, room, "weffect target is NOWHERE.");
      return;
    }
    if (room_get_geffect(g, target) != 0) {
      room_set_geffect(g, target, 5);
    } else {
      wld_log(g, room, "weffect target already has lava.");
      return;
    }
  }
}

/* prints the argument to all the rooms aroud the room */
static WCMD(do_wasound)
{
  int door;

  skip_spaces(&argument);

  if (!*argument) {
    wld_log(g, room, "wasound called with no argument");
    return;
  }

  for (door = 0; door < NUM_OF_DIRS; door++) {
    struct room_direction_data *newexit;

    if ((newexit = room_get_dir_option(room, door)) && (newexit->to_room != NOWHERE) &&
        room != room_of(g, newexit->to_room))
      act_to_room(g, argument, room_of(g, newexit->to_room));
  }
}

static WCMD(do_wecho)
{
  skip_spaces(&argument);

  if (!*argument)
    wld_log(g, room, "wecho called with no args");

  else
    act_to_room(g, argument, room);
}

static WCMD(do_wsend)
{
  char buf[MAX_INPUT_LENGTH], *msg;
  char_data *ch;

  msg = any_one_arg(argument, buf);

  if (!*buf) {
    wld_log(g, room, "wsend called with no args");
    return;
  }

  skip_spaces(&msg);

  if (!*msg) {
    wld_log(g, room, "wsend called without a message");
    return;
  }

  if ((ch = get_char_by_room(g, room, buf))) {
    if (subcmd == SCMD_WSEND)
      sub_write(g, msg, ch, true, TO_CHAR);
    else if (subcmd == SCMD_WECHOAROUND)
      sub_write(g, msg, ch, true, TO_ROOM);
  }

  else
    wld_log(g, room, "no target found for wsend");
}

static WCMD(do_wzoneecho)
{
  struct zone_data *zone;
  char room_num[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

  msg = any_one_arg(argument, room_num);
  skip_spaces(&msg);

  if (!*room_num || !*msg)
    wld_log(g, room, "wzoneecho called with too few args");

  else if ((zone = find_zone_by_thing(g, atoi(room_num))) == nullptr)
    wld_log(g, room, "wzoneecho called for nonexistant zone");

  else {
    sprintf(buf, "%s\r\n", msg);
    send_to_zone(g, buf, zone);
  }
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
static WCMD(do_wrecho)
{
  char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

  msg = two_arguments(argument, start, finish);

  skip_spaces(&msg);

  if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
    wld_log(g, room, "wrecho: too few args");
  else
    send_to_range(g, atoi(start), atoi(finish), "%s\r\n", msg);
}

static WCMD(do_wdoor)
{
  char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], *value;
  room_data *rm;
  int dir, fd;

  argument = two_arguments(argument, target, direction);
  value    = one_argument(argument, field);
  skip_spaces(&value);

  if (!*target || !*direction || !*field) {
    wld_log(g, room, "wdoor called with too few args");
    return;
  }
  if ((rm = get_room(g, target)) == nullptr) {
    wld_log(g, room, "wdoor: invalid target");
    return;
  }
  if (atoi(direction) >= 0 && atoi(direction) <= 11) {
    dir = atoi(direction);
  } else {
    wld_log(g, room, "wdoor: invalid direction");
    return;
  }

  if ((fd = search_block(field, dg_door_fields, false)) == -1) {
    wld_log(g, room, "wdoor: invalid field");
    return;
  }

  if (!dg_door_apply(g, rm, dir, fd, value))
    wld_log(g, room, "wdoor: invalid door target");
}

static WCMD(do_wteleport)
{
  char_data *ch;
  room_vnum target, nr;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    wld_log(g, room, "wteleport called with too few args");
    return;
  }

  nr     = atoi(arg2);
  target = nr;

  if (!room_of(g, target))
    wld_log(g, room, "wteleport target is an invalid room");

  else if (!strcasecmp(arg1, "all")) {
    if (nr == room_get_number(room)) {
      wld_log(g, room, "wteleport all target is itself");
      return;
    }

    for (char_data *tch : room_people(g, room)) {
      if (!valid_dg_target(g, tch, DG_ALLOW_GODS))
        continue;
      char_from_room(g, tch);
      char_to_room(g, tch, target);
      enter_wtrigger(g, room_of(g, char_get_in_room(tch)), tch, -1);
    }
  }

  else {
    if ((ch = get_char_by_room(g, room, arg1))) {
      if (valid_dg_target(g, ch, DG_ALLOW_GODS)) {
        char_from_room(g, ch);
        char_to_room(g, ch, target);
        enter_wtrigger(g, room_of(g, char_get_in_room(ch)), ch, -1);
      }
    }

    else
      wld_log(g, room, "wteleport: no target found");
  }
}

static WCMD(do_wforce)
{
  char_data *ch;
  char arg1[MAX_INPUT_LENGTH], *line;

  line = one_argument(argument, arg1);

  if (!*arg1 || !*line) {
    wld_log(g, room, "wforce called with too few args");
    return;
  }

  if (!strcasecmp(arg1, "all")) {
    for (char_data *tch : room_people(g, room)) {

      if (valid_dg_target(g, tch, 0)) {
        command_interpreter(g, tch, line);
      }
    }
  }

  else {
    if ((ch = get_char_by_room(g, room, arg1))) {
      if (valid_dg_target(g, ch, 0)) {
        command_interpreter(g, ch, line);
      }
    }

    else
      wld_log(g, room, "wforce: no target found");
  }
}

/* purge all objects an npcs in room, or specified object or mob */
static WCMD(do_wpurge)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *ch;
  obj_data *obj;

  one_argument(argument, arg);

  if (!*arg) {
    /* purge all */
    for (char_data *tch : room_people(g, room)) {
      if (char_is_npc(tch))
        extract_char(g, tch);
    }

    for (obj_data *item : obj_list(g, room_get_contents(room))) {
      extract_obj(g, item);
    }

    return;
  }

  if (*arg == UID_CHAR)
    ch = get_char(g, arg);
  else
    ch = get_char_in_room(g, room, arg);

  if (!ch) {
    if (*arg == UID_CHAR)
      obj = get_obj(g, arg);
    else
      obj = get_obj_in_room(g, room, arg);

    if (obj) {
      extract_obj(g, obj);
    } else
      wld_log(g, room, "wpurge: bad argument");

    return;
  }

  if (!char_is_npc(ch)) {
    wld_log(g, room, "wpurge: purging a PC");
    return;
  }

  extract_char(g, ch);
}

/* loads a mobile or object into the room */
static WCMD(do_wload)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  IDXTYPE number = 0;
  char_data *mob;
  obj_data *object;
  char *target;
  char_data *tch;
  obj_data *cnt;
  int pos;

  target = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
    wld_log(g, room, "wload: bad syntax");
    return;
  }

  /* load mob to target room - Jamie Nelson, April 13 2004 */
  if (is_abbrev(arg1, "mob")) {
    room_vnum rnum;
    if (!target || !*target) {
      rnum = room_get_number(room);
    } else {
      if (!isdigit(*target) || !room_of(g, rnum = atoi(target))) {
        wld_log(g, room, "wload: room target vnum doesn't exist (loading mob vnum %d to room %s)", number, target);
        return;
      }
    }
    if ((mob = read_mobile(g, number)) == nullptr) {
      wld_log(g, room, "mload: bad mob vnum");
      return;
    }
    char_to_room(g, mob, rnum);
    if (room_get_script(room)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%c%d", UID_CHAR, char_get_id(mob));
      add_var(&(room_get_script(room)->global_vars), "lastloaded", buf, 0);
    }
    load_mtrigger(g, mob);
  }

  else if (is_abbrev(arg1, "obj")) {
    if ((object = read_object(g, number)) == nullptr) {
      wld_log(g, room, "wload: bad object vnum");
      return;
    }
    /* special handling to make objects able to load on a person/in a container/worn etc. */
    if (!target || !*target) {
      add_unique_id(g, object);
      obj_to_room(g, object, room_get_number(room));
      if (room_get_script(room)) { /* It _should_ have, but it might be detached. */
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%c%d", UID_CHAR, obj_get_id(object));
        add_var(&(room_get_script(room)->global_vars), "lastloaded", buf, 0);
      }
      load_otrigger(g, object);
      return;
    }

    two_arguments(target, arg1, arg2); /* recycling ... */
    tch = get_char_in_room(g, room, arg1);
    if (tch) {
      if (*arg2 &&
          (pos = find_eq_pos_script(arg2)) >= 0 &&
          !char_get_equipment(tch, pos) &&
          can_wear_on_pos(object, pos)) {
        equip_char(g, tch, object, pos);
        load_otrigger(g, object);
        return;
      }
      obj_to_char(g, object, tch);
      load_otrigger(g, object);
      return;
    }
    cnt = get_obj_in_room(g, room, arg1);
    if (cnt && obj_get_type_flag(cnt) == ITEM_CONTAINER) {
      obj_to_obj(g, object, cnt);
      load_otrigger(g, object);
      return;
    }
    /* neither char nor container found - just dump it in room */
    add_unique_id(g, object);
    obj_to_room(g, object, room_get_number(room));
    load_otrigger(g, object);
    return;
  }

  else
    wld_log(g, room, "wload: bad type");
}

static WCMD(do_wdamage)
{
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *ch;

  two_arguments(argument, name, amount);

  /* who cares if it's a number ? if not it'll just be 0 */
  if (!*name || !*amount) {
    wld_log(g, room, "wdamage: bad syntax");
    return;
  }

  dam = atoi(amount);
  ch  = get_char_by_room(g, room, name);

  if (!ch) {
    wld_log(g, room, "wdamage: target not found");
    return;
  }

  script_damage(g, ch, dam);
}

static WCMD(do_wat)
{
  room_vnum loc = NOWHERE;
  struct char_data *ch;
  char arg[MAX_INPUT_LENGTH], *command;

  command = any_one_arg(argument, arg);

  if (!*arg) {
    wld_log(g, room, "wat called with no args");
    return;
  }

  skip_spaces(&command);

  if (!*command) {
    wld_log(g, room, "wat called without a command");
    return;
  }

  if (isdigit(*arg)) {
    loc = atoi(arg);
  } else if ((ch = get_char_by_room(g, room, arg))) {
    loc = char_get_in_room(ch);
  }

  if (!room_of(g, loc)) {
    wld_log(g, room, "wat: location not found (%s)", arg);
    return;
  }

  wld_command_interpreter(g, room_of(g, loc), command);
}

static const struct wld_command_info wld_cmd_info[] = {
    {"RESERVED",     nullptr,      0               }, /* this must be first -- for specprocs */

    {"wasound ",     do_wasound,   0               },
    {"wdoor ",       do_wdoor,     0               },
    {"wecho ",       do_wecho,     0               },
    {"wechoaround ", do_wsend,     SCMD_WECHOAROUND},
    {"wforce ",      do_wforce,    0               },
    {"wload ",       do_wload,     0               },
    {"wpurge ",      do_wpurge,    0               },
    {"wrecho ",      do_wrecho,    0               },
    {"wsend ",       do_wsend,     SCMD_WSEND      },
    {"wteleport ",   do_wteleport, 0               },
    {"wzoneecho ",   do_wzoneecho, 0               },
    {"wdamage ",     do_wdamage,   0               },
    {"wat ",         do_wat,       0               },
    {"weffect ",     do_weffect,   0               },
    {"\n",           nullptr,      0               }  /* this must be last */
};

/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void wld_command_interpreter(GameWorld *g, room_data *room, char *argument)
{
  int cmd, length;
  char *line, arg[MAX_INPUT_LENGTH];

  skip_spaces(&argument);

  /* just drop to next line for hitting CR */
  if (!*argument)
    return;

  line = any_one_arg(argument, arg);

  /* find the command */
  for (length = strlen(arg), cmd = 0;
       *wld_cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(wld_cmd_info[cmd].command, arg, length))
      break;

  if (*wld_cmd_info[cmd].command == '\n')
    wld_log(g, room, "Unknown world cmd: '%s'", argument);
  else
    ((*wld_cmd_info[cmd].command_pointer)(g, room, line, cmd, wld_cmd_info[cmd].subcmd));
}
