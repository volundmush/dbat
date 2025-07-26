/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once
#include <iostream>
#include "fmt/printf.h"
#include "fmt/ranges.h"
#include "db.h"
#include "races.h"
#include "handler.h"
#include "spells.h"
#include "comm.h"


#ifdef _MSC_VER
#define __attribute__(a)
constexpr size_t PATH_MAX = 4096;
#endif
#ifdef _WIN32
using ssize_t = ptrdiff_t;
#endif

constexpr int READ_SIZE = 256;

/* global variables */
extern FILE *player_fl;

/* public functions in utils.c */
extern void demon_refill_lf(struct char_data *ch, int64_t num);

extern void dispel_ash(struct char_data *ch);

extern char *strlwr(char *s);

extern int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech);

extern int armor_evolve(struct char_data *ch);

extern int has_group(struct char_data *ch);

const char *report_party_health(struct char_data *ch);

extern int know_skill(struct char_data *ch, int skill);

extern int roll_aff_duration(int num, int add);

extern void null_affect(struct char_data *ch, int aff_flag);

extern void
assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis,
              int spd);

extern int sec_roll_check(struct char_data *ch);


extern int64_t physical_cost(struct char_data *ch, int skill);

extern int axion_dice(int adjust);

const char *disp_align(struct char_data *ch);

extern void sense_memory_write(struct char_data *ch, struct char_data *vict);

extern int read_sense_memory(struct char_data *ch, struct char_data *vict);

extern int roll_pursue(struct char_data *ch, struct char_data *vict);

extern void broken_update(uint64_t heartPulse, double deltaTime);

extern bool wearable_obj(struct obj_data *obj);

extern void randomize_eq(struct obj_data *obj);

extern const char *sense_location(struct char_data *ch);

extern const char* sense_location_name(room_vnum roomnum);

extern void handle_evolution(struct char_data *ch, int64_t dmg);

extern int64_t molt_threshold(struct char_data *ch);

extern int cook_element(room_rnum room);

extern void purge_homing(struct char_data *ch);

extern int planet_check(struct char_data *ch, struct char_data *vict);

extern void improve_skill(struct char_data *ch, int skill, int num);

extern double speednar(struct char_data *ch);

extern int64_t gear_exp(struct char_data *ch, int64_t exp);

extern int get_flag_by_name(const char *flag_list[], char *flag_name);

std::string add_commas(double X);

extern char *introd_calc(struct char_data *ch);

template<typename... Args>
void basic_mud_log(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        std::cout << formatted_string << std::endl;
    }
    catch(const std::exception &e) {
        std::cout << "SYSERR: Format error in basic_mud_log: " << e.what() << std::endl;
        std::cout << "Template was: " << format.data() << std::endl;
    }
}

template<typename... Args>
void template_mud_log(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        std::cout << formatted_string << std::endl;
    }
    catch(const std::exception &e) {
        std::cout << "SYSERR: Format error in template_mud_log: " << e.what() << std::endl;
        std::cout << "Template was: " << format.data() << std::endl;
    }
}

extern void basic_mud_vlog(const char *format, va_list args);

extern int touch(const char *path);

extern void mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));

extern int rand_number(int from, int to);

extern int64_t large_rand(int64_t from, int64_t to);

extern int dice(int number, int size);

extern size_t sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);

extern size_t sprinttype(int type, const char *names[], char *result, size_t reslen);

extern int get_line(FILE *fl, char *buf);

extern int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);

extern time_t mud_time_to_secs(struct time_info_data *now);

extern int num_pc_in_room(struct room_data *room);

extern void core_dump_real(const char *who, int line);

extern int room_is_dark(room_rnum room);

extern int count_color_chars(char *string);

extern bool is_sparring(struct char_data *ch);

extern void mob_talk(struct char_data *ch, const char *speech);

extern int block_calc(struct char_data *ch);

extern void reveal_hiding(struct char_data *ch, int type);

std::string processColors(const std::string &txt, int parse, char **choices);
size_t countColors(const std::string &txt);

#define core_dump()        core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */


/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

extern int64_t MAX(int64_t a, int64_t b);

extern int64_t MIN(int64_t a, int64_t b);

extern char *CAP(char *txt);

/* Followers */
extern int num_followers_charmed(struct char_data *ch);

extern void die_follower(struct char_data *ch);

extern void add_follower(struct char_data *ch, struct char_data *leader);

extern void stop_follower(struct char_data *ch);

extern bool circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
extern void add_history(struct char_data *ch, char *msg, int type);

/* in act.movement.c */
extern int do_simple_move(struct char_data *ch, int dir, int following);

extern int perform_move(struct char_data *ch, int dir, int following);

/* in act.item.c */
extern int64_t max_carry_weight(struct char_data *ch);

/* in limits.c */
extern void advance_level(struct char_data *ch);

extern void set_title(struct char_data *ch, char *title);

extern void gain_condition(struct char_data *ch, int condition, int value);

extern void point_update(uint64_t heartPulse, double deltaTime);
extern void hunger_update(uint64_t heartPulse, double deltaTime);
extern void relax_update(uint64_t heartPulse, double deltaTime);
extern void auralight_update(uint64_t heartPulse, double deltaTime);
extern void player_misc_update(uint64_t heartPulse, double deltaTime);
extern void kaioken_update(uint64_t heartPulse, double deltaTime);
extern void poison_update(uint64_t heartPulse, double deltaTime);
extern void update_pos(struct char_data *victim);

/* in class.c */
extern int total_skill_levels(struct char_data *ch, int skill);

extern int8_t ability_mod_value(int abil);

extern int highest_skill_value(int level, int type, int skill);


extern int raise_class_only(struct char_data *ch, int cl, int v);

/* in races.c */
extern int get_size(struct char_data *ch);

extern int get_size_bonus(int sz);

extern int wield_type(int chsize, struct obj_data *weap);


/* various constants *****************************************************/

/* defines for mudlog() */
constexpr int OFF = 0;
constexpr int BRF = 1;
constexpr int NRM = 2;
constexpr int CMP = 3;

/* get_filename() */
constexpr int CRASH_FILE = 0;
constexpr int ETEXT_FILE = 1;
constexpr int ALIAS_FILE = 2;
constexpr int SCRIPT_VARS_FILE = 3;
constexpr int NEW_OBJ_FILES = 4;
constexpr int PLR_FILE = 5;
constexpr int PET_FILE = 6;
constexpr int IMC_FILE = 7; /**< The IMC2 Data for players */
constexpr int USER_FILE = 8; /* User Account System */
constexpr int INTRO_FILE = 9;
constexpr int SENSE_FILE = 10;
constexpr int CUSTOME_FILE = 11;
constexpr int MAX_FILES = 12;

/* breadth-first searching */
constexpr int BFS_ERROR = -1;
constexpr int BFS_ALREADY_THERE = -2;
constexpr int BFS_TO_FAR = -3;
constexpr int BFS_NO_PATH = -4;

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */

constexpr double MUD_TIME_ACCELERATION = 12.0;  // 12 MUD seconds pass per real second.

constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr double MINUTES_PER_HOUR = 60.0;
constexpr double HOURS_PER_DAY = 24.0;
constexpr double DAYS_PER_WEEK = 7.0;
constexpr double DAYS_PER_MONTH = 30.0;
constexpr double MONTHS_PER_YEAR = 12.0;
constexpr double DAYS_PER_YEAR = 365.0;

