#pragma once
#include "dbat/consts/types.h"
#include "house.h"

#ifdef __cplusplus
extern "C" {
#endif

void hsedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void free_house(struct house_control_rec *house);

#ifdef __cplusplus
}
#endif
