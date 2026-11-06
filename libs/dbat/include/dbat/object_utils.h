#pragma once
#include "consts/types.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "flags.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Object Classification
// =============================================================================

obj_vnum obj_get_vnum(GameWorld *g, const struct obj_data *obj);

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
spec_proc_func obj_get_spec(GameWorld *g, const struct obj_data *obj);

bool obj_is_corpse(const struct obj_data *obj);

bool obj_wear_flagged(const struct obj_data *obj, int part);
char *obj_get_short_description(const struct obj_data *obj);

const char *obj_sana(const struct obj_data *obj);
room_vnum obj_get_room_loaded(const struct obj_data *obj);
void obj_set_room_loaded(struct obj_data *obj, room_vnum v);

#include "dbat/objects.h"

bool wearable_obj(struct obj_data *obj);
void randomize_eq(GameWorld *g, struct obj_data *obj);
bool is_better(struct obj_data *object, struct obj_data *object2);

int32_t obj_get_id(const struct obj_data *obj);
void obj_set_id(struct obj_data *obj, int32_t v);
room_rnum obj_get_in_room(const struct obj_data *obj);
void obj_set_in_room(struct obj_data *obj, room_rnum v);
struct room_direction_data *obj_get_exit(GameWorld *g, const struct obj_data *obj, int door);
const char *money_desc(int amount);
struct obj_data *create_money(GameWorld *g, int amount);

/* A field's address, for code that fills the field in later or unlinks through it */
struct obj_data **obj_contains_slot(struct obj_data *obj);
struct extra_descr_data **obj_ex_description_slot(struct obj_data *obj);
struct script_data **obj_script_slot(struct obj_data *obj);
char **obj_action_description_slot(struct obj_data *obj);
struct obj_spellbook_spell **obj_sbinfo_slot(struct obj_data *obj);

/* Removes obj from a list linked through next_content: a room's contents, a container or an inventory. */
void obj_unlink_content(struct obj_data **head, struct obj_data *obj);
/* Removes obj from g->objects. */
void obj_unlink_from_list(GameWorld *g, struct obj_data *obj);

/* Field accessors */
char *obj_get_action_description(const struct obj_data *obj);
void obj_set_action_description(struct obj_data *obj, char *v);
struct obj_affected_type *obj_get_affected(struct obj_data *obj);
struct char_data *obj_get_carried_by(const struct obj_data *obj);
void obj_set_carried_by(struct obj_data *obj, struct char_data *v);
struct obj_data *obj_get_contains(const struct obj_data *obj);
void obj_set_contains(struct obj_data *obj, struct obj_data *v);
char *obj_get_description(const struct obj_data *obj);
void obj_set_description(struct obj_data *obj, char *v);
struct extra_descr_data *obj_get_ex_description(const struct obj_data *obj);
void obj_set_ex_description(struct obj_data *obj, struct extra_descr_data *v);
time_t obj_get_generation(const struct obj_data *obj);
void obj_set_generation(struct obj_data *obj, time_t v);
struct obj_data *obj_get_in_obj(const struct obj_data *obj);
void obj_set_in_obj(struct obj_data *obj, struct obj_data *v);
char *obj_get_name(const struct obj_data *obj);
void obj_set_name(struct obj_data *obj, char *v);
struct obj_data *obj_get_next(const struct obj_data *obj);
void obj_set_next(struct obj_data *obj, struct obj_data *v);
struct obj_data *obj_get_next_content(const struct obj_data *obj);
void obj_set_next_content(struct obj_data *obj, struct obj_data *v);
struct trig_proto_list *obj_get_proto_script(const struct obj_data *obj);
void obj_set_proto_script(struct obj_data *obj, struct trig_proto_list *v);
struct obj_spellbook_spell *obj_get_sbinfo(const struct obj_data *obj);
struct script_data *obj_get_script(const struct obj_data *obj);
void obj_set_script(struct obj_data *obj, struct script_data *v);
void obj_set_short_description(struct obj_data *obj, char *v);
int64_t obj_get_unique_id(const struct obj_data *obj);
void obj_set_unique_id(struct obj_data *obj, int64_t v);
struct char_data *obj_get_worn_by(const struct obj_data *obj);
void obj_set_worn_by(struct obj_data *obj, struct char_data *v);
int16_t obj_get_worn_on(const struct obj_data *obj);
void obj_set_worn_on(struct obj_data *obj, int16_t v);

#ifdef __cplusplus
}
#endif
