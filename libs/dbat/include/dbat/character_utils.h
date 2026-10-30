#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "characters.h"
#include "flags.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/sex.h"
#include "consts/prefflags.h"
#include "consts/playerflags.h"
#include "consts/adminflags.h"
#include "consts/mobflags.h"

// Legacy Macros

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("OHNO: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)	((ch)->act)
#define PLR_FLAGS(ch)	((ch)->act)
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pref))
#define AFF_FLAGS(ch)	((ch)->affected_by)
#define ADM_FLAGS(ch)	((ch)->admflags)

/* Return the gauntlet highest room for ch */ 
#define GET_GAUNTLET(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->gauntlet))

bool char_is_npc(const struct char_data *ch);
bool char_is_mob(const struct char_data *ch);

bool char_mob_flagged(const struct char_data *ch, int flag);
bool char_plr_flagged(const struct char_data *ch, int flag);
bool char_aff_flagged(const struct char_data *ch, int flag);
bool char_prf_flagged(const struct char_data *ch, int flag);
bool char_adm_flagged(const struct char_data *ch, int flag);
bool char_body_flagged(const struct char_data *ch, int flag);
bool char_is_affected(const struct char_data *ch, int skill);

bool char_plr_toggle(struct char_data *ch, int flag);
bool char_prf_toggle(struct char_data *ch, int flag);
bool char_aff_toggle(struct char_data *ch, int flag);

/* new define for quick check */
bool char_is_dead(const struct char_data *ch);


#define IN_ROOM(ch)	((ch)->in_room)
zone_vnum char_in_zone(const struct char_data *ch);
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_PC_NAME(ch)	((ch)->name)
char *char_get_name(const struct char_data *ch);
char *char_get_title(const struct char_data *ch);
#define GET_USER_TITLE(d) ((d)->title)
#define GET_PHASE(ch)   ((ch)->starphase)
#define GET_MIMIC(ch)   ((ch)->mimic)
#define GET_VOICE(ch)   ((ch)->voice)
#define GET_CLAN(ch)    ((ch)->clan)
#define GET_TRANSCLASS(ch) ((ch)->transclass)
#define GET_FEATURE(ch) ((ch)->feature)
char *char_get_user(const struct char_data *ch);
#define GET_LOG_USER(ch) ((ch)->loguser)
#define GET_CRANK(ch)   ((ch)->crank)
#define GET_ADMLEVEL(ch)	((ch)->admlevel)
#define GET_CLASS_LEVEL(ch)	((ch)->level)
#define GET_LEVEL_ADJ(ch)	((ch)->level_adj)
#define GET_HITDICE(ch)		((ch)->race_level)
int char_get_level(const struct char_data *ch);
#define GET_PFILEPOS(ch)((ch)->pfilepos)

int char_get_class(const struct char_data *ch);
#define GET_CLASS_NONEPIC(ch, whichclass) ((ch)->chclasses[whichclass])
#define GET_CLASS_EPIC(ch, whichclass) ((ch)->epicclasses[whichclass])
int char_get_class_ranks(const struct char_data *ch, int whichclass);
#define GET_RACE(ch)    ((ch)->race)
#define GET_HAIRL(ch)   ((ch)->hairl)
#define GET_HAIRC(ch)   ((ch)->hairc)
#define GET_HAIRS(ch)   ((ch)->hairs)
#define GET_SKIN(ch)    ((ch)->skin)
#define GET_EYE(ch)     ((ch)->eye)
#define GET_DISTFEA(ch) ((ch)->distfea)
#define GET_HOME(ch)	((ch)->hometown)
#define GET_WEIGHT(ch)  ((ch)->weight)
#define GET_HEIGHT(ch)  ((ch)->height)
int char_get_pc_height(struct char_data *ch);
int char_get_pc_weight(struct char_data *ch);
#define GET_SEX(ch)	((ch)->sex)
#define GET_TLEVEL(ch)	((ch)->player_specials->tlevel)
#define CARRYING(ch)    ((ch)->player_specials->carrying)
#define CARRIED_BY(ch)  ((ch)->player_specials->carried_by)
#define RACIAL_PREF(ch) ((ch)->player_specials->racial_pref)
#define GET_RP(ch)      ((ch)->rp)
#define GET_RBANK(ch)   ((ch)->rbank)
#define GET_TRP(ch)     ((ch)->trp)
#define GET_SUPPRESS(ch) ((ch)->suppression)
#define GET_SUPP(ch)    ((ch)->suppressed)
#define GET_RDISPLAY(ch) ((ch)->rdisplay)

