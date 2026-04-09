#pragma once
#include "dbat/db/consts/types.h"

#define BFS_ERROR		(-1)
#define BFS_ALREADY_THERE	(-2)
#define BFS_TO_FAR              (-3)
#define BFS_NO_PATH		(-4)

// functions
int find_first_step(room_rnum src, room_rnum target);

ACMD(do_track);
ACMD(do_sradar);
ACMD(do_radar);