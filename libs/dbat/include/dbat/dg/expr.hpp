#pragma once
/*
 * The text of a script, compiled: templates, which are text with %variables% in it, and expressions, which are what
 * if, while, switch and eval work out.
 *
 * Both are compiled from the text as written, and give what the old engine got by scanning that text each time it
 * ran, quirks and all: the compiler makes every choice the old scan made from the text alone, and leaves to the run
 * only what depended on the values (see the chain in var_ref).
 *
 * A command line's template is compiled with its trigger. So are conditions, which the old engine read before
 * substituting anything. The commands that work out an expression (eval, and the ids of attach, detach and makeuid)
 * read it from their line after it was substituted, so they compile it when they run, and what a variable held can
 * add operators, as it always could.
 */
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace dg {

struct template_piece;

/* Text with %variables% in it */
struct text_template {
  std::vector<template_piece> pieces;
};

/*
 * A %variable%, with its field and the field's (subfield) if there are any: %name.field(subfield)%.
 *
 * A chain, %name.a.b.c%, is looked up a link at a time: name.a, and if that gives something, the result is worked
 * out as an expression into the local variable tmpvr (quirk), and the next link looks at tmpvr.b, and so on. A link
 * that gives nothing leaves the variable and the field as they were (quirk), so whether the next link reads name.a or
 * tmpvr.b is decided by the values. Each link also passes the subfield text read so far, unsubstituted (quirk).
 */
struct var_ref {
  struct link {
    std::string subfield_so_far; /* as written, whatever came before */
    std::string next_field;      /* the field the following lookup reads, if this one gives something */
  };
  std::string name;
  std::string field; /* "" for a plain %name% */
  std::vector<link> chain;
  text_template subfield; /* every subfield of the chain, run together (quirk) */
};

/* Text copied as it is ("%%" is already one '%'), or a variable */
struct template_piece {
  std::variant<std::string, var_ref> what;
};

/* An expression: lhs op rhs, a bracketed expression, or text */
struct expression {
  enum class kind { text, group, op };
  kind what{};
  std::string op;                   /* for op: one of || && == != <= >= < > /= - + / * ! */
  text_template text;               /* for text */
  std::vector<expression> operands; /* a group's one, or an op's two */
};

/* How deeply an expression's operators and brackets may nest */
constexpr int max_expression_depth = 200;

/* Compiles text, as written, into a template */
text_template compile_template(std::string_view text);

/* Compiles text, as written, into an expression. Deeper than max_depth, what is left is empty text: running it hits
   the evaluator's own limit, which logs it. */
expression compile_expression(std::string_view text, int max_depth);

/* Where the quote starting at text[at] ends: its closing quote, or the last character if it has none; a backslash
   escapes the character after it */
size_t matching_quote(std::string_view text, size_t at);

} // namespace dg
