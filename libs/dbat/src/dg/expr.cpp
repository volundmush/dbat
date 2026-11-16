/* Compiling templates and expressions from a script's text: see dbat/dg/expr.hpp. */
#include "dbat/dg/expr.hpp"

#include <array>
#include <cctype>
#include <optional>

namespace dg {

namespace {

bool is_space(char c) { return isspace(static_cast<unsigned char>(c)); }
bool is_alnum(char c) { return isalnum(static_cast<unsigned char>(c)); }

/*
 * Reads a template the way the old substitution scanned it. The scan cut its copy of the text into names and fields
 * by writing a NUL at each '.', '(' and ')' it passed and at the '%' closing each variable; `cut` marks those places,
 * and a name or field is the text from its start to the next of them.
 */
class template_reader {
public:
  explicit template_reader(std::string_view text) : s_(text), cut_(text.size() + 1) {}

  text_template read()
  {
    text_template t;
    std::string literal;
    size_t p = 0;

    while (p < s_.size()) {
      while (p < s_.size() && s_[p] != '%')
        literal += s_[p++];
      if (p >= s_.size())
        break;
      p++; /* past the '%' */
      if (p < s_.size() && s_[p] == '%') {
        literal += s_[p++];
        continue;
      }
      if (p >= s_.size())
        break; /* a lone '%' at the end is dropped */
      if (!literal.empty())
        t.pieces.push_back({std::move(literal)});
      literal.clear();
      t.pieces.push_back({read_variable(p)});
    }
    if (!literal.empty())
      t.pieces.push_back({std::move(literal)});

    /* the names and fields could only be read once every cut is known */
    for (auto &[start, ref, link] : pending_) {
      if (link)
        ref_at(t, ref)->chain[*link].next_field = text_from(start);
      else
        ref_at(t, ref)->field = text_from(start);
    }
    return t;
  }

private:
  struct pending_field {
    size_t start;
    size_t ref;                 /* which variable piece */
    std::optional<size_t> link; /* a link's next field, or the variable's first */
  };

  std::string_view s_;
  std::vector<bool> cut_;
  std::vector<pending_field> pending_;
  int paren_count_ = 0; /* quirk: not reset between variables, so an unclosed '(' carries into the next */
  size_t refs_     = 0;

  std::string text_from(size_t start) const
  {
    size_t end = start;
    while (end < s_.size() && !cut_[end])
      end++;
    return std::string(s_.substr(start, end - start));
  }

  static var_ref *ref_at(text_template &t, size_t n)
  {
    for (template_piece &piece : t.pieces)
      if (var_ref *r = std::get_if<var_ref>(&piece.what); r && !n--)
        return r;
    return nullptr;
  }

  /* The variable starting at s_[p], after its '%'; leaves p past its closing '%' */
  var_ref read_variable(size_t &p)
  {
    var_ref r;
    std::string subfield;
    size_t ref   = refs_++;
    size_t start = p;

    while (p < s_.size() && s_[p] != '%' && s_[p] != '.')
      p++;
    r.name           = std::string(s_.substr(start, p - start));
    size_t field_at  = p;
    if (p < s_.size() && s_[p] == '.') {
      cut_[p++] = true;
      int dots  = 0;
      for (field_at = p; p < s_.size() && (s_[p] != '%' || paren_count_ > 0 || dots); p++) {
        if (dots > 0) { /* the character after a '.' starts the next link, and is not looked at (quirk) */
          pending_.push_back({p, ref, r.chain.size()});
          r.chain.push_back({subfield, {}});
          dots = 0;
        } else if (s_[p] == '(') {
          cut_[p] = true;
          paren_count_++;
        } else if (s_[p] == ')') {
          cut_[p] = true;
          paren_count_--;
        } else if (paren_count_ > 0) {
          subfield += s_[p];
        } else if (s_[p] == '.') {
          cut_[p] = true;
          dots++;
        }
      }
    }
    pending_.push_back({field_at, ref, std::nullopt});
    if (p < s_.size())
      cut_[p] = true;
    p++; /* past the closing '%', or the end of an unclosed variable */
    if (!subfield.empty())
      r.subfield = template_reader(subfield).read();
    return r;
  }
};

constexpr std::array<std::string_view, 14> operators{"||", "&&", "==", "!=", "<=", ">=", "<",
                                                     ">",  "/=", "-",  "+",  "/",  "*",  "!"};

/* Where the bracket at text[at] closes: its ')', or the last character if it never does */
size_t matching_paren(std::string_view text, size_t at)
{
  size_t p  = at + 1;
  int depth = 1;

  for (; p < text.size() && depth; p++) {
    if (text[p] == '(')
      depth++;
    else if (text[p] == ')')
      depth--;
    else if (text[p] == '"')
      p = matching_quote(text, p);
  }
  return p - 1;
}

/* Where the part of an expression starting at text[p] ends: a bracketed or quoted part, a run of letters, digits
   and spaces, or one character */
size_t next_token(std::string_view text, size_t p)
{
  if (text[p] == '(')
    return matching_paren(text, p) + 1;
  if (text[p] == '"')
    return matching_quote(text, p) + 1;
  if (is_alnum(text[p])) {
    for (p++; p < text.size() && (is_alnum(text[p]) || is_space(text[p])); p++);
    return p;
  }
  return p + 1;
}

expression compile(std::string_view text, int depth_left)
{
  expression e;

  while (!text.empty() && is_space(text.front()))
    text.remove_prefix(1);
  if (depth_left <= 0)
    return e; /* never read: the evaluator stops before it */

  /* the operators by priority, outside brackets and quotes; the first found splits the text in two */
  for (std::string_view op : operators)
    for (size_t p = 0; p < text.size(); p = next_token(text, p))
      if (text.substr(p, op.size()) == op) {
        e.what = expression::kind::op;
        e.op   = std::string(op);
        e.operands.push_back(compile(text.substr(0, p), depth_left - 1));
        e.operands.push_back(compile(text.substr(p + op.size()), depth_left - 1));
        return e;
      }

  if (!text.empty() && text.front() == '(') { /* anything after the closing bracket is ignored (quirk) */
    size_t close = matching_paren(text, 0);
    e.what       = expression::kind::group;
    /* quirk: without a ')' the last character is dropped as though it were one */
    e.operands.push_back(compile(close ? text.substr(1, close - 1) : std::string_view{}, depth_left - 1));
    return e;
  }

  e.what = expression::kind::text;
  e.text = compile_template(text);
  return e;
}

} // namespace

size_t matching_quote(std::string_view text, size_t at)
{
  size_t p = at + 1;

  for (; p < text.size() && text[p] != '"'; p++)
    if (text[p] == '\\')
      p++;
  return p < text.size() ? p : text.size() - 1;
}

text_template compile_template(std::string_view text)
{
  return template_reader(text).read();
}

expression compile_expression(std::string_view text, int max_depth)
{
  return compile(text, max_depth);
}

} // namespace dg
