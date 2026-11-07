#include <cstdlib>
#include <cstring>

#include "dbat/objects.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/game_world.hpp"

bool wearable_obj(struct obj_data *obj)
{
  int pass = false;

  if (obj_wear_flagged(obj, ITEM_WEAR_FINGER))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_NECK))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_BODY))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_HEAD))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_LEGS))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_FEET))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_HANDS))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_ARMS))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_SHIELD))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_ABOUT))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_WAIST))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_WRIST))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_WIELD))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_EYE))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_PACK))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_SH))
    pass = true;
  if (obj_wear_flagged(obj, ITEM_WEAR_EAR))
    pass = true;

  if (pass == true)
    return true;
  else
    return false;
}

void randomize_eq(GameWorld *g, struct obj_data *obj)
{
  if (wearable_obj(obj) && !obj_extra_flagged(obj, ITEM_NORANDOM)) {
    int value = 0, slot = 0, roll = rand_number(g, 2, 12), slot1 = 1, slot2 = 1;
    int stat     = 0;
    int strength = false, wisdom = false, intelligence = false, dexterity = false, speed = false, constitution = false;
    // Setting the strength stats
    int i;
    for (i = 0; i < 6; i++) {
      stat  = obj->affected[slot].location;
      value = obj->affected[slot].modifier;
      if (stat == 1) { /* Strength */
        if (roll == 12) {
          value += 3;
        } else if (roll >= 9) {
          value += 2;
        } else if (roll >= 6) {
          value += 1;
        } else if (roll == 3) {
          value -= 1;
        } else if (roll <= 2) {
          value -= 2;
        }
        if (obj->level >= 80) {
          if (value <= 0) {
            value = 1;
          }
        } else if (obj->level >= 60) {
          if (value < 0) {
            value = 0;
          }
        }
        if (value == 0) {
          obj->affected[slot].location = 0;
          obj->affected[slot].modifier = 0;
        } else {
          obj->affected[slot].modifier = value;
          strength                     = true;
        }
      } else if (stat == 2) { /* Agility */
        if (roll == 12) {
          value += 3;
        } else if (roll >= 9) {
          value += 2;
        } else if (roll >= 6) {
          value += 1;
        } else if (roll == 3) {
          value -= 1;
        } else if (roll <= 2) {
          value -= 2;
        }
        if (obj->level >= 80) {
          if (value <= 0) {
            value = 1;
          }
        } else if (obj->level >= 60) {
          if (value < 0) {
            value = 0;
          }
        }
        if (value == 0) {
          obj->affected[slot].location = 0;
          obj->affected[slot].modifier = 0;
        } else {
          obj->affected[slot].modifier = value;
          dexterity                    = true;
        }
      } else if (stat == 3) { /* Intelligence */
        if (roll == 12) {
          value += 3;
        } else if (roll >= 9) {
          value += 2;
        } else if (roll >= 6) {
          value += 1;
        } else if (roll == 3) {
          value -= 1;
        } else if (roll <= 2) {
          value -= 2;
        }
        if (obj->level >= 80) {
          if (value <= 0) {
            value = 1;
          }
        } else if (obj->level >= 60) {
          if (value < 0) {
            value = 0;
          }
        }
        if (value == 0) {
          obj->affected[slot].location = 0;
          obj->affected[slot].modifier = 0;
        } else {
          obj->affected[slot].modifier = value;
          intelligence                 = true;
        }
      } else if (stat == 4) { /* Wisdom */
        if (roll == 12) {
          value += 3;
        } else if (roll >= 9) {
          value += 2;
        } else if (roll >= 6) {
          value += 1;
        } else if (roll == 3) {
          value -= 1;
        } else if (roll <= 2) {
          value -= 2;
        }
        if (obj->level >= 80) {
          if (value <= 0) {
            value = 1;
          }
        } else if (obj->level >= 60) {
          if (value < 0) {
            value = 0;
          }
        }
        if (value == 0) {
          obj->affected[slot].location = 0;
          obj->affected[slot].modifier = 0;
        } else {
          obj->affected[slot].modifier = value;
          wisdom                       = true;
        }
      } else if (stat == 5) { /* Constitution */
        if (roll == 12) {
          value += 3;
        } else if (roll >= 9) {
          value += 2;
        } else if (roll >= 6) {
          value += 1;
        } else if (roll == 3) {
          value -= 1;
        } else if (roll <= 2) {
          value -= 2;
        }
        if (obj->level >= 80) {
          if (value <= 0) {
            value = 1;
          }
        } else if (obj->level >= 60) {
          if (value < 0) {
            value = 0;
          }
        }
        if (value == 0) {
          obj->affected[slot].location = 0;
          obj->affected[slot].modifier = 0;
        } else {
          obj->affected[slot].modifier = value;
          constitution                 = true;
        }
      } else if (stat == 6) { /* Speed */
        if (roll == 12) {
          value += 3;
        } else if (roll >= 9) {
          value += 2;
        } else if (roll >= 6) {
          value += 1;
        } else if (roll == 3) {
          value -= 1;
        } else if (roll <= 2) {
          value -= 2;
        }
        if (obj->level >= 80) {
          if (value <= 0) {
            value = 1;
          }
        } else if (obj->level >= 60) {
          if (value < 0) {
            value = 0;
          }
        }
        if (value == 0) {
          obj->affected[slot].location = 0;
          obj->affected[slot].modifier = 0;
        } else {
          obj->affected[slot].modifier = value;
          speed                        = true;
        }
      } else if (stat == 0) {
        switch (slot) {
          case 1:
            slot1 = 0;
            break;
          case 2:
            slot2 = 0;
            break;
          case 3:
            break;
          case 4:
            break;
          case 5:
            break;
          case 6:
            break;
        }
      }
      slot += 1;
      roll = rand_number(g, 2, 12);
    }

    if (slot1 == 0) {
      if (strength == false && rand_number(g, 1, 6) == 1) {
        strength                  = true;
        obj->affected[0].location = 1;
        obj->affected[0].modifier = 1;
      } else if (dexterity == false && rand_number(g, 1, 6) == 1) {
        dexterity                 = true;
        obj->affected[0].location = 2;
        obj->affected[0].modifier = 1;
      } else if (intelligence == false && rand_number(g, 1, 6) == 1) {
        intelligence              = true;
        obj->affected[0].location = 3;
        obj->affected[0].modifier = 1;
      } else if (wisdom == false && rand_number(g, 1, 6) == 1) {
        wisdom                    = true;
        obj->affected[0].location = 4;
        obj->affected[0].modifier = 1;
      } else if (constitution == false && rand_number(g, 1, 6) == 1) {
        constitution              = true;
        obj->affected[0].location = 5;
        obj->affected[0].modifier = 1;
      } else if (speed == false && rand_number(g, 1, 6) == 1) {
        speed                     = true;
        obj->affected[0].location = 6;
        obj->affected[0].modifier = 1;
      }
    }
    if (slot2 == 0 && roll >= 10) {
      if (strength == false && rand_number(g, 1, 6) == 1) {
        obj->affected[1].location = 1;
        obj->affected[1].modifier = 1;
      } else if (dexterity == false && rand_number(g, 1, 6) == 1) {
        obj->affected[1].location = 2;
        obj->affected[1].modifier = 1;
      } else if (intelligence == false && rand_number(g, 1, 6) == 1) {
        obj->affected[1].location = 3;
        obj->affected[1].modifier = 1;
      } else if (wisdom == false && rand_number(g, 1, 6) == 1) {
        obj->affected[1].location = 4;
        obj->affected[1].modifier = 1;
      } else if (constitution == false && rand_number(g, 1, 6) == 1) {
        obj->affected[1].location = 5;
        obj->affected[1].modifier = 1;
      } else if (speed == false && rand_number(g, 1, 6) == 1) {
        obj->affected[1].location = 6;
        obj->affected[1].modifier = 1;
      }
    }
    int dice = rand_number(g, 2, 12);
    if (dice >= 10) {
      obj_extra_set(obj, ITEM_SLOT2, true);
    } else if (dice >= 7) {
      obj_extra_set(obj, ITEM_SLOT1, true);
    }
  }
}

