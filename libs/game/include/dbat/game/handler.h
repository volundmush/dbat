#pragma once
#include "dbat/db/consts/types.h"

/* handling the affected-structures */
void update_char_objects(struct char_data *ch);	/* handler.c */
void item_check(struct obj_data *object, struct char_data *ch);

/* utility */
const char *money_desc(int amount);
struct obj_data *create_money(int amount);
