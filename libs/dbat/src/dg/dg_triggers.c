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

void random_mtrigger(char_data *ch)
{
  trig_data *t;

  /*
   * This trigger is only called if a char is in the zone without nohassle.
   */

  if (!script_has_type(ch->script, MTRIG_RANDOM) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_RANDOM) &&
        (rand_number(1, 100) <= t->narg)) {
      script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void bribe_mtrigger(char_data *ch, char_data *actor, int amount)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!script_has_type(ch->script, MTRIG_BRIBE) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_BRIBE) && (amount >= t->narg)) {
      snprintf(buf, sizeof(buf), "%d", amount);
      add_var(&t->var_list, "amount", buf, 0);
      add_char_uid_var(t, "actor", actor, 0);
      script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void greet_memory_mtrigger(char_data *actor)
{
  trig_data *t;
  char_data *ch;
  struct script_memory *mem;
  int command_performed = 0;

  if (!valid_dg_target(actor, DG_ALLOW_GODS))
    return;

  for (ch = world[char_get_in_room(actor)].people; ch; ch = ch->next_in_room) {
    if (!ch->memory || !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;
    /* find memory line with command only */
    for (mem = ch->memory; mem && ch->memory; mem = mem->next) {
      if (char_get_id(actor) != mem->id) continue;
      if (mem->cmd) {
        command_interpreter(ch, mem->cmd); /* no script */
        command_performed = 1;
        break;
      }
      /* if a command was not performed execute the memory script */
      if (mem && !command_performed) {
        for (t = ch->script->trig_list; t; t = t->next) {
          if (IS_SET(t->trigger_type, MTRIG_MEMORY) &&
              char_can_see(ch, actor) &&
              !t->depth &&
              rand_number(1, 100) <= t->narg) {
            add_char_uid_var(t, "actor", actor, 0);
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
          }
        }
      }
      /* delete the memory */
      if (mem) {
        if (ch->memory == mem) {
          ch->memory = mem->next;
        } else {
          struct script_memory *prev;
          prev = ch->memory;
          while (prev->next != mem) prev = prev->next;
          prev->next = mem->next;
        }
        if (mem->cmd) free(mem->cmd);
        free(mem);
      }
    }
  }
}

int greet_mtrigger(char_data *actor, int dir)
{
  trig_data *t;
  char_data *ch;
  int intermediate, final = TRUE;

  if (!valid_dg_target(actor, DG_ALLOW_GODS))
    return TRUE;

  for (ch = world[char_get_in_room(actor)].people; ch; ch = ch->next_in_room) {
    if (!script_has_type(ch->script, MTRIG_GREET | MTRIG_GREET_ALL) ||
        !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;
    if (!script_has_type(ch->script, MTRIG_GREET_ALL) && char_is_npc(actor))
      continue;

    for (t = ch->script->trig_list; t; t = t->next) {
      if (((IS_SET(t->trigger_type, MTRIG_GREET) && char_can_see(ch, actor)) ||
           IS_SET(t->trigger_type, MTRIG_GREET_ALL)) &&
          !t->depth && (rand_number(1, 100) <= t->narg)) {
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[rev_dir[dir]], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        intermediate = script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
        if (!intermediate) final = FALSE;
        continue;
      }
    }
  }
  return final;
}

void entry_memory_mtrigger(char_data *ch)
{
  trig_data *t;
  char_data *actor;
  struct script_memory *mem;

  if (!ch->memory || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (actor = world[char_get_in_room(ch)].people; actor && ch->memory;
       actor = actor->next_in_room) {
    if (actor != ch && ch->memory) {
      for (mem = ch->memory; mem && ch->memory; mem = mem->next) {
        if (char_get_id(actor) == mem->id) {
          struct script_memory *prev;
          if (mem->cmd) command_interpreter(ch, mem->cmd);
          else {
            for (t = ch->script->trig_list; t; t = t->next) {
              if (trigger_ready(t, MTRIG_MEMORY) && (rand_number(1, 100) <=
                                                     t->narg)) {
                add_char_uid_var(t, "actor", actor, 0);
                script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
                break;
              }
            }
          }
          /* delete the memory */
          if (ch->memory == mem) {
            ch->memory = mem->next;
          } else {
            prev = ch->memory;
            while (prev->next != mem) prev = prev->next;
            prev->next = mem->next;
          }
          if (mem->cmd) free(mem->cmd);
          free(mem);
        }
      } /* for (mem =..... */
    }
  }
}

int entry_mtrigger(char_data *ch)
{
  trig_data *t;

  if (!script_has_type(ch->script, MTRIG_ENTRY) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_ENTRY) && (rand_number(1, 100) <= t->narg)) {
      return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }

  return 1;
}

int command_mtrigger(char_data *actor, char *cmd, char *argument)
{
  char_data *ch, *ch_next;
  trig_data *t;

  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(actor, 0))
    return 0;

  for (ch = world[char_get_in_room(actor)].people; ch; ch = ch_next) {
    ch_next = ch->next_in_room;

    if (script_has_type(ch->script, MTRIG_COMMAND) && !char_aff_flagged(ch, AFF_CHARM) &&
        (actor != ch)) {
      for (t = ch->script->trig_list; t; t = t->next) {
        if (!trigger_ready(t, MTRIG_COMMAND))
          continue;

        if (!t->arglist || !*t->arglist) {
          mudlog(LOG_NRM, ADMLVL_BUILDER, TRUE, "SYSERR: Command Trigger #%d has no text argument!",
                 trig_get_vnum(t));
          continue;
        }

        if (*t->arglist == '*' ||
            !strncasecmp(t->arglist, cmd, strlen(t->arglist))) {
          add_char_uid_var(t, "actor", actor, 0);
          skip_spaces(&argument);
          add_var(&t->var_list, "arg", argument, 0);
          skip_spaces(&cmd);
          add_var(&t->var_list, "cmd", cmd, 0);

          if (script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW))
            return 1;
        }
      }
    }
  }

  return 0;
}

void speech_mtrigger(char_data *actor, char *str)
{
  char_data *ch, *ch_next;
  trig_data *t;

  for (ch = world[char_get_in_room(actor)].people; ch; ch = ch_next) {
    ch_next = ch->next_in_room;

    if (script_has_type(ch->script, MTRIG_SPEECH) && char_is_awake(ch) &&
        !char_aff_flagged(ch, AFF_CHARM) && (actor != ch))
      for (t = ch->script->trig_list; t; t = t->next) {
        if (!trigger_ready(t, MTRIG_SPEECH))
          continue;

        if (!t->arglist || !*t->arglist) {
          mudlog(LOG_NRM, ADMLVL_BUILDER, TRUE, "SYSERR: Speech Trigger #%d has no text argument!",
                 trig_get_vnum(t));
          continue;
        }

        if (((t->narg && word_check(str, t->arglist)) ||
             (!t->narg && is_substring(t->arglist, str)))) {
          add_char_uid_var(t, "actor", actor, 0);
          add_var(&t->var_list, "speech", str, 0);
          script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
          break;
        }
      }
  }
}

void act_mtrigger(const char_data *ch, char *str, char_data *actor, const char_data *victim, obj_data *object, const obj_data *target, const char *arg)
{
  trig_data *t;

  if (script_has_type(ch->script, MTRIG_ACT) && !char_aff_flagged(ch, AFF_CHARM) &&
      (actor != ch))
    for (t = ch->script->trig_list; t; t = t->next) {
      if (!trigger_ready(t, MTRIG_ACT))
        continue;

      if (!t->arglist || !*t->arglist) {
        mudlog(LOG_NRM, ADMLVL_BUILDER, TRUE, "SYSERR: Act Trigger #%d has no text argument!",
               trig_get_vnum(t));
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
        script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
        break;
      }
    }
}

void fight_mtrigger(char_data *ch)
{
  struct char_data *actor;
  trig_data *t;

  if (!script_has_type(ch->script, MTRIG_FIGHT) || !char_get_fighting(ch) ||
      char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_FIGHT) &&
        (rand_number(1, 100) <= t->narg)) {
      actor = char_get_fighting(ch);
      if (actor)
        add_char_uid_var(t, "actor", actor, 0);
      else
        add_var(&t->var_list, "actor", "nobody", 0);

      script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void hitprcnt_mtrigger(char_data *ch)
{
  struct char_data *actor;
  trig_data *t;

  if (!script_has_type(ch->script, MTRIG_HITPRCNT) || !char_get_fighting(ch) ||
      char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_HITPRCNT) && getEffMaxPL(ch) &&
        (getCurPL(ch) <= (getEffMaxPL(ch) / 100) * t->narg)) {

      actor = char_get_fighting(ch);
      add_char_uid_var(t, "actor", actor, 0);
      script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

int receive_mtrigger(char_data *ch, char_data *actor, obj_data *obj)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(ch->script, MTRIG_RECEIVE) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_RECEIVE) &&
        (rand_number(1, 100) <= t->narg)) {

      add_char_uid_var(t, "actor", actor, 0);
      add_obj_uid_var(t, "object", obj, 0);
      ret_val = script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      if (char_is_dead(actor) || char_is_dead(ch) || obj->carried_by != actor)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

int death_mtrigger(char_data *ch, char_data *actor)
{
  trig_data *t;

  if (!script_has_type(ch->script, MTRIG_DEATH) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_DEATH) &&
        (rand_number(1, 100) <= t->narg)) {
      if (actor)
        add_char_uid_var(t, "actor", actor, 0);
      return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

void load_mtrigger(char_data *ch)
{
  trig_data *t;
  int result = 0;

  if (!script_has_type(ch->script, MTRIG_LOAD))
    return;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_LOAD) &&
        (rand_number(1, 100) <= t->narg)) {
      result = script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
  if (result == SCRIPT_ERROR_CODE) {
    /* we have recursed beyond reasonable depth */
    /* make sure this mob is the last one in the load chain */
    if (char_get_nr(ch) != NOBODY) {
      free_proto_script(&mob_proto[char_get_nr(ch)], MOB_TRIGGER);
    }
  }
}

int cast_mtrigger(char_data *actor, char_data *ch, int spellnum)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (ch == NULL)
    return 1;

  if (!script_has_type(ch->script, MTRIG_CAST) || char_aff_flagged(ch, AFF_CHARM))
    return 1;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_CAST) &&
        (rand_number(1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      sprintf(buf, "%d", spellnum);
      add_var(&t->var_list, "spell", buf, 0);
      add_var(&t->var_list, "spellname", (const char *)skill_name(spellnum), 0);
      return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int leave_mtrigger(char_data *actor, int dir)
{
  trig_data *t;
  char_data *ch;

  if (!valid_dg_target(actor, DG_ALLOW_GODS))
    return 1;

  for (ch = world[char_get_in_room(actor)].people; ch; ch = ch->next_in_room) {
    if (!script_has_type(ch->script, MTRIG_LEAVE) ||
        !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;

    for (t = ch->script->trig_list; t; t = t->next) {
      if ((IS_SET(t->trigger_type, MTRIG_LEAVE)) &&
          !t->depth && (rand_number(1, 100) <= t->narg)) {
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      }
    }
  }
  return 1;
}

int door_mtrigger(char_data *actor, int subcmd, int dir)
{
  trig_data *t;
  char_data *ch;

  for (ch = world[char_get_in_room(actor)].people; ch; ch = ch->next_in_room) {
    if (!script_has_type(ch->script, MTRIG_DOOR) ||
        !char_is_awake(ch) || char_get_fighting(ch) || (ch == actor) ||
        char_aff_flagged(ch, AFF_CHARM))
      continue;

    for (t = ch->script->trig_list; t; t = t->next) {
      if (IS_SET(t->trigger_type, MTRIG_DOOR) && char_can_see(ch, actor) &&
          !t->depth && (rand_number(1, 100) <= t->narg)) {
        add_var(&t->var_list, "cmd", (const char *)cmd_door[subcmd], 0);
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      }
    }
  }
  return 1;
}

void time_mtrigger(char_data *ch)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  /*
   * This trigger is called if the hour is the same as specified in Narg.
   */

  if (!script_has_type(ch->script, MTRIG_TIME) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (t = ch->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, MTRIG_TIME) &&
        (time_info.hours == t->narg)) {
      sprintf(buf, "%d", time_info.hours);
      add_var(&t->var_list, "time", buf, 0);
      script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

/*
 *  object triggers
 */

void random_otrigger(obj_data *obj)
{
  trig_data *t;

  if (!script_has_type(obj->script, OTRIG_RANDOM))
    return;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_RANDOM) &&
        (rand_number(1, 100) <= t->narg)) {
      script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void timer_otrigger(struct obj_data *obj)
{
  trig_data *t;

  if (!script_has_type(obj->script, OTRIG_TIMER))
    return;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_TIMER)) {
      script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
    }
  }

  return;
}

int get_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  int ret_val;
  if (!script_has_type(obj->script, OTRIG_GET))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_GET) && (rand_number(1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
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
static int cmd_otrig(obj_data *obj, char_data *actor, char *cmd,
                     char *argument, int type)
{
  trig_data *t;

  if (obj && script_has_type(obj->script, OTRIG_COMMAND))
    for (t = obj->script->trig_list; t; t = t->next) {
      if (!trigger_ready(t, OTRIG_COMMAND))
        continue;

      if (IS_SET(t->narg, type) &&
          (!t->arglist || !*t->arglist)) {
        mudlog(LOG_NRM, ADMLVL_BUILDER, TRUE, "SYSERR: O-Command Trigger #%d has no text argument!",
               trig_get_vnum(t));
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

        if (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW))
          return 1;
      }
    }

  return 0;
}

int command_otrigger(char_data *actor, char *cmd, char *argument)
{
  obj_data *obj;
  int i;

  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(actor, 0))
    return 0;

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(actor, i))
      if (cmd_otrig(char_get_equipment(actor, i), actor, cmd, argument, OCMD_EQUIP) && !obj_extra_flagged(char_get_equipment(actor, i), ITEM_FORGED))
        return 1;

  for (obj = actor->carrying; obj; obj = obj->next_content)
    if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN) && !obj_extra_flagged(obj, ITEM_FORGED))
      return 1;

  for (obj = world[char_get_in_room(actor)].contents; obj; obj = obj->next_content)
    if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM) && !obj_extra_flagged(obj, ITEM_FORGED))
      return 1;

  return 0;
}

