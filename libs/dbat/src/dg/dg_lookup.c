/*
 * DG Scripts: finding characters, objects and rooms for scripts, by name or UID, and the UID lookup table.
 * Split out of dg_scripts.c.
 */
#include "dbat/utils.h"
#include "dg_internal.h"

/* Local functions not used elsewhere */
static struct char_data *find_char_by_uid_in_lookup_table(long uid);
static struct obj_data *find_obj_by_uid_in_lookup_table(long uid);

/* Return pointer to first occurrence of string ct in */
/* cs, or NULL if not present.  Case insensitive */
char *str_str(char *cs, char *ct)
{
  char *s, *t;

  if (!cs || !ct || !*ct)
    return NULL;

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

  return NULL;
}

int trgvar_in_room(room_vnum vnum)
{
  room_rnum rnum = real_room(vnum);
  int i          = 0;
  char_data *ch;

  if (rnum == NOWHERE) {
    script_log("people.vnum: world[rnum] does not exist");
    return (-1);
  }

  for (ch = world[rnum].people; ch != NULL; ch = ch->next_in_room)
    i++;

  return i;
}

obj_data *get_obj_in_list(char *name, obj_data *list)
{
  obj_data *i;
  long id;

  if (*name == UID_CHAR) {
    id = atoi(name + 1);
    for (i = list; i; i = i->next_content)
      if (id == obj_get_id(i))
        return i;
  } else {
    for (i = list; i; i = i->next_content)
      if (isname(name, i->name))
        return i;
  }

  return NULL;
}

obj_data *get_object_in_equip(char_data *ch, char *name)
{
  int j, n = 0, number;
  obj_data *obj;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;
  long id;

  if (*name == UID_CHAR) {
    id = atoi(name + 1);

    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = char_get_equipment(ch, j)))
        if (id == obj_get_id(obj))
          return (obj);
  } else if (is_number(name)) {
    obj_vnum ovnum = atoi(name);
    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = char_get_equipment(ch, j)))
        if (obj_get_vnum(obj) == ovnum)
          return (obj);
  } else {
    snprintf(tmpname, sizeof(tmpname), "%s", name);
    if (!(number = get_number(&tmp)))
      return NULL;

    for (j = 0; (j < NUM_WEARS) && (n <= number); j++)
      if ((obj = char_get_equipment(ch, j)))
        if (isname(tmp, obj->name))
          if (++n == number)
            return (obj);
  }

  return NULL;
}

/* Handles 'held', 'light' and 'wield' positions - Welcor
   After idea from Byron Ellacott - bje@apnic.net */
