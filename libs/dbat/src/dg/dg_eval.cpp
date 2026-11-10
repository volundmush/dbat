/*
 * DG Scripts: evaluating script expressions and conditions, and finding the end of if/while/switch blocks.
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
static bool eval_lhs_op_rhs(GameWorld *g, const char *expr, std::string &result, void *go, struct script_data *sc,
                            trig_data *trig, int type);

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
  char result[32]; /* a number, or 0 or 1 */
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
      strcpy(result, "0");
    else
      strcpy(result, "1");
  }

  else if (!strcmp("&&", op)) {
    if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
      strcpy(result, "0");
    else
      strcpy(result, "1");
  }

  else if (!strcmp("==", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoll(lhs) == atoll(rhs));
    else
      sprintf(result, "%d", !strcasecmp(lhs, rhs));
  }

  else if (!strcmp("!=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoll(lhs) != atoll(rhs));
    else
      sprintf(result, "%d", strcasecmp(lhs, rhs));
  }

  else if (!strcmp("<=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoll(lhs) <= atoll(rhs));
    else
      sprintf(result, "%d", strcasecmp(lhs, rhs) <= 0);
  }

  else if (!strcmp(">=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoll(lhs) >= atoll(rhs));
    else
      sprintf(result, "%d", strcasecmp(lhs, rhs) >= 0);
  }

  else if (!strcmp("<", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoll(lhs) < atoll(rhs));
    else
      sprintf(result, "%d", strcasecmp(lhs, rhs) < 0);
  }

  else if (!strcmp(">", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoll(lhs) > atoll(rhs));
    else
      sprintf(result, "%d", strcasecmp(lhs, rhs) > 0);
  }

  else if (!strcmp("/=", op))
    sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');

  else if (!strcmp("*", op))
    sprintf(result, "%lld", wrap_mul(atoll(lhs), atoll(rhs)));

  else if (!strcmp("/", op))
    sprintf(result, "%lld", wrap_div(atoll(lhs), atoll(rhs)));

  else if (!strcmp("+", op))
    sprintf(result, "%lld", wrap_add(atoll(lhs), atoll(rhs)));

  else if (!strcmp("-", op))
    sprintf(result, "%lld", wrap_sub(atoll(lhs), atoll(rhs)));

  else if (!strcmp("!", op)) {
    if (is_num(rhs))
      sprintf(result, "%d", !atoll(rhs));
    else
      sprintf(result, "%d", !*rhs);
  }

  else
    *result = '\0';
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
std::string eval_expr(GameWorld *g, const char *line, void *go, struct script_data *sc, trig_data *trig, int type)
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

  if (eval_lhs_op_rhs(g, line, result, go, sc, trig, type))
    ;

  else if (*line == '(') {
    std::string expr = line;
    char *p          = matching_paren(expr.data());
    *p               = '\0';
    result           = eval_expr(g, expr.c_str() + 1, go, sc, trig, type);
  }

  else
    result = var_subst(g, go, sc, trig, type, line);

  g->dg_eval_depth--;
  return result;
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
static bool eval_lhs_op_rhs(GameWorld *g, const char *expr, std::string &result, void *go, struct script_data *sc,
                            trig_data *trig, int type)
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

        std::string lhr = eval_expr(g, line.c_str(), go, sc, trig, type);
        std::string rhr = eval_expr(g, p, go, sc, trig, type);
        result          = eval_op(ops[i], lhr, rhr);
        return true;
      }

  return false;
}

/* returns 1 if cond is true, else 0 */
bool process_if(GameWorld *g, const char *cond, void *go, struct script_data *sc,
                trig_data *trig, int type)
{
  std::string result = eval_expr(g, cond, go, sc, trig, type);
  const char *p      = result.c_str();

  while (isspace(*p))
    p++;

  if (!*p || *p == '0')
    return false;
  else
    return true;
}

/*
 * scans for end of if-block.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
struct cmdlist_element *find_end(GameWorld *g, trig_data *trig, struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  const char *p;

  if (!(cl->next)) { /* rryan: if this is the last line, theres no end. */
    script_log(g, "Trigger VNum {:d} has 'if' without 'end'. (error 1)", trig_get_vnum(g, trig));
    return cl;
  }

  for (c = cl->next; c; c = c->next) {
    for (p = c->cmd.c_str(); *p && isspace(*p); p++);

    if (!strncasecmp("if ", p, 3))
      c = find_end(g, trig, c);
    else if (!strncasecmp("end", p, 3))
      return c;

    /* thanks to Russell Ryan for this fix */
    if (!c->next) { /* rryan: this is the last line, we didn't find an end. */
      script_log(g, "Trigger VNum {:d} has 'if' without 'end'. (error 2)", trig_get_vnum(g, trig));
      return c;
    }
  }

  /* rryan: we didn't find an end. */
  script_log(g, "Trigger VNum {:d} has 'if' without 'end'. (error 3)", trig_get_vnum(g, trig));
  return c;
}

