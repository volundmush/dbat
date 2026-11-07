#pragma once
#include "dbat/consts/types.hpp"
#include "dbat/db.hpp"

zone_rnum create_new_zone(GameWorld *g, zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);
void remove_room_zone_commands(GameWorld *g, zone_rnum zone, room_rnum room_num);
bool save_zone(GameWorld *g, zone_rnum zone_num);
int count_commands(struct reset_com *list);
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
bool new_command(struct zone_data *zone, int pos);
void delete_zone_command(struct zone_data *zone, int pos);
zone_rnum real_zone_by_thing(GameWorld *g, room_vnum vznum);

/* Make delete_zone() */

