#pragma once
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"
#include <string_view>
#include <concepts>
#include <cstdint>
#include <string>

#include "dbat/consts/types.hpp"

constexpr int MOB_TRIGGER = 0;
constexpr int OBJ_TRIGGER = 1;
constexpr int WLD_TRIGGER = 2;

/* mob trigger types */
extern const name_table trig_types; /* NUM_MTRIG_TYPES of them, a name for each bit (the unused ones too) */
enum class mob_trig : uint8_t {
  global    = 0,   /* check even if zone empty   */
  random    = 1,   /* checked randomly           */
  command   = 2,   /* character types a command  */
  speech    = 3,   /* a char says a word/phrase  */
  act       = 4,   /* word or phrase sent to act */
  death     = 5,   /* character dies             */
  greet     = 6,   /* something enters room seen */
  greet_all = 7,   /* anything enters room       */
  entry     = 8,   /* the mob enters a room      */
  receive   = 9,   /* character is given obj     */
  fight     = 10,  /* each pulse while fighting  */
  hitprcnt  = 11,  /* fighting and below some hp */
  bribe     = 12,  /* coins are given to mob     */
  load      = 13,  /* the mob is loaded          */
  memory    = 14,  /* mob see's someone remembered */
  cast      = 15,  /* mob targetted by spell     */
  leave     = 16,  /* someone leaves room seen   */
  door      = 17,  /* door manipulated in room   */
  time      = 19,  /* trigger based on game hour */
  count_    = 20,
};
template <> struct flag_traits<mob_trig> {
  static constexpr const name_table &names = trig_types;
};

/* The old names, until every use says mob_trig:: */
constexpr mob_trig MTRIG_GLOBAL    = mob_trig::global;
constexpr mob_trig MTRIG_RANDOM    = mob_trig::random;
constexpr mob_trig MTRIG_COMMAND   = mob_trig::command;
constexpr mob_trig MTRIG_SPEECH    = mob_trig::speech;
constexpr mob_trig MTRIG_ACT       = mob_trig::act;
constexpr mob_trig MTRIG_DEATH     = mob_trig::death;
constexpr mob_trig MTRIG_GREET     = mob_trig::greet;
constexpr mob_trig MTRIG_GREET_ALL = mob_trig::greet_all;
constexpr mob_trig MTRIG_ENTRY     = mob_trig::entry;
constexpr mob_trig MTRIG_RECEIVE   = mob_trig::receive;
constexpr mob_trig MTRIG_FIGHT     = mob_trig::fight;
constexpr mob_trig MTRIG_HITPRCNT  = mob_trig::hitprcnt;
constexpr mob_trig MTRIG_BRIBE     = mob_trig::bribe;
constexpr mob_trig MTRIG_LOAD      = mob_trig::load;
constexpr mob_trig MTRIG_MEMORY    = mob_trig::memory;
constexpr mob_trig MTRIG_CAST      = mob_trig::cast;
constexpr mob_trig MTRIG_LEAVE     = mob_trig::leave;
constexpr mob_trig MTRIG_DOOR      = mob_trig::door;
constexpr mob_trig MTRIG_TIME      = mob_trig::time;

/* obj trigger types */
extern const name_table otrig_types; /* NUM_OTRIG_TYPES of them, a name for each bit (the unused ones too) */
enum class obj_trig : uint8_t {
  global  = 0,   /* unused                     */
  random  = 1,   /* checked randomly           */
  command = 2,   /* character types a command  */
  timer   = 5,   /* item's timer expires       */
  get     = 6,   /* item is picked up          */
  drop    = 7,   /* character trys to drop obj */
  give    = 8,   /* character trys to give obj */
  wear    = 9,   /* character trys to wear obj */
  remove  = 11,  /* character trys to remove obj */
  load    = 13,  /* the object is loaded        */
  cast    = 15,  /* object targetted by spell   */
  leave   = 16,  /* someone leaves room seen    */
  consume = 18,  /* char tries to eat/drink obj */
  time    = 19,  /* trigger based on game hour */
  count_  = 20,
};
template <> struct flag_traits<obj_trig> {
  static constexpr const name_table &names = otrig_types;
};

