/* Two GameWorlds built by hand must not affect each other. Needs no data/ files. */
#include <stdio.h>
#include <stdlib.h>
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/descriptor.h"
#include "dbat/dg_event.h"
#include "dbat/genshp.h"
#include "dbat/guild.h"
#include "dbat/handler.h"
#include "dbat/local_limits.h"
#include "dbat/object_systems.h"
#include "dbat/index.h"
#include "dbat/random.h"
#include "dbat/relocate.h"
#include "dbat/search.h"
#include "dbat/shop.h"
#include "dbat/vehicles.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/comm.h"
#include "dbat/game_world.h"

static int failures = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      failures++;                                                \
    }                                                            \
  } while (0)

/* Rooms, one mob and object prototype, one zone and one shop, all numbered from first_vnum. */
static GameWorld *make_world(room_vnum first_vnum, int count)
{
  GameWorld *g = game_world_create();

  CREATE(g->rooms, struct room_data, count);
  g->top_room = count - 1;
  for (room_rnum r = 0; r < count; r++)
    rroom_set_number(g, r, first_vnum + r);

  CREATE(g->mob_proto, struct char_data, 1);
  CREATE(g->mob_index, struct index_data, 1);
  clear_char(g->mob_proto);
  char_set_nr(g->mob_proto, 0);
  char_mob_set(g->mob_proto, MOB_ISNPC, true);
  g->mob_index[0].vnum = first_vnum;
  g->top_mob           = 0;

  CREATE(g->obj_proto, struct obj_data, 1);
  CREATE(g->obj_index, struct index_data, 1);
  clear_object(g->obj_proto);
  obj_set_item_number(g->obj_proto, 0);
  g->obj_index[0].vnum = first_vnum;
  g->top_obj           = 0;

  CREATE(g->zones, struct zone_data, 1);
  g->zones[0].number = first_vnum / 100;
  g->zones[0].bot    = first_vnum;
  g->zones[0].top    = first_vnum + 99;
  g->top_zone        = 0;

  CREATE(g->shops, struct shop_data, 1);
  g->shops[0].vnum = first_vnum;
  g->top_shop      = 0;

  init_lookup_table(g);
  circle_srandom(g, 12345); /* an unseeded generator repeats one value forever */
  return g;
}

static void free_world(GameWorld *g)
{
  free(g->shops);
  free(g->zones);
  free(g->obj_index);
  free(g->obj_proto);
  free(g->mob_index);
  free(g->mob_proto);
  free(g->rooms);
  game_world_destroy(g);
}

static void check_starting_values(void)
{
  GameWorld *g = game_world_create();

  CHECK(g->top_shop == -1);
  CHECK(g->top_guild == -1);
  CHECK(g->top_of_socialt == -1);
  CHECK(g->TOPCOUNTDOWN == 60);
  CHECK(g->exp_multiplier == 1.0f);
  CHECK(g->max_mob_id == MOB_ID_BASE);
  CHECK(g->max_obj_id == OBJ_ID_BASE);
  CHECK(g->SHADOW_DRAGON1 == -1);
  CHECK(g->SHADOW_DRAGON2 == -1);
  CHECK(g->SHADOW_DRAGON3 == -1);
  CHECK(g->SHADOW_DRAGON4 == -1);
  CHECK(g->SHADOW_DRAGON5 == -1);
  CHECK(g->SHADOW_DRAGON6 == -1);
  CHECK(g->SHADOW_DRAGON7 == -1);
  game_world_destroy(g);
}

/* each world searches only its own tables */
static void check_lookups(GameWorld *a, GameWorld *b)
{
  CHECK(real_room(a, 101) == 1);
  CHECK(real_room(b, 101) == NOWHERE);
  CHECK(real_room(b, 201) == 1);
  CHECK(real_room(a, 201) == NOWHERE);

  CHECK(real_mobile(a, 100) == 0);
  CHECK(real_mobile(b, 100) == NOBODY);
  CHECK(real_object(b, 200) == 0);
  CHECK(real_object(a, 200) == NOTHING);
  CHECK(real_zone(a, 1) == 0);
  CHECK(real_zone(b, 1) == NOWHERE);
  CHECK(real_zone_by_thing(a, 150) == 0);
  CHECK(real_zone_by_thing(b, 150) == NOWHERE);
  CHECK(real_shop(a, 100) == 0);
  CHECK(real_shop(b, 100) == NOWHERE);
}

