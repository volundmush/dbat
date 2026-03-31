#pragma once
#include "dbat/db/dgscripts.h"
#include "dbat/db/characters.h"
#include "dbat/db/objects.h"
#include "dbat/db/rooms.h"

#include <stdarg.h>
#include <stdio.h>

#include "genzon.h"
#include "oasis.h"


/* function prototypes from dg_triggers.c */
char *one_phrase(char *arg, char *first_arg);
int is_substring(char *sub, char *string);
int word_check(char *str, char *wordlist);

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

int cmd_otrig(obj_data *obj, char_data *actor, char *cmd,
              char *argument, int type);
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
obj_data *get_obj_in_list(char *name, obj_data *list);
obj_data *get_object_in_equip(char_data * ch, char *name);
int trgvar_in_room(room_vnum vnum);
void script_trigger_check(void);
void check_time_triggers(void);
void find_uid_name(char *uid, char *name, size_t nlen);
void do_sstat_room(struct char_data * ch, struct room_data *rm);
void do_sstat_object(char_data *ch, obj_data *j);
void do_sstat_character(char_data *ch, char_data *k);
void add_trigger(struct script_data *sc, trig_data *t, int loc);
void script_vlog(const char *format, va_list args);
void script_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
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
void add_to_lookup_table(long uid, void *c);
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
int item_in_list(char *item, obj_data *list);
char *skill_percent(struct char_data *ch, char *skill);
int char_has_item(char *item, struct char_data *ch);
void var_subst(void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf);
int text_processed(char *field, char *subfield, struct trig_var_data *vd,
                   char *str, size_t slen);
void find_replacement(void *go, struct script_data *sc, trig_data *trig,
                int type, char *var, char *field, char *subfield, char *str, size_t slen);


/* From dg_handler.c */
void free_var_el(struct trig_var_data *var);
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
char *any_one_name(char *argument, char *first_arg);
void sub_write(char *arg, char_data *ch, int8_t find_invis, int targets);
void send_to_zone(char *messg, zone_rnum zone);

/* from dg_misc.c */
void do_dg_cast(void *go, struct script_data *sc, trig_data *trig,
    int type, char *cmd);
void do_dg_affect(void *go, struct script_data *sc, trig_data *trig,
    int type, char *cmd);
void send_char_pos(struct char_data *ch, int dam);
int valid_dg_target(char_data *ch, int bitvector);
void script_damage(char_data *vict, int dam);

int check_flags_by_name_ar(int *array, int numflags, char *search, const char *namelist[]);

/* from dg_objcmd.c */
room_rnum obj_room(obj_data *obj);

/* defines for valid_dg_target */
#define DG_ALLOW_GODS (1<<0)

/* Macros for scripts */

#define UID_CHAR   '}'
#define GET_TRIG_NAME(t)          ((t)->name)
#define GET_TRIG_RNUM(t)          ((t)->nr)
#define GET_TRIG_VNUM(t)	  (trig_index[(t)->nr]->vnum)
#define GET_TRIG_TYPE(t)          ((t)->trigger_type)
#define GET_TRIG_DATA_TYPE(t)	  ((t)->data_type)
#define GET_TRIG_NARG(t)          ((t)->narg)
#define GET_TRIG_ARG(t)           ((t)->arglist)
#define GET_TRIG_VARS(t)	  ((t)->var_list)
#define GET_TRIG_WAIT(t)	  ((t)->wait_event)
#define GET_TRIG_DEPTH(t)         ((t)->depth)
#define GET_TRIG_LOOPS(t)         ((t)->loops)

/* player id's: 0 to MOB_ID_BASE - 1            */
/* mob id's: MOB_ID_BASE to ROOM_ID_BASE - 1      */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1    */
/* object id's: OBJ_ID_BASE and higher           */
#define MOB_ID_BASE	  50000  /* 50000 player IDNUMS should suffice */
#define ROOM_ID_BASE    1050000 /* 1000000 Mobs */
#define OBJ_ID_BASE     1300000 /* 250000 Rooms */

#define SCRIPT(o)		  ((o)->script)
#define SCRIPT_MEM(c)             ((c)->memory)

#define SCRIPT_TYPES(s)		  ((s)->types)
#define TRIGGERS(s)		  ((s)->trig_list)

#define GET_SHORT(ch)    ((ch)->short_descr)


#define SCRIPT_CHECK(go, type)   (SCRIPT(go) && \
				  IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type)   (IS_SET(GET_TRIG_TYPE(t), type) && \
				  !GET_TRIG_DEPTH(t))

#define ADD_UID_VAR(buf, trig, go, name, context) do { \
		         sprintf(buf, "%c%d", UID_CHAR, GET_ID(go)); \
                         add_var(&GET_TRIG_VARS(trig), name, buf, context); } while (0)

