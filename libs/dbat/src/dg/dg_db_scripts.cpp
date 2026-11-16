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
#include <cstdlib>
#include <memory>

#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/log.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/comm.hpp"
#include "dbat/fileop.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

/* local functions */
static void trig_data_init(trig_data *this_data);

void parse_trigger(GameWorld *g, FILE *trig_f, trig_vnum nr)
{
  int t[2], k, attach_type;
  char line[256], *cmds, flags[256];
  std::string errors;
  struct index_data *t_index;
  struct trig_data *trig;

  trig                             = new trig_data();
  std::unique_ptr<index_data> made = std::make_unique<index_data>();
  t_index                          = made.get();

  t_index->vnum  = nr;
  t_index->proto = trig;

  errors = str_format("trig vnum {:d}", nr);

  trig->index = t_index;
  trig->name  = take_string(fread_string(trig_f, errors.c_str()));

  get_line(trig_f, line);
  k                  = sscanf(line, "%d %255s %d", &attach_type, flags, t);
  trig->attach_type  = (int8_t)attach_type;
  trig->trigger_type = (long)asciiflag_conv(flags);
  trig->narg         = (k == 3) ? t[0] : 0;

  trig->arglist = take_string(fread_string(trig_f, errors.c_str()));

  /* an empty script gets a placeholder */
  cmds          = fread_string(trig_f, errors.c_str());
  trig->program = dg::lex(cmds ? cmds : "", "* Empty script");
  free(cmds);

  if (g->trig_index.find(nr)) {
    mud_log("SYSERR: trigger #{:d} is defined twice; ignoring the second one.", nr);
    free_trigger(g, trig);
    return;
  }
  g->trig_index.insert(nr, std::move(made));

  report_compile_errors(g, trig, nullptr);
}

/*
 * create a new trigger from a prototype, given its entry in the trigger index (or null).
 */
trig_data *read_trigger(GameWorld *g, struct index_data *nr)
{
  index_data *t_index;
  trig_data *trig;

  if ((t_index = nr) == nullptr)
    return nullptr;

  trig = new trig_data();
  trig_data_copy(trig, t_index->proto);

  t_index->number++;

  return trig;
}

static void trig_data_init(trig_data *this_data)
{
  this_data->index        = nullptr;
  this_data->data_type    = 0;
  this_data->name.clear();
  this_data->trigger_type = 0;
  this_data->program.reset();
  this_data->run.stop();
  this_data->refusal_logged = false;
  this_data->narg         = 0;
  this_data->arglist.clear();
  this_data->wait_event   = nullptr;
  this_data->purged       = false;
  this_data->var_list.clear();

  this_data->next = nullptr;
}

void trig_data_copy(trig_data *this_data, const trig_data *trg)
{
  trig_data_init(this_data);

  this_data->index       = trg->index;
  this_data->attach_type = trg->attach_type;
  this_data->data_type   = trg->data_type;
  if (!trg->name.empty())
    this_data->name = trg->name;
  else {
    this_data->name = "unnamed trigger";
    mud_log("Trigger with no name! ({:d})", trg->index ? trg->index->vnum : NOTHING);
  }
  this_data->trigger_type = trg->trigger_type;
  this_data->program      = trg->program;
  this_data->narg         = trg->narg;
  this_data->arglist      = trg->arglist;
}

/* for mobs and rooms: */
void dg_read_trigger(GameWorld *g, FILE *fp, owner_ptr proto)
{
  int type = owner_type(proto);
  char line[READ_SIZE];
  char junk[8];
  trig_vnum vnum;
  struct index_data *rnum;
  int count;
  room_data *room;

  get_line(fp, line);
  count = sscanf(line, "%7s %d", junk, &vnum);

  if (count != 2) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
           "SYSERR: Error assigning trigger! - Line was\n  {}", line);
    return;
  }

  rnum = find_trigger(g, vnum);
  if (rnum == nullptr) {
    switch (type) {
      case MOB_TRIGGER:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_read_trigger: Trigger vnum #{:d} asked for but non-existant! (mob: {} - {:d})",
               vnum, char_get_name(std::get<char_data *>(proto)), char_get_mob_vnum(g, std::get<char_data *>(proto)));
        break;
      case WLD_TRIGGER:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_read_trigger: Trigger vnum #{:d} asked for but non-existant! (room:{:d})",
               vnum, room_get_number(std::get<room_data *>(proto)));
        break;
      default:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_read_trigger: Trigger vnum #{:d} asked for but non-existant! (?)", vnum);
        break;
    }
    return;
  }

  switch (type) {
    case MOB_TRIGGER:
      char_proto_script(std::get<char_data *>(proto)).push_back(vnum);
      break;
    case WLD_TRIGGER:
      room = std::get<room_data *>(proto);
      room_proto_script(room).push_back(vnum);

      if (rnum != nullptr) {
        add_trigger(g, dg_ensure_script(room_script_slot(room)), read_trigger(g, rnum), -1);
      } else {
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
               "SYSERR: non-existant trigger #{:d} assigned to room #{:d}",
               vnum, room_get_number(room));
      }
      break;
    default:
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
             "SYSERR: Trigger vnum #{:d} assigned to non-mob/obj/room", vnum);
  }
}

void dg_obj_trigger(GameWorld *g, char *line, struct obj_data *obj)
{
  char junk[8];
  trig_vnum vnum;
  struct index_data *rnum;
  int count;

  count = sscanf(line, "%7s %d", junk, &vnum);

  if (count != 2) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
           "SYSERR: dg_obj_trigger() : Error assigning trigger! - Line was:\n  {}", line);
    return;
  }

  rnum = find_trigger(g, vnum);
  if (rnum == nullptr) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true,
           "SYSERR: Trigger vnum #{:d} asked for but non-existant! (Object: {} - {:d})",
           vnum, obj_get_short_description(obj), obj_get_vnum(g, obj));
    return;
  }

  obj_proto_script(obj).push_back(vnum);
}

void assign_triggers(GameWorld *g, owner_ptr i)
{
  for (trig_vnum vnum : proto_script_of(i)) {
    struct index_data *rnum = find_trigger(g, vnum);
    if (rnum) {
      add_trigger(g, dg_ensure_script(script_slot(i)), read_trigger(g, rnum), -1);
      continue;
    }
    std::visit(overloaded{[&](char_data *mob) {
                            mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: trigger #{:d} non-existant, for mob #{:d}",
                                   vnum, mob_index_of(g, char_get_nr(mob))->vnum);
                          },
                          [&](obj_data *obj) {
                            mud_log("SYSERR: trigger #{:d} non-existant, for obj #{:d}",
                                    vnum, obj_index_of(g, obj_get_item_number(obj))->vnum);
                          },
                          [&](room_data *room) {
                            mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: trigger #{:d} non-existant, for room #{:d}",
                                   vnum, room_get_number(room));
                          }},
               i);
  }
}
