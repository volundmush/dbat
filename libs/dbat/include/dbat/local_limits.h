#pragma once
#include "dbat/consts/types.h"

// functions
void timed_dt(struct char_data *ch);
void run_autowiz(void);
void set_title(struct char_data *ch, char *title);
void gain_level(struct char_data *ch, int whichclass);
void gain_exp(struct char_data *ch, int64_t gain);
void gain_exp_regardless(struct char_data *ch, int gain);
void gain_condition(struct char_data *ch, int condition, int value);
void point_update(void);
