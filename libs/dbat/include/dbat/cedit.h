#pragma once
#include <stdio.h>
#include "dbat/consts/types.h"

int save_config(IDXTYPE nowhere);                               /* Exported from cedit.c */
void config_load_option(const char *tag, char *line, FILE *fl); /* for load_config() */
