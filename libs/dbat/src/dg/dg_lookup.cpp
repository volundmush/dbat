/*
 * DG Scripts: finding characters, objects and rooms for scripts, by name or UID, and the UID lookup table.
 * Split out of dg_scripts.c.
 */
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

/* Local functions not used elsewhere */
static struct char_data *find_char_by_uid_in_lookup_table(GameWorld *g, long uid);
static struct obj_data *find_obj_by_uid_in_lookup_table(GameWorld *g, long uid);

/* Return pointer to first occurrence of string ct in */
/* cs, or NULL if not present.  Case insensitive */
const char *str_str(const char *cs, const char *ct)
{
  const char *s, *t;

  if (!cs || !ct || !*ct)
    return nullptr;

  while (*cs) {
    t = ct;

    while (*cs && (tolower(*cs) != tolower(*t)))
      cs++;

    s = cs;

    while (*t && *cs && (tolower(*cs) == tolower(*t))) {
      t++;
      cs++;
    }

    if (!*t)
      return s;
  }

  return nullptr;
}

int trgvar_in_room(GameWorld *g, room_vnum vnum)
{
  room_vnum rnum = vnum;
  int i          = 0;

  if (!room_of(g, rnum)) {
    script_log(g, "people.vnum: world[rnum] does not exist");
    return (-1);
  }

  i += count_members(room_people(g, rnum));

  return i;
}

obj_data *get_obj_in_list(GameWorld *g, std::string_view name, const id_list &list)
{
  long id;

  if (name.starts_with(UID_CHAR)) {
    id = parse_int(name.substr(1));
    for (obj_data *i : obj_list(g, list))
      if (id == obj_get_id(i))
        return i;
  } else {
    for (obj_data *i : obj_list(g, list))
      if (isname(name, obj_get_name(i)))
        return i;
  }

  return nullptr;
}

obj_data *get_object_in_equip(GameWorld *g, char_data *ch, std::string_view name)
{
  int j, n = 0, number;
  obj_data *obj;
  long id;

  if (name.starts_with(UID_CHAR)) {
    id = parse_int(name.substr(1));

    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = char_get_equipment(ch, j)))
        if (id == obj_get_id(obj))
          return (obj);
  } else if (is_number(name)) {
    obj_vnum ovnum = parse_int(name);
    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = char_get_equipment(ch, j)))
        if (obj_get_vnum(g, obj) == ovnum)
          return (obj);
  } else {
    std::string tmp(name);
    if (!(number = get_number(tmp)))
      return nullptr;

    for (j = 0; (j < NUM_WEARS) && (n <= number); j++)
      if ((obj = char_get_equipment(ch, j)))
        if (isname(tmp, obj_get_name(obj)))
          if (++n == number)
            return (obj);
  }

  return nullptr;
}

/* Handles 'held', 'light' and 'wield' positions - Welcor
   After idea from Byron Ellacott - bje@apnic.net */
int find_eq_pos_script(std::string_view arg)
{
  int i;
  struct eq_pos_list {
    const char *pos{};
    int where{};
  } eq_pos[] = {
      {"hold",      WEAR_WIELD2  },
      {"held",      WEAR_WIELD2  },
      {"light",     WEAR_WIELD2  },
      {"wield",     WEAR_WIELD1  },
      {"rfinger",   WEAR_FINGER_R},
      {"lfinger",   WEAR_FINGER_L},
      {"neck1",     WEAR_NECK_1  },
      {"neck2",     WEAR_NECK_2  },
      {"body",      WEAR_BODY    },
      {"head",      WEAR_HEAD    },
      {"legs",      WEAR_LEGS    },
      {"feet",      WEAR_FEET    },
      {"hands",     WEAR_HANDS   },
      {"arms",      WEAR_ARMS    },
      {"shield",    WEAR_WIELD2  },
      {"about",     WEAR_ABOUT   },
      {"waist",     WEAR_WAIST   },
      {"rwrist",    WEAR_WRIST_R },
      {"lwrist",    WEAR_WRIST_L },
      {"backpack",  WEAR_BACKPACK},
      {"rear",      WEAR_EAR_R   },
      {"lear",      WEAR_EAR_L   },
      {"shoulders", WEAR_SH      },
      {"scouter",   WEAR_EYE     },
      {"none",      -1           }
  };

  if (is_number(arg) && (i = parse_int(arg)) >= 0 && i < NUM_WEARS)
    return i;

  for (i = 0; eq_pos[i].where != -1; i++) {
    if (str_eq_nocase(eq_pos[i].pos, arg))
      return eq_pos[i].where;
  }
  return (-1);
}