#define GET_STR(ch)     ((ch)->aff_abils.str)
/*
 * We could define GET_ADD to be ((GET_STR(ch) > 18) ?
 *                                ((GET_STR(ch) - 18) * 10) : 0)
 * but it's better to leave it undefined and fix the places that call
 * GET_ADD to use the new semantics for abilities.
 *                               - Elie Rosenblum 13/Dec/2003
 */
/* The old define: */
/* #define GET_ADD(ch)     ((ch)->aff_abils.str_add) */
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

double char_get_mutboost(struct char_data *ch);
double char_get_speedi(struct char_data *ch);
double char_get_speedcalc(struct char_data *ch);
double char_get_speedbonus(struct char_data *ch);
double char_get_speedvar(struct char_data *ch);
double char_get_speedvem(struct char_data *ch);
bool char_is_grap(const struct char_data *ch);
long char_get_speedint(struct char_data *ch);
bool char_is_inferior(const struct char_data *ch);
bool char_is_weighted(struct char_data *ch);


#define GET_EXP(ch)	  ((ch)->exp)
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
bool char_is_spoiled(const struct char_data *ch);
#define GET_DEATH_TYPE(ch) ((ch)->death_type)
#define GET_SLEEPT(ch)    ((ch)->sleeptime)
#define GET_FOODR(ch)     ((ch)->foodr)
#define GET_ALT(ch)       ((ch)->altitude)
#define GET_CHARGE(ch)    ((ch)->charge)
#define GET_CHARGETO(ch)  ((ch)->chargeto)
#define GET_ARMOR(ch)     ((ch)->armor)
#define GET_ARMOR_LAST(ch) ((ch)->armor_last)
#define GET_HIT(ch)	      (getCurPL(ch))
#define GET_MAX_HIT(ch)	  (getEffMaxPL(ch))
#define GET_MAX_MOVE(ch)  (getMaxST(ch))
#define GET_MAX_MANA(ch)  (getMaxKI(ch))
#define GET_KI(ch)	  ((ch)->ki)
#define GET_MAX_KI(ch)    ((ch)->max_ki)
#define GET_DROOM(ch)     ((ch)->droom)
#define GET_OVERFLOW(ch)  ((ch)->overf)
#define GET_SPAM(ch)      ((ch)->spam)
#define GET_SHIP(ch)      ((ch)->ship)
#define GET_SHIPROOM(ch)  ((ch)->shipr)
#define GET_LPLAY(ch)     ((ch)->lastpl)
#define GET_DTIME(ch)     ((ch)->deathtime)
#define GET_RTIME(ch)     ((ch)->rewtime)
#define GET_DCOUNT(ch)    ((ch)->dcount)
#define GET_BOARD(ch, i)  ((ch)->lboard[i])
#define GET_LIMBS(ch, i)  ((ch)->limbs[i])
// why is this i-1? Because whoever wrote it didn't know how C arrays work.
// We'll be replacing the limb system anyways so fuck this macro.
#define GET_LIMBCOND(ch, i) ((ch)->limb_condition[i-1])
#define GET_SONG(ch)      ((ch)->powerattack)
#define GET_BONUS(ch, i)  ((ch)->bonuses[i])
#define GET_TRANSCOST(ch, i) ((ch)->transcost[i-1])
#define GET_CCPOINTS(ch)  ((ch)->ccpoints)
#define GET_NEGCOUNT(ch)  ((ch)->negcount)
#define GET_GENOME(ch, i)    ((ch)->genome[i])
bool char_has_genome(const struct char_data *ch, int i);
#define COMBO(ch)         ((ch)->combo)
#define LASTATK(ch)       ((ch)->lastattack)
#define COMBHITS(ch)      ((ch)->combhits)
#define GET_AURA(ch)      ((ch)->aura)
#define GET_RADAR1(ch)    ((ch)->radar1)
#define GET_RADAR2(ch)    ((ch)->radar2)
#define GET_RADAR3(ch)    ((ch)->radar3)
#define GET_PING(ch)      ((ch)->ping)
#define GET_SLOTS(ch)     ((ch)->skill_slots)
#define GET_TGROWTH(ch)   ((ch)->tail_growth)
#define GET_RMETER(ch)    ((ch)->rage_meter)
#define GET_PERSONALITY(ch) ((ch)->personality)
#define GET_COMBINE(ch)   ((ch)->combine)
#define GET_PREFERENCE(ch) ((ch)->preference)
#define GET_RELAXCOUNT(ch) ((ch)->relax_count)
#define GET_BLESSLVL(ch)  ((ch)->blesslvl)
#define GET_ASB(ch)       ((ch)->asb)
#define GET_REGEN(ch)     ((ch)->regen)
double char_get_blessbonus(struct char_data *ch);
double char_get_poself(struct char_data *ch);
double char_get_posebonus(struct char_data *ch);
double char_get_lifebonus(struct char_data *ch);
double char_get_lifebonuses(struct char_data *ch);
#define GET_LIFEPERC(ch)  ((ch)->lifeperc)
#define GET_STUPIDKISS(ch) ((ch)->stupidkiss)
#define GET_SPEEDBOOST(ch) ((ch)->speedboost)
#define GET_BACKSTAB_COOL(ch) ((ch)->backstabcool)
#define GET_COOLDOWN(ch)  ((ch)->con_cooldown)
#define GET_BARRIER(ch)   ((ch)->barrier)
#define GET_GOLD(ch)	  ((ch)->gold)
#define GET_KAIOKEN(ch)   ((ch)->kaioken)
#define GET_BOOSTS(ch)    ((ch)->boosts)
#define MAJINIZED(ch)     ((ch)->majinize)
#define GET_MAJINIZED(ch) ((ch)->majinizer)
#define GET_FURY(ch)      ((ch)->fury)
#define GET_BTIME(ch)     ((ch)->btime)
#define GET_UP(ch)        ((ch)->upgrade)
#define GET_FORGETING(ch) ((ch)->forgeting)
#define GET_FORGET_COUNT(ch) ((ch)->forgetcount)
#define GET_BANK_GOLD(ch) ((ch)->bank_gold)
#define GET_POLE_BONUS(ch) ((ch)->accuracy)
#define GET_FISHSTATE(ch)  ((ch)->fishstate)
#define GET_FISHD(ch)     ((ch)->accuracy_mod)
#define GET_DAMAGE_MOD(ch) ((ch)->damage_mod)
#define GET_SPELLFAIL(ch) ((ch)->spellfail)
#define GET_ARMORCHECK(ch) ((ch)->armorcheck)
#define GET_ARMORCHECKALL(ch) ((ch)->armorcheckall)
#define GET_MOLT_EXP(ch)  ((ch)->moltexp)
#define GET_MOLT_LEVEL(ch) ((ch)->moltlevel)
#define GET_SDCOOLDOWN(ch) ((ch)->con_sdcooldown)
#define GET_INGESTLEARNED(ch) ((ch)->ingestLearned)
#define GET_POS(ch)		((ch)->position)
#define GET_IDNUM(ch)		((ch)->idnum)
#define GET_ID(x)		((x)->id)
#define IS_CARRYING_W(ch)	((ch)->carry_weight)
#define IS_CARRYING_N(ch)	((ch)->carry_items)
#define FIGHTING(ch)		((ch)->fighting)
#define GET_POWERATTACK(ch)	((ch)->powerattack)
#define GET_GROUPKILLS(ch)	((ch)->combatexpertise)
#define GET_SAVE_BASE(ch, i)	((ch)->saving_throw[i])
#define GET_SAVE_MOD(ch, i)	((ch)->apply_saving_throw[i])
int char_get_save(const struct char_data *ch, int i);
#define GET_ALIGNMENT(ch)	((ch)->alignment)
#define GET_ETHIC_ALIGNMENT(ch)	((ch)->alignment_ethic)
#define SITS(ch)                ((ch)->sits)
#define MINDLINK(ch)            ((ch)->mindlink)
#define LINKER(ch)              ((ch)->linker)
#define LASTHIT(ch)             ((ch)->lasthit)
#define DRAGGING(ch)            ((ch)->drag)
#define DRAGGED(ch)             ((ch)->dragged)
#define GRAPPLING(ch)           ((ch)->grappling)
#define GRAPPLED(ch)            ((ch)->grappled)
#define GRAPTYPE(ch)            ((ch)->grap)
#define GET_ORIGINAL(ch)        ((ch)->original)
#define GET_CLONES(ch)          ((ch)->clones)
#define GET_DEFENDER(ch)        ((ch)->defender)
#define GET_DEFENDING(ch)       ((ch)->defending)
#define BLOCKS(ch)              ((ch)->blocks)
#define BLOCKED(ch)             ((ch)->blocked)
#define ABSORBING(ch)           ((ch)->absorbing)
#define ABSORBBY(ch)            ((ch)->absorbby)
#define GET_EAVESDROP(ch)       ((ch)->listenroom)
#define GET_EAVESDIR(ch)        ((ch)->eavesdir)
#define GET_ABSORBS(ch)         ((ch)->absorbs)
#define GET_LINTEREST(ch)       ((ch)->lastint)

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_room))
#define GET_PRACTICES(ch,cl)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->class_skill_points[cl]))
#define GET_RACE_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->skill_points))
#define GET_TRAINS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->ability_trains))
#define GET_TRAINSTR(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainstr))
#define GET_TRAININT(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainint))
#define GET_TRAINCON(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->traincon))
#define GET_TRAINWIS(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainwis))
#define GET_TRAINAGL(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainagl))
#define GET_TRAINSPD(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainspd))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_OLC_ZONE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->olc_zone))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_HOST(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))
#define GET_HISTORY(ch, i)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->comm_hist[i]))

