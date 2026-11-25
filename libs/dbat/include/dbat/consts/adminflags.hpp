#pragma once
#include <cstdint>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table admin_bits; /* flag_count<admin_flag> of them */

/* ADM flags - define admin privs for chars, char_data.admflags */
enum class admin_flag : uint8_t {
  tellall      = 0,  /* Can use 'tell all' to broadcast GOD */
  seeinv       = 1,  /* Sees other chars inventory IMM */
  seesecret    = 2,  /* Sees secret doors IMM */
  knowweather  = 3,  /* Knows details of weather GOD */
  fullwhere    = 4,  /* Full output of 'where' command IMM */
  money        = 5,  /* Char has a bottomless wallet GOD */
  eatanything  = 6,  /* Char can eat anything GOD */
  nopoison     = 7,  /* Char can't be poisoned IMM */
  walkanywhere = 8,  /* Char has unrestricted walking IMM */
  nokeys       = 9,  /* Char needs no keys for locks GOD */
  instantkill  = 10, /* "kill" command is instant IMPL */
  nosteal      = 11, /* Char cannot be stolen from IMM */
  transall     = 12, /* Can use 'trans all' GRGOD */
  switchmortal = 13, /* Can 'switch' to a mortal PC body IMPL */
  forcemass    = 14, /* Can force rooms or all GRGOD */
  allhouses    = 15, /* Can enter any house GRGOD */
  nodamage     = 16, /* Cannot be damaged IMM */
  allshops     = 17, /* Can use all shops GOD */
  cedit        = 18, /* Can use cedit IMPL */
  count_       = 19,
};
template <> struct flag_traits<admin_flag> {
  static constexpr const name_table &names = admin_bits;
};

