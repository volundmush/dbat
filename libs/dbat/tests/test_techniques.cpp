/*
 * Runs each attack with chosen skill levels, perfections, defences and targets, so that their later branches
 * (block, dodge, miss, zanzoken, a hit on each part, an object as the target) are reached, for tools/cmdcompare.py
 * to compare two builds by. Each case forks from one booted game and has its own seed, with the clock stopped.
 *
 * test_techniques [--transcript FILE] [--full] [--shard K/N] [command]
 *   with no transcript, only checks that no case fails the world audit or a sanitizer;
 *   --full writes the players' saved files out whole, where otherwise each is a hash;
 *   --shard runs every Nth attack from the Kth (the scenes count after the attacks).
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "dbat/act.informative.hpp"
#include "dbat/affect.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/consts/fightprefs.hpp"
#include "dbat/consts/materials.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/prefflags.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/consts/sectortypes.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/relocate.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/senseis.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/db.hpp"
#include "dbat/fight.hpp"
#include "dbat/handler.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.attack.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/random.hpp"
#include "dbat/utils.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "harness.hpp"
#include "scene_kit.hpp"

constexpr time_t TRANSCRIPT_TIME = 1800000000;
constexpr int CASE_SECONDS       = 10;

static const struct {
  const char *command;
  int skill;  /* 0 for none */
  void (*beast)(GameWorld *, struct char_data *, std::string_view, int, int); /* only mobs make it, when they fight, by this: a critter makes it in the attacker's place */
  bool area;  /* it catches everyone, and takes no target: just the charge */
} attacks[] = {
    {"barrage",       SKILL_PBARRAGE     },
    {"scatter",       SKILL_SCATTER      },
    {"kakusanha",     SKILL_KAKUSANHA, nullptr, true},
    {"renzokou",      SKILL_RENZO        },
    {"tsuihidan",     SKILL_TSUIHIDAN    },
    {"galikgun",      SKILL_GALIKGUN     },
    {"finalflash",    SKILL_FINALFLASH   },
    {"tribeam",       SKILL_TRIBEAM      },
    {"dualbeam",      SKILL_DUALBEAM     },
    {"dodonpa",       SKILL_DODONPA      },
    {"masenko",       SKILL_MASENKO      },
    {"kamehameha",    SKILL_KAMEHAMEHA   },
    {"beam",          SKILL_BEAM         },
    {"genocide",      SKILL_GENOCIDE     },
    {"genkidama",     SKILL_GENKIDAMA    },
    {"spiritball",    SKILL_SPIRITBALL   },
    {"deathball",     SKILL_DEATHBALL    },
    {"hellspear",     SKILL_HELLSPEAR, nullptr, true},
    {"crusher",       SKILL_CRUSHER      },
    {"blessedhammer", SKILL_BLESSEDHAMMER},
    {"kiball",        SKILL_KIBALL       },
    {"eraser",        SKILL_ERASER       },
    {"bigbang",       SKILL_BIGBANG      },
    {"balefire",      SKILL_BALEFIRE     },
    {"hellflash",     SKILL_HELLFLASH    },
    {"bakuhatsuha",   SKILL_BAKUHATSUHA, nullptr, true},
    {"shogekiha",     SKILL_SHOGEKIHA    },
    {"kiblast",       SKILL_KIBLAST      },
    {"uppercut",      SKILL_UPPERCUT     },
    {"roundhouse",    SKILL_ROUNDHOUSE   },
    {"elbow",         SKILL_ELBOW        },
    {"kick",          SKILL_KICK         },
    {"knee",          SKILL_KNEE         },
    {"punch",         SKILL_PUNCH        },
    {"rogafufuken",   SKILL_ROGAFUFUKEN  },
    {"heeldrop",      SKILL_HEELDROP     },
    {"slam",          SKILL_SLAM         },
    {"tailwhip",      SKILL_TAILWHIP     },
    {"twinslash",     SKILL_TSLASH       },
    {"phoenix",       SKILL_PSLASH       },
    {"darkness",      SKILL_DDSLASH      },
    {"kienzan",       SKILL_KIENZAN      },
    {"honoo",         SKILL_HONOO        },
    {"psychic",       SKILL_PSYBLAST     },
    {"sbc",           SKILL_SBC          },
    {"kousengan",     SKILL_KOUSENGAN    },
    {"deathbeam",     SKILL_DEATHBEAM    },
    {"light",         SKILL_LIGHTGRENADE },
    {"sunder",        SKILL_SUNDER       },
    {"zen",           SKILL_ZEN          },
    {"malice",        SKILL_MALICE       },
    {"nova",          SKILL_STARNOVA, nullptr, true},
    {"headbutt",      SKILL_HEADBUTT     },
    {"bash",          SKILL_BASH         },
    {"seishou",       SKILL_SEISHOU      },
    {"waterrazor",    SKILL_WRAZOR       },
    {"waterspikes",   SKILL_WSPIKE       },
    {"koteiru",       SKILL_KOTEIRU      },
    {"hspiral",       SKILL_HSPIRAL      },
    {"spiral",        SKILL_SPIRAL       },
    {"starbreaker",   SKILL_BREAKER      },
    {"bite",          0                  },
    {"attack",        0                  },
    {"breath",        0,                  do_breath},
    {"ram",           0,                  do_ram},
    {"strike",        0,                  do_strike},
};