#define GET_SKILL_BONUS(ch, i)		(ch->skills[i].mod)
#define GET_SKILL_PERF(ch, i)           (ch->skills[i].perf)
#define SET_SKILL_BONUS(ch, i, value)	do { (ch)->skills[i].mod = value; } while (0)
#define SET_SKILL_PERF(ch, i, value)    do { (ch)->skills[i].perf = value; } while (0)
#define GET_SKILL_BASE(ch, i)		(ch->skills[i].base)
int char_get_skill(const struct char_data *ch, int i);
#define SET_SKILL(ch, i, val)		do { (ch)->skills[i].base = val; } while(0)
#define BODY_PARTS(ch)  ((ch)->bodyparts)

#define GET_EQ(ch, i)		((ch)->equipment[i])

spec_proc_func char_get_mob_spec(const struct char_data *ch);
#define GET_MOB_RNUM(mob)	((mob)->nr)
mob_vnum char_get_mob_vnum(const struct char_data *mob);

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)
#define MOB_COOLDOWN(ch)        ((ch)->cooldown)

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

#define CAN_CARRY_W(ch) (getMaxCarryWeight(ch))
#define CAN_CARRY_N(ch) (50)
bool char_is_awake(const struct char_data *ch);
bool char_can_see_in_dark(const struct char_data *ch);

