#pragma once
#include <stdio.h>
#include "dbat/consts/types.h"

bool save_config(GameWorld *g, IDXTYPE nowhere);                              /* Exported from cedit.c */
void config_load_option(GameWorld *g, const char *tag, char *line, FILE *fl); /* for load_config() */
