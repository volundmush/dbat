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

#include "structs.h"
#include "db.h"
#include "genzon.h"
#include "oasis.h"
#include <variant>

#ifdef _MSC_VER
#define __attribute__(a)
#endif

using DgUID = UID;

std::string toDgUID(const DgUID& uid);

#define DG_SCRIPT_VERSION "DG Scripts 1.0.14"

#define    MOB_TRIGGER   0
#define    OBJ_TRIGGER   1
#define    WLD_TRIGGER   2

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

/* mob trigger types */
#define MTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define MTRIG_RANDOM           (1 << 1)      /* checked randomly           */
#define MTRIG_COMMAND          (1 << 2)         /* character types a command  */
#define MTRIG_SPEECH           (1 << 3)         /* a char says a word/phrase  */
#define MTRIG_ACT              (1 << 4)      /* word or phrase sent to act */
#define MTRIG_DEATH            (1 << 5)      /* character dies             */
#define MTRIG_GREET            (1 << 6)      /* something enters room seen */
#define MTRIG_GREET_ALL        (1 << 7)      /* anything enters room       */
#define MTRIG_ENTRY            (1 << 8)      /* the mob enters a room      */
#define MTRIG_RECEIVE          (1 << 9)      /* character is given obj     */
#define MTRIG_FIGHT            (1 << 10)     /* each pulse while fighting  */
#define MTRIG_HITPRCNT         (1 << 11)     /* fighting and below some hp */
#define MTRIG_BRIBE           (1 << 12)     /* coins are given to mob     */
#define MTRIG_LOAD             (1 << 13)     /* the mob is loaded          */
#define MTRIG_MEMORY           (1 << 14)     /* mob see's someone remembered */
#define MTRIG_CAST             (1 << 15)     /* mob targetted by spell     */
#define MTRIG_LEAVE            (1 << 16)     /* someone leaves room seen   */
#define MTRIG_DOOR             (1 << 17)     /* door manipulated in room   */

#define MTRIG_TIME             (1 << 19)     /* trigger based on specific game hour */
#define MTRIG_HOURLY             (1 << 20)     /* triggered every game hour */
#define MTRIG_QUARTER             (1 << 21)     /* triggered every 15 game minutes */

/* obj trigger types */
#define OTRIG_GLOBAL           (1 << 0)         /* unused                     */
#define OTRIG_RANDOM           (1 << 1)         /* checked randomly           */
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
#define OTRIG_TIME             (1 << 19)     /* trigger based on specific game hour */
#define OTRIG_HOURLY             (1 << 20)     /* triggered every game hour */
#define OTRIG_QUARTER             (1 << 21)     /* triggered every 15 game minutes */

/* wld trigger types */
#define WTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define WTRIG_RANDOM           (1 << 1)         /* checked randomly           */
#define WTRIG_COMMAND          (1 << 2)         /* character types a command  */
#define WTRIG_SPEECH           (1 << 3)      /* a char says word/phrase    */

#define WTRIG_RESET            (1 << 5)      /* zone has been reset        */
#define WTRIG_ENTER            (1 << 6)         /* character enters room      */
#define WTRIG_DROP             (1 << 7)      /* something dropped in room  */

#define WTRIG_CAST             (1 << 15)     /* spell cast in room */
#define WTRIG_LEAVE            (1 << 16)     /* character leaves the room */
#define WTRIG_DOOR             (1 << 17)     /* door manipulated in room  */

#define WTRIG_TIME             (1 << 19)     /* trigger based on specific game hour */
#define WTRIG_HOURLY             (1 << 20)     /* triggered every game hour */
#define WTRIG_QUARTER             (1 << 21)     /* triggered every 15 game minutes */

/* obj command trigger types */
#define OCMD_EQUIP             (1 << 0)         /* obj must be in char's equip */
#define OCMD_INVEN             (1 << 1)         /* obj must be in char's inven */
#define OCMD_ROOM              (1 << 2)         /* obj must be in char's room  */

