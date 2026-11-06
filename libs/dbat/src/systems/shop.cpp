/*************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/
#include "dbat/shop.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/spells.h"
#include "dbat/feats.h"
#include "dbat/act.comm.h"
#include "dbat/act.wizard.h"
#include "dbat/act.social.h"
#include "dbat/broadcast.h"
#include "dbat/act.other.h"
#include "dbat/class.h"
#include "dbat/fileop.h"
#include "dbat/genolc.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static bool shop_trade_flagged(const struct shop_data *rec, int flag);

const char *const shop_default_msgs[NUM_SHOP_MSGS] = {
    "%s Sorry, I don't stock that item.",  /* SHOP_MSG_NOITEM1 */
    "%s You don't seem to have that.",     /* SHOP_MSG_NOITEM2 */
    "%s I don't trade in such items.",     /* SHOP_MSG_NOBUY */
    "%s I can't afford that!",             /* SHOP_MSG_NOCASH1 */
    "%s You are too poor!",                /* SHOP_MSG_NOCASH2 */
    "%s That'll be %d zenni, thanks.",     /* SHOP_MSG_BUY */
    "%s I'll give you %d zenni for that.", /* SHOP_MSG_SELL */
};

/* Forward/External function declarations */
static void sort_keeper_objs(GameWorld *g, struct char_data *keeper, shop_rnum shop_nr);

/* Local variables */

/* local functions */
static char *read_shop_message(enum shop_message mnum, room_vnum shr, FILE *shop_f, const char *why);
static int read_type_list(GameWorld *g, FILE *shop_f, struct shop_buy_data *list, int new_format, int max);
static int read_list(GameWorld *g, FILE *shop_f, struct shop_buy_data *list, int new_format, int max, int type);
static void shopping_list(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr);
static void shopping_value(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr);
static void shopping_sell(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr);
static struct obj_data *get_selling_obj(GameWorld *g, struct char_data *ch, char *name, struct char_data *keeper, shop_rnum shop_nr, int msg);
static struct obj_data *slide_obj(GameWorld *g, struct obj_data *obj, struct char_data *keeper, int shop_nr);
static void shopping_buy(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr);
static void shopping_app(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr);
static struct obj_data *get_purchase_obj(GameWorld *g, struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg);
static struct obj_data *get_hash_obj_vis(GameWorld *g, struct char_data *ch, char *name, struct obj_data *list);
static struct obj_data *get_slide_obj_vis(GameWorld *g, struct char_data *ch, char *name, struct obj_data *list);
static char *customer_string(GameWorld *g, shop_rnum shop_nr, int detailed);
static void list_all_shops(GameWorld *g, struct char_data *ch);
static void list_detailed_shop(GameWorld *g, struct char_data *ch, shop_rnum shop_nr);
static bool is_ok_char(GameWorld *g, struct char_data *keeper, struct char_data *ch, shop_rnum shop_nr);
static bool is_ok_obj(GameWorld *g, struct char_data *keeper, struct char_data *ch, struct obj_data *obj, shop_rnum shop_nr);
static bool is_open(GameWorld *g, struct char_data *keeper, shop_rnum shop_nr, int msg);
static bool is_ok(GameWorld *g, struct char_data *keeper, struct char_data *ch, shop_rnum shop_nr);
static void push(struct stack_data *stack, int pushval);
static int top(struct stack_data *stack);
static int pop(struct stack_data *stack);
static void evaluate_operation(struct stack_data *ops, struct stack_data *vals);
static int find_oper_num(char token);
static int evaluate_expression(struct obj_data *obj, char *expr);
static int trade_with(GameWorld *g, struct obj_data *item, shop_rnum shop_nr);
static bool same_obj(struct obj_data *obj1, struct obj_data *obj2);

static int transaction_amt(char *arg);
static char *times_message(struct obj_data *obj, char *name, int num);
static int buy_price(GameWorld *g, struct obj_data *obj, shop_rnum shop_nr, struct char_data *keeper, struct char_data *buyer);
static int sell_price(GameWorld *g, struct obj_data *obj, shop_rnum shop_nr, struct char_data *keeper, struct char_data *seller);
static char *list_object(GameWorld *g, struct obj_data *obj, int cnt, int oindex, shop_rnum shop_nr, struct char_data *keeper, struct char_data *seller);

static bool add_to_list(GameWorld *g, struct shop_buy_data *list, int type, int *len, int *val);
static int end_read_list(struct shop_buy_data *list, int len, int error);
static void read_line(GameWorld *g, FILE *shop_f, const char *string, void *data);

/* config arrays */
static const char *const operator_str[] = {
    "[({",
    "])}",
    "|+",
    "&*",
    "^'"};

/* Constant list for printing out who we sell to */
const char *const trade_letters[NUM_TRADERS + 1] = {
    "No Good", /* First, the alignment based ones */
    "No Evil",
    "No Neutral",
    "No Roshi", /* Then the class based ones */
    "No Piccolo",
    "No Krane",
    "No Nail",
    "No Human", /* And finally the race based ones */
    "No Icer",
    "No Saiyan",
    "No Konatsu",
    "No Namek",
    "No Mutant",
    "No Kanassan",
    "No Bio",
    "No Android",
    "No Demon",
    "No Majin",
    "No Kai",
    "No Truffle",
    "No Goblin",
    "No Animal",
    "No Orc",
    "No Snake",
    "No Halfbreed",
    "No Minotaur",
    "No Kobold",
    "No Lizardfolk",
    "No Bardock",
    "No Ginyu",
    "UNUSED",
    "Must be Roshi",
    "Must be Piccolo",
    "Must be Krane",
    "Must be Nail",
    "Must be Bardock",
    "Must be Ginyu",
    "No Frieza",
    "No Tapion",
    "No Android 16",
    "No Dabura",
    "No Kabito",
    "Must be Frieza",
    "Must be Tapion",
    "Must be Android 16",
    "Must be Dabura",
    "Must be Kabito",
    "Must be Jinto",
    "Must be Tsuna",
    "Must be Kurzak",
    "Must be Assassin",
    "Must be Blackguard",
    "Must be Dragon Disciple",
    "Must be Duelist",
    "Must be Dwarven Defender",
    "Must be Eldritch Knight",
    "Must be Hierophant",
    "Must be Horizon Walker",
    "Must be Loremaster",
    "Must be Mystic Theurge",
    "Must be Shadowdancer",
    "Must be Thaumaturgist",
    "No Jinto",
    "No Tsuna",
    "No Kurzak",
    "No Assassin",
    "No Blackguard",
    "No Dragon Disciple",
    "No Duelist",
    "No Dwarven Defender",
    "No Eldritch Knight",
    "No Hierophant",
    "No Horizon Walker",
    "No Loremaster",
    "No Mystic Theurge",
    "No Shadowdancer",
    "No Thaumaturgist",
    "\n"};

const char *const shop_bits[] = {
    "WILL_FIGHT",
    "USES_BANK",
    "ALLOW_STEAL",
    "\n"};

static bool is_ok_char(GameWorld *g, struct char_data *keeper, struct char_data *ch, shop_rnum shop_nr)
{
  char buf[MAX_INPUT_LENGTH];

  if (!char_can_see(g, keeper, ch)) {
    char actbuf[MAX_INPUT_LENGTH] = MSG_NO_SEE_CHAR;
    do_say(g, keeper, actbuf, g->cmd_say, 0);
    return (false);
  }
  if (char_adm_flagged(ch, ADM_ALLSHOPS))
    return (true);

  if ((char_get_alignment(ch) > 0 && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOGOOD)) ||
      (char_get_alignment(ch) < 0 && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOEVIL)) ||
      (char_get_alignment(ch) == 0 && shop_trade_flagged(&g->shops[shop_nr], TRADE_NONEUTRAL))) {
    snprintf(buf, sizeof(buf), "%s %s", char_get_name(ch), MSG_NO_SELL_ALIGN);
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return (false);
  }
  if (char_is_npc(ch))
    return (true);

  if ((char_is_roshi(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOWIZARD)) ||
      (char_is_piccolo(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOCLERIC)) ||
      (char_is_krane(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOROGUE)) ||
      (char_is_bardock(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOMONK)) ||
      (char_is_ginyu(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOPALADIN)) ||
      (char_is_nail(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOFIGHTER)) ||
      (char_is_kabito(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOBARBARIAN)) ||
      (char_is_frieza(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOSORCERER)) ||
      (char_is_andsix(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOBARD)) ||
      (char_is_dabura(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NORANGER)) ||
      (char_is_tapion(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NODRUID)) ||
      (char_is_nail(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOFIGHTER)) ||
      (char_is_jinto(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOARCANE_ARCHER)) ||
      (char_is_tsuna(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOARCANE_TRICKSTER)) ||
      (char_is_kurzak(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOARCHMAGE)) ||
      (char_is_assassin(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOASSASSIN)) ||
      (char_is_blackguard(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOBLACKGUARD)) ||
      (char_is_dragon_disciple(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NODRAGON_DISCIPLE)) ||
      (char_is_duelist(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NODUELIST)) ||
      (char_is_dwarven_defender(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NODWARVEN_DEFENDER)) ||
      (char_is_eldritch_knight(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOELDRITCH_KNIGHT)) ||
      (char_is_hierophant(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOHIEROPHANT)) ||
      (char_is_horizon_walker(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOHORIZON_WALKER)) ||
      (char_is_loremaster(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOLOREMASTER)) ||
      (char_is_mystic_theurge(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOMYSTIC_THEURGE)) ||
      (char_is_shadowdancer(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOSHADOWDANCER)) ||
      (char_is_thaumaturgist(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOTHAUMATURGIST))) {

    snprintf(buf, sizeof(buf), "%s %s", char_get_name(ch), MSG_NO_SELL_CLASS);
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return (false);
  }
  if ((char_is_human(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOHUMAN)) ||
      (char_is_icer(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOICER)) ||
      (char_is_saiyan(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOSAIYAN)) ||
      (char_is_konatsu(ch) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOKONATSU))) {
    snprintf(buf, sizeof(buf), "%s %s", char_get_name(ch), MSG_NO_SELL_RACE);
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return (false);
  }

  return (true);
}