bool char_is_good(const struct char_data *ch);
bool char_is_evil(const struct char_data *ch);
bool char_is_lawful(const struct char_data *ch);
bool char_is_chaotic(const struct char_data *ch);
bool char_is_neutral(const struct char_data *ch);
int char_align_type(const struct char_data *ch);

bool char_in_arena(const struct char_data *ch);
#define ARENA_IDNUM(ch) ((ch)->arenawatch)

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)

#define SENDOK(ch)    (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || char_is_awake(ch)) && \
                      !char_plr_flagged((ch), PLR_WRITING))


bool char_light_ok(const struct char_data *sub);

bool char_invis_ok(const struct char_data *sub, const struct char_data *obj);

bool char_mort_can_see(const struct char_data *sub, const struct char_data *obj);

bool char_imm_can_see(const struct char_data *sub, const struct char_data *obj);

bool char_is_self(const struct char_data *sub, const struct char_data *obj);

/* Can subject see character "obj"? */
bool char_can_see(const struct char_data *sub, const struct char_data *obj);

bool char_not_hidden(const struct char_data *ch);
/* End of CAN_SEE */


bool char_invis_ok_obj(const struct char_data *sub, const struct obj_data *obj);

/* Is anyone carrying this object and if so, are they visible? */
bool char_can_see_obj_carrier(const struct char_data *sub, const struct obj_data *obj);

bool char_mort_can_see_obj(const struct char_data *sub, const struct obj_data *obj);

bool char_can_see_obj(const struct char_data *sub, const struct obj_data *obj);

bool char_can_carry_obj(struct char_data *ch, struct obj_data *obj);

bool char_can_get_obj(struct char_data *ch, struct obj_data *obj);

bool char_disg(const struct char_data *ch, const struct char_data *vict);

bool char_introd(struct char_data *ch, struct char_data *vict);

bool char_iswiz(const struct char_data *ch, const struct char_data *vict);

const char *char_pers(struct char_data *ch, struct char_data *vict);

