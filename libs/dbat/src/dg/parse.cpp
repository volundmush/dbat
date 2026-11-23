/* Parsing a trigger's lines into its tree, by the strict grammar: see dbat/dg/program.hpp. */
#include "dbat/dg/program.hpp"

#include <algorithm>
#include <cstdio>
#include <strings.h>

#include "dbat/args.hpp"
#include "dbat/format.hpp"
#include "dbat/stringutils.hpp"

namespace dg {

/* Whether s starts with a number, as sscanf()'s %ld reads one: whitespace, a sign, then a digit */
static bool starts_with_number(std::string_view s)
{
  s = skip_spaces(s);
  if (!s.empty() && (s.front() == '+' || s.front() == '-'))
    s.remove_prefix(1);
  return !s.empty() && isdigit(static_cast<unsigned char>(s.front()));
}

bool readable_wait(std::string_view arg)
{
  if (arg.empty())
    return false;
  if (!compare_nocase_n(arg, "until ", 6))
    return arg.starts_with("until") && starts_with_number(arg.substr(5)); /* quirk: "until" only in lower case */
  return starts_with_number(arg);
}

namespace {

class parser {
public:
  explicit parser(program &p) : p_(p) {}

  void run()
  {
    block body = parse_block(nullptr);
    std::stable_sort(p_.errors.begin(), p_.errors.end(),
                     [&](const diagnostic &a, const diagnostic &b) { return a.line < b.line; });
    if (p_.errors.empty())
      p_.body = std::move(body);
  }

private:
  struct open_block {
    keyword kw;
    size_t line;
  };

  program &p_;
  size_t at_ = 0;                /* the line being read */
  std::vector<open_block> open_; /* innermost last */

  const line &here() const { return p_.lines[at_]; }

  void error(size_t at, std::string message) { p_.errors.push_back({at, std::move(message)}); }

  /* " (the open block is the 'while' at line 8)", or "" at the top */
  std::string open_note() const
  {
    if (open_.empty())
      return {};
    return str_format(" (the open block is the '{}' at line {:d})", keyword_name(open_.back().kw),
                      p_.lines[open_.back().line].number);
  }

  /* The expression after an if, elseif, while or switch: read as written, before any substitution */
  static expression condition(const line &l) { return compile_expression(l.tail(), max_expression_depth); }

  bool innermost_is(keyword kw) const { return !open_.empty() && open_.back().kw == kw; }

  bool in_loop_or_switch() const
  {
    return std::any_of(open_.begin(), open_.end(),
                       [](const open_block &b) { return b.kw == keyword::while_ || b.kw == keyword::switch_; });
  }

  /* A keyword either needs something after it, or must stand alone */
  void check_argument(const line &l)
  {
    switch (l.kw) {
      case keyword::if_:
      case keyword::elseif_:
      case keyword::while_:
      case keyword::switch_:
      case keyword::case_:
        if (l.argument().empty())
          error(at_, str_format("'{}' with nothing to test", keyword_name(l.kw)));
        break;
      case keyword::else_:
      case keyword::end_:
      case keyword::done_:
      case keyword::break_:
      case keyword::default_:
        if (!l.argument().empty())
          error(at_, str_format("text after '{}': {}", keyword_name(l.kw), l.argument().substr(0, 40)));
        break;
      default:
        break;
    }
  }

  /* A wait whose time is written out must be one the wait command can read */
  void check_wait(const line &l)
  {
    std::string_view s = l.body();
    if (s.size() < 5 || strncasecmp(s.data(), "wait ", 5) || s.find('%') != std::string_view::npos)
      return;
    if (!readable_wait(skip_spaces(s.substr(5))))
      error(at_, "'wait' needs a time it can read, such as 'wait 5 s' or 'wait until 14:30'");
  }

  /* The nodes up to the line that closes or continues the innermost open block, or to the end of the script.
     switch_of is the switch whose own body this is, which its case and default lines label. */
  block parse_block(switch_node *switch_of)
  {
    block body;

    while (at_ < p_.lines.size()) {
      const line &l = here();
      check_argument(l);

      switch (l.kw) {
        case keyword::comment:
          at_++;
          break;
        case keyword::blank: /* quirk: runs as an empty command, which stands a stunned mob up */
        case keyword::command:
          check_wait(l);
          body.push_back({command_node{at_++, compile_template(l.body())}});
          break;
        case keyword::if_:
          body.push_back({parse_if()});
          break;
        case keyword::while_:
          body.push_back({parse_while()});
          break;
        case keyword::switch_:
          body.push_back({parse_switch()});
          break;
        case keyword::elseif_:
        case keyword::else_:
          if (innermost_is(keyword::if_))
            return body;
          error(at_++, str_format("'{}' with no 'if' open{}", keyword_name(l.kw), open_note()));
          break;
        case keyword::end_:
          if (innermost_is(keyword::if_))
            return body;
          error(at_++, "'end' closes nothing" + open_note());
          break;
        case keyword::done_:
          if (innermost_is(keyword::while_) || innermost_is(keyword::switch_))
            return body;
          error(at_++, "'done' closes nothing" + open_note());
          break;
        case keyword::case_:
        case keyword::default_:
          if (switch_of && innermost_is(keyword::switch_))
            switch_of->labels.push_back({at_, l.kw == keyword::default_, std::string(l.tail()), body.size()});
          else
            error(at_, str_format("'{}' outside a switch's own lines", keyword_name(l.kw)));
          at_++;
          break;
        case keyword::break_:
          if (in_loop_or_switch())
            body.push_back({break_node{at_}});
          else
            error(at_, "'break' outside a while or switch");
          at_++;
          break;
      }
    }
    return body;
  }

  /* Reads the block opened at line `opened` up to its closer; false if the script ends first */
  bool closed(size_t opened)
  {
    if (at_ < p_.lines.size())
      return true;
    error(opened, str_format("'{}' never closed", keyword_name(p_.lines[opened].kw)));
    return false;
  }

  if_node parse_if()
  {
    if_node n;
    size_t opened = at_;

    open_.push_back({keyword::if_, opened});
    for (bool tests = true;; tests = here().kw == keyword::elseif_) {
      if_node::branch b{at_, tests, tests ? condition(here()) : expression{}, {}};
      at_++;
      b.body = parse_block(nullptr);
      n.branches.push_back(std::move(b));
      if (!closed(opened))
        break;
      if (here().kw == keyword::end_) {
        n.end = at_++;
        break;
      }
    }
    open_.pop_back();
    return n;
  }

  while_node parse_while()
  {
    while_node n;

    n.line      = at_;
    n.condition = condition(here());
    open_.push_back({keyword::while_, at_++});
    n.body = parse_block(nullptr);
    if (closed(n.line))
      n.done = at_++;
    open_.pop_back();
    return n;
  }

  switch_node parse_switch()
  {
    switch_node n;

    n.line    = at_;
    n.subject = condition(here());
    open_.push_back({keyword::switch_, at_++});
    n.body = parse_block(&n);
    if (closed(n.line))
      n.done = at_++;
    open_.pop_back();
    return n;
  }
};

} // namespace

void parse(program &p)
{
  parser(p).run();
}

} // namespace dg
