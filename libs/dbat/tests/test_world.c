/* Plays through whole game systems in the fixture world, checking the world audit after each. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"
#include "dbat/db.h"
#include "dbat/extract.h"
#include "dbat/comm.h"
#include "dbat/fight.h"
#include "dbat/guild.h"
#include "dbat/shop.h"
#include "dbat/handler.h"
#include "dbat/utils.h"
#include "dbat/world_audit.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/rooms.h"
#include "dbat/game_world.h"

static int failures = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      failures++;                                                \
    }                                                            \
  } while (0)

static void print_problem(void *, const char *problem)
{
  printf("  audit: %s\n", problem);
}

static void quiet(void *, const char *) {}

#define CHECK_AUDIT(g) CHECK(world_audit((g), print_problem, nullptr) == 0)
/* Expects the audit to notice the damage just done. */
#define CHECK_NOTICED(g) CHECK(world_audit((g), quiet, nullptr) > 0)

static void place(GameWorld *g, struct char_data *ch, room_vnum vnum)
{
  char_from_room(g, ch);
  char_to_room(g, ch, real_room(g, vnum));
}

static struct char_data *mob_in(GameWorld *g, room_vnum vnum, mob_vnum mob)
{
  for (struct char_data *ch = rroom_get_people(g, real_room(g, vnum)); ch; ch = char_get_next_in_room(ch))
    if (char_is_npc(ch) && char_get_mob_vnum(g, ch) == mob)
      return ch;
  return nullptr;
}

static struct obj_data *carried(GameWorld *g, struct char_data *ch, obj_vnum vnum)
{
  for (struct obj_data *obj = char_get_inventory(ch); obj; obj = obj_get_next_content(obj))
    if (obj_get_vnum(g, obj) == vnum)
      return obj;
  return nullptr;
}

static bool saw(struct char_data *ch, const char *text)
{
  return strstr(th_output(ch), text) != nullptr;
}

static void check_commands_reach_the_player(GameWorld *g, struct char_data *ch)
{
  place(g, ch, 300);
  th_take_output(g, ch);
  th_type(g, ch, "look");
  CHECK(saw(ch, "Town Square"));
  th_take_output(g, ch);
  th_type(g, ch, "north");
  CHECK(saw(ch, "Training Yard"));
  CHECK(char_get_in_room(ch) == real_room(g, 301));
  th_take_output(g, ch);
  th_type(g, ch, "nosuchcommand");
  CHECK(!saw(ch, "Training Yard"));
  CHECK_AUDIT(g);
}

/* The audit is only worth trusting if it notices each kind of damage. */
static void check_audit_notices_damage(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj      = read_object(g, 301, VIRTUAL);
  struct obj_data *chest    = read_object(g, 300, VIRTUAL);
  struct char_data *stray   = (struct char_data *)&failures; /* never dereferenced: it isn't in the game */
  room_rnum room            = char_get_in_room(ch);
  struct room_direction_data *exit;

  obj_to_char(g, obj, ch);
  obj_to_room(g, chest, room);
  CHECK_AUDIT(g);

  char_set_in_room(ch, room + 1);
  CHECK_NOTICED(g);
  char_set_in_room(ch, room);

  char_set_in_room(ch, g->top_room + 5);
  CHECK_NOTICED(g);
  char_set_in_room(ch, room);

  obj_set_carried_by(obj, nullptr);
  CHECK_NOTICED(g);
  obj_set_carried_by(obj, ch);

  obj_set_in_room(obj, room);
  CHECK_NOTICED(g);
  obj_set_in_room(obj, NOWHERE);

  obj_set_in_room(chest, room + 1);
  CHECK_NOTICED(g);
  obj_set_in_room(chest, room);

  /* says it's carried, but isn't in the inventory */
  obj_from_char(g, obj);
  obj_set_carried_by(obj, ch);
  CHECK_NOTICED(g);
  obj_set_carried_by(obj, nullptr);
  obj_to_char(g, obj, ch);

  char_set_fighting(ch, stray);
  CHECK_NOTICED(g);
  char_set_fighting(ch, nullptr);

  char_set_master(ch, stray);
  CHECK_NOTICED(g);
  char_set_master(ch, nullptr);

  exit = rroom_get_dir_option(g, room, SOUTH);
  CHECK(exit != nullptr);
  if (exit) {
    room_rnum to = exit->to_room;

    exit->to_room = g->top_room + 5;
    CHECK_NOTICED(g);
    exit->to_room = to;
  }

  CHECK_AUDIT(g);
  extract_obj(g, obj);
  extract_obj(g, chest);
  CHECK_AUDIT(g);
}

