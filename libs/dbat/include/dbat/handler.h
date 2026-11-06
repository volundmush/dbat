#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* handling the affected-structures */
void update_char_objects(GameWorld *g, struct char_data *ch); /* handler.c */
void item_check(GameWorld *g, struct obj_data *object, struct char_data *ch);

/* utility */

#ifdef __cplusplus
}
#endif
