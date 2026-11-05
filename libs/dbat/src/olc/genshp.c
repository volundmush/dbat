/************************************************************************
 * Generic OLC Library - Shops / genshp.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/genshp.h"
#include "dbat/utils.h"
#include "dbat/db.h"

#include "dbat/genolc.h"
#include "dbat/genzon.h"
#include "dbat/atomicfile.h"
#include "dbat/game_world.h"

static void copy_list(IDXTYPE **tlist, IDXTYPE *flist);
static void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist);
static void free_shop_strings(struct shop_data *shop);
static void free_type_list(struct shop_buy_data **list);

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_shop(struct shop_data *tshop, struct shop_data *fshop, int free_old_strings)
{
  int i;

  /*
   * Copy basic information over.
   */
  shop_set_vnum(tshop, shop_get_vnum(fshop));
  shop_set_keeper(tshop, shop_get_keeper(fshop));
  shop_set_open1(tshop, shop_get_open1(fshop));
  shop_set_close1(tshop, shop_get_close1(fshop));
  shop_set_open2(tshop, shop_get_open2(fshop));
  shop_set_close2(tshop, shop_get_close2(fshop));
  shop_set_bank(tshop, shop_get_bank(fshop));
  shop_set_temper(tshop, shop_get_temper(fshop));
  shop_set_bitvector(tshop, shop_get_bitvector(fshop));
  for (i = 0; i < SW_ARRAY_MAX; i++)
    shop_get_with_who(tshop)
        [i] = shop_get_with_who(fshop)[i];
  shop_set_lastsort(tshop, shop_get_lastsort(fshop));
  shop_set_profit_buy(tshop, shop_get_profit_buy(fshop));
  shop_set_profit_sell(tshop, shop_get_profit_sell(fshop));
  shop_set_func(tshop, shop_get_func(fshop));

  /*
   * Copy lists over.
   */
  copy_list(&(tshop)->in_room, shop_get_rooms(fshop));
  copy_list(&(tshop)->producing, shop_get_products(fshop));
  copy_type_list(&(tshop->type), fshop->type);

  /*
   * Copy notification strings over.
   */
  if (free_old_strings)
    free_shop_strings(tshop);
  shop_set_no_such_item1(tshop, str_udup(shop_get_no_such_item1(fshop)));
  shop_set_no_such_item2(tshop, str_udup(shop_get_no_such_item2(fshop)));
  shop_set_missing_cash1(tshop, str_udup(shop_get_missing_cash1(fshop)));
  shop_set_missing_cash2(tshop, str_udup(shop_get_missing_cash2(fshop)));
  shop_set_do_not_buy(tshop, str_udup(shop_get_do_not_buy(fshop)));
  shop_set_message_buy(tshop, str_udup(shop_get_message_buy(fshop)));
  shop_set_message_sell(tshop, str_udup(shop_get_message_sell(fshop)));
}

/*-------------------------------------------------------------------*/

/*
 * Copy a 'NOTHING' terminated integer array list.
 */
static void copy_list(IDXTYPE **tlist, IDXTYPE *flist)
{
  int num_items, i;

  if (*tlist)
    free(*tlist);

  /*
   * Count number of entries. A shop still being built might have no list at all.
   */
  for (i = 0; flist && flist[i] != NOTHING; i++);
  num_items = i + 1;

  /*
   * Make space for entries.
   */
  CREATE(*tlist, IDXTYPE, num_items);

  /*
   * Copy entries over.
   */
  for (i = 0; i < num_items; i++)
    (*tlist)[i] = flist[i];
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated (in the type field) shop_buy_data 
 * array list.
 */
static void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist)
{
  int num_items, i;

  if (*tlist)
    free_type_list(tlist);

  /*
   * Count number of entries.
   */
  for (i = 0; shop_buy_get_type(&(flist[i])) != NOTHING; i++);
  num_items = i + 1;

  /*
   * Make space for entries.
   */
  CREATE(*tlist, struct shop_buy_data, num_items);

  /*
   * Copy entries over.
   */
  for (i = 0; i < num_items; i++) {
    (*tlist)[i].type = flist[i].type;
    if (shop_buy_get_keywords(&(flist[i])))
      shop_buy_set_keywords(&((*tlist)[i]), strdup(shop_buy_get_keywords(&(flist[i]))));
  }
}

