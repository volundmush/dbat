/*
 * The loaders' fix-ups: see load_rules.hpp.
 */
#include "load_rules.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/extradesc.hpp"

namespace rules {

void gravity_by_flags(GameWorld *g, room_vnum room)
{
  if (room_flagged(g, room, ROOM_VEGETA) || room_flagged(g, room, ROOM_GRAVITYX10))
    room_set_gravity(g, room, 10);
}

void gravity_by_vnum(GameWorld *g, room_vnum room)
{
  static constexpr struct {
    room_vnum low, high;
    int gravity;
  } ranges[] = {
      {19800, 19899, 1000 },
      {64000, 64006, 100  },
      {64007, 64016, 300  },
      {64017, 64030, 500  },
      {64031, 64048, 1000 },
      {64049, 64070, 5000 },
      {64071, 64096, 10000},
      {64097, 64097, 1000 },
  };
  for (const auto &r : ranges)
    if (room >= r.low && room <= r.high)
      room_set_gravity(g, room, r.gravity);
}

void extra_description_ends_a_line(extra_descr_data &descr)
{
  if (!descr.description.empty() && descr.description.back() != '\n')
    descr.description += "\r\n";
}

} // namespace rules
