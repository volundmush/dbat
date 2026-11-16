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
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/dg/expr.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/oasis.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/spells.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/args.hpp"

#include "dg_internal.hpp"

/*
 * Every dispatcher below fires its owner's triggers of one type through fire(): it says which of them fire (the
 * test), what variables each gets before it runs, and whether to go on to the next after one has run. What differs
 * between them is only that, and what they make of the result.
 */
namespace {

/* What fire() does after a trigger has run */
enum class then { stop, go_on };

/* Most dispatchers run only the first trigger that fires */
const auto stop_at_first = [](const run_result &) { return then::stop; };
const auto no_vars       = [](trig_data *) {};

/* Whether t can run now: it compiled and isn't running already */
bool ready(GameWorld *g, trig_data *t)
{
  return trigger_ready(g, t, t->trigger_type);
}

/* Whether t's chance comes up: the percentage in its number */
bool chance(GameWorld *g, trig_data *t)
{
  return rand_number(g, 1, 100) <= t->narg;
}

/* The usual test: ready, then its chance comes up */
auto ready_and_chance(GameWorld *g)
{
  return [g](trig_data *t) { return ready(g, t) && chance(g, t); };
}

auto ready_only(GameWorld *g)
{
  return [g](trig_data *t) { return ready(g, t); };
}

/*
 * Fires owner's triggers of type, in order: each that test(t) says fires (test is asked only of triggers of the type,
 * and calls ready() itself, so that what it checks comes in the order the dispatcher always had) gets its variables
 * from vars(t) and runs. after(result) says whether to stop there. Returns the last run's result, or nothing if none
 * ran.
 */
template <class Test, class Vars, class After>
std::optional<run_result> fire(GameWorld *g, owner_ptr owner, bitvector_t type, Test &&test, Vars &&vars, After &&after)
{
  struct script_data *sc = *script_slot(owner);
  if (!script_has_type(sc, type))
    return std::nullopt;

  std::optional<run_result> last;
  /* a trigger removed by an earlier one stays in memory until the pulse ends, and runs no more */
  for (trig_data *t = sc->trig_list; t; t = t->next) {
    if (!(t->trigger_type & type) || !test(t))
      continue;
    vars(t);
    last = script_driver(g, owner, t, TRIG_NEW);
    if (after(*last) == then::stop)
      break;
  }
  return last;
}

/* Sets %direction%: dir's name, or the one it comes from, or "none" */
void set_direction(trig_data *t, int dir, bool coming_from)
{
  if (dir >= 0 && dir < NUM_OF_DIRS)
    add_var(t->var_list, "direction", dirs[coming_from ? rev_dir[dir] : dir], 0);
  else
    add_var(t->var_list, "direction", "none", 0);
}

/* Sets %spell% and %spellname% */
void set_spell(GameWorld *g, trig_data *t, int spellnum)
{
  add_var(t->var_list, "spell", str_format("{:d}", spellnum), 0);
  add_var(t->var_list, "spellname", skill_name(g, spellnum), 0);
}

/* Whether t has the text it needs; if not, that's logged as a 'what' trigger with none */
bool has_text(GameWorld *g, trig_data *t, std::string_view what)
{
  if (!t->arglist.empty())
    return true;
  mudlog(g, LOG_NRM, ADMLVL_BUILDER, true, "SYSERR: {} Trigger #{:d} has no text argument!", what, trig_get_vnum(g, t));
  return false;
}

/* Whether a command trigger's text starts cmd, ignoring case, or is '*' for any command */
bool command_matches(const trig_data *t, std::string_view cmd)
{
  const std::string &word = t->arglist;
  return word[0] == '*' || (cmd.size() >= word.size() && str_eq_nocase(cmd.substr(0, word.size()), word));
}

/* Sets a command trigger's %actor%, %arg% and %cmd% */
void set_command_vars(trig_data *t, char_data *actor, std::string_view cmd, std::string_view argument)
{
  add_char_uid_var(t, "actor", actor, 0);
  add_var(t->var_list, "arg", skip_spaces(argument), 0);
  add_var(t->var_list, "cmd", skip_spaces(cmd), 0);
}

/*
 * Where sub is first found in text, ignoring case, or npos.
 * quirk: after a partial match it carries on from where that failed, not from the next character, as str_str()
 * always has, so "aaab" isn't found in "aaaab".
 */
size_t find_nocase(std::string_view text, std::string_view sub)
{
  if (sub.empty())
    return std::string_view::npos;
  size_t at = 0;
  while (at < text.size()) {
    size_t t = 0;
    while (at < text.size() && tolower(text[at]) != tolower(sub[t]))
      at++;
    size_t start = at;
    while (t < sub.size() && at < text.size() && tolower(text[at]) == tolower(sub[t])) {
      t++;
      at++;
    }
    if (t == sub.size())
      return start;
  }
  return std::string_view::npos;
}

/* Whether sub is in text as a whole word or words, between spaces, punctuation or the ends (the first time only) */
bool is_substring(std::string_view sub, std::string_view text)
{
  size_t s = find_nocase(text, sub);
  if (s == std::string_view::npos)
    return false;
  size_t end     = s + sub.size();
  auto separates = [](char c) { return isspace(c) || ispunct(c); };
  return (s == 0 || separates(text[s - 1])) && (end == text.size() || separates(text[end]));
}

/* The next word of text, or "phrase" in double quotes, taking it off text; "" when there is none */
std::string_view take_phrase(std::string_view &text)
{
  text = skip_spaces(text);
  if (text.empty())
    return {};

  if (text[0] == '"') {
    size_t close = dg::matching_quote(text, 0); /* quirk: with no closing quote, the last character is left off */
    std::string_view phrase = close ? text.substr(1, close - 1) : std::string_view();
    text.remove_prefix(close + 1);
    return phrase;
  }

  size_t end = 0;
  while (end < text.size() && !isspace(text[end]) && text[end] != '"')
    end++;
  std::string_view word = text.substr(0, end);
  text.remove_prefix(end);
  return word;
}

/* Whether text has one of the words or "phrases" in wordlist, or wordlist is "*" */
bool word_check(std::string_view text, std::string_view wordlist)
{
  if (wordlist.starts_with('*'))
    return true;
  /* quirk: an empty "" ends the list */
  for (std::string_view phrase = take_phrase(wordlist); !phrase.empty(); phrase = take_phrase(wordlist))
    if (is_substring(phrase, text))
      return true;
  return false;
}

/* Whether a speech or act trigger's text is in what was said: any of its words if its number is set, else all of it */
bool speech_matches(const trig_data *t, std::string_view said)
{
  return t->narg ? word_check(said, t->arglist) : is_substring(t->arglist, said);
}

/* A mob too charmed, asleep or busy to notice actor, or actor itself */
bool inattentive(GameWorld *g, char_data *ch, const char_data *actor)
{
  return !char_is_awake(ch) || char_get_fighting(g, ch) || ch == actor || char_aff_flagged(ch, AFF_CHARM);
}

} // namespace