const char *money_desc(int amount)
{
  int cnt;
  struct {
    int limit{};
    const char *description{};
  } money_table[] = {
      {1,     "a single zenni"               },
      {10,    "a tiny pile of zenni"         },
      {20,    "a handful of zenni"           },
      {75,    "a little pile of zenni"       },
      {150,   "a small pile of zenni"        },
      {250,   "a pile of zenni"              },
      {500,   "a big pile of zenni"          },
      {1000,  "a large heap of zenni"        },
      {5000,  "a huge mound of zenni"        },
      {10000, "an enormous mound of zenni"   },
      {15000, "a small mountain of zenni"    },
      {20000, "a mountain of zenni"          },
      {25000, "a huge mountain of zenni"     },
      {50000, "an enormous mountain of zenni"},
      {0,     nullptr                        },
  };

  if (amount <= 0) {
    mud_log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (nullptr);
  }

  for (cnt = 0; money_table[cnt].limit; cnt++)
    if (amount <= money_table[cnt].limit)
      return (money_table[cnt].description);

  return ("an absolutely colossal mountain of zenni");
}

struct obj_data *create_money(GameWorld *g, int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];
  int y;

  if (amount <= 0) {
    mud_log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (nullptr);
  }
  obj = create_obj(g);
  new_descr = new extra_descr_data();

  if (amount == 1) {
    obj->name              = strdup("zenni money");
    obj->short_description = strdup("a single zenni");
    obj->description       = strdup("One miserable zenni is lying here");
    new_descr->keyword     = strdup("zenni money");
    new_descr->description = strdup("It's just one miserable little zenni.");
  } else {
    obj->name              = strdup("zenni money");
    obj->short_description = strdup(money_desc(amount));
    snprintf(buf, sizeof(buf), "%s is lying here", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("zenni money");
    if (amount < 10)
      snprintf(buf, sizeof(buf), "There is %d zenni.", amount);
    else if (amount < 100)
      snprintf(buf, sizeof(buf), "There is about %d zenni.", 10 * (amount / 10));
    else if (amount < 1000)
      snprintf(buf, sizeof(buf), "It looks to be about %d zenni.", 100 * (amount / 100));
    else if (amount < 100000)
      snprintf(buf, sizeof(buf), "You guess there is, maybe, %d zenni.",
               1000 * ((amount / 1000) + rand_number(g, 0, (amount / 1000))));
    else
      strcpy(buf, "There are is LOT of zenni."); /* strcpy: OK (is < 200) */
    new_descr->description = strdup(buf);
  }

  new_descr->next     = nullptr;
  obj->ex_description = new_descr;

  obj_set_type_flag(obj, ITEM_MONEY);
  obj_set_value(obj, 7, MATERIAL_GOLD);
  obj_set_value(obj, VAL_ALL_MAXHEALTH, 100);
  obj_set_value(obj, VAL_ALL_HEALTH, 100);
  for (y = 0; y < TW_ARRAY_MAX; y++)
    obj->wear_flags[y] = 0;
  obj_wear_set(obj, ITEM_WEAR_TAKE, true);
  obj_set_value(obj, VAL_MONEY_SIZE, amount);
  obj_set_cost(obj, amount);
  obj->item_number = NOTHING;

  return (obj);
}

