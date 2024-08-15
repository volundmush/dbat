#pragma once
#include "sysdep.h"

constexpr int ALIAS_SIMPLE = 0;
constexpr int ALIAS_COMPLEX = 1;

#define ALIAS_SEP_CHAR    ';'
#define ALIAS_VAR_CHAR    '$'
#define ALIAS_GLOB_CHAR    '*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
constexpr int SCMD_NORTH = 1;
constexpr int SCMD_EAST = 2;
constexpr int SCMD_SOUTH = 3;
constexpr int SCMD_WEST = 4;
constexpr int SCMD_UP = 5;
constexpr int SCMD_DOWN = 6;
constexpr int SCMD_NW = 7;
constexpr int SCMD_NE = 8;
constexpr int SCMD_SE = 9;
constexpr int SCMD_SW = 10;
constexpr int SCMD_IN = 11;
constexpr int SCMD_OUT = 12;

/* do_gen_ps */
constexpr int SCMD_INFO = 0;
constexpr int SCMD_HANDBOOK = 1;
constexpr int SCMD_CREDITS = 2;
constexpr int SCMD_NEWS = 3;
constexpr int SCMD_WIZLIST = 4;
constexpr int SCMD_POLICIES = 5;
constexpr int SCMD_VERSION = 6;
constexpr int SCMD_IMMLIST = 7;
constexpr int SCMD_MOTD = 8;
constexpr int SCMD_IMOTD = 9;
constexpr int SCMD_CLEAR = 10;
constexpr int SCMD_WHOAMI = 11;

/* do_gen_tog */
constexpr int SCMD_NOSUMMON = 0;
constexpr int SCMD_NOHASSLE = 1;
constexpr int SCMD_BRIEF = 2;
constexpr int SCMD_COMPACT = 3;
constexpr int SCMD_NOTELL = 4;
constexpr int SCMD_NOAUCTION = 5;
constexpr int SCMD_DEAF = 6;
constexpr int SCMD_NOGOSSIP = 7;
constexpr int SCMD_NOGRATZ = 8;
constexpr int SCMD_NOWIZ = 9;
constexpr int SCMD_QUEST = 10;
constexpr int SCMD_ROOMFLAGS = 11;
constexpr int SCMD_NOREPEAT = 12;
constexpr int SCMD_HOLYLIGHT = 13;
constexpr int SCMD_SLOWNS = 14;
constexpr int SCMD_AUTOEXIT = 15;
constexpr int SCMD_TRACK = 16;
constexpr int SCMD_BUILDWALK = 17;
constexpr int SCMD_AFK = 18;
constexpr int SCMD_AUTOASSIST = 19;
constexpr int SCMD_AUTOLOOT = 20;
constexpr int SCMD_AUTOGOLD = 21;
constexpr int SCMD_CLS = 22;
constexpr int SCMD_AUTOSPLIT = 23;
constexpr int SCMD_AUTOSAC = 24;
constexpr int SCMD_SNEAK = 25;
constexpr int SCMD_HIDE = 26;
constexpr int SCMD_AUTOMEM = 27;
constexpr int SCMD_VIEWORDER = 28;
constexpr int SCMD_NOCOMPRESS = 29;
constexpr int SCMD_TEST = 30;
constexpr int SCMD_WHOHIDE = 31;
constexpr int SCMD_NMWARN = 32;
constexpr int SCMD_HINTS = 33;
constexpr int SCMD_NODEC = 34;
constexpr int SCMD_NOEQSEE = 35;
constexpr int SCMD_NOMUSIC = 36;
constexpr int SCMD_NOPARRY = 37;
constexpr int SCMD_LKEEP = 38;
constexpr int SCMD_CARVE = 39;
constexpr int SCMD_NOGIVE = 40;
constexpr int SCMD_INSTRUCT = 41;
constexpr int SCMD_GHEALTH = 42;
constexpr int SCMD_IHEALTH = 43;

/* do_wizutil */
constexpr int SCMD_REROLL = 0;
constexpr int SCMD_PARDON = 1;
constexpr int SCMD_NOTITLE = 2;
constexpr int SCMD_SQUELCH = 3;
constexpr int SCMD_FREEZE = 4;
constexpr int SCMD_THAW = 5;
constexpr int SCMD_UNAFFECT = 6;

/* do_spec_com */
constexpr int SCMD_WHISPER = 0;
constexpr int SCMD_ASK = 1;

/* do_gen_com */
constexpr int SCMD_HOLLER = 0;
constexpr int SCMD_SHOUT = 1;
constexpr int SCMD_GOSSIP = 2;
constexpr int SCMD_AUCTION = 3;
constexpr int SCMD_GRATZ = 4;
constexpr int SCMD_GEMOTE = 5;

/* do_shutdown */
constexpr int SCMD_SHUTDOW = 0;
constexpr int SCMD_SHUTDOWN = 1;

/* do_quit */
constexpr int SCMD_QUI = 0;
constexpr int SCMD_QUIT = 1;

/* do_date */
constexpr int SCMD_DATE = 0;
constexpr int SCMD_UPTIME = 1;

