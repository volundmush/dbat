/* Plays through whole game systems in the fixture world, checking the world audit after each. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"
#include "dbat/db.h"
#include "dbat/extract.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/affect.h"
#include "dbat/config.h"
#include "dbat/genobj.h"
#include "dbat/spells.h"
#include "dbat/gengld.h"
#include "dbat/local_limits.h"
#include "dbat/magic.h"
#include "dbat/zones.h"
#include "dbat/guild.h"
#include "dbat/shop.h"
#include "dbat/handler.h"
#include "dbat/relocate.h"
#include "dbat/dg_scripts.h"
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/world_audit.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/rooms.h"
#include "dbat/game_world.h"

static int failures = 0;

/* Things that aren't in the game, for the audit to notice when something points at them. */
static struct char_data stray_char;
static struct obj_data stray_obj;
static struct descriptor_data stray_desc;

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

#define CHECK_POINTER_NOTICED(setter, value) \
  do {                                      \
    setter(ch, value);                      \
    CHECK_NOTICED(g);                       \
    setter(ch, nullptr);                    \
  } while (0)

static void check_audit_notices_broken_links(GameWorld *g, struct char_data *ch, struct obj_data *obj,
                                             struct obj_data *chest, struct char_data *stray)
{
  struct descriptor_data *d = char_get_desc(ch);

  CHECK_POINTER_NOTICED(char_set_grappling, stray);
  CHECK_POINTER_NOTICED(char_set_grappled, stray);
  CHECK_POINTER_NOTICED(char_set_drag, stray);
  CHECK_POINTER_NOTICED(char_set_dragged, stray);
  CHECK_POINTER_NOTICED(char_set_absorbing, stray);
  CHECK_POINTER_NOTICED(char_set_absorbby, stray);
  CHECK_POINTER_NOTICED(char_set_defender, stray);
  CHECK_POINTER_NOTICED(char_set_defending, stray);
  CHECK_POINTER_NOTICED(char_set_blocks, stray);
  CHECK_POINTER_NOTICED(char_set_blocked, stray);
  char_set_carrying(g, ch, stray);
  CHECK_NOTICED(g);
  char_set_carrying(g, ch, nullptr);
  char_set_carried_by(g, ch, stray);
  CHECK_NOTICED(g);
  char_set_carried_by(g, ch, nullptr);
  CHECK_POINTER_NOTICED(char_set_mindlink, stray);
  CHECK_POINTER_NOTICED(char_set_poisonby, stray);
  CHECK_POINTER_NOTICED(char_set_grappling, ch); /* present, but doesn't point back */

  CHECK_POINTER_NOTICED(char_set_sits, chest);
  obj_set_sitting(chest, ch);
  CHECK_NOTICED(g);
  obj_set_sitting(chest, nullptr);

  obj_set_user(obj, stray);
  CHECK_NOTICED(g);
  obj_set_user(obj, nullptr);
  obj_set_target(obj, stray);
  CHECK_NOTICED(g);
  obj_set_target(obj, nullptr);
  obj_set_fellow_wall(obj, &stray_obj);
  CHECK_NOTICED(g);
  obj_set_fellow_wall(obj, nullptr);
  obj_set_posted_to(obj, &stray_obj);
  CHECK_NOTICED(g);
  obj_set_posted_to(obj, nullptr);

  char_set_position(ch, 99);
  CHECK_NOTICED(g);
  char_set_position(ch, POS_STANDING);
  char_set_gold(ch, -1);
  CHECK_NOTICED(g);
  char_set_gold(ch, 0);
  char_set_carry_items(ch, char_get_carry_items(ch) + 1);
  CHECK_NOTICED(g);
  char_set_carry_items(ch, char_get_carry_items(ch) - 1);
  char_set_carry_weight(ch, char_get_carry_weight(ch) + 1);
  CHECK_NOTICED(g);
  char_set_carry_weight(ch, char_get_carry_weight(ch) - 1);

  g->obj_index[obj_get_item_number(obj)].number++;
  CHECK_NOTICED(g);
  g->obj_index[obj_get_item_number(obj)].number--;

  obj_set_id(obj, obj_get_id(obj) + 1000); /* the lookup table now disagrees */
  CHECK_NOTICED(g);
  obj_set_id(obj, obj_get_id(obj) - 1000);

  d->snooping = &stray_desc;
  CHECK_NOTICED(g);
  d->snooping = nullptr;
  d->snoop_by = &stray_desc;
  CHECK_NOTICED(g);
  d->snoop_by = nullptr;
}

