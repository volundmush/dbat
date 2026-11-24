#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table zone_bits; /* NUM_ZONE_FLAGS of them */

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

/* The old names, until every use says zone_flag:: */
constexpr zone_flag ZONE_CLOSED   = zone_flag::closed;
constexpr zone_flag ZONE_NOIMMORT = zone_flag::noimmort;
constexpr zone_flag ZONE_QUEST    = zone_flag::quest;
constexpr zone_flag ZONE_DBALLS   = zone_flag::dballs;
constexpr zone_flag ZONE_SPARE2   = zone_flag::spare2;
constexpr zone_flag ZONE_SPARE3   = zone_flag::spare3;
constexpr zone_flag ZONE_SPARE4   = zone_flag::spare4;
constexpr zone_flag ZONE_SPARE5   = zone_flag::spare5;
constexpr zone_flag ZONE_SPARE6   = zone_flag::spare6;
constexpr zone_flag ZONE_SPARE7   = zone_flag::spare7;
constexpr zone_flag ZONE_SPARE8   = zone_flag::spare8;
constexpr zone_flag ZONE_SPARE9   = zone_flag::spare9;
constexpr zone_flag ZONE_SPARE10  = zone_flag::spare10;
constexpr zone_flag ZONE_SPARE11  = zone_flag::spare11;
constexpr zone_flag ZONE_SPARE12  = zone_flag::spare12;
constexpr zone_flag ZONE_SPARE13  = zone_flag::spare13;
constexpr zone_flag ZONE_SPARE14  = zone_flag::spare14;
constexpr zone_flag ZONE_SPARE15  = zone_flag::spare15;
constexpr zone_flag ZONE_SPARE16  = zone_flag::spare16;
constexpr zone_flag ZONE_SPARE17  = zone_flag::spare17;
constexpr zone_flag ZONE_SPARE18  = zone_flag::spare18;
constexpr zone_flag ZONE_SPARE19  = zone_flag::spare19;
constexpr zone_flag ZONE_SPARE20  = zone_flag::spare20;
constexpr zone_flag ZONE_SPARE21  = zone_flag::spare21;
constexpr zone_flag ZONE_SPARE22  = zone_flag::spare22;
constexpr zone_flag ZONE_SPARE23  = zone_flag::spare23;
constexpr zone_flag ZONE_SPARE24  = zone_flag::spare24;
constexpr zone_flag ZONE_SPARE25  = zone_flag::spare25;
constexpr zone_flag ZONE_SPARE26  = zone_flag::spare26;
constexpr zone_flag ZONE_SPARE27  = zone_flag::spare27;
constexpr zone_flag ZONE_SPARE28  = zone_flag::spare28;
constexpr zone_flag ZONE_SPARE29  = zone_flag::spare29;
constexpr zone_flag ZONE_SPARE30  = zone_flag::spare30;
constexpr zone_flag ZONE_SPARE31  = zone_flag::spare31;
constexpr zone_flag ZONE_SPARE32  = zone_flag::spare32;
constexpr zone_flag ZONE_SPARE33  = zone_flag::spare33;

constexpr int NUM_ZONE_FLAGS = static_cast<int>(flag_count<zone_flag>);
constexpr int ZF_ARRAY_MAX   = 4;
