#pragma once

/* Declarations shared by the files split out of act.informative.c. */
#include <string_view>

#include "dbat/consts/types.hpp"

void search_room(GameWorld *g, struct char_data *ch);
void bringdesc(GameWorld *g, struct char_data *ch, struct char_data *tch);
void gen_map(GameWorld *g, struct char_data *ch, int num);
void display_spells(GameWorld *g, struct char_data *ch, struct obj_data *obj);
void display_scroll(GameWorld *g, struct char_data *ch, struct obj_data *obj);
void show_obj_to_char(GameWorld *g, struct obj_data *obj, struct char_data *ch, int mode);
int show_obj_modifiers(GameWorld *g, struct obj_data *obj, struct char_data *ch);
void list_obj_to_char(GameWorld *g, const id_list &list, struct char_data *ch, int mode, int show);
void diag_obj_to_char(GameWorld *g, struct obj_data *obj, struct char_data *ch);
void diag_char_to_char(GameWorld *g, struct char_data *i, struct char_data *ch);
void look_at_char(GameWorld *g, struct char_data *i, struct char_data *ch);
void list_char_to_char(GameWorld *g, const id_list &list, struct char_data *ch);
void look_out_window(GameWorld *g, struct char_data *ch, std::string_view arg);
void trans_check(GameWorld *g, struct char_data *ch, struct char_data *vict);
void search_in_direction(GameWorld *g, struct char_data *ch, int dir);

