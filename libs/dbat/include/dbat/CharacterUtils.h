#include "Character.h"
#include "CharacterPrototype.h"

inline bool IS_NPC(const Character* ch) {
    return !ch->isPC;
}

inline bool IS_PC(const Character* ch) {
    return ch->isPC;
}

inline bool IS_MOB(const Character* ch) {
    return ch->getProto() ? true : false;
}


#define IS_AFFECTED(ch, skill) AFF_FLAGGED(ch, skill)

#define PLR_TOG_CHK(ch, flag) ch->player_flags.toggle(flag)
#define PRF_TOG_CHK(ch, flag) ch->pref_flags.toggle(flag)
#define ADM_TOG_CHK(ch, flag) ch->admin_flags.toggle(flag)
#define AFF_TOG_CHK(ch, flag) ch->affect_flags.toggle(flag)

#define MOB_FLAGS(ch)    ch->mobFlags
#define PLR_FLAGS(ch)    ch->playerFlags
#define PRF_FLAGS(ch)    ch->pref
#define AFF_FLAGS(ch)    ch->affect_flags
#define ADM_FLAGS(ch)    ch->admflags
#define BODY_FLAGGED(ch, flag) ch->bodyparts.test(flag)
#define DEAD(ch) (PLR_FLAGGED(ch, PLR_NOTDEADYET) || MOB_FLAGGED(ch, MOB_NOTDEADYET))

#define IN_ROOM(ch)    ch->location.getVnum()
#define IN_ZONE(ch)   ch->getRoom()->zone
#define GET_AGE(ch)     ch->time.currentAge()

#define GET_PC_NAME(ch)    ch->getName()
#define GET_NAME(ch)    (IS_NPC(ch) ? ch->getShortDescription() : GET_PC_NAME(ch))
#define GET_TITLE(ch)   ch->title
#define GET_PHASE(ch)   ch->getBaseStat<int>("starphase")
#define GET_MIMIC(ch)   (ch->mimic ? ch->mimic->getID()+1 : 0)
#define GET_VOICE(ch)   ch->voice
#define GET_CLAN(ch)    ch->clan
#define GET_TRANSCLASS(ch) ch->transclass
#define GET_FEATURE(ch) ch->feature
#define GET_USER(ch)    (ch->desc ? (ch->desc->account ? (char*)(ch->desc->account->name.c_str()) : "NOUSER") : "NOUSER")
#define GET_CRANK(ch)   ch->crank
#define GET_ADMLEVEL(ch)    ch->getBaseStat<int>("admin_level")
#define GET_LEVEL(ch)    ch->getBaseStat<int>("level")

#define GET_CLASS(ch)   ch->sensei

#define GET_RACE(ch)    ch->race
#define GET_HAIRL(ch)   ch->getAppearanceStr(Appearance::hair_length)
#define GET_HAIRC(ch)   ch->getAppearanceStr(Appearance::hair_color)
#define GET_HAIRS(ch)   ch->getAppearanceStr(Appearance::hair_style)
#define GET_SKIN(ch)    ch->getAppearanceStr(Appearance::skin_color)
#define GET_EYE(ch)     ch->getAppearanceStr(Appearance::eye_color)
#define GET_HOME(ch)    ch->getBaseStat<room_vnum>("hometown")
#define GET_WEIGHT(ch)  ch->getEffectiveStat("weight")
#define GET_HEIGHT(ch)  ch->getEffectiveStat("height")
#define GET_PC_HEIGHT(ch)    GET_HEIGHT(ch)
#define GET_PC_WEIGHT(ch)    GET_WEIGHT(ch)
#define GET_SEX(ch)    ch->sex
#define CARRYING(ch)    ch->carrying
#define CARRIED_BY(ch)  ch->carried_by
#define GET_RP(ch)      ch->getRPP()
#define GET_SUPPRESS(ch) ch->getBaseStat<int>("suppression")
#define GET_RDISPLAY(ch) ch->rdisplay

