#include "dbat/db/consts/weapons.h"


/* Armor Types */
const char *armor_type[MAX_ARMOR_TYPES+1] = {
  "Undefined",
  "Light",
  "Medium",
  "Heavy",
  "Shield",
  "\n"
};

/* Weapon Types */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *weapon_type[MAX_WEAPON_TYPES+2] = {
  "undefined",
  "unarmed",
  "dagger",
  "mace",
  "sickle",
  "spear",
  "staff",
  "crossbow",
  "longbow",
  "shortbow",
  "sling",
  "shuriken",
  "hammer",
  "lance",
  "flail",
  "longsword",
  "shortsword",
  "greatsword",
  "rapier",
  "scimitar",
  "polearm",
  "club",
  "bastard sword",
  "monk weapon",
  "double weapon",
  "axe",
  "whip",
  "\n"
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *crit_type[NUM_CRIT_TYPES+1] =
{
  "x2",
  "x3",
  "x4",
  "\n"
};



/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *wield_names[NUM_WIELD_NAMES+1] = {
  "if you were bigger",
  "with ease",
  "one-handed",
  "two-handed",
  "\n"
};