constexpr double SECS_PER_MINUTE = SECONDS_PER_MINUTE;
constexpr double SECS_PER_HOUR =  (SECONDS_PER_MINUTE*MINUTES_PER_HOUR);
constexpr double SECS_PER_DAY =   (SECS_PER_HOUR*HOURS_PER_DAY);
constexpr double SECS_PER_WEEK =  (SECS_PER_DAY*DAYS_PER_WEEK);
constexpr double SECS_PER_MONTH = (SECS_PER_DAY*DAYS_PER_MONTH);
constexpr double SECS_PER_YEAR =  (SECS_PER_DAY*DAYS_PER_YEAR);
constexpr double SECS_PER_GAME_YEAR = (SECS_PER_MONTH*MONTHS_PER_YEAR);


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *(string)) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
    if ((number) * sizeof(type) <= 0)    \
        basic_mud_log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);    \
    if (!((result) = (type *) calloc ((number), sizeof(type))))    \
        { perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
        { perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'cmtemp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next, cmtemp)    \
   if ((item) == (head))        \
      (head) = (item)->next;        \
   else {                \
      (cmtemp) = head;            \
      while ((cmtemp) && ((cmtemp)->next != (item))) \
     (cmtemp) = (cmtemp)->next;        \
      if (cmtemp)                \
         (cmtemp)->next = (item)->next;    \
   }                    \

#define REMOVE_FROM_DOUBLE_LIST(item, head, next, prev)\
      if((item) == (head))            \
      {                        \
            (head) = (item)->next;        \
            if(head) (head)->prev = nullptr;        \
      }                        \
      else                    \
      {                        \
        temp = head;                \
          while(temp && (temp->next != (item)))    \
            temp = temp->next;            \
             if(temp)                \
            {                    \
               temp->next = (item)->next;        \
               if((item)->next)            \
                (item)->next->prev = temp;    \
            }                    \
      }                        \

/* basic bitvector utils *************************************************/


#define Q_FIELD(x)  ((int) (x) / 32)
#define Q_BIT(x)    (1 << ((x) % 32))

#define IS_SET_AR(var, bit)       ((var)[Q_FIELD(bit)] & Q_BIT(bit))
#define SET_BIT_AR(var, bit)      ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
#define REMOVE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
#define TOGGLE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] = \
                                   (var)[Q_FIELD(bit)] ^ Q_BIT(bit))
#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
    (*(((ch)->player_specials == &dummy_mob) ? (basic_mud_log("OHNO: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)    ((ch)->mobFlags)
#define PLR_FLAGS(ch)    ((ch)->playerFlags)
#define PRF_FLAGS(ch)    ((ch)->pref)
#define AFF_FLAGS(ch)    ((ch)->affect_flags)
#define ADM_FLAGS(ch)    ((ch)->admflags)
#define ROOM_FLAGS(loc)    (world[(loc)].room_flags)
#define SPELL_ROUTINES(spl)    (spell_info[spl].routines)
#define ZONE_FLAGS(rnum)       (zone_table[(rnum)].zone_flags)
#define ZONE_MINLVL(rnum)      (zone_table[(rnum)].min_level)
#define ZONE_MAXLVL(rnum)      (zone_table[(rnum)].max_level)
/* Return the gauntlet highest room for ch */
#define GET_GAUNTLET(ch)    ((ch)->getBaseStat<int>("gauntlet"))

/*
 * See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC.
 * IS_MOB() acts as a VALID_MOB_RNUM()-like function.
 */
#define IS_NPC(ch)    ((ch)->character_flags.get(CharacterFlag::is_npc))
#define IS_MOB(ch)    (IS_NPC(ch) && mob_proto.count(GET_MOB_RNUM(ch)))

template<typename T>
bool MOB_FLAGGED(T* ch, int flag) {
    return ch->mob_flags.get(flag);
};


extern bool AFF_FLAGGED(struct char_data *ch, int flag);
extern bool AFF_FLAGGED(struct npc_proto_data *ch, int flag);

extern bool PLR_FLAGGED(struct char_data *ch, int flag);
extern bool PRF_FLAGGED(struct char_data *ch, int flag);
extern bool ADM_FLAGGED(struct char_data *ch, int flag);

bool WHERE_FLAGGED(room_vnum loc, WhereFlag flag);
bool WHERE_FLAGGED(struct room_data *loc, WhereFlag flag);
bool ROOM_FLAGGED(room_vnum loc, int flag);
bool ROOM_FLAGGED(struct room_data *loc, int flag);

template<typename T>
bool OBJWEAR_FLAGGED(T *obj, int flag) {
    return obj->wear_flags.get(static_cast<WearFlag>(flag));
}

#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
extern bool OBJAFF_FLAGGED(struct obj_data *obj, int flag);
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), VAL_CONTAINER_FLAGS), (flag)))

extern bool OBJ_FLAGGED(struct obj_data *obj, int flag);
#define BODY_FLAGGED(ch, flag) ((ch)->bodyparts.test(flag))
#define ZONE_FLAGGED(rnum, flag)   (zone_table[(rnum)].zone_flags.get((flag)))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch, flag) ((ch)->player_flags.toggle(flag))
#define PRF_TOG_CHK(ch, flag) ((ch)->pref_flags.toggle(flag))
#define ADM_TOG_CHK(ch, flag) ((ch)->admin_flags.toggle(flag))
#define AFF_TOG_CHK(ch, flag) ((ch)->affect_flags.toggle(flag))

/* new define for quick check */
#define DEAD(ch) (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))

/* room utils ************************************************************/


#define SECT(room)    (VALID_ROOM_RNUM(room) ? \
                get_room((room))->sector_type : SECT_INSIDE)
#define ROOM_DAMAGE(room)   (get_room((room))->getDamage())
#define ROOM_EFFECT(room)   (get_room((room))->geffect)
#define ROOM_GRAVITY(room)  (get_room((room))->getGravity())
#define SUNKEN(room)    (ROOM_EFFECT(room) < 0 || SECT(room) == SECT_UNDERWATER)

#define IS_DARK(room)    room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)    (world.contains(rnum) > 0 && rnum != NOWHERE)
#define GET_ROOM_VNUM(rnum) (VALID_ROOM_RNUM(rnum) ? (rnum) : NOWHERE)
#define GET_ROOM_SPEC(room) \
    (VALID_ROOM_RNUM(room) ? get_room((room))->func : nullptr)

/* Minor Planet Defines */
#define PLANET_ZENITH(room) ((GET_ROOM_VNUM(room) >= 3400 && GET_ROOM_VNUM(room) <= 3599) || (GET_ROOM_VNUM(room) >= 62900 && GET_ROOM_VNUM(room) <= 62999) || \
                (GET_ROOM_VNUM(room) == 19600))

/* char utils ************************************************************/


#define IN_ROOM(ch)    ((ch)->in_room)
#define IN_ZONE(ch)   (zone_table[((ch)->getRoom()->zone)].number)
#define GET_WAS_IN(ch)    ((ch)->getBaseStat<room_vnum>("was_in_room"))
#define GET_AGE(ch)     ((ch)->time.currentAge())

#define GET_PC_NAME(ch)    ((ch)->getName())
#define GET_NAME(ch)    (IS_NPC(ch) ? \
             (ch)->getShortDescription() : GET_PC_NAME(ch))
