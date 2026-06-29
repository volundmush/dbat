#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// functions
void remember(struct char_data *ch, struct char_data *victim);
void mobile_activity(void);
void forget(struct char_data *ch, struct char_data *victim);
void clearMemory(struct char_data *ch);

// Game lifecycle: call at entity spawn/load and at extract
void char_game_activate(struct char_data *ch);
void char_game_deactivate(struct char_data *ch);
void obj_game_activate(struct obj_data *obj);
void obj_game_deactivate(struct obj_data *obj);

#ifdef __cplusplus
}
#endif
