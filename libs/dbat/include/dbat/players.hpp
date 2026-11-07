#pragma once
#include "dbat/consts/types.hpp"

struct player_index_element {
  char *name;
  long id;
  int level;
  int admlevel;
  int flags;
  time_t last;
  int ship;
  int shiproom;
  time_t played;
};


void remove_player(GameWorld *g, int pfilepos);
void clean_pfiles(GameWorld *g);
void build_player_index(GameWorld *g);