bool is_better(struct obj_data *object, struct obj_data *object2)
{
  int value1 = 0, value2 = 0;

  switch (obj_get_type_flag(object)) {
    case ITEM_ARMOR:
      value1 = obj_get_value(object, VAL_ARMOR_APPLYAC);
      value2 = obj_get_value(object2, VAL_ARMOR_APPLYAC);
      break;
    case ITEM_WEAPON:
      value1 = (1 + obj_get_value(object, VAL_WEAPON_DAMSIZE)) * obj_get_value(object, VAL_WEAPON_DAMDICE);
      value2 = (1 + obj_get_value(object2, VAL_WEAPON_DAMSIZE)) * obj_get_value(object2, VAL_WEAPON_DAMDICE);
      break;
    default:
      break;
  }

  if (value1 > value2)
    return true;
  else
    return false;
}

static bool obj_is_valid(const struct obj_data *obj)
{
  return obj != nullptr;
}

obj_vnum obj_get_vnum(GameWorld *g, const struct obj_data *obj)
{
  if (!obj_is_valid(obj)) return NOTHING;
  return obj->item_number; /* the prototype's vnum: a prototype goes only after every copy of it */
}

/* Formerly macros in object_utils.hpp. */

bool obj_aff_flagged(const struct obj_data *obj, int flag)
{
  return (IS_SET_AR(obj_get_bitvector(obj), (flag)));
}

