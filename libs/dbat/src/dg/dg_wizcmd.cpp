/*
 * DG Scripts: immortal commands for scripts: attach, detach, vdelete, tstat, and script stat output.
 * Split out of dg_scripts.c.
 */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

/* Local functions not used elsewhere */
static void do_stat_trigger(GameWorld *g, struct char_data *ch, trig_data *trig);
static void script_stat(GameWorld *g, char_data *ch, struct script_data *sc);

static void do_stat_trigger(GameWorld *g, struct char_data *ch, trig_data *trig)
{
  std::string sb;
  std::string buf;

  if (!trig) {
    mud_log("SYSERR: NULL trigger passed to do_stat_trigger.");
    return;
  }

  sb = str_format("Name: '@y{}@n',  VNum: [@g{:>5d}@n], RNum: [{:>5d}]\r\n",
                  trig->name, trig_get_vnum(g, trig), g->trig_index.place(trig_get_vnum(g, trig)));

  if (trig->attach_type == OBJ_TRIGGER) {
    sb += "Trigger Intended Assignment: Objects\r\n";
    buf = trig_type_names(trig->attach_type, trig->trigger_type);
  } else if (trig->attach_type == WLD_TRIGGER) {
    sb += "Trigger Intended Assignment: Rooms\r\n";
    buf = trig_type_names(trig->attach_type, trig->trigger_type);
  } else {
    sb += "Trigger Intended Assignment: Mobiles\r\n";
    buf = trig_type_names(trig->attach_type, trig->trigger_type);
  }

  sb += str_format("Trigger Type: {}, Numeric Arg: {:d}, Arg list: {}\r\n",
                   buf, trig->narg,
                   !trig->arglist.empty() ? trig->arglist.c_str() : "None");

  sb += "Commands:\r\n";

  for (const dg::line &l : trig->program->lines)
    sb += str_format("{}\r\n", l.text);

  if (!trig->program->runs()) {
    sb += "@RIt does not run: its script has errors.@n\r\n";
    for (const dg::diagnostic &e : trig->program->errors)
      sb += str_format("  line {:d}: {}\r\n", trig->program->lines[e.line].number, e.message);
  }

  send_to_char(g, ch, sb);
}

namespace {

/* What a script is on, for builders, as the error context names it */
std::string owner_text(GameWorld *g, owner_ptr go)
{
  return std::visit(overloaded{[&](char_data *mob) {
                                 return str_format("mob '{}' #{:d} uid {:d} in room {:d}", char_get_name(mob),
                                                   char_get_mob_vnum(g, mob), char_get_id(mob), char_get_in_room(mob));
                               },
                               [&](obj_data *obj) {
                                 return str_format("obj '{}' #{:d} uid {:d} in room {:d}",
                                                   obj_get_short_description(obj), obj_get_vnum(g, obj),
                                                   obj_get_id(obj), obj_room(obj));
                               },
                               [&](room_data *room) {
                                 return str_format("room '{}' #{:d}", room_get_name(room), room_get_number(room));
                               }},
                    go);
}

/* Calls fn(owner, script) for each thing in the game with a script */
template <class Fn>
void each_script(GameWorld *g, Fn &&fn)
{
  for (char_data *mob : each_char(g))
    if (char_get_script(mob))
      fn(owner_ptr{mob}, char_get_script(mob));
  for (obj_data *obj : each_obj(g))
    if (obj_get_script(obj))
      fn(owner_ptr{obj}, obj_get_script(obj));
  g->rooms.visit([&](room_vnum, room_data *room) {
    if (room_get_script(room))
      fn(owner_ptr{room}, room_get_script(room));
  });
}

/* Where an instance's run is: idle, or what it is doing, the line it carries on at and the blocks it is inside */
std::string run_text(GameWorld *g, const trig_data *t)
{
  const dg::run_state &run = t->run;
  if (!run.running())
    return "idle\r\n";

  std::string out = "running";
  if (t->wait_event) {
    long left = event_time(g, t->wait_event);
    out       = str_format("waiting, {:d} pulse{} left", left, left == 1 ? "" : "s");
  }
  if (run.prog != t->program)
    out += ", on the script as it was before it was last saved";
  if (const dg::line *at = run.next_line())
    out += str_format("\r\n      at line {:d}: {}", at->number, at->body());
  else
    out += "\r\n      at the end of the script";

  std::string inside;
  for (const dg::frame &f : run.frames)
    if (const dg::line *opened = run.opened_at(f))
      inside += str_format("{}{} at line {:d}", inside.empty() ? "" : ", in ", dg::keyword_name(opened->kw),
                           opened->number);
  if (!inside.empty())
    out += "\r\n      inside " + inside;
  return out + "\r\n";
}

/* The live instances of the trigger with index rnum, up to a screenful */
std::string instances_text(GameWorld *g, const index_data *rnum)
{
  constexpr int most_shown = 50;
  int found                = 0;
  std::string out;

  each_script(g, [&](owner_ptr go, script_data *sc) {
    for (const trig_data *t = sc->trig_list; t; t = t->next) {
      if (t->index != rnum)
        continue;
      if (++found <= most_shown)
        out += str_format("  #{:d} on {}: {}", t->id, owner_text(g, go), run_text(g, t));
    }
  });

  if (!found)
    return "Live instances: none\r\n";
  if (found > most_shown)
    out += str_format("  ... and {:d} more\r\n", found - most_shown);
  return str_format("Live instances: {:d}\r\n", found) + out;
}

} // namespace