static bool is_ok_obj(GameWorld *g, struct char_data *keeper, struct char_data *ch, struct obj_data *obj, shop_rnum shop_nr)
{
  char buf[MAX_INPUT_LENGTH];

  if (obj_extra_flagged(obj, ITEM_BROKEN) && shop_trade_flagged(&g->shops[shop_nr], TRADE_NOBROKEN)) {
    snprintf(buf, sizeof(buf), "%s %s", char_get_name(ch), MSG_NO_BUY_BROKEN);
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return (false);
  }
  if (obj_extra_flagged(obj, ITEM_FORGED)) {
    snprintf(buf, sizeof(buf), "%s that piece of junk is an obvious forgery!", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return (false);
  }

  return (true);
}

static bool is_open(GameWorld *g, struct char_data *keeper, shop_rnum shop_nr, int msg)
{
  char buf[MAX_INPUT_LENGTH];

  *buf = '\0';
  if (rshop_get_open1(g, shop_nr) > g->time_info.hours)
    strlcpy(buf, MSG_NOT_OPEN_YET, sizeof(buf));
  else if (rshop_get_close1(g, shop_nr) < g->time_info.hours) {
    if (rshop_get_open2(g, shop_nr) > g->time_info.hours)
      strlcpy(buf, MSG_NOT_REOPEN_YET, sizeof(buf));
    else if (rshop_get_close2(g, shop_nr) < g->time_info.hours)
      strlcpy(buf, MSG_CLOSED_FOR_DAY, sizeof(buf));
  }
  if (!*buf)
    return (true);
  if (msg)
    do_say(g, keeper, buf, g->cmd_tell, 0);
  return (false);
}

static bool is_ok(GameWorld *g, struct char_data *keeper, struct char_data *ch, shop_rnum shop_nr)
{
  if (is_open(g, keeper, shop_nr, true))
    return (is_ok_char(g, keeper, ch, shop_nr));
  else
    return (false);
}

static void push(struct stack_data *stack, int pushval)
{
  stack->data[stack->len++] = pushval;
}

static int top(struct stack_data *stack)
{
  if (stack->len > 0)
    return (stack->data[stack->len - 1]);
  else
    return (-1);
}

static int pop(struct stack_data *stack)
{
  if (stack->len > 0)
    return (stack->data[--stack->len]);
  else {
    mud_log("SYSERR: Illegal expression %d in shop keyword list.", stack->len);
    return (0);
  }
}

static void evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
  int oper;

  if ((oper = pop(ops)) == OPER_NOT)
    push(vals, !pop(vals));
  else {
    int val1 = pop(vals),
        val2 = pop(vals);

    /* Compiler would previously short-circuit these. */
    if (oper == OPER_AND)
      push(vals, val1 && val2);
    else if (oper == OPER_OR)
      push(vals, val1 || val2);
  }
}

static int find_oper_num(char token)
{
  int oindex;

  for (oindex = 0; oindex <= MAX_OPER; oindex++)
    if (strchr(operator_str[oindex], token))
      return (oindex);
  return (NOTHING);
}

static int evaluate_expression(struct obj_data *obj, char *expr)
{
  struct stack_data ops, vals;
  char *ptr, *end, name[MAX_STRING_LENGTH];
  int temp, eindex;

  if (!expr || !*expr) /* Allows opening ( first. */
    return (true);

  ops.len = vals.len = 0;
  ptr                = expr;
  while (*ptr) {
    if (isspace(*ptr))
      ptr++;
    else {
      if ((temp = find_oper_num(*ptr)) == NOTHING) {
        end = ptr;
        while (*ptr && !isspace(*ptr) && find_oper_num(*ptr) == NOTHING)
          ptr++;
        strncpy(name, end, ptr - end); /* strncpy: OK (name/end:MAX_STRING_LENGTH) */
        name[ptr - end] = '\0';
        for (eindex = 0; *extra_bits[eindex] != '\n'; eindex++)
          if (!strcasecmp(name, extra_bits[eindex])) {
            push(&vals, obj_extra_flagged(obj, eindex));
            break;
          }
        if (*extra_bits[eindex] == '\n')
          push(&vals, isname(name, obj_get_name(obj)));
      } else {
        if (temp != OPER_OPEN_PAREN)
          while (top(&ops) > temp)
            evaluate_operation(&ops, &vals);

        if (temp == OPER_CLOSE_PAREN) {
          if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
            mud_log("SYSERR: Illegal parenthesis in shop keyword expression.");
            return (false);
          }
        } else
          push(&ops, temp);
        ptr++;
      }
    }
  }
  while (top(&ops) != -1)
    evaluate_operation(&ops, &vals);
  temp = pop(&vals);
  if (top(&vals) != -1) {
    mud_log("SYSERR: Extra operands left on shop keyword expression stack.");
    return (false);
  }
  return (temp);
}

static int trade_with(GameWorld *g, struct obj_data *item, shop_rnum shop_nr)
{
  int counter;

  if (obj_get_cost(item) < 1)
    return (OBJECT_NOVAL);

  if (obj_extra_flagged(item, ITEM_NOSELL))
    return (OBJECT_NOTOK);

  for (counter = 0; rshop_get_buy_type(g, shop_nr, counter) != NOTHING; counter++)
    if (rshop_get_buy_type(g, shop_nr, counter) == obj_get_type_flag(item)) {
      if (obj_get_value(item, VAL_WAND_CHARGES) == 0 &&
          (obj_get_type_flag(item) == ITEM_WAND ||
           obj_get_type_flag(item) == ITEM_STAFF))
        return (OBJECT_DEAD);
      else if (evaluate_expression(item, rshop_get_buy_word(g, shop_nr, counter)))
        return (OBJECT_OK);
    }
  return (OBJECT_NOTOK);
}

static bool same_obj(struct obj_data *obj1, struct obj_data *obj2)
{
  int aindex, i;
  int ef1, ef2;

  if (!obj1 || !obj2)
    return (obj1 == obj2);

  if (obj_get_item_number(obj1) != obj_get_item_number(obj2))
    return (false);

  if (obj_get_cost(obj1) != obj_get_cost(obj2))
    return (false);

  for (i = 0; i < EF_ARRAY_MAX; i++) {
    ef1 = obj_get_extra_flags(obj1)[i];
    ef2 = obj_get_extra_flags(obj2)[i];
    if (i == Q_FIELD(ITEM_UNIQUE_SAVE)) {
      ef1 &= ~Q_BIT(ITEM_UNIQUE_SAVE);
      ef2 &= ~Q_BIT(ITEM_UNIQUE_SAVE);
    }
    if (ef1 != ef2)
      return (false);
  }

  for (aindex = 0; aindex < MAX_OBJ_AFFECT; aindex++)
    if ((obj_get_affected(obj1)[aindex].location != obj_get_affected(obj2)[aindex].location) ||
        (obj_get_affected(obj1)[aindex].modifier != obj_get_affected(obj2)[aindex].modifier))
      return (false);

  return (true);
}

bool shop_producing(GameWorld *g, struct obj_data *item, shop_rnum shop_nr)
{
  int counter;

  if (obj_get_item_number(item) == NOTHING)
    return (false);

  for (counter = 0; rshop_get_product(g, shop_nr, counter) != NOTHING; counter++)
    if (same_obj(item, &g->obj_proto[rshop_get_product(g, shop_nr, counter)]))
      return (true);
  return (false);
}

static int transaction_amt(char *arg)
{
  char buf[MAX_INPUT_LENGTH];

  char *buywhat;

  /*
   * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
   * We don't do that if we only have one argument, like 'buy 5', buy #5.
   * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
   */
  buywhat = one_argument(arg, buf);
  if (*buywhat && *buf && is_number(buf)) {
    memmove(arg, arg + strlen(buf) + 1, strlen(arg + strlen(buf) + 1) + 1); /* overlapping, so not strcpy */
    return (atoi(buf));
  }
  return (1);
}

