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
#define DB_BOOT_WLD    0
#define DB_BOOT_MOB    1
#define DB_BOOT_OBJ    2
#define DB_BOOT_ZON    3
#define DB_BOOT_SHP    4
#define DB_BOOT_HLP    5
#define DB_BOOT_TRG    6
#define DB_BOOT_GLD    7

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

extern std::unordered_map<std::string, std::shared_ptr<InternedString>> intern;
std::shared_ptr<InternedString> internString(const std::string& str);

bool isUID(const std::string& uid);
GameEntity* resolveUID(const std::string& uid);

extern struct time_info_data old_time_info; /* UNUSED (to be removed) the infomation about the time    */
extern struct time_info_data time_info;/* the infomation about the time    */
extern struct weather_data weather_info;    /* the infomation about the weather */
extern std::set<zone_vnum> zone_reset_queue;

extern bool gameIsLoading;
extern bool saveAll;

extern BaseCharacter *EDRAGON;
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
extern void auc_load(Object *obj);

void create_schema();

void dump_state();

extern void boot_world();

extern int is_empty(zone_rnum zone_nr);

extern void index_boot(int mode);
extern void check_start_rooms();
extern void index_boot_help();
extern void boot_db_textfiles();
extern void boot_db_time();
extern void boot_db_spellfeats();
extern void boot_db_world();
extern void boot_db_help();
extern void boot_db_mail();
extern void boot_db_socials();
extern void boot_db_commands();
extern void boot_db_specials();
extern void boot_db_assemblies();
extern void boot_db_sort();
extern void boot_db_boards();
extern void boot_db_banned();
extern void boot_db_rent();
extern void boot_db_houses();
extern void boot_db_shadow();
extern void boot_db_spacemap();

extern void boot_db_legacy();
extern void boot_db_new();

extern void destroy_db();

extern void zone_update(uint64_t heartPulse, double deltaTime);

extern char *fread_string(FILE *fl, const char *error);

extern long get_id_by_name(const char *name);

extern char *get_name_by_id(long id);

extern void save_mud_time(struct time_info_data *when);

extern void free_text_files();

extern void free_player_index();

extern void load_disabled();

extern void save_disabled();

extern void free_disabled();

extern void free_help_table();

extern void load_help(FILE *fl, char *name);

extern void auc_save();

extern void load_config();

extern zone_rnum real_zone(zone_vnum vnum);

extern room_rnum real_room(room_vnum vnum);

extern mob_rnum real_mobile(mob_vnum vnum);

extern obj_rnum real_object(obj_vnum vnum);

extern void init_char(BaseCharacter *ch);

BaseCharacter *read_mobile(mob_vnum nr, int type);

extern int vnum_mobile(char *searchname, BaseCharacter *ch);

extern void reset_char(BaseCharacter *ch);

extern void free_char(BaseCharacter *ch);

extern void save_player_index();

extern long get_ptable_by_name(const char *name);

extern void read_level_data(BaseCharacter *ch, FILE *fl);

extern void write_level_data(BaseCharacter *ch, FILE *fl);

extern int parse_mobile_from_file(FILE *mob_f, BaseCharacter *ch);

extern void free_obj(Object *obj);

Object *read_object(obj_vnum nr, int type, bool activate = true);

extern int vnum_object(char *searchname, BaseCharacter *ch);

extern char *sprintuniques(int low, int high);

extern int vnum_material(char *searchname, BaseCharacter *ch);

extern int vnum_weapontype(char *searchname, BaseCharacter *ch);

extern int vnum_armortype(char *searchname, BaseCharacter *ch);

extern void migrate_db();

#define REAL 0
#define VIRTUAL 1



/* zone definition structure. for the 'zone-table'   */
#define CUR_WORLD_VERSION 1
#define CUR_ZONE_VERSION  2




struct help_index_element {
    char *index;      /*Future Use */
    char *keywords;   /*Keyword Place holder and sorter */
    char *entry;      /*Entries for help files with Keywords at very top*/
    int duplicate;    /*Duplicate entries for multple keywords*/
    int min_level;    /*Min Level to read help entry*/
};

/* don't change these */
#define BAN_NOT    0
#define BAN_NEW    1
#define BAN_SELECT    2
#define BAN_ALL        3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
    char site[BANNED_SITE_LENGTH + 1];
    int type;
    time_t date;
    char name[MAX_NAME_LENGTH + 1];
    struct ban_list_element *next;
};


/* global buffering system */
extern time_t boot_time;

extern struct config_data config_info;

// dirty sets...
extern std::vector<obj_vnum> dbVnums;

// world data...
extern int64_t nextUID;
extern int64_t getNextUID();


extern std::unordered_set<GameEntity*> pendingDeletions;

extern std::unordered_map<zone_vnum, struct zone_data> zone_table;

extern struct descriptor_data *descriptor_list;
extern std::unordered_map<int64_t, struct descriptor_data*> sessions;

extern BaseCharacter *affect_list;
extern BaseCharacter *affectv_list;

extern std::unordered_map<mob_vnum, struct index_data> mob_index;
extern std::unordered_map<mob_vnum, nlohmann::json> mob_proto;


extern VnumIndex<Object> objectVnumIndex;
extern VnumIndex<BaseCharacter> characterVnumIndex;

extern std::unordered_map<obj_vnum, struct index_data> obj_index;
extern std::unordered_map<obj_vnum, nlohmann::json> obj_proto;

extern struct social_messg *soc_mess_list;
extern int top_of_socialt;
extern std::unordered_map<trig_vnum, std::shared_ptr<trig_proto>> trig_index;

extern void strip_string(char *buffer);

extern bitvector_t asciiflag_conv(char *flag);

extern void reset_zone(zone_rnum zone);

/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE    "disabled.cmds"  /* disabled commands */
#define END_MARKER    "END" /* for load_disabled() and save_disabled() */

typedef struct disabled_data DISABLED_DATA;

extern DISABLED_DATA *disabled_first; /* interpreter.c */

/* one disabled command */
struct disabled_data {
    DISABLED_DATA *next;                /* pointer to next node          */
    struct command_info const *command; /* pointer to the command struct */
    char *disabled_by;                  /* name of disabler              */
    int16_t level;                       /* level of disabler             */
    int subcmd;                         /* the subcmd, if any            */
};

// commands
extern ACMD(do_reboot);


/* create an object, and add it to the object list */
template <typename T = Object>
T *create_obj(EntityFamily family = EntityFamily::Object) {
    auto id = getNextUID();
    auto ent = reg.create();
    auto &t = reg.get_or_emplace<T>(ent);
    t.script = std::make_shared<script_data>(&t);
    t.ent = ent;
    t.uid = id;
    auto &info = reg.get_or_emplace<Info>(ent);
    info.family = family;
    info.uid = id;

    setEntity(id, ent);

    return reg.try_get<T>(ent);
}