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

constexpr time_t TRANSCRIPT_TIME = 1800000000;
constexpr int CASE_SECONDS       = 10;
constexpr room_vnum ARENA        = 301; /* the training yard, with its dummies */

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
static struct {
  struct char_data *helper;  /* another attacker, to defend the victim */
  struct char_data *android; /* kept out of the arena until wanted */
  struct obj_data *chest;
} cast;

/* Everyone leaves the arena but a, b and c */
static void clear_arena(GameWorld *g, struct char_data *a, struct char_data *b, struct char_data *c = nullptr)
{
  for (char_data *who : room_people(g, ARENA))
    if (who != a && who != b && who != c) {
      char_from_room(g, who);
      char_to_room(g, who, 300);
    }
}

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

/* A sword in att's hands for throwing (steel, slashing, three pounds, at full health), made into what is asked */
static struct obj_data *missile(GameWorld *g, struct char_data *att, int damtype = TYPE_SLASH - TYPE_HIT,
                                int material = MATERIAL_STEEL, int flag = -1)
{
  struct obj_data *obj = read_object(g, 301);

  obj_set_value(obj, VAL_WEAPON_DAMTYPE, damtype);
  obj_set_value(obj, VAL_ALL_MATERIAL, material);
  if (flag >= 0)
    obj_extra_set(obj, flag, true);
  obj_to_char(g, obj, att);
  return obj;
}

/* att leads helper's group, or follows them */
static void group(GameWorld *g, struct char_data *leader, struct char_data *follower)
{
  add_follower(g, follower, leader);
  char_aff_set(leader, AFF_GROUP, true);
  char_aff_set(follower, AFF_GROUP, true);
}

/* att wields a sword made into this damage type in pos, in place of whatever was there */
static struct obj_data *wield(GameWorld *g, struct char_data *att, int damtype, int pos = WEAR_WIELD1, int flag = -1)
{
  if (char_get_equipment(att, pos))
    extract_obj(g, unequip_char(g, att, pos));
  struct obj_data *obj = read_object(g, 301);
  obj_set_value(obj, VAL_WEAPON_DAMTYPE, damtype);
  if (flag >= 0)
    obj_extra_set(obj, flag, true);
  equip_char(g, att, obj, pos);
  for (int skill : {SKILL_SWORD, SKILL_DAGGER, SKILL_CLUB, SKILL_SPEAR, SKILL_GUN, SKILL_BRAWL}) /* to hit with it */
    char_set_skills_base(att, skill, 100);
  return obj;
}

/* ... in the off hand alone */
static void off_hand(GameWorld *g, struct char_data *att, int damtype)
{
  if (char_get_equipment(att, WEAR_WIELD1))
    extract_obj(g, unequip_char(g, att, WEAR_WIELD1));
  wield(g, att, damtype, WEAR_WIELD2);
}

/* ... and a sword in the main hand, and a mastery of the two */
static void dual(GameWorld *g, struct char_data *att, int damtype, int mastery = 100)
{
  wield(g, att, TYPE_SLASH - TYPE_HIT);
  wield(g, att, damtype, WEAR_WIELD2);
  char_set_skills_base(att, SKILL_DUALWIELD, mastery);
}

static void two_handed(struct char_data *att, int mastery)
{
  char_plr_set(att, PLR_THANDW, true);
  char_set_skills_base(att, SKILL_TWOHAND, mastery);
}

static void defends(struct char_data *victim)
{
  char_set_skills_base(victim, SKILL_BLOCK, 100);
  char_set_skills_base(victim, SKILL_DODGE, 100);
  char_set_skills_base(victim, SKILL_PARRY, 100);
}

/* att knows every focus at this level */
static void focuses(struct char_data *att, int level)
{
  for (int skill : {SKILL_TSKIN, SKILL_MIGHT, SKILL_WITHER, SKILL_ENLIGHTEN, SKILL_GENIUS, SKILL_FLEX, SKILL_BLESS,
                    SKILL_CURSE, SKILL_YOIK, SKILL_VIGOR, SKILL_CURE, SKILL_POISON})
    char_set_skills_base(att, skill, level);
}

static void drained(GameWorld *g, struct char_data *ch)
{
  decCurKI(g, ch, getMaxKI(g, ch), 0);
}

/* att knows how to create things, at this level, with this much concentration */
static void creates(GameWorld *g, struct char_data *att, int skill, int concentration = 0, int practices = 0)
{
  char_set_skills_base(att, SKILL_CREATE, skill);
  char_set_skills_base(att, SKILL_CONCENTRATION, concentration);
  char_set_practice_points(g, att, practices);
}

/* att trains at level 70 with every stat at 30, carrying 1 in gravity that makes the training about this share of
   their power level */
static void trains(GameWorld *g, struct char_data *att, double share = 0.3)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_weight(obj, 1);
  obj_to_char(g, obj, att);
  char_set_class_level(att, 70);
  for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel,
                   char_set_real_abils_wis, char_set_real_abils_dex})
    set(att, 30);
  room_set_gravity(g, char_get_in_room(att), (int)(getEffMaxPL(g, att) * share));
}

/* ... with their sensei (an assassin's is in the town square), who is paid */
static void with_sensei(GameWorld *g, struct char_data *att, bool paid = true)
{
  char_set_class(att, CLASS_ASSASSIN);
  char_from_room(g, att);
  char_to_room(g, att, 300);
  char_set_gold(att, paid ? 8 : 7);
  char_set_practice_points(g, att, 1);
}

/* att exercises at level 50 in this gravity, with this much more stamina, ki and power */
static void exercises(GameWorld *g, struct char_data *att, int gravity, int64_t vigor = 0)
{
  char_set_class_level(att, 50);
  room_set_gravity(g, char_get_in_room(att), gravity);
  gainBaseST(att, vigor, true);
  gainBaseKI(att, vigor, true);
  gainBasePL(att, vigor, true);
  restoreVitals(att, false);
}

/* ... sitting by item70, of this weight, to meditate on */
static struct obj_data *to_lift(GameWorld *g, struct char_data *att, int64_t weight = 1)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_weight(obj, weight);
  obj_to_room(g, obj, char_get_in_room(att));
  char_set_position(att, POS_SITTING);
  return obj;
}

/* att is under a lasting effect of the skill of this name */
static void lasting(GameWorld *g, struct char_data *att, const char *skill)
{
  for (int i = 0; i < SKILL_TABLE_SIZE; i++)
    if (g->spell_info[i].name && !strcasecmp(skill_name(g, i), skill)) {
      assign_affect(g, att, AFF_INFUSE, i, 5, 0, 0, 0, 0, 0, 0);
      return;
    }
}

/* a scene's setup, with what it doesn't use put aside */
#define SETUP(body) [](GameWorld *g, char_data *att, char_data *victim) { (void)g; (void)att; (void)victim; body }

static void fireshielded(struct char_data *victim)
{
  char_aff_set(victim, AFF_FIRESHIELD, true);
  char_set_skills_base(victim, SKILL_FIRESHIELD, 80);
}

static void throw_ready(struct char_data *att)
{
  char_set_skills_base(att, SKILL_THROW, 100);
}

/*
 * The commands the grid can't set up, each in the ways it needs: with something to throw, in a group, ready to
 * self destruct. Each is tried by every attacker, with a few seeds; cast.helper is another attacker.
 */
