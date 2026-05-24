#include "dbat/db/objects.h"
#include "dbat/db/consts/triggers.h"
#include "dbat/db/consts/search.h"

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

bool obj_search_vnum_match(struct obj_data *obj, void *ctx) {
    struct obj_vnum_search_data *data = (struct obj_vnum_search_data*)ctx;

    if(obj_proto_id_get(obj) != data->vnum) {
        return true; // keep iterating
    }

    if((data->flags & SEARCH_WORKING) && obj_extra_flagged(obj, ITEM_BROKEN)) {
      return true; // skip broken items.
    }

    if((data->flags & SEARCH_GENUINE) && obj_extra_flagged(obj, ITEM_FORGED)) {
      return true; // skip non-genuine items.
    }

    data->found = obj;
    return false; // stop iterating
}

bool obj_search_type_match(struct obj_data *obj, void *ctx) {
    struct obj_type_search_data *data = (struct obj_type_search_data*)ctx;

    if(obj_type_get(obj) != data->type) {
        return true; // keep iterating
    }

    data->found = obj;
    return false; // stop iterating
}

struct obj_data* obj_contents_search_vnum(struct obj_data *obj, obj_vnum vnum, bool recursive, int flags) {
    struct obj_vnum_search_data data = {
        .vnum = vnum,
        .flags = flags,
        .ch = nullptr,
        .found = nullptr
    };
    obj_contents_list_iterate(obj, recursive, obj_search_vnum_match, &data);
    return data.found;
}

struct obj_data* obj_inventory_search_vnum(struct obj_data *obj, obj_vnum vnum, bool recursive, int flags) {
    return obj_contents_search_vnum(obj->contains, vnum, recursive, flags);
}

struct obj_data* obj_contents_search_type(struct obj_data *obj, int type, bool recursive, int flags) {
    struct obj_type_search_data data = {
        .type = type,
        .flags = 0,
        .ch = nullptr,
        .found = nullptr
    };
    obj_contents_list_iterate(obj, recursive, obj_search_type_match, &data);
    return data.found;
}

struct obj_data* obj_inventory_search_type(struct obj_data *obj, int type, bool recursive, int flags) {
    return obj_contents_search_type(obj->contains, type, recursive, flags);
}