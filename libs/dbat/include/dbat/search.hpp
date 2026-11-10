#pragma once
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/registry.hpp"

/* Whether str abbreviates one of the words of namelist, ignoring case (a number must match whole) */
bool isname(std::string_view str, std::string_view namelist);
/* Whether str is one of the words of namelist, ignoring case */
bool is_name(std::string_view str, std::string_view namelist);
/* The same, reading a null string as "" (the getters give null for an empty name) */
bool isname(std::string_view str, const char *namelist);
bool isname(const char *str, const char *namelist);
bool is_name(const char *str, const char *namelist);

/* The N of an "N.name" argument, taking "N." off name: 1 with no dot, 0 if N isn't a number */
int get_number(std::string &name);

/*
 * The lookups take the name as a std::string they may change, as they always have: with no number given, the
 * "N." comes off the front, and get_char_room_vis() capitalises it as it goes. A caller printing the name
 * afterwards prints what is left.
 */
struct char_data *get_char_room(GameWorld *g, std::string &name, int *number, room_vnum room);
struct char_data *get_player_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, int inroom);
struct char_data *get_char_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, int where);
struct char_data *get_char_room_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number);
struct char_data *get_char_world_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number);
struct obj_data *get_obj_in_list_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, const id_list &list);
struct obj_data *get_obj_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number);
struct obj_data *get_obj_in_equip_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, struct obj_data *equipment[]);
int get_obj_pos_in_equip_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, struct obj_data *equipment[]);

/* The char * forms, which change the caller's buffer the same way, for the DG message parser and the shopkeeper,
 * which still work on char * lines */
int get_number(char **name);
struct char_data *get_char_room_vis(GameWorld *g, struct char_data *ch, char *name, int *number);
struct obj_data *get_obj_in_list_vis(GameWorld *g, struct char_data *ch, char *name, int *number, const id_list &list);
struct obj_data *get_obj_in_equip_vis(GameWorld *g, struct char_data *ch, char *arg, int *number, struct obj_data *equipment[]);

struct obj_data *get_obj_in_list_num(GameWorld *g, obj_vnum num, const id_list &list);
struct obj_data *get_obj_num(GameWorld *g, obj_vnum nr);
/* Whether obj answers to name and ch can see it (a light always shows), as get_obj_in_list_vis() requires */
bool obj_named_and_visible(GameWorld *g, struct char_data *ch, std::string_view name, struct obj_data *obj);
/* The wear position arg names ("finger", "neck" ...), telling ch if it names none; with no arg, the first
 * place obj can go */
int find_eq_pos(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view arg = {});

/* find all dots */

/* FIND_ALL for "all", FIND_ALLDOT for "all.x" (taking "all." off arg), FIND_INDIV otherwise */
int find_all_dots(std::string &arg);

constexpr int FIND_INDIV  = 0;
constexpr int FIND_ALL    = 1;
constexpr int FIND_ALLDOT = 2;

/* Generic Find */

int generic_find(GameWorld *g, std::string_view arg, bitvector_t bitvector, struct char_data *ch,
                 struct char_data **tar_ch, struct obj_data **tar_obj);

constexpr int FIND_CHAR_ROOM  = (1 << 0);
constexpr int FIND_CHAR_WORLD = (1 << 1);
constexpr int FIND_OBJ_INV    = (1 << 2);
constexpr int FIND_OBJ_ROOM   = (1 << 3);
constexpr int FIND_OBJ_WORLD  = (1 << 4);
constexpr int FIND_OBJ_EQUIP  = (1 << 5);

