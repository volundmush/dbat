#include "dbat/objects.h"
#include "dbat/object_utils.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/itemdata.h"
#include <stdlib.h>
#include <string.h>


int wearable_obj(struct obj_data *obj)
{
  int pass = FALSE;

    if (obj_can_wear(obj, ITEM_WEAR_FINGER))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_NECK))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_BODY))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_HEAD))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_LEGS))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_FEET))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_HANDS))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_ARMS))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_SHIELD))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_ABOUT))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_WAIST))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_WRIST))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_WIELD))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_EYE))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_PACK))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_SH))
     pass = TRUE;
    if (obj_can_wear(obj, ITEM_WEAR_EAR))
     pass = TRUE;
  
  if (pass == TRUE)
   return (1);
  else
   return (0);
}

void randomize_eq(struct obj_data *obj)
{
    if (wearable_obj(obj) && !obj_flagged(obj, ITEM_NORANDOM)) {
     int value = 0, slot = 0, roll = rand_number(2, 12), slot1 = 1, slot2 = 1, slot3 = 1, slot4 = 1, slot5 = 1, slot6 = 1;
     int stat = 0;
     int strength = FALSE, wisdom = FALSE, intelligence = FALSE, dexterity = FALSE, speed = FALSE, constitution = FALSE;
     // Setting the strength stats
      int i;
      for (i = 0; i < 6; i++) {
       stat = obj->affected[slot].location;
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
         strength = TRUE;
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
         dexterity = TRUE;
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
         intelligence = TRUE;
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
         wisdom = TRUE;
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
         constitution = TRUE;
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
         speed = TRUE;
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
           slot3 = 0;
           break;
          case 4:
           slot4 = 0;
           break;
          case 5:
           slot5 = 0;
           break;
          case 6:
           slot6 = 0;
           break;
         }
       }
      slot += 1;
      roll = rand_number(2, 12);
     }

     if (slot1 == 0) {
      if (strength == FALSE && rand_number(1, 6) == 1) {
       strength = TRUE;
       obj->affected[0].location = 1;
       obj->affected[0].modifier = 1;
      } else if (dexterity == FALSE && rand_number(1, 6) == 1) {
       dexterity = TRUE;
       obj->affected[0].location = 2;
       obj->affected[0].modifier = 1;
      } else if (intelligence == FALSE && rand_number(1, 6) == 1) {
       intelligence = TRUE;
       obj->affected[0].location = 3;
       obj->affected[0].modifier = 1;
      } else if (wisdom == FALSE && rand_number(1, 6) == 1) {
       wisdom = TRUE;
       obj->affected[0].location = 4;
       obj->affected[0].modifier = 1;
      } else if (constitution == FALSE && rand_number(1, 6) == 1) {
       constitution = TRUE;
       obj->affected[0].location = 5;
       obj->affected[0].modifier = 1;
      } else if (speed == FALSE && rand_number(1, 6) == 1) {
       speed = TRUE;
       obj->affected[0].location = 6;
       obj->affected[0].modifier = 1;
      }
     }
     if (slot2 == 0 && roll >= 10) {
      if (strength == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 1;
       obj->affected[1].modifier = 1;
      } else if (dexterity == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 2;
       obj->affected[1].modifier = 1;
      } else if (intelligence == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 3;
       obj->affected[1].modifier = 1;
      } else if (wisdom == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 4;
       obj->affected[1].modifier = 1;
      } else if (constitution == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 5;
       obj->affected[1].modifier = 1;
      } else if (speed == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 6;
       obj->affected[1].modifier = 1;
      }
     }
     int dice = rand_number(2, 12);
     if (dice >= 10) {
      SET_BIT_AR(obj_get_extra_flags(obj), ITEM_SLOT2);
     } else if (dice >= 7) {
      SET_BIT_AR(obj_get_extra_flags(obj), ITEM_SLOT1);
     }
    }
}








const char *money_desc(int amount)
{
  int cnt;
  struct {
    int limit;
    const char *description;
  } money_table[] = {
    {          1, "a single zenni"       		},
    {         10, "a tiny pile of zenni"		},
    {         20, "a handful of zenni"   		},
    {         75, "a little pile of zenni"		},
    {        150, "a small pile of zenni"		},
    {        250, "a pile of zenni"	        	},
    {        500, "a big pile of zenni"		        },
    {       1000, "a large heap of zenni"		},
    {       5000, "a huge mound of zenni"		},
    {      10000, "an enormous mound of zenni"	        },
    {      15000, "a small mountain of zenni"	        },
    {      20000, "a mountain of zenni"		        },
    {      25000, "a huge mountain of zenni"	        },
    {      50000, "an enormous mountain of zenni"	},
    {          0, NULL					},
  };

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (NULL);
  }

  for (cnt = 0; money_table[cnt].limit; cnt++)
    if (amount <= money_table[cnt].limit)
      return (money_table[cnt].description);

  return ("an absolutely colossal mountain of zenni");
}


struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];
  int y;

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("zenni money");
    obj->short_description = strdup("a single zenni");
    obj->description = strdup("One miserable zenni is lying here");
    new_descr->keyword = strdup("zenni money");
    new_descr->description = strdup("It's just one miserable little zenni.");
  } else {
    obj->name = strdup("zenni money");
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
	      1000 * ((amount / 1000) + rand_number(0, (amount / 1000))));
    else
      strcpy(buf, "There are is LOT of zenni.");	/* strcpy: OK (is < 200) */
    new_descr->description = strdup(buf);
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  obj_set_type_flag(obj, ITEM_MONEY);
  obj_set_value(obj, 7, MATERIAL_GOLD);
  obj_set_value(obj, VAL_ALL_MAXHEALTH, 100);
  obj_set_value(obj, VAL_ALL_HEALTH, 100); 
  for(y = 0; y < TW_ARRAY_MAX; y++)
    obj->wear_flags[y] = 0;
  SET_BIT_AR(obj_get_wear_flags(obj), ITEM_WEAR_TAKE);
  obj_set_value(obj, VAL_MONEY_SIZE, amount);
  obj_set_cost(obj, amount);
  obj->item_number = NOTHING;

  return (obj);
}

int is_better(struct obj_data *object, struct obj_data *object2)
{
  int value1=0, value2=0;

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
    return 1;
  else
    return 0;
}

bool obj_is_valid(const struct obj_data *obj) {
    return obj != NULL;
}

obj_vnum obj_get_vnum(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOTHING;
    if (obj->item_number < 0 || obj->item_number > top_of_objt) return NOTHING;
    return obj_index[obj->item_number].vnum;
}

/* Formerly macros in object_utils.h. */

bool obj_aff_flagged(const struct obj_data *obj, int flag) {
    return (IS_SET_AR(obj_get_bitvector(obj), (flag)));
}

bool obj_val_flagged(const struct obj_data *obj, bitvector_t flag) {
    return (IS_SET(obj_get_value((obj), VAL_CONTAINER_FLAGS), (flag)));
}

bool obj_flagged(const struct obj_data *obj, int flag) {
    return (IS_SET_AR(obj_get_extra_flags(obj), (flag)));
}

bool obj_valid_rnum(const struct obj_data *obj) {
    return (obj_get_item_number(obj) <= top_of_objt && obj_get_item_number(obj) != NOTHING);
}

spec_proc_func obj_get_spec(const struct obj_data *obj) {
    return (obj_valid_rnum(obj) ? obj_index[obj_get_item_number(obj)].func : NULL);
}

bool obj_is_corpse(const struct obj_data *obj) {
    return (obj_get_type_flag(obj) == ITEM_CONTAINER && obj_get_value((obj), VAL_CONTAINER_CORPSE) == 1);
}

bool obj_can_wear(const struct obj_data *obj, int part) {
    return (IS_SET_AR(obj_get_wear_flags(obj), (part)));
}

