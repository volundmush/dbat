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

    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_EYE))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_PACK))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_SH))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))
     pass = TRUE;
  
  if (pass == TRUE)
   return (1);
  else
   return (0);
}

void randomize_eq(struct obj_data *obj)
{
    if (wearable_obj(obj) && !OBJ_FLAGGED(obj, ITEM_NORANDOM)) {
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
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOT2);
     } else if (dice >= 7) {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOT1);
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

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  GET_OBJ_MATERIAL(obj) = MATERIAL_GOLD;
  GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) = 100;
  GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = 100; 
  for(y = 0; y < TW_ARRAY_MAX; y++)
    obj->wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_VAL(obj, VAL_MONEY_SIZE) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return (obj);
}

int is_better(struct obj_data *object, struct obj_data *object2)
{
  int value1=0, value2=0;

  switch (GET_OBJ_TYPE(object)) {
    case ITEM_ARMOR:
      value1 = GET_OBJ_VAL(object, VAL_ARMOR_APPLYAC);
      value2 = GET_OBJ_VAL(object2, VAL_ARMOR_APPLYAC);
    break;
    case ITEM_WEAPON:
      value1 = (1 + GET_OBJ_VAL(object, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(object, VAL_WEAPON_DAMDICE);
      value2 = (1 + GET_OBJ_VAL(object2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(object2, VAL_WEAPON_DAMDICE);
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

obj_rnum obj_get_rnum(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOTHING;
    return obj->item_number;
}

obj_vnum obj_get_vnum(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOTHING;
    if (obj->item_number < 0 || obj->item_number > top_of_objt) return NOTHING;
    return obj_index[obj->item_number].vnum;
}

const char *obj_get_name(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->name ? obj->name : "";
}

const char *obj_get_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->description ? obj->description : "";
}

const char *obj_get_short_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->short_description ? obj->short_description : "";
}

const char *obj_get_action_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->action_description ? obj->action_description : "";
}

void obj_set_name(struct obj_data *obj, const char *name) {
    if (!obj_is_valid(obj) || !name) return;
    if (obj->name) free(obj->name);
    obj->name = strdup(name);
}

void obj_set_description(struct obj_data *obj, const char *desc) {
    if (!obj_is_valid(obj) || !desc) return;
    if (obj->description) free(obj->description);
    obj->description = strdup(desc);
}

void obj_set_short_description(struct obj_data *obj, const char *short_desc) {
    if (!obj_is_valid(obj) || !short_desc) return;
    if (obj->short_description) free(obj->short_description);
    obj->short_description = strdup(short_desc);
}

void obj_set_action_description(struct obj_data *obj, const char *action_desc) {
    if (!obj_is_valid(obj)) return;
    if (obj->action_description) free(obj->action_description);
    obj->action_description = action_desc ? strdup(action_desc) : NULL;
}

int obj_get_type(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->type_flag;
}

void obj_set_type(struct obj_data *obj, int8_t type) {
    if (!obj_is_valid(obj)) return;
    obj->type_flag = type;
}

int obj_get_level(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->level;
}

void obj_set_level(struct obj_data *obj, int level) {
    if (!obj_is_valid(obj)) return;
    obj->level = level;
}

int obj_get_size(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->size;
}

void obj_set_size(struct obj_data *obj, int size) {
    if (!obj_is_valid(obj)) return;
    obj->size = size;
}

room_rnum obj_get_in_room(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOWHERE;
    return obj->in_room;
}

void obj_set_in_room(struct obj_data *obj, room_rnum room) {
    if (!obj_is_valid(obj)) return;
    obj->in_room = room;
}

room_vnum obj_get_room_loaded(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOWHERE;
    return obj->room_loaded;
}

void obj_set_room_loaded(struct obj_data *obj, room_vnum vnum) {
    if (!obj_is_valid(obj)) return;
    obj->room_loaded = vnum;
}

struct char_data *obj_get_carried_by(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->carried_by;
}

void obj_set_carried_by(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->carried_by = ch;
}

struct char_data *obj_get_worn_by(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->worn_by;
}

void obj_set_worn_by(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->worn_by = ch;
}

int obj_get_worn_on(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->worn_on;
}

void obj_set_worn_on(struct obj_data *obj, int16_t pos) {
    if (!obj_is_valid(obj)) return;
    obj->worn_on = pos;
}

struct obj_data *obj_get_in_obj(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->in_obj;
}

void obj_set_in_obj(struct obj_data *obj, struct obj_data *container) {
    if (!obj_is_valid(obj)) return;
    obj->in_obj = container;
}

struct obj_data *obj_get_contains(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->contains;
}

void obj_set_contains(struct obj_data *obj, struct obj_data *contents) {
    if (!obj_is_valid(obj)) return;
    obj->contains = contents;
}

struct obj_data *obj_get_next_content(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->next_content;
}

void obj_set_next_content(struct obj_data *obj, struct obj_data *next) {
    if (!obj_is_valid(obj)) return;
    obj->next_content = next;
}

struct obj_data *obj_get_next(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->next;
}

void obj_set_next(struct obj_data *obj, struct obj_data *next) {
    if (!obj_is_valid(obj)) return;
    obj->next = next;
}

int obj_get_value(const struct obj_data *obj, int index) {
    if (!obj_is_valid(obj) || index < 0 || index >= NUM_OBJ_VAL_POSITIONS) return 0;
    return obj->value[index];
}

void obj_set_value(struct obj_data *obj, int index, int value) {
    if (!obj_is_valid(obj) || index < 0 || index >= NUM_OBJ_VAL_POSITIONS) return;
    obj->value[index] = value;
}

int64_t obj_get_weight(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->weight;
}

void obj_set_weight(struct obj_data *obj, int64_t weight) {
    if (!obj_is_valid(obj)) return;
    obj->weight = weight;
}

int obj_get_cost(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->cost;
}

void obj_set_cost(struct obj_data *obj, int cost) {
    if (!obj_is_valid(obj)) return;
    obj->cost = cost;
}

int obj_get_rent_cost(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->cost_per_day;
}

void obj_set_rent_cost(struct obj_data *obj, int cost_per_day) {
    if (!obj_is_valid(obj)) return;
    obj->cost_per_day = cost_per_day;
}

int obj_get_timer(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->timer;
}

void obj_set_timer(struct obj_data *obj, int timer) {
    if (!obj_is_valid(obj)) return;
    obj->timer = timer;
}

struct extra_descr_data *obj_get_ex_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->ex_description;
}

void obj_set_ex_description(struct obj_data *obj, struct extra_descr_data *ex_desc) {
    if (!obj_is_valid(obj)) return;
    obj->ex_description = ex_desc;
}

const struct obj_affected_type *obj_get_affected(const struct obj_data *obj, int index) {
    if (!obj_is_valid(obj) || index < 0 || index >= MAX_OBJ_AFFECT) return NULL;
    return &obj->affected[index];
}

void obj_set_affected_modifier(struct obj_data *obj, int index, int8_t location, int16_t modifier, int skill) {
    if (!obj_is_valid(obj) || index < 0 || index >= MAX_OBJ_AFFECT) return;
    obj->affected[index].location = location;
    obj->affected[index].specific = skill;
    obj->affected[index].modifier = modifier;
}

bool obj_extra_flagged(const struct obj_data *obj, int flag) {
    if (!obj_is_valid(obj)) return false;
    return (obj->extra_flags[0] & flag) != 0;
}

bool obj_wear_flagged(const struct obj_data *obj, int flag) {
    if (!obj_is_valid(obj)) return false;
    return (obj->wear_flags[0] & flag) != 0;
}

bool obj_affect_flagged(const struct obj_data *obj, int flag) {
    if (!obj_is_valid(obj)) return false;
    return (obj->bitvector[0] & flag) != 0;
}

void obj_set_extra_flag(struct obj_data *obj, int flag, bool value) {
    if (!obj_is_valid(obj)) return;
    if (value) {
        obj->extra_flags[0] |= flag;
    } else {
        obj->extra_flags[0] &= ~flag;
    }
}

void obj_set_wear_flag(struct obj_data *obj, int flag, bool value) {
    if (!obj_is_valid(obj)) return;
    if (value) {
        obj->wear_flags[0] |= flag;
    } else {
        obj->wear_flags[0] &= ~flag;
    }
}

void obj_set_affect_flag(struct obj_data *obj, int flag, bool value) {
    if (!obj_is_valid(obj)) return;
    if (value) {
        obj->bitvector[0] |= flag;
    } else {
        obj->bitvector[0] &= ~flag;
    }
}

int32_t obj_get_id(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->id;
}

void obj_set_id(struct obj_data *obj, int32_t id) {
    if (!obj_is_valid(obj)) return;
    obj->id = id;
}

time_t obj_get_generation(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->generation;
}

void obj_set_generation(struct obj_data *obj, time_t gen) {
    if (!obj_is_valid(obj)) return;
    obj->generation = gen;
}

int64_t obj_get_unique_id(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->unique_id;
}

void obj_set_unique_id(struct obj_data *obj, int64_t id) {
    if (!obj_is_valid(obj)) return;
    obj->unique_id = id;
}

int obj_get_scoutfreq(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->scoutfreq;
}

void obj_set_scoutfreq(struct obj_data *obj, int freq) {
    if (!obj_is_valid(obj)) return;
    obj->scoutfreq = freq;
}

time_t obj_get_last_load(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->lload;
}

void obj_set_last_load(struct obj_data *obj, time_t t) {
    if (!obj_is_valid(obj)) return;
    obj->lload = t;
}

int obj_get_healcharge(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->healcharge;
}

void obj_set_healcharge(struct obj_data *obj, int charge) {
    if (!obj_is_valid(obj)) return;
    obj->healcharge = charge;
}

int64_t obj_get_kicharge(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->kicharge;
}

void obj_set_kicharge(struct obj_data *obj, int64_t charge) {
    if (!obj_is_valid(obj)) return;
    obj->kicharge = charge;
}

int obj_get_kitype(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->kitype;
}

void obj_set_kitype(struct obj_data *obj, int type) {
    if (!obj_is_valid(obj)) return;
    obj->kitype = type;
}

struct char_data *obj_get_user(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->user;
}

void obj_set_user(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->user = ch;
}

struct char_data *obj_get_target(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->target;
}

void obj_set_target(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->target = ch;
}

int obj_get_distance(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->distance;
}

void obj_set_distance(struct obj_data *obj, int dist) {
    if (!obj_is_valid(obj)) return;
    obj->distance = dist;
}

int32_t obj_get_aucter(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->aucter;
}

void obj_set_aucter(struct obj_data *obj, int32_t id) {
    if (!obj_is_valid(obj)) return;
    obj->aucter = id;
}

int32_t obj_get_cur_bidder(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->curBidder;
}

void obj_set_cur_bidder(struct obj_data *obj, int32_t id) {
    if (!obj_is_valid(obj)) return;
    obj->curBidder = id;
}

time_t obj_get_auc_time(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->aucTime;
}

void obj_set_auc_time(struct obj_data *obj, time_t t) {
    if (!obj_is_valid(obj)) return;
    obj->aucTime = t;
}

int obj_get_bid(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->bid;
}

void obj_set_bid(struct obj_data *obj, int bid) {
    if (!obj_is_valid(obj)) return;
    obj->bid = bid;
}

int obj_get_start_bid(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->startbid;
}

void obj_set_start_bid(struct obj_data *obj, int bid) {
    if (!obj_is_valid(obj)) return;
    obj->startbid = bid;
}

const char *obj_get_auc_tname(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->auctname ? obj->auctname : "";
}

void obj_set_auc_tname(struct obj_data *obj, const char *name) {
    if (!obj_is_valid(obj)) return;
    if (obj->auctname) free(obj->auctname);
    obj->auctname = name ? strdup(name) : NULL;
}

int obj_get_post_type(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->posttype;
}

void obj_set_post_type(struct obj_data *obj, int type) {
    if (!obj_is_valid(obj)) return;
    obj->posttype = type;
}

struct obj_data *obj_get_posted_to(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->posted_to;
}

void obj_set_posted_to(struct obj_data *obj, struct obj_data *board) {
    if (!obj_is_valid(obj)) return;
    obj->posted_to = board;
}

int obj_get_foob(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->foob;
}

void obj_set_foob(struct obj_data *obj, int val) {
    if (!obj_is_valid(obj)) return;
    obj->foob = val;
}

struct char_data *obj_get_sitting(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->sitting;
}

void obj_set_sitting(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->sitting = ch;
}

struct obj_data *obj_get_fellow_wall(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->fellow_wall;
}

void obj_set_fellow_wall(struct obj_data *obj, struct obj_data *fellow) {
    if (!obj_is_valid(obj)) return;
    obj->fellow_wall = fellow;
}
