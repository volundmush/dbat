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
obj_vnum obj_get_vnum(const struct obj_data *obj);

// =============================================================================
// Misc
// =============================================================================

bool obj_aff_flagged(const struct obj_data *obj, int flag);
bool obj_val_flagged(const struct obj_data *obj, bitvector_t flag);
bool obj_extra_flagged(const struct obj_data *obj, int flag);
void obj_extra_set(struct obj_data *obj, int flag, bool on);
bool obj_extra_toggle(struct obj_data *obj, int flag);
void obj_aff_set(struct obj_data *obj, int flag, bool on);
bool obj_aff_toggle(struct obj_data *obj, int flag);
void obj_wear_set(struct obj_data *obj, int flag, bool on);
bool obj_wear_toggle(struct obj_data *obj, int flag);

bool obj_valid_rnum(const struct obj_data *obj);

int obj_get_level(const struct obj_data *obj);
void obj_set_level(struct obj_data *obj, int v);
bitvector_t *obj_get_bitvector(const struct obj_data *obj);
int8_t obj_get_type_flag(const struct obj_data *obj);
void obj_set_type_flag(struct obj_data *obj, int8_t v);
int obj_get_cost(const struct obj_data *obj);
void obj_set_cost(struct obj_data *obj, int v);
int obj_get_cost_per_day(const struct obj_data *obj);
void obj_set_cost_per_day(struct obj_data *obj, int v);
bitvector_t *obj_get_extra_flags(const struct obj_data *obj);
bitvector_t *obj_get_wear_flags(const struct obj_data *obj);
int obj_get_value(const struct obj_data *obj, int i);
void obj_set_value(struct obj_data *obj, int i, int v);
int64_t obj_get_weight(const struct obj_data *obj);
void obj_set_weight(struct obj_data *obj, int64_t v);
int obj_get_timer(const struct obj_data *obj);
void obj_set_timer(struct obj_data *obj, int v);
struct char_data *obj_get_sitting(const struct obj_data *obj);
void obj_set_sitting(struct obj_data *obj, struct char_data *v);
int obj_get_posttype(const struct obj_data *obj);
void obj_set_posttype(struct obj_data *obj, int v);
struct obj_data *obj_get_posted_to(const struct obj_data *obj);
void obj_set_posted_to(struct obj_data *obj, struct obj_data *v);
struct obj_data *obj_get_fellow_wall(const struct obj_data *obj);
void obj_set_fellow_wall(struct obj_data *obj, struct obj_data *v);
int32_t obj_get_aucter(const struct obj_data *obj);
void obj_set_aucter(struct obj_data *obj, int32_t v);
int32_t obj_get_curBidder(const struct obj_data *obj);
void obj_set_curBidder(struct obj_data *obj, int32_t v);
char *obj_get_auctname(const struct obj_data *obj);
void obj_set_auctname(struct obj_data *obj, char *v);
time_t obj_get_aucTime(const struct obj_data *obj);
void obj_set_aucTime(struct obj_data *obj, time_t v);
int obj_get_bid(const struct obj_data *obj);
void obj_set_bid(struct obj_data *obj, int v);
int obj_get_startbid(const struct obj_data *obj);
void obj_set_startbid(struct obj_data *obj, int v);
int obj_get_foob(const struct obj_data *obj);
void obj_set_foob(struct obj_data *obj, int v);
/* Below is used for "homing" ki attacks */
struct char_data *obj_get_target(const struct obj_data *obj);
void obj_set_target(struct obj_data *obj, struct char_data *v);
int64_t obj_get_kicharge(const struct obj_data *obj);
void obj_set_kicharge(struct obj_data *obj, int64_t v);
int obj_get_kitype(const struct obj_data *obj);
void obj_set_kitype(struct obj_data *obj, int v);
struct char_data *obj_get_user(const struct obj_data *obj);
void obj_set_user(struct obj_data *obj, struct char_data *v);
int obj_get_distance(const struct obj_data *obj);
void obj_set_distance(struct obj_data *obj, int v);
/* Above is used for "homing ki attacks */
int obj_get_scoutfreq(const struct obj_data *obj);
void obj_set_scoutfreq(struct obj_data *obj, int v);
int obj_get_healcharge(const struct obj_data *obj);
void obj_set_healcharge(struct obj_data *obj, int v);
time_t obj_get_lload(const struct obj_data *obj);
void obj_set_lload(struct obj_data *obj, time_t v);
int obj_get_size(const struct obj_data *obj);
void obj_set_size(struct obj_data *obj, int v);
obj_vnum obj_get_item_number(const struct obj_data *obj);
void obj_set_item_number(struct obj_data *obj, obj_vnum v);
spec_proc_func obj_get_spec(const struct obj_data *obj);

bool obj_is_corpse(const struct obj_data *obj);

bool obj_wear_flagged(const struct obj_data *obj, int part);
char *obj_get_short_description(const struct obj_data *obj);

char *obj_sana(const struct obj_data *obj);
room_vnum obj_get_room_loaded(const struct obj_data *obj);
void obj_set_room_loaded(struct obj_data *obj, room_vnum v);

#include "dbat/objects.h"

int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
int is_better(struct obj_data *object, struct obj_data *object2);

int32_t obj_get_id(const struct obj_data *obj);
void obj_set_id(struct obj_data *obj, int32_t v);
room_rnum obj_get_in_room(const struct obj_data *obj);
void obj_set_in_room(struct obj_data *obj, room_rnum v);
struct room_direction_data *obj_get_exit(const struct obj_data *obj, int door);
const char *money_desc(int amount);
struct obj_data *create_money(int amount);