#define GET_TITLE(ch)   ((ch)->desc ? ((ch)->desc->title ? (ch)->desc->title : "[Unset Title]") : "@D[@GNew User@D]")
#define GET_USER_TITLE(d) ((d)->title)
#define GET_PHASE(ch)   ((ch)->getBaseStat<int>("starphase"))
#define GET_MIMIC(ch)   ((ch)->mimic ? (ch)->mimic->getID()+1 : 0)
#define GET_VOICE(ch)   ((ch)->voice)
#define GET_CLAN(ch)    ((ch)->clan)
#define GET_TRANSCLASS(ch) ((ch)->transclass)
#define GET_FEATURE(ch) ((ch)->feature)
#define GET_USER(ch)    ((ch)->desc ? ((ch)->desc->account ? (char*)((ch)->desc->account->name.c_str()) : "NOUSER") : "NOUSER")
#define GET_CRANK(ch)   ((ch)->crank)
#define GET_ADMLEVEL(ch)    ((ch)->getBaseStat<int>("admin_level"))
#define GET_LEVEL(ch)    ((ch)->getBaseStat<int>("level"))

#define GET_CLASS(ch)   ((ch)->sensei)

#define GET_RACE(ch)    ((ch)->race)
#define GET_HAIRL(ch)   ((ch)->getAppearanceStr(Appearance::hair_length))
#define GET_HAIRC(ch)   ((ch)->getAppearanceStr(Appearance::hair_color))
#define GET_HAIRS(ch)   ((ch)->getAppearanceStr(Appearance::hair_style))
#define GET_SKIN(ch)    ((ch)->getAppearanceStr(Appearance::skin_color))
#define GET_EYE(ch)     ((ch)->getAppearanceStr(Appearance::eye_color))
#define GET_HOME(ch)    ((ch)->getBaseStat<room_vnum>("hometown"))
#define GET_WEIGHT(ch)  ((ch)->getEffectiveStat("weight"))
#define GET_HEIGHT(ch)  ((ch)->getEffectiveStat("height"))
#define GET_PC_HEIGHT(ch)    GET_HEIGHT(ch)
#define GET_PC_WEIGHT(ch)    GET_WEIGHT(ch)
#define GET_SEX(ch)    ((ch)->sex)
#define CARRYING(ch)    ((ch)->carrying)
#define CARRIED_BY(ch)  ((ch)->carried_by)
#define GET_RP(ch)      ((ch)->getRPP())
#define GET_SUPPRESS(ch) ((ch)->getBaseStat<int>("suppression"))
#define GET_RDISPLAY(ch) ((ch)->rdisplay)


/*
 * We could define GET_ADD to be ((GET_STR(ch) > 18) ?
 *                                ((GET_STR(ch) - 18) * 10) : 0)
 * but it's better to leave it undefined and fix the places that call
 * GET_ADD to use the new semantics for abilities.
 *                               - Elie Rosenblum 13/Dec/2003
 */
/* The old define: */
/* #define GET_ADD(ch)     ((ch)->aff_abils.str_add) */
#define GET_STR(ch)     ((int)(ch)->getEffectiveStat("strength"))
#define GET_DEX(ch)     ((int)(ch)->getEffectiveStat("agility"))
#define GET_INT(ch)     ((int)(ch)->getEffectiveStat("intelligence"))
#define GET_WIS(ch)     ((int)(ch)->getEffectiveStat("wisdom"))
#define GET_CON(ch)     ((int)(ch)->getEffectiveStat("constitution"))
#define GET_CHA(ch)     ((int)(ch)->getEffectiveStat("speed"))
#define GET_MUTBOOST(ch) (IS_MUTANT(ch) ? (((ch)->mutations.get(Mutation::extreme_speed)) ? (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch)) * 0.3 : 0) : 0)
extern int GET_SPEEDI(struct char_data *ch);
#define GET_SPEEDCALC(ch) (IS_GRAP(ch) ? GET_CHA(ch) : (IS_INFERIOR(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 1.25) : GET_SPEEDVAR(ch)) : GET_SPEEDVAR(ch)))
#define GET_SPEEDBONUS(ch) (IS_ARLIAN(ch) ? AFF_FLAGGED(ch, AFF_SHELL) ? GET_SPEEDVAR(ch) * -0.5 : (IS_MALE(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 0.5) : 0) : 0) : 0)
#define GET_SPEEDVAR(ch) (GET_SPEEDVEM(ch) > GET_CHA(ch) ? GET_SPEEDVEM(ch) : GET_CHA(ch))
#define GET_SPEEDVEM(ch) (GET_SPEEDINT(ch) - (GET_SPEEDINT(ch) * speednar(ch)))
#define IS_GRAP(ch)     (GRAPPLING(ch) || GRAPPLED(ch))
#define GET_SPEEDINT(ch) (IS_BIO(ch) ? ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1200) / 1200) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)) : ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1000) / 1000) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)))
#define IS_INFERIOR(ch) (IS_KONATSU(ch) || IS_DEMON(ch))
#define IS_WEIGHTED(ch) ((ch)->getBaseStat("speednar") < 1.0)


