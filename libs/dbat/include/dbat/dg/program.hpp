#pragma once
/*
 * A trigger's script, read once into its lines and parsed into a tree of blocks.
 *
 * The program is made when the trigger is loaded or saved in trigedit, and never changes after: the prototype and
 * every instance of the trigger share it, and whatever a run needs to remember lives on the instance (see
 * dbat/dg/run.hpp). Editing the trigger makes a new program.
 *
 * The grammar is strict: each block keyword must close or continue the block it belongs to, every block is closed,
 * and else, end, done, break and default stand alone on their lines. A script that breaks these rules is kept, so
 * that trigedit can show and save it, but it has errors and no tree, and never runs.
 */
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "dbat/dg/expr.hpp"

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
  /* For a keyword, what follows it, to the end of the line, as the expression code reads it */
  std::string_view tail() const { return std::string_view(text).substr(arg); }
};

/* The tree. Each node names its line by its index in program::lines. */
struct node;
using block = std::vector<node>;

/* A line that runs: a DG command or the owner's own, after substitution */
struct command_node {
  size_t line{};
  text_template command; /* the line without its indentation */
};

/* if, any elseifs and elses, end. A branch without a condition is an else; the first one ends the choosing, so
   anything after it is never reached, as before. */
struct if_node {
  struct branch {
    size_t line{};
    bool tests{}; /* false for else */
    expression condition;
    block body;
  };
  std::vector<branch> branches{};
  size_t end{}; /* the 'end' line */
};

struct while_node {
  size_t line{};
  expression condition;
  block body;
  size_t done{}; /* the 'done' line, where the condition is tested again */
};

/* The labels are the case and default lines directly in the body, in order; they are not nodes, so a run falls
   through them. */
struct switch_node {
  struct label {
    size_t line{};
    bool is_default{};
    std::string value; /* a case's, as written: it is never substituted (quirk) */
    size_t at{};       /* the body's node after the label */
  };
  size_t line{};
  expression subject;
  block body;
  std::vector<label> labels{};
  size_t done{};
};

/* Leaves the innermost while or switch */
struct break_node {
  size_t line{};
};

struct node {
  std::variant<command_node, if_node, while_node, switch_node, break_node> what;
};

/* Why a script does not compile */
struct diagnostic {
  size_t line{}; /* in program::lines */
  std::string message{};
};

struct program {
  std::vector<line> lines{}; /* never empty */
  block body;              /* empty if there are errors */
  std::vector<diagnostic> errors{}; /* by line */

  bool runs() const { return errors.empty(); }

  /* The script as trigedit shows it: each line followed by "\r\n" */
  std::string script() const;
};

/* Reads a script into its lines. The lines are split at '\n' and '\r', and empty ones are dropped, as the old loader
   did; a script with no lines at all becomes the one line if_empty. Then parses it (see parse.cpp). */
std::shared_ptr<const program> lex(std::string_view script, std::string_view if_empty);

/* Parses p's lines into its body, or its errors */
void parse(program &p);

/* Whether the wait command can read arg, the text after 'wait' */
bool readable_wait(const char *arg);

} // namespace dg
