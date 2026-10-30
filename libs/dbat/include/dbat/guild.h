#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/skills.h"

ACMD(do_teach);

#define GM_NUM(i)  (guild_index[i].vnum)
#define GM_TRAINER(i) (guild_index[i].gm)
#define GM_OPEN(i) (guild_index[i].open)
#define GM_CLOSE(i) (guild_index[i].close)
#define GM_CHARGE(i) (guild_index[i].charge)
#define GM_MINLVL(i) (guild_index[i].minlvl)
#define GM_WITH_WHO(i) (guild_index[i].with_who)
#define GM_NO_SKILL(i)	 (guild_index[i].no_such_skill)
#define GM_NO_GOLD(i)	 (guild_index[i].not_enough_gold)
#define GM_COST(i,j,k) (int) (GM_CHARGE(i)*spell_info[j].min_level[(int)char_get_class(k)])
#define GM_FUNC(i) (guild_index[i].func)

extern int spell_sort_info[SKILL_TABLE_SIZE];

/* Functions defined in guild.c */
int print_skills_by_type(struct char_data *ch, char *buf, int maxsz, int sktype, char *argument);
void levelup_parse(struct descriptor_data *d, char *arg);
int rpp_to_level(struct char_data *ch);
int slot_count(struct char_data *ch);
void show_guild(struct char_data * ch, char *arg);
void handle_ingest_learn(struct char_data *ch, struct char_data *vict);
void list_skills(struct char_data *ch, char *arg);
int count_guilds(guild_vnum low, guild_vnum high);

/*. External . */
SPECIAL(guild);

/* Trade restriction flags (TRADE_*) on a guild_data. */
bool guild_trade_flagged(const struct guild_data *rec, int flag);
void guild_trade_set(struct guild_data *rec, int flag, bool on);
bool guild_trade_toggle(struct guild_data *rec, int flag);
