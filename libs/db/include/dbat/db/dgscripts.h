#pragma once
#include "consts/types.h"

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};

#define DG_SCRIPT_VERSION "DG Scripts 1.0.14"



/* unless you change this, Puff casts all your dg spells */
#define DG_CASTER_PROXY 1
/* spells cast by objects and rooms use this level */
#define DG_SPELL_LEVEL  25

/*
 * define this if you don't want wear/remove triggers to fire when
 * a player is saved.
 */
#define NO_EXTRANEOUS_TRIGGERS
/*
 * %actor.room% behaviour :
 * Until pl 7 %actor.room% returned a room vnum.
 * Working with this number in scripts was unnecessarily hard,
 * especially in those situations one needed the id of the room,
 * the items in it, etc. As a result of this, the output
 * has been changed (as of pl 8) to a room variable.
 * This means old scripts will need a minor adjustment;
 *
 * Before:
 * if %actor.room%==3001
 *   %echo% You are at the main temple.
 *
 * After:
 * eval room %actor.room%
 * if %room.vnum%==3001
 *   %echo% You are at the main temple.
 *
 * If you wish to continue using the old style, comment out the line below.
 *
 * Welcor
 */
#define ACTOR_ROOM_IS_UID 1



/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
#define PULSE_DG_SCRIPT         (13 RL_SEC)


#define MAX_SCRIPT_DEPTH      10          /* maximum depth triggers can
					     recurse into each other */

#define SCRIPT_ERROR_CODE     -9999999   /* this shouldn't happen too often */

/* one line of the trigger */
struct cmdlist_element {
  char *cmd;				/* one line of a trigger */
  struct cmdlist_element *original;
  struct cmdlist_element *next;
};

struct trig_var_data {
  char *name;				/* name of variable  */
  char *value;				/* value of variable */
  long context;				/* 0: global context */

  struct trig_var_data *next;
};

/* structure for triggers */
struct trig_data {
    IDXTYPE nr; 	                /* trigger's rnum                  */
    int8_t attach_type;			/* mob/obj/wld intentions          */
    int8_t data_type;		        /* type of game_data for trig      */
    char *name;			        /* name of trigger                 */
    long trigger_type;			/* type of trigger (for bitvector) */
    struct cmdlist_element *cmdlist;	/* top of command list             */
    struct cmdlist_element *curr_state;	/* ptr to current line of trigger  */
    int narg;				/* numerical argument              */
    char *arglist;			/* argument list                   */
    int depth;				/* depth into nest ifs/whiles/etc  */
    int loops;				/* loop iteration counter          */
    struct event *wait_event;   	/* event to pause the trigger      */
    bool purged;			/* trigger is set to be purged     */
    struct trig_var_data *var_list;	/* list of local vars for trigger  */

    struct trig_data *next;
    struct trig_data *next_in_world;    /* next in the global trigger list */
};


/* a complete script (composed of several triggers) */
struct script_data {
  long types;				/* bitvector of trigger types */
  struct trig_data *trig_list;	        /* list of triggers           */
  struct trig_var_data *global_vars;	/* list of global variables   */
  bool purged;				/* script is set to be purged */
  long context;				/* current context for statics */

  struct script_data *next;		/* used for purged_scripts    */
};

/* The event data for the wait command */
struct wait_event_data {
  struct trig_data *trigger;
  void *go;
  int type;
};

/* typedefs that the dg functions rely on */

typedef struct room_data room_data;
typedef struct obj_data obj_data;
typedef struct trig_data trig_data;
typedef struct char_data char_data;

/* used for actor memory triggers */
struct script_memory {
  long id;				/* id of who to remember */
  char *cmd;				/* command, or NULL for generic */
  struct script_memory *next;
};
