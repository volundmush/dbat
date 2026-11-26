/*
 * The loaders' fix-ups: see load_rules.hpp.
 */
#include <array>
#include "load_rules.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/extradesc.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/feats.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/utils.hpp"

namespace rules {

void gravity_by_flags(GameWorld *g, room_vnum room)
{
  if (room_flagged(g, room, room_flag::vegeta) || room_flagged(g, room, room_flag::gravityx10))
    room_set_gravity(g, room, 10);
}

void gravity_by_vnum(GameWorld *g, room_vnum room)
{
  struct ranges_entry {
    room_vnum low, high;
    int gravity;
  };
  static constexpr auto ranges = std::to_array<ranges_entry>({
      {19800, 19899, 1000 },
      {64000, 64006, 100  },
      {64007, 64016, 300  },
      {64017, 64030, 500  },
      {64031, 64048, 1000 },
      {64049, 64070, 5000 },
      {64071, 64096, 10000},
      {64097, 64097, 1000 },
  });
  for (const auto &r : ranges)
    if (room >= r.low && room <= r.high)
      room_set_gravity(g, room, r.gravity);
}

void extra_description_ends_a_line(extra_descr_data &descr)
{
  if (!descr.description.empty() && descr.description.back() != '\n')
    descr.description += "\r\n";
}

void article_in_lowercase(std::string &text)
{
  std::string_view word = fname(text);
  if (str_eq_nocase(word, "a") || str_eq_nocase(word, "an") || str_eq_nocase(word, "the"))
    text[0] = static_cast<char>(tolower(static_cast<unsigned char>(text[0])));
}

void mob_class_in_range(char_data *mob, mob_vnum vnum)
{
  if (char_get_class(mob) < 0 || char_get_class(mob) >= NUM_CLASSES) {
    mud_log("SYSERR: Invalid class {:d} for mob #{:d}", char_get_class(mob), vnum);
    char_set_class(mob, 28); /* a commoner */
  }
}

void infravision_unless_human(char_data *mob)
{
  if (!char_is_human(mob) && !char_aff_flagged(mob, aff_flag::infravision))
    char_aff_set(mob, aff_flag::infravision, true);
}

void autobalance_starts_bare(char_data *mob)
{
  if (!char_mob_flagged(mob, mob_flag::autobalance))
    return;
  char_set_exp(mob, 0);
  char_set_armor(mob, 0);
  char_set_mob_specials_damnodice(mob, 0);
  char_set_mob_specials_damsizedice(mob, 0);
}

void not_dead_yet_cleared(char_data *mob, mob_vnum vnum)
{
  if (char_mob_flagged(mob, mob_flag::notdeadyet)) {
    mud_log("SYSERR: Mob #{:d} has reserved bit MOB_NOTDEADYET set.", vnum);
    char_mob_set(mob, mob_flag::notdeadyet, false);
  }
}

void portal_and_hatch_dcs(obj_data *obj)
{
  if ((obj_get_type_flag(obj) == ITEM_PORTAL || obj_get_type_flag(obj) == ITEM_HATCH) &&
      (!obj_get_value(obj, VAL_DOOR_DCLOCK) || !obj_get_value(obj, VAL_DOOR_DCHIDE))) {
    obj_set_value(obj, VAL_DOOR_DCLOCK, 20);
    obj_set_value(obj, VAL_DOOR_DCHIDE, 20);
  }
}

void suntzu_weapon_types(obj_data *obj)
{
  static constexpr auto suntzu = std::to_array<int>({
      WEAPON_TYPE_DAGGER, WEAPON_TYPE_SHORTSWORD, WEAPON_TYPE_LONGSWORD, WEAPON_TYPE_GREATSWORD, WEAPON_TYPE_MACE,
      WEAPON_TYPE_AXE,    WEAPON_TYPE_WHIP,       WEAPON_TYPE_SPEAR,     WEAPON_TYPE_POLEARM,    WEAPON_TYPE_UNARMED,
      WEAPON_TYPE_FLAIL,  WEAPON_TYPE_STAFF,      WEAPON_TYPE_HAMMER,
  }); /* 170 to 182 */
  int old = obj_get_value(obj, 0);
  if (obj_get_type_flag(obj) != ITEM_WEAPON || old <= 169)
    return;
  int now = old - 170 < static_cast<int>(std::size(suntzu)) ? suntzu[old - 170] : WEAPON_TYPE_UNDEFINED;
  mud_log("Converted weapon from [{:d}] to [{:d}].", old, now);
  obj_set_value(obj, 0, now);
}

void drink_container_weight(obj_data *obj)
{
  if ((obj_get_type_flag(obj) == ITEM_DRINKCON || obj_get_type_flag(obj) == ITEM_FOUNTAIN) &&
      obj_get_weight(obj) < obj_get_value(obj, 1))
    obj_set_weight(obj, obj_get_value(obj, 1) + 5);
}

void portal_timer(obj_data *obj)
{
  if (obj_get_type_flag(obj) == ITEM_PORTAL)
    obj_set_timer(obj, -1);
}

void object_charm_cleared(obj_data *obj, obj_vnum vnum)
{
  if (obj_aff_flagged(obj, aff_flag::charm)) {
    mud_log("SYSERR: Object #{:d} has reserved bit AFF_CHARM set.", vnum);
    obj_aff_set(obj, aff_flag::charm, false);
  }
}

} // namespace rules
