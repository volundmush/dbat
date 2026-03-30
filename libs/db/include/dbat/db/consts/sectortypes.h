#pragma once

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	     8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/
#define SECT_SHOP            10            /* Shop                      */
#define SECT_IMPORTANT       11            /* Important Rooms           */
#define SECT_DESERT          12            /* A desert                  */
#define SECT_SPACE           13            /* This is a space room      */
#define SECT_LAVA            14            /* This room always has lava */

#define NUM_ROOM_SECTORS     15