/* The old names, until every use says obj_trig:: */
constexpr obj_trig OTRIG_GLOBAL  = obj_trig::global;
constexpr obj_trig OTRIG_RANDOM  = obj_trig::random;
constexpr obj_trig OTRIG_COMMAND = obj_trig::command;
constexpr obj_trig OTRIG_TIMER   = obj_trig::timer;
constexpr obj_trig OTRIG_GET     = obj_trig::get;
constexpr obj_trig OTRIG_DROP    = obj_trig::drop;
constexpr obj_trig OTRIG_GIVE    = obj_trig::give;
constexpr obj_trig OTRIG_WEAR    = obj_trig::wear;
constexpr obj_trig OTRIG_REMOVE  = obj_trig::remove;
constexpr obj_trig OTRIG_LOAD    = obj_trig::load;
constexpr obj_trig OTRIG_CAST    = obj_trig::cast;
constexpr obj_trig OTRIG_LEAVE   = obj_trig::leave;
constexpr obj_trig OTRIG_CONSUME = obj_trig::consume;
constexpr obj_trig OTRIG_TIME    = obj_trig::time;

/* wld trigger types */
extern const name_table wtrig_types; /* NUM_WTRIG_TYPES of them, a name for each bit (the unused ones too) */
enum class wld_trig : uint8_t {
  global  = 0,   /* check even if zone empty   */
  random  = 1,   /* checked randomly           */
  command = 2,   /* character types a command  */
  speech  = 3,   /* a char says word/phrase    */
  reset   = 5,   /* zone has been reset        */
  enter   = 6,   /* character enters room      */
  drop    = 7,   /* something dropped in room  */
  cast    = 15,  /* spell cast in room */
  leave   = 16,  /* character leaves the room */
  door    = 17,  /* door manipulated in room  */
  time    = 19,  /* trigger based on game hour */
  count_  = 20,
};
template <> struct flag_traits<wld_trig> {
  static constexpr const name_table &names = wtrig_types;
};

/* The old names, until every use says wld_trig:: */
constexpr wld_trig WTRIG_GLOBAL  = wld_trig::global;
constexpr wld_trig WTRIG_RANDOM  = wld_trig::random;
constexpr wld_trig WTRIG_COMMAND = wld_trig::command;
constexpr wld_trig WTRIG_SPEECH  = wld_trig::speech;
constexpr wld_trig WTRIG_RESET   = wld_trig::reset;
constexpr wld_trig WTRIG_ENTER   = wld_trig::enter;
constexpr wld_trig WTRIG_DROP    = wld_trig::drop;
constexpr wld_trig WTRIG_CAST    = wld_trig::cast;
constexpr wld_trig WTRIG_LEAVE   = wld_trig::leave;
constexpr wld_trig WTRIG_DOOR    = wld_trig::door;
constexpr wld_trig WTRIG_TIME    = wld_trig::time;

/*
 * The three families together. A trigger's types (trig_data::trigger_type) and a script's (script_data::types) are
 * one word whichever family: what the trigger is attached to says how its bits are read, and a trigger of one kind
 * may be attached to another. So the word stays a word, and each question asked of it names its family.
 */
template <class E>
concept trig_family = std::same_as<E, mob_trig> || std::same_as<E, obj_trig> || std::same_as<E, wld_trig>;

/* The family of a type, or of a set of them */
template <class T> struct trig_family_of;
template <trig_family E> struct trig_family_of<E> { using type = E; };
template <trig_family E> struct trig_family_of<flag_set<E>> { using type = E; };
template <class T> using trig_family_t = typename trig_family_of<T>::type;

