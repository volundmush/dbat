#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table exit_bits; /* flag_count<exit_flag> of them */

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