static char *times_message(struct obj_data *obj, char *name, int num)
{
  static char buf[256];
  size_t len;
  char *ptr;

  if (obj)
    len = strlcpy(buf, obj_get_short_description(obj), sizeof(buf));
  else {
    if ((ptr = strchr(name, '.')) == nullptr)
      ptr = name;
    else
      ptr++;
    len = snprintf(buf, sizeof(buf), "%s %s", AN(ptr), ptr);
  }

  if (num > 1 && len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, " (x %d)", num);

  return (buf);
}

static struct obj_data *get_slide_obj_vis(GameWorld *g, struct char_data *ch, char *name,
                                          struct obj_data *list)
{
  struct obj_data *i, *last_match = nullptr;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strlcpy(tmpname, name, sizeof(tmpname));
  tmp = tmpname;
  if (!(number = get_number(&tmp)))
    return (nullptr);

  for (i = list, j = 1; i && (j <= number); i = obj_get_next_content(i))
    if (isname(tmp, obj_get_name(i)))
      if (char_can_see_obj(g, ch, i) && !same_obj(last_match, i)) {
        if (j == number)
          return (i);
        last_match = i;
        j++;
      }
  return (nullptr);
}

static struct obj_data *get_hash_obj_vis(GameWorld *g, struct char_data *ch, char *name,
                                         struct obj_data *list)
{
  struct obj_data *loop, *last_obj = nullptr;
  int qindex;

  if (is_number(name))
    qindex = atoi(name);
  else if (is_number(name + 1))
    qindex = atoi(name + 1);
  else
    return (nullptr);

  for (loop = list; loop; loop = obj_get_next_content(loop))
    if (char_can_see_obj(g, ch, loop) && obj_get_cost(loop) > 0)
      if (!same_obj(last_obj, loop)) {
        if (--qindex == 0)
          return (loop);
        last_obj = loop;
      }
  return (nullptr);
}

static struct obj_data *get_purchase_obj(GameWorld *g, struct char_data *ch, char *arg,
                                         struct char_data *keeper, shop_rnum shop_nr, int msg)
{
  char name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(arg, name);
  do {
    if (*name == '#' || is_number(name))
      obj = get_hash_obj_vis(g, ch, name, char_get_inventory(keeper));
    else
      obj = get_slide_obj_vis(g, ch, name, char_get_inventory(keeper));
    if (!obj) {
      if (msg) {
        char buf[MAX_INPUT_LENGTH];

        snprintf(buf, sizeof(buf), g->shops[shop_nr].no_such_item1, char_get_name(ch));
        do_tell(g, keeper, buf, g->cmd_tell, 0);
      }
      return (nullptr);
    }
    if (obj_get_cost(obj) <= 0) {
      extract_obj(g, obj);
      obj = nullptr;
    }
  } while (!obj);
  return (obj);
}

/*
 * Shop purchase adjustment, based on charisma-difference from buyer to keeper.
 *
 * for i in `seq 15 -15`; do printf " * %3d: %6.4f\n" $i \
 * `echo "scale=4; 1+$i/70" | bc`; done
 *
 *  Shopkeeper higher charisma (markup)
 *  ^  15: 1.2142  14: 1.2000  13: 1.1857  12: 1.1714  11: 1.1571
 *  |  10: 1.1428   9: 1.1285   8: 1.1142   7: 1.1000   6: 1.0857
 *  |   5: 1.0714   4: 1.0571   3: 1.0428   2: 1.0285   1: 1.0142
 *  +   0: 1.0000
 *  |  -1: 0.9858  -2: 0.9715  -3: 0.9572  -4: 0.9429  -5: 0.9286
 *  |  -6: 0.9143  -7: 0.9000  -8: 0.8858  -9: 0.8715 -10: 0.8572
 *  v -11: 0.8429 -12: 0.8286 -13: 0.8143 -14: 0.8000 -15: 0.7858
 *  Player higher charisma (discount)
 *
 * Most mobiles have 11 charisma so an 18 charisma player would get a 10%
 * discount beyond the basic price.  That assumes they put a lot of points
 * into charisma, because on the flip side they'd get 11% inflation by
 * having a 3.
 */
static int buy_price(GameWorld *g, struct obj_data *obj, shop_rnum shop_nr, struct char_data *keeper, struct char_data *buyer)
{
  int cost = (obj_get_cost(obj) * rshop_get_profit_buy(g, shop_nr));

  /*
  double adjust = 1.0;
  struct obj_data *k;

  for (k = g->objects; k; k = k->next) {
   if (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(obj)) {
    adjust -= 0.00025;
   }
  }
  if (adjust < 0.015) {
   adjust = 0.5;
  }

  cost = cost * adjust;
  */

  if (!char_is_npc(buyer)) {
    if (char_get_bonuses(buyer, BONUS_THRIFTY) > 0) {
      if (char_is_arlian(buyer)) {
        cost += cost * 0.2;
      } else {
        cost -= cost * 0.1;
      }
    } else if (char_get_bonuses(buyer, BONUS_IMPULSE)) {
      cost += cost * 0.25;
    } else if (char_is_arlian(buyer)) {
      cost += cost * 0.20;
    }
  }

  return cost;
}

/*
 * When the shopkeeper is buying, we reverse the discount. Also make sure
 * we don't buy for more than we sell for, to prevent infinite money-making.
 */
static int sell_price(GameWorld *g, struct obj_data *obj, shop_rnum shop_nr, struct char_data *keeper, struct char_data *seller)
{
  float sell_cost_modifier = rshop_get_profit_sell(g, shop_nr);
  float buy_cost_modifier  = rshop_get_profit_buy(g, shop_nr);

  if (sell_cost_modifier > buy_cost_modifier)
    sell_cost_modifier = buy_cost_modifier;

  double adjust = 1.0;
  struct obj_data *k;

  for (k = g->objects; k; k = obj_get_next(k)) {
    if (obj_get_vnum(g, k) == obj_get_vnum(g, obj)) {
      adjust -= 0.00025;
    }
  }
  if (adjust < 0.15) {
    adjust = 0.15;
  }

  if (!char_is_npc(seller) && char_get_bonuses(seller, BONUS_THRIFTY) > 0) {
    int haggle = (obj_get_cost(obj) * (sell_cost_modifier / 2));
    if (char_is_arlian(seller)) {
      haggle -= haggle * 0.2;
    }
    haggle += haggle * 0.1;
    haggle = haggle * adjust;
    return (haggle);
  }
  if (!char_is_npc(seller) && char_is_arlian(seller)) {
    int haggle = (obj_get_cost(obj) * (sell_cost_modifier / 2));
    haggle -= haggle * 0.2;
    haggle = haggle * adjust;
    return (haggle);
  }
  return (int)((obj_get_cost(obj) * (sell_cost_modifier / 2)) * adjust);
}

