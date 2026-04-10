#pragma once
#include "dbat/db/consts/types.h"

struct transform_bonus {
    int64_t bonus;
    long double mult, drain;
    int flag;
    int rpp_cost;
    int64_t requires_pl;
    const char *name;
    const char* msg_transform_self;
    const char* msg_transform_others;
};

struct transforms_available {
    int number;
    struct transform_bonus* bonuses;
};

extern transform_bonus base_form;
extern transform_bonus oozaru;

transforms_available* get_transforms_available(char_data *ch);
transform_bonus get_current_transform(char_data *ch);