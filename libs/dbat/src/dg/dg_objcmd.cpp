/**************************************************************************
*  File: dg_objcmd.c                                                      *
*  Usage: contains the command_interpreter for objects,                   *
*         object commands.                                                *
*                                                                         *
*                                                                         *
*  $Author: galion/Mark A. Heilpern/egreen/Welcor $                       *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/screen.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/extract.hpp"
#include "dbat/relocate.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/game_world.hpp"

/*
 * Local functions
 */
#define OCMD(name) \
  void(name)(GameWorld * g, obj_data * obj, char *argument, int cmd, int subcmd)

static void obj_log_printf(GameWorld *g, obj_data *obj, const char *format, ...) __attribute__((format(printf, 3, 4)));
static room_vnum find_obj_target_room(GameWorld *g, obj_data *obj, std::string_view rawroomstr);
static OCMD(do_oecho);
static OCMD(do_oforce);
static OCMD(do_ozoneecho);
static OCMD(do_osend);
static OCMD(do_orecho);
static OCMD(do_otimer);
static OCMD(do_otransform);
static OCMD(do_opurge);
static OCMD(do_dupe);
static OCMD(do_oteleport);
static OCMD(do_dgoload);
static OCMD(do_odamage);
static OCMD(do_oasound);
static OCMD(do_ogoto);
static OCMD(do_odoor);
static OCMD(do_osetval);
static OCMD(do_oat);

struct obj_command_info {
  const char *command{};
  void (*command_pointer)(GameWorld *g, obj_data *obj, char *argument, int cmd, int subcmd){};
  int subcmd{};
};

/* do_osend */
constexpr int SCMD_OSEND       = 0;
constexpr int SCMD_OECHOAROUND = 1;

/* attaches object name and vnum to msg and sends it to script_log */
static void obj_log_printf(GameWorld *g, obj_data *obj, const char *format, ...)
{
  va_list args;
  char output[MAX_STRING_LENGTH];

  va_start(args, format);
  vsnprintf(output, sizeof(output), format, args);
  va_end(args);

  script_log_printf(g, "Obj (%s, VNum %d):: %s", obj_get_short_description(obj), obj_get_vnum(g, obj), output);
}

/* returns the real room number that the object or object's carrier is in */
room_vnum obj_room(obj_data *obj)
{
  if (obj_get_in_room(obj) != NOWHERE)
    return obj_get_in_room(obj);
  else if (obj_get_carried_by(obj))
    return char_get_in_room(obj_get_carried_by(obj));
  else if (obj_get_worn_by(obj))
    return char_get_in_room(obj_get_worn_by(obj));
  else if (obj_get_in_obj(obj))
    return obj_room(obj_get_in_obj(obj));
  else
    return NOWHERE;
}

/* returns the real room number, or NOWHERE if not found or invalid */
static room_vnum find_obj_target_room(GameWorld *g, obj_data *obj, std::string_view rawroomstr)
{
  room_vnum tmp;
  room_vnum location;
  char_data *target_mob;
  obj_data *target_obj;
  std::string roomstr = take_arg(rawroomstr);

  if (roomstr.empty())
    return NOWHERE;

  if (isdigit(roomstr[0]) && roomstr.find('.') == std::string::npos) {
    tmp = parse_int(roomstr);
    if (!room_of(g, location = tmp))
      return NOWHERE;
  }

  else if ((target_mob = get_char_by_obj(g, obj, roomstr)))
    location = char_get_in_room(target_mob);
  else if ((target_obj = get_obj_by_obj(g, obj, roomstr))) {
    if (obj_get_in_room(target_obj) != NOWHERE)
      location = obj_get_in_room(target_obj);
    else
      return NOWHERE;
  } else
    return NOWHERE;

  /* a room has been found.  Check for permission */
  if (room_flagged(g, location, ROOM_GODROOM) ||
#ifdef ROOM_IMPROOM
      room_flagged(g, location, ROOM_IMPROOM) ||
#endif
      room_flagged(g, location, ROOM_PRIVATE))
    return NOWHERE;

  return location;
}

/* Object commands */

