#pragma once
#include <cstring>
#include <string>
#include <string_view>
#include <cctype>

#include <strings.h>

#include "dbat/consts/types.hpp"

int levenshtein_distance(const char *s1, const char *s2);
void trim(char *s);
char *add_commas(int64_t num);
char *CAP(char *txt);
/* txt with its first letter capitalised, passing over leading colour codes, as CAP(char *) does */
std::string &CAP(std::string &txt);
void prune_crlf(char *txt);
void prune_crlf(std::string &txt);

int count_color_chars(const char *string);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *one_argument(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
bool is_abbrev(const char *arg1, const char *arg2);
/* Whether arg1 is a non-empty start of arg2, ignoring case */
bool is_abbrev(std::string_view arg1, std::string_view arg2);
/* Whether a and b are the same ignoring case, as strcasecmp() finds them */
bool str_eq_nocase(std::string_view a, std::string_view b);
/* The number s starts with, read as atoi() reads it: leading whitespace and a sign allowed, the rest ignored */
int parse_int(std::string_view s);
void half_chop(char *string, char *arg1, char *arg2);
void skip_spaces(char **string);
char *delete_doubledollar(char *string);
/* s with each "$$" made "$", as the char * version does */
void delete_doubledollar(std::string &s);
int search_block(char *arg, const char *const *list, int exact);
/* search_block() without lowercasing arg in place */
int search_block(std::string_view arg, const char *const *list, int exact);
bool is_number(const char *str);
bool is_number(std::string_view str);
bool fill_word(char *argument);
char *fname(const char *namelist);
bool reserved_word(char *argument);
void search_replace(char *string, const char *find, const char *replace);
/* Replaces the first find in s, then searches again from the start, until none is left, as the char * version does
 * (so replace must not contain find; an empty find changes nothing). */
void search_replace(std::string &s, std::string_view find, std::string_view replace);
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

/* A writable copy of s (null reads as ""), for a function that may change its argument: f(writable_copy(s).data()).
 * The copy lasts until the end of the statement. */
inline std::string writable_copy(const char *s) { return s ? s : ""; }

/* s as a C string, null when empty: how the getters of std::string fields read, since the files store an
 * empty string as none. The pointer lasts until s changes. */
inline const char *c_str_or_null(const std::string &s) { return s.empty() ? nullptr : s.c_str(); }
