#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/senseis.h"
#include "dbat/races.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables

// functions
void do_start(GameWorld *g, struct char_data *ch);
bool invalid_class(struct char_data *ch, struct obj_data *obj);
int64_t level_exp(struct char_data *ch, int level);
int load_levels(GameWorld *g);
void cedit_creation(GameWorld *g, struct char_data *ch);
void advance_level(GameWorld *g, struct char_data *ch, int whichclass);
int8_t ability_mod_value(int abil);
int8_t dex_mod_capped(const struct char_data *ch);
char *class_desc_str(struct char_data *ch, int howlong, int wantthe);
int highest_skill_value(int level, int type);
time_t birth_age(GameWorld *g, struct char_data *ch);
time_t max_age(struct char_data *ch);

// Sensei behaviour. Senseis are identified by their CLASS_* constant.
bool sensei_exists(int s);
const char *sensei_name(int s);
bool sensei_is_pc_ok(int s);
bool sensei_available_for_race(int s, int r);
int sensei_find(const char *arg);

#ifdef __cplusplus
}
#endif
