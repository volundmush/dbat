#pragma once
#include "dbat/db/objects.h"


#ifdef __cplusplus
extern "C" {
#endif

int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
int is_better(struct obj_data *object, struct obj_data *object2);

#ifdef __cplusplus
}
#endif