#define GET_EXP(ch)      ((ch)->getBaseStat("experience"))
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
#define SPOILED(ch)       ((ch)->time.played > 86400)
#define GET_DEATH_TYPE(ch) ((ch)->getBaseStat<int>("death_type"))
#define GET_SLEEPT(ch)    ((ch)->getBaseStat<int>("sleeptime"))
#define GET_FOODR(ch)     ((ch)->getBaseStat<int>("food_rejuvenation"))
#define GET_ALT(ch)       ((ch)->getBaseStat<int>("altitude"))
#define GET_CHARGE(ch)    ((ch)->getBaseStat<int64_t>("charge"))
#define GET_CHARGETO(ch)  ((ch)->getBaseStat<int64_t>("chargeto"))
#define GET_ARMOR(ch)     ((ch)->getEffectiveStat<int>("armor"))
#define GET_HIT(ch)      ((ch)->getCurPL())
#define GET_MAX_HIT(ch)      ((ch)->getMaxPL())
#define GET_MAX_MOVE(ch)  ((ch)->getMaxST())
#define GET_MAX_MANA(ch)  ((ch)->getMaxKI())
#define GET_KI(ch)      ((ch)->getCurKI())
#define GET_DROOM(ch)     ((ch)->getBaseStat<room_vnum>("death_room"))
#define GET_SPAM(ch)      ((ch)->getBaseStat<int>("spam"))
#define GET_SHIPROOM(ch)  ((ch)->getBaseStat<room_vnum>("ship_room"))
#define GET_LPLAY(ch)     ((ch)->getBaseStat<time_t>("last_played"))
#define GET_DTIME(ch)     ((ch)->getBaseStat<time_t>("death_time"))
#define GET_RTIME(ch)     ((ch)->getBaseStat<time_t>("rewtime"))
#define GET_DCOUNT(ch)    ((ch)->getBaseStat<int>("death_count"))
#define GET_BOARD(ch, i)  ((ch)->lboard[i])
#define GET_LIMBS(ch, i)  ((ch)->limbs[i])
#define GET_LIMBCOND(ch, i) ((ch)->limb_condition[i])
#define GET_SONG(ch)      ((ch)->getBaseStat<int>("mystic_melody"))
#define GET_BONUS(ch, i)  (false)
#define GET_TRANSCOST(ch, i) ((ch)->transcost[i])
#define COMBO(ch)         ((ch)->getBaseStat<int>("combo"))
#define LASTATK(ch)       ((ch)->getBaseStat<int>("last_attack"))
#define COMBHITS(ch)      ((ch)->getBaseStat<int>("combo_hits"))
#define GET_AURA(ch)      ((ch)->getAppearanceStr(Appearance::aura_color))
#define GET_RADAR1(ch)    ((ch)->getBaseStat<room_vnum>("radar1"))
#define GET_RADAR2(ch)    ((ch)->getBaseStat<room_vnum>("radar2"))
#define GET_RADAR3(ch)    ((ch)->getBaseStat<room_vnum>("radar3"))
#define GET_PING(ch)      ((ch)->getBaseStat<int>("ping"))
#define GET_SLOTS(ch)     ((ch)->getBaseStat<int>("skill_slots"))
#define GET_TGROWTH(ch)   ((ch)->getBaseStat<int>("tail_growth"))
#define GET_RMETER(ch)    ((ch)->rage_meter)
#define GET_PERSONALITY(ch) ((ch)->getBaseStat<int>("personality"))
#define GET_COMBINE(ch)   ((ch)->getBaseStat<int>("combine"))
#define GET_PREFERENCE(ch) ((ch)->getBaseStat<int>("preference"))
#define GET_RELAXCOUNT(ch) ((ch)->getBaseStat<int>("relax_count"))
#define GET_BLESSLVL(ch)  ((ch)->getBaseStat<int>("bless_level"))
#define GET_ASB(ch)       ((ch)->getBaseStat<int>("auto_skill_bonus"))
#define GET_REGEN(ch)     ((ch)->getBaseStat<int>("regen_rate"))
#define GET_BLESSBONUS(ch) (AFF_FLAGGED(ch, AFF_BLESS) ? (GET_BLESSLVL(ch) >= 100 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.1 : GET_BLESSLVL(ch) >= 60 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.05 : GET_BLESSLVL(ch) >= 40 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.02 : 0) : 0)
#define GET_POSELF(ch)    (!IS_NPC(ch) ? PLR_FLAGGED(ch, PLR_POSE) ? GET_SKILL(ch, SKILL_POSE) >= 100 ? 0.15 : GET_SKILL(ch, SKILL_POSE) >= 60 ? 0.1 : GET_SKILL(ch, SKILL_POSE) >= 40 ? 0.05 : 0 : 0 : 0)
#define GET_POSEBONUS(ch) (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * GET_POSELF(ch))
#define GET_LIFEBONUS(ch) (IS_ARLIAN(ch) ? ((GET_MAX_MANA(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) + ((GET_MAX_MOVE(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) : 0)
#define GET_LIFEBONUSES(ch) ((ch)->getBaseStat<int>("lifebonus") > 0 ? (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)) * (((ch)->getBaseStat<int>("lifebonus") + 100) * 0.01) : (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)))
#define GET_LIFEPERC(ch)  ((ch)->getBaseStat<int>("life_percent"))
#define GET_STUPIDKISS(ch) ((ch)->getBaseStat<int>("stupidkiss"))
#define GET_SPEEDBOOST(ch) ((ch)->getBaseStat<int>("speedboost"))
#define GET_BACKSTAB_COOL(ch) ((ch)->getBaseStat<int>("backstab_cooldown"))
#define GET_COOLDOWN(ch)  ((ch)->getBaseStat<int>("concentrate_cooldown"))
#define GET_BARRIER(ch)   ((ch)->getBaseStat<int64_t>("barrier"))
#define GET_GOLD(ch)      ((ch)->getBaseStat<money_t>("money_carried"))
#define GET_KAIOKEN(ch)   ((ch)->getBaseStat<int>("kaioken"))
#define GET_BOOSTS(ch)    ((ch)->getBaseStat<int>("boosts"))

#define GET_FURY(ch)      ((ch)->getBaseStat<int>("fury"))
#define GET_UP(ch)        ((ch)->getBaseStat<int>("upgrade_points"))
#define GET_FORGETING(ch) ((ch)->getBaseStat<int>("forgetting_skill"))
#define GET_FORGET_COUNT(ch) ((ch)->getBaseStat<int>("forget_count"))
#define GET_BANK_GOLD(ch) ((ch)->getBaseStat<money_t>("money_bank"))
#define GET_POLE_BONUS(ch) ((ch)->getBaseStat<int>("pole_bonus"))
#define GET_FISHSTATE(ch)  ((ch)->getBaseStat<int>("fish_state"))
#define GET_FISHD(ch)     ((ch)->getBaseStat<int>("fish_distance"))
#define GET_DAMAGE_MOD(ch) ((ch)->getBaseStat<int>("damage_mod"))
#define GET_SPELLFAIL(ch) ((ch)->getBaseStat<int16_t>("spellfail"))
#define GET_ARMORCHECK(ch) ((ch)->getBaseStat<int16_t>("armorcheck"))
#define GET_ARMORCHECKALL(ch) ((ch)->getBaseStat<int16_t>("armorcheckall"))
#define GET_MOLT_EXP(ch)  ((ch)->getBaseStat<int64_t>("molt_experience"))
#define GET_MOLT_LEVEL(ch) ((ch)->getBaseStat<int>("molt_level"))
#define GET_SDCOOLDOWN(ch) ((ch)->getBaseStat<int>("selfdestruct_cooldown"))
#define GET_INGESTLEARNED(ch) ((ch)->getBaseStat<int>("ingest_learned"))
#define GET_POS(ch)        ((ch)->getBaseStat<int>("position"))
#define GET_IDNUM(ch)        ((ch)->id)
#define IS_CARRYING_W(ch)    ((ch)->getBaseStat("weight_carried"))
#define IS_CARRYING_N(ch)    ((ch)->getInventoryCount())
#define FIGHTING(ch)        ((ch)->fighting)
#define GET_POWERATTACK(ch)    ((ch)->powerattack)
#define GET_GROUPKILLS(ch)    ((ch)->getBaseStat<int>("group_kills"))
#define GET_ALIGNMENT(ch)    ((ch)->getBaseStat<int>("good_evil"))
#define GET_ETHIC_ALIGNMENT(ch)    ((ch)->getBaseStat<int>("law_chaos"))
#define SITS(ch)                ((ch)->sits.lock().get())
#define MINDLINK(ch)            ((ch)->mindlink)
#define LINKER(ch)              ((ch)->getBaseStat<int>("mind_linker"))
#define LASTHIT(ch)             ((ch)->getBaseStat<int>("lasthit"))
#define DRAGGING(ch)            ((ch)->drag)
#define DRAGGED(ch)             ((ch)->dragged)
#define GRAPPLING(ch)           ((ch)->grappling)
#define GRAPPLED(ch)            ((ch)->grappled)
#define GRAPTYPE(ch)            ((ch)->getBaseStat<int>("grapple_type"))
#define GET_ORIGINAL(ch)        ((ch)->original)
#define GET_CLONES(ch)          ((ch)->clones.size())
#define GET_DEFENDER(ch)        ((ch)->defender)
#define GET_DEFENDING(ch)       ((ch)->defending)
#define BLOCKS(ch)              ((ch)->blocks)
#define BLOCKED(ch)             ((ch)->blocked)
#define ABSORBING(ch)           ((ch)->absorbing)
#define ABSORBBY(ch)            ((ch)->absorbby)
#define GET_EAVESDROP(ch)       ((ch)->getBaseStat<room_vnum>("listen_room"))
#define GET_EAVESDIR(ch)        ((ch)->getBaseStat<int>("listen_direction"))
#define GET_ABSORBS(ch)         ((ch)->getBaseStat<int>("absorbs"))
#define GET_LINTEREST(ch)       ((ch)->getBaseStat<time_t>("last_interest"))

