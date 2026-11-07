#pragma once
#include <cstdio>

#include "dbat/guild_utils.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/consts/feats.hpp"

constexpr int GW_ARRAY_MAX = 4;

/* Default guild messages: used for new guilds and in place of a missing or malformed message. */
extern const char *const guild_default_no_skill;
extern const char *const guild_default_no_gold;

struct guild_data {
  room_vnum vnum{};                     /* number of the guild */
  int skills[SKILL_TABLE_SIZE]{};       /* array to keep track of which feats things we'll train */
  float charge{};                       /* charge * skill level = how much we'll charge */
  char *no_such_skill{};                /* message when we don't teach that skill */
  char *not_enough_gold{};              /* message when the student doesn't have enough gold */
  int minlvl{};                         /* Minumum level guildmaster will train */
  mob_rnum gm{};                        /* GM's vnum */
  bitvector_t with_who[GW_ARRAY_MAX]{}; /* whom we dislike */
  int open{}, close{};                    /* when we will train */
  SPECIAL(*func){};                     /* secondary spec_proc for the GM */
  int feats[NUM_FEATS_DEFINED]{};       /* array to keep track of which feats things we'll train */
};


ACMD(do_teach);

/* Functions defined in guild.c */
int print_skills_by_type(GameWorld *g, struct char_data *ch, char *buf, int maxsz, int sktype, char *argument);
void levelup_parse(GameWorld *g, struct descriptor_data *d, char *arg);
int rpp_to_level(struct char_data *ch);
int slot_count(struct char_data *ch);
void show_guild(GameWorld *g, struct char_data *ch, char *arg);
void handle_ingest_learn(GameWorld *g, struct char_data *ch, struct char_data *vict);
void list_skills(GameWorld *g, struct char_data *ch, char *arg);
int count_guilds(GameWorld *g, guild_vnum low, guild_vnum high);

/*. External . */
SPECIAL(guild);

/* Trade restriction flags (TRADE_*) on a guild_data. */
bool guild_trade_toggle(struct guild_data *rec, int flag);
void boot_the_guilds(GameWorld *g, FILE *gm_f, char *filename, int rec_count);
void assign_the_guilds(GameWorld *g);
void list_guilds(GameWorld *g, struct char_data *ch, struct zone_data *rnum, guild_vnum vmin, guild_vnum vmax);
void destroy_guilds(GameWorld *g);
void sort_spells(GameWorld *g);

