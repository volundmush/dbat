#pragma once
#include "types.h"

/* Exit info: used in room_data.dir_option.exit_info */
constexpr bitvector_t EX_ISDOOR		= (1u << 0); /* Exit is a door		*/
constexpr bitvector_t EX_CLOSED		= (1u << 1); /* The door is closed	*/
constexpr bitvector_t EX_LOCKED		= (1u << 2); /* The door is locked	*/
constexpr bitvector_t EX_PICKPROOF		= (1u << 3); /* Lock can't be picked	*/
constexpr bitvector_t EX_SECRET		= (1u << 4); /* The door is hidden        */

constexpr int NUM_EXIT_FLAGS = 5;

extern const char *exit_bits[NUM_EXIT_FLAGS+1];