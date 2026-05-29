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

template <typename Func>
inline void obj_proto_iterate(Func &&func)
{
  void *iterator = obj_proto_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct obj_data *obj = obj_proto_next(iterator);
    if (!obj) {
      break;
    }
    if (!func(obj)) {
      break;
    }
  }

  obj_proto_iterator_free(iterator);
}

inline void obj_proto_iterate(bool (*func)(struct obj_data *obj))
{
  if (!func) {
    return;
  }
  obj_proto_iterate([&](struct obj_data *obj) { return func(obj); });
}

template <typename Func>
inline void trig_proto_iterate(Func &&func)
{
  void *iterator = trig_proto_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct trig_data *trig = trig_proto_next(iterator);
    if (!trig) {
      break;
    }
    if (!func(trig)) {
      break;
    }
  }

  trig_proto_iterator_free(iterator);
}

inline void trig_proto_iterate(bool (*func)(struct trig_data *trig))
{
  if (!func) {
    return;
  }
  trig_proto_iterate([&](struct trig_data *trig) { return func(trig); });
}

template <typename Func>
inline void room_iterate(Func &&func)
{
  void *iterator = room_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct room_data *room = room_next(iterator);
    if (!room) {
      break;
    }
    if (!func(room)) {
      break;
    }
  }

  room_iterator_free(iterator);
}

inline void room_iterate(bool (*func)(struct room_data *room))
{
  if (!func) {
    return;
  }
  room_iterate([&](struct room_data *room) { return func(room); });
}

template <typename Func>
inline void zone_iterate(Func &&func)
{
  void *iterator = zone_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct zone_data *zone = zone_next(iterator);
    if (!zone) {
      break;
    }
    if (!func(zone)) {
      break;
    }
  }

  zone_iterator_free(iterator);
}

inline void zone_iterate(bool (*func)(struct zone_data *zone))
{
  if (!func) {
    return;
  }
  zone_iterate([&](struct zone_data *zone) { return func(zone); });
}

template <typename Func>
inline void shop_iterate(Func &&func)
{
  void *iterator = shop_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct shop_data *shop = shop_next(iterator);
    if (!shop) {
      break;
    }
    if (!func(shop)) {
      break;
    }
  }

  shop_iterator_free(iterator);
}

inline void shop_iterate(bool (*func)(struct shop_data *shop))
{
  if (!func) {
    return;
  }
  shop_iterate([&](struct shop_data *shop) { return func(shop); });
}

template <typename Func>
inline void guild_iterate(Func &&func)
{
  void *iterator = guild_iterator_create();
  if (!iterator) {
    return;
  }

  for (;;) {
    struct guild_data *guild = guild_next(iterator);
    if (!guild) {
      break;
    }
    if (!func(guild)) {
      break;
    }
  }

  guild_iterator_free(iterator);
}

inline void guild_iterate(bool (*func)(struct guild_data *guild))
{
  if (!func) {
    return;
  }
  guild_iterate([&](struct guild_data *guild) { return func(guild); });
}
