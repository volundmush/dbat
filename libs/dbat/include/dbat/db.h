#pragma once
#include "dbat/consts/types.h"
#include <stdio.h>

/* arbitrary constants used by index_boot() (must be unique) */
constexpr int DB_BOOT_WLD = 0;
constexpr int DB_BOOT_MOB = 1;
constexpr int DB_BOOT_OBJ = 2;
constexpr int DB_BOOT_ZON = 3;
constexpr int DB_BOOT_SHP = 4;
constexpr int DB_BOOT_HLP = 5;
constexpr int DB_BOOT_TRG = 6;
constexpr int DB_BOOT_GLD = 7;

#define LIB_USER      "user/"
#define LIB_INTRO     "intro/"
#define LIB_SENSE     "sense/"
#define LIB_WORLD     "world/"
#define LIB_TEXT      "text/"
#define LIB_TEXT_HELP "text/help/"
#define LIB_MISC      "misc/"
#define LIB_ETC       "etc/"
#define LIB_PLRTEXT   "plrtext/"
#define LIB_PLROBJS   "plrobjs/"
#define LIB_PLRVARS   "plrvars/"
#define LIB_PLRALIAS  "plralias/"
#define LIB_PLRFILES  "plrfiles/"
#define LIB_HOUSE     "house/"
#define SLASH         "/"

#define SUF_OBJS   "new"
#define SUF_TEXT   "text"
#define SUF_ALIAS  "alias"
#define SUF_MEM    "mem"
#define SUF_PLR    "plr"
#define SUF_PET    "pet"
#define SUF_USER   "usr"
#define SUF_INTRO  "itr"
#define SUF_SENSE  "sen"
#define SUF_CUSTOM "cus"

#define FASTBOOT_FILE   "../.fastboot"   /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript" /* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"       /* autorun: don't restart mud   */

/* names of various files and directories */
#define INDEX_FILE  "index"               /* index of world files		*/
#define MINDEX_FILE "index.mini"          /* ... and for mini-mud-mode	*/
#define WLD_PREFIX  LIB_WORLD "wld" SLASH /* room definitions	*/
#define MOB_PREFIX  LIB_WORLD "mob" SLASH /* monster prototypes	*/
#define OBJ_PREFIX  LIB_WORLD "obj" SLASH /* object prototypes	*/
#define ZON_PREFIX  LIB_WORLD "zon" SLASH /* zon defs & command tables */
#define SHP_PREFIX  LIB_WORLD "shp" SLASH /* shop definitions	*/
#define HLP_PREFIX  LIB_TEXT "help" SLASH /* for HELP <keyword>	*/
#define TRG_PREFIX  LIB_WORLD "trg" SLASH /* trigger files	*/
#define GLD_PREFIX  LIB_WORLD "gld" SLASH /* guild files		*/

#define CREDITS_FILE      LIB_TEXT "credits"      /* for the 'credits' command	*/
#define NEWS_FILE         LIB_TEXT "news"         /* for the 'news' command	*/
#define MOTD_FILE         LIB_TEXT "motd"         /* messages of the day / mortal	*/
#define IMOTD_FILE        LIB_TEXT "imotd"        /* messages of the day / immort	*/
#define GREETINGS_FILE    LIB_TEXT "greetings"    /* The opening screen.	*/
#define GREETANSI_FILE    LIB_TEXT "greetansi"    /* The opening screen.	*/
#define HELP_PAGE_FILE    LIB_TEXT_HELP "screen"  /* for HELP <CR>	*/
#define CONTEXT_HELP_FILE LIB_TEXT "contexthelp"  /* The opening screen.	*/
#define INFO_FILE         LIB_TEXT "info"         /* for INFO		*/
#define WIZLIST_FILE      LIB_TEXT "wizlist"      /* for WIZLIST		*/
#define IMMLIST_FILE      LIB_TEXT "immlist"      /* for IMMLIST		*/
#define BACKGROUND_FILE   LIB_TEXT "background"   /* for the background story	*/
#define POLICIES_FILE     LIB_TEXT "policies"     /* player policies/rules	*/
#define HANDBOOK_FILE     LIB_TEXT "handbook"     /* handbook for new immorts	*/
#define IHELP_PAGE_FILE   LIB_TEXT_HELP "iscreen" /* for HELP <CR>	*/
#define HELP_FILE         "help.hlp"

#define IDEA_FILE        LIB_MISC "ideas"       /* for the 'idea'-command	*/
#define TYPO_FILE        LIB_MISC "typos"       /*         'typo'		*/
#define BUG_FILE         LIB_MISC "bugs"        /*         'bug'		*/
#define REQUEST_FILE     LIB_MISC "request"     /*      RPP Requests         */
#define CUSTOM_FILE      LIB_MISC "customs"     /*      Custom EQ            */
#define MESS_FILE        LIB_MISC "messages"    /* damage messages		*/
#define SOCMESS_FILE     LIB_MISC "socials"     /* messages for social acts	*/
#define SOCMESS_FILE_NEW LIB_MISC "socials.new" /* messages for social acts with aedit patch*/
#define XNAME_FILE       LIB_MISC "xnames"      /* invalid name substrings	*/

