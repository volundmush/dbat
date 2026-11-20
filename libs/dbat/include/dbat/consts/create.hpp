#pragma once

/* The ways a character can be created (CONFIG_CREATION_METHOD) */
constexpr int CEDIT_CREATION_METHOD_1 = 0;
constexpr int CEDIT_CREATION_METHOD_2 = 1;
constexpr int CEDIT_CREATION_METHOD_3 = 2;
constexpr int CEDIT_CREATION_METHOD_4 = 3;
constexpr int CEDIT_CREATION_METHOD_5 = 4;
constexpr int NUM_CREATION_METHODS    = 5;

extern const char *const creation_methods[NUM_CREATION_METHODS + 1];

