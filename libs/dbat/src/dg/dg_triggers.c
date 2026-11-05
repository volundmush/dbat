/**************************************************************************
*  File: dg_triggers.c                                                    *
*                                                                         *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: galion/Mark A. Heilpern/egreen/Welcor $                       *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include "dbat/utils.h"

#include "dbat/dg_scripts.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/oasis.h"
#include "dbat/spell_parser.h"
#include "dbat/act.movement.h"
#include "dbat/spells.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

/*
 *  General functions used by several triggers
 */

/*
 * Copy first phrase into first_arg, returns rest of string
 */
static char *one_phrase(char *arg, char *first_arg)
{
  skip_spaces(&arg);

  if (!*arg)
    *first_arg = '\0';

  else if (*arg == '"') {
    char *p, c;

    p  = matching_quote(arg);
    c  = *p;
    *p = '\0';
    strcpy(first_arg, arg + 1);
    if (c == '\0')
      return p;
    else
      return p + 1;
  }

  else {
    char *s, *p;

    s = first_arg;
    p = arg;

    while (*p && !isspace(*p) && *p != '"')
      *s++ = *p++;

    *s = '\0';
    return p;
  }

  return arg;
}

static bool is_substring(char *sub, char *string)
{
  char *s;

  if ((s = str_str(string, sub))) {
    int len    = strlen(string);
    int sublen = strlen(sub);

    /* check front */
    if ((s == string || isspace(*(s - 1)) || ispunct(*(s - 1))) &&

        /* check end */
        ((s + sublen == string + len) || isspace(s[sublen]) ||
         ispunct(s[sublen])))
      return true;
  }

  return false;
}

/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
static bool word_check(char *str, char *wordlist)
{
  char *words, *phrase, *s;
  size_t len;
  bool found = false;

  if (*wordlist == '*') return true;

  /*
   * one_phrase() writes into its input, so work on a copy; argument lists have no
   * length limit, so size the copy and the phrase buffer to fit
   */
  len = strlen(wordlist) + 1;
  CREATE(words, char, 2 * len);
  phrase = words + len;
  memcpy(words, wordlist, len);

  for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase))
    if (is_substring(phrase, str)) {
      found = true;
      break;
    }

  free(words);
  return found;
}

/*
 *  mob triggers
 */