static void check_extract_mid_fight(GameWorld *g, struct char_data *ch)
{
  struct char_data *dummy;

  place(g, ch, 301);
  dummy = mob_in(g, 301, 300);
  CHECK(dummy != nullptr);
  if (!dummy)
    return;
  set_fighting(g, ch, dummy);
  set_fighting(g, dummy, ch);
  CHECK_AUDIT(g);
  extract_char(g, dummy);
  extract_pending_chars(g);
  CHECK(char_get_fighting(ch) == nullptr);
  CHECK_AUDIT(g);
  th_pulses(g, 20);
  CHECK_AUDIT(g);
}

static void check_extract_with_followers(GameWorld *g, struct char_data *ch)
{
  struct char_data *leader, *follower;

  reset_zone(g, real_zone(g, 3));
  place(g, ch, 301);
  leader   = read_mobile(g, 300, VIRTUAL);
  follower = read_mobile(g, 300, VIRTUAL);
  char_to_room(g, leader, char_get_in_room(ch));
  char_to_room(g, follower, char_get_in_room(ch));
  add_follower(g, follower, leader);
  add_follower(g, ch, leader);
  CHECK_AUDIT(g);
  extract_char(g, leader);
  extract_pending_chars(g);
  CHECK(char_get_master(ch) == nullptr);
  CHECK(char_get_master(follower) == nullptr);
  CHECK_AUDIT(g);
  extract_char(g, follower);
  extract_pending_chars(g);
  CHECK_AUDIT(g);
}

/* A greet trigger whose mob purges itself mid-script, while the player is still walking in. */
static void check_script_purges_its_owner(GameWorld *g, struct char_data *ch)
{
  place(g, ch, 304);
  th_take_output(g, ch);
  CHECK(mob_in(g, 305, 302) != nullptr);
  th_type(g, ch, "east");
  CHECK(char_get_in_room(ch) == real_room(g, 305));
  CHECK(saw(ch, "The stranger vanishes."));
  extract_pending_chars(g);
  CHECK(mob_in(g, 305, 302) == nullptr);
  CHECK_AUDIT(g);
  th_pulses(g, 20);
  CHECK_AUDIT(g);
}

static void check_room_trigger_runs(GameWorld *g, struct char_data *ch)
{
  place(g, ch, 303);
  th_take_output(g, ch);
  th_type(g, ch, "east");
  CHECK(char_get_in_room(ch) == real_room(g, 304));
  CHECK(saw(ch, "The floor hums."));
  CHECK_AUDIT(g);
}

/* The damaged-ATM messages used to send to rooms[-1] when the ATM was in a container. */
static void check_atm_in_a_container(GameWorld *g, struct char_data *ch)
{
  struct obj_data *atm   = read_object(g, 3034, VIRTUAL);
  struct obj_data *chest = read_object(g, 300, VIRTUAL);

  place(g, ch, 300);
  obj_to_room(g, chest, char_get_in_room(ch));
  obj_to_obj(g, atm, chest);
  obj_set_value(atm, VAL_ALL_HEALTH, 5);
  CHECK_AUDIT(g);
  th_pulses(g, 1000);
  CHECK(obj_get_in_obj(atm) == chest);
  CHECK_AUDIT(g);
  extract_obj(g, chest);
  CHECK_AUDIT(g);
}

static void check_zone_reset(GameWorld *g)
{
  int dummies = 0;

  for (int i = 0; i < 3; i++)
    reset_zone(g, real_zone(g, 3));
  for (struct char_data *ch = rroom_get_people(g, real_room(g, 301)); ch; ch = char_get_next_in_room(ch))
    if (char_is_npc(ch) && char_get_mob_vnum(g, ch) == 300)
      dummies++;
  CHECK(dummies >= 1 && dummies <= 2); /* the zone command's limit is 2 in the room */
  CHECK_AUDIT(g);
}

static void check_shop(GameWorld *g, struct char_data *ch)
{
  int gold;

  CHECK(mob_in(g, 303, 301) != nullptr);
  place(g, ch, 303);
  char_set_gold(ch, 1000);
  th_take_output(g, ch);
  th_type(g, ch, "list");
  CHECK(saw(ch, "loaf of bread"));
  th_take_output(g, ch);

  gold = char_get_gold(ch);
  th_type(g, ch, "buy bread");
  CHECK(carried(g, ch, 302) != nullptr);
  CHECK(char_get_gold(ch) < gold);
  th_take_output(g, ch);
  CHECK_AUDIT(g);

  gold = char_get_gold(ch);
  th_type(g, ch, "sell bread");
  CHECK(carried(g, ch, 302) == nullptr);
  CHECK(char_get_gold(ch) > gold);
  th_take_output(g, ch);
  CHECK_AUDIT(g);
}

static void check_save_and_reload(GameWorld *g, struct char_data *ch)
{
  struct char_data *copy;

  char_set_gold(ch, 4321);
  save_char(g, ch);

  CREATE(copy, struct char_data, 1);
  clear_char(copy);
  CREATE(*char_player_specials_slot(copy), struct player_special_data, 1);
  CHECK(load_char(g, char_get_name(ch), copy) >= 0);
  CHECK(!strcmp(char_get_name(copy), char_get_name(ch)));
  CHECK(char_get_gold(copy) == 4321);
  free_char(g, copy);
}