/*
 *  mob triggers
 */

/* called only while someone without nohassle is in the zone */
void random_mtrigger(GameWorld *g, char_data *ch)
{
  if (char_aff_flagged(ch, AFF_CHARM))
    return;
  fire(g, ch, MTRIG_RANDOM, ready_and_chance(g), no_vars, stop_at_first);
}

void bribe_mtrigger(GameWorld *g, char_data *ch, char_data *actor, int amount)
{
  if (char_aff_flagged(ch, AFF_CHARM))
    return;
  fire(g, ch, MTRIG_BRIBE, [&](trig_data *t) { return ready(g, t) && amount >= t->narg; },
       [&](trig_data *t) {
         add_var(t->var_list, "amount", str_format("{:d}", amount), 0);
         add_char_uid_var(t, "actor", actor, 0);
       },
       stop_at_first);
}

void greet_memory_mtrigger(GameWorld *g, char_data *actor)
{
  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return;

  for (char_data *ch : room_people(g, char_get_in_room(actor))) {
    if (!char_get_memory(ch) || inattentive(g, ch, actor))
      continue;
    /* find memory line with command only */
    struct script_memory *mem = char_get_memory(ch);
    while (mem && mem->id != char_get_id(actor))
      mem = mem->next;
    if (!mem)
      continue;

    long id = mem->id;
    if (!mem->cmd.empty()) {
      std::string cmd = mem->cmd; /* a copy: the command can forget the memory */
      command_interpreter(g, ch, cmd);    /* no script */
    } else /* if a command was not performed execute the memory script */
      fire(g, ch, MTRIG_MEMORY, [&](trig_data *t) { return char_can_see(g, ch, actor) && ready(g, t) && chance(g, t); },
           [&](trig_data *t) { add_char_uid_var(t, "actor", actor, 0); }, stop_at_first);

    /* delete the memory, if what just ran didn't: it can use mforget */
    struct script_memory **link = char_memory_slot(ch);
    while (*link && (*link)->id != id)
      link = &(*link)->next;
    if (struct script_memory *gone = *link) {
      *link = gone->next;
      delete gone;
    }
  }
}

