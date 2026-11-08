/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/handler.hpp"
#include "dbat/utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/shop.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
/* external vars */

/* local functions */

/* external functions */

/* Stock isname().  Leave this here even if you put in a newer  *
 * isname().  Used for OasisOLC.                                */

static void update_objects(GameWorld *g, const id_list &list, int use);

/* Runs down the timers of obj and what it contains */
static void update_object(GameWorld *g, struct obj_data *obj, int use)
{
  /* dont update objects with a timer trigger */
  if (!script_has_type(obj_get_script(obj), OTRIG_TIMER) && (obj_get_timer(obj) > 0))
    obj_set_timer(obj, obj_get_timer(obj) - (use));
  update_objects(g, obj_get_contains(obj), use);
}

static void update_objects(GameWorld *g, const id_list &list, int use)
{
  list.each([&](long id) {
    if (struct obj_data *obj = obj_by_id(g, id))
      update_object(g, obj, use);
  });
}

void update_char_objects(GameWorld *g, struct char_data *ch)
{
  int i, j;

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i)) {
      if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT && obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS) > 0 && obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_TIME) <= 0) {
        obj_set_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS, obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS) - 1);
        j = obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS);
        obj_set_value(char_get_equipment(ch, i), VAL_LIGHT_TIME, 3);
        if (j == 1) {
          send_to_char(g, ch, "Your light begins to flicker and fade.\r\n");
          act(g, "$n's light begins to flicker and fade.", false, ch, nullptr, nullptr, TO_ROOM);
        } else if (j == 0) {
          send_to_char(g, ch, "Your light sputters out and dies.\r\n");
          act(g, "$n's light sputters out and dies.", false, ch, nullptr, nullptr, TO_ROOM);
          room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) - 1);
        }
      } else if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT && obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS) > 0) {
        obj_set_value(char_get_equipment(ch, i), VAL_LIGHT_TIME, obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_TIME) - (1));
      }
      update_object(g, char_get_equipment(ch, i), 2);
    }

  if (char_get_inventory(ch))
    update_objects(g, char_get_inventory(ch), 1);
}

/* check and see if this item is better */
void item_check(GameWorld *g, struct obj_data *object, struct char_data *ch)
{
  int where = 0;

  if (char_is_humanoid(ch) && !(mob_index_of(g, char_get_nr(ch))->func == shop_keeper)) {
    if (invalid_align(ch, object) || invalid_class(ch, object))
      return;

    switch (obj_get_type_flag(object)) {
      case ITEM_WEAPON:
        if (!char_get_equipment(ch, WEAR_WIELD1)) {
          perform_wear(g, ch, object, WEAR_WIELD1);
        } else {
          if (is_better(object, char_get_equipment(ch, WEAR_WIELD1))) {
            perform_remove(g, ch, WEAR_WIELD1);
            perform_wear(g, ch, object, WEAR_WIELD1);
          }
        }
        break;
      case ITEM_ARMOR:
      case ITEM_WORN:
        where = find_eq_pos(g, ch, object, nullptr);
        if (where < 0) /* armor with nowhere to wear it */
          break;
        if (!char_get_equipment(ch, where)) {
          perform_wear(g, ch, object, where);
        } else {
          if (is_better(object, char_get_equipment(ch, where))) {
            perform_remove(g, ch, where);
            perform_wear(g, ch, object, where);
          }
        }

        break;
      default:
        break;
    }
  }
}
