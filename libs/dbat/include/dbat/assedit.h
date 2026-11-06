#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void assedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);

ACMD(do_assedit);

#ifdef __cplusplus
}
#endif
