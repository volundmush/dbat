/**************************************************************************
*  File: dg_scripts.c                                                     *
*  Usage: contains the main script driver interface.                      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>

#include <unistd.h>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/fileop.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

constexpr int PULSES_PER_MUD_HOUR = SECS_PER_MUD_HOUR * PASSES_PER_SEC;


/* Local functions not used elsewhere */
static bool process_wait(GameWorld *g, owner_ptr go, trig_data *trig, char *cmd, size_t at);
static void process_set(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void process_attach(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd);
static void process_detach(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd);
static void makeuid_var(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd);
static int process_return(GameWorld *g, trig_data *trig, char *cmd);
static void process_unset(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void process_remote(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void process_rdelete(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void process_global(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd, long id);
static void process_context(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void extract_value(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void dg_letter_value(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static EVENTFUNC(trig_wait_event);

/* By id, because a trigger can purge any object at all, including the next one in the list. */
static void run_object_triggers(GameWorld *g, bitvector_t type, void (*run)(GameWorld *g, obj_data *obj))
{
  obj_data *obj;
  long *ids;
  size_t count = 0;

  for (obj_data *thing : each_obj(g))
    if (obj_get_script(thing) && IS_SET(obj_get_script(thing)->types, type))
      count++;
  if (!count)
    return;
  ids   = new long[count]();
  count = 0;
  for (obj_data *item : each_obj(g))
    if (obj_get_script(item) && IS_SET(obj_get_script(item)->types, type))
      ids[count++] = obj_get_id(item);
  for (size_t i = 0; i < count; i++)
    if ((obj = find_obj(g, ids[i])) && obj_get_script(obj) && IS_SET(obj_get_script(obj)->types, type))
      run(g, obj);
  delete[] ids;
}

/* checks every PULSE_SCRIPT for random triggers */
void script_trigger_check(GameWorld *g)
{
  struct room_data *room = nullptr;
  struct script_data *sc;

  for (char_data *ch : each_char(g)) {
    if (char_get_script(ch)) {
      sc = char_get_script(ch);

      if (IS_SET(sc->types, WTRIG_RANDOM) &&
          (!is_empty(g, room_get_zone(g, char_get_in_room(ch))) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        random_mtrigger(g, ch);
    }
  }

  run_object_triggers(g, OTRIG_RANDOM, random_otrigger);

  for (room_vnum nr : g->rooms.vnums()) {
    if (room_get_script(g, nr)) {
      room = room_of(g, nr);
      sc   = room_get_script(room);

      if (IS_SET(sc->types, WTRIG_RANDOM) &&
          (!is_empty(g, room_get_zone(room)) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        random_wtrigger(g, room);
    }
  }
}

void check_time_triggers(GameWorld *g)
{
  struct room_data *room = nullptr;
  struct script_data *sc;

  for (char_data *ch : each_char(g)) {
    if (char_get_script(ch)) {
      sc = char_get_script(ch);

      if (IS_SET(sc->types, WTRIG_TIME) &&
          (!is_empty(g, room_get_zone(g, char_get_in_room(ch))) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        time_mtrigger(g, ch);
    }
  }

  run_object_triggers(g, OTRIG_TIME, time_otrigger);

  for (room_vnum nr : g->rooms.vnums()) {
    if (room_get_script(g, nr)) {
      room = room_of(g, nr);
      sc   = room_get_script(room);

      if (IS_SET(sc->types, WTRIG_TIME) &&
          (!is_empty(g, room_get_zone(room)) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        time_wtrigger(g, room);
    }
  }
}

static void free_wait_event_data(void *event_obj)
{
  delete static_cast<struct wait_event_data *>(event_obj);
}

static EVENTFUNC(trig_wait_event)
{
  struct wait_event_data *wait_event_obj = (struct wait_event_data *)event_obj;
  long trigger_id                        = wait_event_obj->trigger_id;
  script_owner owner                     = wait_event_obj->owner;

  delete wait_event_obj;
  trig_data *trig = g->triggers.find(trigger_id);
  if (!trig) /* extracting a trigger cancels its wait, so this can't be */
    return 0;
  trig->wait_event = nullptr;

  std::optional<owner_ptr> go = owner.resolve(g);
  if (!go) {
    mud_log("Trigger restarted on unknown entity. Vnum: {:d}", trig_get_vnum(g, trig));
    mud_log("Type: {} trigger", owner.kind == owner_kind::mob ? "Mob" : owner.kind == owner_kind::obj ? "Obj"
                                                                                                        : "Room");
    mud_log("attached {:d} places", trig->index->number);
    script_log(g, "Trigger restart attempt on unknown entity.");
    return 0;
  }

  script_driver(g, *go, trig, TRIG_RESTART);

  /* Do not reenqueue*/
  return 0;
}


trig_vnum trig_get_vnum(GameWorld *g, const trig_data *t)
{
  return t->index->vnum;
}

/* processes any 'wait' commands in a trigger */
/* The longest a script can wait, in pulses: as good as forever, and far enough from overflowing once the pulse is added. */
constexpr long MAX_WAIT_PULSES = LONG_MAX / 4;

/* a * b, held to MAX_WAIT_PULSES (and as far the other way) instead of overflowing */
static long wait_mul(long a, long b)
{
  long product;

  if (__builtin_mul_overflow(a, b, &product) || product > MAX_WAIT_PULSES || product < -MAX_WAIT_PULSES)
    return (a < 0) != (b < 0) ? -MAX_WAIT_PULSES : MAX_WAIT_PULSES;
  return product;
}

/* Waits when pulses from now, after which the run carries on where it is */
void schedule_wait(GameWorld *g, owner_ptr go, trig_data *trig, long when)
{
  wait_event_data *wait_event_obj = new wait_event_data();
  wait_event_obj->trigger_id      = trig->id;
  wait_event_obj->owner           = script_owner::of(go);

  trig->wait_event = event_create(g, trig_wait_event, wait_event_obj, free_wait_event_data, when);
}

/* The wait command; at is its line. False if it can't read the time, which ends the run. */
static bool process_wait(GameWorld *g, owner_ptr go, trig_data *trig, char *cmd, size_t at)
{
  const std::string &written = trig->run.prog->lines[at].text;
  char buf[MAX_INPUT_LENGTH], *arg;
  long when, hr, min, ntime;
  char c;

  arg = any_one_arg(cmd, buf);
  skip_spaces(&arg);

  if (!*arg) {
    script_log(g, "Trigger: {}, VNum {:d}. wait w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), written);
    return false;
  }
  if (!dg::readable_wait(arg)) {
    script_log(g, "Trigger: {}, VNum {:d}. wait without a time it can read: '{}'",
               trig->name, trig_get_vnum(g, trig), written);
    return false;
  }

  if (!strncasecmp(arg, "until ", 6)) {

    /* valid forms of time are 14:30 and 1430 */
    if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
      min = wait_mul(hr, 60) + min;
    else
      min = (hr % 100) + wait_mul(hr / 100, 60);

    /* calculate the pulse of the day of "until" time */
    ntime = wait_mul(min, SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

    /* calculate pulse of day of current time */
    when = (g->pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
           (g->time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

    if (when >= ntime) /* adjust for next day */
      when = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - when + ntime;
    else
      when = ntime - when;
  }

  else if (sscanf(arg, "%ld %c", &when, &c) == 2) {
    if (c == 't')
      when = wait_mul(when, PULSES_PER_MUD_HOUR);
    else if (c == 's')
      when = wait_mul(when, PASSES_PER_SEC);
  }
  schedule_wait(g, go, trig, std::clamp(when, -MAX_WAIT_PULSES, MAX_WAIT_PULSES));
  return true;
}

/* processes a script set command */
static void process_set(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

  value = two_arguments(cmd, arg, name);

  skip_spaces(&value);

  if (!*name) {
    script_log(g, "Trigger: {}, VNum {:d}. set w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  add_var(trig->var_list, name, value, sc ? sc->context : 0);
}

/* processes a script eval command */
void process_eval(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *expr;

  expr = one_argument(cmd, arg);   /* cut off 'eval' */
  expr = one_argument(expr, name); /* cut off name */

  skip_spaces(&expr);

  if (!*name) {
    script_log(g, "Trigger: {}, VNum {:d}. eval w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  std::string result = eval_expr(g, expr, go, sc, trig);
  add_var(trig->var_list, name, result.c_str(), sc ? sc->context : 0);
}

/* script attaching a trigger to something */
static void process_attach(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char *id_p;
  trig_data *newtrig;
  char_data *c = nullptr;
  obj_data *o  = nullptr;
  room_data *r = nullptr;
  struct index_data *trignum;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log(g, "Trigger: {}, VNum {:d}. attach w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p) == 0) {
    script_log(g, "Trigger: {}, VNum {:d}. attach invalid id arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  std::string result = eval_expr(g, id_p, go, sc, trig);
  if (!(id = atoi(result.c_str()))) {
    script_log(g, "Trigger: {}, VNum {:d}. attach invalid id arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }
  c = find_char(g, id);
  if (!c) {
    o = find_obj(g, id);
    if (!o) {
      r = find_room(g, id);
      if (!r) {
        script_log(g, "Trigger: {}, VNum {:d}. attach invalid id arg: '{}'",
                   trig->name, trig_get_vnum(g, trig), cmd);
        return;
      }
    }
  }

  /* locate and load the trigger specified */
  trignum = find_trigger(g, atoi(trignum_s));
  if (trignum == nullptr || !(newtrig = read_trigger(g, trignum))) {
    script_log(g, "Trigger: {}, VNum {:d}. attach invalid trigger: '{}'",
               trig->name, trig_get_vnum(g, trig), trignum_s);
    return;
  }

  if (c) {
    if (!char_is_npc(c)) {
      script_log(g, "Trigger: {}, VNum {:d}. attach invalid target: '{}'",
                 trig->name, trig_get_vnum(g, trig), char_get_name(c));
      return;
    }
    add_trigger(g, dg_ensure_script(char_script_slot(c)), newtrig, -1);
    return;
  }

  if (o) {
    add_trigger(g, dg_ensure_script(obj_script_slot(o)), newtrig, -1);
    return;
  }

  if (r) {
    add_trigger(g, dg_ensure_script(room_script_slot(r)), newtrig, -1);
    return;
  }
}

/* script detaching a trigger from something */
static void process_detach(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char *id_p;
  char_data *c = nullptr;
  obj_data *o  = nullptr;
  room_data *r = nullptr;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log(g, "Trigger: {}, VNum {:d}. detach w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p) == 0) {
    script_log(g, "Trigger: {}, VNum {:d}. detach invalid id arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  std::string result = eval_expr(g, id_p, go, sc, trig);
  if (!(id = atoi(result.c_str()))) {
    script_log(g, "Trigger: {}, VNum {:d}. detach invalid id arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }
  c = find_char(g, id);
  if (!c) {
    o = find_obj(g, id);
    if (!o) {
      r = find_room(g, id);
      if (!r) {
        script_log(g, "Trigger: {}, VNum {:d}. detach invalid id arg: '{}'",
                   trig->name, trig_get_vnum(g, trig), cmd);
        return;
      }
    }
  }

  if (c && char_get_script(c)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(g, c);
      return;
    }
    if (remove_trigger(g, char_get_script(c), trignum_s)) {
      if (!char_get_script(c)->trig_list) {
        extract_script(g, c);
      }
    }
    return;
  }

  if (o && obj_get_script(o)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(g, o);
      return;
    }
    if (remove_trigger(g, obj_get_script(o), trignum_s)) {
      if (!obj_get_script(o)->trig_list) {
        extract_script(g, o);
      }
    }
    return;
  }

  if (r && room_get_script(r)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(g, r);
      return;
    }
    if (remove_trigger(g, room_get_script(r), trignum_s)) {
      if (!room_get_script(r)->trig_list) {
        extract_script(g, r);
      }
    }
    return;
  }
}

struct room_data *dg_room_of_obj(GameWorld *g, struct obj_data *obj)
{
  if (obj_get_in_room(obj) != NOWHERE) return room_of(g, obj_get_in_room(obj));
  if (obj_get_carried_by(obj)) return room_of(g, char_get_in_room(obj_get_carried_by(obj)));
  if (obj_get_worn_by(obj)) return room_of(g, char_get_in_room(obj_get_worn_by(obj)));
  if (obj_get_in_obj(obj)) return (dg_room_of_obj(g, obj_get_in_obj(obj)));
  return nullptr;
}

/* create a UID variable from the id number */
static void makeuid_var(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, char *cmd)
{
  int type = owner_type(go);
  char junk[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  std::string uid;
  half_chop(cmd, junk, cmd);    /* makeuid */
  half_chop(cmd, varname, cmd); /* variable name */
  half_chop(cmd, arg, cmd);     /* numerical id or 'obj' 'mob' or 'room' */
  half_chop(cmd, name, cmd);    /* if the above was obj, mob or room, this is the name */

  if (!*varname) {
    script_log(g, "Trigger: {}, VNum {:d}. makeuid w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);

    return;
  }

  if (!*arg) {
    script_log(g, "Trigger: {}, VNum {:d}. makeuid invalid id arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (atoi(arg) != 0) { /* easy, if you pass an id number */

    std::string result = eval_expr(g, arg, go, sc, trig);
    uid                = str_format("{:c}{}", as_char(UID_CHAR), result);
  } else { /* a lot more work without it */
    if (!*name) {
      script_log(g, "Trigger: {}, VNum {:d}. makeuid needs name: '{}'",
                 trig->name, trig_get_vnum(g, trig), cmd);
      return;
    }

    if (is_abbrev(arg, "mob")) {
      struct char_data *c = nullptr;
      switch (type) {
        case WLD_TRIGGER:
          c = get_char_in_room(g, std::get<room_data *>(go), name);
          break;
        case OBJ_TRIGGER:
          c = get_char_near_obj(g, std::get<obj_data *>(go), name);
          break;
        case MOB_TRIGGER:
          c = get_char_room_vis(g, std::get<char_data *>(go), name, nullptr);
          break;
      }
      if (c)
        uid = str_format("{:c}{:d}", as_char(UID_CHAR), char_get_id(c));
    } else if (is_abbrev(arg, "obj")) {
      struct obj_data *o = nullptr;
      switch (type) {
        case WLD_TRIGGER:
          o = get_obj_in_room(g, std::get<room_data *>(go), name);
          break;
        case OBJ_TRIGGER:
          o = get_obj_near_obj(g, std::get<obj_data *>(go), name);
          break;
        case MOB_TRIGGER:
          if ((o = get_obj_in_list_vis(g, std::get<char_data *>(go), name, nullptr,
                                       char_get_inventory(std::get<char_data *>(go)))) == nullptr)
            o = get_obj_in_list_vis(g, std::get<char_data *>(go), name, nullptr,
                                    room_get_contents(g, char_get_in_room(std::get<char_data *>(go))));
          break;
      }
      if (o)
        uid = str_format("{:c}{:d}", as_char(UID_CHAR), obj_get_id(o));
    } else if (is_abbrev(arg, "room")) {
      room_vnum r = NOWHERE;
      switch (type) {
        case WLD_TRIGGER:
          r = room_get_number(std::get<room_data *>(go));
          break;
        case OBJ_TRIGGER:
          r = obj_room(std::get<obj_data *>(go));
          break;
        case MOB_TRIGGER:
          r = char_get_in_room(std::get<char_data *>(go));
          break;
      }
      if (room_of(g, r))
        uid = str_format("{:c}{:d}", as_char(UID_CHAR), r + ROOM_ID_BASE);
    } else {
      script_log(g, "Trigger: {}, VNum {:d}. makeuid syntax error: '{}'",
                 trig->name, trig_get_vnum(g, trig), cmd);

      return;
    }
  }
  if (!uid.empty())
    add_var(trig->var_list, varname, uid.c_str(), sc ? sc->context : 0);
}

/*
 * processes a script return command.
 * returns the new value for the script to return.
 */
static int process_return(GameWorld *g, trig_data *trig, char *cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(cmd, arg1, arg2);

  if (!*arg2) {
    script_log(g, "Trigger: {}, VNum {:d}. return w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);

    return 1;
  }

  return atoi(arg2);
}

/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
static void process_unset(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log(g, "Trigger: {}, VNum {:d}. unset w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (!sc->global_vars.erase(var))
    trig->var_list.erase(var);
}

/*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
static void process_remote(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  const dg::variable *vd;
  struct script_data *sc_remote = nullptr;
  char *line, *var, *uid_p;
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  line = any_one_arg(cmd, arg);
  two_arguments(line, buf, buf2);
  var   = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);

  if (!*buf || !*buf2) {
    script_log(g, "Trigger: {}, VNum {:d}. remote: invalid arguments '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* find the locally owned variable */
  vd = trig->var_list.find(buf);
  if (!vd)
    vd = sc->global_vars.find_in(var, sc->context);

  if (!vd) {
    script_log(g, "Trigger: {}, VNum {:d}. local var '{}' not found in remote call",
               trig->name, trig_get_vnum(g, trig), buf);
    return;
  }
  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    script_log(g, "Trigger: {}, VNum {:d}. remote: illegal uid '{}'",
               trig->name, trig_get_vnum(g, trig), buf2);
    return;
  }

  /* for all but PC's, context comes from the existing context. */
  /* for PC's, context is 0 (global) */
  context = vd->context;

  if ((room = find_room(g, uid))) {
    sc_remote = room_get_script(room);
  } else if ((mob = find_char(g, uid))) {
    sc_remote = char_get_script(mob);
    if (!char_is_npc(mob)) context = 0;
  } else if ((obj = find_obj(g, uid))) {
    sc_remote = obj_get_script(obj);
  } else {
    script_log(g, "Trigger: {}, VNum {:d}. remote: uid '{:d}' invalid",
               trig->name, trig_get_vnum(g, trig), uid);
    return;
  }

  if (sc_remote == nullptr) return; /* no script to assign */

  add_var(sc_remote->global_vars, vd->name, vd->value, context); /* which may be sc's own */
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
static void process_rdelete(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  struct script_data *sc_remote = nullptr;
  char *line, *var, *uid_p;
  char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  long uid;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  line = any_one_arg(cmd, arg);
  two_arguments(line, buf, buf2);
  var   = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);

  if (!*buf || !*buf2) {
    script_log(g, "Trigger: {}, VNum {:d}. rdelete: invalid arguments '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    script_log(g, "Trigger: {}, VNum {:d}. rdelete: illegal uid '{}'",
               trig->name, trig_get_vnum(g, trig), buf2);
    return;
  }

  if ((room = find_room(g, uid))) {
    sc_remote = room_get_script(room);
  } else if ((mob = find_char(g, uid))) {
    sc_remote = char_get_script(mob);
  } else if ((obj = find_obj(g, uid))) {
    sc_remote = obj_get_script(obj);
  } else {
    script_log(g, "Trigger: {}, VNum {:d}. remote: uid '{:d}' invalid",
               trig->name, trig_get_vnum(g, trig), uid);
    return;
  }

  if (sc_remote == nullptr) return; /* no script to delete a trigger from */

  /* the global, unless it doesn't exist or is of the wrong context */
  if (const dg::variable *vd = sc_remote->global_vars.find_in(var, sc->context))
    sc_remote->global_vars.erase(vd);
}

/*
 * makes a local variable into a global variable
 */
static void process_global(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd, long id)
{
  const dg::variable *vd;
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log(g, "Trigger: {}, VNum {:d}. global w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  vd = trig->var_list.find(var);
  if (!vd) {
    script_log(g, "Trigger: {}, VNum {:d}. local var '{}' not found in global call",
               trig->name, trig_get_vnum(g, trig), var);
    return;
  }

  add_var(sc->global_vars, vd->name, vd->value, id);
  trig->var_list.erase(vd);
}

/* set the current context for a script */
static void process_context(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log(g, "Trigger: {}, VNum {:d}. context w/o an arg: '{}'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  sc->context = atol(var);
}

static void extract_value(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  char *buf3;
  char to[128];
  int num;

  buf3 = any_one_arg(cmd, buf);
  half_chop(buf3, buf2, buf);
  if (strlen(buf2) >= sizeof(to)) {
    script_log(g, "extract: variable name is longer than {:d} characters", as_unsigned(sizeof(to) - 1));
    return;
  }
  strcpy(to, buf2); /* strcpy: OK (length checked above) */

  num = atoi(buf);
  if (num < 1) {
    script_log(g, "extract number < 1!");
    return;
  }

  half_chop(buf, buf3, buf2);

  while (num > 0) {
    half_chop(buf2, buf, buf2);
    num--;
  }

  add_var(trig->var_list, to, buf, sc ? sc->context : 0);
}

/*
  Thanks to Jamie Nelson for 4 dimensions for this addition

  Syntax :
    dg_letter <new varname> <letter position> <string to get from>

    ie:
    set string L337-String
    dg_letter var1 4 %string%
    dg_letter var2 11 %string%

    now %var1% == 7 and %var2% == g

    Note that the index starts at 1.

*/

static void dg_letter_value(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  //set the letter/number at position 'num' as the variable.
  char junk[MAX_INPUT_LENGTH];
  char varname[MAX_INPUT_LENGTH];
  char num_s[MAX_INPUT_LENGTH];
  char string[MAX_INPUT_LENGTH];
  int num;

  half_chop(cmd, junk, cmd); /* "dg_letter" */
  half_chop(cmd, varname, cmd);
  half_chop(cmd, num_s, string);

  num = atoi(num_s);

  script_log(g, "The use of dg_letter is deprecated");
  script_log(g, "- Use 'set <new variable> %<text/var>.charat(index)%' instead.");

  if (num < 1) {
    script_log(g, "Trigger #{:d} : dg_letter number < 1!", trig_get_vnum(g, trig));
    return;
  }

  if ((size_t)num > strlen(string)) {
    script_log(g, "Trigger #{:d} : dg_letter number > strlen!", trig_get_vnum(g, trig));
    return;
  }

  *junk       = string[num - 1];
  *(junk + 1) = '\0';
  add_var(trig->var_list, varname, junk, sc->context);
}

/*
 * The DG commands, which a command line runs after substitution, ahead of the owner's own. Each is found by the start
 * of the line, in this order and without regard to case, so the command word can come from a variable.
 */
namespace {

struct dg_command {
  std::string_view prefix;
  command_result (*run)(command_call &c);
};

command_result carry_on() { return command_result::next; }

constexpr dg_command dg_commands[] = {
    {"eval ", [](command_call &c) { process_eval(c.g, c.go, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"nop ", [](command_call &) { return carry_on(); }},
    {"extract ", [](command_call &c) { extract_value(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"dg_letter ", [](command_call &c) { dg_letter_value(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"makeuid ", [](command_call &c) { makeuid_var(c.g, c.go, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"halt", [](command_call &) { return command_result::halt; }},
    {"dg_cast ", [](command_call &c) { do_dg_cast(c.g, c.go, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"dg_affect ", [](command_call &c) { do_dg_affect(c.g, c.go, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"global ", [](command_call &c) { process_global(c.g, c.sc, c.trig, c.cmd, c.sc->context); return carry_on(); }},
    {"context ", [](command_call &c) { process_context(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"remote ", [](command_call &c) { process_remote(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"rdelete ", [](command_call &c) { process_rdelete(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"return ", [](command_call &c) { c.ret_val = process_return(c.g, c.trig, c.cmd); return carry_on(); }},
    {"set ", [](command_call &c) { process_set(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"unset ", [](command_call &c) { process_unset(c.g, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"wait ",
     [](command_call &c) {
       return process_wait(c.g, c.go, c.trig, c.cmd, c.at) ? command_result::stop : command_result::halt;
     }},
    {"attach ", [](command_call &c) { process_attach(c.g, c.go, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"detach ", [](command_call &c) { process_detach(c.g, c.go, c.sc, c.trig, c.cmd); return carry_on(); }},
    {"version", [](command_call &c) { mudlog(c.g, LOG_NRM, ADMLVL_GOD, true, "{}", DG_SCRIPT_VERSION); return carry_on(); }},
};

} // namespace

command_result run_command(command_call &c)
{
  for (const dg_command &command : dg_commands)
    if (!strncasecmp(c.cmd, command.prefix.data(), command.prefix.size()))
      return command.run(c);

  std::visit(overloaded{[&](char_data *ch) { command_interpreter(c.g, ch, c.cmd); },
                        [&](obj_data *obj) { obj_command_interpreter(c.g, obj, c.cmd); },
                        [&](room_data *room) { wld_command_interpreter(c.g, room, c.cmd); }},
             c.go);
  if (c.g->dg_owner_purged) {
    c.owner_gone = true;
    return command_result::stop;
  }
  return command_result::next;
}

/* The trigger index entry for vnum, or null. */
struct index_data *find_trigger(GameWorld *g, trig_vnum vnum)
{
  return g->trig_index.find(vnum);
}

/* load in a character's saved variables */
void read_saved_vars(struct char_data *ch)
{
  FILE *file;
  long context;
  char fn[127];
  char input_line[1024], *temp, *p;
  char varname[sizeof(input_line)];
  char context_str[sizeof(input_line)];

  /*
   * If getting to the menu from inside the game, the vars aren't removed.
   * So let's not allocate them again.
   */
  if (char_get_script(ch))
    return;

  /* create the space for the script structure which holds the vars */
  /* We need to do this first, because later calls to 'remote' will need */
  /* a script already assigned. */
  *char_script_slot(ch) = new script_data();

  /* find the file that holds the saved variables and open it*/
  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, char_get_name(ch));
  file = fopen(fn, "r");

  /* if we failed to open the file, return */
  if (!file) {
    mud_log("{} had no variable file", char_get_name(ch));
    return;
  }
  /* walk through each line in the file parsing variables */
  do {
    if (get_line(file, input_line) > 0) {
      p = temp = strdup(input_line);
      temp     = any_one_arg(temp, varname);
      temp     = any_one_arg(temp, context_str);
      skip_spaces(&temp); /* temp now points to the rest of the line */

      context = atol(context_str);
      add_var(char_get_script(ch)->global_vars, varname, temp, context);
      free(p); /* plug memory hole */
    }
  } while (!feof(file));

  /* close the file and return */
  fclose(file);
}

/* save a characters variables out to disk */
void save_char_vars(GameWorld *g, struct char_data *ch)
{
  struct atomic_file af;
  FILE *file;
  char fn[127];

  /* immediate return if no script (and therefore no variables) structure */
  /* has been created. this will happen when the player is logging in */
  if (char_get_script(ch) == nullptr) return;

  /* we should never be called for an NPC, but just in case... */
  if (char_is_npc(ch)) return;

  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, char_get_name(ch));

  /* make sure this char has global variables to save */
  const dg::var_store &vars = char_get_script(ch)->global_vars;
  if (vars.empty()) {
    unlink(fn);
    return;
  }

  file = atomic_open(&af, fn, true);
  if (!file) {
    mudlog(g, LOG_NRM, ADMLVL_GOD, true,
           "SYSERR: Could not open player variable file {} for writing.:{}",
           fn, strerror(errno));
    return;
  }
  /* note that currently, context will always be zero. this may change */
  /* in the future */
  for (const dg::variable &v : vars)
    if (v.name[0] != '-') /* don't save if it begins with - */
      file_print(file, "{} {:d} {}\n", v.name, v.context, v.value);

  atomic_commit(&af);
}

