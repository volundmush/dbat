#pragma once

#include "characters.h"
#include "objects.h"
#include "rooms.h"
#include "zones.h"
#include "shops.h"
#include "guilds.h"
#include "dgscripts.h"

template <typename Func>
inline void mob_proto_iterate(Func &&func)
{
  void *iterator = mob_proto_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct char_data *mob = mob_proto_next(iterator);
    if (!mob) {
      break;
    }
    if (!func(mob)) {
      break;
    }
  }

  mob_proto_iterator_free(iterator);
}

inline void mob_proto_iterate(bool (*func)(struct char_data *mob))
{
  if (!func) {
    return;
  }
  mob_proto_iterate([&](struct char_data *mob) { return func(mob); });
}