int wear_otrigger(obj_data *obj, char_data *actor, int where)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj->script, OTRIG_WEAR))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_WEAR)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
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

int remove_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj->script, OTRIG_REMOVE))
    return 1;

  if (!valid_dg_target(actor, 0))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_REMOVE)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
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

int drop_otrigger(obj_data *obj, char_data *actor)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj->script, OTRIG_DROP))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_DROP) && (rand_number(1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      ret_val = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
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

int give_otrigger(obj_data *obj, char_data *actor, char_data *victim)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj->script, OTRIG_GIVE))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_GIVE) && (rand_number(1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      add_char_uid_var(t, "victim", victim, 0);
      ret_val = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
      /* don't allow a give to take place, if
       * a) the object is purged.
       * b) the object is not carried by the giver.
       */
      if (!obj || obj->carried_by != actor)
        return 0;
      else
        return ret_val;
    }
  }

  return 1;
}

void load_otrigger(obj_data *obj)
{
  trig_data *t;
  int result = 0;

  if (!script_has_type(obj->script, OTRIG_LOAD))
    return;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_LOAD) &&
        (rand_number(1, 100) <= t->narg)) {
      result = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
      break;
    }
  }
  if (result == SCRIPT_ERROR_CODE) {
    /* we have recursed beyond reasonable depth */
    /* make sure this mob is the last one in the load chain */
    if (obj_get_item_number(obj) != NOTHING) {
      free_proto_script(&obj_proto[obj_get_item_number(obj)], OBJ_TRIGGER);
    }
  }
}

