/*
 * DG Scripts: immortal commands for scripts: attach, detach, vdelete, tstat, and script stat output.
 * Split out of dg_scripts.c.
 */
#include "dbat/utils.h"
#include "dbat/act.wizard.h"
#include "dbat/dg_event.h"
#include "dg_internal.h"

/* Local functions not used elsewhere */
static void do_stat_trigger(struct char_data *ch, trig_data *trig);
static void script_stat(char_data *ch, struct script_data *sc);

static void do_stat_trigger(struct char_data *ch, trig_data *trig)
{
  struct cmdlist_element *cmd_list;
  char sb[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int len = 0;

  if (!trig) {
    mud_log("SYSERR: NULL trigger passed to do_stat_trigger.");
    return;
  }

  len += snprintf(sb, sizeof(sb), "Name: '@y%s@n',  VNum: [@g%5d@n], RNum: [%5d]\r\n",
                  trig->name, trig_get_vnum(trig), trig->nr);

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
                  ((trig->arglist && *trig->arglist)
                       ? trig->arglist
                       : "None"));

  len += snprintf(sb + len, sizeof(sb) - len, "Commands:\r\n");

  cmd_list = trig->cmdlist;
  while (cmd_list) {
    if (cmd_list->cmd)
      len += snprintf(sb + len, sizeof(sb) - len, "%s\r\n", cmd_list->cmd);

    if (len > MAX_STRING_LENGTH - 80) {
      len += snprintf(sb + len, sizeof(sb) - len, "*** Overflow - script too long! ***\r\n");
      break;
    }
    cmd_list = cmd_list->next;
  }

  send_to_char(ch, "%s", sb);
}

/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name, size_t nlen)
{
  char_data *ch;
  obj_data *obj;

  if ((ch = get_char(uid)))
    snprintf(name, nlen, "%s", ch->name);
  else if ((obj = get_obj(uid)))
    snprintf(name, nlen, "%s", obj->name);
  else
    snprintf(name, nlen, "uid = %s, (not found)", uid + 1);
}

/* general function to display stats on script sc */
static void script_stat(char_data *ch, struct script_data *sc)
{
  struct trig_var_data *tv;
  trig_data *t;
  char name[MAX_INPUT_LENGTH];
  char namebuf[512];
  char buf1[MAX_STRING_LENGTH];

  send_to_char(ch, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
  send_to_char(ch, "Global context: %ld\r\n", sc->context);

  for (tv = sc->global_vars; tv; tv = tv->next) {
    snprintf(namebuf, sizeof(namebuf), "%s:%ld", tv->name, tv->context);
    if (*(tv->value) == UID_CHAR) {
      find_uid_name(tv->value, name, sizeof(name));
      send_to_char(ch, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name, name);
    } else
      send_to_char(ch, "    %15s:  %s\r\n", tv->context ? namebuf : tv->name, tv->value);
  }

  for (t = sc->trig_list; t; t = t->next) {
    send_to_char(ch, "\r\n  Trigger: @y%s@n, VNum: [@y%5d@n], RNum: [%5d]\r\n",
                 t->name, trig_get_vnum(t), t->nr);

    if (t->attach_type == OBJ_TRIGGER) {
      send_to_char(ch, "  Trigger Intended Assignment: Objects\r\n");
      sprintbit(t->trigger_type, otrig_types, buf1, sizeof(buf1));
    } else if (t->attach_type == WLD_TRIGGER) {
      send_to_char(ch, "  Trigger Intended Assignment: Rooms\r\n");
      sprintbit(t->trigger_type, wtrig_types, buf1, sizeof(buf1));
    } else {
      send_to_char(ch, "  Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(t->trigger_type, trig_types, buf1, sizeof(buf1));
    }

    send_to_char(ch, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
                 buf1, t->narg,
                 ((t->arglist && *t->arglist) ? t->arglist : "None"));

    if (t->wait_event) {
      send_to_char(ch, "    Wait: %ld, Current line: %s\r\n",
                   event_time(t->wait_event),
                   t->curr_state ? t->curr_state->cmd : "End of Script");
      send_to_char(ch, "  Variables: %s\r\n", t->var_list ? "" : "None");

      for (tv = t->var_list; tv; tv = tv->next) {
        if (*(tv->value) == UID_CHAR) {
          find_uid_name(tv->value, name, sizeof(name));
          send_to_char(ch, "    %15s:  %s\r\n", tv->name, name);
        } else
          send_to_char(ch, "    %15s:  %s\r\n", tv->name, tv->value);
      }
    }
  }
}

void do_sstat_room(struct char_data *ch, struct room_data *rm)
{
  send_to_char(ch, "Triggers:\r\n");
  if (!rm->script) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, rm->script);
}

void do_sstat_object(char_data *ch, obj_data *j)
{
  send_to_char(ch, "Triggers:\r\n");
  if (!j->script) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, j->script);
}

