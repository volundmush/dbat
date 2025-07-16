/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once

#include "structs.h"


/* arbitrary constants used by index_boot() (must be unique) */
constexpr int DB_BOOT_WLD = 0;
constexpr int DB_BOOT_MOB = 1;
constexpr int DB_BOOT_OBJ = 2;
constexpr int DB_BOOT_ZON = 3;
constexpr int DB_BOOT_SHP = 4;
constexpr int DB_BOOT_HLP = 5;
constexpr int DB_BOOT_TRG = 6;
constexpr int DB_BOOT_GLD = 7;

#define LIB_USER        "user/"
#define LIB_INTRO       "intro/"
#define LIB_SENSE       "sense/"
#define LIB_WORLD    "world/"
#define LIB_TEXT    "text/"
#define LIB_TEXT_HELP    "text/help/"
#define LIB_MISC    "misc/"
#define LIB_ETC        "etc/"
#define LIB_PLRTEXT    "plrtext/"
#define LIB_PLROBJS    "plrobjs/"
#define LIB_PLRVARS    "plrvars/"
#define LIB_PLRALIAS    "plralias/"
#define LIB_PLRFILES    "plrfiles/"
#define LIB_HOUSE    "house/"
#define LIB_PLRIMC      "plrimc/"
#define SLASH        "/"

#define SUF_OBJS    "new"
#define SUF_TEXT    "text"
#define SUF_ALIAS    "alias"
#define SUF_MEM            "mem"
#define SUF_PLR            "plr"
#define SUF_PET        "pet"
#define SUF_IMC         "imc"
#define SUF_USER        "usr"
#define SUF_INTRO       "itr"
#define SUF_SENSE       "sen"
#define SUF_CUSTOM      "cus"

#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */

/* names of various files and directories */
#define INDEX_FILE    "index"        /* index of world files		*/
#define MINDEX_FILE    "index.mini"    /* ... and for mini-mud-mode	*/
#define WLD_PREFIX    LIB_WORLD "wld" SLASH    /* room definitions	*/
#define MOB_PREFIX    LIB_WORLD "mob" SLASH    /* monster prototypes	*/
#define OBJ_PREFIX    LIB_WORLD "obj" SLASH    /* object prototypes	*/
#define ZON_PREFIX    LIB_WORLD "zon" SLASH    /* zon defs & command tables */
#define SHP_PREFIX    LIB_WORLD "shp" SLASH    /* shop definitions	*/
#define HLP_PREFIX    LIB_TEXT "help" SLASH    /* for HELP <keyword>	*/
#define TRG_PREFIX    LIB_WORLD "trg" SLASH    /* trigger files	*/
#define GLD_PREFIX    LIB_WORLD "gld" SLASH    /* guild files		*/

#define CREDITS_FILE    LIB_TEXT "credits" /* for the 'credits' command	*/
#define NEWS_FILE    LIB_TEXT "news"    /* for the 'news' command	*/
#define MOTD_FILE    LIB_TEXT "motd"    /* messages of the day / mortal	*/
#define IMOTD_FILE    LIB_TEXT "imotd"    /* messages of the day / immort	*/
#define GREETINGS_FILE    LIB_TEXT "greetings"    /* The opening screen.	*/
#define GREETANSI_FILE    LIB_TEXT "greetansi"    /* The opening screen.	*/
#define HELP_PAGE_FILE    LIB_TEXT_HELP "screen"    /* for HELP <CR>	*/
#define CONTEXT_HELP_FILE LIB_TEXT "contexthelp"    /* The opening screen.	*/
#define INFO_FILE    LIB_TEXT "info"        /* for INFO		*/
#define WIZLIST_FILE    LIB_TEXT "wizlist"    /* for WIZLIST		*/
#define IMMLIST_FILE    LIB_TEXT "immlist"    /* for IMMLIST		*/
#define BACKGROUND_FILE    LIB_TEXT "background"/* for the background story	*/
#define POLICIES_FILE    LIB_TEXT "policies"  /* player policies/rules	*/
#define HANDBOOK_FILE    LIB_TEXT "handbook"  /* handbook for new immorts	*/
#define IHELP_PAGE_FILE    LIB_TEXT_HELP "iscreen"    /* for HELP <CR>	*/
#define HELP_FILE       "help.hlp"

