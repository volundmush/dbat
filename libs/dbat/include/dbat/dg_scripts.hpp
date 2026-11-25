/**************************************************************************
*  File: dg_scripts.hpp                                                     *
*  Usage: header file for script structures and constants, and            *
*         function prototypes for dg_scripts.c                            *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#pragma once
#include "dbat/name_table.hpp"
#include <span>
#include <string>
#include <string_view>
#include "dbat/format.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/pulse.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/triggers.hpp"
#include "dbat/dg/owner.hpp"
#include "dbat/dg/program.hpp"
#include "dbat/dg/run.hpp"
#include "dbat/dg/vars.hpp"
#include "dbat/index.hpp"
#include "dbat/db.hpp"
#include "dbat/genzon.hpp"
#include "dbat/registry.hpp"

#define DG_SCRIPT_VERSION "DG Scripts 1.0.14"

/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
constexpr int PULSE_DG_SCRIPT = 13 * PASSES_PER_SEC;

/* The longest a script variable's value can be: what a substituted script line can hold. */
constexpr size_t MAX_DG_VAR_LENGTH = MAX_INPUT_LENGTH - 1;

/* structure for triggers */
struct trig_data {
  struct index_data *index{};           /* its prototype's entry */
  uint8_t attach_type{};                /* mob/obj/wld intentions          */
  uint8_t data_type{};                  /* type of game_data for trig      */
  std::string name{};                   /* name of trigger                 */
  bitvector_t trigger_type{};           /* type of trigger (for bitvector) */
  std::shared_ptr<const dg::program> program{}; /* its script, shared with its prototype */
  dg::run_state run{};                  /* where a run is, while one is going or waiting */
  bool refusal_logged{};                /* on the prototype: that it does not compile, when it first would have run */
  int narg{};                           /* numerical argument              */
  std::string arglist{};                /* argument list                   */
  struct event *wait_event{};           /* event to pause the trigger      */
  bool purged{};                        /* trigger is set to be purged     */
  dg::var_store var_list{};             /* its local variables             */

  struct trig_data *next{};
  long id{};                         /* in g->triggers, while attached to something; 0 if not */
  bool extracted{};                  /* removed while a script was running: see free_extracted_triggers() */
  bool owner_gone{};                 /* and what it belonged to went too */
};

/* a complete script (composed of several triggers) */
struct script_data {
  bitvector_t types{};                 /* bitvector of trigger types */
  struct trig_data *trig_list{};       /* list of triggers           */
  dg::var_store global_vars{};         /* its global variables       */
  bool purged{};                       /* script is set to be purged */
  long context{};                      /* current context for statics */

  struct script_data *next{}; /* used for purged_scripts    */
};

/* The event data for the wait command: the trigger and what it runs on, found again when the wait is over */
struct wait_event_data {
  long trigger_id{}; /* in g->triggers */
  script_owner owner{};
};

/* typedefs that the dg functions rely on */

typedef struct room_data room_data;
typedef struct obj_data obj_data;
typedef struct trig_data trig_data;
typedef struct char_data char_data;

/* used for actor memory triggers */
struct script_memory {
  long id{};   /* id of who to remember */
  std::string cmd{}; /* command, or empty for generic */
  struct script_memory *next{};
};

ACMD(do_attach);
ACMD(do_detach);
ACMD(do_vdelete);
ACMD(do_tstat);
ACMD(do_tcheck);

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

void act_mtrigger(GameWorld *g, const char_data *ch, std::string_view str, char_data *actor, const char_data *victim, obj_data *object, const obj_data *target, std::string_view /* unused */ = {});
void speech_mtrigger(GameWorld *g, char_data *actor, std::string_view str);
void speech_wtrigger(GameWorld *g, char_data *actor, std::string_view str);
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

bool command_mtrigger(GameWorld *g, char_data *actor, std::string_view cmd, std::string_view argument);
bool command_otrigger(GameWorld *g, char_data *actor, std::string_view cmd, std::string_view argument);
int command_wtrigger(GameWorld *g, char_data *actor, std::string_view cmd, std::string_view argument);

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

