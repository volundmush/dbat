#include "dbat/db/objects.h"
#include "dbat/db/consts/triggers.h"

struct index_data *obj_index;
struct obj_data *object_list;
struct obj_data *obj_proto;
obj_rnum top_of_objt;
struct htree_node *obj_htree;
long max_obj_id = OBJ_ID_BASE;