void random_mtrigger(GameWorld *g, char_data *ch)
{
  trig_data *t;

  /*
   * This trigger is only called if a char is in the zone without nohassle.
   */

  if (!script_has_type(char_get_script(ch), MTRIG_RANDOM) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_RANDOM) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void bribe_mtrigger(GameWorld *g, char_data *ch, char_data *actor, int amount)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!script_has_type(char_get_script(ch), MTRIG_BRIBE) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_BRIBE) && (amount >= t->narg)) {
      snprintf(buf, sizeof(buf), "%d", amount);
      add_var(&t->var_list, "amount", buf, 0);
      add_char_uid_var(t, "actor", actor, 0);
      script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void greet_memory_mtrigger(GameWorld *g, char_data *actor)
{
  trig_data *t;
  char_data *ch;
  struct script_memory *mem;
  int command_performed = 0;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return;

  for (ch = rroom_get_people(g, char_get_in_room(actor)); ch; ch = char_get_next_in_room(ch)) {
    if (!char_get_memory(ch) || !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;
    /* find memory line with command only */
    for (mem = char_get_memory(ch); mem && char_get_memory(ch); mem = mem->next) {
      if (char_get_id(actor) != mem->id) continue;
      if (mem->cmd) {
        command_interpreter(g, ch, mem->cmd); /* no script */
        command_performed = 1;
        break;
      }
      /* if a command was not performed execute the memory script */
      if (mem && !command_performed) {
        for (t = char_get_script(ch)->trig_list; t; t = t->next) {
          if (IS_SET(t->trigger_type, MTRIG_MEMORY) &&
              char_can_see(g, ch, actor) &&
              !t->depth &&
              rand_number(g, 1, 100) <= t->narg) {
            add_char_uid_var(t, "actor", actor, 0);
            script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
          }
        }
      }
      /* delete the memory */
      if (mem) {
        if (char_get_memory(ch) == mem) {
          char_set_memory(ch, mem->next);
        } else {
          struct script_memory *prev;
          prev = char_get_memory(ch);
          while (prev->next != mem) prev = prev->next;
          prev->next = mem->next;
        }
        if (mem->cmd) free(mem->cmd);
        free(mem);
      }
    }
  }
}

bool greet_mtrigger(GameWorld *g, char_data *actor, int dir)
{
  trig_data *t;
  char_data *ch;
  int intermediate, final = true;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return true;

  for (ch = rroom_get_people(g, char_get_in_room(actor)); ch; ch = char_get_next_in_room(ch)) {
    if (!script_has_type(char_get_script(ch), MTRIG_GREET | MTRIG_GREET_ALL) ||
        !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;
    if (!script_has_type(char_get_script(ch), MTRIG_GREET_ALL) && char_is_npc(actor))
      continue;

    for (t = char_get_script(ch)->trig_list; t; t = t->next) {
      if (((IS_SET(t->trigger_type, MTRIG_GREET) && char_can_see(g, ch, actor)) ||
           IS_SET(t->trigger_type, MTRIG_GREET_ALL)) &&
          !t->depth && (rand_number(g, 1, 100) <= t->narg)) {
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[rev_dir[dir]], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        intermediate = script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
        if (!intermediate) final = false;
        continue;
      }
    }
  }
  return final;
}

void entry_memory_mtrigger(GameWorld *g, char_data *ch)
{
  trig_data *t;
  char_data *actor;
  struct script_memory *mem, *next_mem;

  if (!char_get_memory(ch) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (actor = rroom_get_people(g, char_get_in_room(ch)); actor && char_get_memory(ch);
       actor = char_get_next_in_room(actor)) {
    if (actor != ch && char_get_memory(ch)) {
      for (mem = char_get_memory(ch); mem; mem = next_mem) {
        next_mem = mem->next;
        if (char_get_id(actor) != mem->id)
          continue;

        /* unlink it first: the command or trigger below may change ch->memory (mforget) or purge ch */
        struct script_memory **link = char_memory_slot(ch);
        while (*link != mem) link = &(*link)->next;
        *link = mem->next;

        if (mem->cmd) command_interpreter(g, ch, mem->cmd);
        else {
          for (t = char_get_script(ch)->trig_list; t; t = t->next) {
            if (trigger_ready(t, MTRIG_MEMORY) && (rand_number(g, 1, 100) <=
                                                   t->narg)) {
              add_char_uid_var(t, "actor", actor, 0);
              script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
              break;
            }
          }
        }
        free(mem->cmd);
        free(mem);
        if (!ch || !char_get_memory(ch))
          return;

        /* carry on only if the next memory survived */
        struct script_memory *p = char_get_memory(ch);
        while (p && p != next_mem) p = p->next;
        next_mem = p;
      } /* for (mem =..... */
    }
  }
}

int entry_mtrigger(GameWorld *g, char_data *ch)
{
  trig_data *t;

  if (!script_has_type(char_get_script(ch), MTRIG_ENTRY) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_ENTRY) && (rand_number(g, 1, 100) <= t->narg)) {
      return script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }

  return 1;
}

bool command_mtrigger(GameWorld *g, char_data *actor, char *cmd, char *argument)
{
  char_data *ch, *ch_next;
  trig_data *t;

  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(g, actor, 0))
    return false;

  for (ch = rroom_get_people(g, char_get_in_room(actor)); ch; ch = ch_next) {
    ch_next = char_get_next_in_room(ch);

    if (script_has_type(char_get_script(ch), MTRIG_COMMAND) && !char_aff_flagged(ch, AFF_CHARM) &&
        (actor != ch)) {
      for (t = char_get_script(ch)->trig_list; t; t = t->next) {
        if (!trigger_ready(t, MTRIG_COMMAND))
          continue;

        if (!t->arglist || !*t->arglist) {
          mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: Command Trigger #%d has no text argument!",
                 trig_get_vnum(g, t));
          continue;
        }

        if (*t->arglist == '*' ||
            !strncasecmp(t->arglist, cmd, strlen(t->arglist))) {
          add_char_uid_var(t, "actor", actor, 0);
          skip_spaces(&argument);
          add_var(&t->var_list, "arg", argument, 0);
          skip_spaces(&cmd);
          add_var(&t->var_list, "cmd", cmd, 0);

          if (script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW))
            return true;
        }
      }
    }
  }

  return false;
}