char *obj_shortdesc_for(const struct obj_data *obj, const struct char_data *vict);

char *obj_fname_for(const struct obj_data *obj, const struct char_data *vict);

#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door]) 
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])


bool char_can_go(const struct char_data *ch, int door);

#define RACE(ch)      (juggleRaceName(ch, true))
#define LRACE(ch)     (juggleRaceName(ch, false))
#define TRUE_RACE(ch) (pc_race_types[ch->race])
#define SENSEI_NAME(ch) (pc_class_types[ch->chclass])
#define SENSEI_NAME_LOWER(ch) (class_names[ch->chclass])
#define SENSEI_STYLE(ch) (sensei_style[ch->chclass])

#define CLASS_ABBR(ch) (class_abbrevs[ch->chclass])
#define RACE_ABBR(ch) (race_abbrevs[ch->race])

bool char_is_roshi(const struct char_data *ch);
bool char_is_piccolo(const struct char_data *ch);
bool char_is_krane(const struct char_data *ch);
bool char_is_nail(const struct char_data *ch);
bool char_is_bardock(const struct char_data *ch);
bool char_is_ginyu(const struct char_data *ch);
bool char_is_frieza(const struct char_data *ch);
bool char_is_tapion(const struct char_data *ch);
bool char_is_andsix(const struct char_data *ch);
bool char_is_dabura(const struct char_data *ch);
bool char_is_kabito(const struct char_data *ch);
bool char_is_jinto(const struct char_data *ch);
bool char_is_tsuna(const struct char_data *ch);
bool char_is_kurzak(const struct char_data *ch);

bool char_is_assassin(const struct char_data *ch);
bool char_is_blackguard(const struct char_data *ch);
bool char_is_dragon_disciple(const struct char_data *ch);
bool char_is_duelist(const struct char_data *ch);
bool char_is_dwarven_defender(const struct char_data *ch);
bool char_is_eldritch_knight(const struct char_data *ch);
bool char_is_hierophant(const struct char_data *ch);
bool char_is_horizon_walker(const struct char_data *ch);
bool char_is_loremaster(const struct char_data *ch);
bool char_is_mystic_theurge(const struct char_data *ch);
bool char_is_shadowdancer(const struct char_data *ch);
bool char_is_thaumaturgist(const struct char_data *ch);


int char_gold_carry(const struct char_data *ch);
bool char_is_shadow_dragon1(const struct char_data *ch);
bool char_is_shadow_dragon2(const struct char_data *ch);
bool char_is_shadow_dragon3(const struct char_data *ch);
bool char_is_shadow_dragon4(const struct char_data *ch);
bool char_is_shadow_dragon5(const struct char_data *ch);
bool char_is_shadow_dragon6(const struct char_data *ch);
bool char_is_shadow_dragon7(const struct char_data *ch);
bool char_can_grand_master(const struct char_data *ch);
bool char_is_humanoid(const struct char_data *ch);
bool char_is_restricted_race(const struct char_data *ch);
bool char_is_cheap_race(const struct char_data *ch);
bool char_spar_train(const struct char_data *ch);
bool char_is_nonptrans(const struct char_data *ch);
bool char_is_fullpssj(const struct char_data *ch);
bool char_is_transformed(const struct char_data *ch);
#define BIRTH_PHASE             (time_info.day <= 15)
#define LIFE_PHASE              (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE             (!BIRTH_PHASE && !LIFE_PHASE)
bool char_moon_ok(const struct char_data *ch);
bool char_oozaru_ok(const struct char_data *ch);
bool char_is_oozaru_race(const struct char_data *ch);
#define MOON_TIME               (time_info.hours >= 21 || time_info.hours <= 4)
#define MOON_DATE               (time_info.day == 19 || time_info.day == 20 || time_info.day == 21)
bool MOON_TIMECHECK();
bool char_ether_stream(const struct char_data *ch);
bool char_has_moon(const struct char_data *ch);
bool char_has_arms(const struct char_data *ch);
bool char_has_legs(const struct char_data *ch);

