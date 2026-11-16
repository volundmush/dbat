#pragma once
/*
 * A trigger's script, read once into its lines.
 *
 * The program is made when the trigger is loaded or saved in trigedit, and never changes after: the prototype and
 * every instance of the trigger share it, and whatever a run needs to remember lives on the instance. Editing the
 * trigger makes a new program.
 */
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace dg {

/* What a line is, read by the strict grammar: a keyword is the line's whole first word, in any case, ending at a
   space or '('. */
enum class keyword {
  command, /* anything else: a DG command, or the owner's own */
  comment, /* starts with '*' */
  blank,   /* only spaces */
  if_,
  elseif_,
  else_,
  end_,
  while_,
  done_,
  switch_,
  case_,
  default_,
  break_,
};

/* The keyword's name, as a script writes it */
std::string_view keyword_name(keyword kw);

struct line {
  int number{};       /* in the script as written, from 1, blank lines counted */
  std::string text{}; /* as written, without the line break */
  size_t indent{};    /* where the statement starts, past the leading spaces */
  keyword kw{};
  size_t arg{};       /* where the keyword's argument starts, past the spaces after it; text.size() if none */

  /* The line without its indentation */
  std::string_view body() const { return std::string_view(text).substr(indent); }
  /* For a keyword, what follows it, without the spaces either side */
  std::string_view argument() const;
};

struct program {
  std::vector<line> lines{}; /* never empty */

  /* The script as trigedit shows it: each line followed by "\r\n" */
  std::string script() const;
};

/* Reads a script into its lines. The lines are split at '\n' and '\r', and empty ones are dropped, as the old loader
   did; a script with no lines at all becomes the one line if_empty. */
std::shared_ptr<const program> lex(std::string_view script, std::string_view if_empty);

} // namespace dg