/* do_commands */
constexpr int SCMD_COMMANDS = 0;
constexpr int SCMD_SOCIALS = 1;
constexpr int SCMD_WIZHELP = 2;

/* do_drop */
constexpr int SCMD_DROP = 0;
constexpr int SCMD_JUNK = 1;
constexpr int SCMD_DONATE = 2;

/* do_gen_write */
constexpr int SCMD_BUG = 0;
constexpr int SCMD_TYPO = 1;
constexpr int SCMD_IDEA = 2;

/* do_look */
constexpr int SCMD_LOOK = 0;
constexpr int SCMD_READ = 1;
constexpr int SCMD_SEARCH = 2;

/* do_qcomm */
constexpr int SCMD_QSAY = 0;
constexpr int SCMD_QECHO = 1;

/* do_pour */
constexpr int SCMD_POUR = 0;
constexpr int SCMD_FILL = 1;

/* do_poof */
constexpr int SCMD_POOFIN = 0;
constexpr int SCMD_POOFOUT = 1;

/* do_hit */
constexpr int SCMD_HIT = 0;
constexpr int SCMD_MURDER = 1;

/* do_eat */
constexpr int SCMD_EAT = 0;
constexpr int SCMD_TASTE = 1;
constexpr int SCMD_DRINK = 2;
constexpr int SCMD_SIP = 3;

/* do_use */
constexpr int SCMD_USE = 0;
constexpr int SCMD_QUAFF = 1;
constexpr int SCMD_RECITE = 2;

/* do_echo */
constexpr int SCMD_ECHO = 0;
constexpr int SCMD_EMOTE = 1;
constexpr int SCMD_SMOTE = 2;

/* do_gen_door */
constexpr int SCMD_OPEN = 0;
constexpr int SCMD_CLOSE = 1;
constexpr int SCMD_UNLOCK = 2;
constexpr int SCMD_LOCK = 3;
constexpr int SCMD_PICK = 4;

/* do_olc */
constexpr int SCMD_OASIS_REDIT = 0;
constexpr int SCMD_OASIS_OEDIT = 1;
constexpr int SCMD_OASIS_ZEDIT = 2;
constexpr int SCMD_OASIS_MEDIT = 3;
constexpr int SCMD_OASIS_SEDIT = 4;
constexpr int SCMD_OASIS_CEDIT = 5;
constexpr int SCMD_OLC_SAVEINFO = 7;
constexpr int SCMD_OASIS_RLIST = 8;
constexpr int SCMD_OASIS_MLIST = 9;
constexpr int SCMD_OASIS_OLIST = 10;
constexpr int SCMD_OASIS_SLIST = 11;
constexpr int SCMD_OASIS_ZLIST = 12;
constexpr int SCMD_OASIS_TRIGEDIT = 13;
constexpr int SCMD_OASIS_AEDIT = 14;
constexpr int SCMD_OASIS_TLIST = 15;
constexpr int SCMD_OASIS_LINKS = 16;
constexpr int SCMD_OASIS_GEDIT = 17;
constexpr int SCMD_OASIS_GLIST = 18;
constexpr int SCMD_OASIS_HEDIT = 19;
constexpr int SCMD_OASIS_HSEDIT = 20;

/* do_builder_list */

constexpr int SCMD_RLIST = 0;
constexpr int SCMD_OLIST = 1;
constexpr int SCMD_MLIST = 2;
constexpr int SCMD_TLIST = 3;
constexpr int SCMD_SLIST = 4;
constexpr int SCMD_GLIST = 5;

/* * do_assemble * These constants *must* corespond with
     the ASSM_xxx constants in * assemblies.h. */
constexpr int SCMD_MAKE = 0;
constexpr int SCMD_BAKE = 1;
constexpr int SCMD_BREW = 2;
constexpr int SCMD_ASSEMBLE = 3;
constexpr int SCMD_CRAFT = 4;
constexpr int SCMD_FLETCH = 5;
constexpr int SCMD_KNIT = 6;
constexpr int SCMD_MIX = 7;
constexpr int SCMD_THATCH = 8;
constexpr int SCMD_WEAVE = 9;
constexpr int SCMD_FORGE = 10;


constexpr int SCMD_MEMORIZE = 1;
constexpr int SCMD_FORGET = 2;
constexpr int SCMD_STOP = 3;
constexpr int SCMD_WHEN_SLOT = 4;

/* do_value list */
constexpr int SCMD_WIMPY = 0;
constexpr int SCMD_POWERATT = 1;
constexpr int SCMD_COMBATEXP = 2;

/* do_cast */
constexpr int SCMD_CAST = 0;
constexpr int SCMD_ART = 1;

/* oasis_copy */
constexpr int SCMD_TEDIT = 0;
constexpr int SCMD_REDIT = 1;
constexpr int SCMD_OEDIT = 2;
constexpr int SCMD_MEDIT = 3;


struct command_info {
    const char *command;
    const char *sort_as;
    int8_t minimum_position;
    CommandFunc command_pointer;
    int16_t minimum_level;
    int16_t minimum_admlevel;
    int subcmd;
    int wait_list;
};

extern const struct command_info cmd_info[];
extern struct command_info *complete_cmd_info;