#define GET_COND(ch, i)        ((ch)->conditions[(i)])
#define GET_LOADROOM(ch)    ((ch)->getBaseStat<room_vnum>("load_room"))
#define GET_PRACTICES(ch)    ((ch)->getPractices())
#define GET_TRAINSTR(ch)        ((ch)->getBaseStat<int>("train_strength"))
#define GET_TRAININT(ch)        ((ch)->getBaseStat<int>("train_intelligence"))
#define GET_TRAINCON(ch)        ((ch)->getBaseStat<int>("train_constitution"))
#define GET_TRAINWIS(ch)        ((ch)->getBaseStat<int>("train_wisdom"))
#define GET_TRAINAGL(ch)        ((ch)->getBaseStat<int>("train_agility"))
#define GET_TRAINSPD(ch)        ((ch)->getBaseStat<int>("train_speed"))
#define GET_INVIS_LEV(ch)    ((int)(ch)->getBaseStat("invis_level"))
#define GET_WIMP_LEV(ch)    ((ch)->getBaseStat<int>("wimp_level"))
#define GET_FREEZE_LEV(ch)    ((ch)->getBaseStat<int>("freeze_level"))
#define POOFIN(ch)        ((ch)->poofin)
#define POOFOUT(ch)        ((ch)->poofout)
#define GET_OLC_ZONE(ch)    ((ch)->getBaseStat<int>("olc_zone"))
#define GET_LAST_OLC_TARG(ch)    ((ch)->last_olc_targ)
#define GET_LAST_TELL(ch)    ((ch)->getBaseStat<int>("last_tell"))

int16_t GET_SKILL_BONUS(struct char_data *ch, uint16_t skill);
int16_t GET_SKILL_PERF(struct char_data *ch, uint16_t skill);
int16_t GET_SKILL_BASE(struct char_data *ch, uint16_t skill);
int16_t GET_SKILL(struct char_data *ch, uint16_t skill);

void SET_SKILL(struct char_data *ch, uint16_t skill, int16_t val);
void SET_SKILL_BONUS(struct char_data *ch, uint16_t skill, int16_t val);
void SET_SKILL_PERF(struct char_data *ch, uint16_t skill, int16_t val);

#define GET_EQ(ch, i)        ((ch)->equipment[i])

#define GET_MOB_SPEC(ch)    (IS_MOB(ch) ? mob_index[(ch)->getVnum()].func : 0)
#define GET_MOB_RNUM(mob)    ((mob)->getVnum())
#define GET_MOB_VNUM(mob)    ((mob)->getVnum())

#define GET_DEFAULT_POS(ch)    ((ch)->mob_specials.default_pos)
#define MEMORY(ch)        ((ch)->mob_specials.memory)

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

#define CAN_CARRY_W(ch) ((ch)->getEffectiveStat<int>("carry_capacity"))
#define CAN_CARRY_N(ch) (50)
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || PRF_FLAGGED(ch, PRF_HOLYLIGHT) || (ch)->mutations.get(Mutation::infravision) || PLR_FLAGGED(ch, PLR_AURALIGHT))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 50)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -50)
#define IS_LAWFUL(ch)   (GET_ETHIC_ALIGNMENT(ch) >= 350)
#define IS_CHAOTIC(ch)  (GET_ETHIC_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_ENEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define ALIGN_TYPE(ch)    ((IS_GOOD(ch) ? 0 : (IS_EVIL(ch) ? 6 : 3)) + \
                         (IS_LAWFUL(ch) ? 0 : (IS_CHAOTIC(ch) ? 2 : 1)))

#define IN_ARENA(ch)   (GET_ROOM_VNUM(IN_ROOM(ch)) >= 17800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 17874)
#define ARENA_IDNUM(ch) ((ch)->getBaseStat<room_vnum>("arena_watch"))

/* These three deprecated. */
extern void WAIT_STATE(struct char_data *ch, double timeToWait);
#define GET_WAIT_STATE(ch)    ((ch)->getBaseStat("waitTime"))
#define CHECK_WAIT(ch)                (GET_WAIT_STATE(ch) > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */


#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_CEDIT || STATE(d) == CON_PLAYING ||     \
                        STATE(d) == CON_TRIGEDIT || STATE(d) == CON_AEDIT ||    \
                        STATE(d) == CON_GEDIT || STATE(d) == CON_IEDIT ||       \
                        STATE(d) == CON_HEDIT || STATE(d) == CON_NEWSEDIT ||    \
                        STATE(d) == CON_POBJ)
#define IS_INMENU(d)    (STATE(d) == CON_MENU || STATE(d) == CON_EXDESC || STATE(d) == CON_UMENU || STATE(d) == CON_GET_USER || STATE(d) == CON_GET_EMAIL || STATE(d) == CON_CHPWD_GETOLD || STATE(d) == CON_CHPWD_GETNEW || STATE(d) == CON_CHPWD_VRFY || STATE(d) == CON_DELCNF1 || STATE(d) == CON_DELCNF2 || STATE(d) == CON_QRACE || STATE(d) == CON_QCLASS || STATE(d) == CON_CLASS_HELP || STATE(d) == CON_RACE_HELP || STATE(d) == CON_BONUS || STATE(d) == CON_NEGATIVE || STATE(d) == CON_DISTFEA || STATE(d) == CON_HW || STATE(d) == CON_AURA)

#define SENDOK(ch)    (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch)) && \
                      !PLR_FLAGGED((ch), PLR_WRITING))
/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)    ((d)->connected)

/* object utils **********************************************************/

/*
 * Check for NOWHERE or the top array index?
 * If using unsigned types, the top array index will catch everything.
 * If using signed types, NOTHING will catch the majority of bad accesses.
 */
#define VALID_OBJ_RNUM(obj)    (obj_proto.contains(GET_OBJ_RNUM(obj)))

template<typename T>
int64_t GET_OBJ_VAL(T* obj, const std::string& val) {
    return obj->getBaseStat(val);
}
template<typename T>
int64_t SET_OBJ_VAL(T* obj, const std::string& val, int newval) {
    return obj->setBaseStat(val, newval);
}
template<typename T>
int64_t MOD_OBJ_VAL(T* obj, const std::string& val, int mod) {
    return obj->modBaseStat(val, mod);
}

