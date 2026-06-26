#pragma once
#include "character_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Legacy Macros

/* Player autoexit levels: used as an index to exitlevels           */
#define EXIT_OFF 0      /* Autoexit off                     */
#define EXIT_NORMAL 1   /* Brief display (stock behaviour)  */
#define EXIT_NA 2       /* Not implemented - do not use     */
#define EXIT_COMPLETE 3 /* Full display                     */

#define _exitlevel(ch)                                                         \
  (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_AUTOEXIT) ? 1 : 0) +                   \
                     (PRF_FLAGGED((ch), PRF_FULL_EXIT) ? 2 : 0)                \
               : 0)
#define EXIT_LEV(ch) (_exitlevel(ch))

#define GET_BANK_INTEREST(ch)                                                  \
  MIN(25000, (int)(((double)GET_BANK_GOLD((ch)) * 0.04)))

#define MOB_FLAGS(ch) ((ch)->act)
#define PLR_FLAGS(ch) ((ch)->act)
#define PRF_FLAGS(ch) ((ch)->pref)
#define AFF_FLAGS(ch) ((ch)->affected_by)
#define ADM_FLAGS(ch) ((ch)->admflags)

#define IS_NPC(ch) (IS_SET_AR(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch) (IS_NPC(ch) && ch->proto_id != NOTHING)

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET_AR(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET_AR(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET_AR(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET_AR(PRF_FLAGS(ch), (flag)))
#define ADM_FLAGGED(ch, flag) char_admflagged(ch, flag)
#define BODY_FLAGGED(ch, flag) (IS_SET_AR(BODY_PARTS(ch), (flag)))
#define IS_AFFECTED(ch, skill) is_affected(ch, skill)

#define PLR_TOG_CHK(ch, flag)                                                  \
  ((TOGGLE_BIT_AR(PLR_FLAGS(ch), (flag))) & Q_BIT(flag))
#define PRF_TOG_CHK(ch, flag)                                                  \
  ((TOGGLE_BIT_AR(PRF_FLAGS(ch), (flag))) & Q_BIT(flag))
#define ADM_TOG_CHK(ch, flag) char_admflag_toggle(ch, flag)
#define AFF_TOG_CHK(ch, flag)                                                  \
  ((TOGGLE_BIT_AR(AFF_FLAGS(ch), (flag))) & Q_BIT(flag))

/* new define for quick check */
#define DEAD(ch)                                                               \
  (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))

#define IN_ROOM(ch) ((ch)->in_room)
#define IN_ZONE(ch) char_zone_vnum_get(ch)
#define GET_WAS_IN(ch) ((ch)->was_in_room)
#define GET_AGE(ch) (age(ch)->year)

#define GET_PC_NAME(ch) ((ch)->name)
#define GET_NAME(ch) (IS_NPC(ch) ? (ch)->short_descr : GET_PC_NAME(ch))
#define GET_TITLE(ch)                                                          \
  ((ch)->desc ? ((ch)->desc->title ? (ch)->desc->title : "[Unset Title]")      \
              : "@D[@GNew User@D]")
#define GET_USER_TITLE(d) ((d)->title)
#define GET_PHASE(ch) ((ch)->starphase)
#define GET_MIMIC(ch) ((ch)->mimic)
#define GET_VOICE(ch) ((ch)->voice)
#define GET_CLAN(ch) ((ch)->clan)
#define GET_TRANSCLASS(ch) ((ch)->transclass)
#define GET_FEATURE(ch) ((ch)->feature)
#define GET_USER(ch)                                                           \
  ((ch)->desc ? ((ch)->desc->user ? (ch)->desc->user : "NOUSER") : "NOUSER")
#define GET_LOG_USER(ch) ((ch)->loguser)
#define GET_CRANK(ch) ((ch)->crank)
#define GET_ADMLEVEL(ch) ((ch)->admlevel)
#define GET_CLASS_LEVEL(ch) char_stat_get((ch), "level")
#define GET_LEVEL(ch) char_stat_get((ch), "level")
#define GET_PFILEPOS(ch) ((ch)->pfilepos)

#define GET_CLASS(ch) ((ch)->chclass)
#define GET_RACE(ch) ((ch)->race)
#define GET_HAIRL(ch) ((ch)->hairl)
#define GET_HAIRC(ch) ((ch)->hairc)
#define GET_HAIRS(ch) ((ch)->hairs)
#define GET_SKIN(ch) ((ch)->skin)
#define GET_EYE(ch) ((ch)->eye)
#define GET_DISTFEA(ch) ((ch)->distfea)
#define GET_HOME(ch) ((ch)->hometown)
#define GET_WEIGHT(ch) char_der_total_get((ch), "weight")
#define GET_HEIGHT(ch) char_der_total_get((ch), "height")
#define GET_PC_HEIGHT(ch) char_der_total_get((ch), "height")
#define GET_PC_WEIGHT(ch) char_der_total_get((ch), "weight")
#define GET_SEX(ch) ((ch)->sex)
#define CARRYING(ch) char_carrying_char_get(ch)
#define CARRIED_BY(ch) char_carried_by_char_get(ch)
#define RACIAL_PREF(ch) ((ch)->racial_pref)
#define GET_RP(ch) ((ch)->rp)
#define GET_TRP(ch) ((ch)->trp)
#define GET_SUPPRESS(ch) char_stat_get((ch), "suppression")
#define GET_RDISPLAY(ch) ((ch)->rdisplay)

#define GET_STR(ch) char_der_total_get((ch), "strength")
#define GET_DEX(ch) char_der_total_get((ch), "agility")
#define GET_INT(ch) char_der_total_get((ch), "intelligence")
#define GET_WIS(ch) char_der_total_get((ch), "wisdom")
#define GET_CON(ch) char_der_total_get((ch), "constitution")
#define GET_CHA(ch) char_der_total_get((ch), "speed")

#define GET_SPEEDI(ch) char_der_total_get((ch), "speed_index")


#define IS_INFERIOR(ch) (IS_KONATSU(ch) || IS_DEMON(ch))
#define IS_WEIGHTED(ch) (getMaxPL(ch) < GET_MAX_HIT(ch))
#define IS_GRAP(ch) (GRAPPLING(ch) || GRAPPLED(ch))
#define GET_EXP(ch) char_stat_get((ch), "experience")
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
#define SPOILED(ch) ((ch)->time.played > 86400)
#define GET_DEATH_TYPE(ch) ((ch)->death_type)
#define GET_SLEEPT(ch) ((ch)->sleeptime)
#define GET_ALT(ch) char_condition_number_get(ch, "flying", "altitude")
#define GET_CHARGE(ch) char_charge_get(ch)
#define GET_ARMOR(ch) char_der_total_get((ch), "armor")
#define GET_HIT(ch) getCurPL(ch)
#define GET_MAX_HIT(ch) getMaxPL(ch)
#define GET_MAX_MOVE(ch) getMaxST(ch)
#define GET_MAX_MANA(ch) getMaxKI(ch)
#define GET_KI(ch) ((ch)->ki)
#define GET_MAX_KI(ch) ((ch)->max_ki)
#define GET_DROOM(ch) ((ch)->droom)
#define GET_SPAM(ch) ((ch)->spam)
#define GET_LPLAY(ch) ((ch)->lastpl)
#define GET_DTIME(ch) ((ch)->deathtime)
#define GET_RTIME(ch) ((ch)->rewtime)
#define GET_DCOUNT(ch) char_stat_get((ch), "death_count")
#define GET_BOARD(ch, i) ((ch)->lboard[i])
#define GET_LIMBCOND(ch, i) char_limbcond_get((ch), (i))
#define SET_LIMBCOND(ch, i, v) char_limbcond_set((ch), (i), (v))
/* GET_SONG removed — song tracking moved to mystic_melody condition */
#define GET_BONUS(ch, i) ((ch)->bonuses[i])
#define GET_TRANSCOST(ch, i) ((ch)->transcost[i - 1])
#define GET_CCPOINTS(ch) ((ch)->ccpoints)
#define GET_NEGCOUNT(ch) ((ch)->negcount)
#define GET_GENOME(ch, i) ((ch)->genome[i])
#define HAS_GENOME(ch, i) ((ch)->genome[0] == (i) || (ch)->genome[1] == (i))
#define LASTATK(ch) ((ch)->lastattack)

#define GET_AURA(ch) ((ch)->aura)
#define GET_RADAR1(ch) ((ch)->radar1)
#define GET_RADAR2(ch) ((ch)->radar2)
#define GET_RADAR3(ch) ((ch)->radar3)
#define GET_PING(ch) ((ch)->ping)
#define GET_SLOTS(ch) char_stat_get((ch), "skill_slots")
#define GET_RMETER(ch) ((ch)->rage_meter)
#define GET_PERSONALITY(ch) ((ch)->personality)
#define GET_COMBINE(ch) ((ch)->combine)
#define GET_PREFERENCE(ch) ((ch)->preference)
#define GET_ASB(ch) char_der_total_get((ch), "autoskill_bonus")
#define GET_REGEN(ch) char_der_total_get((ch), "regen_bonus")

#define GET_LIFEPERC(ch) char_stat_get((ch), "life_percent")
#define GET_BACKSTAB_COOL(ch) ((ch)->backstabcool)
#define GET_COOLDOWN(ch) ((ch)->con_cooldown)
#define GET_BARRIER(ch) char_barrier_get(ch)
#define GET_GOLD(ch) char_stat_get((ch), "money")
#define GET_KAIOKEN(ch) char_stat_get((ch), "kaioken")
#define GET_BOOSTS(ch)        ((int)char_stat_get((ch), "boosts"))
#define SET_BOOSTS(ch, v)     ((void)char_stat_set((ch), "boosts", (int64_t)(v)))
#define MOD_BOOSTS(ch, delta) ((void)char_stat_mod((ch), "boosts", (int64_t)(delta)))
#define MAJINIZED(ch) ((ch)->majinize)
#define GET_MAJINIZED(ch) ((ch)->majinizer)
#define GET_FURY(ch) char_stat_get((ch), "fury")
#define GET_UP(ch) char_stat_get((ch), "upgrades")
#define GET_BANK_GOLD(ch) char_stat_get((ch), "money_bank")
#define GET_POLE_BONUS(ch) char_legacy_modifier(ch, APPLY_ACCURACY, -1)
#define GET_SPELLFAIL(ch) ((ch)->spellfail)
#define GET_ARMORCHECK(ch) ((ch)->armorcheck)
#define GET_ARMORCHECKALL(ch) ((ch)->armorcheckall)
#define GET_MOLT_EXP(ch) char_stat_get((ch), "molt_experience")
#define GET_MOLT_LEVEL(ch) char_stat_get((ch), "molt_level")
#define GET_SDCOOLDOWN(ch) ((ch)->con_sdcooldown)
#define GET_INGESTLEARNED(ch) ((ch)->ingestLearned)
#define GET_POS(ch) char_position_get(ch)
#define GET_IDNUM(ch) ((ch)->idnum)
#define GET_ID(x) ((x)->id)
#define IS_CARRYING_W(ch) char_der_total_get(ch, "weight_carried")
#define IS_CARRYING_N(ch) char_inventory_count(ch, false)
#define FIGHTING(ch) char_fighting_get(ch)
#define GET_GROUPKILLS(ch) char_condition_number_get(ch, "group", "kills")
#define GET_ALIGNMENT(ch) char_stat_get((ch), "alignment")
#define GET_ETHIC_ALIGNMENT(ch) 0
#define SITS(ch) ((ch)->sits)
#define MINDLINK(ch) char_mindlinked_get(ch)
#define LINKER(ch) ((ch)->linker)
#define LASTHIT(ch) ((ch)->lasthit)
#define DRAGGING(ch) char_dragging_get(ch)
#define DRAGGED(ch) char_being_dragged_get(ch)
#define GRAPPLING(ch) char_grappling_get(ch)
#define GRAPPLED(ch) char_grappled_get(ch)
#define GRAPTYPE(ch) char_graptype_get(ch)
#define GET_ORIGINAL(ch) char_multiform_clone_get(ch)
#define GET_DEFENDER(ch) char_defending_for_get(ch)
#define GET_DEFENDING(ch) char_defended_by_get(ch)
#define BLOCKS(ch) char_blocking_get(ch)
#define BLOCKED(ch) char_blocked_by_get(ch)
#define ABSORBING(ch) char_absorbing_get(ch)
#define ABSORBBY(ch) char_absorbed_by_get(ch)
#define MASTER(ch) char_following_get(ch)
#define GET_EAVESDROP(ch) ((ch)->listenroom)
#define GET_EAVESDIR(ch) ((ch)->eavesdir)
#define GET_ABSORBS(ch) ((ch)->absorbs)
#define GET_LINTEREST(ch) ((ch)->lastint)

#define GET_LOADROOM(ch) ((ch)->load_room)
// This used to be a a macro that accessed an array, hence the ... that is
// ignored.
#define GET_PRACTICES(ch, ...) char_stat_get((ch), "practices")
#define GET_TRAINSTR(ch) char_stat_get((ch), "train_strength")
#define GET_TRAININT(ch) char_stat_get((ch), "train_intelligence")
#define GET_TRAINCON(ch) char_stat_get((ch), "train_constitution")
#define GET_TRAINWIS(ch) char_stat_get((ch), "train_wisdom")
#define GET_TRAINAGL(ch) char_stat_get((ch), "train_agility")
#define GET_TRAINSPD(ch) char_stat_get((ch), "train_speed")
#define GET_INVIS_LEV(ch) ((ch)->invis_level)
#define GET_WIMP_LEV(ch) ((ch)->wimp_level)
#define GET_FREEZE_LEV(ch) ((ch)->freeze_level)
#define GET_BAD_PWS(ch) ((ch)->bad_pws)
#define POOFIN(ch) ((ch)->poofin)
#define POOFOUT(ch) ((ch)->poofout)
#define GET_OLC_ZONE(ch) ((ch)->olc_zone)
#define GET_LAST_OLC_TARG(ch) ((ch)->last_olc_targ)
#define GET_LAST_OLC_MODE(ch) ((ch)->last_olc_mode)
#define GET_ALIASES(ch) ((ch)->aliases)
#define GET_LAST_TELL(ch) ((ch)->last_tell)
#define GET_HOST(ch) ((ch)->host)
#define GET_HISTORY(ch, i) ((ch)->comm_hist[i])

#define GET_SKILL_BONUS(ch, i) char_legacy_modifier(ch, APPLY_SKILL, i)
#define GET_SKILL_PERF(ch, i) (ch->skills[i].perf)
#define SET_SKILL_PERF(ch, i, value)                                           \
  do {                                                                         \
    (ch)->skills[i].perf = value;                                              \
  } while (0)
#define GET_SKILL_BASE(ch, i) (ch->skills[i].base)
#define GET_SKILL(ch, i) ((ch)->skills[i].base + GET_SKILL_BONUS(ch, i))
#define SET_SKILL(ch, i, val)                                                  \
  do {                                                                         \
    (ch)->skills[i].base = val;                                                \
  } while (0)
#define BODY_PARTS(ch) ((ch)->bodyparts)

#define GET_EQ(ch, i) ((ch)->equipment[i])

#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? mob_proto_special_get((ch)->proto_id) : NULL)
#define GET_MOB_VNUM(mob) (mob->proto_id)

#define GET_DEFAULT_POS(ch) ((ch)->mob_specials.default_pos)
#define MEMORY(ch) ((ch)->mob_specials.memory)
#define MOB_COOLDOWN(ch) ((ch)->cooldown)

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

#define CAN_CARRY_W(ch) getMaxCarryWeight(ch)
#define CAN_CARRY_N(ch) (50)
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) char_can_see_in_dark(ch)