/*-------------------------------------------------------------------*/

void remove_from_type_list(struct shop_buy_data **list, int num)
{
  int i, num_items;
  struct shop_buy_data *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i].type != NOTHING; i++);

  if (num < 0 || num >= i)
    return;
  num_items = i;

  CREATE(nlist, struct shop_buy_data, num_items);

  for (i = 0; i < num_items; i++)
    nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

  free(shop_buy_get_keywords(&((*list)[num])));
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl)
{
  int i, num_items;
  struct shop_buy_data *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i].type != NOTHING; i++);
  num_items = i;

  /*
   * Make a new list and slot in the new entry.
   */
  CREATE(nlist, struct shop_buy_data, num_items + 2);

  for (i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items]          = *newl;
  nlist[num_items + 1].type = NOTHING;

  /*
   * Out with the old, in with the new.
   */
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_int_list(IDXTYPE **list, IDXTYPE newi)
{
  IDXTYPE i, num_items, *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i] != NOTHING; i++);
  num_items = i;

  /*
   * Make a new list and slot in the new entry.
   */
  CREATE(nlist, IDXTYPE, num_items + 2);

  for (i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items]     = newi;
  nlist[num_items + 1] = NOTHING;

  /*
   * Out with the old, in with the new.
   */
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void remove_from_int_list(IDXTYPE **list, IDXTYPE num)
{
  IDXTYPE i, num_items, *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i] != NOTHING; i++);

  if (num < 0 || num >= i)
    return;
  num_items = i;

  CREATE(nlist, IDXTYPE, num_items);

  for (i = 0; i < num_items; i++)
    nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

/*
 * Free all the notice character strings in a shop structure.
 */
static void free_shop_strings(struct shop_data *shop)
{
  if (shop_get_no_such_item1(shop)) {
    free(shop_get_no_such_item1(shop));
    shop_set_no_such_item1(shop, nullptr);
  }
  if (shop_get_no_such_item2(shop)) {
    free(shop_get_no_such_item2(shop));
    shop_set_no_such_item2(shop, nullptr);
  }
  if (shop_get_missing_cash1(shop)) {
    free(shop_get_missing_cash1(shop));
    shop_set_missing_cash1(shop, nullptr);
  }
  if (shop_get_missing_cash2(shop)) {
    free(shop_get_missing_cash2(shop));
    shop_set_missing_cash2(shop, nullptr);
  }
  if (shop_get_do_not_buy(shop)) {
    free(shop_get_do_not_buy(shop));
    shop_set_do_not_buy(shop, nullptr);
  }
  if (shop_get_message_buy(shop)) {
    free(shop_get_message_buy(shop));
    shop_set_message_buy(shop, nullptr);
  }
  if (shop_get_message_sell(shop)) {
    free(shop_get_message_sell(shop));
    shop_set_message_sell(shop, nullptr);
  }
}

/*-------------------------------------------------------------------*/

/*
 * Free a type list and all the strings it contains.
 */
static void free_type_list(struct shop_buy_data **list)
{
  int i;

  for (i = 0; (*list)[i].type != NOTHING; i++)
    if (shop_buy_get_keywords(&((*list)[i])))
      free(shop_buy_get_keywords(&((*list)[i])));

  free(*list);
  *list = nullptr;
}

/*-------------------------------------------------------------------*/

/*
 * Free up the whole shop structure and it's content.
 */
void free_shop(struct shop_data *shop)
{
  free_shop_strings(shop);
  free_type_list(&(shop)->type);
  free(shop_get_rooms(shop));
  free(shop_get_products(shop));
  free(shop);
}

/*-------------------------------------------------------------------*/

