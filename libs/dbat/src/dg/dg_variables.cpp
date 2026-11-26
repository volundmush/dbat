/**************************************************************************
*  File: dg_variables.c                                                   *
*  Usage: contains the functions dealing with variable substitution.      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00 $                                           *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include <array>
#include <cstdlib>

#include "dbat/io/reader.hpp"
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/relocate.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/db.hpp"
#include "dbat/screen.hpp"
#include "dbat/spells.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/class.hpp"
#include "dbat/races.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "fields.hpp"

/* Utility functions */

/*
 * Thanks to James Long for his assistance in plugging the memory leak
 * that used to be here.   -- Welcor
 */
/* adds a variable with given name and value to trigger */
void add_var(dg::var_store &vars, std::string_view name, std::string_view value, long id)
{
  if (name.find('.') != std::string_view::npos) {
    mud_log("add_var() : Attempt to add illegal var: {}", name);
    return;
  }

  if (value.size() > MAX_DG_VAR_LENGTH) {
    mud_log("SYSERR: Script variable {} is longer than {:d} characters; cutting it there.", name, as_unsigned(MAX_DG_VAR_LENGTH));
    value = value.substr(0, MAX_DG_VAR_LENGTH);
  }
  vars.set(name, value, id);
}

/* sets trigger variable name to character c's UID, so the script can use %name% to refer to it */
void add_char_uid_var(trig_data *trig, std::string_view name, const struct char_data *c, long context)
{
  std::string buf = str_format("{:c}{:d}", as_char(UID_CHAR), char_get_id(c));
  add_var(trig->var_list, name, buf, context);
}

/* sets trigger variable name to object o's UID, so the script can use %name% to refer to it */
void add_obj_uid_var(trig_data *trig, std::string_view name, const struct obj_data *o, long context)
{
  std::string buf = str_format("{:c}{:d}", as_char(UID_CHAR), obj_get_id(o));
  add_var(trig->var_list, name, buf, context);
}


namespace dg {

std::string uid_text(long id)
{
  return str_format("{:c}{:d}", as_char(UID_CHAR), id);
}

/*
   search through all the persons items, including containers
   and 0 if it doesnt exist, and greater then 0 if it does!
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000

   Now also searches by vnum -- Welcor
   Now returns the number of matching objects -- Welcor 02/04
*/
int item_in_list(GameWorld *g, std::string_view item, const id_list &list)
{
  int count = 0;

  if (item.empty())
    return 0;

  for (obj_data *i : obj_list(g, list)) {
    if (item.front() == UID_CHAR) {
      if (io::c_atol(item.substr(1)) == obj_get_id(i))
        count++;
    } else if (is_number(item)) { /* by vnum */
      if (obj_get_vnum(g, i) == parse_int(item))
        count++;
    } else if (isname(item, obj_get_name(i)))
      count++;
    if (obj_get_type_flag(i) == ITEM_CONTAINER)
      count += item_in_list(g, item, obj_get_contains(i));
  }
  return count;
}

} // namespace dg