/* find the name of what the uid points to */
std::string find_uid_name(GameWorld *g, std::string_view uid)
{
  char_data *ch;
  obj_data *obj;

  if ((ch = get_char(g, uid)))
    return str_format("{}", char_get_alias(ch));
  else if ((obj = get_obj(g, uid)))
    return str_format("{}", obj_get_name(obj));
  else
    return str_format("uid = {}, (not found)", uid.substr(1));
}

/* general function to display stats on script sc */
static void script_stat(GameWorld *g, char_data *ch, struct script_data *sc)
{
  trig_data *t;
  std::string namebuf;
  std::string buf1;

  send_to_char(g, ch, "Global Variables: {}\r\n", sc->global_vars.empty() ? "None" : "");
  send_to_char(g, ch, "Global context: {:d}\r\n", sc->context);

  for (const dg::variable &tv : sc->global_vars) {
    namebuf = str_format("{}:{:d}", tv.name, tv.context);
    if (tv.value[0] == UID_CHAR) {
      send_to_char(g, ch, "    {:>15}:  {}\r\n", tv.context ? namebuf : tv.name, find_uid_name(g, tv.value));
    } else
      send_to_char(g, ch, "    {:>15}:  {}\r\n", tv.context ? namebuf : tv.name, tv.value);
  }

  for (t = sc->trig_list; t; t = t->next) {
    send_to_char(g, ch, "\r\n  Trigger: @y{}@n, VNum: [@y{:>5d}@n], RNum: [{:>5d}]\r\n",
                 t->name, trig_get_vnum(g, t), g->trig_index.place(trig_get_vnum(g, t)));

    if (t->attach_type == OBJ_TRIGGER) {
      send_to_char(g, ch, "  Trigger Intended Assignment: Objects\r\n");
      buf1 = trig_type_names(t->attach_type, t->trigger_type);
    } else if (t->attach_type == WLD_TRIGGER) {
      send_to_char(g, ch, "  Trigger Intended Assignment: Rooms\r\n");
      buf1 = trig_type_names(t->attach_type, t->trigger_type);
    } else {
      send_to_char(g, ch, "  Trigger Intended Assignment: Mobiles\r\n");
      buf1 = trig_type_names(t->attach_type, t->trigger_type);
    }

    send_to_char(g, ch, "  Trigger Type: {}, Numeric Arg: {:d}, Arg list: {}\r\n",
                 buf1, t->narg,
                 !t->arglist.empty() ? t->arglist.c_str() : "None");

    if (t->wait_event) {
      send_to_char(g, ch, "    Wait: {:d}, Current line: {}\r\n",
                   event_time(g, t->wait_event),
                   t->run.next_line() ? t->run.next_line()->text.c_str() : "End of Script");
      send_to_char(g, ch, "  Variables: {}\r\n", t->var_list.empty() ? "None" : "");

      for (const dg::variable &tv : t->var_list) {
        if (tv.value[0] == UID_CHAR) {
          send_to_char(g, ch, "    {:>15}:  {}\r\n", tv.name, find_uid_name(g, tv.value));
        } else
          send_to_char(g, ch, "    {:>15}:  {}\r\n", tv.name, tv.value);
      }
    }
  }
}

