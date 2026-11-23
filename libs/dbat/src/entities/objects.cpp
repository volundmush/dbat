/*
 * Objects: made from a prototype or from nothing, freed, and their unique ids, which find duplicated objects; and
 * the prototype searches of "vnum".
 */
#include <cstdlib>

#include "dbat/objects.hpp"
#include "dbat/iterate.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/stringutils.hpp"

/* You can define this to anything you want; 1 would work but it would
   be very inefficient. I would recommend that it actually be close to
   your total number of in-game objects if not double or triple it just
   to minimize collisions. The only O(n) [n=NUM_OBJ_UNIQUE_POOLS]
   operation is initialization of the hash table, all other operations
   that have to traverse are O(n) [n=num elements in pool], so more
   pools are better.
     - Elie Rosenblum Dec. 12 2003 */
constexpr int NUM_OBJ_UNIQUE_POOLS = 5000;


void init_obj_unique_hash(GameWorld *g)
{
  g->obj_unique_hash_pools = new obj_unique_hash_elem *[NUM_OBJ_UNIQUE_POOLS]();
}

int vnum_object(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (isname(searchname, obj_get_name(obj_proto_of(g, nr))))
      send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                   ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                   !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");

  return (found);
}

int vnum_material(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (isname(searchname, material_names[obj_get_value(obj_proto_of(g, nr), VAL_ALL_MATERIAL)])) {
      send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                   ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                   !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");
    }

  return (found);
}

int vnum_weapontype(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (obj_get_type_flag(obj_proto_of(g, nr)) == ITEM_WEAPON) {
      if (isname(searchname, weapon_type[obj_get_value(obj_proto_of(g, nr), VAL_WEAPON_SKILL)])) {
        send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                     ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                     !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");
      }
    }

  return (found);
}

int vnum_armortype(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (obj_get_type_flag(obj_proto_of(g, nr)) == ITEM_ARMOR) {
      if (isname(searchname, armor_type[obj_get_value(obj_proto_of(g, nr), VAL_ARMOR_SKILL)])) {
        send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                     ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                     !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");
      }
    }

  return (found);
}

struct obj_unique_hash_elem {
  time_t generation{};
  int64_t unique_id{};
  struct obj_data *obj{};
  struct obj_unique_hash_elem *next_e{};
};

void free_obj_unique_hash(GameWorld *g)
{
  int i;
  struct obj_unique_hash_elem *elem;
  struct obj_unique_hash_elem *next_elem;
  if (g->obj_unique_hash_pools) {
    for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
      elem = g->obj_unique_hash_pools[i];
      while (elem) {
        next_elem = elem->next_e;
        delete elem;
        elem = next_elem;
      }
    }
    delete[] g->obj_unique_hash_pools;
  }
}

void add_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  int i;
  if (!g->obj_unique_hash_pools)
    init_obj_unique_hash(g);
  if (obj_get_unique_id(obj) < 0) { /* none yet: circle_random() is below 2^31, so a new one is never negative */
    if (sizeof(long long) > sizeof(long))
      obj_set_unique_id(obj, (((long long)circle_random(g)) << (sizeof(long long) * 4)) +
                       circle_random(g));
    else
      obj_set_unique_id(obj, circle_random(g));
  }
  if (CONFIG_ALL_ITEMS_UNIQUE) {
    if (!obj_extra_flagged(obj, ITEM_UNIQUE_SAVE))
      obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);
  }
  elem                        = new obj_unique_hash_elem();
  elem->generation         = obj_get_generation(obj);
  elem->unique_id          = obj_get_unique_id(obj);
  elem->obj                = obj;
  i                        = obj_get_unique_id(obj) % NUM_OBJ_UNIQUE_POOLS;
  elem->next_e             = g->obj_unique_hash_pools[i];
  g->obj_unique_hash_pools[i] = elem;
}

static void remove_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem, **ptr, *tmp;

  if (obj == nullptr || obj_get_unique_id(obj) < 0)
    return;

  ptr = g->obj_unique_hash_pools + (obj_get_unique_id(obj) % NUM_OBJ_UNIQUE_POOLS);

  if (!(ptr && *ptr))
    return;

  elem = *ptr;

  while (elem) {
    tmp = elem->next_e;
    if (elem->obj == obj) {
      delete elem;
      *ptr = tmp;
    } else {
      ptr = &(elem->next_e);
    }
    elem = tmp;
  }
}

void obj_retire(GameWorld *g, struct obj_data *obj)
{
  remove_unique_id(g, obj);
  obj_unregister(g, obj);
}

