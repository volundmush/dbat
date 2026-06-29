#pragma once
#include "consts/types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Object API functions, implemented in objects_api.zig
obj_vnum obj_proto_self_id_get(struct obj_proto_data *proto);
void obj_proto_self_id_set(struct obj_proto_data *proto, obj_vnum id);
int64_t obj_id_get(struct obj_data *obj);
void obj_id_set(struct obj_data *obj, int64_t id);
obj_vnum obj_proto_id_get(struct obj_data *obj);
void obj_proto_id_set(struct obj_data *obj, obj_vnum vnum);
obj_vnum obj_vnum_get(struct obj_data *obj);
void obj_vnum_set(struct obj_data *obj, obj_vnum vnum);
struct room_data *obj_room_get(struct obj_data *obj);
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
void obj_broken_set(struct obj_data *obj, bool value);
bool obj_aff_flagged(struct obj_data *obj, int pos);
bool obj_aff_flag_toggle(struct obj_data *obj, int pos);
void obj_aff_flag_set(struct obj_data *obj, int pos, bool value);
int64_t obj_weight_get(struct obj_data *obj);
int64_t obj_weight_get_contained(
    struct obj_data *obj); // weight of contained objects only
int64_t obj_weight_get_total(
    struct obj_data *obj); // this includes the weight of contained objects
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
const char *obj_name_get(struct obj_data *obj);
void obj_name_set(struct obj_data *obj, const char *value);
const char *obj_description_get(struct obj_data *obj);
void obj_description_set(struct obj_data *obj, const char *value);
const char *obj_short_description_get(struct obj_data *obj);
void obj_short_description_set(struct obj_data *obj, const char *value);
const char *obj_action_description_get(struct obj_data *obj);
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
int64_t *obj_inventory_get(struct obj_data *obj, size_t *count);

bool obj_search_vnum_match(struct obj_data *obj, void *ctx);
bool obj_search_type_match(struct obj_data *obj, void *ctx);

struct obj_data* obj_contains_get(struct obj_data *obj);
struct obj_data* obj_next_content_get(struct obj_data *obj);

void obj_contents_list_iterate(struct obj_data *obj, bool recursive,
                               obj_iter_fn func, void *ctx);
void obj_inventory_iterate(struct obj_data *obj, bool recursive,
                           obj_iter_fn func, void *ctx);

struct obj_data *obj_contents_search_vnum(struct obj_data *obj, obj_vnum vnum,
                                          bool recursive, int flags);
struct obj_data *obj_contents_search_type(struct obj_data *obj, int type,
                                          bool recursive, int flags);

/* Object placement helpers — from_obj/to_obj for container movement */
void obj_from_container(struct obj_data *obj);
void obj_to_container(struct obj_data *obj, struct obj_data *container);

/* Show object action description to a character (wraps show_obj_to_char SHOW_OBJ_ACTION) */
void obj_show_action_to_char(struct obj_data *obj, struct char_data *ch);

#ifdef __cplusplus
}
#endif
