#pragma once

/* Shared between the files of this directory only. */
#include "dbat/consts/types.hpp"

bool valid_silk(GameWorld *g, struct obj_data *obj);


/* vict is in ch's group: both grouped, and one follows the other or both follow the same leader */
bool grouped_with(GameWorld *g, struct char_data *ch, struct char_data *vict);
