#pragma once
#include "consts/types.h"
#include "consts/prefflags.h"
#include "consts/magic.h"
#include "consts/colors.h"
#include "consts/conditions.h"
#include "consts/history.h"
#include "consts/senseis.h"
#include "consts/itemdata.h"
#include "consts/skills.h"
#include "consts/bonus.h"
#include "consts/feats.h"
#include "consts/affflags.h"
#include "consts/adminflags.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/fightprefs.h"
#include "consts/appearance.h"
#include "consts/admlevel.h"
#include "consts/races.h"
#include "consts/senseis.h"
#include "consts/sizes.h"
#include "index.h"
#include "htree.h"

#define PM_ARRAY_MAX    4

/* These data contain information about a players time data */
struct time_data {
   time_t birth;	/* This represents the characters current age        */
   time_t created;	/* This does not change                              */
   time_t maxage;	/* This represents death by natural causes           */
   time_t logon;	/* Time of the last logon (used to calculate played) */
   time_t played;	/* This is the total accumulated time played in secs */
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
  int level;		/* max level for this time limit	*/
  int days;		/* time limit in days			*/
}; 


/* Char's abilities. */
struct abil_data {
   int8_t str;            /* New stats can go over 18 freely, no more /xx */
   int8_t intel;
   int8_t wis;
   int8_t dex;
   int8_t con;
   int8_t cha;
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct player_special_data {
  char *poofin;			/* Description on arrival of a god.     */
  char *poofout;		/* Description upon a god's exit.       */
  struct alias_data *aliases;	/* Character's aliases                  */
  int32_t last_tell;		/* idnum of last tell from              */
  void *last_olc_targ;		/* olc control                          */
  int last_olc_mode;		/* olc control                          */
  char *host;			/* host of last logon                   */
  int spell_level[MAX_SPELL_LEVEL];
				/* bonus to number of spells memorized */
  int memcursor;		/* points to the next free slot in spellmem */
  int wimp_level;		/* Below this # of hit points, flee!	*/
  int8_t freeze_level;		/* Level of god who froze char, if any	*/
  int16_t invis_level;		/* level of invisibility		*/
  room_vnum load_room;		/* Which room to place char in		*/
  int pref[PR_ARRAY_MAX];	/* preference flags for PC's.		*/
  uint8_t bad_pws;		/* number of bad password attemps	*/
  int8_t conditions[NUM_CONDITIONS];		/* Drunk, full, thirsty			*/
  int skill_points;		/* Skill points earned from race HD	*/
  int class_skill_points[NUM_CLASSES];
				/* Skill points earned from a class	*/
  struct txt_block *comm_hist[NUM_HIST]; /* Player's communcations history     */
  int olc_zone;			/* Zone where OLC is permitted		*/
  int gauntlet;                 /* Highest Gauntlet Position */
  int speaking;			/* Language currently speaking		*/
  int tlevel;			/* Turning level			*/
  int ability_trains;		/* How many stat points can you train?	*/
  int spellmem[MAX_MEM];	/* Spell slots				*/
  int feat_points;		/* How many general feats you can take	*/
  int epic_feat_points;		/* How many epic feats you can take	*/
  int class_feat_points[NUM_CLASSES];
				/* How many class feats you can take	*/
  int epic_class_feat_points[NUM_CLASSES];
				/* How many epic class feats 		*/
  int domain[NUM_DOMAINS];
  int school[NUM_SCHOOLS];
  int deity;
  int spell_mastery_points;
  char *color_choices[NUM_COLOR]; /* Choices for custom colors		*/
  uint8_t page_length;
  int murder;                   /* Murder of PC's count                 */
  int trainstr;
  int trainint;
  int traincon;
  int trainwis;
  int trainagl;
  int trainspd;

  struct char_data *carrying;
  struct char_data *carried_by;

  int racial_pref;
};


/* this can be used for skills that can be used per-day */
struct memorize_node {
   int		timer;			/* how many ticks till memorized */
   int		spell; 			/* the spell number */
   struct 	memorize_node *next; 	/* link to the next node */
};

struct innate_node {
   int timer;
   int spellnum;
   struct innate_node *next;
};

/* memory structure for characters */
struct memory_rec_struct {
   int32_t id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   memory_rec *memory;	    /* List of attackers to remember	       */
   int8_t	attack_type;        /* The Attack Type Bitvector for NPC's     */
   int8_t default_pos;        /* Default position for NPC                */
   int8_t damnodice;          /* The number of damage dice's	       */
   int8_t damsizedice;        /* The size of the damage dice's           */
   int newitem;             /* Check if mob has new inv item       */
};

/* Queued spell entry */
struct queued_act {
   int level;
   int spellnum;
};

/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


#define LEVELTYPE_CLASS	1
#define LEVELTYPE_RACE	2

struct level_learn_entry {
  struct level_learn_entry *next;
  int location;
  int specific;
  int8_t value;
};

struct levelup_data {
  struct levelup_data *next;	/* Form a linked list			*/
  struct levelup_data *prev;	/* Form a linked list			*/
  int8_t type;		/* LEVELTYPE_ value			*/
  int8_t spec;		/* Specific class or race		*/
  int8_t level;		/* Level ir HD # for that class or race	*/

  int8_t hp_roll;		/* Straight die-roll value with no mods	*/
  int8_t mana_roll;		/* Straight die-roll value with no mods	*/
  int8_t ki_roll;		/* Straight die-roll value with no mods	*/
  int8_t move_roll;		/* Straight die-roll value with no mods	*/

  int8_t accuracy;		/* Hit accuracy change			*/
  int8_t fort;		/* Fortitude change			*/
  int8_t reflex;		/* Reflex change			*/
  int8_t will;		/* Will change				*/

  int8_t add_skill;		/* Total added skill points		*/
  int8_t add_gen_feats;	/* General feat points			*/
  int8_t add_epic_feats;	/* General epic feat points		*/
  int8_t add_class_feats;	/* Class feat points			*/
  int8_t add_class_epic_feats;/* Epic class feat points		*/

  struct level_learn_entry *skills;	/* Head of linked list		*/
  struct level_learn_entry *feats;	/* Head of linked list		*/
};