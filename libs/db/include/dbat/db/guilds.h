#pragma once
#include "consts/types.h"
#include "consts/skills.h"
#include "consts/feats.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GW_ARRAY_MAX    4
 
struct guild_data {
  room_vnum vnum;                /* number of the guild */
  int skills[SKILL_TABLE_SIZE];  /* array to keep track of which feats things we'll train */
  float charge;                  /* charge * skill level = how much we'll charge */
  char *no_such_skill;           /* message when we don't teach that skill */
  char *not_enough_gold;         /* message when the student doesn't have enough gold */
  int minlvl;                    /* Minumum level guildmaster will train */
  mob_rnum gm;                   /* GM's vnum */
  bitvector_t with_who[GW_ARRAY_MAX];    /* whom we dislike */
  int open, close;               /* when we will train */
  SPECIAL(*func);                /* secondary spec_proc for the GM */
  int feats[NUM_FEATS_DEFINED];  /* array to keep track of which feats things we'll train */
};

// Guild API functions, implemented in guilds_api.zig
guild_vnum guild_id_get(struct guild_data *guild);
void guild_id_set(struct guild_data *guild, guild_vnum id);
bool guild_skill_get(struct guild_data *guild, size_t skill);
void guild_skill_set(struct guild_data *guild, size_t skill, bool value);
bool guild_feat_get(struct guild_data *guild, size_t feat);
void guild_feat_set(struct guild_data *guild, size_t feat, bool value);
float guild_charge_get(struct guild_data *guild);
void guild_charge_set(struct guild_data *guild, float charge);
const char *guild_no_such_skill_get(struct guild_data *guild);
void guild_no_such_skill_set(struct guild_data *guild, const char *value);
const char *guild_not_enough_gold_get(struct guild_data *guild);
void guild_not_enough_gold_set(struct guild_data *guild, const char *value);
int guild_min_level_get(struct guild_data *guild);
void guild_min_level_set(struct guild_data *guild, int level);
mob_vnum guild_master_get(struct guild_data *guild);
void guild_master_set(struct guild_data *guild, mob_vnum vnum);
bool guild_trade_flagged(struct guild_data *guild, int pos);
bool guild_trade_flag_toggle(struct guild_data *guild, int pos);
void guild_trade_flag_set(struct guild_data *guild, int pos, bool value);
int guild_open_get(struct guild_data *guild);
void guild_open_set(struct guild_data *guild, int value);
int guild_close_get(struct guild_data *guild);
void guild_close_set(struct guild_data *guild, int value);
SpecialFunc guild_func_get(struct guild_data *guild);
void guild_func_set(struct guild_data *guild, SpecialFunc func);

extern struct guild_data *guild_index;
extern int top_guild;

guild_rnum real_guild(guild_vnum vnum);
struct guild_data *guild_by_id(guild_vnum vnum);

#ifdef __cplusplus
}
#endif
