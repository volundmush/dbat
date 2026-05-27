#pragma once
#include "consts/types.h"
#include "consts/skills.h"
#include "consts/senseis.h"
#include "consts/races.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SKTYPE_NONE		0
#define SKTYPE_SPELL		(1 << 0)
#define SKTYPE_SKILL		(1 << 1)
#define SKTYPE_LANG		(1 << 2)
#define SKTYPE_WEAPON		(1 << 3)
#define SKTYPE_ART		(1 << 4)

#define SKFLAG_NEEDTRAIN	(1 << 0) /* Disallow use of 0 skill with only stat mod */
#define SKFLAG_STRMOD		(1 << 1)
#define SKFLAG_DEXMOD		(1 << 2)
#define SKFLAG_CONMOD		(1 << 3)
#define SKFLAG_INTMOD		(1 << 4)
#define SKFLAG_WISMOD		(1 << 5)
#define SKFLAG_CHAMOD		(1 << 6)
#define SKFLAG_ARMORBAD		(1 << 7)
#define SKFLAG_ARMORALL		(1 << 8)
#define SKFLAG_TIER1            (1 << 9)
#define SKFLAG_TIER2            (1 << 10)
#define SKFLAG_TIER3            (1 << 11)
#define SKFLAG_TIER4            (1 << 12)
#define SKFLAG_TIER5            (1 << 13)

#define SKLEARN_CANT		0 /* This class can't learn this skill */
#define SKLEARN_CROSSCLASS	1 /* Cross-class skill for this class */
#define SKLEARN_CLASS		2 /* Class skill for this class */
#define SKLEARN_BOOL		3 /* Skill is known or not */

struct spell_info_type {
   int8_t min_position;	/* Position for caster	 */
   int mana_min;        /* Min amount of mana used by a spell (highest lev) */
   int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
   int mana_change;     /* Change in mana used by spell from lev to lev */
   int ki_min;		/* Min amount of mana used by a spell (highest lev) */
   int ki_max;		/* Max amount of mana used by a spell (lowest lev) */
   int ki_change;	/* Change in mana used by spell from lev to lev */

   int min_level[NUM_CLASSES];
   int routines;
   int8_t violent;
   int targets;         /* See below for use with TAR_XXX  */
   const char *name;	/* Input size not limited. Originates from string constants. */
   const char *wear_off_msg;	/* Input size not limited. Originates from string constants. */
   int race_can_learn[NUM_RACES];
   int skilltype;       /* Is it a spell, skill, art, feat, or what? used as bitvector */
   int flags;
   int save_flags;
   int comp_flags;
   int8_t can_learn_skill[NUM_CLASSES];
   int spell_level;
   int school;
   int domain;
};

extern struct spell_info_type spell_info[SKILL_TABLE_SIZE];

#ifdef __cplusplus
}
#endif

