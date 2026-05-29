#pragma once
#include "consts/types.h"
#include "consts/itemdata.h"
#include "consts/maximums.h"
#include "consts/affflags.h"
#include "consts/sizes.h"
#include "consts/weapons.h"
#include "consts/applies.h"
#include "affected.h"
#include "index.h"
#include "htree.h"
#include "extradesc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct obj_spellbook_spell {
   int spellname;	/* Which spell is written */
   int pages;		/* How many pages does it take up */
};

struct obj_data {
   obj_vnum vnum;	/* Where in data-base			*/
   room_vnum in_room;		/* In what room -1 when conta/carr	*/
   room_vnum room_loaded;	/* Room loaded in, for room_max checks	*/

   int  value[NUM_OBJ_VAL_POSITIONS];   /* Values of the item (see list)    */
   int8_t type_flag;      /* Type of item                        */
   int  level;           /* Minimum level of object.            */
   bitvector_t  wear_flags[TW_ARRAY_MAX]; /* Where you can wear it     */
   bitvector_t  extra_flags[EF_ARRAY_MAX]; /* If it hums, glows, etc.  */
   int64_t  weight;         /* Weigt what else                     */
   int  cost;           /* Value when sold (gp.)               */
   int  timer;          /* Timer for object                    */
   bitvector_t  bitvector[AF_ARRAY_MAX]; /* To set chars bits          */
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

   struct char_data *sitting;       /* Who is sitting on me? */

   // scouter frequency
   int scoutfreq;

   time_t lload;

   // healing tank charge
   int healcharge;

   // ki attacks
   int64_t kicharge;
   int kitype;
   struct char_data *user;
   struct char_data *target;
   int distance;

   // something about food
   int foob;

   // auction data
   int32_t aucter;
   int32_t curBidder;
   time_t aucTime;
   int bid;
   int startbid;
   char *auctname;
   
   // notes and boards
   int posttype;
   struct obj_data *posted_to;

   // icewall stuff
   struct obj_data *fellow_wall;

   // UNUSED FIELDS below here
   int  cost_per_day;   /* Cost to keep pr. real day           */
   struct obj_spellbook_spell *sbinfo;  /* For spellbook info */
};

// Object API functions, implemented in objects_api.zig
int64_t obj_id_get(struct obj_data *obj);
void obj_id_set(struct obj_data *obj, int64_t id);
obj_vnum obj_proto_id_get(struct obj_data *obj);
void obj_proto_id_set(struct obj_data *obj, obj_vnum vnum);
obj_vnum obj_vnum_get(struct obj_data *obj);
void obj_vnum_set(struct obj_data *obj, obj_vnum vnum);
struct room_data* obj_room_get(struct obj_data *obj);
room_vnum obj_room_vnum_get(struct obj_data *obj);
void obj_room_vnum_set(struct obj_data *obj, room_vnum vnum);
room_vnum obj_room_loaded_get(struct obj_data *obj);
void obj_room_loaded_set(struct obj_data *obj, room_vnum vnum);
int obj_value_get(struct obj_data *obj, size_t pos);
int obj_value_mod(struct obj_data *obj, size_t pos, int delta);
void obj_value_set(struct obj_data *obj, size_t pos, int value);
int8_t obj_type_get(struct obj_data *obj);
void obj_type_set(struct obj_data *obj, int8_t type);
int obj_level_get(struct obj_data *obj);
void obj_level_set(struct obj_data *obj, int level);
bool obj_wear_flagged(struct obj_data *obj, int pos);
bool obj_wear_flag_toggle(struct obj_data *obj, int pos);
void obj_wear_flag_set(struct obj_data *obj, int pos, bool value);
bool obj_extra_flagged(struct obj_data *obj, int pos);
bool obj_extra_flag_toggle(struct obj_data *obj, int pos);
void obj_extra_flag_set(struct obj_data *obj, int pos, bool value);
bool obj_aff_flagged(struct obj_data *obj, int pos);
bool obj_aff_flag_toggle(struct obj_data *obj, int pos);
void obj_aff_flag_set(struct obj_data *obj, int pos, bool value);
int64_t obj_weight_get(struct obj_data *obj);
int64_t obj_weight_get_contained(struct obj_data *obj); // weight of contained objects only
int64_t obj_weight_get_total(struct obj_data *obj); // this includes the weight of contained objects
int64_t obj_weight_mod(struct obj_data *obj, int64_t delta);
void obj_weight_set(struct obj_data *obj, int64_t weight);
int obj_cost_get(struct obj_data *obj);
int obj_cost_mod(struct obj_data *obj, int delta);
void obj_cost_set(struct obj_data *obj, int cost);
int obj_timer_get(struct obj_data *obj);
int obj_timer_mod(struct obj_data *obj, int delta);
void obj_timer_set(struct obj_data *obj, int timer);
int obj_size_get(struct obj_data *obj);
void obj_size_set(struct obj_data *obj, int size);
// Not sure how to handle affected array yet...
const char* obj_name_get(struct obj_data *obj);
void obj_name_set(struct obj_data *obj, const char *value);
const char* obj_description_get(struct obj_data *obj);
void obj_description_set(struct obj_data *obj, const char *value);
const char* obj_short_description_get(struct obj_data *obj);
void obj_short_description_set(struct obj_data *obj, const char *value);
const char* obj_action_description_get(struct obj_data *obj);
void obj_action_description_set(struct obj_data *obj, const char *value);
int64_t obj_carried_by_get(struct obj_data *obj);
void obj_carried_by_set(struct obj_data *obj, struct char_data *ch);
int64_t obj_worn_by_get(struct obj_data *obj);
void obj_worn_by_set(struct obj_data *obj, struct char_data *ch);
int16_t obj_worn_on_get(struct obj_data *obj);
void obj_worn_on_set(struct obj_data *obj, int16_t pos);
int64_t obj_in_obj_get(struct obj_data *obj);
void obj_in_obj_set(struct obj_data *obj, struct obj_data *in_obj);
int64_t obj_sitting_get(struct obj_data *obj);
void obj_sitting_set(struct obj_data *obj, struct char_data *ch);

