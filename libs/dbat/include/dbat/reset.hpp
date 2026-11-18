#pragma once
#include "dbat/consts/types.hpp"

/* returned by pre-reset function */
constexpr int PERFORM_NORMAL_RESET = 0;
constexpr int BLOCK_NORMAL_RESET   = 1;

/* Zone virtual numbers used by reset.c */
constexpr int RESET_GAUNTLET = 328;

/* Miscellaneous defines */
constexpr int NUM_GAUNTLET_ROOMS = 20;

struct gauntlet_mob {
  room_vnum vroom{};
  mob_vnum vmob{};
};

/* Reset functions */
bool pre_reset(GameWorld *g, zone_vnum znum);
void post_reset(zone_vnum znum);
/* Logs a bad command in zone's reset table, cmd_no its index */
void log_zone_error(GameWorld *g, struct zone_data *zone, int cmd_no, const char *message);

/* Zone pre-reset functions */

/* Zone post-reset functions */

/* Other local functions */

