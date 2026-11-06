#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void free_object_strings(GameWorld *g, struct obj_data *obj);
void free_object_strings_proto(GameWorld *g, struct obj_data *obj);
bool copy_object(GameWorld *g, struct obj_data *to, struct obj_data *from);
bool save_objects(GameWorld *g, zone_rnum vznum);
obj_rnum add_object(GameWorld *g, struct obj_data *, obj_vnum ovnum);
obj_rnum delete_object(GameWorld *g, obj_rnum);

#ifdef __cplusplus
}
#endif
