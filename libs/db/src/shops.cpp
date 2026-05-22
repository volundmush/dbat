#include "dbat/db/shops.h"

struct shop_data *shop_index;
int top_shop = -1;

shop_rnum real_shop(shop_vnum vnum)
{
  shop_rnum bot, top, mid, last_top;

  if (top_shop < 0)
    return NOWHERE;

  bot = 0;
  top = top_shop;

  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

    if (shop_index[mid].vnum == vnum)
      return mid;
    if (bot >= top)
      return NOWHERE;
    if (shop_index[mid].vnum > vnum)
      top = mid;
    else
      bot = mid + 1;

    if (top > last_top)
      return NOWHERE;
  }
}

struct shop_data *shop_by_id(shop_vnum vnum)
{
  shop_rnum rnum = real_shop(vnum);

  if (rnum == NOWHERE || !shop_index)
    return nullptr;

  return &shop_index[rnum];
}