static void log_dupe_objects(GameWorld *g, struct obj_data *obj1, struct obj_data *obj2)
{
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: Dupe object found: {} [{:d}] [{:d}:{:d}]",
         or_if_empty(obj_get_short_description(obj1), "<No name>"),
         obj_get_vnum(g, obj1), (long long)obj_get_generation(obj1), obj_get_unique_id(obj1));
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: First: In room: {:d} ({}), In object: {}, Carried by: {}, Worn by: {}",
         obj_get_in_room(obj1),
         obj_get_in_room(obj1) == NOWHERE ? "Nowhere" : room_get_name(g, obj_get_in_room(obj1)),
         obj_get_in_obj(obj1) ? obj_get_short_description(obj_get_in_obj(obj1)) : "None",
         obj_get_carried_by(obj1) ? char_get_name(obj_get_carried_by(obj1)) : "Nobody",
         obj_get_worn_by(obj1) ? char_get_name(obj_get_worn_by(obj1)) : "Nobody");
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: Newer: In room: {:d} ({}), In object: {}, Carried by: {}, Worn by: {}",
         obj_get_in_room(obj2),
         obj_get_in_room(obj2) == NOWHERE ? "Nowhere" : room_get_name(g, obj_get_in_room(obj2)),
         obj_get_in_obj(obj2) ? obj_get_short_description(obj_get_in_obj(obj2)) : "None",
         obj_get_carried_by(obj2) ? char_get_name(obj_get_carried_by(obj2)) : "Nobody",
         obj_get_worn_by(obj2) ? char_get_name(obj_get_worn_by(obj2)) : "Nobody");
}

void check_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  if (!obj || obj_get_unique_id(obj) < 0) /* -1 for none; no real one is negative */
    return;
  elem = g->obj_unique_hash_pools[obj_get_unique_id(obj) % NUM_OBJ_UNIQUE_POOLS];
  while (elem) {
    if (elem->obj == obj) {
      mud_log("SYSERR: check_unique_id checking for existing object?!");
    }
    if (elem->generation == obj_get_generation(obj) && elem->unique_id == obj_get_unique_id(obj)) {
      log_dupe_objects(g, elem->obj, obj);
      obj_extra_set(obj, ITEM_PURGE, true);
    }
    elem = elem->next_e;
  }
}

std::string sprintuniques(GameWorld *g, obj_vnum low, obj_vnum high)
{
  std::string str;
  bool any = false;

  for (int i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++)
    if (g->obj_unique_hash_pools[i])
      any = true;
  if (!any)
    return "No objects in unique hash.\r\n";

  str = str_format("Unique object hashes (vnums {:d} - {:d})\r\n", low, high);
  for (int i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    bool header = false;
    for (struct obj_unique_hash_elem *q = g->obj_unique_hash_pools[i]; q; q = q->next_e)
      if (obj_get_vnum(g, q->obj) >= low && obj_get_vnum(g, q->obj) <= high) {
        if (!header) {
          header = true;
          str += str_format("|-Hash {:d}\r\n", i);
        }
        str += str_format("| |- [@g{:>6d}@n] - [@y{:>10d}:{:<19d}@n] - {}\r\n",
                          obj_get_vnum(g, q->obj), (long long)q->generation, q->unique_id,
                          or_if_empty(obj_get_short_description(q->obj), "<Unknown>"));
      }
  }
  return str;
}

/* create an object, and add it to the object list */
struct obj_data *create_obj(GameWorld *g)
{
  struct obj_data *obj;

  obj = new obj_data();
  clear_object(obj);

  obj_set_id(obj, dg_new_obj_uid(g));
  obj_register(g, obj);

  obj_set_generation(obj, game_now(g));
  obj_set_unique_id(obj, -1);

  assign_triggers(g, obj);

  return (obj);
}

/* create a new object from a prototype */
struct obj_data *read_object(GameWorld *g, obj_vnum nr)
{
  struct obj_data *obj;
  obj_vnum i = nr;

  if (!obj_proto_of(g, i)) {
    mud_log("Object (V) {:d} does not exist in database.", nr);
    return (nullptr);
  }

  obj = new obj_data();
  clear_object(obj);
  *obj = (*obj_proto_of(g, i));
  obj_set_room_loaded(obj, NOWHERE);

  obj_index_of(g, i)->number++;

  obj_set_id(obj, dg_new_obj_uid(g));
  obj_register(g, obj);

  obj_set_generation(obj, game_now(g));
  obj_set_unique_id(obj, -1);

  copy_proto_script(obj_proto_of(g, i), obj);
  assign_triggers(g, obj);
  if (obj_get_vnum(g, obj) == 65) {
    obj_set_healcharge(obj, 20);
  }
  if (obj_get_type_flag(obj) == ITEM_FOOD) {
    if (obj_get_value(obj, 1) == 0) {
      obj_set_value(obj, 1, obj_get_value(obj, VAL_FOOD_FOODVAL));
    }
    obj_set_foob(obj, obj_get_value(obj, 1));
  }
  return (obj);
}

/* release memory allocated for an obj struct */
void free_obj(GameWorld *g, struct obj_data *obj)
{
  remove_unique_id(g, obj);
  /* its strings, extra descriptions and trigger list go with it */

  /* free any assigned scripts */
  if (obj_get_script(obj))
    extract_script(g, obj);

  obj_unregister(g, obj);

  delete obj;
}

void clear_object(struct obj_data *obj)
{
  *obj = obj_data{}; /* each field's initial value: it holds containers, so it must never be memset */

  obj_set_item_number(obj, NOTHING);
  obj_set_in_room(obj, NOWHERE);
  obj_set_worn_on(obj, NOWHERE);
}
