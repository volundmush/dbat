/*
 * DG Scripts: evaluating script expressions and conditions, and finding the end of if/while/switch blocks.
 * Split out of dg_scripts.c.
 */
#include "dbat/utils.h"
#include "dg_internal.h"

/* Local functions not used elsewhere */
static bool is_num(char *arg);
static void eval_op(const char *op, char *lhs, char *rhs, char *result);
static char *matching_paren(char *p);
static int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
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

/* evaluates 'lhs op rhs', and copies to result */
static void eval_op(const char *op, char *lhs, char *rhs, char *result)
{
  unsigned char *p;
  long long n;

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
    sprintf(result, "%lld", atoll(lhs) * atoll(rhs));

  else if (!strcmp("/", op))
    sprintf(result, "%lld", (n = atoll(rhs)) ? (atoll(lhs) / n) : 0);

  else if (!strcmp("+", op))
    sprintf(result, "%lld", atoll(lhs) + atoll(rhs));

  else if (!strcmp("-", op))
    sprintf(result, "%lld", atoll(lhs) - atoll(rhs));

  else if (!strcmp("!", op)) {
    if (is_num(rhs))
      sprintf(result, "%d", !atoll(rhs));
    else
      sprintf(result, "%d", !*rhs);
  }
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

/* evaluates line, and returns answer in result */
void eval_expr(char *line, char *result, void *go, struct script_data *sc,
               trig_data *trig, int type)
{
  char expr[MAX_INPUT_LENGTH], *p;

  while (*line && isspace(*line))
    line++;

  if (eval_lhs_op_rhs(line, result, go, sc, trig, type))
    ;

  else if (*line == '(') {
    p  = strcpy(expr, line);
    p  = matching_paren(expr);
    *p = '\0';
    eval_expr(expr + 1, result, go, sc, trig, type);
  }

  else
    var_subst(go, sc, trig, type, line, result);
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
static int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
                           trig_data *trig, int type)
{
  char *p, *tokens[MAX_INPUT_LENGTH];
  char line[MAX_INPUT_LENGTH], lhr[MAX_INPUT_LENGTH], rhr[MAX_INPUT_LENGTH];

  lhr[0] = '\0';
  rhr[0] = '\0';
  int i, j;

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

  p = strcpy(line, expr);

  /*
   * initialize tokens, an array of pointers to locations
   * in line where the ops could possibly occur.
   */

  /* Might be game breaking - Iovan
  *lhr = '\0'; */

  for (j = 0; *p; j++) {
    tokens[j] = p;
    if (*p == '(')
      p = matching_paren(p) + 1;
    else if (*p == '"')
      p = matching_quote(p) + 1;
    else if (isalnum(*p))
      for (p++; *p && (isalnum(*p) || isspace(*p)); p++);
    else
      p++;
  }
  tokens[j] = NULL;

  for (i = 0; *ops[i] != '\n'; i++)
    for (j = 0; tokens[j]; j++)
      if (!strncasecmp(ops[i], tokens[j], strlen(ops[i]))) {
        *tokens[j] = '\0';
        p          = tokens[j] + strlen(ops[i]);

        eval_expr(line, lhr, go, sc, trig, type);
        eval_expr(p, rhr, go, sc, trig, type);
        eval_op(ops[i], lhr, rhr, result);

        return 1;
      }

  return 0;
}

/* returns 1 if cond is true, else 0 */
bool process_if(char *cond, void *go, struct script_data *sc,
                trig_data *trig, int type)
{
  char result[MAX_INPUT_LENGTH], *p;

  eval_expr(cond, result, go, sc, trig, type);

  p = result;
  skip_spaces(&p);

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
struct cmdlist_element *find_end(trig_data *trig, struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  char *p;

  if (!(cl->next)) { /* rryan: if this is the last line, theres no end. */
    script_log("Trigger VNum %d has 'if' without 'end'. (error 1)", trig_get_vnum(trig));
    return cl;
  }

  for (c = cl->next; c; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strncasecmp("if ", p, 3))
      c = find_end(trig, c);
    else if (!strncasecmp("end", p, 3))
      return c;

    /* thanks to Russell Ryan for this fix */
    if (!c->next) { /* rryan: this is the last line, we didn't find an end. */
      script_log("Trigger VNum %d has 'if' without 'end'. (error 2)", trig_get_vnum(trig));
      return c;
    }
  }

  /* rryan: we didn't find an end. */
  script_log("Trigger VNum %d has 'if' without 'end'. (error 3)", trig_get_vnum(trig));
  return c;
}

/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */
struct cmdlist_element *find_else_end(trig_data *trig,
                                      struct cmdlist_element *cl, void *go,
                                      struct script_data *sc, int type)
{
  struct cmdlist_element *c;
  char *p;

  if (!(cl->next))
    return cl;

  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++); /* skip spaces */

    if (!strncasecmp("if ", p, 3))
      c = find_end(trig, c);

    else if (!strncasecmp("elseif ", p, 7)) {
      if (process_if(p + 7, go, sc, trig, type)) {
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
      script_log("Trigger VNum %d has 'if' without 'end'. (error 4)", trig_get_vnum(trig));
      return c;
    }
  }

  /* rryan: if we got here, it's the last line, if its not an end, script_log it. */
  for (p = c->cmd; *p && isspace(*p); p++); /* skip spaces */
  if (strncasecmp("end", p, 3))
    script_log("Trigger VNum %d has 'if' without 'end'. (error 5)", trig_get_vnum(trig));
  return c;
}
/*
* scans for a case/default instance
* returns the line containg the correct case instance, or the last
* line of the trigger if not found.
*/
struct cmdlist_element *
find_case(struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, char *cond)
{
  char result[MAX_INPUT_LENGTH], match[MAX_INPUT_LENGTH];
  struct cmdlist_element *c;
  char *p;

  eval_expr(cond, result, go, sc, trig, type);

  if (!(cl->next))
    return cl;

  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strncasecmp("while ", p, 6) || !strncasecmp("switch", p, 6))
      c = find_done(c);
    else if (!strncasecmp("case ", p, 5)) {
      eval_op("==", result, p + 5, match);
      if (*match && *match != '0')
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
  char *p;

  if (!cl || !(cl->next))
    return cl;

  for (c = cl->next; c && c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strncasecmp("while ", p, 6) || !strncasecmp("switch ", p, 7))
      c = find_done(c);
    else if (!strncasecmp("done", p, 3))
      return c;
  }

  return c;
}
