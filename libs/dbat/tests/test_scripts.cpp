/*
 * Every trigger in a world, each run on its own, to compare two builds of the script engine:
 *
 *   dbat_test_scripts [--transcript FILE] [--shard K/N] [--world DIR | --synthetic] [VNUM-PREFIX]
 *
 * The world is the fixture's unless --world names another (such as the real data), or --synthetic adds the
 * triggers in tests/scripts to it. Its zones load nothing at boot, so only the trigger being tested runs. A trigger
 * is one case, in its own process, with its own seed and the clock stopped:
 *   - it goes on a stand-in owner of its kind with every other trigger taken off: a mob or object from its zone
 *     (or the first in the world), or the first room of its zone, where a player called Actor and another called
 *     Witness stand;
 *   - each of its trigger types is fired through the game's own dispatcher for that type, with Actor doing it;
 *   - after each, the script queue runs on, pulse by pulse as its waits come due, until nothing waits;
 * and the transcript has what each dispatcher returned, what the players saw and saved, the owner's state, the
 * stage room, how many characters and objects there are, and what the game logged.
 *
 * The test reaches the engine only through the dispatchers, the attach and detach commands and the event queue,
 * so that the same source builds against the old engine and the new.
 */
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "dbat/io/reader.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/consts/triggers.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fight.hpp"
#include "dbat/fileop.hpp"
#include "dbat/game_world.hpp"
#include "dbat/iterate.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/relocate.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/utils.hpp"
#include "harness.hpp"

/* A trigger type as the bit a types word holds it at, on either tree (the masks, or the typed flags); and several */
template <auto Type> constexpr bitvector_t bit_of = [] {
  if constexpr (std::is_integral_v<decltype(Type)>)
    return static_cast<bitvector_t>(Type);
  else
    return bitvector_t{1} << std::to_underlying(Type);
}();
template <auto... Types> constexpr bitvector_t bits_of = (bitvector_t{0} | ... | bit_of<Types>);

namespace {
  constexpr time_t TRANSCRIPT_TIME = 1700000000; /* the clock, stopped */
  constexpr int CASE_SECONDS       = 60;
  constexpr int MAX_EVENTS         = 400;                               /* events run after one firing */
  constexpr unsigned long MAX_WAIT = 2UL * SECS_PER_MUD_DAY * PASSES_PER_SEC; /* two game days of pulses */
  constexpr int CAST_SPELL         = 1;

  FILE *transcript;
  int cases, failures;

  /* A trigger as its file has it */
  struct trigger {
    int vnum{};
    int attach{};           /* MOB_TRIGGER, OBJ_TRIGGER or WLD_TRIGGER */
    bitvector_t types{};
    int narg{};
    std::string arglist;
  };

  std::string trimmed(std::string_view s)
  {
    while (!s.empty() && isspace((unsigned char)s.front()))
      s.remove_prefix(1);
    while (!s.empty() && isspace((unsigned char)s.back()))
      s.remove_suffix(1);
    return std::string(s);
  }

  std::vector<std::string> lines_of(const std::string &text)
  {
    std::vector<std::string> lines;
    size_t at = 0;
    while (at < text.size()) {
      size_t eol = text.find('\n', at);
      lines.push_back(text.substr(at, eol == std::string::npos ? std::string::npos : eol - at));
      at = eol == std::string::npos ? text.size() : eol + 1;
    }
    return lines;
  }

