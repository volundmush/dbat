#pragma once
#include "dbat/db/objects.h"


#ifdef __cplusplus
extern "C" {
#endif

int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
int is_better(struct obj_data *object, struct obj_data *object2);

struct room_direction_data* obj_exit_dir(struct obj_data *obj, int dir);
struct room_direction_data* obj_exit_dir_2nd(struct obj_data *obj, int dir);
struct room_direction_data* obj_exit_dir_3rd(struct obj_data *obj, int dir);

bool obj_planet_zenith(struct obj_data *obj);

#ifdef __cplusplus
}
#endif
