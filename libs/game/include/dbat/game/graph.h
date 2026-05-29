#pragma once
#include "dbat/db/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BFS_ERROR		(-1)
#define BFS_ALREADY_THERE	(-2)
#define BFS_TO_FAR              (-3)
#define BFS_NO_PATH		(-4)

// functions
int find_first_step(struct room_data *src, struct room_data *target);

ACMD(do_track);
ACMD(do_sradar);
ACMD(do_radar);

#ifdef __cplusplus
}
#endif