/* Types as the word holds them, and whether a word has any of them */
template <trig_family E> constexpr bitvector_t trig_word(const flag_set<E> &types) { return types.word(0); }
template <trig_family E> constexpr bool trig_word_has(bitvector_t word, const flag_set<E> &types) { return (word & types.word(0)) != 0; }
template <trig_family E> constexpr bool trig_word_has(bitvector_t word, E type) { return trig_word_has(word, flag_set<E>{type}); }

/* A word's type names, read as the family of what it's attached to (MOB_TRIGGER and anything else as a mob's), as
   sprintbit() gave them */
std::string trig_type_names(int attach_type, bitvector_t word);

/* obj command trigger types */
/* Where an object command trigger hears the command from: bits of its narg, which is a plain number (the other
   triggers' chance) */
extern const name_table ocmd_bits; /* flag_count<ocmd_where> of them */
enum class ocmd_where : uint8_t {
  equip  = 0, /* obj must be in char's equip */
  inven  = 1, /* obj must be in char's inven */
  room   = 2, /* obj must be in char's room  */
  count_ = 3,
};
template <> struct flag_traits<ocmd_where> {
  static constexpr const name_table &names = ocmd_bits;
};
constexpr ocmd_where OCMD_EQUIP = ocmd_where::equip;
constexpr ocmd_where OCMD_INVEN = ocmd_where::inven;
constexpr ocmd_where OCMD_ROOM  = ocmd_where::room;

/* Whether a trigger's narg has this where */
constexpr bool narg_hears(int narg, ocmd_where where) { return (static_cast<bitvector_t>(narg) & flag_set<ocmd_where>{where}.word(0)) != 0; }

/* obj consume trigger commands */
enum class consume_cmd : uint8_t { eat = 1, drink = 2, quaff = 3 };
constexpr consume_cmd OCMD_EAT   = consume_cmd::eat;
constexpr consume_cmd OCMD_DRINK = consume_cmd::drink;
constexpr consume_cmd OCMD_QUAFF = consume_cmd::quaff;

constexpr int TRIG_NEW     = 0; /* trigger starts from top  */
constexpr int TRIG_RESTART = 1; /* trigger restarting       */

constexpr int NUM_MTRIG_TYPES = static_cast<int>(flag_count<mob_trig>);
constexpr int NUM_OTRIG_TYPES = static_cast<int>(flag_count<obj_trig>);
constexpr int NUM_WTRIG_TYPES = static_cast<int>(flag_count<wld_trig>);

/*
 * DG Scripts UID ranges. The range a UID falls in tells scripts what kind of
 * entity it refers to, so the ranges must never overlap. Room UIDs are saved
 * in player variable files, so PLAYER/ROOM/OBJ bases must not move.
 */
constexpr int PLAYER_ID_MAX       = 50000;      /* 0 .. 49999: player idnums           */
constexpr int ROOM_ID_BASE        = 1050000;    /* vnum + ROOM_ID_BASE (250000 rooms)  */
constexpr int OBJ_ID_BASE         = 1300000;    /* objects                             */
constexpr bitvector_t MOB_ID_BASE = (1u << 30); /* mobs; objects end just below this   */
constexpr bitvector_t OBJ_ID_MAX  = (MOB_ID_BASE - 1);
constexpr int MOB_ID_MAX          = INT32_MAX;
/* Mobs used 50000..1049999 before; find_char() still accepts that range. */
constexpr int LEGACY_MOB_ID_BASE = 50000;

/* unless you change this, Puff casts all your dg spells */
constexpr mob_vnum DG_CASTER_PROXY = 1;
/* spells cast by objects and rooms use this level */
constexpr int DG_SPELL_LEVEL = 25;

constexpr int MAX_SCRIPT_DEPTH  = 10;       /* maximum depth triggers can recurse into each other */
constexpr int SCRIPT_ERROR_CODE = -9999999; /* this shouldn't happen too often */

/* flag for valid_dg_target() */
constexpr int DG_ALLOW_GODS = (1 << 0);

/* first character of a UID as it appears in script text, e.g. "}1063303" */
constexpr char UID_CHAR = '}';