static OCMD(do_oecho)
{
  room_vnum room;

  skip_spaces(&argument);

  if (!*argument)
    obj_log_printf(g, obj, "oecho called with no args");

  else if ((room = obj_room(obj)) != NOWHERE) {
    if (room_get_people(g, room)) {
      sub_write(g, argument, first_char(g, room_get_people(g, room)), true, TO_ROOM);
      sub_write(g, argument, first_char(g, room_get_people(g, room)), true, TO_CHAR);
    }
  }

  else
    obj_log_printf(g, obj, "oecho called by object in NOWHERE");
}

static OCMD(do_oforce)
{
  char_data *ch;
  room_vnum room;
  char arg1[MAX_INPUT_LENGTH], *line;

  line = one_argument(argument, arg1);

  if (!*arg1 || !*line) {
    obj_log_printf(g, obj, "oforce called with too few args");
    return;
  }

  if (!strcasecmp(arg1, "all")) {
    if ((room = obj_room(obj)) == NOWHERE)
      obj_log_printf(g, obj, "oforce called by object in NOWHERE");
    else {
      for (char_data *tch : room_people(g, room)) {
        if (valid_dg_target(g, tch, 0)) {
          command_interpreter(g, tch, line);
        }
      }
    }
  }

  else {
    if ((ch = get_char_by_obj(g, obj, arg1))) {
      if (valid_dg_target(g, ch, 0)) {
        command_interpreter(g, ch, line);
      }
    }

    else
      obj_log_printf(g, obj, "oforce: no target found");
  }
}

static OCMD(do_ozoneecho)
{
  struct zone_data *zone;
  char room_number[MAX_INPUT_LENGTH], *msg;

  msg = any_one_arg(argument, room_number);
  skip_spaces(&msg);

  if (!*room_number || !*msg)
    obj_log_printf(g, obj, "ozoneecho called with too few args");

  else if ((zone = find_zone_by_thing(g, atoi(room_number))) == nullptr)
    obj_log_printf(g, obj, "ozoneecho called for nonexistant zone");

  else {
    send_to_zone(g, (std::string(msg) + "\r\n").c_str(), zone);
  }
}

static OCMD(do_osend)
{
  char buf[MAX_INPUT_LENGTH], *msg;
  char_data *ch;

  msg = any_one_arg(argument, buf);

  if (!*buf) {
    obj_log_printf(g, obj, "osend called with no args");
    return;
  }

  skip_spaces(&msg);

  if (!*msg) {
    obj_log_printf(g, obj, "osend called without a message");
    return;
  }

  if ((ch = get_char_by_obj(g, obj, buf))) {
    if (subcmd == SCMD_OSEND)
      sub_write(g, msg, ch, true, TO_CHAR);
    else if (subcmd == SCMD_OECHOAROUND) {
      std::string text = msg;

      if (const char *name = char_get_name(ch)) /* null for a mob with no short description */
        search_replace(text, name, "$n");
      act(g, text.c_str(), true, ch, nullptr, nullptr, TO_ROOM);
      /*sub_write(msg, ch, TRUE, TO_ROOM);*/
    }
  }

  else
    obj_log_printf(g, obj, "no target found for osend");
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
static OCMD(do_orecho)
{
  char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

  msg = two_arguments(argument, start, finish);

  skip_spaces(&msg);

  if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
    obj_log_printf(g, obj, "orecho: too few args");
  else
    send_to_range_printf(g, atoi(start), atoi(finish), "%s\r\n", msg);
}

/* set the object's timer value */
static OCMD(do_otimer)
{
  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty())
    obj_log_printf(g, obj, "otimer: missing argument");
  else if (!isdigit(arg[0]))
    obj_log_printf(g, obj, "otimer: bad argument");
  else
    obj_set_timer(obj, parse_int(arg));
}