/* returns the real number of the shop with given virtual number 
 *
 * We take so good care to keep it sorted - let's use it :) - Welcor
 */
static int shop_order(const void *key, const void *shop)
{
  shop_vnum k = *(const shop_vnum *)key, v = ((const struct shop_data *)shop)->vnum;

  return (k > v) - (k < v);
}

shop_rnum real_shop(GameWorld *g, shop_vnum vnum)
{
  struct shop_data *found;

  if (g->top_shop < 0)
    return NOWHERE;
  found = bsearch(&vnum, g->shops, g->top_shop + 1, sizeof(*g->shops), shop_order);
  return found ? found - g->shops : NOWHERE;
}

/*-------------------------------------------------------------------*/

/*
 * Generic string modifier for shop keeper messages. Returns NULL once stored, or why the message can't be used (the
 * old one is then kept).
 */
const char *modify_string(char **str, char *new_s, bool allow_int)
{

  char buf[MAX_STRING_LENGTH];
  char *pointer;

  /*
   * Check the '%s' is present, if not, add it.
   */
  if (*new_s != '%') {
    snprintf(buf, sizeof(buf), "%%s %s", new_s);
    pointer = buf;
  } else
    pointer = new_s;

  const char *problem = npc_msg_format_problem(pointer, allow_int);
  if (problem)
    return problem;

  if (*str)
    free(*str);
  *str = strdup(pointer);
  return nullptr;
}

/*-------------------------------------------------------------------*/

shop_rnum add_shop(GameWorld *g, struct shop_data *nshp)
{
  shop_rnum rshop;
  shop_rnum found = 0;
  zone_rnum rznum = real_zone_by_thing(g, shop_get_vnum(nshp));

  /*
   * The shop already exists, just update it.
   */
  if ((rshop = real_shop(g, shop_get_vnum(nshp))) != NOWHERE) {
    /* free old strings. They're not used in any other place -- Welcor */
    copy_shop(&g->shops[rshop], nshp, true);
    if (rznum != NOWHERE)
      add_to_save_list(g, g->zones[rznum].number, SL_SHP);
    else
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine shop zone.");
    return rshop;
  }

  g->top_shop++;
  RECREATE(g->shops, struct shop_data, g->top_shop + 1);

  for (rshop = g->top_shop; rshop > 0; rshop--) {
    if (nshp->vnum > rshop_get_vnum(g, rshop - 1)) {
      found = rshop;

      /* Make a "nofree" variant and remove these later. */
      g->shops[rshop].in_room   = nullptr;
      g->shops[rshop].producing = nullptr;
      g->shops[rshop].type      = nullptr;
      /* don't free old strings - they're still in use -- Welcor */
      copy_shop(&g->shops[rshop], nshp, false);
      break;
    }
    g->shops[rshop] = g->shops[rshop - 1];
  }

  if (!found) {
    /* Make a "nofree" variant and remove these later. */
    g->shops[rshop].in_room   = nullptr;
    g->shops[rshop].producing = nullptr;
    g->shops[rshop].type      = nullptr;
    /* don't free old strings - they're still in use -- Welcor */
    copy_shop(&g->shops[0], nshp, false);
  }

  if (rznum != NOWHERE)
    add_to_save_list(g, g->zones[rznum].number, SL_SHP);
  else
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine shop zone.");

  return rshop;
}

/*-------------------------------------------------------------------*/

