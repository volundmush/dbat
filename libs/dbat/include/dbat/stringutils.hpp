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
/* Whether arg1 is a non-empty start of arg2, ignoring case */
bool is_abbrev(std::string_view arg1, std::string_view arg2);
/* Whether a and b are the same ignoring case, as strcasecmp() finds them */
bool str_eq_nocase(std::string_view a, std::string_view b);
/* Whether a sorts before b ignoring case, as strcasecmp() orders them */
bool str_less_nocase(std::string_view a, std::string_view b);
/* How the first n characters of a and b compare ignoring case, as strncasecmp() finds them: the end of either
   reads as a '\0' */
int compare_nocase_n(std::string_view a, std::string_view b, size_t n);

/* s with its first byte in upper case, colour codes and all (CAP skips those); empty stays empty */
std::string upper_first(std::string_view s);

/* The line's first character, or '\0' if it is empty, as *line read it when it was a C string */
constexpr char first_char(std::string_view line) { return line.empty() ? '\0' : line.front(); }
/* s, or otherwise when s is empty: the getters' "none" is empty, where it was null (x ? x : otherwise) */
constexpr std::string_view or_if_empty(std::string_view s, std::string_view otherwise) { return s.empty() ? otherwise : s; }
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
/* s in lowercase, as tolower() makes each character */
std::string to_lower(std::string_view s);
