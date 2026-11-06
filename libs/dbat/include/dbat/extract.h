#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void extract_char(GameWorld *g, struct char_data *ch);
void extract_char_final(GameWorld *g, struct char_data *ch);
void extract_pending_chars(GameWorld *g);
void extract_obj(GameWorld *g, struct obj_data *obj);

#ifdef __cplusplus
}
#endif
