/* ************************************************************************
*   File: interpreter.h                                 Part of CircleMUD *
*  Usage: header file: public procs, macro defs, subcommand defines       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include "structs.h"

#define CMD_NAME (complete_cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, complete_cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (complete_cmd_info[cmdnum].command_pointer == do_move)

extern const char *list_bonus[];
extern const struct command_info cmd_info[];

void userLoad(struct descriptor_data *d, char *name);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
void topLoad(void);
void topWrite(struct char_data *ch);
char *rIntro(struct char_data *ch, char *arg);
int special(struct char_data *ch, int cmd, char *arg);
void fingerUser(struct char_data *ch, char *name);
int readUserIndex(char *name);
void userWrite(struct descriptor_data *d, int setTot, int setRpp, int setRBank, char *name);
void	command_interpreter(struct char_data *ch, char *argument);
int	search_block(char *arg, const char **list, int exact);
char	lower( char c );
char	*one_argument(char *argument, char *first_arg);
char	*one_word(char *argument, char *first_arg);
char	*any_one_arg(char *argument, char *first_arg);
char	*two_arguments(char *argument, char *first_arg, char *second_arg);
char    *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg);
int	fill_word(char *argument);
void	half_chop(char *string, char *arg1, char *arg2);
void	nanny(struct descriptor_data *d, char *arg);
int	is_abbrev(const char *arg1, const char *arg2);
int	is_number(const char *str);
int	find_command(const char *command);
void	skip_spaces(char **string);
char	*delete_doubledollar(char *string);

/* WARNING: if you have added diagonal directions and have them at the
 * beginning of the command list.. change this value to 11 or 15 (depending) */
/* reserve these commands to come straight from the cmd list then start
 * sorting */
#define RESERVE_CMDS               15

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)


struct command_info {
   const char *command;
   const char *sort_as;
   int8_t minimum_position;
   void	(*command_pointer)
	   (struct char_data *ch, char *argument, int cmd, int subcmd);
   int16_t minimum_level;
   int16_t minimum_admlevel;
   int	subcmd;
};

/*
 * Necessary for CMD_IS macro.  Borland needs the structure defined first
 * so it has been moved down here.
 */
extern struct command_info *complete_cmd_info;

/*
 * Alert! Changed from 'struct alias' to 'struct alias_data' in bpl15
 * because a Windows 95 compiler gives a warning about it having similiar
 * named member.
 */
struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_NORTH	1
#define SCMD_EAST	2
#define SCMD_SOUTH	3
#define SCMD_WEST	4
#define SCMD_UP		5
#define SCMD_DOWN	6
#define SCMD_NW         7
#define SCMD_NE         8
#define SCMD_SE         9
#define SCMD_SW        10
#define SCMD_IN        11
#define SCMD_OUT       12

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1 
#define SCMD_CREDITS    2
#define SCMD_NEWS       3
#define SCMD_WIZLIST    4
#define SCMD_POLICIES   5
#define SCMD_VERSION    6
#define SCMD_IMMLIST    7
#define SCMD_MOTD	8
#define SCMD_IMOTD	9
#define SCMD_CLEAR	10
#define SCMD_WHOAMI	11

/* do_gen_tog */
#define SCMD_NOSUMMON   0
#define SCMD_NOHASSLE   1
#define SCMD_BRIEF      2
#define SCMD_COMPACT    3
#define SCMD_NOTELL	4
#define SCMD_NOAUCTION	5
#define SCMD_DEAF	6
#define SCMD_NOGOSSIP	7
#define SCMD_NOGRATZ	8
#define SCMD_NOWIZ	9
#define SCMD_QUEST	10
#define SCMD_ROOMFLAGS	11
#define SCMD_NOREPEAT	12
#define SCMD_HOLYLIGHT	13
#define SCMD_SLOWNS	14
#define SCMD_AUTOEXIT	15
#define SCMD_TRACK	16
#define SCMD_BUILDWALK  17
#define SCMD_AFK        18
#define SCMD_AUTOASSIST 19
#define SCMD_AUTOLOOT   20
#define SCMD_AUTOGOLD   21
#define SCMD_CLS        22
#define SCMD_AUTOSPLIT  23
#define SCMD_AUTOSAC    24
#define SCMD_SNEAK	25
#define SCMD_HIDE	26
#define SCMD_AUTOMEM	27
#define SCMD_VIEWORDER  28
#define SCMD_NOCOMPRESS 29
#define SCMD_TEST       30
#define SCMD_WHOHIDE    31
#define SCMD_NMWARN     32
#define SCMD_HINTS      33
#define SCMD_NODEC      34
#define SCMD_NOEQSEE    35
#define SCMD_NOMUSIC    36
#define SCMD_NOPARRY    37
#define SCMD_LKEEP      38
#define SCMD_CARVE      39
#define SCMD_NOGIVE     40
#define SCMD_INSTRUCT   41
#define SCMD_GHEALTH    42
#define SCMD_IHEALTH    43

