#pragma once
#include "dbat/consts/types.h"

ACMD(do_alias);

void write_aliases(struct char_data *ch);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);
