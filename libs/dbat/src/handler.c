/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/handler.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/shop.h"
/* external vars */

/* local functions */
static void update_object(struct obj_data *obj, int use);


/* external functions */

/* Stock isname().  Leave this here even if you put in a newer  *
 * isname().  Used for OasisOLC.                                */


static void update_object(struct obj_data *obj, int use)
{
  if (!obj)
    return;
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (obj_get_timer(obj) > 0))
    obj_set_timer(obj, obj_get_timer(obj) - (use));
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}


void update_char_objects(struct char_data *ch)
{
  int i, j;

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i)) {
      if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT && obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS) > 0 && obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_TIME) <= 0) {
        obj_set_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS, obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS) - 1);
        j = obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS);
        obj_set_value(char_get_equipment(ch, i), VAL_LIGHT_TIME, 3);
	if (j == 1) {
	  send_to_char(ch, "Your light begins to flicker and fade.\r\n");
	  act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (j == 0) {
	  send_to_char(ch, "Your light sputters out and dies.\r\n");
	  act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
	  world[char_get_in_room(ch)].light--;
	}
      } else if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT && obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS) > 0) {
       obj_set_value(char_get_equipment(ch, i), VAL_LIGHT_TIME, obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_TIME) - (1));
      }
      update_object(char_get_equipment(ch, i), 2);
    }

  if (ch->carrying)
    update_object(ch->carrying, 1);
}


/* check and see if this item is better */
void item_check(struct obj_data *object, struct char_data *ch)
{
  int where=0;

  if (char_is_humanoid(ch) && !(mob_index[char_get_nr(ch)].func == shop_keeper)) {
    if (invalid_align(ch, object) || invalid_class(ch, object))
      return;

    switch (obj_get_type_flag(object)) {
      case ITEM_WEAPON:
        if (!char_get_equipment(ch, WEAR_WIELD1)) {
          perform_wear(ch, object, WEAR_WIELD1);
        } else {
          if (is_better(object, char_get_equipment(ch, WEAR_WIELD1))) {
            perform_remove(ch, WEAR_WIELD1);
            perform_wear(ch, object, WEAR_WIELD1);
          }
        }
      break;
      case ITEM_ARMOR:
      case ITEM_WORN:
        where = find_eq_pos(ch, object, 0);
        if (!char_get_equipment(ch, where)) {
          perform_wear(ch, object, where);
        } else {
          if (is_better(object, char_get_equipment(ch, where))) {
            perform_remove(ch, where);
            perform_wear(ch, object, where);
          } 
        }

      break;
      default:
      break;
    }
  }
}
