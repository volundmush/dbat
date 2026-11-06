#pragma once
#include "dbat/consts/types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* get_filename() */
constexpr int CRASH_FILE       = 0;
constexpr int ETEXT_FILE       = 1;
constexpr int ALIAS_FILE       = 2;
constexpr int SCRIPT_VARS_FILE = 3;
constexpr int NEW_OBJ_FILES    = 4;
constexpr int PLR_FILE         = 5;
constexpr int PET_FILE         = 6;
constexpr int USER_FILE        = 8; /* User Account System */
constexpr int INTRO_FILE       = 9;
constexpr int SENSE_FILE       = 10;
constexpr int CUSTOME_FILE     = 11;
constexpr int MAX_FILES        = 12;

int get_line(FILE *fl, char *buf);
bool get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
int touch(const char *path);
void topLoad(GameWorld *g);
void topWrite(GameWorld *g, struct char_data *ch);

#ifdef __cplusplus
}
#endif
