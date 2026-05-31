#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/affected.h"

#ifdef __cplusplus
extern "C" {
#endif

void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data * ch, int loc, int mod, int spec, long bitv, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af);
void	affect_from_char(struct char_data *ch, int type);
bool	affected_by_spell(struct char_data *ch, int type);
bool	affectedv_by_spell(struct char_data *ch, int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void affect_modify_ar(struct char_data * ch, int loc, int mod, int spec, bitvector_t bitv[], bool add);

#ifdef __cplusplus
}
#endif
