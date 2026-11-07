#pragma once

#include <cstdint>

#include "dbat/consts/maximums.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/random.hpp"
#include "dbat/log.hpp"

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define countof(a) (sizeof(a) / sizeof((a)[0])) /* elements in an array (C2y has it as countof) */
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

static inline double safeDiv(double numer, double denom)
{
  return (denom == 0.0) ? 0.0 : (numer / denom);
}

static inline int64_t safeDivInt(int64_t numer, int64_t denom)
{
  return (denom == 0) ? 0 : (numer / denom);
}

static inline double clampHealth(double val)
{
  if (val < 0.0) return 0.0;
  if (val > 1.0) return 1.0;
  if (val != val) return 0.0; // NaN check
  return val;
}

static inline double fixnan(double val)
{
  return (val != val) ? 0.0 : val; // NaN check
}

/* memory utils **********************************************************/

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'cmtemp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next, cmtemp) \
  if ((item) == (head))                            \
    head = (item)->next;                           \
  else {                                           \
    cmtemp = head;                                 \
    while (cmtemp && (cmtemp->next != (item)))     \
      cmtemp = cmtemp->next;                       \
    if (cmtemp)                                    \
      cmtemp->next = (item)->next;                 \
  }

#define REMOVE_FROM_DOUBLE_LIST(item, head, next, prev) \
  if ((item) == (head)) {                               \
    head = (item)->next;                                \
    if (head) head->prev = nullptr;                     \
  } else {                                              \
    temp = head;                                        \
    while (temp && (temp->next != (item)))              \
      temp = temp->next;                                \
    if (temp) {                                         \
      temp->next = item->next;                          \
      if (item->next)                                   \
        item->next->prev = temp;                        \
    }                                                   \
  }

