#pragma once
#include "dbat/consts/types.hpp"

/* Who is selling what at auction, and who bids highest; null when there is none, or they have gone. */
struct char_data *auction_seller(GameWorld *g);
struct char_data *auction_buyer(GameWorld *g);
struct obj_data *auction_obj(GameWorld *g);
void auction_set_seller(GameWorld *g, struct char_data *ch);
void auction_set_buyer(GameWorld *g, struct char_data *ch);
void auction_set_obj(GameWorld *g, struct obj_data *obj);
