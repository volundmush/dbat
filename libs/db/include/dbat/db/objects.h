#pragma once
#include "consts/types.h"
#include "consts/itemdata.h"
#include "consts/maximums.h"
#include "consts/affflags.h"
#include "affected.h"

struct obj_spellbook_spell {
   int spellname;	/* Which spell is written */
   int pages;		/* How many pages does it take up */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/
   room_vnum room_loaded;	/* Room loaded in, for room_max checks	*/

   int  value[NUM_OBJ_VAL_POSITIONS];   /* Values of the item (see list)    */
   int8_t type_flag;      /* Type of item                        */
   int  level;           /* Minimum level of object.            */
   int  wear_flags[TW_ARRAY_MAX]; /* Where you can wear it     */
   int  extra_flags[EF_ARRAY_MAX]; /* If it hums, glows, etc.  */
   int64_t  weight;         /* Weigt what else                     */
   int  cost;           /* Value when sold (gp.)               */
   int  cost_per_day;   /* Cost to keep pr. real day           */
   int  timer;          /* Timer for object                    */
   int  bitvector[AF_ARRAY_MAX]; /* To set chars bits          */
   int  size;           /* Size class of object                */

   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   int16_t worn_on;		  /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   int32_t id;                       /* used by DG triggers              */
   time_t generation;             /* creation time for dupe check     */
   int64_t unique_id;  /* random bits for dupe check       */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */

   struct obj_spellbook_spell *sbinfo;  /* For spellbook info */
   struct char_data *sitting;       /* Who is sitting on me? */
   int scoutfreq;
   time_t lload;
   int healcharge;
   int64_t kicharge;
   int kitype;
   struct char_data *user;
   struct char_data *target;
   int distance;
   int foob;
   int32_t aucter;
   int32_t curBidder;
   time_t aucTime;
   int bid;
   int startbid;
   char *auctname;
   int posttype;
   struct obj_data *posted_to;
   struct obj_data *fellow_wall;
};