#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int json_export_room(room_vnum vnum, const char* filename);
int json_export_rooms(const char* folder);

int json_export_room_exits(room_vnum vnum, const char* filename);
int json_export_all_room_exits(const char* folder);

int json_export_npc_prototype(mob_vnum vnum, const char* filename);
int json_export_npc_prototypes(const char* folder);

int json_export_obj_prototype(obj_vnum vnum, const char* filename);
int json_export_obj_prototypes(const char* folder);

int json_export_zone(zone_vnum vnum, const char* filename);
int json_export_zones(const char* folder);

int json_export_shop(shop_vnum vnum, const char* filename);
int json_export_shops(const char* folder);

int json_export_guild(guild_vnum vnum, const char* filename);
int json_export_guilds(const char* folder);

int json_export_dgscript(trig_vnum vnum, const char* filename);
int json_export_dgscripts(const char* folder);

int json_export_all(const char* folder);

void json_import_all(const char* folder);
int json_import_zones(const char* folder);
int json_import_rooms(const char* folder);
int json_import_room_exits(const char* folder);
int json_import_dgscripts(const char* folder);
int json_import_npc_prototypes(const char* folder);
int json_import_obj_prototypes(const char* folder);
int json_import_shops(const char* folder);
int json_import_guilds(const char* folder);

#ifdef __cplusplus
}
#endif
