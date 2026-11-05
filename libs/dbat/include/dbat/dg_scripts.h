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
#include "dbat/consts/pulse.h"
#include "dbat/consts/types.h"
#include "dbat/consts/triggers.h"
#include "dbat/index.h"

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  trig_vnum vnum;               /* vnum of the trigger   */
  struct trig_proto_list *next; /* next trigger          */
};

#define DG_SCRIPT_VERSION "DG Scripts 1.0.14"

/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
constexpr int PULSE_DG_SCRIPT = 13 * PASSES_PER_SEC;

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
  bool extracted;                  /* removed while a script was running: see free_extracted_triggers() */
  bool owner_gone;                 /* and what it belonged to went too */
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

void act_mtrigger(GameWorld *g, const char_data *ch, char *str, char_data *actor, const char_data *victim, obj_data *object, const obj_data *target, const char *arg);
void speech_mtrigger(GameWorld *g, char_data *actor, char *str);
void speech_wtrigger(GameWorld *g, char_data *actor, char *str);
void greet_memory_mtrigger(GameWorld *g, char_data *ch);
bool greet_mtrigger(GameWorld *g, char_data *actor, int dir);
int entry_mtrigger(GameWorld *g, char_data *ch);
void entry_memory_mtrigger(GameWorld *g, char_data *ch);
int enter_wtrigger(GameWorld *g, room_data *room, char_data *actor, int dir);
int drop_otrigger(GameWorld *g, obj_data *obj, char_data *actor);
void timer_otrigger(GameWorld *g, obj_data *obj);
int get_otrigger(GameWorld *g, obj_data *obj, char_data *actor);
int drop_wtrigger(GameWorld *g, obj_data *obj, char_data *actor);
int give_otrigger(GameWorld *g, obj_data *obj, char_data *actor,
                  char_data *victim);
int receive_mtrigger(GameWorld *g, char_data *ch, char_data *actor,
                     obj_data *obj);
void bribe_mtrigger(GameWorld *g, char_data *ch, char_data *actor,
                    int amount);
int wear_otrigger(GameWorld *g, obj_data *obj, char_data *actor, int where);
int remove_otrigger(GameWorld *g, obj_data *obj, char_data *actor);

bool command_mtrigger(GameWorld *g, char_data *actor, char *cmd, char *argument);
bool command_otrigger(GameWorld *g, char_data *actor, char *cmd, char *argument);
int command_wtrigger(GameWorld *g, char_data *actor, char *cmd, char *argument);

int death_mtrigger(GameWorld *g, char_data *ch, char_data *actor);
void fight_mtrigger(GameWorld *g, char_data *ch);
void hitprcnt_mtrigger(GameWorld *g, char_data *ch);

void random_mtrigger(GameWorld *g, char_data *ch);
void random_otrigger(GameWorld *g, obj_data *obj);
void random_wtrigger(GameWorld *g, room_data *ch);
void reset_wtrigger(GameWorld *g, room_data *ch);

void load_mtrigger(GameWorld *g, char_data *ch);
bool load_otrigger(GameWorld *g, obj_data *obj); /* false if its script purged it */

int cast_mtrigger(GameWorld *g, char_data *actor, char_data *ch, int spellnum);
int cast_otrigger(GameWorld *g, char_data *actor, obj_data *obj, int spellnum);
int cast_wtrigger(GameWorld *g, char_data *actor, char_data *vict, obj_data *obj, int spellnum);

int leave_mtrigger(GameWorld *g, char_data *actor, int dir);
int leave_wtrigger(GameWorld *g, room_data *room, char_data *actor, int dir);
bool leave_otrigger(GameWorld *g, room_data *room, char_data *actor, int dir);

int door_mtrigger(GameWorld *g, char_data *actor, int subcmd, int dir);
int door_wtrigger(GameWorld *g, char_data *actor, int subcmd, int dir);

int consume_otrigger(GameWorld *g, obj_data *obj, char_data *actor, int cmd);

void time_mtrigger(GameWorld *g, char_data *ch);
void time_otrigger(GameWorld *g, obj_data *obj);
void time_wtrigger(GameWorld *g, room_data *room);

/* function prototypes from dg_scripts.c */
char *str_str(char *cs, char *ct);
int find_eq_pos_script(char *arg);
bool can_wear_on_pos(struct obj_data *obj, int pos);
struct char_data *find_char(GameWorld *g, long n);
char_data *get_char(GameWorld *g, char *name);
char_data *get_char_near_obj(GameWorld *g, obj_data *obj, char *name);
char_data *get_char_in_room(GameWorld *g, room_data *room, char *name);
obj_data *get_obj_near_obj(GameWorld *g, obj_data *obj, char *name);
obj_data *get_obj(GameWorld *g, char *name);
room_data *get_room(GameWorld *g, char *name);
char_data *get_char_by_obj(GameWorld *g, obj_data *obj, char *name);
char_data *get_char_by_room(GameWorld *g, room_data *room, char *name);
obj_data *get_obj_by_obj(GameWorld *g, obj_data *obj, char *name);
obj_data *get_obj_in_room(room_data *room, char *name);
obj_data *get_obj_by_room(GameWorld *g, room_data *room, char *name);
int trgvar_in_room(GameWorld *g, room_vnum vnum);
obj_data *get_obj_in_list(char *name, obj_data *list);
obj_data *get_object_in_equip(GameWorld *g, char_data *ch, char *name);
void script_trigger_check(GameWorld *g);
void check_time_triggers(GameWorld *g);
void find_uid_name(GameWorld *g, char *uid, char *name, size_t nlen);
void do_sstat_room(GameWorld *g, struct char_data *ch, struct room_data *rm);
void do_sstat_object(GameWorld *g, char_data *ch, obj_data *j);
void do_sstat_character(GameWorld *g, char_data *ch, char_data *k);
void add_trigger(GameWorld *g, struct script_data *sc, trig_data *t, int loc);
void script_vlog(GameWorld *g, const char *format, va_list args) __attribute__((format(printf, 2, 0)));
void script_log(GameWorld *g, const char *format, ...) __attribute__((format(printf, 2, 3)));
char *matching_quote(char *p);
struct room_data *dg_room_of_obj(GameWorld *g, struct obj_data *obj);
void obj_command_interpreter(GameWorld *g, obj_data *obj, char *argument);
void wld_command_interpreter(GameWorld *g, room_data *room, char *argument);

