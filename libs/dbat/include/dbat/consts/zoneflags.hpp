#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table zone_bits; /* flag_count<zone_flag> of them */

/* A zone's flags, zone_data.zone_flags */
enum class zone_flag : uint8_t {
  closed    = 0,
  noimmort  = 1,
  quest     = 2,
  dballs    = 3,
  spare2    = 4,
  spare3    = 5,
  spare4    = 6,
  spare5    = 7,
  spare6    = 8,
  spare7    = 9,
  spare8    = 10,
  spare9    = 11,
  spare10   = 12,
  spare11   = 13,
  spare12   = 14,
  spare13   = 15,
  spare14   = 16,
  spare15   = 17,
  spare16   = 18,
  spare17   = 19,
  spare18   = 20,
  spare19   = 21,
  spare20   = 22,
  spare21   = 23,
  spare22   = 24,
  spare23   = 25,
  spare24   = 26,
  spare25   = 27,
  spare26   = 28,
  spare27   = 29,
  spare28   = 30,
  spare29   = 31,
  spare30   = 32,
  spare31   = 33,
  spare32   = 34,
  spare33   = 35,
  count_    = 36,
};
template <> struct flag_traits<zone_flag> {
  static constexpr const name_table &names = zone_bits;
};