#define IS_GOOD(ch) (GET_ALIGNMENT(ch) >= 50)
#define IS_EVIL(ch) (GET_ALIGNMENT(ch) <= -50)
#define IS_LAWFUL(ch) (GET_ETHIC_ALIGNMENT(ch) >= 350)
#define IS_CHAOTIC(ch) (GET_ETHIC_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_ENEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define ALIGN_TYPE(ch)                                                         \
  ((IS_GOOD(ch) ? 0 : (IS_EVIL(ch) ? 6 : 3)) +                                 \
   (IS_LAWFUL(ch) ? 0 : (IS_CHAOTIC(ch) ? 2 : 1)))

#define IN_ARENA(ch)                                                           \
  (char_room_vnum_get(ch) >= 17800 && char_room_vnum_get(ch) <= 17874)
#define ARENA_IDNUM(ch) ((ch)->arenawatch)

/* These three deprecated. */
#define WAIT_STATE(ch, cycle)                                                  \
  do {                                                                         \
    GET_WAIT_STATE(ch) = (cycle);                                              \
  } while (0)
#define CHECK_WAIT(ch) ((ch)->wait > 0)
#define GET_MOB_WAIT(ch) GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch) ((ch)->wait)

#define SENDOK(ch)                                                             \
  (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) &&                            \
   (to_sleeping || AWAKE(ch)) && !PLR_FLAGGED((ch), PLR_WRITING))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) char_can_see_char(sub, obj)