bool greet_mtrigger(GameWorld *g, char_data *actor, int dir)
{
  bool final = true;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return true;

  for (char_data *ch : room_people(g, char_get_in_room(actor))) {
    if (!script_has_type(char_get_script(ch), MTRIG_GREET | MTRIG_GREET_ALL) || inattentive(g, ch, actor))
      continue;
    if (!script_has_type(char_get_script(ch), MTRIG_GREET_ALL) && char_is_npc(actor))
      continue;

    /* every greeting runs, and any one of them can stop the actor */
    fire(g, ch, MTRIG_GREET | MTRIG_GREET_ALL,
         [&](trig_data *t) {
           return ((IS_SET(t->trigger_type, MTRIG_GREET) && char_can_see(g, ch, actor)) || IS_SET(t->trigger_type, MTRIG_GREET_ALL)) &&
                  ready(g, t) && chance(g, t);
         },
         [&](trig_data *t) {
           set_direction(t, dir, true);
           add_char_uid_var(t, "actor", actor, 0);
         },
         [&](const run_result &r) {
           if (!r.value)
             final = false;
           return then::go_on;
         });
  }
  return final;
}

void entry_memory_mtrigger(GameWorld *g, char_data *ch)
{
  struct script_memory *mem, *next_mem;

  if (!char_get_memory(ch) || char_aff_flagged(ch, AFF_CHARM))
    return;

  for (char_data *actor : room_people(g, char_get_in_room(ch))) {
    if (!char_get_memory(ch))
      break;
    if (actor == ch)
      continue;
    for (mem = char_get_memory(ch); mem; mem = next_mem) {
      next_mem = mem->next;
      if (char_get_id(actor) != mem->id)
        continue;

      /* unlink it first: the command or trigger below may change ch->memory (mforget) or purge ch */
      struct script_memory **link = char_memory_slot(ch);
      while (*link != mem)
        link = &(*link)->next;
      *link = mem->next;

      if (!mem->cmd.empty())
        command_interpreter(g, ch, mem->cmd);
      else
        fire(g, ch, MTRIG_MEMORY, ready_and_chance(g), [&](trig_data *t) { add_char_uid_var(t, "actor", actor, 0); }, stop_at_first);
      delete mem;
      if (!char_get_memory(ch))
        return;

      /* carry on only if the next memory survived */
      struct script_memory *p = char_get_memory(ch);
      while (p && p != next_mem)
        p = p->next;
      next_mem = p;
    }
  }
}

int entry_mtrigger(GameWorld *g, char_data *ch)
{
  if (char_aff_flagged(ch, AFF_CHARM))
    return 1;
  auto r = fire(g, ch, MTRIG_ENTRY, ready_and_chance(g), no_vars, stop_at_first);
  return r ? r->value : 1;
}