  /* The triggers in the booted world's trigger files, in file order. Each is "#vnum", its name up to a ~, a line
     "attach-type flags narg", its argument list up to a ~, and its script up to a ~. */
  std::vector<trigger> read_triggers()
  {
    std::vector<trigger> found;
    std::vector<std::string> names;
    if (DIR *dir = opendir("world/trg")) {
      while (struct dirent *e = readdir(dir))
        if (strstr(e->d_name, ".trg"))
          names.emplace_back(e->d_name);
      closedir(dir);
    }
    std::sort(names.begin(), names.end());
    for (const std::string &name : names) {
      std::vector<std::string> lines = lines_of(th_file_text(("world/trg/" + name).c_str()));
      for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i].size() < 2 || lines[i][0] != '#' || !isdigit((unsigned char)lines[i][1]))
          continue;
        trigger t;
        t.vnum = atoi(lines[i].c_str() + 1);
        size_t j = i + 1;
        while (j < lines.size() && lines[j].find('~') == std::string::npos) /* the name */
          j++;
        if (++j >= lines.size())
          break;
        char flags[128] = "";
        if (sscanf(lines[j].c_str(), "%d %127s %d", &t.attach, flags, &t.narg) < 2)
          continue;
        t.types = io::ascii_flags(flags, 0, "asciiflag_conv");
        std::string args;
        for (j++; j < lines.size(); j++) { /* the argument list */
          size_t tilde = lines[j].find('~');
          args += (args.empty() ? "" : " ") + lines[j].substr(0, tilde);
          if (tilde != std::string::npos)
            break;
        }
        t.arglist = trimmed(args);
        found.push_back(t);
        i = j;
      }
    }
    return found;
  }

  /* So that only the trigger being tested runs: the zones load nothing at boot */
  void strip_zone_commands()
  {
    DIR *dir = opendir("world/zon");
    if (!dir)
      return;
    while (struct dirent *e = readdir(dir)) {
      if (!strstr(e->d_name, ".zon"))
        continue;
      std::string path = std::string("world/zon/") + e->d_name;
      std::string text = th_file_text(path.c_str()), out;
      size_t at = 0;
      int tildes = 0; /* the header: #vnum, name~, builders~, then one line of numbers */
      bool header = true;
      while (at < text.size()) {
        size_t eol       = text.find('\n', at);
        std::string line = text.substr(at, eol == std::string::npos ? std::string::npos : eol - at + 1);
        at               = eol == std::string::npos ? text.size() : eol + 1;
        if (header) {
          out += line;
          if (line.find('~') != std::string::npos && ++tildes == 2) {
            /* the numbers line follows */
            size_t next = text.find('\n', at);
            out += text.substr(at, next == std::string::npos ? std::string::npos : next - at + 1);
            at     = next == std::string::npos ? text.size() : next + 1;
            header = false;
          }
          continue;
        }
        if (line[0] == 'S' || line[0] == '$') { /* the end of the zone's commands, and what follows */
          out += line + text.substr(at);
          break;
        }
      }
      FILE *fl = fopen(path.c_str(), "w");
      if (fl) {
        fputs(out.c_str(), fl);
        fclose(fl);
      }
    }
    closedir(dir);
  }

  /* --synthetic: the triggers in tests/scripts join the fixture's, in the trigger index */
  void add_synthetic_triggers()
  {
    strip_zone_commands();
    DIR *dir = opendir(DBAT_TEST_SCRIPTS);
    if (!dir)
      return;
    std::string index = th_file_text("world/trg/index");
    if (index.size() >= 2 && index.compare(index.size() - 2, 2, "$\n") == 0)
      index.resize(index.size() - 2);
    std::vector<std::string> names;
    while (struct dirent *e = readdir(dir))
      if (strstr(e->d_name, ".trg"))
        names.emplace_back(e->d_name);
    closedir(dir);
    std::sort(names.begin(), names.end());
    for (const std::string &name : names) {
      std::string text = th_file_text((std::string(DBAT_TEST_SCRIPTS) + "/" + name).c_str());
      FILE *fl         = fopen(("world/trg/" + name).c_str(), "w");
      if (fl) {
        fputs(text.c_str(), fl);
        fclose(fl);
      }
      index += name + "\n";
    }
    index += "$\n";
    FILE *fl = fopen("world/trg/index", "w");
    if (fl) {
      fputs(index.c_str(), fl);
      fclose(fl);
    }
  }

  /* ---- The stage ----------------------------------------------------------------------------- */

  room_vnum first_room_from(GameWorld *g, room_vnum lo, room_vnum hi)
  {
    for (room_vnum v = lo; v <= hi; v++)
      if (room_of(g, v))
        return v;
    return NOWHERE;
  }

  mob_vnum first_mob_from(GameWorld *g, mob_vnum lo, mob_vnum hi)
  {
    for (mob_vnum v = lo; v <= hi; v++)
      if (mob_proto_of(g, v))
        return v;
    return NOBODY;
  }

  obj_vnum first_obj_from(GameWorld *g, obj_vnum lo, obj_vnum hi)
  {
    for (obj_vnum v = lo; v <= hi; v++)
      if (obj_proto_of(g, v))
        return v;
    return NOTHING;
  }

  /* --synthetic: its triggers all play in the fixture's own zone, which is the one they're written for */
  int synthetic_zone = -1;

  /* the trigger's zone's first, or the world's */
  template <class F>
  int in_zone_or_world(int vnum, F first)
  {
    int lo = (synthetic_zone >= 0 ? synthetic_zone : vnum / 100) * 100, found = first(lo, lo + 99);
    return found >= 0 ? found : first(0, 999999);
  }

  void remove_player_files(const char *name)
  {
    for (int mode : {PLR_FILE, SCRIPT_VARS_FILE, NEW_OBJ_FILES, ALIAS_FILE})
      if (std::optional<std::string> path = player_path(mode, name))
        unlink(path->c_str());
  }

  /* --synthetic: Actor comes into the game with saved script variables, which it reads */
  bool saved_variables = false;

  struct char_data *player_in(GameWorld *g, const char *name, int admlevel, room_vnum room)
  {
    remove_player_files(name);
    std::optional<std::string> path = player_path(SCRIPT_VARS_FILE, name);
    if (saved_variables && !strcmp(name, "Actor") && path) {
      std::string dir = *path;
      if (system(str_format("mkdir -p '{}'", dir.substr(0, dir.rfind('/'))).c_str()) != 0)
        th_exit(2);
      if (FILE *fl = fopen(path->c_str(), "w")) {
        fputs("savedvar 0 a saved value\nincontext 5 in context five\n\nbare\n", fl);
        fclose(fl);
      }
    }
    struct char_data *ch = th_player(g, name, admlevel);
    char_from_room(g, ch);
    char_to_room(g, ch, room);
    th_take_output(g, ch);
    return ch;
  }

  /* ---- Running the queue --------------------------------------------------------------------- */

  bool queue_empty(GameWorld *g)
  {
    for (int i = 0; i < NUM_EVENT_QUEUES; i++)
      if (g->event_q->head[i])
        return false;
    return true;
  }

  long next_due(GameWorld *g)
  {
    long due = -1;
    for (int i = 0; i < NUM_EVENT_QUEUES; i++)
      if (g->event_q->head[i] && (due < 0 || g->event_q->head[i]->key < due))
        due = g->event_q->head[i]->key;
    return due;
  }

  /* Runs the waiting scripts as their waits come due, until none waits or a limit is reached. */
  void run_queue(GameWorld *g)
  {
    unsigned long start = g->pulse;
    int n;
    for (n = 0; n < MAX_EVENTS && !queue_empty(g); n++) {
      long due = next_due(g);
      if (due > (long)(start + MAX_WAIT))
        break;
      if (due > (long)g->pulse)
        g->pulse = due;
      event_process(g);
    }
    if (!queue_empty(g))
      file_print(stdout, "(still waiting after {:d} events, {:d} pulses)\n", n, (long)(g->pulse - start));
  }

  /* ---- Firing --------------------------------------------------------------------------------- */

  struct stage {
    trigger t;
    room_vnum room{};
    struct char_data *actor{}, *witness{};
    long owner_id{};         /* a mob's or object's id */
    struct obj_data *prop{}; /* an object for receive and the like, carried by Actor */
  };

  struct char_data *owner_mob(GameWorld *g, const stage &s) { return char_by_id(g, s.owner_id); }
  struct obj_data *owner_obj(GameWorld *g, const stage &s) { return obj_by_id(g, s.owner_id); }

  bool owner_here(GameWorld *g, const stage &s)
  {
    switch (s.t.attach) {
      case MOB_TRIGGER:
        return owner_mob(g, s) != nullptr;
      case OBJ_TRIGGER:
        return owner_obj(g, s) != nullptr;
      default:
        return room_of(g, s.room) != nullptr;
    }
  }

  /* what a speech, act or command trigger is listening for: its argument list, or a word when that's empty */
  std::string words_for(const trigger &t) { return t.arglist.empty() || t.arglist == "*" ? std::string("test") : t.arglist; }

  std::string first_word(const std::string &s)
  {
    size_t end = s.find_first_of(" \t");
    return s.substr(0, end);
  }

  /* Fires one trigger type at the owner, returning what the dispatcher did. */
  std::string fire_mob(GameWorld *g, stage &s, bitvector_t type)
  {
    struct char_data *mob = owner_mob(g, s);
    std::string words     = words_for(s.t);
    char text[MAX_INPUT_LENGTH], cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH] = "";
    snprintf(text, sizeof(text), "%s", words.c_str());
    snprintf(cmd, sizeof(cmd), "%s", first_word(words).c_str());

    switch (type) {
      case bit_of<MTRIG_RANDOM>: random_mtrigger(g, mob); return "ran";
      case bit_of<MTRIG_COMMAND>: return str_format("{:d}", (int)command_mtrigger(g, s.actor, cmd, arg));
      case bit_of<MTRIG_SPEECH>: speech_mtrigger(g, s.actor, text); return "ran";
      case bit_of<MTRIG_ACT>: { /* as act() sends it, ending in a line break */
        char line[MAX_INPUT_LENGTH + 2];
        snprintf(line, sizeof(line), "%s\r\n", words.c_str());
        act_mtrigger(g, mob, line, s.actor, s.witness, s.prop, nullptr, "");
        return "ran";
      }
      case bit_of<MTRIG_DEATH>: return str_format("{:d}", death_mtrigger(g, mob, s.actor));
      case bit_of<MTRIG_GREET>:
      case bit_of<MTRIG_GREET_ALL>: return str_format("{:d}", (int)greet_mtrigger(g, s.actor, 0));
      case bit_of<MTRIG_ENTRY>: return str_format("{:d}", entry_mtrigger(g, mob));
      case bit_of<MTRIG_RECEIVE>: return s.prop ? str_format("{:d}", receive_mtrigger(g, mob, s.actor, s.prop)) : "no prop";
      case bit_of<MTRIG_FIGHT>: set_fighting(g, mob, s.actor); fight_mtrigger(g, mob); return "ran";
      case bit_of<MTRIG_HITPRCNT>: set_fighting(g, mob, s.actor); hitprcnt_mtrigger(g, mob); return "ran";
      case bit_of<MTRIG_BRIBE>: bribe_mtrigger(g, mob, s.actor, std::max(s.t.narg, 1)); return "ran";
      case bit_of<MTRIG_LOAD>: load_mtrigger(g, mob); return "ran";
      case bit_of<MTRIG_MEMORY>:
        greet_memory_mtrigger(g, s.actor);
        if (owner_here(g, s))
          entry_memory_mtrigger(g, owner_mob(g, s));
        return "ran";
      case bit_of<MTRIG_CAST>: return str_format("{:d}", cast_mtrigger(g, s.actor, mob, CAST_SPELL));
      case bit_of<MTRIG_LEAVE>: return str_format("{:d}", leave_mtrigger(g, s.actor, 0));
      case bit_of<MTRIG_DOOR>: return str_format("{:d}", door_mtrigger(g, s.actor, 0, 0));
      case bit_of<MTRIG_TIME>: time_mtrigger(g, mob); return "ran";
      default: return "";
    }
  }

  std::string fire_obj(GameWorld *g, stage &s, bitvector_t type)
  {
    struct obj_data *obj = owner_obj(g, s);
    std::string words    = words_for(s.t);
    char cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH] = "";
    snprintf(cmd, sizeof(cmd), "%s", first_word(words).c_str());

    switch (type) {
      case bit_of<OTRIG_RANDOM>: random_otrigger(g, obj); return "ran";
      case bit_of<OTRIG_COMMAND>: return str_format("{:d}", (int)command_otrigger(g, s.actor, cmd, arg));
      case bit_of<OTRIG_TIMER>: timer_otrigger(g, obj); return "ran";
      case bit_of<OTRIG_GET>: return str_format("{:d}", get_otrigger(g, obj, s.actor));
      case bit_of<OTRIG_DROP>: return str_format("{:d}", drop_otrigger(g, obj, s.actor));
      case bit_of<OTRIG_GIVE>: return str_format("{:d}", give_otrigger(g, obj, s.actor, s.witness));
      case bit_of<OTRIG_WEAR>: return str_format("{:d}", wear_otrigger(g, obj, s.actor, 0));
      case bit_of<OTRIG_REMOVE>: return str_format("{:d}", remove_otrigger(g, obj, s.actor));
      case bit_of<OTRIG_LOAD>: return str_format("{:d}", (int)load_otrigger(g, obj));
      case bit_of<OTRIG_CAST>: return str_format("{:d}", cast_otrigger(g, s.actor, obj, CAST_SPELL));
      case bit_of<OTRIG_LEAVE>: return str_format("{:d}", (int)leave_otrigger(g, room_of(g, s.room), s.actor, 0));
      case bit_of<OTRIG_CONSUME>: return str_format("{:d}", consume_otrigger(g, obj, s.actor, OCMD_EAT));
      case bit_of<OTRIG_TIME>: time_otrigger(g, obj); return "ran";
      default: return "";
    }
  }

  std::string fire_room(GameWorld *g, stage &s, bitvector_t type)
  {
    struct room_data *room = room_of(g, s.room);
    std::string words      = words_for(s.t);
    char text[MAX_INPUT_LENGTH], cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH] = "";
    snprintf(text, sizeof(text), "%s", words.c_str());
    snprintf(cmd, sizeof(cmd), "%s", first_word(words).c_str());

    switch (type) {
      case bit_of<WTRIG_RANDOM>: random_wtrigger(g, room); return "ran";
      case bit_of<WTRIG_COMMAND>: return str_format("{:d}", command_wtrigger(g, s.actor, cmd, arg));
      case bit_of<WTRIG_SPEECH>: speech_wtrigger(g, s.actor, text); return "ran";
      case bit_of<WTRIG_RESET>: reset_wtrigger(g, room); return "ran";
      case bit_of<WTRIG_ENTER>: return str_format("{:d}", enter_wtrigger(g, room, s.actor, 0));
      case bit_of<WTRIG_DROP>: return s.prop ? str_format("{:d}", drop_wtrigger(g, s.prop, s.actor)) : "no prop";
      case bit_of<WTRIG_CAST>: return str_format("{:d}", cast_wtrigger(g, s.actor, s.witness, nullptr, CAST_SPELL));
      case bit_of<WTRIG_LEAVE>: return str_format("{:d}", leave_wtrigger(g, room, s.actor, 0));
      case bit_of<WTRIG_DOOR>: return str_format("{:d}", door_wtrigger(g, s.actor, 0, 0));
      case bit_of<WTRIG_TIME>: time_wtrigger(g, room); return "ran";
      default: return "";
    }
  }

  /* ---- A case ---------------------------------------------------------------------------------- */

  /* Puts the owner on the stage with only this trigger, and Actor, Witness and their prop beside it. */
  bool set_stage(GameWorld *g, stage &s)
  {
    const trigger &t = s.t;
    s.room           = in_zone_or_world(t.vnum, [&](int lo, int hi) { return first_room_from(g, lo, hi); });
    if (s.room == NOWHERE)
      return false;
    room_vnum away = first_room_from(g, 0, 999999);
    if (away == s.room)
      away = first_room_from(g, s.room + 1, 999999);

    struct char_data *builder = player_in(g, "Builder", ADMLVL_IMPL, s.room);
    std::string attach;
    if (t.attach == MOB_TRIGGER) {
      mob_vnum v = in_zone_or_world(t.vnum, [&](int lo, int hi) { return first_mob_from(g, lo, hi); });
      if (v == NOBODY)
        return false;
      struct char_data *mob = read_mobile(g, v);
      char_to_room(g, mob, s.room);
      s.owner_id = char_get_id(mob);
      th_type(g, builder, str_format("detach mob {:d} all", v).c_str());
      attach = str_format("attach mob {:d} {:d}", t.vnum, v);
    } else if (t.attach == OBJ_TRIGGER) {
      obj_vnum v = in_zone_or_world(t.vnum, [&](int lo, int hi) { return first_obj_from(g, lo, hi); });
      if (v == NOTHING)
        return false;
      struct obj_data *obj = read_object(g, v);
      obj_to_char(g, obj, builder);
      s.owner_id = obj_get_id(obj);
      th_type(g, builder, str_format("detach obj {:d} all", v).c_str());
      attach = str_format("attach obj {:d} {:d}", t.vnum, v);
    } else {
      th_type(g, builder, "detach room all");
      attach = str_format("attach room {:d} .", t.vnum);
    }
    th_type(g, builder, attach.c_str());
    file_print(stdout, "== setup: {}", th_output(builder));
    th_take_output(g, builder);
    char_from_room(g, builder);
    char_to_room(g, builder, away);

    s.actor   = player_in(g, "Actor", 0, s.room);
    s.witness = player_in(g, "Witness", 0, s.room);
    obj_vnum prop = first_obj_from(g, 0, 999999);
    if (prop != NOTHING) {
      s.prop = read_object(g, prop);
      obj_to_char(g, s.prop, s.actor);
    }
    if (t.attach == OBJ_TRIGGER) { /* where the object would be for its triggers to fire */
      struct obj_data *obj = owner_obj(g, s);
      obj_from_char(g, obj);
      bool carried = (t.types & bits_of<OTRIG_DROP, OTRIG_GIVE, OTRIG_WEAR, OTRIG_REMOVE, OTRIG_CONSUME>) ||
                     ((t.types & bit_of<OTRIG_COMMAND>) && (t.narg & bit_of<OCMD_INVEN>));
      int pos = (t.types & bit_of<OTRIG_COMMAND>) && !(t.narg & bits_of<OCMD_INVEN, OCMD_ROOM>) ? find_eq_pos(g, s.actor, obj) : -1;
      if (pos >= 0)
        equip_char(g, s.actor, obj, pos);
      else if (carried)
        obj_to_char(g, obj, s.actor);
      else
        obj_to_room(g, obj, s.room);
    }
    return true;
  }

  long log_size()
  {
    struct stat st;
    fflush(nullptr);
    return stat("test.log", &st) == 0 ? (long)st.st_size : 0;
  }

  void play(GameWorld *g, const trigger &t)
  {
    circle_srandom(g, 5381UL * 33 + (unsigned long)t.vnum);
    stage s{t};
    file_print(stdout, "== trigger {:d}: attach {:d}, types {:d}, narg {:d}, args '{}'\n", t.vnum, t.attach, (long)t.types, t.narg, t.arglist);
    if (!set_stage(g, s)) {
      file_print(stdout, "(no stage for it)\n");
      return;
    }
    long log_from = log_size();
    for (int bit = 0; bit < 32; bit++) {
      bitvector_t type = (bitvector_t)1 << bit;
      if (!(t.types & type) || !owner_here(g, s))
        continue;
      std::string did = t.attach == MOB_TRIGGER ? fire_mob(g, s, type) : t.attach == OBJ_TRIGGER ? fire_obj(g, s, type) : fire_room(g, s, type);
      if (did.empty())
        continue;
      file_print(stdout, "-- fired type bit {:d}: {}\n", bit, did);
      run_queue(g);
    }
    /* and as the game's own pulse finds random and time triggers */
    if ((t.types & bit_of<MTRIG_RANDOM>) && owner_here(g, s)) {
      file_print(stdout, "-- the random check\n");
      script_trigger_check(g);
      run_queue(g);
    }
    if ((t.types & bit_of<MTRIG_TIME>) && owner_here(g, s)) {
      file_print(stdout, "-- the time check\n");
      check_time_triggers(g);
      run_queue(g);
    }

    struct char_data *people[] = {s.actor, s.witness};
    th_write_case_record(g, people, 2, false);
    save_char_vars(g, s.actor);
    if (std::optional<std::string> path = player_path(SCRIPT_VARS_FILE, "Actor"))
      file_print(stdout, "== Actor's variables:\n{}", th_file_text(path->c_str()));
    file_print(stdout, "== owner: {}\n", owner_here(g, s) ? "here" : "gone");
    std::string line;
    for (char_data *who : room_people(g, s.room))
      line += str_format(" {}", char_get_name(who));
    for (obj_data *obj : room_contents(g, s.room))
      line += str_format(" #{:d}", obj_get_vnum(g, obj));
    file_print(stdout, "== stage room {:d}:{}\n", s.room, line);
    int chars = 0, objs = 0;
    for (char_data *ch : each_char(g)) {
      (void)ch;
      chars++;
    }
    for (obj_data *obj : each_obj(g)) {
      (void)obj;
      objs++;
    }
    file_print(stdout, "== world: {:d} characters, {:d} objects\n", chars, objs);
    std::string log = th_file_text("test.log");
    file_print(stdout, "== log:\n");
    for (const std::string &logged : lines_of(log.size() > (size_t)log_from ? log.substr(log_from) : std::string())) {
      if (logged.find("(+0x") != std::string::npos || (logged.ends_with("]") && logged.find(" [0x") != std::string::npos))
        continue; /* a backtrace's frames: addresses in this one build */
      size_t stamp = logged.find(" :: "); /* the log's own time is the real clock's */
      std::string text = stamp == std::string::npos ? logged : logged.substr(stamp + 4);
      if (size_t where = text.find(" [trig #"); where != std::string::npos)
        text = text.substr(0, where) + " [trig ...]"; /* where a script error was: each engine says it its own way */
      file_print(stdout, "{}\n", text);
    }
  }

  void run(GameWorld *g, const trigger &t)
  {
    char report[64];
    int status;
    snprintf(report, sizeof(report), "case-%ld.txt", (long)getpid());
    cases++;
    fflush(nullptr);
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(2);
    }
    if (pid == 0) {
      int fd = open(report, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      dup2(fd, STDOUT_FILENO);
      alarm(CASE_SECONDS);
      play(g, t);
      th_exit(0);
    }
    waitpid(pid, &status, 0);
    std::string text = th_file_text(report);
    unlink(report);
    int outcome = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
    if (transcript) {
      file_print(transcript, "######## trigger {:d} | {:d}\n{}", t.vnum, outcome, text);
      fflush(transcript);
    }
    if (outcome) {
      failures++;
      file_print(stdout, "FAIL ({:d}): trigger {:d}\n{}", outcome, t.vnum, text);
    }
  }
} // namespace