#define CAN_SEE_OBJ(sub, obj) char_can_see_obj(sub, obj)

#define CAN_CARRY_OBJ(ch, obj)                                                 \
  (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&           \
   ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)                                                   \
  (CAN_WEAR((obj), ITEM_WEAR_TAKE) && !SITTING(obj) &&                         \
   CAN_CARRY_OBJ((ch), (obj)) && CAN_SEE_OBJ((ch), (obj)))

#define DISG(ch, vict)                                                         \
  ((!PLR_FLAGGED(ch, PLR_DISGUISED)) ||                                        \
   (PLR_FLAGGED(ch, PLR_DISGUISED) &&                                          \
    (GET_ADMLEVEL(vict) > 0 || IS_NPC(vict))))

#define INTROD(ch, vict)                                                       \
  (ch == vict || readIntro(ch, vict) == 1 ||                                   \
   (IS_NPC(vict) || IS_NPC(ch) ||                                              \
    (GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0)))

#define ISWIZ(ch, vict)                                                        \
  (ch == vict || GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0 ||             \
   IS_NPC(vict) || IS_NPC(ch))

#define PERS(ch, vict)                                                         \
  ((DISG(ch, vict)                                                             \
        ? (CAN_SEE(vict, ch)                                                   \
               ? (INTROD(vict, ch) ? (ISWIZ(ch, vict) ? GET_NAME(ch)           \
                                                      : get_i_name(vict, ch))  \
                                   : introd_calc(ch))                          \
               : "Someone")                                                    \
        : TRUE_RACE(ch)))

