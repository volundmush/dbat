#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table exit_bits; /* NUM_EXIT_FLAGS of them */

/* Exit info: room_direction_data::exit_info. The world files save a door number (door_flags(), door_number()) and
   DG's wdoor the word, as letters. */
enum class exit_flag : uint8_t {
  isdoor    = 0, /* Exit is a door		*/
  closed    = 1, /* The door is closed	*/
  locked    = 2, /* The door is locked	*/
  pickproof = 3, /* Lock can't be picked	*/
  secret    = 4, /* The door is hidden        */
  count_    = 5,
};
template <> struct flag_traits<exit_flag> {
  static constexpr const name_table &names = exit_bits;
};

/* The old names, until every use says exit_flag:: */
constexpr exit_flag EX_ISDOOR    = exit_flag::isdoor;
constexpr exit_flag EX_CLOSED    = exit_flag::closed;
constexpr exit_flag EX_LOCKED    = exit_flag::locked;
constexpr exit_flag EX_PICKPROOF = exit_flag::pickproof;
constexpr exit_flag EX_SECRET    = exit_flag::secret;

constexpr int NUM_EXIT_FLAGS = static_cast<int>(flag_count<exit_flag>);