/* To maintain strict-aliasing we'll have to do this trick with a union */
/* Thanks to Chris Gilbert for reminding me that there are other options. */
int script_driver(GameWorld *g, void *go_adress, trig_data *trig, int type, int mode);
trig_rnum real_trigger(GameWorld *g, trig_vnum vnum);
obj_data *find_obj(GameWorld *g, long n);
void process_eval(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd);
void read_saved_vars(struct char_data *ch);
void save_char_vars(GameWorld *g, struct char_data *ch);
void init_lookup_table(GameWorld *g);
void add_to_lookup_table(GameWorld *g, long uid, int type, void *c);
long dg_new_mob_uid(GameWorld *g);
long dg_new_obj_uid(GameWorld *g);
void dg_log_uid_stats(GameWorld *g);
void remove_from_lookup_table(GameWorld *g, long uid);

/* from dg_db_scripts.c */
void parse_trigger(GameWorld *g, FILE *trig_f, trig_vnum nr);
int dg_lint_trigger(GameWorld *g, const trig_data *trig, struct descriptor_data *d);
trig_data *read_trigger(GameWorld *g, trig_rnum nr);
void trig_data_copy(trig_data *this_data, const trig_data *trg);
void dg_read_trigger(GameWorld *g, FILE *fp, void *proto, int type);
void dg_obj_trigger(GameWorld *g, char *line, struct obj_data *obj);
void assign_triggers(GameWorld *g, void *i, int type);

/* From dg_variables.c */
void add_var(struct trig_var_data **var_list, const char *name, const char *value, long id);
void add_char_uid_var(trig_data *trig, const char *name, const struct char_data *c, long context);
void add_obj_uid_var(trig_data *trig, const char *name, const struct obj_data *o, long context);
void var_subst(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf);

/* From dg_handler.c */
void free_varlist(struct trig_var_data *vd);
struct script_data *dg_ensure_script(struct script_data **slot);
bool remove_var(struct trig_var_data **var_list, char *name);
void free_trigger(GameWorld *g, trig_data *trig);
void extract_trigger(GameWorld *g, struct trig_data *trig);
void extract_script(GameWorld *g, void *thing, int type);
void free_extracted_triggers(GameWorld *g);
void extract_script_mem(struct script_memory *sc);
void free_proto_script(void *thing, int type);
void copy_proto_script(void *source, void *dest, int type);
void delete_variables(const char *charname);
void update_wait_events(struct room_data *to, struct room_data *from);

/* from dg_comm.c */
void sub_write(GameWorld *g, char *arg, char_data *ch, int8_t find_invis, int targets);
void send_to_zone(GameWorld *g, const char *messg, zone_rnum zone);

/* from dg_misc.c */
void do_dg_cast(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                int type, char *cmd);
void do_dg_affect(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd);
bool valid_dg_target(GameWorld *g, char_data *ch, int bitvector);
void script_damage(GameWorld *g, char_data *vict, int dam);
extern const char *const dg_door_fields[];
bool dg_door_apply(GameWorld *g, room_data *rm, int dir, int field, char *value);

int check_flags_by_name_ar(bitvector_t *array, int numflags, char *search, const char *const namelist[]);

/* from dg_objcmd.c */
room_rnum obj_room(obj_data *obj);

/* player id's: 0 to PLAYER_ID_MAX - 1            */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1      */
/* object id's: OBJ_ID_BASE to OBJ_ID_MAX         */
/* mob id's: MOB_ID_BASE to MOB_ID_MAX            */
/* See consts/triggers.h; allocate with dg_new_mob_uid()/dg_new_obj_uid(). */

/* the virtual number of trigger t */
trig_vnum trig_get_vnum(GameWorld *g, const trig_data *t);

/* true if script sc exists and has a trigger of the given type */
static inline bool script_has_type(const struct script_data *sc, bitvector_t type)
{
  return sc && (sc->types & type);
}

/* true if trigger t is of the given type and not already running */
static inline bool trigger_ready(const trig_data *t, bitvector_t type)
{
  return (t->trigger_type & type) && !t->depth;
}

/*
 * The trigger currently being run by script_driver(), so errors can say which
 * trigger, which entity and which line caused them. Everything is copied in
 * because the owner may be purged while its script is still running.
 */
struct dg_run_ctx {
  bool active;
  long long trig_vnum;
  char trig_name[64];
  int owner_type;
  char owner_name[64];
  long long owner_vnum;
  long owner_uid;
  long long room_vnum;
  char line[256];
};

/* find_char() helpers */
// Must be power of 2
constexpr int BUCKET_COUNT = 4096;
// to recognize an empty bucket head; real UIDs are never negative
constexpr long UID_OUT_OF_RANGE = -1;

struct lookup_table_t {
  long uid;
  int type; /* MOB_TRIGGER for characters, OBJ_TRIGGER for objects */
  void *c;
  struct lookup_table_t *next;
};
