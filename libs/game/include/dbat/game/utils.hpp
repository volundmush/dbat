#pragma once
/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "Log.hpp"
#include "db.hpp"
#include "config.hpp"
#include "races.hpp"
#include "handler.hpp"
#include "spells.hpp"
#include "comm.hpp"
#include "Result.hpp"

#include "Location.hpp"

constexpr int READ_SIZE = 256;

#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))

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

#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
    (((major) << 16) + ((minor) << 8) + (patchlevel))

#define CREATE(result, type, number)  do {\
    if ((number) * sizeof(type) <= 0)    \
        LERROR("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);    \
    if (!((result) = (type *) calloc ((number), sizeof(type))))    \
        { perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
        { perror("SYSERR: realloc failure"); abort(); } } while(0)

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *(string)) ? "an" : "a")

/* public functions in utils.c */
extern char *strlwr(char *s);

extern int roll_aff_duration(int num, int add);

extern int axion_dice(int adjust);

extern void broken_update(uint64_t heartPulse, double deltaTime);

extern int get_flag_by_name(const char *flag_list[], char *flag_name);

std::string add_commas(double X);

extern void basic_mud_vlog(const char *format, va_list args);

extern int touch(const char *path);

/* defines for mudlog() */
constexpr int OFF = 0;
constexpr int BRF = 1;
constexpr int NRM = 2;
constexpr int CMP = 3;
extern void mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));

extern int dice(int number, int size);

extern size_t sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);

extern size_t sprinttype(int type, const char *names[], char *result, size_t reslen);

extern int get_line(FILE *fl, char *buf);

extern int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);

extern time_t mud_time_to_secs(struct time_info_data *now);

extern void core_dump_real(const char *who, int line);

#define core_dump()        core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a !boost::iequals or stricmp exists.
 */

extern char *CAP(char *txt);

extern void point_update(uint64_t heartPulse, double deltaTime);
extern void hunger_update(uint64_t heartPulse, double deltaTime);
extern void relax_update(uint64_t heartPulse, double deltaTime);
extern void auralight_update(uint64_t heartPulse, double deltaTime);
extern void player_misc_update(uint64_t heartPulse, double deltaTime);
extern void kaioken_update(uint64_t heartPulse, double deltaTime);
extern void poison_update(uint64_t heartPulse, double deltaTime);


/* in class.c */
extern int8_t ability_mod_value(int abil);

extern int highest_skill_value(int level, int type, int skill);
/* in races.c */

#define SPELL_ROUTINES(spl)    (spell_info[spl].routines)
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

#define MOON_TIME               (time_info.hours >= 21 || time_info.hours <= 4)
#define MOON_DATE               (time_info.day >= 20 && time_info.day <= 23)
extern bool MOON_TIMECHECK();
bool ETHER_STREAM(Character *ch);

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




/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */


extern int levenshtein_distance(std::string_view s1, std::string_view s2);

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
    
    auto casted = enchantum::cast<typename Container::value_type>(search);
    if (!casted.has_value()) {
        return false;
    }
    return container.contains(casted.value());
}




extern bool is_numeric(const std::string& str);
extern bool is_all_alpha(const std::string& str);


extern std::string format_double(double value);

extern Result<Zone*> getZone(std::string_view arg, Character* ch);
extern Result<std::string> validateZoneName(std::string_view arg, bool checkExists = false, zone_vnum exclude = NOTHING);
extern Result<Room*> getRoom(std::string_view arg, Character* ch);

extern Result<Location> getLocation(std::string_view arg, Character* ch);

extern std::string ansiSafeString(std::string_view arg);
extern std::string replaceStringLine(std::string_view arg, bool enforceNewLine = true);

int file_to_string_alloc(const char *name, char **buf);
int file_to_string(const char *name, char *buf);
