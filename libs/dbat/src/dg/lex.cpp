/* Reading a trigger's script into its lines: see dbat/dg/program.hpp. */
#include "dbat/dg/program.hpp"

#include <array>
#include <cctype>
#include <utility>

namespace dg {

namespace {

constexpr std::array<std::pair<std::string_view, keyword>, 10> keywords{{
    {"if", keyword::if_},
    {"elseif", keyword::elseif_},
    {"else", keyword::else_},
    {"end", keyword::end_},
    {"while", keyword::while_},
    {"done", keyword::done_},
    {"switch", keyword::switch_},
    {"case", keyword::case_},
    {"default", keyword::default_},
    {"break", keyword::break_},
}};

bool is_space(char c) { return isspace(static_cast<unsigned char>(c)); }

bool same_word(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); i++)
    if (tolower(static_cast<unsigned char>(a[i])) != tolower(static_cast<unsigned char>(b[i])))
      return false;
  return true;
}

/* Sorts a line into its kind, and finds a keyword's argument */
void classify(line &l)
{
  std::string_view s = l.text;

  l.indent = 0;
  while (l.indent < s.size() && is_space(s[l.indent]))
    l.indent++;
  l.arg = s.size();
  if (l.indent == s.size()) {
    l.kw = keyword::blank;
    return;
  }
  if (s[l.indent] == '*') {
    l.kw = keyword::comment;
    return;
  }

  while (is_space(s.back())) /* there is something else, so this stops */
    s.remove_suffix(1);
  size_t end = l.indent;
  while (end < s.size() && (isalpha(static_cast<unsigned char>(s[end])) || s[end] == '_'))
    end++;
  l.kw = keyword::command;
  if (end < s.size() && s[end] != ' ' && s[end] != '\t' && s[end] != '(')
    return; /* more of the same word, as in 'ending' or 'if%x%' */
  for (const auto &[name, kw] : keywords)
    if (same_word(s.substr(l.indent, end - l.indent), name)) {
      l.kw = kw;
      while (end < s.size() && is_space(s[end]))
        end++;
      l.arg = end < s.size() ? end : l.text.size();
      return;
    }
}

} // namespace

std::string_view keyword_name(keyword kw)
{
  for (const auto &[name, k] : keywords)
    if (k == kw)
      return name;
  return kw == keyword::comment ? "comment" : kw == keyword::blank ? "blank line" : "command";
}

std::string_view line::argument() const
{
  std::string_view s = std::string_view(text).substr(arg);
  while (!s.empty() && isspace(static_cast<unsigned char>(s.back())))
    s.remove_suffix(1);
  return s;
}

std::string program::script() const
{
  std::string text;
  for (const line &l : lines) {
    text += l.text;
    text += "\r\n";
  }
  return text;
}

std::shared_ptr<const program> lex(std::string_view script, std::string_view if_empty)
{
  auto made = std::make_shared<program>();
  int number = 1;

  for (size_t at = 0; at < script.size();) {
    size_t stop = script.find_first_of("\r\n", at);
    if (stop == std::string_view::npos)
      stop = script.size();
    if (stop > at) {
      line l;
      l.number = number;
      l.text   = std::string(script.substr(at, stop - at));
      classify(l);
      made->lines.push_back(std::move(l));
    }
    if (stop < script.size() && script[stop] == '\n')
      number++;
    at = stop + 1;
  }

  if (made->lines.empty()) {
    line l;
    l.number = 1;
    l.text   = std::string(if_empty);
    classify(l);
    made->lines.push_back(std::move(l));
  }
  parse(*made);
  return made;
}

} // namespace dg
