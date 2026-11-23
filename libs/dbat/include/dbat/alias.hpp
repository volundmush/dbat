#pragma once
#include <string_view>
#include "dbat/consts/types.hpp"

ACMD(do_alias);

void write_aliases(GameWorld *g, struct char_data *ch);
void read_aliases(GameWorld *g, struct char_data *ch);
void delete_aliases(std::string_view charname);