#define GET_STR(ch)     ch->getEffectiveStat<int>("strength")
#define GET_DEX(ch)     ch->getEffectiveStat<int>("agility")
#define GET_INT(ch)     ch->getEffectiveStat<int>("intelligence")
#define GET_WIS(ch)     ch->getEffectiveStat<int>("wisdom")
#define GET_CON(ch)     ch->getEffectiveStat<int>("constitution")
#define GET_CHA(ch)     ch->getEffectiveStat<int>("speed")
#define GET_MUTBOOST(ch) (IS_MUTANT(ch) ? (((ch)->mutations.get(Mutation::extreme_speed)) ? (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch)) * 0.3 : 0) : 0)
extern int GET_SPEEDI(Character *ch);
#define GET_SPEEDCALC(ch) (IS_GRAP(ch) ? GET_CHA(ch) : (IS_INFERIOR(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 1.25) : GET_SPEEDVAR(ch)) : GET_SPEEDVAR(ch)))
#define GET_SPEEDBONUS(ch) (IS_ARLIAN(ch) ? AFF_FLAGGED(ch, AFF_SHELL) ? GET_SPEEDVAR(ch) * -0.5 : (IS_MALE(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 0.5) : 0) : 0) : 0)
#define GET_SPEEDVAR(ch) (GET_SPEEDVEM(ch) > GET_CHA(ch) ? GET_SPEEDVEM(ch) : GET_CHA(ch))
#define GET_SPEEDVEM(ch) (GET_SPEEDINT(ch) - (GET_SPEEDINT(ch) * (ch)->getBaseStat("speednar")))
#define IS_GRAP(ch)     (GRAPPLING(ch) || GRAPPLED(ch))
#define GET_SPEEDINT(ch) (IS_BIO(ch) ? ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1200) / 1200) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)) : ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1000) / 1000) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)))
#define IS_INFERIOR(ch) (IS_KONATSU(ch) || IS_DEMON(ch))
#define IS_WEIGHTED(ch) (ch->getBaseStat("speednar") < 1.0)


#define GET_EXP(ch)      ch->getBaseStat<int64_t>("experience")

