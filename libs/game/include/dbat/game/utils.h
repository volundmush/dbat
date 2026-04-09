#pragma once
#include "dbat/db/consts/types.h"
#include "db.h"
#include "races.h"
#include "handler.h"

#define log			basic_mud_log

#define READ_SIZE	256

/* global variables */
extern FILE *player_fl;

/* public functions in utils.c */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);

int masadv(char *tmp, struct char_data *ch);
void demon_refill_lf(struct char_data *ch, int64_t num);
void dispel_ash(struct char_data *ch);

void prune_crlf(char *txt);
int count_metamagic_feats(struct char_data *ch);
int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech);
int armor_evolve(struct char_data *ch);
int has_group(struct char_data *ch);
const char *report_party_health(struct char_data *ch);
int know_skill(struct char_data *ch, int skill);
int roll_aff_duration(int num, int add);
void null_affect(struct char_data *ch, int aff_flag);
void assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd);
int sec_roll_check(struct char_data *ch);
int get_measure(struct char_data *ch, int height, int weight);
int64_t physical_cost(struct char_data *ch, int skill);
int trans_cost(struct char_data *ch, int trans);
int trans_req(struct char_data *ch, int trans);
void customRead(struct descriptor_data *d, int type, char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);
void customCreate(struct descriptor_data *d);
int axion_dice(int adjust);
const char *disp_align(struct char_data *ch);
void senseCreate(struct char_data *ch);
void sense_memory_write(struct char_data *ch, struct char_data *vict);
int read_sense_memory(struct char_data *ch, struct char_data *vict);
int roll_pursue(struct char_data *ch, struct char_data *vict);
void broken_update(void);
int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
char *sense_location(struct char_data *ch);
void handle_evolution(struct char_data *ch, int64_t dmg);
int64_t molt_threshold(struct char_data *ch);
int cook_element(room_rnum room);
void purge_homing(struct char_data *ch);

int planet_check(struct char_data *ch, struct char_data *vict);
void improve_skill(struct char_data *ch, int skill, int num);
double speednar(struct char_data *ch);

int64_t gear_exp(struct char_data *ch, int64_t exp);
int get_flag_by_name(const char *flag_list[], char *flag_name);
char* add_commas(int64_t X);
void trim(char *s);
char *introd_calc(struct char_data *ch);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void	basic_mud_vlog(const char *format, va_list args);
int	touch(const char *path);
void	mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));
void	log_death_trap(struct char_data *ch);
int	rand_number(int from, int to);
int64_t large_rand(int64_t from, int64_t to);
int	dice(int number, int size);
size_t	sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);
size_t	sprinttype(int type, const char *names[], char *result, size_t reslen);
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
time_t	mud_time_to_secs(struct time_info_data *now);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
void    core_dump_real(const char *who, int line);
int	room_is_dark(room_rnum room);
int     count_color_chars(char *string);
void    game_info(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
bool is_sparring(struct char_data *ch);
void mob_talk(struct char_data *ch, const char *speech);
int block_calc(struct char_data *ch);
void reveal_hiding(struct char_data *ch, int type);

#define core_dump()		core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */


/* undefine MAX and MIN so that our functions are used instead */

char *CAP(char *txt);

/* Followers */

/* in act.informative.c */
extern void	look_at_room(room_rnum target_room, struct char_data *ch, int mode);
extern void	add_history(struct char_data *ch, char *msg, int type);

/* in act.movement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in act.item.c */
int64_t	max_carry_weight(struct char_data *ch);

/* in limits.c */


void	point_update(void);

/* in class.c */
char *  class_desc_str(struct char_data *ch, int howlong, int wantthe);
int     total_skill_levels(struct char_data *ch, int skill);
int8_t  ability_mod_value(int abil);
int8_t  dex_mod_capped(const struct char_data *ch);
int	highest_skill_value(int level, int type);
int     calc_penalty_exp(struct char_data *ch, int gain);
int	raise_class_only(struct char_data *ch, int cl, int v);

/* in races.c */
int	get_size(struct char_data *ch);
int get_size_bonus(int sz);
int wield_type(int chsize, const struct obj_data *weap);


/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2
#define SCRIPT_VARS_FILE 3
#define NEW_OBJ_FILES   4
#define PLR_FILE        5
#define PET_FILE        6
#define USER_FILE       8 /* User Account System */
#define INTRO_FILE      9
#define SENSE_FILE      10
#define CUSTOME_FILE    11
#define MAX_FILES       12

/* breadth-first searching */
#define BFS_ERROR		(-1)
#define BFS_ALREADY_THERE	(-2)
#define BFS_TO_FAR              (-3)
#define BFS_NO_PATH		(-4)

/* basic bitvector utils *************************************************/

/*
 * See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC.
 * IS_MOB() acts as a VALID_MOB_RNUM()-like function.
 */





/* object utils **********************************************************/

/*
 * Check for NOWHERE or the top array index?
 * If using unsigned types, the top array index will catch everything.
 * If using signed types, NOTHING will catch the majority of bad accesses.
 */


/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))



