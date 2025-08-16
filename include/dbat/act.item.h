#pragma once

#include "structs.h"

/* act.item.c */
// global variables
extern Object *obj_selling;
extern Character *ch_selling, *ch_buying;

// functions
extern void dball_load(uint64_t heartPulse, double deltaTime);

extern int check_insidebag(Object *cont, double mult);

extern int perform_get_from_room(Character *ch, Object *obj);

extern void weight_change_object(Object *obj, int weight);

extern void name_from_drinkcon(Object *obj);

extern void name_to_drinkcon(Object *obj, int type);

extern void perform_wear(Character *ch, Object *obj, int where);

extern int find_eq_pos(Character *ch, Object *obj, char *arg);

extern void perform_remove(Character *ch, int pos);

extern int64_t max_carry_weight(Character *ch);

extern void stop_auction(int type, Character *ch);

extern void check_auction(uint64_t heartPulse, double deltaTime);

// commands