#define SPOILED(ch)       ch->time.played > 86400
#define GET_DEATH_TYPE(ch) ch->getBaseStat<int>("death_type")
#define GET_SLEEPT(ch)    ch->getBaseStat<int>("sleeptime")
#define GET_FOODR(ch)     ch->getBaseStat<int>("food_rejuvenation")
#define GET_ALT(ch)       ch->getBaseStat<int>("altitude")
#define GET_CHARGE(ch)    ch->getBaseStat<int64_t>("charge")
#define GET_CHARGETO(ch)  ch->getBaseStat<int64_t>("chargeto")
#define GET_ARMOR(ch)     ch->getEffectiveStat<int>("armor")
#define GET_HIT(ch)      ch->getCurVital(CharVital::health)
#define GET_MAX_HIT(ch)      ch->getEffectiveStat<int64_t>("health")
#define GET_MAX_MOVE(ch)  ch->getEffectiveStat<int64_t>("stamina")
#define GET_MAX_MANA(ch)  ch->getEffectiveStat<int64_t>("ki")
#define GET_KI(ch)      ch->getCurVital(CharVital::ki)
#define GET_DROOM(ch)     (ch->registeredLocations.contains("death_room") ? ch->registeredLocations.at("death_room").getVnum() : Location())
#define GET_SPAM(ch)      ch->getBaseStat<int>("spam")
#define GET_SHIPROOM(ch)  ch->getBaseStat<room_vnum>("ship_room")
#define GET_LPLAY(ch)     ch->getBaseStat<time_t>("last_played")
#define GET_DTIME(ch)     ch->getBaseStat<time_t>("death_time")
#define GET_RTIME(ch)     ch->getBaseStat<time_t>("rewtime")
#define GET_DCOUNT(ch)    ch->getBaseStat<int>("death_count")
#define GET_BOARD(ch, i)  ch->lboard[i]
#define GET_LIMBS(ch, i)  ch->limbs[i]
#define GET_LIMBCOND(ch, i) ch->limb_condition[i]
#define GET_SONG(ch)      ch->getBaseStat<int>("mystic_melody")
#define GET_BONUS(ch, i)  false
#define GET_TRANSCOST(ch, i) ch->transcost[i]
#define COMBO(ch)         ch->getBaseStat<int>("combo")
#define LASTATK(ch)       ch->getBaseStat<int>("last_attack")
#define COMBHITS(ch)      ch->getBaseStat<int>("combo_hits")
#define GET_AURA(ch)      ch->getAppearanceStr(Appearance::aura_color)
#define GET_RADAR1(ch)    ch->getBaseStat<room_vnum>("radar1")
#define GET_RADAR2(ch)    ch->getBaseStat<room_vnum>("radar2")
#define GET_RADAR3(ch)    ch->getBaseStat<room_vnum>("radar3")
#define GET_PING(ch)      ch->getBaseStat<int>("ping")
#define GET_SLOTS(ch)     ch->getBaseStat<int>("skill_slots")
#define GET_TGROWTH(ch)   ch->getBaseStat<int>("tail_growth")
#define GET_RMETER(ch)    ch->rage_meter
#define GET_PERSONALITY(ch) ch->getBaseStat<int>("personality")
#define GET_COMBINE(ch)   ch->getBaseStat<int>("combine")
#define GET_PREFERENCE(ch) ch->getBaseStat<int>("preference")
#define GET_RELAXCOUNT(ch) ch->getBaseStat<int>("relax_count")
#define GET_BLESSLVL(ch)  ch->getBaseStat<int>("bless_level")
#define GET_ASB(ch)       ch->getBaseStat<int>("auto_skill_bonus")
#define GET_REGEN(ch)     ch->getBaseStat<int>("regen_rate")
#define GET_BLESSBONUS(ch) (AFF_FLAGGED(ch, AFF_BLESS) ? (GET_BLESSLVL(ch) >= 100 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.1 : GET_BLESSLVL(ch) >= 60 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.05 : GET_BLESSLVL(ch) >= 40 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.02 : 0) : 0)
#define GET_POSELF(ch)    (!IS_NPC(ch) ? PLR_FLAGGED(ch, PLR_POSE) ? GET_SKILL(ch, SKILL_POSE) >= 100 ? 0.15 : GET_SKILL(ch, SKILL_POSE) >= 60 ? 0.1 : GET_SKILL(ch, SKILL_POSE) >= 40 ? 0.05 : 0 : 0 : 0)
#define GET_POSEBONUS(ch) (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * GET_POSELF(ch))
#define GET_LIFEBONUS(ch) (IS_ARLIAN(ch) ? ((GET_MAX_MANA(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) + ((GET_MAX_MOVE(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) : 0)
#define GET_LIFEBONUSES(ch) ch->getBaseStat<int>("lifebonus") > 0 ? (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)) * (((ch)->getBaseStat<int>("lifebonus") + 100) * 0.01) : (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch))
#define GET_LIFEPERC(ch)  ch->getBaseStat<int>("life_percent")
#define GET_STUPIDKISS(ch) ch->getBaseStat<int>("stupidkiss")
#define GET_SPEEDBOOST(ch) ch->getBaseStat<int>("speedboost")
#define GET_BACKSTAB_COOL(ch) ch->getBaseStat<int>("backstab_cooldown")
#define GET_COOLDOWN(ch)  ch->getBaseStat<int>("concentrate_cooldown")
#define GET_BARRIER(ch)   ch->getBaseStat<int64_t>("barrier")
#define GET_GOLD(ch)      ch->getBaseStat<money_t>("money_carried")
#define GET_KAIOKEN(ch)   ch->getBaseStat<int>("kaioken")
#define GET_BOOSTS(ch)    ch->getBaseStat<int>("boosts")

