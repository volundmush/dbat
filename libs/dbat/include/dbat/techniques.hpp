#pragma once
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"

bool tech_handle_android_absorb(GameWorld *g, char_data *ch, char_data *vict);
bool tech_handle_zanzoken(GameWorld *g, char_data *ch, char_data *vict, const char *name);
void tech_handle_posmodifier(char_data *vict, int *pry, int *blk, int *dge, int *prob);
void tech_handle_fireshield(GameWorld *g, char_data *ch, char_data *vict, const char *part);
/* ch's fire burns vict when rand_number(1, 4) rolls face, unless they are burned already, a demon or fireproof; if it
   doesn't, a fire-prone vict is burned all the same */
void tech_handle_burn(GameWorld *g, char_data *ch, char_data *vict, int face);
/* What an impact in where does to it, as ch (and vict) see: debris, dust, cracks or water thrown about, by what where
   is; outdoors, it can set lava flowing, as the room's ground effect lava. With water_too (a quirk), water is shaken as
   the ground outdoors is, as well */
void tech_impact_scenery(GameWorld *g, char_data *ch, char_data *vict, room_vnum where, int lava, bool water_too = false);
bool tech_handle_targeting(GameWorld *g, char_data *ch, std::string &arg, char_data **vict, obj_data **obj);
/* the user has arms, and using a broken one hurts it more; false (and says why) if they have none */
bool tech_handle_arms(GameWorld *g, char_data *ch);
bool tech_handle_charge(GameWorld *g, char_data *ch, std::string_view arg, double minimum, double *attperc);
void tech_handle_crashdown(GameWorld *g, char_data *ch, char_data *vict);