/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */
struct cmdlist_element *find_else_end(GameWorld *g, trig_data *trig,
                                      struct cmdlist_element *cl, void *go,
                                      struct script_data *sc, int type)
{
  struct cmdlist_element *c;
  const char *p;

  if (!(cl->next))
    return cl;

  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd.c_str(); *p && isspace(*p); p++); /* skip spaces */

    if (!strncasecmp("if ", p, 3))
      c = find_end(g, trig, c);

    else if (!strncasecmp("elseif ", p, 7)) {
      if (process_if(g, p + 7, go, sc, trig, type)) {
        trig->depth++;
        return c;
      }
    }

    else if (!strncasecmp("else", p, 4)) {
      trig->depth++;
      return c;
    }

    else if (!strncasecmp("end", p, 3))
      return c;

    /* thanks to Russell Ryan for this fix */
    if (!c->next) { /* rryan: this is the last line, return. */
      script_log(g, "Trigger VNum {:d} has 'if' without 'end'. (error 4)", trig_get_vnum(g, trig));
      return c;
    }
  }

  /* rryan: if we got here, it's the last line, if its not an end, script_log it. */
  for (p = c->cmd.c_str(); *p && isspace(*p); p++); /* skip spaces */
  if (strncasecmp("end", p, 3))
    script_log(g, "Trigger VNum {:d} has 'if' without 'end'. (error 5)", trig_get_vnum(g, trig));
  return c;
}
/*
* scans for a case/default instance
* returns the line containg the correct case instance, or the last
* line of the trigger if not found.
*/
struct cmdlist_element *
find_case(GameWorld *g, struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, const char *cond)
{
  std::string result = eval_expr(g, cond, go, sc, trig, type);
  struct cmdlist_element *c;
  const char *p;

  if (!(cl->next))
    return cl;

  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd.c_str(); *p && isspace(*p); p++);

    if (!strncasecmp("while ", p, 6) || !strncasecmp("switch", p, 6)) {
      c = find_done(c);
      if (!c->next) /* it never ended, and neither does this */
        return c;
    } else if (!strncasecmp("case ", p, 5)) {
      std::string match = eval_op("==", result, p + 5);
      if (!match.empty() && match[0] != '0')
        return c;
    } else if (!strncasecmp("default", p, 7))
      return c;
    else if (!strncasecmp("done", p, 3))
      return c;
  }
  return c;
}

/*
* scans for end of while/switch-blocks.
* returns the line containg 'end', or the last
* line of the trigger if not found.
* Malformed scripts may cause NULL to be returned.
*/
struct cmdlist_element *find_done(struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  const char *p;

  if (!cl || !(cl->next))
    return cl;

  for (c = cl->next; c && c->next; c = c->next) {
    for (p = c->cmd.c_str(); *p && isspace(*p); p++);

    if (!strncasecmp("while ", p, 6) || !strncasecmp("switch ", p, 7)) {
      c = find_done(c);
      if (!c->next) /* it never ended, and neither does this */
        return c;
    } else if (!strncasecmp("done", p, 3))
      return c;
  }

  return c;
}
