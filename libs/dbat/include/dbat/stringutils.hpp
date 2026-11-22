#pragma once
#include <cstring>
#include <string>
#include <string_view>
#include <cctype>

#include <strings.h>

#include "dbat/consts/types.hpp"

/* How many letters must be added, removed or changed to turn s1 into s2 */
int levenshtein_distance(std::string_view s1, std::string_view s2);
/* s without the spaces and tabs at either end */
void trim(std::string &s);
/* num with a comma between each group of three digits: 1,234,567 */
std::string add_commas(int64_t num);
/* txt with its first letter capitalised, passing over leading colour codes */
std::string &CAP(std::string &txt);
void prune_crlf(std::string &txt);

/* How many of text's characters are colour codes, which take no room on the screen: 1 for "@@" (one "@" shows),
   4 for "@[" and 2 for any other "@" code, a lone "@" at the end included */
int count_color_chars(std::string_view text);
/* count_color_chars() for a getter that gives null for no text, until they give std::string_views */
inline int count_color_chars(const char *text) { return text ? count_color_chars(std::string_view(text)) : 0; }
/* Whether arg1 is a non-empty start of arg2, ignoring case */
bool is_abbrev(std::string_view arg1, std::string_view arg2);
/* Whether a and b are the same ignoring case, as strcasecmp() finds them */
bool str_eq_nocase(std::string_view a, std::string_view b);
/* Copies src into dst, a buffer of size bytes, cut short to fit and always ended by a '\0', as strlcpy() did;
   nothing is written if size is 0 */
void copy_cstr(char *dst, std::string_view src, size_t size);
/* The number s starts with, read as atoi() reads it: leading whitespace and a sign allowed, the rest ignored */
int parse_int(std::string_view s);
/* s with each "$$" made "$" */
void delete_doubledollar(std::string &s);
/* The place of arg in list, a table ended by "\n", ignoring case: the whole entry if exact, else the first that
   starts with arg. -1 if there is none, or if arg starts with '!'. */
int search_block(std::string_view arg, const char *const *list, int exact);
bool is_number(std::string_view str);
/* The first keyword of a name list: its leading letters */
std::string_view fname(std::string_view namelist);
/* Replaces the first find in s, then searches again from the start, until none is left
 * (so replace must not contain find; an empty find changes nothing). */
void search_replace(std::string &s, std::string_view find, std::string_view replace);
/* s, a malloc'd string (null for none), as a std::string; s is freed */
std::string take_string(char *s);
/* A zero-filled buffer of n chars. Like strdup()'s strings, it is released with free(), so that a field can hold
 * either; both go when strings become std::string. */
char *str_alloc(size_t n);

/* A writable copy of s (null reads as ""), for a function that may change its argument: f(writable_copy(s).data()).
 * The copy lasts until the end of the statement. */
inline std::string writable_copy(const char *s) { return s ? s : ""; }

/* s as a C string, null when empty: how the getters of std::string fields read, since the files store an
 * empty string as none. The pointer lasts until s changes. */
inline const char *c_str_or_null(const std::string &s) { return s.empty() ? nullptr : s.c_str(); }
