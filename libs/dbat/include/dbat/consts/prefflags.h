#pragma once

/*  flags: used by char_data.player_specials.pref */
constexpr int PRF_BRIEF      = 0;  /* Room descs won't normally be shown	*/
constexpr int PRF_COMPACT    = 1;  /* No extra CRLF pair before prompts		*/
constexpr int PRF_DEAF       = 2;  /* Can't hear shouts              		*/
constexpr int PRF_NOTELL     = 3;  /* Can't receive tells		    	*/
constexpr int PRF_DISPHP     = 4;  /* Display hit points in prompt  		*/
constexpr int PRF_DISPMANA   = 5;  /* Display mana points in prompt    		*/
constexpr int PRF_DISPMOVE   = 6;  /* Display move points in prompt 		*/
constexpr int PRF_AUTOEXIT   = 7;  /* Display exits in a room          		*/
constexpr int PRF_NOHASSLE   = 8;  /* Aggr mobs won't attack           		*/
constexpr int PRF_QUEST      = 9;  /* On quest					*/
constexpr int PRF_SUMMONABLE = 10; /* Can be summoned				*/
constexpr int PRF_NOREPEAT   = 11; /* No repetition of comm commands		*/
constexpr int PRF_HOLYLIGHT  = 12; /* Can see in dark				*/
constexpr int PRF_COLOR      = 13; /* Color					*/
constexpr int PRF_SPARE      = 14; /* Used to be second color bit		*/
constexpr int PRF_NOWIZ      = 15; /* Can't hear wizline			*/
constexpr int PRF_LOG1       = 16; /* On-line System Log (low bit)		*/
constexpr int PRF_LOG2       = 17; /* On-line System Log (high bit)		*/
constexpr int PRF_NOAUCT     = 18; /* Can't hear auction channel		*/
constexpr int PRF_NOGOSS     = 19; /* Can't hear gossip channel			*/
constexpr int PRF_NOGRATZ    = 20; /* Can't hear grats channel			*/
constexpr int PRF_ROOMFLAGS  = 21; /* Can see room flags (ROOM_x)		*/
constexpr int PRF_DISPAUTO   = 22; /* Show prompt HP, MP, MV when < 30%.	*/
constexpr int PRF_CLS        = 23; /* Clear screen in OasisOLC 			*/
constexpr int PRF_BUILDWALK  = 24; /* Build new rooms when walking		*/
constexpr int PRF_AFK        = 25; /* Player is AFK				*/
constexpr int PRF_AUTOLOOT   = 26; /* Loot everything from a corpse		*/
constexpr int PRF_AUTOGOLD   = 27; /* Loot gold from a corpse			*/
constexpr int PRF_AUTOSPLIT  = 28; /* Split gold with group			*/
constexpr int PRF_FULL_EXIT  = 29; /* Shows full autoexit details		*/
constexpr int PRF_AUTOSAC    = 30; /* Sacrifice a corpse 			*/
constexpr int PRF_AUTOMEM    = 31; /* Memorize spells				*/
constexpr int PRF_VIEWORDER  = 32; /* if you want to see the newest first 	*/
constexpr int PRF_NOCOMPRESS = 33; /* If you want to force MCCP2 off          	*/
constexpr int PRF_AUTOASSIST = 34; /* Auto-assist toggle                      	*/
constexpr int PRF_DISPKI     = 35; /* Display ki points in prompt 		*/
constexpr int PRF_DISPEXP    = 36; /* Display exp points in prompt 		*/
constexpr int PRF_DISPTNL    = 37; /* Display TNL exp points in prompt 		*/
constexpr int PRF_TEST       = 38; /* Sets triggers safety off for imms         */
constexpr int PRF_HIDE       = 39; /* Hide on who from other mortals            */
constexpr int PRF_NMWARN     = 40; /* No mail warning                           */
constexpr int PRF_HINTS      = 41; /* Receives hints                            */
constexpr int PRF_FURY       = 42; /* Sees fury meter                           */
constexpr int PRF_NODEC      = 43;
constexpr int PRF_NOEQSEE    = 44;
constexpr int PRF_NOMUSIC    = 45;
constexpr int PRF_LKEEP      = 46;
constexpr int PRF_DISTIME    = 47; /* Part of Prompt Options */
constexpr int PRF_DISGOLD    = 48; /* Part of Prompt Options */
constexpr int PRF_DISPRAC    = 49; /* Part of Prompt Options */
constexpr int PRF_NOPARRY    = 50;
constexpr int PRF_DISHUTH    = 51; /* Part of Prompt Options */
constexpr int PRF_DISPERC    = 52; /* Part of Prompt Options */
constexpr int PRF_CARVE      = 53;
constexpr int PRF_ARENAWATCH = 54;
constexpr int PRF_NOGIVE     = 55;
constexpr int PRF_INSTRUCT   = 56;
constexpr int PRF_GHEALTH    = 57;
constexpr int PRF_IHEALTH    = 58;
constexpr int PRF_ENERGIZE   = 59;

constexpr int NUM_PRF_FLAGS = 60;
constexpr int PR_ARRAY_MAX  = 4;

extern const char *preference_bits[NUM_PRF_FLAGS + 1];