#pragma once
#include "dbat/consts/types.h"

// functions
void timed_dt(GameWorld *g, struct char_data *ch);
void run_autowiz(GameWorld *g);
void set_title(GameWorld *g, struct char_data *ch, const char *title);
void gain_level(GameWorld *g, struct char_data *ch, int whichclass);
void gain_exp(GameWorld *g, struct char_data *ch, int64_t gain);
void gain_exp_regardless(GameWorld *g, struct char_data *ch, int gain);
void gain_condition(GameWorld *g, struct char_data *ch, int condition, int value);
void point_update(GameWorld *g);
