#pragma once
#include "consts/types.h"
#include "consts/roomflags.h"
#include "consts/directions.h"

struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   int16_t exit_info;		/* Exit info			*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
   int dclock;			/* DC to pick the lock			*/
   int dchide;			/* DC to find hidden			*/
   int dcskill;			/* Skill req. to move through exit	*/
   int dcmove;			/* DC for skill to move through exit	*/
   int failsavetype;		/* Saving Throw type on skill fail	*/
   int dcfailsave;		/* DC to save against on fail		*/
   int failroom;		/* Room # to put char in when fail > 5  */
   int totalfailroom;		/* Room # if char fails save < 5	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   zone_rnum zone;              /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   int room_flags[RF_ARRAY_MAX];   /* DEATH,DARK ... etc */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */

   int8_t light;                  /* Number of lightsources in room     */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */

   int timed;                   /* For timed Dt's                     */
   int dmg;                     /* How damaged the room is            */
   int gravity;                 /* What is the level of gravity?      */
   int geffect;			/* Effect of ground destruction       */

};
/* ====================================================================== */

extern struct room_data *world;
extern room_rnum top_of_world;