#pragma once
#include "dbat/consts/types.h"

constexpr int MAX_HOUSES = 1000;
constexpr int MAX_GUESTS = 10;

constexpr int HOUSE_PRIVATE = 0;
constexpr int HOUSE_GOD     = 1; /* Imm owned house             */
constexpr int HOUSE_CLAN    = 2; /* Clan crash-save room        */
constexpr int HOUSE_UNOWNED = 3;

constexpr int NUM_HOUSE_TYPES = 4;

struct house_control_rec {
  room_vnum vnum;          /* vnum of this house		*/
  room_vnum atrium;        /* vnum of atrium		*/
  int16_t exit_num;        /* direction of house's exit	*/
  time_t built_on;         /* date this house was built	*/
  int mode;                /* mode of ownership		*/
  long owner;              /* idnum of house's owner	*/
  int num_of_guests;       /* how many guests for house	*/
  long guests[MAX_GUESTS]; /* idnums of house's guests	*/
  time_t last_payment;     /* date of last house payment   */
  long bitvector;
  long builtby;
  long spare2;
  long spare3;
  long spare4;
  long spare5;
  long spare6;
  long spare7;
};

/* House can have up to 31 bitvectors - don't go higher */
constexpr int HOUSE_NOGUESTS   = (1 << 0); /* Owner cannot add guests     */
constexpr int HOUSE_FREE       = (1 << 1); /* House does not require payments */
constexpr int HOUSE_NOIMMS     = (1 << 2); /* Imms below level 999 cannot enter */
constexpr int HOUSE_IMPONLY    = (1 << 3); /* Imms below level 1000 cannot enter */
constexpr int HOUSE_RENTFREE   = (1 << 4); /* No rent is charged on items left here */
constexpr int HOUSE_SAVENORENT = (1 << 5); /* NORENT items are crashsaved too */
constexpr int HOUSE_NOSAVE     = (1 << 6); /* Do not crash save this room - private only */

constexpr int HOUSE_NUM_FLAGS = 7;


void House_boot(GameWorld *g);
void House_save_all(GameWorld *g);
bool House_can_enter(GameWorld *g, struct char_data *ch, room_vnum house);
void hcontrol_list_houses(GameWorld *g, struct char_data *ch);
int find_house(GameWorld *g, room_vnum vnum);         /* house.c */
void House_save_control(GameWorld *g);          /* house.c */
void House_delete_file(room_vnum vnum); /* house.c */

ACMD(do_hcontrol);
ACMD(do_house);
