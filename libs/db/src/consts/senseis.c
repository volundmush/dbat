#include "dbat/db/consts/senseis.h"

#define Y   TRUE
#define N   FALSE

/* Sensei Fighting Style */
const char *sensei_style[MAX_SENSEI] = {
 "Kame Arts", /* Roshi */
 "Demon Taijutsu", /* Piccolo */
 "Crane Arts", /* Krane */
 "Tranquil Palm", /* Nail */
 "Brutal Beast", /* Bardock */
 "Flaunted Style", /* Ginyu */
 "Frozen Fist", /* Frieza */
 "Shadow Grappling", /* Tapion */
 "Iron Hand", /* Sixteen */
 "Devil Dance", /* Dabura */
 "Gentle Fist", /* Kibito */
 "Star's Radiance", /* Jinto */
 "Sacred Tsunami", /* Tsuna */
 "Adaptive Taijutsu", /* Kurzak */
 "\n"
};


const char *class_abbrevs[NUM_CLASSES+1] = {
  "Ro",
  "Pi",
  "Kr",
  "Na",
  "Ba",
  "Gi",
  "Fr",
  "Ta",
  "An",
  "Da",
  "Ki",
  "Ji",
  "Ts",
  "Ku",
  "As",
  "Bl",
  "Dd",
  "Du",
  "Dw",
  "Ek",
  "Ht",
  "Hw",
  "Lo",
  "Mt",
  "Sh",
  "Th",
  "Ex",
  "Ad",
  "Co",
  "Ar",
  "Wa",
  "\n"
};


/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *pc_class_types[NUM_CLASSES+1] = {
  "Roshi",
  "Piccolo",
  "Krane",
  "Nail",
  "Bardock",
  "Ginyu",
  "Frieza",
  "Tapion",
  "Android 16",
  "Dabura",
  "Kibito",
  "Jinto",
  "Tsuna",
  "Kurzak",
  "Assassin",
  "Blackguard",
  "Dragon Disciple",
  "Duelist",
  "Dwarven Defender",
  "Eldritch Knight",
  "Hierophant",
  "Horizon Walker",
  "Loremaster",
  "Mystic Theurge",
  "Shinobi",
  "Thaumaturgist",
  "Expert",
  "Adept",
  "Commoner",
  "Aristrocrat",
  "Warrior",
  "\n"
};

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *class_names[NUM_CLASSES+1] = {
  "roshi",
  "piccolo",
  "krane",
  "nail",
  "bardock",
  "ginyu",
  "frieza",
  "tapion",
  "android 16",
  "dabura",
  "kibito",
  "jinto",
  "tsuna",
  "kurzak",
  "assassin",
  "blackguard",
  "dragon disciple",
  "duelist",
  "dwarven defender",
  "eldritch knight",
  "hierophant",
  "horizon walker",
  "loremaster",
  "mystic theurge",
  "shadowdancer",
  "thaumaturgist",
  "artisan",
  "magi",
  "normal",
  "noble",
  "soldier",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_display[NUM_CLASSES] = {
  "@B1@W) @MRoshi\r\n",
  "@B2@W) @WPiccolo\r\n",
  "@B3@W) @YKrane\r\n",
  "@B4@W) @BNail\r\n",
  "@B5@W) @BBardock\r\n",
  "@B6@W) @BGinyu\r\n",
  "@B7@W) @WFrieza\r\n",
  "@B8@W) @YTapion\r\n",
  "@B9@W) @BAndroid 16\r\n",
  "@B10@W) @BDabura\r\n",
  "@B11@W) @BKibito\r\n",
  "@B12@W) @BJinto\r\n",
  "@B13@W) @BTsuna\r\n",
  "@B14@W) @BKurzak\r\n",
  "assassin (P)\r\n",
  "blackguard (P)\r\n",
  "dragon disciple (P)\r\n",
  "duelist (P)\r\n",
  "dwarven defender (P)\r\n",
  "eldritch knight (P)\r\n",
  "hierophant (P)\r\n",
  "horizon walker (P)\r\n",
  "loremaster (P)\r\n",
  "mystic theurge (P)\r\n",
  "shadowdancer (P)\r\n",
  "thaumaturgist (P)\r\n",
  "Artisan NPC\r\n",
  "Magi NPC\r\n",
  "Normal NPC\r\n",
  "Noble NPC\r\n",
  "Soldier NPC\r\n",
};