#define IDEA_FILE    LIB_MISC "ideas"       /* for the 'idea'-command	*/
#define TYPO_FILE    LIB_MISC "typos"       /*         'typo'		*/
#define BUG_FILE    LIB_MISC "bugs"       /*         'bug'		*/
#define REQUEST_FILE    LIB_MISC "request"  /*      RPP Requests         */
#define CUSTOM_FILE     LIB_MISC "customs"  /*      Custom EQ            */
#define MESS_FILE    LIB_MISC "messages" /* damage messages		*/
#define SOCMESS_FILE    LIB_MISC "socials"  /* messages for social acts	*/
#define SOCMESS_FILE_NEW LIB_MISC "socials.new"  /* messages for social acts with aedit patch*/
#define XNAME_FILE    LIB_MISC "xnames"   /* invalid name substrings	*/

#define CONFIG_FILE   LIB_ETC "config"      /* OasisOLC * GAME CONFIG FL */
#define PLAYER_FILE    LIB_ETC "players"   /* the player database	*/
#define MAIL_FILE    LIB_ETC "plrmail"   /* for the mudmail system	*/
#define BAN_FILE    LIB_ETC "badsites"  /* for the siteban system	*/
#define HCONTROL_FILE    LIB_ETC "hcontrol"  /* for the house system	*/
#define TIME_FILE    LIB_ETC "time"       /* for calendar system	*/
#define AUCTION_FILE    LIB_ETC "auction"   /* for the auction house system */
#define ASSEMBLIES_FILE LIB_ETC "assemblies"/* for assemblies system 	*/
#define LEVEL_CONFIG    LIB_ETC "levels"       /* set various level values  */

/* new bitvector data for use in player_index_element */
#define PINDEX_DELETED        (1 << 0)    /* deleted player	*/
#define PINDEX_NODELETE        (1 << 1)    /* protected player	*/
#define PINDEX_SELFDELETE    (1 << 2)    /* player is selfdeleting*/
#define PINDEX_NOWIZLIST    (1 << 3)    /* Player shouldn't be on wizlist*/


// global variables

extern struct time_info_data old_time_info; /* UNUSED (to be removed) the infomation about the time    */

extern std::unordered_set<zone_vnum> zone_reset_queue;

extern bool gameIsLoading;
extern bool saveAll;

extern struct char_data *EDRAGON;
extern int WISH[2];
extern int DRAGONR, DRAGONZ, DRAGONC, SHENRON;
extern int circle_restrict;
extern struct help_index_element *help_table;
extern char *help, *ihelp, *credits, *news, *info, *wizlist, *immlist, *background;
extern char *policies, *handbook, *motd, *imotd, *GREETINGS, *GREETANSI;
extern int top_of_helpt, dballtime;
extern int mini_mud, no_rent_check, no_mail;
extern room_rnum r_mortal_start_room;    /* rnum of mortal start room	 */
extern room_rnum r_immort_start_room;    /* rnum of immort start room	 */
extern room_rnum r_frozen_start_room;    /* rnum of frozen start room	 */

extern time_t old_beginning_of_time;

/* public procedures in db.c */
extern void auc_load(struct obj_data *obj);

void create_schema();

void dump_state();

extern void boot_world();

extern int is_empty(zone_rnum zone_nr);

extern void check_start_rooms();
extern void boot_db_textfiles();
extern void boot_db_time();
extern void boot_db_spellfeats();
extern void boot_db_world();
extern void boot_db_help();
extern void boot_db_mail();
extern void boot_db_socials();
extern void boot_db_clans();
extern void boot_db_commands();
extern void boot_db_specials();
extern void boot_db_sort();
extern void boot_db_boards();
extern void boot_db_banned();
extern void boot_db_rent();
extern void boot_db_houses();
extern void boot_db_shadow();
extern void boot_db_spacemap();
extern void boot_db_sort();

extern void boot_db_new();


extern void zone_update(uint64_t heartPulse, double deltaTime);
extern void repairRoomDamage(uint64_t heartPulse, double deltaTime);

extern char *fread_string(FILE *fl, const char *error);

extern long get_id_by_name(const char *name);

extern char *get_name_by_id(long id);

extern void save_mud_time(struct time_info_data *when);

extern void free_extra_descriptions(struct extra_descr_data *edesc);

extern void free_text_files();

extern void free_player_index();

extern void load_disabled();

extern void save_disabled();

extern void free_disabled();

extern void free_help_table();

extern void auc_save();

extern void load_config();

extern zone_rnum real_zone(zone_vnum vnum);

