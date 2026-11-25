#include <algorithm>
#include <ctime>
#include <format>
#include <numeric>
#include <vector>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "dbat/stringutils.hpp"
#include "dbat/args.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/utils.hpp"
#include "dbat/log.hpp"
#include "dbat/consts/maximums.hpp"

int levenshtein_distance(std::string_view s1, std::string_view s2)
{
  /* the edit distances from s1's first i letters to each start of s2, a row at a time */
  std::vector<int> above(s2.size() + 1), row(s2.size() + 1);

  std::ranges::iota(above, 0);
  for (size_t i = 1; i <= s1.size(); i++) {
    row[0] = static_cast<int>(i);
    for (size_t j = 1; j <= s2.size(); j++)
      row[j] = std::min({above[j] + 1, row[j - 1] + 1, above[j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1)});
    std::swap(above, row);
  }
  return above[s2.size()];
}

int count_color_chars(std::string_view text)
{
  int num = 0;

  for (size_t i = 0; i < text.size();) {
    if (text[i] != '@') {
      i++;
      continue;
    }
    char code = i + 1 < text.size() ? text[i + 1] : '\0';
    num += code == '@' ? 1 : code == '[' ? 4 : 2;
    i += 2;
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

std::string add_commas(int64_t num)
{
  std::string digits = str_format("{:d}", num);
  size_t first       = num < 0; /* no comma straight after the sign */
  std::string out;

  for (size_t i = 0; i < digits.size(); i++) {
    if (i > first && (digits.size() - i) % 3 == 0)
      out += ',';
    out += digits[i];
  }
  return out;
}

/* the letter of a colour code ("@r"), which CAP() passes over */
static bool is_colour_letter(char c)
{
  static constexpr std::string_view letters = "nbBcCgGmMrRyYwWkK0234567oeul";
  return c != '\0' && letters.contains(c);
}

/* Removes the line endings from the end of txt. */
void prune_crlf(std::string &txt)
{
  while (!txt.empty() && (txt.back() == '\n' || txt.back() == '\r'))
    txt.pop_back();
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

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
std::string_view fname(std::string_view namelist)
{
  auto letters = std::ranges::find_if_not(namelist, [](unsigned char c) { return std::isalpha(c); });
  return namelist.substr(0, static_cast<size_t>(letters - namelist.begin()));
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

std::string to_lower(std::string_view s)
{
  std::string out(s);
  std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

std::string upper_first(std::string_view s)
{
  std::string out(s);
  if (!out.empty())
    out.front() = static_cast<char>(toupper(static_cast<unsigned char>(out.front())));
  return out;
}

std::string &CAP(std::string &txt)
{
  size_t i = 0;
  while (i + 1 < txt.size() && txt[i] == '@' && is_colour_letter(txt[i + 1]))
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

int compare_nocase_n(std::string_view a, std::string_view b, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    int ca = i < a.size() ? std::tolower(static_cast<unsigned char>(a[i])) : 0;
    int cb = i < b.size() ? std::tolower(static_cast<unsigned char>(b[i])) : 0;
    if (ca != cb)
      return ca - cb;
    if (ca == 0)
      return 0;
  }
  return 0;
}

bool str_less_nocase(std::string_view a, std::string_view b)
{
  auto lower = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };
  return std::ranges::lexicographical_compare(a, b, {}, lower, lower);
}

bool is_abbrev(std::string_view arg1, std::string_view arg2)
{
  return !arg1.empty() && arg1.size() <= arg2.size() && str_eq_nocase(arg1, arg2.substr(0, arg1.size()));
}

int parse_int(std::string_view s) { return static_cast<int>(io::c_atol(s)); }

int search_block(std::string_view arg, std::span<const std::string_view> list, bool exact)
{
  if (!arg.empty() && arg[0] == '!')
    return -1;

  std::string low = to_lower(until_nul(arg)); /* as strncmp() read it */
  for (size_t i = 0; i < list.size(); i++)
    if (exact ? low == list[i] : low.empty() ? list[i].empty() : list[i].starts_with(low))
      return static_cast<int>(i);
  return -1;
}

bool is_number(std::string_view str)
{
  for (char c : str)
    if (!isdigit((unsigned char)c))
      return false;
  return true;
}

std::string asctime_text(time_t when)
{
  static constexpr std::string_view DAYS[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static constexpr std::string_view MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const std::tm *t                           = std::localtime(&when);
  return std::format("{} {}{:3d} {:02d}:{:02d}:{:02d} {:d}", DAYS[t->tm_wday], MONTHS[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min,
                     t->tm_sec, 1900 + t->tm_year);
}