bool command_mtrigger(GameWorld *g, char_data *actor, std::string_view cmd, std::string_view argument)
{
  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(g, actor, 0))
    return false;

  for (char_data *ch : room_people(g, char_get_in_room(actor))) {
    if (char_aff_flagged(ch, AFF_CHARM) || actor == ch)
      continue;
    /* each that matches runs, until one takes the command */
    auto r = fire(g, ch, MTRIG_COMMAND,
                  [&](trig_data *t) { return ready(g, t) && has_text(g, t, "Command") && command_matches(t, cmd); },
                  [&](trig_data *t) { set_command_vars(t, actor, cmd, argument); },
                  [](const run_result &ran) { return ran.value ? then::stop : then::go_on; });
    if (r && r->value)
      return true;
  }
  return false;
}

void speech_mtrigger(GameWorld *g, char_data *actor, std::string_view str)
{
  for (char_data *ch : room_people(g, char_get_in_room(actor))) {
    if (!char_is_awake(ch) || char_aff_flagged(ch, AFF_CHARM) || actor == ch)
      continue;
    fire(g, ch, MTRIG_SPEECH, [&](trig_data *t) { return ready(g, t) && has_text(g, t, "Speech") && speech_matches(t, str); },
         [&](trig_data *t) {
           add_char_uid_var(t, "actor", actor, 0);
           add_var(t->var_list, "speech", str, 0);
         },
         stop_at_first);
  }
}

/* str is what ch saw, ending in "\r\n"; arg is unused */
void act_mtrigger(GameWorld *g, const char_data *ch, std::string_view str, char_data *actor, const char_data *victim, obj_data *object,
                  const obj_data *target, const char * /* arg */)
{
  if (char_aff_flagged(ch, AFF_CHARM) || actor == ch)
    return;

  fire(g, const_cast<char_data *>(ch), MTRIG_ACT, [&](trig_data *t) { return ready(g, t) && has_text(g, t, "Act") && speech_matches(t, str); },
       [&](trig_data *t) {
         if (actor)
           add_char_uid_var(t, "actor", actor, 0);
         if (victim)
           add_char_uid_var(t, "victim", victim, 0);
         if (object)
           add_obj_uid_var(t, "object", object, 0);
         if (target)
           add_obj_uid_var(t, "target", target, 0);
         /* %arg% is the line without its leading spaces, up to the "\r\n" */
         size_t cut             = str.find('\r');
         std::string_view arg   = skip_spaces(str);
         size_t start           = str.size() - arg.size();
         if (cut != std::string_view::npos && cut >= start) /* quirk: a line starting "\r\n" keeps all the rest */
           arg = arg.substr(0, cut - start);
         add_var(t->var_list, "arg", arg, 0);
       },
       stop_at_first);
}

void fight_mtrigger(GameWorld *g, char_data *ch)
{
  if (!char_get_fighting(g, ch) || char_aff_flagged(ch, AFF_CHARM))
    return;
  fire(g, ch, MTRIG_FIGHT, ready_and_chance(g),
       [&](trig_data *t) {
         if (char_data *actor = char_get_fighting(g, ch))
           add_char_uid_var(t, "actor", actor, 0);
         else
           add_var(t->var_list, "actor", "nobody", 0);
       },
       stop_at_first);
}

/* fires when the mob's health is down to the percentage in the trigger's number */
void hitprcnt_mtrigger(GameWorld *g, char_data *ch)
{
  if (!char_get_fighting(g, ch) || char_aff_flagged(ch, AFF_CHARM))
    return;
  fire(g, ch, MTRIG_HITPRCNT,
       [&](trig_data *t) { return ready(g, t) && getEffMaxPL(g, ch) && getCurPL(g, ch) <= (getEffMaxPL(g, ch) / 100) * t->narg; },
       [&](trig_data *t) { add_char_uid_var(t, "actor", char_get_fighting(g, ch), 0); }, stop_at_first);
}

