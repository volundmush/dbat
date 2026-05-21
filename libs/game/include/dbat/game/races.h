#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/consts/races.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables

// functions
void racial_body_parts(struct char_data *ch);

void set_height_and_weight_by_race(struct char_data *ch);

int invalid_race(struct char_data *ch, struct obj_data *obj);

int get_size(struct char_data *ch);
int wield_type(int chsize, const struct obj_data *weap);

#ifdef __cplusplus
}
#endif
