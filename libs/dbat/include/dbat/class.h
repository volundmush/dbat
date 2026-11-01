#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/senseis.h"
#include "dbat/races.h"

// global variables

// functions
void do_start(struct char_data *ch);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(struct char_data *ch, int level);
int load_levels();
void cedit_creation(struct char_data *ch);
void advance_level(struct char_data *ch, int whichclass);
int8_t ability_mod_value(int abil);
int8_t dex_mod_capped(const struct char_data *ch);
char *class_desc_str(struct char_data *ch, int howlong, int wantthe);
int highest_skill_value(int level, int type);
time_t birth_age(struct char_data *ch);
time_t max_age(struct char_data *ch);

// Sensei behaviour. Senseis are identified by their CLASS_* constant.
bool sensei_exists(int s);
const char *sensei_name(int s);
bool sensei_is_pc_ok(int s);
bool sensei_available_for_race(int s, int r);
int sensei_find(const char *arg);
