#include "dbat/game/affect.h"
#include "dbat/db/consts/applies.h"
#include "dbat/game/utils.h"

#include "dbat/game/spells.h"
#include "dbat/game/feats.h"

void aff_apply_modify(struct char_data *ch, int loc, int mod, int spec, char *msg)
{
  switch (loc) {
  case APPLY_NONE:
    break;

  case APPLY_STR:
    GET_STR(ch) += mod;
    break;
  case APPLY_DEX:
    GET_DEX(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT(ch) += mod;
    break;
  case APPLY_WIS:
    GET_WIS(ch) += mod;
    break;
  case APPLY_CON:
    GET_CON(ch) += mod;
    break;
  case APPLY_CHA:
    GET_CHA(ch) += mod;
    break;

  case APPLY_CLASS:
  case APPLY_LEVEL:
    break;

  case APPLY_AGE:
    ch->time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT(ch) += mod;
    break;

  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT(ch) += mod;
    break;

  case APPLY_MANA:
  case APPLY_KI:
    ch->max_ki += mod;
    break;

  case APPLY_HIT:
    ch->max_hit += mod;
    break;

  case APPLY_MOVE:
    ch->max_move += mod;
    break;

  case APPLY_GOLD:
  case APPLY_EXP:
    break;

  case APPLY_AC:
    GET_ARMOR(ch) += mod;
    break;

  case APPLY_ACCURACY:
  case APPLY_DAMAGE:
  case APPLY_REGEN:
  case APPLY_TRAIN:
    break;

  case APPLY_LIFEMAX:
    ch->lifebonus += mod;
    break;
  case APPLY_UNUSED3:
  case APPLY_UNUSED4:
  case APPLY_RACE:
  case APPLY_TURN_LEVEL:
  case APPLY_SPELL_LVL_0:
  case APPLY_SPELL_LVL_1:
  case APPLY_SPELL_LVL_2:
  case APPLY_SPELL_LVL_3:
  case APPLY_SPELL_LVL_4:
  case APPLY_SPELL_LVL_5:
  case APPLY_SPELL_LVL_6:
  case APPLY_SPELL_LVL_7:
  case APPLY_SPELL_LVL_8:
  case APPLY_SPELL_LVL_9:
  case APPLY_FORTITUDE:
  case APPLY_REFLEX:
  case APPLY_WILL:
    break;

  case APPLY_SKILL:
    break;

  case APPLY_FEAT:
    HAS_FEAT(ch, spec) += mod;
    break;

  case APPLY_ALLSAVES:
    break;

  case APPLY_ALL_STATS:
    GET_STR(ch) += mod;
    GET_INT(ch) += mod;
    GET_WIS(ch) += mod;
    GET_DEX(ch) += mod;
    GET_CON(ch) += mod;
    GET_CHA(ch) += mod;
    break;

  case APPLY_RESISTANCE:
    break;

  default:
    log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
    break;

  } /* switch */
}


void affect_modify(struct char_data * ch, int loc, int mod, int spec, long bitv, bool add)
{
  if (add) {
   if (bitv != AFF_INFRAVISION || !IS_ANDROID(ch)) {
    SET_BIT_AR(AFF_FLAGS(ch), bitv);
   }
  } else {
   if (bitv != AFF_INFRAVISION || !IS_ANDROID(ch)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), bitv);
    mod = -mod;
   }
  }

  aff_apply_modify(ch, loc, mod, spec, "affect_modify");
}


void affect_modify_ar(struct char_data * ch, int loc, int mod, int spec, bitvector_t bitv[], bool add)
{
  int i , j;

  if (add) {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j)) {
         if ((i*32)+j != AFF_INFRAVISION || !IS_ANDROID(ch)) {
          SET_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
         }
        }
  } else {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j)) {
         if ((i*32)+j != AFF_INFRAVISION || !IS_ANDROID(ch)) {
          REMOVE_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
         }
        }
    mod = -mod;
  }

  aff_apply_modify(ch, loc, mod, spec, "affect_modify_ar");
}


/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data *ch)
{
  struct affected_type *af;
  int i, j;

  GET_SPELLFAIL(ch) = GET_ARMORCHECK(ch) = GET_ARMORCHECKALL(ch) = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
		      GET_EQ(ch, i)->affected[j].modifier,
		      GET_EQ(ch, i)->affected[j].specific,
		      GET_OBJ_PERM(GET_EQ(ch, i)), FALSE);
  }


  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);

  ch->aff_abils = ch->real_abils;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_ARMOR) {
        GET_SPELLFAIL(ch) += GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_SPELLFAIL);
        GET_ARMORCHECKALL(ch) += GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_CHECK);
        if (!is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_SKILL)))
          GET_ARMORCHECK(ch) += GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_CHECK);
      }
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
		      GET_EQ(ch, i)->affected[j].modifier,
		      GET_EQ(ch, i)->affected[j].specific,
		      GET_OBJ_PERM(GET_EQ(ch, i)), TRUE);
    }
  }


  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);

  /* Make certain values are between 0..100, not < 0 and not > 100! */

   if (GET_BONUS(ch, BONUS_WIMP) > 0) {
    GET_STR(ch) = MAX(0, MIN(GET_STR(ch), 70));
   } else {
    GET_STR(ch) = MAX(0, MIN(GET_STR(ch), 100));
   } if (GET_BONUS(ch, BONUS_DULL) > 0) {
    GET_INT(ch) = MAX(0, MIN(GET_INT(ch), 70));
   } else {
    GET_INT(ch) = MAX(0, MIN(GET_INT(ch), 100));
   } if (GET_BONUS(ch, BONUS_FOOLISH) > 0) {
    GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), 70));
   } else {
    GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), 100));
   } if (GET_BONUS(ch, BONUS_SLOW) > 0) {
    GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), 70));
   } else {
    GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), 100));
   } if (GET_BONUS(ch, BONUS_CLUMSY) > 0) {
    GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), 70));
   } else {
    GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), 100));
   } if (GET_BONUS(ch, BONUS_FRAIL) > 0) {
    GET_CON(ch) = MAX(0, MIN(GET_CON(ch), 70));
   } else {
    GET_CON(ch) = MAX(0, MIN(GET_CON(ch), 100));
   }



}



/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  if (!ch->affected) {
    ch->next_affect = affect_list;
    affect_list = ch;
  }
  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);
  affect_total(ch);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *cmtemp;

  if (ch->affected == NULL) {
    core_dump();
    return;
  }

  affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);
  REMOVE_FROM_LIST(af, ch->affected, next, cmtemp);
  free(af);
  affect_total(ch);
  if (!ch->affected) {
    struct char_data *temp;
    REMOVE_FROM_LIST(ch, affect_list, next_affect, temp);
    ch->next_affect = NULL;
  }
}



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      affect_remove(ch, hjp);
  }
}



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(struct char_data *ch, int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (TRUE);

  return (FALSE);
}


void affect_join(struct char_data *ch, struct affected_type *af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      if (add_dur)
	af->duration += hjp->duration;
      if (avg_dur)
	af->duration /= 2;

      if (add_mod)
	af->modifier += hjp->modifier;
      if (avg_mod)
	af->modifier /= 2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}