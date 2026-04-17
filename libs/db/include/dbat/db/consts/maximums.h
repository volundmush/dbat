#pragma once

#define SG_MIN		2 /* Skill gain check must be less than this
			     number in order to be successful. 
			     IE: 1% of a skill gain */

#define READ_SIZE	256


/* First character level that forces epic levels */
#define LVL_EPICSTART		101

/* Level of the 'freeze' command */
#define ADMLVL_FREEZE	ADMLVL_GRGOD

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (96 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       1024          /* Max length of prompt        */
#define GARBAGE_SPACE		512          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE		6020        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE		5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH	64936   
#define MAX_INPUT_LENGTH	16384	/* Max length per *line* of 
input */
#define MAX_RAW_INPUT_LENGTH	16384	/* Max size of *raw* input */
#define MAX_MESSAGES		100
#define MAX_NAME_LENGTH		20
#define MAX_PWD_LENGTH		30
#define MAX_TITLE_LENGTH	120
#define HOST_LENGTH		40
#define EXDSCR_LENGTH		16384
#define MAX_TONGUE		3
#define MAX_SKILLS		200
#define MAX_AFFECT		32
#define MAX_OBJ_AFFECT		6
#define MAX_NOTE_LENGTH		6000	/* arbitrary */

#define SPELLBOOK_SIZE		50

#define MAX_HELP_KEYWORDS       256
#define MAX_HELP_ENTRY          MAX_STRING_LENGTH


#define NUM_CONFIG_SECTIONS     7

#define NUM_ATTACK_TYPES        15


#define NUM_TRADERS             78
#define NUM_SHOP_FLAGS          3
#define NUM_DOOR_CMD            5






/* define the largest set of commands for a trigger */
#define MAX_CMD_LENGTH          16384 /* 16k should be plenty and then some */
