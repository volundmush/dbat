#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// functions
void remember(GameWorld *g, struct char_data *ch, struct char_data *victim);
void mobile_activity(GameWorld *g);
void clearMemory(struct char_data *ch);

#ifdef __cplusplus
}
#endif
