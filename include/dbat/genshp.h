/************************************************************************
 * Generic OLC Library - Shops / genshp.h			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once

#include "structs.h"
#include "shop.h"

extern void copy_shop(struct Shop *tshop, struct Shop *fshop, int free_old_strings);

extern void copy_list(vnum **tlist, vnum *flist);

extern void remove_from_int_list(vnum **list, vnum num);

extern void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl);

extern void add_to_int_list(vnum **tlist, vnum newi);

extern void free_shop_string(struct Shop *shop);

extern void free_shop(struct Shop *shop);

extern void free_shop_strings(struct Shop *shop);

extern void modify_string(char **str, char *newstr);

extern int add_shop(struct Shop *shop);

extern int save_shops(zone_rnum zone_num);

extern shop_rnum real_shop(shop_vnum vnum);

/*
 * Handy macros.
 */
#define S_NUM(i)        ((i)->vnum)
#define S_KEEPER(i)        ((i)->keeper)
#define S_OPEN1(i)        ((i)->open1)
#define S_CLOSE1(i)        ((i)->close1)
#define S_OPEN2(i)        ((i)->open2)
#define S_CLOSE2(i)        ((i)->close2)
#define S_BANK(i)        ((i)->bankAccount)
#define S_BROKE_TEMPER(i)    ((i)->temper1)
#define S_BITVECTOR(i)        ((i)->bitvector)
#define S_NOTRADE(i)        ((i)->with_who)
#define S_SORT(i)        ((i)->lastsort)
#define S_BUYPROFIT(i)        ((i)->profit_buy)
#define S_SELLPROFIT(i)        ((i)->profit_sell)
#define S_FUNC(i)        ((i)->func)

#define S_ROOMS(i)        ((i)->in_room)
#define S_PRODUCTS(i)        ((i)->producing)
#define S_NAMELISTS(i)        ((i)->type)
#define S_ROOM(i, num)        ((i)->in_room[(num)])
#define S_PRODUCT(i, num)    ((i)->producing[(num)])
#define S_BUYTYPE(i, num)    (BUY_TYPE((i)->type[(num)]))
#define S_BUYWORD(i, num)    (BUY_WORD((i)->type[(num)]))

#define S_NOITEM1(i)        ((i)->no_such_item1)
#define S_NOITEM2(i)        ((i)->no_such_item2)
#define S_NOCASH1(i)        ((i)->missing_cash1)
#define S_NOCASH2(i)        ((i)->missing_cash2)
#define S_NOBUY(i)        ((i)->do_not_buy)
#define S_BUY(i)        ((i)->message_buy)
#define S_SELL(i)        ((i)->message_sell)
