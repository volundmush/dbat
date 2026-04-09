#pragma once
#include "dbat/db/consts/types.h"

void	extract_char(struct char_data *ch);
void	extract_char_final(struct char_data *ch);
void	extract_pending_chars(void);
void	extract_obj(struct obj_data *obj);