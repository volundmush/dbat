#pragma once
#include "consts/types.h"
#include "consts/zoneflags.h"


#ifdef __cplusplus
extern "C" {
#endif

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
   zone_vnum number;	    /* virtual number of this zone	  */
   char	*name;		    /* name of this zone                  */
   char *builders;          /* namelist of builders allowed to modify this zone.		  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   room_vnum bot;           /* starting room number for this zone */
   room_vnum top;           /* upper limit for rooms in this zone */
   int	reset_mode;         /* conditions for reset (see below)   */
   int min_level;           /* Minimum level to enter zone        */
   int max_level;           /* Max Mortal level to enter zone     */
   bitvector_t zone_flags[ZF_ARRAY_MAX];          /* Flags for the zone.                */
   struct reset_com *cmd;   /* command table for reset	          */

   /*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
    */
};

// Zone API functions, implemented in zones_api.zig
zone_vnum zone_id_get(struct zone_data *zone);
void zone_id_set(struct zone_data *zone, zone_vnum id);
const char *zone_name_get(struct zone_data *zone);
void zone_name_set(struct zone_data *zone, const char *value);
const char *zone_builders_get(struct zone_data *zone);
void zone_builders_set(struct zone_data *zone, const char *value);
int zone_lifespan_get(struct zone_data *zone);
void zone_lifespan_set(struct zone_data *zone, int lifespan);
int zone_age_get(struct zone_data *zone);
void zone_age_set(struct zone_data *zone, int age);
room_vnum zone_bottom_get(struct zone_data *zone);
void zone_bottom_set(struct zone_data *zone, room_vnum bottom);
room_vnum zone_top_get(struct zone_data *zone);
void zone_top_set(struct zone_data *zone, room_vnum top);
int zone_reset_mode_get(struct zone_data *zone);
void zone_reset_mode_set(struct zone_data *zone, int mode);
int zone_min_level_get(struct zone_data *zone);
void zone_min_level_set(struct zone_data *zone, int level);
int zone_max_level_get(struct zone_data *zone);
void zone_max_level_set(struct zone_data *zone, int level);
bool zone_flagged(struct zone_data *zone, int pos);
bool zone_flag_toggle(struct zone_data *zone, int pos);
void zone_flag_set(struct zone_data *zone, int pos, bool value);
struct reset_com *zone_command_get(struct zone_data *zone, size_t index);

char zone_command_type_get(struct reset_com *cmd);
void zone_command_type_set(struct reset_com *cmd, char command);
bool zone_command_if_flag_get(struct reset_com *cmd);
void zone_command_if_flag_set(struct reset_com *cmd, bool value);
int zone_command_arg_get(struct reset_com *cmd, size_t index);
void zone_command_arg_set(struct reset_com *cmd, size_t index, int value);
int zone_command_line_get(struct reset_com *cmd);
void zone_command_line_set(struct reset_com *cmd, int line);
const char *zone_command_sarg1_get(struct reset_com *cmd);
void zone_command_sarg1_set(struct reset_com *cmd, const char *value);
const char *zone_command_sarg2_get(struct reset_com *cmd);
void zone_command_sarg2_set(struct reset_com *cmd, const char *value);



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


extern struct reset_q_type reset_q;	/* queue of zones to be reset	 */

zone_vnum real_zone(zone_vnum vnum);
struct zone_data *zone_by_id(zone_vnum vnum);
struct zone_data *zone_get(zone_vnum vnum);

zone_vnum virtual_zone_by_thing(room_vnum vznum);

void* zone_iterator_create();
struct zone_data* zone_next(void* iterator);
void zone_iterator_free(void* iterator);

void zone_put(zone_vnum vnum, struct zone_data *zone);
void zone_delete(zone_vnum vnum);
size_t zone_count();

#ifdef __cplusplus
}
#endif