bool char_is_human(const struct char_data *ch);
bool char_is_saiyan(const struct char_data *ch);
bool char_is_icer(const struct char_data *ch);
bool char_is_konatsu(const struct char_data *ch);
bool char_is_namek(const struct char_data *ch);
bool char_is_mutant(const struct char_data *ch);
bool char_is_kanassan(const struct char_data *ch);
bool char_is_halfbreed(const struct char_data *ch);
bool char_is_bio(const struct char_data *ch);
bool char_is_android(const struct char_data *ch);
bool char_is_demon(const struct char_data *ch);
bool char_is_majin(const struct char_data *ch);
bool char_is_kai(const struct char_data *ch);
bool char_is_truffle(const struct char_data *ch);
bool char_is_hoshijin(const struct char_data *ch);
bool char_is_animal(const struct char_data *ch);
bool char_is_saiba(const struct char_data *ch);
bool char_is_serpent(const struct char_data *ch);
bool char_is_ogre(const struct char_data *ch);
bool char_is_yardratian(const struct char_data *ch);
bool char_is_arlian(const struct char_data *ch);
bool char_is_dragon(const struct char_data *ch);
bool char_is_mechanical(const struct char_data *ch);
bool char_is_undead(const struct char_data *ch);

bool char_is_male(const struct char_data *ch);
bool char_is_female(const struct char_data *ch);

bool char_is_outside(const struct char_data *ch);

bool char_outside_roomflag(const struct char_data *ch);

bool char_outside_secttype(const struct char_data *ch);


#define SPEAKING(ch)     ((ch)->player_specials->speaking)

char *char_hshr(const struct char_data *ch);
char *char_hssh(const struct char_data *ch);
char *char_hmhr(const struct char_data *ch);
char *char_mafe(const struct char_data *ch);


#define GET_SPELLMEM(ch, i)	((ch->player_specials->spellmem[i]))
#define GET_MEMCURSOR(ch)	((ch->player_specials->memcursor))
/* returns the number of spells per slot */
#define GET_SPELL_LEVEL(ch, i)	((ch)->player_specials->spell_level[i])
#define HAS_FEAT(ch, i)		((ch)->feats[i])
bool char_has_combat_feat(const struct char_data *ch, int i, int j);
#define SET_COMBAT_FEAT(ch,i,j)	SET_BIT_AR((ch)->combat_feats[(i)], (j))
bool char_has_school_feat(const struct char_data *ch, int i, bitvector_t j);
#define SET_SCHOOL_FEAT(ch,i,j)	SET_BIT((ch)->school_feats[(i)], (j))
#define GET_BAB(ch)		GET_POLE_BONUS(ch)
#define SET_FEAT(ch, i, value)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->feats[i]) = value; } while(0)
#define GET_SPELL_MASTERY_POINTS(ch) \
				(ch->player_specials->spell_mastery_points)
#define GET_FEAT_POINTS(ch)	(ch->player_specials->feat_points)
#define GET_EPIC_FEAT_POINTS(ch) \
				(ch->player_specials->epic_feat_points)
#define GET_CLASS_FEATS(ch,cl)	(ch->player_specials->class_feat_points[cl])
#define GET_EPIC_CLASS_FEATS(ch,cl) \
				(ch->player_specials->epic_class_feat_points[cl])

#define MOB_LOADROOM(ch)      ((ch)->hometown)  /*hometown not used for mobs*/
#define GET_MURDER(ch)          CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->murder))

#define GET_PAGE_LENGTH(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->page_length))

#include "dbat/consts/types.h"
#include "dbat/characters.h"
#include "dbat/log.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/weather.h"

int roll_stats(struct char_data *ch, int type, int bonus);
const char* juggleRaceName(char_data *ch, bool capitalized);
void restore_by(char_data *ch, char_data *healer);
void restore(char_data *ch, bool announce);
void resurrect(char_data *ch, int mode);
void ghostify(char_data *ch);
void teleport_to(char_data *ch, IDXTYPE rnum);
bool in_room_range(char_data *ch, IDXTYPE low_rnum, IDXTYPE high_rnum);
bool in_past(char_data *ch);
bool is_newbie(char_data *ch);
bool in_northran(char_data *ch);
bool can_tolerate_gravity(char_data *ch, int grav);
int calcTier(char_data *ch);
int64_t calc_soft_cap(char_data *ch);
bool is_soft_cap(char_data *ch, int64_t type, long double mult);
int wearing_android_canister(char_data *ch);
int calcGravCost(char_data *ch, int64_t num);