int receive_mtrigger(GameWorld *g, char_data *ch, char_data *actor, obj_data *obj)
{
  long id = obj_get_id(obj);

  if (char_aff_flagged(ch, AFF_CHARM))
    return 1;
  auto r = fire(g, ch, MTRIG_RECEIVE, ready_and_chance(g),
                [&](trig_data *t) {
                  add_char_uid_var(t, "actor", actor, 0);
                  add_obj_uid_var(t, "object", obj, 0);
                },
                stop_at_first);
  if (!r)
    return 1;
  /* by id, because plenty of scripts purge what they're given */
  if (char_is_dead(actor) || char_is_dead(ch) || !(obj = find_obj(g, id)) || obj_get_carried_by(obj) != actor)
    return 0;
  return r->value;
}

int death_mtrigger(GameWorld *g, char_data *ch, char_data *actor)
{
  if (char_aff_flagged(ch, AFF_CHARM))
    return 1;
  auto r = fire(g, ch, MTRIG_DEATH, ready_and_chance(g),
                [&](trig_data *t) {
                  if (actor)
                    add_char_uid_var(t, "actor", actor, 0);
                },
                stop_at_first);
  return r ? r->value : 1;
}

void load_mtrigger(GameWorld *g, char_data *ch)
{
  auto r = fire(g, ch, MTRIG_LOAD, ready_and_chance(g), no_vars, stop_at_first);
  /* recursed beyond reasonable depth: make sure this mob is the last one in the load chain */
  if (r && r->value == SCRIPT_ERROR_CODE && char_get_nr(ch) != NOBODY)
    char_proto_script(mob_proto_of(g, char_get_nr(ch))).clear();
}

int cast_mtrigger(GameWorld *g, char_data *actor, char_data *ch, int spellnum)
{
  if (ch == nullptr || char_aff_flagged(ch, AFF_CHARM))
    return 1;
  auto r = fire(g, ch, MTRIG_CAST, ready_and_chance(g),
                [&](trig_data *t) {
                  add_char_uid_var(t, "actor", actor, 0);
                  set_spell(g, t, spellnum);
                },
                stop_at_first);
  return r ? r->value : 1;
}

int leave_mtrigger(GameWorld *g, char_data *actor, int dir)
{
  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return 1;

  for (char_data *ch : room_people(g, char_get_in_room(actor))) {
    if (inattentive(g, ch, actor))
      continue;
    auto r = fire(g, ch, MTRIG_LEAVE, ready_and_chance(g),
                  [&](trig_data *t) {
                    set_direction(t, dir, false);
                    add_char_uid_var(t, "actor", actor, 0);
                  },
                  stop_at_first);
    if (r)
      return r->value;
  }
  return 1;
}

int door_mtrigger(GameWorld *g, char_data *actor, int subcmd, int dir)
{
  for (char_data *ch : room_people(g, char_get_in_room(actor))) {
    if (inattentive(g, ch, actor))
      continue;
    auto r = fire(g, ch, MTRIG_DOOR, [&](trig_data *t) { return char_can_see(g, ch, actor) && ready(g, t) && chance(g, t); },
                  [&](trig_data *t) {
                    add_var(t->var_list, "cmd", cmd_door[subcmd], 0);
                    set_direction(t, dir, false);
                    add_char_uid_var(t, "actor", actor, 0);
                  },
                  stop_at_first);
    if (r)
      return r->value;
  }
  return 1;
}

/* A time trigger fires when the hour is the one in its number, and gets %time% */
static auto at_its_hour(GameWorld *g)
{
  return [g](trig_data *t) { return ready(g, t) && g->time_info.hours == t->narg; };
}

static auto set_time(GameWorld *g)
{
  return [g](trig_data *t) { add_var(t->var_list, "time", str_format("{:d}", g->time_info.hours), 0); };
}

void time_mtrigger(GameWorld *g, char_data *ch)
{
  if (char_aff_flagged(ch, AFF_CHARM))
    return;
  fire(g, ch, MTRIG_TIME, at_its_hour(g), set_time(g), stop_at_first);
}

/*
 *  object triggers
 */

