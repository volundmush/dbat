#pragma once
#include "consts/types.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "flags.h"

// =============================================================================
// Object Classification
// =============================================================================

bool obj_is_valid(const struct obj_data *obj);
obj_rnum obj_get_rnum(const struct obj_data *obj);
obj_vnum obj_get_vnum(const struct obj_data *obj);

// =============================================================================
// Identity & Names
// =============================================================================

const char *obj_get_name(const struct obj_data *obj);
const char *obj_get_description(const struct obj_data *obj);
const char *obj_get_short_description(const struct obj_data *obj);
const char *obj_get_action_description(const struct obj_data *obj);

void obj_set_name(struct obj_data *obj, const char *name);
void obj_set_description(struct obj_data *obj, const char *desc);
void obj_set_short_description(struct obj_data *obj, const char *short_desc);
void obj_set_action_description(struct obj_data *obj, const char *action_desc);

// =============================================================================
// Object Type & Level
// =============================================================================

int obj_get_type(const struct obj_data *obj);
void obj_set_type(struct obj_data *obj, int8_t type);

int obj_get_level(const struct obj_data *obj);
void obj_set_level(struct obj_data *obj, int level);

int obj_get_size(const struct obj_data *obj);
void obj_set_size(struct obj_data *obj, int size);

// =============================================================================
// Location
// =============================================================================

room_rnum obj_get_in_room(const struct obj_data *obj);
void obj_set_in_room(struct obj_data *obj, room_rnum room);

room_vnum obj_get_room_loaded(const struct obj_data *obj);
void obj_set_room_loaded(struct obj_data *obj, room_vnum vnum);

// =============================================================================
// Carried By / Worn By
// =============================================================================

struct char_data *obj_get_carried_by(const struct obj_data *obj);
void obj_set_carried_by(struct obj_data *obj, struct char_data *ch);

struct char_data *obj_get_worn_by(const struct obj_data *obj);
void obj_set_worn_by(struct obj_data *obj, struct char_data *ch);

int obj_get_worn_on(const struct obj_data *obj);
void obj_set_worn_on(struct obj_data *obj, int16_t pos);

// =============================================================================
// Container Hierarchy
// =============================================================================

struct obj_data *obj_get_in_obj(const struct obj_data *obj);
void obj_set_in_obj(struct obj_data *obj, struct obj_data *container);

struct obj_data *obj_get_contains(const struct obj_data *obj);
void obj_set_contains(struct obj_data *obj, struct obj_data *contents);

// =============================================================================
// List Links
// =============================================================================

struct obj_data *obj_get_next_content(const struct obj_data *obj);
void obj_set_next_content(struct obj_data *obj, struct obj_data *next);

struct obj_data *obj_get_next(const struct obj_data *obj);
void obj_set_next(struct obj_data *obj, struct obj_data *next);

// =============================================================================
// Values
// =============================================================================

int obj_get_value(const struct obj_data *obj, int index);
void obj_set_value(struct obj_data *obj, int index, int value);

// =============================================================================
// Weight & Cost
// =============================================================================

int64_t obj_get_weight(const struct obj_data *obj);
void obj_set_weight(struct obj_data *obj, int64_t weight);

int obj_get_cost(const struct obj_data *obj);
void obj_set_cost(struct obj_data *obj, int cost);

int obj_get_rent_cost(const struct obj_data *obj);
void obj_set_rent_cost(struct obj_data *obj, int cost_per_day);

// =============================================================================
// Timer
// =============================================================================

int obj_get_timer(const struct obj_data *obj);
void obj_set_timer(struct obj_data *obj, int timer);

// =============================================================================
// Extra Descriptions
// =============================================================================

struct extra_descr_data *obj_get_ex_description(const struct obj_data *obj);
void obj_set_ex_description(struct obj_data *obj, struct extra_descr_data *ex_desc);

// =============================================================================
// Affected
// =============================================================================

const struct obj_affected_type *obj_get_affected(const struct obj_data *obj, int index);
void obj_set_affected_modifier(struct obj_data *obj, int index, int8_t location, int16_t modifier, int skill);

// =============================================================================
// Flags
// =============================================================================

bool obj_extra_flagged(const struct obj_data *obj, int flag);
bool obj_wear_flagged(const struct obj_data *obj, int flag);
bool obj_affect_flagged(const struct obj_data *obj, int flag);

void obj_set_extra_flag(struct obj_data *obj, int flag, bool value);
void obj_set_wear_flag(struct obj_data *obj, int flag, bool value);
void obj_set_affect_flag(struct obj_data *obj, int flag, bool value);

// =============================================================================
// IDs
// =============================================================================

int32_t obj_get_id(const struct obj_data *obj);
void obj_set_id(struct obj_data *obj, int32_t id);

time_t obj_get_generation(const struct obj_data *obj);
void obj_set_generation(struct obj_data *obj, time_t gen);

int64_t obj_get_unique_id(const struct obj_data *obj);
void obj_set_unique_id(struct obj_data *obj, int64_t id);

// =============================================================================
// Ki Attacks (projectiles, etc.)
// =============================================================================

int obj_get_scoutfreq(const struct obj_data *obj);
void obj_set_scoutfreq(struct obj_data *obj, int freq);

time_t obj_get_last_load(const struct obj_data *obj);
void obj_set_last_load(struct obj_data *obj, time_t t);