/* transform into a different object */
/* note: this shouldn't be used with containers unless both objects */
/* are containers! */
static OCMD(do_otransform)
{
  std::string arg;
  obj_data *o, tmpobj;
  struct char_data *wearer = nullptr;
  int pos                  = 0;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty())
    obj_log_printf(g, obj, "otransform: missing argument");
  else if (!isdigit(arg[0]))
    obj_log_printf(g, obj, "otransform: bad argument");
  else {
    o = read_object(g, parse_int(arg));
    if (o == nullptr) {
      obj_log_printf(g, obj, "otransform: bad object vnum");
      return;
    }

    if (obj_get_worn_by(obj)) {
      pos    = obj_get_worn_on(obj);
      wearer = obj_get_worn_by(obj);
      unequip_char(g, obj_get_worn_by(obj), pos);
    }

    /* move new obj info over to old object and delete new obj */
    tmpobj = *o;
    obj_set_in_room(&tmpobj, obj_get_in_room(obj));
    obj_set_carried_by(&tmpobj, obj_get_carried_by(obj));
    obj_set_worn_by(&tmpobj, obj_get_worn_by(obj));
    obj_set_worn_on(&tmpobj, obj_get_worn_on(obj));
    obj_set_in_obj(&tmpobj, obj_get_in_obj(obj));
    *obj_contains_slot(&tmpobj) = obj_get_contains(obj);
    obj_set_id(&tmpobj, obj_get_id(obj));
    obj_set_proto_script(&tmpobj, obj_get_proto_script(obj));
    obj_set_script(&tmpobj, obj_get_script(obj));
    *obj = tmpobj;

    if (wearer) {
      equip_char(g, wearer, obj, pos);
    }

    extract_obj(g, o);
  }
}

static OCMD(do_dupe)
{

  obj_extra_set(obj, ITEM_DUPLICATE, true);
}

/* purge all objects an npcs in room, or specified object or mob */
static OCMD(do_opurge)
{
  std::string arg;
  char_data *ch;
  obj_data *o;
  room_vnum rm;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    /* purge all */
    if ((rm = obj_room(obj)) != NOWHERE) {
      for (char_data *tch : room_people(g, rm)) {
        if (char_is_npc(tch))
          extract_char(g, tch);
      }

      for (obj_data *item : room_contents(g, rm)) {
        if (item != obj)
          extract_obj(g, item);
      }
    }

    return;
  } /* no arg */

  ch = get_char_by_obj(g, obj, arg);
  if (!ch) {
    o = get_obj_by_obj(g, obj, arg);
    if (o) {
      if (o == obj)
        g->dg_owner_purged = 1;
      extract_obj(g, o);
    } else
      obj_log_printf(g, obj, "opurge: bad argument");

    return;
  }

  if (!char_is_npc(ch)) {
    obj_log_printf(g, obj, "opurge: purging a PC");
    return;
  }

  extract_char(g, ch);
}

/* (ogoto) Written by Iovan -- 4/5/10 
   Allows objects to move into specific rooms
   on their own.                             */
static OCMD(do_ogoto)
{
  room_vnum target;
  std::string arg1;

  std::string_view rest = argument;
  arg1                  = take_arg(rest);

  if (arg1.empty()) {
    obj_log_printf(g, obj, "ogoto called with too few args");
    return;
  }

  target = find_obj_target_room(g, obj, arg1);

  if (target == NOWHERE) {
    obj_log_printf(g, obj, "ogoto target is an invalid room");
  } else if (obj_get_in_room(obj) == NOWHERE) {
    obj_log_printf(g, obj, "ogoto tried to leave nowhere");
  } else {
    obj_from_room(g, obj);
    obj_to_room(g, obj, target);
  }
}

static OCMD(do_oteleport)
{
  char_data *ch;
  room_vnum target, rm;
  std::string arg1, arg2;

  std::string_view rest = argument;
  arg1                  = take_arg(rest);
  arg2                  = take_arg(rest);

  if (arg1.empty() || arg2.empty()) {
    obj_log_printf(g, obj, "oteleport called with too few args");
    return;
  }

  target = find_obj_target_room(g, obj, arg2);

  if (target == NOWHERE)
    obj_log_printf(g, obj, "oteleport target is an invalid room");

  else if (str_eq_nocase(arg1, "all")) {
    rm = obj_room(obj);
    if (target == rm)
      obj_log_printf(g, obj, "oteleport target is itself");

    for (char_data *tch : room_people(g, rm)) {
      if (!valid_dg_target(g, tch, DG_ALLOW_GODS))
        continue;
      char_from_room(g, tch);
      char_to_room(g, tch, target);
      enter_wtrigger(g, room_of(g, char_get_in_room(tch)), tch, -1);
    }
  }

  else {
    if ((ch = get_char_by_obj(g, obj, arg1))) {
      if (valid_dg_target(g, ch, DG_ALLOW_GODS)) {
        char_from_room(g, ch);
        char_to_room(g, ch, target);
        enter_wtrigger(g, room_of(g, char_get_in_room(ch)), ch, -1);
      }
    }

    else
      obj_log_printf(g, obj, "oteleport: no target found");
  }
}

