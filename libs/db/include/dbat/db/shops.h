#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shop_buy_data {
   int type;
   char *keywords;
};

#define SW_ARRAY_MAX	4

struct shop_data {
   room_vnum vnum;		/* Virtual number of this shop		*/
   obj_vnum *producing;		/* Which item to produce (virtual)	*/
   float profit_buy;		/* Factor to multiply cost with		*/
   float profit_sell;		/* Factor to multiply cost with		*/
   struct shop_buy_data *type;	/* Which items to trade			*/
   char	*no_such_item1;		/* Message if keeper hasn't got an item	*/
   char	*no_such_item2;		/* Message if player hasn't got an item	*/
   char	*missing_cash1;		/* Message if keeper hasn't got cash	*/
   char	*missing_cash2;		/* Message if player hasn't got cash	*/
   char	*do_not_buy;		/* If keeper dosn't buy such things	*/
   char	*message_buy;		/* Message when player buys item	*/
   char	*message_sell;		/* Message when player sells item	*/
   int	 temper1;		/* How does keeper react if no money	*/
   bitvector_t	 bitvector;	/* Can attack? Use bank? Cast here?	*/
   mob_rnum	 keeper;	/* The mobile who owns the shop (rnum)	*/
   bitvector_t	 with_who[SW_ARRAY_MAX];/* Who does the shop trade with?	*/
   room_vnum *in_room;		/* Where is the shop?			*/
   int	 open1, open2;		/* When does the shop open?		*/
   int	 close1, close2;	/* When does the shop close?		*/
   int	 bankAccount;		/* Store all gold over 15000 (disabled)	*/
   int	 lastsort;		/* How many items are sorted in inven?	*/
   SpecialFunc func;		/* Secondary spec_proc for shopkeeper	*/
};

// Shop API functions, implemented in shops_api.zig
shop_vnum shop_id_get(struct shop_data *shop);
void shop_id_set(struct shop_data *shop, shop_vnum id);
float shop_profit_buy_get(struct shop_data *shop);
void shop_profit_buy_set(struct shop_data *shop, float value);
float shop_profit_sell_get(struct shop_data *shop);
void shop_profit_sell_set(struct shop_data *shop, float value);
const char *shop_no_such_item1_get(struct shop_data *shop);
void shop_no_such_item1_set(struct shop_data *shop, const char *value);
const char *shop_no_such_item2_get(struct shop_data *shop);
void shop_no_such_item2_set(struct shop_data *shop, const char *value);
const char *shop_missing_cash1_get(struct shop_data *shop);
void shop_missing_cash1_set(struct shop_data *shop, const char *value);
const char *shop_missing_cash2_get(struct shop_data *shop);
void shop_missing_cash2_set(struct shop_data *shop, const char *value);
const char *shop_do_not_buy_get(struct shop_data *shop);
void shop_do_not_buy_set(struct shop_data *shop, const char *value);
const char *shop_message_buy_get(struct shop_data *shop);
void shop_message_buy_set(struct shop_data *shop, const char *value);
const char *shop_message_sell_get(struct shop_data *shop);
void shop_message_sell_set(struct shop_data *shop, const char *value);
int shop_temper_get(struct shop_data *shop);
void shop_temper_set(struct shop_data *shop, int temper);
bool shop_flagged(struct shop_data *shop, int pos);
bool shop_flag_toggle(struct shop_data *shop, int pos);
void shop_flag_set(struct shop_data *shop, int pos, bool value);
mob_vnum shop_keeper_get(struct shop_data *shop);
void shop_keeper_set(struct shop_data *shop, mob_vnum vnum);
bool shop_trade_flagged(struct shop_data *shop, int pos);
bool shop_trade_flag_toggle(struct shop_data *shop, int pos);
void shop_trade_flag_set(struct shop_data *shop, int pos, bool value);
int shop_open1_get(struct shop_data *shop);
void shop_open1_set(struct shop_data *shop, int value);
int shop_open2_get(struct shop_data *shop);
void shop_open2_set(struct shop_data *shop, int value);
int shop_close1_get(struct shop_data *shop);
void shop_close1_set(struct shop_data *shop, int value);
int shop_close2_get(struct shop_data *shop);
void shop_close2_set(struct shop_data *shop, int value);
int shop_bank_get(struct shop_data *shop);
void shop_bank_set(struct shop_data *shop, int value);
int shop_lastsort_get(struct shop_data *shop);
void shop_lastsort_set(struct shop_data *shop, int value);
SpecialFunc shop_func_get(struct shop_data *shop);
void shop_func_set(struct shop_data *shop, SpecialFunc func);
obj_vnum shop_product_get(struct shop_data *shop, size_t index);
void shop_product_set(struct shop_data *shop, size_t index, obj_vnum vnum);
room_vnum shop_room_get(struct shop_data *shop, size_t index);
void shop_room_set(struct shop_data *shop, size_t index, room_vnum vnum);
struct shop_buy_data *shop_buy_type_get(struct shop_data *shop, size_t index);

int shop_buy_data_type_get(struct shop_buy_data *data);
void shop_buy_data_type_set(struct shop_buy_data *data, int type);
const char *shop_buy_data_keywords_get(struct shop_buy_data *data);
void shop_buy_data_keywords_set(struct shop_buy_data *data, const char *keywords);

extern struct shop_data *shop_index;
extern int top_shop;

shop_rnum real_shop(shop_vnum vnum);
struct shop_data *shop_by_id(shop_vnum vnum);

void* shop_iterator_create();
struct shop_data* shop_next(void* iterator);
void shop_iterator_free(void* iterator);

void shop_put(shop_vnum vnum, struct shop_data *shop);
void shop_delete(shop_vnum vnum);
size_t shop_count();

#ifdef __cplusplus
}
#endif