static const struct scene {
  const char *name;
  const char *line; /* what the attacker types; "namek" is the victim */
  bool fighting;    /* fighting the dummy when it starts */
  void (*setup)(GameWorld *g, struct char_data *att, struct char_data *victim);
  bool by_mob;      /* a critter makes it in the attacker's place, by do_attack() (mobs only fight so) */
  int seeds;        /* if not SCENE_SEEDS */
} scenes[] = {
    /* attack: each kind of weapon (by damage type), in each hand */
    {"a sword",              "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); }},
    {"a dagger",             "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_PIERCE - TYPE_HIT); }},
    {"a club",               "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_CRUSH - TYPE_HIT); }},
    {"a spear",              "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT); }},
    {"a gun",                "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT); }},
    {"a whip",               "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_WHIP - TYPE_HIT); }},
    {"an off-hand sword",    "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_SLASH - TYPE_HIT); }, false, 12},
    {"an off-hand dagger",   "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_PIERCE - TYPE_HIT); }, false, 12},
    {"an off-hand club",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_CRUSH - TYPE_HIT); }, false, 12},
    {"an off-hand spear",    "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_STAB - TYPE_HIT); }, false, 12},
    {"an off-hand gun",      "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_BLAST - TYPE_HIT); }, false, 12},
    {"an off-hand whip",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_WHIP - TYPE_HIT); }, false, 12},
    {"two swords",           "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); }, false, 12},
    {"and a dagger",         "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_PIERCE - TYPE_HIT); }, false, 12},
    {"and a club",           "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_CRUSH - TYPE_HIT); }, false, 12},
    {"and a spear",          "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_STAB - TYPE_HIT); }, false, 12},
    {"and a gun",            "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_BLAST - TYPE_HIT); char_set_gold(att, 100); }, false, 12},
    {"and a gun, broke",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_BLAST - TYPE_HIT); char_set_gold(att, 0); }, false, 12},
    {"and a whip",           "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_WHIP - TYPE_HIT); }, false, 12},
    {"two swords, fair",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT, 50); }, false, 12},
    {"two swords, good",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT, 75); }, false, 12},
    {"two swords, unlearned","attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT, 0); }, false, 12},
    {"two swords, defended", "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { dual(g, att, TYPE_SLASH - TYPE_HIT); defends(victim); }, false, 12},
    {"two swords, shielded", "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { dual(g, att, TYPE_SLASH - TYPE_HIT); fireshielded(victim); }, false, 12},
    {"two swords, fireproof","attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { dual(g, att, TYPE_SLASH - TYPE_HIT); fireshielded(victim); char_set_bonuses(att, BONUS_FIREPROOF, 1); }, false, 12},
    {"two swords, right arm broken", "attack namek",         false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_limb_condition(att, 0, 2); char_set_limb_condition(att, 1, -1); }, false, 12},
    {"two swords, left arm broken",  "attack namek",         false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_limb_condition(att, 1, 4); char_set_limb_condition(att, 0, -1); }, false, 12},
    {"two daggers, unseen",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_PIERCE - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 4); wield(g, att, TYPE_PIERCE - TYPE_HIT, WEAR_WIELD2); char_set_skills_base(att, SKILL_DUALWIELD, 100); char_set_skills_base(att, SKILL_DAGGER, 100); char_set_skills_base(att, SKILL_MOVE_SILENTLY, 100); char_set_skills_base(att, SKILL_SPOT, 100); }, false, 24},
    {"a sharp dagger",       "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_PIERCE - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1); char_set_skills_base(att, SKILL_DAGGER, 100); }},
    {"a level 2 gun",        "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 1); char_set_gold(att, 100); }},
    {"a level 3 gun",        "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 2); char_set_gold(att, 100); }},
    {"a level 4 gun",        "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 3); char_set_gold(att, 100); }},
    {"a level 5 gun",        "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 4); char_set_gold(att, 100); }},
    {"a level 5 gun, broke", "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 4); char_set_gold(att, 5); }},
    {"a gun, two-handed",    "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_BLAST - TYPE_HIT); two_handed(att, 100); char_set_skills_base(att, SKILL_GUN, 100); }},
    {"a gun, defended",      "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { wield(g, att, TYPE_BLAST - TYPE_HIT); defends(victim); }},
    {"a sword, two-handed",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 0); }},
    {"a sword, two-handed fairly", "attack namek",           false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 50); }},
    {"a sword, two-handed well",   "attack namek",           false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 75); }},
    {"a sword, two-handed masterfully", "attack namek",      false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 100); char_set_skills_base(att, SKILL_SWORD, 100); }, false, 24},
    {"a level 1 spear, two-handed", "attack namek",          false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1); two_handed(att, 50); }},
    {"a level 2 spear, two-handed", "attack namek",          false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 1); two_handed(att, 50); }},
    {"a level 3 spear, two-handed", "attack namek",          false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 2); two_handed(att, 50); }},
    {"a level 4 spear, two-handed", "attack namek",          false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 3); two_handed(att, 50); }},
    {"a level 5 spear, two-handed", "attack namek",          false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 4); two_handed(att, 50); }},
    {"a spear, skilfully",   "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT); char_set_skills_base(att, SKILL_SPEAR, 60); }},
    {"a spear, masterfully", "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_STAB - TYPE_HIT); char_set_skills_base(att, SKILL_SPEAR, 100); }},
    {"a whip, brawling",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_WHIP - TYPE_HIT); char_set_skills_base(att, SKILL_BRAWL, 60); }, false, 48},
    {"a whip, brawling masterfully", "attack namek",         false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_WHIP - TYPE_HIT); char_set_skills_base(att, SKILL_BRAWL, 100); }},
    {"and a whip, brawling masterfully", "attack namek",     false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_WHIP - TYPE_HIT); char_set_skills_base(att, SKILL_BRAWL, 100); }, false, 12},
    {"a sword, as a konatsu","attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); char_set_race(att, RACE_KONATSU); }},
    {"two swords, soft",     "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_bonuses(att, BONUS_SOFT, 1); }, false, 12},
    {"a sword, shielded",    "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { wield(g, att, TYPE_SLASH - TYPE_HIT); fireshielded(victim); }},
    {"a sword, fireproof",   "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { wield(g, att, TYPE_SLASH - TYPE_HIT); fireshielded(victim); char_set_bonuses(att, BONUS_FIREPROOF, 1); }},
    {"an off-hand sword, shielded",   "attack namek",        false, [](GameWorld *g, char_data *att, char_data *victim) { off_hand(g, att, TYPE_SLASH - TYPE_HIT); fireshielded(victim); }, false, 12},
    {"an off-hand sword, fireproof",  "attack namek",        false, [](GameWorld *g, char_data *att, char_data *victim) { off_hand(g, att, TYPE_SLASH - TYPE_HIT); fireshielded(victim); char_set_race(att, RACE_DEMON); }, false, 12},
    {"an off-hand sword, tired",     "attack namek",         false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_SLASH - TYPE_HIT); decCurST(g, att, getCurST(g, att), 0); }},
    {"an off-hand sword, as an android", "attack namek",     false, [](GameWorld *g, char_data *att, char_data *) { off_hand(g, att, TYPE_SLASH - TYPE_HIT); char_set_race(att, RACE_ANDROID); }},
    {"an off-hand sword, at a newbie",   "attack namek",     false, [](GameWorld *g, char_data *att, char_data *victim) { off_hand(g, att, TYPE_SLASH - TYPE_HIT); char_set_class_level(victim, 5); }},
    {"an off-hand gun, at a newbie",     "attack namek",     false, [](GameWorld *g, char_data *att, char_data *victim) { off_hand(g, att, TYPE_BLAST - TYPE_HIT); char_set_class_level(victim, 5); }},
    {"two swords, both zanzoken", "attack namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_aff_set(victim, AFF_ZANZOKEN, true); char_aff_set(att, AFF_ZANZOKEN, true); }, false, 12},
    {"a level 4 sword",      "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT, WEAR_WIELD1, ITEM_WEAPLVL1 + 3); }},
    {"and a level 3 sword",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); obj_extra_set(char_get_equipment(att, WEAR_WIELD2), ITEM_WEAPLVL1 + 2, true); }, false, 12},
    {"two swords, broken arms", "attack namek",              false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_limb_condition(att, 0, -1); char_set_limb_condition(att, 1, -1); }, false, 12},
    {"and a level 1 sword",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); obj_extra_set(char_get_equipment(att, WEAR_WIELD2), ITEM_WEAPLVL1, true); }, false, 12},
    {"and a level 2 sword",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); obj_extra_set(char_get_equipment(att, WEAR_WIELD2), ITEM_WEAPLVL1 + 1, true); }, false, 12},
    {"and a level 4 sword",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); obj_extra_set(char_get_equipment(att, WEAR_WIELD2), ITEM_WEAPLVL1 + 3, true); }, false, 12},
    {"and a level 5 sword",  "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); obj_extra_set(char_get_equipment(att, WEAR_WIELD2), ITEM_WEAPLVL1 + 4, true); }, false, 12},
    {"and a whip, brawling", "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_WHIP - TYPE_HIT); char_set_skills_base(att, SKILL_BRAWL, 60); }, false, 24},
    {"and a spear, skilfully", "attack namek",               false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_STAB - TYPE_HIT); char_set_skills_base(att, SKILL_SPEAR, 60); }, false, 12},
    {"two swords, as a konatsu", "attack namek",             false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_race(att, RACE_KONATSU); }, false, 12},
    {"two swords, dodged",   "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_skills_base(victim, SKILL_DODGE, 100); }, false, 24},
    {"two swords, at the spent", "attack namek",             false, [](GameWorld *g, char_data *att, char_data *victim) { dual(g, att, TYPE_SLASH - TYPE_HIT); decCurST(g, victim, getCurST(g, victim), 0); }, false, 12},
    {"two swords, clumsily", "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_skills_base(att, SKILL_SWORD, 40); }, false, 24},
    {"a sword, at nobody",   "attack",                       false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); }},
    {"a sword, tired",       "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { wield(g, att, TYPE_SLASH - TYPE_HIT); decCurST(g, att, getCurST(g, att), 0); }},
    {"a gun, at a newbie",   "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *victim) { wield(g, att, TYPE_BLAST - TYPE_HIT); char_set_class_level(victim, 5); }},
    {"as an android",        "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_race(att, RACE_ANDROID); }, false, 12},
    {"as a fighter",         "attack namek",                 false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_preference(att, PREFERENCE_H2H); }, false, 12},
    {"a mob, two swords",    "victim", false, [](GameWorld *g, char_data *att, char_data *) { dual(g, att, TYPE_SLASH - TYPE_HIT); char_set_class_level(att, 45); },  true, 24},
    /* throw */
    {"nothing",              "throw",                        false, [](GameWorld *, char_data *, char_data *) {}},
    {"sparring",             "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); char_plr_set(att, PLR_SPAR, true); }},
    {"empty handed",         "throw sword namek",            false, [](GameWorld *, char_data *, char_data *) {}},
    {"at no one",            "throw sword",                  false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); }},
    {"at the foe",           "throw sword",                  true,  [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); }},
    {"at the foe by default","throw sword ghost",            true,  [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); }},
    {"beaten victim",        "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att); char_set_health(victim, 0); }},
    {"newbie victim",        "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att); char_set_class_level(victim, 5); }},
    {"defended",             "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att); throw_ready(att);
       char_set_defender(victim, cast.helper); char_set_speedboost(cast.helper, 100000); char_set_defending(cast.helper, victim); }},
    {"tired",                "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); decCurST(g, att, getCurST(g, att), 0); }},
    {"broken sword",         "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_BROKEN); }},
    {"heavy sword",          "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); room_set_gravity(g, ARENA, 1000000000); }},
    {"sword",                "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); }},
    {"swords",               "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { for (int i = 0; i < 4; i++) missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_THROW); throw_ready(att); }},
    {"swords one at a time", "throw sword namek single",     false, [](GameWorld *g, char_data *att, char_data *) { for (int i = 0; i < 4; i++) missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_THROW); throw_ready(att); }},
    {"swords badly",         "throw sword namek twice",      false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); }},
    {"clumsily",             "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { for (int i = 0; i < 3; i++) missile(g, att); char_set_skills_base(att, SKILL_THROW, 1); }},
    {"at a dodger",          "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { for (int i = 0; i < 3; i++) missile(g, att); char_set_skills_base(victim, SKILL_DODGE, 100); }},
    {"at a zanzoken",        "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att); throw_ready(att); char_aff_set(victim, AFF_ZANZOKEN, true); }},
    {"a level 1 sword",      "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_WEAPLVL1); throw_ready(att); }},
    {"a level 2 sword",      "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_WEAPLVL1 + 1); throw_ready(att); }},
    {"a level 3 sword",      "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_WEAPLVL1 + 2); throw_ready(att); }},
    {"a level 4 sword",      "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_WEAPLVL1 + 3); throw_ready(att); }},
    {"a level 5 sword",      "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_WEAPLVL1 + 4); throw_ready(att); }},
    {"a level 5 dart",       "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_extra_set(missile(g, att, TYPE_PIERCE - TYPE_HIT, MATERIAL_STEEL, ITEM_WEAPLVL1 + 4), ITEM_THROW, true); throw_ready(att); }},
    {"a spear",              "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_PIERCE - TYPE_HIT); throw_ready(att); }},
    {"a hammer",             "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_CRUSH - TYPE_HIT); throw_ready(att); }},
    {"a dagger",             "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_STAB - TYPE_HIT); throw_ready(att); }},
    {"a gun",                "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_BLAST - TYPE_HIT); throw_ready(att); }},
    {"a whip",               "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_WHIP - TYPE_HIT); throw_ready(att); }},
    {"a rock",               "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_STONE), ITEM_OTHER); throw_ready(att); }},
    {"iron",                 "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_IRON), ITEM_OTHER); throw_ready(att); }},
    {"mithril",              "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_MITHRIL), ITEM_OTHER); throw_ready(att); }},
    {"kachin",               "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_KACHIN), ITEM_OTHER); throw_ready(att); }},
    {"diamond",              "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_DIAMOND), ITEM_OTHER); throw_ready(att); }},
    {"energy",               "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_ENERGY), ITEM_OTHER); throw_ready(att); }},
    {"cloth",                "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_COTTON), ITEM_OTHER); throw_ready(att); }},
    {"an unbreakable one",   "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_type_flag(missile(g, att, 0, MATERIAL_COTTON, ITEM_UNBREAKABLE), ITEM_OTHER); throw_ready(att); }},
    {"a worn one",           "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { obj_set_value(missile(g, att), VAL_ALL_HEALTH, 1); throw_ready(att); }},
    {"a hot one",            "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_HOT); throw_ready(att); }},
    {"a hot one, fireproof", "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_HOT); throw_ready(att); char_set_bonuses(victim, BONUS_FIREPROOF, 1); }},
    {"an icy one",           "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_ICE); throw_ready(att); char_set_aff_abils_dex(att, 100); }},
    {"an icy one at an icer","throw sword icer",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_ICE); throw_ready(att); char_set_aff_abils_dex(att, 100); char_set_race(victim, RACE_ICER); }},
    {"an icy one at a demon","throw sword demon",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_ICE); char_set_skills_base(att, SKILL_THROW, 1); char_set_race(victim, RACE_DEMON); }},
    {"at a majin",           "throw sword majin",            false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att); throw_ready(att); char_set_aff_abils_dex(att, 100); char_set_race(victim, RACE_MAJIN); }},
    {"at an android",        "throw sword android",          false, [](GameWorld *g, char_data *att, char_data *victim) { missile(g, att); throw_ready(att); char_set_aff_abils_dex(att, 100); char_set_race(victim, RACE_ANDROID); }},
    {"a dexterous throw",    "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); char_set_aff_abils_dex(att, 100); }},
    {"as a thrower",         "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { for (int i = 0; i < 3; i++) missile(g, att, TYPE_SLASH - TYPE_HIT, MATERIAL_STEEL, ITEM_THROW); throw_ready(att); char_set_preference(att, PREFERENCE_THROWING); }},
    {"as a ki user",         "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); char_set_preference(att, PREFERENCE_KI); }},
    {"energized",            "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); char_set_skills_base(att, SKILL_ENERGIZE, 50); char_prf_set(g, att, PRF_ENERGIZE, true); }},
    {"energized, no ki",     "throw sword namek",            false, [](GameWorld *g, char_data *att, char_data *) { missile(g, att); throw_ready(att); char_prf_set(g, att, PRF_ENERGIZE, true); decCurKI(g, att, getCurKI(g, att), 0); }},
    {"the dummy",            "throw dummy namek",            false, [](GameWorld *, char_data *att, char_data *) { throw_ready(att); char_set_speedboost(att, 100000); }},
    {"the dummy clumsily",   "throw dummy namek",            false, [](GameWorld *, char_data *att, char_data *) { char_set_skills_base(att, SKILL_THROW, 30); char_set_speedboost(att, 100000); }},
    {"the dummy at a dodger","throw dummy namek",            false, [](GameWorld *, char_data *att, char_data *victim) { char_set_skills_base(att, SKILL_THROW, 60); char_set_speedboost(att, 100000); char_set_skills_base(victim, SKILL_DODGE, 100); }},
    {"the dummy, slowly",    "throw dummy namek",            false, [](GameWorld *, char_data *att, char_data *) { throw_ready(att); }},
    {"the dummy at itself",  "throw dummy dummy",            false, [](GameWorld *, char_data *att, char_data *) { throw_ready(att); }},
    {"an unkillable dummy",  "throw dummy namek",            false, [](GameWorld *g, char_data *att, char_data *) { throw_ready(att); std::string dummy = "dummy"; char_mob_set(get_char_room_vis(g, att, dummy, nullptr), MOB_NOKILL, true); }},
    {"the dummy, tired",     "throw dummy namek",            false, [](GameWorld *g, char_data *att, char_data *) { throw_ready(att); char_set_speedboost(att, 100000); decCurST(g, att, getCurST(g, att), 0); }},
    {"the dummy, heavily",   "throw dummy namek",            false, [](GameWorld *g, char_data *att, char_data *) { throw_ready(att); char_set_speedboost(att, 100000); room_set_gravity(g, ARENA, 1000000000); }},
    /* combine */
    {"alone",                "combine kamehameha namek",     false, [](GameWorld *, char_data *, char_data *) {}},
    {"leader, no attack",    "combine",                      false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); }},
    {"leader, no target",    "combine kamehameha",           false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); }},
    {"leader stops",         "combine stop namek",           false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); }},
    {"leader, no such",      "combine kamekame namek",       false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); }},
    {"leader, two words",    "combine twin namek",           false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); }},
    {"leader, unskilled",    "combine kamehameha namek",     false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); char_set_skills_base(att, SKILL_KAMEHAMEHA, 0); }},
    {"leader, alone",        "combine kamehameha namek",     false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); }},
    {"leader, low",          "combine kamehameha namek",     false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); char_set_charge(att, 1); }},
    {"leader, nobody",       "combine kamehameha ghost",     false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); }},
    {"leader, at self",      "combine kamehameha self",      false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); }},
    {"leader, combined",     "combine kamehameha namek",     false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); add_follower(g, cast.android, att); /* not in the group */ char_set_skills_base(att, SKILL_KAMEHAMEHA, 50);
       char_set_skills_base(cast.helper, SKILL_MASENKO, 50); char_set_combine(cast.helper, 2); restoreVitals(cast.helper, false); char_set_charge(cast.helper, getMaxKI(g, cast.helper)); }},
    {"leader, spent helper", "combine kamehameha namek",     false, [](GameWorld *g, char_data *att, char_data *) { group(g, att, cast.helper); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); char_set_combine(cast.helper, 2); char_set_charge(cast.helper, 0); }},
    {"follower stops",       "combine stop",                 false, [](GameWorld *g, char_data *att, char_data *) { group(g, cast.helper, att); group(g, cast.helper, cast.android); char_set_combine(att, 0); }},
    {"follower, unready",    "combine stop",                 false, [](GameWorld *g, char_data *att, char_data *) { group(g, cast.helper, att); char_set_combine(att, -1); }},
    {"follower readies",     "combine kamehameha",           false, [](GameWorld *g, char_data *att, char_data *) { group(g, cast.helper, att); group(g, cast.helper, cast.android); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); }},
    {"follower, low",        "combine kamehameha",           false, [](GameWorld *g, char_data *att, char_data *) { group(g, cast.helper, att); char_set_skills_base(att, SKILL_KAMEHAMEHA, 50); char_set_charge(att, 1); }},
    {"follower, unskilled",  "combine deathbeam",            false, [](GameWorld *g, char_data *att, char_data *) { group(g, cast.helper, att); char_set_skills_base(att, SKILL_DEATHBEAM, 0); }},
    /* self destruct */
    {"dead",                 "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_aff_set(att, AFF_SPIRIT, true); }},
    {"newbie",               "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_set_class_level(att, 5); }},
    {"recovering",           "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_set_con_sdcooldown(att, 10); }},
    {"starting",             "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_set_skills_base(att, SKILL_SELFD, 0); }},
    {"calming",              "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_plr_set(att, PLR_SELFD, true); }},
    {"exploding",            "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); }},
    {"exploding, immortal",  "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); char_plr_set(att, PLR_IMMORTAL, true); char_set_skills_base(att, SKILL_SELFD, 60); }},
    {"exploding near a newbie", "selfdestruct",              false, [](GameWorld *, char_data *att, char_data *victim) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); char_set_class_level(victim, 5); }},
    {"exploding on a newbie","selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *victim) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); char_set_grappling(att, victim); char_set_grappled(victim, att); char_set_grap(att, 3); char_set_class_level(victim, 5); }},
    {"exploding on one",     "selfdestruct",                 false, [](GameWorld *, char_data *att, char_data *victim) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); char_set_grappling(att, victim); char_set_grappled(victim, att); char_set_grap(att, 3); }},
    {"exploding on one, immortal", "selfdestruct",           false, [](GameWorld *, char_data *att, char_data *victim) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); char_set_grappling(att, victim); char_set_grappled(victim, att); char_set_grap(att, 3); char_plr_set(att, PLR_IMMORTAL, true); char_set_skills_base(att, SKILL_SELFD, 60); }},
    {"exploding on one as a majin", "selfdestruct",          false, [](GameWorld *, char_data *att, char_data *victim) { char_plr_set(att, PLR_SELFD, true); char_plr_set(att, PLR_SELFD2, true); char_set_grappling(att, victim); char_set_grappled(victim, att); char_set_grap(att, 3); char_set_race(att, RACE_MAJIN); restoreLF(att, false); }},
    /* energize */
    {"not a thrower",        "energize",                     false, [](GameWorld *, char_data *, char_data *) {}},
    {"unfocused",            "energize",                     false, [](GameWorld *, char_data *att, char_data *) { char_set_preference(att, PREFERENCE_THROWING); }},
    {"focused",              "energize",                     false, [](GameWorld *, char_data *att, char_data *) { char_set_preference(att, PREFERENCE_THROWING); char_set_skills_base(att, SKILL_FOCUS, 30); }},
    {"on",                   "energize",                     false, [](GameWorld *, char_data *att, char_data *) { char_set_preference(att, PREFERENCE_THROWING); char_set_skills_base(att, SKILL_ENERGIZE, 30); }},
    {"off",                  "energize",                     false, [](GameWorld *g, char_data *att, char_data *) { char_set_preference(att, PREFERENCE_THROWING); char_set_skills_base(att, SKILL_ENERGIZE, 30); char_prf_set(g, att, PRF_ENERGIZE, true); }},
    /* focus: each buff on the user and on another, and the rest */
    {"nothing", "focus", false, SETUP(focuses(att, 70);)},
    {"something unknown", "focus nothing", false, SETUP(focuses(att, 70);)},
    {"in a healing tank", "focus tough", false, SETUP(focuses(att, 70); char_plr_set(att, PLR_HEALT, true);)},
    {"cursed", "focus tough", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_CURSE, true);)},
    {"tough, on themselves", "focus tough", false, SETUP(focuses(att, 70);), false, 12},
    {"tough, on another", "focus tough namek", false, SETUP(focuses(att, 70);), false, 12},
    {"tough, unknown", "focus tough", false, SETUP()},
    {"tough, on no one", "focus tough ghost", false, SETUP(focuses(att, 70);)},
    {"tough, on themselves by name", "focus tough self", false, SETUP(focuses(att, 70);)},
    {"tough, on a mob", "focus tough dummy", false, SETUP(focuses(att, 70);)},
    {"tough, had", "focus tough", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_STONESKIN, true);)},
    {"tough, had by another", "focus tough namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_STONESKIN, true);)},
    {"tough, without the ki", "focus tough", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"tough, on another without the ki", "focus tough namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"might, on themselves", "focus might", false, SETUP(focuses(att, 70);), false, 12},
    {"might, on another", "focus might namek", false, SETUP(focuses(att, 70);), false, 12},
    {"might, unknown", "focus might", false, SETUP()},
    {"might, on no one", "focus might ghost", false, SETUP(focuses(att, 70);)},
    {"might, on themselves by name", "focus might self", false, SETUP(focuses(att, 70);)},
    {"might, on a mob", "focus might dummy", false, SETUP(focuses(att, 70);)},
    {"might, had", "focus might", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_MIGHT, true);)},
    {"might, had by another", "focus might namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_MIGHT, true);)},
    {"might, without the ki", "focus might", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"might, on another without the ki", "focus might namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"enlighten, on themselves", "focus enlighten", false, SETUP(focuses(att, 70);), false, 12},
    {"enlighten, on another", "focus enlighten namek", false, SETUP(focuses(att, 70);), false, 12},
    {"enlighten, unknown", "focus enlighten", false, SETUP()},
    {"enlighten, on no one", "focus enlighten ghost", false, SETUP(focuses(att, 70);)},
    {"enlighten, on themselves by name", "focus enlighten self", false, SETUP(focuses(att, 70);)},
    {"enlighten, on a mob", "focus enlighten dummy", false, SETUP(focuses(att, 70);)},
    {"enlighten, had", "focus enlighten", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_ENLIGHTEN, true);)},
    {"enlighten, had by another", "focus enlighten namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_ENLIGHTEN, true);)},
    {"enlighten, without the ki", "focus enlighten", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"enlighten, on another without the ki", "focus enlighten namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"genius, on themselves", "focus genius", false, SETUP(focuses(att, 70);), false, 12},
    {"genius, on another", "focus genius namek", false, SETUP(focuses(att, 70);), false, 12},
    {"genius, unknown", "focus genius", false, SETUP()},
    {"genius, on no one", "focus genius ghost", false, SETUP(focuses(att, 70);)},
    {"genius, on themselves by name", "focus genius self", false, SETUP(focuses(att, 70);)},
    {"genius, on a mob", "focus genius dummy", false, SETUP(focuses(att, 70);)},
    {"genius, had", "focus genius", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_GENIUS, true);)},
    {"genius, had by another", "focus genius namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_GENIUS, true);)},
    {"genius, without the ki", "focus genius", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"genius, on another without the ki", "focus genius namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"flex, on themselves", "focus flex", false, SETUP(focuses(att, 70);), false, 12},
    {"flex, on another", "focus flex namek", false, SETUP(focuses(att, 70);), false, 12},
    {"flex, unknown", "focus flex", false, SETUP()},
    {"flex, on no one", "focus flex ghost", false, SETUP(focuses(att, 70);)},
    {"flex, on themselves by name", "focus flex self", false, SETUP(focuses(att, 70);)},
    {"flex, on a mob", "focus flex dummy", false, SETUP(focuses(att, 70);)},
    {"flex, had", "focus flex", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_FLEX, true);)},
    {"flex, had by another", "focus flex namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_FLEX, true);)},
    {"flex, without the ki", "focus flex", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"flex, on another without the ki", "focus flex namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"bless, on themselves", "focus bless", false, SETUP(focuses(att, 70);), false, 12},
    {"bless, on another", "focus bless namek", false, SETUP(focuses(att, 70);), false, 12},
    {"bless, unknown", "focus bless", false, SETUP()},
    {"bless, on no one", "focus bless ghost", false, SETUP(focuses(att, 70);)},
    {"bless, on themselves by name", "focus bless self", false, SETUP(focuses(att, 70);)},
    {"bless, on a mob", "focus bless dummy", false, SETUP(focuses(att, 70);)},
    {"bless, had", "focus bless", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_BLESS, true);)},
    {"bless, had by another", "focus bless namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_BLESS, true);)},
    {"bless, without the ki", "focus bless", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"bless, on another without the ki", "focus bless namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"curse, on themselves, failing", "focus curse", false, SETUP(focuses(att, 1);)},
    {"curse, on themselves", "focus curse", false, SETUP(focuses(att, 70);), false, 12},
    {"curse, on themselves, blessed", "focus curse", false, SETUP(focuses(att, 100); char_aff_set(att, AFF_BLESS, true);), false, 8},
    {"curse, on another", "focus curse namek", false, SETUP(focuses(att, 70);), false, 12},
    {"curse, unknown", "focus curse", false, SETUP()},
    {"curse, on no one", "focus curse ghost", false, SETUP(focuses(att, 70);)},
    {"curse, on themselves by name", "focus curse self", false, SETUP(focuses(att, 70);)},
    {"curse, on a mob", "focus curse dummy", false, SETUP(focuses(att, 70);)},
    {"curse, had by another", "focus curse namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_CURSE, true);)},
    {"curse, without the ki", "focus curse", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"curse, on another without the ki", "focus curse namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"might, BONUS_WIMP str 55 on themselves", "focus might", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_WIMP, 1); char_set_aff_abils_str(att, 55);)},
    {"might, BONUS_WIMP str 69 on themselves", "focus might", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_WIMP, 1); char_set_aff_abils_str(att, 69);)},
    {"might, BONUS_WIMP str 55 on another", "focus might namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_WIMP, 1); char_set_aff_abils_str(victim, 55);)},
    {"might, BONUS_WIMP str 69 on another", "focus might namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_WIMP, 1); char_set_aff_abils_str(victim, 69);)},
    {"might, BONUS_FRAIL str 55 on themselves", "focus might", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_FRAIL, 1); char_set_aff_abils_str(att, 55);)},
    {"might, BONUS_FRAIL str 69 on themselves", "focus might", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_FRAIL, 1); char_set_aff_abils_str(att, 69);)},
    {"might, BONUS_FRAIL str 55 on another", "focus might namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_FRAIL, 1); char_set_aff_abils_str(victim, 55);)},
    {"might, BONUS_FRAIL str 69 on another", "focus might namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_FRAIL, 1); char_set_aff_abils_str(victim, 69);)},
    {"might, BONUS_FRAIL con 55 on themselves", "focus might", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_FRAIL, 1); char_set_aff_abils_con(att, 55);)},
    {"might, BONUS_FRAIL con 69 on themselves", "focus might", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_FRAIL, 1); char_set_aff_abils_con(att, 69);)},
    {"might, BONUS_FRAIL con 55 on another", "focus might namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_FRAIL, 1); char_set_aff_abils_con(victim, 55);)},
    {"might, BONUS_FRAIL con 69 on another", "focus might namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_FRAIL, 1); char_set_aff_abils_con(victim, 69);)},
    {"enlighten, BONUS_FOOLISH wis 55 on themselves", "focus enlighten", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_FOOLISH, 1); char_set_aff_abils_wis(att, 55);)},
    {"enlighten, BONUS_FOOLISH wis 69 on themselves", "focus enlighten", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_FOOLISH, 1); char_set_aff_abils_wis(att, 69);)},
    {"enlighten, BONUS_FOOLISH wis 55 on another", "focus enlighten namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_FOOLISH, 1); char_set_aff_abils_wis(victim, 55);)},
    {"enlighten, BONUS_FOOLISH wis 69 on another", "focus enlighten namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_FOOLISH, 1); char_set_aff_abils_wis(victim, 69);)},
    {"genius, BONUS_DULL intel 55 on themselves", "focus genius", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_DULL, 1); char_set_aff_abils_intel(att, 55);)},
    {"genius, BONUS_DULL intel 69 on themselves", "focus genius", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_DULL, 1); char_set_aff_abils_intel(att, 69);)},
    {"genius, BONUS_DULL intel 55 on another", "focus genius namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_DULL, 1); char_set_aff_abils_intel(victim, 55);)},
    {"genius, BONUS_DULL intel 69 on another", "focus genius namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_DULL, 1); char_set_aff_abils_intel(victim, 69);)},
    {"flex, BONUS_CLUMSY dex 55 on themselves", "focus flex", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_CLUMSY, 1); char_set_aff_abils_dex(att, 55);)},
    {"flex, BONUS_CLUMSY dex 69 on themselves", "focus flex", false, SETUP(focuses(att, 70); char_set_bonuses(att, BONUS_CLUMSY, 1); char_set_aff_abils_dex(att, 69);)},
    {"flex, BONUS_CLUMSY dex 55 on another", "focus flex namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_CLUMSY, 1); char_set_aff_abils_dex(victim, 55);)},
    {"flex, BONUS_CLUMSY dex 69 on another", "focus flex namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_CLUMSY, 1); char_set_aff_abils_dex(victim, 69);)},
    {"enlighten by a jinto at 100", "focus enlighten", false, SETUP(focuses(att, 100); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 24},
    {"enlighten another by a jinto at 100", "focus enlighten namek", false, SETUP(focuses(att, 100); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 24},
    {"enlighten by a jinto at 60", "focus enlighten", false, SETUP(focuses(att, 60); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 24},
    {"enlighten another by a jinto at 60", "focus enlighten namek", false, SETUP(focuses(att, 60); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 24},
    {"enlighten by a jinto at 45", "focus enlighten", false, SETUP(focuses(att, 45); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 48},
    {"enlighten another by a jinto at 45", "focus enlighten namek", false, SETUP(focuses(att, 45); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 48},
    {"enlighten by a jinto at 30", "focus enlighten", false, SETUP(focuses(att, 30); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 12},
    {"enlighten another by a jinto at 30", "focus enlighten namek", false, SETUP(focuses(att, 30); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 100);), false, 12},
    {"enlighten by an unpractised jinto", "focus enlighten", false, SETUP(focuses(att, 100); char_set_class(att, CLASS_JINTO); char_set_practice_points(g, att, 10);), false, 12},
    {"genius by a grouped kai", "focus genius namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, att, victim);), false, 24},
    {"genius by a kai following", "focus genius namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, victim, att);), false, 12},
    {"genius by a kai, ungrouped", "focus genius namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI);), false, 12},
    {"genius by a fellow follower", "focus genius namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, cast.helper, att); group(g, cast.helper, victim);), false, 12},
    {"flex by a grouped kai", "focus flex namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, att, victim);), false, 24},
    {"flex by a kai following", "focus flex namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, victim, att);), false, 12},
    {"flex by a kai, ungrouped", "focus flex namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI);), false, 12},
    {"flex by a fellow follower", "focus flex namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, cast.helper, att); group(g, cast.helper, victim);), false, 12},
    {"bless by a grouped kai", "focus bless namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, att, victim);), false, 24},
    {"bless by a kai following", "focus bless namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, victim, att);), false, 12},
    {"bless by a kai, ungrouped", "focus bless namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI);), false, 12},
    {"bless by a fellow follower", "focus bless namek", false, SETUP(focuses(att, 70); char_set_race(att, RACE_KAI); group(g, cast.helper, att); group(g, cast.helper, victim);), false, 12},
    {"bless by a kabito", "focus bless", false, SETUP(focuses(att, 70); char_set_class(att, CLASS_KABITO);), false, 12},
    {"bless another by a kabito", "focus bless namek", false, SETUP(focuses(att, 70); char_set_class(att, CLASS_KABITO);), false, 12},
    {"bless a cursed one", "focus bless namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_CURSE, true);), false, 12},
    {"curse a blessed one", "focus curse namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_BLESS, true);), false, 12},
    {"curse, as a demon", "focus curse", false, SETUP(focuses(att, 70); char_set_race(att, RACE_DEMON);)},
    {"curse a demon", "focus curse demon", false, SETUP(focuses(att, 70); char_set_race(victim, RACE_DEMON);)},
    {"wither", "focus wither namek", false, SETUP(focuses(att, 70);), false, 12},
    {"wither, unknown", "focus wither namek", false, SETUP()},
    {"wither no one", "focus wither ghost", false, SETUP(focuses(att, 70);)},
    {"wither a newbie", "focus wither namek", false, SETUP(focuses(att, 70); char_set_class_level(victim, 5);)},
    {"wither themselves", "focus wither self", false, SETUP(focuses(att, 70);)},
    {"wither a mob", "focus wither dummy", false, SETUP(focuses(att, 70);)},
    {"wither the withered", "focus wither namek", false, SETUP(focuses(att, 70); assign_affect(g, victim, AFF_WITHER, SKILL_WITHER, -1, -3, 0, 0, 0, 0, -3);)},
    {"wither without the ki", "focus wither namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"yoik", "focus yoik namek", false, SETUP(focuses(att, 70);), false, 24},
    {"yoikominminken", "focus yoikominminken namek", false, SETUP(focuses(att, 100);), false, 12},
    {"yoik, unknown", "focus yoik namek", false, SETUP()},
    {"yoik no one", "focus yoik ghost", false, SETUP(focuses(att, 70);)},
    {"yoik a newbie", "focus yoik namek", false, SETUP(focuses(att, 70); char_set_class_level(victim, 5);)},
    {"yoik a mob", "focus yoik dummy", false, SETUP(focuses(att, 100);), false, 12},
    {"yoik the sleeping", "focus yoik namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_SLEEP, true);)},
    {"yoik closed eyes", "focus yoik namek", false, SETUP(focuses(att, 70); char_plr_set(victim, PLR_EYEC, true);)},
    {"yoik the blind", "focus yoik namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_BLIND, true);)},
    {"yoik an insomniac", "focus yoik namek", false, SETUP(focuses(att, 70); char_set_bonuses(victim, BONUS_INSOMNIAC, 1);)},
    {"yoik the flying", "focus yoik namek", false, SETUP(focuses(att, 100); char_aff_set(victim, AFF_FLYING, true); char_set_altitude(victim, 1); char_aff_set(att, AFF_FLYING, true); char_set_altitude(att, 1);), false, 12},
    {"yoik without the ki", "focus yoik namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"vigor, tired", "focus vigor", false, SETUP(focuses(att, 70); decCurST(g, att, getMaxST(g, att) / 2, 0);), false, 12},
    {"vigor, rested", "focus vigor", false, SETUP(focuses(att, 70);), false, 12},
    {"vigor, as a healer", "focus vigor", false, SETUP(focuses(att, 100); decCurST(g, att, getMaxST(g, att) / 2, 0); char_set_bonuses(att, BONUS_HEALER, 1);)},
    {"vigor, unknown", "focus vigor", false, SETUP()},
    {"vigor without the ki", "focus vigor", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"vigor another, tired", "focus vigor namek", false, SETUP(focuses(att, 70); decCurST(g, victim, getMaxST(g, victim) / 2, 0);), false, 12},
    {"vigor another, rested", "focus vigor namek", false, SETUP(focuses(att, 70);)},
    {"vigor another, as a healer", "focus vigor namek", false, SETUP(focuses(att, 100); decCurST(g, victim, getMaxST(g, victim) / 2, 0); char_set_bonuses(att, BONUS_HEALER, 1);)},
    {"vigor no one", "focus vigor ghost", false, SETUP(focuses(att, 70);)},
    {"vigor a newbie", "focus vigor namek", false, SETUP(focuses(att, 70); char_set_class_level(victim, 5);)},
    {"vigor a mob", "focus vigor dummy", false, SETUP(focuses(att, 70);)},
    {"vigor another without the ki", "focus vigor namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"cure, poisoned", "focus cure", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_POISON, true);), false, 12},
    {"cure, well", "focus cure", false, SETUP(focuses(att, 70);)},
    {"cure, unknown", "focus cure", false, SETUP()},
    {"cure without the ki", "focus cure", false, SETUP(focuses(att, 70); char_aff_set(att, AFF_POISON, true); drained(g, att);)},
    {"cure another, poisoned", "focus cure namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_POISON, true);), false, 12},
    {"cure another, well", "focus cure namek", false, SETUP(focuses(att, 70);)},
    {"cure themselves by name", "focus cure self", false, SETUP(focuses(att, 70);)},
    {"cure no one", "focus cure ghost", false, SETUP(focuses(att, 70);)},
    {"cure a mob", "focus cure dummy", false, SETUP(focuses(att, 70);)},
    {"cure another without the ki", "focus cure namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_POISON, true); drained(g, att);)},
    {"poison", "focus poison namek", false, SETUP(focuses(att, 70);), false, 12},
    {"poison, unknown", "focus poison namek", false, SETUP()},
    {"poison no one", "focus poison ghost", false, SETUP(focuses(att, 70);)},
    {"poison themselves", "focus poison self", false, SETUP(focuses(att, 70);)},
    {"poison a mob", "focus poison dummy", false, SETUP(focuses(att, 100);), false, 8},
    {"poison an immune mob", "focus poison dummy", false, SETUP(focuses(att, 100); std::string d = "dummy"; char_mob_set(get_char_room_vis(g, att, d, nullptr), MOB_NOPOISON, true);)},
    {"poison the poisoned", "focus poison namek", false, SETUP(focuses(att, 70); char_aff_set(victim, AFF_POISON, true);)},
    {"poison a mutant", "focus poison mutant", false, SETUP(focuses(att, 100); char_set_race(victim, RACE_MUTANT); char_set_genome(victim, 0, 7);), false, 8},
    {"poison a mutant without the genome", "focus poison mutant", false, SETUP(focuses(att, 100); char_set_race(victim, RACE_MUTANT); char_set_genome(victim, 0, 0); char_set_genome(victim, 1, 0);), false, 8},
    {"poison one charging", "focus poison namek", false, SETUP(focuses(att, 100); char_set_charge(victim, getMaxKI(g, victim) / 10);), false, 8},
    {"poison without the ki", "focus poison namek", false, SETUP(focuses(att, 70); drained(g, att);)},
    {"the menu at 1", "create", false, SETUP(creates(g, att, 1);), false, 1},
    {"the menu at 19", "create", false, SETUP(creates(g, att, 19);), false, 1},
    {"the menu at 20", "create", false, SETUP(creates(g, att, 20);), false, 1},
    {"the menu at 29", "create", false, SETUP(creates(g, att, 29);), false, 1},
    {"the menu at 30", "create", false, SETUP(creates(g, att, 30);), false, 1},
    {"the menu at 40", "create", false, SETUP(creates(g, att, 40);), false, 1},
    {"the menu at 50", "create", false, SETUP(creates(g, att, 50);), false, 1},
    {"the menu at 60", "create", false, SETUP(creates(g, att, 60);), false, 1},
    {"the menu at 70", "create", false, SETUP(creates(g, att, 70);), false, 1},
    {"the menu at 80", "create", false, SETUP(creates(g, att, 80);), false, 1},
    {"the menu at 90", "create", false, SETUP(creates(g, att, 90);), false, 1},
    {"the menu at 100", "create", false, SETUP(creates(g, att, 100);), false, 1},
    {"the menu, hidden", "create", false, SETUP(creates(g, att, 100); char_aff_set(att, AFF_HIDE, true);), false, 1},
    {"unknown", "create food", false, SETUP(creates(g, att, 0);), false, 1},
    {"something unknown", "create nothing", false, SETUP(creates(g, att, 100);), false, 1},
    {"something unknown, hidden", "create nothing", false, SETUP(creates(g, att, 100); char_aff_set(att, AFF_HIDE, true);), false, 1},
    {"in capitals", "create Food", false, SETUP(creates(g, att, 100);), false, 1},
    {"light, below the skill", "create light", false, SETUP(creates(g, att, 19, 0, 100);), false, 1},
    {"bag, below the skill", "create bag", false, SETUP(creates(g, att, 29, 0, 100);), false, 1},
    {"mattress, below the skill", "create mattress", false, SETUP(creates(g, att, 39, 0, 100);), false, 1},
    {"weapon, below the skill", "create weapon sword", false, SETUP(creates(g, att, 49, 0, 100);), false, 1},
    {"pants, below the skill", "create pants", false, SETUP(creates(g, att, 49, 0, 100);), false, 1},
    {"gi, below the skill", "create gi", false, SETUP(creates(g, att, 49, 0, 100);), false, 1},
    {"wristband, below the skill", "create wristband", false, SETUP(creates(g, att, 49, 0, 100);), false, 1},
    {"boots, below the skill", "create boots", false, SETUP(creates(g, att, 49, 0, 100);), false, 1},
    {"clothesbeam, below the skill", "create clothesbeam namek", false, SETUP(creates(g, att, 59, 0, 100);), false, 1},
    {"shuriken, below the skill", "create shuriken", false, SETUP(creates(g, att, 69, 0, 100);), false, 1},
    {"senzu, below the skill", "create senzu", false, SETUP(creates(g, att, 79, 0, 100);), false, 1},
    {"kachin, below the skill", "create kachin", false, SETUP(creates(g, att, 89, 0, 100);), false, 1},
    {"elixir, below the skill", "create elixir", false, SETUP(creates(g, att, 99, 0, 100);), false, 1},
    {"food, without the ki", "create food", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"water, without the ki", "create water", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"light, without the ki", "create light", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"bag, without the ki", "create bag", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"mattress, without the ki", "create mattress", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"weapon, without the ki", "create weapon sword", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"pants, without the ki", "create pants", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"gi, without the ki", "create gi", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"wristband, without the ki", "create wristband", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"boots, without the ki", "create boots", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"clothesbeam, without the ki", "create clothesbeam namek", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"shuriken, without the ki", "create shuriken", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"senzu, without the ki", "create senzu", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"kachin, without the ki", "create kachin", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"elixir, without the ki", "create elixir", false, SETUP(creates(g, att, 100, 0, 100); add_follower(g, victim, att); drained(g, att);), false, 1},
    {"food, hidden", "create food", false, SETUP(creates(g, att, 50); char_aff_set(att, AFF_HIDE, true);), false, 1},
    {"food", "create food", false, SETUP(creates(g, att, 1);), false, 1},
    {"food at 20", "create food", false, SETUP(creates(g, att, 20);), false, 1},
    {"food mid, unskilled", "create food mid", false, SETUP(creates(g, att, 49);), false, 1},
    {"food mid", "create food mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"food high, unskilled", "create food high", false, SETUP(creates(g, att, 74);), false, 1},
    {"food high", "create food high", false, SETUP(creates(g, att, 75);), false, 1},
    {"food highest, unskilled", "create food highest", false, SETUP(creates(g, att, 99);), false, 1},
    {"food highest", "create food highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"food HIGH", "create food HIGH", false, SETUP(creates(g, att, 100);), false, 1},
    {"food of no grade", "create food junk", false, SETUP(creates(g, att, 100);), false, 1},
    {"water", "create water", false, SETUP(creates(g, att, 1);), false, 1},
    {"water at 20", "create water", false, SETUP(creates(g, att, 20);), false, 1},
    {"water mid, unskilled", "create water mid", false, SETUP(creates(g, att, 49);), false, 1},
    {"water mid", "create water mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"water high, unskilled", "create water high", false, SETUP(creates(g, att, 74);), false, 1},
    {"water high", "create water high", false, SETUP(creates(g, att, 75);), false, 1},
    {"water highest, unskilled", "create water highest", false, SETUP(creates(g, att, 99);), false, 1},
    {"water highest", "create water highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"water HIGH", "create water HIGH", false, SETUP(creates(g, att, 100);), false, 1},
    {"water of no grade", "create water junk", false, SETUP(creates(g, att, 100);), false, 1},
    {"light", "create light", false, SETUP(creates(g, att, 20);), false, 1},
    {"light at 100", "create light", false, SETUP(creates(g, att, 100, 100);), false, 1},
    {"bag", "create bag", false, SETUP(creates(g, att, 30);), false, 1},
    {"bag at 100", "create bag", false, SETUP(creates(g, att, 100, 100);), false, 1},
    {"mattress", "create mattress", false, SETUP(creates(g, att, 40);), false, 1},
    {"mattress at 100", "create mattress", false, SETUP(creates(g, att, 100, 100);), false, 1},
    {"shuriken", "create shuriken", false, SETUP(creates(g, att, 70);), false, 1},
    {"shuriken at 100", "create shuriken", false, SETUP(creates(g, att, 100, 100);), false, 1},
    {"kachin", "create kachin", false, SETUP(creates(g, att, 90);), false, 1},
    {"kachin at 100", "create kachin", false, SETUP(creates(g, att, 100, 100);), false, 1},
    {"gi, concentrating at 19", "create gi", false, SETUP(creates(g, att, 50, 19);), false, 1},
    {"gi, concentrating at 20", "create gi", false, SETUP(creates(g, att, 50, 20);), false, 1},
    {"gi, concentrating at 30", "create gi", false, SETUP(creates(g, att, 50, 30);), false, 1},
    {"gi, concentrating at 40", "create gi", false, SETUP(creates(g, att, 50, 40);), false, 1},
    {"gi, concentrating at 50", "create gi", false, SETUP(creates(g, att, 50, 50);), false, 1},
    {"gi, concentrating at 60", "create gi", false, SETUP(creates(g, att, 50, 60);), false, 1},
    {"gi, concentrating at 70", "create gi", false, SETUP(creates(g, att, 50, 70);), false, 1},
    {"gi, concentrating at 80", "create gi", false, SETUP(creates(g, att, 50, 80);), false, 1},
    {"gi, concentrating at 90", "create gi", false, SETUP(creates(g, att, 50, 90);), false, 1},
    {"gi, concentrating at 99", "create gi", false, SETUP(creates(g, att, 50, 99);), false, 1},
    {"gi, concentrating at 100", "create gi", false, SETUP(creates(g, att, 50, 100);), false, 1},
    {"gi at level 1", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 1);), false, 1},
    {"gi at level 29", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 29);), false, 1},
    {"gi at level 30", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 30);), false, 1},
    {"gi at level 45", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 45);), false, 1},
    {"gi at level 55", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 55);), false, 1},
    {"gi at level 65", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 65);), false, 1},
    {"gi at level 75", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 75);), false, 1},
    {"gi at level 85", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 85);), false, 1},
    {"gi at level 99", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 99);), false, 1},
    {"gi at level 100", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 100);), false, 1},
    {"gi at level 120", "create gi", false, SETUP(creates(g, att, 50); char_set_class_level(att, 120);), false, 1},
    {"pants", "create pants", false, SETUP(creates(g, att, 50);), false, 1},
    {"pants at level 45", "create pants", false, SETUP(creates(g, att, 50); char_set_class_level(att, 45);), false, 1},
    {"wristband", "create wristband", false, SETUP(creates(g, att, 50);), false, 1},
    {"wristband at level 45", "create wristband", false, SETUP(creates(g, att, 50); char_set_class_level(att, 45);), false, 1},
    {"boots", "create boots", false, SETUP(creates(g, att, 50);), false, 1},
    {"boots at level 45", "create boots", false, SETUP(creates(g, att, 50); char_set_class_level(att, 45);), false, 1},
    {"weapon of no kind", "create weapon", false, SETUP(creates(g, att, 100);), false, 1},
    {"weapon of an unknown kind", "create weapon axe", false, SETUP(creates(g, att, 100);), false, 1},
    {"weapon of an unknown kind, graded", "create weapon axe high", false, SETUP(creates(g, att, 100);), false, 1},
    {"a sword", "create weapon sword", false, SETUP(creates(g, att, 50);), false, 1},
    {"a sword, mid", "create weapon sword mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"a sword, high", "create weapon sword high", false, SETUP(creates(g, att, 50);), false, 1},
    {"a sword, higher", "create weapon sword higher", false, SETUP(creates(g, att, 75);), false, 1},
    {"a sword, highest", "create weapon sword highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"a dagger", "create weapon dagger", false, SETUP(creates(g, att, 50);), false, 1},
    {"a dagger, mid", "create weapon dagger mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"a dagger, high", "create weapon dagger high", false, SETUP(creates(g, att, 50);), false, 1},
    {"a dagger, higher", "create weapon dagger higher", false, SETUP(creates(g, att, 75);), false, 1},
    {"a dagger, highest", "create weapon dagger highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"a club", "create weapon club", false, SETUP(creates(g, att, 50);), false, 1},
    {"a club, mid", "create weapon club mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"a club, high", "create weapon club high", false, SETUP(creates(g, att, 50);), false, 1},
    {"a club, higher", "create weapon club higher", false, SETUP(creates(g, att, 75);), false, 1},
    {"a club, highest", "create weapon club highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"a spear", "create weapon spear", false, SETUP(creates(g, att, 50);), false, 1},
    {"a spear, mid", "create weapon spear mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"a spear, high", "create weapon spear high", false, SETUP(creates(g, att, 50);), false, 1},
    {"a spear, higher", "create weapon spear higher", false, SETUP(creates(g, att, 75);), false, 1},
    {"a spear, highest", "create weapon spear highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"a gun", "create weapon gun", false, SETUP(creates(g, att, 50);), false, 1},
    {"a gun, mid", "create weapon gun mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"a gun, high", "create weapon gun high", false, SETUP(creates(g, att, 50);), false, 1},
    {"a gun, higher", "create weapon gun higher", false, SETUP(creates(g, att, 75);), false, 1},
    {"a gun, highest", "create weapon gun highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"a sword, higher, unskilled", "create weapon sword higher", false, SETUP(creates(g, att, 74);), false, 1},
    {"a sword, highest, unskilled", "create weapon sword highest", false, SETUP(creates(g, att, 99);), false, 1},
    {"a sword, mid, at 50", "create weapon sword mid", false, SETUP(creates(g, att, 50);), false, 1},
    {"a DAGGER, Highest", "create weapon DAGGER Highest", false, SETUP(creates(g, att, 100);), false, 1},
    {"a sword, high and more", "create weapon sword high extra", false, SETUP(creates(g, att, 100);), false, 1},
    {"a sword of no grade", "create weapon sword junk", false, SETUP(creates(g, att, 100);), false, 1},
    {"clothesbeam on no one named", "create clothesbeam", false, SETUP(creates(g, att, 60);), false, 1},
    {"clothesbeam on no one", "create clothesbeam ghost", false, SETUP(creates(g, att, 60);), false, 1},
    {"clothesbeam on one not following", "create clothesbeam namek", false, SETUP(creates(g, att, 60);), false, 1},
    {"clothesbeam on a follower", "create clothesbeam namek", false, SETUP(creates(g, att, 60); add_follower(g, victim, att);), false, 1},
    {"clothesbeam on a follower, at level 45", "create clothesbeam namek", false, SETUP(creates(g, att, 60); add_follower(g, victim, att); char_set_class_level(att, 45);), false, 1},
    {"clothesbeam on themselves", "create clothesbeam self", false, SETUP(creates(g, att, 60);), false, 1},
    {"elixir", "create elixir", false, SETUP(creates(g, att, 100, 0, 20);), false, 1},
    {"elixir, concentrating", "create elixir", false, SETUP(creates(g, att, 100, 100, 10);), false, 1},
    {"elixir without the practices", "create elixir", false, SETUP(creates(g, att, 100, 0, 9);), false, 1},
    {"elixir, hurt", "create elixir", false, SETUP(creates(g, att, 100, 0, 20); decCurHealth(g, att, 1, 0);), false, 1},
    {"senzu", "create senzu", false, SETUP(creates(g, att, 80, 100, 60);), false, 1},
    {"senzu without the practices", "create senzu", false, SETUP(creates(g, att, 100, 0, 49);), false, 1},
    {"senzu, hurt", "create senzu", false, SETUP(creates(g, att, 100, 0, 60); decCurHealth(g, att, 1, 0);), false, 1},
    {"senzu, tired", "create senzu", false, SETUP(creates(g, att, 100, 0, 60); decCurST(g, att, 1, 0);), false, 1},
    {"senzu, short of ki", "create senzu", false, SETUP(creates(g, att, 100, 0, 60); decCurKI(g, att, 1, 0);), false, 1},
    {"the status", "train", false, SETUP(trains(g, att);), false, 1},
    {"the status, with progress", "train", false, SETUP(trains(g, att); char_set_trainstr(g, att, 12345); char_set_traincon(g, att, 7);), false, 1},
    {"the status, capped", "train", false, SETUP(trains(g, att); char_set_real_abils_str(att, 80); char_set_real_abils_wis(att, 80);), false, 1},
    {"the status, as a human", "train", false, SETUP(trains(g, att); char_set_race(att, RACE_HUMAN);), false, 1},
    {"the status, as a kanassan", "train", false, SETUP(trains(g, att); char_set_race(att, RACE_KANASSAN);), false, 1},
    {"the status, as a halfbreed", "train", false, SETUP(trains(g, att); char_set_race(att, RACE_HALFBREED);), false, 1},
    {"the status, as a truffle", "train", false, SETUP(trains(g, att); char_set_race(att, RACE_TRUFFLE);), false, 1},
    {"weighted down", "train str", false, SETUP(trains(g, att); obj_data *rock = read_object(g, 70); obj_set_weight(rock, 2000000000); obj_to_char(g, rock, att);), false, 1},
    {"something unknown", "train luck", false, SETUP(trains(g, att);), false, 1},
    {"something unknown, with a sensei", "train luck", false, SETUP(with_sensei(g, att); trains(g, att);), false, 1},
    {"with a sensei, unpaid", "train str", false, SETUP(with_sensei(g, att, false); trains(g, att);), false, 1},
    {"with a sensei, without practices", "train str", false, SETUP(with_sensei(g, att); char_set_practice_points(g, att, 0); trains(g, att);), false, 1},
    {"with a sensei, at level 5", "train str", false, SETUP(with_sensei(g, att); trains(g, att, 0.001); char_set_class_level(att, 5);), false, 1},
    {"with a sensei, at level 10", "train str", false, SETUP(with_sensei(g, att); trains(g, att, 0.001); char_set_class_level(att, 10);), false, 1},
    {"with a sensei, at level 20", "train str", false, SETUP(with_sensei(g, att); trains(g, att, 0.001); char_set_class_level(att, 20);), false, 1},
    {"with a sensei, at level 40", "train str", false, SETUP(with_sensei(g, att); trains(g, att, 0.001); char_set_class_level(att, 40);), false, 1},
    {"with a sensei, at level 80", "train str", false, SETUP(with_sensei(g, att); trains(g, att, 0.001); char_set_class_level(att, 80);), false, 1},
    {"with a sensei, at level 100", "train str", false, SETUP(with_sensei(g, att); trains(g, att, 0.001); char_set_class_level(att, 100);), false, 1},
    {"str", "train str", false, SETUP(trains(g, att);), false, 4},
    {"str, improving", "train str", false, SETUP(trains(g, att); char_set_trainstr(g, att, 99999); char_set_trainspd(g, att, 99999); char_set_traincon(g, att, 99999); char_set_trainint(g, att, 99999); char_set_trainwis(g, att, 99999); char_set_trainagl(g, att, 99999);), false, 1},
    {"str, maxed", "train str", false, SETUP(trains(g, att); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 80);), false, 1},
    {"spd", "train spd", false, SETUP(trains(g, att);), false, 4},
    {"spd, improving", "train spd", false, SETUP(trains(g, att); char_set_trainstr(g, att, 99999); char_set_trainspd(g, att, 99999); char_set_traincon(g, att, 99999); char_set_trainint(g, att, 99999); char_set_trainwis(g, att, 99999); char_set_trainagl(g, att, 99999);), false, 1},
    {"spd, maxed", "train spd", false, SETUP(trains(g, att); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 80);), false, 1},
    {"con", "train con", false, SETUP(trains(g, att);), false, 4},
    {"con, improving", "train con", false, SETUP(trains(g, att); char_set_trainstr(g, att, 99999); char_set_trainspd(g, att, 99999); char_set_traincon(g, att, 99999); char_set_trainint(g, att, 99999); char_set_trainwis(g, att, 99999); char_set_trainagl(g, att, 99999);), false, 1},
    {"con, maxed", "train con", false, SETUP(trains(g, att); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 80);), false, 1},
    {"int", "train int", false, SETUP(trains(g, att);), false, 4},
    {"int, improving", "train int", false, SETUP(trains(g, att); char_set_trainstr(g, att, 99999); char_set_trainspd(g, att, 99999); char_set_traincon(g, att, 99999); char_set_trainint(g, att, 99999); char_set_trainwis(g, att, 99999); char_set_trainagl(g, att, 99999);), false, 1},
    {"int, maxed", "train int", false, SETUP(trains(g, att); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 80);), false, 1},
    {"wis", "train wis", false, SETUP(trains(g, att);), false, 4},
    {"wis, improving", "train wis", false, SETUP(trains(g, att); char_set_trainstr(g, att, 99999); char_set_trainspd(g, att, 99999); char_set_traincon(g, att, 99999); char_set_trainint(g, att, 99999); char_set_trainwis(g, att, 99999); char_set_trainagl(g, att, 99999);), false, 1},
    {"wis, maxed", "train wis", false, SETUP(trains(g, att); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 80);), false, 1},
    {"agl", "train agl", false, SETUP(trains(g, att);), false, 4},
    {"agl, improving", "train agl", false, SETUP(trains(g, att); char_set_trainstr(g, att, 99999); char_set_trainspd(g, att, 99999); char_set_traincon(g, att, 99999); char_set_trainint(g, att, 99999); char_set_trainwis(g, att, 99999); char_set_trainagl(g, att, 99999);), false, 1},
    {"agl, maxed", "train agl", false, SETUP(trains(g, att); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 80);), false, 1},
    {"STR", "train STR", false, SETUP(trains(g, att);), false, 1},
    {"str, improving as a human", "train str", false, SETUP(trains(g, att); char_set_race(att, RACE_HUMAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"con, improving as a human", "train con", false, SETUP(trains(g, att); char_set_race(att, RACE_HUMAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"int, improving as a human", "train int", false, SETUP(trains(g, att); char_set_race(att, RACE_HUMAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"agl, improving as a human", "train agl", false, SETUP(trains(g, att); char_set_race(att, RACE_HUMAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"str, improving as a kanassan", "train str", false, SETUP(trains(g, att); char_set_race(att, RACE_KANASSAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"con, improving as a kanassan", "train con", false, SETUP(trains(g, att); char_set_race(att, RACE_KANASSAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"int, improving as a kanassan", "train int", false, SETUP(trains(g, att); char_set_race(att, RACE_KANASSAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"agl, improving as a kanassan", "train agl", false, SETUP(trains(g, att); char_set_race(att, RACE_KANASSAN); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"str, improving as a halfbreed", "train str", false, SETUP(trains(g, att); char_set_race(att, RACE_HALFBREED); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"con, improving as a halfbreed", "train con", false, SETUP(trains(g, att); char_set_race(att, RACE_HALFBREED); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"int, improving as a halfbreed", "train int", false, SETUP(trains(g, att); char_set_race(att, RACE_HALFBREED); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"agl, improving as a halfbreed", "train agl", false, SETUP(trains(g, att); char_set_race(att, RACE_HALFBREED); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"str, improving as a truffle", "train str", false, SETUP(trains(g, att); char_set_race(att, RACE_TRUFFLE); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"con, improving as a truffle", "train con", false, SETUP(trains(g, att); char_set_race(att, RACE_TRUFFLE); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"int, improving as a truffle", "train int", false, SETUP(trains(g, att); char_set_race(att, RACE_TRUFFLE); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"agl, improving as a truffle", "train agl", false, SETUP(trains(g, att); char_set_race(att, RACE_TRUFFLE); char_set_trainstr(g, att, 18000); char_set_traincon(g, att, 18000); char_set_trainint(g, att, 18000); char_set_trainagl(g, att, 18000);), false, 1},
    {"str, flawed at 70", "train str", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_WIMP, 1); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 70);), false, 1},
    {"spd, flawed at 70", "train spd", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_SLOW, 1); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 70);), false, 1},
    {"con, flawed at 70", "train con", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_FRAIL, 1); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 70);), false, 1},
    {"int, flawed at 70", "train int", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_DULL, 1); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 70);), false, 1},
    {"wis, flawed at 70", "train wis", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_FOOLISH, 1); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 70);), false, 1},
    {"agl, flawed at 70", "train agl", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_CLUMSY, 1); for (auto set : {char_set_real_abils_str, char_set_real_abils_cha, char_set_real_abils_con, char_set_real_abils_intel, char_set_real_abils_wis, char_set_real_abils_dex}) set(att, 70);), false, 1},
    {"str, flawed at 69", "train str", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_WIMP, 1); char_set_real_abils_str(att, 69);), false, 1},
    {"str at level 20, at 20", "train str", false, SETUP(trains(g, att); char_set_class_level(att, 20); char_set_real_abils_str(att, 20);), false, 1},
    {"str at level 21, at 20", "train str", false, SETUP(trains(g, att); char_set_class_level(att, 21); char_set_real_abils_str(att, 20);), false, 1},
    {"str at level 40, at 40", "train str", false, SETUP(trains(g, att); char_set_class_level(att, 40); char_set_real_abils_str(att, 40);), false, 1},
    {"str at level 41, at 40", "train str", false, SETUP(trains(g, att); char_set_class_level(att, 41); char_set_real_abils_str(att, 40);), false, 1},
    {"str at level 60, at 60", "train str", false, SETUP(trains(g, att); char_set_class_level(att, 60); char_set_real_abils_str(att, 60);), false, 1},
    {"str at level 61, at 60", "train str", false, SETUP(trains(g, att); char_set_class_level(att, 61); char_set_real_abils_str(att, 60);), false, 1},
    {"str, tired", "train str", false, SETUP(trains(g, att); decCurST(g, att, getMaxST(g, att), 0);), false, 1},
    {"int, without the ki", "train int", false, SETUP(trains(g, att); drained(g, att);), false, 1},
    {"str, with a share of 0", "train str", false, SETUP(trains(g, att, 0);), false, 1},
    {"str, with a share of 0.05", "train str", false, SETUP(trains(g, att, 0.05);), false, 1},
    {"int, with a share of 0.05", "train int", false, SETUP(trains(g, att, 0.05);), false, 1},
    {"str, with a share of 0.08", "train str", false, SETUP(trains(g, att, 0.08);), false, 1},
    {"int, with a share of 0.08", "train int", false, SETUP(trains(g, att, 0.08);), false, 1},
    {"str, with a share of 0.15", "train str", false, SETUP(trains(g, att, 0.15);), false, 1},
    {"int, with a share of 0.15", "train int", false, SETUP(trains(g, att, 0.15);), false, 1},
    {"int, with a share of 0", "train int", false, SETUP(trains(g, att, 0);), false, 1},
    {"str, with a share of 0.1", "train str", false, SETUP(trains(g, att, 0.1);), false, 1},
    {"int, with a share of 0.1", "train int", false, SETUP(trains(g, att, 0.1);), false, 1},
    {"str, with a share of 0.2", "train str", false, SETUP(trains(g, att, 0.2);), false, 1},
    {"int, with a share of 0.2", "train int", false, SETUP(trains(g, att, 0.2);), false, 1},
    {"str, with a share of 0.4", "train str", false, SETUP(trains(g, att, 0.4);), false, 1},
    {"int, with a share of 0.4", "train int", false, SETUP(trains(g, att, 0.4);), false, 1},
    {"str, with a share of 0.7", "train str", false, SETUP(trains(g, att, 0.7);), false, 1},
    {"int, with a share of 0.7", "train int", false, SETUP(trains(g, att, 0.7);), false, 1},
    {"str, with a share of 1.5", "train str", false, SETUP(trains(g, att, 1.5);), false, 1},
    {"int, with a share of 1.5", "train int", false, SETUP(trains(g, att, 1.5);), false, 1},
    {"str, with a share of 3", "train str", false, SETUP(trains(g, att, 3);), false, 1},
    {"int, with a share of 3", "train int", false, SETUP(trains(g, att, 3);), false, 1},
    {"str, relaxed 115 times", "train str", false, SETUP(trains(g, att); char_set_relax_count(att, 115);), false, 1},
    {"str, relaxed 116 times", "train str", false, SETUP(trains(g, att); char_set_relax_count(att, 116);), false, 1},
    {"str, relaxed 232 times", "train str", false, SETUP(trains(g, att); char_set_relax_count(att, 232);), false, 1},
    {"str, relaxed 464 times", "train str", false, SETUP(trains(g, att); char_set_relax_count(att, 464);), false, 1},
    {"str, brawny", "train str", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_BRAWNY, 1);), false, 1},
    {"spd, quick", "train spd", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_QUICK, 1);), false, 1},
    {"con, sturdy", "train con", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_STURDY, 1);), false, 1},
    {"int, scholarly", "train int", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_SCHOLARLY, 1);), false, 1},
    {"wis, sage", "train wis", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_SAGE, 1);), false, 1},
    {"agl, agile", "train agl", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_AGILE, 1);), false, 1},
    {"str, hardworker", "train str", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_HARDWORKER, 1);), false, 1},
    {"str, loner", "train str", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_LONER, 1);), false, 1},
    {"int, hardworker", "train int", false, SETUP(trains(g, att); char_set_bonuses(att, BONUS_HARDWORKER, 1);), false, 1},
    {"str, in the time chamber", "train str", false, SETUP(trains(g, att); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"str, with a sensei", "train str", false, SETUP(with_sensei(g, att); trains(g, att);), false, 4},
    {"int, with a sensei", "train int", false, SETUP(with_sensei(g, att); trains(g, att);), false, 1},
    {"str, improving with a sensei", "train str", false, SETUP(with_sensei(g, att); trains(g, att); char_set_trainstr(g, att, 99999);), false, 1},
    {"str, hidden", "train str", false, SETUP(trains(g, att); char_aff_set(att, AFF_HIDE, true);), false, 1},
    {"str, improving as Nail's namek", "train str", false, SETUP(trains(g, att); char_set_class(att, CLASS_PICCOLO); char_set_race(att, RACE_NAMEK); char_set_exp(att, 0); char_set_trainstr(g, att, 99999);), false, 1},
    {"str, improving as Nail's namek, past the level", "train str", false, SETUP(trains(g, att); char_set_class(att, CLASS_PICCOLO); char_set_race(att, RACE_NAMEK); char_set_exp(att, 4000000000000LL); char_set_trainstr(g, att, 99999);), false, 1},
    {"str, improving as Piccolo's saiyan", "train str", false, SETUP(trains(g, att); char_set_class(att, CLASS_PICCOLO); char_set_race(att, RACE_SAIYAN); char_set_exp(att, 0); char_set_trainstr(g, att, 99999);), false, 1},
    {"in hell", "situp", false, SETUP(exercises(g, att, 10); room_set_flag(g, char_get_in_room(att), ROOM_HELL, true);), false, 1},
    {"fishing", "situp", false, SETUP(exercises(g, att, 10); char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"dragging", "situp", false, SETUP(exercises(g, att, 10); char_set_drag(att, victim); char_set_dragged(victim, att);), false, 1},
    {"carrying", "situp", false, SETUP(exercises(g, att, 10); char_set_carrying(g, att, victim); char_set_carried_by(g, victim, att);), false, 1},
    {"dragging and fishing", "situp", false, SETUP(exercises(g, att, 10); char_set_drag(att, victim); char_set_dragged(victim, att); char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"as an android", "situp", false, SETUP(exercises(g, att, 10); char_set_race(att, RACE_ANDROID);), false, 1},
    {"as a majin", "situp", false, SETUP(exercises(g, att, 10); char_set_race(att, RACE_MAJIN);), false, 1},
    {"ensnared", "situp", false, SETUP(exercises(g, att, 0); char_aff_set(att, AFF_ENSNARED, true);), false, 4},
    {"with no legs", "situp", false, SETUP(exercises(g, att, 0); char_set_limb_condition(att, 2, 0); char_set_limb_condition(att, 3, 0);), false, 1},
    {"with no arms", "pushup", false, SETUP(exercises(g, att, 0); char_set_limb_condition(att, 0, 0); char_set_limb_condition(att, 1, 0);), false, 1},
    {"tired", "situp", false, SETUP(exercises(g, att, 10); decCurST(g, att, getMaxST(g, att), 0);), false, 1},
    {"sparring", "situp", false, SETUP(exercises(g, att, 0); char_plr_set(att, PLR_SPAR, true);), false, 1},
    {"fighting", "situp", true, SETUP(exercises(g, att, 0);), false, 1},
    {"flying", "situp", false, SETUP(exercises(g, att, 0); char_aff_set(att, AFF_FLYING, true);), false, 1},
    {"in gravity 0", "situp", false, SETUP(exercises(g, att, 0, 1000000000LL);), false, 2},
    {"in gravity 5", "situp", false, SETUP(exercises(g, att, 5, 1000000000LL);), false, 2},
    {"in gravity 10", "situp", false, SETUP(exercises(g, att, 10, 1000000000LL);), false, 2},
    {"in gravity 20", "situp", false, SETUP(exercises(g, att, 20, 1000000000LL);), false, 2},
    {"in gravity 30", "situp", false, SETUP(exercises(g, att, 30, 1000000000LL);), false, 2},
    {"in gravity 40", "situp", false, SETUP(exercises(g, att, 40, 1000000000LL);), false, 2},
    {"in gravity 50", "situp", false, SETUP(exercises(g, att, 50, 1000000000LL);), false, 2},
    {"in gravity 100", "situp", false, SETUP(exercises(g, att, 100, 1000000000LL);), false, 2},
    {"in gravity 200", "situp", false, SETUP(exercises(g, att, 200, 1000000000LL);), false, 2},
    {"in gravity 300", "situp", false, SETUP(exercises(g, att, 300, 1000000000LL);), false, 2},
    {"in gravity 400", "situp", false, SETUP(exercises(g, att, 400, 1000000000LL);), false, 2},
    {"in gravity 500", "situp", false, SETUP(exercises(g, att, 500, 1000000000LL);), false, 2},
    {"in gravity 1000", "situp", false, SETUP(exercises(g, att, 1000, 1000000000LL);), false, 2},
    {"in gravity 5000", "situp", false, SETUP(exercises(g, att, 5000, 1000000000LL);), false, 2},
    {"in gravity 10000", "situp", false, SETUP(exercises(g, att, 10000, 1000000000LL);), false, 2},
    {"in gravity 20000", "situp", false, SETUP(exercises(g, att, 20000, 1000000000LL);), false, 2},
    {"in gravity 10, with 0 more", "situp", false, SETUP(exercises(g, att, 10, 0);), false, 1},
    {"in gravity 0, with 0 more", "situp", false, SETUP(exercises(g, att, 0, 0);), false, 1},
    {"in gravity 10, with 2000 more", "situp", false, SETUP(exercises(g, att, 10, 2000);), false, 1},
    {"in gravity 0, with 2000 more", "situp", false, SETUP(exercises(g, att, 0, 2000);), false, 1},
    {"in gravity 10, with 5000 more", "situp", false, SETUP(exercises(g, att, 10, 5000);), false, 1},
    {"in gravity 0, with 5000 more", "situp", false, SETUP(exercises(g, att, 0, 5000);), false, 1},
    {"in gravity 10, with 10000 more", "situp", false, SETUP(exercises(g, att, 10, 10000);), false, 1},
    {"in gravity 0, with 10000 more", "situp", false, SETUP(exercises(g, att, 0, 10000);), false, 1},
    {"in gravity 10, with 18000 more", "situp", false, SETUP(exercises(g, att, 10, 18000);), false, 1},
    {"in gravity 0, with 18000 more", "situp", false, SETUP(exercises(g, att, 0, 18000);), false, 1},
    {"in gravity 10, with 30000 more", "situp", false, SETUP(exercises(g, att, 10, 30000);), false, 1},
    {"in gravity 0, with 30000 more", "situp", false, SETUP(exercises(g, att, 0, 30000);), false, 1},
    {"in gravity 10, with 50000 more", "situp", false, SETUP(exercises(g, att, 10, 50000);), false, 1},
    {"in gravity 0, with 50000 more", "situp", false, SETUP(exercises(g, att, 0, 50000);), false, 1},
    {"in gravity 10, with 100000 more", "situp", false, SETUP(exercises(g, att, 10, 100000);), false, 1},
    {"in gravity 0, with 100000 more", "situp", false, SETUP(exercises(g, att, 0, 100000);), false, 1},
    {"in gravity 10, with 1000000 more", "situp", false, SETUP(exercises(g, att, 10, 1000000);), false, 1},
    {"in gravity 0, with 1000000 more", "situp", false, SETUP(exercises(g, att, 0, 1000000);), false, 1},
    {"relaxed 116 times", "situp", false, SETUP(exercises(g, att, 10, 1000000000LL); char_set_relax_count(att, 116);), false, 1},
    {"relaxed 232 times", "situp", false, SETUP(exercises(g, att, 10, 1000000000LL); char_set_relax_count(att, 232);), false, 1},
    {"relaxed 464 times", "situp", false, SETUP(exercises(g, att, 10, 1000000000LL); char_set_relax_count(att, 464);), false, 1},
    {"hardworker", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_bonuses(att, BONUS_HARDWORKER, 1);), false, 1},
    {"slacker", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_bonuses(att, BONUS_SLACKER, 1);), false, 1},
    {"loner", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_bonuses(att, BONUS_LONER, 1);), false, 1},
    {"as a namek", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_race(att, RACE_NAMEK);), false, 1},
    {"as a human", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_race(att, RACE_HUMAN);), false, 1},
    {"as Bardock's, in gravity 10", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_class(att, CLASS_BARDOCK);), false, 1},
    {"as Bardock's, in gravity 20", "situp", false, SETUP(exercises(g, att, 20, 10000); char_set_class(att, CLASS_BARDOCK);), false, 1},
    {"in the time chamber", "situp", false, SETUP(exercises(g, att, 10, 10000); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"in the time chamber, at no gain", "situp", false, SETUP(exercises(g, att, 5, 10000); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"in a workout room", "situp", false, SETUP(exercises(g, att, 10, 10000); room_set_flag(g, char_get_in_room(att), ROOM_WORKOUT, true);), false, 1},
    {"in a workout room, at no gain", "situp", false, SETUP(exercises(g, att, 5, 10000); room_set_flag(g, char_get_in_room(att), ROOM_WORKOUT, true);), false, 1},
    {"at level 1", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_class_level(att, 1);), false, 1},
    {"at level 100", "situp", false, SETUP(exercises(g, att, 10, 10000); char_set_class_level(att, 100);), false, 1},
    {"at the soft cap", "situp", false, SETUP(exercises(g, att, 10, 100000000000LL); char_set_class_level(att, 1);), false, 1},
    {"in hell", "pushup", false, SETUP(exercises(g, att, 10); room_set_flag(g, char_get_in_room(att), ROOM_HELL, true);), false, 1},
    {"fishing", "pushup", false, SETUP(exercises(g, att, 10); char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"dragging", "pushup", false, SETUP(exercises(g, att, 10); char_set_drag(att, victim); char_set_dragged(victim, att);), false, 1},
    {"carrying", "pushup", false, SETUP(exercises(g, att, 10); char_set_carrying(g, att, victim); char_set_carried_by(g, victim, att);), false, 1},
    {"dragging and fishing", "pushup", false, SETUP(exercises(g, att, 10); char_set_drag(att, victim); char_set_dragged(victim, att); char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"as an android", "pushup", false, SETUP(exercises(g, att, 10); char_set_race(att, RACE_ANDROID);), false, 1},
    {"as a majin", "pushup", false, SETUP(exercises(g, att, 10); char_set_race(att, RACE_MAJIN);), false, 1},
    {"ensnared", "pushup", false, SETUP(exercises(g, att, 0); char_aff_set(att, AFF_ENSNARED, true);), false, 4},
    {"tired", "pushup", false, SETUP(exercises(g, att, 10); decCurST(g, att, getMaxST(g, att), 0);), false, 1},
    {"sparring", "pushup", false, SETUP(exercises(g, att, 0); char_plr_set(att, PLR_SPAR, true);), false, 1},
    {"fighting", "pushup", true, SETUP(exercises(g, att, 0);), false, 1},
    {"flying", "pushup", false, SETUP(exercises(g, att, 0); char_aff_set(att, AFF_FLYING, true);), false, 1},
    {"in gravity 0", "pushup", false, SETUP(exercises(g, att, 0, 1000000000LL);), false, 2},
    {"in gravity 5", "pushup", false, SETUP(exercises(g, att, 5, 1000000000LL);), false, 2},
    {"in gravity 10", "pushup", false, SETUP(exercises(g, att, 10, 1000000000LL);), false, 2},
    {"in gravity 20", "pushup", false, SETUP(exercises(g, att, 20, 1000000000LL);), false, 2},
    {"in gravity 30", "pushup", false, SETUP(exercises(g, att, 30, 1000000000LL);), false, 2},
    {"in gravity 40", "pushup", false, SETUP(exercises(g, att, 40, 1000000000LL);), false, 2},
    {"in gravity 50", "pushup", false, SETUP(exercises(g, att, 50, 1000000000LL);), false, 2},
    {"in gravity 100", "pushup", false, SETUP(exercises(g, att, 100, 1000000000LL);), false, 2},
    {"in gravity 200", "pushup", false, SETUP(exercises(g, att, 200, 1000000000LL);), false, 2},
    {"in gravity 300", "pushup", false, SETUP(exercises(g, att, 300, 1000000000LL);), false, 2},
    {"in gravity 400", "pushup", false, SETUP(exercises(g, att, 400, 1000000000LL);), false, 2},
    {"in gravity 500", "pushup", false, SETUP(exercises(g, att, 500, 1000000000LL);), false, 2},
    {"in gravity 1000", "pushup", false, SETUP(exercises(g, att, 1000, 1000000000LL);), false, 2},
    {"in gravity 5000", "pushup", false, SETUP(exercises(g, att, 5000, 1000000000LL);), false, 2},
    {"in gravity 10000", "pushup", false, SETUP(exercises(g, att, 10000, 1000000000LL);), false, 2},
    {"in gravity 20000", "pushup", false, SETUP(exercises(g, att, 20000, 1000000000LL);), false, 2},
    {"in gravity 10, with 0 more", "pushup", false, SETUP(exercises(g, att, 10, 0);), false, 1},
    {"in gravity 0, with 0 more", "pushup", false, SETUP(exercises(g, att, 0, 0);), false, 1},
    {"in gravity 10, with 2000 more", "pushup", false, SETUP(exercises(g, att, 10, 2000);), false, 1},
    {"in gravity 0, with 2000 more", "pushup", false, SETUP(exercises(g, att, 0, 2000);), false, 1},
    {"in gravity 10, with 5000 more", "pushup", false, SETUP(exercises(g, att, 10, 5000);), false, 1},
    {"in gravity 0, with 5000 more", "pushup", false, SETUP(exercises(g, att, 0, 5000);), false, 1},
    {"in gravity 10, with 10000 more", "pushup", false, SETUP(exercises(g, att, 10, 10000);), false, 1},
    {"in gravity 0, with 10000 more", "pushup", false, SETUP(exercises(g, att, 0, 10000);), false, 1},
    {"in gravity 10, with 18000 more", "pushup", false, SETUP(exercises(g, att, 10, 18000);), false, 1},
    {"in gravity 0, with 18000 more", "pushup", false, SETUP(exercises(g, att, 0, 18000);), false, 1},
    {"in gravity 10, with 30000 more", "pushup", false, SETUP(exercises(g, att, 10, 30000);), false, 1},
    {"in gravity 0, with 30000 more", "pushup", false, SETUP(exercises(g, att, 0, 30000);), false, 1},
    {"in gravity 10, with 50000 more", "pushup", false, SETUP(exercises(g, att, 10, 50000);), false, 1},
    {"in gravity 0, with 50000 more", "pushup", false, SETUP(exercises(g, att, 0, 50000);), false, 1},
    {"in gravity 10, with 100000 more", "pushup", false, SETUP(exercises(g, att, 10, 100000);), false, 1},
    {"in gravity 0, with 100000 more", "pushup", false, SETUP(exercises(g, att, 0, 100000);), false, 1},
    {"in gravity 10, with 1000000 more", "pushup", false, SETUP(exercises(g, att, 10, 1000000);), false, 1},
    {"in gravity 0, with 1000000 more", "pushup", false, SETUP(exercises(g, att, 0, 1000000);), false, 1},
    {"relaxed 116 times", "pushup", false, SETUP(exercises(g, att, 10, 1000000000LL); char_set_relax_count(att, 116);), false, 1},
    {"relaxed 232 times", "pushup", false, SETUP(exercises(g, att, 10, 1000000000LL); char_set_relax_count(att, 232);), false, 1},
    {"relaxed 464 times", "pushup", false, SETUP(exercises(g, att, 10, 1000000000LL); char_set_relax_count(att, 464);), false, 1},
    {"hardworker", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_bonuses(att, BONUS_HARDWORKER, 1);), false, 1},
    {"slacker", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_bonuses(att, BONUS_SLACKER, 1);), false, 1},
    {"loner", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_bonuses(att, BONUS_LONER, 1);), false, 1},
    {"as a namek", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_race(att, RACE_NAMEK);), false, 1},
    {"as a human", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_race(att, RACE_HUMAN);), false, 1},
    {"as Bardock's, in gravity 10", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_class(att, CLASS_BARDOCK);), false, 1},
    {"as Bardock's, in gravity 20", "pushup", false, SETUP(exercises(g, att, 20, 10000); char_set_class(att, CLASS_BARDOCK);), false, 1},
    {"in the time chamber", "pushup", false, SETUP(exercises(g, att, 10, 10000); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"in the time chamber, at no gain", "pushup", false, SETUP(exercises(g, att, 5, 10000); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"in a workout room", "pushup", false, SETUP(exercises(g, att, 10, 10000); room_set_flag(g, char_get_in_room(att), ROOM_WORKOUT, true);), false, 1},
    {"in a workout room, at no gain", "pushup", false, SETUP(exercises(g, att, 5, 10000); room_set_flag(g, char_get_in_room(att), ROOM_WORKOUT, true);), false, 1},
    {"at level 1", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_class_level(att, 1);), false, 1},
    {"at level 100", "pushup", false, SETUP(exercises(g, att, 10, 10000); char_set_class_level(att, 100);), false, 1},
    {"at the soft cap", "pushup", false, SETUP(exercises(g, att, 10, 100000000000LL); char_set_class_level(att, 1);), false, 1},
    {"in hell", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); room_set_flag(g, char_get_in_room(att), ROOM_HELL, true);), false, 1},
    {"fishing", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"as an android", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_race(att, RACE_ANDROID);), false, 1},
    {"as an arlian", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_race(att, RACE_ARLIAN);), false, 1},
    {"carrying", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_carrying(g, att, victim); char_set_carried_by(g, victim, att);), false, 1},
    {"dragging", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_drag(att, victim); char_set_dragged(victim, att);), false, 1},
    {"carrying and dragging", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_carrying(g, att, victim); char_set_carried_by(g, victim, att); char_set_drag(att, victim); char_set_dragged(victim, att);), false, 1},
    {"sparring", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_plr_set(att, PLR_SPAR, true);), false, 1},
    {"fighting", "meditate item70", true, SETUP(exercises(g, att, 10); to_lift(g, att);), false, 1},
    {"standing", "meditate item70", false, SETUP(exercises(g, att, 10);), false, 1},
    {"nothing", "meditate", false, SETUP(exercises(g, att, 10); to_lift(g, att);), false, 1},
    {"something not here", "meditate rock", false, SETUP(exercises(g, att, 10); to_lift(g, att);), false, 1},
    {"the frozen", "meditate item79", false, SETUP(exercises(g, att, 10); to_lift(g, att); obj_to_room(g, read_object(g, 79), char_get_in_room(att));), false, 1},
    {"without the ki", "meditate item70", false, SETUP(exercises(g, att, 10); to_lift(g, att); drained(g, att);), false, 1},
    {"expanding without the practices", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 3499);), false, 1},
    {"expanding", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 3500);), false, 1},
    {"expanding as a saiyan", "meditate EXPAND", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_race(att, RACE_SAIYAN); char_set_practice_points(g, att, 3500);), false, 1},
    {"expanding as a saiyan, with the practices", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_race(att, RACE_SAIYAN); char_set_practice_points(g, att, 7000);), false, 1},
    {"expanding at 59 slots, with memory 0", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 7000); char_set_skill_slots(att, 59); char_set_bonuses(att, BONUS_GMEMORY, 0);), false, 1},
    {"expanding at 60 slots, with memory 0", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 7000); char_set_skill_slots(att, 60); char_set_bonuses(att, BONUS_GMEMORY, 0);), false, 1},
    {"expanding at 60 slots, with memory 1", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 7000); char_set_skill_slots(att, 60); char_set_bonuses(att, BONUS_GMEMORY, 1);), false, 1},
    {"expanding at 64 slots, with memory 1", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 7000); char_set_skill_slots(att, 64); char_set_bonuses(att, BONUS_GMEMORY, 1);), false, 1},
    {"expanding at 65 slots, with memory 1", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 7000); char_set_skill_slots(att, 65); char_set_bonuses(att, BONUS_GMEMORY, 1);), false, 1},
    {"expanding at 65 slots, with memory 2", "meditate expand", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_practice_points(g, att, 7000); char_set_skill_slots(att, 65); char_set_bonuses(att, BONUS_GMEMORY, 2);), false, 1},
    {"breaking no link", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att);), false, 1},
    {"breaking their own link", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_mindlink(att, victim); char_set_mindlink(victim, att); char_set_linker(att, 1);), false, 1},
    {"breaking a link without the ki", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_mindlink(att, victim); char_set_mindlink(victim, att); char_set_linker(victim, 1); drained(g, att);), false, 1},
    {"breaking a link", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_mindlink(att, victim); char_set_mindlink(victim, att); char_set_linker(victim, 1);), false, 12},
    {"breaking a link, sharp", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_mindlink(att, victim); char_set_mindlink(victim, att); char_set_linker(victim, 1); char_set_aff_abils_intel(att, 100); char_set_aff_abils_intel(victim, 5);), false, 12},
    {"breaking a link, dull", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_mindlink(att, victim); char_set_mindlink(victim, att); char_set_linker(victim, 1); char_set_aff_abils_intel(att, 5); char_set_aff_abils_intel(victim, 100); char_set_skills_base(victim, SKILL_TELEPATHY, 100);), false, 4},
    {"breaking a link, sharp, with them shocked", "meditate break", false, SETUP(exercises(g, att, 10); to_lift(g, att); char_set_mindlink(att, victim); char_set_mindlink(victim, att); char_set_linker(victim, 1); char_set_aff_abils_intel(att, 100); char_set_aff_abils_intel(victim, 5); char_aff_set(victim, AFF_SHOCKED, true);), false, 4},
    {"in gravity 0", "meditate item70", false, SETUP(exercises(g, att, 0, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 5", "meditate item70", false, SETUP(exercises(g, att, 5, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 20", "meditate item70", false, SETUP(exercises(g, att, 20, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 30", "meditate item70", false, SETUP(exercises(g, att, 30, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 40", "meditate item70", false, SETUP(exercises(g, att, 40, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 50", "meditate item70", false, SETUP(exercises(g, att, 50, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 100", "meditate item70", false, SETUP(exercises(g, att, 100, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 200", "meditate item70", false, SETUP(exercises(g, att, 200, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 300", "meditate item70", false, SETUP(exercises(g, att, 300, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 400", "meditate item70", false, SETUP(exercises(g, att, 400, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 500", "meditate item70", false, SETUP(exercises(g, att, 500, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 1000", "meditate item70", false, SETUP(exercises(g, att, 1000, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 5000", "meditate item70", false, SETUP(exercises(g, att, 5000, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 10000", "meditate item70", false, SETUP(exercises(g, att, 10000, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 20000", "meditate item70", false, SETUP(exercises(g, att, 20000, 1000000000LL); to_lift(g, att);), false, 2},
    {"in gravity 10, with 0 more", "meditate item70", false, SETUP(exercises(g, att, 10, 0); to_lift(g, att);), false, 1},
    {"in gravity 10, with 2000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 2000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 5000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 5000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 10000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 18000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 18000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 30000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 30000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 50000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 50000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 100000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 100000); to_lift(g, att);), false, 1},
    {"in gravity 10, with 1000000 more", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000); to_lift(g, att);), false, 1},
    {"lifting 0", "meditate item70", false, SETUP(exercises(g, att, 0, 1000000000LL); to_lift(g, att, 0);), false, 1},
    {"lifting 0 in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att, 0);), false, 1},
    {"lifting 499", "meditate item70", false, SETUP(exercises(g, att, 0, 1000000000LL); to_lift(g, att, 499);), false, 1},
    {"lifting 499 in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att, 499);), false, 1},
    {"lifting 500", "meditate item70", false, SETUP(exercises(g, att, 0, 1000000000LL); to_lift(g, att, 500);), false, 1},
    {"lifting 500 in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att, 500);), false, 1},
    {"lifting 5000", "meditate item70", false, SETUP(exercises(g, att, 0, 1000000000LL); to_lift(g, att, 5000);), false, 1},
    {"lifting 5000 in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att, 5000);), false, 1},
    {"lifting 100000", "meditate item70", false, SETUP(exercises(g, att, 0, 1000000000LL); to_lift(g, att, 100000);), false, 1},
    {"lifting 100000 in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att, 100000);), false, 1},
    {"lifting one sitting on it", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); obj_data *obj = to_lift(g, att); obj_set_sitting(obj, victim); char_set_sits(victim, obj); char_set_position(victim, POS_SITTING);), false, 1},
    {"relaxed 116 times", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att); char_set_relax_count(att, 116);), false, 1},
    {"relaxed 232 times", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att); char_set_relax_count(att, 232);), false, 1},
    {"relaxed 464 times", "meditate item70", false, SETUP(exercises(g, att, 10, 1000000000LL); to_lift(g, att); char_set_relax_count(att, 464);), false, 1},
    {"hardworker", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_bonuses(att, BONUS_HARDWORKER, 1);), false, 1},
    {"slacker", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_bonuses(att, BONUS_SLACKER, 1);), false, 1},
    {"loner", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_bonuses(att, BONUS_LONER, 1);), false, 1},
    {"as a namek", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_race(att, RACE_NAMEK);), false, 1},
    {"as a demon", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_race(att, RACE_DEMON);), false, 12},
    {"as a demon, at no gain", "meditate item70", false, SETUP(exercises(g, att, 5, 10000); to_lift(g, att); char_set_race(att, RACE_DEMON);), false, 8},
    {"as Bardock's, in gravity 10", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_class(att, CLASS_BARDOCK);), false, 1},
    {"in the time chamber", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"in the time chamber, at no gain", "meditate item70", false, SETUP(exercises(g, att, 5, 10000); to_lift(g, att); room_set_flag(g, char_get_in_room(att), ROOM_HBTC, true);), false, 1},
    {"in a workout room", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); room_set_flag(g, char_get_in_room(att), ROOM_WORKOUT, true);), false, 1},
    {"in a workout room, at no gain", "meditate item70", false, SETUP(exercises(g, att, 5, 10000); to_lift(g, att); room_set_flag(g, char_get_in_room(att), ROOM_WORKOUT, true);), false, 1},
    {"at the soft cap", "meditate item70", false, SETUP(exercises(g, att, 10, 100000000000LL); to_lift(g, att); char_set_class_level(att, 1);), false, 1},
    {"at level 100", "meditate item70", false, SETUP(exercises(g, att, 10, 10000); to_lift(g, att); char_set_class_level(att, 100);), false, 1},
    {"plainly", "status", false, SETUP(), false, 1},
    {"traits", "status traits", false, SETUP(), false, 1},
    {"TRAITS", "status TRAITS", false, SETUP(), false, 1},
    {"something else", "status luck", false, SETUP(), false, 1},
    {"traits, with some", "status traits", false, SETUP(char_set_bonuses(att, BONUS_THRIFTY, 1); char_set_bonuses(att, BONUS_GMEMORY, 1); char_set_bonuses(att, BONUS_SOFT, 1); char_set_bonuses(att, BONUS_BMEMORY, 1); char_set_bonuses(att, BONUS_LEECH, 1); char_set_bonuses(att, BONUS_INTOLERANT, 1);), false, 1},
    {"traits, with only negatives", "status traits", false, SETUP(for (int i = 0; i < 52; i++) char_set_bonuses(att, i, 0); char_set_bonuses(att, BONUS_LONER, 1);), false, 1},
    {"traits, with none", "status traits", false, SETUP(for (int i = 0; i < 52; i++) char_set_bonuses(att, i, 0);), false, 1},
    {"without a head", "status", false, SETUP(char_plr_set(att, PLR_HEAD, false);), false, 1},
    {"with a head", "status", false, SETUP(char_plr_set(att, PLR_HEAD, true);), false, 1},
    {"limbs at 100", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 100);), false, 1},
    {"cybernetic limbs at 100", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 100); for (int f : {PLR_CRARM, PLR_CLARM, PLR_CRLEG, PLR_CLLEG}) char_plr_set(att, f, true);), false, 1},
    {"limbs at 50", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 50);), false, 1},
    {"cybernetic limbs at 50", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 50); for (int f : {PLR_CRARM, PLR_CLARM, PLR_CRLEG, PLR_CLLEG}) char_plr_set(att, f, true);), false, 1},
    {"limbs at 49", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 49);), false, 1},
    {"cybernetic limbs at 49", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 49); for (int f : {PLR_CRARM, PLR_CLARM, PLR_CRLEG, PLR_CLLEG}) char_plr_set(att, f, true);), false, 1},
    {"limbs at 1", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 1);), false, 1},
    {"cybernetic limbs at 1", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 1); for (int f : {PLR_CRARM, PLR_CLARM, PLR_CRLEG, PLR_CLLEG}) char_plr_set(att, f, true);), false, 1},
    {"limbs at 0", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 0);), false, 1},
    {"cybernetic limbs at 0", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 0); for (int f : {PLR_CRARM, PLR_CLARM, PLR_CRLEG, PLR_CLLEG}) char_plr_set(att, f, true);), false, 1},
    {"limbs at -5", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, -5);), false, 1},
    {"cybernetic limbs at -5", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, -5); for (int f : {PLR_CRARM, PLR_CLARM, PLR_CRLEG, PLR_CLLEG}) char_plr_set(att, f, true);), false, 1},
    {"a cybernetic left arm", "status", false, SETUP(char_plr_set(att, PLR_CLARM, true);), false, 1},
    {"a cybernetic left arm and right leg", "status", false, SETUP(char_plr_set(att, PLR_CLARM, true); char_plr_set(att, PLR_CRLEG, true);), false, 1},
    {"a cybernetic right leg", "status", false, SETUP(char_plr_set(att, PLR_CRLEG, true);), false, 1},
    {"a cybernetic left arm, limbs at 30", "status", false, SETUP(for (int i = 0; i < 4; i++) char_set_limb_condition(att, i, 30); char_plr_set(att, PLR_CLARM, true);), false, 1},
    {"as a saiyan with a tail", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_STAIL, true);), false, 1},
    {"as a saiyan without a tail", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_STAIL, false); char_plr_set(att, PLR_TAIL, false);), false, 1},
    {"as a halfbreed with a tail", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_STAIL, true);), false, 1},
    {"as a halfbreed without a tail", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_STAIL, false); char_plr_set(att, PLR_TAIL, false);), false, 1},
    {"as a icer with a tail", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TAIL, true);), false, 1},
    {"as a icer without a tail", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_STAIL, false); char_plr_set(att, PLR_TAIL, false);), false, 1},
    {"as a bio with a tail", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TAIL, true);), false, 1},
    {"as a bio without a tail", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_STAIL, false); char_plr_set(att, PLR_TAIL, false);), false, 1},
    {"as a human with a tail", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TAIL, true);), false, 1},
    {"as a human without a tail", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_STAIL, false); char_plr_set(att, PLR_TAIL, false);), false, 1},
    {"hunger and thirst at -1", "status", false, SETUP(char_set_conditions(g, att, HUNGER, -1); char_set_conditions(g, att, THIRST, -1);), false, 1},
    {"hunger and thirst at 0", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 0); char_set_conditions(g, att, THIRST, 0);), false, 1},
    {"hunger and thirst at 1", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 1); char_set_conditions(g, att, THIRST, 1);), false, 1},
    {"hunger and thirst at 2", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 2); char_set_conditions(g, att, THIRST, 2);), false, 1},
    {"hunger and thirst at 4", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 4); char_set_conditions(g, att, THIRST, 4);), false, 1},
    {"hunger and thirst at 5", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 5); char_set_conditions(g, att, THIRST, 5);), false, 1},
    {"hunger and thirst at 9", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 9); char_set_conditions(g, att, THIRST, 9);), false, 1},
    {"hunger and thirst at 10", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 10); char_set_conditions(g, att, THIRST, 10);), false, 1},
    {"hunger and thirst at 14", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 14); char_set_conditions(g, att, THIRST, 14);), false, 1},
    {"hunger and thirst at 15", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 15); char_set_conditions(g, att, THIRST, 15);), false, 1},
    {"hunger and thirst at 20", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 20); char_set_conditions(g, att, THIRST, 20);), false, 1},
    {"hunger and thirst at 21", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 21); char_set_conditions(g, att, THIRST, 21);), false, 1},
    {"hunger and thirst at 29", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 29); char_set_conditions(g, att, THIRST, 29);), false, 1},
    {"hunger and thirst at 30", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 30); char_set_conditions(g, att, THIRST, 30);), false, 1},
    {"hunger and thirst at 39", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 39); char_set_conditions(g, att, THIRST, 39);), false, 1},
    {"hunger and thirst at 40", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 40); char_set_conditions(g, att, THIRST, 40);), false, 1},
    {"hunger and thirst at 47", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 47); char_set_conditions(g, att, THIRST, 47);), false, 1},
    {"hunger and thirst at 48", "status", false, SETUP(char_set_conditions(g, att, HUNGER, 48); char_set_conditions(g, att, THIRST, 48);), false, 1},
    {"drunk at 0", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 0);), false, 1},
    {"drunk at 1", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 1);), false, 1},
    {"drunk at 4", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 4);), false, 1},
    {"drunk at 5", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 5);), false, 1},
    {"drunk at 10", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 10);), false, 1},
    {"drunk at 11", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 11);), false, 1},
    {"drunk at 15", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 15);), false, 1},
    {"drunk at 16", "status", false, SETUP(char_set_conditions(g, att, DRUNK, 16);), false, 1},
    {"muted, hintless, hidden", "status", false, SETUP(char_plr_set(att, PLR_NOSHOUT, true); char_prf_set(g, att, PRF_HINTS, false); char_prf_set(g, att, PRF_HIDE, true);), false, 1},
    {"with hints", "status", false, SETUP(char_prf_set(g, att, PRF_HINTS, true);), false, 1},
    {"in punishment hell", "status", false, SETUP(char_from_room(g, att); char_to_room(g, att, 9);), false, 1},
    {"behind on the news", "status", false, SETUP(g->NEWSUPDATE = 1000; char_set_lastpl(att, 1);), false, 1},
    {"with a voice", "status", false, SETUP(char_set_voice(att, "gravelly");), false, 1},
    {"most distinct by eye", "status", false, SETUP(char_set_distfea(att, DISTFEA_EYE);), false, 1},
    {"most distinct by hair", "status", false, SETUP(char_set_distfea(att, DISTFEA_HAIR);), false, 1},
    {"most distinct by skin", "status", false, SETUP(char_set_distfea(att, DISTFEA_SKIN);), false, 1},
    {"most distinct by height", "status", false, SETUP(char_set_distfea(att, DISTFEA_HEIGHT);), false, 1},
    {"most distinct by weight", "status", false, SETUP(char_set_distfea(att, DISTFEA_WEIGHT);), false, 1},
    {"most distinct by hair, as a demon", "status", false, SETUP(char_set_distfea(att, DISTFEA_HAIR); char_set_race(att, RACE_DEMON);), false, 1},
    {"most distinct by hair, as a majin", "status", false, SETUP(char_set_distfea(att, DISTFEA_HAIR); char_set_race(att, RACE_MAJIN);), false, 1},
    {"most distinct by hair, as a icer", "status", false, SETUP(char_set_distfea(att, DISTFEA_HAIR); char_set_race(att, RACE_ICER);), false, 1},
    {"most distinct by hair, as a namek", "status", false, SETUP(char_set_distfea(att, DISTFEA_HAIR); char_set_race(att, RACE_NAMEK);), false, 1},
    {"most distinct by hair, as a human", "status", false, SETUP(char_set_distfea(att, DISTFEA_HAIR); char_set_race(att, RACE_HUMAN);), false, 1},
    {"preferring 0", "status", false, SETUP(char_set_preference(att, 0);), false, 1},
    {"preferring 1", "status", false, SETUP(char_set_preference(att, 1);), false, 1},
    {"preferring 2", "status", false, SETUP(char_set_preference(att, 2);), false, 1},
    {"preferring 3", "status", false, SETUP(char_set_preference(att, 3);), false, 1},
    {"preferring 4", "status", false, SETUP(char_set_preference(att, 4);), false, 1},
    {"preferring 5", "status", false, SETUP(char_set_preference(att, 5);), false, 1},
    {"with a scouter", "status", false, SETUP(equip_char(g, att, read_object(g, 70), WEAR_EYE);), false, 1},
    {"with a tuned scouter", "status", false, SETUP(obj_data *s = read_object(g, 70); obj_set_scoutfreq(s, 7); equip_char(g, att, s, WEAR_EYE);), false, 1},
    {"charged, kaioken, barriered, aflame, suppressed", "status", false, SETUP(char_set_charge(att, 12345); char_set_kaioken(att, 3); char_aff_set(att, AFF_SANCTUARY, true); char_set_barrier(att, 4321); char_aff_set(att, AFF_FIRESHIELD, true); char_set_suppression(att, 50);), false, 1},
    {"uncharged", "status", false, SETUP(char_set_charge(att, 0);), false, 1},
    {"as a majin", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_absorbs(att, 3);), false, 1},
    {"as a bio-android", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_absorbs(att, 2);), false, 1},
    {"as an android", "status", false, SETUP(char_set_race(att, RACE_ANDROID);), false, 1},
    {"as an absorbing android", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_ABSORB, true);), false, 1},
    {"at level 100", "status", false, SETUP(char_set_class_level(att, 100);), false, 1},
    {"forgetting, 0 along", "status", false, SETUP(char_set_forgeting(att, SKILL_KAMEHAMEHA); char_set_forgetcount(att, 0);), false, 1},
    {"forgetting, 1 along", "status", false, SETUP(char_set_forgeting(att, SKILL_KAMEHAMEHA); char_set_forgetcount(att, 1);), false, 1},
    {"forgetting, 2 along", "status", false, SETUP(char_set_forgeting(att, SKILL_KAMEHAMEHA); char_set_forgetcount(att, 2);), false, 1},
    {"forgetting, 3 along", "status", false, SETUP(char_set_forgeting(att, SKILL_KAMEHAMEHA); char_set_forgetcount(att, 3);), false, 1},
    {"forgetting, 4 along", "status", false, SETUP(char_set_forgeting(att, SKILL_KAMEHAMEHA); char_set_forgetcount(att, 4);), false, 1},
    {"forgetting, 5 along", "status", false, SETUP(char_set_forgeting(att, SKILL_KAMEHAMEHA); char_set_forgetcount(att, 5);), false, 1},
    {"with daggers", "status", false, SETUP(char_set_skills_base(att, SKILL_DAGGER, 50);), false, 1},
    {"with daggers, cooling", "status", false, SETUP(char_set_skills_base(att, SKILL_DAGGER, 50); char_set_backstabcool(att, 3);), false, 1},
    {"with a feature and a display", "status", false, SETUP(char_set_feature(att, "a scar"); char_set_rdisplay(att, "is lounging");), false, 1},
    {"with an empty display", "status", false, SETUP(char_set_rdisplay(att, "Empty");), false, 1},
    {"as an insomniac", "status", false, SETUP(char_set_bonuses(att, BONUS_INSOMNIAC, 1);), false, 1},
    {"slept -1", "status", false, SETUP(char_set_sleeptime(att, -1);), false, 1},
    {"slept -1, asleep", "status", false, SETUP(char_set_sleeptime(att, -1); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 0", "status", false, SETUP(char_set_sleeptime(att, 0);), false, 1},
    {"slept 0, asleep", "status", false, SETUP(char_set_sleeptime(att, 0); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 1", "status", false, SETUP(char_set_sleeptime(att, 1);), false, 1},
    {"slept 1, asleep", "status", false, SETUP(char_set_sleeptime(att, 1); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 2", "status", false, SETUP(char_set_sleeptime(att, 2);), false, 1},
    {"slept 2, asleep", "status", false, SETUP(char_set_sleeptime(att, 2); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 3", "status", false, SETUP(char_set_sleeptime(att, 3);), false, 1},
    {"slept 3, asleep", "status", false, SETUP(char_set_sleeptime(att, 3); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 4", "status", false, SETUP(char_set_sleeptime(att, 4);), false, 1},
    {"slept 4, asleep", "status", false, SETUP(char_set_sleeptime(att, 4); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 5", "status", false, SETUP(char_set_sleeptime(att, 5);), false, 1},
    {"slept 5, asleep", "status", false, SETUP(char_set_sleeptime(att, 5); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 6", "status", false, SETUP(char_set_sleeptime(att, 6);), false, 1},
    {"slept 6, asleep", "status", false, SETUP(char_set_sleeptime(att, 6); char_set_position(att, POS_SLEEPING);), false, 1},
    {"slept 7", "status", false, SETUP(char_set_sleeptime(att, 7);), false, 1},
    {"slept 7, asleep", "status", false, SETUP(char_set_sleeptime(att, 7); char_set_position(att, POS_SLEEPING);), false, 1},
    {"relaxed 116", "status", false, SETUP(char_set_relax_count(att, 116);), false, 1},
    {"relaxed 117", "status", false, SETUP(char_set_relax_count(att, 117);), false, 1},
    {"relaxed 232", "status", false, SETUP(char_set_relax_count(att, 232);), false, 1},
    {"relaxed 233", "status", false, SETUP(char_set_relax_count(att, 233);), false, 1},
    {"relaxed 464", "status", false, SETUP(char_set_relax_count(att, 464);), false, 1},
    {"relaxed 465", "status", false, SETUP(char_set_relax_count(att, 465);), false, 1},
    {"mimicking", "status", false, SETUP(char_set_mimic(att, RACE_ICER + 1);), false, 1},
    {"as a mutant of genome 0", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 0); char_set_genome(att, 1, 3);), false, 1},
    {"as a bio-android of genome 0", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 0); char_set_genome(att, 1, 5);), false, 1},
    {"as a mutant of genome 1", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 1); char_set_genome(att, 1, 4);), false, 1},
    {"as a bio-android of genome 1", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 1); char_set_genome(att, 1, 6);), false, 1},
    {"as a mutant of genome 2", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 2); char_set_genome(att, 1, 5);), false, 1},
    {"as a bio-android of genome 2", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 2); char_set_genome(att, 1, 7);), false, 1},
    {"as a mutant of genome 3", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 3); char_set_genome(att, 1, 6);), false, 1},
    {"as a bio-android of genome 3", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 3); char_set_genome(att, 1, 8);), false, 1},
    {"as a mutant of genome 4", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 4); char_set_genome(att, 1, 7);), false, 1},
    {"as a bio-android of genome 4", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 4); char_set_genome(att, 1, 9);), false, 1},
    {"as a mutant of genome 5", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 5); char_set_genome(att, 1, 8);), false, 1},
    {"as a bio-android of genome 5", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 5); char_set_genome(att, 1, 0);), false, 1},
    {"as a mutant of genome 6", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 6); char_set_genome(att, 1, 9);), false, 1},
    {"as a bio-android of genome 6", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 6); char_set_genome(att, 1, 1);), false, 1},
    {"as a mutant of genome 7", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 7); char_set_genome(att, 1, 10);), false, 1},
    {"as a bio-android of genome 7", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 7); char_set_genome(att, 1, 2);), false, 1},
    {"as a mutant of genome 8", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 8); char_set_genome(att, 1, 11);), false, 1},
    {"as a bio-android of genome 8", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 8); char_set_genome(att, 1, 3);), false, 1},
    {"as a mutant of genome 9", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 9); char_set_genome(att, 1, 0);), false, 1},
    {"as a bio-android of genome 9", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 9); char_set_genome(att, 1, 4);), false, 1},
    {"as a mutant of genome 10", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 10); char_set_genome(att, 1, 1);), false, 1},
    {"as a bio-android of genome 10", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 10); char_set_genome(att, 1, 5);), false, 1},
    {"as a mutant of genome 11", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 11); char_set_genome(att, 1, 2);), false, 1},
    {"as a bio-android of genome 11", "status", false, SETUP(char_set_race(att, RACE_BIO); char_set_genome(att, 0, 11); char_set_genome(att, 1, 6);), false, 1},
    {"not parrying", "status", false, SETUP(char_prf_set(g, att, PRF_NOPARRY, true);), false, 1},
    {"in position dead", "status", false, SETUP(char_set_position(att, POS_DEAD);), false, 1},
    {"in position mortallyw", "status", false, SETUP(char_set_position(att, POS_MORTALLYW);), false, 1},
    {"in position incap", "status", false, SETUP(char_set_position(att, POS_INCAP);), false, 1},
    {"in position stunned", "status", false, SETUP(char_set_position(att, POS_STUNNED);), false, 1},
    {"in position sleeping", "status", false, SETUP(char_set_position(att, POS_SLEEPING);), false, 1},
    {"in position resting", "status", false, SETUP(char_set_position(att, POS_RESTING);), false, 1},
    {"in position sitting", "status", false, SETUP(char_set_position(att, POS_SITTING);), false, 1},
    {"in position fighting", "status", false, SETUP(char_set_position(att, POS_FIGHTING);), false, 1},
    {"in position standing", "status", false, SETUP(char_set_position(att, POS_STANDING);), false, 1},
    {"fighting someone", "status", true, SETUP(), false, 1},
    {"in a group", "status", false, SETUP(group(g, att, victim); char_set_combatexpertise(att, 1234);), false, 1},
    {"with plr eyec", "status", false, SETUP(char_plr_set(att, PLR_EYEC, true);), false, 1},
    {"with aff sneak", "status", false, SETUP(char_aff_set(att, AFF_SNEAK, true);), false, 1},
    {"with plr disguised", "status", false, SETUP(char_plr_set(att, PLR_DISGUISED, true);), false, 1},
    {"with aff flying", "status", false, SETUP(char_aff_set(att, AFF_FLYING, true);), false, 1},
    {"with plr piloting", "status", false, SETUP(char_plr_set(att, PLR_PILOTING, true);), false, 1},
    {"with aff zanzoken", "status", false, SETUP(char_aff_set(att, AFF_ZANZOKEN, true);), false, 1},
    {"with aff hass", "status", false, SETUP(char_aff_set(att, AFF_HASS, true);), false, 1},
    {"with aff infuse", "status", false, SETUP(char_aff_set(att, AFF_INFUSE, true);), false, 1},
    {"with plr tailhide", "status", false, SETUP(char_plr_set(att, PLR_TAILHIDE, true);), false, 1},
    {"with plr nogrow", "status", false, SETUP(char_plr_set(att, PLR_NOGROW, true);), false, 1},
    {"with aff knocked", "status", false, SETUP(char_aff_set(att, AFF_KNOCKED, true);), false, 1},
    {"with aff invisible", "status", false, SETUP(char_aff_set(att, AFF_INVISIBLE, true);), false, 1},
    {"with aff detect_invis", "status", false, SETUP(char_aff_set(att, AFF_DETECT_INVIS, true);), false, 1},
    {"with aff mbreak", "status", false, SETUP(char_aff_set(att, AFF_MBREAK, true);), false, 1},
    {"with aff wither", "status", false, SETUP(char_aff_set(att, AFF_WITHER, true);), false, 1},
    {"with aff shocked", "status", false, SETUP(char_aff_set(att, AFF_SHOCKED, true);), false, 1},
    {"with aff charm", "status", false, SETUP(char_aff_set(att, AFF_CHARM, true);), false, 1},
    {"with aff infravision", "status", false, SETUP(char_aff_set(att, AFF_INFRAVISION, true);), false, 1},
    {"with aff detect_align", "status", false, SETUP(char_aff_set(att, AFF_DETECT_ALIGN, true);), false, 1},
    {"with aff detect_magic", "status", false, SETUP(char_aff_set(att, AFF_DETECT_MAGIC, true);), false, 1},
    {"with aff spirit", "status", false, SETUP(char_aff_set(att, AFF_SPIRIT, true);), false, 1},
    {"with aff ethereal", "status", false, SETUP(char_aff_set(att, AFF_ETHEREAL, true);), false, 1},
    {"with plr fishing", "status", false, SETUP(char_plr_set(att, PLR_FISHING, true);), false, 1},
    {"with plr auralight", "status", false, SETUP(char_plr_set(att, PLR_AURALIGHT, true);), false, 1},
    {"summonable and refusing gifts", "status", false, SETUP(char_prf_set(g, att, PRF_SUMMONABLE, true); char_prf_set(g, att, PRF_NOGIVE, true);), false, 1},
    {"posing and zapped", "status", false, SETUP(assign_affect(g, att, AFF_SPECIAL_POSE, SKILL_POSE, 3, 0, 0, 0, 0, 0, 0); assign_affect(g, att, AFF_HYDROZAP, SKILL_KAMEHAMEHA, 3, 0, 0, 0, 0, 0, 0);), false, 1},
    {"mage armored", "status", false, SETUP(assign_affect(g, att, 0, SPELL_MAGE_ARMOR, 3, 0, 0, 0, 0, 0, 0);), false, 1},
    {"playing a song", "status", false, SETUP(char_set_powerattack(att, 2);), false, 1},
    {"augmented", "status", false, SETUP(char_set_regen(att, 10); char_set_asb(att, 5); char_set_raw_lifebonus(att, 20); char_set_accuracy(att, 7);), false, 1},
    {"diminished", "status", false, SETUP(char_set_regen(att, -10); char_set_asb(att, -5); char_set_raw_lifebonus(att, -20);), false, 1},
    {"under runic", "status", false, SETUP(lasting(g, att, "runic");), false, 1},
    {"under punch", "status", false, SETUP(lasting(g, att, "punch");), false, 1},
    {"under knee", "status", false, SETUP(lasting(g, att, "knee");), false, 1},
    {"under slam", "status", false, SETUP(lasting(g, att, "slam");), false, 1},
    {"under heeldrop", "status", false, SETUP(lasting(g, att, "heeldrop");), false, 1},
    {"under special beam cannon", "status", false, SETUP(lasting(g, att, "special beam cannon");), false, 1},
    {"under might", "status", false, SETUP(lasting(g, att, "might");), false, 1},
    {"under flex", "status", false, SETUP(lasting(g, att, "flex");), false, 1},
    {"under bless", "status", false, SETUP(lasting(g, att, "bless");), false, 1},
    {"under curse", "status", false, SETUP(lasting(g, att, "curse");), false, 1},
    {"under healing glow", "status", false, SETUP(lasting(g, att, "healing glow");), false, 1},
    {"under genius", "status", false, SETUP(lasting(g, att, "genius");), false, 1},
    {"under enlighten", "status", false, SETUP(lasting(g, att, "enlighten");), false, 1},
    {"under yoikominminken", "status", false, SETUP(lasting(g, att, "yoikominminken");), false, 1},
    {"under solar flare", "status", false, SETUP(lasting(g, att, "solar flare");), false, 1},
    {"under spirit control", "status", false, SETUP(lasting(g, att, "spirit control");), false, 1},
    {"under !UNUSED!", "status", false, SETUP(lasting(g, att, "!UNUSED!");), false, 1},
    {"under tough skin", "status", false, SETUP(lasting(g, att, "tough skin");), false, 1},
    {"under poison", "status", false, SETUP(lasting(g, att, "poison");), false, 1},
    {"under warp pool", "status", false, SETUP(lasting(g, att, "warp pool");), false, 1},
    {"under dark metamorphosis", "status", false, SETUP(lasting(g, att, "dark metamorphosis");), false, 1},
    {"under hayasa", "status", false, SETUP(lasting(g, att, "hayasa");), false, 1},
    {"under kamehameha", "status", false, SETUP(lasting(g, att, "kamehameha");), false, 1},
    {"under might twice and flex", "status", false, SETUP(lasting(g, att, "might"); lasting(g, att, "might"); lasting(g, att, "flex"); lasting(g, att, "might");), false, 1},
    {"aura 3, died twice", "status", false, SETUP(char_set_aura(att, 3); char_set_dcount(att, 2);), false, 1},
    {"off", "brief", false, SETUP(), false, 1},
    {"on", "brief", false, SETUP(char_prf_set(g, att, PRF_BRIEF, true);), false, 1},
    {"off", "compact", false, SETUP(), false, 1},
    {"on", "compact", false, SETUP(char_prf_set(g, att, PRF_COMPACT, true);), false, 1},
    {"off", "notell", false, SETUP(), false, 1},
    {"on", "notell", false, SETUP(char_prf_set(g, att, PRF_NOTELL, true);), false, 1},
    {"off", "nonewbie", false, SETUP(), false, 1},
    {"on", "nonewbie", false, SETUP(char_prf_set(g, att, PRF_NOAUCT, true);), false, 1},
    {"off", "noshout", false, SETUP(), false, 1},
    {"on", "noshout", false, SETUP(char_prf_set(g, att, PRF_DEAF, true);), false, 1},
    {"off", "noooc", false, SETUP(), false, 1},
    {"on", "noooc", false, SETUP(char_prf_set(g, att, PRF_NOGOSS, true);), false, 1},
    {"off", "nograts", false, SETUP(), false, 1},
    {"on", "nograts", false, SETUP(char_prf_set(g, att, PRF_NOGRATZ, true);), false, 1},
    {"off", "nosummon", false, SETUP(), false, 1},
    {"on", "nosummon", false, SETUP(char_prf_set(g, att, PRF_SUMMONABLE, true);), false, 1},
    {"off", "autogold", false, SETUP(), false, 1},
    {"on", "autogold", false, SETUP(char_prf_set(g, att, PRF_AUTOGOLD, true);), false, 1},
    {"off", "autoloot", false, SETUP(), false, 1},
    {"on", "autoloot", false, SETUP(char_prf_set(g, att, PRF_AUTOLOOT, true);), false, 1},
    {"off", "carve", false, SETUP(), false, 1},
    {"on", "carve", false, SETUP(char_prf_set(g, att, PRF_CARVE, true);), false, 1},
    {"off", "health", false, SETUP(), false, 1},
    {"on", "health", false, SETUP(char_prf_set(g, att, PRF_GHEALTH, true);), false, 1},
    {"off", "hints", false, SETUP(), false, 1},
    {"on", "hints", false, SETUP(char_prf_set(g, att, PRF_HINTS, true);), false, 1},
    {"off", "ihealth", false, SETUP(), false, 1},
    {"on", "ihealth", false, SETUP(char_prf_set(g, att, PRF_IHEALTH, true);), false, 1},
    {"off", "instruct", false, SETUP(), false, 1},
    {"on", "instruct", false, SETUP(char_prf_set(g, att, PRF_INSTRUCT, true);), false, 1},
    {"off", "lkeep", false, SETUP(), false, 1},
    {"on", "lkeep", false, SETUP(char_prf_set(g, att, PRF_LKEEP, true);), false, 1},
    {"off", "noeq", false, SETUP(), false, 1},
    {"on", "noeq", false, SETUP(char_prf_set(g, att, PRF_NOEQSEE, true);), false, 1},
    {"off", "nolin", false, SETUP(), false, 1},
    {"on", "nolin", false, SETUP(char_prf_set(g, att, PRF_NODEC, true);), false, 1},
    {"off", "nomusic", false, SETUP(), false, 1},
    {"on", "nomusic", false, SETUP(char_prf_set(g, att, PRF_NOMUSIC, true);), false, 1},
    {"off", "nogive", false, SETUP(), false, 1},
    {"on", "nogive", false, SETUP(char_prf_set(g, att, PRF_NOGIVE, true);), false, 1},
    {"off", "nomail", false, SETUP(), false, 1},
    {"on", "nomail", false, SETUP(char_prf_set(g, att, PRF_NMWARN, true);), false, 1},
    {"off", "noparry", false, SETUP(), false, 1},
    {"on", "noparry", false, SETUP(char_prf_set(g, att, PRF_NOPARRY, true);), false, 1},
    {"off", "norepeat", false, SETUP(), false, 1},
    {"on", "norepeat", false, SETUP(char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"off", "vieworder", false, SETUP(), false, 1},
    {"on", "vieworder", false, SETUP(char_prf_set(g, att, PRF_VIEWORDER, true);), false, 1},
    {"off", "whohide", false, SETUP(), false, 1},
    {"on", "whohide", false, SETUP(char_prf_set(g, att, PRF_HIDE, true);), false, 1},
    {"off, immortal", "nohassle", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "nohassle", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_NOHASSLE, true);), false, 1},
    {"off, immortal", "nowiz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "nowiz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_NOWIZ, true);), false, 1},
    {"off, immortal", "roomflags", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "roomflags", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_ROOMFLAGS, true);), false, 1},
    {"off, immortal", "holylight", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "holylight", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_HOLYLIGHT, true);), false, 1},
    {"off, immortal", "autosplit", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "autosplit", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_AUTOSPLIT, true);), false, 1},
    {"off, immortal", "clsolc", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "clsolc", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_CLS, true);), false, 1},
    {"off, immortal", "buildwalk", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"on, immortal", "buildwalk", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_BUILDWALK, true);), false, 1},
    {"mortal", "nohassle", false, SETUP(), false, 1},
    {"implementor", "slowns", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"implementor", "trackthru", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL);), false, 1},
    {"builder", "test", false, SETUP(char_set_admlevel(att, ADMLVL_BUILDER);), false, 1},
    {"builder, testing", "test", false, SETUP(char_set_admlevel(att, ADMLVL_BUILDER); char_prf_set(g, att, PRF_TEST, true);), false, 1},
    {"off", "afk", false, SETUP(), false, 1},
    {"on", "afk", false, SETUP(char_prf_set(g, att, PRF_AFK, true);), false, 1},
    {"allowed", "nocompress", false, SETUP(g->config_info.play.enable_compression = 1;), false, 1},
    {"allowed, on", "nocompress", false, SETUP(g->config_info.play.enable_compression = 1; char_prf_set(g, att, PRF_NOCOMPRESS, true);), false, 1},
    {"disabled", "nocompress", false, SETUP(g->config_info.play.enable_compression = 0;), false, 1},
    {"off", "sneak", false, SETUP(), false, 1},
    {"on", "sneak", false, SETUP(char_aff_set(att, AFF_SNEAK, true);), false, 1},
    {"charged", "hide", false, SETUP(), false, 1},
    {"charged, ki user", "hide", false, SETUP(char_set_preference(att, PREFERENCE_KI);), false, 1},
    {"slightly charged, ki user", "hide", false, SETUP(char_set_preference(att, PREFERENCE_KI); char_set_charge(att, 1); char_set_skills_base(att, SKILL_HIDE, 50);), false, 1},
    {"slightly charged", "hide", false, SETUP(char_set_preference(att, PREFERENCE_H2H); char_set_charge(att, 1); char_set_skills_base(att, SKILL_HIDE, 50);), false, 1},
    {"powering up", "hide", false, SETUP(char_set_charge(att, 0); char_plr_set(att, PLR_POWERUP, true);), false, 1},
    {"flying", "hide", false, SETUP(char_set_charge(att, 0); char_aff_set(att, AFF_FLYING, true);), false, 1},
    {"in a healing tank", "hide", false, SETUP(char_set_charge(att, 0); char_plr_set(att, PLR_HEALT, true);), false, 1},
    {"skilled", "hide", false, SETUP(char_set_charge(att, 0); char_set_skills_base(att, SKILL_HIDE, 50);), false, 1},
    {"skilled, hidden", "hide", false, SETUP(char_set_charge(att, 0); char_set_skills_base(att, SKILL_HIDE, 50); char_aff_set(att, AFF_HIDE, true);), false, 1},
    {"unskilled", "hide", false, SETUP(char_set_charge(att, 0); char_set_skills_base(att, SKILL_HIDE, 0); char_set_skill_slots(att, 200);), false, 3},
    {"unskilled, no slots", "hide", false, SETUP(char_set_charge(att, 0); char_set_skills_base(att, SKILL_HIDE, 0); char_set_skill_slots(att, 0);), false, 1},
    {"menu", "rpp", false, SETUP(), false, 1},
    {"menu at level 80", "rpp", false, SETUP(char_set_class_level(att, 80);), false, 1},
    {"off the menu", "rpp 0", false, SETUP(), false, 1},
    {"off the menu", "rpp 16", false, SETUP(), false, 1},
    {"off the menu", "rpp x", false, SETUP(), false, 1},
    {"off the menu", "rpp -3", false, SETUP(), false, 1},
    {"without RPP", "rpp 1", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"without RPP", "rpp 3", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"without RPP", "rpp 5", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"without RPP", "rpp 7", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"without RPP", "rpp 9", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"without RPP", "rpp 11", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"without RPP", "rpp 15", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"rich", "rpp 1", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 2", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 8", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 3", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"short of RPP", "rpp 4", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"short of RPP", "rpp 6", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"short of RPP", "rpp 10", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"short of RPP", "rpp 14", false, SETUP(char_set_rp(att, 0);), false, 1},
    {"no choice", "rpp 4", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 4 evil", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 4 sorta-evil", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 4 NEUTRAL", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 4 sorta-good", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 4 good", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 4 chaotic", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 5", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"no choice", "rpp 6", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 str", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 con", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 int", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 wis", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 spd", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 AGL", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 6 cha", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"capped by a trait", "rpp 6 str", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_WIMP, 1); char_set_real_abils_str(att, 70);), false, 1},
    {"trait, below 70", "rpp 6 str", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_WIMP, 1); char_set_real_abils_str(att, 69);), false, 1},
    {"at 100", "rpp 6 str", false, SETUP(char_set_rp(att, 10000); char_set_real_abils_str(att, 100); affect_total(g, att);), false, 1},
    {"capped by a trait", "rpp 6 con", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_FRAIL, 1); char_set_real_abils_con(att, 70);), false, 1},
    {"trait, below 70", "rpp 6 con", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_FRAIL, 1); char_set_real_abils_con(att, 69);), false, 1},
    {"at 100", "rpp 6 con", false, SETUP(char_set_rp(att, 10000); char_set_real_abils_con(att, 100); affect_total(g, att);), false, 1},
    {"capped by a trait", "rpp 6 int", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_DULL, 1); char_set_real_abils_intel(att, 70);), false, 1},
    {"trait, below 70", "rpp 6 int", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_DULL, 1); char_set_real_abils_intel(att, 69);), false, 1},
    {"at 100", "rpp 6 int", false, SETUP(char_set_rp(att, 10000); char_set_real_abils_intel(att, 100); affect_total(g, att);), false, 1},
    {"capped by a trait", "rpp 6 wis", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_FOOLISH, 1); char_set_real_abils_wis(att, 70);), false, 1},
    {"trait, below 70", "rpp 6 wis", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_FOOLISH, 1); char_set_real_abils_wis(att, 69);), false, 1},
    {"at 100", "rpp 6 wis", false, SETUP(char_set_rp(att, 10000); char_set_real_abils_wis(att, 100); affect_total(g, att);), false, 1},
    {"capped by a trait", "rpp 6 spd", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_SLOW, 1); char_set_real_abils_cha(att, 70);), false, 1},
    {"trait, below 70", "rpp 6 spd", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_SLOW, 1); char_set_real_abils_cha(att, 69);), false, 1},
    {"at 100", "rpp 6 spd", false, SETUP(char_set_rp(att, 10000); char_set_real_abils_cha(att, 100); affect_total(g, att);), false, 1},
    {"capped by a trait", "rpp 6 agl", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_CLUMSY, 1); char_set_real_abils_dex(att, 70);), false, 1},
    {"trait, below 70", "rpp 6 agl", false, SETUP(char_set_rp(att, 10000); char_set_bonuses(att, BONUS_CLUMSY, 1); char_set_real_abils_dex(att, 69);), false, 1},
    {"at 100", "rpp 6 agl", false, SETUP(char_set_rp(att, 10000); char_set_real_abils_dex(att, 100); affect_total(g, att);), false, 1},
    {"rich", "rpp 7", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 9", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich, level 80", "rpp 9", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 80);), false, 1},
    {"level 100", "rpp 9", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 100);), false, 1},
    {"past the next level", "rpp 9", false, SETUP(char_set_rp(att, 10000); char_set_exp(att, level_exp(att, char_get_level(att) + 1) + 1);), false, 1},
    {"level 80, 2 RPP", "rpp 9", false, SETUP(char_set_class_level(att, 80); char_set_rp(att, 2);), false, 1},
    {"no choice", "rpp 10", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 white", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 blue", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 red", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 green", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 pink", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 purple", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 yellow", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 black", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 Orange", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 10 grey", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 11", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"level 100", "rpp 11", false, SETUP(char_set_rp(att, 10000); char_set_class_level(att, 100);), false, 1},
    {"arlian", "rpp 11", false, SETUP(char_set_rp(att, 10000); char_set_race(att, RACE_ARLIAN);), false, 1},
    {"above the softcap", "rpp 11", false, SETUP(char_set_rp(att, 10000); char_set_basepl(att, 1000000000000LL); char_set_baseki(att, 1000000000000LL); char_set_basest(att, 1000000000000LL);), false, 1},
    {"above only in pl", "rpp 11", false, SETUP(char_set_rp(att, 10000); char_set_basepl(att, 1000000000000LL);), false, 1},
    {"rich", "rpp 12", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 12 0", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 12 x", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 12 1", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 12 99", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 13", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 13 something", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"nothing named", "rpp 14 item70", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"rich", "rpp 14", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"carrying it", "rpp 14 item70", false, SETUP(char_set_rp(att, 10000); obj_to_char(g, read_object(g, 70), att);), false, 1},
    {"carrying a custom one", "rpp 14 item70", false, SETUP(char_set_rp(att, 10000); obj_data *o = read_object(g, 70); obj_extra_set(o, ITEM_CUSTOM, true); obj_to_char(g, o, att);), false, 1},
    {"rich", "rpp 15", false, SETUP(char_set_rp(att, 10000); ), false, 1},
    {"at 60 slots", "rpp 15", false, SETUP(char_set_rp(att, 10000); char_set_skill_slots(att, 60);), false, 1},
    {"at 60 slots, good memory", "rpp 15", false, SETUP(char_set_rp(att, 10000); char_set_skill_slots(att, 60); char_set_bonuses(att, BONUS_GMEMORY, 1);), false, 1},
    {"at 65 slots, good memory", "rpp 15", false, SETUP(char_set_rp(att, 10000); char_set_skill_slots(att, 65); char_set_bonuses(att, BONUS_GMEMORY, 1);), false, 1},
};
#undef SETUP
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
  for (int i = 0; i < (int)(sizeof(scenes) / sizeof(*scenes)); i++) {
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
