#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/consts/races.h"
#include "dbat/db/consts/sex.h"
#include "dbat/game/transformations.h"


// global variables

// functions
void racial_body_parts(struct char_data *ch);

void set_height_and_weight_by_race(struct char_data *ch);

int invalid_race(struct char_data *ch, struct obj_data *obj);

// C++ conversion

int get_size(struct char_data *ch);
int wield_type(int chsize, const struct obj_data *weap);

int race_rpp_cost(int race_id);
int race_rpp_refund(int race_id);
int race_soft_type(struct char_data *ch);
int64_t *race_soft_map(struct char_data *ch);
int parse_race(struct char_data *ch, int arg);
extern const int race_ok_gender[NUM_SEX][NUM_RACES];