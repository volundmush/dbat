#pragma once
#include "dbat/db/consts/types.h"


#ifdef __cplusplus
extern "C" {
#endif

// functions
void remember(struct char_data *ch, struct char_data *victim);
void mobile_activity(void);
void forget(struct char_data *ch, struct char_data *victim);
void mob_taunt(struct char_data *ch);
void clearMemory(struct char_data *ch);

#ifdef __cplusplus
}
#endif
