#pragma once
#include "dbat/consts/types.hpp"

ACMD(do_tedit);
void tedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator);
void news_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator);

