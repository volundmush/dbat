/* What a DG script runs on: see dbat/dg/owner.hpp. */
#include "dbat/dg/owner.hpp"

#include "dbat/character_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"

long owner_uid(const owner_ptr &go)
{
  return std::visit(overloaded{[](char_data *ch) { return (long)char_get_id(ch); },
                               [](obj_data *obj) { return (long)obj_get_id(obj); },
                               [](room_data *room) { return (long)room_get_number(room) + ROOM_ID_BASE; }},
                    go);
}

script_owner script_owner::of(const owner_ptr &go)
{
  return {static_cast<owner_kind>(go.index()), owner_uid(go)};
}

std::optional<owner_ptr> script_owner::resolve(GameWorld *g) const
{
  switch (kind) {
    case owner_kind::mob:
      if (char_data *ch = char_by_id(g, id))
        return owner_ptr{ch};
      break;
    case owner_kind::obj:
      if (obj_data *obj = obj_by_id(g, id))
        return owner_ptr{obj};
      break;
    case owner_kind::room:
      if (room_data *room = room_of(g, (room_vnum)(id - ROOM_ID_BASE)))
        return owner_ptr{room};
      break;
  }
  return std::nullopt;
}
