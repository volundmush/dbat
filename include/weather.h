//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_WEATHER_H
#define CIRCLE_WEATHER_H

#include "structs.h"


// commands
void star_phase(struct char_data *ch, int type);
void oozaru_add();
void oozaru_drop();
void oozaru_transform(struct char_data *ch);
void oozaru_revert(struct char_data *ch);
void weather_and_time(int mode);

#endif //CIRCLE_WEATHER_H