void random_otrigger(GameWorld *g, obj_data *obj)
{
  fire(g, obj, OTRIG_RANDOM, ready_and_chance(g), no_vars, stop_at_first);
}

/* every timer trigger runs */
void timer_otrigger(GameWorld *g, struct obj_data *obj)
{
  fire(g, obj, OTRIG_TIMER, ready_only(g), no_vars, [](const run_result &) { return then::go_on; });
}

int get_otrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  auto r = fire(g, obj, OTRIG_GET, ready_and_chance(g), [&](trig_data *t) { add_char_uid_var(t, "actor", actor, 0); }, stop_at_first);
  if (!r)
    return 1;
  /* no get if the actor was killed (the mud would choke on obj_to_char) or the object purged */
  if (char_is_dead(actor) || r->owner_gone)
    return 0;
  return r->value;
}

/* Runs obj's command triggers for where the actor has it (type), until one takes the command or purges obj */
static bool cmd_otrig(GameWorld *g, obj_data *obj, char_data *actor, std::string_view cmd, std::string_view argument, int type)
{
  if (!obj)
    return false;
  auto r = fire(g, obj, OTRIG_COMMAND,
                [&](trig_data *t) {
                  return ready(g, t) && IS_SET(t->narg, type) && has_text(g, t, "O-Command") && command_matches(t, cmd);
                },
                [&](trig_data *t) { set_command_vars(t, actor, cmd, argument); },
                [](const run_result &ran) { return ran.value || ran.owner_gone ? then::stop : then::go_on; });
  return r && (r->value || r->owner_gone);
}

/* Runs the command triggers on a list of objects, returning true if one of them took the command. */
static bool cmd_otrig_list(GameWorld *g, const id_list &list, char_data *actor, std::string_view cmd, std::string_view argument, int type)
{
  /* a script can purge any object at all: the loop passes over whatever goes */
  for (obj_data *o : obj_list(g, list)) {
    bool forged = obj_extra_flagged(o, ITEM_FORGED);

    if (cmd_otrig(g, o, actor, cmd, argument, type) && !forged)
      return true;
    if (char_is_dead(actor))
      return true;
  }
  return false;
}

bool command_otrigger(GameWorld *g, char_data *actor, std::string_view cmd, std::string_view argument)
{
  /* prevent people we like from becoming trapped :P */
  if (!valid_dg_target(g, actor, 0))
    return false;

  for (int i = 0; i < NUM_WEARS; i++)
    if (obj_data *obj = char_get_equipment(actor, i)) {
      bool forged = obj_extra_flagged(obj, ITEM_FORGED);

      if ((cmd_otrig(g, obj, actor, cmd, argument, OCMD_EQUIP) && !forged) || char_is_dead(actor))
        return true;
    }

  return cmd_otrig_list(g, char_get_inventory(actor), actor, cmd, argument, OCMD_INVEN) ||
         cmd_otrig_list(g, room_get_contents(g, char_get_in_room(actor)), actor, cmd, argument, OCMD_ROOM);
}

/* The result of the first trigger, of wear, remove or consume, that runs; 0 if it purged the object */
static int run_unless_purged(const std::optional<run_result> &r)
{
  if (!r)
    return 1;
  return r->owner_gone ? 0 : r->value;
}

int wear_otrigger(GameWorld *g, obj_data *obj, char_data *actor, int /* where */)
{
  return run_unless_purged(fire(g, obj, OTRIG_WEAR, ready_only(g), [&](trig_data *t) { add_char_uid_var(t, "actor", actor, 0); }, stop_at_first));
}

int remove_otrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  if (!valid_dg_target(g, actor, 0))
    return 1;
  return run_unless_purged(fire(g, obj, OTRIG_REMOVE, ready_only(g), [&](trig_data *t) { add_char_uid_var(t, "actor", actor, 0); }, stop_at_first));
}

int drop_otrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  /* a purged object leaves nothing to drop */
  return run_unless_purged(fire(g, obj, OTRIG_DROP, ready_and_chance(g), [&](trig_data *t) { add_char_uid_var(t, "actor", actor, 0); }, stop_at_first));
}

