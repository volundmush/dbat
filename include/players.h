#pragma once

#include "structs.h"

// global variables
extern long top_idnum;
extern int top_of_p_table;
extern struct player_index_element *player_table;

extern void remove_player(int pfilepos);

extern void load_imc_pfile(struct char_data *ch);