#define GET_FURY(ch)      ch->getBaseStat<int>("fury")
#define GET_UP(ch)        ch->getBaseStat<int>("upgrade_points")
#define GET_FORGETING(ch) ch->getBaseStat<int>("forgetting_skill")
#define GET_FORGET_COUNT(ch) ch->getBaseStat<int>("forget_count")
#define GET_BANK_GOLD(ch) ch->getBaseStat<money_t>("money_bank")
#define GET_POLE_BONUS(ch) ch->getBaseStat<int>("pole_bonus")
#define GET_FISHSTATE(ch)  ch->getBaseStat<int>("fish_state")
#define GET_FISHD(ch)     ch->getBaseStat<int>("fish_distance")
#define GET_DAMAGE_MOD(ch) ch->getBaseStat<int>("damage_mod")
#define GET_SPELLFAIL(ch) ch->getBaseStat<int16_t>("spellfail")
#define GET_ARMORCHECK(ch) ch->getBaseStat<int16_t>("armorcheck")
#define GET_ARMORCHECKALL(ch) ch->getBaseStat<int16_t>("armorcheckall")
#define GET_MOLT_EXP(ch)  ch->getBaseStat<int64_t>("molt_experience")
#define GET_MOLT_LEVEL(ch) ch->getBaseStat<int>("molt_level")
#define GET_SDCOOLDOWN(ch) ch->getBaseStat<int>("selfdestruct_cooldown")
#define GET_INGESTLEARNED(ch) ch->getBaseStat<int>("ingest_learned")
#define GET_POS(ch)        ch->position
#define GET_IDNUM(ch)        ch->id
#define IS_CARRYING_W(ch)    ch->getBaseStat("weight_carried")
#define IS_CARRYING_N(ch)    ch->getInventory().size()
#define FIGHTING(ch)        ch->fighting
#define GET_POWERATTACK(ch)    ch->powerattack
#define GET_GROUPKILLS(ch)    ch->getBaseStat<int>("group_kills")
#define GET_ALIGNMENT(ch)    ch->getBaseStat<int>("good_evil")
#define GET_ETHIC_ALIGNMENT(ch)    ch->getBaseStat<int>("law_chaos")
#define SITS(ch)                ch->sits.lock().get()
#define MINDLINK(ch)            ch->mindlink
#define LINKER(ch)              ch->getBaseStat<int>("mind_linker")
#define LASTHIT(ch)             ch->getBaseStat<int>("lasthit")
#define DRAGGING(ch)            ch->drag
#define DRAGGED(ch)             ch->dragged
#define GRAPPLING(ch)           ch->grappling
#define GRAPPLED(ch)            ch->grappled
#define GRAPTYPE(ch)            ch->getBaseStat<int>("grapple_type")
#define GET_ORIGINAL(ch)        ch->original
#define GET_CLONES(ch)          ch->clones.size()
#define GET_DEFENDER(ch)        ch->defender
#define GET_DEFENDING(ch)       ch->defending
#define BLOCKS(ch)              ch->blocks
#define BLOCKED(ch)             ch->blocked
#define ABSORBING(ch)           ch->absorbing
#define ABSORBBY(ch)            ch->absorbby
#define GET_EAVESDROP(ch)       ch->getBaseStat<room_vnum>("listen_room")
#define GET_EAVESDIR(ch)        ch->getBaseStat<int>("listen_direction")
#define GET_ABSORBS(ch)         ch->getBaseStat<int>("absorbs")
#define GET_LINTEREST(ch)       ch->getBaseStat<time_t>("last_interest")

