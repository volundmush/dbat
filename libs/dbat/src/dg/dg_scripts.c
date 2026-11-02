/**************************************************************************
*  File: dg_scripts.c                                                     *
*  Usage: contains the main script driver interface.                      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include "dbat/utils.h"
#include "dbat/dg_event.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/fileop.h"
#include "dbat/atomicfile.h"
#include "dg_internal.h"

#include <unistd.h>
#include <errno.h>

constexpr int PULSES_PER_MUD_HOUR = SECS_PER_MUD_HOUR * PASSES_PER_SEC;

struct index_data **trig_index;
struct trig_data *trigger_list;
int top_of_trigt;

/* Local functions not used elsewhere */
static void process_wait(void *go, trig_data *trig, int type, char *cmd,
                         struct cmdlist_element *cl);
static void process_set(struct script_data *sc, trig_data *trig, char *cmd);
static void process_attach(void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd);
static void process_detach(void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd);
static void makeuid_var(void *go, struct script_data *sc, trig_data *trig,
                        int type, char *cmd);
static int process_return(trig_data *trig, char *cmd);
static void process_unset(struct script_data *sc, trig_data *trig, char *cmd);
static void process_remote(struct script_data *sc, trig_data *trig, char *cmd);
static void process_rdelete(struct script_data *sc, trig_data *trig, char *cmd);
static void process_global(struct script_data *sc, trig_data *trig, char *cmd, long id);
static void process_context(struct script_data *sc, trig_data *trig, char *cmd);
static void extract_value(struct script_data *sc, trig_data *trig, char *cmd);
static void dg_letter_value(struct script_data *sc, trig_data *trig, char *cmd);
static EVENTFUNC(trig_wait_event);