#define GET_OBJ_LEVEL(obj)      ((obj)->getBaseStat<int>("level"))
#define GET_OBJ_PERM(obj)       ((obj)->affect_flags)
#define GET_OBJ_TYPE(obj)    (static_cast<int>((obj)->type_flag))
#define GET_OBJ_COST(obj)    ((obj)->getBaseStat<int>("cost"))
#define GET_OBJ_RENT(obj)    ((obj)->getBaseStat<int>("cost_per_day"))
#define GET_OBJ_EXTRA(obj)    ((obj)->item_flags)
#define GET_OBJ_EXTRA_AR(obj, i)   ((obj)->item_flags.get((i)))
#define GET_OBJ_WEAR(obj)    ((obj)->wear_flags)
#define GET_OBJ_WEIGHT(obj)    ((obj)->getBaseStat<weight_t>("weight"))
#define GET_OBJ_TIMER(obj)    ((obj)->getBaseStat<int>("timer"))
#define SITTING(obj)            ((obj)->sitting.lock().get())
#define GET_OBJ_POSTTYPE(obj)   ((obj)->posttype)
#define GET_OBJ_POSTED(obj)     ((obj)->posted_to)
#define GET_FELLOW_WALL(obj)    ((obj)->fellow_wall)
#define GET_AUCTER(obj)         ((obj)->aucter)
#define GET_CURBID(obj)         ((obj)->curBidder)
#define GET_AUCTERN(obj)        ((obj)->auctname)
#define GET_AUCTIME(obj)        ((obj)->aucTime)
#define GET_BID(obj)            ((obj)->bid)
#define GET_STARTBID(obj)       ((obj)->startbid)
#define FOOB(obj)               ((obj)->foob)
/* Below is used for "homing" ki attacks */
#define TARGET(obj)             ((obj)->target)
#define KICHARGE(obj)           ((obj)->kicharge)
#define KITYPE(obj)             ((obj)->kitype)
#define USER(obj)               ((obj)->user)
#define KIDIST(obj)             ((obj)->distance)
/* Above is used for "homing ki attacks */
#define SFREQ(obj)              ((obj)->scoutfreq)
#define HCHARGE(obj)            (GET_OBJ_VAL((obj), VAL_BED_HTANK_CHARGE))
#define GET_LAST_LOAD(obj)      ((obj)->lload)
#define GET_OBJ_SIZE(obj)    (static_cast<int>((obj)->size))
#define GET_OBJ_RNUM(obj)    ((obj)->getVnum())
#define GET_OBJ_VNUM(obj)    ((obj)->getVnum())
#define GET_OBJ_SPEC(obj)    (VALID_OBJ_RNUM(obj) ? \
                obj_index[GET_OBJ_RNUM(obj)].func : 0)
#define GET_FUEL(obj)           (GET_OBJ_VAL((obj), VAL_VEHICLE_FUEL))
#define GET_FUELCOUNT(obj)      (GET_OBJ_VAL((obj), VAL_VEHICLE_FUELCOUNT))

#define IS_CORPSE(obj)        (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
                    GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == 1)

#define CAN_WEAR(obj, part)    OBJWEAR_FLAGGED((obj), (part))
#define GET_OBJ_MATERIAL(obj)   (GET_OBJ_VAL((obj), VAL_ALL_MATERIAL))
#define GET_OBJ_SHORT(obj)    ((obj)->getShortDescription())

/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
    (((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "his": (GET_SEX(ch)==SEX_FEMALE ? "her" : "their")) :"its")
#define HSSH(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "he" : (GET_SEX(ch)==SEX_FEMALE ? "she" : "they")) : "it")
#define HMHR(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "him": (GET_SEX(ch)==SEX_FEMALE ? "her" : "their")) : "it")
#define MAFE(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "male": (GET_SEX(ch)==SEX_FEMALE ? "female" : "androgynous")) : "questionably gendered")

#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->getName()) ? "An" : "A")
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->getName()) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)    (!AFF_FLAGGED(sub, AFF_BLIND) && !PLR_FLAGGED(sub, PLR_EYEC) && \
   (IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION) || (sub)->mutations.get(Mutation::infravision) || PLR_FLAGGED(sub, PLR_AURALIGHT)) )

#define INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || ((GET_ADMLEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && IMM_CAN_SEE(sub, obj) && (NOT_HIDDEN(obj) || GET_ADMLEVEL(sub) > 0)))

#define NOT_HIDDEN(ch) (!AFF_FLAGGED(ch, AFF_HIDE))
/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) ((!(obj)->carried_by || CAN_SEE((sub), (obj)->carried_by)) && (!(obj)->worn_by || CAN_SEE((sub), (obj)->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) ((LIGHT_OK(sub) || (obj)->carried_by == (sub) || (obj)->worn_by) && INVIS_OK_OBJ((sub), (obj)) && CAN_SEE_OBJ_CARRIER((sub), (obj)))

#define CAN_SEE_OBJ(sub, obj) (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch, obj) ((ch)->canCarryWeight((obj)) && ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj) (CAN_WEAR((obj), ITEM_WEAR_TAKE) && !SITTING(obj) && CAN_CARRY_OBJ((ch),(obj)) && CAN_SEE_OBJ((ch),(obj)))

#define DISG(ch, vict) ((!PLR_FLAGGED(ch, PLR_DISGUISED)) || \
   (PLR_FLAGGED(ch, PLR_DISGUISED) && (GET_ADMLEVEL(vict) > 0 || IS_NPC(vict))))

#define INTROD(ch, vict) ((ch) == (vict) || readIntro((ch), (vict)) == 1 || (IS_NPC(vict) || IS_NPC(ch) || (GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0)))

#define ISWIZ(ch, vict) ((ch) == (vict) || GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0 || IS_NPC(vict) || IS_NPC(ch))

#define PERS(ch, vict) ((DISG(ch, vict) ? (CAN_SEE(vict, ch) ? (INTROD(vict, ch) ? (ISWIZ(ch, vict) ? GET_NAME(ch) :\
                        get_i_name(vict, ch)) : introd_calc(ch)) : "Someone") :\
                        race::getName((ch)->race).c_str()))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
    (obj)->getShortDescription()  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
    fname((obj)->getName()) : "something")


#define EXIT(ch, door)  ((ch)->getRoom()->dir_option[door])
#define SECOND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define THIRD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])
#define W_EXIT(room, num)     (get_room((room))->dir_option[(num)])
#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
             (EXIT(ch,door)->to_room != NOWHERE) && \
             !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define RACE(ch)      ((ch)->juggleRaceName(true).c_str())
#define LRACE(ch)     ((ch)->juggleRaceName(false).c_str())
#define TRUE_RACE(ch) (race::getName((ch)->race).c_str())

#define CLASS_ABBR(ch) (sensei::getAbbr((ch)->sensei).c_str())
#define RACE_ABBR(ch) ((ch)->race->getAbbr().c_str())

#define IS_ROSHI(ch)            (GET_CLASS(ch) == Sensei::roshi)
#define IS_PICCOLO(ch)          (GET_CLASS(ch) == Sensei::piccolo)
#define IS_KRANE(ch)            (GET_CLASS(ch) == Sensei::crane)
#define IS_NAIL(ch)             (GET_CLASS(ch) == Sensei::nail)
#define IS_BARDOCK(ch)          (GET_CLASS(ch) == Sensei::bardock)
#define IS_GINYU(ch)            (GET_CLASS(ch) == Sensei::ginyu)
#define IS_FRIEZA(ch)           (GET_CLASS(ch) == Sensei::frieza)
#define IS_TAPION(ch)           (GET_CLASS(ch) == Sensei::tapion)
#define IS_ANDSIX(ch)           (GET_CLASS(ch) == Sensei::sixteen)
#define IS_DABURA(ch)           (GET_CLASS(ch) == Sensei::dabura)
#define IS_KABITO(ch)           (GET_CLASS(ch) == Sensei::kibito)
#define IS_JINTO(ch)            (GET_CLASS(ch) == Sensei::jinto)
#define IS_TSUNA(ch)            (GET_CLASS(ch) == Sensei::tsuna)
#define IS_KURZAK(ch)           (GET_CLASS(ch) == Sensei::kurzak)

