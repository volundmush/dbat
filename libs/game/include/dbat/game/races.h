#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/consts/sex.h"
#include "dbat/db/consts/races.h"

// global variables
extern const int race_ok_gender[NUM_SEX][NUM_RACES];

// functions
void racial_body_parts(struct char_data *ch);
void racial_ability_modifiers(struct char_data *ch);
void set_height_and_weight_by_race(struct char_data *ch);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int parse_race(struct char_data *ch, int arg);