/* checks every PULSE_SCRIPT for random triggers */
void script_trigger_check(void)
{
  char_data *ch;
  obj_data *obj;
  struct room_data *room = NULL;
  int nr;
  struct script_data *sc;

  for (ch = character_list; ch; ch = ch->next) {
    if (ch->script) {
      sc = ch->script;

      if (IS_SET(sc->types, WTRIG_RANDOM) &&
          (!is_empty(world[char_get_in_room(ch)].zone) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        random_mtrigger(ch);
    }
  }

  for (obj = object_list; obj; obj = obj->next) {
    if (obj->script) {
      sc = obj->script;

      if (IS_SET(sc->types, OTRIG_RANDOM))
        random_otrigger(obj);
    }
  }

  for (nr = 0; nr <= top_of_world; nr++) {
    if (world[nr].script) {
      room = &world[nr];
      sc   = room->script;

      if (IS_SET(sc->types, WTRIG_RANDOM) &&
          (!is_empty(room->zone) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        random_wtrigger(room);
    }
  }
}

void check_time_triggers(void)
{
  char_data *ch;
  obj_data *obj;
  struct room_data *room = NULL;
  int nr;
  struct script_data *sc;

  for (ch = character_list; ch; ch = ch->next) {
    if (ch->script) {
      sc = ch->script;

      if (IS_SET(sc->types, WTRIG_TIME) &&
          (!is_empty(world[char_get_in_room(ch)].zone) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        time_mtrigger(ch);
    }
  }

  for (obj = object_list; obj; obj = obj->next) {
    if (obj->script) {
      sc = obj->script;

      if (IS_SET(sc->types, OTRIG_TIME))
        time_otrigger(obj);
    }
  }

  for (nr = 0; nr <= top_of_world; nr++) {
    if (world[nr].script) {
      room = &world[nr];
      sc   = room->script;

      if (IS_SET(sc->types, WTRIG_TIME) &&
          (!is_empty(room->zone) ||
           IS_SET(sc->types, WTRIG_GLOBAL)))
        time_wtrigger(room);
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
  trig->wait_event = NULL;

#if 1 /* debugging */
  {
    int found = FALSE;
    if (type == MOB_TRIGGER) {
      struct char_data *tch;
      for (tch = character_list; tch && !found; tch = tch->next)
        if (tch == (struct char_data *)go)
          found = TRUE;
    } else if (type == OBJ_TRIGGER) {
      struct obj_data *obj;
      for (obj = object_list; obj && !found; obj = obj->next)
        if (obj == (struct obj_data *)go)
          found = TRUE;
    } else {
      room_rnum i;
      for (i = 0; i < top_of_world && !found; i++)
        if (&world[i] == (struct room_data *)go)
          found = TRUE;
    }
    if (!found) {
      mud_log("Trigger restarted on unknown entity. Vnum: %d", trig_get_vnum(trig));
      mud_log("Type: %s trigger", type == MOB_TRIGGER ? "Mob" : type == OBJ_TRIGGER ? "Obj"
                                                                                    : "Room");
      mud_log("attached %d places", trig_index[trig->nr]->number);
      script_log("Trigger restart attempt on unknown entity.");
      return 0;
    }
  }
#endif

  script_driver(&go, trig, type, TRIG_RESTART);

  /* Do not reenqueue*/
  return 0;
}

/*
 * The trigger currently being run by script_driver(), so errors can say which
 * trigger, which entity and which line caused them. Everything is copied in
 * because the owner may be purged while its script is still running.
 */
struct dg_run_ctx {
  bool active;
  long long trig_vnum;
  char trig_name[64];
  int owner_type;
  char owner_name[64];
  long long owner_vnum;
  long owner_uid;
  long long room_vnum;
  char line[256];
};
static struct dg_run_ctx dg_ctx;

static void dg_ctx_begin(void *go, trig_data *trig, int type)
{
  room_rnum room = NOWHERE;

  memset(&dg_ctx, 0, sizeof(dg_ctx));
  dg_ctx.active     = TRUE;
  dg_ctx.owner_type = type;
  dg_ctx.trig_vnum  = (long long)trig_get_vnum(trig);
  snprintf(dg_ctx.trig_name, sizeof(dg_ctx.trig_name), "%s", trig->name ? trig->name : "");
  dg_ctx.room_vnum = -1;

  if (!go)
    return;

  switch (type) {
    case MOB_TRIGGER:
      snprintf(dg_ctx.owner_name, sizeof(dg_ctx.owner_name), "%s", char_get_name((char_data *)go) ? char_get_name((char_data *)go) : "");
      dg_ctx.owner_vnum = (long long)char_get_mob_vnum((char_data *)go);
      dg_ctx.owner_uid  = char_get_id((char_data *)go);
      room              = char_get_in_room((char_data *)go);
      break;
    case OBJ_TRIGGER:
      snprintf(dg_ctx.owner_name, sizeof(dg_ctx.owner_name), "%s", ((obj_data *)go)->short_description ? ((obj_data *)go)->short_description : "");
      dg_ctx.owner_vnum = (long long)obj_get_vnum((obj_data *)go);
      dg_ctx.owner_uid  = obj_get_id((obj_data *)go);
      room              = obj_room((obj_data *)go);
      break;
    case WLD_TRIGGER:
      snprintf(dg_ctx.owner_name, sizeof(dg_ctx.owner_name), "%s", ((room_data *)go)->name ? ((room_data *)go)->name : "");
      dg_ctx.owner_vnum = (long long)((room_data *)go)->number;
      dg_ctx.owner_uid  = ((room_data *)go)->number + ROOM_ID_BASE;
      dg_ctx.room_vnum  = dg_ctx.owner_vnum;
      return;
  }

  if (room != NOWHERE && room >= 0 && room <= top_of_world)
    dg_ctx.room_vnum = (long long)world[room].number;
}

static void dg_ctx_set_line(const char *cmd)
{
  snprintf(dg_ctx.line, sizeof(dg_ctx.line), "%s", cmd ? cmd : "");
}

/* Writes " [trig ... ]" describing the running trigger, or "" when none is. */
void dg_context_str(char *buf, size_t len)
{
  static const char *owner_types[] = {"mob", "obj", "room"};

  if (!dg_ctx.active) {
    snprintf(buf, len, "%s", "");
    return;
  }

  snprintf(buf, len, " [trig #%lld '%s' on %s '%s' #%lld uid %ld in room %lld; line: %s]",
           dg_ctx.trig_vnum, dg_ctx.trig_name,
           (dg_ctx.owner_type >= 0 && dg_ctx.owner_type <= 2) ? owner_types[dg_ctx.owner_type] : "?",
           dg_ctx.owner_name, dg_ctx.owner_vnum, dg_ctx.owner_uid, dg_ctx.room_vnum,
           *dg_ctx.line ? dg_ctx.line : "(none)");
}

/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void script_vlog(const char *format, va_list args)
{
  char output[MAX_STRING_LENGTH];
  char print_to[MAX_STRING_LENGTH];
  char ctx[MAX_STRING_LENGTH];
  struct descriptor_data *i;

  vsnprintf(print_to, sizeof(print_to), format, args);
  dg_context_str(ctx, sizeof(ctx));

  mud_log("SCRIPT ERR: %s%s", print_to, ctx);

  /* the rest is mostly a rip from basic_mud_log() */
  snprintf(output, sizeof(output), "[ %s%s ]\r\n", print_to, ctx);

  for (i = descriptor_list; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < ADMLVL_BUILDER)
      continue;
    if (char_plr_flagged(i->character, PLR_WRITING))
      continue;
    if (LOG_NRM > (char_prf_flagged(i->character, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", output);
  }
}

void script_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  script_vlog(format, args);
  va_end(args);
}

/* processes any 'wait' commands in a trigger */
static void process_wait(void *go, trig_data *trig, int type, char *cmd,
                         struct cmdlist_element *cl)
{
  char buf[MAX_INPUT_LENGTH], *arg;
  struct wait_event_data *wait_event_obj;
  long when, hr, min, ntime;
  char c;

  arg = any_one_arg(cmd, buf);
  skip_spaces(&arg);

  if (!*arg) {
    script_log("Trigger: %s, VNum %d. wait w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cl->cmd);
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
    when = (pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
           (time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

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

  trig->wait_event = event_create(trig_wait_event, wait_event_obj, when);
  trig->curr_state = cl->next;
}

/* processes a script set command */
static void process_set(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

  value = two_arguments(cmd, arg, name);

  skip_spaces(&value);

  if (!*name) {
    script_log("Trigger: %s, VNum %d. set w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  add_var(&trig->var_list, name, value, sc ? sc->context : 0);
}

/* processes a script eval command */
void process_eval(void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *expr;

  expr = one_argument(cmd, arg);   /* cut off 'eval' */
  expr = one_argument(expr, name); /* cut off name */

  skip_spaces(&expr);

  if (!*name) {
    script_log("Trigger: %s, VNum %d. eval w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  eval_expr(expr, result, go, sc, trig, type);
  add_var(&trig->var_list, name, result, sc ? sc->context : 0);
}

/* script attaching a trigger to something */
static void process_attach(void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  trig_data *newtrig;
  char_data *c = NULL;
  obj_data *o  = NULL;
  room_data *r = NULL;
  long trignum, id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log("Trigger: %s, VNum %d. attach w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p) == 0) {
    script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }
  c = find_char(id);
  if (!c) {
    o = find_obj(id);
    if (!o) {
      r = find_room(id);
      if (!r) {
        script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
                   trig->name, trig_get_vnum(trig), cmd);
        return;
      }
    }
  }

  /* locate and load the trigger specified */
  trignum = real_trigger(atoi(trignum_s));
  if (trignum == NOTHING || !(newtrig = read_trigger(trignum))) {
    script_log("Trigger: %s, VNum %d. attach invalid trigger: '%s'",
               trig->name, trig_get_vnum(trig), trignum_s);
    return;
  }

  if (c) {
    if (!char_is_npc(c)) {
      script_log("Trigger: %s, VNum %d. attach invalid target: '%s'",
                 trig->name, trig_get_vnum(trig), char_get_name(c));
      return;
    }
    add_trigger(dg_ensure_script(&c->script), newtrig, -1);
    return;
  }

  if (o) {
    add_trigger(dg_ensure_script(&o->script), newtrig, -1);
    return;
  }

  if (r) {
    add_trigger(dg_ensure_script(&r->script), newtrig, -1);
    return;
  }
}

/* script detaching a trigger from something */
static void process_detach(void *go, struct script_data *sc, trig_data *trig,
                           int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  char_data *c = NULL;
  obj_data *o  = NULL;
  room_data *r = NULL;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log("Trigger: %s, VNum %d. detach w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p) == 0) {
    script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }
  c = find_char(id);
  if (!c) {
    o = find_obj(id);
    if (!o) {
      r = find_room(id);
      if (!r) {
        script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
                   trig->name, trig_get_vnum(trig), cmd);
        return;
      }
    }
  }

  if (c && c->script) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(c, MOB_TRIGGER);
      return;
    }
    if (remove_trigger(c->script, trignum_s)) {
      if (!c->script->trig_list) {
        extract_script(c, MOB_TRIGGER);
      }
    }
    return;
  }

  if (o && o->script) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(o, OBJ_TRIGGER);
      return;
    }
    if (remove_trigger(o->script, trignum_s)) {
      if (!o->script->trig_list) {
        extract_script(o, OBJ_TRIGGER);
      }
    }
    return;
  }

  if (r && r->script) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(r, WLD_TRIGGER);
      return;
    }
    if (remove_trigger(r->script, trignum_s)) {
      if (!r->script->trig_list) {
        extract_script(r, WLD_TRIGGER);
      }
    }
    return;
  }
}

struct room_data *dg_room_of_obj(struct obj_data *obj)
{
  if (obj_get_in_room(obj) != NOWHERE) return &world[obj_get_in_room(obj)];
  if (obj->carried_by) return &world[char_get_in_room(obj->carried_by)];
  if (obj->worn_by) return &world[char_get_in_room(obj->worn_by)];
  if (obj->in_obj) return (dg_room_of_obj(obj->in_obj));
  return NULL;
}

/* create a UID variable from the id number */
static void makeuid_var(void *go, struct script_data *sc, trig_data *trig,
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
    script_log("Trigger: %s, VNum %d. makeuid w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);

    return;
  }

  if (!*arg) {
    script_log("Trigger: %s, VNum %d. makeuid invalid id arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  if (atoi(arg) != 0) { /* easy, if you pass an id number */
    char result[MAX_INPUT_LENGTH];

    eval_expr(arg, result, go, sc, trig, type);
    snprintf(uid, sizeof(uid), "%c%s", UID_CHAR, result);
  } else { /* a lot more work without it */
    if (!*name) {
      script_log("Trigger: %s, VNum %d. makeuid needs name: '%s'",
                 trig->name, trig_get_vnum(trig), cmd);
      return;
    }

    if (is_abbrev(arg, "mob")) {
      struct char_data *c = NULL;
      switch (type) {
        case WLD_TRIGGER:
          c = get_char_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          c = get_char_near_obj((struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          c = get_char_room_vis((struct char_data *)go, name, NULL);
          break;
      }
      if (c)
        snprintf(uid, sizeof(uid), "%c%d", UID_CHAR, char_get_id(c));
    } else if (is_abbrev(arg, "obj")) {
      struct obj_data *o = NULL;
      switch (type) {
        case WLD_TRIGGER:
          o = get_obj_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          o = get_obj_near_obj((struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          if ((o = get_obj_in_list_vis((struct char_data *)go, name, NULL,
                                       ((struct char_data *)go)->carrying)) == NULL)
            o = get_obj_in_list_vis((struct char_data *)go, name, NULL,
                                    world[char_get_in_room((struct char_data *)go)].contents);
          break;
      }
      if (o)
        snprintf(uid, sizeof(uid), "%c%d", UID_CHAR, obj_get_id(o));
    } else if (is_abbrev(arg, "room")) {
      room_rnum r = NOWHERE;
      switch (type) {
        case WLD_TRIGGER:
          r = real_room(((struct room_data *)go)->number);
          break;
        case OBJ_TRIGGER:
          r = obj_room((struct obj_data *)go);
          break;
        case MOB_TRIGGER:
          r = char_get_in_room((struct char_data *)go);
          break;
      }
      if (r != NOWHERE)
        snprintf(uid, sizeof(uid), "%c%d", UID_CHAR, world[r].number + ROOM_ID_BASE);
    } else {
      script_log("Trigger: %s, VNum %d. makeuid syntax error: '%s'",
                 trig->name, trig_get_vnum(trig), cmd);

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
static int process_return(trig_data *trig, char *cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(cmd, arg1, arg2);

  if (!*arg2) {
    script_log("Trigger: %s, VNum %d. return w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);

    return 1;
  }

  return atoi(arg2);
}

/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
static void process_unset(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. unset w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  if (!remove_var(&(sc->global_vars), var))
    remove_var(&trig->var_list, var);
}

/*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
static void process_remote(struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd;
  struct script_data *sc_remote = NULL;
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
    script_log("Trigger: %s, VNum %d. remote: invalid arguments '%s'",
               trig->name, trig_get_vnum(trig), cmd);
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
    script_log("Trigger: %s, VNum %d. local var '%s' not found in remote call",
               trig->name, trig_get_vnum(trig), buf);
    return;
  }
  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    script_log("Trigger: %s, VNum %d. remote: illegal uid '%s'",
               trig->name, trig_get_vnum(trig), buf2);
    return;
  }

  /* for all but PC's, context comes from the existing context. */
  /* for PC's, context is 0 (global) */
  context = vd->context;

  if ((room = find_room(uid))) {
    sc_remote = room->script;
  } else if ((mob = find_char(uid))) {
    sc_remote = mob->script;
    if (!char_is_npc(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = obj->script;
  } else {
    script_log("Trigger: %s, VNum %d. remote: uid '%ld' invalid",
               trig->name, trig_get_vnum(trig), uid);
    return;
  }

  if (sc_remote == NULL) return; /* no script to assign */

  add_var(&(sc_remote->global_vars), vd->name, vd->value, context);
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
static void process_rdelete(struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd, *vd_prev = NULL;
  struct script_data *sc_remote = NULL;
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
    script_log("Trigger: %s, VNum %d. rdelete: invalid arguments '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    script_log("Trigger: %s, VNum %d. rdelete: illegal uid '%s'",
               trig->name, trig_get_vnum(trig), buf2);
    return;
  }

  if ((room = find_room(uid))) {
    sc_remote = room->script;
  } else if ((mob = find_char(uid))) {
    sc_remote = mob->script;
  } else if ((obj = find_obj(uid))) {
    sc_remote = obj->script;
  } else {
    script_log("Trigger: %s, VNum %d. remote: uid '%ld' invalid",
               trig->name, trig_get_vnum(trig), uid);
    return;
  }

  if (sc_remote == NULL) return;              /* no script to delete a trigger from */
  if (sc_remote->global_vars == NULL) return; /* no script globals */

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
static void process_global(struct script_data *sc, trig_data *trig, char *cmd, long id)
{
  struct trig_var_data *vd;
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. global w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  for (vd = trig->var_list; vd; vd = vd->next)
    if (!strcasecmp(vd->name, var))
      break;

  if (!vd) {
    script_log("Trigger: %s, VNum %d. local var '%s' not found in global call",
               trig->name, trig_get_vnum(trig), var);
    return;
  }

  add_var(&(sc->global_vars), vd->name, vd->value, id);
  remove_var(&trig->var_list, vd->name);
}

/* set the current context for a script */
static void process_context(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. context w/o an arg: '%s'",
               trig->name, trig_get_vnum(trig), cmd);
    return;
  }

  sc->context = atol(var);
}

static void extract_value(struct script_data *sc, trig_data *trig, char *cmd)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  char *buf3;
  char to[128];
  int num;

  buf3 = any_one_arg(cmd, buf);
  half_chop(buf3, buf2, buf);
  if (strlen(buf2) >= sizeof(to)) {
    script_log("extract: variable name is longer than %zu characters", sizeof(to) - 1);
    return;
  }
  strcpy(to, buf2); /* strcpy: OK (length checked above) */

  num = atoi(buf);
  if (num < 1) {
    script_log("extract number < 1!");
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

static void dg_letter_value(struct script_data *sc, trig_data *trig, char *cmd)
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

  script_log("The use of dg_letter is deprecated");
  script_log("- Use 'set <new variable> %%<text/var>.charat(index)%%' instead.");

  if (num < 1) {
    script_log("Trigger #%d : dg_letter number < 1!", trig_get_vnum(trig));
    return;
  }

  if ((size_t)num > strlen(string)) {
    script_log("Trigger #%d : dg_letter number > strlen!", trig_get_vnum(trig));
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
  return slot ? *slot : NULL;
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
static enum dg_line_result dg_run_command(void *go_adress, void *go, struct script_data *sc, trig_data *trig,
                                          int type, char *cmd, struct cmdlist_element *cl, int *ret_val)
{
  if (!strncasecmp(cmd, "eval ", 5))
    process_eval(go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "nop ", 4))
    ; /* nop: do nothing */

  else if (!strncasecmp(cmd, "extract ", 8))
    extract_value(sc, trig, cmd);

  else if (!strncasecmp(cmd, "dg_letter ", 10))
    dg_letter_value(sc, trig, cmd);

  else if (!strncasecmp(cmd, "makeuid ", 8))
    makeuid_var(go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "halt", 4))
    return DG_LINE_HALT;

  else if (!strncasecmp(cmd, "dg_cast ", 8))
    do_dg_cast(go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "dg_affect ", 10))
    do_dg_affect(go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "global ", 7))
    process_global(sc, trig, cmd, sc->context);

  else if (!strncasecmp(cmd, "context ", 8))
    process_context(sc, trig, cmd);

  else if (!strncasecmp(cmd, "remote ", 7))
    process_remote(sc, trig, cmd);

  else if (!strncasecmp(cmd, "rdelete ", 8))
    process_rdelete(sc, trig, cmd);

  else if (!strncasecmp(cmd, "return ", 7))
    *ret_val = process_return(trig, cmd);

  else if (!strncasecmp(cmd, "set ", 4))
    process_set(sc, trig, cmd);

  else if (!strncasecmp(cmd, "unset ", 6))
    process_unset(sc, trig, cmd);

  else if (!strncasecmp(cmd, "wait ", 5)) {
    process_wait(go, trig, type, cmd, cl);
    return DG_LINE_STOP;
  }

  else if (!strncasecmp(cmd, "attach ", 7))
    process_attach(go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "detach ", 7))
    process_detach(go, sc, trig, type, cmd);

  else if (!strncasecmp(cmd, "version", 7))
    mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "%s", DG_SCRIPT_VERSION);

  else {
    switch (type) {
      case MOB_TRIGGER:
        command_interpreter((char_data *)go, cmd);
        break;
      case OBJ_TRIGGER:
        obj_command_interpreter((obj_data *)go, cmd);
        break;
      case WLD_TRIGGER:
        wld_command_interpreter((struct room_data *)go, cmd);
        break;
    }
    if (dg_owner_purged) {
      if (type == OBJ_TRIGGER)
        *(obj_data **)go_adress = NULL;
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
static int script_driver_body(void *go_adress, trig_data *trig, int type, int mode);

/* Runs a trigger with dg_ctx describing it, restoring the caller's context after. */
int script_driver(void *go_adress, trig_data *trig, int type, int mode)
{
  struct dg_run_ctx saved = dg_ctx;
  void *go                = go_adress ? *(void **)go_adress : NULL;
  int ret;

  dg_ctx_begin(go, trig, type);
  ret    = script_driver_body(go_adress, trig, type, mode);
  dg_ctx = saved;
  return ret;
}

static int script_driver_body(void *go_adress, trig_data *trig, int type, int mode)
{
  static int depth = 0;
  int ret_val      = 1;
  struct cmdlist_element *cl;
  char cmd[MAX_INPUT_LENGTH], *p;
  struct script_data *sc = 0;
  struct cmdlist_element *temp;
  unsigned long loops = 0;
  void *go            = *(void **)go_adress;

  sc = script_of(go, type);

  if (depth > MAX_SCRIPT_DEPTH) {
    script_log("Trigger %d recursed beyond maximum allowed depth.", trig_get_vnum(trig));
    switch (type) {
      case MOB_TRIGGER:
        script_log("It was attached to %s [%d]",
                   char_get_name((char_data *)go), char_get_mob_vnum((char_data *)go));
        break;
      case OBJ_TRIGGER:
        script_log("It was attached to %s [%d]",
                   ((obj_data *)go)->short_description, obj_get_vnum((obj_data *)go));
        break;
      case WLD_TRIGGER:
        script_log("It was attached to %s [%d]",
                   ((room_data *)go)->name, ((room_data *)go)->number);
        break;
    }

    extract_script(go, type);

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

  depth++;

  if (mode == TRIG_NEW) {
    trig->depth = 1;
    trig->loops = 0;
    sc->context = 0;
  }

  dg_owner_purged = 0;

  for (cl                    = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state;
       cl && trig->depth; cl = cl->next) {
    for (p = cl->cmd; *p && isspace(*p); p++);
    dg_ctx_set_line(p);

    if (*p == '*') /* comment */
      continue;

    else if (!strncasecmp(p, "if ", 3)) {
      if (process_if(p + 3, go, sc, trig, type))
        trig->depth++;
      else cl = find_else_end(trig, cl, go, sc, type);
    }

    else if (!strncasecmp("elseif ", p, 7) ||
             !strncasecmp("else", p, 4)) {
      /*
       * if not in an if-block, ignore the extra 'else[if]' and warn about it
       */
      if (trig->depth == 1) {
        script_log("Trigger VNum %d has 'else' without 'if'.",
                   trig_get_vnum(trig));
        continue;
      }
      cl = find_end(trig, cl);
      trig->depth--;
    } else if (!strncasecmp("while ", p, 6)) {
      temp = find_done(cl);
      if (!temp) {
        script_log("Trigger VNum %d has 'while' without 'done'.",
                   trig_get_vnum(trig));
        break; /* end the run normally, so depth and the trigger's state are reset */
      }
      if (process_if(p + 6, go, sc, trig, type)) {
        temp->original = cl;
      } else {
        cl    = temp;
        loops = 0;
      }
    } else if (!strncasecmp("switch ", p, 7)) {
      cl = find_case(trig, cl, go, sc, type, p + 7);
    } else if (!strncasecmp("end", p, 3)) {
      /*
       * if not in an if-block, ignore the extra 'end' and warn about it.
       */
      if (trig->depth == 1) {
        script_log("Trigger VNum %d has 'end' without 'if'.",
                   trig_get_vnum(trig));
        continue;
      }
      trig->depth--;
    } else if (!strncasecmp("done", p, 4)) {
      /* if in a while loop, cl->original is non-NULL */
      if (cl->original) {
        char *orig_cmd = cl->original->cmd;
        while (*orig_cmd && isspace(*orig_cmd)) orig_cmd++;
        if (cl->original && process_if(orig_cmd + 6, go, sc, trig,
                                       type)) {
          cl = cl->original;
          loops++;
          trig->loops++;
          if (loops == 40) {
            process_wait(go, trig, type, (char[]){"wait 1"}, cl);
            depth--;
            return ret_val;
          }
          if (trig->loops >= 5000) {
            script_log("Trigger VNum %d has looped 5,000 times!!!",
                       trig_get_vnum(trig));
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

      var_subst(go, sc, trig, type, p, cmd);

      enum dg_line_result res = dg_run_command(go_adress, go, sc, trig, type, cmd, cl, &ret_val);
      if (res == DG_LINE_HALT)
        break;
      if (res == DG_LINE_STOP) {
        depth--;
        return ret_val;
      }
    }
  }

  sc = script_of(go, type); /* the script may have been detached */
  if (sc)
    free_varlist(trig->var_list);
  trig->var_list = NULL;
  trig->depth    = 0;

  depth--;
  return ret_val;
}

/* returns the real number of the trigger with given virtual number */
trig_rnum real_trigger(trig_vnum vnum)
{
  trig_rnum bot, top, mid;

  bot = 0;
  top = top_of_trigt - 1;

  if (!top_of_trigt || trig_index[bot]->vnum > vnum || trig_index[top]->vnum < vnum)
    return (NOTHING);

  /* perform binary search on trigger-table */
  while (bot <= top) {
    mid = (bot + top) / 2;

    if (trig_index[mid]->vnum == vnum)
      return (mid);
    if (trig_index[mid]->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOTHING);
}

/* load in a character's saved variables */
void read_saved_vars(struct char_data *ch)
{
  FILE *file;
  long context;
  char fn[127];
  char input_line[1024], *temp, *p;
  char varname[32];
  char context_str[16];

  /*
   * If getting to the menu from inside the game, the vars aren't removed.
   * So let's not allocate them again.
   */
  if (ch->script)
    return;

  /* create the space for the script structure which holds the vars */
  /* We need to do this first, because later calls to 'remote' will need */
  /* a script already assigned. */
  CREATE(ch->script, struct script_data, 1);

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
      add_var(&(ch->script->global_vars), varname, temp, context);
      free(p); /* plug memory hole */
    }
  } while (!feof(file));

  /* close the file and return */
  fclose(file);
}

/* save a characters variables out to disk */
void save_char_vars(struct char_data *ch)
{
  struct atomic_file af;
  FILE *file;
  char fn[127];
  struct trig_var_data *vars;

  /* immediate return if no script (and therefore no variables) structure */
  /* has been created. this will happen when the player is logging in */
  if (ch->script == NULL) return;

  /* we should never be called for an NPC, but just in case... */
  if (char_is_npc(ch)) return;

  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, char_get_name(ch));

  /* make sure this char has global variables to save */
  if (ch->script->global_vars == NULL) {
    unlink(fn);
    return;
  }
  vars = ch->script->global_vars;

  file = atomic_open(&af, fn, true);
  if (!file) {
    mudlog(LOG_NRM, ADMLVL_GOD, TRUE,
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
