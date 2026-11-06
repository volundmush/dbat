#pragma once
#include "dbat/shop_utils.h"
#include "dbat/consts/types.h"
#include "shop.h"

#ifdef __cplusplus
extern "C" {
#endif

void copy_shop(struct shop_data *tshop, struct shop_data *fshop, int free_old_strings);
void remove_from_type_list(struct shop_buy_data **list, int num);
void remove_from_int_list(IDXTYPE **list, IDXTYPE num);
void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl);
void add_to_int_list(IDXTYPE **tlist, IDXTYPE newi);
void free_shop(struct shop_data *shop);
const char *modify_string(char **str, char *newstr, bool allow_int);
shop_rnum add_shop(GameWorld *g, struct shop_data *shop);
bool save_shops(GameWorld *g, zone_rnum zone_num);
shop_rnum real_shop(GameWorld *g, shop_vnum vnum);

/*
 * Handy macros.
 */

#ifdef __cplusplus
}
#endif
