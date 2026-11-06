#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool buildwalk(GameWorld *g, struct char_data *ch, int dir);

// commands
ACMD(do_dig);

#ifdef __cplusplus
}
#endif
