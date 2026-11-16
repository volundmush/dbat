/*
 * DG Scripts: the fields of an object, as in %self.vnum% or %obj.cost(10)%. See fields.hpp.
 */
#include <algorithm>
#include <cstdlib>

#include "dbat/utils.hpp"
#include "dbat/iterate.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/flags.hpp"
#include "dbat/handler.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "fields.hpp"

namespace dg {
namespace {

using result = std::optional<std::string>;

std::string number(long long n) { return str_format("{:d}", n); }

/* Whether the flag the argument names is set in flags, of the numflags in names; "0" without an argument */
result has_flag(const field_query &q, bitvector_t *flags, int numflags, const char *const names[])
{
  if (!q.has_arg())
    return "0";
  /* quirk: the first flag of the list never counts as set, as check_flags_by_name_ar gives its index */
  return check_flags_by_name_ar(flags, numflags, q.arg(), names) > 0 ? "1" : "0";
}

/* Turns the flag the argument names over, if it is set, giving "1"; otherwise it isn't a field after all */
result toggle_flag(const field_query &q, bitvector_t *flags, int numflags, const char *const names[], obj_data *o,
                   bool (*toggle)(obj_data *, int))
{
  if (!q.has_arg())
    return std::nullopt;
  int ns = check_flags_by_name_ar(flags, numflags, q.arg(), names);
  if (ns <= 0)
    return std::nullopt;
  toggle(o, ns);
  return "1";
}

/* The objects in a container: how many the argument names; "0" for what's no container */
int count_in(const field_query &q, obj_data *o)
{
  return item_in_list(q.g, q.arg(), obj_get_contains(o));
}

/* value N of the object's values */
template <int N>
result value_field(const field_query &, obj_data *o)
{
  return number(obj_get_value(o, N));
}

/* The fields, by name */
const field<obj_data> obj_fields[] = {
    {"affects", [](const field_query &q, obj_data *o) { return has_flag(q, obj_get_bitvector(o), NUM_AFF_FLAGS, affected_bits); }},
    {"carried_by", [](const field_query &, obj_data *o) -> result {
       return obj_get_carried_by(o) ? uid_text(char_get_id(obj_get_carried_by(o))) : "";
     }},
    {"contents", [](const field_query &, obj_data *o) -> result {
       return obj_get_contains(o) ? uid_text(obj_get_contains(o).newest()) : "";
     }},
    {"cost", [](const field_query &q, obj_data *o) -> result {
       if (q.has_arg())
         obj_set_cost(o, std::max(0, atoi(q.arg()) + obj_get_cost(o)));
       return number(obj_get_cost(o));
     }},
    {"cost_per_day", [](const field_query &q, obj_data *o) -> result {
       if (q.has_arg())
         obj_set_cost_per_day(o, std::max(0, atoi(q.arg()) + obj_get_cost_per_day(o)));
       return number(obj_get_cost_per_day(o));
     }},
    {"count", [](const field_query &q, obj_data *o) -> result { /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
       return obj_get_type_flag(o) == ITEM_CONTAINER ? number(count_in(q, o)) : "0";
     }},
    {"extra", [](const field_query &q, obj_data *o) { return has_flag(q, obj_get_extra_flags(o), NUM_ITEM_FLAGS, extra_bits); }},
    {"has_in", [](const field_query &q, obj_data *o) -> result {
       return obj_get_type_flag(o) == ITEM_CONTAINER && count_in(q, o) ? "1" : "0";
     }},
    {"health", [](const field_query &q, obj_data *o) -> result {
       if (q.has_arg()) {
         obj_set_value(o, VAL_ALL_HEALTH, std::max(1, atoi(q.arg()) + obj_get_value(o, VAL_ALL_HEALTH)));
         if (obj_extra_flagged(o, ITEM_BROKEN) && obj_get_value(o, VAL_ALL_HEALTH) >= 100)
           obj_extra_set(o, ITEM_BROKEN, false);
       }
       return number(obj_get_value(o, VAL_ALL_HEALTH));
     }},
    {"id", [](const field_query &, obj_data *o) -> result { return number(obj_get_id(o)); }},
    {"is_inroom", [](const field_query &, obj_data *o) -> result {
       return obj_get_in_room(o) != NOWHERE ? uid_text(obj_get_in_room(o) + ROOM_ID_BASE) : "";
     }},
    {"is_pc", [](const field_query &, obj_data *) -> result { return "-1"; }},
    {"itemflag", [](const field_query &q, obj_data *o) -> result {
       if (!q.has_arg())
         return "0";
       int item = get_flag_by_name(extra_bits, q.arg());
       return item != NOFLAG && obj_extra_flagged(o, item) ? "1" : "0";
     }},
    {"level", [](const field_query &, obj_data *o) -> result { return number(obj_get_level(o)); }},
    {"name", [](const field_query &q, obj_data *o) -> result {
       if (!q.has_arg())
         return str_format("{}", obj_get_name(o));
       obj_set_name(o, str_format("{} {}", obj_get_name(o), q.subfield).c_str());
       return std::nullopt; /* quirk: renaming gives the value of a global called "name", or logs */
     }},
    {"next_in_list", [](const field_query &q, obj_data *o) -> result {
       obj_data *next = obj_next_in_list(q.g, o);
       return next ? uid_text(obj_get_id(next)) : "";
     }},
    {"room", [](const field_query &, obj_data *o) -> result {
       return obj_room(o) != NOWHERE ? uid_text(obj_room(o) + ROOM_ID_BASE) : "";
     }},
    {"setaffects", [](const field_query &q, obj_data *o) {
       return toggle_flag(q, obj_get_bitvector(o), NUM_AFF_FLAGS, affected_bits, o, obj_aff_toggle);
     }},
    {"setextra", [](const field_query &q, obj_data *o) {
       return toggle_flag(q, obj_get_extra_flags(o), NUM_ITEM_FLAGS, extra_bits, o, obj_extra_toggle);
     }},
    {"shortdesc", [](const field_query &q, obj_data *o) -> result {
       if (!q.has_arg())
         return str_format("{}", obj_get_short_description(o));
       obj_set_short_description(o, str_format("{} @wnicknamed @D(@C{}@D)@n", obj_get_short_description(o), q.subfield).c_str());
       return std::nullopt; /* quirk: as for name */
     }},
    {"size", [](const field_query &q, obj_data *o) -> result {
       if (q.has_arg()) {
         int ns = search_block(q.arg(), size_names, false);
         if (ns > -1)
           obj_set_size(o, ns);
       }
       return sprinttype(obj_get_size(o), size_names);
     }},
    {"timer", [](const field_query &, obj_data *o) -> result { return number(obj_get_timer(o)); }},
    {"type", [](const field_query &, obj_data *o) -> result { return sprinttype(obj_get_type_flag(o), item_types); }},
    {"val0", value_field<0>},
    {"val1", value_field<1>},
    {"val2", value_field<2>},
    {"val3", value_field<3>},
    {"val4", value_field<4>},
    {"val5", value_field<5>},
    {"val6", value_field<6>},
    {"val7", value_field<7>},
    {"vnum", [](const field_query &q, obj_data *o) -> result {
       if (q.has_arg())
         return number((int)(obj_get_vnum(q.g, o) == atoi(q.arg())));
       return number(obj_get_vnum(q.g, o));
     }},
    {"weight", [](const field_query &q, obj_data *o) -> result {
       if (q.has_arg()) {
         int addition  = atoi(q.arg());
         int64_t before = obj_get_weight(o);
         /* an argument of 0, or what isn't a number, makes it weigh nothing */
         obj_set_weight(o, addition ? std::max<int64_t>(0, addition + obj_get_weight(o)) : 0);
         obj_weight_changed(q.g, o, obj_get_weight(o) - before);
       }
       return number(obj_get_weight(o));
     }},
    {"worn_by", [](const field_query &, obj_data *o) -> result {
       return obj_get_worn_by(o) ? uid_text(char_get_id(obj_get_worn_by(o))) : "";
     }},
};

/* A trigger by this name renames objects with name(...), and the warning that gives isn't logged for it */
constexpr std::string_view rename_trigger = "Rename Object";

} // namespace

std::string obj_field(const field_query &q, obj_data *o)
{
  if (const field<obj_data> *f = find_field<obj_data>(obj_fields, q.field)) {
    if (result value = f->get(q, o))
      return *value;
  } else if (tolower(static_cast<unsigned char>(q.field[0])) == 'e')
    /* quirk: any other field starting with e gives the extra flags */
    return sprintbitarray(obj_get_extra_flags(o), extra_bits, EF_ARRAY_MAX);

  /* not a field: one of o's global variables */
  if (obj_get_script(o))
    if (const variable *vd = obj_get_script(o)->global_vars.find(q.field))
      return vd->value;
  if (!str_eq_nocase(q.trig->name, rename_trigger))
    script_log(q.g, "Trigger: {}, VNum {:d}, type: {:d}. unknown object field: '{}'", q.trig->name,
               trig_get_vnum(q.g, q.trig), q.type, q.field);
  return "";
}

} // namespace dg
