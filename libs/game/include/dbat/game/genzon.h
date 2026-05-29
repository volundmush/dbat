#pragma once
#include "dbat/db/consts/types.h"
#include "db.h"

#ifdef __cplusplus
extern "C" {
#endif

zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);
void remove_room_zone_commands(struct zone_data *zone, struct room_data *room);
int save_zone(struct zone_data *zone);
int count_commands(struct reset_com *list);
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
void remove_cmd_from_list(struct reset_com **list, int pos);
int new_command(struct zone_data *zone, int pos);
void delete_zone_command(struct zone_data *zone, int pos);

/* Make delete_zone() */


#ifdef __cplusplus
}
#endif