static int count_objects(const GameWorld *g)
{
  int count = 0;

  for (struct obj_data *obj = g->objects; obj; obj = obj_get_next(obj))
    count++;
  return count;
}

/* The first player ever is made an Implementor, which used to crash; th_player() then demoted Tester. */
static void check_admin_levels(GameWorld *g, struct char_data *ch)
{
  admin_set(g, ch, ADMLVL_VICE);
  CHECK(char_get_admlevel(ch) == ADMLVL_VICE);
  CHECK(char_adm_flagged(ch, ADM_ALLSHOPS));
  CHECK(!char_adm_flagged(ch, ADM_TRANSALL));

  admin_set(g, ch, ADMLVL_GRGOD);
  CHECK(char_adm_flagged(ch, ADM_TRANSALL));
  CHECK(!char_adm_flagged(ch, ADM_CEDIT));

  admin_set(g, ch, ADMLVL_IMPL);
  CHECK(char_get_admlevel(ch) == ADMLVL_IMPL);
  CHECK(char_adm_flagged(ch, ADM_CEDIT));

  admin_set(g, ch, ADMLVL_NONE);
  CHECK(char_get_admlevel(ch) == ADMLVL_NONE);
  CHECK(!char_adm_flagged(ch, ADM_CEDIT));
  CHECK(!char_adm_flagged(ch, ADM_ALLSHOPS));
}

/* iedit used to cut the object list, and to leave a freed working copy in it on disconnect. */
static void check_iedit(GameWorld *g)
{
  struct char_data *builder = th_player(g, "Builder", ADMLVL_IMPL);
  struct obj_data *chest    = read_object(g, 300, VIRTUAL);
  struct obj_data *sword    = read_object(g, 301, VIRTUAL);
  struct obj_data *bread    = read_object(g, 302, VIRTUAL); /* newer than the sword */
  int objects;

  place(g, builder, 300);
  obj_to_char(g, sword, builder);
  obj_to_char(g, bread, builder);
  obj_to_room(g, chest, char_get_in_room(builder));
  objects = count_objects(g);

  /* editing changes nothing in the game until it's committed */
  th_type(g, builder, "iedit sword");
  CHECK(count_objects(g) == objects);
  CHECK_AUDIT(g);

  /* where the object goes meanwhile must survive the commit */
  obj_from_char(g, sword);
  obj_to_obj(g, sword, chest);
  th_type(g, builder, "2");
  th_type(g, builder, "a renamed sword");
  th_type(g, builder, "q");
  CHECK(!strcmp(obj_get_short_description(sword), "a renamed sword"));
  CHECK(obj_get_in_obj(sword) == chest);
  CHECK(obj_get_carried_by(sword) == nullptr);
  CHECK(!strcmp(obj_get_short_description(&g->obj_proto[real_object(g, 301)]), "a plain sword"));
  CHECK(count_objects(g) == objects);
  CHECK_AUDIT(g);

  /* the object is gone by the time the edit is committed */
  th_type(g, builder, "iedit bread");
  extract_obj(g, bread);
  objects = count_objects(g);
  th_type(g, builder, "q");
  CHECK(count_objects(g) == objects);
  CHECK_AUDIT(g);

  /* losing the connection mid-edit */
  th_type(g, builder, "iedit chest");
  close_socket(g, char_get_desc(builder));
  th_pulses(g, 5);
  CHECK(count_objects(g) == objects);
  CHECK_AUDIT(g);

  extract_obj(g, chest);
  extract_char(g, builder);
  extract_pending_chars(g);
  CHECK_AUDIT(g);
}

/* Each of these used to read outside its table. */
static void check_lookups_past_the_end(GameWorld *g)
{
  CHECK(count_shops(g, 300, 399) == 1);
  CHECK(count_shops(g, 400, 99999) == 0);
  CHECK(count_guilds(g, 0, 99999) == 0);
}

int main(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch;

  CHECK_AUDIT(g);
  ch = th_player(g, "Tester", ADMLVL_NONE);
  CHECK_AUDIT(g);

  check_commands_reach_the_player(g, ch);
  check_audit_notices_damage(g, ch);
  check_extract_mid_fight(g, ch);
  check_extract_with_followers(g, ch);
  check_room_trigger_runs(g, ch);
  check_script_purges_its_owner(g, ch);
  check_atm_in_a_container(g, ch);
  check_zone_reset(g);
  check_shop(g, ch);
  check_save_and_reload(g, ch);
  check_admin_levels(g, ch);
  check_iedit(g);
  check_lookups_past_the_end(g);

  th_shutdown(g);
  if (failures)
    printf("%d check(s) failed\n", failures);
  else
    printf("all checks passed\n");
  return failures != 0;
}
