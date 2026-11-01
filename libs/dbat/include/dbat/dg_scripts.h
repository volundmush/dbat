/**************************************************************************
*  File: dg_scripts.h                                                     *
*  Usage: header file for script structures and constants, and            *
*         function prototypes for dg_scripts.c                            *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/triggers.h"
#include "dbat/index.h"

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  int vnum;                     /* vnum of the trigger   */
  struct trig_proto_list *next; /* next trigger          */
};

#define DG_SCRIPT_VERSION "DG Scripts 1.0.14"

/* unless you change this, Puff casts all your dg spells */
#define DG_CASTER_PROXY 1
/* spells cast by objects and rooms use this level */
#define DG_SPELL_LEVEL 25

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
#define PULSE_DG_SCRIPT (13 RL_SEC)

#define MAX_SCRIPT_DEPTH 10 /* maximum depth triggers can
					     recurse into each other */

#define SCRIPT_ERROR_CODE -9999999 /* this shouldn't happen too often */

/* one line of the trigger */
struct cmdlist_element {
  char *cmd; /* one line of a trigger */
  struct cmdlist_element *original;
  struct cmdlist_element *next;
};

struct trig_var_data {
  char *name;   /* name of variable  */
  char *value;  /* value of variable */
  long context; /* 0: global context */

  struct trig_var_data *next;
};

/* structure for triggers */
struct trig_data {
  IDXTYPE nr;                         /* trigger's rnum                  */
  uint8_t attach_type;                /* mob/obj/wld intentions          */
  uint8_t data_type;                  /* type of game_data for trig      */
  char *name;                         /* name of trigger                 */
  bitvector_t trigger_type;           /* type of trigger (for bitvector) */
  struct cmdlist_element *cmdlist;    /* top of command list             */
  struct cmdlist_element *curr_state; /* ptr to current line of trigger  */
  int narg;                           /* numerical argument              */
  char *arglist;                      /* argument list                   */
  int depth;                          /* depth into nest ifs/whiles/etc  */
  int loops;                          /* loop iteration counter          */
  struct event *wait_event;           /* event to pause the trigger      */
  bool purged;                        /* trigger is set to be purged     */
  struct trig_var_data *var_list;     /* list of local vars for trigger  */

  struct trig_data *next;
  struct trig_data *next_in_world; /* next in the global trigger list */
};

/* a complete script (composed of several triggers) */
struct script_data {
  bitvector_t types;                 /* bitvector of trigger types */
  struct trig_data *trig_list;       /* list of triggers           */
  struct trig_var_data *global_vars; /* list of global variables   */
  bool purged;                       /* script is set to be purged */
  long context;                      /* current context for statics */

  struct script_data *next; /* used for purged_scripts    */
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
  long id;   /* id of who to remember */
  char *cmd; /* command, or NULL for generic */
  struct script_memory *next;
};

extern struct index_data **trig_index;

extern struct trig_data *trigger_list;
extern int top_of_trigt;

#include "db.h"
#include "genzon.h"
#include "oasis.h"

ACMD(do_attach);
ACMD(do_detach);
ACMD(do_vdelete);
ACMD(do_tstat);

ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mheal);
ACMD(do_mjunk);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mzoneecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mteleport);
ACMD(do_mdamage);
ACMD(do_mforce);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mdoor);
ACMD(do_mfollow);
ACMD(do_mrecho);

/* function prototypes from dg_triggers.c */

void act_mtrigger(const char_data *ch, char *str, char_data *actor, const char_data *victim, obj_data *object, const obj_data *target, const char *arg);
void speech_mtrigger(char_data *actor, char *str);
void speech_wtrigger(char_data *actor, char *str);
void greet_memory_mtrigger(char_data *ch);
int greet_mtrigger(char_data *actor, int dir);
int entry_mtrigger(char_data *ch);
void entry_memory_mtrigger(char_data *ch);
int enter_wtrigger(room_data *room, char_data *actor, int dir);
int drop_otrigger(obj_data *obj, char_data *actor);
void timer_otrigger(obj_data *obj);
int get_otrigger(obj_data *obj, char_data *actor);
int drop_wtrigger(obj_data *obj, char_data *actor);
int give_otrigger(obj_data *obj, char_data *actor,
                  char_data *victim);
