#pragma once
#include "consts/types.h"
#include "character_db.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Characters API, implemented in characters_api.zig
int64_t char_id_get(struct char_data *ch);
void char_id_set(struct char_data *ch, int64_t id);
mob_vnum char_proto_id_get(struct char_data *ch);
void char_proto_id_set(struct char_data *ch, mob_vnum vnum);
mob_vnum char_vnum_get(struct char_data *ch);
void char_vnum_set(struct char_data *ch, mob_vnum vnum);
struct room_data *char_room_get(struct char_data *ch);
struct zone_data *char_zone_get(struct char_data *ch);
zone_vnum char_zone_vnum_get(struct char_data *ch);
room_vnum char_room_vnum_get(struct char_data *ch);
void char_room_vnum_set(struct char_data *ch, room_vnum vnum);
const char *char_name_get(struct char_data *ch);
void char_name_set(struct char_data *ch, const char *value);
const char *char_description_get(struct char_data *ch);
void char_description_set(struct char_data *ch, const char *value);
const char *char_short_description_get(struct char_data *ch);
void char_short_description_set(struct char_data *ch, const char *value);
const char *char_long_description_get(struct char_data *ch);
void char_long_description_set(struct char_data *ch, const char *value);
const char *char_title_get(struct char_data *ch);
void char_title_set(struct char_data *ch, const char *value);

bool char_is_npc(struct char_data *ch);

struct obj_data* char_carrying_get(struct char_data *ch);
struct char_data* char_next_in_room_get(struct char_data *ch);

int char_class_get(struct char_data *ch);
void char_class_set(struct char_data *ch, int chclass);
int char_race_get(struct char_data *ch);
void char_race_set(struct char_data *ch, int race);
int char_size_get(struct char_data *ch);
void char_size_set(struct char_data *ch, int size);
int char_sex_get(struct char_data *ch);
void char_sex_set(struct char_data *ch, int sex);
int char_admlevel_get(struct char_data *ch);
void char_admlevel_set(struct char_data *ch, int admlevel);
bool char_admflagged(struct char_data *ch, int pos);
bool char_admflag_toggle(struct char_data *ch, int pos);
void char_admflag_set(struct char_data *ch, int pos, bool value);
bool char_plrflagged(struct char_data *ch, int pos);
bool char_plrflag_toggle(struct char_data *ch, int pos);
void char_plrflag_set(struct char_data *ch, int pos, bool value);
bool char_prfflagged(struct char_data *ch, int pos);
bool char_prfflag_toggle(struct char_data *ch, int pos);
void char_prfflag_set(struct char_data *ch, int pos, bool value);
bool char_affflagged(struct char_data *ch, int pos);
bool char_bodyflagged(struct char_data *ch, int pos);
const char *char_user_get(struct char_data *ch); /* returns desc->user or NULL */

void char_inventory_iterate(struct char_data *ch, bool recursive,
                            obj_iter_fn func, void *ctx);
void char_equipment_iterate(struct char_data *ch, bool recursive,
                            obj_iter_fn func, void *ctx);
struct obj_data *char_inventory_search_vnum(struct char_data *ch, obj_vnum vnum,
                                            bool recursive, int flags);
struct obj_data *char_inventory_search_type(struct char_data *ch, int type,
                                            bool recursive, int flags);

size_t char_inventory_count(struct char_data *ch, bool recursive);
size_t char_equipment_count(struct char_data *ch, bool recursive);
int64_t* char_inventory_get(struct char_data *ch, size_t* count);
struct obj_data *char_equipment_get(struct char_data *ch, size_t pos);

void    char_clone_add(struct char_data *original, struct char_data *clone);
void    char_clone_remove(struct char_data *original, struct char_data *clone);
size_t  char_clone_count(struct char_data *original);
int64_t *char_clone_ids(struct char_data *original, size_t *out_count);
void    char_clone_ids_free(int64_t *ptr);

void char_send_text(struct char_data *ch, const char *text);
void char_send_textf(struct char_data *ch, const char *format, ...);
bool char_cmd_execute(struct char_data *ch, const char *command,
                      const char *arguments);

// Per-character command queue (managed by Zig).
void char_command_enqueue(struct char_data *ch, const char *cmd);
char *char_command_dequeue(struct char_data *ch);  // caller must free(); NULL if empty
bool char_command_has_pending(struct char_data *ch);
void char_command_clear(struct char_data *ch);

// Lua command dispatch.
bool char_pcommand_try(struct char_data *ch, const char *full_input);
bool char_command_try(struct char_data *ch, const char *cmd_word,
                      const char *arguments);

