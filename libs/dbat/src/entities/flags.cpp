#include <cstring>
#include <string>
#include <utility>

#include "dbat/flags.hpp"

static bool flag_test(const bitvector_t bitvector[], int flag)
{
  int array_pos = flag / 32;
  int bit_pos   = flag % 32;

  return (bitvector[array_pos] & (1u << bit_pos)) != 0;
}

/* the words in a finished list, the space after the last taken off; an empty list is "None " (with its space) */
static std::string flag_list(std::string out)
{
  if (out.empty())
    return "None ";
  out.pop_back();
  return out;
}

std::string sprintbit(bitvector_t bitvector, const char *const names[])
{
  std::string out;
  int nr = 0;

  while (bitvector) {
    if (bitvector & 1) {
      out += (*names[nr] != '\n') ? names[nr] : "UNDEFINED";
      out += ' ';
    }
    if (*names[nr] != '\n')
      nr++; /* past the end of names, each bit is UNDEFINED */
    bitvector >>= 1;
  }
  return flag_list(std::move(out));
}

std::string sprinttype(int type, const char *const names[])
{
  int nr = 0;

  while (type > 0 && *names[nr] != '\n') {
    type--;
    nr++;
  }
  return (*names[nr] != '\n') ? names[nr] : "UNDEFINED";
}

std::string sprintbitarray(bitvector_t bitvector[], const char *const names[], int maxar)
{
  std::string out;

  for (int flag = 0; flag < maxar * 32 && *names[flag] != '\n'; flag++)
    if (flag_test(bitvector, flag) && *names[flag] != '\0') {
      out += names[flag];
      out += ' ';
    }
  return flag_list(std::move(out));
}

int get_flag_by_name(const char *const flag_list[], char *flag_name)
{
  int i = 0;
  for (; flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++)
    if (!strcmp(flag_list[i], flag_name))
      return (i);
  return (NOFLAG);
}
