#pragma once
#include <string>

#include "dbat/consts/types.hpp"
#include "dbat/db.hpp"
#include "dbat/flag_set.hpp"

struct player_index_element {
  std::string name; /* lowercase; empty for a deleted character */
  long id{};
  int level{};
  int admlevel{};
  flag_set<pindex_flag> flags;
  time_t last{};
  int ship{};
  int shiproom{};
  time_t played{};
};

/* Whether a player index entry has flag PINDEX_*, and setting or clearing one */
bool pindex_flagged(const player_index_element &entry, pindex_flag flag);
void pindex_set_flag(player_index_element &entry, pindex_flag flag, bool on);

void remove_player(GameWorld *g, int pfilepos);
void clean_pfiles(GameWorld *g);
void build_player_index(GameWorld *g);