void speech_mtrigger(GameWorld *g, char_data *actor, char *str)
{
  char_data *ch, *ch_next;
  trig_data *t;

  for (ch = rroom_get_people(g, char_get_in_room(actor)); ch; ch = ch_next) {
    ch_next = char_get_next_in_room(ch);

    if (script_has_type(char_get_script(ch), MTRIG_SPEECH) && char_is_awake(ch) &&
        !char_aff_flagged(ch, AFF_CHARM) && (actor != ch))
      for (t = char_get_script(ch)->trig_list; t; t = t->next) {
        if (!trigger_ready(t, MTRIG_SPEECH))
          continue;

        if (!t->arglist || !*t->arglist) {
          mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: Speech Trigger #%d has no text argument!",
                 trig_get_vnum(g, t));
          continue;
        }

        if (((t->narg && word_check(str, t->arglist)) ||
             (!t->narg && is_substring(t->arglist, str)))) {
          add_char_uid_var(t, "actor", actor, 0);
          add_var(&t->var_list, "speech", str, 0);
          script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
          break;
        }
      }
  }
}

void act_mtrigger(GameWorld *g, const char_data *ch, char *str, char_data *actor, const char_data *victim, obj_data *object, const obj_data *target, const char *arg)
{
  trig_data *t;

  if (script_has_type(char_get_script(ch), MTRIG_ACT) && !char_aff_flagged(ch, AFF_CHARM) &&
      (actor != ch))
    for (t = char_get_script(ch)->trig_list; t; t = t->next) {
      if (!trigger_ready(t, MTRIG_ACT))
        continue;

      if (!t->arglist || !*t->arglist) {
        mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: Act Trigger #%d has no text argument!",
               trig_get_vnum(g, t));
        continue;
      }

      if (((t->narg && word_check(str, t->arglist)) ||
           (!t->narg && is_substring(t->arglist, str)))) {
        if (actor)
          add_char_uid_var(t, "actor", actor, 0);
        if (victim)
          add_char_uid_var(t, "victim", victim, 0);
        if (object)
          add_obj_uid_var(t, "object", object, 0);
        if (target)
          add_obj_uid_var(t, "target", target, 0);
        if (str) {
          /* we're guaranteed to have a string ending with \r\n\0 */
          char *nstr = strdup(str), *fstr = nstr, *p = strchr(nstr, '\r');
          skip_spaces(&nstr);
          *p = '\0';
          add_var(&t->var_list, "arg", nstr, 0);
          free(fstr);
        }
        script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
        break;
      }
    }
}

