#pragma once
#include "dbat/consts/types.h"

/* act.item.c */
// global variables
extern struct obj_data *obj_selling;
extern struct char_data *ch_selling;

// functions
int check_saveroom_count(struct char_data *ch, struct obj_data *cont);
int check_insidebag(struct obj_data *cont, double mult);

void weight_change_object(struct obj_data *obj, int weight);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);

void stop_auction(int type, struct char_data *ch);
void check_auction(void);

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