extern room_rnum real_room(room_vnum vnum);

extern mob_rnum real_mobile(mob_vnum vnum);

extern obj_rnum real_object(obj_vnum vnum);

extern void init_char(struct char_data *ch);

struct char_data *read_mobile(mob_vnum nr, int type);

extern int vnum_mobile(char *searchname, struct char_data *ch);

extern void reset_char(struct char_data *ch);

extern void free_char(struct char_data *ch);

extern void save_player_index();

extern long get_ptable_by_name(const char *name);

extern void read_level_data(struct char_data *ch, FILE *fl);

extern void write_level_data(struct char_data *ch, FILE *fl);

struct obj_data *create_obj();

extern void free_obj(struct obj_data *obj);

struct obj_data *read_object(obj_vnum nr, int type);

extern int vnum_object(char *searchname, struct char_data *ch);

extern void add_unique_id(struct obj_data *obj);

extern void check_unique_id(struct obj_data *obj);

extern void add_unique_id(struct char_data *ch);

extern void check_unique_id(struct char_data *ch);

extern char *sprintuniques(int low, int high);

extern int vnum_material(char *searchname, struct char_data *ch);

extern int vnum_weapontype(char *searchname, struct char_data *ch);

extern int vnum_armortype(char *searchname, struct char_data *ch);

extern void migrate_db();

extern room_data* get_room(room_vnum vn);

constexpr int REAL = 0;
constexpr int VIRTUAL = 1;

/* don't change these */
constexpr int BAN_NOT = 0;
constexpr int BAN_NEW = 1;
constexpr int BAN_SELECT = 2;
constexpr int BAN_ALL = 3;

extern std::vector<obj_vnum> dbVnums;

/* global buffering system */
extern time_t boot_time;

extern struct config_data config_info;


// world data...
extern std::map<room_vnum, std::shared_ptr<room_data>> world;
extern struct time_info_data time_info;/* the infomation about the time    */
extern struct time_info_data era_uptime;/* the infomation about the time    */
extern struct weather_data weather_info;    /* the infomation about the weather */

extern std::unordered_map<int, std::shared_ptr<struct unit_data>> units;

extern std::map<room_vnum, std::shared_ptr<room_data>> world;
extern std::map<zone_vnum, struct zone_data> zone_table;

extern struct descriptor_data *descriptor_list;
extern std::map<int64_t, struct descriptor_data*> sessions;

extern std::map<mob_vnum, struct index_data> mob_index;
extern std::map<mob_vnum, struct char_data> mob_proto;

extern std::unordered_map<int, std::shared_ptr<char_data>> uniqueCharacters;
extern std::vector<std::weak_ptr<char_data>> getAllCharacters();


extern std::map<obj_vnum, struct index_data> obj_index;
extern std::map<obj_vnum, struct obj_data> obj_proto;

extern std::unordered_map<int, std::shared_ptr<obj_data>> uniqueObjects;
extern std::vector<std::weak_ptr<obj_data>> getAllObjects();

extern std::map<trig_vnum, struct index_data> trig_index;
extern std::unordered_map<int, std::shared_ptr<trig_data>> uniqueScripts;

extern std::map<vnum, account_data> accounts;
extern std::map<shop_vnum, struct shop_data> shop_index;
extern std::map<guild_vnum, struct guild_data> guild_index;

extern std::map<int64_t, player_data> players;

int getNextUnitID();
int getNextAccountID();
bool isUID(const std::string& uid);
std::shared_ptr<unit_data> resolveUID(const std::string& uid);

extern struct char_data *affect_list;
extern struct char_data *affectv_list;

extern SubscriptionManager<char_data> characterSubscriptions;
extern SubscriptionManager<obj_data> objectSubscriptions;
extern SubscriptionManager<room_data> roomSubscriptions;
extern SubscriptionManager<trig_data> triggerSubscriptions;

extern int create_join_session(int account_id, int character_id, int64_t connection_id, const std::string& ip);

extern struct social_messg *soc_mess_list;
extern int top_of_socialt;

extern struct trig_data *trigger_list;

extern int dg_owner_purged;

extern void strip_string(char *buffer);

extern bitvector_t asciiflag_conv(char *flag);

extern void reset_zone(zone_rnum zone);

/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE    "disabled.cmds"  /* disabled commands */
#define END_MARKER    "END" /* for load_disabled() and save_disabled() */

// commands
extern ACMD(do_reboot);
