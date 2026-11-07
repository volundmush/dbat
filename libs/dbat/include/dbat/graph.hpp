#pragma once
#include "dbat/consts/types.hpp"

constexpr int BFS_ERROR         = (-1);
constexpr int BFS_ALREADY_THERE = (-2);
constexpr int BFS_TO_FAR        = (-3);
constexpr int BFS_NO_PATH       = (-4);

// functions
int find_first_step(GameWorld *g, room_rnum src, room_rnum target);

ACMD(do_track);
ACMD(do_sradar);
ACMD(do_radar);

