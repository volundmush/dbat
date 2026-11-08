#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>

#include "dbat/world_audit.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/index.hpp"
#include "dbat/game_world.hpp"

constexpr int MAX_REPORTED = 50;

struct pointer_set {
  const void **items{};
  size_t count{};
};

static bool held_by_a_connection(const GameWorld *g, const struct char_data *ch);

struct audit {
  world_audit_report *report{};
  void *context{};
  int problems{};
  struct pointer_set characters{};
  struct pointer_set objects{};
};

__attribute__((format(printf, 2, 3)))
static void problem(struct audit *a, const char *format, ...)
{
  char text[MAX_STRING_LENGTH];
  va_list args;

  if (a->problems++ >= MAX_REPORTED)
    return;
  va_start(args, format);
  vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  a->report(a->context, text);
}

static int compare_pointers(const void *x, const void *y)
{
  const void *a = *(const void *const *)x, *b = *(const void *const *)y;

  return (a > b) - (a < b);
}

static void set_sort(struct pointer_set *set)
{
  if (set->count)
    qsort(set->items, set->count, sizeof(*set->items), compare_pointers);
}

static bool set_has(const struct pointer_set *set, const void *item)
{
  return set->count && bsearch(&item, set->items, set->count, sizeof(*set->items), compare_pointers);
}

static bool valid_room(const GameWorld *g, room_vnum room)
{
  return g->rooms.find(room) != nullptr;
}

static void collect(GameWorld *g, struct audit *a)
{
  a->characters.items = new const void *[g->characters.size() + 1]();
  g->characters.visit([a](long, const struct char_data *ch) { a->characters.items[a->characters.count++] = ch; });
  set_sort(&a->characters);

  a->objects.items = new const void *[g->objects.size() + 1]();
  g->objects.visit([a](long, const struct obj_data *obj) { a->objects.items[a->objects.count++] = obj; });
  set_sort(&a->objects);
}

/* This runs after every step of the bot tests, so it reads each room's fields directly rather than looking the room
 * up for every one. */
static void audit_rooms(GameWorld *g, struct audit *a)
{
  g->rooms.visit([&](room_vnum vnum, const struct room_data *room) {

    for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
      const struct room_direction_data *exit = room->dir_option[dir];

      if (exit && exit->to_room != NOWHERE && !valid_room(g, exit->to_room))
        problem(a, "room %d: exit %d leads to room %d, which doesn't exist", vnum, dir, exit->to_room);
    }

    bool missing = false;

    if (room->people.has_duplicates())
      problem(a, "room %d: someone is listed here twice", vnum);
    room->people.each([&](long id) {
      struct char_data *ch = char_by_id(g, id);
      if (!ch)
        missing = true;
      else if (char_get_in_room(ch) != vnum)
        problem(a, "room %d: %s is listed here, but says room %d", vnum, char_get_name(ch), char_get_in_room(ch));
    });
    if (missing)
      problem(a, "room %d: a person here isn't in the game", vnum);

    missing = false;
    if (room->contents.has_duplicates())
      problem(a, "room %d: an object is listed here twice", vnum);
    room->contents.each([&](long id) {
      struct obj_data *obj = obj_by_id(g, id);
      if (!obj)
        missing = true;
      else if (obj_get_in_room(obj) != vnum)
        problem(a, "room %d: object %d is listed here, but says room %d", vnum, obj_get_vnum(g, obj), obj_get_in_room(obj));
    });
    if (missing)
      problem(a, "room %d: an object here isn't in the game", vnum);
  });
}

/* References between entities are ids. One that is set but leads nowhere is the id of something that has gone. */

