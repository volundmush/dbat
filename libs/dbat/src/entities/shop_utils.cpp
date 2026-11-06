#include "dbat/shop_utils.h"
#include "dbat/shop.h"
#include "dbat/flags.h"
#include "dbat/game_world.h"

room_vnum shop_get_vnum(const struct shop_data *shop)
{
  return shop->vnum;
}

void shop_set_vnum(struct shop_data *shop, room_vnum v)
{
  shop->vnum = v;
}

mob_rnum shop_get_keeper(const struct shop_data *shop)
{
  return shop->keeper;
}

void shop_set_keeper(struct shop_data *shop, mob_rnum v)
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

room_vnum *shop_get_rooms(struct shop_data *shop)
{
  return shop->in_room;
}

obj_vnum *shop_get_products(struct shop_data *shop)
{
  return shop->producing;
}

struct shop_buy_data *shop_get_buy_types(struct shop_data *shop)
{
  return shop->type;
}

bitvector_t *shop_get_with_who(struct shop_data *shop)
{
  return shop->with_who;
}

char *shop_get_no_such_item1(const struct shop_data *shop)
{
  return shop->no_such_item1;
}

void shop_set_no_such_item1(struct shop_data *shop, char *v)
{
  shop->no_such_item1 = v;
}

char *shop_get_no_such_item2(const struct shop_data *shop)
{
  return shop->no_such_item2;
}

void shop_set_no_such_item2(struct shop_data *shop, char *v)
{
  shop->no_such_item2 = v;
}

char *shop_get_missing_cash1(const struct shop_data *shop)
{
  return shop->missing_cash1;
}

void shop_set_missing_cash1(struct shop_data *shop, char *v)
{
  shop->missing_cash1 = v;
}

char *shop_get_missing_cash2(const struct shop_data *shop)
{
  return shop->missing_cash2;
}

void shop_set_missing_cash2(struct shop_data *shop, char *v)
{
  shop->missing_cash2 = v;
}

char *shop_get_do_not_buy(const struct shop_data *shop)
{
  return shop->do_not_buy;
}

void shop_set_do_not_buy(struct shop_data *shop, char *v)
{
  shop->do_not_buy = v;
}

char *shop_get_message_buy(const struct shop_data *shop)
{
  return shop->message_buy;
}

void shop_set_message_buy(struct shop_data *shop, char *v)
{
  shop->message_buy = v;
}

char *shop_get_message_sell(const struct shop_data *shop)
{
  return shop->message_sell;
}

void shop_set_message_sell(struct shop_data *shop, char *v)
{
  shop->message_sell = v;
}

room_vnum shop_get_room(const struct shop_data *shop, int n)
{
  return shop->in_room[n];
}

void shop_set_room(struct shop_data *shop, int n, room_vnum v)
{
  shop->in_room[n] = v;
}

obj_vnum shop_get_product(const struct shop_data *shop, int n)
{
  return shop->producing[n];
}

void shop_set_product(struct shop_data *shop, int n, obj_vnum v)
{
  shop->producing[n] = v;
}

int shop_get_buy_type(const struct shop_data *shop, int n)
{
  return shop->type[n].type;
}

void shop_set_buy_type(struct shop_data *shop, int n, int v)
{
  shop->type[n].type = v;
}

char *shop_get_buy_word(const struct shop_data *shop, int n)
{
  return shop->type[n].keywords;
}

room_vnum rshop_get_vnum(GameWorld *g, shop_rnum i)
{
  return g->shops[i].vnum;
}

void rshop_set_vnum(GameWorld *g, shop_rnum i, room_vnum v)
{
  g->shops[i].vnum = v;
}

mob_rnum rshop_get_keeper(GameWorld *g, shop_rnum i)
{
  return g->shops[i].keeper;
}

void rshop_set_keeper(GameWorld *g, shop_rnum i, mob_rnum v)
{
  g->shops[i].keeper = v;
}

int rshop_get_open1(GameWorld *g, shop_rnum i)
{
  return g->shops[i].open1;
}

