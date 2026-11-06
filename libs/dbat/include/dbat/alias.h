#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

ACMD(do_alias);

void write_aliases(GameWorld *g, struct char_data *ch);
void read_aliases(GameWorld *g, struct char_data *ch);
void delete_aliases(const char *charname);

#ifdef __cplusplus
}
#endif
