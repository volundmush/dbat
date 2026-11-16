/*
 * DG Scripts: evaluating script expressions and conditions.
 * Split out of dg_scripts.c.
 */
#include <cstdlib>

#include "dbat/utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

/* Local functions not used elsewhere */
static bool is_num(char *arg);
static std::string eval_op(const char *op, std::string lhs_text, std::string rhs_text);
static char *matching_paren(char *p);
constexpr int MAX_EVAL_DEPTH = 200;
static bool eval_lhs_op_rhs(GameWorld *g, const char *expr, std::string &result, owner_ptr go, struct script_data *sc,
                            trig_data *trig);

/* returns 1 if string is all digits, else 0 */
/* bugfixed - would have returned true on num="------" */
static bool is_num(char *arg)
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
static std::string eval_op(const char *op, std::string lhs_text, std::string rhs_text)
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
  if (!strcmp("||", op)) {
    if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
      result = "0";
    else
      result = "1";
  }

  else if (!strcmp("&&", op)) {
    if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
      result = "0";
    else
      result = "1";
  }

  else if (!strcmp("==", op)) {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) == atoll(rhs));
    else
      result = str_format("{:d}", !strcasecmp(lhs, rhs));
  }

  else if (!strcmp("!=", op)) {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) != atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs));
  }

  else if (!strcmp("<=", op)) {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) <= atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) <= 0);
  }

  else if (!strcmp(">=", op)) {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) >= atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) >= 0);
  }

  else if (!strcmp("<", op)) {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) < atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) < 0);
  }

  else if (!strcmp(">", op)) {
    if (is_num(lhs) && is_num(rhs))
      result = str_format("{:d}", atoll(lhs) > atoll(rhs));
    else
      result = str_format("{:d}", strcasecmp(lhs, rhs) > 0);
  }

  else if (!strcmp("/=", op))
    result = str_str(lhs, rhs) ? "1" : "0";

  else if (!strcmp("*", op))
    result = str_format("{:d}", wrap_mul(atoll(lhs), atoll(rhs)));

  else if (!strcmp("/", op))
    result = str_format("{:d}", wrap_div(atoll(lhs), atoll(rhs)));

  else if (!strcmp("+", op))
    result = str_format("{:d}", wrap_add(atoll(lhs), atoll(rhs)));

  else if (!strcmp("-", op))
    result = str_format("{:d}", wrap_sub(atoll(lhs), atoll(rhs)));

  else if (!strcmp("!", op)) {
    if (is_num(rhs))
      result = str_format("{:d}", !atoll(rhs));
    else
      result = str_format("{:d}", !*rhs);
  }

  else
    result.clear();
  return result;
}

/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
char *matching_quote(char *p)
{
  for (p++; *p && (*p != '"'); p++) {
    if (*p == '\\')
      p++;
  }

  if (!*p)
    p--;

  return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
static char *matching_paren(char *p)
{
  int i;

  for (p++, i = 1; *p && i; p++) {
    if (*p == '(')
      i++;
    else if (*p == ')')
      i--;
    else if (*p == '"')
      p = matching_quote(p);
  }

  return --p;
}

/* where the part of an expression starting at p ends */
static char *next_token(char *p)
{
  if (*p == '(')
    return matching_paren(p) + 1;
  if (*p == '"')
    return matching_quote(p) + 1;
  if (isalnum(*p)) {
    for (p++; *p && (isalnum(*p) || isspace(*p)); p++);
    return p;
  }
  return p + 1;
}

/* evaluates line */
std::string eval_expr(GameWorld *g, const char *line, owner_ptr go, struct script_data *sc, trig_data *trig)
{
  std::string result;

  while (*line && isspace(*line))
    line++;

  /* each operator or bracket goes one deeper; real scripts reach about 40, and the stack is finite */
  if (g->dg_eval_depth >= MAX_EVAL_DEPTH) {
    script_log(g, "Trigger VNum {:d} has an expression nested too deeply.", trig ? trig_get_vnum(g, trig) : NOTHING);
    return result;
  }
  g->dg_eval_depth++;

  if (eval_lhs_op_rhs(g, line, result, go, sc, trig))
    ;

  else if (*line == '(') {
    std::string expr = line;
    char *p          = matching_paren(expr.data());
    *p               = '\0';
    result           = eval_expr(g, expr.c_str() + 1, go, sc, trig);
  }

  else
    result = var_subst(g, go, sc, trig, line);

  g->dg_eval_depth--;
  return result;
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
static bool eval_lhs_op_rhs(GameWorld *g, const char *expr, std::string &result, owner_ptr go, struct script_data *sc,
                            trig_data *trig)
{
  char *p;
  int i;

  /*
   * valid operands, in order of priority
   * each must also be defined in eval_op()
   */
  static const char *ops[] = {
      "||",
      "&&",
      "==",
      "!=",
      "<=",
      ">=",
      "<",
      ">",
      "/=",
      "-",
      "+",
      "/",
      "*",
      "!",
      "\n"};

  std::string line = expr; /* cut in two at the operator */

  /* the places in line where an op could possibly occur */
  for (i = 0; *ops[i] != '\n'; i++)
    for (p = line.data(); *p; p = next_token(p))
      if (!strncasecmp(ops[i], p, strlen(ops[i]))) {
        *p = '\0';
        p += strlen(ops[i]);

        std::string lhr = eval_expr(g, line.c_str(), go, sc, trig);
        std::string rhr = eval_expr(g, p, go, sc, trig);
        result          = eval_op(ops[i], lhr, rhr);
        return true;
      }

  return false;
}

/* returns 1 if cond is true, else 0 */
bool process_if(GameWorld *g, const char *cond, owner_ptr go, struct script_data *sc,
                trig_data *trig)
{
  std::string result = eval_expr(g, cond, go, sc, trig);
  const char *p      = result.c_str();

  while (isspace(*p))
    p++;

  if (!*p || *p == '0')
    return false;
  else
    return true;
}

/* quirk: the value is compared as written, without substituting its variables */
bool case_matches(const std::string &subject, const char *value)
{
  std::string match = eval_op("==", subject, value);
  return !match.empty() && match[0] != '0';
}
