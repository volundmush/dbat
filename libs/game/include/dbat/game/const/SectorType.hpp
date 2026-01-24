#pragma once
#include <cstdint>

enum class SectorType : std::uint8_t
{
    inside = 0,       // Indoors
    city = 1,         // In a city
    field = 2,        // In a field
    forest = 3,       // In a forest
    hills = 4,        // In the hills
    mountain = 5,     // On a mountain
    water_swim = 6,   // Swimmable water
    water_noswim = 7, // Water - need a boat
    flying = 8,       // Wheee!
    underwater = 9,   // Underwater
    shop = 10,        // Shop
    important = 11,   // Important Rooms
    desert = 12,      // A desert
    space = 13,       // This is a space room
    lava = 14         // This room always has lava
};

/* Sector types: used in Room.sector_type */
constexpr int SECT_INSIDE = 0;       /* Indoors			*/
constexpr int SECT_CITY = 1;         /* In a city			*/
constexpr int SECT_FIELD = 2;        /* In a field		*/
constexpr int SECT_FOREST = 3;       /* In a forest		*/
constexpr int SECT_HILLS = 4;        /* In the hills		*/
constexpr int SECT_MOUNTAIN = 5;     /* On a mountain		*/
constexpr int SECT_WATER_SWIM = 6;   /* Swimmable water		*/
constexpr int SECT_WATER_NOSWIM = 7; /* Water - need a boat	*/
constexpr int SECT_FLYING = 8;       /* Wheee!			*/
constexpr int SECT_UNDERWATER = 9;   /* Underwater		*/
constexpr int SECT_SHOP = 10;        /* Shop                      */
constexpr int SECT_IMPORTANT = 11;   /* Important Rooms           */
constexpr int SECT_DESERT = 12;      /* A desert                  */
constexpr int SECT_SPACE = 13;       /* This is a space room      */
constexpr int SECT_LAVA = 14;        /* This room always has lava */