int obj_get_healcharge(const struct obj_data *obj);
void obj_set_healcharge(struct obj_data *obj, int charge);

int64_t obj_get_kicharge(const struct obj_data *obj);
void obj_set_kicharge(struct obj_data *obj, int64_t charge);

int obj_get_kitype(const struct obj_data *obj);
void obj_set_kitype(struct obj_data *obj, int type);

struct char_data *obj_get_user(const struct obj_data *obj);
void obj_set_user(struct obj_data *obj, struct char_data *ch);

struct char_data *obj_get_target(const struct obj_data *obj);
void obj_set_target(struct obj_data *obj, struct char_data *ch);

int obj_get_distance(const struct obj_data *obj);
void obj_set_distance(struct obj_data *obj, int dist);

// =============================================================================
// Auction
// =============================================================================

int32_t obj_get_aucter(const struct obj_data *obj);
void obj_set_aucter(struct obj_data *obj, int32_t id);

int32_t obj_get_cur_bidder(const struct obj_data *obj);
void obj_set_cur_bidder(struct obj_data *obj, int32_t id);

time_t obj_get_auc_time(const struct obj_data *obj);
void obj_set_auc_time(struct obj_data *obj, time_t t);

int obj_get_bid(const struct obj_data *obj);
void obj_set_bid(struct obj_data *obj, int bid);

int obj_get_start_bid(const struct obj_data *obj);
void obj_set_start_bid(struct obj_data *obj, int bid);

const char *obj_get_auc_tname(const struct obj_data *obj);
void obj_set_auc_tname(struct obj_data *obj, const char *name);

// =============================================================================
// Board/Posting
// =============================================================================

int obj_get_post_type(const struct obj_data *obj);
void obj_set_post_type(struct obj_data *obj, int type);

struct obj_data *obj_get_posted_to(const struct obj_data *obj);
void obj_set_posted_to(struct obj_data *obj, struct obj_data *board);

// =============================================================================
// Misc
// =============================================================================

int obj_get_foob(const struct obj_data *obj);
void obj_set_foob(struct obj_data *obj, int val);

struct char_data *obj_get_sitting(const struct obj_data *obj);
void obj_set_sitting(struct obj_data *obj, struct char_data *ch);

struct obj_data *obj_get_fellow_wall(const struct obj_data *obj);
void obj_set_fellow_wall(struct obj_data *obj, struct obj_data *fellow);



bool obj_aff_flagged(const struct obj_data *obj, int flag);
bool obj_val_flagged(const struct obj_data *obj, bitvector_t flag);
bool obj_flagged(const struct obj_data *obj, int flag);

bool obj_valid_rnum(const struct obj_data *obj);

#define GET_OBJ_LEVEL(obj)      ((obj)->level)
#define GET_OBJ_PERM(obj)       ((obj)->bitvector)
#define GET_OBJ_TYPE(obj)	((obj)->type_flag)
#define GET_OBJ_COST(obj)	((obj)->cost)
#define GET_OBJ_RENT(obj)	((obj)->cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->extra_flags)
#define GET_OBJ_EXTRA_AR(obj, i)   ((obj)->extra_flags[(i)])
#define GET_OBJ_WEAR(obj)	((obj)->wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->weight)
#define GET_OBJ_TIMER(obj)	((obj)->timer)
#define SITTING(obj)            ((obj)->sitting)
#define GET_OBJ_POSTTYPE(obj)   ((obj)->posttype)
#define GET_OBJ_POSTED(obj)     ((obj)->posted_to)
#define GET_FELLOW_WALL(obj)    ((obj)->fellow_wall)
#define GET_AUCTER(obj)         ((obj)->aucter)
#define GET_CURBID(obj)         ((obj)->curBidder)
#define GET_AUCTERN(obj)        ((obj)->auctname)
#define GET_AUCTIME(obj)        ((obj)->aucTime)
#define GET_BID(obj)            ((obj)->bid)
#define GET_STARTBID(obj)       ((obj)->startbid)
#define FOOB(obj)               ((obj)->foob)
/* Below is used for "homing" ki attacks */
#define TARGET(obj)             ((obj)->target)
#define KICHARGE(obj)           ((obj)->kicharge)
#define KITYPE(obj)             ((obj)->kitype)
#define USER(obj)               ((obj)->user)
#define KIDIST(obj)             ((obj)->distance)
/* Above is used for "homing ki attacks */
#define SFREQ(obj)              ((obj)->scoutfreq)
#define HCHARGE(obj)            ((obj)->healcharge)
#define GET_LAST_LOAD(obj)      ((obj)->lload)
#define GET_OBJ_SIZE(obj)	((obj)->size)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
spec_proc_func obj_get_spec(const struct obj_data *obj);
#define GET_FUEL(obj)           (GET_OBJ_VAL((obj), 2))
#define GET_FUELCOUNT(obj)      (GET_OBJ_VAL((obj), 3))

bool obj_is_corpse(const struct obj_data *obj);

bool obj_can_wear(const struct obj_data *obj, int part);
#define GET_OBJ_MATERIAL(obj)   ((obj)->value[7])
#define GET_OBJ_SHORT(obj)	((obj)->short_description)


char *obj_sana(const struct obj_data *obj);
#define OBJ_LOADROOM(obj)     ((obj)->room_loaded)

#include "dbat/objects.h"


int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
int is_better(struct obj_data *object, struct obj_data *object2);
