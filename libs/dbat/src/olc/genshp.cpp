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

  std::string buf;
  const char *pointer;

  /*
   * Check the '%s' is present, if not, add it.
   */
  if (*new_s != '%') {
    buf     = str_format("%s {}", new_s);
    pointer = buf.c_str();
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