bool save_shops(GameWorld *g, zone_rnum zone_num)
{
  IDXTYPE i;
  int j;
  shop_rnum rshop;
  struct atomic_file af;
  FILE *shop_file;
  char fname[128], oldname[128];
  struct shop_data *shop;

  if (zone_num < 0 || zone_num > g->top_zone) {
    mud_log("SYSERR: GenOLC: save_shops: Invalid real zone number %d. (0-%d)", zone_num, g->top_zone);
    return false;
  }

  snprintf(fname, sizeof(fname), "%s%d.shp", SHP_PREFIX, g->zones[zone_num].number);
  if (!(shop_file = atomic_open(&af, fname, false))) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot open shop file!");
    return false;
  }
  if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot write to shop file!");
    atomic_abort(&af);
    return false;
  }
  /*
   * Search database for shops in this zone.
   */
  for (i = genolc_zone_bottom(g, zone_num); i <= g->zones[zone_num].top; i++) {
    if ((rshop = real_shop(g, i)) != NOWHERE) {
      fprintf(shop_file, "#%d~\n", i);
      shop = g->shops + rshop;

      /*
       * Save the products.
       */
      for (j = 0; shop_get_product(shop, j) != NOTHING; j++)
        fprintf(shop_file, "%d\n", g->obj_index[shop_get_product(shop, j)].vnum);
      fprintf(shop_file, "-1\n");

      /*
       * Save the rates.
       */
      fprintf(shop_file, "%1.2f\n"
                         "%1.2f\n",
              shop_get_profit_buy(shop),
              shop_get_profit_sell(shop));

      /*
       * Save the buy types and namelists.
       */
      for (j = 0; shop_get_buy_type(shop, j) != NOTHING; j++)
        fprintf(shop_file, "%d%s\n",
                shop_get_buy_type(shop, j),
                shop_get_buy_word(shop, j) ? shop_get_buy_word(shop, j) : "");
      fprintf(shop_file, "-1\n");

      /* Not allowed to use Ascii in shopfile anymore (bpl21)
      sprintascii(buf1, shop_get_bitvector(shop));
      sprintascii(buf2, shop_get_with_who(shop));
*/

      /*
       * Save messages'n'stuff.
       * Added some small'n'silly defaults as sanity checks.
       */
      fprintf(shop_file,
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%d\n"
              "%d\n"
              "%d\n",
              shop_get_no_such_item1(shop) ? shop_get_no_such_item1(shop) : "%s Ke?!",
              shop_get_no_such_item2(shop) ? shop_get_no_such_item2(shop) : "%s Ke?!",
              shop_get_do_not_buy(shop) ? shop_get_do_not_buy(shop) : "%s Ke?!",
              shop_get_missing_cash1(shop) ? shop_get_missing_cash1(shop) : "%s Ke?!",
              shop_get_missing_cash2(shop) ? shop_get_missing_cash2(shop) : "%s Ke?!",
              shop_get_message_buy(shop) ? shop_get_message_buy(shop) : "%s Ke?! %d?",
              shop_get_message_sell(shop) ? shop_get_message_sell(shop) : "%s Ke?! %d?",
              shop_get_temper(shop),
              shop_get_bitvector(shop),
              shop_get_keeper(shop) == NOBODY ? -1 : g->mob_index[shop_get_keeper(shop)].vnum);
      for (j = 0; j < SW_ARRAY_MAX; j++)
        fprintf(shop_file, "%s%d", j ? " " : "", shop_get_with_who(shop)[j]);
      fprintf(shop_file, "\n");

      /*
       * Save the rooms.
       */
      for (j = 0; shop_get_room(shop, j) != NOWHERE; j++)
        fprintf(shop_file, "%d\n", shop_get_room(shop, j));
      fprintf(shop_file, "-1\n");

      /*
       * Save open/closing times 
       */
      fprintf(shop_file, "%d\n%d\n%d\n%d\n", shop_get_open1(shop), shop_get_close1(shop),
              shop_get_open2(shop), shop_get_close2(shop));
    }
  }
  fprintf(shop_file, "$~\n");
  if (!atomic_commit(&af))
    return false;
  snprintf(oldname, sizeof(oldname), "%s", fname);

  if (in_save_list(g, g->zones[zone_num].number, SL_SHP)) {
    remove_from_save_list(g, g->zones[zone_num].number, SL_SHP);
    create_world_index(g, g->zones[zone_num].number, "shp");
    mud_log("GenOLC: save_shops: Saving shops '%s'", oldname);
  }
  return true;
}
