#include "dbat/game/transformations.h"
#include "dbat/db/consts/races.h"
#include "dbat/db/consts/playerflags.h"

struct transform_bonus base_form = {
    .bonus=0, 
    .mult=1.0,
    .drain=0.0,
    .flag=0,
    .rpp_cost=0,
    .requires_pl=0,
    .name="base form",
    .msg_transform="",
    .msg_revert=""
};

static struct transform_bonus saiyan_super_saiyan_1 = {
    .bonus=800000,
    .mult=2.0,
    .drain=0.1,
    .flag=PLR_TRANS1,
    .rpp_cost=0,
    .requires_pl=1200000,
    
};

static struct transform_bonus saiyan_super_saiyan_2 = {
    .bonus=20000000,
    .mult=3.0,
    .drain=0.2,
    .flag=PLR_TRANS2,
    .rpp_cost=0,
    .requires_pl=55000000,
    
};

static struct transform_bonus saiyan_super_saiyan_3 = {
    .bonus=80000000,
    .mult=4.0,
    .drain=0.2,
    .flag=PLR_TRANS3,
    .rpp_cost=0,
    .requires_pl=200000000
};

static struct transform_bonus saiyan_super_saiyan_4 = {
    .bonus=200000000,
    .mult=5.0,
    .drain=0.2,
    .flag=PLR_TRANS4,
    .rpp_cost=0,
    .requires_pl=1625000000
};

static struct transform_bonus saiyan_transformations[] = {
    saiyan_super_saiyan_1,
    saiyan_super_saiyan_2,
    saiyan_super_saiyan_3,
    saiyan_super_saiyan_4
};

static struct transforms_available saiyan_transforms_available = {
    .number=4,
    .bonuses=saiyan_transformations
};