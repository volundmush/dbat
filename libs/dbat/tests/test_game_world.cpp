/* Two GameWorlds built by hand must not affect each other. Needs no data/ files. */
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/fileop.hpp"
#include "dbat/extract.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/guild.hpp"
#include "dbat/handler.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/object_systems.hpp"
#include "dbat/index.hpp"
#include "dbat/iterate.hpp"
#include "dbat/random.hpp"
#include "dbat/relocate.hpp"
#include "dbat/search.hpp"
#include "dbat/shop.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/game_world.hpp"

static int failures = 0;

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      file_print(stdout, "FAIL {}:{:d}: {}\n", __FILE__, __LINE__, #cond); \
      failures++;                                                          \
    }                                                                      \
  } while (0)

/* Rooms, one mob and object prototype, one zone and one shop, all numbered from first_vnum. */
static GameWorld *make_world(room_vnum first_vnum, int count)
{
  GameWorld *g = game_world_create();

  for (room_vnum r = first_vnum; r < first_vnum + count; r++)
    room_set_number(g->rooms.emplace(r), r);

  struct char_data *mob_proto = g->mob_proto.emplace(first_vnum);
  clear_char(mob_proto);
  char_set_nr(mob_proto, first_vnum);
  char_mob_set(mob_proto, MOB_ISNPC, true);
  g->mob_index.emplace(first_vnum)->vnum = first_vnum;

  struct obj_data *proto = g->obj_proto.emplace(first_vnum);
  clear_object(proto);
  obj_set_item_number(proto, first_vnum);
  g->obj_index.emplace(first_vnum)->vnum = first_vnum;

  struct zone_data *zone = g->zones.emplace(first_vnum / 100);
  zone->number           = first_vnum / 100;
  zone->bot              = first_vnum;
  zone->top              = first_vnum + 99;
  zone->cmd              = empty_cmd_list();

  g->shops.emplace(first_vnum)->vnum = first_vnum;

  circle_srandom(g, 12345); /* an unseeded generator repeats one value forever */
  return g;
}

static void free_world(GameWorld *g)
{
  free_extracted_objects(g, NOTHING); /* while their prototypes exist */
  g->zones.clear();
  g->obj_index.clear();
  g->obj_proto.clear();
  g->mob_index.clear();
  g->mob_proto.clear();
  g->rooms.clear();
  game_world_destroy(g);
}

static void check_starting_values(void)
{
  GameWorld *g = game_world_create();

  CHECK(g->shops.empty());
  CHECK(g->guilds.empty());
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
  CHECK(room_of(a, 101) != nullptr);
  CHECK(room_of(b, 101) == nullptr);
  CHECK(room_of(b, 201) != nullptr);
  CHECK(room_of(a, 201) == nullptr);

  CHECK(mob_proto_of(a, 100) != nullptr);
  CHECK(mob_proto_of(b, 100) == nullptr);
  CHECK(obj_proto_of(b, 200) != nullptr);
  CHECK(obj_proto_of(a, 200) == nullptr);
  CHECK(find_zone(a, 1) != nullptr);
  CHECK(find_zone(b, 1) == nullptr);
  CHECK(find_zone_by_thing(a, 150) == find_zone(a, 1));
  CHECK(find_zone_by_thing(b, 150) == nullptr);
  CHECK(find_shop(a, 100) != nullptr);
  CHECK(find_shop(b, 100) == nullptr);
}

/* a character and an object placed in A appear only in A */
static void check_rooms(GameWorld *a, GameWorld *b)
{
  struct char_data *ch;
  struct obj_data *obj;

  ch = new char_data();
  clear_char(ch);
  obj = new obj_data();
  clear_object(obj);
  /* rooms list their people and objects by id, so they must be registered */
  char_set_id(ch, 1);
  char_register(a, ch);
  obj_set_id(obj, OBJ_ID_BASE);
  obj_register(a, obj);

  char_to_room(a, ch, 101);
  obj_to_room(a, obj, 102);
  CHECK(char_get_in_room(ch) == 101);
  CHECK(obj_get_in_room(obj) == 102);
  CHECK(first_char(a, room_get_people(a, 101)) == ch);
  CHECK(first_obj(a, room_get_contents(a, 102)) == obj);
  CHECK(room_get_people(b, 201).empty());
  CHECK(room_get_contents(a, 101).empty());

  /* the room count is per world too: room 102 exists in A, but B has no room 202 */
  char_from_room(a, ch);
  CHECK(room_get_people(a, 101).empty());
  CHECK(char_get_in_room(ch) == NOWHERE);
  char_to_room(b, ch, 202);
  CHECK(char_get_in_room(ch) == NOWHERE);
  char_to_room(a, ch, 102);
  CHECK(first_char(a, room_get_people(a, 102)) == ch);
  CHECK(room_get_people(b, 201).empty());

  char_from_room(a, ch);
  obj_from_room(a, obj);
  char_unregister(a, ch);
  obj_unregister(a, obj);
  delete obj;
  delete ch;
}

