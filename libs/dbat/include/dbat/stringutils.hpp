#pragma once
#include <cstring>
#include <string>
#include <cctype>

#include <strings.h>

#include "dbat/consts/types.hpp"

int levenshtein_distance(char *s1, char *s2);
void trim(char *s);
char *add_commas(int64_t num);
char *CAP(char *txt);
void prune_crlf(char *txt);

int count_color_chars(const char *string);
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
/* The same, as a std::string */
std::string str_format(const char *format, ...) __attribute__((format(printf, 1, 2)));
/* s, a malloc'd string (null for none), as a std::string; s is freed */
std::string take_string(char *s);
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

/* A writable copy of s (null reads as ""), for a function that may change its argument: f(writable_copy(s).data()).
 * The copy lasts until the end of the statement. */
inline std::string writable_copy(const char *s) { return s ? s : ""; }

/* s as a C string, null when empty: how the getters of std::string fields read, since the files store an
 * empty string as none. The pointer lasts until s changes. */
inline const char *c_str_or_null(const std::string &s) { return s.empty() ? nullptr : s.c_str(); }
