#include "dbat/iterate.hpp"
#include "dbat/object_systems.hpp"
#include "dbat/utils.hpp"
#include "dbat/handler.hpp"
#include "dbat/comm.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/game_world.hpp"

/* This updates the malfunctioning of certain objects that are damaged. */
void broken_update(GameWorld *g)
{
  struct obj_data *money;

  int rand_gravity[14] = {0, 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 5000, 10000};
  int dice = rand_number(g, 2, 12), grav_roll = 0, grav_change = false, health = 0;

  for (obj_data *k : each_obj(g)) {
    if (obj_get_carried_by(k) != nullptr) {
      continue;
    }

    if (rand_number(g, 1, 2) == 2) {
      continue;
    }

    health = obj_get_value(k, VAL_ALL_HEALTH); // Indicated the health of the object in question

    if (obj_get_vnum(g, k) == 11) { /* Gravity Generator */
      grav_roll = rand_number(g, 0, 13);
      if (health <= 10) {
        grav_change = true;
      } else if (health <= 40 && dice <= 8) {
        grav_change = true;
      } else if (health <= 80 && dice <= 5) {
        grav_change = true;
      } else if (health <= 99 && dice <= 3) {
        grav_change = true;
      }
      if (grav_change == true && obj_get_in_room(k) != NOWHERE) {
        room_set_gravity(g, obj_get_in_room(k), rand_gravity[grav_roll]);
        obj_set_weight(k, rand_gravity[grav_roll]);
        send_to_room(g, obj_get_in_room(k), "@RThe gravity generator malfunctions! The gravity level has changed!@n\r\n");
      }
    } /* End Gravity Section */

    if (obj_get_vnum(g, k) == 3034 && obj_get_in_room(k) != NOWHERE) { /* ATM */
      if (health <= 10) {
        send_to_room(g, obj_get_in_room(k), "@RThe ATM machine shoots smoking bills from its money slot. The bills burn up as they float through the air!@n\r\n");
      } else if (health <= 40 && dice <= 8) {
        send_to_room(g, obj_get_in_room(k), "@RGibberish flashes across the cracked ATM info screen.@n\r\n");
      } else if (health <= 80 && dice == 4) {
        send_to_room(g, obj_get_in_room(k), "@GThe damaged ATM spits out some money while flashing ERROR on its screen!@n\r\n");
        money = create_money(g, rand_number(g, 1, 30));
        obj_to_room(g, money, obj_get_in_room(k));
      } else if (health <= 99 && dice < 4) {
        send_to_room(g, obj_get_in_room(k), "@RThe ATM machine emits a loud grinding sound from inside.@n\r\n");
      }
    } /* End ATM */

    dice = rand_number(g, 2, 12); // Reset the dice
  } /* End For */
}