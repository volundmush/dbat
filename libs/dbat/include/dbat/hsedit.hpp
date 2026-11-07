#pragma once
#include "dbat/consts/types.hpp"
#include "dbat/house.hpp"

void hsedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void free_house(struct house_control_rec *house);

