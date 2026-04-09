#include "dbat/db/object_utils.h"
#include "dbat/db/objects.h"
#include "dbat/db/consts/maximums.h"
#include "dbat/db/consts/itemdata.h"
#include <stdlib.h>
#include <string.h>

bool obj_is_valid(const struct obj_data *obj) {
    return obj != NULL;
}

obj_rnum obj_get_rnum(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOTHING;
    return obj->item_number;
}

obj_vnum obj_get_vnum(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOTHING;
    if (obj->item_number < 0 || obj->item_number > top_of_objt) return NOTHING;
    return obj_index[obj->item_number].vnum;
}

const char *obj_get_name(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->name ? obj->name : "";
}

const char *obj_get_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->description ? obj->description : "";
}

const char *obj_get_short_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->short_description ? obj->short_description : "";
}

const char *obj_get_action_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->action_description ? obj->action_description : "";
}

void obj_set_name(struct obj_data *obj, const char *name) {
    if (!obj_is_valid(obj) || !name) return;
    if (obj->name) free(obj->name);
    obj->name = strdup(name);
}

void obj_set_description(struct obj_data *obj, const char *desc) {
    if (!obj_is_valid(obj) || !desc) return;
    if (obj->description) free(obj->description);
    obj->description = strdup(desc);
}

void obj_set_short_description(struct obj_data *obj, const char *short_desc) {
    if (!obj_is_valid(obj) || !short_desc) return;
    if (obj->short_description) free(obj->short_description);
    obj->short_description = strdup(short_desc);
}

void obj_set_action_description(struct obj_data *obj, const char *action_desc) {
    if (!obj_is_valid(obj)) return;
    if (obj->action_description) free(obj->action_description);
    obj->action_description = action_desc ? strdup(action_desc) : NULL;
}

int obj_get_type(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->type_flag;
}

void obj_set_type(struct obj_data *obj, int8_t type) {
    if (!obj_is_valid(obj)) return;
    obj->type_flag = type;
}

int obj_get_level(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->level;
}

void obj_set_level(struct obj_data *obj, int level) {
    if (!obj_is_valid(obj)) return;
    obj->level = level;
}

int obj_get_size(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->size;
}

void obj_set_size(struct obj_data *obj, int size) {
    if (!obj_is_valid(obj)) return;
    obj->size = size;
}

room_rnum obj_get_in_room(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOWHERE;
    return obj->in_room;
}

void obj_set_in_room(struct obj_data *obj, room_rnum room) {
    if (!obj_is_valid(obj)) return;
    obj->in_room = room;
}

room_vnum obj_get_room_loaded(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NOWHERE;
    return obj->room_loaded;
}

void obj_set_room_loaded(struct obj_data *obj, room_vnum vnum) {
    if (!obj_is_valid(obj)) return;
    obj->room_loaded = vnum;
}

struct char_data *obj_get_carried_by(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->carried_by;
}

void obj_set_carried_by(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->carried_by = ch;
}

struct char_data *obj_get_worn_by(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->worn_by;
}

void obj_set_worn_by(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->worn_by = ch;
}

int obj_get_worn_on(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->worn_on;
}

void obj_set_worn_on(struct obj_data *obj, int16_t pos) {
    if (!obj_is_valid(obj)) return;
    obj->worn_on = pos;
}

struct obj_data *obj_get_in_obj(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->in_obj;
}

void obj_set_in_obj(struct obj_data *obj, struct obj_data *container) {
    if (!obj_is_valid(obj)) return;
    obj->in_obj = container;
}

struct obj_data *obj_get_contains(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->contains;
}

void obj_set_contains(struct obj_data *obj, struct obj_data *contents) {
    if (!obj_is_valid(obj)) return;
    obj->contains = contents;
}

struct obj_data *obj_get_next_content(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->next_content;
}

void obj_set_next_content(struct obj_data *obj, struct obj_data *next) {
    if (!obj_is_valid(obj)) return;
    obj->next_content = next;
}

struct obj_data *obj_get_next(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->next;
}