int main(int argc, char **argv)
{
  int shard = 0, shards = 1, arg = 1;
  bool synthetic = false;

  while (arg < argc && !strncmp(argv[arg], "--", 2)) {
    if (!strcmp(argv[arg], "--transcript") && arg + 1 < argc) {
      if (!(transcript = fopen(argv[arg + 1], "w"))) {
        perror("--transcript");
        return 2;
      }
      arg += 2;
    } else if (!strcmp(argv[arg], "--shard") && arg + 1 < argc && sscanf(argv[arg + 1], "%d/%d", &shard, &shards) == 2 && shards > 0 && shard >= 0 &&
               shard < shards) {
      arg += 2;
    } else if (!strcmp(argv[arg], "--world") && arg + 1 < argc) {
      th_use_world(argv[arg + 1]);
      arg += 2;
    } else if (!strcmp(argv[arg], "--synthetic")) {
      synthetic       = true;
      synthetic_zone  = 3;
      saved_variables = true;
      arg++;
    } else {
      return 2;
    }
  }
  th_before_boot(synthetic ? add_synthetic_triggers : strip_zone_commands);
  const char *only = argc > arg ? argv[arg] : nullptr;

  GameWorld *g = th_boot_at(TRANSCRIPT_TIME);
  /* The stopped clock can come before a world's own saved game time, which leaves the day and month negative; the
     same afternoon everywhere, then, as the fixture has. */
  g->time_info.hours = 13;
  g->time_info.day   = 16;
  g->time_info.month = 11;
  run_queue(g); /* whatever the boot left waiting */
  int i = 0;
  for (const trigger &t : read_triggers()) {
    std::string vnum = std::to_string(t.vnum);
    if (i++ % shards != shard || (only && vnum.compare(0, strlen(only), only)))
      continue;
    run(g, t);
  }
  if (transcript)
    fclose(transcript);
  file_print(stdout, "{:d} cases, {:d} failed\n", cases, failures);
  th_shutdown(g);
  return failures != 0;
}
