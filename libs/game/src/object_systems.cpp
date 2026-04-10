#include "dbat/game/object_systems.h"
#include "dbat/game/utils.h"
#include "dbat/game/handler.h"
#include "dbat/game/comm.h"

/* This updates the malfunctioning of certain objects that are damaged. */
void broken_update()
{
 struct obj_data *k, *money;

 int rand_gravity[14] = {0, 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 5000, 10000};
 int dice = rand_number(2, 12), grav_roll = 0, grav_change = FALSE, health = 0;

 for (k = object_list; k; k = k->next) {
  if (k->carried_by != NULL) {
   continue;
  }

  if (rand_number(1, 2) == 2) {
   continue;
  }

  health = GET_OBJ_VAL(k, VAL_ALL_HEALTH); // Indicated the health of the object in question

  if (GET_OBJ_VNUM(k) == 11) { /* Gravity Generator */
   grav_roll = rand_number(0, 13);
   if (health <= 10) {
    grav_change = TRUE;
   } else if (health <= 40 && dice <= 8) {
    grav_change = TRUE;
   } else if (health <= 80 && dice <= 5) {
    grav_change = TRUE;
   } else if (health <= 99 && dice <= 3) {
    grav_change = TRUE;
   }
   if (grav_change == TRUE) {
    ROOM_GRAVITY(IN_ROOM(k)) = rand_gravity[grav_roll];
    GET_OBJ_WEIGHT(k) = rand_gravity[grav_roll];
    send_to_room(IN_ROOM(k), "@RThe gravity generator malfunctions! The gravity level has changed!@n\r\n");
   }
  } /* End Gravity Section */

  if (GET_OBJ_VNUM(k) == 3034) { /* ATM */
   if (health <= 10) {
    send_to_room(IN_ROOM(k), "@RThe ATM machine shoots smoking bills from its money slot. The bills burn up as they float through the air!@n\r\n");
   } else if (health <= 40 && dice <= 8) {
    send_to_room(IN_ROOM(k), "@RGibberish flashes across the cracked ATM info screen.@n\r\n");
   } else if (health <= 80 && dice == 4) {
    send_to_room(IN_ROOM(k), "@GThe damaged ATM spits out some money while flashing ERROR on its screen!@n\r\n");
    money = create_money(rand_number(1, 30));
    obj_to_room(money, IN_ROOM(k));
   } else if (health <= 99 && dice < 4) {
    send_to_room(IN_ROOM(k), "@RThe ATM machine emits a loud grinding sound from inside.@n\r\n");
   }
  } /* End ATM */

  dice = rand_number(2, 12); // Reset the dice
 } /* End For */

}