/* obj consume trigger commands */
#define OCMD_EAT    1
#define OCMD_DRINK  2
#define OCMD_QUAFF  3

#define TRIG_NEW                0         /* trigger starts from top  */
#define TRIG_RESTART            1         /* trigger restarting       */


/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
#define PULSE_DG_SCRIPT         (13 RL_SEC)


#define MAX_SCRIPT_DEPTH      10          /* maximum depth triggers can
					     recurse into each other */

#define SCRIPT_ERROR_CODE     (-9999999)   /* this shouldn't happen too often */

struct trig_proto {
    trig_proto() = default;
    explicit trig_proto(const nlohmann::json& j);
    trig_vnum vn{NOTHING};                    /* trigger's rnum                  */
    int8_t attach_type{};            /* mob/obj/wld intentions          */
    int8_t data_type{};                /* type of game_data for trig      */
    std::string name;
    long trigger_type{};            /* type of trigger (for bitvector) */
    int narg{};                /* numerical argument              */
    std::string arglist{};            /* argument list                   */
    std::vector<std::string> lines;
    std::set<std::shared_ptr<trig_data>> instances;
    nlohmann::json serialize();
};

/* one line of the trigger */
struct cmdlist_element {
    char *cmd{};                /* one line of a trigger */
    struct cmdlist_element *original{};
    struct cmdlist_element *next{};
};

struct trig_var_data {
    trig_var_data() = default;
    explicit trig_var_data(const nlohmann::json& j);
    nlohmann::json serialize();
    char *name{};                /* name of variable  */
    char *value{};                /* value of variable */
    long context{};                /* 0: global context */

    struct trig_var_data *next{};
};

enum class NestType : uint8_t {
    IF = 0,
    WHILE = 1,
    SWITCH = 2
};

enum class DgScriptState : uint8_t {
    DORMANT = 0,
    RUNNING = 1,
    WAITING = 2,
    ERROR = 3,
    DONE = 4,
    PURGED = 5
};

class DgScriptException : public std::exception {
private:
    std::string message;

public:
    // Constructor taking a std::string parameter
    explicit DgScriptException(const std::string& msg) : message(msg) {}

    // Override the what() method to return the error message
    const char* what() const noexcept override {
        return message.c_str();
    }
};

struct HasVars {
    std::unordered_map<std::string, std::string> vars;
    void addVar(const std::string &name, const std::string &value);
    std::string getVar(const std::string& name);
};

/* structure for triggers */
struct trig_data : public HasVars {
    trig_data(struct script_data *sc, std::shared_ptr<trig_proto> parent);

    nlohmann::json serialize();
    std::string serializeLocation();

    int64_t id{};
    time_t generation{};

    std::shared_ptr<trig_proto> parent;
    struct script_data *sc;

    std::vector<std::pair<NestType, std::size_t>> depth; /* depth into nest ifs/whiles/etc  */
    std::size_t lineNumber{0};
    int loops{};                /* loop iteration counter          */
    int totalLoops{};
    double waiting{0.0};    /* event to pause the trigger      */
    bool purged{};            /* trigger is set to be purged     */
    int order{0};
    bool active{false};
    DgScriptState state{DgScriptState::DORMANT};
    void activate();
    void deactivate();

    struct trig_data *next{};
    struct trig_data *next_in_world{};    /* next in the global trigger list */
    void deserialize(const nlohmann::json& j);
    void deserializeLocation(const std::string& txt);

    

    void reset();
    void setState(DgScriptState st);

    int execute();
    int executeBlock(std::size_t start, std::size_t end);
    std::string getLine(std::size_t num);

    bool processIf(const std::string &cond);

    std::string evalExpr(const std::string& expr);
    std::string varSubst(const std::string& expr);
    std::string handleSubst(const std::string& expr);

    std::optional<std::string> evalLhsOpRhs(const std::string& expr);

    std::string evalOp(const std::string& op, const std::string& lhr, const std::string& rhr);