/* The audit is only worth trusting if it notices each kind of damage. */
static void check_audit_notices_damage(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj      = read_object(g, 301, VIRTUAL);
  struct obj_data *chest    = read_object(g, 300, VIRTUAL);
  struct char_data *stray   = &stray_char;
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

  check_audit_notices_broken_links(g, ch, obj, chest, stray);

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
  int masters = 0;

  for (int i = 0; i < 3; i++)
    reset_zone(g, real_zone(g, 3));
  /* nothing else loads this mob, and the zone command's limit is 1 */
  for (struct char_data *ch = rroom_get_people(g, real_room(g, 303)); ch; ch = char_get_next_in_room(ch))
    if (char_is_npc(ch) && char_get_mob_vnum(g, ch) == 331)
      masters++;
  CHECK(masters == 1);
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

static int count_objects_of(GameWorld *g, obj_vnum vnum)
{
  int count = 0;

  for (struct obj_data *obj = g->objects; obj; obj = obj_get_next(obj))
    if (obj_get_vnum(g, obj) == vnum)
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
  CHECK(count_guilds(g, 0, 99999) == 1);
  CHECK(count_guilds(g, 400, 99999) == 0);
}

/* Inserting used to free uninitialized memory, or strings that a neighbouring guild was still using. */
static void check_new_guilds(GameWorld *g)
{
  guild_vnum vnums[] = {320, 310, 330}; /* the fixture already has 300 */

  for (size_t i = 0; i < sizeof(vnums) / sizeof(*vnums); i++) {
    char message[32];
    struct guild_data guild = {0};

    snprintf(message, sizeof(message), "guild %d", vnums[i]);
    guild_set_vnum(&guild, vnums[i]);
    guild_set_no_such_skill(&guild, message);
    guild_set_not_enough_gold(&guild, message);
    add_guild(g, &guild);
  }
  CHECK(g->top_guild == 3);
  for (guild_rnum r = 1; r <= g->top_guild; r++) {
    char message[32];

    snprintf(message, sizeof(message), "guild %d", 300 + r * 10);
    CHECK(rguild_get_vnum(g, r) == 300 + r * 10);
    CHECK(!strcmp(guild_get_no_such_skill(&g->guild_index[r]), message));
  }
  CHECK(rguild_get_vnum(g, 0) == 300);
  CHECK(count_guilds(g, 0, 99999) == 4);
}

/* Deleting a prototype used to read the object it had just freed, and to lose what its copies held. */
static void check_delete_object_prototype(GameWorld *g, struct char_data *ch)
{
  obj_rnum chest_rnum = real_object(g, 300);
  struct obj_data *in_room, *carried, *room_bread, *carried_bread;

  /* no zone command may load the deleted prototype later */
  for (int i = 0; zone_get_cmd(g, real_zone(g, 3), i)->command != 'S'; i++)
    zone_get_cmd(g, real_zone(g, 3), i)->if_flag = 1;

  place(g, ch, 301);
  in_room       = read_object(g, 300, VIRTUAL);
  carried       = read_object(g, 300, VIRTUAL);
  room_bread    = read_object(g, 302, VIRTUAL);
  carried_bread = read_object(g, 302, VIRTUAL);
  obj_to_room(g, in_room, char_get_in_room(ch));
  obj_to_char(g, carried, ch);
  obj_to_obj(g, room_bread, in_room);
  obj_to_obj(g, carried_bread, carried);
  CHECK_AUDIT(g);

  delete_object(g, chest_rnum);
  CHECK(real_object(g, 300) == NOTHING);
  CHECK(obj_get_in_room(room_bread) == char_get_in_room(ch));
  CHECK(obj_get_carried_by(carried_bread) == ch);
  CHECK_AUDIT(g);

  extract_obj(g, room_bread);
  extract_obj(g, carried_bread);
  CHECK_AUDIT(g);
}

/* Each of these used to divide by zero, read outside a table, or follow a null pointer. */
static void check_awkward_commands(GameWorld *g, struct char_data *ch)
{
  struct char_data *admin = th_player(g, "Admin", ADMLVL_IMPL);
  struct char_data *leaver = th_player(g, "Leaver", ADMLVL_NONE);

  place(g, admin, 300);
  place(g, ch, 300);

  char_set_skills_base(ch, SKILL_HAYASA, 1);
  th_type(g, ch, "hayasa");
  th_take_output(g, ch);
  char_set_skills_base(ch, SKILL_HAYASA, 0);

  char_plr_set(ch, PLR_NOSHOUT, true);
  th_type(g, ch, "gemote waves");
  CHECK(saw(ch, "You cannot gemote"));
  th_take_output(g, ch);
  char_plr_set(ch, PLR_NOSHOUT, false);

  /* an NPC has no scouter, and used to be assumed to have one */
  {
    struct char_data *mob = read_mobile(g, 300, VIRTUAL);

    char_to_room(g, mob, char_get_in_room(ch));
    command_interpreter(g, mob, writable{"snet 5"});
    extract_char(g, mob);
    extract_pending_chars(g);
  }

  th_type(g, leaver, "quit"); /* leaves a nameless character at the menu */
  th_pulses(g, 2);
  th_type(g, admin, "lag Tester 1");
  CHECK(char_get_wait(ch) > 0);
  CHECK(!strcmp(char_get_name(admin), "Admin"));
  char_set_wait(ch, 0);
  th_take_output(g, admin);
  CHECK_AUDIT(g);

  extract_char(g, admin);
  extract_pending_chars(g);
}

/* Experience is 64 bits wide everywhere: level_exp() * 5 used to overflow int. */
static void check_experience_width(GameWorld *g, struct char_data *ch)
{
  int high_level = 1;
  int64_t before;

  while (high_level < 99 && level_exp(ch, high_level + 1) < 500000000)
    high_level++;
  CHECK(level_exp(ch, high_level + 1) >= 500000000);

  char_set_class_level(ch, high_level);
  char_set_exp(ch, 0);
  gain_exp(g, ch, 1000);
  CHECK(char_get_exp(ch) > 0);

  before = char_get_exp(ch);
  gain_exp_regardless(g, ch, INT64_C(3000000000));
  CHECK(char_get_exp(ch) >= before + 2900000000);
  char_set_exp(ch, 0);
  char_set_class_level(ch, 1);
  th_take_output(g, ch);
}

/* whois used to free a loaded player with free() alone, leaving them in the affect list. */
static void check_whois_with_affects(GameWorld *g, struct char_data *ch)
{
  struct char_data *admin = th_player(g, "Asker", ADMLVL_IMPL);
  struct affected_type af = {.type = SPELL_SLEEP, .duration = 5, .location = APPLY_NONE};

  affect_to_char(g, ch, &af);
  save_char(g, ch);
  CHECK_AUDIT(g);
  th_type(g, admin, "whois Tester");
  CHECK(saw(admin, "Tester"));
  CHECK_AUDIT(g);
  affect_from_char(g, ch, SPELL_SLEEP);
  CHECK_AUDIT(g);

  extract_char(g, admin);
  extract_pending_chars(g);
}

/* The walk used to stop at whoever's last affect ended, so everyone after them missed the hour. */
static void check_affects_all_count_down(GameWorld *g, struct char_data *ch)
{
  struct char_data *ending = read_mobile(g, 300, VIRTUAL);
  struct char_data *other  = read_mobile(g, 300, VIRTUAL);
  struct affected_type lasting = {.type = SPELL_SLEEP, .duration = 3, .location = APPLY_NONE};
  struct affected_type short_lived = {.type = SPELL_SLEEP, .duration = 0, .location = APPLY_NONE};
  struct affected_type permanent = {.type = SPELL_SLEEP, .duration = -1, .location = APPLY_NONE};

  place(g, ch, 301);
  char_to_room(g, ending, char_get_in_room(ch));
  char_to_room(g, other, char_get_in_room(ch));

  /* each joins the front of the list, so ending comes before the others */
  affect_to_char(g, ch, &permanent);
  affect_to_char(g, other, &lasting);
  affect_to_char(g, ending, &short_lived);
  CHECK(g->affect_list == ending);
  th_take_output(g, ch);

  affect_update(g);
  CHECK(char_get_affected(ending) == nullptr);
  CHECK(char_get_affected(other) && char_get_affected(other)->duration == 2);
  CHECK(char_get_affected(ch) && char_get_affected(ch)->duration == -1);

  /* and it counts down to the end, without touching anyone else */
  affect_update(g);
  affect_update(g);
  CHECK(char_get_affected(other) && char_get_affected(other)->duration == 0);
  affect_update(g);
  CHECK(char_get_affected(other) == nullptr);
  CHECK(char_get_affected(ch) && char_get_affected(ch)->duration == -1);
  CHECK_AUDIT(g);

  affect_from_char(g, ch, SPELL_SLEEP);
  extract_char(g, ending);
  extract_char(g, other);
  extract_pending_chars(g);
  CHECK_AUDIT(g);
}

/* A script used to carry on after forcing the player holding its object to quit, which freed it. */
static void check_script_loses_its_owner(GameWorld *g)
{
  struct char_data *holder = th_player(g, "Holder", ADMLVL_NONE);
  struct obj_data *zapper  = read_object(g, 351, VIRTUAL);

  obj_to_char(g, zapper, holder);
  CHECK(count_objects_of(g, 351) == 1);
  th_type(g, holder, "zap");
  CHECK(count_objects_of(g, 351) == 0);
  CHECK_AUDIT(g);
  th_pulses(g, 3);
  CHECK(g->extracted_triggers == nullptr);
  CHECK_AUDIT(g);
}

/* The loop over objects used to read the next one from an object whose trigger had just purged it. */
static void check_random_trigger_purges_itself(GameWorld *g, struct char_data *ch)
{
  for (int i = 0; i < 3; i++)
    obj_to_room(g, read_object(g, 352, VIRTUAL), char_get_in_room(ch));
  CHECK(count_objects_of(g, 352) == 3);
  script_trigger_check(g);
  th_pulses(g, 1);
  CHECK(count_objects_of(g, 352) == 0);
  CHECK_AUDIT(g);
}

static void attach_trigger(GameWorld *g, struct script_data **slot, trig_vnum vnum)
{
  add_trigger(g, dg_ensure_script(slot), read_trigger(g, real_trigger(g, vnum)), -1);
}

/* A plain object in ch's room that runs the given trigger. */
static struct obj_data *object_with_trigger(GameWorld *g, struct char_data *ch, trig_vnum vnum)
{
  struct obj_data *obj = read_object(g, 350, VIRTUAL);

  obj_to_room(g, obj, char_get_in_room(ch));
  attach_trigger(g, obj_script_slot(obj), vnum);
  return obj;
}

/* The loop over objects used to read the next one from an object whose trigger had just purged it. */
static void check_time_trigger_purges_itself(GameWorld *g, struct char_data *ch)
{
  long ids[3];
  int hours = g->time_info.hours;

  for (int i = 0; i < 3; i++)
    ids[i] = obj_get_id(object_with_trigger(g, ch, 384));
  g->time_info.hours = 5;
  check_time_triggers(g);
  g->time_info.hours = hours;
  for (int i = 0; i < 3; i++)
    CHECK(find_obj(g, ids[i]) == nullptr);
  CHECK_AUDIT(g);
}

/* ...and the timer loop could step onto a neighbour that the trigger had cleared away. */
static void check_timer_trigger_clears_the_room(GameWorld *g, struct char_data *ch)
{
  long neighbour, purger;
  struct obj_data *obj;

  place(g, ch, 302);
  obj = read_object(g, 350, VIRTUAL);
  obj_to_room(g, obj, char_get_in_room(ch));
  neighbour = obj_get_id(obj);
  obj = object_with_trigger(g, ch, 385); /* newest first, so the neighbour comes next in the list */
  purger = obj_get_id(obj);
  obj_set_timer(obj, 1);
  point_update(g);
  CHECK(find_obj(g, neighbour) == nullptr);
  if ((obj = find_obj(g, purger)))
    extract_obj(g, obj);
  CHECK_AUDIT(g);
}

/* The spell used to go on to work on the object it had just lost. */
static void check_cast_trigger_purges_its_target(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj = object_with_trigger(g, ch, 386);

  CHECK(cast_otrigger(g, ch, obj, 1) == 0);
  CHECK_AUDIT(g);
}

/* A zone reset used to give variables to, or equip, an object that had purged itself as it loaded. */
static void check_load_trigger_purges_itself(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj = object_with_trigger(g, ch, 387);

  CHECK(!load_otrigger(g, obj));
  CHECK_AUDIT(g);
}

/* Taking the equipment off a dead mob used to carry on after the item's trigger had purged it. */
static void check_remove_trigger_on_a_dying_mob(GameWorld *g, struct char_data *ch)
{
  struct char_data *mob = read_mobile(g, 300, VIRTUAL);
  struct obj_data *obj  = read_object(g, 350, VIRTUAL);
  long id               = obj_get_id(obj);

  char_to_room(g, mob, char_get_in_room(ch));
  attach_trigger(g, obj_script_slot(obj), 388);
  equip_char(g, mob, obj, WEAR_FINGER_R);
  die(g, mob, nullptr);
  extract_pending_chars(g);
  CHECK(find_obj(g, id) == nullptr);
  CHECK_AUDIT(g);
}

/* Real scripts do this all the time, and the object used to be read after it was gone. */
static void check_receive_trigger_purges_the_gift(GameWorld *g, struct char_data *ch)
{
  struct char_data *mob = read_mobile(g, 300, VIRTUAL);
  struct obj_data *obj  = read_object(g, 350, VIRTUAL);
  long id               = obj_get_id(obj);

  char_to_room(g, mob, char_get_in_room(ch));
  attach_trigger(g, char_script_slot(mob), 389);
  obj_to_char(g, obj, ch);
  CHECK(receive_mtrigger(g, mob, ch, obj) == 0);
  CHECK(find_obj(g, id) == nullptr);
  extract_char(g, mob);
  extract_pending_chars(g);
  CHECK_AUDIT(g);
}

/* A script could force someone to quit and then let their command through. */
static void check_command_runs_for_nobody(GameWorld *g)
{
  struct char_data *holder = th_player(g, "Quitter", ADMLVL_NONE);
  struct obj_data *obj     = object_with_trigger(g, holder, 390);

  th_type(g, holder, "zop");
  CHECK(saw(holder, "Goodbye"));
  CHECK(!saw(holder, "Huh"));
  extract_obj(g, obj);
  th_pulses(g, 3);
  CHECK_AUDIT(g);
}

/* Someone who has just quit is still in the lists for a moment, and used to be teleported back into a room. */
static void check_scripts_ignore_whoever_has_left(GameWorld *g, struct char_data *ch)
{
  room_rnum room = char_get_in_room(ch);

  CHECK(valid_dg_target(g, ch, DG_ALLOW_GODS));
  char_set_in_room(ch, NOWHERE);
  CHECK(!valid_dg_target(g, ch, DG_ALLOW_GODS));
  char_set_in_room(ch, room);
}

/* Changing an object's weight used to leave whoever carries it with the old total. */
static void check_weight_changes_reach_the_carrier(GameWorld *g, struct char_data *ch)
{
  struct obj_data *chest = read_object(g, 325, VIRTUAL); /* a container: the wooden chest was deleted above */
  struct obj_data *bread = read_object(g, 302, VIRTUAL);
  int64_t before;

  obj_to_obj(g, bread, chest);
  obj_to_char(g, chest, ch);
  CHECK_AUDIT(g);
  before = char_get_carry_weight(ch);

  obj_set_weight(bread, obj_get_weight(bread) + 7);
  obj_weight_changed(g, bread, 7);
  CHECK(char_get_carry_weight(ch) == before + 7);
  CHECK_AUDIT(g);

  extract_obj(g, chest);
  CHECK_AUDIT(g);
}

/* A new login used to read rooms[-1] when the start room didn't exist. */
static void check_login_without_a_start_room(GameWorld *g)
{
  struct char_data *late;

  CONFIG_MORTAL_START = 99999;
  CHECK(real_room(g, CONFIG_MORTAL_START) == NOWHERE);
  late = th_player(g, "Latecomer", ADMLVL_NONE);
  CHECK(char_get_in_room(late) == 0);
  CHECK_AUDIT(g);
}

/* A master freed before its follower used to be read by the follower's stop_follower(). */
static void leave_followers_for_shutdown(GameWorld *g, struct char_data *ch)
{
  struct char_data *master = read_mobile(g, 300, VIRTUAL); /* newer, so freed first */

  place(g, ch, 301);
  char_to_room(g, master, char_get_in_room(ch));
  add_follower(g, ch, master);
  CHECK_AUDIT(g);

  /* freeing a player used to free their skills first, then remove this affect, which reads them */
  {
    struct affected_type skilled = {.type = SPELL_SLEEP, .duration = 5, .modifier = 1, .location = APPLY_SKILL, .specific = SKILL_HAYASA};

    affect_to_char(g, ch, &skilled);
  }
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
  check_new_guilds(g);
  check_awkward_commands(g, ch);
  check_experience_width(g, ch);
  check_delete_object_prototype(g, ch);
  check_whois_with_affects(g, ch);
  check_affects_all_count_down(g, ch);
  check_script_loses_its_owner(g);
  check_random_trigger_purges_itself(g, ch);
  check_scripts_ignore_whoever_has_left(g, ch);
  check_weight_changes_reach_the_carrier(g, ch);
  check_time_trigger_purges_itself(g, ch);
  check_cast_trigger_purges_its_target(g, ch);
  check_load_trigger_purges_itself(g, ch);
  check_remove_trigger_on_a_dying_mob(g, ch);
  check_receive_trigger_purges_the_gift(g, ch);
  check_command_runs_for_nobody(g);
  check_timer_trigger_clears_the_room(g, ch);
  leave_followers_for_shutdown(g, ch);
  check_login_without_a_start_room(g);

  th_shutdown(g);
  if (failures)
    printf("%d check(s) failed\n", failures);
  else
    printf("all checks passed\n");
  return failures != 0;
}