#define GET_COND(ch, i)        ch->conditions[i]
#define GET_LOADROOM(ch)    (ch->registeredLocations.contains("load_room") ? ch->registeredLocations.at("load_room") : Location())
#define GET_PRACTICES(ch)    ch->getPractices()
#define GET_TRAINSTR(ch)        ch->getBaseStat<int>("train_strength")
#define GET_TRAININT(ch)        ch->getBaseStat<int>("train_intelligence")
#define GET_TRAINCON(ch)        ch->getBaseStat<int>("train_constitution")
#define GET_TRAINWIS(ch)        ch->getBaseStat<int>("train_wisdom")
#define GET_TRAINAGL(ch)        ch->getBaseStat<int>("train_agility")
#define GET_TRAINSPD(ch)        ch->getBaseStat<int>("train_speed")
#define GET_INVIS_LEV(ch)    ch->getBaseStat<int>("invis_level")
#define GET_WIMP_LEV(ch)    ch->getBaseStat<int>("wimp_level")
#define GET_FREEZE_LEV(ch)    ch->getBaseStat<int>("freeze_level")
#define POOFIN(ch)        ch->poofin
#define POOFOUT(ch)        ch->poofout
#define GET_OLC_ZONE(ch)    ch->getBaseStat<int>("olc_zone")
#define GET_LAST_OLC_TARG(ch)    ch->last_olc_targ
#define GET_LAST_TELL(ch)    ch->getBaseStat<int>("last_tell")

int16_t GET_SKILL_BONUS(Character *ch, uint16_t skill);
int16_t GET_SKILL_PERF(Character *ch, uint16_t skill);
int16_t GET_SKILL_BASE(Character *ch, uint16_t skill);
int16_t GET_SKILL(Character *ch, uint16_t skill);

void SET_SKILL(Character *ch, uint16_t skill, int16_t val);
void SET_SKILL_BONUS(Character *ch, uint16_t skill, int16_t val);
void SET_SKILL_PERF(Character *ch, uint16_t skill, int16_t val);

#define GET_EQ(ch, i)        ch->getEquipSlot(i)

inline SpecialFunc GET_MOB_SPEC(Character *ch) {
    if(auto find = mob_proto.find(ch->getVnum()); find != mob_proto.end())
        return find->second->func;
    return nullptr;
}

#define GET_MOB_RNUM(mob)    mob->getVnum()
#define GET_MOB_VNUM(mob)    mob->getVnum()

#define MEMORY(ch)        ch->mob_specials.memory

#define CAN_CARRY_W(ch) (ch->getEffectiveStat<int>("carry_capacity"))
#define CAN_CARRY_N(ch) 50
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || PRF_FLAGGED(ch, PRF_HOLYLIGHT) || ch->mutations.get(Mutation::infravision) || PLR_FLAGGED(ch, PLR_AURALIGHT))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 50)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -50)
#define IS_LAWFUL(ch)   (GET_ETHIC_ALIGNMENT(ch) >= 350)
#define IS_CHAOTIC(ch)  (GET_ETHIC_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_ENEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define ALIGN_TYPE(ch)    ((IS_GOOD(ch) ? 0 : (IS_EVIL(ch) ? 6 : 3)) + \
                         (IS_LAWFUL(ch) ? 0 : (IS_CHAOTIC(ch) ? 2 : 1)))

#define IN_ARENA(ch)   (GET_ROOM_VNUM(IN_ROOM(ch)) >= 17800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 17874)
#define ARENA_IDNUM(ch) ch->getBaseStat<room_vnum>("arena_watch")

/* These three deprecated. */
extern void WAIT_STATE(Character *ch, double timeToWait);
#define GET_WAIT_STATE(ch)    ch->getBaseStat("waitTime")
#define CHECK_WAIT(ch)                (GET_WAIT_STATE(ch) > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)

#define SENDOK(ch)    ((ch->desc || SCRIPT_CHECK(ch, MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch)) && \
                      !PLR_FLAGGED(ch, PLR_WRITING))

                      
