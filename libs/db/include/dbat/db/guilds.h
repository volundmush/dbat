#pragma once
#include "consts/types.h"
#include "consts/skills.h"
#include "consts/feats.h"

#define GW_ARRAY_MAX    4
 
struct guild_data {
  room_vnum vnum;                /* number of the guild */
  int skills[SKILL_TABLE_SIZE];  /* array to keep track of which feats things we'll train */
  float charge;                  /* charge * skill level = how much we'll charge */
  char *no_such_skill;           /* message when we don't teach that skill */
  char *not_enough_gold;         /* message when the student doesn't have enough gold */
  int minlvl;                    /* Minumum level guildmaster will train */
  mob_rnum gm;                   /* GM's vnum */
  int with_who[GW_ARRAY_MAX];    /* whom we dislike */
  int open, close;               /* when we will train */
  SPECIAL(*func);                /* secondary spec_proc for the GM */
  int feats[NUM_FEATS_DEFINED];  /* array to keep track of which feats things we'll train */
};

extern struct guild_data *guild_index;
extern int top_guild;