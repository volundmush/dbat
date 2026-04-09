#pragma once
#include "dbat/db/consts/types.h"

void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);
int64_t large_rand(int64_t from, int64_t to);
int rand_number(int from, int to);
int axion_dice(int adjust);
int dice(int num, int size);
int roll_aff_duration(int num, int add);