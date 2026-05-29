#include "dbat/db/characters.h"
#include "dbat/db/consts/triggers.h"
#include "dbat/db/consts/search.h"
#include "dbat/db/objects.h"

struct char_data *character_list;
struct char_data *affect_list;
struct char_data *affectv_list;
struct player_special_data dummy_mob;


long max_mob_id = MOB_ID_BASE;

struct char_data *mob_proto_by_id(mob_vnum vnum)
{
  return mob_proto_get(vnum);
}

mob_vnum mob_proto_vnum_check(mob_vnum vnum)
{
  return mob_proto_by_id(vnum) ? vnum : NOTHING;
}

struct obj_data* char_inventory_search_vnum(struct char_data *ch, obj_vnum vnum, bool recursive, int flags) {
    struct obj_vnum_search_data data = {
        .vnum = vnum,
        .flags = flags,
        .ch = ch,
        .found = nullptr
    };
    char_inventory_iterate(ch, recursive, obj_search_vnum_match, &data);
    return data.found;
}

struct obj_data* char_inventory_search_type(struct char_data *ch, int type, bool recursive, int flags) {
    struct obj_type_search_data data = {
        .type = type,
        .flags = flags,
        .ch = ch,
        .found = nullptr
    };
    char_inventory_iterate(ch, recursive, obj_search_type_match, &data);
    return data.found;
}
