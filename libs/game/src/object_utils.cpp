#include "dbat/db/objects.h"
#include "dbat/db/object_utils.h"
#include "dbat/game/object_utils.h"


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