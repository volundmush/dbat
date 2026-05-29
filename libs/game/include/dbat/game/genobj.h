#pragma once
#include "dbat/db/consts/types.h"


#ifdef __cplusplus
extern "C" {
#endif

void copy_object_strings(struct obj_data *to, struct obj_data *from);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
int copy_object(struct obj_data *to, struct obj_data *from);
int copy_object_preserve(struct obj_data *to, struct obj_data *from);
int save_objects(struct zone_data *zone);
int update_objects(struct obj_data *refobj);
obj_rnum add_object(struct obj_data *, obj_vnum ovnum);
int delete_object(obj_rnum);


#ifdef __cplusplus
}
#endif