void fight_mtrigger(GameWorld *g, char_data *ch)
{
  struct char_data *actor;
  trig_data *t;

  if (!script_has_type(char_get_script(ch), MTRIG_FIGHT) || !char_get_fighting(ch) ||
      char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_FIGHT) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      actor = char_get_fighting(ch);
      if (actor)
        add_char_uid_var(t, "actor", actor, 0);
      else
        add_var(&t->var_list, "actor", "nobody", 0);

      script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void hitprcnt_mtrigger(GameWorld *g, char_data *ch)
{
  struct char_data *actor;
  trig_data *t;

  if (!script_has_type(char_get_script(ch), MTRIG_HITPRCNT) || !char_get_fighting(ch) ||
      char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_HITPRCNT) && getEffMaxPL(g, ch) &&
        (getCurPL(g, ch) <= (getEffMaxPL(g, ch) / 100) * t->narg)) {

      actor = char_get_fighting(ch);
      add_char_uid_var(t, "actor", actor, 0);
      script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

int receive_mtrigger(GameWorld *g, char_data *ch, char_data *actor, obj_data *obj)
{
  trig_data *t;
  int ret_val;
  long id = obj_get_id(obj);

  if (!script_has_type(char_get_script(ch), MTRIG_RECEIVE) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_RECEIVE) &&
        (rand_number(g, 1, 100) <= t->narg)) {

      add_char_uid_var(t, "actor", actor, 0);
      add_obj_uid_var(t, "object", obj, 0);
      ret_val = script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      /* by id, because plenty of scripts purge what they're given */
      if (char_is_dead(actor) || char_is_dead(ch) || !(obj = find_obj(g, id)) || obj_get_carried_by(obj) != actor)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

int death_mtrigger(GameWorld *g, char_data *ch, char_data *actor)
{
  trig_data *t;

  if (!script_has_type(char_get_script(ch), MTRIG_DEATH) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_DEATH) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      if (actor)
        add_char_uid_var(t, "actor", actor, 0);
      return script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

void load_mtrigger(GameWorld *g, char_data *ch)
{
  trig_data *t;
  int result = 0;

  if (!script_has_type(char_get_script(ch), MTRIG_LOAD))
    return;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_LOAD) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      result = script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
  if (result == SCRIPT_ERROR_CODE) {
    /* we have recursed beyond reasonable depth */
    /* make sure this mob is the last one in the load chain */
    if (char_get_nr(ch) != NOBODY) {
      free_proto_script(&g->mob_proto[char_get_nr(ch)], MOB_TRIGGER);
    }
  }
}

int cast_mtrigger(GameWorld *g, char_data *actor, char_data *ch, int spellnum)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (ch == nullptr)
    return 1;

  if (!script_has_type(char_get_script(ch), MTRIG_CAST) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_CAST) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      sprintf(buf, "%d", spellnum);
      add_var(&t->var_list, "spell", buf, 0);
      add_var(&t->var_list, "spellname", (const char *)skill_name(g, spellnum), 0);
      return script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int leave_mtrigger(GameWorld *g, char_data *actor, int dir)
{
  trig_data *t;
  char_data *ch;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return 1;

  for (ch = rroom_get_people(g, char_get_in_room(actor)); ch; ch = char_get_next_in_room(ch)) {
    if (!script_has_type(char_get_script(ch), MTRIG_LEAVE) ||
        !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;

    for (t = char_get_script(ch)->trig_list; t; t = t->next) {
      if ((IS_SET(t->trigger_type, MTRIG_LEAVE)) &&
          !t->depth && (rand_number(g, 1, 100) <= t->narg)) {
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        return script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      }
    }
  }
  return 1;
}

int door_mtrigger(GameWorld *g, char_data *actor, int subcmd, int dir)
{
  trig_data *t;
  char_data *ch;

  for (ch = rroom_get_people(g, char_get_in_room(actor)); ch; ch = char_get_next_in_room(ch)) {
    if (!script_has_type(char_get_script(ch), MTRIG_DOOR) ||
        !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;

    for (t = char_get_script(ch)->trig_list; t; t = t->next) {
      if (IS_SET(t->trigger_type, MTRIG_DOOR) && char_can_see(g, ch, actor) &&
          !t->depth && (rand_number(g, 1, 100) <= t->narg)) {
        add_var(&t->var_list, "cmd", (const char *)cmd_door[subcmd], 0);
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        return script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      }
    }
  }
  return 1;
}

void time_mtrigger(GameWorld *g, char_data *ch)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  /*
   * This trigger is called if the hour is the same as specified in Narg.
   */

  if (!script_has_type(char_get_script(ch), MTRIG_TIME) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = char_get_script(ch)->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_TIME) &&
        (g->time_info.hours == t->narg)) {
      sprintf(buf, "%d", g->time_info.hours);
      add_var(&t->var_list, "time", buf, 0);
      script_driver(g, &ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

/*
 *  object triggers
 */

void random_otrigger(GameWorld *g, obj_data *obj)
{
  trig_data *t;

  if (!script_has_type(obj_get_script(obj), OTRIG_RANDOM))
    return;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_RANDOM) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void timer_otrigger(GameWorld *g, struct obj_data *obj)
{
  trig_data *t;

  if (!script_has_type(obj_get_script(obj), OTRIG_TIMER))
    return;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_TIMER)) {
      script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
    }
  }

  return;
}

int get_otrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  trig_data *t;
  int ret_val;
  if (!script_has_type(obj_get_script(obj), OTRIG_GET))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_GET) && (rand_number(g, 1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a get to take place, if
       * a) the actor is killed (the mud would choke on obj_to_char).
       * b) the object is purged.
       */
      if (char_is_dead(actor) || !obj)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

/* checks for command trigger on specific object. assumes obj has cmd trig */
/* Sets *ran if a script ran, since that can free any object at all, this one included. */
static bool cmd_otrig(GameWorld *g, obj_data *obj, char_data *actor, char *cmd,
                      char *argument, int type, bool *ran)
{
  trig_data *t;

  if (obj && script_has_type(obj_get_script(obj), OTRIG_COMMAND))
    for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
      if (!trigger_ready(t, OTRIG_COMMAND))
        continue;

      if (IS_SET(t->narg, type) &&
          (!t->arglist || !*t->arglist)) {
        mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: O-Command Trigger #%d has no text argument!",
               trig_get_vnum(g, t));
        continue;
      }

      if (IS_SET(t->narg, type) &&
          (*t->arglist == '*' ||
           !strncasecmp(t->arglist, cmd, strlen(t->arglist)))) {

        add_char_uid_var(t, "actor", actor, 0);
        skip_spaces(&argument);
        add_var(&t->var_list, "arg", argument, 0);
        skip_spaces(&cmd);
        add_var(&t->var_list, "cmd", cmd, 0);

        *ran = true;
        if (script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW) || !obj)
          return true;
      }
    }

  return false;
}

