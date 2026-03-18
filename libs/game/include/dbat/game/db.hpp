#pragma once
#include <memory>
#include <vector>
#include <map>
#include "Typedefs.hpp"
#include "Command.hpp"

// Function to collect pointers to objects within a vnum range from a map


#define LIB_USER "data/user/"
#define LIB_INTRO "data/intro/"
#define LIB_SENSE "data/sense/"
#define LIB_WORLD "data/world/"
#define LIB_TEXT "data/text/"
#define LIB_TEXT_HELP "data/text/help/"
#define LIB_MISC "data/misc/"
#define LIB_ETC "data/etc/"
#define LIB_PLRTEXT "data/plrtext/"
#define LIB_PLROBJS "data/plrobjs/"
#define LIB_PLRVARS "data/plrvars/"
#define LIB_PLRALIAS "data/plralias/"
#define LIB_PLRFILES "data/plrfiles/"
#define LIB_HOUSE "data/house/"
#define LIB_PLRIMC "data/plrimc/"
#define SLASH "/"

#define SUF_OBJS "new"
#define SUF_TEXT "text"
#define SUF_ALIAS "alias"
#define SUF_MEM "mem"
#define SUF_PLR "plr"
#define SUF_PET "pet"
#define SUF_IMC "imc"
#define SUF_USER "usr"
#define SUF_INTRO "itr"
#define SUF_SENSE "sen"
#define SUF_CUSTOM "cus"

/* names of various files and directories */
#define INDEX_FILE "index"               /* index of world files		*/
#define MINDEX_FILE "index.mini"         /* ... and for mini-mud-mode	*/
#define WLD_PREFIX LIB_WORLD "wld" SLASH /* room definitions	*/
#define MOB_PREFIX LIB_WORLD "mob" SLASH /* monster prototypes	*/
#define OBJ_PREFIX LIB_WORLD "obj" SLASH /* object prototypes	*/
#define ZON_PREFIX LIB_WORLD "zon" SLASH /* zon defs & command tables */
#define SHP_PREFIX LIB_WORLD "shp" SLASH /* shop definitions	*/
#define HLP_PREFIX LIB_TEXT "help" SLASH /* for HELP <keyword>	*/
#define TRG_PREFIX LIB_WORLD "trg" SLASH /* trigger files	*/
#define GLD_PREFIX LIB_WORLD "gld" SLASH /* guild files		*/

#define CREDITS_FILE LIB_TEXT "credits"          /* for the 'credits' command	*/
#define NEWS_FILE LIB_TEXT "news"                /* for the 'news' command	*/
#define MOTD_FILE LIB_TEXT "motd"                /* messages of the day / mortal	*/
#define IMOTD_FILE LIB_TEXT "imotd"              /* messages of the day / immort	*/
#define GREETINGS_FILE LIB_TEXT "greetings"      /* The opening screen.	*/
#define GREETANSI_FILE LIB_TEXT "greetansi"      /* The opening screen.	*/
#define HELP_PAGE_FILE LIB_TEXT_HELP "screen"    /* for HELP <CR>	*/
#define CONTEXT_HELP_FILE LIB_TEXT "contexthelp" /* The opening screen.	*/
#define INFO_FILE LIB_TEXT "info"                /* for INFO		*/
#define WIZLIST_FILE LIB_TEXT "wizlist"          /* for WIZLIST		*/
#define IMMLIST_FILE LIB_TEXT "immlist"          /* for IMMLIST		*/
#define BACKGROUND_FILE LIB_TEXT "background"    /* for the background story	*/
#define POLICIES_FILE LIB_TEXT "policies"        /* player policies/rules	*/
#define HANDBOOK_FILE LIB_TEXT "handbook"        /* handbook for new immorts	*/
#define IHELP_PAGE_FILE LIB_TEXT_HELP "iscreen"  /* for HELP <CR>	*/
#define HELP_FILE "help.hlp"
#define MAP_FILE "data/surface.map" /* for the map command */

#define IDEA_FILE LIB_MISC "ideas"              /* for the 'idea'-command	*/
#define TYPO_FILE LIB_MISC "typos"              /*         'typo'		*/
#define BUG_FILE LIB_MISC "bugs"                /*         'bug'		*/
#define REQUEST_FILE LIB_MISC "request"         /*      RPP Requests         */
#define CUSTOM_FILE LIB_MISC "customs"          /*      Custom EQ            */
#define MESS_FILE LIB_MISC "messages"           /* damage messages		*/
#define SOCMESS_FILE LIB_MISC "socials"         /* messages for social acts	*/
#define SOCMESS_FILE_NEW LIB_MISC "socials.new" /* messages for social acts with aedit patch*/
#define XNAME_FILE LIB_MISC "xnames"            /* invalid name substrings	*/

#define CONFIG_FILE LIB_ETC "config"         /* OasisOLC * GAME CONFIG FL */
#define PLAYER_FILE LIB_ETC "players"        /* the player database	*/
#define MAIL_FILE LIB_ETC "plrmail"          /* for the mudmail system	*/
#define BAN_FILE LIB_ETC "badsites"          /* for the siteban system	*/
#define HCONTROL_FILE LIB_ETC "hcontrol"     /* for the house system	*/
#define TIME_FILE LIB_ETC "time"             /* for calendar system	*/
#define AUCTION_FILE LIB_ETC "auction"       /* for the auction house system */
#define ASSEMBLIES_FILE LIB_ETC "assemblies" /* for assemblies system 	*/
#define LEVEL_CONFIG LIB_ETC "levels"        /* set various level values  */

/* new bitvector data for use in player_index_element */
#define PINDEX_DELETED (1 << 0)    /* deleted player	*/
#define PINDEX_NODELETE (1 << 1)   /* protected player	*/
#define PINDEX_SELFDELETE (1 << 2) /* player is selfdeleting*/
#define PINDEX_NOWIZLIST (1 << 3)  /* Player shouldn't be on wizlist*/

// global variables

extern bool isMigrating;

extern bool gameIsLoading;
extern bool saveAll;

extern time_t boot_time;

extern int circle_restrict;

extern char *help, *ihelp, *credits, *news, *info, *wizlist, *immlist, *background;
extern char *policies, *handbook, *motd, *imotd, *GREETINGS, *GREETANSI;

extern int mini_mud, no_rent_check, no_mail;
extern room_rnum r_mortal_start_room; /* rnum of mortal start room	 */
extern room_rnum r_immort_start_room; /* rnum of immort start room	 */
extern room_rnum r_frozen_start_room; /* rnum of frozen start room	 */

extern time_t old_beginning_of_time;

/* public procedures in db.c */
extern char *fread_string(FILE *fl, const char *error);

extern void free_text_files();

extern void auc_save();

extern void destroy_db();

constexpr int REAL = 0;
constexpr int VIRTUAL = 1;

/* don't change these */
constexpr int BAN_NOT = 0;
constexpr int BAN_NEW = 1;
constexpr int BAN_SELECT = 2;
constexpr int BAN_ALL = 3;

/* global buffering system */


extern int dg_owner_purged;

extern void strip_string(char *buffer);

extern bitvector_t asciiflag_conv(char *flag);



/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE "data/disabled.cmds" /* disabled commands */
#define END_MARKER "END"                   /* for load_disabled() and save_disabled() */

// commands

extern void load_config();
