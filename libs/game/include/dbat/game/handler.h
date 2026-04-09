#pragma once
#include "dbat/db/consts/types.h"

/* handling the affected-structures */
void update_char_objects(struct char_data *ch);	/* handler.c */
void item_check(struct obj_data *object, struct char_data *ch);
void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data * ch, int loc, int mod, int spec, long bitv, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af);
void	affect_from_char(struct char_data *ch, int type);
bool	affected_by_spell(struct char_data *ch, int type);
bool	affectedv_by_spell(struct char_data *ch, int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void	affectv_join(struct char_data *ch, struct affected_type *af,
bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void	affectv_remove(struct char_data *ch, struct affected_type *af);
void	affectv_to_char(struct char_data *ch, struct affected_type *af);
void	affectv_from_char(struct char_data *ch, int type);


/* utility */
const char *money_desc(int amount);
struct obj_data *create_money(int amount);


/* ******** objects *********** */






/* ******* characters ********* */




/* find if character can see */

