#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void circle_srandom(GameWorld *g, unsigned long initial_seed);
unsigned long circle_random(GameWorld *g);
int64_t large_rand(GameWorld *g, int64_t from, int64_t to);
int rand_number(GameWorld *g, int from, int to);
int axion_dice(GameWorld *g, int adjust);
int dice(GameWorld *g, int num, int size);
int roll_aff_duration(GameWorld *g, int num, int add);

#ifdef __cplusplus
}
#endif
