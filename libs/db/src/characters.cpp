#include "dbat/db/characters.h"
#include "dbat/db/consts/triggers.h"
#include "dbat/db/consts/search.h"
#include "dbat/db/objects.h"

struct char_data *character_list;
struct char_data *affect_list;
struct char_data *affectv_list;
struct player_special_data dummy_mob;

struct index_data *mob_index;
struct char_data *mob_proto;
mob_rnum top_of_mobt;
struct htree_node *mob_htree;

long max_mob_id = MOB_ID_BASE;

/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  if (vnum == NOTHING) {
    return NOBODY;
  }
  mob_rnum bot, top, mid, i, last_top;

  i = htree_find(mob_htree, vnum);

  if (i != NOBODY && mob_index[i].vnum == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_mobt;

    /* perform binary search on mob-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((mob_index + mid)->vnum == vnum) {
        htree_add(mob_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOBODY);
      if ((mob_index + mid)->vnum > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}

struct char_data *mob_proto_by_id(mob_vnum vnum)
{
  mob_rnum rnum = real_mobile(vnum);

  if (rnum == NOBODY || !mob_proto)
    return nullptr;

  return &mob_proto[rnum];
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