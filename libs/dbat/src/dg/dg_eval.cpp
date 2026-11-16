/*
 * DG Scripts: working out expressions, compiled by dg/expr.cpp, and conditions.
 */
#include <cstdlib>

#include "dbat/utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

constexpr int MAX_EVAL_DEPTH = dg::max_expression_depth;

/* returns 1 if string is all digits, else 0 */
/* bugfixed - would have returned true on num="------" */
static bool is_num(const char *arg)
{
  if (*arg == '\0')
    return false;

  if (*arg == '+' || *arg == '-')
    arg++;

  for (; *arg != '\0'; arg++) {
    if (!isdigit(*arg))
      return false;
  }

  return true;
}

/*
 * Script arithmetic wraps around when it overflows, which is what the game's builds always did in practice, but
 * done in unsigned arithmetic it is defined behaviour. Dividing the smallest number by -1 wraps the same way,
 * where the processor would trap and take the game down.
 */
static long long wrap_add(long long a, long long b)
{
  return (long long)((unsigned long long)a + (unsigned long long)b);
}

static long long wrap_sub(long long a, long long b)
{
  return (long long)((unsigned long long)a - (unsigned long long)b);
}

static long long wrap_mul(long long a, long long b)
{
  return (long long)((unsigned long long)a * (unsigned long long)b);
}

static long long wrap_div(long long a, long long b)
{
  if (!b)
    return 0;
  if (b == -1)
    return wrap_sub(0, a);
  return a / b;
}

/* evaluates 'lhs op rhs' */
static std::string eval_op(std::string_view op, std::string lhs_text, std::string rhs_text)
{
  char *lhs = lhs_text.data(), *rhs = rhs_text.data();
  std::string result; /* a number, or 0 or 1 */
  unsigned char *p;

  /* strip off extra spaces at begin and end */
  while (*lhs && isspace(*lhs))
    lhs++;
  while (*rhs && isspace(*rhs))
    rhs++;

  /* strip trailing spaces without stepping before the start of an empty operand */
  for (p = (unsigned char *)lhs + strlen(lhs); (char *)p > lhs && isspace(p[-1]);)
    *--p = '\0';
  for (p = (unsigned char *)rhs + strlen(rhs); (char *)p > rhs && isspace(p[-1]);)
    *--p = '\0';

  /* find the op, and figure out the value */
  if (op == "||") {
    if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
      result = "0";
    else
      result = "1";
  }

  else if (op == "&&") {
    if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
      result = "0";
    else
      result = "1";
  }

  else if (op == "==") {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) == atoll(rhs));
    else
      result = str_format("{:d}", !strcasecmp(lhs, rhs));
  }

  else if (op == "!=") {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) != atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs));
  }

  else if (op == "<=") {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) <= atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) <= 0);
  }

  else if (op == ">=") {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) >= atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) >= 0);
  }

  else if (op == "<") {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) < atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) < 0);
  }

  else if (op == ">") {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) > atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) > 0);
  }

  else if (op == "/=")
    result = str_str(lhs, rhs) ? "1" : "0";

  else if (op == "*")
    result = str_format("{:d}", wrap_mul(atoll(lhs), atoll(rhs)));

  else if (op == "/")
    result = str_format("{:d}", wrap_div(atoll(lhs), atoll(rhs)));

  else if (op == "+")
    result = str_format("{:d}", wrap_add(atoll(lhs), atoll(rhs)));

  else if (op == "-")
    result = str_format("{:d}", wrap_sub(atoll(lhs), atoll(rhs)));

  else if (op == "!") {
    if (is_num(rhs))
      result = str_format("{:d}", !atoll(rhs));
    else
      result = str_format("{:d}", !*rhs);
  }

  else
    result.clear();
  return result;
}

/* Works out e: an operator's two sides in order, then the operator */
std::string evaluate(GameWorld *g, const dg::expression &e, owner_ptr go, struct script_data *sc, trig_data *trig)
{
  std::string result;

  /* each operator or bracket goes one deeper; real scripts reach about 40, and the stack is finite */
  if (g->dg_eval_depth >= MAX_EVAL_DEPTH) {
    script_log(g, "Trigger VNum {:d} has an expression nested too deeply.", trig ? trig_get_vnum(g, trig) : NOTHING);
    return result;
  }
  g->dg_eval_depth++;

  switch (e.what) {
    case dg::expression::kind::op: {
      std::string lhs = evaluate(g, e.operands[0], go, sc, trig);
      std::string rhs = evaluate(g, e.operands[1], go, sc, trig);
      result          = eval_op(e.op, lhs, rhs);
      break;
    }
    case dg::expression::kind::group:
      result = evaluate(g, e.operands[0], go, sc, trig);
      break;
    case dg::expression::kind::text:
      result = render(g, e.text, go, sc, trig);
      break;
  }

  g->dg_eval_depth--;
  return result;
}

std::string eval_expr(GameWorld *g, std::string_view text, owner_ptr go, struct script_data *sc, trig_data *trig)
{
  return evaluate(g, dg::compile_expression(text, MAX_EVAL_DEPTH), go, sc, trig);
}

/* Whether a condition's value counts as true: anything but nothing or a leading 0 */
bool is_true(std::string_view value)
{
  while (!value.empty() && isspace(static_cast<unsigned char>(value.front())))
    value.remove_prefix(1);
  return !value.empty() && value.front() != '0';
}

/* quirk: the value is compared as written, without substituting its variables */
bool case_matches(const std::string &subject, std::string_view value)
{
  return is_true(eval_op("==", subject, std::string(value)));
}
