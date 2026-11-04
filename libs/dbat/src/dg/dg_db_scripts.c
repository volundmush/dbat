/* ************************************************************************
*  File: dg_db_scripts.c                         Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */
#include "dbat/consts/maximums.h"
#include "dbat/utils.h"
#include "dbat/log.h"

#include "dbat/dg_scripts.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/comm.h"
#include "dbat/fileop.h"
#include "dbat/stringutils.h"
#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"

#include <stdlib.h>
#include "dbat/game_world.h"

/* local functions */
static void trig_data_init(trig_data *this_data);

/* the block a script line opens, for dg_lint_trigger() */
struct dg_lint_block {
  const char *kind; /* "if", "while" or "switch" */
  int line;
};

static void dg_lint_warn(GameWorld *g, const trig_data *trig, struct descriptor_data *d, int line, const char *msg)
{
  mud_log("SCRIPT WARN: trig #%d '%s' line %d: %s", trig_get_vnum(g, trig),
          trig->name ? trig->name : "", line, msg);
  if (d)
    write_to_output(g, d, "Warning: line %d: %s\r\n", line, msg);
}

/*
 * Warns about control-flow mistakes in a trigger's script: a block that is never
 * closed, a closer with nothing to close, and 'else if', which runs as a plain 'else'.
 * Keywords are matched the way script_driver_body() matches them, so the warnings
 * describe what will actually run. Warnings go to the syslog, and also to d if it is
 * not NULL. Returns the number of warnings.
 */
int dg_lint_trigger(GameWorld *g, const trig_data *trig, struct descriptor_data *d)
{
  struct dg_lint_block open[64];
  int depth = 0, line = 0, warnings = 0;
  char msg[MAX_INPUT_LENGTH];

  for (const struct cmdlist_element *cl = trig->cmdlist; cl; cl = cl->next) {
    const char *p = cl->cmd;
    line++;
    while (*p && isspace(*p))
      p++;

    if (*p == '*') /* comment */
      continue;

    if (!strncasecmp(p, "if ", 3) || !strncasecmp("while ", p, 6) || !strncasecmp("switch ", p, 7)) {
      if (depth == (int)(countof(open))) {
        dg_lint_warn(g, trig, d, line, "blocks nested too deeply to check the rest of the script");
        return warnings + 1;
      }
      if (!strncasecmp(p, "if ", 3))
        open[depth].kind = "if";
      else if (!strncasecmp(p, "while ", 6))
        open[depth].kind = "while";
      else
        open[depth].kind = "switch";
      open[depth++].line = line;
    } else if (!strncasecmp("elseif ", p, 7) || !strncasecmp("else", p, 4)) {
      const char *q = p + 4;
      while (*q && isspace(*q))
        q++;
      if (q > p + 4 && !strncasecmp(q, "if", 2) && (!q[2] || q[2] == '(' || isspace(q[2]))) {
        dg_lint_warn(g, trig, d, line, "'else if' runs as a plain 'else' and its condition is ignored; write 'elseif'");
        warnings++;
      }
      if (!depth || strcmp(open[depth - 1].kind, "if")) {
        dg_lint_warn(g, trig, d, line, "'else' without 'if'");
        warnings++;
      }
    } else if (!strncasecmp("end", p, 3)) {
      if (depth && !strcmp(open[depth - 1].kind, "if"))
        depth--;
      else {
        dg_lint_warn(g, trig, d, line, "'end' without 'if'");
        warnings++;
      }
    } else if (!strncasecmp("done", p, 4)) {
      while (depth && !strcmp(open[depth - 1].kind, "if")) {
        snprintf(msg, sizeof(msg), "'if' has no 'end' before the 'done' at line %d", line);
        dg_lint_warn(g, trig, d, open[--depth].line, msg);
        warnings++;
      }
      if (depth)
        depth--;
      else {
        dg_lint_warn(g, trig, d, line, "'done' without 'while' or 'switch'");
        warnings++;
      }
    }
  }

  while (depth) {
    depth--;
    snprintf(msg, sizeof(msg), "'%s' has no '%s'", open[depth].kind, strcmp(open[depth].kind, "if") ? "done" : "end");
    dg_lint_warn(g, trig, d, open[depth].line, msg);
    warnings++;
  }
  return warnings;
}

