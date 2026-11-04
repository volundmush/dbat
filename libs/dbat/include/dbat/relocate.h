#pragma once
#include "dbat/consts/types.h"

void obj_from_char(GameWorld *g, struct obj_data *object);

void equip_char(GameWorld *g, struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(GameWorld *g, struct char_data *ch, int pos);
bool invalid_align(struct char_data *ch, struct obj_data *obj);

void obj_to_room(GameWorld *g, struct obj_data *object, room_rnum room);
void obj_from_room(GameWorld *g, struct obj_data *object);
void obj_to_obj(GameWorld *g, struct obj_data *obj, struct obj_data *obj_to);
void obj_from_obj(GameWorld *g, struct obj_data *obj);
void object_list_new_owner(struct obj_data *list, struct char_data *ch);

void obj_to_char(GameWorld *g, struct obj_data *object, struct char_data *ch);

struct char_data *get_char_room(GameWorld *g, char *name, int *num, room_rnum room);
struct char_data *get_char_num(GameWorld *g, mob_rnum nr);
void char_from_room(GameWorld *g, struct char_data *ch);
void char_to_room(GameWorld *g, struct char_data *ch, room_rnum room);