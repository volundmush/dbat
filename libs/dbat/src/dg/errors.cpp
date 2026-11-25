/*
 * Script errors: what a running trigger logs, with where it was. Every message ends with the context, as
 *   [trig #vnum 'name' instance #id, line N: text; on mob 'name' #vnum uid U in room R; actor 'name' uid A]
 * where the line is the one running, numbered as the script is written.
 */
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"

#include "dg_internal.hpp"

/* The name of the trigger's actor, from its variable, or "" */
static void find_actor(GameWorld *g, const trig_data *trig, struct dg_run_ctx &ctx)
{
  const dg::variable *actor = trig->var_list.find("actor");
  if (!actor || actor->value.size() < 2 || actor->value[0] != UID_CHAR)
    return;

  long uid = io::c_atol(std::string_view(actor->value).substr(1));
  if (char_data *ch = char_by_id(g, uid))
    ctx.actor = char_get_name(ch);
  else if (obj_data *obj = obj_by_id(g, uid))
    ctx.actor = obj_get_short_description(obj);
  else
    return;
  ctx.actor_uid = uid;
}

void dg_ctx_begin(GameWorld *g, owner_ptr go, trig_data *trig)
{
  room_vnum room = NOWHERE;
  struct dg_run_ctx &ctx = g->dg_ctx;

  ctx             = {};
  ctx.active      = true;
  ctx.owner_type  = owner_type(go);
  ctx.trig_vnum   = (long long)trig_get_vnum(g, trig);
  ctx.trig_name   = trig->name;
  ctx.instance_id = trig->id;
  ctx.room_vnum   = -1;
  find_actor(g, trig, ctx);

  if (std::visit([](auto *thing) { return thing == nullptr; }, go))
    return;

  std::visit(overloaded{[&](char_data *ch) {
                          ctx.owner_name = char_get_name(ch);
                          ctx.owner_vnum = (long long)char_get_mob_vnum(g, ch);
                          ctx.owner_uid  = char_get_id(ch);
                          room           = char_get_in_room(ch);
                        },
                        [&](obj_data *obj) {
                          ctx.owner_name = obj_get_short_description(obj);
                          ctx.owner_vnum = (long long)obj_get_vnum(g, obj);
                          ctx.owner_uid  = obj_get_id(obj);
                          room           = obj_room(obj);
                        },
                        [&](room_data *rm) {
                          ctx.owner_name = room_get_name(rm);
                          ctx.owner_vnum = (long long)room_get_number(rm);
                          ctx.owner_uid  = room_get_number(rm) + ROOM_ID_BASE;
                        }},
             go);

  if (ctx.owner_type == WLD_TRIGGER)
    ctx.room_vnum = ctx.owner_vnum;
  else if (room_of(g, room))
    ctx.room_vnum = (long long)room;
}

void dg_ctx_set_line(GameWorld *g, const dg::line &l)
{
  g->dg_ctx.line_number = l.number;
  g->dg_ctx.line        = l.body();
}

/* " [trig ... ]" describing the running trigger, or "" when none is. */
std::string dg_context(GameWorld *g)
{
  static constexpr std::string_view owner_types[] = {"mob", "obj", "room"};
  const struct dg_run_ctx &ctx     = g->dg_ctx;

  if (!ctx.active)
    return {};

  std::string where = ctx.line_number ? str_format(", line {:d}: {}", ctx.line_number, ctx.line) : "";
  std::string actor = ctx.actor_uid ? str_format("; actor '{}' uid {:d}", ctx.actor, ctx.actor_uid) : "";
  return str_format(" [trig #{:d} '{}' instance #{:d}{}; on {} '{}' #{:d} uid {:d} in room {:d}{}]", ctx.trig_vnum,
                    ctx.trig_name, ctx.instance_id, where,
                    (ctx.owner_type >= 0 && ctx.owner_type <= 2) ? owner_types[ctx.owner_type] : "?", ctx.owner_name,
                    ctx.owner_vnum, ctx.owner_uid, ctx.room_vnum, actor);
}

/*
 *  Logs any errors caused by scripts to the system log, and tells the builders who are listening.
 */
void script_log(GameWorld *g, std::string_view text)
{
  std::string ctx = dg_context(g);

  mud_log("SCRIPT ERR: {}{}", text, ctx);

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < ADMLVL_BUILDER)
      continue;
    if (char_plr_flagged(i->character, player_flag::writing))
      continue;
    if (LOG_NRM > (char_prf_flagged(g, i->character, pref_flag::log1) ? 1 : 0) + (char_prf_flagged(g, i->character, pref_flag::log2) ? 2 : 0))
      continue;

    send_to_char(g, i->character, "@g[ {}{} ]\r\n@n", text, ctx);
  }
}

/* Logs why trig's script does not compile, and tells d too if it is not null */
void report_compile_errors(GameWorld *g, const trig_data *trig, struct descriptor_data *d)
{
  const dg::program &prog = *trig->program;

  for (const dg::diagnostic &e : prog.errors) {
    const dg::line &l = prog.lines[e.line];
    mud_log("SCRIPT ERR: {} [trig #{:d} '{}', line {:d}: {}]", e.message, trig_get_vnum(g, trig), trig->name,
            l.number, l.body());
    if (d)
      write_to_output(g, d, "Error: line {:d}: {}\r\n", l.number, e.message);
  }
  if (d && !prog.runs())
    write_to_output(g, d, "This trigger will not run until its errors are fixed.\r\n");
}
