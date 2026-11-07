#pragma once
#include "dbat/consts/types.hpp"
#include "dbat/affected.hpp"

/* What an affect's specific can be, given its location: a skill or feat id, otherwise 0. */
int affect_checked_specific(int location, int specific);
/* Whether an affect read from a file is one the game could have made. */
bool affect_is_loadable(const struct affected_type *af);
void affect_total(GameWorld *g, struct char_data *ch);
void affect_to_char(GameWorld *g, struct char_data *ch, struct affected_type *af);
void affect_remove(GameWorld *g, struct char_data *ch, struct affected_type *af);
void affect_from_char(GameWorld *g, struct char_data *ch, int type);
bool affected_by_spell(struct char_data *ch, int type);
void affect_join(GameWorld *g, struct char_data *ch, struct affected_type *af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void affect_modify_ar(GameWorld *g, struct char_data *ch, int loc, int mod, int spec, bitvector_t bitv[], bool add);