int give_otrigger(GameWorld *g, obj_data *obj, char_data *actor, char_data *victim)
{
  auto r = fire(g, obj, OTRIG_GIVE, ready_and_chance(g),
                [&](trig_data *t) {
                  add_char_uid_var(t, "actor", actor, 0);
                  add_char_uid_var(t, "victim", victim, 0);
                },
                stop_at_first);
  if (!r)
    return 1;
  /* no give if the object was purged, or the giver no longer has it */
  if (r->owner_gone || obj_get_carried_by(obj) != actor)
    return 0;
  return r->value;
}

bool load_otrigger(GameWorld *g, obj_data *obj)
{
  auto r = fire(g, obj, OTRIG_LOAD, ready_and_chance(g), no_vars, stop_at_first);
  if (!r)
    return true;
  if (r->owner_gone)
    return false;
  /* recursed beyond reasonable depth: make sure this object is the last one in the load chain */
  if (r->value == SCRIPT_ERROR_CODE && obj_get_item_number(obj) != NOTHING)
    obj_proto_script(obj_proto_of(g, obj_get_item_number(obj))).clear();
  return true;
}

int cast_otrigger(GameWorld *g, char_data *actor, obj_data *obj, int spellnum)
{
  if (obj == nullptr)
    return 1;
  auto r = fire(g, obj, OTRIG_CAST, ready_and_chance(g),
                [&](trig_data *t) {
                  add_char_uid_var(t, "actor", actor, 0);
                  set_spell(g, t, spellnum);
                },
                stop_at_first);
  if (!r)
    return 1;
  /* the spell has nothing left to work on, or nobody left to cast it */
  if (r->owner_gone || char_is_dead(actor))
    return 0;
  return r->value;
}

/* every leave trigger on every object in the room runs, and any one of them can stop the actor */
bool leave_otrigger(GameWorld *g, room_data *room, char_data *actor, int dir)
{
  bool final = true;

  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return true;

  for (obj_data *obj : room_contents(g, room))
    fire(g, obj, OTRIG_LEAVE, ready_and_chance(g),
         [&](trig_data *t) {
           set_direction(t, dir, false);
           add_char_uid_var(t, "actor", actor, 0);
         },
         [&](const run_result &r) {
           if (r.value == 0)
             final = false;
           return then::go_on;
         });
  return final;
}

int consume_otrigger(GameWorld *g, obj_data *obj, char_data *actor, int cmd)
{
  return run_unless_purged(fire(g, obj, OTRIG_CONSUME, ready_only(g),
                                [&](trig_data *t) {
                                  add_char_uid_var(t, "actor", actor, 0);
                                  switch (cmd) {
                                    case OCMD_EAT: add_var(t->var_list, "command", "eat", 0); break;
                                    case OCMD_DRINK: add_var(t->var_list, "command", "drink", 0); break;
                                    case OCMD_QUAFF: add_var(t->var_list, "command", "quaff", 0); break;
                                  }
                                },
                                stop_at_first));
}

void time_otrigger(GameWorld *g, obj_data *obj)
{
  fire(g, obj, OTRIG_TIME, at_its_hour(g), set_time(g), stop_at_first);
}

/*
 *  world triggers
 */

void reset_wtrigger(GameWorld *g, struct room_data *room)
{
  fire(g, room, WTRIG_RESET, ready_and_chance(g), no_vars, stop_at_first);
}

void random_wtrigger(GameWorld *g, struct room_data *room)
{
  fire(g, room, WTRIG_RANDOM, ready_and_chance(g), no_vars, stop_at_first);
}

int enter_wtrigger(GameWorld *g, struct room_data *room, char_data *actor, int dir)
{
  auto r = fire(g, room, WTRIG_ENTER, ready_and_chance(g),
                [&](trig_data *t) {
                  set_direction(t, dir, true);
                  add_char_uid_var(t, "actor", actor, 0);
                },
                stop_at_first);
  return r ? r->value : 1;
}

