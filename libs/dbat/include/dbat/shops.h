#pragma once
#include "consts/types.h"

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
   SPECIAL (*func);		/* Secondary spec_proc for shopkeeper	*/
};

extern struct shop_data *shop_index;
extern int top_shop;