#pragma once
#include <vector>

#include "dbat/zones.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/db.hpp"

struct zone_data *create_new_zone(GameWorld *g, zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);
void remove_room_zone_commands(GameWorld *g, struct zone_data *zone, room_vnum room_num);
bool save_zone(GameWorld *g, struct zone_data *zone_num);
/* The commands in a list, not counting the 'S' that ends it */
int count_commands(const std::vector<reset_com> &list);
/* Puts a copy of newcmd at pos in list */
void add_cmd_to_list(std::vector<reset_com> *list, const struct reset_com *newcmd, int pos);
/* A list with no commands: just the 'S' that ends every one */
std::vector<reset_com> empty_cmd_list();
bool new_command(struct zone_data *zone, int pos);
void delete_zone_command(struct zone_data *zone, int pos);
struct zone_data *find_zone_by_thing(GameWorld *g, room_vnum vznum);

/* Make delete_zone() */

