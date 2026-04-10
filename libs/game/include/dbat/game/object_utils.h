#pragma once
#include "dbat/db/object_utils.h"
#include "dbat/db/objects.h"


int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
int is_better(struct obj_data *object, struct obj_data *object2);