#define HSHR(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "his": (GET_SEX(ch)==SEX_FEMALE ? "her" : "their")) :"its")
#define HSSH(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "he" : (GET_SEX(ch)==SEX_FEMALE ? "she" : "they")) : "it")
#define HMHR(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "him": (GET_SEX(ch)==SEX_FEMALE ? "her" : "their")) : "it")
#define MAFE(ch) (GET_SEX(ch) != Sex::neutral ? (GET_SEX(ch)==SEX_MALE ? "male": (GET_SEX(ch)==SEX_FEMALE ? "female" : "androgynous")) : "questionably gendered")


#define RACE(ch)      ch->juggleRaceName(true).c_str()
#define LRACE(ch)     ch->juggleRaceName(false).c_str()
#define TRUE_RACE(ch) race::getName(ch->race).c_str()

#define CLASS_ABBR(ch) sensei::getAbbr(ch->sensei).c_str()
#define RACE_ABBR(ch) ch->race->getAbbr().c_str()

#define IS_ROSHI(ch)            (GET_CLASS(ch) == Sensei::roshi)
#define IS_PICCOLO(ch)          (GET_CLASS(ch) == Sensei::piccolo)
#define IS_KRANE(ch)            (GET_CLASS(ch) == Sensei::crane)
#define IS_NAIL(ch)             (GET_CLASS(ch) == Sensei::nail)
#define IS_BARDOCK(ch)          (GET_CLASS(ch) == Sensei::bardock)
#define IS_GINYU(ch)            (GET_CLASS(ch) == Sensei::ginyu)
#define IS_FRIEZA(ch)           (GET_CLASS(ch) == Sensei::frieza)
#define IS_TAPION(ch)           (GET_CLASS(ch) == Sensei::tapion)
#define IS_ANDSIX(ch)           (GET_CLASS(ch) == Sensei::sixteen)
#define IS_DABURA(ch)           (GET_CLASS(ch) == Sensei::dabura)
#define IS_KABITO(ch)           (GET_CLASS(ch) == Sensei::kibito)
#define IS_JINTO(ch)            (GET_CLASS(ch) == Sensei::jinto)
#define IS_TSUNA(ch)            (GET_CLASS(ch) == Sensei::tsuna)
#define IS_KURZAK(ch)           (GET_CLASS(ch) == Sensei::kurzak)

#define GOLD_CARRY(ch)        (GET_LEVEL(ch) < 100 ? (GET_LEVEL(ch) < 50 ? GET_LEVEL(ch) * 10000 : 500000) : 50000000)
#define IS_SHADOW_DRAGON1(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON1_VNUM)
#define IS_SHADOW_DRAGON2(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON2_VNUM)
#define IS_SHADOW_DRAGON3(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON3_VNUM)
#define IS_SHADOW_DRAGON4(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON4_VNUM)
#define IS_SHADOW_DRAGON5(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON5_VNUM)
#define IS_SHADOW_DRAGON6(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON6_VNUM)
#define IS_SHADOW_DRAGON7(ch)   (GET_MOB_VNUM(ch) == SHADOW_DRAGON7_VNUM)
#define CAN_GRAND_MASTER(ch)    IS_HUMAN(ch)
#define IS_HUMANOID(ch)         (!IS_SERPENT(ch) && !IS_ANIMAL(ch))
#define IS_ROBOT(ch)            (IS_ANDROID(ch) || IS_MECHANICAL(ch))
#define RESTRICTED_RACE(ch)     (IS_MAJIN(ch) || IS_SAIYAN(ch) || IS_BIO(ch) || IS_HOSHIJIN(ch))
#define CHEAP_RACE(ch)          (IS_TRUFFLE(ch) || IS_MUTANT(ch) || IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define SPAR_TRAIN(ch)          (FIGHTING(ch) && !IS_NPC(ch) && (ch)->character_flags.get(CharacterFlag::sparring) &&\
                                 !IS_NPC(FIGHTING(ch)) && FIGHTING(ch)->character_flags.get(CharacterFlag::sparring))