size_t obj_inventory_count(struct obj_data *obj, bool recursive);

bool obj_search_vnum_match(struct obj_data *obj, void *ctx);
bool obj_search_type_match(struct obj_data *obj, void *ctx);

void obj_contents_list_iterate(struct obj_data *obj, bool recursive, obj_iter_fn func, void *ctx);
void obj_inventory_iterate(struct obj_data *obj, bool recursive, obj_iter_fn func, void *ctx);


struct obj_data* obj_contents_search_vnum(struct obj_data *obj, obj_vnum vnum, bool recursive, int flags);
struct obj_data* obj_contents_search_type(struct obj_data *obj, int type, bool recursive, int flags);
struct obj_data* obj_inventory_search_vnum(struct obj_data *obj, obj_vnum vnum, bool recursive, int flags);
struct obj_data* obj_inventory_search_type(struct obj_data *obj, int type, bool recursive, int flags);




// Below this is global variables and database functions
extern struct obj_data *object_list;
extern long max_obj_id;

obj_rnum real_object(obj_vnum vnum);
struct obj_data *obj_proto_by_id(obj_vnum vnum);

void* obj_proto_iterator_create();
struct obj_data* obj_proto_next(void* iterator);
void obj_proto_iterator_free(void* iterator);

struct obj_data* obj_proto_get(obj_vnum vnum);
size_t obj_proto_count();
void obj_proto_put(obj_vnum vnum, struct obj_data *obj);
void obj_proto_delete(obj_vnum vnum);
SpecialFunc obj_proto_special_get(obj_vnum vnum);
void obj_proto_special_set(obj_vnum vnum, SpecialFunc func);
void obj_proto_count_increment(obj_vnum vnum);
size_t obj_proto_count_get(obj_vnum vnum);
void obj_proto_count_decrement(obj_vnum vnum);

struct obj_data *obj_by_id(int64_t id);
int obj_register_id(int64_t id, struct obj_data *obj);
void obj_unregister_id(int64_t id);
int obj_subscribe(int64_t id, const char *list_name);
void obj_unsubscribe(int64_t id, const char *list_name);
void obj_clear_subscriptions(int64_t id);
void obj_for_each(const char *list_name, void (*func)(struct obj_data *obj));

#ifdef __cplusplus
}
#endif
