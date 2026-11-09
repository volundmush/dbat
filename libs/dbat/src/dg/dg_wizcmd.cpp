/*
 * DG Scripts: immortal commands for scripts: attach, detach, vdelete, tstat, and script stat output.
 * Split out of dg_scripts.c.
 */
#include <cstdlib>

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
  struct cmdlist_element *cmd_list;
  char sb[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int len = 0;

  if (!trig) {
    mud_log("SYSERR: NULL trigger passed to do_stat_trigger.");
    return;
  }

  len += snprintf(sb, sizeof(sb), "Name: '@y%s@n',  VNum: [@g%5d@n], RNum: [%5d]\r\n",
                  trig->name.c_str(), trig_get_vnum(g, trig), g->trig_index.place(trig_get_vnum(g, trig)));

  if (trig->attach_type == OBJ_TRIGGER) {
    len += snprintf(sb + len, sizeof(sb) - len, "Trigger Intended Assignment: Objects\r\n");
    sprintbit(trig->trigger_type, otrig_types, buf, sizeof(buf));
  } else if (trig->attach_type == WLD_TRIGGER) {
    len += snprintf(sb + len, sizeof(sb) - len, "Trigger Intended Assignment: Rooms\r\n");
    sprintbit(trig->trigger_type, wtrig_types, buf, sizeof(buf));
  } else {
    len += snprintf(sb + len, sizeof(sb) - len, "Trigger Intended Assignment: Mobiles\r\n");
    sprintbit(trig->trigger_type, trig_types, buf, sizeof(buf));
  }

  len += snprintf(sb + len, sizeof(sb) - len, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
                  buf, trig->narg,
                  !trig->arglist.empty() ? trig->arglist.c_str() : "None");

  len += snprintf(sb + len, sizeof(sb) - len, "Commands:\r\n");

  cmd_list = trig->cmdlist;
  while (cmd_list) {
    if (!cmd_list->cmd.empty())
      len += snprintf(sb + len, sizeof(sb) - len, "%s\r\n", cmd_list->cmd.c_str());

    if (len > MAX_STRING_LENGTH - 80) {
      len += snprintf(sb + len, sizeof(sb) - len, "*** Overflow - script too long! ***\r\n");
      break;
    }
    cmd_list = cmd_list->next;
  }

  send_to_char(g, ch, "%s", sb);
}

/* find the name of what the uid points to */
void find_uid_name(GameWorld *g, char *uid, char *name, size_t nlen)
{
  char_data *ch;
  obj_data *obj;

  if ((ch = get_char(g, uid)))
    snprintf(name, nlen, "%s", char_get_alias(ch));
  else if ((obj = get_obj(g, uid)))
    snprintf(name, nlen, "%s", obj_get_name(obj));
  else
    snprintf(name, nlen, "uid = %s, (not found)", uid + 1);
}

