#pragma once

#define    MOB_TRIGGER   0
#define    OBJ_TRIGGER   1
#define    WLD_TRIGGER   2

/* mob trigger types */
#define MTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define MTRIG_RANDOM           (1 << 1)      /* checked randomly           */
#define MTRIG_COMMAND          (1 << 2)	     /* character types a command  */
#define MTRIG_SPEECH           (1 << 3)	     /* a char says a word/phrase  */
#define MTRIG_ACT              (1 << 4)      /* word or phrase sent to act */
#define MTRIG_DEATH            (1 << 5)      /* character dies             */
#define MTRIG_GREET            (1 << 6)      /* something enters room seen */
#define MTRIG_GREET_ALL        (1 << 7)      /* anything enters room       */
#define MTRIG_ENTRY            (1 << 8)      /* the mob enters a room      */
#define MTRIG_RECEIVE          (1 << 9)      /* character is given obj     */
#define MTRIG_FIGHT            (1 << 10)     /* each pulse while fighting  */
#define MTRIG_HITPRCNT         (1 << 11)     /* fighting and below some hp */
#define MTRIG_BRIBE	       (1 << 12)     /* coins are given to mob     */
#define MTRIG_LOAD             (1 << 13)     /* the mob is loaded          */
#define MTRIG_MEMORY           (1 << 14)     /* mob see's someone remembered */
#define MTRIG_CAST             (1 << 15)     /* mob targetted by spell     */
#define MTRIG_LEAVE            (1 << 16)     /* someone leaves room seen   */
#define MTRIG_DOOR             (1 << 17)     /* door manipulated in room   */

#define MTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* obj trigger types */
#define OTRIG_GLOBAL           (1 << 0)	     /* unused                     */
#define OTRIG_RANDOM           (1 << 1)	     /* checked randomly           */
#define OTRIG_COMMAND          (1 << 2)      /* character types a command  */

#define OTRIG_TIMER            (1 << 5)     /* item's timer expires       */
#define OTRIG_GET              (1 << 6)     /* item is picked up          */
#define OTRIG_DROP             (1 << 7)     /* character trys to drop obj */
#define OTRIG_GIVE             (1 << 8)     /* character trys to give obj */
#define OTRIG_WEAR             (1 << 9)     /* character trys to wear obj */
#define OTRIG_REMOVE           (1 << 11)    /* character trys to remove obj */

#define OTRIG_LOAD             (1 << 13)    /* the object is loaded        */

#define OTRIG_CAST             (1 << 15)    /* object targetted by spell   */
#define OTRIG_LEAVE            (1 << 16)    /* someone leaves room seen    */

#define OTRIG_CONSUME          (1 << 18)    /* char tries to eat/drink obj */
#define OTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* wld trigger types */
#define WTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define WTRIG_RANDOM           (1 << 1)	     /* checked randomly           */
#define WTRIG_COMMAND          (1 << 2)	     /* character types a command  */
#define WTRIG_SPEECH           (1 << 3)      /* a char says word/phrase    */

#define WTRIG_RESET            (1 << 5)      /* zone has been reset        */
#define WTRIG_ENTER            (1 << 6)	     /* character enters room      */
#define WTRIG_DROP             (1 << 7)      /* something dropped in room  */

#define WTRIG_CAST             (1 << 15)     /* spell cast in room */
#define WTRIG_LEAVE            (1 << 16)     /* character leaves the room */
#define WTRIG_DOOR             (1 << 17)     /* door manipulated in room  */

#define WTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* obj command trigger types */
#define OCMD_EQUIP             (1 << 0)	     /* obj must be in char's equip */
#define OCMD_INVEN             (1 << 1)	     /* obj must be in char's inven */
#define OCMD_ROOM              (1 << 2)	     /* obj must be in char's room  */

/* obj consume trigger commands */
#define OCMD_EAT    1
#define OCMD_DRINK  2
#define OCMD_QUAFF  3

#define TRIG_NEW                0	     /* trigger starts from top  */
#define TRIG_RESTART            1	     /* trigger restarting       */

#define NUM_MTRIG_TYPES         20
#define NUM_OTRIG_TYPES         20
#define NUM_WTRIG_TYPES         20

extern const char *trig_types[NUM_MTRIG_TYPES+1];
extern const char *otrig_types[NUM_OTRIG_TYPES+1];
extern const char *wtrig_types[NUM_WTRIG_TYPES+1];

#define MOB_ID_BASE	  50000  /* 50000 player IDNUMS should suffice */
#define ROOM_ID_BASE    1050000 /* 1000000 Mobs */
#define OBJ_ID_BASE     1300000 /* 250000 Rooms */