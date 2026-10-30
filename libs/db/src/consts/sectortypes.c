#include "dbat/db/consts/sectortypes.h"

/* SECT_ */
const char *sector_types[NUM_ROOM_SECTORS+1] = {
  "Inside",
  "City",
  "Plain",
  "Forest",
  "Hills",
  "Mountains",
  "Shallows",
  "Water (Fly)",
  "Sky",
  "Underwater",
  "$Shop",
  "#Important",
  "Desert",
  "Space",
  "Lava",
  "\n"
};

int movement_loss[NUM_ROOM_SECTORS] =
{
  1,	/* Inside     */
  1,	/* City       */
  1,	/* Field      */
  1,	/* Forest     */
  1,	/* Hills      */
  1,	/* Mountains  */
  1,	/* Swimming   */
  1,	/* Unswimable */
  1,	/* Flying     */
  1,    /* Underwater */
  1,    /* Shop       */
  1,    /* Important  */
  1,    /* Desert     */
  1
};