/* Runs the command triggers on a list of objects, returning true if one of them took the command. */
static bool cmd_otrig_list(GameWorld *g, obj_data *obj, char_data *actor, char *cmd, char *argument, int type)
{
  while (obj) {
    obj_data *next = obj_get_next_content(obj);
    long next_id   = next ? obj_get_id(next) : 0;
    bool forged    = obj_extra_flagged(obj, ITEM_FORGED);
    bool ran       = false;

    if (cmd_otrig(g, obj, actor, cmd, argument, type, &ran) && !forged)
      return true;
    if (char_is_dead(actor))
      return true;
    obj = ran ? find_obj(g, next_id) : next;
  }
  return false;
}

bool command_otrigger(GameWorld *g, char_data *actor, char *cmd, char *argument)
{
  obj_data *obj;
  int i;

  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(g, actor, 0))
    return false;

  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = char_get_equipment(actor, i))) {
      bool forged = obj_extra_flagged(obj, ITEM_FORGED);
      bool ran    = false;

      if ((cmd_otrig(g, obj, actor, cmd, argument, OCMD_EQUIP, &ran) && !forged) || char_is_dead(actor))
        return true;
    }

  return cmd_otrig_list(g, char_get_inventory(actor), actor, cmd, argument, OCMD_INVEN) ||
         cmd_otrig_list(g, rroom_get_contents(g, char_get_in_room(actor)), actor, cmd, argument, OCMD_ROOM);
}

int wear_otrigger(GameWorld *g, obj_data *obj, char_data *actor, int where)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj_get_script(obj), OTRIG_WEAR))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_WEAR)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a wear to take place, if
       * the object is purged.
       */
      if (!obj)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

int remove_otrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj_get_script(obj), OTRIG_REMOVE))
    return 1;

  if (!valid_dg_target(g, actor, 0))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_REMOVE)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a remove to take place, if
       * the object is purged.
       */
      if (!obj)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

int drop_otrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj_get_script(obj), OTRIG_DROP))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_DROP) && (rand_number(g, 1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a drop to take place, if
       * the object is purged (nothing to drop).
       */
      if (!obj)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

int give_otrigger(GameWorld *g, obj_data *obj, char_data *actor, char_data *victim)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj_get_script(obj), OTRIG_GIVE))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_GIVE) && (rand_number(g, 1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      add_char_uid_var(t, "victim", victim, 0);
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a give to take place, if
       * a) the object is purged.
       * b) the object is not carried by the giver.
       */
      if (!obj || obj_get_carried_by(obj) != actor)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

bool load_otrigger(GameWorld *g, obj_data *obj)
{
  trig_data *t;
  int result = 0;

  if (!script_has_type(obj_get_script(obj), OTRIG_LOAD))
    return true;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_LOAD) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      result = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      break;
    }
  }
  if (!obj)
    return false;
  if (result == SCRIPT_ERROR_CODE) {
    /* we have recursed beyond reasonable depth */
    /* make sure this mob is the last one in the load chain */
    if (obj_get_item_number(obj) != NOTHING) {
      free_proto_script(&g->obj_proto[obj_get_item_number(obj)], OBJ_TRIGGER);
    }
  }
  return true;
}

