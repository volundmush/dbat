#pragma once
#include "consts/types.h"

int flag_test(const bitvector_t bitvector[], int flag);
void flag_set(bitvector_t bitvector[], int flag, int value);
int flag_toggle(bitvector_t bitvector[], int flag);

size_t sprintbit(bitvector_t vector, const char *names[], char *result, size_t reslen);
size_t sprinttype(int type, const char *names[], char *result, size_t reslen);
size_t sprintbitarray(bitvector_t bitvector[], const char *names[], int maxar, char *result, size_t reslen);