bool can_wear_on_pos(struct obj_data *obj, int pos)
{
  switch (pos) {
    case WEAR_WIELD1: return obj_wear_flagged(obj, ITEM_WEAR_WIELD);
    case WEAR_WIELD2: return obj_wear_flagged(obj, ITEM_WEAR_TAKE);
    case WEAR_FINGER_R:
    case WEAR_FINGER_L: return obj_wear_flagged(obj, ITEM_WEAR_FINGER);
    case WEAR_NECK_1:
    case WEAR_NECK_2: return obj_wear_flagged(obj, ITEM_WEAR_NECK);
    case WEAR_BODY: return obj_wear_flagged(obj, ITEM_WEAR_BODY);
    case WEAR_HEAD: return obj_wear_flagged(obj, ITEM_WEAR_HEAD);
    case WEAR_LEGS: return obj_wear_flagged(obj, ITEM_WEAR_LEGS);
    case WEAR_FEET: return obj_wear_flagged(obj, ITEM_WEAR_FEET);
    case WEAR_HANDS: return obj_wear_flagged(obj, ITEM_WEAR_HANDS);
    case WEAR_ARMS: return obj_wear_flagged(obj, ITEM_WEAR_ARMS);
    case WEAR_ABOUT: return obj_wear_flagged(obj, ITEM_WEAR_ABOUT);
    case WEAR_WAIST: return obj_wear_flagged(obj, ITEM_WEAR_WAIST);
    case WEAR_WRIST_R:
    case WEAR_WRIST_L: return obj_wear_flagged(obj, ITEM_WEAR_WRIST);
    case WEAR_BACKPACK: return obj_wear_flagged(obj, ITEM_WEAR_PACK);
    case WEAR_EAR_R:
    case WEAR_EAR_L: return obj_wear_flagged(obj, ITEM_WEAR_EAR);
    case WEAR_SH: return obj_wear_flagged(obj, ITEM_WEAR_SH);
    case WEAR_EYE: return obj_wear_flagged(obj, ITEM_WEAR_EYE);
    default: return false;
  }
}

/************************************************************
 * search by number routines
 ************************************************************/

/* return char with UID n */
struct char_data *find_char(GameWorld *g, long n)
{
  /* Players and legacy mobs live below ROOM_ID_BASE; mobs from MOB_ID_BASE. */
  if (n < 0 || (n >= ROOM_ID_BASE && n < MOB_ID_BASE)) /* See note in dg_scripts.hpp */
    return nullptr;

  return find_char_by_uid_in_lookup_table(g, n);
}

/* return object with UID n */
obj_data *find_obj(GameWorld *g, long n)
{
  if (n < OBJ_ID_BASE || n > OBJ_ID_MAX) /* see note in dg_scripts.hpp */
    return nullptr;

  return find_obj_by_uid_in_lookup_table(g, n);
}

/* return room with UID n */
room_data *find_room(GameWorld *g, long n)
{
  room_vnum rnum;

  n -= ROOM_ID_BASE;
  if (n < 0)
    return nullptr;
  rnum = (room_vnum)n;

  if (room_of(g, rnum))
    return room_of(g, rnum);

  return nullptr;
}

/************************************************************
 * generic searches based only on name
 ************************************************************/

/* search the entire world for a char, and return a pointer */
char_data *get_char(GameWorld *g, std::string_view name)
{
  char_data *i;

  if (name.starts_with(UID_CHAR)) {
    i = find_char(g, parse_int(name.substr(1)));

    if (i && valid_dg_target(g, i, DG_ALLOW_GODS))
      return i;
  } else {
    for (char_data *tch : each_char(g))
      if (isname(name, char_get_alias(tch)) &&
          valid_dg_target(g, tch, DG_ALLOW_GODS))
        return tch;
  }

  return nullptr;
}

/*
 * Finds a char in the same room as the object with the name 'name'
 */
