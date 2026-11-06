#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool isname(const char *str, const char *namelist);
bool is_name(const char *str, const char *namelist);
int get_number(char **name);

struct char_data *get_player_vis(GameWorld *g, struct char_data *ch, char *name, int *number, int inroom);
struct char_data *get_char_vis(GameWorld *g, struct char_data *ch, char *name, int *number, int where);
struct char_data *get_char_room_vis(GameWorld *g, struct char_data *ch, char *name, int *number);
struct char_data *get_char_world_vis(GameWorld *g, struct char_data *ch, char *name, int *number);

struct obj_data *get_obj_in_list_num(obj_vnum num, struct obj_data *list);
struct obj_data *get_obj_num(GameWorld *g, obj_rnum nr);
struct obj_data *get_obj_in_list_vis(GameWorld *g, struct char_data *ch, char *name, int *number, struct obj_data *list);
struct obj_data *get_obj_vis(GameWorld *g, struct char_data *ch, char *name, int *num);
struct obj_data *get_obj_in_equip_vis(GameWorld *g, struct char_data *ch, char *arg, int *number, struct obj_data *equipment[]);
int get_obj_pos_in_equip_vis(GameWorld *g, struct char_data *ch, char *arg, int *num, struct obj_data *equipment[]);
int find_eq_pos(GameWorld *g, struct char_data *ch, struct obj_data *obj, char *arg);

/* find all dots */

int find_all_dots(char *arg);

constexpr int FIND_INDIV  = 0;
constexpr int FIND_ALL    = 1;
constexpr int FIND_ALLDOT = 2;

/* Generic Find */

int generic_find(GameWorld *g, char *arg, bitvector_t bitvector, struct char_data *ch,
                 struct char_data **tar_ch, struct obj_data **tar_obj);

constexpr int FIND_CHAR_ROOM  = (1 << 0);
constexpr int FIND_CHAR_WORLD = (1 << 1);
constexpr int FIND_OBJ_INV    = (1 << 2);
constexpr int FIND_OBJ_ROOM   = (1 << 3);
constexpr int FIND_OBJ_WORLD  = (1 << 4);
constexpr int FIND_OBJ_EQUIP  = (1 << 5);

#ifdef __cplusplus
}
#endif
