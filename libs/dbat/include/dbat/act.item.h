#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* act.item.c */
// global variables

// functions
int check_saveroom_count(GameWorld *g, struct char_data *ch, struct obj_data *cont);
int check_insidebag(struct obj_data *cont, double mult);

void weight_change_object(GameWorld *g, struct obj_data *obj, int weight);
void name_from_drinkcon(GameWorld *g, struct obj_data *obj);
void name_to_drinkcon(GameWorld *g, struct obj_data *obj, int type);

void stop_auction(GameWorld *g, int type, struct char_data *ch);
void check_auction(GameWorld *g);

// commands
ACMD(do_auction);
ACMD(do_bid);
ACMD(do_assemble);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_twohand);
ACMD(do_deploy);
ACMD(do_pack);
ACMD(do_garden);
ACMD(do_refuel);

#ifdef __cplusplus
}
#endif