namespace {

using dg::field_query;

/* The fields of any variable's text, as in %arg.car%, which work on the text and not on what it names */
namespace {
struct text_fields_entry {
  std::string_view name;
  std::string (*get)(const std::string &text, const std::string &arg);
};
}
const auto text_fields = std::to_array<text_fields_entry>({
    {"strlen", [](const std::string &text, const std::string &) { return str_format("{:d}", until_nul(text).size()); }},
    {"trim", [](const std::string &text, const std::string &) { /* without whitespace at either end */
       constexpr std::string_view space = " \t\n\v\f\r"; /* what isspace() finds */
       size_t start      = text.find_first_not_of(space);
       return start == std::string::npos ? std::string() : text.substr(start, text.find_last_not_of(space) + 1 - start);
     }},
    {"contains", [](const std::string &text, const std::string &arg) {
       return std::string(str_str(text, arg) ? "1" : "0");
     }},
    {"car", [](const std::string &text, const std::string &) { /* the first word */
       std::string_view car = until_nul(text);
       return std::string(car.substr(0, std::ranges::find_if(car, [](char c) { return isspace(c); }) - car.begin()));
     }},
    {"cdr", [](const std::string &text, const std::string &) { /* all but the first word */
       std::string_view cdr = until_nul(text);
       auto space           = [](char c) { return isspace(c) != 0; };
       cdr.remove_prefix(std::ranges::find_if(cdr, space) - cdr.begin());
       cdr.remove_prefix(std::ranges::find_if_not(cdr, space) - cdr.begin());
       return std::string(cdr);
     }},
    {"charat", [](const std::string &text, const std::string &arg) { /* the character at a place, from 1 */
       size_t len = until_nul(text).size(), at = parse_int(arg);
       return at > len || at < 1 ? std::string() : std::string(1, text[at - 1]);
     }},
    {"mudcommand", [](const std::string &text, const std::string &) { /* the command the text is the start of */
       std::string_view start = until_nul(text); /* as strncmp() read it */
       for (int cmd = 0; cmd_info[cmd].command != "\n"; cmd++)
         if (cmd_info[cmd].command.starts_with(start))
           return std::string(cmd_info[cmd].command);
       return std::string();
     }},
});

/* The names %send%, %echo% and the rest stand for: the command of that name for each kind of owner */
namespace {
struct command_names_entry {
  std::string_view name;
  std::array<std::string_view, 3> per_kind; /* for a mob, an object, a room */
};
}
const auto command_names = std::to_array<command_names_entry>({
    {"door",       {"mdoor ", "odoor ", "wdoor "}                     },
    {"force",      {"mforce ", "oforce ", "wforce "}                  },
    {"load",       {"mload ", "oload ", "wload "}                     },
    {"purge",      {"mpurge ", "opurge ", "wpurge "}                  },
    {"teleport",   {"mteleport ", "oteleport ", "wteleport "}         },
    {"damage",     {"mdamage ", "odamage ", "wdamage "}               },
    {"send",       {"msend ", "osend ", "wsend "}                     },
    {"echo",       {"mecho ", "oecho ", "wecho "}                     },
    {"echoaround", {"mechoaround ", "oechoaround ", "wechoaround "}   },
    {"zoneecho",   {"mzoneecho ", "ozoneecho ", "wzoneecho "}         },
    {"asound",     {"masound ", "oasound ", "wasound "}               },
    {"at",         {"mat ", "oat ", "wat "}                           },
    {"transform",  {"mtransform ", "otransform ", "wecho "}           }, /* there is no wtransform */
    {"recho",      {"mrecho ", "orecho ", "wrecho "}                  },
});

/* The variable var: the trigger's own, or else the script's global seen from its context */
const dg::variable *find_variable(struct script_data *sc, trig_data *trig, const std::string &var)
{
  const dg::variable *vd = trig ? trig->var_list.find(var) : nullptr; /* X.global() has no trigger */
  if (!vd && sc)                                                       /* some waits once came here with no script */
    vd = sc->global_vars.find_in(var, sc->context);
  return vd;
}

std::string number(long long n) { return str_format("{:d}", n); }

/* The room the owner is in, or NOWHERE */
room_vnum owner_room(const owner_ptr &go)
{
  return std::visit(overloaded{
                        [](char_data *ch) { return char_get_in_room(ch); },
                        [](obj_data *obj) { return obj_room(obj); },
                        [](room_data *room) { return room_get_number(room); },
                    },
                    go);
}

/* %random.char%: someone in the owner's room, a mob choosing only among those it sees */
std::string random_char(GameWorld *g, const owner_ptr &go)
{
  char_data *chosen = nullptr;
  int count         = 0;
  auto consider     = [&](char_data *who) {
    if (!rand_number(g, 0, count))
      chosen = who;
    count++;
  };

  std::visit(overloaded{
                 [&](char_data *ch) {
                   for (char_data *tch : room_people(g, char_get_in_room(ch)))
                     if (tch != ch && valid_dg_target(g, tch, dg_gods::allow) && char_can_see(g, ch, tch))
                       consider(tch);
                 },
                 [&](obj_data *obj) {
                   for (char_data *other : room_people(g, obj_room(obj)))
                     if (valid_dg_target(g, other, dg_gods::allow))
                       consider(other);
                 },
                 [&](room_data *room) {
                   for (char_data *who : room_people(g, room))
                     if (valid_dg_target(g, who, dg_gods::allow))
                       consider(who);
                 },
             },
             go);
  return chosen ? dg::uid_text(char_get_id(chosen)) : "";
}

/* %random.dir%: an exit of the owner's room */
std::string random_dir(GameWorld *g, const owner_ptr &go)
{
  room_data *room = room_of(g, owner_room(go));
  if (!room)
    return "";

  int doors = 0;
  for (int i = 0; i < NUM_OF_DIRS; i++)
    if (room_get_dir_option(room, i))
      doors++;
  if (!doors)
    return "";

  for (;;) {
    int dir = rand_number(g, 0, NUM_OF_DIRS - 1);
    if (room_get_dir_option(room, dir))
      return str_format("{}", dirs[dir]);
  }
}

/*
 * The special variables that take a field, when no variable of the name hides them:
 *
 * %global.<name>%     - the game's global variable of that name
 * %people.<room vnum>% - how many are in the room
 * %time.<hour|day|month|year>% - the game's time
 * %findmob.<room vnum X>(<mob vnum Y>)% - how many mobs in room X have vnum Y; players have the vnum -1, so
 *                       %findmob.1204(-1)% counts the players in room 1204
 * %findobj.<room vnum X>(<object vnum/id/name>)% - how many objects in room X have this name/id/vnum, so
 *                       %findobj.453(1234)% counts the bags of gold (vnum 1234) in the vault (vnum 453)
 * %random.<char|dir|number>% - someone in the owner's room, an exit of it, or a number from 1 up to that
 *
 * findmob and findobj are additions inspired by Jamie Nelson - mordecai@xtra.co.nz
 */
namespace {
struct special_variables_entry {
  std::string_view name;
  std::string (*get)(const field_query &);
};
}
const auto special_variables = std::to_array<special_variables_entry>({
    {"global", [](const field_query &q) {
       struct script_data *thescript = room_get_script(q.g, 0);
       if (!thescript) {
         script_log(q.g, "Attempt to find global var. Apparently the void has no script.");
         return std::string();
       }
       const dg::variable *vd = thescript->global_vars.find(q.field);
       return vd ? vd->value : "";
     }},
    {"people", [](const field_query &q) {
       room_vnum num = parse_int(q.field);
       return number(num > 0 ? trgvar_in_room(q.g, num) : 0);
     }},
    {"time", [](const field_query &q) {
       if (str_eq_nocase(q.field, "hour"))
         return number(q.g->time_info.hours);
       if (str_eq_nocase(q.field, "day"))
         return number(q.g->time_info.day + 1);
       if (str_eq_nocase(q.field, "month"))
         return number(q.g->time_info.month + 1);
       if (str_eq_nocase(q.field, "year"))
         return number(q.g->time_info.year);
       return std::string();
     }},
    {"findmob", [](const field_query &q) {
       if (!q.has_arg()) {
         script_log(q.g, "findmob.vnum(mvnum) - illegal syntax");
         return std::string("0");
       }
       room_vnum rrnum = parse_int(q.field);
       if (!room_of(q.g, rrnum)) {
         script_log(q.g, "findmob.vnum(ovnum): No room with vnum {:d}", rrnum);
         return std::string("0");
       }
       mob_vnum mvnum = parse_int(q.subfield);
       int count      = 0;
       for (char_data *vict : room_people(q.g, rrnum))
         if (char_get_mob_vnum(q.g, vict) == mvnum)
           count++;
       return number(count);
     }},
    {"findobj", [](const field_query &q) {
       if (!q.has_arg()) {
         script_log(q.g, "findobj.vnum(ovnum) - illegal syntax");
         return std::string("0");
       }
       room_vnum rrnum = parse_int(q.field);
       if (!room_of(q.g, rrnum)) {
         script_log(q.g, "findobj.vnum(ovnum): No room with vnum {:d}", rrnum);
         return std::string("0");
       }
       /* item_in_list looks within containers as well. */
       return number(dg::item_in_list(q.g, q.subfield, room_get_contents(q.g, rrnum)));
     }},
    {"random", [](const field_query &q) {
       if (str_eq_nocase(q.field, "char"))
         return random_char(q.g, q.go);
       if (str_eq_nocase(q.field, "dir"))
         return random_dir(q.g, q.go);
       int num = parse_int(q.field);
       return number(num > 0 ? rand_number(q.g, 1, num) : 0);
     }},
});

/* What a script owned by go finds by name: as a mob would, as an object would, or as a room would */
using found = std::variant<std::monostate, char_data *, obj_data *, room_data *>;

found find_by_name(GameWorld *g, const owner_ptr &go, std::string_view name)
{
  return std::visit(overloaded{
                        [&](char_data *ch) -> found {
                          if (obj_data *o = get_object_in_equip(g, ch, name))
                            return o;
                          if (obj_data *o = get_obj_in_list(g, name, char_get_inventory(ch)))
                            return o;
                          if (char_get_in_room(ch) != NOWHERE)
                            if (char_data *c = get_char_in_room(g, room_of(g, char_get_in_room(ch)), name))
                              return c;
                          if (obj_data *o = get_obj_in_list(g, name, room_get_contents(g, char_get_in_room(ch))))
                            return o;
                          if (char_data *c = get_char(g, name))
                            return c;
                          if (obj_data *o = get_obj(g, name))
                            return o;
                          if (room_data *r = get_room(g, name))
                            return r;
                          return {};
                        },
                        [&](obj_data *obj) -> found {
                          if (char_data *c = get_char_by_obj(g, obj, name))
                            return c;
                          if (obj_data *o = get_obj_by_obj(g, obj, name))
                            return o;
                          if (room_data *r = get_room(g, name))
                            return r;
                          return {};
                        },
                        [&](room_data *room) -> found {
                          if (char_data *c = get_char_by_room(g, room, name))
                            return c;
                          if (obj_data *o = get_obj_by_room(g, room, name))
                            return o;
                          if (room_data *r = get_room(g, name))
                            return r;
                          return {};
                        },
                    },
                    go);
}

/* The value of var.field(subfield); type is go's trigger type */
std::string find_replacement(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, int type,
                             const std::string &var, const std::string &field, std::string &subfield)
{
  if (field.empty())
    return dg::plain_variable(g, go, sc, trig, type, var);

  field_query q{g, go, trig, type, field, subfield};
  const dg::variable *vd = find_variable(sc, trig, var);
  found what;

  if (vd)
    what = find_by_name(g, go, until_nul(vd->value));
  else if (str_eq_nocase(var, "self"))
    std::visit([&](auto *self) { what = self; }, go);
  else {
    for (const auto &special : special_variables)
      if (str_eq_nocase(var, special.name))
        return special.get(q);
  }

  if (std::holds_alternative<std::monostate>(what))
    return ""; /* quirk: even the fields of the text, as strlen, need the text to name something */

  if (vd)
    for (const auto &f : text_fields)
      if (str_eq_nocase(field, f.name))
        return f.get(vd->value, subfield);

  return std::visit(overloaded{
                        [](std::monostate) { return std::string(); },
                        [&](char_data *c) { return dg::char_field(q, c); },
                        [&](obj_data *o) { return dg::obj_field(q, o); },
                        [&](room_data *r) { return dg::room_field(q, r); },
                    },
                    what);
}

} // namespace

