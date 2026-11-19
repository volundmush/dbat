#pragma once
#include "dbat/consts/types.hpp"

#include <string_view>

/* Character creation's stat points (creation methods 2 and 3): the pool the new character's base stats are bought
   from, kept as d's session */
void start_stat_points(GameWorld *g, struct descriptor_data *d, int points);
/* Shows the stats and the points left, and what to type */
void stats_disp_menu(GameWorld *g, struct descriptor_data *d);
/* A line typed at the stats menu; true when the player is done */
bool parse_stats(GameWorld *g, struct descriptor_data *d, std::string_view line);