static void shopping_app(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr)
{
  struct obj_data *obj;
  int i, found = false;
  char buf[MAX_STRING_LENGTH];

  if (!is_ok(g, keeper, ch, shop_nr))
    return;

  if (rshop_get_lastsort(g, shop_nr) < char_get_carry_items(keeper))
    sort_keeper_objs(g, keeper, shop_nr);

  if (!*arg) {
    snprintf(buf, sizeof(buf), "%s What do you want to appraise?", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  if (!(obj = get_purchase_obj(g, ch, arg, keeper, shop_nr, true))) {
    do_appraise(g, ch, arg, 0, 0);
    return;
  }

  act(g, "@C$N@W hands you @G$p@W for a moment and let's you examine it before taking it back.@n", true, ch, obj, keeper, TO_CHAR);
  act(g, "@c$N@W hands @C$n@W @G$p@W for a moment and let's $m examine it before taking it back.@n", true, ch, obj, keeper, TO_ROOM);

  if (!char_get_skill(ch, SKILL_APPRAISE)) {
    send_to_char(g, ch, "You are unskilled at appraising.\r\n");
    return;
  }
  improve_skill(g, ch, SKILL_APPRAISE, 1);
  if (char_get_skill(ch, SKILL_APPRAISE) < rand_number(g, 1, 101)) {
    send_to_char(g, ch, "@wYou were completely stumped about the worth of %s@n\r\n", obj_get_short_description(obj));
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  int displevel = obj_get_level(obj);

  if (obj_get_type_flag(obj) == ITEM_WEAPON && obj_extra_flagged(obj, ITEM_CUSTOM))
    displevel = 20;

  send_to_char(g, ch, "@c---------------------------------------------------------------@n\n");
  send_to_char(g, ch, "@GItem Name   @W: @w%s@n\n", obj_get_short_description(obj));
  send_to_char(g, ch, "@GTrue Value  @W: @Y%s@n\n", add_commas(obj_get_cost(obj)));
  send_to_char(g, ch, "@GItem Min LVL@W: @w%d@n\n", displevel);
  if (obj_get_value(obj, VAL_ALL_HEALTH) >= 100) {
    send_to_char(g, ch, "@GCondition   @W: @C%d%s@n\n", obj_get_value(obj, VAL_ALL_HEALTH), "%");
  } else if (obj_get_value(obj, VAL_ALL_HEALTH) >= 50) {
    send_to_char(g, ch, "@GCondition   @W: @y%d%s@n\n", obj_get_value(obj, VAL_ALL_HEALTH), "%");
  } else if (obj_get_value(obj, VAL_ALL_HEALTH) >= 1) {
    send_to_char(g, ch, "@GCondition   @W: @r%d%s@n\n", obj_get_value(obj, VAL_ALL_HEALTH), "%");
  } else {
    send_to_char(g, ch, "@GCondition   @W: @D%d%s@n\n", obj_get_value(obj, VAL_ALL_HEALTH), "%");
  }
  send_to_char(g, ch, "@GItem Size   @W:@w %s@n\n", size_names[obj_get_size(obj)]);
  send_to_char(g, ch, "@GItem Weight @W: @w%s@n\n", add_commas(obj_get_weight(obj)));
  if (obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "GToken Slots  @W: @m0/1@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT1) && obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "GToken Slots  @W: @m1/1@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "GToken Slots  @W: @m0/2@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "GToken Slots  @W: @m1/2@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "GToken Slots  @W: @m2/2@n\n");
  }
  char bits[MAX_STRING_LENGTH];
  sprintbitarray(obj_get_wear_flags(obj), wear_bits, TW_ARRAY_MAX, bits, sizeof(bits));
  search_replace(bits, "TAKE", "");
  send_to_char(g, ch, "@GWear Loc.   @W:@w%s\n", bits);
  if (obj_get_type_flag(obj) == ITEM_WEAPON) {
    if (obj_extra_flagged(obj, ITEM_WEAPLVL1)) {
      send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w5%s@D]@n\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL2)) {
      send_to_char(g, ch, "@GWeapon Level@W: @D[@C2@D]\n@GDamage Bonus@W: @D[@w10%s@D]@n\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL3)) {
      send_to_char(g, ch, "@GWeapon Level@W: @D[@C3@D]\n@GDamage Bonus@W: @D[@w20%s@D]@n\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL4)) {
      send_to_char(g, ch, "@GWeapon Level@W: @D[@C4@D]\n@GDamage Bonus@W: @D[@w30%s@D]@n\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL5)) {
      send_to_char(g, ch, "@GWeapon Level@W: @D[@C5@D]\n@GDamage Bonus@W: @D[@w50%s@D]@n\r\n", "%");
    }
  }
  send_to_char(g, ch, "@GItem Bonuses@W:@w");
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj_get_affected(obj)[i].modifier) {
      sprinttype(obj_get_affected(obj)[i].location, apply_types, buf, sizeof(buf));
      send_to_char(g, ch, "%s %+d to %s", found++ ? "," : "", obj_get_affected(obj)[i].modifier, buf);
      switch (obj_get_affected(obj)[i].location) {
        case APPLY_FEAT:
          send_to_char(g, ch, " (%s)", g->feat_list[obj_get_affected(obj)[i].specific].name);
          break;
        case APPLY_SKILL:
          send_to_char(g, ch, " (%s)", g->spell_info[obj_get_affected(obj)[i].specific].name);
          break;
      }
    }
  }
  if (!found)
    send_to_char(g, ch, " None@n");
  else
    send_to_char(g, ch, "@n");
  char buf2[MAX_STRING_LENGTH];
  sprintbitarray(obj_get_bitvector(obj), affected_bits, AF_ARRAY_MAX, buf2, sizeof(buf2));
  send_to_char(g, ch, "\n@GSpecial     @W:@w %s", buf2);
  send_to_char(g, ch, "\n@c---------------------------------------------------------------@n\n");
}

static void shopping_buy(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr)
{
  char tempstr[MAX_INPUT_LENGTH], tempbuf[MAX_INPUT_LENGTH];
  struct obj_data *obj, *last_obj = nullptr;
  int goldamt = 0, buynum, bought = 0;

  if (!is_ok(g, keeper, ch, shop_nr))
    return;

  if (rshop_get_lastsort(g, shop_nr) < char_get_carry_items(keeper))
    sort_keeper_objs(g, keeper, shop_nr);

  if ((buynum = transaction_amt(arg)) < 0) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s A negative amount?  Try selling me something.",
             char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  if (!*arg || !buynum) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s What do you want to buy?", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  if (!(obj = get_purchase_obj(g, ch, arg, keeper, shop_nr, true)))
    return;

  if (buy_price(g, obj, shop_nr, keeper, ch) > char_get_gold(ch) && !char_adm_flagged(ch, ADM_MONEY)) {
    char actbuf[MAX_INPUT_LENGTH];

    snprintf(actbuf, sizeof(actbuf), g->shops[shop_nr].missing_cash2, char_get_name(ch));
    do_tell(g, keeper, actbuf, g->cmd_tell, 0);

    switch (rshop_get_temper(g, shop_nr)) {
      case 0:
        do_action(g, keeper, strcpy(actbuf, char_get_name(ch)), g->cmd_puke, 0); /* strcpy: OK (MAX_NAME_LENGTH < MAX_INPUT_LENGTH) */
        return;
      case 1:
        do_echo(g, keeper, strcpy(actbuf, "smokes on his joint."), g->cmd_emote, SCMD_EMOTE); /* strcpy: OK */
        return;
      default:
        return;
    }
  }
  if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
    send_to_char(g, ch, "%s: You can't carry any more items.\r\n", fname(obj_get_name(obj)));
    return;
  }
  if (char_get_carry_weight(ch) + obj_get_weight(obj) > getMaxCarryWeight(g, ch)) {
    send_to_char(g, ch, "%s: You can't carry that much weight.\r\n", fname(obj_get_name(obj)));
    return;
  }
  while (obj && (char_get_gold(ch) >= buy_price(g, obj, shop_nr, keeper, ch) || char_adm_flagged(ch, ADM_MONEY)) && char_get_carry_items(ch) < char_can_carry_n(ch) && bought < buynum && char_get_carry_weight(ch) + obj_get_weight(obj) <= getMaxCarryWeight(g, ch)) {
    int charged;

    bought++;
    /* Test if producing shop ! */
    if (shop_producing(g, obj, shop_nr)) {
      obj = read_object(g, obj_get_item_number(obj), REAL);
      add_unique_id(g, obj);
    } else {
      obj_from_char(g, obj);
      rshop_set_lastsort(g, shop_nr, rshop_get_lastsort(g, shop_nr) - 1);
    }
    obj_to_char(g, obj, ch);
    if (obj_extra_flagged(obj, ITEM_MATURE)) {
      obj_set_value(obj, VAL_MAXMATURE, 6);
    }

    charged = buy_price(g, obj, shop_nr, keeper, ch);
    goldamt += charged;
    if (!char_adm_flagged(ch, ADM_MONEY))
      char_set_gold(ch, char_get_gold(ch) - (charged));
    else {
      send_to_imm(g, "IMM PURCHASE: %s has purchased %s for free.", char_get_name(ch), obj_get_short_description(obj));
      log_imm_action(g, "IMM PURCHASE: %s has purchased %s for free.", char_get_name(ch), obj_get_short_description(obj));
    }

    last_obj = obj;
    obj      = get_purchase_obj(g, ch, arg, keeper, shop_nr, false);
    if (!same_obj(obj, last_obj))
      break;
  }

  if (bought < buynum) {
    char buf[MAX_INPUT_LENGTH];

    if (!obj || !same_obj(last_obj, obj))
      snprintf(buf, sizeof(buf), "%s I only have %d to sell you.", char_get_name(ch), bought);
    else if (char_get_gold(ch) < buy_price(g, obj, shop_nr, keeper, ch))
      snprintf(buf, sizeof(buf), "%s You can only afford %d.", char_get_name(ch), bought);
    else if (char_get_carry_items(ch) >= char_can_carry_n(ch))
      snprintf(buf, sizeof(buf), "%s You can only hold %d.", char_get_name(ch), bought);
    else if (char_get_carry_weight(ch) + obj_get_weight(obj) > getMaxCarryWeight(g, ch))
      snprintf(buf, sizeof(buf), "%s You can only carry %d.", char_get_name(ch), bought);
    else
      snprintf(buf, sizeof(buf), "%s Something screwy only gave you %d.", char_get_name(ch), bought);
    do_tell(g, keeper, buf, g->cmd_tell, 0);
  }
  if (!char_adm_flagged(ch, ADM_MONEY))
    char_set_gold(keeper, char_get_gold(keeper) + (goldamt));

  strlcpy(tempstr, times_message(char_get_inventory(ch), 0, bought), sizeof(tempstr));

  snprintf(tempbuf, sizeof(tempbuf), "$n buys %s.", tempstr);
  act(g, tempbuf, false, ch, obj, 0, TO_ROOM);

  snprintf(tempbuf, sizeof(tempbuf), g->shops[shop_nr].message_buy, char_get_name(ch), goldamt);
  do_tell(g, keeper, tempbuf, g->cmd_tell, 0);

  send_to_char(g, ch, "You now have %s.\r\n", tempstr);

  if (rshop_uses_bank(g, shop_nr))
    if (char_get_gold(keeper) > MAX_OUTSIDE_BANK) {
      rshop_set_bank(g, shop_nr, rshop_get_bank(g, shop_nr) + (char_get_gold(keeper) - MAX_OUTSIDE_BANK));
      char_set_gold(keeper, MAX_OUTSIDE_BANK);
    }
}

