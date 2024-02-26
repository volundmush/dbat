#pragma once

#include "structs.h"

/* act.item.c */
// global variables
extern Object *obj_selling;
extern Character *ch_selling, *ch_buying;

// functions
extern int check_saveroom_count(Character *ch, Object *cont);

extern void dball_load(uint64_t heartPulse, double deltaTime);

extern int check_insidebag(Object *cont, double mult);

extern int perform_get_from_room(Character *ch, Object *obj);

extern void weight_change_object(Object *obj, int weight);

extern void name_from_drinkcon(Object *obj);

extern void name_to_drinkcon(Object *obj, int type);

extern void perform_wear(Character *ch, Object *obj, int where);

extern int find_eq_pos(Character *ch, Object *obj, char *arg);

extern void perform_remove(Character *ch, int pos);

extern void stop_auction(int type, Character *ch);

extern void check_auction(uint64_t heartPulse, double deltaTime);

// commands
extern ACMD(do_split);

extern ACMD(do_auction);

extern ACMD(do_bid);

extern ACMD(do_assemble);

extern ACMD(do_remove);

extern ACMD(do_put);

extern ACMD(do_get);

extern ACMD(do_drop);

extern ACMD(do_give);

extern ACMD(do_drink);

extern ACMD(do_eat);

extern ACMD(do_pour);

extern ACMD(do_wear);

extern ACMD(do_wield);

extern ACMD(do_grab);

extern ACMD(do_twohand);

extern ACMD(do_deploy);

extern ACMD(do_pack);

extern ACMD(do_garden);

extern ACMD(do_refuel);

extern ACMD(do_sac);
