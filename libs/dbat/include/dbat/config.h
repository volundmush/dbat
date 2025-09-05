#pragma once
#include <cstdint>
#include <string>

#include "const/Max.h"
#include "Typedefs.h"

namespace config
{
    extern int heartbeatIntervalMillis;
    extern std::string hostAddress;
    extern uint16_t port;

    extern std::string logFile;
    // the filename for the game save database.
    extern std::string assetDbName;
    extern std::string stateDbName;
    extern bool logEgregiousTimings;
}


struct game_data {
    int pk_allowed;         /* Is player killing allowed? 	  */
    int pt_allowed;         /* Is player thieving allowed?	  */
    int level_can_shout;      /* Level player must be to shout.	  */
    int holler_move_cost;      /* Cost to holler in move points.	  */
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
    int stack_mobs;      /* Turn mob stacking on                 */
    int stack_objs;      /* Turn obj stacking on                 */
    int mob_fighting;       /* Allow mobs to attack other mobs.     */
    char *OK;               /* When player receives 'Okay.' text.	  */
    char *NOPERSON;         /* 'No-one by that name here.'	  */
    char *NOEFFECT;         /* 'Nothing seems to happen.'	          */
    int disp_closed_doors;  /* Display closed doors in autoexit?	  */
    int reroll_player;      /* Players can reroll stats on creation */
    int initial_points;      /* Initial points pool size		  */
    int enable_compression; /* Enable MCCP2 stream compression      */
    int enable_languages;   /* Enable spoken languages              */
    int all_items_unique;   /* Treat all items as unique 		  */
    float exp_multiplier;     /* Experience gain  multiplier	  */
};


struct crash_save_data {
    int free_rent;          /* Should the MUD allow rent for free?  */
    int max_obj_save;       /* Max items players can rent.          */
    int min_rent_cost;      /* surcharge on top of item costs.	  */
    int auto_save;          /* Does the game automatically save ppl?*/
    int autosave_time;      /* if auto_save=TRUE, how often?        */
    int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
    int rent_file_timeout;  /* Lifetime of normal rent files in days*/
};


struct room_numbers {
    room_vnum mortal_start_room;    /* vnum of room that mortals enter at.  */
    room_vnum immort_start_room;  /* vnum of room that immorts enter at.  */
    room_vnum frozen_start_room;  /* vnum of room that frozen ppl enter.  */
    room_vnum donation_room_1;    /* vnum of donation room #1.            */
    room_vnum donation_room_2;    /* vnum of donation room #2.            */
    room_vnum donation_room_3;    /* vnum of donation room #3.	        */
};

struct game_operation {
    uint16_t DFLT_PORT;      /* The default port to run the game.  */
    char *DFLT_IP;            /* Bind to all interfaces.		  */
    char *DFLT_DIR;           /* The default directory (lib).	  */
    char *LOGNAME;            /* The file to log messages to.	  */
    int max_playing;          /* Maximum number of players allowed. */
    int max_filesize;         /* Maximum size of misc files.	  */
    int max_bad_pws;          /* Maximum number of pword attempts.  */
    int siteok_everyone;        /* Everyone from all sites are SITEOK.*/
    int nameserver_is_slow;   /* Is the nameserver slow or fast?	  */
    int use_new_socials;      /* Use new or old socials file ?      */
    int auto_save_olc;        /* Does OLC save to disk right away ? */
    char *MENU;               /* The MAIN MENU.			  */
    char *WELC_MESSG;        /* The welcome message.		  */
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


struct config_data {
    char *CONFFILE;    /* config file path	 */
    struct game_data play;        /* play related config   */
    struct crash_save_data csd;        /* rent and save related */
    struct room_numbers room_nums;    /* room numbers          */
    struct game_operation operation;    /* basic operation       */
    struct autowiz_data autowiz;    /* autowiz related stuff */

};

extern struct config_data config_info;

// global variables
extern int pk_allowed, pt_allowed, level_can_shout, holler_move_cost, CURRENT_ERA;
extern int mob_specials_used, number_of_assassins, TOPLOADED;

extern char last_user_freed[MAX_INPUT_LENGTH];

extern char *topname[25];
extern int64_t toppoint[25];

extern int ERAPLAYERS, TOPCOUNTDOWN, HEDITS, WISHTIME, TOP_OF_NEWS, LASTNEWS;
extern char *NEWS_TITLE;
extern int crash_file_timeout, rent_file_timeout, imc_is_enabled;
extern time_t BOARDNEWMORT, BOARDNEWIMM, BOARDNEWCOD, BOARDNEWDUO, BOARDNEWBUI, NEWSUPDATE;
extern time_t INTERESTTIME, LASTINTEREST, LASTPAYOUT;
extern int LASTPAYTYPE;

extern int SAIYAN_ALLOWED, MAJIN_ALLOWED, MOON_UP, death_phase, PCOUNT, HIGHPCOUNT;
extern time_t PCOUNTDATE, PCOUNTDAY;



extern int tunnel_size, max_exp_gain, max_exp_loss, max_npc_corpse_time, max_pc_corpse_time;

extern int idle_void, idle_rent_time, idle_max_level, dts_are_dumps, pulse_violence;

extern int pulse_zone, pulse_mobile, pulse_autosave, pulse_idlepwd, pulse_sanity, pulse_usage;
extern int pulse_timesave, pulse_current, load_into_inventory;

extern const char *OK, *NOPERSON, *NOEFFECT;

extern int track_through_doors, level_cap, show_obj_stacking, show_mob_stacking, mob_fighting;

extern int free_rent, max_obj_save, min_rent_cost, auto_save, autosave_time;

extern int auto_pwipe, selfdelete_fastwipe;

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

