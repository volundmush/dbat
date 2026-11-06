#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void aedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);

// commands
ACMD(do_oasis_aedit);

ACMD(do_astat);

#ifdef __cplusplus
}
#endif
