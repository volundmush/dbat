/* ************************************************************************
*   File: guild.h                                                         *
*  Usage: GuildMaster definition for everything needed by guild.c         *
*                                                                         *
* Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
************************************************************************ */
#pragma once

#include "structs.h"

#define GW_ARRAY_MAX    4

#define GM_NUM(i)  (guild_index[i]->vnum)
#define GM_TRAINER(i) (guild_index[i]->gm)
#define GM_OPEN(i) (guild_index[i]->open)
#define GM_CLOSE(i) (guild_index[i]->close)
#define GM_CHARGE(i) (guild_index[i]->charge)
#define GM_MINLVL(i) (guild_index[i]->minlvl)
#define GM_WITH_WHO(i) (guild_index[i]->with_who)
#define GM_NO_SKILL(i)     (guild_index[i]->no_such_skill)
#define GM_NO_GOLD(i)     (guild_index[i]->not_enough_gold)
#define GM_COST(i, j, k) (int) (GM_CHARGE(i)*spell_info[j].min_level[(int)GET_CLASS(k)])
#define GM_FUNC(i) (guild_index[i]->func)

#define NOTRAIN_GOOD(i)        (GM_WITH_WHO(i).contains(TRADE_NOGOOD))
#define NOTRAIN_EVIL(i)        (GM_WITH_WHO(i).contains(TRADE_NOEVIL))
#define NOTRAIN_NEUTRAL(i)    (GM_WITH_WHO(i).contains(TRADE_NONEUTRAL))

#define TRAIN_WIZARD(i)    (GM_WITH_WHO(i).contains(TRADE_ONLYWIZARD))
#define TRAIN_CLERIC(i)        (GM_WITH_WHO(i).contains(TRADE_ONLYCLERIC))
#define TRAIN_ROGUE(i)        (GM_WITH_WHO(i).contains(TRADE_ONLYROGUE))
#define TRAIN_FIGHTER(i)    (GM_WITH_WHO(i).contains(TRADE_ONLYFIGHTER))
#define TRAIN_MONK(i)        (GM_WITH_WHO(i).contains(TRADE_ONLYMONK))
#define TRAIN_PALADIN(i)    (GM_WITH_WHO(i).contains(TRADE_ONLYPALADIN))
#define TRAIN_SORCERER(i)    (GM_WITH_WHO(i).contains(TRADE_ONLYSORCERER))
#define TRAIN_DRUID(i)        (GM_WITH_WHO(i).contains(TRADE_ONLYDRUID))
#define TRAIN_BARD(i)        (GM_WITH_WHO(i).contains(TRADE_ONLYBARD))
#define TRAIN_RANGER(i)        (GM_WITH_WHO(i).contains(TRADE_ONLYRANGER))
#define TRAIN_BARBARIAN(i)    (GM_WITH_WHO(i).contains(TRADE_ONLYBARBARIAN))
#define TRAIN_ARCANE_ARCHER(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYARCANE_ARCHER))
#define TRAIN_ARCANE_TRICKSTER(i) (GM_WITH_WHO(i).contains(TRADE_ONLYARCANE_TRICKSTER))
#define TRAIN_ARCHMAGE(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYARCHMAGE))
#define TRAIN_ASSASSIN(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYASSASSIN))
#define TRAIN_BLACKGUARD(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYBLACKGUARD))
#define TRAIN_DRAGON_DISCIPLE(i)  (GM_WITH_WHO(i).contains(TRADE_ONLYDRAGON_DISCIPLE))
#define TRAIN_DUELIST(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYDUELIST))
#define TRAIN_DWARVEN_DEFENDER(i) (GM_WITH_WHO(i).contains(TRADE_ONLYDWARVEN_DEFENDER))
#define TRAIN_ELDRITCH_KNIGHT(i)  (GM_WITH_WHO(i).contains(TRADE_ONLYELDRITCH_KNIGHT))
#define TRAIN_HIEROPHANT(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYHIEROPHANT))
#define TRAIN_HORIZON_WALKER(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYHORIZON_WALKER))
#define TRAIN_LOREMASTER(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYLOREMASTER))
#define TRAIN_MYSTIC_THEURGE(i)   (GM_WITH_WHO(i).contains(TRADE_ONLYMYSTIC_THEURGE))
#define TRAIN_SHADOWDANCER(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYSHADOWDANCER))
#define TRAIN_THAUMATURGIST(i)      (GM_WITH_WHO(i).contains(TRADE_ONLYTHAUMATURGIST))


