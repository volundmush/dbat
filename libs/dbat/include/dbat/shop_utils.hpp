#pragma once
#include "dbat/consts/types.hpp"

struct shop_data;
struct shop_buy_data;

/* Field accessors for shops. */
room_vnum shop_get_vnum(const struct shop_data *shop);
void shop_set_vnum(struct shop_data *shop, room_vnum v);
mob_rnum shop_get_keeper(const struct shop_data *shop);
void shop_set_keeper(struct shop_data *shop, mob_rnum v);
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
room_vnum *shop_get_rooms(struct shop_data *shop);
obj_vnum *shop_get_products(struct shop_data *shop);
void shop_set_products(struct shop_data *shop, obj_vnum *v);
void shop_set_rooms(struct shop_data *shop, room_vnum *v);
void shop_set_buy_types(struct shop_data *shop, struct shop_buy_data *v);
struct shop_buy_data *shop_get_buy_types(struct shop_data *shop);
bitvector_t *shop_get_with_who(struct shop_data *shop);
char *shop_get_no_such_item1(const struct shop_data *shop);
void shop_set_no_such_item1(struct shop_data *shop, char *v);
char *shop_get_no_such_item2(const struct shop_data *shop);
void shop_set_no_such_item2(struct shop_data *shop, char *v);
char *shop_get_missing_cash1(const struct shop_data *shop);
void shop_set_missing_cash1(struct shop_data *shop, char *v);
char *shop_get_missing_cash2(const struct shop_data *shop);
void shop_set_missing_cash2(struct shop_data *shop, char *v);
char *shop_get_do_not_buy(const struct shop_data *shop);
void shop_set_do_not_buy(struct shop_data *shop, char *v);
char *shop_get_message_buy(const struct shop_data *shop);
void shop_set_message_buy(struct shop_data *shop, char *v);
char *shop_get_message_sell(const struct shop_data *shop);
void shop_set_message_sell(struct shop_data *shop, char *v);
room_vnum shop_get_room(const struct shop_data *shop, int n);
void shop_set_room(struct shop_data *shop, int n, room_vnum v);
obj_vnum shop_get_product(const struct shop_data *shop, int n);
void shop_set_product(struct shop_data *shop, int n, obj_vnum v);
int shop_get_buy_type(const struct shop_data *shop, int n);
void shop_set_buy_type(struct shop_data *shop, int n, int v);
char *shop_get_buy_word(const struct shop_data *shop, int n);
void shop_set_buy_word(struct shop_data *shop, int n, char *v);
bool shop_uses_bank(const struct shop_data *shop);
bool shop_allows_steal(const struct shop_data *shop);
int shop_buy_get_type(const struct shop_buy_data *buy);
void shop_buy_set_type(struct shop_buy_data *buy, int v);
char *shop_buy_get_keywords(const struct shop_buy_data *buy);
void shop_buy_set_keywords(struct shop_buy_data *buy, char *v);

