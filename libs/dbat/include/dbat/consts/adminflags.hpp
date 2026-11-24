#pragma once
#include <cstdint>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table admin_bits; /* NUM_ADMFLAGS of them */

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

/* The old names, until every use says admin_flag:: */
constexpr admin_flag ADM_TELLALL      = admin_flag::tellall;
constexpr admin_flag ADM_SEEINV       = admin_flag::seeinv;
constexpr admin_flag ADM_SEESECRET    = admin_flag::seesecret;
constexpr admin_flag ADM_KNOWWEATHER  = admin_flag::knowweather;
constexpr admin_flag ADM_FULLWHERE    = admin_flag::fullwhere;
constexpr admin_flag ADM_MONEY        = admin_flag::money;
constexpr admin_flag ADM_EATANYTHING  = admin_flag::eatanything;
constexpr admin_flag ADM_NOPOISON     = admin_flag::nopoison;
constexpr admin_flag ADM_WALKANYWHERE = admin_flag::walkanywhere;
constexpr admin_flag ADM_NOKEYS       = admin_flag::nokeys;
constexpr admin_flag ADM_INSTANTKILL  = admin_flag::instantkill;
constexpr admin_flag ADM_NOSTEAL      = admin_flag::nosteal;
constexpr admin_flag ADM_TRANSALL     = admin_flag::transall;
constexpr admin_flag ADM_SWITCHMORTAL = admin_flag::switchmortal;
constexpr admin_flag ADM_FORCEMASS    = admin_flag::forcemass;
constexpr admin_flag ADM_ALLHOUSES    = admin_flag::allhouses;
constexpr admin_flag ADM_NODAMAGE     = admin_flag::nodamage;
constexpr admin_flag ADM_ALLSHOPS     = admin_flag::allshops;
constexpr admin_flag ADM_CEDIT        = admin_flag::cedit;

constexpr int NUM_ADMFLAGS = static_cast<int>(flag_count<admin_flag>);
constexpr int AD_ARRAY_MAX = 4;