/* a character and an object placed in A appear only in A */
static void check_rooms(GameWorld *a, GameWorld *b)
{
  struct char_data *ch;
  struct obj_data *obj;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  CREATE(obj, struct obj_data, 1);
  clear_object(obj);

  char_to_room(a, ch, 1);
  obj_to_room(a, obj, 2);
  CHECK(char_get_in_room(ch) == 1);
  CHECK(obj_get_in_room(obj) == 2);
  CHECK(rroom_get_people(a, 1) == ch);
  CHECK(rroom_get_contents(a, 2) == obj);
  CHECK(rroom_get_people(b, 1) == nullptr);
  CHECK(rroom_get_contents(a, 1) == nullptr);

  /* the room count is per world too: room 2 exists in A but not in B */
  char_from_room(a, ch);
  CHECK(rroom_get_people(a, 1) == nullptr);
  CHECK(char_get_in_room(ch) == NOWHERE);
  char_to_room(b, ch, 2);
  CHECK(char_get_in_room(ch) == NOWHERE);
  char_to_room(a, ch, 2);
  CHECK(rroom_get_people(a, 2) == ch);
  CHECK(rroom_get_people(b, 1) == nullptr);

  char_from_room(a, ch);
  obj_from_room(a, obj);
  free(obj);
  free(ch);
}

/* entities made from a prototype join only the world they were made in */
static void check_read_entities(GameWorld *a, GameWorld *b)
{
  struct char_data *mob = read_mobile(a, 100, VIRTUAL);
  struct obj_data *obj  = read_object(a, 100, VIRTUAL);

  CHECK(mob != nullptr);
  CHECK(obj != nullptr);
  CHECK(a->characters == mob);
  CHECK(a->objects == obj);
  CHECK(a->mob_index[0].number == 1);
  CHECK(a->obj_index[0].number == 1);
  CHECK(char_get_mob_vnum(a, mob) == 100);
  CHECK(obj_get_vnum(a, obj) == 100);

  CHECK(b->characters == nullptr);
  CHECK(b->objects == nullptr);
  CHECK(b->mob_index[0].number == 0);
  CHECK(b->obj_index[0].number == 0);
  CHECK(read_mobile(b, 100, VIRTUAL) == nullptr);
  CHECK(read_object(b, 100, VIRTUAL) == nullptr);

  CHECK(find_char(a, char_get_id(mob)) == mob);
  CHECK(find_char(b, char_get_id(mob)) == nullptr);

  remove_from_lookup_table(a, char_get_id(mob));
  remove_from_lookup_table(a, obj_get_id(obj));
  a->characters = nullptr;
  a->objects    = nullptr;
  free(mob);
  free(obj);
}

/* the same seed gives the same rolls, and rolling in one world leaves the other where it was */
static void check_rng(GameWorld *a, GameWorld *b)
{
  unsigned long first, second;

  circle_srandom(a, 42);
  circle_srandom(b, 42);
  first = circle_random(a);
  CHECK(first == circle_random(b));

  second = circle_random(a);
  (void)circle_random(a);
  (void)circle_random(a);
  CHECK(second == circle_random(b));

  circle_srandom(a, 42);
  CHECK(circle_random(a) == first);
  CHECK(rand_number(a, 1, 100) >= 1);
}

/* a random colour code rolls on the world's own generator, once */
static void check_random_color(GameWorld *a, GameWorld *b)
{
  struct descriptor_data d = {0};
  struct char_data *mob    = read_mobile(a, 100, VIRTUAL);
  unsigned long second;

  d.output    = d.small_outbuf;
  d.bufspace  = SMALL_BUFSIZE - 1;
  d.character = mob;

  circle_srandom(a, 42);
  circle_srandom(b, 42);
  write_to_output(a, &d, "@xrandom colour");
  (void)circle_random(b);
  second = circle_random(b);
  CHECK(circle_random(a) == second);

  remove_from_lookup_table(a, char_get_id(mob));
  a->mob_index[0].number--;
  a->characters = nullptr;
  free(mob);
}

static EVENTFUNC(count_event)
{
  ++*(int *)event_obj;
  return 0;
}

