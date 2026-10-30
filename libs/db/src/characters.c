#include "dbat/db/characters.h"
#include "dbat/db/consts/triggers.h"
struct char_data *character_list;
struct char_data *affect_list;
struct char_data *affectv_list;
struct player_special_data dummy_mob;

struct index_data *mob_index;
struct char_data *mob_proto;
mob_rnum top_of_mobt;
struct htree_node *mob_htree;

long max_mob_id = MOB_ID_BASE;