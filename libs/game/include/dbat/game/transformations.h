#pragma once
#include "dbat/db/consts/types.h"

struct transform_bonus {
    int64_t bonus;
    long double mult, drain;
    int flag;
    int rpp_cost;
    int64_t requires_pl;
    const char *key;
    const char *name;
    const char* msg_transform_self;
    const char* msg_transform_others;
};

struct transforms_available {
    int number;
    struct transform_bonus **bonuses;
};

extern struct transform_bonus base_form;
extern struct transform_bonus oozaru;

struct transforms_available* get_transforms_available(char_data *ch);
struct transform_bonus get_current_transform(char_data *ch);

int get_current_trans_tier(struct char_data *ch);
int race_can_transform(int race_id);
int race_can_revert(int race_id);
int trans_flag_to_tier(int flag);
int race_has_noisy_transformations(int race_id);
void display_transforms(struct char_data* ch);
int check_can_transform(struct char_data *ch);
struct transform_bonus select_transformation(struct char_data* ch, const char* key);
int check_trans_unlock(struct char_data *ch, int tier);
