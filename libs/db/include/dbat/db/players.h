#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct player_index_element {
   char	*name;
   long id;
   int level;
   int admlevel;
   int flags;
   time_t last;
   int ship;
   int shiproom;
   time_t played;
   char *clan;
};

extern int top_of_p_table;
extern struct player_index_element *player_table;

struct char_data;

int json_player_save(const char *path, struct char_data *ch);
int json_player_load(const char *path, struct char_data *ch);
int json_player_objects_save(const char *path, struct char_data *ch, int rentcode, int cost);
int json_player_objects_load(const char *path, struct char_data *ch);
int json_house_objects_save(const char *path, room_vnum room_vnum);
int json_house_objects_load(const char *path, room_vnum room_vnum);

#ifdef __cplusplus
}
#endif
