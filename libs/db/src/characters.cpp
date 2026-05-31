#include "dbat/db/characters.h"
#include "dbat/db/consts/triggers.h"
#include "dbat/db/consts/search.h"
#include "dbat/db/objects.h"

struct char_data *character_list;
struct char_data *affect_list;


long max_mob_id = MOB_ID_BASE;

struct mob_proto_data *mob_proto_by_id(mob_vnum vnum)
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

int64_t char_legacy_modifier(struct char_data *ch, int location, int specific) {
    int64_t mod = 0;

    for(auto i = 0; i < NUM_WEARS; i++) {
        if (auto eq = ch->equipment[i]) {
            for (auto j = 0; j < MAX_OBJ_AFFECT; j++) {
                if(eq->affected[j].location != location) continue;
                if (eq->affected[j].specific == specific || eq->affected[j].specific == -1) {
                    mod += eq->affected[j].modifier;
                }
            }
        }
    }

    for (auto af = ch->affected; af; af = af->next) {
        if(af->location != location) continue;
        if (af->specific == specific || af->specific == -1) {
            mod += af->modifier;
        }
    }
    return mod;
}
