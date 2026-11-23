#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/senseis.hpp"

/* Sensei Fighting Style */
static constexpr std::string_view sensei_style_entries[MAX_SENSEI] = {
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
static_assert(std::size(sensei_style_entries) == MAX_SENSEI);
constinit const name_table sensei_style(sensei_style_entries);

static constexpr std::string_view class_abbrevs_entries[] = {
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
    "Wa"};
static_assert(std::size(class_abbrevs_entries) == NUM_CLASSES);
constinit const name_table class_abbrevs(class_abbrevs_entries);

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
static constexpr std::string_view pc_class_types_entries[] = {
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
    "Warrior"};
static_assert(std::size(pc_class_types_entries) == NUM_CLASSES);
constinit const name_table pc_class_types(pc_class_types_entries);

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
static constexpr std::string_view class_names_entries[] = {
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
    "soldier"};
static_assert(std::size(class_names_entries) == NUM_CLASSES);
constinit const name_table class_names(class_names_entries);

/* The menu for choosing a class in interpreter.c: */