int receive_mtrigger(char_data *ch, char_data *actor,
                     obj_data *obj);
void bribe_mtrigger(char_data *ch, char_data *actor,
                    int amount);
int wear_otrigger(obj_data *obj, char_data *actor, int where);
int remove_otrigger(obj_data *obj, char_data *actor);

int command_mtrigger(char_data *actor, char *cmd, char *argument);
int command_otrigger(char_data *actor, char *cmd, char *argument);
int command_wtrigger(char_data *actor, char *cmd, char *argument);

int death_mtrigger(char_data *ch, char_data *actor);
void fight_mtrigger(char_data *ch);
void hitprcnt_mtrigger(char_data *ch);

void random_mtrigger(char_data *ch);
void random_otrigger(obj_data *obj);
void random_wtrigger(room_data *ch);
void reset_wtrigger(room_data *ch);

void load_mtrigger(char_data *ch);
void load_otrigger(obj_data *obj);

int cast_mtrigger(char_data *actor, char_data *ch, int spellnum);
int cast_otrigger(char_data *actor, obj_data *obj, int spellnum);
int cast_wtrigger(char_data *actor, char_data *vict, obj_data *obj, int spellnum);

int leave_mtrigger(char_data *actor, int dir);
int leave_wtrigger(room_data *room, char_data *actor, int dir);
int leave_otrigger(room_data *room, char_data *actor, int dir);

int door_mtrigger(char_data *actor, int subcmd, int dir);
int door_wtrigger(char_data *actor, int subcmd, int dir);

int consume_otrigger(obj_data *obj, char_data *actor, int cmd);

void time_mtrigger(char_data *ch);
void time_otrigger(obj_data *obj);
void time_wtrigger(room_data *room);

/* function prototypes from dg_scripts.c */
char *str_str(char *cs, char *ct);
int find_eq_pos_script(char *arg);
int can_wear_on_pos(struct obj_data *obj, int pos);
struct char_data *find_char(long n);
char_data *get_char(char *name);
char_data *get_char_near_obj(obj_data *obj, char *name);
char_data *get_char_in_room(room_data *room, char *name);
obj_data *get_obj_near_obj(obj_data *obj, char *name);
obj_data *get_obj(char *name);
room_data *get_room(char *name);
char_data *get_char_by_obj(obj_data *obj, char *name);
char_data *get_char_by_room(room_data *room, char *name);
obj_data *get_obj_by_obj(obj_data *obj, char *name);
obj_data *get_obj_in_room(room_data *room, char *name);
obj_data *get_obj_by_room(room_data *room, char *name);
int trgvar_in_room(room_vnum vnum);
obj_data *get_obj_in_list(char *name, obj_data *list);
obj_data *get_object_in_equip(char_data *ch, char *name);
void script_trigger_check(void);
void check_time_triggers(void);
void find_uid_name(char *uid, char *name, size_t nlen);
void do_sstat_room(struct char_data *ch, struct room_data *rm);
void do_sstat_object(char_data *ch, obj_data *j);
void do_sstat_character(char_data *ch, char_data *k);
void add_trigger(struct script_data *sc, trig_data *t, int loc);
void script_vlog(const char *format, va_list args) __attribute__((format(printf, 1, 0)));
void script_log(const char *format, ...) __attribute__((format(printf, 1, 2)));
char *matching_quote(char *p);
struct room_data *dg_room_of_obj(struct obj_data *obj);

/* To maintain strict-aliasing we'll have to do this trick with a union */
/* Thanks to Chris Gilbert for reminding me that there are other options. */
int script_driver(void *go_adress, trig_data *trig, int type, int mode);
trig_rnum real_trigger(trig_vnum vnum);
void process_eval(void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd);
void read_saved_vars(struct char_data *ch);
void save_char_vars(struct char_data *ch);
void init_lookup_table(void);
void add_to_lookup_table(long uid, int type, void *c);
long dg_new_mob_uid(void);
long dg_new_obj_uid(void);
void dg_log_uid_stats(void);
void remove_from_lookup_table(long uid);