static struct obj_data *get_selling_obj(GameWorld *g, struct char_data *ch, char *name, struct char_data *keeper, shop_rnum shop_nr, int msg)
{
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int result;

  if (!(obj = get_obj_in_list_vis(g, ch, name, nullptr, char_get_inventory(ch)))) {
    if (msg) {
      char tbuf[MAX_INPUT_LENGTH];

      snprintf(tbuf, sizeof(tbuf), g->shops[shop_nr].no_such_item2, char_get_name(ch));
      do_tell(g, keeper, tbuf, g->cmd_tell, 0);
    }
    return (nullptr);
  }
  if ((result = trade_with(g, obj, shop_nr)) == OBJECT_OK)
    return (obj);

  if (!msg)
    return (0);

  switch (result) {
    case OBJECT_NOVAL:
      snprintf(buf, sizeof(buf), "%s You've got to be kidding, that thing is worthless!", char_get_name(ch));
      break;
    case OBJECT_NOTOK:
      snprintf(buf, sizeof(buf), g->shops[shop_nr].do_not_buy, char_get_name(ch));
      break;
    case OBJECT_DEAD:
      snprintf(buf, sizeof(buf), "%s %s", char_get_name(ch), MSG_NO_USED_WANDSTAFF);
      break;
    default:
      mud_log("SYSERR: Illegal return value of %d from trade_with() (%s)", result, __FILE__); /* Someone might rename it... */
      snprintf(buf, sizeof(buf), "%s An error has occurred.", char_get_name(ch));
      break;
  }
  do_tell(g, keeper, buf, g->cmd_tell, 0);
  return (nullptr);
}

static struct obj_data *slide_obj(GameWorld *g, struct obj_data *obj, struct char_data *keeper,
                                  shop_rnum shop_nr)
/*
   This function is a slight hack!  To make sure that duplicate items are
   only listed once on the "list", this function groups "identical"
   objects together on the shopkeeper's inventory list.  The hack involves
   knowing how the list is put together, and manipulating the order of
   the objects on the list.  (But since most of DIKU is not encapsulated,
   and information hiding is almost never used, it isn't that big a deal) -JF
*/
{
  struct obj_data *loop;
  IDXTYPE temp;

  if (rshop_get_lastsort(g, shop_nr) < char_get_carry_items(keeper))
    sort_keeper_objs(g, keeper, shop_nr);

  /* Extract the object if it is identical to one produced */
  if (shop_producing(g, obj, shop_nr)) {
    temp = obj_get_item_number(obj);
    extract_obj(g, obj);
    return (&g->obj_proto[temp]);
  }
  rshop_set_lastsort(g, shop_nr, rshop_get_lastsort(g, shop_nr) + 1);
  loop = char_get_inventory(keeper);
  obj_to_char(g, obj, keeper);
  char_set_inventory(keeper, loop);
  while (loop) {
    if (same_obj(obj, loop)) {
      obj_set_next_content(obj, obj_get_next_content(loop));
      obj_set_next_content(loop, obj);
      return (obj);
    }
    loop = obj_get_next_content(loop);
  }
  char_set_inventory(keeper, obj);
  return (obj);
}

static void sort_keeper_objs(GameWorld *g, struct char_data *keeper, shop_rnum shop_nr)
{
  struct obj_data *list = nullptr, *temp;

  while (rshop_get_lastsort(g, shop_nr) < char_get_carry_items(keeper)) {
    temp = char_get_inventory(keeper);
    obj_from_char(g, temp);
    obj_set_next_content(temp, list);
    list               = temp;
  }

  while (list) {
    temp = list;
    list = obj_get_next_content(list);
    if (shop_producing(g, temp, shop_nr) &&
        !get_obj_in_list_num(obj_get_item_number(temp), char_get_inventory(keeper))) {
      obj_to_char(g, temp, keeper);
      rshop_set_lastsort(g, shop_nr, rshop_get_lastsort(g, shop_nr) + 1);
    } else
      slide_obj(g, temp, keeper, shop_nr);
  }
}

static void shopping_sell(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr)
{
  char tempstr[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], tempbuf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int sellnum, sold = 0, goldamt = 0;

  if (!(is_ok(g, keeper, ch, shop_nr)))
    return;

  if ((sellnum = transaction_amt(arg)) < 0) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s A negative amount?  Try buying something.", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  if (!*arg || !sellnum) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s What do you want to sell??", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(g, ch, name, keeper, shop_nr, true)))
    return;

  if (obj_get_type_flag(obj) == ITEM_PLANT && obj_get_value(obj, VAL_WATERLEVEL) <= -10) {
    char buf[MAX_INPUT_LENGTH];
    snprintf(buf, sizeof(buf), "%s That thing is dead!", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }

  if (!(is_ok_obj(g, keeper, ch, obj, shop_nr)))
    return;

  if (char_get_gold(keeper) + rshop_get_bank(g, shop_nr) < sell_price(g, obj, shop_nr, keeper, ch)) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), g->shops[shop_nr].missing_cash1, char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  while (obj && char_get_gold(keeper) + rshop_get_bank(g, shop_nr) >= sell_price(g, obj, shop_nr, keeper, ch) && sold < sellnum) {
    int charged = sell_price(g, obj, shop_nr, keeper, ch);

    goldamt += charged;
    char_set_gold(keeper, char_get_gold(keeper) - (charged));

    sold++;
    obj_from_char(g, obj);
    slide_obj(g, obj, keeper, shop_nr); /* Seems we don't use return value. */
    obj = get_selling_obj(g, ch, name, keeper, shop_nr, false);
  }

  if (sold < sellnum) {
    char buf[MAX_INPUT_LENGTH];

    if (!obj)
      snprintf(buf, sizeof(buf), "%s You only have %d of those.", char_get_name(ch), sold);
    else if (char_get_gold(keeper) + rshop_get_bank(g, shop_nr) < sell_price(g, obj, shop_nr, keeper, ch))
      snprintf(buf, sizeof(buf), "%s I can only afford to buy %d of those.", char_get_name(ch), sold);
    else
      snprintf(buf, sizeof(buf), "%s Something really screwy made me buy %d.", char_get_name(ch), sold);

    do_tell(g, keeper, buf, g->cmd_tell, 0);
  }
  strlcpy(tempstr, times_message(0, name, sold), sizeof(tempstr));
  snprintf(tempbuf, sizeof(tempbuf), "$n sells something to %s.\r\n", char_get_name(keeper));
  act(g, tempbuf, false, ch, obj, 0, TO_ROOM);

  snprintf(tempbuf, sizeof(tempbuf), g->shops[shop_nr].message_sell, char_get_name(ch), goldamt);
  do_tell(g, keeper, tempbuf, g->cmd_tell, 0);

  send_to_char(g, ch, "The shopkeeper gives you %s zenni.\r\n", add_commas(goldamt));
  if (char_get_gold(ch) + goldamt > char_gold_carry(ch)) {
    goldamt = (char_get_gold(ch) + goldamt) - char_gold_carry(ch);
    char_set_gold(ch, char_gold_carry(ch));
    char_set_bank_gold(ch, char_get_bank_gold(ch) + (goldamt));
    send_to_char(g, ch, "You couldn't hold all of the money. The rest was deposited for you.\r\n");
  } else {
    char_set_gold(ch, char_get_gold(ch) + (goldamt));
  }

  if (char_get_gold(keeper) < MIN_OUTSIDE_BANK) {
    goldamt = MIN(MAX_OUTSIDE_BANK - char_get_gold(keeper), rshop_get_bank(g, shop_nr));
    rshop_set_bank(g, shop_nr, rshop_get_bank(g, shop_nr) - (goldamt));
    char_set_gold(keeper, char_get_gold(keeper) + (goldamt));
  }
}

static void shopping_value(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr)
{
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (!is_ok(g, keeper, ch, shop_nr))
    return;

  if (!*arg) {
    snprintf(buf, sizeof(buf), "%s What do you want me to evaluate??", char_get_name(ch));
    do_tell(g, keeper, buf, g->cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(g, ch, name, keeper, shop_nr, true)))
    return;

  if (!is_ok_obj(g, keeper, ch, obj, shop_nr))
    return;

  snprintf(buf, sizeof(buf), "%s I'll give you %d zenni for that!", char_get_name(ch), sell_price(g, obj, shop_nr, keeper, ch));
  do_tell(g, keeper, buf, g->cmd_tell, 0);
}