/* general function to display stats on script sc */
static void script_stat(GameWorld *g, char_data *ch, struct script_data *sc)
{
  struct trig_var_data *tv;
  trig_data *t;
  char name[MAX_INPUT_LENGTH];
  char namebuf[512];
  char buf1[MAX_STRING_LENGTH];

  send_to_char(g, ch, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
  send_to_char(g, ch, "Global context: %ld\r\n", sc->context);

  for (tv = sc->global_vars; tv; tv = tv->next) {
    snprintf(namebuf, sizeof(namebuf), "%s:%ld", tv->name.c_str(), tv->context);
    if (tv->value[0] == UID_CHAR) {
      find_uid_name(g, tv->value.data(), name, sizeof(name));
      send_to_char(g, ch, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name.c_str(), name);
    } else
      send_to_char(g, ch, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name.c_str(), tv->value.c_str());
  }

  for (t = sc->trig_list; t; t = t->next) {
    send_to_char(g, ch, "\r\n  Trigger: @y%s@n, VNum: [@y%5d@n], RNum: [%5d]\r\n",
                 t->name.c_str(), trig_get_vnum(g, t), g->trig_index.place(trig_get_vnum(g, t)));

    if (t->attach_type == OBJ_TRIGGER) {
      send_to_char(g, ch, "  Trigger Intended Assignment: Objects\r\n");
      sprintbit(t->trigger_type, otrig_types, buf1, sizeof(buf1));
    } else if (t->attach_type == WLD_TRIGGER) {
      send_to_char(g, ch, "  Trigger Intended Assignment: Rooms\r\n");
      sprintbit(t->trigger_type, wtrig_types, buf1, sizeof(buf1));
    } else {
      send_to_char(g, ch, "  Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(t->trigger_type, trig_types, buf1, sizeof(buf1));
    }

    send_to_char(g, ch, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
                 buf1, t->narg,
                 !t->arglist.empty() ? t->arglist.c_str() : "None");

    if (t->wait_event) {
      send_to_char(g, ch, "    Wait: %ld, Current line: %s\r\n",
                   event_time(g, t->wait_event),
                   t->curr_state ? t->curr_state->cmd.c_str() : "End of Script");
      send_to_char(g, ch, "  Variables: %s\r\n", t->var_list ? "" : "None");

      for (tv = t->var_list; tv; tv = tv->next) {
        if (tv->value[0] == UID_CHAR) {
          find_uid_name(g, tv->value.data(), name, sizeof(name));
          send_to_char(g, ch, "    %15s:  %s\r\n", tv->name.c_str(), name);
        } else
          send_to_char(g, ch, "    %15s:  %s\r\n", tv->name.c_str(), tv->value.c_str());
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

ACMD(do_attach)
{
  char_data *victim;
  obj_data *object;
  room_data *room;
  trig_data *trig;
  char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
  char loc_name[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  int loc;
  trig_vnum tn;
  struct index_data *rn;
  IDXTYPE num_arg;
  room_vnum rnum;

  argument = two_arguments(argument, arg, trig_name);
  two_arguments(argument, targ_name, loc_name);

  if (!*arg || !*targ_name || !*trig_name) {
    send_to_char(g, ch, "Usage: attach { mob | obj | room } { trigger } { name } [ location ]\r\n");
    return;
  }

  num_arg = atoi(targ_name);
  tn      = atoi(trig_name);
  loc     = (*loc_name) ? atoi(loc_name) : -1;

  if (is_abbrev(arg, "mobile") || is_abbrev(arg, "mtr")) {
    victim = get_char_vis(g, ch, targ_name, nullptr, FIND_CHAR_WORLD);
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
    /* have a valid mob, now get trigger */
    rn = find_trigger(g, tn);
    if ((rn == nullptr) || !(trig = read_trigger(g, rn))) {
      send_to_char(g, ch, "That trigger does not exist.\r\n");
      return;
    }

    add_trigger(g, dg_ensure_script(char_script_slot(victim)), trig, loc);

    send_to_char(g, ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, trig->name.c_str(), char_get_sdesc(victim), char_get_mob_vnum(g, victim));
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
    /* have a valid obj, now get trigger */
    rn = find_trigger(g, tn);
    if ((rn == nullptr) || !(trig = read_trigger(g, rn))) {
      send_to_char(g, ch, "That trigger does not exist.\r\n");
      return;
    }

    add_trigger(g, dg_ensure_script(obj_script_slot(object)), trig, loc);

    send_to_char(g, ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, trig->name.c_str(),
                 (obj_get_short_description(object) ? obj_get_short_description(object) : obj_get_name(object)),
                 obj_get_vnum(g, object));
  }

  else if (is_abbrev(arg, "room") || is_abbrev(arg, "wtr")) {
    if (strchr(targ_name, '.'))
      rnum = char_get_in_room(ch);
    else if (isdigit(*targ_name))
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
    /* have a valid room, now get trigger */
    rn = find_trigger(g, tn);
    if ((rn == nullptr) || !(trig = read_trigger(g, rn))) {
      send_to_char(g, ch, "That trigger does not exist.\r\n");
      return;
    }

    room = room_of(g, rnum);

    add_trigger(g, dg_ensure_script(room_script_slot(room)), trig, loc);

    send_to_char(g, ch, "Trigger %d (%s) attached to room %d.\r\n",
                 tn, trig->name.c_str(), rnum);
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
bool remove_trigger(GameWorld *g, struct script_data *sc, char *name)
{
  trig_data *i, *j;
  mob_vnum num = 0;
  int string   = false, n;
  char *cname;

  if (!sc)
    return false;

  if ((cname = strstr(name, ".")) || (!isdigit(*name))) {
    string = true;
    if (cname) {
      *cname = '\0';
      num    = atoi(name);
      name   = ++cname;
    }
  } else
    num = atoi(name);

  for (n = 0, j = nullptr, i = sc->trig_list; i; j = i, i = i->next) {
    if (string) {
      if (isname(name, i->name.c_str()))
        if (++n >= num)
          break;
    }

    /* this isn't clean... */
    /* a numeric value will match if it's position OR vnum */
    /* is found. originally the number was position-only */
    else if (++n >= num)
      break;
    else if (i->index->vnum == num)
      break;
  }

  if (i) {
    if (j) {
      j->next = i->next;
      extract_trigger(g, i);
    }

    /* this was the first trigger */
    else {
      sc->trig_list = i->next;
      extract_trigger(g, i);
    }

    /* update the script type bitvector */
    sc->types = 0;
    for (i = sc->trig_list; i; i = i->next)
      sc->types |= i->trigger_type;

    return true;
  } else
    return false;
}

ACMD(do_detach)
{
  char_data *victim = nullptr;
  obj_data *object  = nullptr;
  struct room_data *room;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char *trigger = nullptr;
  IDXTYPE num_arg;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);

  if (!*arg1 || !*arg2) {
    send_to_char(g, ch, "Usage: detach [ mob | object | room ] { target } { trigger |"
                     " 'all' }\r\n");
    return;
  }

  /* vnum of mob/obj, if given */
  num_arg = atoi(arg2);

  if (!strcasecmp(arg1, "room") || !strcasecmp(arg1, "wtr")) {
    room = room_of(g, char_get_in_room(ch));
    if (!can_edit_zone(g, ch, room_get_zone(room))) {
      send_to_char(g, ch, "You can only detach triggers in your own zone\r\n");
      return;
    }
    if (!room_get_script(room))
      send_to_char(g, ch, "This room does not have any triggers.\r\n");
    else if (!strcasecmp(arg2, "all")) {
      extract_script(g, room, WLD_TRIGGER);
      send_to_char(g, ch, "All triggers removed from room.\r\n");
    } else if (remove_trigger(g, room_get_script(room), arg2)) {
      send_to_char(g, ch, "Trigger removed.\r\n");
      if (!room_get_script(room)->trig_list) {
        extract_script(g, room, WLD_TRIGGER);
      }
    } else
      send_to_char(g, ch, "That trigger was not found.\r\n");
  }

  else {
    if (is_abbrev(arg1, "mobile") || !strcasecmp(arg1, "mtr")) {
      victim = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_WORLD);
      if (!victim) { /* search room for one with this vnum */
        victim = find_first(room_people(g, char_get_in_room(ch)),
                            [&](char_data *v) { return char_get_mob_vnum(g, v) == num_arg; });

        if (!victim) {
          send_to_char(g, ch, "No such mobile around.\r\n");
          return;
        }
      }

      if (!*arg3)
        send_to_char(g, ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    }

    else if (is_abbrev(arg1, "object") || !strcasecmp(arg1, "otr")) {
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

      if (!*arg3)
        send_to_char(g, ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
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
      else if ((victim = get_char_vis(g, ch, arg1, nullptr, FIND_CHAR_WORLD)))
        ;
      else if ((object = get_obj_vis(g, ch, arg1, nullptr)))
        ;
      else
        send_to_char(g, ch, "Nothing around by that name.\r\n");

      trigger = arg2;
    }

    if (victim) {
      if (!char_is_npc(victim))
        send_to_char(g, ch, "Players don't have triggers.\r\n");

      else if (!char_get_script(victim))
        send_to_char(g, ch, "That mob doesn't have any triggers.\r\n");
      else if (!can_edit_zone(g, ch, find_zone_by_thing(g, char_get_mob_vnum(g, victim)))) {
        send_to_char(g, ch, "You can only detach triggers in your own zone\r\n");
        return;
      } else if (trigger && !strcasecmp(trigger, "all")) {
        extract_script(g, victim, MOB_TRIGGER);
        send_to_char(g, ch, "All triggers removed from %s.\r\n", char_get_sdesc(victim));
      }

      else if (trigger && remove_trigger(g, char_get_script(victim), trigger)) {
        send_to_char(g, ch, "Trigger removed.\r\n");
        if (!char_get_script(victim)->trig_list) {
          extract_script(g, victim, MOB_TRIGGER);
        }
      } else
        send_to_char(g, ch, "That trigger was not found.\r\n");
    }

    else if (object) {
      if (!obj_get_script(object))
        send_to_char(g, ch, "That object doesn't have any triggers.\r\n");

      else if (!can_edit_zone(g, ch, find_zone_by_thing(g, obj_get_vnum(g, object)))) {
        send_to_char(g, ch, "You can only detach triggers in your own zone\r\n");
        return;
      } else if (trigger && !strcasecmp(trigger, "all")) {
        extract_script(g, object, OBJ_TRIGGER);
        send_to_char(g, ch, "All triggers removed from %s.\r\n",
                     obj_get_short_description(object) ? obj_get_short_description(object) : obj_get_name(object));
      }

      else if (trigger && remove_trigger(g, obj_get_script(object), trigger)) {
        send_to_char(g, ch, "Trigger removed.\r\n");
        if (!obj_get_script(object)->trig_list) {
          extract_script(g, object, OBJ_TRIGGER);
        }
      } else
        send_to_char(g, ch, "That trigger was not found.\r\n");
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
  char *var, *uid_p;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  long uid;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  argument = two_arguments(argument, buf, buf2);
  var      = buf;
  uid_p    = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);

  if (!*buf || !*buf2) {
    send_to_char(g, ch, "Usage: vdelete { <variablename> | * | all } <id>\r\n");
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
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

  if (sc_remote->global_vars == nullptr) {
    send_to_char(g, ch, "That id represents no global variables.(2)\r\n");
    return;
  }

  if (*var == '*' || is_abbrev(var, "all")) {
    free_varlist(sc_remote->global_vars);
    sc_remote->global_vars = nullptr;
    send_to_char(g, ch, "All variables deleted from that id.\r\n");
    return;
  }

  if (!remove_var(&sc_remote->global_vars, var)) {
    send_to_char(g, ch, "That variable cannot be located.\r\n");
    return;
  }

  send_to_char(g, ch, "Deleted.\r\n");
}
ACMD(do_tstat)
{
  struct index_data *rnum;
  char str[MAX_INPUT_LENGTH];

  half_chop(argument, str, argument);
  if (*str) {
    rnum = find_trigger(g, atoi(str));
    if (rnum == nullptr) {
      send_to_char(g, ch, "That vnum does not exist.\r\n");
      return;
    }

    do_stat_trigger(g, ch, rnum->proto);
  } else
    send_to_char(g, ch, "Usage: tstat <vnum>\r\n");
}
