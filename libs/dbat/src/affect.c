#include "dbat/affect.h"
#include "dbat/consts/applies.h"
#include "dbat/utils.h"

#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/spells.h"
#include "dbat/feats.h"

void aff_apply_modify(struct char_data *ch, int loc, int mod, int spec, char *msg)
{
  switch (loc) {
  case APPLY_NONE:
    break;

  case APPLY_STR:
    char_set_aff_abils_str(ch, char_get_aff_abils_str(ch) + (mod));
    break;
  case APPLY_DEX:
    char_set_aff_abils_dex(ch, char_get_aff_abils_dex(ch) + (mod));
    break;
  case APPLY_INT:
    char_set_aff_abils_intel(ch, char_get_aff_abils_intel(ch) + (mod));
    break;
  case APPLY_WIS:
    char_set_aff_abils_wis(ch, char_get_aff_abils_wis(ch) + (mod));
    break;
  case APPLY_CON:
    char_set_aff_abils_con(ch, char_get_aff_abils_con(ch) + (mod));
    break;
  case APPLY_CHA:
    char_set_aff_abils_cha(ch, char_get_aff_abils_cha(ch) + (mod));
    break;

  case APPLY_CLASS:
    /* ??? GET_CLASS(ch) += mod; */
    break;

  /*
   * My personal thoughts on these two would be to set the person to the
   * value of the apply.  That way you won't have to worry about people
   * making +1 level things to be imp (you restrict anything that gives
   * immortal level of course).  It also makes more sense to set someone
   * to a class rather than adding to the class number. -gg
   */

  case APPLY_LEVEL:
    /* ??? GET_LEVEL(ch) += mod; */
    break;

  case APPLY_AGE:
    ch->time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    char_set_weight(ch, char_get_weight(ch) + (mod));
    break;

  case APPLY_CHAR_HEIGHT:
    char_set_height(ch, char_get_height(ch) + (mod));
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
    break;

  case APPLY_EXP:
    break;

  case APPLY_AC:
    char_set_armor(ch, char_get_armor(ch) + (mod));
    break;

  case APPLY_ACCURACY:
    char_set_accuracy(ch, char_get_accuracy(ch) + (mod));
    break;

  case APPLY_DAMAGE: /* the damage mod it adjusted no longer exists */
    break;

  case APPLY_REGEN:
    char_set_regen(ch, char_get_regen(ch) + (mod));
    break;

  case APPLY_TRAIN:
    char_set_asb(ch, char_get_asb(ch) + (mod));
    break;

  case APPLY_LIFEMAX:
    ch->lifebonus += mod;
    break;
  case APPLY_UNUSED3:
  case APPLY_UNUSED4:
    /* Don't exist anymore */
    break;

  case APPLY_RACE:
    /* ??? GET_RACE(ch) += mod; */
    break;

  case APPLY_TURN_LEVEL:
    GET_TLEVEL(ch) += mod;
    break;

  case APPLY_SPELL_LVL_0:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_0) += mod;
    break;

  case APPLY_SPELL_LVL_1:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_1) += mod;
    break;

  case APPLY_SPELL_LVL_2:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_2) += mod;
    break;

  case APPLY_SPELL_LVL_3:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_3) += mod;
    break;

  case APPLY_SPELL_LVL_4:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_4) += mod;
    break;

  case APPLY_SPELL_LVL_5:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_5) += mod;
    break;

  case APPLY_SPELL_LVL_6:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_6) += mod;
    break;

  case APPLY_SPELL_LVL_7:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_7) += mod;
    break;

  case APPLY_SPELL_LVL_8:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_8) += mod;
    break;

  case APPLY_SPELL_LVL_9:
    if(!char_is_npc(ch))
      GET_SPELL_LEVEL(ch, SPELL_LEVEL_9) += mod;
    break;

  case APPLY_FORTITUDE:
    char_set_apply_saving_throw(ch, SAVING_FORTITUDE, char_get_apply_saving_throw(ch, SAVING_FORTITUDE) + (mod));
    break;

  case APPLY_REFLEX:
    char_set_apply_saving_throw(ch, SAVING_REFLEX, char_get_apply_saving_throw(ch, SAVING_REFLEX) + (mod));
    break;

  case APPLY_WILL:
    char_set_apply_saving_throw(ch, SAVING_WILL, char_get_apply_saving_throw(ch, SAVING_WILL) + (mod));
    break;

  case APPLY_SKILL:
    SET_SKILL_BONUS(ch, spec, char_get_skills_mod(ch, spec) + mod);
    break;

  case APPLY_FEAT:
    char_set_feats(ch, spec, char_get_feats(ch, spec) + (mod));
    break;

  case APPLY_ALLSAVES:
    char_set_apply_saving_throw(ch, SAVING_FORTITUDE, char_get_apply_saving_throw(ch, SAVING_FORTITUDE) + (mod));
    char_set_apply_saving_throw(ch, SAVING_REFLEX, char_get_apply_saving_throw(ch, SAVING_REFLEX) + (mod));
    char_set_apply_saving_throw(ch, SAVING_WILL, char_get_apply_saving_throw(ch, SAVING_WILL) + (mod));
    break;

  case APPLY_ALL_STATS:
    char_set_aff_abils_str(ch, char_get_aff_abils_str(ch) + (mod));
    char_set_aff_abils_intel(ch, char_get_aff_abils_intel(ch) + (mod));
    char_set_aff_abils_wis(ch, char_get_aff_abils_wis(ch) + (mod));
    char_set_aff_abils_dex(ch, char_get_aff_abils_dex(ch) + (mod));
    char_set_aff_abils_con(ch, char_get_aff_abils_con(ch) + (mod));
    char_set_aff_abils_cha(ch, char_get_aff_abils_cha(ch) + (mod));
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
   if (bitv != AFF_INFRAVISION || !char_is_android(ch)) {
    SET_BIT_AR(char_get_affected_by(ch), bitv);
   }
  } else {
   if (bitv != AFF_INFRAVISION || !char_is_android(ch)) {
    REMOVE_BIT_AR(char_get_affected_by(ch), bitv);
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
         if ((i*32)+j != AFF_INFRAVISION || !char_is_android(ch)) {
          SET_BIT_AR(char_get_affected_by(ch), (i*32)+j);
         }
        }
  } else {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j)) {
         if ((i*32)+j != AFF_INFRAVISION || !char_is_android(ch)) {
          REMOVE_BIT_AR(char_get_affected_by(ch), (i*32)+j);
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

  char_set_armorcheckall(ch, 0);
  char_set_armorcheck(ch, 0);
  char_set_spellfail(ch, 0);

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify_ar(ch, char_get_equipment(ch, i)->affected[j].location,
		      char_get_equipment(ch, i)->affected[j].modifier,
		      char_get_equipment(ch, i)->affected[j].specific,
		      obj_get_bitvector(char_get_equipment(ch, i)), FALSE);
  }


  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);

  ch->aff_abils = ch->real_abils;

  char_set_apply_saving_throw(ch, SAVING_FORTITUDE, char_get_feats(ch, FEAT_GREAT_FORTITUDE) * 3);
  char_set_apply_saving_throw(ch, SAVING_REFLEX, char_get_feats(ch, FEAT_LIGHTNING_REFLEXES) * 3);
  char_set_apply_saving_throw(ch, SAVING_WILL, char_get_feats(ch, FEAT_IRON_WILL) * 3);

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i)) {
      if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_ARMOR) {
        char_set_spellfail(ch, char_get_spellfail(ch) + (obj_get_value(char_get_equipment(ch, i), VAL_ARMOR_SPELLFAIL)));
        char_set_armorcheckall(ch, char_get_armorcheckall(ch) + (obj_get_value(char_get_equipment(ch, i), VAL_ARMOR_CHECK)));
        if (!is_proficient_with_armor(ch, obj_get_value(char_get_equipment(ch, i), VAL_ARMOR_SKILL)))
          char_set_armorcheck(ch, char_get_armorcheck(ch) + (obj_get_value(char_get_equipment(ch, i), VAL_ARMOR_CHECK)));
      }
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify_ar(ch, char_get_equipment(ch, i)->affected[j].location,
		      char_get_equipment(ch, i)->affected[j].modifier,
		      char_get_equipment(ch, i)->affected[j].specific,
		      obj_get_bitvector(char_get_equipment(ch, i)), TRUE);
    }
  }


  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);

  /* Make certain values are between 0..100, not < 0 and not > 100! */

   if (char_get_bonuses(ch, BONUS_WIMP) > 0) {
    char_set_aff_abils_str(ch, MAX(0, MIN(char_get_aff_abils_str(ch), 70)));
   } else {
    char_set_aff_abils_str(ch, MAX(0, MIN(char_get_aff_abils_str(ch), 100)));
   } if (char_get_bonuses(ch, BONUS_DULL) > 0) {
    char_set_aff_abils_intel(ch, MAX(0, MIN(char_get_aff_abils_intel(ch), 70)));
   } else {
    char_set_aff_abils_intel(ch, MAX(0, MIN(char_get_aff_abils_intel(ch), 100)));
   } if (char_get_bonuses(ch, BONUS_FOOLISH) > 0) {
    char_set_aff_abils_wis(ch, MAX(0, MIN(char_get_aff_abils_wis(ch), 70)));
   } else {
    char_set_aff_abils_wis(ch, MAX(0, MIN(char_get_aff_abils_wis(ch), 100)));
   } if (char_get_bonuses(ch, BONUS_SLOW) > 0) {
    char_set_aff_abils_cha(ch, MAX(0, MIN(char_get_aff_abils_cha(ch), 70)));
   } else {
    char_set_aff_abils_cha(ch, MAX(0, MIN(char_get_aff_abils_cha(ch), 100)));
   } if (char_get_bonuses(ch, BONUS_CLUMSY) > 0) {
    char_set_aff_abils_dex(ch, MAX(0, MIN(char_get_aff_abils_dex(ch), 70)));
   } else {
    char_set_aff_abils_dex(ch, MAX(0, MIN(char_get_aff_abils_dex(ch), 100)));
   } if (char_get_bonuses(ch, BONUS_FRAIL) > 0) {
    char_set_aff_abils_con(ch, MAX(0, MIN(char_get_aff_abils_con(ch), 70)));
   } else {
    char_set_aff_abils_con(ch, MAX(0, MIN(char_get_aff_abils_con(ch), 100)));
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



/* Call affect_remove with every spell of spelltype "skill" */
void affectv_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affectedv; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      affectv_remove(ch, hjp);
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



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affectedv_by_spell(struct char_data *ch, int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affectedv; hjp; hjp = hjp->next)
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


void affectv_to_char(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  if (!ch->affectedv) {
    ch->next_affectv = affectv_list;
    affectv_list = ch;
  }
  *affected_alloc = *af;
  affected_alloc->next = ch->affectedv;
  ch->affectedv = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, TRUE);
  affect_total(ch);
}

void affectv_remove(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *cmtemp;

  if (ch->affectedv == NULL) {
    core_dump();
    return;
  }

  affect_modify(ch, af->location, af->modifier, af->specific, af->bitvector, FALSE);
  REMOVE_FROM_LIST(af, ch->affectedv, next, cmtemp);
  free(af);
  affect_total(ch);
  if (!ch->affectedv) {
    struct char_data *temp;
    REMOVE_FROM_LIST(ch, affectv_list, next_affectv, temp);
    ch->next_affectv = NULL;
  }
}

void affectv_join(struct char_data *ch, struct affected_type *af,
                      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affectedv; !found && hjp; hjp = next) {
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

      affectv_remove(ch, hjp);
      affectv_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affectv_to_char(ch, af);
}
