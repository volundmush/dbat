#include "dbat/world_audit.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "dbat/utils.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/room_utils.h"
#include "dbat/descriptor.h"
#include "dbat/dg_scripts.h"
#include "dbat/index.h"
#include "dbat/game_world.h"

constexpr int MAX_REPORTED = 50;

struct pointer_set {
  const void **items;
  size_t count;
};

static bool held_by_a_connection(const GameWorld *g, const struct char_data *ch);

struct audit {
  world_audit_report *report;
  void *context;
  int problems;
  struct pointer_set characters;
  struct pointer_set objects;
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

static bool valid_room(const GameWorld *g, room_rnum room)
{
  return room >= 0 && room <= g->top_room;
}

static void collect(GameWorld *g, struct audit *a)
{
  struct char_data *ch;
  struct obj_data *obj;
  size_t n = 0;

  for (ch = g->characters; ch; ch = char_get_next(ch))
    n++;
  CREATE(a->characters.items, const void *, n + 1);
  for (ch = g->characters; ch; ch = char_get_next(ch))
    a->characters.items[a->characters.count++] = ch;
  set_sort(&a->characters);

  n = 0;
  for (obj = g->objects; obj; obj = obj_get_next(obj))
    n++;
  CREATE(a->objects.items, const void *, n + 1);
  for (obj = g->objects; obj; obj = obj_get_next(obj))
    a->objects.items[a->objects.count++] = obj;
  set_sort(&a->objects);
}

static void audit_rooms(GameWorld *g, struct audit *a)
{
  for (room_rnum r = 0; r <= g->top_room; r++) {
    room_vnum vnum = rroom_get_vnum(g, r);
    size_t limit;

    for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
      struct room_direction_data *exit = rroom_get_dir_option(g, r, dir);

      if (exit && exit->to_room != NOWHERE && !valid_room(g, exit->to_room))
        problem(a, "room %d: exit %d leads to room index %d, which doesn't exist", vnum, dir, exit->to_room);
    }

    limit = a->characters.count + 1;
    for (struct char_data *ch = rroom_get_people(g, r); ch; ch = char_get_next_in_room(ch)) {
      if (!limit--) {
        problem(a, "room %d: the list of people loops", vnum);
        break;
      }
      if (!set_has(&a->characters, ch)) {
        problem(a, "room %d: a person here isn't in the game", vnum);
        break;
      }
      if (char_get_in_room(ch) != r)
        problem(a, "room %d: %s is listed here, but says room index %d", vnum, char_get_name(ch), char_get_in_room(ch));
    }

    limit = a->objects.count + 1;
    for (struct obj_data *obj = rroom_get_contents(g, r); obj; obj = obj_get_next_content(obj)) {
      if (!limit--) {
        problem(a, "room %d: the list of contents loops", vnum);
        break;
      }
      if (!set_has(&a->objects, obj)) {
        problem(a, "room %d: an object here isn't in the game", vnum);
        break;
      }
      if (obj_get_in_room(obj) != r)
        problem(a, "room %d: object %d is listed here, but says room index %d", vnum, obj_get_vnum(g, obj), obj_get_in_room(obj));
    }
  }
}

