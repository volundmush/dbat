#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"
#include "dbat/registry.hpp"

/* Where a lookup looks: generic_find() takes a set of these, the character lookups one of the first two */
extern const name_table find_bits; /* flag_count<find_flag> of them */
enum class find_flag : uint8_t {
  char_room  = 0,
  char_world = 1,
  obj_inv    = 2,
  obj_room   = 3,
  obj_world  = 4,
  obj_equip  = 5,
  count_     = 6,
};
template <> struct flag_traits<find_flag> {
  static constexpr const name_table &names = find_bits;
};

/* What an argument asks for: one thing, "all", or "all.x" */
enum class find_dots : uint8_t { indiv = 0, all = 1, alldot = 2 };

/* Whether str abbreviates one of the words of namelist, ignoring case (a number must match whole) */
bool isname(std::string_view str, std::string_view namelist);
/* Whether str is one of the words of namelist, ignoring case */
bool is_name(std::string_view str, std::string_view namelist);

/* The N of an "N.name" argument, taking "N." off name: 1 with no dot, 0 if N isn't a number */
int get_number(std::string &name);

/*
 * The lookups take the name as a std::string they may change, as they always have: with no number given, the
 * "N." comes off the front, and get_char_room_vis() capitalises it as it goes. A caller printing the name
 * afterwards prints what is left.
 */
struct char_data *get_char_room(GameWorld *g, std::string &name, int *number, room_vnum room);
/* inroom: find_flag::char_room for the room only; anything else, or none, for anywhere */
struct char_data *get_player_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, std::optional<find_flag> inroom);
struct char_data *get_char_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, find_flag where);
struct char_data *get_char_room_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number);
struct char_data *get_char_world_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number);
struct obj_data *get_obj_in_list_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, const id_list &list);
struct obj_data *get_obj_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number);
struct obj_data *get_obj_in_equip_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, struct obj_data *equipment[]);
int get_obj_pos_in_equip_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, struct obj_data *equipment[]);

struct obj_data *get_obj_in_list_num(GameWorld *g, obj_vnum num, const id_list &list);
struct obj_data *get_obj_num(GameWorld *g, obj_vnum nr);
/* Whether obj answers to name and ch can see it (a light always shows), as get_obj_in_list_vis() requires */
bool obj_named_and_visible(GameWorld *g, struct char_data *ch, std::string_view name, struct obj_data *obj);
/* The wear position arg names ("finger", "neck" ...), telling ch if it names none; with no arg, the first
 * place obj can go */
int find_eq_pos(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view arg = {});

/* find all dots */

/* find_dots::all for "all", find_dots::alldot for "all.x" (taking "all." off arg), find_dots::indiv otherwise */
find_dots find_all_dots(std::string &arg);

/* Generic Find */

/* Looks where says, in the order char_room, char_world, obj_equip, obj_inv, obj_room, obj_world; the place it was
   found, or none */
std::optional<find_flag> generic_find(GameWorld *g, std::string_view arg, flag_set<find_flag> where, struct char_data *ch,
                                      struct char_data **tar_ch, struct obj_data **tar_obj);