int consume_otrigger(GameWorld *g, obj_data *obj, char_data *actor, consume_cmd cmd);

void time_mtrigger(GameWorld *g, char_data *ch);
void time_otrigger(GameWorld *g, obj_data *obj);
void time_wtrigger(GameWorld *g, room_data *room);

/* function prototypes from dg_scripts.c */
bool str_str(std::string_view cs, std::string_view ct);
int find_eq_pos_script(std::string_view arg);
bool can_wear_on_pos(struct obj_data *obj, int pos);
struct char_data *find_char(GameWorld *g, long n);
/* find_char() and find_obj() without their log line, for code that expects the entity may be gone */
struct char_data *char_by_id(GameWorld *g, long id);
struct obj_data *obj_by_id(GameWorld *g, long id);
/* Make an entity findable by its id, and part of each_char()/each_obj(), as the newest; or stop that. Unregistering
 * removes the entity only if it is the one registered under its id. */
void char_register(GameWorld *g, struct char_data *ch);
void char_unregister(GameWorld *g, struct char_data *ch);
void obj_register(GameWorld *g, struct obj_data *obj);
void obj_unregister(GameWorld *g, struct obj_data *obj);
char_data *get_char(GameWorld *g, std::string_view name);
char_data *get_char_near_obj(GameWorld *g, obj_data *obj, std::string_view name);
char_data *get_char_in_room(GameWorld *g, room_data *room, std::string_view name);
obj_data *get_obj_near_obj(GameWorld *g, obj_data *obj, std::string_view name);
obj_data *get_obj(GameWorld *g, std::string_view name);
room_data *get_room(GameWorld *g, std::string_view name);
char_data *get_char_by_obj(GameWorld *g, obj_data *obj, std::string_view name);
char_data *get_char_by_room(GameWorld *g, room_data *room, std::string_view name);
obj_data *get_obj_by_obj(GameWorld *g, obj_data *obj, std::string_view name);
obj_data *get_obj_in_room(GameWorld *g, room_data *room, std::string_view name);
obj_data *get_obj_by_room(GameWorld *g, room_data *room, std::string_view name);
int trgvar_in_room(GameWorld *g, room_vnum vnum);
obj_data *get_obj_in_list(GameWorld *g, std::string_view name, const id_list &list);
obj_data *get_object_in_equip(GameWorld *g, char_data *ch, std::string_view name);
void script_trigger_check(GameWorld *g);
void check_time_triggers(GameWorld *g);
/* the name of what uid (a UID variable's value) points to, or that it was not found */
std::string find_uid_name(GameWorld *g, std::string_view uid);
void do_sstat_room(GameWorld *g, struct char_data *ch, struct room_data *rm);
void do_sstat_object(GameWorld *g, char_data *ch, obj_data *j);
void do_sstat_character(GameWorld *g, char_data *ch, char_data *k);
void add_trigger(GameWorld *g, struct script_data *sc, trig_data *t, int loc);

/* Logs why trig's script does not compile, and tells d too if it is not null */
void report_compile_errors(GameWorld *g, const trig_data *trig, struct descriptor_data *d);
/* script_log(): the text as it is */
void script_log(GameWorld *g, std::string_view text);
/* script_log(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void script_log(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  script_log(g, str_format(fmt, std::forward<Args>(args)...));
}
struct room_data *dg_room_of_obj(GameWorld *g, struct obj_data *obj);
void obj_command_interpreter(GameWorld *g, obj_data *obj, std::string_view argument);
void wld_command_interpreter(GameWorld *g, room_data *room, std::string_view argument);

/* What running a trigger came to: its return value, and whether what it ran on was purged */
struct run_result {
  int value{};
  bool owner_gone{};
};

/* Runs trig on go, from the top (TRIG_NEW) or from where it waited (TRIG_RESTART). */
run_result script_driver(GameWorld *g, owner_ptr go, trig_data *trig, int mode);
struct index_data *find_trigger(GameWorld *g, trig_vnum vnum);
obj_data *find_obj(GameWorld *g, long n);
void process_eval(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view cmd);
void read_saved_vars(struct char_data *ch);
void save_char_vars(GameWorld *g, struct char_data *ch);
long dg_new_mob_uid(GameWorld *g);
long dg_new_obj_uid(GameWorld *g);
void dg_log_uid_stats(GameWorld *g);

