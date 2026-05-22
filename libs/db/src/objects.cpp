#include "dbat/db/objects.h"
#include "dbat/db/consts/triggers.h"

struct index_data *obj_index;
struct obj_data *object_list;
struct obj_data *obj_proto;
obj_rnum top_of_objt;
struct htree_node *obj_htree;
long max_obj_id = OBJ_ID_BASE;

/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid, i, last_top;

  i = htree_find(obj_htree, vnum);

  if (i != NOWHERE && obj_index[i].vnum == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_objt;

    /* perform binary search on obj-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((obj_index + mid)->vnum == vnum) {
        htree_add(obj_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOTHING);
      if ((obj_index + mid)->vnum > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}

struct obj_data *obj_proto_by_id(obj_vnum vnum)
{
  obj_rnum rnum = real_object(vnum);

  if (rnum == NOTHING || !obj_proto)
    return nullptr;

  return &obj_proto[rnum];
}