int cast_otrigger(char_data *actor, obj_data *obj, int spellnum)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (obj == NULL)
    return 1;

  if (!script_has_type(obj->script, OTRIG_CAST))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_CAST) &&
        (rand_number(1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      sprintf(buf, "%d", spellnum);
      add_var(&t->var_list, "spell", buf, 0);
      add_var(&t->var_list, "spellname", (const char *)skill_name(spellnum), 0);
      return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int leave_otrigger(room_data *room, char_data *actor, int dir)
{
  trig_data *t;
  int temp, final = 1;
  obj_data *obj, *obj_next;

  if (!valid_dg_target(actor, DG_ALLOW_GODS))
    return 1;

  for (obj = room->contents; obj; obj = obj_next) {
    obj_next = obj->next_content;
    if (!script_has_type(obj->script, OTRIG_LEAVE))
      continue;

    for (t = obj->script->trig_list; t; t = t->next) {
      if (trigger_ready(t, OTRIG_LEAVE) &&
          (rand_number(1, 100) <= t->narg)) {
        if (dir >= 0 && dir < NUM_OF_DIRS)
          add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
        else
          add_var(&t->var_list, "direction", "none", 0);
        add_char_uid_var(t, "actor", actor, 0);
        temp = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
        if (temp == 0)
          final = 0;
      }
    }
  }

  return final;
}

int consume_otrigger(obj_data *obj, char_data *actor, int cmd)
{
  trig_data *t;
  int ret_val;

  if (!script_has_type(obj->script, OTRIG_CONSUME))
    return 1;

  for (t = obj->script->trig_list; t; t = t->next) {
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
      ret_val = script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
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

void time_otrigger(obj_data *obj)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!script_has_type(obj->script, OTRIG_TIME))
    return;

  for (t = obj->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, OTRIG_TIME) &&
        (time_info.hours == t->narg)) {
      sprintf(buf, "%d", time_info.hours);
      add_var(&t->var_list, "time", buf, 0);
      script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

/*
 *  world triggers
 */

void reset_wtrigger(struct room_data *room)
{
  trig_data *t;

  if (!script_has_type(room->script, WTRIG_RESET))
    return;

  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_RESET) &&
        (rand_number(1, 100) <= t->narg)) {
      script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

void random_wtrigger(struct room_data *room)
{
  trig_data *t;

  if (!script_has_type(room->script, WTRIG_RANDOM))
    return;

  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_RANDOM) &&
        (rand_number(1, 100) <= t->narg)) {
      script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

int enter_wtrigger(struct room_data *room, char_data *actor, int dir)
{
  trig_data *t;

  if (!script_has_type(room->script, WTRIG_ENTER))
    return 1;

  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_ENTER) &&
        (rand_number(1, 100) <= t->narg)) {
      if (dir >= 0 && dir < NUM_OF_DIRS)
        add_var(&t->var_list, "direction", (const char *)dirs[rev_dir[dir]], 0);
      else
        add_var(&t->var_list, "direction", "none", 0);
      add_char_uid_var(t, "actor", actor, 0);
      return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int command_wtrigger(char_data *actor, char *cmd, char *argument)
{
  struct room_data *room;
  trig_data *t;

  if (!actor || !script_has_type(world[char_get_in_room(actor)].script, WTRIG_COMMAND))
    return 0;

  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(actor, 0))
    return 0;

  room = &world[char_get_in_room(actor)];
  for (t = room->script->trig_list; t; t = t->next) {
    if (!trigger_ready(t, WTRIG_COMMAND))
      continue;

    if (!t->arglist || !*t->arglist) {
      mudlog(LOG_NRM, ADMLVL_BUILDER, TRUE, "SYSERR: W-Command Trigger #%d has no text argument!",
             trig_get_vnum(t));
      continue;
    }

    if (*t->arglist == '*' ||
        !strncasecmp(t->arglist, cmd, strlen(t->arglist))) {
      add_char_uid_var(t, "actor", actor, 0);
      skip_spaces(&argument);
      add_var(&t->var_list, "arg", argument, 0);
      skip_spaces(&cmd);
      add_var(&t->var_list, "cmd", cmd, 0);

      return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 0;
}

void speech_wtrigger(char_data *actor, char *str)
{
  struct room_data *room;
  trig_data *t;

  if (!actor || !script_has_type(world[char_get_in_room(actor)].script, WTRIG_SPEECH))
    return;

  room = &world[char_get_in_room(actor)];
  for (t = room->script->trig_list; t; t = t->next) {
    if (!trigger_ready(t, WTRIG_SPEECH))
      continue;

    if (!t->arglist || !*t->arglist) {
      mudlog(LOG_NRM, ADMLVL_BUILDER, TRUE, "SYSERR: W-Speech Trigger #%d has no text argument!",
             trig_get_vnum(t));
      continue;
    }

    if (*t->arglist == '*' ||
        (t->narg && word_check(str, t->arglist)) ||
        (!t->narg && is_substring(t->arglist, str))) {
      add_char_uid_var(t, "actor", actor, 0);
      add_var(&t->var_list, "speech", str, 0);
      script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}

int drop_wtrigger(obj_data *obj, char_data *actor)
{
  struct room_data *room;
  trig_data *t;
  int ret_val;

  if (!actor || !script_has_type(world[char_get_in_room(actor)].script, WTRIG_DROP))
    return 1;

  room = &world[char_get_in_room(actor)];
  for (t = room->script->trig_list; t; t = t->next)
    if (trigger_ready(t, WTRIG_DROP) &&
        (rand_number(1, 100) <= t->narg)) {
      add_char_uid_var(t, "actor", actor, 0);
      add_obj_uid_var(t, "object", obj, 0);
      ret_val = script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
      if (obj->carried_by != actor)
        return 0;
      else
        return ret_val;
      break;
    }

  return 1;
}

int cast_wtrigger(char_data *actor, char_data *vict, obj_data *obj, int spellnum)
{
  room_data *room;
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!actor || !script_has_type(world[char_get_in_room(actor)].script, WTRIG_CAST))
    return 1;

  room = &world[char_get_in_room(actor)];
  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_CAST) &&
        (rand_number(1, 100) <= t->narg)) {

      add_char_uid_var(t, "actor", actor, 0);
      if (vict)
        add_char_uid_var(t, "victim", vict, 0);
      if (obj)
        add_obj_uid_var(t, "object", obj, 0);
      sprintf(buf, "%d", spellnum);
      add_var(&t->var_list, "spell", buf, 0);
      add_var(&t->var_list, "spellname", (const char *)skill_name(spellnum), 0);
      return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int leave_wtrigger(struct room_data *room, char_data *actor, int dir)
{
  trig_data *t;

  if (!valid_dg_target(actor, DG_ALLOW_GODS))
    return 1;

  if (!script_has_type(room->script, WTRIG_LEAVE))
    return 1;

  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_LEAVE) &&
        (rand_number(1, 100) <= t->narg)) {
      if (dir >= 0 && dir < NUM_OF_DIRS)
        add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
      else
        add_var(&t->var_list, "direction", "none", 0);
      add_char_uid_var(t, "actor", actor, 0);
      return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

int door_wtrigger(char_data *actor, int subcmd, int dir)
{
  room_data *room;
  trig_data *t;

  if (!actor || !script_has_type(world[char_get_in_room(actor)].script, WTRIG_DOOR))
    return 1;

  room = &world[char_get_in_room(actor)];
  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_DOOR) &&
        (rand_number(1, 100) <= t->narg)) {
      add_var(&t->var_list, "cmd", (const char *)cmd_door[subcmd], 0);
      if (dir >= 0 && dir < NUM_OF_DIRS)
        add_var(&t->var_list, "direction", (const char *)dirs[dir], 0);
      else
        add_var(&t->var_list, "direction", "none", 0);
      add_char_uid_var(t, "actor", actor, 0);
      return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
    }
  }

  return 1;
}

void time_wtrigger(struct room_data *room)
{
  trig_data *t;
  char buf[MAX_INPUT_LENGTH];

  if (!script_has_type(room->script, WTRIG_TIME))
    return;

  for (t = room->script->trig_list; t; t = t->next) {
    if (trigger_ready(t, WTRIG_TIME) &&
        (time_info.hours == t->narg)) {
      sprintf(buf, "%d", time_info.hours);
      add_var(&t->var_list, "time", buf, 0);
      script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
      break;
    }
  }
}
