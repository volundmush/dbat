#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
