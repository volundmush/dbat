/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and constants                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef CIRCLE_STRUCTS_H
#define CIRCLE_STRUCTS_H

#include "sysdep.h"

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */


/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */


/* preamble *************************************************************/

/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: This will likely be unconditionally unsigned later.
 */



/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */

/**********************************************************************
* Structures                                                          *
**********************************************************************/

/* ======================================================================= */


/* room-related structures ************************************************/



/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   int32_t id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   int16_t year;
};


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

/* ====================================================================== */


/* descriptor-related structures ******************************************/


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};

/* used in the socials */
struct social_messg {
  int act_nr;
  char *command;               /* holds copy of activating command */
  char *sort_as;              /* holds a copy of a similar command or
                               * abbreviation to sort by for the parser */
  int hide;                   /* ? */
  int min_victim_position;    /* Position of victim */
  int min_char_position;      /* Position of char */
  int min_level_char;          /* Minimum level of socialing char */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;
  char *others_found;
  char *vict_found;

  /* An argument was there, as well as a body part, and a victim was found */
  char *char_body_found;
  char *others_body_found;
  char *vict_body_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;

  /* If the char cant be found search the char's inven and do these: */
  char *char_obj_found;
  char *others_obj_found;
};


/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */




struct guild_info_type {
  int pc_class;
  room_vnum guild_room;
  int direction;
};

/*
 * Config structs
 * 
 */
 
 /*
 * The game configuration structure used for configurating the game play 
 * variables.
 */
struct game_data {
  int pk_allowed;         /* Is player killing allowed? 	  */
  int pt_allowed;         /* Is player thieving allowed?	  */
  int level_can_shout;	  /* Level player must be to shout.	  */
  int holler_move_cost;	  /* Cost to holler in move points.	  */
  int tunnel_size;        /* Number of people allowed in a tunnel.*/
  int max_exp_gain;       /* Maximum experience gainable per kill.*/
  int max_exp_loss;       /* Maximum experience losable per death.*/
  int max_npc_corpse_time;/* Num tics before NPC corpses decompose*/
  int max_pc_corpse_time; /* Num tics before PC corpse decomposes.*/
  int idle_void;          /* Num tics before PC sent to void(idle)*/
  int idle_rent_time;     /* Num tics before PC is autorented.	  */
  int idle_max_level;     /* Level of players immune to idle.     */
  int dts_are_dumps;      /* Should items in dt's be junked?	  */
  int load_into_inventory;/* Objects load in immortals inventory. */
  int track_through_doors;/* Track through doors while closed?    */
  int level_cap;          /* You cannot level to this level       */
  int stack_mobs;	  /* Turn mob stacking on                 */
  int stack_objs;	  /* Turn obj stacking on                 */
  int mob_fighting;       /* Allow mobs to attack other mobs.     */	 
  char *OK;               /* When player receives 'Okay.' text.	  */
  char *NOPERSON;         /* 'No-one by that name here.'	  */
  char *NOEFFECT;         /* 'Nothing seems to happen.'	          */
  int disp_closed_doors;  /* Display closed doors in autoexit?	  */
  int reroll_player;      /* Players can reroll stats on creation */
  int initial_points;	  /* Initial points pool size		  */
  int enable_compression; /* Enable MCCP2 stream compression      */
  int enable_languages;   /* Enable spoken languages              */
  int all_items_unique;   /* Treat all items as unique 		  */
  float exp_multiplier;     /* Experience gain  multiplier	  */
};



/*
 * The rent and crashsave options.
 */
struct crash_save_data {
  int free_rent;          /* Should the MUD allow rent for free?  */
  int max_obj_save;       /* Max items players can rent.          */
  int min_rent_cost;      /* surcharge on top of item costs.	  */
  int auto_save;          /* Does the game automatically save ppl?*/
  int autosave_time;      /* if auto_save=TRUE, how often?        */
  int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
  int rent_file_timeout;  /* Lifetime of normal rent files in days*/
};


/*
 * The room numbers. 
 */
struct room_numbers {
  room_vnum mortal_start_room;	/* vnum of room that mortals enter at.  */
  room_vnum immort_start_room;  /* vnum of room that immorts enter at.  */
  room_vnum frozen_start_room;  /* vnum of room that frozen ppl enter.  */
  room_vnum donation_room_1;    /* vnum of donation room #1.            */
  room_vnum donation_room_2;    /* vnum of donation room #2.            */
  room_vnum donation_room_3;    /* vnum of donation room #3.	        */
};


/*
 * The game operational constants.
 */
struct game_operation {
  uint16_t DFLT_PORT;      /* The default port to run the game.  */
  char *DFLT_IP;            /* Bind to all interfaces.		  */
  char *DFLT_DIR;           /* The default directory (lib).	  */
  char *LOGNAME;            /* The file to log messages to.	  */
  int max_playing;          /* Maximum number of players allowed. */
  int max_filesize;         /* Maximum size of misc files.	  */
  int max_bad_pws;          /* Maximum number of pword attempts.  */
  int siteok_everyone;	    /* Everyone from all sites are SITEOK.*/
  int nameserver_is_slow;   /* Is the nameserver slow or fast?	  */
  int use_new_socials;      /* Use new or old socials file ?      */
  int auto_save_olc;        /* Does OLC save to disk right away ? */
  char *MENU;               /* The MAIN MENU.			  */
  char *WELC_MESSG;	    /* The welcome message.		  */
  char *START_MESSG;        /* The start msg for new characters.  */
  int imc_enabled; /**< Is connection to IMC allowed ? */
};

/*
 * The Autowizard options.
 */
struct autowiz_data {
  int use_autowiz;        /* Use the autowiz feature?		*/
  int min_wizlist_lev;    /* Minimun level to show on wizlist.	*/
};

/* This is for the tick system.
 *
 */
 
struct tick_data {
  int pulse_violence;
  int pulse_mobile;
  int pulse_zone;
  int pulse_autosave;
  int pulse_idlepwd;
  int pulse_sanity;
  int pulse_usage;
  int pulse_timesave;
  int pulse_current;
};

/*
 * The character advancement (leveling) options.
 */
struct advance_data {
  int allow_multiclass; /* Allow advancement in multiple classes     */
  int allow_prestige;   /* Allow advancement in prestige classes     */
};

/*
 * The new character creation method options.
 */
struct creation_data {
  int method; /* What method to use for new character creation */
};

/*
 * The main configuration structure;
 */
struct config_data {
  char                   *CONFFILE;	/* config file path	 */
  struct game_data       play;		/* play related config   */
  struct crash_save_data csd;		/* rent and save related */
  struct room_numbers    room_nums;	/* room numbers          */
  struct game_operation  operation;	/* basic operation       */
  struct autowiz_data    autowiz;	/* autowiz related stuff */
  struct advance_data    advance;   /* char advancement stuff */
  struct tick_data       ticks;		/* game tick stuff 	 */
  struct creation_data	 creation;	/* char creation method	 */
};

/*
 * Data about character aging
 */


#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif