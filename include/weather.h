//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"


// commands
extern void star_phase(struct char_data *ch, int type);

extern void oozaru_add();

extern void oozaru_drop();

extern void oozaru_transform(struct char_data *ch);

extern void oozaru_revert(struct char_data *ch);

extern void weather_and_time(int mode);
