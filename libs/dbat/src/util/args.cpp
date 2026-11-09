#include <cctype>

#include "dbat/args.hpp"

static bool is_space(char c)
{
  return isspace((unsigned char)c);
}

static char lower(char c)
{
  return (char)tolower((unsigned char)c);
}

std::string_view skip_spaces(std::string_view args)
{
  size_t i = 0;

  while (i < args.size() && is_space(args[i]))
    i++;
  return args.substr(i);
}

bool is_fill_word(std::string_view word)
{
  static constexpr std::string_view fill[] = {"in", "into", "from", "with", "the", "on", "at", "to"};

  for (std::string_view f : fill)
    if (word == f)
      return true;
  return false;
}

std::string take_word(std::string_view &args)
{
  std::string word;
  size_t i;

  args = skip_spaces(args);
  for (i = 0; i < args.size() && !is_space(args[i]); i++)
    word += lower(args[i]);
  args.remove_prefix(i);
  return word;
}

std::string take_arg(std::string_view &args)
{
  std::string word;

  do
    word = take_word(args);
  while (is_fill_word(word));
  return word;
}

std::string take_quoted(std::string_view &args)
{
  std::string word;
  size_t i = 0;

  args = skip_spaces(args);
  if (!args.empty() && args[0] == '"') {
    for (i = 1; i < args.size() && args[i] != '"'; i++)
      word += lower(args[i]);
    if (i < args.size()) /* the closing quote, if the word has one */
      i++;
  } else {
    for (; i < args.size() && !is_space(args[i]); i++)
      word += lower(args[i]);
  }
  args.remove_prefix(i);
  return word;
}
