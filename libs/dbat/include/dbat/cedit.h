#pragma once
#include <stdio.h>
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool save_config(GameWorld *g, IDXTYPE nowhere);                              /* Exported from cedit.c */
void config_load_option(GameWorld *g, const char *tag, char *line, FILE *fl); /* for load_config() */

#ifdef __cplusplus
}
#endif
