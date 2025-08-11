/* ************************************************************************
*   File: guild.h                                                         *
*  Usage: GuildMaster definition for everything needed by guild.c         *
*                                                                         *
* Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
************************************************************************ */
#pragma once

#include "structs.h"



#define GM_NUM(i)  (guild_index.at((i)).vnum)
#define GM_TRAINER(i) (guild_index.at((i)).keeper)
#define GM_OPEN(i) (guild_index.at((i)).open)
#define GM_CLOSE(i) (guild_index.at((i)).close)
#define GM_CHARGE(i) (guild_index.at((i)).charge)
#define GM_MINLVL(i) (guild_index.at((i)).minlvl)
#define GM_WITH_WHO(i) (guild_index.at((i)).with_who)
#define GM_NO_SKILL(i)     (guild_index.at((i)).no_such_skill)
#define GM_NO_GOLD(i)     (guild_index.at((i)).not_enough_gold)
#define GM_COST(i, j, k) (int) (GM_CHARGE(i)*spell_info[j].min_level[(int)GET_CLASS(k)])
#define GM_FUNC(i) (guild_index.at((i)).func)

#define NOTRAIN_GOOD(i)        (IS_SET_AR((GM_WITH_WHO(i)),TRADE_NOGOOD))
#define NOTRAIN_EVIL(i)        (IS_SET_AR((GM_WITH_WHO(i)),TRADE_NOEVIL))
#define NOTRAIN_NEUTRAL(i)    (IS_SET_AR((GM_WITH_WHO(i)),TRADE_NONEUTRAL))

#define TRAIN_WIZARD(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYWIZARD))
#define TRAIN_CLERIC(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYCLERIC))
#define TRAIN_ROGUE(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYROGUE))
#define TRAIN_FIGHTER(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYFIGHTER))
#define TRAIN_MONK(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYMONK))
#define TRAIN_PALADIN(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYPALADIN))
#define TRAIN_SORCERER(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYSORCERER))
#define TRAIN_DRUID(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYDRUID))
#define TRAIN_BARD(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYBARD))
#define TRAIN_RANGER(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYRANGER))
#define TRAIN_BARBARIAN(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYBARBARIAN))
#define TRAIN_ARCANE_ARCHER(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYARCANE_ARCHER))
#define TRAIN_ARCANE_TRICKSTER(i) (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYARCANE_TRICKSTER))
#define TRAIN_ARCHMAGE(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_ONLYARCHMAGE))


#define NOTRAIN_WIZARD(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOWIZARD))
#define NOTRAIN_CLERIC(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOCLERIC))
#define NOTRAIN_ROGUE(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOROGUE))
#define NOTRAIN_FIGHTER(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOFIGHTER))
#define NOTRAIN_MONK(i)          (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOMONK))
#define NOTRAIN_PALADIN(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOPALADIN))
#define NOTRAIN_SORCERER(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOSORCERER))
#define NOTRAIN_DRUID(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NODRUID))
#define NOTRAIN_BARD(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOBARD))
#define NOTRAIN_RANGER(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NORANGER))
#define NOTRAIN_BARBARIAN(i)      (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOBARBARIAN))
#define NOTRAIN_ARCANE_ARCHER(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOARCANE_ARCHER))
#define NOTRAIN_ARCANE_TRICKSTER(i) (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOARCANE_TRICKSTER))
#define NOTRAIN_ARCHMAGE(i)        (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOARCHMAGE))


#define NOTRAIN_HUMAN(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOHUMAN))
#define NOTRAIN_SAIYAN(i)       (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOSAIYAN))
#define NOTRAIN_ICER(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOICER))
#define NOTRAIN_KONATSU(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOKONATSU))
#define NOTRAIN_NAMEK(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NONAMEK))
#define NOTRAIN_MUTANT(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOMUTANT))
#define NOTRAIN_KANASSAN(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOKANASSAN))
#define NOTRAIN_BIO(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOBIO))
#define NOTRAIN_ANDROID(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOANDROID))
#define NOTRAIN_DEMON(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NODEMON))
#define NOTRAIN_MAJIN(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOMAJIN))
#define NOTRAIN_KAI(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOKAI))
#define NOTRAIN_TRUFFLE(i)    (IS_SET_AR((GM_WITH_WHO(i)), TRADE_NOTRUFFLE))

extern guild_vnum top_guild;
extern int spell_sort_info[SKILL_TABLE_SIZE + 1];

/* Functions defined in guild.c */
extern int print_skills_by_type(Character *ch, char *buf, int maxsz, int sktype, char *argument);

extern void levelup_parse(struct descriptor_data *d, char *arg);

extern int rpp_to_level(Character *ch);

extern int slot_count(Character *ch);

extern void show_guild(Character *ch, char *arg);

extern void handle_ingest_learn(Character *ch, Character *vict);

extern void list_skills(Character *ch, char *arg);

extern void assign_the_guilds();

/*. External . */
extern SPECIAL(guild);