/* The room the actor is in, if it has triggers of type and there is an actor */
static room_data *actor_room_with(GameWorld *g, char_data *actor, bitvector_t type)
{
  if (!actor || !script_has_type(room_get_script(g, char_get_in_room(actor)), type))
    return nullptr;
  return room_of(g, char_get_in_room(actor));
}

int command_wtrigger(GameWorld *g, char_data *actor, std::string_view cmd, std::string_view argument)
{
  room_data *room = actor_room_with(g, actor, WTRIG_COMMAND);
  /* prevent people we like from becoming trapped :P */
  if (!room || !valid_dg_target(g, actor, 0))
    return 0;
  auto r = fire(g, room, WTRIG_COMMAND, [&](trig_data *t) { return ready(g, t) && has_text(g, t, "W-Command") && command_matches(t, cmd); },
                [&](trig_data *t) { set_command_vars(t, actor, cmd, argument); }, stop_at_first);
  return r ? r->value : 0;
}

void speech_wtrigger(GameWorld *g, char_data *actor, std::string_view str)
{
  room_data *room = actor_room_with(g, actor, WTRIG_SPEECH);
  if (!room)
    return;
  fire(g, room, WTRIG_SPEECH,
       [&](trig_data *t) { return ready(g, t) && has_text(g, t, "W-Speech") && (t->arglist[0] == '*' || speech_matches(t, str)); },
       [&](trig_data *t) {
         add_char_uid_var(t, "actor", actor, 0);
         add_var(t->var_list, "speech", str, 0);
       },
       stop_at_first);
}

int drop_wtrigger(GameWorld *g, obj_data *obj, char_data *actor)
{
  room_data *room = actor_room_with(g, actor, WTRIG_DROP);
  if (!room)
    return 1;
  auto r = fire(g, room, WTRIG_DROP, ready_and_chance(g),
                [&](trig_data *t) {
                  add_char_uid_var(t, "actor", actor, 0);
                  add_obj_uid_var(t, "object", obj, 0);
                },
                stop_at_first);
  if (!r)
    return 1;
  return obj_get_carried_by(obj) != actor ? 0 : r->value;
}

int cast_wtrigger(GameWorld *g, char_data *actor, char_data *vict, obj_data *obj, int spellnum)
{
  room_data *room = actor_room_with(g, actor, WTRIG_CAST);
  if (!room)
    return 1;
  auto r = fire(g, room, WTRIG_CAST, ready_and_chance(g),
                [&](trig_data *t) {
                  add_char_uid_var(t, "actor", actor, 0);
                  if (vict)
                    add_char_uid_var(t, "victim", vict, 0);
                  if (obj)
                    add_obj_uid_var(t, "object", obj, 0);
                  set_spell(g, t, spellnum);
                },
                stop_at_first);
  return r ? r->value : 1;
}

int leave_wtrigger(GameWorld *g, struct room_data *room, char_data *actor, int dir)
{
  if (!valid_dg_target(g, actor, DG_ALLOW_GODS))
    return 1;
  auto r = fire(g, room, WTRIG_LEAVE, ready_and_chance(g),
                [&](trig_data *t) {
                  set_direction(t, dir, false);
                  add_char_uid_var(t, "actor", actor, 0);
                },
                stop_at_first);
  return r ? r->value : 1;
}

int door_wtrigger(GameWorld *g, char_data *actor, int subcmd, int dir)
{
  room_data *room = actor_room_with(g, actor, WTRIG_DOOR);
  if (!room)
    return 1;
  auto r = fire(g, room, WTRIG_DOOR, ready_and_chance(g),
                [&](trig_data *t) {
                  add_var(t->var_list, "cmd", cmd_door[subcmd], 0);
                  set_direction(t, dir, false);
                  add_char_uid_var(t, "actor", actor, 0);
                },
                stop_at_first);
  return r ? r->value : 1;
}

void time_wtrigger(GameWorld *g, struct room_data *room)
{
  fire(g, room, WTRIG_TIME, at_its_hour(g), set_time(g), stop_at_first);
}
