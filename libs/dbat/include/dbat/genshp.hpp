#pragma once
#include "dbat/shop_utils.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/shop.hpp"

void copy_shop(struct shop_data *tshop, const struct shop_data *fshop);
const char *modify_string(std::string *str, const char *newstr, bool allow_int);
struct shop_data *add_shop(GameWorld *g, struct shop_data *shop);
bool save_shops(GameWorld *g, struct zone_data *zone_num);
struct shop_data *find_shop(GameWorld *g, shop_vnum vnum);

/*
 * Handy macros.
 */