#define IS_PTRANS(ch)           (IS_ANDROID(ch) || IS_TRUFFLE(ch) || IS_BIO(ch) || IS_MAJIN(ch))
#define IS_NONPTRANS(ch)        !IS_PTRANS(ch)
#define OOZARU_RACE(ch)         (IS_SAIYAN(ch) || IS_HALFBREED(ch))
#define IS_TRANSFORMED(ch)      (ch->form != Form::base)
#define BIRTH_PHASE             (time_info.day <= 15)
#define LIFE_PHASE              (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE             (!BIRTH_PHASE && !LIFE_PHASE)

#define OOZARU_RACE(ch)         (IS_SAIYAN(ch) || IS_HALFBREED(ch))

#define _HAS_LIMB(ch, num, flag) (GET_LIMBCOND((ch), (num)) > 0 || (ch)->character_flags.get((flag)))

#define HAS_ARMS(ch)            ((_HAS_LIMB((ch), 0, CharacterFlag::cyber_right_arm)) || _HAS_LIMB((ch), 1, CharacterFlag::cyber_left_arm) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1 && GRAPTYPE(ch) != 4)))
#define HAS_LEGS(ch)            ((_HAS_LIMB((ch), 2, CharacterFlag::cyber_right_leg)) || _HAS_LIMB((ch), 3, CharacterFlag::cyber_left_leg) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1)))

#define IS_HUMAN(ch)            (GET_RACE(ch) == Race::human)
#define IS_SAIYAN(ch)           (GET_RACE(ch) == Race::saiyan)
#define IS_ICER(ch)             (GET_RACE(ch) == Race::icer)
#define IS_KONATSU(ch)          (GET_RACE(ch) == Race::konatsu)
#define IS_NAMEK(ch)            (GET_RACE(ch) == Race::namekian)
#define IS_MUTANT(ch)           (GET_RACE(ch) == Race::mutant)
#define IS_KANASSAN(ch)         (GET_RACE(ch) == Race::kanassan)
#define IS_HALFBREED(ch)        (GET_RACE(ch) == Race::half_saiyan)
#define IS_BIO(ch)              (GET_RACE(ch) == Race::bio_android)
#define IS_ANDROID(ch)          (GET_RACE(ch) == Race::android)
#define IS_DEMON(ch)            (GET_RACE(ch) == Race::demon)
#define IS_MAJIN(ch)            (GET_RACE(ch) == Race::majin)
#define IS_KAI(ch)              (GET_RACE(ch) == Race::kai)
#define IS_TRUFFLE(ch)          (GET_RACE(ch) == Race::tuffle)
#define IS_HOSHIJIN(ch)         (GET_RACE(ch) == Race::hoshijin)
#define IS_ANIMAL(ch)           (GET_RACE(ch) == Race::animal)
#define IS_SAIBA(ch)              (GET_RACE(ch) == Race::saiba)
#define IS_SERPENT(ch)            (GET_RACE(ch) == Race::serpent)
#define IS_OGRE(ch)            (GET_RACE(ch) == Race::ogre)
#define IS_YARDRATIAN(ch)         (GET_RACE(ch) == Race::yardratian)
#define IS_ARLIAN(ch)           (GET_RACE(ch) == Race::arlian)
#define IS_DRAGON(ch)           (GET_RACE(ch) == Race::dragon)
#define IS_MECHANICAL(ch)          (GET_RACE(ch) == Race::mechanical)
#define IS_FAERIE(ch)           (GET_RACE(ch) == Race::spirit)
#define IS_UNDEAD(ch)           IS_AFFECTED(ch, AFF_UNDEAD)

/* Define Gender More Easily */
#define IS_MALE(ch)             (GET_SEX(ch) == SEX_MALE)
#define IS_FEMALE(ch)           (GET_SEX(ch) == SEX_FEMALE)
#define IS_NEUTER(ch)           (!IS_MALE(ch) && !IS_FEMALE(ch))

