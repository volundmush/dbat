#pragma once
#include "dbat/registry.hpp"
#include "dbat/consts/types.hpp"

/* After changing an object's weight: tells what contains it, and whoever carries it. */
void obj_weight_changed(GameWorld *g, struct obj_data *obj, int64_t change);
void obj_from_char(GameWorld *g, struct obj_data *object);

void equip_char(GameWorld *g, struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(GameWorld *g, struct char_data *ch, int pos);
bool invalid_align(struct char_data *ch, struct obj_data *obj);

void obj_to_room(GameWorld *g, struct obj_data *object, room_vnum room);
void obj_from_room(GameWorld *g, struct obj_data *object);
void obj_to_obj(GameWorld *g, struct obj_data *obj, struct obj_data *obj_to);
void obj_from_obj(GameWorld *g, struct obj_data *obj);
void object_list_new_owner(GameWorld *g, const id_list &list, struct char_data *ch);

void obj_to_char(GameWorld *g, struct obj_data *object, struct char_data *ch);

struct char_data *get_char_num(GameWorld *g, mob_vnum nr);
void char_from_room(GameWorld *g, struct char_data *ch);
void char_to_room(GameWorld *g, struct char_data *ch, room_vnum room);
