#include <algorithm>
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

std::string sprintbit(bitvector_t bitvector, std::span<const std::string_view> names)
{
  std::string out;
  size_t nr = 0;

  for (; bitvector; bitvector >>= 1, nr++)
    if (bitvector & 1) {
      out += nr < names.size() ? names[nr] : "UNDEFINED"; /* past the end of names, each bit is UNDEFINED */
      out += ' ';
    }
  return flag_list(std::move(out));
}

std::string sprinttype(int type, std::span<const std::string_view> names)
{
  size_t nr = std::max(type, 0); /* quirk: a negative type is the first name */
  return std::string(nr < names.size() ? names[nr] : "UNDEFINED");
}

std::string sprintbitarray(bitvector_t bitvector[], std::span<const std::string_view> names, int maxar)
{
  std::string out;

  for (size_t flag = 0; flag < static_cast<size_t>(maxar) * 32 && flag < names.size(); flag++)
    if (flag_test(bitvector, static_cast<int>(flag)) && !names[flag].empty()) {
      out += names[flag];
      out += ' ';
    }
  return flag_list(std::move(out));
}

int get_flag_by_name(std::span<const std::string_view> flag_list, std::string_view flag_name)
{
  for (size_t i = 0; i < flag_list.size() && !flag_list[i].empty(); i++)
    if (flag_list[i] == flag_name)
      return static_cast<int>(i);
  return NOFLAG;
}