int cast_otrigger(GameWorld *g, char_data *actor, obj_data *obj, int spellnum)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];
  int ret_val;

  if (obj == nullptr)
    return 1;

  if (!script_has_type(obj_get_script(obj), OTRIG_CAST))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_CAST) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      sprintf(buf, "%d", spellnum);
      add_var(&t->var_list, "spell", buf, 0);
      add_var(&t->var_list, "spellname", (const char *)skill_name(g, spellnum), 0);
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* the spell has nothing left to work on, or nobody left to cast it */
      if (!obj || char_is_dead(actor))
        return 0;
      return ret_val;
    }
  }

  return 1;
}

bool leave_otrigger(GameWorld *g, room_data *room, char_data *actor, int dir)
{
  trig_data *t;
  int temp, final = 1;
  obj_data *obj, *obj_next;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return true;

  for (obj = room_get_contents(room); obj; obj = obj_next) {
    obj_next = obj_get_next_content(obj);
    if (!script_has_type(obj_get_script(obj), OTRIG_LEAVE))
      continue;

    for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
      if (trigger_ready(t, OTRIG_LEAVE) &&
          (rand_number(g, 1, 100) <= t->narg)) {
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        temp = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
        if (temp == 0)
          final = 0;
      }
    }
  }

  return final;
}

