#pragma once
/*
 * The fix-ups the loaders make to what the world's files say: each is a rule with a name, called from its kind's
 * reader as the record loads.
 */
#include "dbat/consts/types.hpp"

struct GameWorld;
struct extra_descr_data;

namespace rules {

/* Rooms flagged Vegeta or gravity x10 have 10 times gravity */
void gravity_by_flags(GameWorld *g, room_vnum room);
/* The gravity-training rooms have the gravity of their range of vnums, whatever their flags say */
void gravity_by_vnum(GameWorld *g, room_vnum room);
/* An extra description ends in a line end, as the editors' formatting assumes (Welcor 09/03) */
void extra_description_ends_a_line(extra_descr_data &descr);

} // namespace rules
