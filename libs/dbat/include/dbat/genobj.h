#pragma once
#include "dbat/consts/types.h"

void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
int copy_object(struct obj_data *to, struct obj_data *from);
int save_objects(zone_rnum vznum);
obj_rnum add_object(struct obj_data *, obj_vnum ovnum);
int delete_object(obj_rnum);
