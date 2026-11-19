#pragma once
#include <string>
#include <string_view>

#include "dbat/shop_utils.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/shop.hpp"

void copy_shop(struct shop_data *tshop, const struct shop_data *fshop);
/* Stores a builder's shopkeeper or guildmaster message, "%s " put before it if it doesn't start with a code, or tells
   d why it won't do (the old one then stays) */
void set_npc_message(GameWorld *g, struct descriptor_data *d, std::string &message, std::string_view text, bool allow_int);
struct shop_data *add_shop(GameWorld *g, struct shop_data *shop);
bool save_shops(GameWorld *g, struct zone_data *zone_num);
struct shop_data *find_shop(GameWorld *g, shop_vnum vnum);

/*
 * Handy macros.
 */

