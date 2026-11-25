#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/consts/maximums.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table shop_bits; /* NUM_SHOP_FLAGS of them */

/* A shop's flags (shop_data::bitvector), saved in the shop file as a number */
enum class shop_flag : uint8_t {
  start_fight = 0,
  bank_money  = 1,
  allow_steal = 2,
  count_      = 3,
};
template <> struct flag_traits<shop_flag> {
  static constexpr const name_table &names = shop_bits;
};
static_assert(flag_count<shop_flag> == NUM_SHOP_FLAGS);

