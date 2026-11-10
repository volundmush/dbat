#include <algorithm>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/log.hpp"
#include "dbat/consts/maximums.hpp"

int levenshtein_distance(const char *s1, const char *s2)
{
  int s1_len = strlen(s1), s2_len = strlen(s2);
  int **d, i, j;

  d = new int *[s1_len + 1]();
  for (i = 0; i <= s1_len; i++) {
    d[i]    = new int[s2_len + 1]();
    d[i][0] = i;
  }

  for (j = 0; j <= s2_len; j++)
    d[0][j] = j;
  for (i = 1; i <= s1_len; i++)
    for (j = 1; j <= s2_len; j++)
      d[i][j] = MIN(d[i - 1][j] + 1, MIN(d[i][j - 1] + 1,
                                         d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

  i = d[s1_len][s2_len];

  for (j = 0; j <= s1_len; j++)
    delete[] d[j];
  delete[] d;

  return i;
}

int count_color_chars(const char *string)
{
  int i, len;
  int num = 0;

  if (!string || !*string)
    return 0;

  len = strlen(string);
  for (i = 0; i < len; i++) {
    while (string[i] == '@') {
      if (string[i + 1] == '@') {
        num++;
      } else if (string[i + 1] == '[') {
        num += 4;
      } else {
        num += 2;
      }
      i += 2;
    }
  }
  return num;
}

/* Trims leading and trailing spaces from string */
void trim(std::string &s)
{
  size_t start = s.find_first_not_of(" \t");

  if (start == std::string::npos) { /* empty or all blank */
    s.clear();
    return;
  }
  s.erase(s.find_last_not_of(" \t") + 1);
  s.erase(0, start);
}

/* Turns number into string and adds commas to it. */
char *add_commas(int64_t num)
{
  constexpr int DIGITS_PER_GROUP  = 3;
  constexpr int BUFFER_COUNT      = 19;
  constexpr int DIGITS_PER_BUFFER = 25;

  int64_t i, j, len, negative = (num < 0);
  static char comma_string[BUFFER_COUNT][DIGITS_PER_BUFFER];
  static int64_t which = 0;

  std::string num_string = str_format("{:d}", num);
  len                    = static_cast<int64_t>(num_string.size());

  for (i = j = 0; num_string[i]; ++i) {
    if ((len - i) % DIGITS_PER_GROUP == 0 && i && i - negative)
      comma_string[which][j++] = ',';
    comma_string[which][j++] = num_string[i];
  }
  comma_string[which][j] = '\0';

  i     = which;
  which = (which + 1) % BUFFER_COUNT;

  return comma_string[i];
}

#define IS_COLOR_CHAR(c) (c == 'n' || c == 'b' || c == 'B' || c == 'c' ||                         \
                          c == 'C' || c == 'g' || c == 'G' || c == 'm' || c == 'M' || c == 'r' || \
                          c == 'R' || c == 'y' || c == 'Y' || c == 'w' || c == 'W' || c == 'k' || \
                          c == 'K' || c == '0' || c == '2' || c == '3' || c == '4' || c == '5' || \
                          c == '6' || c == '7' || c == 'o' || c == 'e' || c == 'u' || c == 'l')

char *CAP(char *txt)
{
  int i;
  for (i = 0; txt[i] != '\0' && (txt[i] == '@' && IS_COLOR_CHAR(txt[i + 1])); i += 2);

  txt[i] = toupper(txt[i]);
  return (txt);
}

/* Strips \r\n from end of string.  */
/* Removes the line endings from the end of txt. */
void prune_crlf(char *txt)
{
  size_t n = strlen(txt);

  while (n > 0 && (txt[n - 1] == '\n' || txt[n - 1] == '\r'))
    txt[--n] = '\0';
}

void prune_crlf(std::string &txt)
{
  while (!txt.empty() && (txt.back() == '\n' || txt.back() == '\r'))
    txt.pop_back();
}

char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    *first_arg = '\0';
    return (nullptr);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = tolower(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}

/*
 * one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word.
 *
 * No longer ignores fill words.  -dak, 6 Jan 2003.
 */
char *one_word(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  if (*argument == '\"') {
    argument++;
    while (*argument && *argument != '\"') {
      *(first_arg++) = tolower(*argument);
      argument++;
    }
    if (*argument) /* the closing quote, if the word has one */
      argument++;
  } else {
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = tolower(*argument);
      argument++;
    }
  }

  *first_arg = '\0';
  return (argument);
}

/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = tolower(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

/*
 * Same as two_arguments only, well you get the idea... - Iovan
 *
 */
bool is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return false;

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (tolower(*arg1) != tolower(*arg2))
      return false;

  if (!*arg1)
    return true;
  else
    return false;
}

