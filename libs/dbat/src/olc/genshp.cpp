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
#include "dbat/comm.hpp"
#include "dbat/game_world.hpp"
#include "dbat/format.hpp"

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

/* Stores a builder's shopkeeper or guildmaster message, "%s " put before it if it doesn't start with a code, or tells
   d why it won't do (the old one then stays) */
void set_npc_message(GameWorld *g, struct descriptor_data *d, std::string &message, std::string_view text, bool allow_int)
{
  std::string wanted = text.starts_with('%') ? std::string(text) : str_format("%s {}", text);
  if (std::optional<std::string> problem = npc_msg_format_problem(wanted.c_str(), allow_int))
    write_to_output(g, d,
                    "Invalid message: {}. It starts with %s, the customer's name (added if you leave it off){}, and can have no other % codes.\r\n",
                    *problem, allow_int ? ", may then use one %d for the price" : "");
  else
    message = std::move(wanted);
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
