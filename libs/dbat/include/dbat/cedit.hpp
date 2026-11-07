#pragma once
#include <cstdio>

#include "dbat/consts/types.hpp"

bool save_config(GameWorld *g, struct zone_data *unused);                     /* Exported from cedit.c */
void config_load_option(GameWorld *g, const char *tag, char *line, FILE *fl); /* for load_config() */