/* time, weather, connections and event queues are per world */
static void check_other_state(GameWorld *a, GameWorld *b)
{
  struct descriptor_data d = {0};
  int ran                  = 0;

  a->time_info.hours            = 7;
  a->weather_info.pressure      = 1000;
  a->descriptors                = &d;
  a->config_info.play.level_cap = 50;
  CHECK(b->time_info.hours == 0);
  CHECK(b->weather_info.pressure == 0);
  CHECK(b->descriptors == nullptr);
  CHECK(b->config_info.play.level_cap == 0);
  a->descriptors = nullptr;

  event_init(a);
  event_init(b);
  event_create(a, count_event, &ran, 1);
  a->pulse++;
  b->pulse++;
  event_process(b);
  CHECK(ran == 0);
  event_process(a);
  CHECK(ran == 1);
  event_free_all(a);
  event_free_all(b);
}

/* An object in a world where its vnum exists, carried by holder. The caller frees the world. */
static struct obj_data *carried_object(GameWorld *g, struct char_data *holder)
{
  struct obj_data *obj = read_object(g, g->obj_index[0].vnum, VIRTUAL);

  obj_to_char(g, obj, holder);
  return obj;
}

/* Objects off the ground used to make these updates read and write rooms[-1]; ASan catches those. */
static void check_objects_off_the_ground(void)
{
  struct char_data *holder;
  struct obj_data *obj, *bag;
  GameWorld *g;

  CREATE(holder, struct char_data, 1);
  clear_char(holder);
  CHECK(!char_ether_stream(nullptr, holder)); /* no world needed: it never looks a room up */

  /* a portal or an ashcloud whose timer runs out is still removed */
  g   = make_world(1306, 1);
  obj = carried_object(g, holder);
  obj_set_timer(obj, 1);
  point_update(g);
  CHECK(g->objects == nullptr);
  free_world(g);

  g   = make_world(500, 1);
  obj = carried_object(g, holder);
  obj_set_type_flag(obj, ITEM_PORTAL);
  obj_set_timer(obj, 1);
  point_update(g);
  CHECK(g->objects == nullptr);
  free_world(g);

  /* a carried Glacial Wall doesn't melt */
  g   = make_world(79, 1);
  obj = carried_object(g, holder);
  obj_extra_set(obj, ITEM_ICE, true);
  obj_set_weight(obj, 500);
  for (int i = 0; i < 20; i++)
    point_update(g);
  CHECK(obj_get_weight(obj) == 500);
  obj_from_char(g, obj);
  extract_obj(g, obj);
  free_world(g);

  /* a gravity generator in a container changes nothing */
  g   = make_world(11, 1);
  bag = read_object(g, 11, VIRTUAL);
  obj = read_object(g, 11, VIRTUAL);
  obj_to_room(g, bag, 0);
  obj_to_obj(g, obj, bag);
  obj_set_weight(obj, 7);
  obj_set_value(bag, VAL_ALL_HEALTH, 100);
  for (int i = 0; i < 20; i++)
    broken_update(g);
  CHECK(obj_get_weight(obj) == 7);
  extract_obj(g, bag);
  free_world(g);

  /* a carried vehicle can't be found as somewhere to fly out of */
  g   = make_world(600, 1);
  obj = carried_object(g, holder);
  obj_set_type_flag(obj, ITEM_VEHICLE);
  CHECK(find_vehicle_by_vnum(g, 600) == nullptr);
  obj_from_char(g, obj);
  obj_to_room(g, obj, 0);
  CHECK(find_vehicle_by_vnum(g, 600) == obj);
  extract_obj(g, obj);
  free_world(g);

  /* nobody is listening nowhere */
  g = make_world(700, 1);
  send_to_room(g, NOWHERE, "unheard");
  free_world(g);

  free(holder);
}

static void check_get_number(void)
{
  char text[]  = "2.a long enough name to overlap";
  char plain[] = "bread";
  char *name   = text;

  CHECK(get_number(&name) == 2);
  CHECK(!strcmp(name, "a long enough name to overlap"));
  name = plain;
  CHECK(get_number(&name) == 1);
  CHECK(!strcmp(name, "bread"));
}

int main(void)
{
  GameWorld *a = make_world(100, 3);
  GameWorld *b = make_world(200, 2);

  logfile = stderr; /* the logger stays global so crash handlers can use it; several checks below log */

  check_starting_values();
  check_lookups(a, b);
  check_rooms(a, b);
  check_read_entities(a, b);
  check_rng(a, b);
  check_random_color(a, b);
  check_other_state(a, b);
  check_objects_off_the_ground();
  check_get_number();

  free_world(b);
  free_world(a);

  if (failures) {
    printf("%d check(s) failed\n", failures);
    return 1;
  }
  puts("game_world: all checks passed");
  return 0;
}