int rshop_get_close1(GameWorld *g, shop_rnum i)
{
  return g->shops[i].close1;
}

int rshop_get_open2(GameWorld *g, shop_rnum i)
{
  return g->shops[i].open2;
}

int rshop_get_close2(GameWorld *g, shop_rnum i)
{
  return g->shops[i].close2;
}

int rshop_get_bank(GameWorld *g, shop_rnum i)
{
  return g->shops[i].bankAccount;
}

void rshop_set_bank(GameWorld *g, shop_rnum i, int v)
{
  g->shops[i].bankAccount = v;
}

int rshop_get_temper(GameWorld *g, shop_rnum i)
{
  return g->shops[i].temper1;
}

bitvector_t rshop_get_bitvector(GameWorld *g, shop_rnum i)
{
  return g->shops[i].bitvector;
}

bitvector_t *rshop_get_with_who(GameWorld *g, shop_rnum i)
{
  return g->shops[i].with_who;
}

int rshop_get_lastsort(GameWorld *g, shop_rnum i)
{
  return g->shops[i].lastsort;
}

void rshop_set_lastsort(GameWorld *g, shop_rnum i, int v)
{
  g->shops[i].lastsort = v;
}

float rshop_get_profit_buy(GameWorld *g, shop_rnum i)
{
  return g->shops[i].profit_buy;
}

float rshop_get_profit_sell(GameWorld *g, shop_rnum i)
{
  return g->shops[i].profit_sell;
}

spec_proc_func rshop_get_func(GameWorld *g, shop_rnum i)
{
  return g->shops[i].func;
}

void rshop_set_func(GameWorld *g, shop_rnum i, spec_proc_func v)
{
  g->shops[i].func = v;
}

room_vnum rshop_get_room(GameWorld *g, shop_rnum i, int n)
{
  return g->shops[i].in_room[n];
}

void rshop_set_room(GameWorld *g, shop_rnum i, int n, room_vnum v)
{
  g->shops[i].in_room[n] = v;
}

obj_vnum rshop_get_product(GameWorld *g, shop_rnum i, int n)
{
  return g->shops[i].producing[n];
}

void rshop_set_product(GameWorld *g, shop_rnum i, int n, obj_vnum v)
{
  g->shops[i].producing[n] = v;
}

int rshop_get_buy_type(GameWorld *g, shop_rnum i, int n)
{
  return g->shops[i].type[n].type;
}

void rshop_set_buy_type(GameWorld *g, shop_rnum i, int n, int v)
{
  g->shops[i].type[n].type = v;
}

char *rshop_get_buy_word(GameWorld *g, shop_rnum i, int n)
{
  return g->shops[i].type[n].keywords;
}

void rshop_set_buy_word(GameWorld *g, shop_rnum i, int n, char *v)
{
  g->shops[i].type[n].keywords = v;
}

bool rshop_uses_bank(GameWorld *g, shop_rnum i)
{
  return IS_SET(g->shops[i].bitvector, WILL_BANK_MONEY);
}

bool rshop_allows_steal(GameWorld *g, shop_rnum i)
{
  return IS_SET(g->shops[i].bitvector, WILL_ALLOW_STEAL);
}

int shop_buy_get_type(const struct shop_buy_data *buy)
{
  return buy->type;
}

void shop_buy_set_type(struct shop_buy_data *buy, int v)
{
  buy->type = v;
}

char *shop_buy_get_keywords(const struct shop_buy_data *buy)
{
  return buy->keywords;
}

void shop_buy_set_keywords(struct shop_buy_data *buy, char *v)
{
  buy->keywords = v;
}

void shop_set_products(struct shop_data *shop, obj_vnum *v)
{
  shop->producing = v;
}

void shop_set_rooms(struct shop_data *shop, room_vnum *v)
{
  shop->in_room = v;
}

void shop_set_buy_types(struct shop_data *shop, struct shop_buy_data *v)
{
  shop->type = v;
}