    std::string evalNumericOp(const std::string& op, const std::string &lhs, const std::string &rhs);

    bool truthy(const std::string& expr);

    std::size_t findElseEnd(bool matchElseIf = true, bool matchElse = true);
    std::size_t findEnd();
    std::size_t findDone();
    std::size_t findCase(const std::string& cond);

    void processEval(const std::string& expr);
    void extractValue(const std::string& expr);
    void processContext(const std::string& expr);
    void processGlobal(const std::string& cmd);
    void processRemote(const std::string& cmd);
    void processRdelete(const std::string& cmd);
    void processSet(const std::string& cmd);
    void processUnset(const std::string& cmd);
    void processWait(const std::string& cmd);
    void processAttach(const std::string& cmd);
    void processDetach(const std::string& cmd);

};


/* a complete script (composed of several triggers) */
struct script_data : public HasVars {
    script_data() = default;
    explicit script_data(struct unit_data *u) : script_data() {
        owner = u;
    };
    long types{};                /* bitvector of trigger types */
    struct trig_data *trig_list{};            /* list of triggers           */
    bool purged{};                /* script is set to be purged */
    struct unit_data* owner{};
    struct script_data *next{};        /* used for purged_scripts    */
    void activate();
    void deactivate();

    std::unordered_map<std::string, std::string> vars;
    trig_var_data *getVar(const std::string& name);
};

/* The event data for the wait command */
struct wait_event_data {
    struct trig_data *trigger{};
    void *go{};
    int type{};
};

/* typedefs that the dg functions rely on */
typedef struct index_data index_data;
typedef struct room_data room_data;
typedef struct obj_data obj_data;
typedef struct trig_data trig_data;
typedef struct char_data char_data;

/* used for actor memory triggers */
struct script_memory {
    int64_t id{};                /* id of who to remember */
    char *cmd{};                /* command, or nullptr for generic */
    struct script_memory *next{};
};


/* function prototypes from dg_triggers.c */
extern char *one_phrase(char *arg, char *first_arg);

extern int is_substring(char *sub, char *string);

extern int word_check(char *str, char *wordlist);

extern void act_mtrigger(char_data *ch, char *str, char_data *actor, char_data *victim, obj_data *object,
                         obj_data *target, char *arg);

extern void speech_mtrigger(char_data *actor, char *str);

extern void speech_wtrigger(char_data *actor, char *str);

extern void greet_memory_mtrigger(char_data *ch);

extern int greet_mtrigger(char_data *actor, int dir);

extern int entry_mtrigger(char_data *ch);

extern void entry_memory_mtrigger(char_data *ch);

extern int enter_wtrigger(room_data *room, char_data *actor, int dir);

extern int drop_otrigger(obj_data *obj, char_data *actor);

extern void timer_otrigger(obj_data *obj);

extern int get_otrigger(obj_data *obj, char_data *actor);

extern int drop_wtrigger(obj_data *obj, char_data *actor);

extern int give_otrigger(obj_data *obj, char_data *actor,
                         char_data *victim);

extern int receive_mtrigger(char_data *ch, char_data *actor,
                            obj_data *obj);

extern void bribe_mtrigger(char_data *ch, char_data *actor,
                           int amount);

extern int wear_otrigger(obj_data *obj, char_data *actor, int where);

extern int remove_otrigger(obj_data *obj, char_data *actor);

extern int cmd_otrig(obj_data *obj, char_data *actor, char *cmd,
                     char *argument, int type);

extern int command_mtrigger(char_data *actor, char *cmd, char *argument);

extern int command_otrigger(char_data *actor, char *cmd, char *argument);

extern int command_wtrigger(char_data *actor, char *cmd, char *argument);

extern int death_mtrigger(char_data *ch, char_data *actor);

extern void fight_mtrigger(char_data *ch);

extern void hitprcnt_mtrigger(char_data *ch);

extern void random_mtrigger(char_data *ch);

extern void random_otrigger(obj_data *obj);

extern void random_wtrigger(room_data *ch);

