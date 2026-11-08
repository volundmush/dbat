#include "dbat/shop_utils.hpp"
#include "dbat/shop.hpp"
#include "dbat/flags.hpp"
#include "dbat/game_world.hpp"
#include "dbat/stringutils.hpp"

room_vnum shop_get_vnum(const struct shop_data *shop)
{
  return shop->vnum;
}

void shop_set_vnum(struct shop_data *shop, room_vnum v)
{
  shop->vnum = v;
}

mob_vnum shop_get_keeper(const struct shop_data *shop)
{
  return shop->keeper;
}

void shop_set_keeper(struct shop_data *shop, mob_vnum v)
{
  shop->keeper = v;
}

int shop_get_open1(const struct shop_data *shop)
{
  return shop->open1;
}

void shop_set_open1(struct shop_data *shop, int v)
{
  shop->open1 = v;
}

int shop_get_close1(const struct shop_data *shop)
{
  return shop->close1;
}

void shop_set_close1(struct shop_data *shop, int v)
{
  shop->close1 = v;
}

int shop_get_open2(const struct shop_data *shop)
{
  return shop->open2;
}

void shop_set_open2(struct shop_data *shop, int v)
{
  shop->open2 = v;
}

int shop_get_close2(const struct shop_data *shop)
{
  return shop->close2;
}

void shop_set_close2(struct shop_data *shop, int v)
{
  shop->close2 = v;
}

int shop_get_bank(const struct shop_data *shop)
{
  return shop->bankAccount;
}

void shop_set_bank(struct shop_data *shop, int v)
{
  shop->bankAccount = v;
}

int shop_get_temper(const struct shop_data *shop)
{
  return shop->temper1;
}

void shop_set_temper(struct shop_data *shop, int v)
{
  shop->temper1 = v;
}

bitvector_t shop_get_bitvector(const struct shop_data *shop)
{
  return shop->bitvector;
}

void shop_set_bitvector(struct shop_data *shop, bitvector_t v)
{
  shop->bitvector = v;
}

int shop_get_lastsort(const struct shop_data *shop)
{
  return shop->lastsort;
}

void shop_set_lastsort(struct shop_data *shop, int v)
{
  shop->lastsort = v;
}

float shop_get_profit_buy(const struct shop_data *shop)
{
  return shop->profit_buy;
}

void shop_set_profit_buy(struct shop_data *shop, float v)
{
  shop->profit_buy = v;
}

float shop_get_profit_sell(const struct shop_data *shop)
{
  return shop->profit_sell;
}

void shop_set_profit_sell(struct shop_data *shop, float v)
{
  shop->profit_sell = v;
}

spec_proc_func shop_get_func(const struct shop_data *shop)
{
  return shop->func;
}

void shop_set_func(struct shop_data *shop, spec_proc_func v)
{
  shop->func = v;
}

bitvector_t *shop_get_with_who(struct shop_data *shop)
{
  return shop->with_who;
}

std::vector<obj_vnum> &shop_products(struct shop_data *shop)
{
  return shop->producing;
}

const std::vector<obj_vnum> &shop_products(const struct shop_data *shop)
{
  return shop->producing;
}

std::vector<room_vnum> &shop_rooms(struct shop_data *shop)
{
  return shop->in_room;
}

const std::vector<room_vnum> &shop_rooms(const struct shop_data *shop)
{
  return shop->in_room;
}

std::vector<shop_buy_data> &shop_buy_types(struct shop_data *shop)
{
  return shop->type;
}

const std::vector<shop_buy_data> &shop_buy_types(const struct shop_data *shop)
{
  return shop->type;
}

const char *shop_get_no_such_item1(const struct shop_data *shop)
{
  return c_str_or_null(shop->no_such_item1);
}

void shop_set_no_such_item1(struct shop_data *shop, const char *v)
{
  shop->no_such_item1 = v ? v : "";
}

const char *shop_get_no_such_item2(const struct shop_data *shop)
{
  return c_str_or_null(shop->no_such_item2);
}

void shop_set_no_such_item2(struct shop_data *shop, const char *v)
{
  shop->no_such_item2 = v ? v : "";
}

const char *shop_get_missing_cash1(const struct shop_data *shop)
{
  return c_str_or_null(shop->missing_cash1);
}

void shop_set_missing_cash1(struct shop_data *shop, const char *v)
{
  shop->missing_cash1 = v ? v : "";
}

const char *shop_get_missing_cash2(const struct shop_data *shop)
{
  return c_str_or_null(shop->missing_cash2);
}

void shop_set_missing_cash2(struct shop_data *shop, const char *v)
{
  shop->missing_cash2 = v ? v : "";
}

const char *shop_get_do_not_buy(const struct shop_data *shop)
{
  return c_str_or_null(shop->do_not_buy);
}

void shop_set_do_not_buy(struct shop_data *shop, const char *v)
{
  shop->do_not_buy = v ? v : "";
}

const char *shop_get_message_buy(const struct shop_data *shop)
{
  return c_str_or_null(shop->message_buy);
}

void shop_set_message_buy(struct shop_data *shop, const char *v)
{
  shop->message_buy = v ? v : "";
}

const char *shop_get_message_sell(const struct shop_data *shop)
{
  return c_str_or_null(shop->message_sell);
}

void shop_set_message_sell(struct shop_data *shop, const char *v)
{
  shop->message_sell = v ? v : "";
}

int shop_buy_get_type(const struct shop_buy_data *buy)
{
  return buy->type;
}

void shop_buy_set_type(struct shop_buy_data *buy, int v)
{
  buy->type = v;
}

const char *shop_buy_get_keywords(const struct shop_buy_data *buy)
{
  return c_str_or_null(buy->keywords);
}

void shop_buy_set_keywords(struct shop_buy_data *buy, const char *v)
{
  buy->keywords = v ? v : "";
}

bool shop_uses_bank(const struct shop_data *shop)
{
  return IS_SET(shop->bitvector, WILL_BANK_MONEY);
}

bool shop_allows_steal(const struct shop_data *shop)
{
  return IS_SET(shop->bitvector, WILL_ALLOW_STEAL);
}
