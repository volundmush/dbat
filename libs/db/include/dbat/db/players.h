#pragma once
#include "consts/types.h"

struct player_index_element {
   char	*name;
   long id;
   int level;
   int admlevel;
   int flags;
   time_t last;
   int ship;
   int shiproom;
   time_t played;
   char *clan;
};

extern int top_of_p_table;
extern struct player_index_element *player_table;