#define NOTRAIN_WIZARD(i)    (GM_WITH_WHO(i).contains(TRADE_NOWIZARD))
#define NOTRAIN_CLERIC(i)    (GM_WITH_WHO(i).contains(TRADE_NOCLERIC))
#define NOTRAIN_ROGUE(i)    (GM_WITH_WHO(i).contains(TRADE_NOROGUE))
#define NOTRAIN_FIGHTER(i)    (GM_WITH_WHO(i).contains(TRADE_NOFIGHTER))
#define NOTRAIN_MONK(i)          (GM_WITH_WHO(i).contains(TRADE_NOMONK))
#define NOTRAIN_PALADIN(i)      (GM_WITH_WHO(i).contains(TRADE_NOPALADIN))
#define NOTRAIN_SORCERER(i)      (GM_WITH_WHO(i).contains(TRADE_NOSORCERER))
#define NOTRAIN_DRUID(i)      (GM_WITH_WHO(i).contains(TRADE_NODRUID))
#define NOTRAIN_BARD(i)      (GM_WITH_WHO(i).contains(TRADE_NOBARD))
#define NOTRAIN_RANGER(i)      (GM_WITH_WHO(i).contains(TRADE_NORANGER))
#define NOTRAIN_BARBARIAN(i)      (GM_WITH_WHO(i).contains(TRADE_NOBARBARIAN))
#define NOTRAIN_ARCANE_ARCHER(i)    (GM_WITH_WHO(i).contains(TRADE_NOARCANE_ARCHER))
#define NOTRAIN_ARCANE_TRICKSTER(i) (GM_WITH_WHO(i).contains(TRADE_NOARCANE_TRICKSTER))
#define NOTRAIN_ARCHMAGE(i)        (GM_WITH_WHO(i).contains(TRADE_NOARCHMAGE))
#define NOTRAIN_ASSASSIN(i)        (GM_WITH_WHO(i).contains(TRADE_NOASSASSIN))
#define NOTRAIN_BLACKGUARD(i)        (GM_WITH_WHO(i).contains(TRADE_NOBLACKGUARD))
#define NOTRAIN_DRAGON_DISCIPLE(i)  (GM_WITH_WHO(i).contains(TRADE_NODRAGON_DISCIPLE))
#define NOTRAIN_DUELIST(i)        (GM_WITH_WHO(i).contains(TRADE_NODUELIST))
#define NOTRAIN_DWARVEN_DEFENDER(i) (GM_WITH_WHO(i).contains(TRADE_NODWARVEN_DEFENDER))
#define NOTRAIN_ELDRITCH_KNIGHT(i)  (GM_WITH_WHO(i).contains(TRADE_NOELDRITCH_KNIGHT))
#define NOTRAIN_HIEROPHANT(i)        (GM_WITH_WHO(i).contains(TRADE_NOHIEROPHANT))
#define NOTRAIN_HORIZON_WALKER(i)   (GM_WITH_WHO(i).contains(TRADE_NOHORIZON_WALKER))
#define NOTRAIN_LOREMASTER(i)        (GM_WITH_WHO(i).contains(TRADE_NOLOREMASTER))
#define NOTRAIN_MYSTIC_THEURGE(i)   (GM_WITH_WHO(i).contains(TRADE_NOMYSTIC_THEURGE))
#define NOTRAIN_SHADOWDANCER(i)        (GM_WITH_WHO(i).contains(TRADE_NOSHADOWDANCER))
#define NOTRAIN_THAUMATURGIST(i)    (GM_WITH_WHO(i).contains(TRADE_NOTHAUMATURGIST))


