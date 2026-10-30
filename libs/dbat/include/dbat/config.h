#pragma once
#include "consts/types.h"


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

extern struct config_data config_info;

/*******************  Config macros *********************/

#define CONFIG_CONFFILE         config_info.CONFFILE

#define CONFIG_PK_ALLOWED       config_info.play.pk_allowed
#define CONFIG_PT_ALLOWED       config_info.play.pt_allowed
#define CONFIG_LEVEL_CAN_SHOUT  config_info.play.level_can_shout
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
#define CONFIG_TUNNEL_SIZE      config_info.play.tunnel_size
#define CONFIG_MAX_EXP_GAIN     config_info.play.max_exp_gain
#define CONFIG_MAX_EXP_LOSS     config_info.play.max_exp_loss
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
#define CONFIG_MAX_PC_CORPSE_TIME config_info.play.max_pc_corpse_time
#define CONFIG_IDLE_VOID        config_info.play.idle_void
#define CONFIG_IDLE_RENT_TIME   config_info.play.idle_rent_time
#define CONFIG_IDLE_MAX_LEVEL   config_info.play.idle_max_level
#define CONFIG_DTS_ARE_DUMPS    config_info.play.dts_are_dumps
#define CONFIG_LOAD_INVENTORY   config_info.play.load_into_inventory
#define CONFIG_TRACK_T_DOORS    config_info.play.track_through_doors
#define CONFIG_LEVEL_CAP	config_info.play.level_cap
#define CONFIG_STACK_MOBS	config_info.play.stack_mobs
#define CONFIG_STACK_OBJS	config_info.play.stack_objs
#define CONFIG_MOB_FIGHTING	config_info.play.mob_fighting
#define CONFIG_OK               config_info.play.OK
#define CONFIG_NOPERSON         config_info.play.NOPERSON
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
#define CONFIG_REROLL_PLAYER_CREATION	config_info.play.reroll_player
#define CONFIG_INITIAL_POINTS_POOL	config_info.play.initial_points
#define CONFIG_ENABLE_COMPRESSION	config_info.play.enable_compression
#define CONFIG_ENABLE_LANGUAGES	config_info.play.enable_languages
#define CONFIG_ALL_ITEMS_UNIQUE	config_info.play.all_items_unique
#define CONFIG_EXP_MULTIPLIER	config_info.play.exp_multiplier

  /** Crash Saves **/
#define CONFIG_FREE_RENT        config_info.csd.free_rent
#define CONFIG_MAX_OBJ_SAVE     config_info.csd.max_obj_save
#define CONFIG_MIN_RENT_COST    config_info.csd.min_rent_cost
#define CONFIG_AUTO_SAVE        config_info.csd.auto_save
#define CONFIG_AUTOSAVE_TIME    config_info.csd.autosave_time
#define CONFIG_CRASH_TIMEOUT    config_info.csd.crash_file_timeout
#define CONFIG_RENT_TIMEOUT     config_info.csd.rent_file_timeout

  /** Room Numbers **/
#define CONFIG_MORTAL_START     config_info.room_nums.mortal_start_room
#define CONFIG_IMMORTAL_START   config_info.room_nums.immort_start_room
#define CONFIG_FROZEN_START     config_info.room_nums.frozen_start_room
#define CONFIG_DON_ROOM_1       config_info.room_nums.donation_room_1
#define CONFIG_DON_ROOM_2       config_info.room_nums.donation_room_2
#define CONFIG_DON_ROOM_3       config_info.room_nums.donation_room_3
#define CONFIG_DEATH_START      config_info.room_nums.death_start_room

  /** Game Operation **/
#define CONFIG_DFLT_PORT        config_info.operation.DFLT_PORT
#define CONFIG_DFLT_IP          config_info.operation.DFLT_IP
#define CONFIG_MAX_PLAYING      config_info.operation.max_playing
#define CONFIG_MAX_FILESIZE     config_info.operation.max_filesize
#define CONFIG_MAX_BAD_PWS      config_info.operation.max_bad_pws
#define CONFIG_SITEOK_ALL       config_info.operation.siteok_everyone
#define CONFIG_OLC_SAVE         config_info.operation.auto_save_olc
#define CONFIG_NEW_SOCIALS      config_info.operation.use_new_socials
#define CONFIG_NS_IS_SLOW       config_info.operation.nameserver_is_slow
#define CONFIG_DFLT_DIR         config_info.operation.DFLT_DIR
#define CONFIG_LOGNAME          config_info.operation.LOGNAME
#define CONFIG_MENU             config_info.operation.MENU
#define CONFIG_WELC_MESSG       config_info.operation.WELC_MESSG
#define CONFIG_START_MESSG      config_info.operation.START_MESSG


  /** Autowiz **/
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

  /** Character Advancement **/