#define GOLD_CARRY(ch)        (GET_LEVEL(ch) < 100 ? (GET_LEVEL(ch) < 50 ? GET_LEVEL(ch) * 10000 : 500000) : 50000000)
#define IS_SHADOW_DRAGON1(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON1_VNUM)
#define IS_SHADOW_DRAGON2(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON2_VNUM)
#define IS_SHADOW_DRAGON3(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON3_VNUM)
#define IS_SHADOW_DRAGON4(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON4_VNUM)
#define IS_SHADOW_DRAGON5(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON5_VNUM)
#define IS_SHADOW_DRAGON6(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON6_VNUM)
#define IS_SHADOW_DRAGON7(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON7_VNUM)
#define CAN_GRAND_MASTER(ch)    (IS_HUMAN(ch))
#define IS_HUMANOID(ch)         (!IS_SERPENT(ch) && !IS_ANIMAL(ch))
#define IS_ROBOT(ch)            (IS_ANDROID(ch) || IS_MECHANICAL(ch))
#define RESTRICTED_RACE(ch)     (IS_MAJIN(ch) || IS_SAIYAN(ch) || IS_BIO(ch) || IS_HOSHIJIN(ch))
#define CHEAP_RACE(ch)          (IS_TRUFFLE(ch) || IS_MUTANT(ch) || IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define SPAR_TRAIN(ch)          (FIGHTING(ch) && !IS_NPC(ch) && (ch)->character_flags.get(CharacterFlag::sparring) &&\
                                 !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->character_flags.get(CharacterFlag::sparring))
#define IS_PTRANS(ch)           (IS_ANDROID(ch) || IS_TRUFFLE(ch) || IS_BIO(ch) || IS_MAJIN(ch))
#define IS_NONPTRANS(ch)        (!IS_PTRANS(ch))
#define OOZARU_RACE(ch)         (IS_SAIYAN(ch) || IS_HALFBREED(ch))
#define IS_TRANSFORMED(ch)      ((ch)->form != Form::base)
#define BIRTH_PHASE             (time_info.day <= 15)
#define LIFE_PHASE              (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE             (!BIRTH_PHASE && !LIFE_PHASE)

#define OOZARU_RACE(ch)         (IS_SAIYAN(ch) || IS_HALFBREED(ch))
#define MOON_TIME               (time_info.hours >= 21 || time_info.hours <= 4)
#define MOON_DATE               (time_info.day >= 20 && time_info.day <= 23)
extern bool MOON_TIMECHECK();
bool ETHER_STREAM(struct char_data *ch);
#define _HAS_LIMB(ch, num, flag) (GET_LIMBCOND((ch), (num)) > 0 || (ch)->character_flags.get((flag)))

#define HAS_ARMS(ch)            ((_HAS_LIMB((ch), 0, CharacterFlag::cyber_right_arm)) || _HAS_LIMB((ch), 1, CharacterFlag::cyber_left_arm) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1 && GRAPTYPE(ch) != 4)))
#define HAS_LEGS(ch)            ((_HAS_LIMB((ch), 2, CharacterFlag::cyber_right_leg)) || _HAS_LIMB((ch), 3, CharacterFlag::cyber_left_leg) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1)))

#define IS_HUMAN(ch)            (GET_RACE(ch) == Race::human)
#define IS_SAIYAN(ch)           (GET_RACE(ch) == Race::saiyan)
#define IS_ICER(ch)             (GET_RACE(ch) == Race::icer)
#define IS_KONATSU(ch)          (GET_RACE(ch) == Race::konatsu)
#define IS_NAMEK(ch)            (GET_RACE(ch) == Race::namekian)
#define IS_MUTANT(ch)           (GET_RACE(ch) == Race::mutant)
#define IS_KANASSAN(ch)         (GET_RACE(ch) == Race::kanassan)
#define IS_HALFBREED(ch)        (GET_RACE(ch) == Race::halfbreed)
#define IS_BIO(ch)              (GET_RACE(ch) == Race::bio_android)
#define IS_ANDROID(ch)          (GET_RACE(ch) == Race::android)
#define IS_DEMON(ch)            (GET_RACE(ch) == Race::demon)
#define IS_MAJIN(ch)            (GET_RACE(ch) == Race::majin)
#define IS_KAI(ch)              (GET_RACE(ch) == Race::kai)
#define IS_TRUFFLE(ch)          (GET_RACE(ch) == Race::tuffle)
#define IS_HOSHIJIN(ch)         (GET_RACE(ch) == Race::hoshijin)
#define IS_ANIMAL(ch)           (GET_RACE(ch) == Race::animal)
#define IS_SAIBA(ch)              (GET_RACE(ch) == Race::saiba)
#define IS_SERPENT(ch)            (GET_RACE(ch) == Race::serpent)
#define IS_OGRE(ch)            (GET_RACE(ch) == Race::ogre)
#define IS_YARDRATIAN(ch)         (GET_RACE(ch) == Race::yardratian)
#define IS_ARLIAN(ch)           (GET_RACE(ch) == Race::arlian)
#define IS_DRAGON(ch)           (GET_RACE(ch) == Race::dragon)
#define IS_MECHANICAL(ch)          (GET_RACE(ch) == Race::mechanical)
#define IS_FAERIE(ch)           (GET_RACE(ch) == Race::spirit)
#define IS_UNDEAD(ch)           (IS_AFFECTED(ch, AFF_UNDEAD))

/* Define Gender More Easily */
#define IS_MALE(ch)             (GET_SEX(ch) == SEX_MALE)
#define IS_FEMALE(ch)           (GET_SEX(ch) == SEX_FEMALE)
#define IS_NEUTER(ch)           (!IS_MALE(ch) && !IS_FEMALE(ch))

#define OUTSIDE(ch)    (OUTSIDE_ROOMFLAG(ch) && OUTSIDE_SECTTYPE(ch))

#define OUTSIDE_ROOMFLAG(ch)    (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS) && \
             !ROOM_FLAGGED(IN_ROOM(ch), ROOM_UNDERGROUND) && \
                          !(ch)->getWhereFlag(WhereFlag::space))

#define OUTSIDE_SECTTYPE(ch)    ((ch->getLocationTileType() != SECT_INSIDE) && \
                         (ch->getLocationTileType() != SECT_UNDERWATER) && \
                          (ch->getLocationTileType() != SECT_IMPORTANT) && \
                           (ch->getLocationTileType() != SECT_SHOP) && \
                            (ch->getLocationTileType() != SECT_SPACE))

#define DIRT_ROOM(ch) (OUTSIDE_SECTTYPE(ch) && ((ch->getLocationTileType() != SECT_WATER_NOSWIM) && \
                       (ch->getLocationTileType() != SECT_WATER_SWIM)))

#define SPEAKING(ch)     ((ch)->getBaseStat<int>("speaking"))

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have nullptr... */




/* defines for fseek */
#ifndef SEEK_SET
constexpr int SEEK_SET = 0;
constexpr int SEEK_CUR = 1;
constexpr int SEEK_END = 2;
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#define CRYPT(a, b) ((char *) crypt((a),(b)))

/*******************  Config macros *********************/

#define CONFIG_CONFFILE         config_info.CONFFILE