// Character API stuff that makes use of the new Lua API.
void char_zig_free(struct char_data *ch);
int64_t char_stat_get(struct char_data *ch, const char *stat);
int64_t char_stat_set(struct char_data *ch, const char *stat, int64_t value);
int64_t char_stat_mod(struct char_data *ch, const char *stat, int64_t mod);
void char_stats_copy_to_mob_proto(struct char_data *ch,
                                  struct mob_proto_data *proto);
void mob_proto_stats_copy_to_char(struct mob_proto_data *proto,
                                  struct char_data *ch);
mob_vnum mob_proto_id_get(struct mob_proto_data *proto);
void mob_proto_id_set(struct mob_proto_data *proto, mob_vnum id);
void mob_proto_zig_free(struct mob_proto_data *proto);
int64_t mob_proto_stat_get(struct mob_proto_data *proto, const char *stat);
int64_t mob_proto_stat_set(struct mob_proto_data *proto, const char *stat,
                           int64_t value);
int64_t mob_proto_stat_mod(struct mob_proto_data *proto, const char *stat,
                           int64_t mod);

int64_t char_legacy_modifier(struct char_data *ch, int location, int specific);

int64_t char_der_base_get(struct char_data *ch, const char *stat);
int64_t char_der_total_get(struct char_data *ch, const char *stat);
void char_der_invalidate(struct char_data *ch);
uint64_t char_modifier_gen_get(struct char_data *ch);

int64_t char_meter_get(struct char_data *ch, const char *meter);
int64_t char_meter_set(struct char_data *ch, const char *meter, int64_t value);
int64_t char_meter_set_int(struct char_data *ch, const char *meter,
                           int64_t value);
int64_t char_meter_mod(struct char_data *ch, const char *meter, int64_t mod);
int64_t char_meter_mod_int(struct char_data *ch, const char *meter,
                           int64_t mod);
int64_t char_meter_current(struct char_data *ch, const char *meter);
int64_t char_meter_max(struct char_data *ch, const char *meter);
bool char_meter_full(struct char_data *ch, const char *meter);
void char_meter_conditions_sync(struct char_data *ch);

int64_t char_skill_base_get(struct char_data *ch, const char *skill);
int64_t char_skill_base_set(struct char_data *ch, const char *skill,
                            int64_t value);
int64_t char_skill_base_mod(struct char_data *ch, const char *skill,
                            int64_t mod);
int64_t char_skill_modifier_get(struct char_data *ch, const char *skill);
int64_t char_skill_total_get(struct char_data *ch, const char *skill);
int64_t char_skill_perf_get(struct char_data *ch, const char *skill);
int64_t char_skill_perf_set(struct char_data *ch, const char *skill,
                            int64_t value);
int64_t char_skill_perf_mod(struct char_data *ch, const char *skill,
                            int64_t mod);

struct condition_number_arg {
  const char *key;
  int64_t value;
};

struct condition_string_arg {
  const char *key;
  const char *value;
};

bool char_condition_check_legacy_affect(struct char_data *ch, int aff_flag);
bool char_condition_has(struct char_data *ch, const char *condition);
bool char_condition_id_has_tag(const char *condition, const char *tag);
bool char_condition_has_tag(struct char_data *ch, const char *tag);
const char *char_condition_active_with_tag(struct char_data *ch,
                                           const char *tag);
bool char_condition_add(struct char_data *ch, const char *condition,
                        const char *source_category, const char *source_id);
bool char_condition_add_with_variables(
    struct char_data *ch, const char *condition, const char *source_category,
    const char *source_id, const struct condition_number_arg *numbers,
    size_t number_count, const struct condition_string_arg *strings,
    size_t string_count);
bool char_condition_add_silent(struct char_data *ch, const char *condition,
                               const char *source_category, const char *source_id);
void char_condition_notify_applied(struct char_data *ch, const char *condition);
void char_condition_event_dispatch(struct char_data *ch, const char *condition,
                                   const char *event_name);
void char_condition_game_activate(struct char_data *ch);
void char_condition_game_deactivate(struct char_data *ch);
bool char_condition_apply(struct char_data *ch, const char *condition,
                          const char *source_category, const char *source_id);
bool char_condition_apply_with_variables(
    struct char_data *ch, const char *condition, const char *source_category,
    const char *source_id, const struct condition_number_arg *numbers,
    size_t number_count, const struct condition_string_arg *strings,
    size_t string_count);
bool char_condition_apply_with_number(struct char_data *ch,
                                      const char *condition,
                                      const char *source_category,
                                      const char *source_id, const char *key,
                                      int64_t value);
