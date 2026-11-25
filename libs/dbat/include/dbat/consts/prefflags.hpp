#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table preference_bits; /* flag_count<pref_flag> of them */

/* Preference flags: player_special_data.pref */
enum class pref_flag : uint8_t {
  brief      = 0,   /* Room descs won't normally be shown	*/
  compact    = 1,   /* No extra CRLF pair before prompts		*/
  deaf       = 2,   /* Can't hear shouts              		*/
  notell     = 3,   /* Can't receive tells		    	*/
  disphp     = 4,   /* Display hit points in prompt  		*/
  dispmana   = 5,   /* Display mana points in prompt    		*/
  dispmove   = 6,   /* Display move points in prompt 		*/
  autoexit   = 7,   /* Display exits in a room          		*/
  nohassle   = 8,   /* Aggr mobs won't attack           		*/
  quest      = 9,   /* On quest					*/
  summonable = 10,  /* Can be summoned				*/
  norepeat   = 11,  /* No repetition of comm commands		*/
  holylight  = 12,  /* Can see in dark				*/
  color      = 13,  /* Color					*/
  spare      = 14,  /* Used to be second color bit		*/
  nowiz      = 15,  /* Can't hear wizline			*/
  log1       = 16,  /* On-line System Log (low bit)		*/
  log2       = 17,  /* On-line System Log (high bit)		*/
  noauct     = 18,  /* Can't hear auction channel		*/
  nogoss     = 19,  /* Can't hear gossip channel			*/
  nogratz    = 20,  /* Can't hear grats channel			*/
  roomflags  = 21,  /* Can see room flags (ROOM_x)		*/
  dispauto   = 22,  /* Show prompt HP, MP, MV when < 30%.	*/
  cls        = 23,  /* Clear screen in OasisOLC 			*/
  buildwalk  = 24,  /* Build new rooms when walking		*/
  afk        = 25,  /* Player is AFK				*/
  autoloot   = 26,  /* Loot everything from a corpse		*/
  autogold   = 27,  /* Loot gold from a corpse			*/
  autosplit  = 28,  /* Split gold with group			*/
  full_exit  = 29,  /* Shows full autoexit details		*/
  autosac    = 30,  /* Sacrifice a corpse 			*/
  automem    = 31,  /* Memorize spells				*/
  vieworder  = 32,  /* if you want to see the newest first 	*/
  nocompress = 33,  /* If you want to force MCCP2 off          	*/
  autoassist = 34,  /* Auto-assist toggle                      	*/
  dispki     = 35,  /* Display ki points in prompt 		*/
  dispexp    = 36,  /* Display exp points in prompt 		*/
  disptnl    = 37,  /* Display TNL exp points in prompt 		*/
  test       = 38,  /* Sets triggers safety off for imms         */
  hide       = 39,  /* Hide on who from other mortals            */
  nmwarn     = 40,  /* No mail warning                           */
  hints      = 41,  /* Receives hints                            */
  fury       = 42,  /* Sees fury meter                           */
  nodec      = 43,
  noeqsee    = 44,
  nomusic    = 45,
  lkeep      = 46,
  distime    = 47,  /* Part of Prompt Options */
  disgold    = 48,  /* Part of Prompt Options */
  disprac    = 49,  /* Part of Prompt Options */
  noparry    = 50,
  dishuth    = 51,  /* Part of Prompt Options */
  disperc    = 52,  /* Part of Prompt Options */
  carve      = 53,
  arenawatch = 54,
  nogive     = 55,
  instruct   = 56,
  ghealth    = 57,
  ihealth    = 58,
  energize   = 59,
  count_     = 60,
};
template <> struct flag_traits<pref_flag> {
  static constexpr const name_table &names = preference_bits;
};


