#pragma once
#include <cstring>
#include <cctype>

#include <strings.h>

#include "dbat/consts/types.hpp"

int levenshtein_distance(char *s1, char *s2);
void trim(char *s);
char *add_commas(int64_t num);
char *CAP(char *txt);
void prune_crlf(char *txt);

int count_color_chars(char *string);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *one_argument(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
bool is_abbrev(const char *arg1, const char *arg2);
void half_chop(char *string, char *arg1, char *arg2);
void skip_spaces(char **string);
char *delete_doubledollar(char *string);
int search_block(char *arg, const char *const *list, int exact);
bool is_number(const char *str);
bool fill_word(char *argument);
char *fname(const char *namelist);
bool reserved_word(char *argument);
void search_replace(char *string, const char *find, const char *replace);
/* A new string formatted as by printf(); aborts if out of memory. */
char *str_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
/* A zero-filled buffer of n chars. Like strdup()'s strings, it is released with free(), so that a field can hold
 * either; both go when strings become std::string. */
char *str_alloc(size_t n);
/* realloc() for a str_alloc() or strdup() buffer, aborting if out of memory. */
char *str_realloc(char *s, size_t n);

/* A writable copy of a fixed string, for passing to a function that may change its argument, as most commands
 * can: do_barrier(g, ch, writable{"40"}, 0, 0). The copy lasts until the end of the statement. A string
 * literal, or a compound literal like (char[]){"40"}, can be in read-only memory. */
template <size_t N> struct writable {
  char text[N]{};
  operator char *() { return text; }
};
template <size_t N> writable(const char (&)[N]) -> writable<N>;
