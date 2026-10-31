#pragma once
#include "dbat/consts/types.h"

// functions
void npc_steal(struct char_data *ch, struct char_data *victim);
int num_players_in_room(room_vnum room);
bool check_mob_in_room(mob_vnum mob, room_vnum room);

// specials
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(guild_guard);
SPECIAL(pet_shops);
SPECIAL(auction);
SPECIAL(bank);
SPECIAL(gravity);
SPECIAL(augmenter);
SPECIAL(healtank);
SPECIAL(augmenter);
SPECIAL(gravity);
SPECIAL(bank);