/* who attacks: race and sensei decide what some attacks can do */
static const struct {
  const char *name;
  int race, sensei;
  bool armed; /* wields the fixture's sword, for the slashes */
} attackers[] = {
    {"Hugo",  RACE_HUMAN,  CLASS_ROSHI,   false},
    {"Sayla", RACE_SAIYAN, CLASS_BARDOCK, false},
    {"Icor",  RACE_ICER,   CLASS_FRIEZA,  false},
    {"Blade", RACE_HUMAN,  CLASS_TAPION,  true },
};

/* the target, typed after the command; empty means none, while fighting the dummy */
static const char *const targets[] = {"namek", "dummy", "chest", ""}; /* players know each other by race */

/* the share of the charge to use, after the target: all of it kills, a little often doesn't */
static const char *const charges[] = {"", "5"};

static const int skill_levels[] = {35, 75, 100};
static const int defences[]     = {0, 100}; /* the victim's block, dodge and parry */
constexpr int PERFECTIONS       = 4;
constexpr int SEEDS             = 2;

/* Who else is about, for the situations below */

/* Everyone leaves the arena but a, b and c */

/*
 * Situations the grid never sets up, each tried with every attack, attacker and perfection: the checks before an
 * attack, and the ways out of one (a defender, zanzoken, an android's absorbing, a clean miss).
 */
