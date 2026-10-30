#pragma once

/* Sector types: used in room_data.sector_type */
constexpr int SECT_INSIDE          = 0; /* Indoors			*/
constexpr int SECT_CITY            = 1; /* In a city			*/
constexpr int SECT_FIELD           = 2; /* In a field		*/
constexpr int SECT_FOREST          = 3; /* In a forest		*/
constexpr int SECT_HILLS           = 4; /* In the hills		*/
constexpr int SECT_MOUNTAIN        = 5; /* On a mountain		*/
constexpr int SECT_WATER_SWIM      = 6; /* Swimmable water		*/
constexpr int SECT_WATER_NOSWIM    = 7; /* Water - need a boat	*/
constexpr int SECT_FLYING	     = 8; /* Wheee!			*/
constexpr int SECT_UNDERWATER	     = 9; /* Underwater		*/
constexpr int SECT_SHOP            = 10; /* Shop                      */
constexpr int SECT_IMPORTANT       = 11; /* Important Rooms           */
constexpr int SECT_DESERT          = 12; /* A desert                  */
constexpr int SECT_SPACE           = 13; /* This is a space room      */
constexpr int SECT_LAVA            = 14; /* This room always has lava */

constexpr int NUM_ROOM_SECTORS     = 15;

extern const char *sector_types[NUM_ROOM_SECTORS+1];

extern int movement_loss[NUM_ROOM_SECTORS];