#define OBJS(obj, vict)                                                        \
  (CAN_SEE_OBJ((vict), (obj)) ? (obj)->short_description : "something")

#define OBJN(obj, vict)                                                        \
  (CAN_SEE_OBJ((vict), (obj)) ? fname((obj)->name) : "something")

#define EXIT(ch, door) char_exit_dir(ch, door)
#define _2ND_EXIT(ch, door) char_exit_dir_2nd(ch, door)
#define _3RD_EXIT(ch, door) char_exit_dir_3rd(ch, door)

#define CAN_GO(ch, door) char_can_go_dir(ch, door)

#define RACE(ch) juggleRaceName(ch, true)
#define LRACE(ch) juggleRaceName(ch, false)
#define TRUE_RACE(ch) (pc_race_types[ch->race])
#define SENSEI_NAME(ch) (pc_class_types[ch->chclass])
#define SENSEI_NAME_LOWER(ch) (class_names[ch->chclass])
#define SENSEI_STYLE(ch) (sensei_style[ch->chclass])

#define CLASS_ABBR(ch) (class_abbrevs[ch->chclass])
#define RACE_ABBR(ch) (race_abbrevs[ch->race])

#define IS_ROSHI(ch) (GET_CLASS(ch) == CLASS_ROSHI)
#define IS_PICCOLO(ch) (GET_CLASS(ch) == CLASS_PICCOLO)
#define IS_KRANE(ch) (GET_CLASS(ch) == CLASS_KRANE)
#define IS_NAIL(ch) (GET_CLASS(ch) == CLASS_NAIL)
#define IS_BARDOCK(ch) (GET_CLASS(ch) == CLASS_BARDOCK)
#define IS_GINYU(ch) (GET_CLASS(ch) == CLASS_GINYU)
#define IS_FRIEZA(ch) (GET_CLASS(ch) == CLASS_FRIEZA)
#define IS_TAPION(ch) (GET_CLASS(ch) == CLASS_TAPION)
#define IS_ANDSIX(ch) (GET_CLASS(ch) == CLASS_ANDSIX)
#define IS_DABURA(ch) (GET_CLASS(ch) == CLASS_DABURA)
#define IS_KABITO(ch) (GET_CLASS(ch) == CLASS_KABITO)
#define IS_JINTO(ch) (GET_CLASS(ch) == CLASS_JINTO)
#define IS_TSUNA(ch) (GET_CLASS(ch) == CLASS_TSUNA)
#define IS_KURZAK(ch) (GET_CLASS(ch) == CLASS_KURZAK)