void parse_trigger(GameWorld *g, FILE *trig_f, trig_vnum nr)
{
  int t[2], k, attach_type;
  char line[256], *cmds, *s, flags[256], errors[MAX_INPUT_LENGTH];
  struct cmdlist_element *cle;
  struct index_data *t_index;
  struct trig_data *trig;

  CREATE(trig, struct trig_data, 1);
  CREATE(t_index, struct index_data, 1);

  t_index->vnum  = nr;
  t_index->proto = trig;

  snprintf(errors, sizeof(errors), "trig vnum %d", nr);

  trig->nr   = g->top_of_trigt;
  trig->name = fread_string(trig_f, errors);

  get_line(trig_f, line);
  k                  = sscanf(line, "%d %255s %d", &attach_type, flags, t);
  trig->attach_type  = (int8_t)attach_type;
  trig->trigger_type = (long)asciiflag_conv(flags);
  trig->narg         = (k == 3) ? t[0] : 0;

  trig->arglist = fread_string(trig_f, errors);

  /* an empty or blank script becomes the same placeholder trigedit saves */
  cmds = fread_string(trig_f, errors);
  s    = cmds ? strtok(cmds, "\n\r") : nullptr;

  CREATE(trig->cmdlist, struct cmdlist_element, 1);
  trig->cmdlist->cmd = strdup(s ? s : "* Empty script");
  cle                = trig->cmdlist;

  while (s && (s = strtok(nullptr, "\n\r"))) {
    CREATE(cle->next, struct cmdlist_element, 1);
    cle      = cle->next;
    cle->cmd = strdup(s);
  }

  free(cmds);

  g->trig_index[g->top_of_trigt++] = t_index;

  dg_lint_trigger(g, trig, nullptr);
}

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
trig_data *read_trigger(GameWorld *g, trig_rnum nr)
{
  index_data *t_index;
  trig_data *trig;

  if (nr >= g->top_of_trigt) return nullptr;
  if ((t_index = g->trig_index[nr]) == nullptr)
    return nullptr;

  CREATE(trig, trig_data, 1);
  trig_data_copy(trig, t_index->proto);

  t_index->number++;

  return trig;
}

static void trig_data_init(trig_data *this_data)
{
  this_data->nr           = NOTHING;
  this_data->data_type    = 0;
  this_data->name         = nullptr;
  this_data->trigger_type = 0;
  this_data->cmdlist      = nullptr;
  this_data->curr_state   = nullptr;
  this_data->narg         = 0;
  this_data->arglist      = nullptr;
  this_data->depth        = 0;
  this_data->wait_event   = nullptr;
  this_data->purged       = false;
  this_data->var_list     = nullptr;

  this_data->next = nullptr;
}

void trig_data_copy(trig_data *this_data, const trig_data *trg)
{
  trig_data_init(this_data);

  this_data->nr          = trg->nr;
  this_data->attach_type = trg->attach_type;
  this_data->data_type   = trg->data_type;
  if (trg->name)
    this_data->name = strdup(trg->name);
  else {
    this_data->name = strdup("unnamed trigger");
    mud_log("Trigger with no name! (%d)", trg->nr);
  }
  this_data->trigger_type = trg->trigger_type;
  this_data->cmdlist      = trg->cmdlist;
  this_data->narg         = trg->narg;
  if (trg->arglist) this_data->arglist = strdup(trg->arglist);
}

/* for mobs and rooms: */
void dg_read_trigger(GameWorld *g, FILE *fp, void *proto, int type)
{
  char line[READ_SIZE];
  char junk[8];
  trig_vnum vnum;
  trig_rnum rnum;
  int count;
  char_data *mob;
  room_data *room;
  struct trig_proto_list *trg_proto, *new_trg;

  get_line(fp, line);
  count = sscanf(line, "%7s %d", junk, &vnum);

  if (count != 2) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
           "SYSERR: Error assigning trigger! - Line was\n  %s", line);
    return;
  }

  rnum = real_trigger(g, vnum);
  if (rnum == NOTHING) {
    switch (type) {
      case MOB_TRIGGER:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (mob: %s - %d)",
               vnum, char_get_name((char_data *)proto), char_get_mob_vnum(g, (char_data *)proto));
        break;
      case WLD_TRIGGER:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (room:%d)",
               vnum, rroom_get_vnum(g, room_get_number((room_data *)proto)));
        break;
      default:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (?)", vnum);
        break;
    }
    return;
  }

  switch (type) {
    case MOB_TRIGGER:
      CREATE(new_trg, struct trig_proto_list, 1);
      new_trg->vnum = vnum;

      mob       = (char_data *)proto;
      trg_proto = char_get_proto_script(mob);
      if (!trg_proto) {
        char_set_proto_script(mob, trg_proto = new_trg);
      } else {
        while (trg_proto->next)
          trg_proto = trg_proto->next;
        trg_proto->next = new_trg;
      }
      break;
    case WLD_TRIGGER:
      CREATE(new_trg, struct trig_proto_list, 1);
      new_trg->vnum = vnum;
      room          = (room_data *)proto;
      trg_proto     = room_get_proto_script(room);
      if (!trg_proto) {
        room_set_proto_script(room, trg_proto = new_trg);
      } else {
        while (trg_proto->next)
          trg_proto = trg_proto->next;
        trg_proto->next = new_trg;
      }

      if (rnum != NOTHING) {
        add_trigger(g, dg_ensure_script(room_script_slot(room)), read_trigger(g, rnum), -1);
      } else {
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: non-existant trigger #%d assigned to room #%d",
               vnum, room_get_number(room));
      }
      break;
    default:
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
             "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
  }
}