/* Various macros building up to CAN_SEE */

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

#if !defined(YES)
#define YES 1
#endif

#if !defined(NO)
#define NO 0
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif


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
#define CONFIG_LEVEL_CAP	config_info.play.level_cap
#define CONFIG_STACK_MOBS	config_info.play.stack_mobs
#define CONFIG_STACK_OBJS	config_info.play.stack_objs
#define CONFIG_MOB_FIGHTING	config_info.play.mob_fighting
#define CONFIG_OK               config_info.play.OK
#define CONFIG_NOPERSON         config_info.play.NOPERSON
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
#define CONFIG_REROLL_PLAYER_CREATION	config_info.play.reroll_player
#define CONFIG_INITIAL_POINTS_POOL	config_info.play.initial_points
#define CONFIG_ENABLE_COMPRESSION	config_info.play.enable_compression
#define CONFIG_ENABLE_LANGUAGES	config_info.play.enable_languages
#define CONFIG_ALL_ITEMS_UNIQUE	config_info.play.all_items_unique
#define CONFIG_EXP_MULTIPLIER	config_info.play.exp_multiplier

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


  /** Autowiz **/
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

  /** Character Advancement **/
#define CONFIG_ALLOW_MULTICLASS	config_info.advance.allow_multiclass
#define CONFIG_ALLOW_PRESTIGE	config_info.advance.allow_prestige

  /** For tick system **/
#define CONFIG_PULSE_VIOLENCE	config_info.ticks.pulse_violence
#define CONFIG_PULSE_MOBILE	config_info.ticks.pulse_mobile
#define CONFIG_PULSE_ZONE	config_info.ticks.pulse_zone
#define CONFIG_PULSE_AUTOSAVE	config_info.ticks.pulse_autosave
#define CONFIG_PULSE_IDLEPWD	config_info.ticks.pulse_idlepwd
#define CONFIG_PULSE_SANITY	config_info.ticks.pulse_sanity
#define CONFIG_PULSE_USAGE	config_info.ticks.pulse_usage
#define CONFIG_PULSE_TIMESAVE	config_info.ticks.pulse_timesave
#define CONFIG_PULSE_CURRENT	config_info.ticks.pulse_current

  /** Character Creation Method **/
#define CONFIG_CREATION_METHOD	config_info.creation.method


/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */


void admin_set(struct char_data *ch, int value);
#define GET_PAGE_LENGTH(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->page_length))
#define IS_COLOR_CHAR(c)  (c == 'n' || c == 'b' || c == 'B' || c == 'c' || \
   c == 'C' || c == 'g' || c == 'G' || c == 'm' || c == 'M' || c == 'r' || \
   c == 'R' || c == 'y' || c == 'Y' || c == 'w' || c == 'W' || c == 'k' || \
   c == 'K' || c == '0' || c == '2' || c == '3' || c == '4' || c == '5' || \
   c == '6' || c == '7' || c == 'o' || c == 'e' || c == 'u' || c == 'l') 



int     levenshtein_distance(char *s1, char *s2);