/* from dg_db_scripts.c */
void parse_trigger(FILE *trig_f, int nr);
trig_data *read_trigger(int nr);
void trig_data_copy(trig_data *this_data, const trig_data *trg);
void dg_read_trigger(FILE *fp, void *proto, int type);
void dg_obj_trigger(char *line, struct obj_data *obj);
void assign_triggers(void *i, int type);

/* From dg_variables.c */
void add_var(struct trig_var_data **var_list, char *name, const char *value, long id);
void var_subst(void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf);

/* From dg_handler.c */
void free_varlist(struct trig_var_data *vd);
int remove_var(struct trig_var_data **var_list, char *name);
void free_trigger(trig_data *trig);
void extract_trigger(struct trig_data *trig);
void extract_script(void *thing, int type);
void extract_script_mem(struct script_memory *sc);
void free_proto_script(void *thing, int type);
void copy_proto_script(void *source, void *dest, int type);
void delete_variables(const char *charname);
void update_wait_events(struct room_data *to, struct room_data *from);

/* from dg_comm.c */
void sub_write(char *arg, char_data *ch, int8_t find_invis, int targets);
void send_to_zone(char *messg, zone_rnum zone);

/* from dg_misc.c */
void do_dg_cast(void *go, struct script_data *sc, trig_data *trig,
                int type, char *cmd);
void do_dg_affect(void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd);
int valid_dg_target(char_data *ch, int bitvector);
void script_damage(char_data *vict, int dam);

int check_flags_by_name_ar(bitvector_t *array, int numflags, char *search, const char *namelist[]);

/* from dg_objcmd.c */
room_rnum obj_room(obj_data *obj);

/* defines for valid_dg_target */
#define DG_ALLOW_GODS (1 << 0)

/* Macros for scripts */

#define UID_CHAR              '}'
#define GET_TRIG_NAME(t)      ((t)->name)
#define GET_TRIG_RNUM(t)      ((t)->nr)
#define GET_TRIG_VNUM(t)      (trig_index[(t)->nr]->vnum)
#define GET_TRIG_TYPE(t)      ((t)->trigger_type)
#define GET_TRIG_DATA_TYPE(t) ((t)->data_type)
#define GET_TRIG_NARG(t)      ((t)->narg)
#define GET_TRIG_ARG(t)       ((t)->arglist)
#define GET_TRIG_VARS(t)      ((t)->var_list)
#define GET_TRIG_WAIT(t)      ((t)->wait_event)
#define GET_TRIG_DEPTH(t)     ((t)->depth)
#define GET_TRIG_LOOPS(t)     ((t)->loops)

/* player id's: 0 to PLAYER_ID_MAX - 1            */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1      */
/* object id's: OBJ_ID_BASE to OBJ_ID_MAX         */
/* mob id's: MOB_ID_BASE to MOB_ID_MAX            */
/* See consts/triggers.h; allocate with dg_new_mob_uid()/dg_new_obj_uid(). */

#define SCRIPT(o)     ((o)->script)
#define SCRIPT_MEM(c) ((c)->memory)

#define SCRIPT_TYPES(s) ((s)->types)
#define TRIGGERS(s)     ((s)->trig_list)

#define GET_SHORT(ch) ((ch)->short_descr)

#define SCRIPT_CHECK(go, type) (SCRIPT(go) && \
                                IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type) (IS_SET(GET_TRIG_TYPE(t), type) && \
                                !GET_TRIG_DEPTH(t))

#define ADD_UID_VAR(buf, trig, go, name, context)                                                                                                                            \
  do {                                                                                                                                                                       \
    sprintf(buf, "%c%d", UID_CHAR,                                                                                                                                           \
            _Generic((go), struct char_data *: char_get_id, const struct char_data *: char_get_id, struct obj_data *: obj_get_id, const struct obj_data *: obj_get_id)(go)); \
    add_var(&GET_TRIG_VARS(trig), name, buf, context);                                                                                                                       \
  } while (0)