#define GOLD_CARRY(ch)                                                         \
  (GET_LEVEL(ch) < 100 ? (GET_LEVEL(ch) < 50 ? GET_LEVEL(ch) * 10000 : 500000) \
                       : 50000000)
#define IS_SHADOW_DRAGON1(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON1_VNUM)
#define IS_SHADOW_DRAGON2(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON2_VNUM)
#define IS_SHADOW_DRAGON3(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON3_VNUM)
#define IS_SHADOW_DRAGON4(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON4_VNUM)
#define IS_SHADOW_DRAGON5(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON5_VNUM)
#define IS_SHADOW_DRAGON6(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON6_VNUM)
#define IS_SHADOW_DRAGON7(ch)                                                  \
  (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON7_VNUM)
#define CAN_GRAND_MASTER(ch) (IS_HUMAN(ch))
#define IS_HUMANOID(ch) (!IS_SERPENT(ch) && !IS_ANIMAL(ch))
#define IS_ROBOT(ch) (IS_ANDROID(ch) || IS_MECHANICAL(ch))
#define RESTRICTED_RACE(ch)                                                    \
  (IS_MAJIN(ch) || IS_SAIYAN(ch) || IS_BIO(ch) || IS_HOSHIJIN(ch))
#define CHEAP_RACE(ch)                                                         \
  (IS_TRUFFLE(ch) || IS_MUTANT(ch) || IS_KONATSU(ch) || IS_DEMON(ch) ||        \
   IS_KANASSAN(ch))