/* entities made from a prototype join only the world they were made in */
static void check_read_entities(GameWorld *a, GameWorld *b)
{
  struct char_data *mob = read_mobile(a, 100);
  struct obj_data *obj  = read_object(a, 100);

  CHECK(mob != nullptr);
  CHECK(obj != nullptr);
  CHECK(a->characters.size() == 1 && a->characters.find(char_get_id(mob)) == mob);
  CHECK(a->objects.size() == 1 && a->objects.find(obj_get_id(obj)) == obj);
  CHECK(mob_index_of(a, 100)->number == 1);
  CHECK(obj_index_of(a, 100)->number == 1);
  CHECK(char_get_mob_vnum(a, mob) == 100);
  CHECK(obj_get_vnum(a, obj) == 100);

  CHECK(b->characters.empty());
  CHECK(b->objects.empty());
  CHECK(mob_index_of(b, 200)->number == 0);
  CHECK(obj_index_of(b, 200)->number == 0);
  CHECK(read_mobile(b, 100) == nullptr);
  CHECK(read_object(b, 100) == nullptr);

  CHECK(find_char(a, char_get_id(mob)) == mob);
  CHECK(find_char(b, char_get_id(mob)) == nullptr);

  char_unregister(a, mob);
  obj_unregister(a, obj);
  a->characters.clear();
  a->objects.clear();
  delete mob;
  delete obj;
}

/* A loop sees the members of a list as they were when it began, skipping any destroyed or moved before their turn */
static void check_iteration(GameWorld *g)
{
  struct char_data *mob[6];
  struct obj_data *obj[4];
  std::vector<struct char_data *> seen;

  for (auto &m : mob)
    m = read_mobile(g, 100);
  for (int i = 0; i < 5; i++)
    char_to_room(g, mob[i], 100); /* the list is newest first: 4 3 2 1 0 */

  for (struct char_data *ch : room_people(g, 100)) {
    seen.push_back(ch);
    if (ch == mob[4]) {
      char_from_room(g, mob[2]);
      char_to_room(g, mob[2], 101);                     /* moved away before its turn */
      char_unregister(g, mob[1]);                       /* destroyed before its turn */
      char_to_room(g, mob[5], 100);                     /* added during the loop */
    }
  }
  CHECK((seen == std::vector<struct char_data *>{mob[4], mob[3], mob[0]}));

  int visits = 0;
  CHECK(!room_people_each(g, 100, [&](struct char_data *) { return ++visits < 2; }));
  CHECK(visits == 2);
  visits = 0;
  CHECK(room_people_each(g, 100, [&](struct char_data *) { visits++; }));
  CHECK(visits == 4); /* 5 4 3 0 */

  for (auto &o : obj) {
    o = read_object(g, 100);
    obj_to_room(g, o, 102); /* 3 2 1 0 */
  }
  std::vector<struct obj_data *> seen_obj;
  for (struct obj_data *o : room_contents(g, 102)) {
    seen_obj.push_back(o);
    if (o == obj[3]) {
      obj_from_room(g, obj[2]);
      obj_to_char(g, obj[2], mob[0]); /* picked up before its turn */
      obj_from_room(g, obj[1]);
      obj_to_obj(g, obj[1], obj[3]); /* put in a container before its turn */
    }
  }
  CHECK((seen_obj == std::vector<struct obj_data *>{obj[3], obj[0]}));
  seen_obj.clear();
  for (struct obj_data *o : char_inventory(g, mob[0]))
    seen_obj.push_back(o);
  CHECK((seen_obj == std::vector<struct obj_data *>{obj[2]}));
  seen_obj.clear();
  for (struct obj_data *o : obj_contents(g, obj[3]))
    seen_obj.push_back(o);
  CHECK((seen_obj == std::vector<struct obj_data *>{obj[1]}));

  visits = 0;
  for (struct char_data *ch : each_char(g)) /* everything still registered, wherever it is */
    visits += ch != nullptr;
  CHECK(visits == 5);

  obj_from_char(g, obj[2]);
  obj_from_obj(g, obj[1]);
  obj_from_room(g, obj[3]);
  obj_from_room(g, obj[0]);
  for (auto &o : obj) {
    obj_unregister(g, o);
    delete o;
  }
  for (auto &m : mob) {
    if (char_get_in_room(m) != NOWHERE)
      char_from_room(g, m);
    if (char_by_id(g, char_get_id(m)))
      char_unregister(g, m);
    delete m;
  }
  g->characters.clear();
  g->objects.clear();
}

