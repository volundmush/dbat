#pragma once
#include "dbat/consts/types.hpp"

// functions
void npc_steal(GameWorld *g, struct char_data *ch, struct char_data *victim);
int num_players_in_room(GameWorld *g, room_vnum room);
bool check_mob_in_room(GameWorld *g, mob_vnum mob, room_vnum room);

// specials
SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(auction);
SPECIAL(bank);
SPECIAL(gravity);
SPECIAL(augmenter);
SPECIAL(healtank);

