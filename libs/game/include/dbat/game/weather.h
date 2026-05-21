#pragma once
#include "dbat/db/consts/types.h"


#ifdef __cplusplus
extern "C" {
#endif

// commands
void star_phase(struct char_data *ch, int type);
void oozaru_add();
void oozaru_drop();
void oozaru_transform(struct char_data *ch);
void oozaru_revert(struct char_data *ch);
void weather_and_time(int mode);


#ifdef __cplusplus
}
#endif
