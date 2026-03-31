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