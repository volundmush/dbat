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
  struct imcchar_data *imcchardata;  /**< IMC2 Data */
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


/* ================== Structure for player/non-player ===================== */
struct char_data {
  int pfilepos;			/* playerfile pos			*/
  mob_rnum nr;			/* Mob's rnum				*/
  room_rnum in_room;		/* Location (real room number)		*/
  room_rnum was_in_room;	/* location for linkdead people		*/
  int wait;			/* wait for how many loops		*/

  char *name;			/* PC / NPC s name (kill ...  )		*/
  char *short_descr;		/* for NPC 'actions'			*/
  char *long_descr;		/* for 'look'				*/
  char *description;		/* Extra descriptions                   */
  char *title;			/* PC / NPC's title                     */
  int size;			/* Size class of char                   */
  int8_t sex;			/* PC / NPC's sex                       */
  int8_t race;		/* PC / NPC's race                      */
  int8_t hairl;               /* PC hair length                       */
  int8_t hairs;               /* PC hair style                        */
  int8_t hairc;               /* PC hair color                        */
  int8_t skin;                /* PC skin color                        */
  int8_t eye;                 /* PC eye color                         */
  int8_t distfea;             /* PC's Distinguishing Feature          */
  int race_level;		/* PC / NPC's racial level / hit dice   */
  int level_adj;		/* PC level adjustment                  */
  int8_t chclass;		/* Last class taken                     */
  int chclasses[NUM_CLASSES];	/* Ranks in all classes        */
  int epicclasses[NUM_CLASSES];	/* Ranks in all epic classes */
  struct levelup_data *level_info;
				/* Info on gained levels */
  int level;			/* PC / NPC's level                     */
  int admlevel;			/* PC / NPC's admin level               */
  int admflags[AD_ARRAY_MAX];	/* Bitvector for admin privs		*/
  room_vnum hometown;		/* PC Hometown / NPC spawn room         */
  struct time_data time;	/* PC's AGE in days			*/
  uint8_t weight;		/* PC / NPC's weight                    */
  uint8_t height;		/* PC / NPC's height                    */

  struct abil_data real_abils;	/* Abilities without modifiers   */
  struct abil_data aff_abils;	/* Abils with spells/stones/etc  */
  struct player_special_data *player_specials;
				/* PC specials				*/
  struct mob_special_data mob_specials;
				/* NPC specials				*/

  struct affected_type *affected;
				/* affected by what spells		*/
  struct affected_type *affectedv;
				/* affected by what combat spells	*/
  struct queued_act *actq;	/* queued spells / other actions	*/

  struct obj_data *equipment[NUM_WEARS];
				/* Equipment array			*/
  struct obj_data *carrying;	/* Head of list				*/

  struct descriptor_data *desc;	/* NULL for mobiles			*/
  int32_t id;			/* used by DG triggers			*/

  struct trig_proto_list *proto_script;
				/* list of default triggers		*/
  struct script_data *script;	/* script info for the object		*/
  struct script_memory *memory;	/* for mob memory triggers		*/

  struct char_data *next_in_room;
				/* For room->people - list		*/
  struct char_data *next;	/* For either monster or ppl-list	*/
  struct char_data *next_fighting;
				/* For fighting list			*/
  struct char_data *next_affect;/* For affect wearoff			*/
  struct char_data *next_affectv;
				/* For round based affect wearoff	*/

  struct follow_type *followers;/* List of chars followers		*/
  struct char_data *master;	/* Who is char following?		*/
  int32_t master_id;

  struct memorize_node *memorized;
  struct innate_node *innate;

  struct char_data *fighting;	/* Opponent				*/

  int8_t position;		/* Standing, fighting, sleeping, etc.	*/

  int carry_weight;		/* Carried weight			*/
  int8_t carry_items;		/* Number of items carried		*/
  int timer;			/* Timer for update			*/

  struct obj_data *sits;      /* What am I sitting on? */
  struct char_data *blocks;    /* Who am I blocking?    */
  struct char_data *blocked;   /* Who is blocking me?    */
  struct char_data *absorbing; /* Who am I absorbing */
  struct char_data *absorbby;  /* Who is absorbing me */

  int8_t feats[MAX_FEATS + 1];	/* Feats (booleans and counters)	*/
  int combat_feats[CFEAT_MAX+1][FT_ARRAY_MAX];
				/* One bitvector array per CFEAT_ type	*/
  int school_feats[SFEAT_MAX+1];/* One bitvector array per CFEAT_ type	*/

  int8_t skills[SKILL_TABLE_SIZE + 1];
				/* array of skills/spells/arts/etc	*/
  int8_t skillmods[SKILL_TABLE_SIZE + 1];
				/* array of skill mods			*/
  int8_t skillperfs[SKILL_TABLE_SIZE + 1];
                                /* array of skill mods                  */

