#pragma once
#include "types.h"
#include <stdint.h>

constexpr int MOB_TRIGGER = 0;
constexpr int OBJ_TRIGGER = 1;
constexpr int WLD_TRIGGER = 2;

/* mob trigger types */
constexpr bitvector_t MTRIG_GLOBAL    = (1u << 0);  /* check even if zone empty   */
constexpr bitvector_t MTRIG_RANDOM    = (1u << 1);  /* checked randomly           */
constexpr bitvector_t MTRIG_COMMAND   = (1u << 2);  /* character types a command  */
constexpr bitvector_t MTRIG_SPEECH    = (1u << 3);  /* a char says a word/phrase  */
constexpr bitvector_t MTRIG_ACT       = (1u << 4);  /* word or phrase sent to act */
constexpr bitvector_t MTRIG_DEATH     = (1u << 5);  /* character dies             */
constexpr bitvector_t MTRIG_GREET     = (1u << 6);  /* something enters room seen */
constexpr bitvector_t MTRIG_GREET_ALL = (1u << 7);  /* anything enters room       */
constexpr bitvector_t MTRIG_ENTRY     = (1u << 8);  /* the mob enters a room      */
constexpr bitvector_t MTRIG_RECEIVE   = (1u << 9);  /* character is given obj     */
constexpr bitvector_t MTRIG_FIGHT     = (1u << 10); /* each pulse while fighting  */
constexpr bitvector_t MTRIG_HITPRCNT  = (1u << 11); /* fighting and below some hp */
constexpr bitvector_t MTRIG_BRIBE     = (1u << 12); /* coins are given to mob     */
constexpr bitvector_t MTRIG_LOAD      = (1u << 13); /* the mob is loaded          */
constexpr bitvector_t MTRIG_MEMORY    = (1u << 14); /* mob see's someone remembered */
constexpr bitvector_t MTRIG_CAST      = (1u << 15); /* mob targetted by spell     */
constexpr bitvector_t MTRIG_LEAVE     = (1u << 16); /* someone leaves room seen   */
constexpr bitvector_t MTRIG_DOOR      = (1u << 17); /* door manipulated in room   */

constexpr bitvector_t MTRIG_TIME = (1u << 19); /* trigger based on game hour */

/* obj trigger types */
constexpr bitvector_t OTRIG_GLOBAL  = (1u << 0); /* unused                     */
constexpr bitvector_t OTRIG_RANDOM  = (1u << 1); /* checked randomly           */
constexpr bitvector_t OTRIG_COMMAND = (1u << 2); /* character types a command  */

constexpr bitvector_t OTRIG_TIMER  = (1u << 5);  /* item's timer expires       */
constexpr bitvector_t OTRIG_GET    = (1u << 6);  /* item is picked up          */
constexpr bitvector_t OTRIG_DROP   = (1u << 7);  /* character trys to drop obj */
constexpr bitvector_t OTRIG_GIVE   = (1u << 8);  /* character trys to give obj */
constexpr bitvector_t OTRIG_WEAR   = (1u << 9);  /* character trys to wear obj */
constexpr bitvector_t OTRIG_REMOVE = (1u << 11); /* character trys to remove obj */

constexpr bitvector_t OTRIG_LOAD = (1u << 13); /* the object is loaded        */

constexpr bitvector_t OTRIG_CAST  = (1u << 15); /* object targetted by spell   */
constexpr bitvector_t OTRIG_LEAVE = (1u << 16); /* someone leaves room seen    */

constexpr bitvector_t OTRIG_CONSUME = (1u << 18); /* char tries to eat/drink obj */
constexpr bitvector_t OTRIG_TIME    = (1u << 19); /* trigger based on game hour */

/* wld trigger types */
constexpr bitvector_t WTRIG_GLOBAL  = (1u << 0); /* check even if zone empty   */
constexpr bitvector_t WTRIG_RANDOM  = (1u << 1); /* checked randomly           */
constexpr bitvector_t WTRIG_COMMAND = (1u << 2); /* character types a command  */
constexpr bitvector_t WTRIG_SPEECH  = (1u << 3); /* a char says word/phrase    */

constexpr bitvector_t WTRIG_RESET = (1u << 5); /* zone has been reset        */
constexpr bitvector_t WTRIG_ENTER = (1u << 6); /* character enters room      */
constexpr bitvector_t WTRIG_DROP  = (1u << 7); /* something dropped in room  */

constexpr bitvector_t WTRIG_CAST  = (1u << 15); /* spell cast in room */
constexpr bitvector_t WTRIG_LEAVE = (1u << 16); /* character leaves the room */
constexpr bitvector_t WTRIG_DOOR  = (1u << 17); /* door manipulated in room  */

constexpr bitvector_t WTRIG_TIME = (1u << 19); /* trigger based on game hour */

/* obj command trigger types */
constexpr bitvector_t OCMD_EQUIP = (1u << 0); /* obj must be in char's equip */
constexpr bitvector_t OCMD_INVEN = (1u << 1); /* obj must be in char's inven */
constexpr bitvector_t OCMD_ROOM  = (1u << 2); /* obj must be in char's room  */

/* obj consume trigger commands */
constexpr int OCMD_EAT   = 1;
constexpr int OCMD_DRINK = 2;
constexpr int OCMD_QUAFF = 3;

constexpr int TRIG_NEW     = 0; /* trigger starts from top  */
constexpr int TRIG_RESTART = 1; /* trigger restarting       */

constexpr int NUM_MTRIG_TYPES = 20;
constexpr int NUM_OTRIG_TYPES = 20;
constexpr int NUM_WTRIG_TYPES = 20;

extern const char *trig_types[NUM_MTRIG_TYPES + 1];
extern const char *otrig_types[NUM_OTRIG_TYPES + 1];
extern const char *wtrig_types[NUM_WTRIG_TYPES + 1];

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