std::string dg::plain_variable(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, int type, const std::string &var)
{
  if (const dg::variable *vd = find_variable(sc, trig, var))
    return vd->value;

  if (str_eq_nocase(var, "self")) /* the owner's own, even for X.global(self) */
    return uid_text(owner_uid(go));
  if (str_eq_nocase(var, "global")) /* so "remote varname %global%" will work */
    return number(ROOM_ID_BASE);
  if (str_eq_nocase(var, "ctime"))
    return number((long long)game_now(g));
  for (const auto &command : command_names)
    if (str_eq_nocase(var, command.name))
      return std::string(command.per_kind[type]);
  return "";
}

/* The value of the variable r names, looking up a chain (%x.y.z%) a link at a time; see dbat/dg/expr.hpp */
static std::string look_up(GameWorld *g, const dg::var_ref &r, owner_ptr go, struct script_data *sc, trig_data *trig)
{
  std::string var = r.name, field = r.field;

  for (const dg::var_ref::link &link : r.chain) {
    std::string subfield = link.subfield_so_far;
    std::string value    = find_replacement(g, go, sc, trig, owner_type(go), var, field, subfield);
    if (value.empty())
      continue;
    std::string eval = "eval tmpvr " + value; /* quirk: the value is worked out as an expression */
    process_eval(g, go, sc, trig, eval);
    var   = "tmpvr";
    field = link.next_field;
  }

  /* Now also lets subfields have variables parsed inside of them, so that %actor.gold(%actor.gold%)% will double
     the actor's gold every time it's called. - Jamie Nelson 31st Oct 2003 */
  std::string subfield = render(g, r.subfield, go, sc, trig);
  return find_replacement(g, go, sc, trig, owner_type(go), var, field, subfield);
}

/* t with its variables' values, up to the length of a command line: once that is full, nothing more is looked up */
std::string render(GameWorld *g, const dg::text_template &t, owner_ptr go, struct script_data *sc, trig_data *trig)
{
  constexpr size_t max_len = MAX_INPUT_LENGTH - 1;
  std::string out;

  for (const dg::template_piece &piece : t.pieces) {
    if (out.size() >= max_len)
      break;
    if (const std::string *text = std::get_if<std::string>(&piece.what))
      out.append(*text, 0, max_len - out.size());
    else
      out += look_up(g, std::get<dg::var_ref>(piece.what), go, sc, trig);
  }
  if (out.size() > max_len)
    out.resize(max_len);
  return out;
}

std::string var_subst(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view text)
{
  return render(g, dg::compile_template(text), go, sc, trig);
}