/* do_wizutil */
#define SCMD_REROLL	0
#define SCMD_PARDON     1
#define SCMD_NOTITLE    2
#define SCMD_SQUELCH    3
#define SCMD_FREEZE	4
#define SCMD_THAW	5
#define SCMD_UNAFFECT	6

/* do_spec_com */
#define SCMD_WHISPER	0
#define SCMD_ASK	1

/* do_gen_com */
#define SCMD_HOLLER	0
#define SCMD_SHOUT	1
#define SCMD_GOSSIP	2
#define SCMD_AUCTION	3
#define SCMD_GRATZ	4
#define SCMD_GEMOTE	5

/* do_shutdown */
#define SCMD_SHUTDOW	0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI	0
#define SCMD_QUIT	1

/* do_date */
#define SCMD_DATE	0
#define SCMD_UPTIME	1

/* do_commands */
#define SCMD_COMMANDS	0
#define SCMD_SOCIALS	1
#define SCMD_WIZHELP	2

/* do_drop */
#define SCMD_DROP	0
#define SCMD_JUNK	1
#define SCMD_DONATE	2

/* do_gen_write */
#define SCMD_BUG	0
#define SCMD_TYPO	1
#define SCMD_IDEA	2

/* do_look */
#define SCMD_LOOK	0
#define SCMD_READ	1
#define SCMD_SEARCH     2

/* do_qcomm */
#define SCMD_QSAY	0
#define SCMD_QECHO	1

/* do_pour */
#define SCMD_POUR	0
#define SCMD_FILL	1

/* do_poof */
#define SCMD_POOFIN	0
#define SCMD_POOFOUT	1

/* do_hit */
#define SCMD_HIT	0
#define SCMD_MURDER	1

/* do_eat */
#define SCMD_EAT	0
#define SCMD_TASTE	1
#define SCMD_DRINK	2
#define SCMD_SIP	3

/* do_use */
#define SCMD_USE	0
#define SCMD_QUAFF	1
#define SCMD_RECITE	2

/* do_echo */
#define SCMD_ECHO	0
#define SCMD_EMOTE	1
#define SCMD_SMOTE      2

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4

/* do_olc */
#define SCMD_OASIS_REDIT      0
#define SCMD_OASIS_OEDIT      1
#define SCMD_OASIS_ZEDIT      2
#define SCMD_OASIS_MEDIT      3
#define SCMD_OASIS_SEDIT      4
#define SCMD_OASIS_CEDIT      5
#define SCMD_OLC_SAVEINFO     7
#define SCMD_OASIS_RLIST      8
#define SCMD_OASIS_MLIST      9
#define SCMD_OASIS_OLIST      10
#define SCMD_OASIS_SLIST      11
#define SCMD_OASIS_ZLIST      12
#define SCMD_OASIS_TRIGEDIT   13
#define SCMD_OASIS_AEDIT      14
#define SCMD_OASIS_TLIST      15
#define SCMD_OASIS_LINKS      16
#define SCMD_OASIS_GEDIT      17
#define SCMD_OASIS_GLIST      18
#define SCMD_OASIS_HEDIT      19
#define SCMD_OASIS_HSEDIT     20

/* do_builder_list */

#define SCMD_RLIST  0
#define SCMD_OLIST  1
#define SCMD_MLIST  2
#define SCMD_TLIST  3
#define SCMD_SLIST  4
#define SCMD_GLIST  5

/* * do_assemble * These constants *must* corespond with
     the ASSM_xxx constants in * assemblies.h. */
#define SCMD_MAKE      0
#define SCMD_BAKE      1
#define SCMD_BREW      2
#define SCMD_ASSEMBLE  3
#define SCMD_CRAFT     4
#define SCMD_FLETCH    5
#define SCMD_KNIT      6
#define SCMD_MIX       7
#define SCMD_THATCH    8
#define SCMD_WEAVE     9
#define SCMD_FORGE     10


#define SCMD_MEMORIZE  1
#define SCMD_FORGET    2
#define SCMD_STOP      3
#define SCMD_WHEN_SLOT 4

/* do_value list */
#define SCMD_WIMPY	0
#define SCMD_POWERATT	1
#define SCMD_COMBATEXP  2

/* do_cast */
#define SCMD_CAST	0
#define SCMD_ART	1

/* oasis_copy */
#define SCMD_TEDIT      0
#define SCMD_REDIT      1
#define SCMD_OEDIT      2
#define SCMD_MEDIT      3

#endif