#define CONFIG_PK_ALLOWED       config_info.play.pk_allowed
#define CONFIG_PT_ALLOWED       config_info.play.pt_allowed
#define CONFIG_LEVEL_CAN_SHOUT  config_info.play.level_can_shout
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
#define CONFIG_TUNNEL_SIZE      config_info.play.tunnel_size
#define CONFIG_MAX_EXP_GAIN     config_info.play.max_exp_gain
#define CONFIG_MAX_EXP_LOSS     config_info.play.max_exp_loss
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
#define CONFIG_MAX_PC_CORPSE_TIME config_info.play.max_pc_corpse_time
#define CONFIG_IDLE_VOID        config_info.play.idle_void
#define CONFIG_IDLE_RENT_TIME   config_info.play.idle_rent_time
#define CONFIG_IDLE_MAX_LEVEL   config_info.play.idle_max_level
#define CONFIG_DTS_ARE_DUMPS    config_info.play.dts_are_dumps
#define CONFIG_LOAD_INVENTORY   config_info.play.load_into_inventory
#define CONFIG_TRACK_T_DOORS    config_info.play.track_through_doors
#define CONFIG_LEVEL_CAP    config_info.play.level_cap
#define CONFIG_STACK_MOBS    config_info.play.stack_mobs
#define CONFIG_STACK_OBJS    config_info.play.stack_objs
#define CONFIG_MOB_FIGHTING    config_info.play.mob_fighting
#define CONFIG_OK               config_info.play.OK
#define CONFIG_NOPERSON         config_info.play.NOPERSON
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
#define CONFIG_REROLL_PLAYER_CREATION    config_info.play.reroll_player
#define CONFIG_INITIAL_POINTS_POOL    config_info.play.initial_points
#define CONFIG_ENABLE_COMPRESSION    config_info.play.enable_compression
#define CONFIG_ENABLE_LANGUAGES    config_info.play.enable_languages
#define CONFIG_ALL_ITEMS_UNIQUE    config_info.play.all_items_unique
#define CONFIG_EXP_MULTIPLIER    config_info.play.exp_multiplier

/** Crash Saves **/
#define CONFIG_FREE_RENT        config_info.csd.free_rent
#define CONFIG_MAX_OBJ_SAVE     config_info.csd.max_obj_save
#define CONFIG_MIN_RENT_COST    config_info.csd.min_rent_cost
#define CONFIG_AUTO_SAVE        config_info.csd.auto_save
#define CONFIG_AUTOSAVE_TIME    config_info.csd.autosave_time
#define CONFIG_CRASH_TIMEOUT    config_info.csd.crash_file_timeout
#define CONFIG_RENT_TIMEOUT     config_info.csd.rent_file_timeout

/** Room Numbers **/
#define CONFIG_MORTAL_START     config_info.room_nums.mortal_start_room
#define CONFIG_IMMORTAL_START   config_info.room_nums.immort_start_room
#define CONFIG_FROZEN_START     config_info.room_nums.frozen_start_room
#define CONFIG_DON_ROOM_1       config_info.room_nums.donation_room_1
#define CONFIG_DON_ROOM_2       config_info.room_nums.donation_room_2
#define CONFIG_DON_ROOM_3       config_info.room_nums.donation_room_3
#define CONFIG_DEATH_START      config_info.room_nums.death_start_room

/** Game Operation **/
#define CONFIG_DFLT_PORT        config_info.operation.DFLT_PORT
#define CONFIG_DFLT_IP          config_info.operation.DFLT_IP
#define CONFIG_MAX_PLAYING      config_info.operation.max_playing
#define CONFIG_MAX_FILESIZE     config_info.operation.max_filesize
#define CONFIG_MAX_BAD_PWS      config_info.operation.max_bad_pws
#define CONFIG_SITEOK_ALL       config_info.operation.siteok_everyone
#define CONFIG_OLC_SAVE         config_info.operation.auto_save_olc
#define CONFIG_NEW_SOCIALS      config_info.operation.use_new_socials
#define CONFIG_NS_IS_SLOW       config_info.operation.nameserver_is_slow
#define CONFIG_DFLT_DIR         config_info.operation.DFLT_DIR
#define CONFIG_LOGNAME          config_info.operation.LOGNAME
#define CONFIG_MENU             config_info.operation.MENU
#define CONFIG_WELC_MESSG       config_info.operation.WELC_MESSG
#define CONFIG_START_MESSG      config_info.operation.START_MESSG
/** Can players communicate on the IMC channel ? */
#define CONFIG_IMC_ENABLED      config_info.operation.imc_enabled

/** Autowiz **/
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

/** Character Advancement **/
#define CONFIG_ALLOW_MULTICLASS    config_info.advance.allow_multiclass
#define CONFIG_ALLOW_PRESTIGE    config_info.advance.allow_prestige

/** For tick system **/
#define CONFIG_PULSE_VIOLENCE    config_info.ticks.pulse_violence
#define CONFIG_PULSE_MOBILE    config_info.ticks.pulse_mobile
#define CONFIG_PULSE_ZONE    config_info.ticks.pulse_zone
#define CONFIG_PULSE_AUTOSAVE    config_info.ticks.pulse_autosave
#define CONFIG_PULSE_IDLEPWD    config_info.ticks.pulse_idlepwd
#define CONFIG_PULSE_SANITY    config_info.ticks.pulse_sanity
#define CONFIG_PULSE_USAGE    config_info.ticks.pulse_usage
#define CONFIG_PULSE_TIMESAVE    config_info.ticks.pulse_timesave
#define CONFIG_PULSE_CURRENT    config_info.ticks.pulse_current

/** Character Creation Method **/
#define CONFIG_CREATION_METHOD    config_info.creation.method

/* returns the number of spells per slot */
#define GET_BAB(ch)        GET_POLE_BONUS(ch)

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */

extern void admin_set(struct char_data *ch, int value);

#define IS_COLOR_CHAR(c)  ((c) == 'n' || (c) == 'b' || (c) == 'B' || (c) == '(c)' || \
   (c) == '(c)' || (c) == 'g' || (c) == 'G' || (c) == 'm' || (c) == 'M' || (c) == 'r' || \
   (c) == 'R' || (c) == 'y' || (c) == 'Y' || (c) == 'w' || (c) == 'W' || (c) == 'k' || \
   (c) == 'K' || (c) == '0' || (c) == '2' || (c) == '3' || (c) == '4' || (c) == '5' || \
   (c) == '6' || (c) == '7' || (c) == 'o' || (c) == 'e' || (c) == 'u' || (c) == 'l')
#define MOB_LOADROOM(ch)      ((ch)->getBaseStat<room_vnum>("hometown"))  /*hometown not used for mobs*/
#define OBJ_LOADROOM(obj)     ((obj)->room_loaded)

extern int levenshtein_distance(char *s1, char *s2);



template<size_t N>
int check_flags_by_name_ar(const std::bitset<N>& bitvector, int numflags, char *search, const char *namelist[]) {
    int i, item = -1;

    for (i = 0; i < bitvector.size() && item < 0; i++)
        if (!strcmp(search, namelist[i]))
            return bitvector.test(i);

    return false;
}

template<typename Container>
int check_flags_by_name_ar(const Container& container, int numflags, char *search, const char *namelist[]) {
    static_assert(std::is_enum<typename Container::value_type>::value, 
        "Container must contain enum values");
    
    auto casted = magic_enum::enum_cast<typename Container::value_type>(search);
    if (!casted.has_value()) {
        return false;
    }
    return container.contains(casted.value());
}


extern bool spar_friendly(struct char_data *ch, struct char_data *npc);

extern bool is_numeric(const std::string& str);
extern bool is_all_alpha(const std::string& str);

extern void doContinuedTask(char_data* ch);

extern std::string format_double(double value);