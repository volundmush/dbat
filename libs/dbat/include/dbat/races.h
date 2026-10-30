#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/races.h"
#include "dbat/transformations.h"



// global variables

// functions
void racial_body_parts(struct char_data *ch);

void set_height_and_weight_by_race(struct char_data *ch);

int invalid_race(struct char_data *ch, struct obj_data *obj);

// Race behaviour. Races are identified by their RACE_* constant.
typedef enum softcap_type {
    SOFTCAP_FIXED = 0,
    SOFTCAP_VARIABLE = 1
} softcap_type;

bool race_exists(int r);
const char *race_name(int r);
bool race_valid_sex(int r, int sex);
bool race_can_be_mimiced(int r);
int race_find(const char *arg, int sex, bool pc_only);
int race_rpp_cost(int r);
int race_rpp_refund(int r);

bool race_can_transform(int r);
bool race_can_revert(int r);
bool race_has_noisy_transformations(int r);
bool race_check_can_transform(char_data *ch);
int race_max_transform_tier(char_data *ch);
int race_flag_to_tier(int flag);
int race_current_trans_tier(const char_data *ch);
transform_bonus race_current_form(char_data *ch);
bool race_find_form(char_data *ch, const char *arg, transform_bonus *out);
bool race_check_trans_unlock(char_data *ch, int tier);
void race_display_forms(char_data *ch);
void race_display_trans_req(char_data *ch);
void race_echo_transform(char_data *ch, int tier);
void race_echo_revert(char_data *ch, int tier);

const int64_t *race_soft_cap_table(const char_data *ch);
softcap_type race_soft_cap_type(const char_data *ch);

int get_size(struct char_data *ch);
int wield_type(int chsize, const struct obj_data *weap);