bool char_condition_apply_with_string(struct char_data *ch,
                                      const char *condition,
                                      const char *source_category,
                                      const char *source_id, const char *key,
                                      const char *value);
bool char_condition_apply_with_duration(struct char_data *ch,
                                        const char *condition,
                                        const char *source_category,
                                        const char *source_id, int64_t duration);
bool char_condition_apply_with_numbers2(struct char_data *ch,
                                        const char *condition,
                                        const char *source_category,
                                        const char *source_id, const char *key1,
                                        int64_t value1, const char *key2,
                                        int64_t value2);
bool char_condition_remove(struct char_data *ch, const char *condition,
                           const char *reason);
int char_condition_remove_tag(struct char_data *ch, const char *tag,
                              const char *reason);
void char_condition_update(struct char_data *ch);
void char_condition_update_with_context(struct char_data *ch, const char *kind,
                                        int64_t pulses, int64_t seconds);
int64_t char_condition_stacks_get(struct char_data *ch, const char *condition);
int64_t char_condition_stacks_set(struct char_data *ch, const char *condition,
                                  int64_t value);
int64_t char_condition_duration_get(struct char_data *ch,
                                    const char *condition);
int64_t char_condition_duration_set(struct char_data *ch, const char *condition,
                                    int64_t value);
int64_t char_condition_number_get(struct char_data *ch, const char *condition,
                                  const char *key);
int64_t char_condition_number_set(struct char_data *ch, const char *condition,
                                  const char *key, int64_t value);
int64_t char_condition_number_mod(struct char_data *ch, const char *condition,
                                  const char *key, int64_t mod);
const char *char_condition_string_get(struct char_data *ch,
                                      const char *condition, const char *key);
bool char_condition_string_set(struct char_data *ch, const char *condition,
                               const char *key, const char *value);

bool char_transform_has(struct char_data *ch, const char *transform);
bool char_transform_add(struct char_data *ch, const char *transform);
bool char_transform_remove(struct char_data *ch, const char *transform);
bool char_transform_unlocked(struct char_data *ch, const char *transform);
bool char_transform_unlock(struct char_data *ch, const char *transform,
                           const char *source);
int64_t char_transform_number_get(struct char_data *ch, const char *transform,
                                  const char *key);
int64_t char_transform_number_set(struct char_data *ch, const char *transform,
                                  const char *key, int64_t value);
int64_t char_transform_number_mod(struct char_data *ch, const char *transform,
                                  const char *key, int64_t mod);
const char *char_transform_string_get(struct char_data *ch,
                                      const char *transform, const char *key);
bool char_transform_string_set(struct char_data *ch, const char *transform,
                               const char *key, const char *value);

bool char_is_extracted(struct char_data *ch);
bool char_is_outside(struct char_data *ch);
struct obj_data *char_sits_get(struct char_data *ch);
void char_sits_set(struct char_data *ch, struct obj_data *obj);

int64_t char_position_get(struct char_data *ch);
void char_position_set(struct char_data *ch, int64_t value);

void char_apply_entry_conditions(struct char_data *ch);

/* --- Relationship helpers: inline get/set for each inter-character pointer --- */
/* Each getter reads the condition's target_id and resolves via char_by_id.     */
/* Each setter removes any existing condition then applies a fresh one.          */

static inline struct char_data *char_fighting_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "fighting", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_fighting_set(struct char_data *ch, struct char_data *vict) {
    char_condition_remove(ch, "fighting", "update");
    if (vict) char_condition_apply_with_number(ch, "fighting", "combat", "engage", "target_id", char_id_get(vict));
}

static inline struct char_data *char_grappling_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "grappling", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_grappling_set(struct char_data *ch, struct char_data *vict, int grap_type) {
    char_condition_remove(ch, "grappling", "update");
    if (vict) char_condition_apply_with_numbers2(ch, "grappling", "combat", "grapple",
        "target_id", char_id_get(vict), "grap_type", (int64_t)grap_type);
}
static inline void char_graptype_set(struct char_data *ch, int val) {
    char_condition_number_set(ch, "grappling", "grap_type", (int64_t)val);
}

static inline struct char_data *char_grappled_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "grappled", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_grappled_set(struct char_data *ch, struct char_data *grappler, int grap_type) {
    char_condition_remove(ch, "grappled", "update");
    if (grappler) char_condition_apply_with_numbers2(ch, "grappled", "combat", "grapple",
        "target_id", char_id_get(grappler), "grap_type", (int64_t)grap_type);
}
static inline int char_graptype_get(struct char_data *ch) {
    int t = (int)char_condition_number_get(ch, "grappling", "grap_type");
    if (t) return t;
    return (int)char_condition_number_get(ch, "grappled", "grap_type");
}

