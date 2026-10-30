#pragma once
#include <stdio.h>

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2
#define SCRIPT_VARS_FILE 3
#define NEW_OBJ_FILES   4
#define PLR_FILE        5
#define PET_FILE        6
#define USER_FILE       8 /* User Account System */
#define INTRO_FILE      9
#define SENSE_FILE      10
#define CUSTOME_FILE    11
#define MAX_FILES       12

int get_line(FILE *fl, char *buf);
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
int touch(const char *path);