static OCMD(do_dgoload)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  IDXTYPE number = 0;
  room_vnum room;
  char_data *mob;
  obj_data *object;
  char *target;
  char_data *tch;
  obj_data *cnt;
  int pos;

  target = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
    obj_log_printf(g, obj, "oload: bad syntax");
    return;
  }

  if ((room = obj_room(obj)) == NOWHERE) {
    obj_log_printf(g, obj, "oload: object in NOWHERE trying to load");
    return;
  }

  /* load mob to target room - Jamie Nelson, April 13 2004 */
  if (is_abbrev(arg1, "mob")) {
    room_vnum rnum;
    if (!target || !*target) {
      rnum = room;
    } else {
      if (!isdigit(*target) || !room_of(g, rnum = atoi(target))) {
        obj_log_printf(g, obj, "oload: room target vnum doesn't exist "
                               "(loading mob vnum %d to room %s)",
                       number, target);
        return;
      }
    }
    if ((mob = read_mobile(g, number)) == nullptr) {
      obj_log_printf(g, obj, "oload: bad mob vnum");
      return;
    }
    char_to_room(g, mob, rnum);

    if (obj_get_script(obj)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%c%d", UID_CHAR, char_get_id(mob));
      add_var(&(obj_get_script(obj)->global_vars), "lastloaded", buf, 0);
    }

    load_mtrigger(g, mob);
  }

  else if (is_abbrev(arg1, "obj")) {
    if ((object = read_object(g, number)) == nullptr) {
      obj_log_printf(g, obj, "oload: bad object vnum");
      return;
    }

    if (obj_get_script(obj)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%c%d", UID_CHAR, obj_get_id(object));
      add_var(&(obj_get_script(obj)->global_vars), "lastloaded", buf, 0);
    }

    /* special handling to make objects able to load on a person/in a container/worn etc. */
    if (!target || !*target) {
      add_unique_id(g, object);
      obj_to_room(g, object, room);
      load_otrigger(g, object);
      return;
    }
    two_arguments(target, arg1, arg2); /* recycling ... */
    tch = get_char_near_obj(g, obj, arg1);
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
    cnt = get_obj_near_obj(g, obj, arg1);
    if (cnt && obj_get_type_flag(cnt) == ITEM_CONTAINER) {
      obj_to_obj(g, object, cnt);
      load_otrigger(g, object);
      return;
    }
    /* neither char nor container found - just dump it in room */
    add_unique_id(g, object);
    obj_to_room(g, object, room);
    load_otrigger(g, object);
    return;
  }

  else
    obj_log_printf(g, obj, "oload: bad type");
}

static OCMD(do_odamage)
{
  std::string name, amount;
  int dam = 0;
  char_data *ch;

  std::string_view rest = argument;
  name                  = take_arg(rest);
  amount                = take_arg(rest);

  /* who cares if it's a number ? if not it'll just be 0 */
  if (name.empty() || amount.empty()) {
    obj_log_printf(g, obj, "odamage: bad syntax");
    return;
  }

  dam = parse_int(amount);
  ch  = get_char_by_obj(g, obj, name);

  if (!ch) {
    obj_log_printf(g, obj, "odamage: target not found");
    return;
  }
  script_damage(g, ch, dam);
}

static OCMD(do_oasound)
{
  room_vnum room;
  int door;

  skip_spaces(&argument);

  if (!*argument) {
    obj_log_printf(g, obj, "oasound called with no args");
    return;
  }

  if ((room = obj_room(obj)) == NOWHERE) {
    obj_log_printf(g, obj, "oecho called by object in NOWHERE");
    return;
  }

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (room_get_dir_option(g, room, door) != nullptr &&
        (room_get_dir_option(g, room, door))->to_room != NOWHERE &&
        (room_get_dir_option(g, room, door))->to_room != room &&
        room_get_people(g, (room_get_dir_option(g, room, door))->to_room)) {
      sub_write(g, argument, first_char(g, room_get_people(g, (room_get_dir_option(g, room, door))->to_room)), true, TO_ROOM);
      sub_write(g, argument, first_char(g, room_get_people(g, (room_get_dir_option(g, room, door))->to_room)), true, TO_CHAR);
    }
  }
}