char_data *get_char_near_obj(GameWorld *g, obj_data *obj, std::string_view name)
{
  char_data *ch;

  if (name.starts_with(UID_CHAR)) {
    ch = find_char(g, parse_int(name.substr(1)));

    if (ch && valid_dg_target(g, ch, DG_ALLOW_GODS))
      return ch;
  } else {
    room_vnum num;
    if ((num = obj_room(obj)) != NOWHERE)
      for (char_data *tch : room_people(g, num))
        if (isname(name, char_get_alias(tch)) &&
            valid_dg_target(g, tch, DG_ALLOW_GODS))
          return tch;
  }

  return nullptr;
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_in_room(GameWorld *g, room_data *room, std::string_view name)
{
  char_data *ch;

  if (name.starts_with(UID_CHAR)) {
    ch = find_char(g, parse_int(name.substr(1)));

    if (ch && valid_dg_target(g, ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (char_data *tch : room_people(g, room))
      if (isname(name, char_get_alias(tch)) &&
          valid_dg_target(g, tch, DG_ALLOW_GODS))
        return tch;
  }

  return nullptr;
}

/* searches the room with the object for an object with name 'name'*/

obj_data *get_obj_near_obj(GameWorld *g, obj_data *obj, std::string_view name)
{
  obj_data *i = nullptr;
  room_vnum rm;
  long id;

  if (str_eq_nocase(name, "self") || str_eq_nocase(name, "me"))
    return obj;

  /* is it inside ? */
  if (obj_get_contains(obj) && (i = get_obj_in_list(g, name, obj_get_contains(obj))))
    return i;

  /* or outside ? */
  if (obj_get_in_obj(obj)) {
    if (name.starts_with(UID_CHAR)) {
      id = parse_int(name.substr(1));

      if (id == obj_get_id(obj_get_in_obj(obj)))
        return obj_get_in_obj(obj);
    } else if (isname(name, obj_get_name(obj_get_in_obj(obj))))
      return obj_get_in_obj(obj);
  }
  /* or worn ?*/
  else if (obj_get_worn_by(obj) && (i = get_object_in_equip(g, obj_get_worn_by(obj), name)))
    return i;
  /* or carried ? */
  else if (obj_get_carried_by(obj) &&
           (i = get_obj_in_list(g, name, char_get_inventory(obj_get_carried_by(obj)))))
    return i;
  else if ((rm = obj_room(obj)) != NOWHERE) {
    /* check the floor */
    if ((i = get_obj_in_list(g, name, room_get_contents(g, rm))))
      return i;

    /* check peoples' inventory */
    for (char_data *ch : room_people(g, rm))
      if ((i = get_object_in_equip(g, ch, name)))
        return i;
  }
  return nullptr;
}

/* returns the object in the world with name name, or NULL if not found */
obj_data *get_obj(GameWorld *g, std::string_view name)
{

  if (name.starts_with(UID_CHAR))
    return find_obj(g, parse_int(name.substr(1)));
  else {
    for (obj_data *obj : each_obj(g))
      if (isname(name, obj_get_name(obj)))
        return obj;
  }

  return nullptr;
}

/* finds room by id or vnum.  returns NULL if not found */
room_data *get_room(GameWorld *g, std::string_view name)
{
  room_vnum nr;

  if (name.starts_with(UID_CHAR))
    return find_room(g, parse_int(name.substr(1)));
  else if (!room_of(g, nr = parse_int(name)))
    return nullptr;
  else
    return room_of(g, nr);
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching with the person owing the object
 */
char_data *get_char_by_obj(GameWorld *g, obj_data *obj, std::string_view name)
{
  char_data *ch;

  if (name.starts_with(UID_CHAR)) {
    ch = find_char(g, parse_int(name.substr(1)));

    if (ch && valid_dg_target(g, ch, DG_ALLOW_GODS))
      return ch;
  } else {
    if (obj_get_carried_by(obj) &&
        isname(name, char_get_alias(obj_get_carried_by(obj))) &&
        valid_dg_target(g, obj_get_carried_by(obj), DG_ALLOW_GODS))
      return obj_get_carried_by(obj);

    if (obj_get_worn_by(obj) &&
        isname(name, char_get_alias(obj_get_worn_by(obj))) &&
        valid_dg_target(g, obj_get_worn_by(obj), DG_ALLOW_GODS))
      return obj_get_worn_by(obj);

    for (char_data *tch : each_char(g))
      if (isname(name, char_get_alias(tch)) &&
          valid_dg_target(g, tch, DG_ALLOW_GODS))
        return tch;
  }

  return nullptr;
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_by_room(GameWorld *g, room_data *room, std::string_view name)
{
  char_data *ch;

  if (name.starts_with(UID_CHAR)) {
    ch = find_char(g, parse_int(name.substr(1)));

    if (ch && valid_dg_target(g, ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (char_data *vict : room_people(g, room))
      if (isname(name, char_get_alias(vict)) &&
          valid_dg_target(g, vict, DG_ALLOW_GODS))
        return vict;

    for (char_data *tch : each_char(g))
      if (isname(name, char_get_alias(tch)) &&
          valid_dg_target(g, tch, DG_ALLOW_GODS))
        return tch;
  }

  return nullptr;
}

/*
 * returns the object in the world with name name, or NULL if not found
 * search based on obj
 */
obj_data *get_obj_by_obj(GameWorld *g, obj_data *obj, std::string_view name)
{
  obj_data *i = nullptr;
  room_vnum rm;

  if (name.starts_with(UID_CHAR))
    return find_obj(g, parse_int(name.substr(1)));

  if (str_eq_nocase(name, "self") || str_eq_nocase(name, "me"))
    return obj;

  if (obj_get_contains(obj) && (i = get_obj_in_list(g, name, obj_get_contains(obj))))
    return i;

  if (obj_get_in_obj(obj) && isname(name, obj_get_name(obj_get_in_obj(obj))))
    return obj_get_in_obj(obj);

  if (obj_get_worn_by(obj) && (i = get_object_in_equip(g, obj_get_worn_by(obj), name)))
    return i;

  if (obj_get_carried_by(obj) &&
      (i = get_obj_in_list(g, name, char_get_inventory(obj_get_carried_by(obj)))))
    return i;

  if (((rm = obj_room(obj)) != NOWHERE) &&
      (i = get_obj_in_list(g, name, room_get_contents(g, rm))))
    return i;

  return get_obj(g, name);
}

/* only searches the room */
obj_data *get_obj_in_room(GameWorld *g, room_data *room, std::string_view name)
{
  long id;

  if (name.starts_with(UID_CHAR)) {
    id = parse_int(name.substr(1));
    for (obj_data *obj : obj_list(g, room_get_contents(room)))
      if (id == obj_get_id(obj))
        return obj;
  } else {
    for (obj_data *obj : obj_list(g, room_get_contents(room)))
      if (isname(name, obj_get_name(obj)))
        return obj;
  }

  return nullptr;
}

/* returns obj with name - searches room, then world */
obj_data *get_obj_by_room(GameWorld *g, room_data *room, std::string_view name)
{

  if (name.starts_with(UID_CHAR))
    return find_obj(g, parse_int(name.substr(1)));

  for (obj_data *obj : obj_list(g, room_get_contents(room)))
    if (isname(name, obj_get_name(obj)))
      return obj;

  for (obj_data *obj : each_obj(g))
    if (isname(name, obj_get_name(obj)))
      return obj;

  return nullptr;
}
/* find_char() helpers */


static const char *uid_range_name(long uid)
{
  if (uid < 0)
    return "invalid";
  if (uid < PLAYER_ID_MAX)
    return "player";
  if (uid < ROOM_ID_BASE)
    return "legacy mob";
  if (uid < OBJ_ID_BASE)
    return "room";
  if (uid <= OBJ_ID_MAX)
    return "object";
  return "mob";
}

/* found_other: the uid belongs to an entity of the other kind */
static void log_lookup_miss(GameWorld *g, const char *what, long uid, bool found_other)
{
  std::string ctx = dg_context(g);
  bool is_obj     = !strcmp(what, "find_obj");

  if (found_other)
    mud_log("{}: uid {:d} ({} range) belongs to a {}, not a {}{}",
            what, uid, uid_range_name(uid), is_obj ? "character" : "object",
            is_obj ? "object" : "character", ctx);
  else
    mud_log("{}: no entity with uid {:d} ({} range; next mob uid {:d}, next obj uid {:d}){}",
            what, uid, uid_range_name(uid), g->max_mob_id, g->max_obj_id, ctx);
}

static struct char_data *find_char_by_uid_in_lookup_table(GameWorld *g, long uid)
{
  if (struct char_data *ch = char_by_id(g, uid))
    return ch;
  log_lookup_miss(g, "find_char", uid, obj_by_id(g, uid) != nullptr);
  return nullptr;
}

static struct obj_data *find_obj_by_uid_in_lookup_table(GameWorld *g, long uid)
{
  if (struct obj_data *obj = obj_by_id(g, uid))
    return obj;
  log_lookup_miss(g, "find_obj", uid, char_by_id(g, uid) != nullptr);
  return nullptr;
}

struct char_data *char_by_id(GameWorld *g, long id)
{
  return g->characters.find(id);
}

struct obj_data *obj_by_id(GameWorld *g, long id)
{
  return g->objects.find(id);
}

/* Never keep two entities under one id: the newcomer replaces the other, which can no longer be found. */
void char_register(GameWorld *g, struct char_data *ch)
{
  long id = char_get_id(ch);

  if (g->characters.add(id, ch))
    return;
  if (g->characters.find(id) == ch)
    return;
  mud_log("SYSERR: char_register: uid {:d} already maps to another character; replacing it with this character.", id);
  g->characters.remove(id, g->characters.find(id));
  g->characters.add(id, ch);
}

void obj_register(GameWorld *g, struct obj_data *obj)
{
  long id = obj_get_id(obj);

  if (g->objects.add(id, obj))
    return;
  if (g->objects.find(id) == obj)
    return;
  mud_log("SYSERR: obj_register: uid {:d} already maps to another object; replacing it with this object.", id);
  g->objects.remove(id, g->objects.find(id));
  g->objects.add(id, obj);
}

void char_unregister(GameWorld *g, struct char_data *ch)
{
  g->characters.remove(char_get_id(ch), ch);
}

void obj_unregister(GameWorld *g, struct obj_data *obj)
{
  g->objects.remove(obj_get_id(obj), obj);
}

/*
 * UID allocation. Each range is large enough to last years, but if one is
 * ever exhausted we wrap and skip UIDs that are still in use rather than
 * spilling into the next entity type's range.
 */
static long next_uid(GameWorld *g, long *counter, long base, long max, const char *what)
{
  bool *warned = (base == MOB_ID_BASE) ? &g->warned_mob_lookup : &g->warned_obj_lookup;
  long tries;

  if (!*warned && *counter - base >= (max - base) / 10 * 9) {
    mud_log("SYSERR: {} UIDs are 90% used (next {:d} of {:d}..{:d}).", what, *counter, base, max);
    *warned = true;
  }

  for (tries = 0; tries <= max - base; tries++) {
    long uid;

    if (*counter < base || *counter > max) {
      mud_log("SYSERR: {} UID range exhausted; wrapping to {:d} and reusing free UIDs.", what, base);
      *counter = base;
    }
    uid = (*counter)++;
    if (!char_by_id(g, uid) && !obj_by_id(g, uid))
      return uid;
  }

  mud_log("SYSERR: no free {} UIDs left!", what);
  return base;
}

long dg_new_mob_uid(GameWorld *g)
{
  return next_uid(g, &g->max_mob_id, MOB_ID_BASE, MOB_ID_MAX, "mob");
}

long dg_new_obj_uid(GameWorld *g)
{
  return next_uid(g, &g->max_obj_id, OBJ_ID_BASE, OBJ_ID_MAX, "object");
}

void dg_log_uid_stats(GameWorld *g)
{
  mud_log("DG UIDs: next mob {:d} ({:.4f}% used), next obj {:d} ({:.4f}% used), lookup entries {:d}",
          g->max_mob_id, 100.0 * (g->max_mob_id - MOB_ID_BASE) / ((double)MOB_ID_MAX - MOB_ID_BASE),
          g->max_obj_id, 100.0 * (g->max_obj_id - OBJ_ID_BASE) / ((double)OBJ_ID_MAX - OBJ_ID_BASE),
          (long)(g->characters.size() + g->objects.size()));
}

int check_flags_by_name_ar(bitvector_t *array, int numflags, char *search, const char *const namelist[])
{
  int i, item = -1;

  for (i = 0; i < numflags && item < 0; i++)
    if (!strcmp(search, namelist[i]))
      item = i;

  if (item < 0)
    return false;

  if (IS_SET_AR(array, item))
    return item;

  return false;
}
