#pragma once
#include "dbat/consts/types.hpp"

ACMD(do_alias);

void write_aliases(GameWorld *g, struct char_data *ch);
void read_aliases(GameWorld *g, struct char_data *ch);
void delete_aliases(const char *charname);

