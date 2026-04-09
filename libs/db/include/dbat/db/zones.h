#pragma once
#include "consts/types.h"
#include "consts/zoneflags.h"


/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int  arg4;		/* room_max  default 0			*/
   int  arg5;           /* percentages variable                 */
   int line;		/* line number this command appears on  */
   char *sarg1;		/* string argument                      */
   char *sarg2;		/* string argument                      */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
	*  'T': Trigger command   *
        *  'V': Assign a variable *
   */
};



/* zone definition structure. for the 'zone-table'   */
#define CUR_WORLD_VERSION 1
#define CUR_ZONE_VERSION  2

struct zone_data {
   char	*name;		    /* name of this zone                  */
   char *builders;          /* namelist of builders allowed to    */
                            /* modify this zone.		  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   room_vnum bot;           /* starting room number for this zone */
   room_vnum top;           /* upper limit for rooms in this zone */

   int	reset_mode;         /* conditions for reset (see below)   */
   zone_vnum number;	    /* virtual number of this zone	  */
   struct reset_com *cmd;   /* command table for reset	          */
   int min_level;           /* Minimum level to enter zone        */
   int max_level;           /* Max Mortal level to enter zone     */
   int zone_flags[ZF_ARRAY_MAX];          /* Flags for the zone.                */


   /*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
    */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

extern struct reset_q_type reset_q;	/* queue of zones to be reset	 */