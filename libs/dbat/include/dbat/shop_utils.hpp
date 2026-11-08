#pragma once
#include <string>
#include <vector>

#include "dbat/consts/types.hpp"

struct shop_data;
struct shop_buy_data;

/* Field accessors for shops. */
room_vnum shop_get_vnum(const struct shop_data *shop);
void shop_set_vnum(struct shop_data *shop, room_vnum v);
mob_vnum shop_get_keeper(const struct shop_data *shop);
void shop_set_keeper(struct shop_data *shop, mob_vnum v);
int shop_get_open1(const struct shop_data *shop);
void shop_set_open1(struct shop_data *shop, int v);
int shop_get_close1(const struct shop_data *shop);
void shop_set_close1(struct shop_data *shop, int v);
int shop_get_open2(const struct shop_data *shop);
void shop_set_open2(struct shop_data *shop, int v);
int shop_get_close2(const struct shop_data *shop);
void shop_set_close2(struct shop_data *shop, int v);
int shop_get_bank(const struct shop_data *shop);
void shop_set_bank(struct shop_data *shop, int v);
int shop_get_temper(const struct shop_data *shop);
void shop_set_temper(struct shop_data *shop, int v);
bitvector_t shop_get_bitvector(const struct shop_data *shop);
void shop_set_bitvector(struct shop_data *shop, bitvector_t v);
int shop_get_lastsort(const struct shop_data *shop);
void shop_set_lastsort(struct shop_data *shop, int v);
float shop_get_profit_buy(const struct shop_data *shop);
void shop_set_profit_buy(struct shop_data *shop, float v);
float shop_get_profit_sell(const struct shop_data *shop);
void shop_set_profit_sell(struct shop_data *shop, float v);
spec_proc_func shop_get_func(const struct shop_data *shop);
void shop_set_func(struct shop_data *shop, spec_proc_func v);
bitvector_t *shop_get_with_who(struct shop_data *shop);
/* What the shop sells, where it is, and what it buys, in file order. */
std::vector<obj_vnum> &shop_products(struct shop_data *shop);
const std::vector<obj_vnum> &shop_products(const struct shop_data *shop);
std::vector<room_vnum> &shop_rooms(struct shop_data *shop);
const std::vector<room_vnum> &shop_rooms(const struct shop_data *shop);
std::vector<shop_buy_data> &shop_buy_types(struct shop_data *shop);
const std::vector<shop_buy_data> &shop_buy_types(const struct shop_data *shop);
const char *shop_get_no_such_item1(const struct shop_data *shop);
void shop_set_no_such_item1(struct shop_data *shop, const char *v);
const char *shop_get_no_such_item2(const struct shop_data *shop);
void shop_set_no_such_item2(struct shop_data *shop, const char *v);
const char *shop_get_missing_cash1(const struct shop_data *shop);
void shop_set_missing_cash1(struct shop_data *shop, const char *v);
const char *shop_get_missing_cash2(const struct shop_data *shop);
void shop_set_missing_cash2(struct shop_data *shop, const char *v);
const char *shop_get_do_not_buy(const struct shop_data *shop);
void shop_set_do_not_buy(struct shop_data *shop, const char *v);
const char *shop_get_message_buy(const struct shop_data *shop);
void shop_set_message_buy(struct shop_data *shop, const char *v);
const char *shop_get_message_sell(const struct shop_data *shop);
void shop_set_message_sell(struct shop_data *shop, const char *v);
bool shop_uses_bank(const struct shop_data *shop);
bool shop_allows_steal(const struct shop_data *shop);
int shop_buy_get_type(const struct shop_buy_data *buy);
void shop_buy_set_type(struct shop_buy_data *buy, int v);
const char *shop_buy_get_keywords(const struct shop_buy_data *buy);
void shop_buy_set_keywords(struct shop_buy_data *buy, const char *v);