#define CONFIG_FILE     LIB_ETC "config"     /* OasisOLC * GAME CONFIG FL */
#define PLAYER_FILE     LIB_ETC "players"    /* the player database	*/
#define MAIL_FILE       LIB_ETC "plrmail"    /* for the mudmail system	*/
#define BAN_FILE        LIB_ETC "badsites"   /* for the siteban system	*/
#define HCONTROL_FILE   LIB_ETC "hcontrol"   /* for the house system	*/
#define TIME_FILE       LIB_ETC "time"       /* for calendar system	*/
#define AUCTION_FILE    LIB_ETC "auction"    /* for the auction house system */
#define ASSEMBLIES_FILE LIB_ETC "assemblies" /* for assemblies system 	*/
#define LEVEL_CONFIG    LIB_ETC "levels"     /* set various level values  */

/* new bitvector data for use in player_index_element */
constexpr int PINDEX_DELETED    = (1 << 0); /* deleted player	*/
constexpr int PINDEX_NODELETE   = (1 << 1); /* protected player	*/
constexpr int PINDEX_SELFDELETE = (1 << 2); /* player is selfdeleting*/
constexpr int PINDEX_NOWIZLIST  = (1 << 3); /* Player shouldn't be on wizlist*/

// global variables



void auc_load(struct obj_data *obj);
/* public procedures in db.c */
void boot_world(GameWorld *g);
bool is_empty(GameWorld *g, zone_rnum zone_nr);
void index_boot(GameWorld *g, int mode);
void boot_db(GameWorld *g);
void destroy_db(GameWorld *g);
int create_entry(GameWorld *g, char *name);
void zone_update(GameWorld *g);
char *fread_string(FILE *fl, const char *error);
long get_id_by_name(GameWorld *g, const char *name);
char *get_name_by_id(GameWorld *g, long id);
void save_mud_time(GameWorld *g, struct time_info_data *when);
void free_text_files(GameWorld *g);
void free_player_index(GameWorld *g);
void load_disabled(GameWorld *g);
void free_disabled(GameWorld *g);
void free_help_table(GameWorld *g);
void auc_save(GameWorld *g);
void load_config(GameWorld *g);

zone_rnum real_zone(GameWorld *g, zone_vnum vnum);
room_rnum real_room(GameWorld *g, room_vnum vnum);
mob_rnum real_mobile(GameWorld *g, mob_vnum vnum);
obj_rnum real_object(GameWorld *g, obj_vnum vnum);

int load_char(GameWorld *g, const char *name, struct char_data *ch);
void save_char(GameWorld *g, struct char_data *ch);
void init_char(GameWorld *g, struct char_data *ch);
struct char_data *read_mobile(GameWorld *g, mob_vnum nr, int type);
int vnum_mobile(GameWorld *g, char *searchname, struct char_data *ch);
void clear_char(struct char_data *ch);
void reset_char(GameWorld *g, struct char_data *ch);
void free_char(GameWorld *g, struct char_data *ch);
void save_player_index(GameWorld *g);
long get_ptable_by_name(GameWorld *g, const char *name);

struct obj_data *create_obj(GameWorld *g);
void clear_object(struct obj_data *obj);
void free_obj(GameWorld *g, struct obj_data *obj);
struct obj_data *read_object(GameWorld *g, obj_vnum nr, int type);
int vnum_object(GameWorld *g, char *searchname, struct char_data *ch);
bool my_obj_save_to_disk(GameWorld *g, FILE *fp, struct obj_data *obj, int locate);
void add_unique_id(GameWorld *g, struct obj_data *obj);
void check_unique_id(GameWorld *g, struct obj_data *obj);
char *sprintuniques(GameWorld *g, obj_vnum low, obj_vnum high);
int vnum_material(GameWorld *g, char *searchname, struct char_data *ch);
int vnum_weapontype(GameWorld *g, char *searchname, struct char_data *ch);
int vnum_armortype(GameWorld *g, char *searchname, struct char_data *ch);

constexpr int REAL    = 0;
constexpr int VIRTUAL = 1;

/* global buffering system */


bitvector_t asciiflag_conv(char *flag);
void reset_zone(GameWorld *g, zone_rnum zone);

/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE "disabled.cmds" /* disabled commands */
#define END_MARKER    "END"           /* for load_disabled() and save_disabled() */

// commands
ACMD(do_reboot);
void reboot_wizlists(GameWorld *g);
