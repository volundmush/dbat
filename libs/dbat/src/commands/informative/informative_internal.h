#pragma once
/* Declarations shared by the files split out of act.informative.c. */
#include "dbat/consts/types.h"

void search_room(struct char_data *ch);
void bringdesc(struct char_data *ch, struct char_data *tch);
void gen_map(struct char_data *ch, int num);
void display_spells(struct char_data *ch, struct obj_data *obj);
void display_scroll(struct char_data *ch, struct obj_data *obj);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);
void list_char_to_char(struct char_data *list, struct char_data *ch);
void look_out_window(struct char_data *ch, char *arg);
void trans_check(struct char_data *ch, struct char_data *vict);
void search_in_direction(struct char_data *ch, int dir);