int64_t getCurHealth(char_data *ch);
int64_t getMaxHealth(char_data *ch);
double getCurHealthPercent(char_data *ch);
int64_t getPercentOfCurHealth(char_data *ch, double amt);
int64_t getPercentOfMaxHealth(char_data *ch, double amt);
bool isFullHealth(char_data *ch);
int64_t setCurHealth(char_data *ch, int64_t amt);
int64_t setCurHealthPercent(char_data *ch, double amt);
int64_t incCurHealth(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurHealth(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurHealthPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurHealthPercent(char_data *ch, double amt, int64_t floor);
void restoreHealth(char_data *ch, bool announce);
int64_t healCurHealth(char_data *ch, int64_t amt);
int64_t harmCurHealth(char_data *ch, int64_t amt);

int64_t getMaxPL(char_data *ch);
int64_t getMaxPLTrans(char_data *ch);
int64_t getCurPL(char_data *ch);
int64_t getBasePL(char_data *ch);
int64_t getEffBasePL(char_data *ch);
double getCurPLPercent(char_data *ch);
int64_t getPercentOfCurPL(char_data *ch, double amt);
int64_t getPercentOfMaxPL(char_data *ch, double amt);
bool isFullPL(char_data *ch);

int64_t getCurKI(char_data *ch);
int64_t getMaxKI(char_data *ch);
int64_t getBaseKI(char_data *ch);
int64_t getEffBaseKI(char_data *ch);
double getCurKIPercent(char_data *ch);
int64_t getPercentOfCurKI(char_data *ch, double amt);
int64_t getPercentOfMaxKI(char_data *ch, double amt);
bool isFullKI(char_data *ch);
int64_t setCurKI(char_data *ch, int64_t amt);
int64_t setCurKIPercent(char_data *ch, double amt);
int64_t incCurKI(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurKI(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurKIPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurKIPercent(char_data *ch, double amt, int64_t floor);
void restoreKI(char_data *ch, bool announce);

int64_t getCurST(char_data *ch);
int64_t getMaxST(char_data *ch);
int64_t getBaseST(char_data *ch);
int64_t getEffBaseST(char_data *ch);
double getCurSTPercent(char_data *ch);
int64_t getPercentOfCurST(char_data *ch, double amt);
int64_t getPercentOfMaxST(char_data *ch, double amt);
bool isFullST(char_data *ch);
int64_t setCurST(char_data *ch, int64_t amt);
int64_t setCurSTPercent(char_data *ch, double amt);
int64_t incCurST(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurST(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurSTPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurSTPercent(char_data *ch, double amt, int64_t floor);
void restoreST(char_data *ch, bool announce);

int64_t getCurLF(char_data *ch);
int64_t getMaxLF(char_data *ch);
double getCurLFPercent(char_data *ch);
int64_t getPercentOfCurLF(char_data *ch, double amt);
int64_t getPercentOfMaxLF(char_data *ch, double amt);
bool isFullLF(char_data *ch);
int64_t setCurLF(char_data *ch, int64_t amt);
int64_t setCurLFPercent(char_data *ch, double amt);
int64_t incCurLF(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurLF(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurLFPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurLFPercent(char_data *ch, double amt, int64_t floor);
void restoreLF(char_data *ch, bool announce);

bool isFullVitals(char_data *ch);
void restoreVitals(char_data *ch, bool announce);
void restoreStatus(char_data *ch, bool announce);
void restoreLimbs(char_data *ch, bool announce);

int64_t gainBasePL(char_data *ch, int64_t amt, bool trans_mult);
int64_t gainBaseKI(char_data *ch, int64_t amt, bool trans_mult);
int64_t gainBaseST(char_data *ch, int64_t amt, bool trans_mult);
void gainBaseAll(char_data *ch, int64_t amt, bool trans_mult);

int64_t loseBasePL(char_data *ch, int64_t amt, bool trans_mult);
int64_t loseBaseKI(char_data *ch, int64_t amt, bool trans_mult);
int64_t loseBaseST(char_data *ch, int64_t amt, bool trans_mult);
void loseBaseAll(char_data *ch, int64_t amt, bool trans_mult);

int64_t gainBasePLPercent(char_data *ch, double amt, bool trans_mult);
int64_t gainBaseKIPercent(char_data *ch, double amt, bool trans_mult);
int64_t gainBaseSTPercent(char_data *ch, double amt, bool trans_mult);
void gainBaseAllPercent(char_data *ch, double amt, bool trans_mult);

int64_t loseBasePLPercent(char_data *ch, double amt, bool trans_mult);
int64_t loseBaseKIPercent(char_data *ch, double amt, bool trans_mult);
int64_t loseBaseSTPercent(char_data *ch, double amt, bool trans_mult);
void loseBaseAllPercent(char_data *ch, double amt, bool trans_mult);

void cureStatusKnockedOut(char_data *ch, bool announce);
void cureStatusBurn(char_data *ch, bool announce);
void cureStatusPoison(char_data *ch, bool announce);
void setStatusKnockedOut(char_data *ch);

int64_t getMaxCarryWeight(char_data *ch);
int64_t getCurGearWeight(char_data *ch);
int64_t getCurCarriedWeight(char_data *ch);
int64_t getAvailableCarryWeight(char_data *ch);

double speednar(char_data *ch);
int64_t getEffMaxPL(char_data *ch);
bool isWeightedPL(char_data *ch);

void apply_kaioken(char_data *ch, int times, bool announce);
void remove_kaioken(char_data *ch, int8_t announce);

void dispel_ash(struct char_data *ch);
int has_group(struct char_data *ch);
const char *report_party_health(struct char_data *ch);
int sec_roll_check(struct char_data *ch);
int get_measure(struct char_data *ch, int height, int weight);
int trans_cost(struct char_data *ch, int trans);
int trans_req(struct char_data *ch, int trans);
const char *disp_align(struct char_data *ch);
void senseCreate(struct char_data *ch);
int read_sense_memory(struct char_data *ch, struct char_data *vict);
void sense_memory_write(struct char_data *ch, struct char_data *vict);
int roll_pursue(struct char_data *ch, struct char_data *vict);
void reveal_hiding(struct char_data *ch, int type);
int block_calc(struct char_data *ch);
int64_t molt_threshold(struct char_data *ch);
int armor_evolve(struct char_data *ch);
void handle_evolution(struct char_data *ch, int64_t dmg);
void demon_refill_lf(struct char_data *ch, int64_t num);
void mob_talk(struct char_data *ch, const char *speech);
int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech);
bool is_sparring(struct char_data *ch);
char *introd_calc(struct char_data *ch);
int64_t gear_exp(struct char_data *ch, int64_t exp);
void purge_homing(struct char_data *ch);
void improve_skill(struct char_data *ch, int skill, int num);
void stop_follower(struct char_data *ch);
int num_followers_charmed(struct char_data *ch);
void switch_leader(struct char_data *old, struct char_data *new_leader);
void die_follower(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
bool circle_follow(struct char_data *ch, struct char_data *victim);


void	advance_level(struct char_data *ch, int whichclass);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int64_t gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	update_pos(struct char_data *victim);

int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_remove(struct char_data *ch, int pos);

int can_grav(struct char_data *ch);
int check_skill(struct char_data *ch, int skill);
int check_points(struct char_data *ch, int64_t ki, int64_t st);
void pcost(struct char_data *ch, double ki, int64_t st);
int limb_ok(struct char_data *ch, int type);
int64_t physical_cost(struct char_data *ch, int skill);

int init_skill(struct char_data *ch, int snum);
int chance_to_hit(struct char_data *ch);
int handle_speed(struct char_data *ch, struct char_data *vict);
struct time_info_data *age(struct char_data *ch);
const char *get_i_name(struct char_data *ch, struct char_data *vict);

void assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd);
int know_skill(struct char_data *ch, int skill);

size_t send_to_char(struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void admin_set(struct char_data *ch, int value);
char *sense_location(struct char_data *ch);
void null_affect(struct char_data *ch, int aff_flag);
bool is_affected(struct char_data *ch, int aff_flag);
void remove_affect(struct char_data *ch, int aff_flag);
int planet_check(struct char_data *ch, struct char_data *vict);
/* Player autoexit levels: used as an index to exitlevels           */
#define EXIT_OFF        0       /* Autoexit off                     */
#define EXIT_NORMAL     1       /* Brief display (stock behaviour)  */
#define EXIT_NA         2       /* Not implemented - do not use     */
#define EXIT_COMPLETE   3       /* Full display                     */

int char_exit_level(const struct char_data *ch);

int char_get_bank_interest(const struct char_data *ch);

bool race_has_tail(int r_id);
void char_lose_tail(char_data *ch);
bool char_has_tail(char_data *ch);
void char_gain_tail(char_data *ch, bool announce);

room_vnum sensei_start_room(int s_id);
room_vnum sensei_location_id(int s_id);
int sensei_grav_tolerance(int s_id);
int sensei_rpp_cost(int s_id, int r_id);

int race_get_size(int r_id);
int race_is_playable(int r_id);
int race_is_people(int r_id);