/* from dg_db_scripts.c */
trig_data *read_trigger(GameWorld *g, struct index_data *nr);
void trig_data_copy(trig_data *this_data, const trig_data *trg);
/* A prototype's "T <vnum>" line, from its world file: the trigger goes on its script list */
void dg_attach_proto_trigger(GameWorld *g, std::string_view line, owner_ptr proto);
void assign_triggers(GameWorld *g, owner_ptr i);

/* From dg_variables.c */
void add_var(dg::var_store &vars, std::string_view name, std::string_view value, long id);
void add_char_uid_var(trig_data *trig, std::string_view name, const struct char_data *c, long context);
void add_obj_uid_var(trig_data *trig, std::string_view name, const struct obj_data *o, long context);
/* text with its variables' values, compiled and substituted as a command line is */
std::string var_subst(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view text);

/* From dg_handler.c */
struct script_data *dg_ensure_script(struct script_data **slot);
void free_trigger(GameWorld *g, trig_data *trig);
void extract_trigger(GameWorld *g, struct trig_data *trig);
void extract_script(GameWorld *g, owner_ptr thing);
void free_extracted_triggers(GameWorld *g);
void extract_script_mem(struct script_memory *sc);
/* Where thing keeps the vnums of the triggers it starts with */
std::vector<trig_vnum> &proto_script_of(owner_ptr thing);
void copy_proto_script(owner_ptr source, owner_ptr dest); /* of the same kind */
void delete_variables(std::string_view charname);

/* from dg_comm.c */
enum class act_to : uint8_t; /* comm.hpp */
void sub_write(GameWorld *g, std::string_view arg, char_data *ch, act_to targets);
void send_to_zone(GameWorld *g, std::string_view messg, struct zone_data *zone);

/* from dg_misc.c */
void do_dg_cast(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view cmd);
void do_dg_affect(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view cmd);
bool valid_dg_target(GameWorld *g, char_data *ch, dg_gods gods);
void script_damage(GameWorld *g, char_data *vict, int dam);
constexpr int NUM_DOOR_FIELDS = 6; /* what dg_door_apply() can change */
extern const name_table dg_door_fields; /* NUM_DOOR_FIELDS of them */
bool dg_door_apply(GameWorld *g, room_data *rm, int dir, int field, std::string_view value);

/* from dg_objcmd.c */
room_vnum obj_room(obj_data *obj);

/* player id's: 0 to PLAYER_ID_MAX - 1            */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1      */
/* object id's: OBJ_ID_BASE to OBJ_ID_MAX         */
/* mob id's: MOB_ID_BASE to MOB_ID_MAX            */
/* See consts/triggers.hpp; allocate with dg_new_mob_uid()/dg_new_obj_uid(). */

/* the virtual number of trigger t */
trig_vnum trig_get_vnum(GameWorld *g, const trig_data *t);

/* true if script sc exists and has a trigger of one of these types */
template <trig_family E> bool script_has_type(const struct script_data *sc, const flag_set<E> &types)
{
  return sc && trig_word_has(sc->types, types);
}
template <trig_family E> bool script_has_type(const struct script_data *sc, E type)
{
  return script_has_type(sc, flag_set<E>{type});
}

/* true if trigger t has any of the types in word (its types word, of whatever family), isn't already running, and
   compiles (which it logs, once, if it doesn't) */
bool trigger_ready(GameWorld *g, trig_data *t, bitvector_t word);

/*
 * The trigger currently being run by script_driver(), so errors can say which
 * trigger, which entity and which line caused them. Everything is copied in
 * because the owner may be purged while its script is still running.
 */
struct dg_run_ctx {
  bool active{};
  long long trig_vnum{};
  std::string trig_name;
  int owner_type{};
  std::string owner_name;
  long long owner_vnum{};
  long owner_uid{};
  long long room_vnum{};
  long instance_id{};
  int line_number{}; /* 0 before the first line */
  std::string line;
  std::string actor;
  long actor_uid{}; /* 0 if none */
};