void do_sstat_character(char_data *ch, char_data *k)
{
  send_to_char(ch, "Script information:\r\n");
  if (!k->script) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, k->script);
}

/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(struct script_data *sc, trig_data *t, int loc)
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

  t->next_in_world = trigger_list;
  trigger_list     = t;
}

ACMD(do_attach)
{
  char_data *victim;
  obj_data *object;
  room_data *room;
  trig_data *trig;
  char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
  char loc_name[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  int loc, tn, rn, num_arg;
  room_rnum rnum;

  argument = two_arguments(argument, arg, trig_name);
  two_arguments(argument, targ_name, loc_name);

  if (!*arg || !*targ_name || !*trig_name) {
    send_to_char(ch, "Usage: attach { mob | obj | room } { trigger } { name } [ location ]\r\n");
    return;
  }

  num_arg = atoi(targ_name);
  tn      = atoi(trig_name);
  loc     = (*loc_name) ? atoi(loc_name) : -1;

  if (is_abbrev(arg, "mobile") || is_abbrev(arg, "mtr")) {
    victim = get_char_vis(ch, targ_name, NULL, FIND_CHAR_WORLD);
    if (!victim) { /* search room for one with this vnum */
      for (victim = world[char_get_in_room(ch)].people; victim; victim = victim->next_in_room)
        if (char_get_mob_vnum(victim) == num_arg)
          break;

      if (!victim) {
        send_to_char(ch, "That mob does not exist.\r\n");
        return;
      }
    }
    if (!char_is_npc(victim)) {
      send_to_char(ch, "Players can't have scripts.\r\n");
      return;
    }
    if (!can_edit_zone(ch, world[char_get_in_room(ch)].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    /* have a valid mob, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    add_trigger(dg_ensure_script(&victim->script), trig, loc);

    send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, trig->name, victim->short_descr, char_get_mob_vnum(victim));
  }

  else if (is_abbrev(arg, "object") || is_abbrev(arg, "otr")) {
    object = get_obj_vis(ch, targ_name, NULL);
    if (!object) { /* search room for one with this vnum */
      for (object = world[char_get_in_room(ch)].contents; object; object = object->next_content)
        if (obj_get_vnum(object) == num_arg)
          break;

      if (!object) { /* search inventory for one with this vnum */
        for (object = ch->carrying; object; object = object->next_content)
          if (obj_get_vnum(object) == num_arg)
            break;

        if (!object) {
          send_to_char(ch, "That object does not exist.\r\n");
          return;
        }
      }
    }

    if (!can_edit_zone(ch, world[char_get_in_room(ch)].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    /* have a valid obj, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    add_trigger(dg_ensure_script(&object->script), trig, loc);

    send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, trig->name,
                 (object->short_description ? object->short_description : object->name),
                 obj_get_vnum(object));
  }

  else if (is_abbrev(arg, "room") || is_abbrev(arg, "wtr")) {
    if (strchr(targ_name, '.'))
      rnum = char_get_in_room(ch);
    else if (isdigit(*targ_name))
      rnum = find_target_room(ch, targ_name);
    else
      rnum = NOWHERE;

    if (rnum == NOWHERE) {
      send_to_char(ch, "You need to supply a room number or . for current room.\r\n");
      return;
    }

    if (!can_edit_zone(ch, world[rnum].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone\r\n");
      return;
    }
    /* have a valid room, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    room = &world[rnum];

    add_trigger(dg_ensure_script(&room->script), trig, loc);

    send_to_char(ch, "Trigger %d (%s) attached to room %d.\r\n",
                 tn, trig->name, world[rnum].number);
  }

  else
    send_to_char(ch, "Please specify 'mob', 'obj', or 'room'.\r\n");
}

/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
bool remove_trigger(struct script_data *sc, char *name)
{
  trig_data *i, *j;
  int num = 0, string = FALSE, n;
  char *cname;

  if (!sc)
    return false;

  if ((cname = strstr(name, ".")) || (!isdigit(*name))) {
    string = TRUE;
    if (cname) {
      *cname = '\0';
      num    = atoi(name);
      name   = ++cname;
    }
  } else
    num = atoi(name);

  for (n = 0, j = NULL, i = sc->trig_list; i; j = i, i = i->next) {
    if (string) {
      if (isname(name, i->name))
        if (++n >= num)
          break;
    }

    /* this isn't clean... */
    /* a numeric value will match if it's position OR vnum */
    /* is found. originally the number was position-only */
    else if (++n >= num)
      break;
    else if (trig_index[i->nr]->vnum == num)
      break;
  }

  if (i) {
    if (j) {
      j->next = i->next;
      extract_trigger(i);
    }

    /* this was the first trigger */
    else {
      sc->trig_list = i->next;
      extract_trigger(i);
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
  char_data *victim = NULL;
  obj_data *object  = NULL;
  struct room_data *room;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char *trigger = 0;
  int num_arg;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Usage: detach [ mob | object | room ] { target } { trigger |"
                     " 'all' }\r\n");
    return;
  }

  /* vnum of mob/obj, if given */
  num_arg = atoi(arg2);

  if (!strcasecmp(arg1, "room") || !strcasecmp(arg1, "wtr")) {
    room = &world[char_get_in_room(ch)];
    if (!can_edit_zone(ch, room->zone)) {
      send_to_char(ch, "You can only detach triggers in your own zone\r\n");
      return;
    }
    if (!room->script)
      send_to_char(ch, "This room does not have any triggers.\r\n");
    else if (!strcasecmp(arg2, "all")) {
      extract_script(room, WLD_TRIGGER);
      send_to_char(ch, "All triggers removed from room.\r\n");
    } else if (remove_trigger(room->script, arg2)) {
      send_to_char(ch, "Trigger removed.\r\n");
      if (!room->script->trig_list) {
        extract_script(room, WLD_TRIGGER);
      }
    } else
      send_to_char(ch, "That trigger was not found.\r\n");
  }

  else {
    if (is_abbrev(arg1, "mobile") || !strcasecmp(arg1, "mtr")) {
      victim = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD);
      if (!victim) { /* search room for one with this vnum */
        for (victim = world[char_get_in_room(ch)].people; victim; victim = victim->next_in_room)
          if (char_get_mob_vnum(victim) == num_arg)
            break;

        if (!victim) {
          send_to_char(ch, "No such mobile around.\r\n");
          return;
        }
      }

      if (!*arg3)
        send_to_char(ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    }

    else if (is_abbrev(arg1, "object") || !strcasecmp(arg1, "otr")) {
      object = get_obj_vis(ch, arg2, NULL);
      if (!object) { /* search room for one with this vnum */
        for (object = world[char_get_in_room(ch)].contents; object; object = object->next_content)
          if (obj_get_vnum(object) == num_arg)
            break;

        if (!object) { /* search inventory for one with this vnum */
          for (object = ch->carrying; object; object = object->next_content)
            if (obj_get_vnum(object) == num_arg)
              break;

          if (!object) { /* give up */
            send_to_char(ch, "No such object around.\r\n");
            return;
          }
        }
      }

      if (!*arg3)
        send_to_char(ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    } else {
      /* Thanks to Carlos Myers for fixing the line below */
      if ((object = get_obj_in_equip_vis(ch, arg1, NULL, ch->equipment)))
        ;
      else if ((object = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
        ;
      else if ((victim = get_char_room_vis(ch, arg1, NULL)))
        ;
      else if ((object = get_obj_in_list_vis(ch, arg1, NULL, world[char_get_in_room(ch)].contents)))
        ;
      else if ((victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)))
        ;
      else if ((object = get_obj_vis(ch, arg1, NULL)))
        ;
      else
        send_to_char(ch, "Nothing around by that name.\r\n");

      trigger = arg2;
    }

    if (victim) {
      if (!char_is_npc(victim))
        send_to_char(ch, "Players don't have triggers.\r\n");

      else if (!victim->script)
        send_to_char(ch, "That mob doesn't have any triggers.\r\n");
      else if (!can_edit_zone(ch, real_zone_by_thing(char_get_mob_vnum(victim)))) {
        send_to_char(ch, "You can only detach triggers in your own zone\r\n");
        return;
      } else if (trigger && !strcasecmp(trigger, "all")) {
        extract_script(victim, MOB_TRIGGER);
        send_to_char(ch, "All triggers removed from %s.\r\n", victim->short_descr);
      }

      else if (trigger && remove_trigger(victim->script, trigger)) {
        send_to_char(ch, "Trigger removed.\r\n");
        if (!victim->script->trig_list) {
          extract_script(victim, MOB_TRIGGER);
        }
      } else
        send_to_char(ch, "That trigger was not found.\r\n");
    }

    else if (object) {
      if (!object->script)
        send_to_char(ch, "That object doesn't have any triggers.\r\n");

      else if (!can_edit_zone(ch, real_zone_by_thing(obj_get_vnum(object)))) {
        send_to_char(ch, "You can only detach triggers in your own zone\r\n");
        return;
      } else if (trigger && !strcasecmp(trigger, "all")) {
        extract_script(object, OBJ_TRIGGER);
        send_to_char(ch, "All triggers removed from %s.\r\n",
                     object->short_description ? object->short_description : object->name);
      }

      else if (trigger && remove_trigger(object->script, trigger)) {
        send_to_char(ch, "Trigger removed.\r\n");
        if (!object->script->trig_list) {
          extract_script(object, OBJ_TRIGGER);
        }
      } else
        send_to_char(ch, "That trigger was not found.\r\n");
    }
  }
}
/*
 * command-line interface to rdelete
 * named vdelete so people didn't think it was to delete rooms
 */
ACMD(do_vdelete)
{
  struct script_data *sc_remote = NULL;
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
    send_to_char(ch, "Usage: vdelete { <variablename> | * | all } <id>\r\n");
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid <= 0) {
    send_to_char(ch, "vdelete: illegal id specified.\r\n");
    return;
  }

  if ((room = find_room(uid))) {
    sc_remote = room->script;
  } else if ((mob = find_char(uid))) {
    sc_remote = mob->script;
  } else if ((obj = find_obj(uid))) {
    sc_remote = obj->script;
  } else {
    send_to_char(ch, "vdelete: cannot resolve specified id.\r\n");
    return;
  }

  if (sc_remote == NULL) {
    send_to_char(ch, "That id represents no global variables.(1)\r\n");
    return;
  }

  if (sc_remote->global_vars == NULL) {
    send_to_char(ch, "That id represents no global variables.(2)\r\n");
    return;
  }

  if (*var == '*' || is_abbrev(var, "all")) {
    free_varlist(sc_remote->global_vars);
    sc_remote->global_vars = NULL;
    send_to_char(ch, "All variables deleted from that id.\r\n");
    return;
  }

  if (!remove_var(&sc_remote->global_vars, var)) {
    send_to_char(ch, "That variable cannot be located.\r\n");
    return;
  }

  send_to_char(ch, "Deleted.\r\n");
}
ACMD(do_tstat)
{
  int rnum;
  char str[MAX_INPUT_LENGTH];

  half_chop(argument, str, argument);
  if (*str) {
    rnum = real_trigger(atoi(str));
    if (rnum == NOTHING) {
      send_to_char(ch, "That vnum does not exist.\r\n");
      return;
    }

    do_stat_trigger(ch, trig_index[rnum]->proto);
  } else
    send_to_char(ch, "Usage: tstat <vnum>\r\n");
}