inline bool OUTSIDE_ROOMFLAG(Character *ch) {
    return !(ch->location.getRoomFlag(ROOM_INDOORS) || ch->location.getRoomFlag(ROOM_UNDERGROUND) || ch->location.getWhereFlag(WhereFlag::space));
}

inline bool OUTSIDE_SECTTYPE(Character *ch) {
    auto sect = ch->location.getTileType();
    switch(sect) {
        case SECT_INSIDE:
        case SECT_UNDERWATER:
        case SECT_IMPORTANT:
        case SECT_SHOP:
        case SECT_SPACE:
            return false;
    }
    return true;
}

#define OUTSIDE(ch)    (OUTSIDE_ROOMFLAG(ch) && OUTSIDE_SECTTYPE(ch))

inline bool DIRT_ROOM(Character *ch) {
    auto sect = ch->location.getTileType();
    switch(sect) {
        case SECT_INSIDE:
        case SECT_UNDERWATER:
        case SECT_IMPORTANT:
        case SECT_SHOP:
        case SECT_SPACE:
        case SECT_WATER_NOSWIM:
        case SECT_WATER_SWIM:
            return false;
    }
    return true;
}

#define SPEAKING(ch)     ch->getBaseStat<int>("speaking")

/* returns the number of spells per slot */
#define GET_BAB(ch)        GET_POLE_BONUS(ch)
#define GET_GAUNTLET(ch)    ch->getBaseStat<int>("gauntlet")

#define MOB_LOADROOM(ch) (ch->registeredLocations.contains("spawn") ? ch->registeredLocations.at("spawn") : Location())

extern void update_pos(Character *victim);

extern void demon_refill_lf(Character *ch, int64_t num);

extern void dispel_ash(Character *ch);

extern int mob_respond(Character *ch, Character *vict, const char *speech);

extern int armor_evolve(Character *ch);

extern int has_group(Character *ch);

const char *report_party_health(Character *ch);

extern int know_skill(Character *ch, int skill);

extern void null_affect(Character *ch, AffectFlag aff_flag);

extern void assign_affect(Character *ch, AffectFlag aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis,
              int spd);

extern int sec_roll_check(Character *ch);


extern int64_t physical_cost(Character *ch, int skill);

const char *disp_align(Character *ch);

extern void sense_memory_write(Character *ch, Character *vict);

extern int read_sense_memory(Character *ch, Character *vict);

extern int roll_pursue(Character *ch, Character *vict);

extern const char *sense_location(Character *ch);

extern void handle_evolution(Character *ch, int64_t dmg);

extern int64_t molt_threshold(Character *ch);

extern void purge_homing(Character *ch);

extern int planet_check(Character *ch, Character *vict);

extern void improve_skill(Character *ch, int skill, int num);

extern int64_t gear_exp(Character *ch, int64_t exp);

extern char *introd_calc(Character *ch);

extern int num_followers_charmed(Character *ch);

extern void die_follower(Character *ch);

extern void add_follower(Character *ch, Character *leader);

extern void stop_follower(Character *ch);

extern bool circle_follow(Character *ch, Character *victim);

/* in act.item.c */
extern int64_t max_carry_weight(Character *ch);

/* in limits.c */
extern void advance_level(Character *ch);

extern void set_title(Character *ch, char *title);

extern void gain_condition(Character *ch, int condition, int value);

extern void admin_set(Character *ch, int value);

extern bool spar_friendly(Character *ch, Character *npc);

extern void doContinuedTask(Character* ch);

extern void mob_talk(Character *ch, const char *speech);

extern int block_calc(Character *ch);

extern void reveal_hiding(Character *ch, int type);

extern void racial_body_parts(Character *ch);

extern void set_height_and_weight_by_race(Character *ch);
extern int get_size(Character *ch);

extern void init_char(Character *ch);

Character *read_mobile(mob_vnum nr, int type);

extern void reset_char(Character *ch);