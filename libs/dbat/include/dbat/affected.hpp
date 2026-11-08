#pragma once
#include <cstdint>

#include "dbat/consts/types.hpp"

struct obj_affected_type {
  int location{}; /* Which ability to change (APPLY_XXX) */
  int specific{}; /* Some locations have parameters      */
  int modifier{}; /* How much it changes by              */
};

/* An affect structure. */
struct affected_type {
  int16_t type{};          /* The type of spell that caused this      */
  int16_t duration{};      /* For how long its effects will last      */
  int modifier{};          /* This is added to apropriate ability     */
  int location{};          /* Tells which ability to change(APPLY_XXX)*/
  int specific{};          /* Some locations have parameters          */
  bitvector_t bitvector{}; /* Tells which bits to set (AFF_XXX) */
};