void obj_set_next(struct obj_data *obj, struct obj_data *next) {
    if (!obj_is_valid(obj)) return;
    obj->next = next;
}

int obj_get_value(const struct obj_data *obj, int index) {
    if (!obj_is_valid(obj) || index < 0 || index >= NUM_OBJ_VAL_POSITIONS) return 0;
    return obj->value[index];
}

void obj_set_value(struct obj_data *obj, int index, int value) {
    if (!obj_is_valid(obj) || index < 0 || index >= NUM_OBJ_VAL_POSITIONS) return;
    obj->value[index] = value;
}

int64_t obj_get_weight(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->weight;
}

void obj_set_weight(struct obj_data *obj, int64_t weight) {
    if (!obj_is_valid(obj)) return;
    obj->weight = weight;
}

int obj_get_cost(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->cost;
}

void obj_set_cost(struct obj_data *obj, int cost) {
    if (!obj_is_valid(obj)) return;
    obj->cost = cost;
}

int obj_get_rent_cost(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->cost_per_day;
}

void obj_set_rent_cost(struct obj_data *obj, int cost_per_day) {
    if (!obj_is_valid(obj)) return;
    obj->cost_per_day = cost_per_day;
}

int obj_get_timer(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->timer;
}

void obj_set_timer(struct obj_data *obj, int timer) {
    if (!obj_is_valid(obj)) return;
    obj->timer = timer;
}

struct extra_descr_data *obj_get_ex_description(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->ex_description;
}

void obj_set_ex_description(struct obj_data *obj, struct extra_descr_data *ex_desc) {
    if (!obj_is_valid(obj)) return;
    obj->ex_description = ex_desc;
}

const struct obj_affected_type *obj_get_affected(const struct obj_data *obj, int index) {
    if (!obj_is_valid(obj) || index < 0 || index >= MAX_OBJ_AFFECT) return NULL;
    return &obj->affected[index];
}

void obj_set_affected_modifier(struct obj_data *obj, int index, int8_t location, int16_t modifier, int skill) {
    if (!obj_is_valid(obj) || index < 0 || index >= MAX_OBJ_AFFECT) return;
    obj->affected[index].location = location;
    obj->affected[index].specific = skill;
    obj->affected[index].modifier = modifier;
}

bool obj_extra_flagged(const struct obj_data *obj, int flag) {
    if (!obj_is_valid(obj)) return false;
    return (obj->extra_flags[0] & flag) != 0;
}

bool obj_wear_flagged(const struct obj_data *obj, int flag) {
    if (!obj_is_valid(obj)) return false;
    return (obj->wear_flags[0] & flag) != 0;
}

bool obj_affect_flagged(const struct obj_data *obj, int flag) {
    if (!obj_is_valid(obj)) return false;
    return (obj->bitvector[0] & flag) != 0;
}

void obj_set_extra_flag(struct obj_data *obj, int flag, bool value) {
    if (!obj_is_valid(obj)) return;
    if (value) {
        obj->extra_flags[0] |= flag;
    } else {
        obj->extra_flags[0] &= ~flag;
    }
}

void obj_set_wear_flag(struct obj_data *obj, int flag, bool value) {
    if (!obj_is_valid(obj)) return;
    if (value) {
        obj->wear_flags[0] |= flag;
    } else {
        obj->wear_flags[0] &= ~flag;
    }
}

void obj_set_affect_flag(struct obj_data *obj, int flag, bool value) {
    if (!obj_is_valid(obj)) return;
    if (value) {
        obj->bitvector[0] |= flag;
    } else {
        obj->bitvector[0] &= ~flag;
    }
}

int32_t obj_get_id(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->id;
}

void obj_set_id(struct obj_data *obj, int32_t id) {
    if (!obj_is_valid(obj)) return;
    obj->id = id;
}

time_t obj_get_generation(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->generation;
}

void obj_set_generation(struct obj_data *obj, time_t gen) {
    if (!obj_is_valid(obj)) return;
    obj->generation = gen;
}

int64_t obj_get_unique_id(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->unique_id;
}

