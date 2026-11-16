#pragma once
/*
 * The fields a script reads off a character, object or room, as in %actor.gold% or %self.room%, and the values
 * of the variables that need no field (%self%, %send%) or are special (%random.char%, %time.hour%).
 *
 * Each kind of entity has a table of its field names, each with the function that works the field out. A field
 * given an argument in brackets, as in %actor.gold(10)%, may change the entity as well. A field not in the table
 * is looked for among the entity's global variables, and failing that it gives nothing and is logged.
 *
 * Adding a field is adding a line to the table in fields_char.cpp, fields_obj.cpp or fields_room.cpp.
 */
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "dg_internal.hpp"

namespace dg {

/* One field being read: who asks, the field's name, and its argument in brackets ("" if none) */
struct field_query {
  GameWorld *g;
  owner_ptr go; /* whose script reads it */
  trig_data *trig;
  int type; /* go's trigger type, for the logs */
  const std::string &field;
  std::string &subfield; /* some of the old lookups lower-case it in place, which is why it isn't const */

  bool has_arg() const { return !subfield.empty(); }
  /* The argument for the lookups that still take a char * */
  char *arg() const { return subfield.data(); }
};

/*
 * A field of an entity of type T. get gives the value, or nothing when the field turns out not to be one after
 * all (a few take an argument that makes them so), in which case it is looked for among the entity's globals.
 */
template <class T>
struct field {
  std::string_view name;
  std::optional<std::string> (*get)(const field_query &, T *);
};

/* The entry for name in table, by the name without regard to case, or nullptr */
template <class T>
const field<T> *find_field(std::span<const field<T>> table, std::string_view name)
{
  for (const field<T> &f : table)
    if (str_eq_nocase(f.name, name))
      return &f;
  return nullptr;
}

/* A DG UID as a script sees it, as in %actor% */
std::string uid_text(long id);

/* The value of field on c, o or r */
std::string char_field(const field_query &q, char_data *c);
std::string obj_field(const field_query &q, obj_data *o);
std::string room_field(const field_query &q, room_data *r);

/* The value of a variable named var that has no field, as the script sc, of trigger type type, sees it */
std::string plain_variable(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, int type, const std::string &var);

/* How many objects in list, and in the containers in it, item names: by UID, by vnum or by name */
int item_in_list(GameWorld *g, const char *item, const id_list &list);

} // namespace dg
