#include "dbat/db/objects.h"
#include "dbat/db/object_utils.h"
#include "dbat/game/utils.h"
#include "dbat/game/db.h"


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