void do_sstat_room(GameWorld *g, struct char_data *ch, struct room_data *rm)
{
  send_to_char(g, ch, "Triggers:\r\n");
  if (!room_get_script(rm)) {
    send_to_char(g, ch, "  None.\r\n");
    return;
  }

  script_stat(g, ch, room_get_script(rm));
}

void do_sstat_object(GameWorld *g, char_data *ch, obj_data *j)
{
  send_to_char(g, ch, "Triggers:\r\n");
  if (!obj_get_script(j)) {
    send_to_char(g, ch, "  None.\r\n");
    return;
  }

  script_stat(g, ch, obj_get_script(j));
}

void do_sstat_character(GameWorld *g, char_data *ch, char_data *k)
{
  send_to_char(g, ch, "Script information:\r\n");
  if (!char_get_script(k)) {
    send_to_char(g, ch, "  None.\r\n");
    return;
  }

  script_stat(g, ch, char_get_script(k));
}

/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(GameWorld *g, struct script_data *sc, trig_data *t, int loc)
{
  trig_data *i;
  int n;

  for (n = loc, i = sc->trig_list; i && i->next && (n != 0); n--, i = i->next);

  if (!loc) {
    t->next       = sc->trig_list;
    sc->trig_list = t;
  } else if (!i)
    sc->trig_list = t;
  else {
    t->next = i->next;
    i->next = t;
  }

  sc->types |= t->trigger_type;

  if (!t->id)
    t->id = ++g->max_trig_id;
  g->triggers.add(t->id, t);
}

namespace {

/* Attaches a new instance of trigger tn to go's script, at place loc as add_trigger() takes it; null, having told ch,
   if there's no such trigger */
trig_data *attach_new(GameWorld *g, char_data *ch, owner_ptr go, trig_vnum tn, int loc)
{
  struct index_data *rn = find_trigger(g, tn);
  trig_data *trig;
  if (!rn || !(trig = read_trigger(g, rn))) {
    send_to_char(g, ch, "That trigger does not exist.\r\n");
    return nullptr;
  }
  add_trigger(g, dg_ensure_script(script_slot(go)), trig, loc);
  return trig;
}

/* Removes the trigger named (as remove_trigger() takes it), or "all" of them, from go's script, telling ch; what is
   go as "All triggers removed from ..." names it. No trigger named finds none. */
void detach_from(GameWorld *g, char_data *ch, owner_ptr go, const std::string *trigger, std::string_view what)
{
  script_data *sc = *script_slot(go);
  if (trigger && str_eq_nocase(*trigger, "all")) {
    extract_script(g, go);
    send_to_char(g, ch, "All triggers removed from {}.\r\n", what);
  } else if (trigger && remove_trigger(g, sc, *trigger)) {
    send_to_char(g, ch, "Trigger removed.\r\n");
    if (!sc->trig_list)
      extract_script(g, go);
  } else
    send_to_char(g, ch, "That trigger was not found.\r\n");
}

} // namespace

