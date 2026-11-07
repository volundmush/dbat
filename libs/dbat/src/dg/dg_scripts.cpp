/**************************************************************************
*  File: dg_scripts.c                                                     *
*  Usage: contains the main script driver interface.                      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cerrno>

#include <unistd.h>

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
static void process_wait(GameWorld *g, void *go, trig_data *trig, int type, char *cmd,
                         struct cmdlist_element *cl);
static void process_set(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd);
static void process_attach(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd);
static void process_detach(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd);
static void makeuid_var(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                        int type, char *cmd);
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

  for (obj = g->objects; obj; obj = obj_get_next(obj))
    if (obj_get_script(obj) && IS_SET(obj_get_script(obj)->types, type))
      count++;
  if (!count)
    return;
  CREATE(ids, long, count);
  count = 0;
  for (obj = g->objects; obj; obj = obj_get_next(obj))
    if (obj_get_script(obj) && IS_SET(obj_get_script(obj)->types, type))
      ids[count++] = obj_get_id(obj);
  for (size_t i = 0; i < count; i++)
    if ((obj = find_obj(g, ids[i])) && obj_get_script(obj) && IS_SET(obj_get_script(obj)->types, type))
      run(g, obj);
  free(ids);
}

/* checks every PULSE_SCRIPT for random triggers */
void script_trigger_check(GameWorld *g)
{
  char_data *ch;
  struct room_data *room = nullptr;
  room_rnum nr;
  struct script_data *sc;

  for (ch = g->characters; ch; ch = char_get_next(ch)) {
    if (char_get_script(ch)) {
      sc = char_get_script(ch);

      if (IS_SET(sc->types, WTRIG_RANDOM) &&
          (!is_empty(g, rroom_get_zone(g, char_get_in_room(ch))) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        random_mtrigger(g, ch);
    }
  }

  run_object_triggers(g, OTRIG_RANDOM, random_otrigger);

  for (nr = 0; nr <= g->top_room; nr++) {
    if (rroom_get_script(g, nr)) {
      room = &g->rooms[nr];
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
  char_data *ch;
  struct room_data *room = nullptr;
  room_rnum nr;
  struct script_data *sc;

  for (ch = g->characters; ch; ch = char_get_next(ch)) {
    if (char_get_script(ch)) {
      sc = char_get_script(ch);

      if (IS_SET(sc->types, WTRIG_TIME) &&
          (!is_empty(g, rroom_get_zone(g, char_get_in_room(ch))) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        time_mtrigger(g, ch);
    }
  }

  run_object_triggers(g, OTRIG_TIME, time_otrigger);

  for (nr = 0; nr <= g->top_room; nr++) {
    if (rroom_get_script(g, nr)) {
      room = &g->rooms[nr];
      sc   = room_get_script(room);

      if (IS_SET(sc->types, WTRIG_TIME) &&
          (!is_empty(g, room_get_zone(room)) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        time_wtrigger(g, room);
    }
  }
}

static EVENTFUNC(trig_wait_event)
{
  struct wait_event_data *wait_event_obj = (struct wait_event_data *)event_obj;
  trig_data *trig;
  void *go;
  int type;

  trig = wait_event_obj->trigger;
  go   = wait_event_obj->go;
  type = wait_event_obj->type;

  free(wait_event_obj);
  trig->wait_event = nullptr;

#if 1 /* debugging */
  {
    int found = false;
    if (type == MOB_TRIGGER) {
      struct char_data *tch;
      for (tch = g->characters; tch && !found; tch = char_get_next(tch))
        if (tch == (struct char_data *)go)
          found = true;
    } else if (type == OBJ_TRIGGER) {
      struct obj_data *obj;
      for (obj = g->objects; obj && !found; obj = obj_get_next(obj))
        if (obj == (struct obj_data *)go)
          found = true;
    } else {
      room_rnum i;
      for (i = 0; i < g->top_room && !found; i++)
        if (&g->rooms[i] == (struct room_data *)go)
          found = true;
    }
    if (!found) {
      mud_log("Trigger restarted on unknown entity. Vnum: %d", trig_get_vnum(g, trig));
      mud_log("Type: %s trigger", type == MOB_TRIGGER ? "Mob" : type == OBJ_TRIGGER ? "Obj"
                                                                                    : "Room");
      mud_log("attached %d places", g->trig_index[trig->nr]->number);
      script_log(g, "Trigger restart attempt on unknown entity.");
      return 0;
    }
  }
#endif

  script_driver(g, &go, trig, type, TRIG_RESTART);

  /* Do not reenqueue*/
  return 0;
}


trig_vnum trig_get_vnum(GameWorld *g, const trig_data *t)
{
  return g->trig_index[t->nr]->vnum;
}

static void dg_ctx_begin(GameWorld *g, void *go, trig_data *trig, int type)
{
  room_rnum room = NOWHERE;

  memset(&g->dg_ctx, 0, sizeof(g->dg_ctx));
  g->dg_ctx.active     = true;
  g->dg_ctx.owner_type = type;
  g->dg_ctx.trig_vnum  = (long long)trig_get_vnum(g, trig);
  snprintf(g->dg_ctx.trig_name, sizeof(g->dg_ctx.trig_name), "%s", trig->name ? trig->name : "");
  g->dg_ctx.room_vnum = -1;

  if (!go)
    return;

  switch (type) {
    case MOB_TRIGGER:
      snprintf(g->dg_ctx.owner_name, sizeof(g->dg_ctx.owner_name), "%s", char_get_name((char_data *)go) ? char_get_name((char_data *)go) : "");
      g->dg_ctx.owner_vnum = (long long)char_get_mob_vnum(g, (char_data *)go);
      g->dg_ctx.owner_uid  = char_get_id((char_data *)go);
      room              = char_get_in_room((char_data *)go);
      break;
    case OBJ_TRIGGER:
      snprintf(g->dg_ctx.owner_name, sizeof(g->dg_ctx.owner_name), "%s", obj_get_short_description((obj_data *)go) ? obj_get_short_description((obj_data *)go) : "");
      g->dg_ctx.owner_vnum = (long long)obj_get_vnum(g, (obj_data *)go);
      g->dg_ctx.owner_uid  = obj_get_id((obj_data *)go);
      room              = obj_room((obj_data *)go);
      break;
    case WLD_TRIGGER:
      snprintf(g->dg_ctx.owner_name, sizeof(g->dg_ctx.owner_name), "%s", room_get_name((room_data *)go) ? room_get_name((room_data *)go) : "");
      g->dg_ctx.owner_vnum = (long long)room_get_number((room_data *)go);
      g->dg_ctx.owner_uid  = room_get_number((room_data *)go) + ROOM_ID_BASE;
      g->dg_ctx.room_vnum  = g->dg_ctx.owner_vnum;
      return;
  }

  if (room != NOWHERE && room >= 0 && room <= g->top_room)
    g->dg_ctx.room_vnum = (long long)rroom_get_vnum(g, room);
}

static void dg_ctx_set_line(GameWorld *g, const char *cmd)
{
  snprintf(g->dg_ctx.line, sizeof(g->dg_ctx.line), "%s", cmd ? cmd : "");
}

/* Writes " [trig ... ]" describing the running trigger, or "" when none is. */
void dg_context_str(GameWorld *g, char *buf, size_t len)
{
  static const char *owner_types[] = {"mob", "obj", "room"};

  if (!g->dg_ctx.active) {
    snprintf(buf, len, "%s", "");
    return;
  }

  snprintf(buf, len, " [trig #%lld '%s' on %s '%s' #%lld uid %ld in room %lld; line: %s]",
           g->dg_ctx.trig_vnum, g->dg_ctx.trig_name,
           (g->dg_ctx.owner_type >= 0 && g->dg_ctx.owner_type <= 2) ? owner_types[g->dg_ctx.owner_type] : "?",
           g->dg_ctx.owner_name, g->dg_ctx.owner_vnum, g->dg_ctx.owner_uid, g->dg_ctx.room_vnum,
           *g->dg_ctx.line ? g->dg_ctx.line : "(none)");
}

/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void script_vlog(GameWorld *g, const char *format, va_list args)
{
  char output[MAX_STRING_LENGTH];
  char print_to[MAX_STRING_LENGTH];
  char ctx[MAX_STRING_LENGTH];
  struct descriptor_data *i;

  vsnprintf(print_to, sizeof(print_to), format, args);
  dg_context_str(g, ctx, sizeof(ctx));

  mud_log("SCRIPT ERR: %s%s", print_to, ctx);

  /* the rest is mostly a rip from basic_mud_log() */
  snprintf(output, sizeof(output), "[ %s%s ]\r\n", print_to, ctx);

  for (i = g->descriptors; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < ADMLVL_BUILDER)
      continue;
    if (char_plr_flagged(i->character, PLR_WRITING))
      continue;
    if (LOG_NRM > (char_prf_flagged(g, i->character, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(g, i->character, "@g%s@n", output);
  }
}

void script_log(GameWorld *g, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  script_vlog(g, format, args);
  va_end(args);
}

/* processes any 'wait' commands in a trigger */
static void process_wait(GameWorld *g, void *go, trig_data *trig, int type, char *cmd,
                         struct cmdlist_element *cl)
{
  char buf[MAX_INPUT_LENGTH], *arg;
  struct wait_event_data *wait_event_obj;
  long when, hr, min, ntime;
  char c;

  arg = any_one_arg(cmd, buf);
  skip_spaces(&arg);

  if (!*arg) {
    script_log(g, "Trigger: %s, VNum %d. wait w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cl->cmd);
    return;
  }

  if (!strncasecmp(arg, "until ", 6)) {

    /* valid forms of time are 14:30 and 1430 */
    if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
      min += (hr * 60);
    else
      min = (hr % 100) + ((hr / 100) * 60);

    /* calculate the pulse of the day of "until" time */
    ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

    /* calculate pulse of day of current time */
    when = (g->pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
           (g->time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

    if (when >= ntime) /* adjust for next day */
      when = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - when + ntime;
    else
      when = ntime - when;
  }

  else {
    if (sscanf(arg, "%ld %c", &when, &c) == 2) {
      if (c == 't')
        when *= PULSES_PER_MUD_HOUR;
      else if (c == 's')
        when *= PASSES_PER_SEC;
    }
  }

  CREATE(wait_event_obj, struct wait_event_data, 1);
  wait_event_obj->trigger = trig;
  wait_event_obj->go      = go;
  wait_event_obj->type    = type;

  trig->wait_event = event_create(g, trig_wait_event, wait_event_obj, when);
  trig->curr_state = cl->next;
}

/* processes a script set command */
static void process_set(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

  value = two_arguments(cmd, arg, name);

  skip_spaces(&value);

  if (!*name) {
    script_log(g, "Trigger: %s, VNum %d. set w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  add_var(&trig->var_list, name, value, sc ? sc->context : 0);
}

/* processes a script eval command */
void process_eval(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *expr;

  expr = one_argument(cmd, arg);   /* cut off 'eval' */
  expr = one_argument(expr, name); /* cut off name */

  skip_spaces(&expr);

  if (!*name) {
    script_log(g, "Trigger: %s, VNum %d. eval w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  eval_expr(g, expr, result, go, sc, trig, type);
  add_var(&trig->var_list, name, result, sc ? sc->context : 0);
}

/* script attaching a trigger to something */
static void process_attach(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  trig_data *newtrig;
  char_data *c = nullptr;
  obj_data *o  = nullptr;
  room_data *r = nullptr;
  trig_rnum trignum;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log(g, "Trigger: %s, VNum %d. attach w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p) == 0) {
    script_log(g, "Trigger: %s, VNum %d. attach invalid id arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(g, id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log(g, "Trigger: %s, VNum %d. attach invalid id arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }
  c = find_char(g, id);
  if (!c) {
    o = find_obj(g, id);
    if (!o) {
      r = find_room(g, id);
      if (!r) {
        script_log(g, "Trigger: %s, VNum %d. attach invalid id arg: '%s'",
                   trig->name, trig_get_vnum(g, trig), cmd);
        return;
      }
    }
  }

  /* locate and load the trigger specified */
  trignum = real_trigger(g, atoi(trignum_s));
  if (trignum == NOTHING || !(newtrig = read_trigger(g, trignum))) {
    script_log(g, "Trigger: %s, VNum %d. attach invalid trigger: '%s'",
               trig->name, trig_get_vnum(g, trig), trignum_s);
    return;
  }

  if (c) {
    if (!char_is_npc(c)) {
      script_log(g, "Trigger: %s, VNum %d. attach invalid target: '%s'",
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
static void process_detach(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  char_data *c = nullptr;
  obj_data *o  = nullptr;
  room_data *r = nullptr;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log(g, "Trigger: %s, VNum %d. detach w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p) == 0) {
    script_log(g, "Trigger: %s, VNum %d. detach invalid id arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(g, id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log(g, "Trigger: %s, VNum %d. detach invalid id arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }
  c = find_char(g, id);
  if (!c) {
    o = find_obj(g, id);
    if (!o) {
      r = find_room(g, id);
      if (!r) {
        script_log(g, "Trigger: %s, VNum %d. detach invalid id arg: '%s'",
                   trig->name, trig_get_vnum(g, trig), cmd);
        return;
      }
    }
  }

  if (c && char_get_script(c)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(g, c, MOB_TRIGGER);
      return;
    }
    if (remove_trigger(g, char_get_script(c), trignum_s)) {
      if (!char_get_script(c)->trig_list) {
        extract_script(g, c, MOB_TRIGGER);
      }
    }
    return;
  }

  if (o && obj_get_script(o)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(g, o, OBJ_TRIGGER);
      return;
    }
    if (remove_trigger(g, obj_get_script(o), trignum_s)) {
      if (!obj_get_script(o)->trig_list) {
        extract_script(g, o, OBJ_TRIGGER);
      }
    }
    return;
  }

  if (r && room_get_script(r)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(g, r, WLD_TRIGGER);
      return;
    }
    if (remove_trigger(g, room_get_script(r), trignum_s)) {
      if (!room_get_script(r)->trig_list) {
        extract_script(g, r, WLD_TRIGGER);
      }
    }
    return;
  }
}

struct room_data *dg_room_of_obj(GameWorld *g, struct obj_data *obj)
{
  if (obj_get_in_room(obj) != NOWHERE) return &g->rooms[obj_get_in_room(obj)];
  if (obj_get_carried_by(obj)) return &g->rooms[char_get_in_room(obj_get_carried_by(obj))];
  if (obj_get_worn_by(obj)) return &g->rooms[char_get_in_room(obj_get_worn_by(obj))];
  if (obj_get_in_obj(obj)) return (dg_room_of_obj(g, obj_get_in_obj(obj)));
  return nullptr;
}

/* create a UID variable from the id number */
static void makeuid_var(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                        int type, char *cmd)
{
  char junk[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char uid[MAX_INPUT_LENGTH];

  *uid = '\0';
  half_chop(cmd, junk, cmd);    /* makeuid */
  half_chop(cmd, varname, cmd); /* variable name */
  half_chop(cmd, arg, cmd);     /* numerical id or 'obj' 'mob' or 'room' */
  half_chop(cmd, name, cmd);    /* if the above was obj, mob or room, this is the name */

  if (!*varname) {
    script_log(g, "Trigger: %s, VNum %d. makeuid w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);

    return;
  }

  if (!*arg) {
    script_log(g, "Trigger: %s, VNum %d. makeuid invalid id arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (atoi(arg) != 0) { /* easy, if you pass an id number */
    char result[MAX_INPUT_LENGTH];

    eval_expr(g, arg, result, go, sc, trig, type);
    snprintf(uid, sizeof(uid), "%c%s", UID_CHAR, result);
  } else { /* a lot more work without it */
    if (!*name) {
      script_log(g, "Trigger: %s, VNum %d. makeuid needs name: '%s'",
                 trig->name, trig_get_vnum(g, trig), cmd);
      return;
    }

    if (is_abbrev(arg, "mob")) {
      struct char_data *c = nullptr;
      switch (type) {
        case WLD_TRIGGER:
          c = get_char_in_room(g, (struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          c = get_char_near_obj(g, (struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          c = get_char_room_vis(g, (struct char_data *)go, name, nullptr);
          break;
      }
      if (c)
        snprintf(uid, sizeof(uid), "%c%d", UID_CHAR, char_get_id(c));
    } else if (is_abbrev(arg, "obj")) {
      struct obj_data *o = nullptr;
      switch (type) {
        case WLD_TRIGGER:
          o = get_obj_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          o = get_obj_near_obj(g, (struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          if ((o = get_obj_in_list_vis(g, (struct char_data *)go, name, nullptr,
                                       char_get_inventory((struct char_data *)go))) == nullptr)
            o = get_obj_in_list_vis(g, (struct char_data *)go, name, nullptr,
                                    rroom_get_contents(g, char_get_in_room((struct char_data *)go)));
          break;
      }
      if (o)
        snprintf(uid, sizeof(uid), "%c%d", UID_CHAR, obj_get_id(o));
    } else if (is_abbrev(arg, "room")) {
      room_rnum r = NOWHERE;
      switch (type) {
        case WLD_TRIGGER:
          r = real_room(g, room_get_number((struct room_data *)go));
          break;
        case OBJ_TRIGGER:
          r = obj_room((struct obj_data *)go);
          break;
        case MOB_TRIGGER:
          r = char_get_in_room((struct char_data *)go);
          break;
      }
      if (r != NOWHERE)
        snprintf(uid, sizeof(uid), "%c%d", UID_CHAR, rroom_get_vnum(g, r) + ROOM_ID_BASE);
    } else {
      script_log(g, "Trigger: %s, VNum %d. makeuid syntax error: '%s'",
                 trig->name, trig_get_vnum(g, trig), cmd);

      return;
    }
  }
  if (*uid)
    add_var(&trig->var_list, varname, uid, sc ? sc->context : 0);
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
    script_log(g, "Trigger: %s, VNum %d. return w/o an arg: '%s'",
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
    script_log(g, "Trigger: %s, VNum %d. unset w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  if (!remove_var(&(sc->global_vars), var))
    remove_var(&trig->var_list, var);
}

/*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
static void process_remote(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd;
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
    script_log(g, "Trigger: %s, VNum %d. remote: invalid arguments '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* find the locally owned variable */
  for (vd = trig->var_list; vd; vd = vd->next)
    if (!strcasecmp(vd->name, buf))
      break;

  if (!vd)
    for (vd = sc->global_vars; vd; vd = vd->next)
      if (!strcasecmp(vd->name, var) &&
          (vd->context == 0 || vd->context == sc->context))
        break;

  if (!vd) {
    script_log(g, "Trigger: %s, VNum %d. local var '%s' not found in remote call",
               trig->name, trig_get_vnum(g, trig), buf);
    return;
  }
  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    script_log(g, "Trigger: %s, VNum %d. remote: illegal uid '%s'",
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
    script_log(g, "Trigger: %s, VNum %d. remote: uid '%ld' invalid",
               trig->name, trig_get_vnum(g, trig), uid);
    return;
  }

  if (sc_remote == nullptr) return; /* no script to assign */

  add_var(&(sc_remote->global_vars), vd->name, vd->value, context);
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
static void process_rdelete(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd, *vd_prev = nullptr;
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
    script_log(g, "Trigger: %s, VNum %d. rdelete: invalid arguments '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    script_log(g, "Trigger: %s, VNum %d. rdelete: illegal uid '%s'",
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
    script_log(g, "Trigger: %s, VNum %d. remote: uid '%ld' invalid",
               trig->name, trig_get_vnum(g, trig), uid);
    return;
  }

  if (sc_remote == nullptr) return;              /* no script to delete a trigger from */
  if (sc_remote->global_vars == nullptr) return; /* no script globals */

  /* find the global */
  for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
    if (!strcasecmp(vd->name, var) &&
        (vd->context == 0 || vd->context == sc->context))
      break;

  if (!vd) return; /* the variable doesn't exist, or is the wrong context */

  /* ok, delete the variable */
  if (vd_prev) vd_prev->next = vd->next;
  else sc_remote->global_vars = vd->next;

  /* and free up the space */
  free(vd->value);
  free(vd->name);
  free(vd);
}

/*
 * makes a local variable into a global variable
 */
static void process_global(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd, long id)
{
  struct trig_var_data *vd;
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log(g, "Trigger: %s, VNum %d. global w/o an arg: '%s'",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  for (vd = trig->var_list; vd; vd = vd->next)
    if (!strcasecmp(vd->name, var))
      break;

  if (!vd) {
    script_log(g, "Trigger: %s, VNum %d. local var '%s' not found in global call",
               trig->name, trig_get_vnum(g, trig), var);
    return;
  }

  add_var(&(sc->global_vars), vd->name, vd->value, id);
  remove_var(&trig->var_list, vd->name);
}

/* set the current context for a script */
static void process_context(GameWorld *g, struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log(g, "Trigger: %s, VNum %d. context w/o an arg: '%s'",
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
    script_log(g, "extract: variable name is longer than %zu characters", sizeof(to) - 1);
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

  add_var(&trig->var_list, to, buf, sc ? sc->context : 0);
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
  script_log(g, "- Use 'set <new variable> %%<text/var>.charat(index)%%' instead.");

  if (num < 1) {
    script_log(g, "Trigger #%d : dg_letter number < 1!", trig_get_vnum(g, trig));
    return;
  }

  if ((size_t)num > strlen(string)) {
    script_log(g, "Trigger #%d : dg_letter number > strlen!", trig_get_vnum(g, trig));
    return;
  }

  *junk       = string[num - 1];
  *(junk + 1) = '\0';
  add_var(&trig->var_list, varname, junk, sc->context);
}

/* the script attached to go, a mob, object or room according to type */
static struct script_data *script_of(void *go, int type)
{
  struct script_data **slot = script_slot(go, type);
  return slot ? *slot : nullptr;
}

/* What script_driver_body does after running one command line. */
enum dg_line_result {
  DG_LINE_NEXT, /* go on to the next line */
  DG_LINE_HALT, /* 'halt': end the run normally */
  DG_LINE_STOP, /* return at once: a 'wait' was scheduled, or the owner was purged */
};

/*
 * Runs one command line of a trigger, after variable substitution: a DG built-in
 * such as 'set' or 'wait', or else a mob, object or room command.
 */
static enum dg_line_result dg_run_command(GameWorld *g, void *go_adress, void *go, struct script_data *sc, trig_data *trig,
                                          int type, char *cmd, struct cmdlist_element *cl, int *ret_val)
{
  if (!strncasecmp(cmd, "eval ", 5))
    process_eval(g, go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "nop ", 4))
    ; /* nop: do nothing */

  else if (!strncasecmp(cmd, "extract ", 8))
    extract_value(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "dg_letter ", 10))
    dg_letter_value(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "makeuid ", 8))
    makeuid_var(g, go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "halt", 4))
    return DG_LINE_HALT;

  else if (!strncasecmp(cmd, "dg_cast ", 8))
    do_dg_cast(g, go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "dg_affect ", 10))
    do_dg_affect(g, go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "global ", 7))
    process_global(g, sc, trig, cmd, sc->context);

  else if (!strncasecmp(cmd, "context ", 8))
    process_context(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "remote ", 7))
    process_remote(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "rdelete ", 8))
    process_rdelete(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "return ", 7))
    *ret_val = process_return(g, trig, cmd);

  else if (!strncasecmp(cmd, "set ", 4))
    process_set(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "unset ", 6))
    process_unset(g, sc, trig, cmd);

  else if (!strncasecmp(cmd, "wait ", 5)) {
    process_wait(g, go, trig, type, cmd, cl);
    return DG_LINE_STOP;
  }

  else if (!strncasecmp(cmd, "attach ", 7))
    process_attach(g, go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "detach ", 7))
    process_detach(g, go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "version", 7))
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "%s", DG_SCRIPT_VERSION);

  else {
    switch (type) {
      case MOB_TRIGGER:
        command_interpreter(g, (char_data *)go, cmd);
        break;
      case OBJ_TRIGGER:
        obj_command_interpreter(g, (obj_data *)go, cmd);
        break;
      case WLD_TRIGGER:
        wld_command_interpreter(g, (struct room_data *)go, cmd);
        break;
    }
    if (g->dg_owner_purged) {
      if (type == OBJ_TRIGGER)
        *(obj_data **)go_adress = nullptr;
      return DG_LINE_STOP;
    }
  }

  return DG_LINE_NEXT;
}

/*  This is the core driver for scripts. */
/*  Arguments:
    void *go_adress
      A pointer to a pointer to the entity running the script.
      The reason for this approcah is that we want to be able to see
      from the calling function, if the entity has been free'd.

    trig_data *trig
      A pointer to the current running trigger.

    int type
      MOB_TRIGGER, OBJ_TRIGGER or WLD_TRIGGER, respectively.

    int mode
      TRIG_NEW     just started from dg_triggers.c
      TRIG_RESTART restarted after a 'wait'
*/
static int script_driver_body(GameWorld *g, void *go_adress, trig_data *trig, int type, int mode);

/* Runs a trigger with dg_ctx describing it, restoring the caller's context after. */
int script_driver(GameWorld *g, void *go_adress, trig_data *trig, int type, int mode)
{
  struct dg_run_ctx saved = g->dg_ctx;
  void *go                = go_adress ? *(void **)go_adress : nullptr;
  int ret;

  if (trig->extracted) /* an earlier trigger on the same thing removed this one */
    return 0;

  dg_ctx_begin(g, go, trig, type);
  ret    = script_driver_body(g, go_adress, trig, type, mode);
  g->dg_ctx = saved;
  if (trig->owner_gone && type == OBJ_TRIGGER)
    *(obj_data **)go_adress = nullptr; /* tell the caller, as dg_run_command() does */
  return ret;
}

static int script_driver_body(GameWorld *g, void *go_adress, trig_data *trig, int type, int mode)
{
  int ret_val      = 1;
  struct cmdlist_element *cl;
  char cmd[MAX_INPUT_LENGTH], *p;
  struct script_data *sc = nullptr;
  struct cmdlist_element *temp;
  unsigned long loops = 0;
  void *go            = *(void **)go_adress;

  sc = script_of(go, type);

  if (g->script_depth > MAX_SCRIPT_DEPTH) {
    script_log(g, "Trigger %d recursed beyond maximum allowed depth.", trig_get_vnum(g, trig));
    switch (type) {
      case MOB_TRIGGER:
        script_log(g, "It was attached to %s [%d]",
                   char_get_name((char_data *)go), char_get_mob_vnum(g, (char_data *)go));
        break;
      case OBJ_TRIGGER:
        script_log(g, "It was attached to %s [%d]",
                   obj_get_short_description((obj_data *)go), obj_get_vnum(g, (obj_data *)go));
        break;
      case WLD_TRIGGER:
        script_log(g, "It was attached to %s [%d]",
                   room_get_name((room_data *)go), room_get_number((room_data *)go));
        break;
    }

    extract_script(g, go, type);

    /*
       extract_script() works on rooms, but on mobiles and objects,
       it will be called again if the
       caller is load_mtrigger or load_otrigger
       if it is one of these, we must make sure the script
       is not just reloaded on the next mob

       We make the calling code decide how to handle it, so it doesn't
       get totally removed unless it's a load_xtrigger().
     */

    return SCRIPT_ERROR_CODE;
  }

  g->script_depth++;

  if (mode == TRIG_NEW) {
    trig->depth = 1;
    trig->loops = 0;
    sc->context = 0;
  }

  g->dg_owner_purged = 0;

  for (cl                    = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state;
       cl && trig->depth && !trig->extracted; cl = cl->next) {
    for (p = cl->cmd; *p && isspace(*p); p++);
    dg_ctx_set_line(g, p);

    if (*p == '*') /* comment */
      continue;

    else if (!strncasecmp(p, "if ", 3)) {
      if (process_if(g, p + 3, go, sc, trig, type))
        trig->depth++;
      else cl = find_else_end(g, trig, cl, go, sc, type);
    }

    else if (!strncasecmp("elseif ", p, 7) ||
             !strncasecmp("else", p, 4)) {
      /*
       * if not in an if-block, ignore the extra 'else[if]' and warn about it
       */
      if (trig->depth == 1) {
        script_log(g, "Trigger VNum %d has 'else' without 'if'.",
                   trig_get_vnum(g, trig));
        continue;
      }
      cl = find_end(g, trig, cl);
      trig->depth--;
    } else if (!strncasecmp("while ", p, 6)) {
      temp = find_done(cl);
      char *end = temp->cmd;
      while (*end && isspace(*end))
        end++;
      if (temp == cl || strncasecmp("done", end, 4)) {
        script_log(g, "Trigger VNum %d has 'while' without 'done'.",
                   trig_get_vnum(g, trig));
        break; /* end the run normally, so depth and the trigger's state are reset */
      }
      if (process_if(g, p + 6, go, sc, trig, type)) {
        temp->original = cl;
      } else {
        cl    = temp;
        loops = 0;
      }
    } else if (!strncasecmp("switch ", p, 7)) {
      cl = find_case(g, trig, cl, go, sc, type, p + 7);
    } else if (!strncasecmp("end", p, 3)) {
      /*
       * if not in an if-block, ignore the extra 'end' and warn about it.
       */
      if (trig->depth == 1) {
        script_log(g, "Trigger VNum %d has 'end' without 'if'.",
                   trig_get_vnum(g, trig));
        continue;
      }
      trig->depth--;
    } else if (!strncasecmp("done", p, 4)) {
      /* if in a while loop, cl->original is non-NULL */
      if (cl->original) {
        char *orig_cmd = cl->original->cmd;
        while (*orig_cmd && isspace(*orig_cmd)) orig_cmd++;
        if (cl->original && process_if(g, orig_cmd + 6, go, sc, trig,
                                       type)) {
          cl = cl->original;
          loops++;
          trig->loops++;
          if (loops == 40) {
            process_wait(g, go, trig, type, writable{"wait 1"}, cl);
            g->script_depth--;
            return ret_val;
          }
          if (trig->loops >= 5000) {
            script_log(g, "Trigger VNum %d has looped 5,000 times!!!",
                       trig_get_vnum(g, trig));
            break;
          }
        } else {
          /* if we're falling through a switch statement, this ends it. */
        }
      }
    } else if (!strncasecmp("break", p, 5)) {
      cl = find_done(cl);
    } else if (!strncasecmp("case", p, 4)) {
      /* Do nothing, this allows multiple cases to a single instance */
    }

    else {

      var_subst(g, go, sc, trig, type, p, cmd);

      enum dg_line_result res = dg_run_command(g, go_adress, go, sc, trig, type, cmd, cl, &ret_val);
      if (res == DG_LINE_HALT)
        break;
      if (res == DG_LINE_STOP) {
        g->script_depth--;
        return ret_val;
      }
    }
  }

  /* Once extracted, what the trigger belonged to may be gone, and free_trigger() has the rest. */
  if (!trig->extracted) {
    sc = script_of(go, type); /* the script may have been detached */
    if (sc)
      free_varlist(trig->var_list);
    trig->var_list = nullptr;
    trig->depth    = 0;
  }

  g->script_depth--;
  return ret_val;
}

/* returns the real number of the trigger with given virtual number */
static int trigger_order(const void *key, const void *trig)
{
  trig_vnum k = *(const trig_vnum *)key, v = (*(struct index_data *const *)trig)->vnum;

  return (k > v) - (k < v);
}

trig_rnum real_trigger(GameWorld *g, trig_vnum vnum)
{
  struct index_data **found;

  if (!g->trig_index || g->top_of_trigt <= 0)
    return NOTHING;
  found = (struct index_data **)bsearch(&vnum, g->trig_index, g->top_of_trigt, sizeof(*g->trig_index), trigger_order);
  return found ? found - g->trig_index : NOTHING;
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
  CREATE(*char_script_slot(ch), struct script_data, 1);

  /* find the file that holds the saved variables and open it*/
  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, char_get_name(ch));
  file = fopen(fn, "r");

  /* if we failed to open the file, return */
  if (!file) {
    mud_log("%s had no variable file", char_get_name(ch));
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
      add_var(&(char_get_script(ch)->global_vars), varname, temp, context);
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
  struct trig_var_data *vars;

  /* immediate return if no script (and therefore no variables) structure */
  /* has been created. this will happen when the player is logging in */
  if (char_get_script(ch) == nullptr) return;

  /* we should never be called for an NPC, but just in case... */
  if (char_is_npc(ch)) return;

  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, char_get_name(ch));

  /* make sure this char has global variables to save */
  if (char_get_script(ch)->global_vars == nullptr) {
    unlink(fn);
    return;
  }
  vars = char_get_script(ch)->global_vars;

  file = atomic_open(&af, fn, true);
  if (!file) {
    mudlog(g, LOG_NRM, ADMLVL_GOD, true,
           "SYSERR: Could not open player variable file %s for writing.:%s",
           fn, strerror(errno));
    return;
  }
  /* note that currently, context will always be zero. this may change */
  /* in the future */
  while (vars) {
    if (*vars->name != '-') /* don't save if it begins with - */
      fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
    vars = vars->next;
  }

  atomic_commit(&af);
}
