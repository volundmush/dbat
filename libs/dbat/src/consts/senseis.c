#include "dbat/consts/senseis.h"

#define Y TRUE
#define N FALSE

/* Sensei Fighting Style */
const char *sensei_style[MAX_SENSEI] = {
    "Kame Arts",         /* Roshi */
    "Demon Taijutsu",    /* Piccolo */
    "Crane Arts",        /* Krane */
    "Tranquil Palm",     /* Nail */
    "Brutal Beast",      /* Bardock */
    "Flaunted Style",    /* Ginyu */
    "Frozen Fist",       /* Frieza */
    "Shadow Grappling",  /* Tapion */
    "Iron Hand",         /* Sixteen */
    "Devil Dance",       /* Dabura */
    "Gentle Fist",       /* Kibito */
    "Star's Radiance",   /* Jinto */
    "Sacred Tsunami",    /* Tsuna */
    "Adaptive Taijutsu", /* Kurzak */
    "\n"};

const char *class_abbrevs[NUM_CLASSES + 1] = {
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
    "\n"};

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *pc_class_types[NUM_CLASSES + 1] = {
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
    "\n"};

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *class_names[NUM_CLASSES + 1] = {
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
    "\n"};

/* The menu for choosing a class in interpreter.c: */