ACMD(do_attach)
{
  char_data *victim;
  obj_data *object;
  trig_data *trig;
  std::string arg, trig_name, targ_name, loc_name;
  int loc;
  trig_vnum tn;
  IDXTYPE num_arg;
  room_vnum rnum;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  trig_name             = take_arg(rest);
  targ_name             = take_arg(rest);
  loc_name              = take_arg(rest);

  if (arg.empty() || targ_name.empty() || trig_name.empty()) {
    send_to_char(g, ch, "Usage: attach { mob | obj | room } { trigger } { name } [ location ]\r\n");
    return;
  }

  num_arg = parse_int(targ_name);
  tn      = parse_int(trig_name);
  loc     = !loc_name.empty() ? parse_int(loc_name) : -1;

  if (is_abbrev(arg, "mobile") || is_abbrev(arg, "mtr")) {
    victim = get_char_vis(g, ch, targ_name, nullptr, find_flag::char_world);
    if (!victim) { /* search room for one with this vnum */
      victim = find_first(room_people(g, char_get_in_room(ch)),
                          [&](char_data *v) { return char_get_mob_vnum(g, v) == num_arg; });

      if (!victim) {
        send_to_char(g, ch, "That mob does not exist.\r\n");
        return;
      }
    }
    if (!char_is_npc(victim)) {
      send_to_char(g, ch, "Players can't have scripts.\r\n");
      return;
    }
    if (!can_edit_zone(g, ch, room_get_zone(g, char_get_in_room(ch)))) {
      send_to_char(g, ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    if (!(trig = attach_new(g, ch, victim, tn, loc)))
      return;

    send_to_char(g, ch, "Trigger {:d} ({}) attached to {} [{:d}].\r\n",
                 tn, trig->name, char_get_sdesc(victim), char_get_mob_vnum(g, victim));
  }

  else if (is_abbrev(arg, "object") || is_abbrev(arg, "otr")) {
    object = get_obj_vis(g, ch, targ_name, nullptr);
    if (!object) { /* search room for one with this vnum */
      object = find_first(room_contents(g, char_get_in_room(ch)),
                          [&](obj_data *o) { return obj_get_vnum(g, o) == num_arg; });

      if (!object) { /* search inventory for one with this vnum */
        object = find_first(char_inventory(g, ch), [&](obj_data *o) { return obj_get_vnum(g, o) == num_arg; });

        if (!object) {
          send_to_char(g, ch, "That object does not exist.\r\n");
          return;
        }
      }
    }

    if (!can_edit_zone(g, ch, room_get_zone(g, char_get_in_room(ch)))) {
      send_to_char(g, ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    if (!(trig = attach_new(g, ch, object, tn, loc)))
      return;

    send_to_char(g, ch, "Trigger {:d} ({}) attached to {} [{:d}].\r\n",
                 tn, trig->name,
                 or_if_empty(obj_get_short_description(object), obj_get_name(object)),
                 obj_get_vnum(g, object));
  }

  else if (is_abbrev(arg, "room") || is_abbrev(arg, "wtr")) {
    if (targ_name.find('.') != std::string::npos)
      rnum = char_get_in_room(ch);
    else if (isdigit(targ_name[0]))
      rnum = find_target_room(g, ch, targ_name);
    else
      rnum = NOWHERE;

    if (rnum == NOWHERE) {
      send_to_char(g, ch, "You need to supply a room number or . for current room.\r\n");
      return;
    }

    if (!can_edit_zone(g, ch, room_get_zone(g, rnum))) {
      send_to_char(g, ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    if (!(trig = attach_new(g, ch, room_of(g, rnum), tn, loc)))
      return;

    send_to_char(g, ch, "Trigger {:d} ({}) attached to room {:d}.\r\n",
                 tn, trig->name, rnum);
  }

  else
    send_to_char(g, ch, "Please specify 'mob', 'obj', or 'room'.\r\n");
}

/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
bool remove_trigger(GameWorld *g, struct script_data *sc, std::string_view name)
{
  if (!sc)
    return false;

  /* "N.name" is the Nth trigger by that name, and a plain name the first; a number is the trigger in that place or
     with that vnum, whichever comes first (it was the place only, once) */
  bool by_name   = false;
  mob_vnum num   = 0;
  size_t dot     = name.find('.');
  std::string word(name);
  if (dot != std::string_view::npos || name.empty() || !isdigit(static_cast<unsigned char>(name[0]))) {
    by_name = true;
    if (dot != std::string_view::npos) {
      num  = parse_int(name.substr(0, dot));
      word = name.substr(dot + 1);
    }
  } else
    num = parse_int(word);

  trig_data *i, *j;
  int n;
  for (n = 0, j = nullptr, i = sc->trig_list; i; j = i, i = i->next) {
    if (by_name) {
      if (isname(word.c_str(), i->name.c_str()) && ++n >= num)
        break;
    } else if (++n >= num || i->index->vnum == num)
      break;
  }
  if (!i)
    return false;

  if (j)
    j->next = i->next;
  else
    sc->trig_list = i->next;
  extract_trigger(g, i);

  /* update the script type bitvector */
  sc->types = 0;
  for (i = sc->trig_list; i; i = i->next)
    sc->types |= i->trigger_type;
  return true;
}

ACMD(do_detach)
{
  char_data *victim = nullptr;
  obj_data *object  = nullptr;
  struct room_data *room;
  std::string arg1, arg2, arg3;
  std::string *trigger = nullptr; /* arg2 or arg3 */
  IDXTYPE num_arg;

  std::string_view rest = argument;
  arg1                  = take_arg(rest);
  arg2                  = take_arg(rest);
  arg3                  = take_arg(rest);

  if (arg1.empty() || arg2.empty()) {
    send_to_char(g, ch, "Usage: detach [ mob | object | room ] { target } { trigger | 'all' }\r\n");
    return;
  }

  /* vnum of mob/obj, if given */
  num_arg = parse_int(arg2);

  if (str_eq_nocase(arg1, "room") || str_eq_nocase(arg1, "wtr")) {
    room = room_of(g, char_get_in_room(ch));
    if (!can_edit_zone(g, ch, room_get_zone(room))) {
      send_to_char(g, ch, "You can only detach triggers in your own zone\r\n");
      return;
    }
    if (!room_get_script(room))
      send_to_char(g, ch, "This room does not have any triggers.\r\n");
    else
      detach_from(g, ch, room, &arg2, "room");
  }

  else {
    if (is_abbrev(arg1, "mobile") || str_eq_nocase(arg1, "mtr")) {
      victim = get_char_vis(g, ch, arg2, nullptr, find_flag::char_world);
      if (!victim) { /* search room for one with this vnum */
        victim = find_first(room_people(g, char_get_in_room(ch)),
                            [&](char_data *v) { return char_get_mob_vnum(g, v) == num_arg; });

        if (!victim) {
          send_to_char(g, ch, "No such mobile around.\r\n");
          return;
        }
      }

      if (arg3.empty())
        send_to_char(g, ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = &arg3;
    }

    else if (is_abbrev(arg1, "object") || str_eq_nocase(arg1, "otr")) {
      object = get_obj_vis(g, ch, arg2, nullptr);
      if (!object) { /* search room for one with this vnum */
        object = find_first(room_contents(g, char_get_in_room(ch)),
                            [&](obj_data *o) { return obj_get_vnum(g, o) == num_arg; });

        if (!object) { /* search inventory for one with this vnum */
          object = find_first(char_inventory(g, ch), [&](obj_data *o) { return obj_get_vnum(g, o) == num_arg; });

          if (!object) { /* give up */
            send_to_char(g, ch, "No such object around.\r\n");
            return;
          }
        }
      }

      if (arg3.empty())
        send_to_char(g, ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = &arg3;
    } else {
      /* Thanks to Carlos Myers for fixing the line below */
      if ((object = get_obj_in_equip_vis(g, ch, arg1, nullptr, char_get_equipment_array(ch))))
        ;
      else if ((object = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch))))
        ;
      else if ((victim = get_char_room_vis(g, ch, arg1, nullptr)))
        ;
      else if ((object = get_obj_in_list_vis(g, ch, arg1, nullptr, room_get_contents(g, char_get_in_room(ch)))))
        ;
      else if ((victim = get_char_vis(g, ch, arg1, nullptr, find_flag::char_world)))
        ;
      else if ((object = get_obj_vis(g, ch, arg1, nullptr)))
        ;
      else
        send_to_char(g, ch, "Nothing around by that name.\r\n");

      trigger = &arg2;
    }

    if (victim) {
      if (!char_is_npc(victim))
        send_to_char(g, ch, "Players don't have triggers.\r\n");

      else if (!char_get_script(victim))
        send_to_char(g, ch, "That mob doesn't have any triggers.\r\n");
      else if (!can_edit_zone(g, ch, find_zone_by_thing(g, char_get_mob_vnum(g, victim)))) {
        send_to_char(g, ch, "You can only detach triggers in your own zone\r\n");
        return;
      } else
        detach_from(g, ch, victim, trigger, char_get_sdesc(victim));
    }

    else if (object) {
      if (!obj_get_script(object))
        send_to_char(g, ch, "That object doesn't have any triggers.\r\n");

      else if (!can_edit_zone(g, ch, find_zone_by_thing(g, obj_get_vnum(g, object)))) {
        send_to_char(g, ch, "You can only detach triggers in your own zone\r\n");
        return;
      } else
        detach_from(g, ch, object, trigger,
                    or_if_empty(obj_get_short_description(object), obj_get_name(object)));
    }
  }
}
/*
 * command-line interface to rdelete
 * named vdelete so people didn't think it was to delete rooms
 */
ACMD(do_vdelete)
{
  struct script_data *sc_remote = nullptr;
  std::string var, uid_s;
  long uid;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  std::string_view rest = argument;
  var                   = take_arg(rest);
  uid_s                 = take_arg(rest);

  if (var.empty() || uid_s.empty()) {
    send_to_char(g, ch, "Usage: vdelete { <variablename> | * | all } <id>\r\n");
    return;
  }

  /* find the target script from the uid number */
  uid = parse_int(uid_s);
  if (uid <= 0) {
    send_to_char(g, ch, "vdelete: illegal id specified.\r\n");
    return;
  }

  if ((room = find_room(g, uid))) {
    sc_remote = room_get_script(room);
  } else if ((mob = find_char(g, uid))) {
    sc_remote = char_get_script(mob);
  } else if ((obj = find_obj(g, uid))) {
    sc_remote = obj_get_script(obj);
  } else {
    send_to_char(g, ch, "vdelete: cannot resolve specified id.\r\n");
    return;
  }

  if (sc_remote == nullptr) {
    send_to_char(g, ch, "That id represents no global variables.(1)\r\n");
    return;
  }

  if (sc_remote->global_vars.empty()) {
    send_to_char(g, ch, "That id represents no global variables.(2)\r\n");
    return;
  }

  if (var[0] == '*' || is_abbrev(var, "all")) {
    sc_remote->global_vars.clear();
    send_to_char(g, ch, "All variables deleted from that id.\r\n");
    return;
  }

  if (!sc_remote->global_vars.erase(var)) {
    send_to_char(g, ch, "That variable cannot be located.\r\n");
    return;
  }

  send_to_char(g, ch, "Deleted.\r\n");
}
ACMD(do_tstat)
{
  struct index_data *rnum;
  std::string str;

  std::string_view rest = argument;
  str                   = take_word(rest);
  if (!str.empty()) {
    rnum = find_trigger(g, parse_int(str));
    if (rnum == nullptr) {
      send_to_char(g, ch, "That vnum does not exist.\r\n");
      return;
    }

    do_stat_trigger(g, ch, rnum->proto);
    send_to_char(g, ch, instances_text(g, rnum));
  } else
    send_to_char(g, ch, "Usage: tstat <vnum>\r\n");
}

/* tcheck: the triggers whose scripts do not compile, and so never run; or with a vnum, all of one's errors */
ACMD(do_tcheck)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);

  if (!arg.empty()) {
    const index_data *rnum = find_trigger(g, parse_int(arg));
    if (!rnum) {
      send_to_char(g, ch, "That vnum does not exist.\r\n");
      return;
    }
    const dg::program &prog = *rnum->proto->program;
    if (prog.runs()) {
      send_to_char(g, ch, "Trigger #{:d} compiles.\r\n", rnum->vnum);
      return;
    }
    std::string out = str_format("Trigger #{:d} '{}' does not run:\r\n", rnum->vnum, rnum->proto->name);
    for (const dg::diagnostic &e : prog.errors)
      out += str_format("  line {:d}: {}\r\n", prog.lines[e.line].number, e.message);
    send_to_char(g, ch, out);
    return;
  }

  std::string out;
  int broken = 0;
  g->trig_index.visit([&](trig_vnum vnum, const index_data *rnum) {
    const dg::program &prog = *rnum->proto->program;
    if (prog.runs())
      return;
    broken++;
    const dg::diagnostic &first = prog.errors.front();
    out += str_format("  #{:<6d} {:<30} line {:d}: {}{}\r\n", vnum, rnum->proto->name, prog.lines[first.line].number,
                      first.message,
                      prog.errors.size() > 1 ? str_format(" (and {:d} more)", prog.errors.size() - 1) : "");
  });

  if (!broken) {
    send_to_char(g, ch, "Every trigger compiles.\r\n");
    return;
  }
  send_to_char(g, ch, "{:d} of {:d} triggers do not compile, and never run:\r\n{}Use tcheck <vnum> for all of one's errors.\r\n",
               broken, g->trig_index.size(), out);
}
