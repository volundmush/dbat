#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table preference_bits; /* NUM_PRF_FLAGS of them */

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

/* The old names, until every use says pref_flag:: */
constexpr pref_flag PRF_BRIEF      = pref_flag::brief;
constexpr pref_flag PRF_COMPACT    = pref_flag::compact;
constexpr pref_flag PRF_DEAF       = pref_flag::deaf;
constexpr pref_flag PRF_NOTELL     = pref_flag::notell;
constexpr pref_flag PRF_DISPHP     = pref_flag::disphp;
constexpr pref_flag PRF_DISPMANA   = pref_flag::dispmana;
constexpr pref_flag PRF_DISPMOVE   = pref_flag::dispmove;
constexpr pref_flag PRF_AUTOEXIT   = pref_flag::autoexit;
constexpr pref_flag PRF_NOHASSLE   = pref_flag::nohassle;
constexpr pref_flag PRF_QUEST      = pref_flag::quest;
constexpr pref_flag PRF_SUMMONABLE = pref_flag::summonable;
constexpr pref_flag PRF_NOREPEAT   = pref_flag::norepeat;
constexpr pref_flag PRF_HOLYLIGHT  = pref_flag::holylight;
constexpr pref_flag PRF_COLOR      = pref_flag::color;
constexpr pref_flag PRF_SPARE      = pref_flag::spare;
constexpr pref_flag PRF_NOWIZ      = pref_flag::nowiz;
constexpr pref_flag PRF_LOG1       = pref_flag::log1;
constexpr pref_flag PRF_LOG2       = pref_flag::log2;
constexpr pref_flag PRF_NOAUCT     = pref_flag::noauct;
constexpr pref_flag PRF_NOGOSS     = pref_flag::nogoss;
constexpr pref_flag PRF_NOGRATZ    = pref_flag::nogratz;
constexpr pref_flag PRF_ROOMFLAGS  = pref_flag::roomflags;
constexpr pref_flag PRF_DISPAUTO   = pref_flag::dispauto;
constexpr pref_flag PRF_CLS        = pref_flag::cls;
constexpr pref_flag PRF_BUILDWALK  = pref_flag::buildwalk;
constexpr pref_flag PRF_AFK        = pref_flag::afk;
constexpr pref_flag PRF_AUTOLOOT   = pref_flag::autoloot;
constexpr pref_flag PRF_AUTOGOLD   = pref_flag::autogold;
constexpr pref_flag PRF_AUTOSPLIT  = pref_flag::autosplit;
constexpr pref_flag PRF_FULL_EXIT  = pref_flag::full_exit;
constexpr pref_flag PRF_AUTOSAC    = pref_flag::autosac;
constexpr pref_flag PRF_AUTOMEM    = pref_flag::automem;
constexpr pref_flag PRF_VIEWORDER  = pref_flag::vieworder;
constexpr pref_flag PRF_NOCOMPRESS = pref_flag::nocompress;
constexpr pref_flag PRF_AUTOASSIST = pref_flag::autoassist;
constexpr pref_flag PRF_DISPKI     = pref_flag::dispki;
constexpr pref_flag PRF_DISPEXP    = pref_flag::dispexp;
constexpr pref_flag PRF_DISPTNL    = pref_flag::disptnl;
constexpr pref_flag PRF_TEST       = pref_flag::test;
constexpr pref_flag PRF_HIDE       = pref_flag::hide;
constexpr pref_flag PRF_NMWARN     = pref_flag::nmwarn;
constexpr pref_flag PRF_HINTS      = pref_flag::hints;
constexpr pref_flag PRF_FURY       = pref_flag::fury;
constexpr pref_flag PRF_NODEC      = pref_flag::nodec;
constexpr pref_flag PRF_NOEQSEE    = pref_flag::noeqsee;
constexpr pref_flag PRF_NOMUSIC    = pref_flag::nomusic;
constexpr pref_flag PRF_LKEEP      = pref_flag::lkeep;
constexpr pref_flag PRF_DISTIME    = pref_flag::distime;
constexpr pref_flag PRF_DISGOLD    = pref_flag::disgold;
constexpr pref_flag PRF_DISPRAC    = pref_flag::disprac;
constexpr pref_flag PRF_NOPARRY    = pref_flag::noparry;
constexpr pref_flag PRF_DISHUTH    = pref_flag::dishuth;
constexpr pref_flag PRF_DISPERC    = pref_flag::disperc;
constexpr pref_flag PRF_CARVE      = pref_flag::carve;
constexpr pref_flag PRF_ARENAWATCH = pref_flag::arenawatch;
constexpr pref_flag PRF_NOGIVE     = pref_flag::nogive;
constexpr pref_flag PRF_INSTRUCT   = pref_flag::instruct;
constexpr pref_flag PRF_GHEALTH    = pref_flag::ghealth;
constexpr pref_flag PRF_IHEALTH    = pref_flag::ihealth;
constexpr pref_flag PRF_ENERGIZE   = pref_flag::energize;

constexpr int NUM_PRF_FLAGS = static_cast<int>(flag_count<pref_flag>);
constexpr int PR_ARRAY_MAX  = 4;