/* Pairs that always point at each other: if I'm grappling you, you're grappled by me. */
#define AUDIT_PAIR(mine, theirs)                                                                          \
  do {                                                                                                    \
    struct char_data *partner = char_get_##mine(ch);                                                      \
    if (partner) {                                                                                        \
      if (!set_has(&a->characters, partner))                                                              \
        problem(a, "%s: %s someone who isn't in the game", name, #mine);                                  \
      else if (char_get_##theirs(partner) != ch)                                                          \
        problem(a, "%s: %s %s, whose %s says otherwise", name, #mine, char_get_name(partner), #theirs);   \
    }                                                                                                     \
  } while (0)

static void audit_character_links(struct audit *a, struct char_data *ch)
{
  const char *name = char_get_name(ch);
  struct char_data *other;
  struct obj_data *seat;

  AUDIT_PAIR(grappling, grappled);
  AUDIT_PAIR(grappled, grappling);
  AUDIT_PAIR(drag, dragged);
  AUDIT_PAIR(dragged, drag);
  AUDIT_PAIR(absorbing, absorbby);
  AUDIT_PAIR(absorbby, absorbing);
  AUDIT_PAIR(defender, defending);
  AUDIT_PAIR(defending, defender);
  AUDIT_PAIR(blocks, blocked);
  AUDIT_PAIR(blocked, blocks);
  AUDIT_PAIR(carrying, carried_by);
  AUDIT_PAIR(carried_by, carrying);

  /* a mind link can be one-way, and a poisoner can poison many */
  if ((other = char_get_mindlink(ch)) && !set_has(&a->characters, other))
    problem(a, "%s: mind-linked to someone who isn't in the game", name);
  if ((other = char_get_poisonby(ch)) && !set_has(&a->characters, other))
    problem(a, "%s: poisoned by someone who isn't in the game", name);

  if ((seat = char_get_sits(ch))) {
    if (!set_has(&a->objects, seat))
      problem(a, "%s: sits on an object that isn't in the game", name);
    else if (obj_get_sitting(seat) != ch)
      problem(a, "%s: sits on object %d, which says otherwise", name, obj_get_item_number(seat));
  }
}

static void audit_character_values(GameWorld *g, struct audit *a, struct char_data *ch)
{
  const char *name = char_get_name(ch);
  int64_t weight   = 0;
  int items        = 0;
  size_t limit     = a->objects.count + 1;

  if (char_get_position(ch) < 0 || char_get_position(ch) > POS_STANDING)
    problem(a, "%s: position %d doesn't exist", name, char_get_position(ch));
  if (char_get_gold(ch) < 0)
    problem(a, "%s: has negative gold", name);
  if (char_get_bank_gold(ch) < 0)
    problem(a, "%s: has negative gold in the bank", name);
  if (char_get_exp(ch) < 0)
    problem(a, "%s: has negative experience", name);

  for (struct obj_data *obj = char_get_inventory(ch); obj && limit; obj = obj_get_next_content(obj), limit--) {
    weight += obj_get_weight(obj);
    items++;
  }
  if (limit) {
    if (items != char_get_carry_items(ch))
      problem(a, "%s: carries %d items, but counts %d", name, items, char_get_carry_items(ch));
    if (weight != char_get_carry_weight(ch))
      problem(a, "%s: carries a weight of %lld, but counts %d", name, (long long)weight, char_get_carry_weight(ch));
  }
}

static void audit_character(GameWorld *g, struct audit *a, struct char_data *ch)
{
  const char *name = char_get_name(ch);
  room_rnum room   = char_get_in_room(ch);
  size_t limit     = a->objects.count + 1;
  struct char_data *other;

  if (room != NOWHERE && !valid_room(g, room))
    problem(a, "%s: in room index %d, which doesn't exist", name, room);
  else if (room != NOWHERE) {
    size_t people = a->characters.count + 1;

    for (other = rroom_get_people(g, room); other && other != ch && people; other = char_get_next_in_room(other))
      people--;
    if (other != ch)
      problem(a, "%s: says room %d, but isn't listed there", name, rroom_get_vnum(g, room));
  }

  for (struct obj_data *obj = char_get_inventory(ch); obj; obj = obj_get_next_content(obj)) {
    if (!limit--) {
      problem(a, "%s: the inventory loops", name);
      break;
    }
    if (!set_has(&a->objects, obj)) {
      problem(a, "%s: carries an object that isn't in the game", name);
      break;
    }
    if (obj_get_carried_by(obj) != ch)
      problem(a, "%s: carries object %d, which doesn't say so", name, obj_get_vnum(g, obj));
  }

  for (int slot = 0; slot < NUM_WEARS; slot++) {
    struct obj_data *obj = char_get_equipment(ch, slot);

    if (!obj)
      continue;
    if (!set_has(&a->objects, obj))
      problem(a, "%s: wears an object that isn't in the game", name);
    else if (obj_get_worn_by(obj) != ch || obj_get_worn_on(obj) != slot)
      problem(a, "%s: wears object %d in slot %d, which doesn't say so", name, obj_get_vnum(g, obj), slot);
  }

  if ((other = char_get_fighting(ch)) && !set_has(&a->characters, other))
    problem(a, "%s: is fighting someone who isn't in the game", name);
  if ((other = char_get_master(ch)) && !set_has(&a->characters, other))
    problem(a, "%s: follows someone who isn't in the game", name);
  audit_character_links(a, ch);
  audit_character_values(g, a, ch);
  limit = a->characters.count + 1;
  for (struct follow_type *f = char_get_followers(ch); f; f = f->next) {
    if (!limit--) {
      problem(a, "%s: the list of followers loops", name);
      break;
    }
    if (!set_has(&a->characters, f->follower))
      problem(a, "%s: is followed by someone who isn't in the game", name);
    else if (char_get_master(f->follower) != ch)
      problem(a, "%s: lists %s as a follower, who says otherwise", name, char_get_name(f->follower));
  }
}

static bool list_has_object(const struct audit *a, struct obj_data *list, const struct obj_data *obj)
{
  size_t limit = a->objects.count + 1;

  for (; list && limit; list = obj_get_next_content(list), limit--)
    if (list == obj)
      return true;
  return false;
}

/* The other direction from the room and character checks: an object must be listed where it says it is. */
static void audit_object_is_listed(GameWorld *g, struct audit *a, struct obj_data *obj)
{
  obj_vnum vnum            = obj_get_vnum(g, obj);
  room_rnum room           = obj_get_in_room(obj);
  struct char_data *owner  = obj_get_carried_by(obj);
  struct char_data *wearer = obj_get_worn_by(obj);
  struct obj_data *outer   = obj_get_in_obj(obj);

  if (room != NOWHERE && valid_room(g, room) && !list_has_object(a, rroom_get_contents(g, room), obj))
    problem(a, "object %d: says room %d, but isn't listed there", vnum, rroom_get_vnum(g, room));
  if (owner && set_has(&a->characters, owner) && !list_has_object(a, char_get_inventory(owner), obj))
    problem(a, "object %d: says %s carries it, but isn't in the inventory", vnum, char_get_name(owner));
  if (wearer && set_has(&a->characters, wearer) &&
      (obj_get_worn_on(obj) < 0 || obj_get_worn_on(obj) >= NUM_WEARS || char_get_equipment(wearer, obj_get_worn_on(obj)) != obj))
    problem(a, "object %d: says %s wears it, but isn't in that slot", vnum, char_get_name(wearer));
  if (outer && set_has(&a->objects, outer) && !list_has_object(a, obj_get_contains(outer), obj))
    problem(a, "object %d: says it's inside object %d, but isn't listed there", vnum, obj_get_vnum(g, outer));
}

static void audit_object(GameWorld *g, struct audit *a, struct obj_data *obj)
{
  obj_vnum vnum = obj_get_vnum(g, obj);
  int places    = (obj_get_in_room(obj) != NOWHERE) + (obj_get_carried_by(obj) != nullptr) +
               (obj_get_worn_by(obj) != nullptr) + (obj_get_in_obj(obj) != nullptr);
  size_t limit  = a->objects.count + 1;

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

  if (obj_get_sitting(obj)) {
    if (!set_has(&a->characters, obj_get_sitting(obj)))
      problem(a, "object %d: sat on by someone who isn't in the game", vnum);
    else if (char_get_sits(obj_get_sitting(obj)) != obj)
      problem(a, "object %d: sat on by %s, who says otherwise", vnum, char_get_name(obj_get_sitting(obj)));
  }
  if (obj_get_user(obj) && !set_has(&a->characters, obj_get_user(obj)))
    problem(a, "object %d: used by someone who isn't in the game", vnum);
  if (obj_get_target(obj) && !set_has(&a->characters, obj_get_target(obj)))
    problem(a, "object %d: targets someone who isn't in the game", vnum);
  if (obj_get_posted_to(obj) && !set_has(&a->objects, obj_get_posted_to(obj)))
    problem(a, "object %d: posted to an object that isn't in the game", vnum);
  if (obj_get_fellow_wall(obj) && !set_has(&a->objects, obj_get_fellow_wall(obj)))
    problem(a, "object %d: joined to a wall that isn't in the game", vnum);

  for (struct obj_data *inner = obj_get_contains(obj); inner; inner = obj_get_next_content(inner)) {
    if (!limit--) {
      problem(a, "object %d: the list of contents loops", vnum);
      break;
    }
    if (!set_has(&a->objects, inner)) {
      problem(a, "object %d: contains an object that isn't in the game", vnum);
      break;
    }
    if (obj_get_in_obj(inner) != obj)
      problem(a, "object %d: contains object %d, which doesn't say so", vnum, obj_get_vnum(g, inner));
  }
}

static void audit_connections(const GameWorld *g, struct audit *a)
{
  for (const struct descriptor_data *d = g->descriptors; d; d = d->next) {
    bool found = !d->snooping;

    for (const struct descriptor_data *other = g->descriptors; other && !found; other = other->next)
      found = other == d->snooping;
    if (!found)
      problem(a, "a connection is snooping one that has closed");
    else if (d->snooping && d->snooping->snoop_by != d)
      problem(a, "a connection is snooping one that says otherwise");

    found = !d->snoop_by;
    for (const struct descriptor_data *other = g->descriptors; other && !found; other = other->next)
      found = other == d->snoop_by;
    if (!found)
      problem(a, "a connection is snooped on by one that has closed");
  }
}

/* The index counts how many of each thing exist, and zone resets and limits go by it. */
static void audit_index_counts(GameWorld *g, struct audit *a)
{
  int *counts;

  CREATE(counts, int, g->top_obj + 1);
  for (struct obj_data *obj = g->objects; obj; obj = obj_get_next(obj)) {
    obj_rnum rnum = obj_get_item_number(obj);

    if (rnum != NOTHING && (rnum < 0 || rnum > g->top_obj))
      problem(a, "object: prototype index %d doesn't exist", rnum);
    else if (rnum != NOTHING)
      counts[rnum]++;
  }
  for (obj_rnum r = 0; r <= g->top_obj; r++)
    if (counts[r] != g->obj_index[r].number)
      problem(a, "object %d: %d exist, but the index counts %d", g->obj_index[r].vnum, counts[r], g->obj_index[r].number);
  free(counts);

  CREATE(counts, int, g->top_mob + 1);
  for (struct char_data *ch = g->characters; ch; ch = char_get_next(ch)) {
    if (!char_is_npc(ch))
      continue;
    if (char_get_nr(ch) != NOBODY && (char_get_nr(ch) < 0 || char_get_nr(ch) > g->top_mob))
      problem(a, "%s: prototype index %d doesn't exist", char_get_name(ch), char_get_nr(ch));
    else if (char_get_nr(ch) != NOBODY)
      counts[char_get_nr(ch)]++;
  }
  for (mob_rnum r = 0; r <= g->top_mob; r++)
    if (counts[r] != g->mob_index[r].number)
      problem(a, "mob %d: %d exist, but the index counts %d", g->mob_index[r].vnum, counts[r], g->mob_index[r].number);
  free(counts);
}

/* Scripts find things through this table, by id. */
static void audit_lookup_table(const GameWorld *g, struct audit *a)
{
  for (int bucket = 0; bucket < BUCKET_COUNT; bucket++)
    for (const struct lookup_table_t *entry = g->lookup_table[bucket].next; entry; entry = entry->next) {
      if (entry->type == OBJ_TRIGGER) {
        if (!set_has(&a->objects, entry->c))
          problem(a, "lookup table: id %ld is an object that isn't in the game", entry->uid);
        else if (obj_get_id(entry->c) != entry->uid)
          problem(a, "lookup table: id %ld is an object that says %d", entry->uid, obj_get_id(entry->c));
      } else if (entry->type == MOB_TRIGGER) {
        if (!set_has(&a->characters, entry->c) && !held_by_a_connection(g, entry->c))
          problem(a, "lookup table: id %ld is someone who isn't in the game", entry->uid);
      }
    }
}

static bool held_by_a_connection(const GameWorld *g, const struct char_data *ch)
{
  for (const struct descriptor_data *d = g->descriptors; d; d = d->next)
    if (d->character == ch || d->original == ch)
      return true;
  return false;
}

int world_audit(GameWorld *g, world_audit_report *report, void *context)
{
  struct audit a = {.report = report, .context = context};
  size_t limit;

  collect(g, &a);
  audit_rooms(g, &a);
  for (struct char_data *ch = g->characters; ch; ch = char_get_next(ch))
    audit_character(g, &a, ch);
  for (struct obj_data *obj = g->objects; obj; obj = obj_get_next(obj))
    audit_object(g, &a, obj);

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
    if (!char_get_fighting(ch))
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

  for (struct descriptor_data *d = g->descriptors; d; d = d->next)
    if (d->character && desc_get_connected(d) == CON_PLAYING && !set_has(&a.characters, d->character))
      problem(&a, "a connection is playing a character who isn't in the game");

  audit_connections(g, &a);
  audit_index_counts(g, &a);
  audit_lookup_table(g, &a);

  free(a.characters.items);
  free(a.objects.items);
  return a.problems;
}
