#pragma once


/*  flags: used by char_data.player_specials.pref */
#define PRF_BRIEF	0  /* Room descs won't normally be shown	*/
#define PRF_COMPACT	1  /* No extra CRLF pair before prompts		*/
#define PRF_DEAF	2  /* Can't hear shouts              		*/
#define PRF_NOTELL	3  /* Can't receive tells		    	*/
#define PRF_DISPHP	4  /* Display hit points in prompt  		*/
#define PRF_DISPMANA	5  /* Display mana points in prompt    		*/
#define PRF_DISPMOVE	6  /* Display move points in prompt 		*/
#define PRF_AUTOEXIT	7  /* Display exits in a room          		*/
#define PRF_NOHASSLE	8  /* Aggr mobs won't attack           		*/
#define PRF_QUEST	9  /* On quest					*/
#define PRF_SUMMONABLE	10 /* Can be summoned				*/
#define PRF_NOREPEAT	11 /* No repetition of comm commands		*/
#define PRF_HOLYLIGHT	12 /* Can see in dark				*/
#define PRF_COLOR	13 /* Color					*/
#define PRF_SPARE	14 /* Used to be second color bit		*/
#define PRF_NOWIZ	15 /* Can't hear wizline			*/
#define PRF_LOG1	16 /* On-line System Log (low bit)		*/
#define PRF_LOG2	17 /* On-line System Log (high bit)		*/
#define PRF_NOAUCT	18 /* Can't hear auction channel		*/
#define PRF_NOGOSS	19 /* Can't hear gossip channel			*/
#define PRF_NOGRATZ	20 /* Can't hear grats channel			*/
#define PRF_ROOMFLAGS	21 /* Can see room flags (ROOM_x)		*/
#define PRF_DISPAUTO	22 /* Show prompt HP, MP, MV when < 30%.	*/
#define PRF_CLS         23 /* Clear screen in OasisOLC 			*/
#define PRF_BUILDWALK   24 /* Build new rooms when walking		*/
#define PRF_AFK         25 /* Player is AFK				*/
#define PRF_AUTOLOOT    26 /* Loot everything from a corpse		*/
#define PRF_AUTOGOLD    27 /* Loot gold from a corpse			*/
#define PRF_AUTOSPLIT   28 /* Split gold with group			*/
#define PRF_FULL_EXIT   29 /* Shows full autoexit details		*/
#define PRF_AUTOSAC     30 /* Sacrifice a corpse 			*/
#define PRF_AUTOMEM     31 /* Memorize spells				*/
#define PRF_VIEWORDER   32 /* if you want to see the newest first 	*/
#define PRF_NOCOMPRESS  33 /* If you want to force MCCP2 off          	*/
#define PRF_AUTOASSIST  34 /* Auto-assist toggle                      	*/
#define PRF_DISPKI	35 /* Display ki points in prompt 		*/
#define PRF_DISPEXP	36 /* Display exp points in prompt 		*/
#define PRF_DISPTNL	37 /* Display TNL exp points in prompt 		*/
#define PRF_TEST        38 /* Sets triggers safety off for imms         */
#define PRF_HIDE        39 /* Hide on who from other mortals            */
#define PRF_NMWARN      40 /* No mail warning                           */
#define PRF_HINTS       41 /* Receives hints                            */
#define PRF_FURY        42 /* Sees fury meter                           */
#define PRF_NODEC       43
#define PRF_NOEQSEE     44
#define PRF_NOMUSIC     45
#define PRF_LKEEP       46
#define PRF_DISTIME     47 /* Part of Prompt Options */
#define PRF_DISGOLD     48 /* Part of Prompt Options */
#define PRF_DISPRAC     49 /* Part of Prompt Options */
#define PRF_NOPARRY     50
#define PRF_DISHUTH     51 /* Part of Prompt Options */
#define PRF_DISPERC     52 /* Part of Prompt Options */
#define PRF_CARVE       53
#define PRF_ARENAWATCH  54
#define PRF_NOGIVE      55
#define PRF_INSTRUCT    56
#define PRF_GHEALTH     57
#define PRF_IHEALTH     58
#define PRF_ENERGIZE    59

#define NUM_PRF_FLAGS   60
#define PR_ARRAY_MAX    4

extern const char *preference_bits[NUM_PRF_FLAGS+1];