#pragma once
#include "dbat/db/consts/types.h"

void	obj_from_char(struct obj_data *object);

void	equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);
int	invalid_align(struct char_data *ch, struct obj_data *obj);

void	obj_to_room(struct obj_data *object, room_rnum room);
void	obj_from_room(struct obj_data *object);
void	obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	obj_to_char(struct obj_data *object, struct char_data *ch);

struct char_data *get_char_room(char *name, int *num, room_rnum room);
struct char_data *get_char_num(mob_rnum nr);
void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, room_rnum room);