/*
 * Return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string)
 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  if (arg2 != temp)
    memmove(arg2, temp, strlen(temp) + 1); /* arg2 may overlap string (half_chop(buf, word, buf)) */
}

void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}

/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
void delete_doubledollar(std::string &s)
{
  std::string out;

  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    out += s[i];
    if (s[i] == '$' && i + 1 < s.size() && s[i + 1] == '$')
      i++; /* skip the second of 2 $'s in a row */
  }
  s = std::move(out);
}

char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == nullptr)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;

  while (*ddread)                            /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
        ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char *const *list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae.
   */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = tolower(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
        return (i);
  } else {
    if (!l)
      l = 1; /* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
        return (i);
  }

  return (-1);
}

bool is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return false;

  return true;
}

static const char *const fill[] =
    {
        "in",
        "into",
        "from",
        "with",
        "the",
        "on",
        "at",
        "to",
        "\n"};

bool fill_word(char *argument)
{
  return (search_block(argument, fill, true) >= 0);
}

static const char *const reserved[] =
    {
        "a",
        "an",
        "self",
        "me",
        "all",
        "room",
        "someone",
        "something",
        "\n"};

bool reserved_word(char *argument)
{
  return (search_block(argument, reserved, true) >= 0);
}

char *fname(const char *namelist)
{
  static char holder[READ_SIZE];
  char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

void search_replace(char *string, const char *find, const char *replace)
{
  std::string text = string;

  search_replace(text, find, replace);
  strcpy(string, text.c_str()); /* the caller's buffer holds the result, as it always had to */
}

void search_replace(std::string &s, std::string_view find, std::string_view replace)
{
  size_t pos = 0;

  if (find.empty()) /* would match forever */
    return;
  /* After replacing the first match there is none before it, so the next one, made by the replacement or not, starts
   * no sooner than find.size() - 1 before it: searching on from there finds what searching from the start would. */
  while ((pos = s.find(find, pos)) != std::string::npos) {
    s.replace(pos, find.size(), replace);
    pos = pos >= find.size() - 1 ? pos - (find.size() - 1) : 0;
  }
}

std::string take_string(char *s)
{
  std::string out = s ? s : "";
  free(s);
  return out;
}

char *str_alloc(size_t n)
{
  char *s = static_cast<char *>(calloc(n, 1));
  if (!s) {
    perror("SYSERR: str_alloc");
    abort();
  }
  return s;
}

char *str_realloc(char *s, size_t n)
{
  char *grown = static_cast<char *>(realloc(s, n));
  if (!grown) {
    perror("SYSERR: str_realloc");
    abort();
  }
  return grown;
}

std::string &CAP(std::string &txt)
{
  size_t i = 0;
  while (i + 1 < txt.size() && txt[i] == '@' && IS_COLOR_CHAR(txt[i + 1]))
    i += 2;
  if (i < txt.size())
    txt[i] = toupper((unsigned char)txt[i]);
  return txt;
}

bool str_eq_nocase(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); i++)
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
      return false;
  return true;
}

bool is_abbrev(std::string_view arg1, std::string_view arg2)
{
  return !arg1.empty() && arg1.size() <= arg2.size() && str_eq_nocase(arg1, arg2.substr(0, arg1.size()));
}

int parse_int(std::string_view s)
{
  size_t i = 0;
  while (i < s.size() && isspace((unsigned char)s[i]))
    i++;
  bool neg = false;
  if (i < s.size() && (s[i] == '+' || s[i] == '-'))
    neg = s[i++] == '-';

  /* strtol()'s reading: past the range it stops at LONG_MAX or LONG_MIN; atoi() then keeps the low bits */
  const unsigned long limit = neg ? (unsigned long)LONG_MAX + 1 : (unsigned long)LONG_MAX;
  unsigned long v           = 0;
  for (; i < s.size() && isdigit((unsigned char)s[i]); i++) {
    unsigned long d = s[i] - '0';
    v               = v > (limit - d) / 10 ? limit : v * 10 + d;
  }
  long r = neg ? (v == limit ? LONG_MIN : -(long)v) : (long)v;
  return (int)r;
}

int search_block(std::string_view arg, const char *const *list, int exact)
{
  if (!arg.empty() && arg[0] == '!')
    return -1;

  std::string low(arg);
  for (char &c : low)
    c = tolower((unsigned char)c);

  size_t l = std::max<size_t>(low.size(), 1); /* so "" doesn't match the first string */
  for (int i = 0; *list[i] != '\n'; i++)
    if (exact ? low == list[i] : !strncmp(low.c_str(), list[i], l))
      return i;
  return -1;
}

bool is_number(std::string_view str)
{
  for (char c : str)
    if (!isdigit((unsigned char)c))
      return false;
  return true;
}
