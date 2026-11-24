#pragma once
#include <cstdio>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/io/load_error.hpp"

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

/* A player index entry's flags (player_index_element::flags), saved in the index as letters */
extern const name_table pindex_bits; /* flag_count<pindex_flag> of them */
enum class pindex_flag : uint8_t {
  deleted    = 0, /* deleted player	*/
  nodelete   = 1, /* protected player	*/
  selfdelete = 2, /* player is selfdeleting*/
  nowizlist  = 3, /* Player shouldn't be on wizlist*/
  count_     = 4,
};
template <> struct flag_traits<pindex_flag> {
  static constexpr const name_table &names = pindex_bits;
};

/* The old names, until every use says pindex_flag:: */
constexpr pindex_flag PINDEX_DELETED    = pindex_flag::deleted;
constexpr pindex_flag PINDEX_NODELETE   = pindex_flag::nodelete;
constexpr pindex_flag PINDEX_SELFDELETE = pindex_flag::selfdelete;
constexpr pindex_flag PINDEX_NOWIZLIST  = pindex_flag::nowizlist;

// global variables



void auc_load(struct obj_data *obj);
/* public procedures in db.cpp, reset.cpp and the entity files */
/* Loads the world: zones, triggers, rooms, mobs, objects, shops and guilds; or why a file of it can't be */
io::loaded<void> boot_world(GameWorld *g);
bool is_empty(GameWorld *g, struct zone_data *zone_nr);
void index_boot(GameWorld *g, int mode);
void boot_db(GameWorld *g);
void destroy_db(GameWorld *g);
int create_entry(GameWorld *g, std::string_view name);
void zone_update(GameWorld *g);
long get_id_by_name(GameWorld *g, std::string_view name);
/* The name (lowercase) of the player with id, or null if there is none */
const std::string *get_name_by_id(GameWorld *g, long id);
/* The name (lowercase) of the player with id, or otherwise if there is none */
std::string_view player_name_or(GameWorld *g, long id, std::string_view otherwise);
void save_mud_time(GameWorld *g, struct time_info_data *when);
/* Reads etc/time: the beginning of the game's time, and the dates and counters kept on g */
void read_time_file(GameWorld *g, time_t &beginning);
void free_player_index(GameWorld *g);
void load_disabled(GameWorld *g);
void free_disabled(GameWorld *g);
void free_help_table(GameWorld *g);
void auc_save(GameWorld *g);
void load_config(GameWorld *g);

struct zone_data *find_zone(GameWorld *g, zone_vnum vnum);

int load_char(GameWorld *g, std::string_view name, struct char_data *ch);
void save_char(GameWorld *g, struct char_data *ch);
void init_char(GameWorld *g, struct char_data *ch);
struct char_data *read_mobile(GameWorld *g, mob_vnum nr);
int vnum_mobile(GameWorld *g, std::string_view searchname, struct char_data *ch);
void clear_char(struct char_data *ch);
/* An enhanced (E) mob's abilities, rolled at random by its level and race as it loads */
void mob_stats(GameWorld *g, struct char_data *mob);
void reset_char(GameWorld *g, struct char_data *ch);
void free_char(GameWorld *g, struct char_data *ch);
void save_player_index(GameWorld *g);
long get_ptable_by_name(GameWorld *g, std::string_view name);

struct obj_data *create_obj(GameWorld *g);
void clear_object(struct obj_data *obj);
void free_obj(GameWorld *g, struct obj_data *obj);
/* Takes obj out of the game: no longer found by id or unique id. free_obj() does this too. */
void obj_retire(GameWorld *g, struct obj_data *obj);
struct obj_data *read_object(GameWorld *g, obj_vnum nr);
int vnum_object(GameWorld *g, std::string_view searchname, struct char_data *ch);
/* Makes the empty unique-id table at boot, and frees it and its entries at shutdown */
void init_obj_unique_hash(GameWorld *g);
void free_obj_unique_hash(GameWorld *g);
void add_unique_id(GameWorld *g, struct obj_data *obj);
void check_unique_id(GameWorld *g, struct obj_data *obj);
/* the unique objects with vnums low to high, as "show uniques" lists them */
std::string sprintuniques(GameWorld *g, obj_vnum low, obj_vnum high);
int vnum_material(GameWorld *g, std::string_view searchname, struct char_data *ch);
int vnum_weapontype(GameWorld *g, std::string_view searchname, struct char_data *ch);
int vnum_armortype(GameWorld *g, std::string_view searchname, struct char_data *ch);

void reset_zone(GameWorld *g, struct zone_data *zone);

/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE "disabled.cmds" /* disabled commands */
#define END_MARKER    "END"           /* for load_disabled() and save_disabled() */

// commands
ACMD(do_reboot);
void reboot_wizlists(GameWorld *g);