static char *list_object(GameWorld *g, struct obj_data *obj, int cnt, int aindex, shop_rnum shop_nr, struct char_data *keeper, struct char_data *ch)
{
  static char result[256];
  char itemname[128],
      quantity[16]; /* "Unlimited" or "%d" */

  if (shop_producing(g, obj, shop_nr))
    strcpy(quantity, "Unlimited"); /* strcpy: OK (for 'quantity >= 10') */
  else
    sprintf(quantity, "%d", cnt); /* sprintf: OK (for 'quantity >= 11', 32-bit int) */

  switch (obj_get_type_flag(obj)) {
    case ITEM_DRINKCON:
      if (obj_get_value(obj, VAL_DRINKCON_HOWFULL))
        snprintf(itemname, sizeof(itemname), "%s", obj_get_short_description(obj));
      else
        strlcpy(itemname, obj_get_short_description(obj), sizeof(itemname));
      break;

    case ITEM_WAND:
    case ITEM_STAFF:
      snprintf(itemname, sizeof(itemname), "%s%s", obj_get_short_description(obj),
               obj_get_value(obj, VAL_WAND_CHARGES) < obj_get_value(obj, VAL_WAND_MAXCHARGES) ? " (partially used)" : "");
      break;

    default:
      strlcpy(itemname, obj_get_short_description(obj), sizeof(itemname));
      break;
  }
  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    char titemname[128];
    strlcpy(titemname, itemname, sizeof(titemname));
    snprintf(itemname, sizeof(itemname), "%s [broken]", titemname);
  }

  CAP(itemname);

  int displevel = obj_get_level(obj);

  if (obj_get_type_flag(obj) == ITEM_WEAPON && obj_extra_flagged(obj, ITEM_CUSTOM))
    displevel = 20;

  snprintf(result, sizeof(result), " %2d)  %9s %-*s %3d %13s\r\n", aindex, quantity,
           count_color_chars(itemname) + 36, itemname, displevel, add_commas(buy_price(g, obj, shop_nr, keeper, ch)));
  return (result);
}

static void shopping_list(GameWorld *g, char *arg, struct char_data *ch, struct char_data *keeper, shop_rnum shop_nr)
{
  char buf[MAX_STRING_LENGTH * 4], name[MAX_INPUT_LENGTH];
  struct obj_data *obj, *last_obj = nullptr;
  int cnt = 0, lindex = 0, found = false;
  size_t len;
  /* cnt is the number of that particular object available */

  if (!is_ok(g, keeper, ch, shop_nr))
    return;

  if (rshop_get_lastsort(g, shop_nr) < char_get_carry_items(keeper))
    sort_keeper_objs(g, keeper, shop_nr);

  one_argument(arg, name);

  len = strlcpy(buf, " ##   Available   Item                             Min. Lvl       Cost\r\n"
                     "----------------------------------------------------------------------\r\n",
                sizeof(buf));
  if (char_get_inventory(keeper))
    for (obj = char_get_inventory(keeper); obj; obj = obj_get_next_content(obj))
      if (char_can_see_obj(g, ch, obj) && obj_get_cost(obj) > 0) {
        if (!last_obj) {
          last_obj = obj;
          cnt      = 1;
        } else if (same_obj(last_obj, obj))
          cnt++;
        else {
          lindex++;
          if (!*name || isname(name, obj_get_name(last_obj))) {
            strncat(buf, list_object(g, last_obj, cnt, lindex, shop_nr, keeper, ch), sizeof(buf) - len - 1); /* strncat: OK */
            len = strlen(buf);
            if (len + 1 >= sizeof(buf))
              break;
            found = true;
          }
          cnt      = 1;
          last_obj = obj;
        }
      }
  lindex++;
  if (!last_obj) /* we actually have nothing in our list for sale, period */
    send_to_char(g, ch, "Currently, there is nothing for sale.\r\n");
  else if (*name && !found) /* nothing the char was looking for was found */
    send_to_char(g, ch, "Presently, none of those are for sale.\r\n");
  else {
    char zen[80];
    if (!*name || isname(name, obj_get_name(last_obj))) /* show last obj */
      if (len < sizeof(buf)) {
        strncat(buf, list_object(g, last_obj, cnt, lindex, shop_nr, keeper, ch), sizeof(buf) - len - 1); /* strncat: OK */
      }
    if (len < sizeof(buf)) {
      sprintf(zen, "@W[@wYour Zenni@D: @Y%s@W]", add_commas(char_get_gold(ch)));
      strncat(buf, zen, sizeof(buf) - len - 1);
    }
    send_to_char(g, ch, "%s", buf);
  }
}

bool ok_shop_room(GameWorld *g, shop_rnum shop_nr, room_vnum room)
{
  int mindex;

  for (mindex = 0; rshop_get_room(g, shop_nr, mindex) != NOWHERE; mindex++)
    if (rshop_get_room(g, shop_nr, mindex) == room)
      return (true);
  return (false);
}

SPECIAL(shop_keeper)
{
  struct char_data *keeper = (struct char_data *)me;
  shop_rnum shop_nr;

  for (shop_nr = 0; shop_nr <= g->top_shop; shop_nr++)
    if (rshop_get_keeper(g, shop_nr) == char_get_nr(keeper))
      break;

  if (shop_nr > g->top_shop)
    return (false);

  if (rshop_get_func(g, shop_nr)) /* Check secondary function */
    if ((rshop_get_func(g, shop_nr))(g, ch, me, cmd, argument))
      return (true);

  if (keeper == ch) {
    if (cmd)
      rshop_set_lastsort(g, shop_nr, 0); /* Safety in case "drop all" */
    return (false);
  }
  if (!ok_shop_room(g, shop_nr, rroom_get_vnum(g, char_get_in_room(ch))))
    return (0);

  if (!char_is_awake(keeper))
    return (false);

  if (CMD_IS(g, "steal")) {
    char argm[MAX_INPUT_LENGTH];

    if (!rshop_allows_steal(g, shop_nr)) {
      snprintf(argm, sizeof(argm), "$N shouts '%s'", MSG_NO_STEAL_HERE);
      act(g, argm, false, ch, 0, keeper, TO_CHAR);
      act(g, argm, false, ch, 0, keeper, TO_ROOM);
      do_action(g, keeper, char_get_name(ch), g->cmd_slap, 0);

      return (true);
    }
    return (false);
  }

  if (CMD_IS(g, "buy")) {
    shopping_buy(g, argument, ch, keeper, shop_nr);
    return (true);
  }
  if (CMD_IS(g, "sell")) {
    shopping_sell(g, argument, ch, keeper, shop_nr);
    return (true);
  }
  if (CMD_IS(g, "value")) {
    shopping_value(g, argument, ch, keeper, shop_nr);
    return (true);
  }
  if (CMD_IS(g, "list")) {
    shopping_list(g, argument, ch, keeper, shop_nr);
    return (true);
  }
  if (CMD_IS(g, "appraise")) {
    shopping_app(g, argument, ch, keeper, shop_nr);
    return (true);
  }
  return (false);
}

/* val == obj_vnum and obj_rnum (?) */
static bool add_to_list(GameWorld *g, struct shop_buy_data *list, int type, int *len, int *val)
{
  if (*val != NOTHING && *val >= 0) { /* necessary after changing to unsigned v/rnums -- Welcor */
    if (type == LIST_TRADE && *val >= NUM_ITEM_TYPES) {
      mud_log("SYSERR: Shop #%d buys item type %d, which doesn't exist; ignoring it.", rshop_get_vnum(g, g->top_shop), *val);
      return (false);
    }
    if (*len < MAX_SHOP_OBJ) {
      if (type == LIST_PRODUCE)
        *val = real_object(g, *val);
      if (*val != NOTHING) {
        shop_buy_set_type(&(list[*len]), *val);
        shop_buy_set_keywords(&(list[(*len)++]), nullptr);
      } else
        *val = NOTHING;
      return (false);
    } else
      return (true);
  }
  return (false);
}

static int end_read_list(struct shop_buy_data *list, int len, int error)
{
  if (error)
    mud_log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
  shop_buy_set_keywords(&(list[len]), nullptr);
  shop_buy_set_type(&(list[len++]), NOTHING);
  return (len);
}

static void read_line(GameWorld *g, FILE *shop_f, const char *string, void *data)
{
  char buf[READ_SIZE];

  if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
    mud_log("SYSERR: Error in shop #%d, near '%s' with '%s'", rshop_get_vnum(g, g->top_shop), buf, string);
    exit(1);
  }
}

static int read_list(GameWorld *g, FILE *shop_f, struct shop_buy_data *list, int new_format,
                     int max, int type)
{
  int count, temp, len = 0, error = 0;

  if (new_format) {
    for (;;) {
      read_line(g, shop_f, "%d", &temp);
      if (temp < 0) /* Always "-1" the string. */
        break;
      error += add_to_list(g, list, type, &len, &temp);
    }
  } else
    for (count = 0; count < max; count++) {
      read_line(g, shop_f, "%d", &temp);
      error += add_to_list(g, list, type, &len, &temp);
    }
  return (end_read_list(list, len, error));
}