  int alignment;		/* +-1000 for alignment good vs. evil	*/
  int alignment_ethic;		/* +-1000 for alignment law vs. chaos	*/
  int32_t idnum;			/* player's idnum; -1 for mobiles	*/
  int act[PM_ARRAY_MAX];	/* act flag for NPC's; player flag for PC's */

  int affected_by[AF_ARRAY_MAX];/* Bitvector for current affects	*/
  int bodyparts[AF_ARRAY_MAX];  /* Bitvector for current bodyparts      */
  int16_t saving_throw[3];	/* Saving throw				*/
  int16_t apply_saving_throw[3];	/* Saving throw bonuses			*/

  int powerattack;		/* Setting for power attack level	*/
  int combatexpertise;		/* Setting for Combat expertise level   */

  int64_t mana;
  int64_t max_mana;	/* Max mana for PC/NPC			*/
  int64_t hit;
  int64_t max_hit;	/* Max hit for PC/NPC			*/
  int64_t move;
  int64_t max_move;	/* Max move for PC/NPC			*/
  int64_t ki;
  int64_t max_ki;/* Max ki for PC/NPC			*/

  int armor;		/* Internally stored *10		*/
  int16_t shield_bonus;       /* Shield bonus for AC			*/
  int gold;			/* Money carried			*/
  int bank_gold;		/* Gold the char has in a bank account	*/
  int64_t exp;			/* The experience of the player		*/

  int accuracy;			/* Base hit accuracy			*/
  int accuracy_mod;		/* Any bonus or penalty to the accuracy	*/
  int damage_mod;		/* Any bonus or penalty to the damage	*/

  int16_t spellfail;		/* Total spell failure %                 */
  int16_t armorcheck;		/* Total armorcheck penalty with proficiency forgiveness */
  int16_t armorcheckall;	/* Total armorcheck penalty regardless of proficiency */

  /* All below added by Iovan for sure o.o */

  int64_t basepl;
  int64_t baseki;
  int64_t basest;
  int64_t charge;
  int64_t chargeto;
  int64_t barrier;

  char *clan;
  
  room_vnum droom;
  int choice;
  int sleeptime;
  int foodr;
  int altitude;
  int overf;
  int spam;

  room_vnum radar1;
  room_vnum radar2;
  room_vnum radar3;
  int ship;
  room_vnum shipr;
  time_t lastpl;
  time_t lboard[5];

  room_vnum listenroom;
  int crank;
  int kaioken;
  int absorbs;
  int boosts;
  int upgrade;
  time_t lastint;
  int majinize;
  short fury;
  short btime;
  int eavesdir;
  time_t deathtime;
  int rp;
  int64_t suppression;
  int64_t suppressed;
  struct char_data *drag;
  struct char_data *dragged;
  int trp;
  struct char_data *mindlink;
  int lasthit;
  int dcount;
  char *voice;                  /* PC's snet voice */
  int limbs[4];                 /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
  int aura;
  time_t rewtime;
  struct char_data *grappling;
  struct char_data *grappled;
  int grap;
  int genome[2];                /* Bio racial bonus, Genome */
  int combo;
  int lastattack;
  int combhits;
  int ping;
  int starphase;
  int mimic;
  int bonuses[MAX_BONUSES];
  int ccpoints;
  int negcount;
  int cooldown;
  int death_type;

  int64_t moltexp;
  int moltlevel;

  char *loguser;                /* What user was I last saved as?      */
  int arenawatch;
  int64_t majinizer;
  int speedboost;
  int skill_slots;
  int tail_growth;
  int rage_meter;
  char *feature;
  int transclass;
  int transcost[6];
  int armor_last;
  int forgeting;
  int forgetcount;
  int backstabcool;
  int con_cooldown;
  short stupidkiss;
  char *temp_prompt;

  int personality;
  int combine;
  int linker;
  int fishstate;
  int throws;

  struct char_data *defender;
  struct char_data *defending;

  int64_t lifeforce;
  int lifeperc;
  int gooptime;
  int blesslvl;
  struct char_data *poisonby;

  int mobcharge;
  int preference;
  int aggtimer;

  int lifebonus;
  int asb;
  int regen;
  int rbank;
  int con_sdcooldown;

  int limb_condition[4];
  
  char placeholder[2];
	
  char *rdisplay;
  
  short song;
  struct char_data *original;
  short clones;
  int relax_count;
 	int ingestLearned;
};


extern struct char_data *character_list;
extern struct char_data *affect_list;
extern struct char_data *affectv_list;
extern struct player_special_data dummy_mob;

extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern mob_rnum top_of_mobt;
extern struct htree_node *mob_htree;
extern long max_mob_id;