/* Pairs that always point at each other: if I'm grappling you, you're grappled by me. */
#define AUDIT_PAIR(uid, mine, theirs)                                                                   \
  do {                                                                                                  \
    if (uid) {                                                                                          \
      struct char_data *partner = char_get_##mine(g, ch);                                               \
      if (!partner || !set_has(&a->characters, partner))                                                \
        problem(a, "%s: %s someone who isn't in the game", name, #mine);                                \
      else if (char_get_##theirs(g, partner) != ch)                                                     \
        problem(a, "%s: %s %s, whose %s says otherwise", name, #mine, char_get_name(partner), #theirs); \
    }                                                                                                   \
  } while (0)

static void audit_character_links(GameWorld *g, struct audit *a, struct char_data *ch)
{
  const char *name                     = char_get_name(ch);
  const struct player_special_data *ps = ch->player_specials;
  struct obj_data *seat;

  AUDIT_PAIR(ch->grappling_uid, grappling, grappled);
  AUDIT_PAIR(ch->grappled_uid, grappled, grappling);
  AUDIT_PAIR(ch->drag_uid, drag, dragged);
  AUDIT_PAIR(ch->dragged_uid, dragged, drag);
  AUDIT_PAIR(ch->absorbing_uid, absorbing, absorbby);
  AUDIT_PAIR(ch->absorbby_uid, absorbby, absorbing);
  AUDIT_PAIR(ch->defender_uid, defender, defending);
  AUDIT_PAIR(ch->defending_uid, defending, defender);
  AUDIT_PAIR(ch->blocks_uid, blocks, blocked);
  AUDIT_PAIR(ch->blocked_uid, blocked, blocks);
  AUDIT_PAIR(ps ? ps->carrying_uid : 0, carrying, carried_by);
  AUDIT_PAIR(ps ? ps->carried_by_uid : 0, carried_by, carrying);

  /* a mind link can be one-way, and a poisoner can poison many */
  if (ch->mindlink_uid && !set_has(&a->characters, char_get_mindlink(g, ch)))
    problem(a, "%s: mind-linked to someone who isn't in the game", name);
  if (ch->poisonby_uid && !set_has(&a->characters, char_get_poisonby(g, ch)))
    problem(a, "%s: poisoned by someone who isn't in the game", name);
  if (ch->master_uid && !set_has(&a->characters, char_get_master(g, ch)))
    problem(a, "%s: follows someone who isn't in the game", name);
  if (ch->fighting_uid && !set_has(&a->characters, char_get_fighting(g, ch)))
    problem(a, "%s: fighting someone who isn't in the game", name);

  if (ch->sits_uid) {
    if (!(seat = char_get_sits(g, ch)) || !set_has(&a->objects, seat))
      problem(a, "%s: sits on an object that isn't in the game", name);
    else if (obj_get_sitting(g, seat) != ch)
      problem(a, "%s: sits on object %d, which says otherwise", name, obj_get_item_number(seat));
  }
}

static void audit_character_values(GameWorld *g, struct audit *a, struct char_data *ch)
{
  const char *name = char_get_name(ch);
  int64_t weight   = 0;
  int items        = 0;

  if (char_get_position(ch) < 0 || char_get_position(ch) > POS_STANDING)
    problem(a, "%s: position %d doesn't exist", name, char_get_position(ch));
  if (char_get_gold(ch) < 0)
    problem(a, "%s: has negative gold", name);
  if (char_get_bank_gold(ch) < 0)
    problem(a, "%s: has negative gold in the bank", name);
  if (char_get_exp(ch) < 0)
    problem(a, "%s: has negative experience", name);

  char_get_inventory(ch).each([&](long id) {
    if (const struct obj_data *obj = obj_by_id(g, id)) {
      weight += obj_get_weight(obj);
      items++;
    }
  });
  if (items != char_get_carry_items(ch))
    problem(a, "%s: carries %d items, but counts %d", name, items, char_get_carry_items(ch));
  if (weight != char_get_carry_weight(ch))
    problem(a, "%s: carries a weight of %lld, but counts %d", name, (long long)weight, char_get_carry_weight(ch));
}

static void audit_character(GameWorld *g, struct audit *a, struct char_data *ch)
{
  const char *name = char_get_name(ch);
  room_vnum room   = char_get_in_room(ch);
  size_t limit     = a->objects.count + 1;
  struct char_data *other;

  if (room != NOWHERE && !valid_room(g, room))
    problem(a, "%s: in room index %d, which doesn't exist", name, room);
  else if (room != NOWHERE) {
    if (!room_get_people(g, room).contains(char_get_id(ch)))
      problem(a, "%s: says room %d, but isn't listed there", name, room);
  }

  bool missing = false;

  if (char_get_inventory(ch).has_duplicates())
    problem(a, "%s: carries an object twice", name);
  char_get_inventory(ch).each([&](long id) {
    struct obj_data *obj = obj_by_id(g, id);
    if (!obj)
      missing = true;
    else if (obj_get_carried_by(obj) != ch)
      problem(a, "%s: carries object %d, which doesn't say so", name, obj_get_vnum(g, obj));
  });
  if (missing)
    problem(a, "%s: carries an object that isn't in the game", name);

  for (int slot = 0; slot < NUM_WEARS; slot++) {
    struct obj_data *obj = char_get_equipment(ch, slot);

    if (!obj)
      continue;
    if (!set_has(&a->objects, obj))
      problem(a, "%s: wears an object that isn't in the game", name);
    else if (obj_get_worn_by(obj) != ch || obj_get_worn_on(obj) != slot)
      problem(a, "%s: wears object %d in slot %d, which doesn't say so", name, obj_get_vnum(g, obj), slot);
  }

  if ((other = char_get_fighting(g, ch)) && !set_has(&a->characters, other))
    problem(a, "%s: is fighting someone who isn't in the game", name);
  if ((other = char_get_master(g, ch)) && !set_has(&a->characters, other))
    problem(a, "%s: follows someone who isn't in the game", name);
  audit_character_links(g, a, ch);
  audit_character_values(g, a, ch);
  limit = a->characters.count + 1;
  for (struct follow_type *f = char_get_followers(ch); f; f = f->next) {
    if (!limit--) {
      problem(a, "%s: the list of followers loops", name);
      break;
    }
    if (!set_has(&a->characters, f->follower))
      problem(a, "%s: is followed by someone who isn't in the game", name);
    else if (char_get_master(g, f->follower) != ch)
      problem(a, "%s: lists %s as a follower, who says otherwise", name, char_get_name(f->follower));
  }
}

static bool list_has_object(const id_list &list, const struct obj_data *obj)
{
  return list.contains(obj_get_id(obj));
}

/* The other direction from the room and character checks: an object must be listed where it says it is. */
static void audit_object_is_listed(GameWorld *g, struct audit *a, struct obj_data *obj)
{
  obj_vnum vnum            = obj_get_vnum(g, obj);
  room_vnum room           = obj_get_in_room(obj);
  struct char_data *owner  = obj_get_carried_by(obj);
  struct char_data *wearer = obj_get_worn_by(obj);
  struct obj_data *outer   = obj_get_in_obj(obj);

  if (room != NOWHERE && valid_room(g, room) && !list_has_object(room_get_contents(g, room), obj))
    problem(a, "object %d: says room %d, but isn't listed there", vnum, room);
  if (owner && set_has(&a->characters, owner) && !list_has_object(char_get_inventory(owner), obj))
    problem(a, "object %d: says %s carries it, but isn't in the inventory", vnum, char_get_name(owner));
  if (wearer && set_has(&a->characters, wearer) &&
      (obj_get_worn_on(obj) < 0 || obj_get_worn_on(obj) >= NUM_WEARS || char_get_equipment(wearer, obj_get_worn_on(obj)) != obj))
    problem(a, "object %d: says %s wears it, but isn't in that slot", vnum, char_get_name(wearer));
  if (outer && set_has(&a->objects, outer) && !list_has_object(obj_get_contains(outer), obj))
    problem(a, "object %d: says it's inside object %d, but isn't listed there", vnum, obj_get_vnum(g, outer));
}

static void audit_object(GameWorld *g, struct audit *a, struct obj_data *obj)
{
  obj_vnum vnum = obj_get_vnum(g, obj);
  int places    = (obj_get_in_room(obj) != NOWHERE) + (obj_get_carried_by(obj) != nullptr) +
               (obj_get_worn_by(obj) != nullptr) + (obj_get_in_obj(obj) != nullptr);

  if (places > 1)
    problem(a, "object %d: is in %d places at once", vnum, places);
  if (obj_get_in_room(obj) != NOWHERE && !valid_room(g, obj_get_in_room(obj)))
    problem(a, "object %d: in room index %d, which doesn't exist", vnum, obj_get_in_room(obj));
  if (obj_get_carried_by(obj) && !set_has(&a->characters, obj_get_carried_by(obj)))
    problem(a, "object %d: carried by someone who isn't in the game", vnum);
  if (obj_get_worn_by(obj) && !set_has(&a->characters, obj_get_worn_by(obj)))
    problem(a, "object %d: worn by someone who isn't in the game", vnum);
  if (obj_get_in_obj(obj) && !set_has(&a->objects, obj_get_in_obj(obj)))
    problem(a, "object %d: inside an object that isn't in the game", vnum);

  audit_object_is_listed(g, a, obj);

  if (obj->sitting_uid) {
    struct char_data *sitter = obj_get_sitting(g, obj);
    if (!sitter || !set_has(&a->characters, sitter))
      problem(a, "object %d: sat on by someone who isn't in the game", vnum);
    else if (char_get_sits(g, sitter) != obj)
      problem(a, "object %d: sat on by %s, who says otherwise", vnum, char_get_name(sitter));
  }
  if (obj->user_uid && !set_has(&a->characters, obj_get_user(g, obj)))
    problem(a, "object %d: used by someone who isn't in the game", vnum);
  if (obj->target_uid && !set_has(&a->characters, obj_get_target(g, obj)))
    problem(a, "object %d: targets someone who isn't in the game", vnum);
  if (obj->posted_to_uid && !set_has(&a->objects, obj_get_posted_to(g, obj)))
    problem(a, "object %d: posted to an object that isn't in the game", vnum);
  if (obj->fellow_wall_uid && !set_has(&a->objects, obj_get_fellow_wall(g, obj)))
    problem(a, "object %d: joined to a wall that isn't in the game", vnum);

  bool missing = false;

  if (obj_get_contains(obj).has_duplicates())
    problem(a, "object %d: contains an object twice", vnum);
  obj_get_contains(obj).each([&](long id) {
    struct obj_data *inner = obj_by_id(g, id);
    if (!inner)
      missing = true;
    else if (obj_get_in_obj(inner) != obj)
      problem(a, "object %d: contains object %d, which doesn't say so", vnum, obj_get_vnum(g, inner));
  });
  if (missing)
    problem(a, "object %d: contains an object that isn't in the game", vnum);
}

/* Is d an open connection? Compares pointers only, as d may have been freed. */
static bool is_open(const GameWorld *g, const struct descriptor_data *d)
{
  bool found = false;
  g->descriptors.visit([&](long, const struct descriptor_data *other) { found = found || other == d; });
  return found;
}

static void audit_connections(const GameWorld *g, struct audit *a)
{
  g->descriptors.visit([&](long, const struct descriptor_data *d) {
    if (d->snooping && !is_open(g, d->snooping))
      problem(a, "a connection is snooping one that has closed");
    else if (d->snooping && d->snooping->snoop_by != d)
      problem(a, "a connection is snooping one that says otherwise");

    if (d->snoop_by && !is_open(g, d->snoop_by))
      problem(a, "a connection is snooped on by one that has closed");
  });
}

/* The index counts how many of each thing exist, and zone resets and limits go by it. */
static void audit_index_counts(GameWorld *g, struct audit *a)
{
  /* Both sides sorted by vnum, so one pass compares them: this runs after every step of every bot. */
  std::vector<obj_vnum> vnums;
  size_t i = 0;

  g->objects.visit([&](long, const struct obj_data *obj) {
    if (obj_get_item_number(obj) != NOTHING)
      vnums.push_back(obj_get_item_number(obj));
  });
  std::sort(vnums.begin(), vnums.end());
  g->obj_index.visit([&](obj_vnum r, const index_data *index) {
    int found = 0;
    for (; i < vnums.size() && vnums[i] < r; i++)
      problem(a, "object: prototype %d doesn't exist", vnums[i]);
    for (; i < vnums.size() && vnums[i] == r; i++)
      found++;
    if (found != index->number)
      problem(a, "object %d: %d exist, but the index counts %d", r, found, index->number);
  });
  for (; i < vnums.size(); i++)
    problem(a, "object: prototype %d doesn't exist", vnums[i]);

  vnums.clear();
  i = 0;
  g->characters.visit([&](long, const struct char_data *ch) {
    if (char_is_npc(ch) && char_get_nr(ch) != NOBODY)
      vnums.push_back(char_get_nr(ch));
  });
  std::sort(vnums.begin(), vnums.end());
  g->mob_index.visit([&](mob_vnum r, const index_data *index) {
    int found = 0;
    for (; i < vnums.size() && vnums[i] < r; i++)
      problem(a, "mob: prototype %d doesn't exist", vnums[i]);
    for (; i < vnums.size() && vnums[i] == r; i++)
      found++;
    if (found != index->number)
      problem(a, "mob %d: %d exist, but the index counts %d", r, found, index->number);
  });
  for (; i < vnums.size(); i++)
    problem(a, "mob: prototype %d doesn't exist", vnums[i]);
}

static bool held_by_a_connection(const GameWorld *g, const struct char_data *ch)
{
  bool held = false;
  g->descriptors.visit([&](long, const struct descriptor_data *d) { held = held || d->character == ch || d->original == ch; });
  return held;
}

int world_audit(GameWorld *g, world_audit_report *report, void *context)
{
  struct audit a = {.report = report, .context = context};
  size_t limit;

  collect(g, &a);
  audit_rooms(g, &a);
  /* Loops over entities (iterate.hpp) find each one again by the id it is registered under */
  g->characters.visit([&](long id, struct char_data *ch) {
    audit_character(g, &a, ch);
    if (char_get_id(ch) != id)
      problem(&a, "%s: registered as id %ld, but says id %d", char_get_name(ch), id, char_get_id(ch));
  });
  g->objects.visit([&](long id, struct obj_data *obj) {
    audit_object(g, &a, obj);
    if (obj_get_id(obj) != id)
      problem(&a, "%s: registered as id %ld, but says id %d", obj_get_short_description(obj), id, obj_get_id(obj));
  });

  limit = a.characters.count + 1;
  for (struct char_data *ch = g->combat_list; ch; ch = char_get_next_fighting(ch)) {
    if (!limit--) {
      problem(&a, "the combat list loops");
      break;
    }
    if (!set_has(&a.characters, ch)) {
      problem(&a, "the combat list holds someone who isn't in the game");
      break;
    }
    if (!char_get_fighting(g, ch))
      problem(&a, "%s: is in the combat list, but isn't fighting", char_get_name(ch));
  }

  limit = a.characters.count + 1;
  for (struct char_data *ch = g->affect_list; ch; ch = char_get_next_affect(ch)) {
    if (!limit--) {
      problem(&a, "the affect list loops");
      break;
    }
    /* someone who has just quit stays attached to their connection, affects and all, at the menu */
    if (!set_has(&a.characters, ch) && !held_by_a_connection(g, ch)) {
      problem(&a, "the affect list holds someone who isn't in the game");
      break;
    }
  }

  g->descriptors.visit([&](long, struct descriptor_data *d) {
    if (d->character && desc_get_connected(d) == CON_PLAYING && !set_has(&a.characters, d->character))
      problem(&a, "a connection is playing a character who isn't in the game");
  });

  audit_connections(g, &a);
  audit_index_counts(g, &a);

  delete[] a.characters.items;
  delete[] a.objects.items;
  return a.problems;
}
