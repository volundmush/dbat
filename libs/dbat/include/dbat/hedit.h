#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hedit_parse(GameWorld *g, struct descriptor_data *, char *);
void hedit_string_cleanup(GameWorld *g, struct descriptor_data *, int);

ACMD(do_oasis_hedit);
ACMD(do_helpcheck);
ACMD(do_hindex);

#ifdef __cplusplus
}
#endif