/* The registry keeps entities newest first, as the old lists did, through growth and removals */
static void check_id_table(void)
{
  id_table<int> t;
  std::vector<int> items(3000);
  std::vector<long> expect;

  for (int i = 0; i < 3000; i++)
    CHECK(t.add(i + 1, &items[i]));
  CHECK(!t.add(5, &items[0])); /* taken */
  CHECK(!t.add(0, &items[0])); /* ids are positive */
  for (int i = 0; i < 3000; i++)
    if (i % 3)
      CHECK(t.remove(i + 1, &items[i]));
  CHECK(!t.remove(1, &items[1])); /* not the one registered under that id */
  CHECK(t.size() == 1000);
  for (int i = 2999; i >= 0; i--)
    if (!(i % 3))
      expect.push_back(i + 1);
  CHECK(t.ids() == expect);
  CHECK(t.find(1) == &items[0]);
  CHECK(t.find(2) == nullptr);
  CHECK(t.add(2, &items[1]) && t.ids().front() == 2); /* back, as the newest */
}

/* Freeing a copy of a character, such as one loaded from its player file, leaves the one in the game registered */
static void check_registry(GameWorld *g)
{
  struct char_data *mob  = read_mobile(g, 100);
  struct char_data *copy = new char_data();

  clear_char(copy);
  char_set_id(copy, char_get_id(mob));
  free_char(g, copy);
  CHECK(char_by_id(g, char_get_id(mob)) == mob);

  char_unregister(g, mob);
  CHECK(char_by_id(g, char_get_id(mob)) == nullptr);
  CHECK(g->characters.empty());
  mob_index_of(g, 100)->number--;
  delete mob;
}

/* References between entities are ids: one to something that has gone reads as nobody, not a dangling pointer */
static void check_references(GameWorld *g)
{
  struct char_data *a = read_mobile(g, 100), *b = read_mobile(g, 100);
  struct obj_data *seat = read_object(g, 100);

  char_set_fighting(a, b);
  char_set_master(b, a);
  char_set_sits(a, seat);
  CHECK(char_get_fighting(g, a) == b);
  CHECK(char_get_master(g, b) == a);
  CHECK(char_get_sits(g, a) == seat);

  char_unregister(g, b); /* b and the seat go */
  obj_unregister(g, seat);
  CHECK(char_get_fighting(g, a) == nullptr);
  CHECK(char_get_sits(g, a) == nullptr);
  CHECK(char_get_master(g, b) == a); /* what b pointed at is still there */

  char_set_fighting(a, nullptr);
  CHECK(a->fighting_uid == 0);

  char_unregister(g, a);
  g->characters.clear();
  g->objects.clear();
  delete a;
  delete b;
  delete seat;
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
  struct char_data *mob    = read_mobile(a, 100);
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

  char_unregister(a, mob);
  mob_index_of(a, 100)->number--;
  a->characters.clear();
  delete mob;
}

/* A new world's generator must work without seeding, and a world's clock can be pinned. */
static void check_defaults(void)
{
  GameWorld *a = game_world_create();
  GameWorld *b = game_world_create();
  unsigned long first = circle_random(a);
  time_t before;

  CHECK(first != circle_random(a));
  (void)circle_random(b);
  CHECK(first != circle_random(b));

  before = time(nullptr);
  CHECK(game_now(a) >= before);
  a->pinned_time = 1000;
  CHECK(game_now(a) == 1000);
  CHECK(game_now(b) >= before);

  game_world_destroy(a);
  game_world_destroy(b);
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
  d.desc_num                    = 1;
  a->descriptors.add(d.desc_num, &d);
  a->config_info.play.level_cap = 50;
  CHECK(b->time_info.hours == 0);
  CHECK(b->weather_info.pressure == 0);
  CHECK(b->descriptors.empty());
  CHECK(b->config_info.play.level_cap == 0);
  a->descriptors.remove(d.desc_num, &d);

  event_init(a);
  event_init(b);
  event_create(a, count_event, &ran, nullptr, 1);
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
  struct obj_data *obj = read_object(g, g->obj_index.vnums().front());

  obj_to_char(g, obj, holder);
  return obj;
}

