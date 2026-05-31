#include "dbat/db/objects.h"
#include "dbat/db/consts/triggers.h"
#include "dbat/db/consts/search.h"

struct obj_data *object_list;
long max_obj_id = OBJ_ID_BASE;

/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  return obj_proto_by_id(vnum) ? vnum : NOTHING;
}

struct obj_proto_data *obj_proto_by_id(obj_vnum vnum)
{
  return obj_proto_get(vnum);
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