static inline struct char_data *char_blocking_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "blocking", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_blocking_set(struct char_data *ch, struct char_data *vict) {
    char_condition_remove(ch, "blocking", "update");
    if (vict) char_condition_apply_with_number(ch, "blocking", "combat", "block", "target_id", char_id_get(vict));
}

static inline struct char_data *char_blocked_by_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "blocked_by", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_blocked_by_set(struct char_data *ch, struct char_data *blocker) {
    char_condition_remove(ch, "blocked_by", "update");
    if (blocker) char_condition_apply_with_number(ch, "blocked_by", "combat", "block", "target_id", char_id_get(blocker));
}

static inline struct char_data *char_absorbing_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "absorbing", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_absorbing_set(struct char_data *ch, struct char_data *vict) {
    char_condition_remove(ch, "absorbing", "update");
    if (vict) char_condition_apply_with_number(ch, "absorbing", "absorb", "absorb", "target_id", char_id_get(vict));
}

static inline struct char_data *char_absorbed_by_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "absorbed_by", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_absorbed_by_set(struct char_data *ch, struct char_data *absorber) {
    char_condition_remove(ch, "absorbed_by", "update");
    if (absorber) char_condition_apply_with_number(ch, "absorbed_by", "absorb", "absorb", "target_id", char_id_get(absorber));
}

static inline struct char_data *char_defending_for_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "defending_for", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_defending_for_set(struct char_data *ch, struct char_data *vict) {
    char_condition_remove(ch, "defending_for", "update");
    if (vict) char_condition_apply_with_number(ch, "defending_for", "combat", "defend", "target_id", char_id_get(vict));
}

static inline struct char_data *char_defended_by_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "defended_by", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_defended_by_set(struct char_data *ch, struct char_data *defender) {
    char_condition_remove(ch, "defended_by", "update");
    if (defender) char_condition_apply_with_number(ch, "defended_by", "combat", "defend", "target_id", char_id_get(defender));
}

static inline struct char_data *char_dragging_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "dragging", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_dragging_set(struct char_data *ch, struct char_data *vict) {
    char_condition_remove(ch, "dragging", "update");
    if (vict) char_condition_apply_with_number(ch, "dragging", "movement", "drag", "target_id", char_id_get(vict));
}

static inline struct char_data *char_being_dragged_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "being_dragged", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_being_dragged_set(struct char_data *ch, struct char_data *dragger) {
    char_condition_remove(ch, "being_dragged", "update");
    if (dragger) char_condition_apply_with_number(ch, "being_dragged", "movement", "drag", "target_id", char_id_get(dragger));
}

static inline struct char_data *char_mindlinked_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "mindlinked", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_mindlinked_set(struct char_data *ch, struct char_data *vict) {
    char_condition_remove(ch, "mindlinked", "update");
    if (vict) char_condition_apply_with_number(ch, "mindlinked", "ability", "mindlink", "target_id", char_id_get(vict));
}

static inline struct char_data *char_carrying_char_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "carrying_char", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_carrying_char_set(struct char_data *ch, struct char_data *carried) {
    char_condition_remove(ch, "carrying_char", "update");
    if (carried) char_condition_apply_with_number(ch, "carrying_char", "movement", "carry", "target_id", char_id_get(carried));
}

static inline struct char_data *char_carried_by_char_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "carried_by_char", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_carried_by_char_set(struct char_data *ch, struct char_data *carrier) {
    char_condition_remove(ch, "carried_by_char", "update");
    if (carrier) char_condition_apply_with_number(ch, "carried_by_char", "movement", "carry", "target_id", char_id_get(carrier));
}

static inline struct char_data *char_multiform_clone_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "multiform_clone", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_multiform_clone_set(struct char_data *ch, struct char_data *orig) {
    char_condition_remove(ch, "multiform_clone", "update");
    if (orig) char_condition_apply_with_number(ch, "multiform_clone", "multiform", "clone", "target_id", char_id_get(orig));
}

static inline struct char_data *char_following_get(struct char_data *ch) {
    int64_t id = char_condition_number_get(ch, "following", "target_id");
    return id ? char_by_id(id) : NULL;
}
static inline void char_following_set(struct char_data *ch, struct char_data *leader) {
    char_condition_remove(ch, "following", "update");
    if (leader) char_condition_apply_with_number(ch, "following", "group", "follow", "target_id", char_id_get(leader));
}

/* Score display helpers — thin wrappers so Zig can call functions in class.h / guild.h */
int64_t char_level_exp(struct char_data *ch, int level);
int     char_rpp_to_level(struct char_data *ch);