static OCMD(do_odoor)
{
  char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], *value;
  room_data *rm;
  int dir, fd;

  argument = two_arguments(argument, target, direction);
  value    = one_argument(argument, field);
  skip_spaces(&value);

  if (!*target || !*direction || !*field) {
    obj_log_printf(g, obj, "odoor called with too few args");
    return;
  }

  if ((rm = get_room(g, target)) == nullptr) {
    obj_log_printf(g, obj, "odoor: invalid target");
    return;
  }

  if ((dir = search_block(direction, dirs, false)) == -1) {
    obj_log_printf(g, obj, "odoor: invalid direction");
    return;
  }

  if ((fd = search_block(field, dg_door_fields, false)) == -1) {
    obj_log_printf(g, obj, "odoor: invalid field");
    return;
  }

  if (!dg_door_apply(g, rm, dir, fd, value))
    obj_log_printf(g, obj, "odoor: invalid door target");
}

static OCMD(do_osetval)
{
  std::string arg1, arg2;
  int position, new_value;

  std::string_view rest = argument;
  arg1                  = take_arg(rest);
  arg2                  = take_arg(rest);
  if (arg1.empty() || arg2.empty() ||
      !is_number(arg1) || !is_number(arg2)) {
    obj_log_printf(g, obj, "osetval: bad syntax");
    return;
  }

  position  = parse_int(arg1);
  new_value = parse_int(arg2);
  if (position >= 0 && position < NUM_OBJ_VAL_POSITIONS)
    obj_set_value(obj, position, new_value);
  else
    obj_log_printf(g, obj, "osetval: position out of bounds!");
}

/* submitted by PurpleOnyx - tkhasi@shadowglen.com*/
static OCMD(do_oat)
{
  room_vnum loc = NOWHERE;
  struct char_data *ch;
  struct obj_data *object;
  char arg[MAX_INPUT_LENGTH], *command;

  command = any_one_arg(argument, arg);

  if (!*arg) {
    obj_log_printf(g, obj, "oat called with no args");
    return;
  }

  skip_spaces(&command);

  if (!*command) {
    obj_log_printf(g, obj, "oat called without a command");
    return;
  }

  if (isdigit(*arg))
    loc = atoi(arg);
  else if ((ch = get_char_by_obj(g, obj, arg)))
    loc = char_get_in_room(ch);

  if (!room_of(g, loc)) {
    obj_log_printf(g, obj, "oat: location not found (%s)", arg);
    return;
  }

  if (!(object = read_object(g, obj_get_vnum(g, obj))))
    return;

  add_unique_id(g, object);
  obj_to_room(g, object, loc);
  obj_command_interpreter(g, object, command);

  if (obj_get_in_room(object) == loc)
    extract_obj(g, object);
}

static const struct obj_command_info obj_cmd_info[] = {
    {"RESERVED",     nullptr,       0               }, /* this must be first -- for specprocs */

    {"oasound ",     do_oasound,    0               },
    {"oat ",         do_oat,        0               },
    {"odoor ",       do_odoor,      0               },
    {"odupe ",       do_dupe,       0               },
    {"odamage ",     do_odamage,    0               },
    {"oecho ",       do_oecho,      0               },
    {"oechoaround ", do_osend,      SCMD_OECHOAROUND},
    {"oforce ",      do_oforce,     0               },
    {"ogoto ",       do_ogoto,      0               },
    {"oload ",       do_dgoload,    0               },
    {"opurge ",      do_opurge,     0               },
    {"orecho ",      do_orecho,     0               },
    {"osend ",       do_osend,      SCMD_OSEND      },
    {"osetval ",     do_osetval,    0               },
    {"oteleport ",   do_oteleport,  0               },
    {"otimer ",      do_otimer,     0               },
    {"otransform ",  do_otransform, 0               },
    {"ozoneecho ",   do_ozoneecho,  0               }, /* fix by Rumble */

    {"\n",           nullptr,       0               }  /* this must be last */
};

/*
 *  This is the command interpreter used by objects, called by script_driver.
 */
void obj_command_interpreter(GameWorld *g, obj_data *obj, char *argument)
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
       *obj_cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(obj_cmd_info[cmd].command, arg, length))
      break;

  if (*obj_cmd_info[cmd].command == '\n')
    obj_log_printf(g, obj, "Unknown object cmd: '%s'", argument);
  else
    ((*obj_cmd_info[cmd].command_pointer)(g, obj, line, cmd, obj_cmd_info[cmd].subcmd));
}
