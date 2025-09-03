#pragma once

constexpr int MEAL_LAST = 1234;
constexpr int NUM_EXIT_FLAGS = 5;

constexpr int NUM_ROOM_SECTORS = 15;

constexpr int NUM_CLASSES = 31;
constexpr int NUM_NPC_CLASSES = 4;
constexpr int NUM_PRESTIGE_CLASSES = 15;
constexpr int NUM_BASIC_CLASSES = 14;

constexpr int MAX_BONUSES = 52;

constexpr int NUM_ALIGNS = 9;
constexpr int NUM_RACES = 24;
constexpr int NUM_SIZES = 9;

constexpr int NUM_CRIT_TYPES = 3;

/* Sex */
constexpr int NUM_ROOM_FLAGS = 69;
constexpr int NUM_SEX = 3;
constexpr int NUM_GENDERS = NUM_SEX;

constexpr int NUM_POSITIONS = 9;
constexpr int NUM_PLR_FLAGS = 80;

/* Mob Personalty */
constexpr int MAX_PERSONALITIES = 5;
constexpr int NUM_MOB_FLAGS = 35;

constexpr int NUM_PRF_FLAGS = 62;

constexpr int NUM_AFF_FLAGS = 82;
constexpr int NUM_CON_TYPES = 77;

constexpr int NUM_COLOR = 16;

constexpr int NUM_WEARS = 23; /* This must be the # of eq positions!! */

constexpr int NUM_DOMAINS = 37;
constexpr int NUM_SCHOOLS = 10;

constexpr int NUM_DEITIES = 0;

constexpr int NUM_ITEM_TYPES = 37;

constexpr int NUM_ITEM_WEARS = 19;

constexpr int NUM_ITEM_FLAGS = 96;
constexpr int NUM_APPLIES = 29;

constexpr int NUM_CONT_FLAGS = 4;

constexpr int NUM_LIQ_TYPES = 16;

constexpr int NUM_MATERIALS = 47;

constexpr int NUM_HIST = 10;

constexpr int NUM_ADMFLAGS = 19;

constexpr int NUM_OF_DIRS = 12; /* number of directions in a room (nsewud) */



/* Variables for the output buffering system */
constexpr int MAX_SOCK_BUF = (96 * 1024); /* Size of kernel's sock buf   */
constexpr int MAX_PROMPT_LENGTH = 1024;   /* Max length of prompt        */
constexpr int GARBAGE_SPACE = 512;        /* Space for **OVERFLOW** etc  */
constexpr int SMALL_BUFSIZE = 6020;       /* Static output buffer size   */
/* Max amount of output that can be buffered */
constexpr int LARGE_BUFSIZE = (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH);

constexpr int HISTORY_SIZE = 5; /* Keep last 5 commands. */
constexpr int MAX_STRING_LENGTH = 64936;
constexpr int MAX_INPUT_LENGTH = 2048;     /* Max length per *line* of
 input */
constexpr int MAX_RAW_INPUT_LENGTH = 4096; /* Max size of *raw* input */
constexpr int MAX_MESSAGES = 100;
constexpr int MAX_NAME_LENGTH = 20;
constexpr int MAX_PWD_LENGTH = 30;
constexpr int MAX_TITLE_LENGTH = 120;
constexpr int HOST_LENGTH = 40;
constexpr int EXDSCR_LENGTH = 16384;
constexpr int MAX_TONGUE = 3;
constexpr int MAX_SKILLS = 200;
constexpr int MAX_AFFECT = 32;
constexpr int MAX_OBJ_AFFECT = 20;
constexpr int MAX_NOTE_LENGTH = 6000; /* arbitrary */
constexpr int SKILL_TABLE_SIZE = 1000;
constexpr int SPELLBOOK_SIZE = 50;
constexpr int MAX_FEATS = 750;
constexpr int MAX_HELP_KEYWORDS = 256;
constexpr int MAX_HELP_ENTRY = MAX_STRING_LENGTH;
constexpr int NUM_FEATS_DEFINED = 252;
constexpr int MAX_ARMOR_TYPES = 5;
constexpr int NUM_CONFIG_SECTIONS = 7;
constexpr int NUM_CREATION_METHODS = 5;
constexpr int NUM_ATTACK_TYPES = 15;
constexpr int NUM_MTRIG_TYPES = 22;
constexpr int NUM_OTRIG_TYPES = 22;
constexpr int NUM_WTRIG_TYPES = 22;
constexpr int NUM_ZONE_FLAGS = 36;
constexpr int NUM_TRADERS = 78;
constexpr int NUM_SHOP_FLAGS = 3;
constexpr int NUM_DOOR_CMD = 5;
constexpr int MAX_ASSM = 11;
constexpr int NUM_FULLNESS = 5;
constexpr int NUM_WEEK_DAYS = 7;
constexpr int NUM_MONTHS = 12;
constexpr int NUM_CONDITIONS = 3;
constexpr int NUM_WIELD_NAMES = 4;

/* define the largest set of commands for a trigger */
constexpr int MAX_CMD_LENGTH = 16384; /* 16k should be plenty and then some */

