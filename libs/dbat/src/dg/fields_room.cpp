/*
 * DG Scripts: the fields of a room, as in %self.vnum% or %room.north(vnum)%. See fields.hpp.
 */
#include <array>
#include <cstdlib>

#include "dbat/utils.hpp"
#include "dbat/iterate.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/game_world.hpp"

#include "fields.hpp"

namespace dg {
namespace {

using result = std::optional<std::string>;

std::string number(long long n) { return str_format("{:d}", n); }

/* %room.<exit>%: the exit's bits, or with an argument its vnum, key, bits or room; "" with no exit there */
template <int Dir>
result exit_field(const field_query &q, room_data *r)
{
  const auto *exit = room_get_dir_option(r, Dir);
  if (!exit)
    return "";
  if (!q.has_arg() || str_eq_nocase(q.subfield, "bits"))
    return flag_mask_names(exit->exit_info);
  if (str_eq_nocase(q.subfield, "vnum"))
    return number(exit->to_room);
  if (str_eq_nocase(q.subfield, "key"))
    return number(exit->key);
  if (str_eq_nocase(q.subfield, "room"))
    return exit->to_room != NOWHERE ? uid_text(exit->to_room + ROOM_ID_BASE) : "";
  return ""; /* quirk: any other argument gives nothing, and no warning */
}

/* The fields, by name */
const auto room_fields = std::to_array<field<room_data>>({
    {"contents", [](const field_query &q, room_data *r) -> result {
       if (!q.has_arg()) /* the newest thing there */
         return room_get_contents(r) ? uid_text(room_get_contents(r).newest()) : "";
       for (obj_data *obj : obj_list(q.g, room_get_contents(r)))
         if (obj_get_vnum(q.g, obj) == parse_int(q.subfield))
           return uid_text(obj_get_id(obj));
       return "";
     }},
    {"fishing", [](const field_query &q, room_data *r) -> result {
       return room_flagged(q.g, room_get_number(r), room_flag::fishing) ? "1" : "0";
     }},
    {"gravity", [](const field_query &, room_data *r) -> result { return number(room_get_gravity(r)); }},
    {"id", [](const field_query &q, room_data *r) -> result {
       return room_of(q.g, room_get_number(r)) ? number(room_get_number(r) + ROOM_ID_BASE) : "";
     }},
    {"name", [](const field_query &, room_data *r) -> result { return str_format("{}", room_get_name(r)); }},
    {"people", [](const field_query &, room_data *r) -> result {
       return room_get_people(r) ? uid_text(room_get_people(r).newest()) : "";
     }},
    {"roomflag", [](const field_query &q, room_data *r) -> result {
       if (!q.has_arg())
         return "0";
       /* quirk: the first flag (DARK) never counts as set, as check_flags_by_name_ar gave its index */
       std::optional<room_flag> flag = flag_by_name<room_flag>(q.subfield);
       return flag && *flag != room_flag::dark && room_flagged(r, *flag) ? "1" : "0";
     }},
    {"sector", [](const field_query &, room_data *r) -> result { return std::string(type_name(sector_types, room_get_sector_type(r))); }},
    {"vnum", [](const field_query &q, room_data *r) -> result {
       if (q.has_arg())
         return number((int)(room_get_number(r) == parse_int(q.subfield)));
       return number(room_get_number(r));
     }},
    {"weather", [](const field_query &q, room_data *r) -> result {
       static constexpr auto sky_look = std::to_array<std::string_view>({"sunny", "cloudy", "rainy", "lightning"});
       return room_flagged(r, room_flag::indoors) ? "" : str_format("{}", sky_look[q.g->weather_info.sky]);
     }},
    {"zonename", [](const field_query &, room_data *r) -> result { return str_format("{}", room_get_zone(r)->name); }},
    {"zonenumber", [](const field_query &, room_data *r) -> result { return number(room_get_zone(r)->number); }},
    {"north", exit_field<NORTH>},
    {"east", exit_field<EAST>},
    {"south", exit_field<SOUTH>},
    {"west", exit_field<WEST>},
    {"up", exit_field<UP>},
    {"down", exit_field<DOWN>},
    {"northwest", exit_field<NORTHWEST>},
    {"northeast", exit_field<NORTHEAST>},
    {"southwest", exit_field<SOUTHWEST>},
    {"southeast", exit_field<SOUTHEAST>},
    {"inside", exit_field<INDIR>},
    {"outside", exit_field<OUTDIR>},
});

/* A global variable of r, or nothing */
const variable *global_of(room_data *r, const std::string &name)
{
  return room_get_script(r) ? room_get_script(r)->global_vars.find(name) : nullptr;
}

} // namespace

std::string room_field(const field_query &q, room_data *r)
{
  /* the void's fields are its global variables, which are the whole game's */
  if (room_get_number(r) == 0) {
    if (!room_get_script(r)) {
      script_log(q.g, "Trigger: {}, Vnum {:d}, type {:d}. Trying to access Global var list of void. Apparently this has not been set up!",
                 q.trig ? q.trig->name.c_str() : "none", q.trig ? trig_get_vnum(q.g, q.trig) : NOTHING, q.type);
      return "";
    }
    const variable *vd = global_of(r, q.field);
    return vd ? vd->value : "";
  }

  if (const field<room_data> *f = find_field<room_data>(room_fields, q.field))
    if (result value = f->get(q, r))
      return *value;

  /* not a field: one of r's global variables */
  if (const variable *vd = global_of(r, q.field))
    return vd->value;
  script_log(q.g, "Trigger: {}, VNum {:d}, type: {:d}. unknown room field: '{}'", q.trig->name,
             trig_get_vnum(q.g, q.trig), q.type, q.field);
  return "";
}

} // namespace dg