void obj_set_unique_id(struct obj_data *obj, int64_t id) {
    if (!obj_is_valid(obj)) return;
    obj->unique_id = id;
}

int obj_get_scoutfreq(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->scoutfreq;
}

void obj_set_scoutfreq(struct obj_data *obj, int freq) {
    if (!obj_is_valid(obj)) return;
    obj->scoutfreq = freq;
}

time_t obj_get_last_load(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->lload;
}

void obj_set_last_load(struct obj_data *obj, time_t t) {
    if (!obj_is_valid(obj)) return;
    obj->lload = t;
}

int obj_get_healcharge(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->healcharge;
}

void obj_set_healcharge(struct obj_data *obj, int charge) {
    if (!obj_is_valid(obj)) return;
    obj->healcharge = charge;
}

int64_t obj_get_kicharge(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->kicharge;
}

void obj_set_kicharge(struct obj_data *obj, int64_t charge) {
    if (!obj_is_valid(obj)) return;
    obj->kicharge = charge;
}

int obj_get_kitype(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->kitype;
}

void obj_set_kitype(struct obj_data *obj, int type) {
    if (!obj_is_valid(obj)) return;
    obj->kitype = type;
}

struct char_data *obj_get_user(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->user;
}

void obj_set_user(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->user = ch;
}

struct char_data *obj_get_target(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->target;
}

void obj_set_target(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->target = ch;
}

int obj_get_distance(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->distance;
}

void obj_set_distance(struct obj_data *obj, int dist) {
    if (!obj_is_valid(obj)) return;
    obj->distance = dist;
}

int32_t obj_get_aucter(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->aucter;
}

void obj_set_aucter(struct obj_data *obj, int32_t id) {
    if (!obj_is_valid(obj)) return;
    obj->aucter = id;
}

int32_t obj_get_cur_bidder(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->curBidder;
}

void obj_set_cur_bidder(struct obj_data *obj, int32_t id) {
    if (!obj_is_valid(obj)) return;
    obj->curBidder = id;
}

time_t obj_get_auc_time(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->aucTime;
}

void obj_set_auc_time(struct obj_data *obj, time_t t) {
    if (!obj_is_valid(obj)) return;
    obj->aucTime = t;
}

int obj_get_bid(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->bid;
}

void obj_set_bid(struct obj_data *obj, int bid) {
    if (!obj_is_valid(obj)) return;
    obj->bid = bid;
}

int obj_get_start_bid(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->startbid;
}

void obj_set_start_bid(struct obj_data *obj, int bid) {
    if (!obj_is_valid(obj)) return;
    obj->startbid = bid;
}

const char *obj_get_auc_tname(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->auctname ? obj->auctname : "";
}

void obj_set_auc_tname(struct obj_data *obj, const char *name) {
    if (!obj_is_valid(obj)) return;
    if (obj->auctname) free(obj->auctname);
    obj->auctname = name ? strdup(name) : NULL;
}

int obj_get_post_type(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->posttype;
}

void obj_set_post_type(struct obj_data *obj, int type) {
    if (!obj_is_valid(obj)) return;
    obj->posttype = type;
}

struct obj_data *obj_get_posted_to(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->posted_to;
}

void obj_set_posted_to(struct obj_data *obj, struct obj_data *board) {
    if (!obj_is_valid(obj)) return;
    obj->posted_to = board;
}

int obj_get_foob(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return 0;
    return obj->foob;
}

void obj_set_foob(struct obj_data *obj, int val) {
    if (!obj_is_valid(obj)) return;
    obj->foob = val;
}

struct char_data *obj_get_sitting(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->sitting;
}

void obj_set_sitting(struct obj_data *obj, struct char_data *ch) {
    if (!obj_is_valid(obj)) return;
    obj->sitting = ch;
}

struct obj_data *obj_get_fellow_wall(const struct obj_data *obj) {
    if (!obj_is_valid(obj)) return NULL;
    return obj->fellow_wall;
}

void obj_set_fellow_wall(struct obj_data *obj, struct obj_data *fellow) {
    if (!obj_is_valid(obj)) return;
    obj->fellow_wall = fellow;
}
