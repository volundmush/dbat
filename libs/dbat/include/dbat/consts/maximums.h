#pragma once
#include "admlevel.h"

constexpr int SG_MIN		= 2; /* Skill gain check must be less than this
			     number in order to be successful. 
			     IE: 1% of a skill gain */

constexpr int READ_SIZE	= 256;


/* First character level that forces epic levels */
constexpr int LVL_EPICSTART		= 101;

/* Level of the 'freeze' command */
constexpr int ADMLVL_FREEZE	= ADMLVL_GRGOD;

/* Variables for the output buffering system */
constexpr int MAX_SOCK_BUF            = (96 * 1024); /* Size of kernel's sock buf   */
constexpr int MAX_PROMPT_LENGTH       = 1024; /* Max length of prompt        */
constexpr int GARBAGE_SPACE		= 512; /* Space for **OVERFLOW** etc  */
constexpr int SMALL_BUFSIZE		= 6020; /* Static output buffer size   */
/* Max amount of output that can be buffered */
constexpr int LARGE_BUFSIZE	   = (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH);

constexpr int HISTORY_SIZE		= 5; /* Keep last 5 commands. */
constexpr int MAX_STRING_LENGTH	= 64936;
constexpr int MAX_INPUT_LENGTH	= 16384; /* Max length per *line* of 
input */
constexpr int MAX_RAW_INPUT_LENGTH	= 16384; /* Max size of *raw* input */
constexpr int MAX_MESSAGES		= 100;
constexpr int MAX_NAME_LENGTH		= 20;
constexpr int MAX_PWD_LENGTH		= 30;
constexpr int MAX_TITLE_LENGTH	= 120;
constexpr int HOST_LENGTH		= 40;
constexpr int EXDSCR_LENGTH		= 16384;
constexpr int MAX_TONGUE		= 3;
constexpr int MAX_SKILLS		= 200;
constexpr int MAX_AFFECT		= 32;
constexpr int MAX_OBJ_AFFECT		= 6;
constexpr int MAX_NOTE_LENGTH		= 6000; /* arbitrary */

constexpr int SPELLBOOK_SIZE		= 50;

constexpr int MAX_HELP_KEYWORDS       = 256;
constexpr int MAX_HELP_ENTRY          = MAX_STRING_LENGTH;


constexpr int NUM_CONFIG_SECTIONS     = 7;

constexpr int NUM_ATTACK_TYPES        = 15;


constexpr int NUM_TRADERS             = 78;
constexpr int NUM_SHOP_FLAGS          = 3;
constexpr int NUM_DOOR_CMD            = 5;






/* define the largest set of commands for a trigger */
constexpr int MAX_CMD_LENGTH          = 16384; /* 16k should be plenty and then some */
