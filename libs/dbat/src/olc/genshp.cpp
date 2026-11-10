/************************************************************************
 * Generic OLC Library - Shops / genshp.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/genshp.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_shop(struct shop_data *tshop, const struct shop_data *fshop)
{
  *tshop = *fshop;
}

/*-------------------------------------------------------------------*/

/* The shop numbered vnum, or null. */
struct shop_data *find_shop(GameWorld *g, shop_vnum vnum)
{
  return g->shops.find(vnum);
}

/*-------------------------------------------------------------------*/

/*
 * Generic string modifier for shop keeper messages. Returns NULL once stored, or why the message can't be used (the
 * old one is then kept).
 */
const char *modify_string(std::string *str, const char *new_s, bool allow_int)
{

  char buf[MAX_STRING_LENGTH];
  const char *pointer;

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

  *str = pointer;
  return nullptr;
}

/*-------------------------------------------------------------------*/

struct shop_data *add_shop(GameWorld *g, struct shop_data *nshp)
{
  struct zone_data *rznum = find_zone_by_thing(g, shop_get_vnum(nshp));
  struct shop_data *shop = find_shop(g, shop_get_vnum(nshp));

  if (!shop) /* Otherwise the shop already exists: just update it. */
    shop = g->shops.emplace(shop_get_vnum(nshp));
  copy_shop(shop, nshp);

  if (rznum != nullptr)
    add_to_save_list(g, rznum->number, SL_SHP);
  else
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine shop zone.");

  return shop;
}

/*-------------------------------------------------------------------*/

bool save_shops(GameWorld *g, struct zone_data *zone_num)
{
  int j;
  struct atomic_file af;
  FILE *shop_file;
  std::string fname;
  std::string oldname;
  struct shop_data *shop;

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  fname = str_format("{}{:d}.shp", SHP_PREFIX, zone_num->number);
  if (!(shop_file = atomic_open(&af, fname.c_str(), false))) {
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
  for (shop_vnum i : g->shops.vnums_in(genolc_zone_bottom(g, zone_num), zone_num->top)) {
    if ((shop = find_shop(g, i)) != nullptr) {
      fprintf(shop_file, "#%d~\n", i);

      /*
       * Save the products.
       */
      for (obj_vnum product : shop_products(shop))
        fprintf(shop_file, "%d\n", product);
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
      for (const shop_buy_data &buy : shop_buy_types(shop))
        fprintf(shop_file, "%d%s\n", buy.type, buy.keywords.c_str());
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
              (int)shop_get_bitvector(shop), /* as signed, as always */
              shop_get_keeper(shop) == NOBODY ? -1 : mob_index_of(g, shop_get_keeper(shop))->vnum);
      for (j = 0; j < SW_ARRAY_MAX; j++)
        fprintf(shop_file, "%s%d", j ? " " : "", (int)shop_get_with_who(shop)[j]);
      fprintf(shop_file, "\n");

      /*
       * Save the rooms.
       */
      for (room_vnum room : shop_rooms(shop))
        fprintf(shop_file, "%d\n", room);
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
  oldname = fname;

  if (in_save_list(g, zone_num->number, SL_SHP)) {
    remove_from_save_list(g, zone_num->number, SL_SHP);
    create_world_index(g, zone_num->number, "shp");
    mud_log("GenOLC: save_shops: Saving shops '{}'", oldname);
  }
  return true;
}