static const struct situation {
  const char *name;
  const char *target; /* "" for none */
  bool fighting;      /* fighting the dummy when it starts */
  void (*setup)(GameWorld *g, struct char_data *att, struct char_data *victim, int skill);
} situations[] = {
    {"unskilled",         "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int skill) { if (skill) char_set_skills_base(att, skill, 0); }                                           },
    {"no target",         "",        false, [](GameWorld *,  char_data *,    char_data *,       int) {}                                                  },
    {"low charge",        "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_charge(att, 1); }                        },
    {"no ki",             "namek",   false, [](GameWorld *g, char_data *att, char_data *,       int) { decCurKI(g, att, getCurKI(g, att), 0); }          },
    {"heavy gravity",     "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { room_set_gravity(g, ARENA, 10); }                 },
    {"no arms",           "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_limb_condition(att, 0, 0); char_set_limb_condition(att, 1, 0); }                                                 },
    {"defended",          "namek",   false, [](GameWorld *,  char_data *,    char_data *victim, int) { char_set_defender(victim, cast.helper);
       char_set_speedboost(cast.helper, 100000); /* fast enough to get there */ char_set_defending(cast.helper, victim); }         },
    {"zanzoken",          "namek",   false, [](GameWorld *,  char_data *,    char_data *victim, int) { char_aff_set(victim, AFF_ZANZOKEN, true); }       },
    {"both zanzoken",     "namek",   false, [](GameWorld *,  char_data *att, char_data *victim, int) { char_aff_set(victim, AFF_ZANZOKEN, true); char_aff_set(att, AFF_ZANZOKEN, true); }                                                 },
    {"android",           "android", false, [](GameWorld *g, char_data *,    char_data *,       int) { char_from_room(g, cast.android); char_to_room(g, cast.android, ARENA); char_set_skills_base(cast.android, SKILL_ABSORB, 100); }                                                 },
    {"clumsy",            "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int skill) { if (skill) char_set_skills_base(att, skill, 1); }                                           },
    {"broken",            "chest",   false, [](GameWorld *,  char_data *,    char_data *,       int) { obj_extra_set(cast.chest, ITEM_BROKEN, true); }   },
    {"peaceful",          "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { room_set_flag(g, ARENA, ROOM_PEACEFUL, true); }   },
    {"asleep",            "namek",   false, [](GameWorld *,  char_data *,    char_data *victim, int) { char_aff_set(victim, AFF_ZANZOKEN, true); char_set_position(victim, POS_SLEEPING); }                                                 },
    {"spent",             "namek",   false, [](GameWorld *g, char_data *,    char_data *victim, int) { decCurST(g, victim, getCurST(g, victim), 0); }    },
    {"hands full",        "namek",   false, [](GameWorld *g, char_data *att, char_data *,       int) {
       if (!char_get_equipment(att, WEAR_WIELD1))
         equip_char(g, att, read_object(g, 301), WEAR_WIELD1);
       equip_char(g, att, read_object(g, 301), WEAR_WIELD2);
     }                                                          },
    {"regenerating",      "majin",   false, [](GameWorld *,  char_data *,    char_data *victim, int) {
       char_set_race(victim, RACE_MAJIN);
       char_set_skills_base(victim, SKILL_REGENERATE, 100);
     }                                                    },
    {"fighting",          "dummy",   true,  [](GameWorld *,  char_data *,    char_data *,       int) {}                                                  },
    {"soft",              "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_bonuses(att, BONUS_SOFT, 1); }           },
    {"middling",          "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int skill) { if (skill) char_set_skills_base(att, skill, 60); }                                           },
    {"piccolo's student", "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_class(att, CLASS_PICCOLO); char_set_skills_base(att, SKILL_STYLE, 75); }                                                 },
    {"nail's student",    "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_class(att, CLASS_NAIL); }                },
    {"fireproof",         "namek",   false, [](GameWorld *g, char_data *,    char_data *victim, int) { char_set_bonuses(victim, BONUS_FIREPROOF, 1); room_set_geffect(g, ARENA, -2); }                                                 },
    {"fire-prone",        "namek",   false, [](GameWorld *g, char_data *,    char_data *victim, int) {
       char_set_bonuses(victim, BONUS_FIREPRONE, 1);
       room_set_geffect(g, ARENA, -1); /* the last of a flood */
       char_set_charge(victim, getMaxKI(g, victim));
       char_aff_set(victim, AFF_SANCTUARY, true);
     }                                                    },
    {"item 79",           "item79",  false, [](GameWorld *g, char_data *,    char_data *,       int) { obj_to_room(g, read_object(g, 79), ARENA); }      },
    {"no legs",           "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_limb_condition(att, 2, 0); char_set_limb_condition(att, 3, 0); }                                                 },
    {"broken leg",        "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_limb_condition(att, 2, 3); char_set_limb_condition(att, 3, -1); }                                                 },
    {"broken left leg",   "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_limb_condition(att, 2, -1); char_set_limb_condition(att, 3, 40); }                                                 },
    {"kabito's student",  "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_class(att, CLASS_KABITO); char_set_skills_base(att, SKILL_STYLE, 75); }                                                 },
    {"exhausted",         "namek",   false, [](GameWorld *g, char_data *att, char_data *,       int) { decCurST(g, att, getCurST(g, att), 0); }          },
    {"no venom",          "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_genome(att, 1, 0); }                     },
    {"outdoors",          "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { room_set_sector_type(g, ARENA, SECT_FIELD); }     },
    {"underwater",        "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { room_set_sector_type(g, ARENA, SECT_UNDERWATER); }},
    {"in the water",      "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { room_set_sector_type(g, ARENA, SECT_WATER_SWIM); }},
    {"in space",          "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { room_set_flag(g, ARENA, ROOM_SPACE, true); }      },
    {"kurzak's student",  "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_class(att, CLASS_KURZAK); char_set_skills_base(att, SKILL_STYLE, 75); }                                                 },
    {"evening",           "namek",   false, [](GameWorld *g, char_data *,    char_data *,       int) { g->time_info.hours = 18; }                        },
    {"molted",            "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_moltlevel(att, 150); }                   },
    {"aqua barrier",      "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_aff_set(att, AFF_SANCTUARY, true); char_set_skills_base(att, SKILL_AQUA_BARRIER, 100); }                                                 },
    {"veteran",           "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_class_level(att, 100); }                 },
    {"novice",            "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_set_class_level(att, 1); }                   },
    {"grouped",           "namek",   false, [](GameWorld *g, char_data *att, char_data *victim, int) { add_follower(g, victim, att); char_aff_set(victim, AFF_GROUP, true); }                                                 },
    {"fellow follower",   "namek",   false, [](GameWorld *g, char_data *att, char_data *victim, int) { add_follower(g, att, cast.helper); add_follower(g, victim, cast.helper); char_aff_set(victim, AFF_GROUP, true); }                                                 },
    {"grouped alone",     "namek",   false, [](GameWorld *,  char_data *,    char_data *victim, int) { char_aff_set(victim, AFF_GROUP, true); }           },
    {"just us",           "namek",   false, [](GameWorld *g, char_data *att, char_data *victim, int) { clear_arena(g, att, victim, cast.helper); }          },
    {"one on one",        "namek",   false, [](GameWorld *g, char_data *att, char_data *victim, int) { clear_arena(g, att, victim); }                       },
    {"one on one outdoors", "namek", false, [](GameWorld *g, char_data *att, char_data *victim, int) { clear_arena(g, att, victim); room_set_sector_type(g, ARENA, SECT_FIELD); }      },
    {"one on one underwater", "namek", false, [](GameWorld *g, char_data *att, char_data *victim, int) { clear_arena(g, att, victim); room_set_sector_type(g, ARENA, SECT_UNDERWATER); } },
    {"one on one in the water", "namek", false, [](GameWorld *g, char_data *att, char_data *victim, int) { clear_arena(g, att, victim); room_set_sector_type(g, ARENA, SECT_WATER_SWIM); } },
    {"alone",             "",        false, [](GameWorld *g, char_data *att, char_data *,       int) { clear_arena(g, att, att); }                          },
    {"newbie victim",     "namek",   false, [](GameWorld *,  char_data *,    char_data *victim, int) { char_set_class_level(victim, 5); }                 },
    {"dead victim",       "namek",   false, [](GameWorld *,  char_data *,    char_data *victim, int) { char_aff_set(victim, AFF_SPIRIT, true); }          },
    {"hiding",            "namek",   false, [](GameWorld *,  char_data *att, char_data *,       int) { char_aff_set(att, AFF_HIDE, true); }              },
};
constexpr int SITUATION_SEEDS = 3;

/* All the scenes, in order */
static const std::vector<scene> scenes = [] {
  std::vector<scene> all;
  for (std::span<const scene> part : {SCENES_OFFENSE, SCENES_DISPATCHERS, SCENES_INFORMATIVE, SCENES_ITEMS_WIZARD, SCENES_OTHER, SCENES_MISC, SCENES_WIZARD})
    all.insert(all.end(), part.begin(), part.end());
  return all;
}();
constexpr int SCENE_SEEDS = 4;

static FILE *transcript = nullptr;
static bool full_saves  = false;
static int failures     = 0;
static int cases        = 0;

static unsigned long seed_for(const std::string &what)
{
  unsigned long h = 14695981039346656037UL;

  for (char c : what)
    h = (h ^ (unsigned char)c) * 1099511628211UL;
  return h % 2147483646UL + 1; /* the generator needs 1 .. 2^31-2 */
}

static void audit_problem(void *, const char *problem)
{
  file_print(stdout, "AUDIT: {}\n", problem);
}

/* sit is a situation, or null for the grid's plain setup */
static void play(GameWorld *g, struct char_data *att, struct char_data *victim, int a, int level, int perf, int defence,
                 const char *target, const char *charge, const struct situation *sit, unsigned long seed)
{
  std::string name        = "dummy";
  struct char_data *dummy = get_char_room_vis(g, att, name, nullptr);

  struct char_data *beast = nullptr;
  if (attacks[a].beast) { /* with no descriptor, it sees nothing and saves nothing: the rooms' record has it */
    beast = att = read_mobile(g, 310);
    char_to_room(g, att, ARENA);
  }
  struct char_data *chars[] = {beast ? victim : att, victim};
  size_t n_chars            = beast ? 1 : 2;

  circle_srandom(g, seed);
  if (attacks[a].skill) {
    char_set_skills_base(att, attacks[a].skill, (int8_t)level);
    char_set_skills_perf(att, attacks[a].skill, (int8_t)perf);
  }
  char_set_skills_base(victim, SKILL_BLOCK, (int8_t)defence);
  char_set_skills_base(victim, SKILL_DODGE, (int8_t)defence);
  char_set_skills_base(victim, SKILL_PARRY, (int8_t)defence);
  restoreVitals(att, false);
  restoreVitals(victim, false);
  char_set_charge(att, getMaxKI(g, att));
  if (!strcmp(attacks[a].command, "bite")) { /* only mutants with the venom genome bite */
    char_set_race(att, RACE_MUTANT);
    char_set_genome(att, 1, 7);
  }
  if (sit)
    sit->setup(g, att, victim, attacks[a].skill);
  if ((sit ? sit->fighting : !*target) && dummy) {
    set_fighting(g, att, dummy);
    set_fighting(g, dummy, att);
  }
  if (attacks[a].area)
    target = "";
  if (beast && !strcmp(target, "namek")) /* a mob knows players by name */
    target = "victim";
  std::string line = str_format("{} {} {}", attacks[a].command, target, charge);
  if (beast)
    attacks[a].beast(g, att, str_format("{} {}", target, charge), 0, 0);
  else
    th_type(g, att, line.c_str());
  th_pulses(g, 1);
  if (world_audit(g, audit_problem, nullptr))
    th_exit(3);
  if (transcript)
    th_write_case_record(g, chars, n_chars, full_saves);
  fflush(stdout);
}

/* Runs play() in a child, with its output to a file, and records what it said under what */
template <class Play> static void fork_case(const std::string &what, Play play)
{
  char report[64];
  int status;
  pid_t pid;

  snprintf(report, sizeof(report), "case-%ld.txt", (long)getpid());
  cases++;
  fflush(nullptr);
  if ((pid = fork()) < 0) {
    perror("fork");
    exit(2);
  }
  if (pid == 0) {
    int fd = open(report, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    alarm(CASE_SECONDS);
    play(seed_for(what));
    th_exit(0);
  }
  waitpid(pid, &status, 0);
  int outcome = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
  if (transcript) {
    file_print(transcript, "######## {} | {:d}\n{}", what, outcome, th_file_text(report));
    fflush(transcript);
  }
  if (outcome) {
    failures++;
    file_print(stdout, "FAIL ({:d}): {}\n{}", outcome, what, th_file_text(report));
  }
}

static void run_case(GameWorld *g, struct char_data *att, struct char_data *victim, int a, int level, int perf,
                     int defence, const char *target, const char *charge, const struct situation *sit, int seed)
{
  std::string what = str_format("{} {} {} by {} at skill {:d} perfection {:d} against defence {:d}{}{}, seed {:d}",
                                attacks[a].command, target, charge, char_get_name(att), level, perf, defence,
                                sit ? ", " : "", sit ? sit->name : "", seed);

  fork_case(what, [&](unsigned long s) { play(g, att, victim, a, level, perf, defence, target, charge, sit, s); });
}

static void play_scene(GameWorld *g, struct char_data *att, struct char_data *victim, const struct scene &sc,
                       unsigned long seed)
{
  std::string name          = "dummy";
  struct char_data *dummy   = get_char_room_vis(g, att, name, nullptr);
  struct char_data *chars[] = {att, victim, cast.helper};

  if (sc.by_mob) { /* with no descriptor, it sees nothing and saves nothing: the rooms' record has it */
    att = read_mobile(g, 310);
    char_to_room(g, att, ARENA);
    chars[0] = victim; /* and the player it stands in for stays out of it */
    chars[1] = cast.helper;
  }
  circle_srandom(g, seed);
  char_set_skills_base(victim, SKILL_BLOCK, 0);
  char_set_skills_base(victim, SKILL_DODGE, 0);
  char_set_skills_base(victim, SKILL_PARRY, 0);
  restoreVitals(att, false);
  restoreVitals(victim, false);
  char_set_charge(att, getMaxKI(g, att));
  sc.setup(g, att, victim);
  if (sc.fighting && dummy) {
    set_fighting(g, att, dummy);
    set_fighting(g, dummy, att);
  }
  if (sc.by_mob)
    do_attack(g, att, sc.line, 0, 0);
  else
    th_type(g, att, sc.line);
  th_pulses(g, 1);
  if (world_audit(g, audit_problem, nullptr))
    th_exit(3);
  if (transcript)
    th_write_case_record(g, chars, sc.by_mob ? 2 : 3, full_saves);
  fflush(stdout);
}

static void run_scene(GameWorld *g, struct char_data *att, struct char_data *victim, const struct scene &sc, int seed)
{
  std::string what = str_format("{} by {}, {}, seed {:d}", sc.line, char_get_name(att), sc.name, seed);

  fork_case(what, [&](unsigned long s) { play_scene(g, att, victim, sc, s); });
}

static void place(GameWorld *g, struct char_data *ch, room_vnum vnum)
{
  char_from_room(g, ch);
  char_to_room(g, ch, vnum);
}

int main(int argc, char **argv)
{
  int shard = 0, shards = 1, arg = 1;

  while (arg < argc && !strncmp(argv[arg], "--", 2)) {
    if (!strcmp(argv[arg], "--transcript")) {
      if (arg + 1 >= argc || !(transcript = fopen(argv[arg + 1], "w"))) {
        perror("--transcript");
        return 2;
      }
      arg += 2;
    } else if (!strcmp(argv[arg], "--full")) {
      full_saves = true;
      arg++;
    } else if (!strcmp(argv[arg], "--shard")) {
      if (arg + 1 >= argc || sscanf(argv[arg + 1], "%d/%d", &shard, &shards) != 2 || shards < 1 || shard < 0 ||
          shard >= shards)
        return 2;
      arg += 2;
    } else
      return 2;
  }
  const char *only = argc > arg ? argv[arg] : nullptr;

  GameWorld *g                 = th_boot_at(TRANSCRIPT_TIME);
  struct char_data *victim     = th_player_as(g, "Victim", ADMLVL_NONE, RACE_NAMEK, CLASS_PICCOLO);
  constexpr size_t n_attackers = sizeof(attackers) / sizeof(*attackers);
  struct char_data *att[n_attackers];

  char_set_class_level(victim, 20); /* past the newbie shield */
  place(g, victim, ARENA);
  for (size_t i = 0; i < n_attackers; i++) {
    att[i] = th_player_as(g, attackers[i].name, ADMLVL_NONE, attackers[i].race, attackers[i].sensei);
    char_set_class_level(att[i], 20);
    place(g, att[i], ARENA);
    if (attackers[i].armed)
      equip_char(g, att[i], read_object(g, 301), WEAR_WIELD1);
    th_take_output(g, att[i]);
  }
  th_take_output(g, victim);
  cast.chest = read_object(g, 300); /* to aim at */
  obj_to_room(g, cast.chest, ARENA);
  cast.android = th_player_as(g, "Andy", ADMLVL_NONE, RACE_ANDROID, CLASS_ANDSIX);
  char_set_class_level(cast.android, 20);
  place(g, cast.android, 300);
  th_take_output(g, cast.android);

  for (int a = 0; a < (int)(sizeof(attacks) / sizeof(*attacks)); a++) {
    if (a % shards != shard || (only && strcmp(only, attacks[a].command)))
      continue;
    for (size_t who = 0; who < n_attackers; who++)
      for (int level : skill_levels)
        for (int perf = 0; perf < PERFECTIONS; perf++)
          for (int defence : defences)
            for (const char *target : targets)
              for (const char *charge : charges)
                for (int seed = 0; seed < SEEDS; seed++)
                  run_case(g, att[who], victim, a, level, perf, defence, target, charge, nullptr, seed);
    for (const struct situation &sit : situations)
      for (size_t who = 0; who < n_attackers; who++)
        for (int perf = 0; perf < PERFECTIONS; perf++)
          for (int seed = 0; seed < SITUATION_SEEDS; seed++) {
            cast.helper = att[(who + 1) % n_attackers];
            run_case(g, att[who], victim, a, 75, perf, 0, sit.target, "", &sit, seed);
          }
  }
  constexpr int n_attacks = sizeof(attacks) / sizeof(*attacks);
  for (int i = 0; i < (int)scenes.size(); i++) {
    std::string_view command = scenes[i].by_mob ? "attack" : std::string_view(scenes[i].line).substr(0, std::string_view(scenes[i].line).find(' '));
    if ((n_attacks + i) % shards != shard || (only && command != only))
      continue;
    for (size_t who = 0; who < n_attackers; who++)
      for (int seed = 0; seed < (scenes[i].seeds ? scenes[i].seeds : SCENE_SEEDS); seed++) {
        cast.helper = att[(who + 1) % n_attackers];
        run_scene(g, att[who], victim, scenes[i], seed);
      }
  }
  if (transcript)
    fclose(transcript);
  file_print(stdout, "{:d} cases, {:d} failed\n", cases, failures);
  fflush(stdout);
  th_shutdown(g);
  return failures != 0;
}