extern void reset_wtrigger(room_data *ch);

extern void load_mtrigger(char_data *ch);

extern void load_otrigger(obj_data *obj);

extern int cast_mtrigger(char_data *actor, char_data *ch, int spellnum);

extern int cast_otrigger(char_data *actor, obj_data *obj, int spellnum);

extern int cast_wtrigger(char_data *actor, char_data *vict, obj_data *obj, int spellnum);

extern int leave_mtrigger(char_data *actor, int dir);

extern int leave_wtrigger(room_data *room, char_data *actor, int dir);

extern int leave_otrigger(room_data *room, char_data *actor, int dir);

extern int door_mtrigger(char_data *actor, int subcmd, int dir);

extern int door_wtrigger(char_data *actor, int subcmd, int dir);

extern int consume_otrigger(obj_data *obj, char_data *actor, int cmd);

extern void time_mtrigger(char_data *ch);
extern void interval_mtrigger(char_data *ch, int trigFlag);

extern void time_otrigger(obj_data *obj);
extern void interval_otrigger(obj_data *obj, int trigFlag);

extern void time_wtrigger(room_data *room);
extern void interval_wtrigger(room_data *room, int trigFlag);

/* function prototypes from dg_scripts.c */
extern char *str_str(char *cs, char *ct);

extern int find_eq_pos_script(char *arg);

extern int can_wear_on_pos(struct obj_data *obj, int pos);

extern char_data *get_char(char *name);

extern char_data *get_char_near_obj(obj_data *obj, char *name);

extern char_data *get_char_in_room(room_data *room, char *name);

extern obj_data *get_obj_near_obj(obj_data *obj, char *name);

extern obj_data *get_obj(char *name);

extern room_data *get_room(char *name);

extern char_data *get_char_by_obj(obj_data *obj, char *name);

extern char_data *get_char_by_room(room_data *room, char *name);

extern obj_data *get_obj_by_obj(obj_data *obj, char *name);

extern obj_data *get_obj_in_room(room_data *room, char *name);

extern obj_data *get_obj_by_room(room_data *room, char *name);

extern int trgvar_in_room(room_vnum vnum);

extern obj_data *get_obj_in_list(char *name, obj_data *list);

extern obj_data *get_object_in_equip(char_data *ch, char *name);

extern void script_trigger_check(uint64_t heartPulse, double deltaTime);

extern void check_time_triggers();

extern void check_interval_triggers(int trigFlag);

extern void find_uid_name(char *uid, char *name, size_t nlen);

extern void do_sstat(struct char_data *ch, struct unit_data *ud);

extern void add_trigger(struct script_data *sc, trig_data *t, int loc);

extern void script_vlog(const char *format, va_list args);

extern void script_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

extern char *matching_quote(char *p);

struct room_data *dg_room_of_obj(struct obj_data *obj);

/* To maintain strict-aliasing we'll have to do this trick with a union */
/* Thanks to Chris Gilbert for reminding me that there are other options. */
extern int script_driver(void *go_adress, trig_data *trig, int type, int mode);

extern trig_rnum real_trigger(trig_vnum vnum);

extern void process_eval(void *go, struct script_data *sc, trig_data *trig,
                         int type, char *cmd);

extern void read_saved_vars(struct char_data *ch);

extern void save_char_vars(struct char_data *ch);

extern void add_to_lookup_table(long uid, void *c);

extern void remove_from_lookup_table(long uid);

/* from dg_db_scripts.c */
extern void parse_trigger(FILE *trig_f, trig_vnum nr);

extern trig_data *read_trigger(int nr);

extern void trig_data_copy(trig_data *this_data, const trig_data *trg);

extern void dg_read_trigger(FILE *fp, struct unit_data *proto, int type);

extern void dg_obj_trigger(char *line, struct obj_data *obj);

extern void assign_triggers(struct unit_data *i, int type);


/* From dg_variables.c */
extern void add_var(struct trig_var_data **var_list, char *name, const char *value, long id);

