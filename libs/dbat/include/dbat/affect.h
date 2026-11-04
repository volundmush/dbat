#pragma once
#include "dbat/consts/types.h"
#include "dbat/affected.h"

void affect_total(GameWorld *g, struct char_data *ch);
void affect_to_char(GameWorld *g, struct char_data *ch, struct affected_type *af);
void affect_remove(GameWorld *g, struct char_data *ch, struct affected_type *af);
void affect_from_char(GameWorld *g, struct char_data *ch, int type);
bool affected_by_spell(struct char_data *ch, int type);
void affect_join(GameWorld *g, struct char_data *ch, struct affected_type *af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void affect_modify_ar(GameWorld *g, struct char_data *ch, int loc, int mod, int spec, bitvector_t bitv[], bool add);