bool obj_val_flagged(const struct obj_data *obj, bitvector_t flag)
{
  return (IS_SET(obj_get_value((obj), VAL_CONTAINER_FLAGS), (flag)));
}

bool obj_extra_flagged(const struct obj_data *obj, int flag)
{
  return (IS_SET_AR(obj_get_extra_flags(obj), (flag)));
}

static bool obj_valid_rnum(GameWorld *g, const struct obj_data *obj)
{
  return obj_get_item_number(obj) != NOTHING && obj_proto_of(g, obj_get_item_number(obj));
}

spec_proc_func obj_get_spec(GameWorld *g, const struct obj_data *obj)
{
  return (obj_valid_rnum(g, obj) ? obj_index_of(g, obj_get_item_number(obj))->func : nullptr);
}

bool obj_is_corpse(const struct obj_data *obj)
{
  return (obj_get_type_flag(obj) == ITEM_CONTAINER && obj_get_value((obj), VAL_CONTAINER_CORPSE) == 1);
}

bool obj_wear_flagged(const struct obj_data *obj, int part)
{
  return (IS_SET_AR(obj_get_wear_flags(obj), (part)));
}

const char *obj_sana(const struct obj_data *obj)
{
  return (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a");
}

/* Field accessors (formerly accessor macros). */

int obj_get_level(const struct obj_data *obj)
{
  return obj->level;
}

void obj_set_level(struct obj_data *obj, int v)
{
  obj->level = v;
}

bitvector_t *obj_get_bitvector(const struct obj_data *obj)
{
  return const_cast<bitvector_t *>(obj->bitvector); /* writable flag array, as the macro allowed */
}

int8_t obj_get_type_flag(const struct obj_data *obj)
{
  return obj->type_flag;
}

void obj_set_type_flag(struct obj_data *obj, int8_t v)
{
  obj->type_flag = v;
}

int obj_get_cost(const struct obj_data *obj)
{
  return obj->cost;
}

void obj_set_cost(struct obj_data *obj, int v)
{
  obj->cost = v;
}

int obj_get_cost_per_day(const struct obj_data *obj)
{
  return obj->cost_per_day;
}

void obj_set_cost_per_day(struct obj_data *obj, int v)
{
  obj->cost_per_day = v;
}

bitvector_t *obj_get_extra_flags(const struct obj_data *obj)
{
  return const_cast<bitvector_t *>(obj->extra_flags); /* writable flag array, as the macro allowed */
}

bitvector_t *obj_get_wear_flags(const struct obj_data *obj)
{
  return const_cast<bitvector_t *>(obj->wear_flags); /* writable flag array, as the macro allowed */
}

int obj_get_value(const struct obj_data *obj, int i)
{
  return obj->value[i];
}

void obj_set_value(struct obj_data *obj, int i, int v)
{
  obj->value[i] = v;
}

int64_t obj_get_weight(const struct obj_data *obj)
{
  return obj->weight;
}

void obj_set_weight(struct obj_data *obj, int64_t v)
{
  obj->weight = v;
}

int obj_get_timer(const struct obj_data *obj)
{
  return obj->timer;
}

void obj_set_timer(struct obj_data *obj, int v)
{
  obj->timer = v;
}

struct char_data *obj_get_sitting(GameWorld *g, const struct obj_data *obj)
{
  return obj->sitting_uid ? char_by_id(g, obj->sitting_uid) : nullptr;
}

void obj_set_sitting(struct obj_data *obj, struct char_data *v)
{
  obj->sitting_uid = v ? char_get_id(v) : 0;
}

int obj_get_posttype(const struct obj_data *obj)
{
  return obj->posttype;
}

void obj_set_posttype(struct obj_data *obj, int v)
{
  obj->posttype = v;
}

struct obj_data *obj_get_posted_to(GameWorld *g, const struct obj_data *obj)
{
  return obj->posted_to_uid ? obj_by_id(g, obj->posted_to_uid) : nullptr;
}

void obj_set_posted_to(struct obj_data *obj, struct obj_data *v)
{
  obj->posted_to_uid = v ? obj_get_id(v) : 0;
}

struct obj_data *obj_get_fellow_wall(GameWorld *g, const struct obj_data *obj)
{
  return obj->fellow_wall_uid ? obj_by_id(g, obj->fellow_wall_uid) : nullptr;
}

void obj_set_fellow_wall(struct obj_data *obj, struct obj_data *v)
{
  obj->fellow_wall_uid = v ? obj_get_id(v) : 0;
}

int32_t obj_get_aucter(const struct obj_data *obj)
{
  return obj->aucter;
}

void obj_set_aucter(struct obj_data *obj, int32_t v)
{
  obj->aucter = v;
}

int32_t obj_get_curBidder(const struct obj_data *obj)
{
  return obj->curBidder;
}

void obj_set_curBidder(struct obj_data *obj, int32_t v)
{
  obj->curBidder = v;
}

char *obj_get_auctname(const struct obj_data *obj)
{
  return obj->auctname;
}

void obj_set_auctname(struct obj_data *obj, char *v)
{
  obj->auctname = v;
}

time_t obj_get_aucTime(const struct obj_data *obj)
{
  return obj->aucTime;
}

void obj_set_aucTime(struct obj_data *obj, time_t v)
{
  obj->aucTime = v;
}

int obj_get_bid(const struct obj_data *obj)
{
  return obj->bid;
}

void obj_set_bid(struct obj_data *obj, int v)
{
  obj->bid = v;
}

int obj_get_startbid(const struct obj_data *obj)
{
  return obj->startbid;
}

void obj_set_startbid(struct obj_data *obj, int v)
{
  obj->startbid = v;
}

int obj_get_foob(const struct obj_data *obj)
{
  return obj->foob;
}

void obj_set_foob(struct obj_data *obj, int v)
{
  obj->foob = v;
}

struct char_data *obj_get_target(GameWorld *g, const struct obj_data *obj)
{
  return obj->target_uid ? char_by_id(g, obj->target_uid) : nullptr;
}

void obj_set_target(struct obj_data *obj, struct char_data *v)
{
  obj->target_uid = v ? char_get_id(v) : 0;
}

int64_t obj_get_kicharge(const struct obj_data *obj)
{
  return obj->kicharge;
}

void obj_set_kicharge(struct obj_data *obj, int64_t v)
{
  obj->kicharge = v;
}

int obj_get_kitype(const struct obj_data *obj)
{
  return obj->kitype;
}

void obj_set_kitype(struct obj_data *obj, int v)
{
  obj->kitype = v;
}

struct char_data *obj_get_user(GameWorld *g, const struct obj_data *obj)
{
  return obj->user_uid ? char_by_id(g, obj->user_uid) : nullptr;
}

void obj_set_user(struct obj_data *obj, struct char_data *v)
{
  obj->user_uid = v ? char_get_id(v) : 0;
}

int obj_get_distance(const struct obj_data *obj)
{
  return obj->distance;
}

void obj_set_distance(struct obj_data *obj, int v)
{
  obj->distance = v;
}

int obj_get_scoutfreq(const struct obj_data *obj)
{
  return obj->scoutfreq;
}

void obj_set_scoutfreq(struct obj_data *obj, int v)
{
  obj->scoutfreq = v;
}

int obj_get_healcharge(const struct obj_data *obj)
{
  return obj->healcharge;
}

void obj_set_healcharge(struct obj_data *obj, int v)
{
  obj->healcharge = v;
}

time_t obj_get_lload(const struct obj_data *obj)
{
  return obj->lload;
}

void obj_set_lload(struct obj_data *obj, time_t v)
{
  obj->lload = v;
}

int obj_get_size(const struct obj_data *obj)
{
  return obj->size;
}

void obj_set_size(struct obj_data *obj, int v)
{
  obj->size = v;
}

obj_vnum obj_get_item_number(const struct obj_data *obj)
{
  return obj->item_number;
}

void obj_set_item_number(struct obj_data *obj, obj_vnum v)
{
  obj->item_number = v;
}

char *obj_get_short_description(const struct obj_data *obj)
{
  return obj->short_description;
}

room_vnum obj_get_room_loaded(const struct obj_data *obj)
{
  return obj->room_loaded;
}

void obj_set_room_loaded(struct obj_data *obj, room_vnum v)
{
  obj->room_loaded = v;
}

int32_t obj_get_id(const struct obj_data *obj)
{
  return obj->id;
}

void obj_set_id(struct obj_data *obj, int32_t v)
{
  obj->id = v;
}

room_vnum obj_get_in_room(const struct obj_data *obj)
{
  return obj->in_room;
}

void obj_set_in_room(struct obj_data *obj, room_vnum v)
{
  obj->in_room = v;
}

struct room_direction_data *obj_get_exit(GameWorld *g, const struct obj_data *obj, int door)
{
  return room_get_dir_option(g, obj_get_in_room(obj), door);
}

/* Flag setters and toggles. */

static void flag_array_set(bitvector_t *array, int flag, bool on)
{
  if (on)
    SET_BIT_AR(array, flag);
  else
    REMOVE_BIT_AR(array, flag);
}

static bool flag_array_toggle(bitvector_t *array, int flag)
{
  return (TOGGLE_BIT_AR(array, flag) & Q_BIT(flag)) != 0;
}

void obj_extra_set(struct obj_data *obj, int flag, bool on)
{
  flag_array_set(obj_get_extra_flags(obj), flag, on);
}

bool obj_extra_toggle(struct obj_data *obj, int flag)
{
  return flag_array_toggle(obj_get_extra_flags(obj), flag);
}

void obj_aff_set(struct obj_data *obj, int flag, bool on)
{
  flag_array_set(obj_get_bitvector(obj), flag, on);
}

bool obj_aff_toggle(struct obj_data *obj, int flag)
{
  return flag_array_toggle(obj_get_bitvector(obj), flag);
}

void obj_wear_set(struct obj_data *obj, int flag, bool on)
{
  flag_array_set(obj_get_wear_flags(obj), flag, on);
}

bool obj_wear_toggle(struct obj_data *obj, int flag)
{
  return flag_array_toggle(obj_get_wear_flags(obj), flag);
}

struct obj_data **obj_contains_slot(struct obj_data *obj)
{
  return &obj->contains;
}

struct extra_descr_data **obj_ex_description_slot(struct obj_data *obj)
{
  return &obj->ex_description;
}

struct script_data **obj_script_slot(struct obj_data *obj)
{
  return &obj->script;
}

char **obj_action_description_slot(struct obj_data *obj)
{
  return &obj->action_description;
}

struct obj_spellbook_spell **obj_sbinfo_slot(struct obj_data *obj)
{
  return &obj->sbinfo;
}

void obj_unlink_from_list(GameWorld *g, struct obj_data *obj)
{
  struct obj_data *temp;

  REMOVE_FROM_LIST(obj, g->objects, next, temp);
}

void obj_unlink_content(struct obj_data **head, struct obj_data *obj)
{
  struct obj_data *temp;

  REMOVE_FROM_LIST(obj, *head, next_content, temp);
}

char *obj_get_action_description(const struct obj_data *obj)
{
  return obj->action_description;
}

void obj_set_action_description(struct obj_data *obj, char *v)
{
  obj->action_description = v;
}

struct obj_affected_type *obj_get_affected(struct obj_data *obj)
{
  return obj->affected;
}

struct char_data *obj_get_carried_by(const struct obj_data *obj)
{
  return obj->carried_by;
}

void obj_set_carried_by(struct obj_data *obj, struct char_data *v)
{
  obj->carried_by = v;
}

struct obj_data *obj_get_contains(const struct obj_data *obj)
{
  return obj->contains;
}

void obj_set_contains(struct obj_data *obj, struct obj_data *v)
{
  obj->contains = v;
}

char *obj_get_description(const struct obj_data *obj)
{
  return obj->description;
}

void obj_set_description(struct obj_data *obj, char *v)
{
  obj->description = v;
}

struct extra_descr_data *obj_get_ex_description(const struct obj_data *obj)
{
  return obj->ex_description;
}

void obj_set_ex_description(struct obj_data *obj, struct extra_descr_data *v)
{
  obj->ex_description = v;
}

time_t obj_get_generation(const struct obj_data *obj)
{
  return obj->generation;
}

void obj_set_generation(struct obj_data *obj, time_t v)
{
  obj->generation = v;
}

struct obj_data *obj_get_in_obj(const struct obj_data *obj)
{
  return obj->in_obj;
}

void obj_set_in_obj(struct obj_data *obj, struct obj_data *v)
{
  obj->in_obj = v;
}

char *obj_get_name(const struct obj_data *obj)
{
  return obj->name;
}

void obj_set_name(struct obj_data *obj, char *v)
{
  obj->name = v;
}

struct obj_data *obj_get_next(const struct obj_data *obj)
{
  return obj->next;
}

void obj_set_next(struct obj_data *obj, struct obj_data *v)
{
  obj->next = v;
}

struct obj_data *obj_get_next_content(const struct obj_data *obj)
{
  return obj->next_content;
}

void obj_set_next_content(struct obj_data *obj, struct obj_data *v)
{
  obj->next_content = v;
}

struct trig_proto_list *obj_get_proto_script(const struct obj_data *obj)
{
  return obj->proto_script;
}

void obj_set_proto_script(struct obj_data *obj, struct trig_proto_list *v)
{
  obj->proto_script = v;
}

struct obj_spellbook_spell *obj_get_sbinfo(const struct obj_data *obj)
{
  return obj->sbinfo;
}

struct script_data *obj_get_script(const struct obj_data *obj)
{
  return obj->script;
}

void obj_set_script(struct obj_data *obj, struct script_data *v)
{
  obj->script = v;
}

void obj_set_short_description(struct obj_data *obj, char *v)
{
  obj->short_description = v;
}

int64_t obj_get_unique_id(const struct obj_data *obj)
{
  return obj->unique_id;
}

void obj_set_unique_id(struct obj_data *obj, int64_t v)
{
  obj->unique_id = v;
}

struct char_data *obj_get_worn_by(const struct obj_data *obj)
{
  return obj->worn_by;
}

void obj_set_worn_by(struct obj_data *obj, struct char_data *v)
{
  obj->worn_by = v;
}

int16_t obj_get_worn_on(const struct obj_data *obj)
{
  return obj->worn_on;
}

void obj_set_worn_on(struct obj_data *obj, int16_t v)
{
  obj->worn_on = v;
}

struct obj_data *obj_proto_of(GameWorld *g, obj_vnum vnum)
{
  return g->obj_proto.find(vnum);
}

struct index_data *obj_index_of(GameWorld *g, obj_vnum vnum)
{
  return g->obj_index.find(vnum);
}
