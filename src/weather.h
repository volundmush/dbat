//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_WEATHER_H
#define CIRCLE_WEATHER_H

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "act.h"

// commands
void star_phase(struct char_data *ch, int type);
void oozaru_add(struct char_data *tch);
void oozaru_drop(struct char_data *tch);
void weather_and_time(int mode);

#endif //CIRCLE_WEATHER_H