#define SPAR_TRAIN(ch)                                                         \
  (FIGHTING(ch) && !IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPAR) &&                 \
   !IS_NPC(FIGHTING(ch)) && PLR_FLAGGED(FIGHTING(ch), PLR_SPAR))
#define IS_NONPTRANS(ch)                                                       \
  (IS_HUMAN(ch) ||                                                             \
   ((IS_SAIYAN(ch) || IS_HALFBREED(ch)) && !IS_FULLPSSJ(ch) &&                 \
    !PLR_FLAGGED(ch, PLR_LSSJ) && !PLR_FLAGGED(ch, PLR_OOZARU)) ||             \
   IS_NAMEK(ch) || IS_MUTANT(ch) || IS_ICER(ch) || IS_KAI(ch) ||               \
   IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define IS_FULLPSSJ(ch)                                                        \
  ((IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_FPSSJ) &&                             \
    PLR_FLAGGED(ch, PLR_TRANS1)) ||                                            \
   (IS_HALFBREED(ch) && PLR_FLAGGED(ch, PLR_FPSSJ) &&                          \
    PLR_FLAGGED(ch, PLR_TRANS1)))
#define IS_TRANSFORMED(ch)                                                     \
  (PLR_FLAGGED(ch, PLR_TRANS1) || PLR_FLAGGED(ch, PLR_TRANS2) ||               \
   PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4) ||               \
   PLR_FLAGGED(ch, PLR_TRANS5) || PLR_FLAGGED(ch, PLR_TRANS6) ||               \
   PLR_FLAGGED(ch, PLR_OOZARU))
#define BIRTH_PHASE (time_info.day <= 15)
#define LIFE_PHASE (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE (!BIRTH_PHASE && !LIFE_PHASE)
#define MOON_OK(ch) (HAS_MOON(ch) && MOON_TIMECHECK() && OOZARU_OK(ch))
#define OOZARU_OK(ch)                                                          \
  (OOZARU_RACE(ch) && PLR_FLAGGED(ch, PLR_STAIL) && !IS_TRANSFORMED(ch))
#define OOZARU_RACE(ch) (IS_SAIYAN(ch) || IS_HALFBREED(ch))
#define MOON_TIME (time_info.hours >= 21 || time_info.hours <= 4)
#define MOON_DATE                                                              \
  (time_info.day == 19 || time_info.day == 20 || time_info.day == 21)
#define ETHER_STREAM(ch) char_ether_stream(ch)
#define HAS_MOON(ch) char_has_moon(ch)
#define HAS_ARMS(ch) char_has_arms(ch)
#define HAS_LEGS(ch) char_has_legs(ch)