char *obj_sana(const struct obj_data *obj) {
    return (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a");
}

/* Field accessors (formerly accessor macros). */

int obj_get_level(const struct obj_data *obj) {
    return obj->level;
}

void obj_set_level(struct obj_data *obj, int v) {
    obj->level = v;
}

bitvector_t *obj_get_bitvector(const struct obj_data *obj) {
    return (bitvector_t *)obj->bitvector; /* writable flag array, as the macro allowed */
}

int8_t obj_get_type_flag(const struct obj_data *obj) {
    return obj->type_flag;
}

void obj_set_type_flag(struct obj_data *obj, int8_t v) {
    obj->type_flag = v;
}

int obj_get_cost(const struct obj_data *obj) {
    return obj->cost;
}

void obj_set_cost(struct obj_data *obj, int v) {
    obj->cost = v;
}

int obj_get_cost_per_day(const struct obj_data *obj) {
    return obj->cost_per_day;
}

void obj_set_cost_per_day(struct obj_data *obj, int v) {
    obj->cost_per_day = v;
}

bitvector_t *obj_get_extra_flags(const struct obj_data *obj) {
    return (bitvector_t *)obj->extra_flags; /* writable flag array, as the macro allowed */
}

bitvector_t *obj_get_wear_flags(const struct obj_data *obj) {
    return (bitvector_t *)obj->wear_flags; /* writable flag array, as the macro allowed */
}

int obj_get_value(const struct obj_data *obj, int i) {
    return obj->value[i];
}

void obj_set_value(struct obj_data *obj, int i, int v) {
    obj->value[i] = v;
}

int64_t obj_get_weight(const struct obj_data *obj) {
    return obj->weight;
}

void obj_set_weight(struct obj_data *obj, int64_t v) {
    obj->weight = v;
}

int obj_get_timer(const struct obj_data *obj) {
    return obj->timer;
}

void obj_set_timer(struct obj_data *obj, int v) {
    obj->timer = v;
}

struct char_data *obj_get_sitting(const struct obj_data *obj) {
    return obj->sitting;
}

void obj_set_sitting(struct obj_data *obj, struct char_data *v) {
    obj->sitting = v;
}

int obj_get_posttype(const struct obj_data *obj) {
    return obj->posttype;
}

void obj_set_posttype(struct obj_data *obj, int v) {
    obj->posttype = v;
}

struct obj_data *obj_get_posted_to(const struct obj_data *obj) {
    return obj->posted_to;
}

void obj_set_posted_to(struct obj_data *obj, struct obj_data *v) {
    obj->posted_to = v;
}

struct obj_data *obj_get_fellow_wall(const struct obj_data *obj) {
    return obj->fellow_wall;
}

void obj_set_fellow_wall(struct obj_data *obj, struct obj_data *v) {
    obj->fellow_wall = v;
}

int32_t obj_get_aucter(const struct obj_data *obj) {
    return obj->aucter;
}

void obj_set_aucter(struct obj_data *obj, int32_t v) {
    obj->aucter = v;
}

int32_t obj_get_curBidder(const struct obj_data *obj) {
    return obj->curBidder;
}

void obj_set_curBidder(struct obj_data *obj, int32_t v) {
    obj->curBidder = v;
}

char *obj_get_auctname(const struct obj_data *obj) {
    return obj->auctname;
}

void obj_set_auctname(struct obj_data *obj, char *v) {
    obj->auctname = v;
}

time_t obj_get_aucTime(const struct obj_data *obj) {
    return obj->aucTime;
}

void obj_set_aucTime(struct obj_data *obj, time_t v) {
    obj->aucTime = v;
}

int obj_get_bid(const struct obj_data *obj) {
    return obj->bid;
}

void obj_set_bid(struct obj_data *obj, int v) {
    obj->bid = v;
}

int obj_get_startbid(const struct obj_data *obj) {
    return obj->startbid;
}

void obj_set_startbid(struct obj_data *obj, int v) {
    obj->startbid = v;
}

int obj_get_foob(const struct obj_data *obj) {
    return obj->foob;
}

void obj_set_foob(struct obj_data *obj, int v) {
    obj->foob = v;
}

struct char_data *obj_get_target(const struct obj_data *obj) {
    return obj->target;
}

void obj_set_target(struct obj_data *obj, struct char_data *v) {
    obj->target = v;
}

int64_t obj_get_kicharge(const struct obj_data *obj) {
    return obj->kicharge;
}

void obj_set_kicharge(struct obj_data *obj, int64_t v) {
    obj->kicharge = v;
}

int obj_get_kitype(const struct obj_data *obj) {
    return obj->kitype;
}

void obj_set_kitype(struct obj_data *obj, int v) {
    obj->kitype = v;
}

struct char_data *obj_get_user(const struct obj_data *obj) {
    return obj->user;
}

void obj_set_user(struct obj_data *obj, struct char_data *v) {
    obj->user = v;
}

int obj_get_distance(const struct obj_data *obj) {
    return obj->distance;
}

void obj_set_distance(struct obj_data *obj, int v) {
    obj->distance = v;
}

int obj_get_scoutfreq(const struct obj_data *obj) {
    return obj->scoutfreq;
}

void obj_set_scoutfreq(struct obj_data *obj, int v) {
    obj->scoutfreq = v;
}

int obj_get_healcharge(const struct obj_data *obj) {
    return obj->healcharge;
}

void obj_set_healcharge(struct obj_data *obj, int v) {
    obj->healcharge = v;
}

time_t obj_get_lload(const struct obj_data *obj) {
    return obj->lload;
}

void obj_set_lload(struct obj_data *obj, time_t v) {
    obj->lload = v;
}

int obj_get_size(const struct obj_data *obj) {
    return obj->size;
}

void obj_set_size(struct obj_data *obj, int v) {
    obj->size = v;
}

obj_vnum obj_get_item_number(const struct obj_data *obj) {
    return obj->item_number;
}

void obj_set_item_number(struct obj_data *obj, obj_vnum v) {
    obj->item_number = v;
}

char *obj_get_short_description(const struct obj_data *obj) {
    return obj->short_description;
}

void obj_set_short_description(struct obj_data *obj, char *v) {
    obj->short_description = v;
}

room_vnum obj_get_room_loaded(const struct obj_data *obj) {
    return obj->room_loaded;
}

void obj_set_room_loaded(struct obj_data *obj, room_vnum v) {
    obj->room_loaded = v;
}

int32_t obj_get_id(const struct obj_data *obj) {
    return obj->id;
}

void obj_set_id(struct obj_data *obj, int32_t v) {
    obj->id = v;
}

room_rnum obj_get_in_room(const struct obj_data *obj) {
    return obj->in_room;
}

void obj_set_in_room(struct obj_data *obj, room_rnum v) {
    obj->in_room = v;
}

struct room_direction_data *obj_get_exit(const struct obj_data *obj, int door) {
    return world[obj_get_in_room(obj)].dir_option[door];
}
