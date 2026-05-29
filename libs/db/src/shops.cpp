#include "dbat/db/shops.h"

shop_vnum real_shop(shop_vnum vnum)
{
  return shop_by_id(vnum) ? vnum : NOTHING;
}