void dg_obj_trigger(GameWorld *g, char *line, struct obj_data *obj)
{
  char junk[8];
  trig_vnum vnum;
  trig_rnum rnum;
  int count;
  struct trig_proto_list *trg_proto, *new_trg;

  count = sscanf(line, "%7s %d", junk, &vnum);

  if (count != 2) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
           "SYSERR: dg_obj_trigger() : Error assigning trigger! - Line was:\n  %s", line);
    return;
  }

  rnum = real_trigger(g, vnum);
  if (rnum == NOTHING) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
           "SYSERR: Trigger vnum #%d asked for but non-existant! (Object: %s - %d)",
           vnum, obj_get_short_description(obj), obj_get_vnum(g, obj));
    return;
  }

  CREATE(new_trg, struct trig_proto_list, 1);
  new_trg->vnum = vnum;

  trg_proto = obj_get_proto_script(obj);
  if (!trg_proto) {
    obj_set_proto_script(obj, trg_proto = new_trg);
  } else {
    while (trg_proto->next) trg_proto = trg_proto->next;
    trg_proto->next = new_trg;
  }
}

void assign_triggers(GameWorld *g, void *i, int type)
{
  struct char_data *mob  = nullptr;
  struct obj_data *obj   = nullptr;
  struct room_data *room = nullptr;
  trig_rnum rnum;
  struct trig_proto_list *trg_proto;

  switch (type) {
    case MOB_TRIGGER:
      mob       = (char_data *)i;
      trg_proto = char_get_proto_script(mob);
      while (trg_proto) {
        rnum = real_trigger(g, trg_proto->vnum);
        if (rnum == NOTHING) {
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
                 "SYSERR: trigger #%d non-existant, for mob #%d",
                 trg_proto->vnum, g->mob_index[char_get_nr(mob)].vnum);
        } else {
          add_trigger(g, dg_ensure_script(char_script_slot(mob)), read_trigger(g, rnum), -1);
        }
        trg_proto = trg_proto->next;
      }
      break;
    case OBJ_TRIGGER:
      obj       = (obj_data *)i;
      trg_proto = obj_get_proto_script(obj);
      while (trg_proto) {
        rnum = real_trigger(g, trg_proto->vnum);
        if (rnum == NOTHING) {
          mud_log("SYSERR: trigger #%d non-existant, for obj #%d",
                  trg_proto->vnum, g->obj_index[obj_get_item_number(obj)].vnum);
        } else {
          add_trigger(g, dg_ensure_script(obj_script_slot(obj)), read_trigger(g, rnum), -1);
        }
        trg_proto = trg_proto->next;
      }
      break;
    case WLD_TRIGGER:
      room      = (struct room_data *)i;
      trg_proto = room_get_proto_script(room);
      while (trg_proto) {
        rnum = real_trigger(g, trg_proto->vnum);
        if (rnum == NOTHING) {
          mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
                 "SYSERR: trigger #%d non-existant, for room #%d",
                 trg_proto->vnum, room_get_number(room));
        } else {
          add_trigger(g, dg_ensure_script(room_script_slot(room)), read_trigger(g, rnum), -1);
        }
        trg_proto = trg_proto->next;
      }
      break;
    default:
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
             "SYSERR: unknown type for assign_triggers()");
      break;
  }
}