#define IS_HUMAN(ch) (GET_RACE(ch) == RACE_HUMAN)
#define IS_SAIYAN(ch) (GET_RACE(ch) == RACE_SAIYAN)
#define IS_ICER(ch) (GET_RACE(ch) == RACE_ICER)
#define IS_KONATSU(ch) (GET_RACE(ch) == RACE_KONATSU)
#define IS_NAMEK(ch) (GET_RACE(ch) == RACE_NAMEK)
#define IS_MUTANT(ch) (GET_RACE(ch) == RACE_MUTANT)
#define IS_KANASSAN(ch) (GET_RACE(ch) == RACE_KANASSAN)
#define IS_HALFBREED(ch) (GET_RACE(ch) == RACE_HALFBREED)
#define IS_BIO(ch) (GET_RACE(ch) == RACE_BIO)
#define IS_ANDROID(ch) (GET_RACE(ch) == RACE_ANDROID)
#define IS_DEMON(ch) (GET_RACE(ch) == RACE_DEMON)
#define IS_MAJIN(ch) (GET_RACE(ch) == RACE_MAJIN)
#define IS_KAI(ch) (GET_RACE(ch) == RACE_KAI)
#define IS_TRUFFLE(ch) (GET_RACE(ch) == RACE_TRUFFLE)
#define IS_HOSHIJIN(ch) (GET_RACE(ch) == RACE_HOSHIJIN)
#define IS_ANIMAL(ch) (GET_RACE(ch) == RACE_ANIMAL)
#define IS_SAIBA(ch) (GET_RACE(ch) == RACE_SAIBA)
#define IS_SERPENT(ch) (GET_RACE(ch) == RACE_SERPENT)
#define IS_OGRE(ch) (GET_RACE(ch) == RACE_OGRE)
#define IS_YARDRATIAN(ch) (GET_RACE(ch) == RACE_YARDRATIAN)
#define IS_ARLIAN(ch) (GET_RACE(ch) == RACE_ARLIAN)
#define IS_DRAGON(ch) (GET_RACE(ch) == RACE_DRAGON)
#define IS_MECHANICAL(ch) (GET_RACE(ch) == RACE_MECHANICAL)
#define IS_SPIRIT(ch) (GET_RACE(ch) == RACE_SPIRIT)
#define IS_UNDEAD(ch) (IS_AFFECTED(ch, AFF_UNDEAD))

#define IS_MALE(ch) (GET_SEX(ch) == SEX_MALE)
#define IS_FEMALE(ch) (GET_SEX(ch) == SEX_FEMALE)
#define IS_NEUTER(ch) (!IS_MALE(ch) && !IS_FEMALE(ch))

#define OUTSIDE(ch) (OUTSIDE_ROOMFLAG(ch) && OUTSIDE_SECTTYPE(ch))

#define OUTSIDE_ROOMFLAG(ch) char_outside_roomflag(ch)

#define OUTSIDE_SECTTYPE(ch) char_outside_sector_type(ch)

#define SPEAKING(ch) ((ch)->speaking)

#define HSHR(ch)                                                               \
  (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "his" : "her") : "its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "he" : "she") : "it")
#define HMHR(ch)                                                               \
  (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "him" : "her") : "it")
#define MAFE(ch)                                                               \
  (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "male" : "female")                 \
               : "questionably gendered")

/* returns the number of spells per slot */
#define IS_ARCANE(ch) (IS_WIZARD(ch))
#define IS_DIVINE(ch) (IS_CLERIC(ch))
#define GET_BAB(ch) GET_POLE_BONUS(ch)
#define SET_FEAT(ch, i, value)                                                 \
  do {                                                                         \
    (ch)->feats[i] = value;                                                    \
  } while (0)
#define IS_EPIC_LEVEL(ch) (GET_CLASS_LEVEL(ch) >= 20)
#define HAS_CRAFT_SKILL(ch, i, j) IS_SET_AR((ch)->craft_skill[(i)], (j))
#define SET_CRAFT_SKILL(ch, i, j) SET_BIT_AR((ch)->craft_skill[(i)], (j))
#define HAS_KNOWLEDGE_SKILL(ch, i, j) IS_SET_AR((ch)->knowledge_skill[(i)], (j))
#define SET_KNOWLEDGE_SKILL(ch, i, j)                                          \
  SET_BIT_AR((ch)->knowledge_skill[(i)], (j))
#define HAS_PROFESSION_SKILL(ch, i, j)                                         \
  IS_SET_AR((ch)->profession_skill[(i)], (j))
#define SET_PROFESSION_SKILL(ch, i, j)                                         \
  SET_BIT_AR((ch)->profession_skill[(i)], (j))

#define MOB_LOADROOM(ch) ((ch)->hometown) /*hometown not used for mobs*/
#define GET_MURDER(ch) ((ch)->murder)

#ifdef __cplusplus
}
#endif
