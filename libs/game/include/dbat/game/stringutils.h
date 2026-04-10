#pragma once
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "dbat/db/consts/types.h"

int levenshtein_distance(char *s1, char *s2);
void trim(char *s);
char *add_commas(int64_t num);
char *CAP(char *txt);
char *strlwr(char *s);
void prune_crlf(char *txt);

#define IS_COLOR_CHAR(c)  (c == 'n' || c == 'b' || c == 'B' || c == 'c' || \
   c == 'C' || c == 'g' || c == 'G' || c == 'm' || c == 'M' || c == 'r' || \
   c == 'R' || c == 'y' || c == 'Y' || c == 'w' || c == 'W' || c == 'k' || \
   c == 'K' || c == '0' || c == '2' || c == '3' || c == '4' || c == '5' || \
   c == '6' || c == '7' || c == 'o' || c == 'e' || c == 'u' || c == 'l') 

int count_color_chars(char *string);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *one_argument(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
char *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg);
int is_abbrev(const char *arg1, const char *arg2);
void half_chop(char *string, char *arg1, char *arg2);
void skip_spaces(char **string);
char *delete_doubledollar(char *string);
int search_block(char *arg, const char **list, int exact);
int is_number(const char *str);
int fill_word(char *argument);
char *fname(const char *namelist);
int reserved_word(char *argument);