int find_eq_pos_script(char *arg)
{
  int i;
  struct eq_pos_list {
    const char *pos;
    int where;
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

  if (is_number(arg) && (i = atoi(arg)) >= 0 && i < NUM_WEARS)
    return i;

  for (i = 0; eq_pos[i].where != -1; i++) {
    if (!strcasecmp(eq_pos[i].pos, arg))
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
struct char_data *find_char(long n)
{
  /* Players and legacy mobs live below ROOM_ID_BASE; mobs from MOB_ID_BASE. */
  if (n < 0 || (n >= ROOM_ID_BASE && n < MOB_ID_BASE)) /* See note in dg_scripts.h */
    return NULL;

  return find_char_by_uid_in_lookup_table(n);
}

/* return object with UID n */
obj_data *find_obj(long n)
{
  if (n < OBJ_ID_BASE || n > OBJ_ID_MAX) /* see note in dg_scripts.h */
    return NULL;

  return find_obj_by_uid_in_lookup_table(n);
}

/* return room with UID n */
room_data *find_room(long n)
{
  room_rnum rnum;

  n -= ROOM_ID_BASE;
  if (n < 0)
    return NULL;
  rnum = real_room((room_vnum)n);

  if (rnum != NOWHERE)
    return &world[rnum];

  return NULL;
}

/************************************************************
 * generic searches based only on name
 ************************************************************/

/* search the entire world for a char, and return a pointer */
char_data *get_char(char *name)
{
  char_data *i;

  if (*name == UID_CHAR) {
    i = find_char(atoi(name + 1));

    if (i && valid_dg_target(i, DG_ALLOW_GODS))
      return i;
  } else {
    for (i = character_list; i; i = i->next)
      if (isname(name, i->name) &&
          valid_dg_target(i, DG_ALLOW_GODS))
        return i;
  }

  return NULL;
}

/*
 * Finds a char in the same room as the object with the name 'name'
 */
char_data *get_char_near_obj(obj_data *obj, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    room_rnum num;
    if ((num = obj_room(obj)) != NOWHERE)
      for (ch = world[num].people; ch; ch = ch->next_in_room)
        if (isname(name, ch->name) &&
            valid_dg_target(ch, DG_ALLOW_GODS))
          return ch;
  }

  return NULL;
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_in_room(room_data *room, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (ch = room->people; ch; ch = ch->next_in_room)
      if (isname(name, ch->name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }

  return NULL;
}

/* searches the room with the object for an object with name 'name'*/

obj_data *get_obj_near_obj(obj_data *obj, char *name)
{
  obj_data *i = NULL;
  char_data *ch;
  int rm;
  long id;

  if (!strcasecmp(name, "self") || !strcasecmp(name, "me"))
    return obj;

  /* is it inside ? */
  if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
    return i;

  /* or outside ? */
  if (obj->in_obj) {
    if (*name == UID_CHAR) {
      id = atoi(name + 1);

      if (id == obj_get_id(obj->in_obj))
        return obj->in_obj;
    } else if (isname(name, obj->in_obj->name))
      return obj->in_obj;
  }
  /* or worn ?*/
  else if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
    return i;
  /* or carried ? */
  else if (obj->carried_by &&
           (i = get_obj_in_list(name, obj->carried_by->carrying)))
    return i;
  else if ((rm = obj_room(obj)) != NOWHERE) {
    /* check the floor */
    if ((i = get_obj_in_list(name, world[rm].contents)))
      return i;

    /* check peoples' inventory */
    for (ch = world[rm].people; ch; ch = ch->next_in_room)
      if ((i = get_object_in_equip(ch, name)))
        return i;
  }
  return NULL;
}

/* returns the object in the world with name name, or NULL if not found */
obj_data *get_obj(char *name)
{
  obj_data *obj;

  if (*name == UID_CHAR)
    return find_obj(atoi(name + 1));
  else {
    for (obj = object_list; obj; obj = obj->next)
      if (isname(name, obj->name))
        return obj;
  }

  return NULL;
}

/* finds room by id or vnum.  returns NULL if not found */
room_data *get_room(char *name)
{
  room_rnum nr;

  if (*name == UID_CHAR)
    return find_room(atoi(name + 1));
  else if ((nr = real_room(atoi(name))) == NOWHERE)
    return NULL;
  else
    return &world[nr];
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching with the person owing the object
 */
char_data *get_char_by_obj(obj_data *obj, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    if (obj->carried_by &&
        isname(name, obj->carried_by->name) &&
        valid_dg_target(obj->carried_by, DG_ALLOW_GODS))
      return obj->carried_by;

    if (obj->worn_by &&
        isname(name, obj->worn_by->name) &&
        valid_dg_target(obj->worn_by, DG_ALLOW_GODS))
      return obj->worn_by;

    for (ch = character_list; ch; ch = ch->next)
      if (isname(name, ch->name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }

  return NULL;
}

/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_by_room(room_data *room, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (ch = room->people; ch; ch = ch->next_in_room)
      if (isname(name, ch->name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;

    for (ch = character_list; ch; ch = ch->next)
      if (isname(name, ch->name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }

  return NULL;
}

/*
 * returns the object in the world with name name, or NULL if not found
 * search based on obj
 */
obj_data *get_obj_by_obj(obj_data *obj, char *name)
{
  obj_data *i = NULL;
  int rm;

  if (*name == UID_CHAR)
    return find_obj(atoi(name + 1));

  if (!strcasecmp(name, "self") || !strcasecmp(name, "me"))
    return obj;

  if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
    return i;

  if (obj->in_obj && isname(name, obj->in_obj->name))
    return obj->in_obj;

  if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
    return i;

  if (obj->carried_by &&
      (i = get_obj_in_list(name, obj->carried_by->carrying)))
    return i;

  if (((rm = obj_room(obj)) != NOWHERE) &&
      (i = get_obj_in_list(name, world[rm].contents)))
    return i;

  return get_obj(name);
}

/* only searches the room */
obj_data *get_obj_in_room(room_data *room, char *name)
{
  obj_data *obj;
  long id;

  if (*name == UID_CHAR) {
    id = atoi(name + 1);
    for (obj = room->contents; obj; obj = obj->next_content)
      if (id == obj_get_id(obj))
        return obj;
  } else {
    for (obj = room->contents; obj; obj = obj->next_content)
      if (isname(name, obj->name))
        return obj;
  }

  return NULL;
}

/* returns obj with name - searches room, then world */
obj_data *get_obj_by_room(room_data *room, char *name)
{
  obj_data *obj;

  if (*name == UID_CHAR)
    return find_obj(atoi(name + 1));

  for (obj = room->contents; obj; obj = obj->next_content)
    if (isname(name, obj->name))
      return obj;

  for (obj = object_list; obj; obj = obj->next)
    if (isname(name, obj->name))
      return obj;

  return NULL;
}
/* find_char() helpers */

// Must be power of 2
constexpr int BUCKET_COUNT = 4096;
// to recognize an empty bucket head; real UIDs are never negative
constexpr long UID_OUT_OF_RANGE = -1;

struct lookup_table_t {
  long uid;
  int type; /* MOB_TRIGGER for characters, OBJ_TRIGGER for objects */
  void *c;
  struct lookup_table_t *next;
};
static struct lookup_table_t lookup_table[BUCKET_COUNT];
static long lookup_table_entries = 0;

void init_lookup_table(void)
{
  int i;
  for (i = 0; i < BUCKET_COUNT; i++) {
    lookup_table[i].uid  = UID_OUT_OF_RANGE;
    lookup_table[i].type = -1;
    lookup_table[i].c    = NULL;
    lookup_table[i].next = NULL;
  }
}

static struct lookup_table_t *lookup_table_find(long uid)
{
  struct lookup_table_t *lt;

  if (uid < 0)
    return NULL;

  for (lt = lookup_table[uid & (BUCKET_COUNT - 1)].next; lt; lt = lt->next)
    if (lt->uid == uid)
      return lt;

  return NULL;
}

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

static void log_lookup_miss(const char *what, long uid, struct lookup_table_t *lt)
{
  char ctx[MAX_STRING_LENGTH];

  dg_context_str(ctx, sizeof(ctx));
  if (lt)
    mud_log("%s: uid %ld (%s range) belongs to a %s, not a %s%s",
            what, uid, uid_range_name(uid), lt->type == OBJ_TRIGGER ? "object" : "character",
            strcmp(what, "find_obj") ? "character" : "object", ctx);
  else
    mud_log("%s: no entity with uid %ld (%s range; next mob uid %ld, next obj uid %ld)%s",
            what, uid, uid_range_name(uid), max_mob_id, max_obj_id, ctx);
}

static struct char_data *find_char_by_uid_in_lookup_table(long uid)
{
  struct lookup_table_t *lt = lookup_table_find(uid);

  if (lt && lt->type == MOB_TRIGGER)
    return (struct char_data *)(lt->c);

  log_lookup_miss("find_char", uid, lt);
  return NULL;
}

static struct obj_data *find_obj_by_uid_in_lookup_table(long uid)
{
  struct lookup_table_t *lt = lookup_table_find(uid);

  if (lt && lt->type == OBJ_TRIGGER)
    return (struct obj_data *)(lt->c);

  log_lookup_miss("find_obj", uid, lt);
  return NULL;
}

void add_to_lookup_table(long uid, int type, void *c)
{
  struct lookup_table_t *lt = lookup_table_find(uid), *head;

  if (lt) {
    if (lt->c == c && lt->type == type)
      return;
    /* Never keep two entries for one UID: find and remove would disagree. */
    mud_log("SYSERR: add_to_lookup_table: uid %ld already maps to another %s; replacing it with this %s.",
            uid, lt->type == OBJ_TRIGGER ? "object" : "character",
            type == OBJ_TRIGGER ? "object" : "character");
    lt->type = type;
    lt->c    = c;
    return;
  }

  head = &lookup_table[uid & (BUCKET_COUNT - 1)];
  CREATE(lt, struct lookup_table_t, 1);
  lt->uid    = uid;
  lt->type   = type;
  lt->c      = c;
  lt->next   = head->next;
  head->next = lt;
  lookup_table_entries++;
}

void remove_from_lookup_table(long uid)
{
  struct lookup_table_t *lt, *prev;

  /*
   * This is not supposed to happen. UID 0 is not used.
   * However, while I'm debugging the issue, let's just return right away.
   *
   * Welcor 02/04
   */
  if (uid <= 0)
    return;

  prev = &lookup_table[uid & (BUCKET_COUNT - 1)];
  for (lt = prev->next; lt; prev = lt, lt = lt->next)
    if (lt->uid == uid) {
      prev->next = lt->next;
      free(lt);
      lookup_table_entries--;
      return;
    }

  mud_log("remove_from_lookup. UID %ld not found.", uid);
}

/*
 * UID allocation. Each range is large enough to last years, but if one is
 * ever exhausted we wrap and skip UIDs that are still in use rather than
 * spilling into the next entity type's range.
 */
static long next_uid(long *counter, long base, long max, const char *what)
{
  static bool warned_mob = FALSE, warned_obj = FALSE;
  bool *warned = (base == MOB_ID_BASE) ? &warned_mob : &warned_obj;
  long tries;

  if (!*warned && *counter - base >= (max - base) / 10 * 9) {
    mud_log("SYSERR: %s UIDs are 90%% used (next %ld of %ld..%ld).", what, *counter, base, max);
    *warned = TRUE;
  }

  for (tries = 0; tries <= max - base; tries++) {
    long uid;

    if (*counter < base || *counter > max) {
      mud_log("SYSERR: %s UID range exhausted; wrapping to %ld and reusing free UIDs.", what, base);
      *counter = base;
    }
    uid = (*counter)++;
    if (!lookup_table_find(uid))
      return uid;
  }

  mud_log("SYSERR: no free %s UIDs left!", what);
  return base;
}

long dg_new_mob_uid(void)
{
  return next_uid(&max_mob_id, MOB_ID_BASE, MOB_ID_MAX, "mob");
}

long dg_new_obj_uid(void)
{
  return next_uid(&max_obj_id, OBJ_ID_BASE, OBJ_ID_MAX, "object");
}

void dg_log_uid_stats(void)
{
  mud_log("DG UIDs: next mob %ld (%.4f%% used), next obj %ld (%.4f%% used), lookup entries %ld",
          max_mob_id, 100.0 * (max_mob_id - MOB_ID_BASE) / ((double)MOB_ID_MAX - MOB_ID_BASE),
          max_obj_id, 100.0 * (max_obj_id - OBJ_ID_BASE) / ((double)OBJ_ID_MAX - OBJ_ID_BASE),
          lookup_table_entries);
}

int check_flags_by_name_ar(bitvector_t *array, int numflags, char *search, const char *namelist[])
{
  int i, item = -1;

  for (i = 0; i < numflags && item < 0; i++)
    if (!strcmp(search, namelist[i]))
      item = i;

  if (item < 0)
    return FALSE;

  if (IS_SET_AR(array, item))
    return item;

  return FALSE;
}