/* Status display helpers — new Lua bindings for do_status port */
int     char_limbcond_get(struct char_data *ch, int n);   /* n=1..4 */
void    char_limbcond_set(struct char_data *ch, int n, int val);
void    char_limb_healing_sync(struct char_data *ch);
void    char_die(struct char_data *ch, int64_t killer_id);
int64_t char_charge_get(struct char_data *ch);
void    char_charge_set(struct char_data *ch, int64_t value);
int64_t char_barrier_get(struct char_data *ch);
const char *char_voice_get(struct char_data *ch);
int     char_distfea_get(struct char_data *ch);
const char *char_rdisplay_get(struct char_data *ch);
const char *char_feature_get(struct char_data *ch);
void    char_feature_set(struct char_data *ch, const char *value);
void    char_rp_set(struct char_data *ch, int value);
int     char_radar1_get(struct char_data *ch);
void    char_radar1_set(struct char_data *ch, int vnum);
int     char_radar2_get(struct char_data *ch);
void    char_radar2_set(struct char_data *ch, int vnum);
int     char_radar3_get(struct char_data *ch);
void    char_radar3_set(struct char_data *ch, int vnum);
int     char_idnum_get(struct char_data *ch);
void    char_rp_save(struct char_data *ch);
void    char_bring_to_cap(struct char_data *ch);
void    char_rpp_custom_equip_launch(struct char_data *ch);
void    char_rpp_restring_launch(struct char_data *ch, struct obj_data *obj);
int     char_absorbs_get(struct char_data *ch);
int     char_mimic_get(struct char_data *ch);
int     char_backstab_cooldown_get(struct char_data *ch);
int     char_preference_get(struct char_data *ch);
void    char_preference_set(struct char_data *ch, int value);
int     char_genome_get(struct char_data *ch, int slot);
void    char_wait_set(struct char_data *ch, int pulses);
int     char_cooldown_get(struct char_data *ch);
void    char_cooldown_set(struct char_data *ch, int val);
bool    char_know_skill(struct char_data *ch, const char *skill_name);
void    char_improve_skill(struct char_data *ch, const char *skill_name, int flag);
int     char_aura_get(struct char_data *ch);
void    char_aura_set(struct char_data *ch, int value);
int     char_hairl_get(struct char_data *ch);
void    char_hairl_set(struct char_data *ch, int value);
int     char_hairs_get(struct char_data *ch);
void    char_hairs_set(struct char_data *ch, int value);
int     char_hairc_get(struct char_data *ch);
void    char_hairc_set(struct char_data *ch, int value);
int     char_skin_get(struct char_data *ch);
void    char_skin_set(struct char_data *ch, int value);
int     char_eye_get(struct char_data *ch);
void    char_eye_set(struct char_data *ch, int value);
void    char_distfea_set(struct char_data *ch, int value);
int     char_sleeptime_get(struct char_data *ch);
bool    char_has_group(struct char_data *ch);
bool    char_has_mail(struct char_data *ch);
int64_t char_soft_cap(struct char_data *ch);
bool    char_news_pending(struct char_data *ch);

/* Position commands */
bool    char_bonus_flagged(struct char_data *ch, int n);
void    char_affflag_set(struct char_data *ch, int flag, bool val);
void    char_barrier_set(struct char_data *ch, int64_t val);
void    char_carry_drop(struct char_data *ch, int type);
void    char_land(struct char_data *ch);
int     char_arena_idnum_get(struct char_data *ch);
void    char_arena_idnum_set(struct char_data *ch, int idnum);

/* Intro / name system */
int         char_intro_known(struct char_data *ch, struct char_data *vict);
const char *char_intro_name_get(struct char_data *ch, struct char_data *vict);
const char *char_introd_calc(struct char_data *ch);

/* Wizard commands */
const char *char_poofin_get(struct char_data *ch);
const char *char_poofout_get(struct char_data *ch);
int     char_loadroom_get(struct char_data *ch);
void    char_loadroom_set(struct char_data *ch, int vnum);
int     char_droom_get(struct char_data *ch);
void    char_droom_set(struct char_data *ch, int vnum);
void    char_look_at_room(struct char_data *ch);
void    char_look_at_specific_room(struct char_data *ch, struct room_data *room);
void    char_restore(struct char_data *vict, struct char_data *healer);
struct room_data *char_find_target_room(struct char_data *ch, const char *arg);

/* Imm logging / messaging */
void    char_send_to_imm(const char *msg);
void    char_log_imm_action(const char *msg);

#ifdef __cplusplus
}
#endif