#define CONFIG_ALLOW_MULTICLASS	config_info.advance.allow_multiclass
#define CONFIG_ALLOW_PRESTIGE	config_info.advance.allow_prestige

  /** For tick system **/
#define CONFIG_PULSE_VIOLENCE	config_info.ticks.pulse_violence
#define CONFIG_PULSE_MOBILE	config_info.ticks.pulse_mobile
#define CONFIG_PULSE_ZONE	config_info.ticks.pulse_zone
#define CONFIG_PULSE_AUTOSAVE	config_info.ticks.pulse_autosave
#define CONFIG_PULSE_IDLEPWD	config_info.ticks.pulse_idlepwd
#define CONFIG_PULSE_SANITY	config_info.ticks.pulse_sanity
#define CONFIG_PULSE_USAGE	config_info.ticks.pulse_usage
#define CONFIG_PULSE_TIMESAVE	config_info.ticks.pulse_timesave
#define CONFIG_PULSE_CURRENT	config_info.ticks.pulse_current

  /** Character Creation Method **/
#define CONFIG_CREATION_METHOD	config_info.creation.method

#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"
#include "dbat/characters.h"

/* GAME PLAY OPTIONS */
#if !defined(NO)
#define NO 0
#endif

#if !defined(YES)
#define YES 1
#endif

// global variables
extern int pk_allowed, pt_allowed, level_can_shout, holler_move_cost, CURRENT_ERA;
extern int mob_specials_used, number_of_assassins, TOPLOADED;

extern char last_user_freed[MAX_INPUT_LENGTH];

extern char *topname[25];
extern int64_t toppoint[25];

extern int ERAPLAYERS, TOPCOUNTDOWN, HEDITS, WISHTIME, TOP_OF_NEWS, LASTNEWS;
extern char *NEWS_TITLE;

extern time_t BOARDNEWMORT, BOARDNEWIMM, BOARDNEWCOD, BOARDNEWDUO, BOARDNEWBUI, NEWSUPDATE;
extern time_t INTERESTTIME, LASTINTEREST, LASTPAYOUT;
extern int LASTPAYTYPE;

extern int SAIYAN_ALLOWED, MAJIN_ALLOWED, MOON_UP, DEATHPHASE, PCOUNT, HIGHPCOUNT;
extern time_t PCOUNTDATE, PCOUNTDAY;

extern int SELFISHMETER, SHADOW_DRAGON1, SHADOW_DRAGON2, SHADOW_DRAGON3, SHADOW_DRAGON4, SHADOW_DRAGON5;
extern int SHADOW_DRAGON6, SHADOW_DRAGON7;

extern int DBALL_HUNTER1, DBALL_HUNTER2, DBALL_HUNTER3, DBALL_HUNTER4;
extern int DBALL_HUNTER1_VNUM, DBALL_HUNTER2_VNUM, DBALL_HUNTER3_VNUM, DBALL_HUNTER4_VNUM;

extern int tunnel_size, max_exp_gain, max_exp_loss, max_npc_corpse_time, max_pc_corpse_time;

extern int idle_void, idle_rent_time, idle_max_level, dts_are_dumps, pulse_violence;

extern int pulse_zone, pulse_mobile, pulse_autosave, pulse_idlepwd, pulse_sanity, pulse_usage;
extern int pulse_timesave, pulse_current, load_into_inventory;

extern const char *OK, *NOPERSON, *NOEFFECT;

extern int track_through_doors, level_cap, show_obj_stacking, show_mob_stacking, mob_fighting;

extern int imt_is_enabled, free_rent, max_obj_save, min_rent_cost, auto_save, autosave_time;

extern int auto_pwipe, selfdelete_fastwipe;

extern const struct pclean_criteria_data pclean_criteria[7];

extern room_vnum death_start_room, mortal_start_room, immort_start_room, frozen_start_room;

extern room_vnum donation_room_1, donation_room_2, donation_room_3;

extern int bitwarning, bitsavetodisk;

extern uint16_t DFLT_PORT;

extern const char *DFLT_IP, *DFLT_DIR, *LOGNAME, *ANSIQUESTION, *MENU, *WELC_MESSG, *START_MESSG;

extern int max_playing, max_filesize, max_bad_pws, siteok_everyone, nameserver_is_slow;

extern int auto_save_olc, use_new_socials, use_autowiz, min_wizlist_lev, initial_points;

extern obj_vnum portal_object;

extern int disp_closed_doors, reroll_status, allow_multiclass, allow_prestige, auto_level;

extern int enable_compression, enable_languages, all_items_unique, method;

extern float exp_multiplier;
