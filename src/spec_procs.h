#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// functions
void npc_steal(struct char_data *ch, struct char_data *victim);
int num_players_in_room(room_vnum room);
bool check_mob_in_room(mob_vnum mob, room_vnum room);
bool check_obj_in_room(obj_vnum obj, room_vnum room);
int dump_drop_special_try(struct char_data *ch, const char *argument);

// specials
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(auction);
SPECIAL(bank);
SPECIAL(gravity);
SPECIAL(augmenter);
SPECIAL(magic_user_orig);
SPECIAL(healtank);
SPECIAL(augmenter);
SPECIAL(gravity);
SPECIAL(bank);
SPECIAL(cleric_marduk);
SPECIAL(cleric_ao);
SPECIAL(dziak);
SPECIAL(azimer);
SPECIAL(lyrzaxyn);

#ifdef __cplusplus
}
#endif
