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
/* ... with Shenron summoned where they stand, played by the victim */
static void before_shenron(GameWorld *g, struct char_data *att, struct char_data *dragon, int wishes = 0)
{
  g->SHENRON      = true;
  g->DRAGONR      = char_get_in_room(att);
  g->EDRAGON_uid  = char_get_id(dragon);
  g->WISH[0]      = wishes > 0;
  g->WISH[1]      = wishes > 1;
}

/* ... and every player there dead, but `alive` */
static void the_dead(GameWorld *g, struct char_data *att, const char *alive = "")
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (!char_is_npc(c))
      char_aff_set(c, AFF_SPIRIT, strcmp(char_get_name(c), alive) != 0);
}

/* ... with the first other player there snooping them, and `then` done to that snooper */
template <class Then>
static void snooped(GameWorld *g, struct char_data *att, Then then)
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (c != att && char_get_desc(c) && char_get_desc(att) && !char_get_desc(att)->snoop_by) {
      char_get_desc(c)->snooping   = char_get_desc(att);
      char_get_desc(att)->snoop_by = char_get_desc(c);
      then(c);
    }
}

/* ... with an exit from room 301 in `dir` to `to` */
static void exit_to(GameWorld *g, int dir, room_vnum to, const char *description = "")
{
  auto *exit                = new room_direction_data();
  exit->to_room             = to;
  exit->general_description = description;
  room_set_dir_option(g, ARENA, dir, exit);
}

/* ... carrying something standing in for gardening gear (or a plant) of this vnum */
/* Gives obj the vnum of something the fixture world doesn't have, with an index and a bare prototype made for it. */
static void renumber(GameWorld *g, struct obj_data *obj, obj_vnum vnum)
{
  struct index_data *index = g->obj_index.find(vnum);
  if (!index) {
    index       = g->obj_index.emplace(vnum);
    index->vnum = vnum;
    obj_set_item_number(g->obj_proto.emplace(vnum), vnum);
  }
  obj_index_of(g, obj_get_item_number(obj))->number--;
  index->number++;
  obj_set_item_number(obj, vnum);
}

static struct obj_data *carries_as(GameWorld *g, struct char_data *att, obj_vnum vnum, const char *name = "item70 tool")
{
  struct obj_data *obj = read_object(g, 70);
  renumber(g, obj, vnum);
  obj_set_name(obj, name);
  obj_set_weight(obj, 1);
  obj_to_char(g, obj, att);
  return obj;
}

/* ... a gardener of this skill, in a garden of the arena, by a seedling of this vnum planted there */
static struct obj_data *gardening(GameWorld *g, struct char_data *att, int skill, obj_vnum seedling = 250)
{
  char_set_skills_base(att, SKILL_GARDENING, skill);
  room_set_flag(g, ARENA, ROOM_GARDEN1, true);
  if (seedling < 0)
    return nullptr;
  struct obj_data *plant = read_object(g, 70);
  renumber(g, plant, seedling);
  obj_set_name(plant, "seedling");
  obj_set_type_flag(plant, ITEM_PLANT);
  obj_to_room(g, plant, ARENA);
  return plant;
}

/* ... with `count` more plants growing there */
static void planters(GameWorld *g, int count)
{
  for (int i = 0; i < count; i++) {
    struct obj_data *plant = read_object(g, 70);
    obj_set_type_flag(plant, ITEM_PLANT);
    obj_to_room(g, plant, ARENA);
  }
}

/* ... with a toplist file of this many places, or none */
static void toplist_of(int places)
{
  remove("intro/P-T/toplist.itr");
  if (places < 0)
    return;
  FILE *fl = fopen("intro/P-T/toplist.itr", "w");
  for (int x = 0; fl && x < places; x++)
    fprintf(fl, "Name%d %d\n", x, (x + 1) * 100);
  if (fl)
    fclose(fl);
}

/* ... another player there, by name */
static struct char_data *player_named(GameWorld *g, struct char_data *att, const char *name)
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (!char_is_npc(c) && !strcmp(char_get_name(c), name))
      return c;
  return nullptr;
}

/* ... and everyone else there, players and not */
template <class Each>
static void others(GameWorld *g, struct char_data *att, Each each)
{
  for (char_data *c : room_people(g, char_get_in_room(att)))
    if (c != att)
      each(c);
}

/* ... holding item70 under another name, of another type */
static struct obj_data *holding(GameWorld *g, struct char_data *att, const char *name, int type = -1)
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_name(obj, name);
  obj_set_weight(obj, 1);
  if (type >= 0)
    obj_set_type_flag(obj, type);
  obj_to_char(g, obj, att);
  return obj;
}

/* ... sitting at an official duel table (vnum 604), its own table (600) in the room, with a case of cards */
static struct obj_data *at_duel_table(GameWorld *g, struct char_data *att, int cards)
{
  struct obj_data *seat = read_object(g, 70);
  renumber(g, seat, 604);
  obj_to_room(g, seat, char_get_in_room(att));
  char_set_sits(att, seat);
  obj_set_sitting(seat, att);
  char_set_position(att, POS_SITTING);
  struct obj_data *table = read_object(g, 70);
  renumber(g, table, 600);
  obj_set_name(table, "table");
  obj_to_room(g, table, char_get_in_room(att));
  struct obj_data *box = holding(g, att, "case");
  for (int i = 0; i < cards; i++) {
    struct obj_data *card = read_object(g, 70);
    obj_set_name(card, "card");
    obj_set_weight(card, 0);
    obj_extra_set(card, ITEM_ANTI_HIEROPHANT, true);
    obj_to_obj(g, card, box);
  }
  return table;
}

/* ... with a help table of five entries, the first "fixture" one for level 5 */
static void restricted_help_table(GameWorld *g)
{
  g->help_table = {{"QQQ", "q\n", 0, 5}};
}

static void longer_help_table(GameWorld *g)
{
  g->help_table = {{"AAA", "a\n", 0, 0}, {"FIXTURE", "f\n", 0, 5}, {"FIXTUREB", "fb\n", 0, 0}, {"FIXTUREC", "fc\n", 0, 0}, {"ZZZ", "z\n", 0, 0}};
}

/* ... with a room in every direction from the arena, of this sector and ground effect, through exits like so */
static void surrounded(GameWorld *g, int sect, int geffect = 0, int exit_info = 0, const char *keyword = "door")
{
  static constexpr room_vnum around[NUM_OF_DIRS] = {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86};
  for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
    exit_to(g, dir, around[dir]);
    struct room_direction_data *exit = room_get_dir_option(g, ARENA, dir);
    exit->exit_info                  = exit_info;
    exit->keyword                    = keyword;
    room_set_sector_type(g, around[dir], sect);
    room_set_geffect(g, around[dir], geffect);
    /* and onward, for scanning */
    auto *far    = new room_direction_data();
    far->to_room = 300;
    room_set_dir_option(g, around[dir], dir, far);
  }
}

/* ... and a thing in the room by that name, of this type, with these values */
static struct obj_data *thing_here(GameWorld *g, const char *name, int type, std::initializer_list<std::pair<int, int>> values = {})
{
  struct obj_data *obj = read_object(g, 70);
  obj_set_name(obj, name);
  obj_to_room(g, obj, ARENA);
  obj_set_type_flag(obj, type);
  for (auto [i, v] : values)
    obj_set_value(obj, i, v);
  return obj;
}