#define NOTRAIN_HUMAN(i)    (GM_WITH_WHO(i).contains(TRADE_NOHUMAN))
#define NOTRAIN_SAIYAN(i)       (GM_WITH_WHO(i).contains(TRADE_NOSAIYAN))
#define NOTRAIN_ICER(i)    (GM_WITH_WHO(i).contains(TRADE_NOICER))
#define NOTRAIN_KONATSU(i)    (GM_WITH_WHO(i).contains(TRADE_NOKONATSU))
#define NOTRAIN_NAMEK(i)    (GM_WITH_WHO(i).contains(TRADE_NONAMEK))
#define NOTRAIN_MUTANT(i)    (GM_WITH_WHO(i).contains(TRADE_NOMUTANT))
#define NOTRAIN_KANASSAN(i)    (GM_WITH_WHO(i).contains(TRADE_NOKANASSAN))
#define NOTRAIN_BIO(i)    (GM_WITH_WHO(i).contains(TRADE_NOBIO))
#define NOTRAIN_ANDROID(i)    (GM_WITH_WHO(i).contains(TRADE_NOANDROID))
#define NOTRAIN_DEMON(i)    (GM_WITH_WHO(i).contains(TRADE_NODEMON))
#define NOTRAIN_MAJIN(i)    (GM_WITH_WHO(i).contains(TRADE_NOMAJIN))
#define NOTRAIN_KAI(i)    (GM_WITH_WHO(i).contains(TRADE_NOKAI))
#define NOTRAIN_TRUFFLE(i)    (GM_WITH_WHO(i).contains(TRADE_NOTRUFFLE))
#define NOTRAIN_GOBLIN(i)    (GM_WITH_WHO(i).contains(TRADE_NOGOBLIN))
#define NOTRAIN_ANIMAL(i)    (GM_WITH_WHO(i).contains(TRADE_NOANIMAL))
#define NOTRAIN_ORC(i)    (GM_WITH_WHO(i).contains(TRADE_NOORC))
#define NOTRAIN_SNAKE(i)    (GM_WITH_WHO(i).contains(TRADE_NOSNAKE))
#define NOTRAIN_TROLL(i)    (GM_WITH_WHO(i).contains(TRADE_NOTROLL))
#define NOTRAIN_HALFBREED(i)    (GM_WITH_WHO(i).contains(TRADE_NOHALFBREED))
#define NOTRAIN_MINOTAUR(i)    (GM_WITH_WHO(i).contains(TRADE_NOMINOTAUR))
#define NOTRAIN_KOBOLD(i)    (GM_WITH_WHO(i).contains(TRADE_NOKOBOLD))
#define NOTRAIN_LIZARDFOLK(i)    (GM_WITH_WHO(i).contains(TRADE_NOLIZARDFOLK))


extern std::unordered_map<guild_vnum, std::shared_ptr<Guild>> guild_index;
extern guild_vnum top_guild;
extern int spell_sort_info[SKILL_TABLE_SIZE + 1];

/* Functions defined in guild.c */
extern int print_skills_by_type(BaseCharacter *ch, char *buf, int maxsz, int sktype, char *argument);

extern void levelup_parse(struct descriptor_data *d, char *arg);

extern int rpp_to_level(BaseCharacter *ch);

extern int slot_count(BaseCharacter *ch);

extern void show_guild(BaseCharacter *ch, char *arg);

extern void handle_ingest_learn(BaseCharacter *ch, BaseCharacter *vict);

extern void list_skills(BaseCharacter *ch, char *arg);

extern void assign_the_guilds();

/*. External . */
extern SPECIAL(guild);