extern int item_in_list(char *item, obj_data *list);

extern char *skill_percent(struct char_data *ch, char *skill);

extern int char_has_item(char *item, struct char_data *ch);

extern void var_subst(void *go, struct script_data *sc, trig_data *trig,
                      int type, char *line, char *buf);

extern std::optional<std::string> text_processed(char *field, char *subfield, struct trig_var_data *vd);

extern std::string find_replacement(trig_data *trig, char *var, char *field, char *subfield);

/* From dg_handler.c */
extern void free_var_el(struct trig_var_data *var);

extern void free_varlist(struct trig_var_data *vd);

extern int remove_var(struct trig_var_data **var_list, char *name);

extern void free_trigger(trig_data *trig);

extern void extract_trigger(struct trig_data *trig);

extern void extract_script(void *thing, int type);

extern void extract_script_mem(struct script_memory *sc);

extern void free_proto_script(struct unit_data *thing, int type);

extern void copy_proto_script(struct unit_data *source, struct unit_data *dest, int type);

/* from dg_comm.c */
extern char *any_one_name(char *argument, char *first_arg);

extern void sub_write(char *arg, char_data *ch, int8_t find_invis, int targets);

extern void send_to_zone(char *messg, zone_rnum zone);

/* from dg_misc.c */
extern void do_dg_cast(void *go, struct script_data *sc, trig_data *trig,
                       int type, char *cmd);

extern void do_dg_affect(void *go, struct script_data *sc, trig_data *trig,
                         int type, char *cmd);

extern void send_char_pos(struct char_data *ch, int dam);

extern int valid_dg_target(char_data *ch, int bitvector);

extern void script_damage(char_data *vict, int dam);

extern int check_flags_by_name_ar(bitvector_t *array, int numflags, char *search, const char *namelist[]);

/* from dg_objcmd.c */
extern room_rnum obj_room(obj_data *obj);

/* defines for valid_dg_target */
#define DG_ALLOW_GODS (1<<0)

/* Macros for scripts */

#define UID_CHAR   '#'
#define GET_TRIG_NAME(t)          ((t)->parent->name)
#define GET_TRIG_RNUM(t)          ((t)->parent->vn)
#define GET_TRIG_VNUM(t)          ((t)->parent->vn)
#define GET_TRIG_TYPE(t)          ((t)->parent->trigger_type)
#define GET_TRIG_NARG(t)          ((t)->parent->narg)
#define GET_TRIG_ARG(t)           ((t)->parent->arglist)
#define GET_TRIG_VARS(t)          ((t)->var_list)

#define GET_TRIG_DEPTH(t)         ((t)->depth)
#define GET_TRIG_LOOPS(t)         ((t)->loops)

/* player id's: 0 to MOB_ID_BASE - 1            */
/* mob id's: MOB_ID_BASE to ROOM_ID_BASE - 1      */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1    */
/* object id's: OBJ_ID_BASE and higher           */
#define MOB_ID_BASE      50000  /* 50000 player IDNUMS should suffice */
#define ROOM_ID_BASE    1050000 /* 1000000 Mobs */
#define OBJ_ID_BASE     1300000 /* 250000 Rooms */

#define SCRIPT(o)          ((o)->script)
#define SCRIPT_MEM(c)             ((c)->memory)

#define SCRIPT_TYPES(s)          ((s)->types)
#define TRIGGERS(s)          ((s)->trig_list)

#define GET_SHORT(ch)    ((ch)->short_description)


#define SCRIPT_CHECK(go, type)   (SCRIPT(go) && \
                  IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type)   (IS_SET(GET_TRIG_TYPE(t), type) && \
                  !GET_TRIG_DEPTH(t))

extern void ADD_UID_VAR(char *buf, struct trig_data *trig, struct unit_data *thing, char *name, long context);

extern nlohmann::json serializeVars(struct trig_var_data *vd);

extern void deserializeVars(struct trig_var_data **vd, const nlohmann::json& j);