/* Objects off the ground used to make these updates read and write rooms[-1]; ASan catches those. */
static void check_objects_off_the_ground(void)
{
  struct char_data *holder;
  struct obj_data *obj, *bag;
  GameWorld *g;

  holder = new char_data();
  clear_char(holder);
  CHECK(!char_ether_stream(nullptr, holder)); /* no world needed: it never looks a room up */

  /* a portal or an ashcloud whose timer runs out is still removed */
  g   = make_world(1306, 1);
  obj = carried_object(g, holder);
  obj_set_timer(obj, 1);
  point_update(g);
  CHECK(g->objects.empty());
  free_world(g);

  g   = make_world(500, 1);
  obj = carried_object(g, holder);
  obj_set_type_flag(obj, ITEM_PORTAL);
  obj_set_timer(obj, 1);
  point_update(g);
  CHECK(g->objects.empty());
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

  /* carried ice that melts away leaves its carrier's load as it was, weighed once */
  g   = make_world(500, 1);
  obj = carried_object(g, holder);
  obj_from_char(g, obj);
  obj_extra_set(obj, ITEM_ICE, true);
  obj_set_weight(obj, 20);
  int64_t load = char_get_carry_weight(holder);
  obj_to_char(g, obj, holder);
  for (int i = 0; i < 20 && !g->objects.empty(); i++)
    point_update(g);
  CHECK(g->objects.empty());
  CHECK(char_get_carry_weight(holder) == load);
  free_world(g);

  /* a gravity generator in a container changes nothing */
  g   = make_world(11, 1);
  bag = read_object(g, 11);
  obj = read_object(g, 11);
  obj_to_room(g, bag, 11);
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
  obj_to_room(g, obj, 600);
  CHECK(find_vehicle_by_vnum(g, 600) == obj);
  extract_obj(g, obj);
  free_world(g);

  /* nobody is listening nowhere */
  g = make_world(700, 1);
  send_to_room(g, NOWHERE, "unheard");
  free_world(g);

  delete holder;
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

/* A toplist file of 25 places loads into the 25 places, and nothing past them. */
static void check_toplist_load(void)
{
  const char *tmp = getenv("TMPDIR");
  std::string dir = std::string(tmp && *tmp ? tmp : "/tmp") + "/dbat-toplist.XXXXXX";
  char cwd[4096];
  CHECK(mkdtemp(dir.data()) != nullptr && getcwd(cwd, sizeof(cwd)) != nullptr);
  CHECK(chdir(dir.c_str()) == 0 && mkdir("intro", 0755) == 0 && mkdir("intro/P-T", 0755) == 0);
  FILE *fl = fopen("intro/P-T/toplist.itr", "w");
  CHECK(fl != nullptr);
  for (int x = 0; x < 25; x++)
    fprintf(fl, "Name%d %d\n", x, (x + 1) * 100);
  fclose(fl);

  GameWorld *g = make_world(800, 1);
  topLoad(g);
  CHECK(g->toppoint[0] == 100);
  CHECK(g->toppoint[24] == 2500);
  CHECK(g->topname[24] && !strcmp(g->topname[24], "Name24"));
  CHECK(g->ch_selling_uid == 0 && g->ch_buying_uid == 0);
  for (char *&name : g->topname) {
    free(name);
    name = nullptr;
  }
  free_world(g);

  CHECK(remove("intro/P-T/toplist.itr") == 0 && rmdir("intro/P-T") == 0 && rmdir("intro") == 0);
  CHECK(chdir(cwd) == 0 && rmdir(dir.c_str()) == 0);
}

int main(void)
{
  GameWorld *a = make_world(100, 3);
  GameWorld *b = make_world(200, 2);

  logfile = stderr; /* the logger stays global so crash handlers can use it; several checks below log */

  check_starting_values();
  check_defaults();
  check_lookups(a, b);
  check_rooms(a, b);
  check_read_entities(a, b);
  check_iteration(a);
  check_references(a);
  check_id_table();
  check_registry(a);
  check_rng(a, b);
  check_random_color(a, b);
  check_other_state(a, b);
  check_objects_off_the_ground();
  check_get_number();
  check_toplist_load();

  free_world(b);
  free_world(a);

  if (failures) {
    file_print(stdout, "{:d} check(s) failed\n", failures);
    return 1;
  }
  puts("game_world: all checks passed");
  return 0;
}