int consume_otrigger(GameWorld *g, obj_data *obj, char_data *actor, int cmd)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj_get_script(obj), OTRIG_CONSUME))
    return 1;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_CONSUME)) {
      add_char_uid_var(t, "actor", actor, 0);
      switch (cmd) {
        case OCMD_EAT:
          add_var(&t->var_list, "command", "eat", 0);
          break;
        case OCMD_DRINK:
          add_var(&t->var_list, "command", "drink", 0);
          break;
        case OCMD_QUAFF:
          add_var(&t->var_list, "command", "quaff", 0);
          break;
      }
      ret_val = script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a wear to take place, if
       * the object is purged.
       */
      if (!obj)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

void time_otrigger(GameWorld *g, obj_data *obj)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!script_has_type(obj_get_script(obj), OTRIG_TIME))
    return;

  for (t = obj_get_script(obj)->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_TIME) &&
        (g->time_info.hours == t->narg)) {
      sprintf(buf, "%d", g->time_info.hours);
      add_var(&t->var_list, "time", buf, 0);
      script_driver(g, &obj, t, OBJ_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

/*
 *  world triggers
 */

void reset_wtrigger(GameWorld *g, struct room_data *room)
{
  trig_data *t;

  if (!script_has_type(room_get_script(room), WTRIG_RESET))
    return;

  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_RESET) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void random_wtrigger(GameWorld *g, struct room_data *room)
{
  trig_data *t;

  if (!script_has_type(room_get_script(room), WTRIG_RANDOM))
    return;

  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_RANDOM) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

int enter_wtrigger(GameWorld *g, struct room_data *room, char_data *actor, int dir)
{
  trig_data *t;

  if (!script_has_type(room_get_script(room), WTRIG_ENTER))
    return 1;

  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_ENTER) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      if (dir >= 0 && dir < NUM_OF_DIRS)
        add_var(&t->var_list, "direction", (const char *)dirs[rev_dir[dir]], 0);
      else
        add_var(&t->var_list, "direction", "none", 0);
      add_char_uid_var(t, "actor", actor, 0);
      return script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int command_wtrigger(GameWorld *g, char_data *actor, char *cmd, char *argument)
{
  struct room_data *room;
  trig_data *t;

  if (!actor || !script_has_type(rroom_get_script(g, char_get_in_room(actor)), WTRIG_COMMAND))
    return 0;

  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(g, actor, 0))
    return 0;

  room = &g->rooms[char_get_in_room(actor)];
  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (!trigger_ready(t, WTRIG_COMMAND))
      continue;

    if (!t->arglist || !*t->arglist) {
      mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: W-Command Trigger #%d has no text argument!",
             trig_get_vnum(g, t));
      continue;
    }

    if (*t->arglist == '*' ||
        !strncasecmp(t->arglist, cmd, strlen(t->arglist))) {
      add_char_uid_var(t, "actor", actor, 0);
      skip_spaces(&argument);
      add_var(&t->var_list, "arg", argument, 0);
      skip_spaces(&cmd);
      add_var(&t->var_list, "cmd", cmd, 0);

      return script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 0;
}

void speech_wtrigger(GameWorld *g, char_data *actor, char *str)
{
  struct room_data *room;
  trig_data *t;

  if (!actor || !script_has_type(rroom_get_script(g, char_get_in_room(actor)), WTRIG_SPEECH))
    return;

  room = &g->rooms[char_get_in_room(actor)];
  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (!trigger_ready(t, WTRIG_SPEECH))
      continue;

    if (!t->arglist || !*t->arglist) {
      mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: W-Speech Trigger #%d has no text argument!",
             trig_get_vnum(g, t));
      continue;
    }

    if (*t->arglist == '*' ||
        (t->narg && word_check(str, t->arglist)) ||
        (!t->narg && is_substring(t->arglist, str))) {
      add_char_uid_var(t, "actor", actor, 0);
      add_var(&t->var_list, "speech", str, 0);
      script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

int drop_wtrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  struct room_data *room;
  trig_data *t;
  int ret_val;

  if (!actor || !script_has_type(rroom_get_script(g, char_get_in_room(actor)), WTRIG_DROP))
    return 1;

  room = &g->rooms[char_get_in_room(actor)];
  for (t = room_get_script(room)->trig_list; t; t = t->next)
    if (trigger_ready(t, WTRIG_DROP) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      add_obj_uid_var(t, "object", obj, 0);
      ret_val = script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
      if (obj_get_carried_by(obj) != actor)
        return 0;
      else
        return ret_val;
      break;
    }

  return 1;
}

int cast_wtrigger(GameWorld *g, char_data *actor, char_data *vict, obj_data *obj, int spellnum)
{
  room_data *room;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!actor || !script_has_type(rroom_get_script(g, char_get_in_room(actor)), WTRIG_CAST))
    return 1;

  room = &g->rooms[char_get_in_room(actor)];
  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_CAST) &&
        (rand_number(g, 1, 100) <= t->narg)) {

      add_char_uid_var(t, "actor", actor, 0);
      if (vict)
        add_char_uid_var(t, "victim", vict, 0);
      if (obj)
        add_obj_uid_var(t, "object", obj, 0);
      sprintf(buf, "%d", spellnum);
      add_var(&t->var_list, "spell", buf, 0);
      add_var(&t->var_list, "spellname", (const char *)skill_name(g, spellnum), 0);
      return script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int leave_wtrigger(GameWorld *g, struct room_data *room, char_data *actor, int dir)
{
  trig_data *t;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return 1;

  if (!script_has_type(room_get_script(room), WTRIG_LEAVE))
    return 1;

  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_LEAVE) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      if (dir >= 0 && dir < NUM_OF_DIRS)
        add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
      else
        add_var(&t->var_list, "direction", "none", 0);
      add_char_uid_var(t, "actor", actor, 0);
      return script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int door_wtrigger(GameWorld *g, char_data *actor, int subcmd, int dir)
{
  room_data *room;
  trig_data *t;

  if (!actor || !script_has_type(rroom_get_script(g, char_get_in_room(actor)), WTRIG_DOOR))
    return 1;

  room = &g->rooms[char_get_in_room(actor)];
  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_DOOR) &&
        (rand_number(g, 1, 100) <= t->narg)) {
      add_var(&t->var_list, "cmd", (const char *)cmd_door[subcmd], 0);
      if (dir >= 0 && dir < NUM_OF_DIRS)
        add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
      else
        add_var(&t->var_list, "direction", "none", 0);
      add_char_uid_var(t, "actor", actor, 0);
      return script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

void time_wtrigger(GameWorld *g, struct room_data *room)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!script_has_type(room_get_script(room), WTRIG_TIME))
    return;

  for (t = room_get_script(room)->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_TIME) &&
        (g->time_info.hours == t->narg)) {
      sprintf(buf, "%d", g->time_info.hours);
      add_var(&t->var_list, "time", buf, 0);
      script_driver(g, &room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}