/* END_OF inefficient. */
static int read_type_list(GameWorld *g, FILE *shop_f, struct shop_buy_data *list,
                          int new_format, int max)
{
  int tindex, num, len = 0, error = 0, before;
  char *ptr;
  char buf[MAX_STRING_LENGTH];

  if (!new_format)
    return (read_list(g, shop_f, list, 0, max, LIST_TRADE));

  do {
    if (!fgets(buf, sizeof(buf), shop_f))
      break;
    if ((ptr = strchr(buf, ';')) != nullptr)
      *ptr = '\0';
    else
      *(END_OF(buf) - 1) = '\0';

    num = -1;

    if (strncmp(buf, "-1", 2) != 0)
      for (tindex = 0; *item_types[tindex] != '\n'; tindex++)
        if (!strncasecmp(item_types[tindex], buf, strlen(item_types[tindex]))) {
          num = tindex;
          memmove(buf, buf + strlen(item_types[tindex]), strlen(buf + strlen(item_types[tindex])) + 1);
          break;
        }

    ptr = buf;
    if (num == -1) {
      sscanf(buf, "%d", &num);
      while (*ptr && !isdigit(*ptr))
        ptr++;
      while (isdigit(*ptr))
        ptr++;
    }
    while (isspace(*ptr))
      ptr++;
    while (*ptr && isspace(*(END_OF(ptr) - 1)))
      *(END_OF(ptr) - 1) = '\0';
    before = len;
    error += add_to_list(g, list, LIST_TRADE, &len, &num);
    if (*ptr && len > before)
      shop_buy_set_keywords(&(list[len - 1]), strdup(ptr));
  } while (num >= 0);
  return (end_read_list(list, len, error));
}

static char *read_shop_message(enum shop_message mnum, room_vnum shr, FILE *shop_f, const char *why)
{
  const char *problem;
  char *tbuf;

  /* An empty or malformed message would reach snprintf() as a NULL or unsafe format, so use the default. */
  if (!(tbuf = fread_string(shop_f, why)))
    return strdup(shop_default_msgs[mnum]);

  if ((problem = npc_msg_format_problem(tbuf, mnum == SHOP_MSG_BUY || mnum == SHOP_MSG_SELL))) {
    mud_log("SYSERR: Shop #%d message #%d: %s; using the default.", shr, mnum, problem);
    free(tbuf);
    return strdup(shop_default_msgs[mnum]);
  }
  return (tbuf);
}