static void described(ex_desc_list *list, const char *keyword) { list->push_back({keyword, "You see it closely.\r\n"}); }

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
    {"plainly", "say hello there", false, SETUP(), false, 1},
    {"plainly", "say hello there!", false, SETUP(), false, 1},
    {"plainly", "say hello there?", false, SETUP(), false, 1},
    {"nothing", "say", false, SETUP(), false, 1},
    {"mute", "say hello", false, SETUP(char_set_bonuses(att, BONUS_MUTE, 1);), false, 1},
    {"mute, at MUD school", "say hello", false, SETUP(char_set_bonuses(att, BONUS_MUTE, 1); char_from_room(g, att); char_to_room(g, att, 50);), false, 1},
    {"with norepeat", "say hello", false, SETUP(char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"to someone", "say to Sayla hello", false, SETUP(), false, 1},
    {"to someone", "say to Hugo hello", false, SETUP(), false, 1},
    {"to someone", "say to Victim hello", false, SETUP(), false, 1},
    {"to someone", "say to namek hello", false, SETUP(), false, 1},
    {"to someone", "say to dummy hello", false, SETUP(), false, 1},
    {"to someone", "say hello to Icor and to Blade ", false, SETUP(), false, 1},
    {"to Shenron, wishless", "say knowledge for me", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron, naming no one", "say I wish for knowledge", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron elsewhere", "say I wish myself knowledge", false, SETUP(before_shenron(g, att, victim); g->DRAGONR = 0;), false, 1},
    {"to Shenron gone", "say I wish myself knowledge", false, SETUP(before_shenron(g, att, victim); char_from_room(g, victim); char_to_room(g, victim, 302);), false, 1},
    {"to Shenron, with no dragon", "say I wish myself knowledge", false, SETUP(before_shenron(g, att, victim); g->EDRAGON_uid = -1;), false, 1},
    {"to Shenron unsummoned", "say I wish myself knowledge", false, SETUP(before_shenron(g, att, victim); g->SHENRON = false;), false, 1},
    {"to Shenron, with norepeat", "say I wish myself knowledge", false, SETUP(before_shenron(g, att, victim); char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"to Shenron", "say I wish myself knowledge", false, SETUP(before_shenron(g, att, victim); ), false, 3},
    {"to Shenron", "say I wish myself speed", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself tough", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself strength", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself intelligence", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself wisdom", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself agility", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself constitution", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself skill", false, SETUP(before_shenron(g, att, victim); ), false, 3},
    {"to Shenron", "say I wish myself power", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself money", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself immunity", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself vitality", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself senzu", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself roleplay", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself to be a mortal", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself to be immortal", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself cake", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself power and money", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron", "say I wish myself vitality and a cake", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron, second wish", "say I wish myself money", false, SETUP(before_shenron(g, att, victim, 1);), false, 1},
    {"to Shenron, second wish", "say I wish myself to be immortal", false, SETUP(before_shenron(g, att, victim, 1);), false, 1},
    {"to Shenron, wishes spent", "say I wish myself to be immortal", false, SETUP(before_shenron(g, att, victim, 2);), false, 1},
    {"to Shenron, at 95", "say I wish myself speed", false, SETUP(before_shenron(g, att, victim); char_set_real_abils_cha(att, 95);), false, 1},
    {"to Shenron, at 95", "say I wish myself strength", false, SETUP(before_shenron(g, att, victim); char_set_real_abils_str(att, 95);), false, 1},
    {"to Shenron, at 95", "say I wish myself intelligence", false, SETUP(before_shenron(g, att, victim); char_set_real_abils_intel(att, 95);), false, 1},
    {"to Shenron, at 95", "say I wish myself wisdom", false, SETUP(before_shenron(g, att, victim); char_set_real_abils_wis(att, 95);), false, 1},
    {"to Shenron, at 95", "say I wish myself agility", false, SETUP(before_shenron(g, att, victim); char_set_real_abils_dex(att, 95);), false, 1},
    {"to Shenron, at 95", "say I wish myself constitution", false, SETUP(before_shenron(g, att, victim); char_set_real_abils_con(att, 95);), false, 1},
    {"to Shenron, an immortal", "say I wish myself to be a mortal", false, SETUP(before_shenron(g, att, victim); char_plr_set(att, PLR_IMMORTAL, true);), false, 1},
    {"to Shenron, naming Sayla", "say I wish Sayla had money", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron, naming all", "say I wish Hugo, Sayla, Icor and Blade had money", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron, alive", "say I wish myself revive", false, SETUP(before_shenron(g, att, victim); ), false, 1},
    {"to Shenron, dead", "say I wish myself revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att);), false, 1},
    {"to Shenron, dead in room 0", "say I wish myself revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att); char_set_droom(att, 0);), false, 1},
    {"to Shenron, dead nowhere", "say I wish myself revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att); char_set_droom(att, 19100);), false, 1},
    {"to Shenron, all dead", "say I wish Hugo and Sayla would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att);), false, 1},
    {"to Shenron, Hugo alive", "say I wish Hugo and Sayla would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Hugo");), false, 1},
    {"to Shenron, Sayla alive", "say I wish Hugo and Sayla would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Sayla");), false, 1},
    {"to Shenron, all dead", "say I wish Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att);), false, 1},
    {"to Shenron, Icor alive", "say I wish Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Icor");), false, 1},
    {"to Shenron, Blade alive", "say I wish Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Blade");), false, 1},
    {"to Shenron, all dead", "say I wish Hugo, Sayla and Icor would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att);), false, 1},
    {"to Shenron, Hugo alive", "say I wish Hugo, Sayla and Icor would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Hugo");), false, 1},
    {"to Shenron, Sayla alive", "say I wish Hugo, Sayla and Icor would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Sayla");), false, 1},
    {"to Shenron, Icor alive", "say I wish Hugo, Sayla and Icor would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Icor");), false, 1},
    {"to Shenron, all dead", "say I wish Sayla, Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att);), false, 1},
    {"to Shenron, Sayla alive", "say I wish Sayla, Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Sayla");), false, 1},
    {"to Shenron, Icor alive", "say I wish Sayla, Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Icor");), false, 1},
    {"to Shenron, Blade alive", "say I wish Sayla, Icor and Blade would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att, "Blade");), false, 1},
    {"to Shenron, all dead in room 0", "say I wish Hugo and Sayla would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att); for (char_data *c : room_people(g, char_get_in_room(att))) if (!char_is_npc(c)) char_set_droom(c, 0);), false, 1},
    {"to Shenron, all dead nowhere", "say I wish Hugo, Sayla and Icor would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att); for (char_data *c : room_people(g, char_get_in_room(att))) if (!char_is_npc(c)) char_set_droom(c, 19100);), false, 1},
    {"to Shenron, two dead nowhere", "say I wish Hugo and Sayla would revive", false, SETUP(before_shenron(g, att, victim); the_dead(g, att); for (char_data *c : room_people(g, char_get_in_room(att))) if (!char_is_npc(c)) char_set_droom(c, 19100);), false, 1},
    {"to Shenron, Sayla at a menu", "say I wish Hugo and Sayla had money", false, SETUP(before_shenron(g, att, victim); for (char_data *c : room_people(g, char_get_in_room(att))) if (c != att && !strcmp(char_get_name(c), "Sayla") && char_get_desc(c)) desc_set_connected(char_get_desc(c), CON_POBJ);), false, 1},
    {"implementor", "game", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"immortal", "game", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"implementor", "game zones", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game zones .", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game zones 3", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game zones 77", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game zones abc", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game player", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game player Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game player Nobody", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game rent", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game rent Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game stats", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game errors", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game death", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game godrooms", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game shops", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game houses", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game snoop", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game assemblies", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game guilds", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game levels", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game uniques", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game uniques 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game uniques 1-100", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game uniques 5-", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game uniques x", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game affect", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game affect Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game affect Nobody", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game affectv", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game affectv Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game n", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game nothing", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game xyz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game z", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game a", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "game aff", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"immortal", "game errors", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game player Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game rent Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game snoop", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game uniques", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game affect", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game stats", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"immortal", "game zones", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"implementor, affected", "game affect", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); lasting(g, att, "might"); lasting(g, att, "flex");), false, 1},
    {"implementor, affected", "game affectv Hugo", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); lasting(g, att, "might"); lasting(g, att, "zanzoken");), false, 1},
    {"implementor, with a snoop", "game snoop", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); snooped(g, att, [](char_data *) {});), false, 1},
    {"greater god, snooped by a higher one", "game snoop", false, SETUP(char_set_admlevel(att, ADMLVL_GRGOD); snooped(g, att, [](char_data *c) { char_set_admlevel(c, ADMLVL_IMPL); });), false, 1},
    {"greater god, snooped by an unseen one", "game snoop", false, SETUP(char_set_admlevel(att, ADMLVL_GRGOD); snooped(g, att, [&](char_data *c) { char_set_admlevel(c, ADMLVL_GRGOD); char_set_invis_level(g, c, ADMLVL_IMPL); });), false, 1},
    {"implementor, snooped from a menu", "game snoop", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); snooped(g, att, [](char_data *c) { desc_set_connected(char_get_desc(c), CON_POBJ); });), false, 1},
    {"implementor, with errant exits", "game errors", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); exit_to(g, 10, 0); exit_to(g, 11, NOWHERE); exit_to(g, 9, NOWHERE, "A wall.");), false, 1},
    {"implementor, with a death trap", "game death", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, 5, ROOM_DEATH, true);), false, 1},
    {"implementor, with a godroom", "game godrooms", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, 5, ROOM_GODROOM, true); room_set_flag(g, 6, ROOM_GODROOM, true);), false, 1},
    {"implementor, multiclassing", "game player Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); g->config_info.advance.allow_multiclass = 1;), false, 1},
    {"with only a restricted entry", "help qqq", false, SETUP(restricted_help_table(g);), false, 1},
    {"implementor, with modifiers", "game affect Hugo", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); affected_type af{}; af.type = 1; af.duration = 3; af.modifier = 2; af.location = APPLY_STR; affect_to_char(g, att, &af); af.bitvector = AFF_FLYING; affect_to_char(g, att, &af); af.modifier = 0; affect_to_char(g, att, &af);), false, 1},
    {"mimicking a namek", "mimic", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); char_set_mimic(att, RACE_NAMEK);), false, 1},
    {"mimicking a namek", "mimic stop", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); char_set_mimic(att, RACE_NAMEK);), false, 1},
    {"mimicking a namek", "mimic namek", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); char_set_mimic(att, RACE_NAMEK);), false, 1},
    {"mimicking badly", "mimic saiyan", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 1);), false, 3},
    {"mimicking, kiless", "mimic saiyan", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 1); decCurKI(g, att, getCurKI(g, att), 0);), false, 1},
    {"big namek, twice", "kyodaika", false, SETUP(char_set_race(att, RACE_NAMEK); th_type(g, att, "kyodaika");), false, 1},
    {"at another table", "shuffle", false, SETUP(obj_data *seat = read_object(g, 70); obj_to_room(g, seat, ARENA); char_set_sits(att, seat); obj_set_sitting(seat, att); char_set_position(att, POS_SITTING);), false, 1},
    {"at another table", "play card", false, SETUP(obj_data *seat = read_object(g, 70); obj_to_room(g, seat, ARENA); char_set_sits(att, seat); obj_set_sitting(seat, att); char_set_position(att, POS_SITTING);), false, 1},
    {"at a duel table without a case", "shuffle", false, SETUP(at_duel_table(g, att, 0); extract_obj(g, find_first(char_inventory(g, att), [&](obj_data *o) { return isname("case", obj_get_name(o)); }));), false, 1},
    {"at a duel table, cards and a stone", "shuffle", false, SETUP(at_duel_table(g, att, 3); obj_data *box = find_first(char_inventory(g, att), [&](obj_data *o) { return isname("case", obj_get_name(o)); }); obj_to_obj(g, read_object(g, 70), box);), false, 2},
    {"with a note, in a field", "post note", false, SETUP(holding(g, att, "note", ITEM_NOTE); room_set_sector_type(g, ARENA, SECT_FIELD);), false, 1},
    {"by a ship", "nickname ship @rSwift", false, SETUP(obj_data *ship = read_object(g, 70); renumber(g, ship, 45000); obj_to_room(g, ship, ARENA);), false, 1},
    {"holding a sword", "showoff sword namek", false, SETUP(holding(g, att, "sword");), false, 1},
    {"plainly", "dub namek Pal", false, SETUP(), false, 1},
    {"plainly", "dub self Pal", false, SETUP(), false, 1},
    {"having dubbed", "dub namek Bob", false, SETUP(th_type(g, att, "dub namek Pal");), false, 1},
    {"at level 1", "consider namek", false, SETUP(char_set_class_level(att, 1); char_set_class_level(victim, 10);), false, 1},
    {"at level 4", "consider namek", false, SETUP(char_set_class_level(att, 4); char_set_class_level(victim, 10);), false, 1},
    {"at level 6", "consider namek", false, SETUP(char_set_class_level(att, 6); char_set_class_level(victim, 10);), false, 1},
    {"at level 8", "consider namek", false, SETUP(char_set_class_level(att, 8); char_set_class_level(victim, 10);), false, 1},
    {"at level 9", "consider namek", false, SETUP(char_set_class_level(att, 9); char_set_class_level(victim, 10);), false, 1},
    {"at level 10", "consider namek", false, SETUP(char_set_class_level(att, 10); char_set_class_level(victim, 10);), false, 1},
    {"at level 11", "consider namek", false, SETUP(char_set_class_level(att, 11); char_set_class_level(victim, 10);), false, 1},
    {"at level 12", "consider namek", false, SETUP(char_set_class_level(att, 12); char_set_class_level(victim, 10);), false, 1},
    {"at level 13", "consider namek", false, SETUP(char_set_class_level(att, 13); char_set_class_level(victim, 10);), false, 1},
    {"at level 15", "consider namek", false, SETUP(char_set_class_level(att, 15); char_set_class_level(victim, 10);), false, 1},
    {"at level 20", "consider namek", false, SETUP(char_set_class_level(att, 20); char_set_class_level(victim, 10);), false, 1},
    {"at level 40", "consider namek", false, SETUP(char_set_class_level(att, 40); char_set_class_level(victim, 10);), false, 1},
    {"at level 100", "consider namek", false, SETUP(char_set_class_level(att, 100); char_set_class_level(victim, 10);), false, 1},
    {"below the victim", "commands namek", false, SETUP(char_set_class_level(att, 1); char_set_class_level(victim, 100);), false, 1},
    {"plainly", "commands namek", false, SETUP(), false, 1},
    {"autoexit off", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, false);), false, 1},
    {"immortal, amid secret openings", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_SECRET);), false, 1},
    {"by a window onto the dark", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}}); room_set_flag(g, 300, ROOM_DARK, true);), false, 1},
    {"by a window onto a vehicle", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, 5000}, {VAL_WINDOW_UNUSED4, -1}}); obj_data *v = read_object(g, 70); renumber(g, v, 5000); obj_set_type_flag(v, ITEM_VEHICLE); obj_to_room(g, v, 300);), false, 1},
    {"with a way inside", "look inside", false, SETUP(exit_to(g, INDIR, 300);), false, 1},
    {"with a way inside", "search inside", false, SETUP(exit_to(g, INDIR, 300);), false, 1},
    {"plainly", "search towards north", false, SETUP(exit_to(g, NORTH, 300);), false, 1},
    {"north undescribed, without 17", "look north", false, SETUP(exit_to(g, NORTH, 300); for (obj_data *o : char_inventory(g, att)) if (obj_get_vnum(g, o) == 17) extract_obj(g, o);), false, 1},
    {"a keen searcher, by buried things", "search", false, SETUP(char_set_skills_base(att, SKILL_SEARCH, 100); char_set_skills_base(att, SKILL_SPOT, 100); char_set_skills_base(att, SKILL_LISTEN, 100); for (int i = 0; i < 4; i++) obj_extra_set(thing_here(g, "thing", ITEM_OTHER), ITEM_BURIED, true);), false, 3},
    {"amid fields, lava beyond", "scan", false, SETUP(surrounded(g, SECT_FIELD); room_set_geffect(g, 300, 3);), false, 1},
    {"amid fields, much lava beyond", "scan", false, SETUP(surrounded(g, SECT_FIELD); room_set_geffect(g, 300, 7);), false, 1},
    {"a master of all", "perfect masenko 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect galik 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect psychic 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect twin 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect honoo 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect flash 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect deathbeam 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect crusher 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect dodonpa 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect spikes 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"a master of all", "perfect starnova 1", false, SETUP(for (int i = 400; i < 560; i++) char_set_skills_base(att, i, 100);), false, 1},
    {"master", "perfect kam 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100);), false, 1},
    {"implementor, with a snoop of another", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_data *a = nullptr; char_data *b = nullptr; others(g, att, [&](char_data *c) { if (char_is_npc(c) || !char_get_desc(c)) return; if (!a) a = c; else if (!b) b = c; }); if (a && b && !char_get_desc(b)->snoop_by) { char_get_desc(a)->snooping = char_get_desc(b); char_get_desc(b)->snoop_by = char_get_desc(a); }), false, 1},
    {"holy, among the invisible", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_prf_set(g, att, PRF_HOLYLIGHT, true); others(g, att, [&](char_data *c) { if (!char_is_npc(c)) char_aff_set(c, AFF_INVISIBLE, true); });), false, 1},
    {"implementor, with a hostless one", "users", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); others(g, att, [&](char_data *c) { if (!char_is_npc(c) && char_get_desc(c)) char_get_desc(c)->host[0] = 0; });), false, 1},
    {"immortal, with others elsewhere", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); others(g, att, [&](char_data *c) { if (!char_is_npc(c)) { char_from_room(g, c); char_to_room(g, c, 5); } });), false, 1},
    {"immortal, with others away", "where namek", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); char_from_room(g, victim); char_to_room(g, victim, 5);), false, 1},
    {"immortal", "where Blade", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT);), false, 1},
    {"implementor, with a lost thing", "where item70", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); read_object(g, 70);), false, 1},
    {"hair length 0", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 0);), false, 1},
    {"hair length 1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 1);), false, 1},
    {"hair length 2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 2);), false, 1},
    {"hair length 3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 3);), false, 1},
    {"hair length 4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 4);), false, 1},
    {"hair style 0", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 0);), false, 1},
    {"hair style 1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 1);), false, 1},
    {"hair style 2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 2);), false, 1},
    {"hair style 3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 3);), false, 1},
    {"hair style 4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 4);), false, 1},
    {"hair style 5", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 5);), false, 1},
    {"hair style 6", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 6);), false, 1},
    {"hair style 7", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 7);), false, 1},
    {"hair style 8", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 8);), false, 1},
    {"hair style 9", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 9);), false, 1},
    {"hair style 10", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 10);), false, 1},
    {"hair style 11", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 11);), false, 1},
    {"hair colour 0", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 0);), false, 1},
    {"hair colour 1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 1);), false, 1},
    {"hair colour 2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 2);), false, 1},
    {"hair colour 3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 3);), false, 1},
    {"hair colour 4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 4);), false, 1},
    {"hair colour 5", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 5);), false, 1},
    {"hair colour 6", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 6);), false, 1},
    {"hair colour 7", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 7);), false, 1},
    {"hair colour 8", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 8);), false, 1},
    {"hair colour 9", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 9);), false, 1},
    {"hair colour 10", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 10);), false, 1},
    {"hair colour 11", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 11);), false, 1},
    {"hair colour 12", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 12);), false, 1},
    {"hair colour 13", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairc(att, 13);), false, 1},
    {"an arlian woman, wings 0", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 0);), false, 1},
    {"an arlian woman, wings 1", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 1);), false, 1},
    {"an arlian woman, wings 2", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 2);), false, 1},
    {"an arlian woman, wings 3", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 3);), false, 1},
    {"an arlian woman, wings 4", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 4);), false, 1},
    {"an arlian woman, wings 5", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 5);), false, 1},
    {"an arlian woman, wings 6", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 6);), false, 1},
    {"an arlian woman, wings 7", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 7);), false, 1},
    {"an arlian woman, wings 8", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 8);), false, 1},
    {"an arlian woman, wings 9", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 9);), false, 1},
    {"an arlian woman, wings 10", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 10);), false, 1},
    {"an arlian woman, wings 11", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 11);), false, 1},
    {"an arlian woman, wings 12", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 12);), false, 1},
    {"an arlian woman, wings 13", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 13);), false, 1},
    {"eyes 0", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 0);), false, 1},
    {"eyes 1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 1);), false, 1},
    {"eyes 2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 2);), false, 1},
    {"eyes 3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 3);), false, 1},
    {"eyes 4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 4);), false, 1},
    {"eyes 5", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 5);), false, 1},
    {"eyes 6", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 6);), false, 1},
    {"eyes 7", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 7);), false, 1},
    {"eyes 8", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 8);), false, 1},
    {"eyes 9", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 9);), false, 1},
    {"eyes 10", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 10);), false, 1},
    {"eyes 11", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_eye(att, 11);), false, 1},
    {"skin 0", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 0);), false, 1},
    {"skin 1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 1);), false, 1},
    {"skin 2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 2);), false, 1},
    {"skin 3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 3);), false, 1},
    {"skin 4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 4);), false, 1},
    {"skin 5", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 5);), false, 1},
    {"skin 6", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 6);), false, 1},
    {"skin 7", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 7);), false, 1},
    {"skin 8", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 8);), false, 1},
    {"skin 9", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 9);), false, 1},
    {"skin 10", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 10);), false, 1},
    {"skin 11", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_skin(att, 11);), false, 1},
    {"a saiyan in PLR_TRANS1, hair 0", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true); char_set_hairl(att, 0);), false, 1},
    {"a saiyan in PLR_TRANS1, hair 1", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true); char_set_hairl(att, 1);), false, 1},
    {"a saiyan in PLR_TRANS1, hair 2", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true); char_set_hairl(att, 2);), false, 1},
    {"a saiyan in PLR_TRANS1, hair 3", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true); char_set_hairl(att, 3);), false, 1},
    {"a saiyan in PLR_TRANS1, hair 4", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true); char_set_hairl(att, 4);), false, 1},
    {"a saiyan in PLR_TRANS2, hair 0", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true); char_set_hairl(att, 0);), false, 1},
    {"a saiyan in PLR_TRANS2, hair 1", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true); char_set_hairl(att, 1);), false, 1},
    {"a saiyan in PLR_TRANS2, hair 2", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true); char_set_hairl(att, 2);), false, 1},
    {"a saiyan in PLR_TRANS2, hair 3", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true); char_set_hairl(att, 3);), false, 1},
    {"a saiyan in PLR_TRANS2, hair 4", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true); char_set_hairl(att, 4);), false, 1},
    {"disguised, skin 0, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 0);), false, 1},
    {"disguised, skin 1, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 1);), false, 1},
    {"disguised, skin 2, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 2);), false, 1},
    {"disguised, skin 3, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 3);), false, 1},
    {"disguised, skin 4, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 4);), false, 1},
    {"disguised, skin 5, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 5);), false, 1},
    {"disguised, skin 6, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 6);), false, 1},
    {"disguised, skin 7, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 7);), false, 1},
    {"disguised, skin 8, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 8);), false, 1},
    {"disguised, skin 9, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 9);), false, 1},
    {"disguised, skin 10, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 10);), false, 1},
    {"disguised, skin 11, looked at", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 11);), false, 1},
    {"amid SECT_INSIDE, 0 lava", "map", false, SETUP(surrounded(g, SECT_INSIDE, 0);), false, 1},
    {"amid SECT_INSIDE, 3 lava", "map", false, SETUP(surrounded(g, SECT_INSIDE, 3);), false, 1},
    {"amid SECT_INSIDE", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_INSIDE, 3);), false, 1},
    {"amid SECT_FIELD, 0 lava", "map", false, SETUP(surrounded(g, SECT_FIELD, 0);), false, 1},
    {"amid SECT_FIELD, 3 lava", "map", false, SETUP(surrounded(g, SECT_FIELD, 3);), false, 1},
    {"amid SECT_FIELD", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FIELD, 3);), false, 1},
    {"amid SECT_DESERT, 0 lava", "map", false, SETUP(surrounded(g, SECT_DESERT, 0);), false, 1},
    {"amid SECT_DESERT, 3 lava", "map", false, SETUP(surrounded(g, SECT_DESERT, 3);), false, 1},
    {"amid SECT_DESERT", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_DESERT, 3);), false, 1},
    {"amid SECT_CITY, 0 lava", "map", false, SETUP(surrounded(g, SECT_CITY, 0);), false, 1},
    {"amid SECT_CITY, 3 lava", "map", false, SETUP(surrounded(g, SECT_CITY, 3);), false, 1},
    {"amid SECT_CITY", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_CITY, 3);), false, 1},
    {"amid SECT_FOREST, 0 lava", "map", false, SETUP(surrounded(g, SECT_FOREST, 0);), false, 1},
    {"amid SECT_FOREST, 3 lava", "map", false, SETUP(surrounded(g, SECT_FOREST, 3);), false, 1},
    {"amid SECT_FOREST", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FOREST, 3);), false, 1},
    {"amid SECT_MOUNTAIN, 0 lava", "map", false, SETUP(surrounded(g, SECT_MOUNTAIN, 0);), false, 1},
    {"amid SECT_MOUNTAIN, 3 lava", "map", false, SETUP(surrounded(g, SECT_MOUNTAIN, 3);), false, 1},
    {"amid SECT_MOUNTAIN", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_MOUNTAIN, 3);), false, 1},
    {"amid SECT_HILLS, 0 lava", "map", false, SETUP(surrounded(g, SECT_HILLS, 0);), false, 1},
    {"amid SECT_HILLS, 3 lava", "map", false, SETUP(surrounded(g, SECT_HILLS, 3);), false, 1},
    {"amid SECT_HILLS", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_HILLS, 3);), false, 1},
    {"amid SECT_FLYING, 0 lava", "map", false, SETUP(surrounded(g, SECT_FLYING, 0);), false, 1},
    {"amid SECT_FLYING, 3 lava", "map", false, SETUP(surrounded(g, SECT_FLYING, 3);), false, 1},
    {"amid SECT_FLYING", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FLYING, 3);), false, 1},
    {"amid SECT_WATER_NOSWIM, 0 lava", "map", false, SETUP(surrounded(g, SECT_WATER_NOSWIM, 0);), false, 1},
    {"amid SECT_WATER_NOSWIM, 3 lava", "map", false, SETUP(surrounded(g, SECT_WATER_NOSWIM, 3);), false, 1},
    {"amid SECT_WATER_NOSWIM", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_WATER_NOSWIM, 3);), false, 1},
    {"amid SECT_WATER_SWIM, 0 lava", "map", false, SETUP(surrounded(g, SECT_WATER_SWIM, 0);), false, 1},
    {"amid SECT_WATER_SWIM, 3 lava", "map", false, SETUP(surrounded(g, SECT_WATER_SWIM, 3);), false, 1},
    {"amid SECT_WATER_SWIM", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_WATER_SWIM, 3);), false, 1},
    {"amid SECT_SHOP, 0 lava", "map", false, SETUP(surrounded(g, SECT_SHOP, 0);), false, 1},
    {"amid SECT_SHOP, 3 lava", "map", false, SETUP(surrounded(g, SECT_SHOP, 3);), false, 1},
    {"amid SECT_SHOP", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_SHOP, 3);), false, 1},
    {"amid SECT_IMPORTANT, 0 lava", "map", false, SETUP(surrounded(g, SECT_IMPORTANT, 0);), false, 1},
    {"amid SECT_IMPORTANT, 3 lava", "map", false, SETUP(surrounded(g, SECT_IMPORTANT, 3);), false, 1},
    {"amid SECT_IMPORTANT", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_IMPORTANT, 3);), false, 1},
    {"amid SECT_SPACE, 0 lava", "map", false, SETUP(surrounded(g, SECT_SPACE, 0);), false, 1},
    {"amid SECT_SPACE, 3 lava", "map", false, SETUP(surrounded(g, SECT_SPACE, 3);), false, 1},
    {"amid SECT_SPACE", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_SPACE, 3);), false, 1},
    {"amid SECT_UNDERWATER, 0 lava", "map", false, SETUP(surrounded(g, SECT_UNDERWATER, 0);), false, 1},
    {"amid SECT_UNDERWATER, 3 lava", "map", false, SETUP(surrounded(g, SECT_UNDERWATER, 3);), false, 1},
    {"amid SECT_UNDERWATER", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_UNDERWATER, 3);), false, 1},
    {"amid water", "map", false, SETUP(surrounded(g, SECT_FIELD, -1);), false, 1},
    {"amid closed doors", "map", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"amid closed doors", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"amid closed doors", "exits", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"immortal, amid closed doors", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"amid closed doors, nodec", "exits", false, SETUP(char_prf_set(g, att, PRF_NODEC, true); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"amid secret doors", "map", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_SECRET);), false, 1},
    {"amid secret doors", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_SECRET);), false, 1},
    {"amid secret doors", "exits", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_SECRET);), false, 1},
    {"immortal, amid secret doors", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_SECRET);), false, 1},
    {"amid secret doors, nodec", "exits", false, SETUP(char_prf_set(g, att, PRF_NODEC, true); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_SECRET);), false, 1},
    {"amid locked doors", "map", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF);), false, 1},
    {"amid locked doors", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF);), false, 1},
    {"amid locked doors", "exits", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF);), false, 1},
    {"immortal, amid locked doors", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF);), false, 1},
    {"amid locked doors, nodec", "exits", false, SETUP(char_prf_set(g, att, PRF_NODEC, true); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF);), false, 1},
    {"amid open doors", "map", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR);), false, 1},
    {"amid open doors", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FIELD, 0, EX_ISDOOR);), false, 1},
    {"amid open doors", "exits", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR);), false, 1},
    {"immortal, amid open doors", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR);), false, 1},
    {"amid open doors, nodec", "exits", false, SETUP(char_prf_set(g, att, PRF_NODEC, true); surrounded(g, SECT_FIELD, 0, EX_ISDOOR);), false, 1},
    {"amid open secret doors", "map", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_SECRET);), false, 1},
    {"amid open secret doors", "look", false, SETUP(char_prf_set(g, att, PRF_AUTOEXIT, true); char_prf_set(g, att, PRF_FULL_EXIT, false); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_SECRET);), false, 1},
    {"amid open secret doors", "exits", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_SECRET);), false, 1},
    {"immortal, amid open secret doors", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_SECRET);), false, 1},
    {"amid open secret doors, nodec", "exits", false, SETUP(char_prf_set(g, att, PRF_NODEC, true); surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_SECRET);), false, 1},
    {"immortal, amid doors called 'doors'", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR, "doors");), false, 1},
    {"immortal, amid doors called 'undefined'", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR, "undefined");), false, 1},
    {"immortal, amid doors called ''", "exits", false, SETUP(char_set_admlevel(att, 5); surrounded(g, SECT_FIELD, 0, EX_ISDOOR, "");), false, 1},
    {"amid closed doors, shown closed", "exits", false, SETUP(g->config_info.play.disp_closed_doors = 1; surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"amid closed doors, shown closed, unnamed", "exits", false, SETUP(g->config_info.play.disp_closed_doors = 1; surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED, "");), false, 1},
    {"amid dark rooms", "exits", false, SETUP(surrounded(g, SECT_FIELD); for (room_vnum r : {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86}) room_set_flag(g, r, ROOM_DARK, true);), false, 1},
    {"amid dark rooms, with a light", "exits", false, SETUP(surrounded(g, SECT_FIELD); for (room_vnum r : {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86}) room_set_flag(g, r, ROOM_DARK, true); obj_data *l = read_object(g, 70); obj_set_type_flag(l, ITEM_LIGHT); obj_set_value(l, VAL_LIGHT_HOURS, 5); equip_char(g, att, l, WEAR_EYE);), false, 1},
    {"amid dark rooms, glowing", "exits", false, SETUP(surrounded(g, SECT_FIELD); for (room_vnum r : {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86}) room_set_flag(g, r, ROOM_DARK, true); char_plr_set(att, PLR_AURALIGHT, true);), false, 1},
    {"with no exits", "exits", false, SETUP(for (int d = 0; d < NUM_OF_DIRS; d++) room_set_dir_option(g, ARENA, d, nullptr);), false, 1},
    {"blind", "exits", false, SETUP(char_aff_set(att, AFF_BLIND, true);), false, 1},
    {"eyes closed", "exits", false, SETUP(char_plr_set(att, PLR_EYEC, true);), false, 1},
    {"in the dark", "exits", false, SETUP(room_set_flag(g, ARENA, ROOM_DARK, true);), false, 1},
    {"in a house ", "exits", false, SETUP(room_set_flag(g, ARENA, ROOM_HOUSE, true);), false, 1},
    {"in a house", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_HOUSE, true);), false, 1},
    {"in a house ROOM_GARDEN1", "exits", false, SETUP(room_set_flag(g, ARENA, ROOM_HOUSE, true); room_set_flag(g, ARENA, ROOM_GARDEN1, true);), false, 1},
    {"in ROOM_GARDEN1", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_GARDEN1, true);), false, 1},
    {"in a house ROOM_GARDEN2", "exits", false, SETUP(room_set_flag(g, ARENA, ROOM_HOUSE, true); room_set_flag(g, ARENA, ROOM_GARDEN2, true);), false, 1},
    {"in ROOM_GARDEN2", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_GARDEN2, true);), false, 1},
    {"with buoys (1, 0, 0)", "exits", false, SETUP(char_set_radar1(att, ARENA); char_set_radar2(att, 0); char_set_radar3(att, 0);), false, 1},
    {"with buoys (0, 1, 0)", "exits", false, SETUP(char_set_radar1(att, 0); char_set_radar2(att, ARENA); char_set_radar3(att, 0);), false, 1},
    {"with buoys (0, 0, 1)", "exits", false, SETUP(char_set_radar1(att, 0); char_set_radar2(att, 0); char_set_radar3(att, ARENA);), false, 1},
    {"with buoys (1, 1, 0)", "exits", false, SETUP(char_set_radar1(att, ARENA); char_set_radar2(att, ARENA); char_set_radar3(att, 0);), false, 1},
    {"with buoys (1, 0, 1)", "exits", false, SETUP(char_set_radar1(att, ARENA); char_set_radar2(att, 0); char_set_radar3(att, ARENA);), false, 1},
    {"with buoys (0, 1, 1)", "exits", false, SETUP(char_set_radar1(att, 0); char_set_radar2(att, ARENA); char_set_radar3(att, ARENA);), false, 1},
    {"with buoys (1, 1, 1)", "exits", false, SETUP(char_set_radar1(att, ARENA); char_set_radar2(att, ARENA); char_set_radar3(att, ARENA);), false, 1},
    {"with all buoys in room 0", "exits", false, SETUP(char_from_room(g, att); char_to_room(g, att, 0); char_set_radar1(att, 0); char_set_radar2(att, 0); char_set_radar3(att, 0);), false, 1},
    {"plainly", "autoexit", false, SETUP(), false, 1},
    {"plainly", "autoexit off", false, SETUP(), false, 1},
    {"plainly", "autoexit normal", false, SETUP(), false, 1},
    {"plainly", "autoexit complete", false, SETUP(), false, 1},
    {"plainly", "autoexit n/a", false, SETUP(), false, 1},
    {"plainly", "autoexit xyz", false, SETUP(), false, 1},
    {"with room flags shown", "look", false, SETUP(char_prf_set(g, att, PRF_ROOMFLAGS, true);), false, 1},
    {"with room flags shown, nodec", "look", false, SETUP(char_prf_set(g, att, PRF_ROOMFLAGS, true); char_prf_set(g, att, PRF_NODEC, true);), false, 1},
    {"on ROOM_EARTH", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_EARTH, true);), false, 1},
    {"on ROOM_CERRIA", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_CERRIA, true);), false, 1},
    {"on ROOM_AETHER", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_AETHER, true);), false, 1},
    {"on ROOM_FRIGID", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_FRIGID, true);), false, 1},
    {"on ROOM_SPACE", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_SPACE, true);), false, 1},
    {"on ROOM_VEGETA", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_VEGETA, true);), false, 1},
    {"on ROOM_NAMEK", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_NAMEK, true);), false, 1},
    {"on ROOM_KONACK", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_KONACK, true);), false, 1},
    {"on ROOM_NEO", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_NEO, true);), false, 1},
    {"on ROOM_AL", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_AL, true);), false, 1},
    {"on ROOM_HELL", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_HELL, true);), false, 1},
    {"on ROOM_RHELL", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_RHELL, true);), false, 1},
    {"on ROOM_YARDRAT", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_YARDRAT, true);), false, 1},
    {"on ROOM_KANASSA", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_KANASSA, true);), false, 1},
    {"on ROOM_ARLIA", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_ARLIA, true);), false, 1},
    {"on ROOM_REGEN", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_REGEN, true);), false, 1},
    {"on ROOM_AURA", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_AURA, true);), false, 1},
    {"on ROOM_HBTC", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_HBTC, true);), false, 1},
    {"at gravity 10", "look", false, SETUP(room_set_gravity(g, ARENA, 10);), false, 1},
    {"at gravity 20", "look", false, SETUP(room_set_gravity(g, ARENA, 20);), false, 1},
    {"at gravity 30", "look", false, SETUP(room_set_gravity(g, ARENA, 30);), false, 1},
    {"at gravity 40", "look", false, SETUP(room_set_gravity(g, ARENA, 40);), false, 1},
    {"at gravity 50", "look", false, SETUP(room_set_gravity(g, ARENA, 50);), false, 1},
    {"at gravity 100", "look", false, SETUP(room_set_gravity(g, ARENA, 100);), false, 1},
    {"at gravity 200", "look", false, SETUP(room_set_gravity(g, ARENA, 200);), false, 1},
    {"at gravity 300", "look", false, SETUP(room_set_gravity(g, ARENA, 300);), false, 1},
    {"at gravity 400", "look", false, SETUP(room_set_gravity(g, ARENA, 400);), false, 1},
    {"at gravity 500", "look", false, SETUP(room_set_gravity(g, ARENA, 500);), false, 1},
    {"at gravity 1000", "look", false, SETUP(room_set_gravity(g, ARENA, 1000);), false, 1},
    {"at gravity 5000", "look", false, SETUP(room_set_gravity(g, ARENA, 5000);), false, 1},
    {"at gravity 10000", "look", false, SETUP(room_set_gravity(g, ARENA, 10000);), false, 1},
    {"at gravity 7", "look", false, SETUP(room_set_gravity(g, ARENA, 7);), false, 1},
    {"briefly", "look", false, SETUP(char_prf_set(g, att, PRF_BRIEF, true);), false, 1},
    {"briefly, in a death room", "look", false, SETUP(char_prf_set(g, att, PRF_BRIEF, true); room_set_flag(g, ARENA, ROOM_DEATH, true);), false, 1},
    {"in SECT_INSIDE damaged 1", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 1);), false, 1},
    {"in SECT_INSIDE damaged 3", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 3);), false, 1},
    {"in SECT_INSIDE damaged 5", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 5);), false, 1},
    {"in SECT_INSIDE damaged 8", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 8);), false, 1},
    {"in SECT_INSIDE damaged 15", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 15);), false, 1},
    {"in SECT_INSIDE damaged 25", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 25);), false, 1},
    {"in SECT_INSIDE damaged 40", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 40);), false, 1},
    {"in SECT_INSIDE damaged 60", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 60);), false, 1},
    {"in SECT_INSIDE damaged 90", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 90);), false, 1},
    {"in SECT_INSIDE damaged 100", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_INSIDE); room_set_dmg(g, ARENA, 100);), false, 1},
    {"in SECT_CITY damaged 1", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 1);), false, 1},
    {"in SECT_CITY damaged 3", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 3);), false, 1},
    {"in SECT_CITY damaged 5", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 5);), false, 1},
    {"in SECT_CITY damaged 8", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 8);), false, 1},
    {"in SECT_CITY damaged 15", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 15);), false, 1},
    {"in SECT_CITY damaged 25", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 25);), false, 1},
    {"in SECT_CITY damaged 40", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 40);), false, 1},
    {"in SECT_CITY damaged 60", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 60);), false, 1},
    {"in SECT_CITY damaged 90", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 90);), false, 1},
    {"in SECT_CITY damaged 100", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); room_set_dmg(g, ARENA, 100);), false, 1},
    {"in SECT_FOREST damaged 1", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 1);), false, 1},
    {"in SECT_FOREST damaged 3", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 3);), false, 1},
    {"in SECT_FOREST damaged 5", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 5);), false, 1},
    {"in SECT_FOREST damaged 8", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 8);), false, 1},
    {"in SECT_FOREST damaged 15", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 15);), false, 1},
    {"in SECT_FOREST damaged 25", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 25);), false, 1},
    {"in SECT_FOREST damaged 40", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 40);), false, 1},
    {"in SECT_FOREST damaged 60", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 60);), false, 1},
    {"in SECT_FOREST damaged 90", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 90);), false, 1},
    {"in SECT_FOREST damaged 100", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); room_set_dmg(g, ARENA, 100);), false, 1},
    {"in SECT_MOUNTAIN damaged 1", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 1);), false, 1},
    {"in SECT_MOUNTAIN damaged 3", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 3);), false, 1},
    {"in SECT_MOUNTAIN damaged 5", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 5);), false, 1},
    {"in SECT_MOUNTAIN damaged 8", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 8);), false, 1},
    {"in SECT_MOUNTAIN damaged 15", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 15);), false, 1},
    {"in SECT_MOUNTAIN damaged 25", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 25);), false, 1},
    {"in SECT_MOUNTAIN damaged 40", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 40);), false, 1},
    {"in SECT_MOUNTAIN damaged 60", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 60);), false, 1},
    {"in SECT_MOUNTAIN damaged 90", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 90);), false, 1},
    {"in SECT_MOUNTAIN damaged 100", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); room_set_dmg(g, ARENA, 100);), false, 1},
    {"in SECT_WATER_SWIM damaged 1", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 1);), false, 1},
    {"in SECT_WATER_SWIM damaged 3", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 3);), false, 1},
    {"in SECT_WATER_SWIM damaged 5", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 5);), false, 1},
    {"in SECT_WATER_SWIM damaged 8", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 8);), false, 1},
    {"in SECT_WATER_SWIM damaged 15", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 15);), false, 1},
    {"in SECT_WATER_SWIM damaged 25", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 25);), false, 1},
    {"in SECT_WATER_SWIM damaged 40", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 40);), false, 1},
    {"in SECT_WATER_SWIM damaged 60", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 60);), false, 1},
    {"in SECT_WATER_SWIM damaged 90", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 90);), false, 1},
    {"in SECT_WATER_SWIM damaged 100", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_SWIM); room_set_dmg(g, ARENA, 100);), false, 1},
    {"in SECT_FLYING damaged 1", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 1);), false, 1},
    {"in SECT_FLYING damaged 3", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 3);), false, 1},
    {"in SECT_FLYING damaged 5", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 5);), false, 1},
    {"in SECT_FLYING damaged 8", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 8);), false, 1},
    {"in SECT_FLYING damaged 15", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 15);), false, 1},
    {"in SECT_FLYING damaged 25", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 25);), false, 1},
    {"in SECT_FLYING damaged 40", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 40);), false, 1},
    {"in SECT_FLYING damaged 60", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 60);), false, 1},
    {"in SECT_FLYING damaged 90", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 90);), false, 1},
    {"in SECT_FLYING damaged 100", "look", false, SETUP(room_set_sector_type(g, ARENA, SECT_FLYING); room_set_dmg(g, ARENA, 100);), false, 1},
    {"with ground effect 3", "look", false, SETUP(room_set_geffect(g, ARENA, 3);), false, 1},
    {"with ground effect 7", "look", false, SETUP(room_set_geffect(g, ARENA, 7);), false, 1},
    {"with ground effect -1", "look", false, SETUP(room_set_geffect(g, ARENA, -1);), false, 1},
    {"nodec", "look", false, SETUP(char_prf_set(g, att, PRF_NODEC, true);), false, 1},
    {"sleeping", "look", false, SETUP(char_set_position(att, POS_SLEEPING);), false, 1},
    {"stunned", "look", false, SETUP(char_set_position(att, POS_STUNNED);), false, 1},
    {"blind", "look", false, SETUP(char_aff_set(att, AFF_BLIND, true);), false, 1},
    {"eyes closed", "look", false, SETUP(char_plr_set(att, PLR_EYEC, true);), false, 1},
    {"in the dark", "look", false, SETUP(room_set_flag(g, ARENA, ROOM_DARK, true);), false, 1},
    {"Sayla a RACE_HUMAN, look 0", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_HUMAN, look 1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_HUMAN, look 2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_HUMAN, look 3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_HUMAN, look 4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_HUMAN, look 5", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_HUMAN, colour 6", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_HUMAN, colour 8", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_HUMAN, colour 10", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_HUMAN, colour 12", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_HUMAN, colour 14", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_SAIYAN, look 0", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_SAIYAN, look 1", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_SAIYAN, look 2", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_SAIYAN, look 3", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_SAIYAN, look 4", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_SAIYAN, look 5", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_SAIYAN, colour 6", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_SAIYAN, colour 8", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_SAIYAN, colour 10", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_SAIYAN, colour 12", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_SAIYAN, colour 14", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_DEMON, look 0", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_DEMON, look 1", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_DEMON, look 2", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_DEMON, look 3", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_DEMON, look 4", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_DEMON, look 5", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_DEMON, colour 6", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_DEMON, colour 8", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_DEMON, colour 10", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_DEMON, colour 12", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_DEMON, colour 14", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_ICER, look 0", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_ICER, look 1", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_ICER, look 2", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_ICER, look 3", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_ICER, look 4", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_ICER, look 5", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_ICER, colour 6", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_ICER, colour 8", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_ICER, colour 10", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_ICER, colour 12", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_ICER, colour 14", "status", false, SETUP(char_set_race(att, RACE_ICER); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_NAMEK, look 0", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_NAMEK, look 1", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_NAMEK, look 2", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_NAMEK, look 3", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_NAMEK, look 4", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_NAMEK, look 5", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_NAMEK, colour 6", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_NAMEK, colour 8", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_NAMEK, colour 10", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_NAMEK, colour 12", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_NAMEK, colour 14", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_ARLIAN, look 0", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_ARLIAN, look 1", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_ARLIAN, look 2", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_ARLIAN, look 3", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_ARLIAN, look 4", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_ARLIAN, look 5", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_ARLIAN, colour 6", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_ARLIAN, colour 8", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_ARLIAN, colour 10", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_ARLIAN, colour 12", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_ARLIAN, colour 14", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_MAJIN, look 0", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_MAJIN, look 1", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_MAJIN, look 2", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_MAJIN, look 3", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_MAJIN, look 4", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_MAJIN, look 5", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_MAJIN, colour 6", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_MAJIN, colour 8", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_MAJIN, colour 10", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_MAJIN, colour 12", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_MAJIN, colour 14", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_ANDROID, look 0", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_ANDROID, look 1", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_ANDROID, look 2", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_ANDROID, look 3", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_ANDROID, look 4", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_ANDROID, look 5", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_ANDROID, colour 6", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_ANDROID, colour 8", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_ANDROID, colour 10", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_ANDROID, colour 12", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_ANDROID, colour 14", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_KAI, look 0", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_KAI, look 1", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_KAI, look 2", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_KAI, look 3", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_KAI, look 4", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_KAI, look 5", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_KAI, colour 6", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_KAI, colour 8", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_KAI, colour 10", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_KAI, colour 12", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_KAI, colour 14", "status", false, SETUP(char_set_race(att, RACE_KAI); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a RACE_HALFBREED, look 0", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairl(att, 0); char_set_hairs(att, 0); char_set_hairc(att, 0); char_set_eye(att, 0); char_set_skin(att, 0);), false, 1},
    {"Sayla a RACE_HALFBREED, look 1", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairl(att, 1); char_set_hairs(att, 2); char_set_hairc(att, 2); char_set_eye(att, 2); char_set_skin(att, 2);), false, 1},
    {"Sayla a RACE_HALFBREED, look 2", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairl(att, 2); char_set_hairs(att, 4); char_set_hairc(att, 4); char_set_eye(att, 4); char_set_skin(att, 4);), false, 1},
    {"Sayla a RACE_HALFBREED, look 3", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairl(att, 3); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_HALFBREED, look 4", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairl(att, 4); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_HALFBREED, look 5", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairl(att, 5); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_HALFBREED, colour 6", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairs(att, 6); char_set_hairc(att, 6); char_set_eye(att, 6); char_set_skin(att, 6);), false, 1},
    {"Sayla a RACE_HALFBREED, colour 8", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairs(att, 8); char_set_hairc(att, 8); char_set_eye(att, 8); char_set_skin(att, 8);), false, 1},
    {"Sayla a RACE_HALFBREED, colour 10", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairs(att, 10); char_set_hairc(att, 10); char_set_eye(att, 10); char_set_skin(att, 10);), false, 1},
    {"Sayla a RACE_HALFBREED, colour 12", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairs(att, 12); char_set_hairc(att, 12); char_set_eye(att, 12); char_set_skin(att, 12);), false, 1},
    {"Sayla a RACE_HALFBREED, colour 14", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_set_hairs(att, 14); char_set_hairc(att, 14); char_set_eye(att, 14); char_set_skin(att, 14);), false, 1},
    {"Sayla a saiyan in PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a saiyan in PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a saiyan in PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a saiyan in PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a saiyan in PLR_OOZARU", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_OOZARU, true);), false, 1},
    {"Sayla an arlian woman", "status", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_sex(att, SEX_FEMALE); char_set_hairc(att, 3);), false, 1},
    {"Sayla a majinized human", "status", false, SETUP(char_set_majinize(att, 1);), false, 1},
    {"Sayla disguised", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 4);), false, 1},
    {"Sayla disguised, skin 0", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 0);), false, 1},
    {"Sayla disguised, skin 1", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 1);), false, 1},
    {"Sayla disguised, skin 2", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 2);), false, 1},
    {"Sayla disguised, skin 3", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 3);), false, 1},
    {"Sayla disguised, skin 4", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 4);), false, 1},
    {"Sayla disguised, skin 5", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 5);), false, 1},
    {"Sayla disguised, skin 6", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 6);), false, 1},
    {"Sayla disguised, skin 7", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 7);), false, 1},
    {"Sayla disguised, skin 8", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 8);), false, 1},
    {"Sayla disguised, skin 9", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 9);), false, 1},
    {"Sayla disguised, skin 10", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 10);), false, 1},
    {"Sayla disguised, skin 11", "look namek", false, SETUP(char_plr_set(victim, PLR_DISGUISED, true); char_set_skin(victim, 11);), false, 1},
    {"Sayla a RACE_HUMAN ", "status", false, SETUP(char_set_race(att, RACE_HUMAN);), false, 1},
    {"Sayla a RACE_HUMAN PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_HUMAN PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_HUMAN PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_HUMAN PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_HUMAN PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_HUMAN PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_HUMAN); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_HOSHIJIN ", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN);), false, 1},
    {"Sayla a RACE_HOSHIJIN PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_HOSHIJIN PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_HOSHIJIN PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_HOSHIJIN PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_HOSHIJIN PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_HOSHIJIN PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_SAIYAN ", "status", false, SETUP(char_set_race(att, RACE_SAIYAN);), false, 1},
    {"Sayla a RACE_SAIYAN PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_SAIYAN PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_SAIYAN PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_SAIYAN PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_SAIYAN PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_SAIYAN PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_HALFBREED ", "status", false, SETUP(char_set_race(att, RACE_HALFBREED);), false, 1},
    {"Sayla a RACE_HALFBREED PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_HALFBREED PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_HALFBREED PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_HALFBREED PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_HALFBREED PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_HALFBREED PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_HALFBREED); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_NAMEK ", "status", false, SETUP(char_set_race(att, RACE_NAMEK);), false, 1},
    {"Sayla a RACE_NAMEK PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_NAMEK PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_NAMEK PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_NAMEK PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_NAMEK PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_NAMEK PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_NAMEK); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_ICER ", "status", false, SETUP(char_set_race(att, RACE_ICER);), false, 1},
    {"Sayla a RACE_ICER PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_ICER PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_ICER PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_ICER PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_ICER PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_ICER PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_ICER); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_KONATSU ", "status", false, SETUP(char_set_race(att, RACE_KONATSU);), false, 1},
    {"Sayla a RACE_KONATSU PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_KONATSU); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_KONATSU PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_KONATSU); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_KONATSU PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_KONATSU); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_KONATSU PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_KONATSU); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_KONATSU PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_KONATSU); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_KONATSU PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_KONATSU); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_MUTANT ", "status", false, SETUP(char_set_race(att, RACE_MUTANT);), false, 1},
    {"Sayla a RACE_MUTANT PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_MUTANT PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_MUTANT PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_MUTANT PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_MUTANT PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_MUTANT PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_MUTANT); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_BIO ", "status", false, SETUP(char_set_race(att, RACE_BIO);), false, 1},
    {"Sayla a RACE_BIO PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_BIO PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_BIO PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_BIO PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_BIO PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_BIO PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_BIO); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_ANDROID ", "status", false, SETUP(char_set_race(att, RACE_ANDROID);), false, 1},
    {"Sayla a RACE_ANDROID PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_ANDROID PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_ANDROID PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_ANDROID PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_ANDROID PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_ANDROID PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_MAJIN ", "status", false, SETUP(char_set_race(att, RACE_MAJIN);), false, 1},
    {"Sayla a RACE_MAJIN PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_MAJIN PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_MAJIN PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_MAJIN PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_MAJIN PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_MAJIN PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_MAJIN); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_TRUFFLE ", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE);), false, 1},
    {"Sayla a RACE_TRUFFLE PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_TRUFFLE PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_TRUFFLE PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_TRUFFLE PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_TRUFFLE PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_TRUFFLE PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_TRUFFLE); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_KAI ", "status", false, SETUP(char_set_race(att, RACE_KAI);), false, 1},
    {"Sayla a RACE_KAI PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_KAI); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_KAI PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_KAI); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_KAI PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_KAI); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_KAI PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_KAI); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_KAI PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_KAI); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_KAI PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_KAI); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a RACE_DEMON ", "status", false, SETUP(char_set_race(att, RACE_DEMON);), false, 1},
    {"Sayla a RACE_DEMON PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a RACE_DEMON PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a RACE_DEMON PLR_TRANS3", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"Sayla a RACE_DEMON PLR_TRANS4", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"Sayla a RACE_DEMON PLR_TRANS5", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"Sayla a RACE_DEMON PLR_TRANS6", "status", false, SETUP(char_set_race(att, RACE_DEMON); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"Sayla a full powered saiyan", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_TRANS1, true); char_plr_set(att, PLR_FPSSJ, true);), false, 1},
    {"Sayla a legendary saiyan", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_LSSJ, true);), false, 1},
    {"Sayla a legendary saiyan PLR_TRANS1", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_LSSJ, true); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"Sayla a legendary saiyan PLR_TRANS2", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_LSSJ, true); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"Sayla a legendary saiyan PLR_OOZARU", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_LSSJ, true); char_plr_set(att, PLR_OOZARU, true);), false, 1},
    {"Sayla a full powered legend", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_LSSJ, true); char_plr_set(att, PLR_TRANS1, true); char_plr_set(att, PLR_FPSSJ, true);), false, 1},
    {"Sayla an oozaru", "status", false, SETUP(char_set_race(att, RACE_SAIYAN); char_plr_set(att, PLR_OOZARU, true);), false, 1},
    {"Sayla a hoshijin in phase 1", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_set_starphase(att, 1);), false, 1},
    {"a hoshijin in phase 1", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_set_starphase(att, 1); char_set_mimic(att, RACE_HUMAN);), false, 1},
    {"Sayla a hoshijin in phase 2", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_set_starphase(att, 2);), false, 1},
    {"a hoshijin in phase 2", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_set_starphase(att, 2); char_set_mimic(att, RACE_HUMAN);), false, 1},
    {"Sayla a hoshijin in phase 3", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_set_starphase(att, 3);), false, 1},
    {"a hoshijin in phase 3", "status", false, SETUP(char_set_race(att, RACE_HOSHIJIN); char_set_starphase(att, 3); char_set_mimic(att, RACE_HUMAN);), false, 1},
    {"Sayla a mimicking hoshijin", "look namek", false, SETUP(char_set_race(victim, RACE_HOSHIJIN); char_set_starphase(victim, 1); char_set_mimic(victim, RACE_HUMAN);), false, 1},
    {"with nothing north", "look north", false, SETUP(room_set_dir_option(g, ARENA, NORTH, nullptr);), false, 1},
    {"north described", "look north", false, SETUP(exit_to(g, NORTH, 300, "A path.\r\n");), false, 1},
    {"north undescribed", "look north", false, SETUP(exit_to(g, NORTH, 300);), false, 1},
    {"north undescribed, holding 17", "look north", false, SETUP(exit_to(g, NORTH, 300); obj_to_char(g, read_object(g, 17), att);), false, 1},
    {"north through closed doors", "look north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_CLOSED; room_get_dir_option(g, ARENA, NORTH)->keyword = "door";), false, 1},
    {"searching north through closed doors", "search north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_CLOSED; room_get_dir_option(g, ARENA, NORTH)->keyword = "door"; char_set_skills_base(att, SKILL_SEARCH, 100);), false, 1},
    {"north through secret doors", "look north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_CLOSED | EX_SECRET; room_get_dir_option(g, ARENA, NORTH)->keyword = "door";), false, 1},
    {"searching north through secret doors", "search north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_CLOSED | EX_SECRET; room_get_dir_option(g, ARENA, NORTH)->keyword = "door"; char_set_skills_base(att, SKILL_SEARCH, 100);), false, 1},
    {"north through locked doors", "look north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF; room_get_dir_option(g, ARENA, NORTH)->keyword = "door";), false, 1},
    {"searching north through locked doors", "search north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF; room_get_dir_option(g, ARENA, NORTH)->keyword = "door"; char_set_skills_base(att, SKILL_SEARCH, 100);), false, 1},
    {"north through open doors", "look north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR; room_get_dir_option(g, ARENA, NORTH)->keyword = "door";), false, 1},
    {"searching north through open doors", "search north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR; room_get_dir_option(g, ARENA, NORTH)->keyword = "door"; char_set_skills_base(att, SKILL_SEARCH, 100);), false, 1},
    {"north through open secret doors", "look north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_SECRET; room_get_dir_option(g, ARENA, NORTH)->keyword = "door";), false, 1},
    {"searching north through open secret doors", "search north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_SECRET; room_get_dir_option(g, ARENA, NORTH)->keyword = "door"; char_set_skills_base(att, SKILL_SEARCH, 100);), false, 1},
    {"searching north, described", "search north", false, SETUP(exit_to(g, NORTH, 300, "A path.\r\n");), false, 1},
    {"searching north, unskilled human", "search north", false, SETUP(exit_to(g, NORTH, 300); room_get_dir_option(g, ARENA, NORTH)->exit_info = EX_ISDOOR | EX_SECRET; room_get_dir_option(g, ARENA, NORTH)->keyword = "door"; char_set_race(att, RACE_HUMAN);), false, 1},
    {"searching north, halfbreed", "search north", false, SETUP(exit_to(g, NORTH, 300); char_set_race(att, RACE_HALFBREED);), false, 1},
    {"searching nowhere", "search north", false, SETUP(room_set_dir_option(g, ARENA, NORTH, nullptr);), false, 1},
    {"by a portal appearing -1 to 300", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, -1}, {VAL_PORTAL_DEST, 300}});), false, 1},
    {"by a portal appearing -1 to 99999", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, -1}, {VAL_PORTAL_DEST, 99999}});), false, 1},
    {"by a portal appearing -1 to 51", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, -1}, {VAL_PORTAL_DEST, 51}}); room_set_flag(g, 51, ROOM_DARK, true);), false, 1},
    {"by a portal appearing 0 to 0", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, 0}, {VAL_PORTAL_DEST, 0}});), false, 1},
    {"by a portal appearing 3 to 0", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, 3}, {VAL_PORTAL_DEST, 0}});), false, 1},
    {"by a portal appearing 5 to 0", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, 5}, {VAL_PORTAL_DEST, 0}});), false, 1},
    {"by a portal appearing 9 to 0", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_PORTAL_APPEAR, 9}, {VAL_PORTAL_DEST, 0}});), false, 1},
    {"by a closeable portal", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_CONTAINER_FLAGS, CONT_CLOSEABLE}});), false, 1},
    {"by a closed portal", "look in portal", false, SETUP(thing_here(g, "portal", ITEM_PORTAL, {{VAL_CONTAINER_FLAGS, CONT_CLOSEABLE | CONT_CLOSED}});), false, 1},
    {"by a vehicle 0", "look in vehicle", false, SETUP(thing_here(g, "vehicle", ITEM_VEHICLE, {{VAL_VEHICLE_ROOM, 300}, {VAL_VEHICLE_APPEAR, -1}, {VAL_CONTAINER_FLAGS, 0}});), false, 1},
    {"by a vehicle 1", "look in vehicle", false, SETUP(thing_here(g, "vehicle", ITEM_VEHICLE, {{VAL_VEHICLE_ROOM, 99999}, {VAL_VEHICLE_APPEAR, -1}, {VAL_CONTAINER_FLAGS, 0}});), false, 1},
    {"by a vehicle 2", "look in vehicle", false, SETUP(thing_here(g, "vehicle", ITEM_VEHICLE, {{VAL_VEHICLE_ROOM, 51}, {VAL_VEHICLE_APPEAR, -1}, {VAL_CONTAINER_FLAGS, 0}}); room_set_flag(g, 51, ROOM_DARK, true);), false, 1},
    {"by a vehicle 3", "look in vehicle", false, SETUP(thing_here(g, "vehicle", ITEM_VEHICLE, {{VAL_VEHICLE_ROOM, 300}, {VAL_VEHICLE_APPEAR, 2}, {VAL_CONTAINER_FLAGS, 0}});), false, 1},
    {"by a vehicle 4", "look in vehicle", false, SETUP(thing_here(g, "vehicle", ITEM_VEHICLE, {{VAL_VEHICLE_ROOM, 300}, {VAL_VEHICLE_APPEAR, -1}, {VAL_CONTAINER_FLAGS, CONT_CLOSED}});), false, 1},
    {"by a window", "look in window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}});), false, 1},
    {"by a box", "look in box", false, SETUP(thing_here(g, "box", ITEM_CONTAINER);), false, 1},
    {"by a closed box", "look in box", false, SETUP(thing_here(g, "box", ITEM_CONTAINER, {{VAL_CONTAINER_FLAGS, CONT_CLOSED}});), false, 1},
    {"by a watched box", "look in box", false, SETUP(obj_data *b = thing_here(g, "box", ITEM_CONTAINER); renumber(g, b, 697);), false, 1},
    {"holding a box", "look in box", false, SETUP(holding(g, att, "box", ITEM_CONTAINER);), false, 1},
    {"wearing a box", "look in box", false, SETUP(obj_data *b = read_object(g, 70); obj_set_name(b, "box"); obj_set_type_flag(b, ITEM_CONTAINER); equip_char(g, att, b, WEAR_BACKPACK);), false, 1},
    {"holding a flask 0/0", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 0); obj_set_value(f, VAL_DRINKCON_HOWFULL, 0);), false, 1},
    {"holding a flask 5/-1", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, -1); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"holding a flask 20/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 20);), false, 1},
    {"holding a flask 10/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 10);), false, 1},
    {"holding a flask 9/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 9);), false, 1},
    {"holding a flask 6/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 6);), false, 1},
    {"holding a flask 3/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 3);), false, 1},
    {"holding a flask 1/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 1);), false, 1},
    {"holding a flask 0/10", "look in flask", false, SETUP(obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 0);), false, 1},
    {"by a fountain", "look in fountain", false, SETUP(thing_here(g, "fountain", ITEM_FOUNTAIN, {{VAL_DRINKCON_CAPACITY, 10}, {VAL_DRINKCON_HOWFULL, 10}});), false, 1},
    {"with things", "look in sword", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "look in", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "look in nothing", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "look into box", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "look onto box", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "examine box", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "examine flask", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "examine sword", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"with things", "examine", false, SETUP(holding(g, att, "sword"); holding(g, att, "box", ITEM_CONTAINER); obj_data *f = holding(g, att, "flask", ITEM_DRINKCON); obj_set_value(f, VAL_DRINKCON_CAPACITY, 10); obj_set_value(f, VAL_DRINKCON_HOWFULL, 5);), false, 1},
    {"by a window -1/-1", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, -1}});), false, 1},
    {"by a window -1/-1, outside", "look outside", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, -1}}); room_set_dir_option(g, ARENA, OUTDIR, nullptr);), false, 1},
    {"by a window -1/300", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}});), false, 1},
    {"by a window -1/300, outside", "look outside", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}}); room_set_dir_option(g, ARENA, OUTDIR, nullptr);), false, 1},
    {"by a window -1/99999", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 99999}});), false, 1},
    {"by a window -1/99999, outside", "look outside", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 99999}}); room_set_dir_option(g, ARENA, OUTDIR, nullptr);), false, 1},
    {"by a window 5/0", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, 5}, {VAL_WINDOW_UNUSED4, 0}});), false, 1},
    {"by a window 5/0, outside", "look outside", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, 5}, {VAL_WINDOW_UNUSED4, 0}}); room_set_dir_option(g, ARENA, OUTDIR, nullptr);), false, 1},
    {"by a described window", "look through window", false, SETUP(obj_data *w = thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}}); obj_set_action_description(w, "You peer out.");), false, 1},
    {"by a closed window", "look through window", false, SETUP(thing_here(g, "window", ITEM_WINDOW, {{VAL_CONTAINER_FLAGS, CONT_CLOSEABLE | CONT_CLOSED}});), false, 1},
    {"by a sword", "look through sword", false, SETUP(thing_here(g, "sword", ITEM_WEAPON);), false, 1},
    {"by nothing", "look through nothing", false, SETUP(), false, 1},
    {"outside", "look outside", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, false); room_set_sector_type(g, ARENA, SECT_FIELD); room_set_dir_option(g, ARENA, OUTDIR, nullptr);), false, 1},
    {"inside, windowless", "look outside", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, true); room_set_dir_option(g, ARENA, OUTDIR, nullptr);), false, 1},
    {"by a window, out to the default", "look outside", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, true); room_set_dir_option(g, ARENA, OUTDIR, nullptr); thing_here(g, "window", ITEM_WINDOW, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, -1}}); exit_to(g, NORTH, 300);), false, 1},
    {"by a statue", "look at statue", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look statue", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look at 2.statue", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look at 0.statue", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look around", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look at Sayla", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look at", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "search statue", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "search at statue", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look towards north", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look towards nowhere", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look inside", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "search inside", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"by a statue", "look dummy", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "statue"); ), false, 1},
    {"hidden, by a statue", "look at Sayla", false, SETUP(char_aff_set(att, AFF_HIDE, true);), false, 1},
    {"by nothing to look around at", "look around", false, SETUP(), false, 1},
    {"holding a described ITEM_WEAPON", "look at thing", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_WEAPON); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"wearing a described ITEM_WEAPON", "look at thing", false, SETUP(obj_data *o = read_object(g, 70); obj_set_name(o, "thing"); obj_set_type_flag(o, ITEM_WEAPON); described(obj_ex_description_slot(o), "thing"); equip_char(g, att, o, WEAR_EYE);), false, 1},
    {"holding something described ITEM_WEAPON", "look at feature", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_WEAPON); described(obj_ex_description_slot(o), "feature");), false, 1},
    {"holding a described ITEM_SPELLBOOK", "look at thing", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_SPELLBOOK); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"wearing a described ITEM_SPELLBOOK", "look at thing", false, SETUP(obj_data *o = read_object(g, 70); obj_set_name(o, "thing"); obj_set_type_flag(o, ITEM_SPELLBOOK); described(obj_ex_description_slot(o), "thing"); equip_char(g, att, o, WEAR_EYE);), false, 1},
    {"holding something described ITEM_SPELLBOOK", "look at feature", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_SPELLBOOK); described(obj_ex_description_slot(o), "feature");), false, 1},
    {"holding a described ITEM_SCROLL", "look at thing", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_SCROLL); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"wearing a described ITEM_SCROLL", "look at thing", false, SETUP(obj_data *o = read_object(g, 70); obj_set_name(o, "thing"); obj_set_type_flag(o, ITEM_SCROLL); described(obj_ex_description_slot(o), "thing"); equip_char(g, att, o, WEAR_EYE);), false, 1},
    {"holding something described ITEM_SCROLL", "look at feature", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_SCROLL); described(obj_ex_description_slot(o), "feature");), false, 1},
    {"holding a described ITEM_OTHER", "look at thing", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_OTHER); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"wearing a described ITEM_OTHER", "look at thing", false, SETUP(obj_data *o = read_object(g, 70); obj_set_name(o, "thing"); obj_set_type_flag(o, ITEM_OTHER); described(obj_ex_description_slot(o), "thing"); equip_char(g, att, o, WEAR_EYE);), false, 1},
    {"holding something described ITEM_OTHER", "look at feature", false, SETUP(obj_data *o = holding(g, att, "thing", ITEM_OTHER); described(obj_ex_description_slot(o), "feature");), false, 1},
    {"wearing something described", "look at feature", false, SETUP(obj_data *o = read_object(g, 70); obj_set_name(o, "thing"); described(obj_ex_description_slot(o), "feature"); equip_char(g, att, o, WEAR_EYE);), false, 1},
    {"holding a described board", "look at board", false, SETUP(obj_data *o = holding(g, att, "board", ITEM_BOARD); described(obj_ex_description_slot(o), "board");), false, 1},
    {"by a described ITEM_VEHICLE", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}, {2, 150}}); obj_set_type_flag(o, ITEM_VEHICLE); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by a described ITEM_WINDOW", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}, {2, 150}}); obj_set_type_flag(o, ITEM_WINDOW); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by a described ITEM_CONTROL", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}, {2, 150}}); obj_set_type_flag(o, ITEM_CONTROL); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by a described ITEM_WEAPON", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER, {{VAL_WEAPON_SKILL, 1}, {VAL_WINDOW_UNUSED4, 300}, {2, 150}}); obj_set_type_flag(o, ITEM_WEAPON); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by a described ITEM_BOARD", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}, {2, 150}}); obj_set_type_flag(o, ITEM_BOARD); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by a described ITEM_OTHER", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER, {{VAL_WINDOW_UNUSED1, -1}, {VAL_WINDOW_UNUSED4, 300}, {2, 150}}); obj_set_type_flag(o, ITEM_OTHER); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by a described hatch", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_OTHER); obj_set_type_flag(o, ITEM_HATCH); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by controls with 50 fuel", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_CONTROL, {{2, 50}}); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by controls with 250 fuel", "look at thing", false, SETUP(obj_data *o = thing_here(g, "thing", ITEM_CONTROL, {{2, 250}}); described(obj_ex_description_slot(o), "thing");), false, 1},
    {"by an undescribed thing", "look at thing", false, SETUP(thing_here(g, "thing", ITEM_OTHER);), false, 1},
    {"by a board", "read board", false, SETUP(thing_here(g, "board", ITEM_BOARD);), false, 1},
    {"holding a board", "read", false, SETUP(holding(g, att, "board", ITEM_BOARD);), false, 1},
    {"holding a board", "read 1", false, SETUP(holding(g, att, "board", ITEM_BOARD);), false, 1},
    {"holding a board", "read board", false, SETUP(holding(g, att, "board", ITEM_BOARD);), false, 1},
    {"holding a board", "read 2.board", false, SETUP(holding(g, att, "board", ITEM_BOARD);), false, 1},
    {"holding a board", "read xyz", false, SETUP(holding(g, att, "board", ITEM_BOARD);), false, 1},
    {"holding a board", "read 0", false, SETUP(holding(g, att, "board", ITEM_BOARD);), false, 1},
    {"without a board", "read 1", false, SETUP(), false, 1},
    {"among hiders in SECT_FOREST", "search", false, SETUP(room_set_sector_type(g, ARENA, SECT_FOREST); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 2},
    {"among hiders in SECT_SPACE", "search", false, SETUP(room_set_sector_type(g, ARENA, SECT_SPACE); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 2},
    {"among hiders in SECT_WATER_NOSWIM", "search", false, SETUP(room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 2},
    {"among hiders in SECT_MOUNTAIN", "search", false, SETUP(room_set_sector_type(g, ARENA, SECT_MOUNTAIN); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 2},
    {"among hiders in SECT_CITY", "search", false, SETUP(room_set_sector_type(g, ARENA, SECT_CITY); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 2},
    {"among hiders in space", "search", false, SETUP(room_set_flag(g, ARENA, ROOM_SPACE, true); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 1},
    {"sensing, among suppressed liquid hiders", "search", false, SETUP(char_set_skills_base(att, SKILL_SENSE, 50); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); char_aff_set(c, AFF_LIQUEFIED, true); char_set_suppression(c, 50); });), false, 1},
    {"a keen mutant, among mutant hiders", "search", false, SETUP(char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 4); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); if (!char_is_npc(c)) { char_set_race(c, RACE_MUTANT); char_set_genome(c, 0, 5); } });), false, 1},
    {"a keen searcher, among hiders", "search", false, SETUP(char_set_skills_base(att, SKILL_SEARCH, 100); char_set_skills_base(att, SKILL_SPOT, 100); others(g, att, [&](char_data *c) { char_aff_set(c, AFF_HIDE, true); });), false, 1},
    {"by buried things", "search", false, SETUP(for (int i = 0; i < 4; i++) obj_extra_set(thing_here(g, "thing", ITEM_OTHER), ITEM_BURIED, true);), false, 3},
    {"exhausted", "search", false, SETUP(decCurST(g, att, getCurST(g, att), 0);), false, 1},
    {"amid fields", "scan", false, SETUP(surrounded(g, SECT_FIELD, 3);), false, 1},
    {"amid lava", "scan", false, SETUP(surrounded(g, SECT_FIELD, 7);), false, 1},
    {"amid dark rooms", "scan", false, SETUP(surrounded(g, SECT_FIELD); for (room_vnum r : {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86}) room_set_flag(g, r, ROOM_DARK, true);), false, 1},
    {"in the dark", "scan", false, SETUP(surrounded(g, SECT_FIELD); room_set_flag(g, ARENA, ROOM_DARK, true);), false, 1},
    {"amid closed doors", "scan", false, SETUP(surrounded(g, SECT_FIELD, 0, EX_ISDOOR | EX_CLOSED);), false, 1},
    {"blind", "scan", false, SETUP(char_aff_set(att, AFF_BLIND, true);), false, 1},
    {"eyes closed", "scan", false, SETUP(char_plr_set(att, PLR_EYEC, true);), false, 1},
    {"asleep", "scan", false, SETUP(char_set_position(att, POS_SLEEPING);), false, 1},
    {"stunned", "scan", false, SETUP(char_set_position(att, POS_STUNNED);), false, 1},
    {"amid fields, the far rooms dark", "scan", false, SETUP(surrounded(g, SECT_FIELD); room_set_flag(g, 300, ROOM_DARK, true);), false, 1},
    {"amid fields, the far doors closed", "scan", false, SETUP(surrounded(g, SECT_FIELD); for (room_vnum r : {51, 52, 53, 54, 55, 56, 81, 82, 83, 84, 85, 86}) for (int d = 0; d < NUM_OF_DIRS; d++) if (auto *e = room_get_dir_option(g, r, d)) e->exit_info = EX_ISDOOR | EX_CLOSED;), false, 1},
    {"day 0", "time", false, SETUP(g->time_info.day = 0;), false, 1},
    {"day 1", "time", false, SETUP(g->time_info.day = 1;), false, 1},
    {"day 2", "time", false, SETUP(g->time_info.day = 2;), false, 1},
    {"day 3", "time", false, SETUP(g->time_info.day = 3;), false, 1},
    {"day 10", "time", false, SETUP(g->time_info.day = 10;), false, 1},
    {"day 11", "time", false, SETUP(g->time_info.day = 11;), false, 1},
    {"day 12", "time", false, SETUP(g->time_info.day = 12;), false, 1},
    {"day 20", "time", false, SETUP(g->time_info.day = 20;), false, 1},
    {"day 21", "time", false, SETUP(g->time_info.day = 21;), false, 1},
    {"day 22", "time", false, SETUP(g->time_info.day = 22;), false, 1},
    {"day 29", "time", false, SETUP(g->time_info.day = 29;), false, 1},
    {"outside, sky 0", "weather", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, false); room_set_sector_type(g, ARENA, SECT_FIELD); g->weather_info.sky = 0; g->weather_info.change = 0 - 2;), false, 1},
    {"outside, sky 1", "weather", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, false); room_set_sector_type(g, ARENA, SECT_FIELD); g->weather_info.sky = 1; g->weather_info.change = 1 - 2;), false, 1},
    {"outside, sky 2", "weather", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, false); room_set_sector_type(g, ARENA, SECT_FIELD); g->weather_info.sky = 2; g->weather_info.change = 2 - 2;), false, 1},
    {"outside, sky 3", "weather", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, false); room_set_sector_type(g, ARENA, SECT_FIELD); g->weather_info.sky = 3; g->weather_info.change = 3 - 2;), false, 1},
    {"outside, knowing weather", "weather", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, false); room_set_sector_type(g, ARENA, SECT_FIELD); SET_BIT_AR(char_get_admflags(att), ADM_KNOWWEATHER);), false, 1},
    {"inside", "weather", false, SETUP(room_set_flag(g, ARENA, ROOM_INDOORS, true);), false, 1},
    {"plainly", "help", false, SETUP(), false, 1},
    {"plainly", "help fixture", false, SETUP(), false, 1},
    {"plainly", "help FIXT", false, SETUP(), false, 1},
    {"plainly", "help fixtuer", false, SETUP(), false, 1},
    {"plainly", "help fxture", false, SETUP(), false, 1},
    {"plainly", "help zzz", false, SETUP(), false, 1},
    {"plainly", "help fixture help", false, SETUP(), false, 1},
    {"immortal", "help fixture", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"without help", "help fixture", false, SETUP(g->help_table.clear();), false, 1},
    {"with a longer table", "help fixture", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help fixture", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"with a longer table", "help fixtureb", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help fixtureb", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"with a longer table", "help fixturec", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help fixturec", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"with a longer table", "help fixturex", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help fixturex", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"with a longer table", "help aaa", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help aaa", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"with a longer table", "help zzz", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help zzz", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"with a longer table", "help fix", false, SETUP(longer_help_table(g); ), false, 1},
    {"immortal, with a longer table", "help fix", false, SETUP(longer_help_table(g); char_set_admlevel(att, 5);), false, 1},
    {"plainly", "color off", false, SETUP(), false, 1},
    {"plainly", "color on", false, SETUP(), false, 1},
    {"plainly", "color purple", false, SETUP(), false, 1},
    {"plainly", "color", false, SETUP(), false, 1},
    {"plainly", "color OFF", false, SETUP(), false, 1},
    {"immortal, wimpy", "toggle", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); char_set_wimp_level(g, att, 5);), false, 1},
    {"compressing", "toggle", false, SETUP(g->config_info.play.enable_compression = 1;), false, 1},
    {"plainly", "commands Sayla", false, SETUP(), false, 1},
    {"plainly", "commands dummy", false, SETUP(), false, 1},
    {"plainly", "commands Nobody", false, SETUP(), false, 1},
    {"plainly", "commands", false, SETUP(), false, 1},
    {"below Sayla", "commands Sayla", false, SETUP(char_set_class_level(att, 1); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 100);), false, 1},
    {"immortal", "wizhelp", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"plainly", "socials", false, SETUP(), false, 1},
    {"with a full history", "history say", false, SETUP(for (int i = 0; i < 101; i++) add_history(g, att, "a line\r\n", HIST_SAY);), false, 1},
    {"with no history", "history say", false, SETUP(), false, 1},
    {"mortal", "history xyz", false, SETUP(), false, 1},
    {"immortal", "history xyz", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"mortal", "history", false, SETUP(), false, 1},
    {"plainly", "who", false, SETUP(), false, 1},
    {"implementor", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"among hiders", "who", false, SETUP(others(g, att, [&](char_data *c) { if (!char_is_npc(c)) char_prf_set(g, c, PRF_HIDE, true); });), false, 1},
    {"one among hiders", "who", false, SETUP(bool first = true; others(g, att, [&](char_data *c) { if (!char_is_npc(c)) { char_prf_set(g, c, PRF_HIDE, !first); first = false; } });), false, 1},
    {"immortal among hiders", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); others(g, att, [&](char_data *c) { if (!char_is_npc(c)) char_prf_set(g, c, PRF_HIDE, true); });), false, 1},
    {"blind", "who", false, SETUP(char_aff_set(att, AFF_BLIND, true);), false, 1},
    {"alone in sight", "who", false, SETUP(others(g, att, [&](char_data *c) { if (!char_is_npc(c)) char_aff_set(c, AFF_INVISIBLE, true); });), false, 1},
    {"among the flagged", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); others(g, att, [&](char_data *c) { if (char_is_npc(c)) return; char_prf_set(g, c, PRF_DEAF, true); char_prf_set(g, c, PRF_NOTELL, true); char_prf_set(g, c, PRF_NOGOSS, true); char_prf_set(g, c, PRF_HIDE, true); char_prf_set(g, c, PRF_BUILDWALK, true); char_prf_set(g, c, PRF_AFK, true); char_prf_set(g, c, PRF_NOWIZ, true); char_plr_set(c, PLR_NOSHOUT, true); char_plr_set(c, PLR_FISHING, true); char_plr_set(c, PLR_MAILING, true); });), false, 1},
    {"among the writing", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); others(g, att, [&](char_data *c) { if (!char_is_npc(c)) char_plr_set(c, PLR_WRITING, true); });), false, 1},
    {"among the invisible", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); others(g, att, [&](char_data *c) { if (!char_is_npc(c)) { char_aff_set(c, AFF_INVISIBLE, true); char_set_invis_level(g, c, 0); } }); if (auto *s = player_named(g, att, "Sayla")) char_set_invis_level(g, s, 2);), false, 1},
    {"among immortals", "who", false, SETUP(others(g, att, [&](char_data *c) { if (!char_is_npc(c)) char_set_admlevel(c, ADMLVL_GOD); });), false, 1},
    {"by one in CON_OEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_OEDIT); done = true; } });), false, 1},
    {"by one in CON_MEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_MEDIT); done = true; } });), false, 1},
    {"by one in CON_ZEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_ZEDIT); done = true; } });), false, 1},
    {"by one in CON_SEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_SEDIT); done = true; } });), false, 1},
    {"by one in CON_REDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_REDIT); done = true; } });), false, 1},
    {"by one in CON_TEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_TEDIT); done = true; } });), false, 1},
    {"by one in CON_TRIGEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_TRIGEDIT); done = true; } });), false, 1},
    {"by one in CON_AEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_AEDIT); done = true; } });), false, 1},
    {"by one in CON_CEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_CEDIT); done = true; } });), false, 1},
    {"by one in CON_HEDIT", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); bool done = false; others(g, att, [&](char_data *c) { if (!done && !char_is_npc(c) && char_get_desc(c)) { desc_set_connected(char_get_desc(c), CON_HEDIT); done = true; } });), false, 1},
    {"implementor, with a snoop", "who", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); snooped(g, att, [](char_data *) {});), false, 1},
    {"wizlocked", "who", false, SETUP(g->circle_restrict = 50;), false, 1},
    {"locked to immortals", "who", false, SETUP(g->circle_restrict = 101;), false, 1},
    {"implementor", "users", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -o", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -k", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -p", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -d", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -l 1-50", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -n Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -h nowhere", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -x", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users foo", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "users -", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"immortal, among hiders", "users", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); others(g, att, [&](char_data *c) { if (!char_is_npc(c)) { char_prf_set(g, c, PRF_HIDE, true); char_set_invis_level(g, c, 5); } });), false, 1},
    {"implementor, with one at a menu", "users", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); if (auto *s = player_named(g, att, "Sayla"); s && char_get_desc(s)) desc_set_connected(char_get_desc(s), CON_POBJ);), false, 1},
    {"implementor, among outlaws", "users -o", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); if (auto *s = player_named(g, att, "Sayla")) char_plr_set(s, PLR_KILLER, true);), false, 1},
    {"immortal", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"implementor", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"immortal", "where Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"implementor", "where Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"immortal", "where dummy", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"implementor", "where dummy", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"immortal", "where Nobody", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); ), false, 1},
    {"implementor", "where Nobody", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "where item70", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); holding(g, att, "item70"); equip_char(g, att, read_object(g, 70), WEAR_EYE); obj_data *box = read_object(g, 70); obj_set_type_flag(box, ITEM_CONTAINER); obj_to_room(g, box, ARENA); obj_to_obj(g, read_object(g, 70), box);), false, 1},
    {"implementor", "where chest", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor, on ROOM_EARTH", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_EARTH, true);), false, 1},
    {"implementor, on ROOM_FRIGID", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_FRIGID, true);), false, 1},
    {"implementor, on ROOM_VEGETA", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_VEGETA, true);), false, 1},
    {"implementor, on ROOM_KONACK", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_KONACK, true);), false, 1},
    {"implementor, on ROOM_NAMEK", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_NAMEK, true);), false, 1},
    {"implementor, on ROOM_AETHER", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_AETHER, true);), false, 1},
    {"implementor, on ROOM_ARLIA", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_ARLIA, true);), false, 1},
    {"implementor, on ROOM_YARDRAT", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_YARDRAT, true);), false, 1},
    {"implementor, on ROOM_KANASSA", "where", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); room_set_flag(g, ARENA, ROOM_KANASSA, true);), false, 1},
    {"plainly", "whois", false, SETUP(), false, 1},
    {"plainly", "whois Sayla", false, SETUP(), false, 1},
    {"plainly", "whois Nobody", false, SETUP(), false, 1},
    {"asking of an immortal", "whois Sayla", false, SETUP(if (auto *s = player_named(g, att, "Sayla")) { char_set_admlevel(s, ADMLVL_GOD); save_char(g, s); }), false, 1},
    {"plainly", "finger", false, SETUP(), false, 1},
    {"plainly", "finger zorbexb", false, SETUP(), false, 1},
    {"plainly", "finger nobody", false, SETUP(), false, 1},
    {"arlian", "evolve", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian", "evolve pl", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian", "evolve powerlevel", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian", "evolve ki", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian", "evolve st", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian", "evolve stamina", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian", "evolve xyz", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); ), false, 1},
    {"arlian, unbased", "evolve pl", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); char_set_basepl(att, 0);), false, 1},
    {"arlian, unbased", "evolve ki", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); char_set_baseki(att, 0);), false, 1},
    {"arlian, unbased", "evolve st", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); char_set_basest(att, 0);), false, 1},
    {"arlian, poor", "evolve pl", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); char_set_moltexp(att, 0);), false, 1},
    {"arlian, unevolved", "evolve pl", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 0);), false, 1},
    {"arlian, poor", "evolve ki", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); char_set_moltexp(att, 0);), false, 1},
    {"arlian, unevolved", "evolve ki", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 0);), false, 1},
    {"arlian, poor", "evolve st", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 60); char_set_moltexp(att, 1000000000000LL); char_set_moltexp(att, 0);), false, 1},
    {"arlian, unevolved", "evolve st", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_moltlevel(att, 0);), false, 1},
    {"not an arlian", "evolve", false, SETUP(), false, 1},
    {"master", "perfect", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kamehameha", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kame 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect zzzz 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kamehameha 0", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kamehameha 4", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kamehameha 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kamehameha 2", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect kamehameha 3", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"master", "perfect first 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_base(att, SKILL_FIRST_AID, 100);), false, 1},
    {"unskilled", "perfect kamehameha 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 0);), false, 1},
    {"learning", "perfect kamehameha 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 50);), false, 1},
    {"perfected", "perfect kamehameha 1", false, SETUP(char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); char_set_skills_perf(att, SKILL_KAMEHAMEHA, 2);), false, 1},
    {"immortal", "perfect kamehameha 1", false, SETUP(char_set_admlevel(att, ADMLVL_IMMORT); char_set_skills_base(att, SKILL_KAMEHAMEHA, 100); ), false, 1},
    {"with 0", "gold", false, SETUP(char_set_gold(att, 0);), false, 1},
    {"with 1", "gold", false, SETUP(char_set_gold(att, 1);), false, 1},
    {"with 500", "gold", false, SETUP(char_set_gold(att, 500);), false, 1},
    {"plainly", "score", false, SETUP(), false, 1},
    {"plainly", "score personal", false, SETUP(), false, 1},
    {"plainly", "score Health", false, SETUP(), false, 1},
    {"plainly", "score stat", false, SETUP(), false, 1},
    {"plainly", "score other", false, SETUP(), false, 1},
    {"plainly", "score x", false, SETUP(), false, 1},
    {"android ", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); ), false, 1},
    {"android PLR_ABSORB", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_ABSORB, true);), false, 1},
    {"android PLR_REPAIR", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_REPAIR, true);), false, 1},
    {"android PLR_SENSEM", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_SENSEM, true);), false, 1},
    {"android PLR_TRANS1", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS1, true);), false, 1},
    {"android PLR_TRANS2", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS2, true);), false, 1},
    {"android PLR_TRANS3", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS3, true);), false, 1},
    {"android PLR_TRANS4", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS4, true);), false, 1},
    {"android PLR_TRANS5", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS5, true);), false, 1},
    {"android PLR_TRANS6", "score personal", false, SETUP(char_set_race(att, RACE_ANDROID); char_plr_set(att, PLR_TRANS6, true);), false, 1},
    {"arlian, at level 100", "score other", false, SETUP(char_set_race(att, RACE_ARLIAN); char_set_class_level(att, 100);), false, 1},
    {"lifeless", "score health", false, SETUP(decCurLF(g, att, getCurLF(g, att), 0);), false, 1},
    {"robbed", "inventory", false, SETUP(char_plr_set(att, PLR_STOLEN, true);), false, 1},
    {"dressed", "equipment", false, SETUP(equip_char(g, att, read_object(g, 70), WEAR_EYE); obj_data *sheath = read_object(g, 70); obj_extra_set(sheath, ITEM_SHEATH, true); obj_to_obj(g, read_object(g, 70), sheath); equip_char(g, att, sheath, WEAR_BACKPACK); obj_data *blade = read_object(g, 70); obj_extra_set(blade, ITEM_SHEATH, true); obj_to_obj(g, read_object(g, 70), blade); equip_char(g, att, blade, WEAR_WIELD1); obj_data *dim = read_object(g, 70); obj_extra_set(dim, ITEM_INVISIBLE, true); equip_char(g, att, dim, WEAR_FINGER_R);), false, 1},
    {"two-handed", "equipment", false, SETUP(equip_char(g, att, read_object(g, 70), WEAR_WIELD1); char_plr_set(att, PLR_THANDW, true);), false, 1},
    {"legless", "equipment", false, SETUP(char_plr_set(att, PLR_THANDW, true);), false, 1},
    {"at level 1", "consider Sayla", false, SETUP(char_set_class_level(att, 1); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 4", "consider Sayla", false, SETUP(char_set_class_level(att, 4); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 6", "consider Sayla", false, SETUP(char_set_class_level(att, 6); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 8", "consider Sayla", false, SETUP(char_set_class_level(att, 8); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 9", "consider Sayla", false, SETUP(char_set_class_level(att, 9); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 10", "consider Sayla", false, SETUP(char_set_class_level(att, 10); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 11", "consider Sayla", false, SETUP(char_set_class_level(att, 11); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 12", "consider Sayla", false, SETUP(char_set_class_level(att, 12); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 13", "consider Sayla", false, SETUP(char_set_class_level(att, 13); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 15", "consider Sayla", false, SETUP(char_set_class_level(att, 15); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 20", "consider Sayla", false, SETUP(char_set_class_level(att, 20); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 40", "consider Sayla", false, SETUP(char_set_class_level(att, 40); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"at level 100", "consider Sayla", false, SETUP(char_set_class_level(att, 100); if (auto *s = player_named(g, att, "Sayla")) char_set_class_level(s, 10);), false, 1},
    {"plainly", "consider me", false, SETUP(), false, 1},
    {"plainly", "consider Nobody", false, SETUP(), false, 1},
    {"plainly", "consider", false, SETUP(), false, 1},
    {"plainly", "diagnose", false, SETUP(), false, 1},
    {"plainly", "diagnose Sayla", false, SETUP(), false, 1},
    {"plainly", "diagnose Nobody", false, SETUP(), false, 1},
    {"plainly", "diagnose namek", false, SETUP(), false, 1},
    {"fighting", "diagnose", false, SETUP(), false, 1},
    {"mimicking", "mimic", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); ), false, 1},
    {"mimicking", "mimic stop", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); ), false, 1},
    {"mimicking", "mimic human", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); ), false, 1},
    {"mimicking", "mimic namek", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); ), false, 1},
    {"mimicking", "mimic saiyan", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); ), false, 1},
    {"mimicking", "mimic xyz", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); ), false, 1},
    {"already mimicking", "mimic human", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); char_set_mimic(att, RACE_HUMAN);), false, 1},
    {"already mimicking", "mimic stop", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); char_set_mimic(att, RACE_HUMAN);), false, 1},
    {"already mimicking", "mimic", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 100); char_set_mimic(att, RACE_HUMAN);), false, 1},
    {"mimicking badly", "mimic human", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 1);), false, 3},
    {"mimicking, kiless", "mimic human", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 1); decCurKI(g, att, getCurKI(g, att), 0);), false, 1},
    {"unable", "mimic human", false, SETUP(char_set_skills_base(att, SKILL_MIMIC, 0);), false, 1},
    {"namek", "kyodaika", false, SETUP(char_set_race(att, RACE_NAMEK);), false, 1},
    {"big namek", "kyodaika", false, SETUP(char_set_race(att, RACE_NAMEK); assign_affect(g, att, AFF_KYODAIKA, 0, -1, 5, 0, 0, 0, 0, -2);), false, 1},
    {"not a namek", "kyodaika", false, SETUP(), false, 1},
    {"at a duel table", "table", false, SETUP(at_duel_table(g, att, 3); obj_data *t = find_first(room_contents(g, ARENA), [&](obj_data *o) { return obj_get_vnum(g, o) == 600; }); obj_data *c = read_object(g, 70); obj_set_name(c, "card"); obj_to_obj(g, c, t);), false, 1},
    {"at a duel table", "table table", false, SETUP(at_duel_table(g, att, 3); obj_data *t = find_first(room_contents(g, ARENA), [&](obj_data *o) { return obj_get_vnum(g, o) == 600; }); obj_data *c = read_object(g, 70); obj_set_name(c, "card"); obj_to_obj(g, c, t);), false, 1},
    {"at a duel table", "table table card", false, SETUP(at_duel_table(g, att, 3); obj_data *t = find_first(room_contents(g, ARENA), [&](obj_data *o) { return obj_get_vnum(g, o) == 600; }); obj_data *c = read_object(g, 70); obj_set_name(c, "card"); obj_to_obj(g, c, t);), false, 1},
    {"at a duel table", "table nothing card", false, SETUP(at_duel_table(g, att, 3); obj_data *t = find_first(room_contents(g, ARENA), [&](obj_data *o) { return obj_get_vnum(g, o) == 600; }); obj_data *c = read_object(g, 70); obj_set_name(c, "card"); obj_to_obj(g, c, t);), false, 1},
    {"at a duel table", "table table nothing", false, SETUP(at_duel_table(g, att, 3); obj_data *t = find_first(room_contents(g, ARENA), [&](obj_data *o) { return obj_get_vnum(g, o) == 600; }); obj_data *c = read_object(g, 70); obj_set_name(c, "card"); obj_to_obj(g, c, t);), false, 1},
    {"at a duel table, 0 cards", "draw", false, SETUP(at_duel_table(g, att, 0); ), false, 1},
    {"at a duel table, 0 cards", "shuffle", false, SETUP(at_duel_table(g, att, 0); ), false, 2},
    {"at a duel table, 1 cards", "draw", false, SETUP(at_duel_table(g, att, 1); ), false, 1},
    {"at a duel table, 1 cards", "shuffle", false, SETUP(at_duel_table(g, att, 1); ), false, 2},
    {"at a duel table, 3 cards", "draw", false, SETUP(at_duel_table(g, att, 3); ), false, 1},
    {"at a duel table, 3 cards", "shuffle", false, SETUP(at_duel_table(g, att, 3); ), false, 2},
    {"not sitting", "draw", false, SETUP(), false, 1},
    {"not sitting", "shuffle", false, SETUP(), false, 1},
    {"not sitting", "play card", false, SETUP(), false, 1},
    {"at another table", "draw", false, SETUP(obj_data *seat = read_object(g, 70); obj_to_room(g, seat, ARENA); char_set_sits(att, seat); obj_set_sitting(seat, att); char_set_position(att, POS_SITTING);), false, 1},
    {"at a duel table without a case", "draw", false, SETUP(at_duel_table(g, att, 0); extract_obj(g, find_first(char_inventory(g, att), [&](obj_data *o) { return isname("case", obj_get_name(o)); }));), false, 1},
    {"holding cards", "hand", false, SETUP(for (int i = 0; i < 3; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding many cards", "hand", false, SETUP(for (int i = 0; i < 9; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding no cards", "hand", false, SETUP(), false, 1},
    {"holding cards", "hand look", false, SETUP(for (int i = 0; i < 3; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding many cards", "hand look", false, SETUP(for (int i = 0; i < 9; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding no cards", "hand look", false, SETUP(), false, 1},
    {"holding cards", "hand show", false, SETUP(for (int i = 0; i < 3; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding many cards", "hand show", false, SETUP(for (int i = 0; i < 9; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding no cards", "hand show", false, SETUP(), false, 1},
    {"holding cards", "hand xyz", false, SETUP(for (int i = 0; i < 3; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding many cards", "hand xyz", false, SETUP(for (int i = 0; i < 9; i++) { obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true); }), false, 1},
    {"holding no cards", "hand xyz", false, SETUP(), false, 1},
    {"at a duel table", "play", false, SETUP(at_duel_table(g, att, 0); obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true);), false, 1},
    {"at a duel table", "play card", false, SETUP(at_duel_table(g, att, 0); obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true);), false, 1},
    {"at a duel table", "play nothing", false, SETUP(at_duel_table(g, att, 0); obj_data *c = holding(g, att, "card"); obj_extra_set(c, ITEM_ANTI_HIEROPHANT, true);), false, 1},
    {"at a duel table without its table", "play card", false, SETUP(at_duel_table(g, att, 0); extract_obj(g, find_first(room_contents(g, ARENA), [&](obj_data *o) { return obj_get_vnum(g, o) == 600; })); holding(g, att, "card");), false, 1},
    {"standing at a duel table", "play card", false, SETUP(at_duel_table(g, att, 0); char_set_position(att, POS_STANDING);), false, 1},
    {"with a note", "post", false, SETUP(holding(g, att, "note", ITEM_NOTE); holding(g, att, "sword");), false, 1},
    {"with a note", "post note", false, SETUP(holding(g, att, "note", ITEM_NOTE); holding(g, att, "sword");), false, 1},
    {"with a note", "post nothing", false, SETUP(holding(g, att, "note", ITEM_NOTE); holding(g, att, "sword");), false, 1},
    {"with a note", "post sword", false, SETUP(holding(g, att, "note", ITEM_NOTE); holding(g, att, "sword");), false, 1},
    {"with a note", "post note chest", false, SETUP(holding(g, att, "note", ITEM_NOTE); holding(g, att, "sword");), false, 1},
    {"with a note", "post note nothing", false, SETUP(holding(g, att, "note", ITEM_NOTE); holding(g, att, "sword");), false, 1},
    {"with a note, in a city", "post note", false, SETUP(holding(g, att, "note", ITEM_NOTE); room_set_sector_type(g, ARENA, SECT_CITY);), false, 1},
    {"with a note, inside", "post note", false, SETUP(holding(g, att, "note", ITEM_NOTE); room_set_sector_type(g, ARENA, SECT_INSIDE);), false, 1},
    {"with a note, in a garden", "post note", false, SETUP(holding(g, att, "note", ITEM_NOTE); room_set_flag(g, ARENA, ROOM_GARDEN1, true);), false, 1},
    {"with a note, by a board", "post note board", false, SETUP(holding(g, att, "note", ITEM_NOTE); obj_data *b = read_object(g, 70); obj_set_name(b, "board"); obj_set_type_flag(b, ITEM_BOARD); obj_to_room(g, b, ARENA);), false, 1},
    {"with a note, by a posted chest", "post note chest", false, SETUP(holding(g, att, "note", ITEM_NOTE); obj_data *other = holding(g, att, "other", ITEM_NOTE); obj_from_char(g, other); obj_to_room(g, other, ARENA); for (obj_data *o : room_contents(g, ARENA)) if (isname("chest", obj_get_name(o))) { obj_set_posted_to(o, other); obj_set_posted_to(other, o); break; }), false, 1},
    {"holding a sword", "nickname", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "nickname sword", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "nickname sword Blade", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "nickname sword averyveryverylongnickname", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "nickname nothing Blade", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "nickname ship Swift", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "nickname ship @RSwift", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a nicknamed sword", "nickname sword Again", false, SETUP(obj_set_short_description(holding(g, att, "sword"), "a sword nicknamed (Old)");), false, 1},
    {"holding a corpse", "nickname corpse Bob", false, SETUP(holding(g, att, "corpse");), false, 1},
    {"by a ship", "nickname ship Swift", false, SETUP(obj_data *ship = read_object(g, 70); renumber(g, ship, 45000); obj_to_room(g, ship, ARENA); obj_data *inside = read_object(g, 70); renumber(g, inside, 46000); obj_to_room(g, inside, 5);), false, 1},
    {"holding a sword", "showoff", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "showoff sword", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "showoff sword Sayla", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "showoff sword Nobody", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "showoff nothing Sayla", false, SETUP(holding(g, att, "sword");), false, 1},
    {"holding a sword", "showoff sword dummy", false, SETUP(holding(g, att, "sword");), false, 1},
    {"plainly", "dub", false, SETUP(), false, 1},
    {"plainly", "dub Sayla", false, SETUP(), false, 1},
    {"plainly", "dub Sayla ab", false, SETUP(), false, 1},
    {"plainly", "dub Sayla averyveryverylongname", false, SETUP(), false, 1},
    {"plainly", "dub Sayla B@d", false, SETUP(), false, 1},
    {"plainly", "dub Nobody Pal", false, SETUP(), false, 1},
    {"plainly", "dub me Pal", false, SETUP(), false, 1},
    {"plainly", "dub dummy Pal", false, SETUP(), false, 1},
    {"plainly", "dub Sayla Pal", false, SETUP(), false, 1},
    {"with RPP", "exchange", false, SETUP(char_set_rp(att, 10);), false, 1},
    {"with RPP", "exchange zorbexb", false, SETUP(char_set_rp(att, 10);), false, 1},
    {"with RPP", "exchange zorbexb 0", false, SETUP(char_set_rp(att, 10);), false, 1},
    {"with RPP", "exchange zorbexb 5", false, SETUP(char_set_rp(att, 10);), false, 1},
    {"with RPP", "exchange zorbexb 500", false, SETUP(char_set_rp(att, 10);), false, 1},
    {"with RPP", "exchange nobody 5", false, SETUP(char_set_rp(att, 10);), false, 1},
    {"plainly", "rdisplay", false, SETUP(), false, 1},
    {"plainly", "rdisplay a fine display", false, SETUP(), false, 1},
    {"with 25 places", "toplist", false, SETUP(toplist_of(25);), false, 1},
    {"with 7 places", "toplist", false, SETUP(toplist_of(7);), false, 1},
    {"with 30 places", "toplist", false, SETUP(toplist_of(30);), false, 1},
    {"without the file", "toplist", false, SETUP(toplist_of(-1);), false, 1},
    {"unskilled", "garden", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 0); char_set_skill_slots(att, 200);), false, 2},
    {"unskilled, no slots", "garden", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 0); char_set_skill_slots(att, 0);), false, 1},
    {"skilled", "garden", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); ), false, 1},
    {"skilled", "garden seedling", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); ), false, 1},
    {"skilled", "garden COLLECT", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); ), false, 1},
    {"skilled, with a shovel", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); carries_as(g, att, 254);), false, 1},
    {"skilled, with a broken shovel, in a forest", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_FOREST); obj_extra_set(carries_as(g, att, 254), ITEM_BROKEN, true);), false, 1},
    {"skilled, with a forged shovel, in a forest", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_FOREST); obj_extra_set(carries_as(g, att, 254), ITEM_FORGED, true);), false, 1},
    {"skilled, with a shovel, SECT_FOREST", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_FOREST); carries_as(g, att, 254);), false, 2},
    {"skilled, with a shovel, SECT_FIELD", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_FIELD); carries_as(g, att, 254);), false, 2},
    {"skilled, with a shovel, SECT_MOUNTAIN", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_MOUNTAIN); carries_as(g, att, 254);), false, 2},
    {"skilled, with a shovel, SECT_HILLS", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_HILLS); carries_as(g, att, 254);), false, 2},
    {"skilled, with a shovel, on fertile ground", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_FOREST); carries_as(g, att, 254); room_set_flag(g, ARENA, ROOM_FERTILE1, true);), false, 1},
    {"skilled, with a shovel, on good ground", "garden collect", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); room_set_sector_type(g, ARENA, SECT_FOREST); carries_as(g, att, 254); room_set_flag(g, ARENA, ROOM_FERTILE2, true);), false, 2},
    {"skilled, out of a garden", "garden seedling water", false, SETUP(char_set_skills_base(att, SKILL_GARDENING, 50); ), false, 1},
    {"in a garden, no plant", "garden seedling water", false, SETUP(gardening(g, att, 50, -1);), false, 1},
    {"in a garden, no plant", "garden seedling plant", false, SETUP(gardening(g, att, 50, -1);), false, 1},
    {"in the other garden", "garden seedling prune", false, SETUP(gardening(g, att, 50); room_set_flag(g, ARENA, ROOM_GARDEN1, false); room_set_flag(g, ARENA, ROOM_GARDEN2, true);), false, 1},
    {"in a garden, exhausted", "garden seedling water", false, SETUP(gardening(g, att, 50); decCurST(g, att, getCurST(g, att), 0);), false, 1},
    {"in a garden", "garden seedling prune", false, SETUP(gardening(g, att, 50);), false, 1},
    {"in a garden, no water", "garden seedling water", false, SETUP(gardening(g, att, 100);), false, 1},
    {"in a garden, forged water", "garden seedling water", false, SETUP(gardening(g, att, 100); obj_extra_set(carries_as(g, att, 251), ITEM_FORGED, true);), false, 1},
    {"expert, plant full", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 100), VAL_WATERLEVEL, 500); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant full", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, 500); carries_as(g, att, 251); ), false, 1},
    {"expert, plant dead", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 100), VAL_WATERLEVEL, -10); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant dead", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, -10); carries_as(g, att, 251); ), false, 1},
    {"expert, plant dry", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 100), VAL_WATERLEVEL, 0); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant dry", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, 0); carries_as(g, att, 251); ), false, 1},
    {"expert, plant at 275", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 100), VAL_WATERLEVEL, 275); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant at 275", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, 275); carries_as(g, att, 251); ), false, 1},
    {"expert, plant at 300", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 100), VAL_WATERLEVEL, 300); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant at 300", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, 300); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant at 460", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, 460); carries_as(g, att, 251); ), false, 1},
    {"clumsy, plant at 480", "garden seedling water", false, SETUP(obj_set_value(gardening(g, att, 1), VAL_WATERLEVEL, 480); carries_as(g, att, 251); ), false, 1},
    {"expert, no clippers", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } ), false, 1},
    {"expert, broken clippers", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } obj_extra_set(carries_as(g, att, 253), ITEM_BROKEN, true);), false, 1},
    {"expert, unharvestable", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 70); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } carries_as(g, att, 253); ), false, 1},
    {"expert, dead plant", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); obj_set_value(p, VAL_WATERLEVEL, -10);} carries_as(g, att, 253); ), false, 1},
    {"expert, young plant", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); obj_set_value(p, VAL_MATURITY, 2);} carries_as(g, att, 253); ), false, 1},
    {"clumsy", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 1, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } carries_as(g, att, 253); ), false, 1},
    {"expert, dull clippers", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } obj_set_value(carries_as(g, att, 253), VAL_ALL_HEALTH, 1);), false, 1},
    {"expert", "garden seedling harvest", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } obj_set_value(carries_as(g, att, 253), VAL_ALL_HEALTH, 5);), false, 2},
    {"expert, no shovel", "garden seedling dig", false, SETUP(gardening(g, att, 100);), false, 1},
    {"expert", "garden seedling dig", false, SETUP(gardening(g, att, 100); carries_as(g, att, 254);), false, 1},
    {"expert, no shovel", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); ), false, 1},
    {"expert, no soil", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254);), false, 1},
    {"expert, forged soil", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_extra_set(soil, ITEM_FORGED, true); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 0", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 0); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 1", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 1); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 2", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 2); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 3", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 3); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 4", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 4); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 5", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 5); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 6", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 6); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 7", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 7); obj_to_char(g, soil, att); }), false, 1},
    {"expert, soil of 8", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); { obj_data *soil = read_object(g, 255); obj_set_value(soil, 0, 8); obj_to_char(g, soil, att); }), false, 1},
    {"clumsy", "garden seedling plant", false, SETUP(gardening(g, att, 1, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); obj_to_char(g, read_object(g, 255), att);), false, 1},
    {"expert, planters full", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); carries_as(g, att, 254); obj_to_char(g, read_object(g, 255), att); room_set_flag(g, ARENA, ROOM_HOUSE, true); planters(g, 8);), false, 1},
    {"expert, big garden, 8 planted", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); room_set_flag(g, ARENA, ROOM_GARDEN1, false); room_set_flag(g, ARENA, ROOM_GARDEN2, true); carries_as(g, att, 254); obj_to_char(g, read_object(g, 255), att); room_set_flag(g, ARENA, ROOM_HOUSE, true); planters(g, 8);), false, 1},
    {"expert, big garden full", "garden seedling plant", false, SETUP(gardening(g, att, 100, -1); obj_set_type_flag(carries_as(g, att, 250, "seedling"), ITEM_PLANT); room_set_flag(g, ARENA, ROOM_GARDEN1, false); room_set_flag(g, ARENA, ROOM_GARDEN2, true); carries_as(g, att, 254); obj_to_char(g, read_object(g, 255), att); room_set_flag(g, ARENA, ROOM_HOUSE, true); planters(g, 20);), false, 1},
    {"expert, unpickable", "garden seedling pick", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); } ), false, 1},
    {"expert, young", "garden seedling pick", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); obj_extra_set(p, ITEM_MATURE, true); obj_set_value(p, VAL_MATURITY, 2);} ), false, 1},
    {"clumsy", "garden seedling pick", false, SETUP({ obj_data *p = gardening(g, att, 1, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); obj_extra_set(p, ITEM_MATURE, true);} ), false, 1},
    {"expert", "garden seedling pick", false, SETUP({ obj_data *p = gardening(g, att, 100, 250); obj_set_value(p, VAL_MATURITY, 6); obj_set_value(p, VAL_MAXMATURE, 6); obj_extra_set(p, ITEM_MATURE, true);} ), false, 1},
    {"implementor", "set", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxpl 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxpl -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxpl 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxki 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxki -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxki 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxst 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxst -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla maxst 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla pl 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla pl -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla pl 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ki 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ki -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ki 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sta 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sta -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sta 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla align 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla align -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla align 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla str -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla str 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla stradd 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla stradd -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla stradd 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla int 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla int -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla int 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla wis 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla wis -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla wis 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla dex 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla dex -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla dex 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla con 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla con -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla con 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla cha 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla cha -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla cha 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ac 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ac -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ac 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla zenni 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla zenni -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla zenni 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla bank 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla bank -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla bank 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla exp 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla exp -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla exp 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla accuracy 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla accuracy -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla accuracy 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla damage 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla damage -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla damage 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla invis 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla invis -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla invis 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla practices 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla practices -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla practices 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla lessons 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla lessons -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla lessons 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla level 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla level -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla level 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla room 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla room -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla room 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla idnum 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla idnum -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla idnum 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla age 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla age -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla age 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla height 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla height -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla height 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla weight 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla weight -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla weight 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla trains 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla trains -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla trains 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla feats 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla feats -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla feats 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ethic 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ethic -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ethic 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla unused1 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla unused1 -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla unused1 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla unused2 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla unused2 -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla unused2 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla adminlevel 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla adminlevel -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla adminlevel 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairl 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairl -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairl 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairs 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairs -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairs 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairc 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairc -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hairc 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla skin 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla skin -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla skin 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla eye 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla eye -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla eye 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla basepl 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla basepl -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla basepl 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla baseki 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla baseki -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla baseki 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla basest 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla basest -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla basest 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla droom 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla droom -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla droom 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla absorbs 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla absorbs -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla absorbs 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ugp 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ugp -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla ugp 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla aura 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla aura -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla aura 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla trp 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla trp -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla trp 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla boost 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla boost -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla boost 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla deaths 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla deaths -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla deaths 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla phase 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla phase -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla phase 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla racial 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla racial -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla racial 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla slots 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla slots -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla slots 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla tclass 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla tclass -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla tclass 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla clones 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla clones -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla clones 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla armor 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla armor -99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla armor 99999999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla brief on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla brief off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla brief maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla invstart on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla invstart off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla invstart maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nosummon on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nosummon off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nosummon maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nohassle on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nohassle off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nohassle maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla frozen on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla frozen off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla frozen maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla killer on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla killer off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla killer maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla thief on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla thief off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla thief maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla roomflag on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla roomflag off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla roomflag maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla siteok on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla siteok off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla siteok maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla deleted on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla deleted off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla deleted maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nowizlist on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nowizlist off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nowizlist maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla quest on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla quest off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla quest maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla color on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla color off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla color maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nodelete on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nodelete off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla nodelete maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla multi on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla multi off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla multi maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla feature on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla feature off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla feature maybe", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla title the Brave", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla title ", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla drunk off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla drunk 30", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla drunk 99", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla drunk x", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hunger off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hunger 12", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla thirst off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla thirst 12", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla level 50", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla level 101", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla room 300", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla room 999999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla class Roshi", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla class xyz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla loadroom off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla loadroom 300", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla loadroom 99999", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla loadroom abc", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla passwd secret", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sex male", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sex female", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sex neutral", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla sex xyz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla age 1", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla age 30", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla age 30000", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla olc socials", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla olc actions", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla olc hedit", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla olc off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla olc 3", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla olc xyz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla race human", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla race namek", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla race xyz", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla user bob", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla adminlevel 3", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla adminlevel 9", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla adminlevel 0", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Nobody str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set player Sayla str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set player dummy str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set mob dummy str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set dummy str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set dummy level 150", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set dummy brief on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set dummy bank 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set file Sayla str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set file Nobody str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set file Sayla phase 1", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla xyzzy 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla stra 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla s 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set Sayla hu 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set me frozen on", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set me frozen off", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set me invis 3", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set PLAYER Sayla zenni 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor", "set MOB dummy zenni 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); ), false, 1},
    {"implementor, Sayla saved as one", "set file Sayla str 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); for (char_data *c : room_people(g, char_get_in_room(att))) if (!char_is_npc(c) && !strcmp(char_get_name(c), "Sayla")) { char_set_admlevel(c, ADMLVL_IMPL); save_char(g, c); }), false, 1},
    {"god", "set Sayla ac 5", false, SETUP(char_set_admlevel(att, ADMLVL_GOD);), false, 1},
    {"vice", "set Sayla nohassle on", false, SETUP(char_set_admlevel(att, ADMLVL_VICE);), false, 1},
    {"vice", "set me nohassle on", false, SETUP(char_set_admlevel(att, ADMLVL_VICE);), false, 1},
    {"vice, among equals", "set Sayla str 5", false, SETUP(char_set_admlevel(att, ADMLVL_VICE); for (char_data *c : room_people(g, char_get_in_room(att))) if (!char_is_npc(c) && c != att) char_set_admlevel(c, ADMLVL_VICE);), false, 1},
    {"implementor, among implementors", "set Sayla adminlevel 3", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); for (char_data *c : room_people(g, char_get_in_room(att))) if (!char_is_npc(c)) char_set_admlevel(c, ADMLVL_IMPL);), false, 1},
    {"implementor, unleveled", "set me practices 50", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_set_class_level(att, 0);), false, 1},
    {"implementor without an id", "set Sayla idnum 5", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_set_idnum(att, 0);), false, 1},
    {"implementor number 1", "set Sayla passwd secret", false, SETUP(char_set_admlevel(att, ADMLVL_IMPL); char_set_idnum(att, 1);), false, 1},
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