void boot_the_shops(GameWorld *g, FILE *shop_f, char *filename, int rec_count)
{
  char *buf, buf2[256], *p;
  room_vnum temp;
  int count, new_format = false;
  struct shop_buy_data list[MAX_SHOP_OBJ + 1];
  int done = false;

  snprintf(buf2, sizeof(buf2), "beginning of shop file %s", filename);

  while (!done) {
    buf = fread_string(shop_f, buf2);
    if (*buf == '#') { /* New shop */
      sscanf(buf, "#%d\n", &temp);
      snprintf(buf2, sizeof(buf2), "shop #%d in shop file %s", temp, filename);
      free(buf); /* Plug memory leak! */
      g->top_shop++;
      if (!g->top_shop)
        CREATE(g->shops, struct shop_data, rec_count);
      rshop_set_vnum(g, g->top_shop, temp);
      temp = read_list(g, shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);
      CREATE(g->shops[g->top_shop].producing, obj_vnum, temp);
      for (count = 0; count < temp; count++)
        rshop_set_product(g, g->top_shop, count, shop_buy_get_type(&(list[count])));

      read_line(g, shop_f, "%f", &(g->shops[(g->top_shop)].profit_buy));
      read_line(g, shop_f, "%f", &(g->shops[(g->top_shop)].profit_sell));

      temp = read_type_list(g, shop_f, list, new_format, MAX_TRADE);
      CREATE(g->shops[g->top_shop].type, struct shop_buy_data, temp);
      for (count = 0; count < temp; count++) {
        rshop_set_buy_type(g, g->top_shop, count, shop_buy_get_type(&(list[count])));
        rshop_set_buy_word(g, g->top_shop, count, shop_buy_get_keywords(&(list[count])));
      }

      g->shops[g->top_shop].no_such_item1 = read_shop_message(SHOP_MSG_NOITEM1, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      g->shops[g->top_shop].no_such_item2 = read_shop_message(SHOP_MSG_NOITEM2, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      g->shops[g->top_shop].do_not_buy    = read_shop_message(SHOP_MSG_NOBUY, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      g->shops[g->top_shop].missing_cash1 = read_shop_message(SHOP_MSG_NOCASH1, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      g->shops[g->top_shop].missing_cash2 = read_shop_message(SHOP_MSG_NOCASH2, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      g->shops[g->top_shop].message_buy   = read_shop_message(SHOP_MSG_BUY, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      g->shops[g->top_shop].message_sell  = read_shop_message(SHOP_MSG_SELL, rshop_get_vnum(g, g->top_shop), shop_f, buf2);
      read_line(g, shop_f, "%d", &(g->shops[(g->top_shop)].temper1));
      read_line(g, shop_f, "%ld", &(g->shops[(g->top_shop)].bitvector));
      read_line(g, shop_f, "%hd", &(g->shops[(g->top_shop)].keeper));

      rshop_set_keeper(g, g->top_shop, real_mobile(g, rshop_get_keeper(g, g->top_shop)));
      CREATE(buf, char, READ_SIZE);
      get_line(shop_f, buf);
      p = buf;
      for (temp = 0; temp < SW_ARRAY_MAX; temp++) {
        if (!p || !*p)
          break;
        if (sscanf(p, "%d", &count) != 1) {
          mud_log("SYSERR: Can't parse TRADE_WITH line in %s: '%s'", buf2, buf);
          break;
        }
        rshop_get_with_who(g, g->top_shop)
            [temp] = count;
        while (isdigit(*p) || *p == '-') {
          p++;
        }
        while (*p && !(isdigit(*p) || *p == '-')) {
          p++;
        }
      }
      free(buf);
      while (temp < SW_ARRAY_MAX)
        rshop_get_with_who(g, g->top_shop)
            [temp++] = 0;

      temp = read_list(g, shop_f, list, new_format, 1, LIST_ROOM);
      CREATE(g->shops[g->top_shop].in_room, room_vnum, temp);
      for (count = 0; count < temp; count++)
        rshop_set_room(g, g->top_shop, count, shop_buy_get_type(&(list[count])));

      read_line(g, shop_f, "%d", &(g->shops[(g->top_shop)].open1));
      read_line(g, shop_f, "%d", &(g->shops[(g->top_shop)].close1));
      read_line(g, shop_f, "%d", &(g->shops[(g->top_shop)].open2));
      read_line(g, shop_f, "%d", &(g->shops[(g->top_shop)].close2));

      rshop_set_bank(g, g->top_shop, 0);
      rshop_set_lastsort(g, g->top_shop, 0);
      rshop_set_func(g, g->top_shop, nullptr);
    } else {
      if (*buf == '$') /* EOF */
        done = true;
      else if (strstr(buf, VERSION3_TAG)) /* New format marker */
        new_format = true;
      free(buf); /* Plug memory leak! */
    }
  }
}

void assign_the_shopkeepers(GameWorld *g)
{
  shop_rnum cindex;

  g->cmd_say   = find_command(g, "say");
  g->cmd_tell  = find_command(g, "tell");
  g->cmd_emote = find_command(g, "emote");
  g->cmd_slap  = find_command(g, "slap");
  g->cmd_puke  = find_command(g, "puke");

  for (cindex = 0; cindex <= g->top_shop; cindex++) {
    if (rshop_get_keeper(g, cindex) == NOBODY)
      continue;

    /* Having rshop_get_func() as 'shop_keeper' will cause infinite recursion. */
    if (g->mob_index[rshop_get_keeper(g, cindex)].func && g->mob_index[rshop_get_keeper(g, cindex)].func != shop_keeper)
      rshop_set_func(g, cindex, g->mob_index[rshop_get_keeper(g, cindex)].func);

    g->mob_index[rshop_get_keeper(g, cindex)].func = shop_keeper;
  }
}

static char *customer_string(GameWorld *g, shop_rnum shop_nr, int detailed)
{
  int sindex = 0, flag = 0, nlen;
  size_t len = 0;
  static char buf[256];

  while (*trade_letters[sindex] != '\n' && len + 1 < sizeof(buf)) {
    if (detailed) {
      if (!shop_trade_flagged(&g->shops[shop_nr], flag)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, ", %s", trade_letters[sindex]);

        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;

        len += nlen;
      }
    } else {
      buf[len++] = (shop_trade_flagged(&g->shops[shop_nr], flag) ? '_' : *trade_letters[sindex]);
      buf[len]   = '\0';

      if (len >= sizeof(buf))
        break;
    }

    sindex++;
    flag += 1;
  }

  buf[sizeof(buf) - 1] = '\0';
  return (buf);
}

/* END_OF inefficient */
static void list_all_shops(GameWorld *g, struct char_data *ch)
{
  const char *list_all_shops_header =
      " ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n"
      "---------------------------------------------------------\r\n";
  shop_rnum shop_nr;
  int headerlen = strlen(list_all_shops_header);
  char buf[MAX_STRING_LENGTH], buf1[16];

  strcpy(buf, list_all_shops_header); /* strcpy: OK (a short constant) */
  size_t len = headerlen;
  for (shop_nr = 0; shop_nr <= g->top_shop && len < sizeof(buf); shop_nr++) {
    if (rshop_get_keeper(g, shop_nr) == NOBODY)
      strcpy(buf1, "<NONE>"); /* strcpy: OK (for 'buf1 >= 7') */
    else
      sprintf(buf1, "%6d", g->mob_index[rshop_get_keeper(g, shop_nr)].vnum); /* sprintf: OK (for 'buf1 >= 11', 32-bit int) */

    len += snprintf(buf + len, sizeof(buf) - len,
                    "%3d   %6d   %6d    %s   %3.2f   %3.2f    %s\r\n",
                    shop_nr + 1, rshop_get_vnum(g, shop_nr), rshop_get_room(g, shop_nr, 0), buf1,
                    rshop_get_profit_sell(g, shop_nr), rshop_get_profit_buy(g, shop_nr),
                    customer_string(g, shop_nr, false));
  }

  send_to_char(g, ch, "%s", buf);
}

static void list_detailed_shop(GameWorld *g, struct char_data *ch, shop_rnum shop_nr)
{
  struct char_data *k;
  int sindex, column;
  char *ptrsave;

  send_to_char(g, ch, "Vnum:       [%5d], Rnum: [%5d]\r\n", rshop_get_vnum(g, shop_nr), shop_nr + 1);

  send_to_char(g, ch, "Rooms:      ");
  column = 12; /* ^^^ strlen ^^^ */
  for (sindex = 0; rshop_get_room(g, shop_nr, sindex) != NOWHERE; sindex++) {
    char buf1[128];
    int linelen;
    room_rnum temp;

    if (sindex) {
      send_to_char(g, ch, ", ");
      column += 2;
    }

    if ((temp = real_room(g, rshop_get_room(g, shop_nr, sindex))) != NOWHERE)
      linelen = snprintf(buf1, sizeof(buf1), "%s (#%d)", rroom_get_name(g, temp), rroom_get_vnum(g, temp));
    else
      linelen = snprintf(buf1, sizeof(buf1), "<UNKNOWN> (#%d)", rshop_get_room(g, shop_nr, sindex));

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(g, ch, "\r\n            ");
      /* 12 is to line up with "Rooms:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(g, ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!sindex)
    send_to_char(g, ch, "Rooms:      None!");

  send_to_char(g, ch, "\r\nShopkeeper: ");
  if (rshop_get_keeper(g, shop_nr) != NOBODY) {
    send_to_char(g, ch, "%s (#%d), Special Function: %s\r\n",
                 char_get_name(&g->mob_proto[rshop_get_keeper(g, shop_nr)]),
                 g->mob_index[rshop_get_keeper(g, shop_nr)].vnum,
                 YESNO(rshop_get_func(g, shop_nr)));

    if ((k = get_char_num(g, rshop_get_keeper(g, shop_nr))))
      send_to_char(g, ch, "Coins:      [%9d], Bank: [%9d] (Total: %d)\r\n",
                   char_get_gold(k), rshop_get_bank(g, shop_nr), char_get_gold(k) + rshop_get_bank(g, shop_nr));
  } else
    send_to_char(g, ch, "<NONE>\r\n");

  send_to_char(g, ch, "Customers:  %s\r\n", (ptrsave = customer_string(g, shop_nr, true)) ? ptrsave : "None");

  send_to_char(g, ch, "Produces:   ");
  column = 12; /* ^^^ strlen ^^^ */
  for (sindex = 0; rshop_get_product(g, shop_nr, sindex) != NOTHING; sindex++) {
    char buf1[128];
    int linelen;

    if (sindex) {
      send_to_char(g, ch, ", ");
      column += 2;
    }
    linelen = snprintf(buf1, sizeof(buf1), "%s (#%d)",
                       obj_get_short_description(&g->obj_proto[rshop_get_product(g, shop_nr, sindex)]),
                       g->obj_index[rshop_get_product(g, shop_nr, sindex)].vnum);

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(g, ch, "\r\n            ");
      /* 12 is to line up with "Produces:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(g, ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!sindex)
    send_to_char(g, ch, "Produces:   Nothing!");

  send_to_char(g, ch, "\r\nBuys:       ");
  column = 12; /* ^^^ strlen ^^^ */
  for (sindex = 0; rshop_get_buy_type(g, shop_nr, sindex) != NOTHING; sindex++) {
    char buf1[128];
    size_t linelen;

    if (sindex) {
      send_to_char(g, ch, ", ");
      column += 2;
    }

    linelen = snprintf(buf1, sizeof(buf1), "%s (#%d) [%s]",
                       item_types[rshop_get_buy_type(g, shop_nr, sindex)],
                       rshop_get_buy_type(g, shop_nr, sindex),
                       rshop_get_buy_word(g, shop_nr, sindex) ? rshop_get_buy_word(g, shop_nr, sindex) : "all");

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(g, ch, "\r\n            ");
      /* 12 is to line up with "Buys:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(g, ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!sindex)
    send_to_char(g, ch, "Buys:       Nothing!");

  send_to_char(g, ch, "\r\nBuy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]\r\n",
               rshop_get_profit_sell(g, shop_nr), rshop_get_profit_buy(g, shop_nr), rshop_get_open1(g, shop_nr),
               rshop_get_close1(g, shop_nr), rshop_get_open2(g, shop_nr), rshop_get_close2(g, shop_nr));

  /* Need a local buffer. */
  {
    char buf1[128];
    sprintbit(rshop_get_bitvector(g, shop_nr), shop_bits, buf1, sizeof(buf1));
    send_to_char(g, ch, "Bits:       %s\r\n", buf1);
  }
}

void show_shops(GameWorld *g, struct char_data *ch, char *arg)
{
  shop_rnum shop_nr;

  if (!*arg)
    list_all_shops(g, ch);
  else {
    if (!strcasecmp(arg, ".")) {
      for (shop_nr = 0; shop_nr <= g->top_shop; shop_nr++)
        if (ok_shop_room(g, shop_nr, rroom_get_vnum(g, char_get_in_room(ch))))
          break;

      if (shop_nr > g->top_shop) {
        send_to_char(g, ch, "This isn't a shop!\r\n");
        return;
      }
    } else if (is_number(arg))
      shop_nr = atoi(arg) - 1;
    else
      shop_nr = -1;

    if (shop_nr < 0 || shop_nr > g->top_shop) {
      send_to_char(g, ch, "Illegal shop number.\r\n");
      return;
    }
    list_detailed_shop(g, ch, shop_nr);
  }
}

void destroy_shops(GameWorld *g)
{
  ssize_t cnt, itr;

  if (!g->shops)
    return;

  for (cnt = 0; cnt <= g->top_shop; cnt++) {
    if (g->shops[cnt].no_such_item1)
      free(g->shops[cnt].no_such_item1);
    if (g->shops[cnt].no_such_item2)
      free(g->shops[cnt].no_such_item2);
    if (g->shops[cnt].missing_cash1)
      free(g->shops[cnt].missing_cash1);
    if (g->shops[cnt].missing_cash2)
      free(g->shops[cnt].missing_cash2);
    if (g->shops[cnt].do_not_buy)
      free(g->shops[cnt].do_not_buy);
    if (g->shops[cnt].message_buy)
      free(g->shops[cnt].message_buy);
    if (g->shops[cnt].message_sell)
      free(g->shops[cnt].message_sell);
    if (g->shops[cnt].in_room)
      free(g->shops[cnt].in_room);
    if (g->shops[cnt].producing)
      free(g->shops[cnt].producing);

    if (g->shops[cnt].type) {
      for (itr = 0; shop_buy_get_type(&(g->shops[cnt].type[itr])) != NOTHING; itr++)
        if (shop_buy_get_keywords(&(g->shops[cnt].type[itr])))
          free(shop_buy_get_keywords(&(g->shops[cnt].type[itr])));
      free(g->shops[cnt].type);
    }
  }

  free(g->shops);
  g->shops = nullptr;
  g->top_shop   = -1;
}

int count_shops(GameWorld *g, shop_vnum low, shop_vnum high)
{
  shop_rnum i;
  int j;

  for (i = j = 0; i <= g->top_shop && rshop_get_vnum(g, i) <= high; i++) {
    if (rshop_get_vnum(g, i) >= low) {
      j++;
    }
  }

  return j;
}

static bool shop_trade_flagged(const struct shop_data *rec, int flag)
{
  return IS_SET_AR(rec->with_who, flag) != 0;
}

void shop_trade_set(struct shop_data *rec, int flag, bool on)
{
  if (on)
    SET_BIT_AR(rec->with_who, flag);
  else
    REMOVE_BIT_AR(rec->with_who, flag);
}

bool shop_trade_toggle(struct shop_data *rec, int flag)
{
  return (TOGGLE_BIT_AR(rec->with_who, flag) & Q_BIT(flag)) != 0;
}
