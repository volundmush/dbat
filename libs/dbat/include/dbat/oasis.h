/************************************************************************
 * OasisOLC - General / oasis.h					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#pragma once

#include "structs.h"
#include "Destination.h"

//constexpr int _OASISOLC = 0;x206   /* 2.0.6 */
/*
 * Used to determine what version of OasisOLC is installed.
 *
 * Ex: #if _OASISOLC >= OASIS_VERSION(2,0,0)
 */
#define OASIS_VERSION(x, y, z)    (((x) << 8 | (y) << 4 | (z))

constexpr int AEDIT_PERMISSION = 999;  /* arbitrary number higher than max zone vnum*/
constexpr int HEDIT_PERMISSION = 888;  /* arbitrary number higher then max zone vnum*/

/*
 * Macros, defines, structs and globals for the OLC suite.  You will need
 * to adjust these numbers if you ever add more.
 */



/* -------------------------------------------------------------------------- */

/*
 * Limit information.
 */
constexpr int MAX_ROOM_NAME = 100;
constexpr int MAX_MOB_NAME = 50;
constexpr int MAX_OBJ_NAME = 50;
constexpr int MAX_ROOM_DESC = 4096;
constexpr int MAX_EXIT_DESC = 256;
constexpr int MAX_EXTRA_DESC = 512;
constexpr int MAX_MOB_DESC = 1024;
constexpr int MAX_OBJ_DESC = 512;
constexpr int MAX_DUPLICATES = 10000;  /* when loading in zedit */
constexpr int MAX_FROM_ROOM = 50;    /* when loading in zedit */

/* arbitrary limits - roll your own */
/* max weapon is 50d50 .. avg. 625 dam... */
constexpr int MAX_WEAPON_SDICE = 50;
constexpr int MAX_WEAPON_NDICE = 50;

constexpr int64_t MAX_OBJ_WEIGHT = 100000000000000;
constexpr int MAX_OBJ_COST = 2000000;
constexpr int MAX_OBJ_RENT = 2000000;
constexpr int64_t MAX_CONTAINER_SIZE = 500000000000000;

constexpr int MAX_MOB_GOLD = 1000000;
constexpr int MAX_MOB_EXP = 1500000;
/* this is one mud year.. */
constexpr int MAX_OBJ_TIMER = 1071000;

/* this defines how much memory is alloacted for 'bit strings' when
 * saving in OLC. Remember to change it if you go for longer bitvectors.
 */
constexpr int BIT_STRING_LENGTH = 33;
/*
 * The data types for miscellaneous functions.
 */
constexpr int OASIS_WLD = 0;
constexpr int OASIS_MOB = 1;
constexpr int OASIS_OBJ = 2;
constexpr int OASIS_ZON = 3;
constexpr int OASIS_EXI = 4;
constexpr int OASIS_CFG = 5;

/* -------------------------------------------------------------------------- */

/*
 * Utilities exported from olc.c.
 *   -- Umm, shouldn't this say 'from oasis.c' now???  * Mythran
 */
extern void cleanup_olc(struct descriptor_data *d, int8_t cleanup_type);

extern void split_argument(char *argument, char *tag);

extern void send_cannot_edit(Character *ch, zone_vnum zone);

/*
 * OLC structures.
 */
/* -------------------------------------------------------------------------- */

/*
 * The following defines used to be in config.c.
 */

struct oasis_olc_data {
    int mode;                      /* how to parse input       */
    zone_rnum zone_num;            /* current zone             */
    room_vnum number;              /* vnum of subject          */
    int value;                     /* mostly 'has changed' flag*/
    char *storage;                 /* used for 'tedit'         */
    CharacterPrototype *mob;    /* used for 'medit'         */
    Room *room;        /* used for 'redit'         */
    std::optional<Destination> dest; /* used for 'redit'       */
    ObjectPrototype *obj;   /* used for 'oedit'         */
    Object *iobj;         /* used for 'iedit'         */
    struct Zone *zone;        /* used for 'zedit'         */
    struct Shop *shop;        /* used for 'sedit'         */
    struct config_data *config;    /* used for 'cedit'         */
    struct extra_descr_data *desc; /* used in '[r|o|m]edit'    */
    struct social_messg *action;   /* Aedit uses this one      */
    DgScriptPrototype *trig;
    int script_mode;
    int trigger_position;
    int item_type;
    std::vector<trig_vnum> script; /* for assigning triggers in [r|o|m]edit*/
    struct assembly_data *OlcAssembly; /* used for 'assedit'         */
    struct Guild *guild; /* used for 'gedit'         */
    struct help_index_element *help;   /* Hedit uses this */
};

/*
 * Descriptor access macros.
 */
#define OLC(d)        ((d)->olc)
#define OLC_MODE(d)    (OLC(d)->mode)        /* Parse input mode.	*/
#define OLC_NUM(d)    (OLC(d)->number)    /* Room/Obj VNUM.	*/
#define OLC_VAL(d)    (OLC(d)->value)        /* Scratch variable.	*/
#define OLC_ZNUM(d)    (OLC(d)->zone_num)    /* Real zone number.	*/

#define OLC_STORAGE(d)  (OLC(d)->storage)    /* char pointer.	*/
#define OLC_ROOM(d)    (OLC(d)->room)        /* Room structure.	*/
#define OLC_OBJ(d)    (OLC(d)->obj)        /* Object structure.	*/
#define OLC_IOBJ(d)    (OLC(d)->iobj)        /* Individual object structure.	*/
#define OLC_ZONE(d)     (OLC(d)->zone)          /* Zone structure.	*/
#define OLC_MOB(d)    (OLC(d)->mob)        /* Mob structure.	*/
#define OLC_HOUSE(d)    (OLC(d)->house)         /* house structure      */
#define OLC_SHOP(d)    (OLC(d)->shop)        /* Shop structure.	*/
#define OLC_DESC(d)    (OLC(d)->desc)        /* Extra description.	*/
#define OLC_CONFIG(d)    (OLC(d)->config)    /* Config structure.	*/
#define OLC_TRIG(d)     (OLC(d)->trig)          /* Trigger structure.   */
#define OLC_ASSEDIT(d)  (OLC(d)->OlcAssembly)   /* assembly olc        */

#define OLC_ACTION(d)   (OLC(d)->action)        /* Action structure     */
#define OLC_GUILD(d)    (OLC(d)->guild)        /* Guild structure      */
#define OLC_HELP(d)     (OLC(d)->help)          /* Hedit structure      */

#define OLC_SCRIPT_EDIT_MODE(d)	(OLC(d)->script_mode)	/* parse input mode */
#define OLC_SCRIPT(d)           (OLC(d)->script)	/* script editing   */
#define OLC_ITEM_TYPE(d)	(OLC(d)->item_type)	/* mob/obj/room     */

/*
 * Other macros.
 */
#define OLC_EXIT(d)        (OLC(d)->dest)

/*
 * Cleanup types.
 */
constexpr int CLEANUP_ALL = 1;    /* Free the whole lot.			*/
constexpr int CLEANUP_STRUCTS = 2;    /* Don't free strings.			*/
constexpr int CLEANUP_CONFIG = 3;       /* Used just to send proper message. 	*/

/* Submodes of AEDIT connectedness     */
constexpr int AEDIT_CONFIRM_SAVESTRING = 0;
constexpr int AEDIT_CONFIRM_EDIT = 1;
constexpr int AEDIT_CONFIRM_ADD = 2;
constexpr int AEDIT_MAIN_MENU = 3;
constexpr int AEDIT_ACTION_NAME = 4;
constexpr int AEDIT_SORT_AS = 5;
constexpr int AEDIT_MIN_CHAR_POS = 6;
constexpr int AEDIT_MIN_VICT_POS = 7;
constexpr int AEDIT_HIDDEN_FLAG = 8;
constexpr int AEDIT_MIN_CHAR_LEVEL = 9;
constexpr int AEDIT_NOVICT_CHAR = 10;
constexpr int AEDIT_NOVICT_OTHERS = 11;
constexpr int AEDIT_VICT_CHAR_FOUND = 12;
constexpr int AEDIT_VICT_OTHERS_FOUND = 13;
constexpr int AEDIT_VICT_VICT_FOUND = 14;
constexpr int AEDIT_VICT_NOT_FOUND = 15;
constexpr int AEDIT_SELF_CHAR = 16;
constexpr int AEDIT_SELF_OTHERS = 17;
constexpr int AEDIT_VICT_CHAR_BODY_FOUND = 18;
constexpr int AEDIT_VICT_OTHERS_BODY_FOUND = 19;
constexpr int AEDIT_VICT_VICT_BODY_FOUND = 20;
constexpr int AEDIT_OBJ_CHAR_FOUND = 21;
constexpr int AEDIT_OBJ_OTHERS_FOUND = 22;
/*
 * Submodes of OEDIT connectedness.
 */
constexpr int OEDIT_MAIN_MENU = 1;
constexpr int OEDIT_EDIT_NAMELIST = 2;
constexpr int OEDIT_SHORTDESC = 3;
constexpr int OEDIT_LONGDESC = 4;
constexpr int OEDIT_ACTDESC = 5;
constexpr int OEDIT_TYPE = 6;
constexpr int OEDIT_EXTRAS = 7;
constexpr int OEDIT_WEAR = 8;
constexpr int OEDIT_WEIGHT = 9;
constexpr int OEDIT_COST = 10;
constexpr int OEDIT_COSTPERDAY = 11;
constexpr int OEDIT_TIMER = 12;
constexpr int OEDIT_VALUE_1 = 13;
constexpr int OEDIT_VALUE_2 = 14;
constexpr int OEDIT_VALUE_3 = 15;
constexpr int OEDIT_VALUE_4 = 16;
constexpr int OEDIT_APPLY = 17;
constexpr int OEDIT_APPLYMOD = 18;
constexpr int OEDIT_EXTRADESC_KEY = 19;
constexpr int OEDIT_CONFIRM_SAVEDB = 20;
constexpr int OEDIT_CONFIRM_SAVESTRING = 21;
constexpr int OEDIT_PROMPT_APPLY = 22;
constexpr int OEDIT_EXTRADESC_DESCRIPTION = 23;
constexpr int OEDIT_EXTRADESC_MENU = 24;
constexpr int OEDIT_LEVEL = 25;
constexpr int OEDIT_PERM = 26;
constexpr int OEDIT_VALUE_5 = 27;
constexpr int OEDIT_VALUE_6 = 28;
constexpr int OEDIT_VALUE_7 = 29;
constexpr int OEDIT_VALUE_8 = 30;
constexpr int OEDIT_MATERIAL = 31;
constexpr int OEDIT_VALUE_9 = 32;
constexpr int OEDIT_VALUE_10 = 33;
constexpr int OEDIT_VALUE_11 = 34;
constexpr int OEDIT_VALUE_12 = 35;
constexpr int OEDIT_VALUE_13 = 36;
constexpr int OEDIT_VALUE_14 = 37;
constexpr int OEDIT_VALUE_15 = 38;
constexpr int OEDIT_VALUE_16 = 39;
constexpr int OEDIT_SIZE = 40;
constexpr int OEDIT_APPLYSPEC = 41;
constexpr int OEDIT_PROMPT_SPELLBOOK = 42;
constexpr int OEDIT_SPELLBOOK = 43;
constexpr int OEDIT_COPY = 44;
constexpr int OEDIT_DELETE = 45;


/*
 * Submodes of REDIT connectedness.
 */
constexpr int REDIT_MAIN_MENU = 1;
constexpr int REDIT_NAME = 2;
constexpr int REDIT_DESC = 3;
constexpr int REDIT_FLAGS = 4;
constexpr int REDIT_SECTOR = 5;
constexpr int REDIT_EXIT_MENU = 6;
constexpr int REDIT_CONFIRM_SAVEDB = 7;
constexpr int REDIT_CONFIRM_SAVESTRING = 8;
constexpr int REDIT_EXIT_NUMBER = 9;
constexpr int REDIT_EXIT_DESCRIPTION = 10;
constexpr int REDIT_EXIT_KEYWORD = 11;
constexpr int REDIT_EXIT_KEY = 12;
constexpr int REDIT_EXIT_DOORFLAGS = 13;
constexpr int REDIT_EXTRADESC_MENU = 14;
constexpr int REDIT_EXTRADESC_KEY = 15;
constexpr int REDIT_EXTRADESC_DESCRIPTION = 16;
constexpr int REDIT_DELETE = 17;
constexpr int REDIT_EXIT_DCLOCK = 18;
constexpr int REDIT_EXIT_DCHIDE = 19;
constexpr int REDIT_EXIT_DCSKILL = 20;
constexpr int REDIT_EXIT_DCMOVE = 21;
constexpr int REDIT_EXIT_SAVETYPE = 22;
constexpr int REDIT_EXIT_DCSAVE = 23;
constexpr int REDIT_EXIT_FAILROOM = 24;
constexpr int REDIT_EXIT_TOTALFAILROOM = 25;
constexpr int REDIT_COPY = 26;

/*
 * Submodes of ZEDIT connectedness.
 */
constexpr int ZEDIT_MAIN_MENU = 0;
constexpr int ZEDIT_DELETE_ENTRY = 1;
constexpr int ZEDIT_NEW_ENTRY = 2;
constexpr int ZEDIT_CHANGE_ENTRY = 3;
constexpr int ZEDIT_COMMAND_TYPE = 4;
constexpr int ZEDIT_IF_FLAG = 5;
constexpr int ZEDIT_ARG1 = 6;
constexpr int ZEDIT_ARG2 = 7;
constexpr int ZEDIT_ARG3 = 8;
constexpr int ZEDIT_ARG4 = 9;
constexpr int ZEDIT_ARG5 = 10;
constexpr int ZEDIT_ZONE_NAME = 11;
constexpr int ZEDIT_ZONE_LIFE = 12;
constexpr int ZEDIT_ZONE_BOT = 13;
constexpr int ZEDIT_ZONE_TOP = 14;
constexpr int ZEDIT_ZONE_RESET = 15;
constexpr int ZEDIT_CONFIRM_SAVESTRING = 16;
constexpr int ZEDIT_ZONE_BUILDERS = 17;
constexpr int ZEDIT_SARG1 = 18;
constexpr int ZEDIT_SARG2 = 19;
constexpr int ZEDIT_ZONE_FLAGS = 20;
constexpr int ZEDIT_MIN_LEVEL = 21;
constexpr int ZEDIT_MAX_LEVEL = 22;

/*
 * Submodes of MEDIT connectedness.
 */
constexpr int MEDIT_MAIN_MENU = 0;
constexpr int MEDIT_ALIAS = 1;
constexpr int MEDIT_S_DESC = 2;
constexpr int MEDIT_L_DESC = 3;
constexpr int MEDIT_D_DESC = 4;
constexpr int MEDIT_NPC_FLAGS = 5;
constexpr int MEDIT_AFF_FLAGS = 6;
constexpr int MEDIT_CONFIRM_SAVESTRING = 7;
/*
 * Numerical responses.
 */
constexpr int MEDIT_NUMERICAL_RESPONSE = 10;
constexpr int MEDIT_SEX = 11;
constexpr int MEDIT_ACCURACY = 12;
constexpr int MEDIT_DAMAGE = 13;
constexpr int MEDIT_NDD = 14;
constexpr int MEDIT_SDD = 15;
constexpr int MEDIT_NUM_HP_DICE = 16;
constexpr int MEDIT_SIZE_HP_DICE = 17;
constexpr int MEDIT_ADD_HP = 18;
constexpr int MEDIT_AC = 19;
constexpr int MEDIT_EXP = 20;
constexpr int MEDIT_GOLD = 21;
constexpr int MEDIT_POS = 22;
constexpr int MEDIT_DEFAULT_POS = 23;
constexpr int MEDIT_ATTACK = 24;
constexpr int MEDIT_LEVEL = 25;
constexpr int MEDIT_ALIGNMENT = 26;
constexpr int MEDIT_CLASS = 33;
constexpr int MEDIT_RACE = 34;
constexpr int MEDIT_SIZE = 35;
constexpr int MEDIT_COPY = 36;
constexpr int MEDIT_DELETE = 37;
constexpr int MEDIT_PERSONALITY = 38;

/*
 * Submodes of SEDIT connectedness.
 */
constexpr int SEDIT_MAIN_MENU = 0;
constexpr int SEDIT_CONFIRM_SAVESTRING = 1;
constexpr int SEDIT_NOITEM1 = 2;
constexpr int SEDIT_NOITEM2 = 3;
constexpr int SEDIT_NOCASH1 = 4;
constexpr int SEDIT_NOCASH2 = 5;
constexpr int SEDIT_NOBUY = 6;
constexpr int SEDIT_BUY = 7;
constexpr int SEDIT_SELL = 8;
constexpr int SEDIT_PRODUCTS_MENU = 11;
constexpr int SEDIT_ROOMS_MENU = 12;
constexpr int SEDIT_NAMELIST_MENU = 13;
constexpr int SEDIT_NAMELIST = 14;
/*
 * Numerical responses.
 */
constexpr int SEDIT_NUMERICAL_RESPONSE = 20;
constexpr int SEDIT_OPEN1 = 21;
constexpr int SEDIT_OPEN2 = 22;
constexpr int SEDIT_CLOSE1 = 23;
constexpr int SEDIT_CLOSE2 = 24;
constexpr int SEDIT_KEEPER = 25;
constexpr int SEDIT_BUY_PROFIT = 26;
constexpr int SEDIT_SELL_PROFIT = 27;
constexpr int SEDIT_TYPE_MENU = 29;
constexpr int SEDIT_DELETE_TYPE = 30;
constexpr int SEDIT_DELETE_PRODUCT = 31;
constexpr int SEDIT_NEW_PRODUCT = 32;
constexpr int SEDIT_DELETE_ROOM = 33;
constexpr int SEDIT_NEW_ROOM = 34;
constexpr int SEDIT_SHOP_FLAGS = 35;
constexpr int SEDIT_NOTRADE = 36;
constexpr int SEDIT_COPY = 37;

/* 
 * Submodes of CEDIT connectedness.
 */
constexpr int CEDIT_MAIN_MENU = 0;
constexpr int CEDIT_CONFIRM_SAVESTRING = 1;
constexpr int CEDIT_GAME_OPTIONS_MENU = 2;
constexpr int CEDIT_CRASHSAVE_OPTIONS_MENU = 3;
constexpr int CEDIT_OPERATION_OPTIONS_MENU = 4;
constexpr int CEDIT_DISP_EXPERIENCE_MENU = 5;
constexpr int CEDIT_ROOM_NUMBERS_MENU = 6;
constexpr int CEDIT_AUTOWIZ_OPTIONS_MENU = 7;
constexpr int CEDIT_OK = 8;
constexpr int CEDIT_NOPERSON = 9;
constexpr int CEDIT_NOEFFECT = 10;
constexpr int CEDIT_DFLT_IP = 11;
constexpr int CEDIT_DFLT_DIR = 12;
constexpr int CEDIT_LOGNAME = 13;
constexpr int CEDIT_MENU = 14;
constexpr int CEDIT_WELC_MESSG = 15;
constexpr int CEDIT_START_MESSG = 16;
constexpr int CEDIT_ADVANCE_OPTIONS_MENU = 17;

/*
 * Numerical responses.
 */
constexpr int CEDIT_NUMERICAL_RESPONSE = 20;
constexpr int CEDIT_LEVEL_CAN_SHOUT = 21;
constexpr int CEDIT_HOLLER_MOVE_COST = 22;
constexpr int CEDIT_TUNNEL_SIZE = 23;
constexpr int CEDIT_MAX_EXP_GAIN = 24;
constexpr int CEDIT_MAX_EXP_LOSS = 25;
constexpr int CEDIT_MAX_NPC_CORPSE_TIME = 26;
constexpr int CEDIT_MAX_PC_CORPSE_TIME = 27;
constexpr int CEDIT_IDLE_VOID = 28;
constexpr int CEDIT_IDLE_RENT_TIME = 29;
constexpr int CEDIT_IDLE_MAX_LEVEL = 30;
constexpr int CEDIT_DTS_ARE_DUMPS = 31;
constexpr int CEDIT_LOAD_INTO_INVENTORY = 32;
constexpr int CEDIT_TRACK_THROUGH_DOORS = 33;
constexpr int CEDIT_LEVEL_CAP = 34;
constexpr int CEDIT_MAX_OBJ_SAVE = 35;
constexpr int CEDIT_MIN_RENT_COST = 36;
constexpr int CEDIT_AUTOSAVE_TIME = 37;
constexpr int CEDIT_CRASH_FILE_TIMEOUT = 38;
constexpr int CEDIT_RENT_FILE_TIMEOUT = 39;
constexpr int CEDIT_MORTAL_START_ROOM = 40;
constexpr int CEDIT_IMMORT_START_ROOM = 41;
constexpr int CEDIT_FROZEN_START_ROOM = 42;
constexpr int CEDIT_DONATION_ROOM_1 = 43;
constexpr int CEDIT_DONATION_ROOM_2 = 44;
constexpr int CEDIT_DONATION_ROOM_3 = 45;
constexpr int CEDIT_DFLT_PORT = 46;
constexpr int CEDIT_MAX_PLAYING = 47;
constexpr int CEDIT_MAX_FILESIZE = 48;
constexpr int CEDIT_MAX_BAD_PWS = 49;
constexpr int CEDIT_SITEOK_EVERYONE = 50;
constexpr int CEDIT_NAMESERVER_IS_SLOW = 51;
constexpr int CEDIT_USE_AUTOWIZ = 52;
constexpr int CEDIT_MIN_WIZLIST_LEV = 53;
constexpr int CEDIT_ALLOW_MULTICLASS = 54;
constexpr int CEDIT_EXP_MULTIPLIER = 55;
constexpr int CEDIT_PULSE_VIOLENCE = 56;
constexpr int CEDIT_PULSE_MOBILE = 57;
constexpr int CEDIT_PULSE_ZONE = 58;
constexpr int CEDIT_PULSE_CURRENT = 59;
constexpr int CEDIT_PULSE_IDLEPWD = 60;
constexpr int CEDIT_PULSE_USAGE = 61;
constexpr int CEDIT_PULSE_SANITY = 62;
constexpr int CEDIT_PULSE_AUTOSAVE = 63;
constexpr int CEDIT_PULSE_TIMESAVE = 64;
constexpr int CEDIT_TICKS_OPTIONS_MENU = 65;
constexpr int CEDIT_CREATION_OPTIONS_MENU = 66;
constexpr int CEDIT_CREATION_MENU = 67;
constexpr int CEDIT_POINTS_MENU = 68;

constexpr int ASSEDIT_DO_NOT_USE = 0;
constexpr int ASSEDIT_MAIN_MENU = 1;
constexpr int ASSEDIT_ADD_COMPONENT = 2;
constexpr int ASSEDIT_EDIT_COMPONENT = 3;
constexpr int ASSEDIT_DELETE_COMPONENT = 4;
constexpr int ASSEDIT_EDIT_EXTRACT = 5;
constexpr int ASSEDIT_EDIT_INROOM = 6;
constexpr int ASSEDIT_EDIT_TYPES = 7;

constexpr int CEDIT_CREATION_METHOD_1 = 0;
constexpr int CEDIT_CREATION_METHOD_2 = 1;
constexpr int CEDIT_CREATION_METHOD_3 = 2;
constexpr int CEDIT_CREATION_METHOD_4 = 3;
constexpr int CEDIT_CREATION_METHOD_5 = 4;

/* Submodes of HEDIT connectedness     */
constexpr int HEDIT_CONFIRM_SAVESTRING = 0;
constexpr int HEDIT_CONFIRM_EDIT = 1;
constexpr int HEDIT_CONFIRM_ADD = 2;
constexpr int HEDIT_MAIN_MENU = 3;
constexpr int HEDIT_ENTRY = 4;
constexpr int HEDIT_KEYWORDS = 5;
constexpr int HEDIT_MIN_LEVEL = 6;

/*. House editor .*/
constexpr int HSEDIT_MAIN_MENU = 0;
constexpr int HSEDIT_CONFIRM_SAVESTRING = 1;
constexpr int HSEDIT_OWNER_MENU = 2;
constexpr int HSEDIT_OWNER_NAME = 3;
constexpr int HSEDIT_OWNER_ID = 4;
constexpr int HSEDIT_ROOM = 5;
constexpr int HSEDIT_ATRIUM = 6;
constexpr int HSEDIT_DIR_MENU = 7;
constexpr int HSEDIT_GUEST_MENU = 8;
constexpr int HSEDIT_GUEST_ADD = 9;
constexpr int HSEDIT_GUEST_DELETE = 10;
constexpr int HSEDIT_GUEST_CLEAR = 11;
constexpr int HSEDIT_FLAGS = 12;
constexpr int HSEDIT_BUILD_DATE = 13;
constexpr int HSEDIT_PAYMENT = 14;
constexpr int HSEDIT_TYPE = 15;
constexpr int HSEDIT_DELETE = 16;
constexpr int HSEDIT_VALUE_0 = 17;
constexpr int HSEDIT_VALUE_1 = 18;
constexpr int HSEDIT_VALUE_2 = 19;
constexpr int HSEDIT_VALUE_3 = 20;
constexpr int HSEDIT_NOVNUM = 21;
constexpr int HSEDIT_BUILDER = 22;

/* -------------------------------------------------------------------------- */

#ifndef __GENOLC_C__

/*
 * Prototypes to keep.
 */

extern void clear_screen(struct descriptor_data *);



/*
 * Prototypes, to be moved later.
 */


extern void medit_free_mobile(CharacterPrototype *mob);

extern void medit_setup_new(struct descriptor_data *d);

extern void medit_setup_existing(struct descriptor_data *d, int rmob_num);

extern void init_mobile(CharacterPrototype *mob);

extern void medit_save_internally(struct descriptor_data *d);

extern void medit_save_to_disk(zone_vnum zone_num);

extern void medit_disp_positions(struct descriptor_data *d);

extern void medit_disp_mprog(struct descriptor_data *d);

extern void medit_change_mprog(struct descriptor_data *d);

extern void medit_disp_mprog_types(struct descriptor_data *d);

extern void medit_disp_sex(struct descriptor_data *d);

extern void medit_disp_attack_types(struct descriptor_data *d);

extern void medit_disp_mob_flags(struct descriptor_data *d);

extern void medit_disp_aff_flags(struct descriptor_data *d);

extern void medit_disp_menu(struct descriptor_data *d);

extern void medit_parse(struct descriptor_data *d, char *arg);

extern void medit_string_cleanup(struct descriptor_data *d, int terminator);

extern void oedit_setup_new(struct descriptor_data *d);

extern void oedit_setup_existing(struct descriptor_data *d, int real_num);

extern void oedit_save_internally(struct descriptor_data *d);

extern void oedit_disp_container_flags_menu(struct descriptor_data *d);

extern void oedit_disp_extradesc_menu(struct descriptor_data *d);

extern void oedit_disp_prompt_apply_menu(struct descriptor_data *d);

extern void oedit_liquid_type(struct descriptor_data *d);

extern void oedit_disp_apply_menu(struct descriptor_data *d);

extern void oedit_disp_weapon_menu(struct descriptor_data *d);

extern void oedit_disp_crittype_menu(struct descriptor_data *d);

extern void oedit_disp_spells_menu(struct descriptor_data *d);

extern void oedit_disp_val1_menu(struct descriptor_data *d);

extern void oedit_disp_val2_menu(struct descriptor_data *d);

extern void oedit_disp_val3_menu(struct descriptor_data *d);

extern void oedit_disp_val4_menu(struct descriptor_data *d);

extern void oedit_disp_val5_menu(struct descriptor_data *d);

extern void oedit_disp_val7_menu(struct descriptor_data *d);

extern void oedit_disp_val9_menu(struct descriptor_data *d);

extern void oedit_disp_type_menu(struct descriptor_data *d);

extern void oedit_disp_extra_menu(struct descriptor_data *d);

extern void oedit_disp_wear_menu(struct descriptor_data *d);

extern void oedit_disp_menu(struct descriptor_data *d);

extern void oedit_parse(struct descriptor_data *d, char *arg);

extern void oedit_disp_perm_menu(struct descriptor_data *d);

extern void oedit_string_cleanup(struct descriptor_data *d, int terminator);



extern void oedit_disp_use_menu(struct descriptor_data *d);

extern void iedit_setup_existing(struct descriptor_data *d, Object *obj);

extern void iedit_parse(struct descriptor_data *d, char *arg);

extern void redit_string_cleanup(struct descriptor_data *d, int terminator);

extern void redit_setup_new(struct descriptor_data *d);

extern void redit_setup_existing(struct descriptor_data *d, int real_num);

extern void redit_save_internally(struct descriptor_data *d);

extern void redit_save_to_disk(zone_vnum zone_num);

extern void redit_disp_extradesc_menu(struct descriptor_data *d);

extern void redit_disp_exit_menu(struct descriptor_data *d);

extern void redit_disp_exit_flag_menu(struct descriptor_data *d);

extern void redit_disp_flag_menu(struct descriptor_data *d);

extern void redit_disp_sector_menu(struct descriptor_data *d);

extern void redit_disp_menu(struct descriptor_data *d);

extern void redit_parse(struct descriptor_data *d, char *arg);

extern void free_room(Room *room);

extern void sedit_setup_new(struct descriptor_data *d);

extern void sedit_setup_existing(struct descriptor_data *d, vnum rshop_num);

extern void sedit_save_internally(struct descriptor_data *d);

extern void sedit_products_menu(struct descriptor_data *d);

extern void sedit_compact_rooms_menu(struct descriptor_data *d);

extern void sedit_rooms_menu(struct descriptor_data *d);

extern void sedit_namelist_menu(struct descriptor_data *d);

extern void sedit_shop_flags_menu(struct descriptor_data *d);

extern void sedit_no_trade_menu(struct descriptor_data *d);

extern void sedit_types_menu(struct descriptor_data *d);

extern void sedit_disp_menu(struct descriptor_data *d);

extern void sedit_parse(struct descriptor_data *d, char *arg);



extern void gedit_setup_new(struct descriptor_data *d);

extern void gedit_setup_existing(struct descriptor_data *d, int rgm_num);

extern void gedit_parse(struct descriptor_data *d, char *arg);

extern void gedit_disp_menu(struct descriptor_data *d);

extern void gedit_no_train_menu(struct descriptor_data *d);

extern void gedit_save_internally(struct descriptor_data *d);

extern void gedit_save_to_disk(int num);

extern void copy_guild(struct Guild *tgm, struct Guild *fgm);

extern void gedit_modify_string(std::string &str, char *new_g);

extern void zedit_setup(struct descriptor_data *d, int room_num);

extern void zedit_new_zone(Character *ch, zone_vnum vzone_num);

extern void zedit_save_internally(struct descriptor_data *d);

extern void zedit_disp_menu(struct descriptor_data *d);

extern void zedit_disp_comtype(struct descriptor_data *d);

extern void zedit_disp_arg1(struct descriptor_data *d);

extern void zedit_disp_arg2(struct descriptor_data *d);

extern void zedit_disp_arg3(struct descriptor_data *d);

extern void zedit_disp_arg4(struct descriptor_data *d);

extern void zedit_disp_arg5(struct descriptor_data *d);

extern void zedit_parse(struct descriptor_data *d, char *arg);

extern void zedit_disp_flag_menu(struct descriptor_data *d);



extern void cedit_setup(struct descriptor_data *d);

extern void cedit_parse(struct descriptor_data *d, char *arg);

extern void cedit_save_to_disk();

extern void cedit_string_cleanup(struct descriptor_data *d, int terminator);



extern void trigedit_parse(struct descriptor_data *d, char *arg);

extern void trigedit_setup_existing(struct descriptor_data *d, int rtrg_num);

extern void trigedit_setup_new(struct descriptor_data *d);


extern void aedit_disp_menu(struct descriptor_data *d);

extern void aedit_parse(struct descriptor_data *d, char *arg);

extern void aedit_setup_new(struct descriptor_data *d);

extern void aedit_setup_existing(struct descriptor_data *d, int real_num);

extern void aedit_save_to_disk(struct descriptor_data *d);

extern void aedit_save_internally(struct descriptor_data *d);

extern void free_action(struct social_messg *mess);

extern void hedit_parse(struct descriptor_data *d, char *arg);

extern void hedit_string_cleanup(struct descriptor_data *d, int terminator);

extern void free_help(struct help_index_element *help);

extern int parse_stats(struct descriptor_data *d, char *arg);

extern int stats_disp_menu(struct descriptor_data *d);

extern int free_strings(void *data, int type);

extern void list_rooms(Character *ch, room_vnum vmin, room_vnum vmax);

extern void list_mobiles(Character *ch, mob_vnum vmin, mob_vnum vmax);

extern void list_objects(Character *ch, obj_vnum vmin, obj_vnum vmax);

extern void list_shops(Character *ch, shop_vnum vmin, shop_vnum vmax);

extern void list_zones(Character *ch);

extern void print_zone(Character *ch, zone_vnum vnum);

extern int can_edit_zone(Character *ch, zone_rnum rnum);

#define CONTEXT_HELP_STRING "help"

constexpr int CONTEXT_OEDIT_MAIN_MENU = 1;
constexpr int CONTEXT_OEDIT_EDIT_NAMELIST = 2;
constexpr int CONTEXT_OEDIT_SHORTDESC = 3;
constexpr int CONTEXT_OEDIT_LONGDESC = 4;
constexpr int CONTEXT_OEDIT_ACTDESC = 5;
constexpr int CONTEXT_OEDIT_TYPE = 6;
constexpr int CONTEXT_OEDIT_EXTRAS = 7;
constexpr int CONTEXT_OEDIT_WEAR = 8;
constexpr int CONTEXT_OEDIT_WEIGHT = 9;
constexpr int CONTEXT_OEDIT_COST = 10;
constexpr int CONTEXT_OEDIT_COSTPERDAY = 11;
constexpr int CONTEXT_OEDIT_TIMER = 12;
constexpr int CONTEXT_OEDIT_VALUE_1 = 13;
constexpr int CONTEXT_OEDIT_VALUE_2 = 14;
constexpr int CONTEXT_OEDIT_VALUE_3 = 15;
constexpr int CONTEXT_OEDIT_VALUE_4 = 16;
constexpr int CONTEXT_OEDIT_APPLY = 17;
constexpr int CONTEXT_OEDIT_APPLYMOD = 18;
constexpr int CONTEXT_OEDIT_EXTRADESC_KEY = 19;
constexpr int CONTEXT_OEDIT_CONFIRM_SAVEDB = 20;
constexpr int CONTEXT_OEDIT_CONFIRM_SAVESTRING = 21;
constexpr int CONTEXT_OEDIT_PROMPT_APPLY = 22;
constexpr int CONTEXT_OEDIT_EXTRADESC_DESCRIPTION = 23;
constexpr int CONTEXT_OEDIT_EXTRADESC_MENU = 24;
constexpr int CONTEXT_OEDIT_LEVEL = 25;
constexpr int CONTEXT_OEDIT_PERM = 26;
constexpr int CONTEXT_REDIT_MAIN_MENU = 27;
constexpr int CONTEXT_REDIT_NAME = 28;
constexpr int CONTEXT_REDIT_DESC = 29;
constexpr int CONTEXT_REDIT_FLAGS = 30;
constexpr int CONTEXT_REDIT_SECTOR = 31;
constexpr int CONTEXT_REDIT_EXIT_MENU = 32;
constexpr int CONTEXT_REDIT_CONFIRM_SAVEDB = 33;
constexpr int CONTEXT_REDIT_CONFIRM_SAVESTRING = 34;
constexpr int CONTEXT_REDIT_EXIT_NUMBER = 35;
constexpr int CONTEXT_REDIT_EXIT_DESCRIPTION = 36;
constexpr int CONTEXT_REDIT_EXIT_KEYWORD = 37;
constexpr int CONTEXT_REDIT_EXIT_KEY = 38;
constexpr int CONTEXT_REDIT_EXIT_DOORFLAGS = 39;
constexpr int CONTEXT_REDIT_EXTRADESC_MENU = 40;
constexpr int CONTEXT_REDIT_EXTRADESC_KEY = 41;
constexpr int CONTEXT_REDIT_EXTRADESC_DESCRIPTION = 42;
constexpr int CONTEXT_ZEDIT_MAIN_MENU = 43;
constexpr int CONTEXT_ZEDIT_DELETE_ENTRY = 44;
constexpr int CONTEXT_ZEDIT_NEW_ENTRY = 45;
constexpr int CONTEXT_ZEDIT_CHANGE_ENTRY = 46;
constexpr int CONTEXT_ZEDIT_COMMAND_TYPE = 47;
constexpr int CONTEXT_ZEDIT_IF_FLAG = 48;
constexpr int CONTEXT_ZEDIT_ARG1 = 49;
constexpr int CONTEXT_ZEDIT_ARG2 = 50;
constexpr int CONTEXT_ZEDIT_ARG3 = 51;
constexpr int CONTEXT_ZEDIT_ZONE_NAME = 52;
constexpr int CONTEXT_ZEDIT_ZONE_LIFE = 53;
constexpr int CONTEXT_ZEDIT_ZONE_BOT = 54;
constexpr int CONTEXT_ZEDIT_ZONE_TOP = 55;
constexpr int CONTEXT_ZEDIT_ZONE_RESET = 56;
constexpr int CONTEXT_ZEDIT_CONFIRM_SAVESTRING = 57;
constexpr int CONTEXT_ZEDIT_SARG1 = 58;
constexpr int CONTEXT_ZEDIT_SARG2 = 59;
constexpr int CONTEXT_MEDIT_MAIN_MENU = 60;
constexpr int CONTEXT_MEDIT_ALIAS = 61;
constexpr int CONTEXT_MEDIT_S_DESC = 62;
constexpr int CONTEXT_MEDIT_L_DESC = 63;
constexpr int CONTEXT_MEDIT_D_DESC = 64;
constexpr int CONTEXT_MEDIT_NPC_FLAGS = 65;
constexpr int CONTEXT_MEDIT_AFF_FLAGS = 66;
constexpr int CONTEXT_MEDIT_CONFIRM_SAVESTRING = 67;
constexpr int CONTEXT_MEDIT_SEX = 68;
constexpr int CONTEXT_MEDIT_ACCURACY = 69;
constexpr int CONTEXT_MEDIT_DAMAGE = 70;
constexpr int CONTEXT_MEDIT_NDD = 71;
constexpr int CONTEXT_MEDIT_SDD = 72;
constexpr int CONTEXT_MEDIT_NUM_HP_DICE = 73;
constexpr int CONTEXT_MEDIT_SIZE_HP_DICE = 74;
constexpr int CONTEXT_MEDIT_ADD_HP = 75;
constexpr int CONTEXT_MEDIT_AC = 76;
constexpr int CONTEXT_MEDIT_EXP = 77;
constexpr int CONTEXT_MEDIT_GOLD = 78;
constexpr int CONTEXT_MEDIT_POS = 79;
constexpr int CONTEXT_MEDIT_DEFAULT_POS = 80;
constexpr int CONTEXT_MEDIT_ATTACK = 81;
constexpr int CONTEXT_MEDIT_LEVEL = 82;
constexpr int CONTEXT_MEDIT_ALIGNMENT = 83;
constexpr int CONTEXT_SEDIT_MAIN_MENU = 84;
constexpr int CONTEXT_SEDIT_CONFIRM_SAVESTRING = 85;
constexpr int CONTEXT_SEDIT_NOITEM1 = 86;
constexpr int CONTEXT_SEDIT_NOITEM2 = 87;
constexpr int CONTEXT_SEDIT_NOCASH1 = 88;
constexpr int CONTEXT_SEDIT_NOCASH2 = 89;
constexpr int CONTEXT_SEDIT_NOBUY = 90;
constexpr int CONTEXT_SEDIT_BUY = 91;
constexpr int CONTEXT_SEDIT_SELL = 92;
constexpr int CONTEXT_SEDIT_PRODUCTS_MENU = 93;
constexpr int CONTEXT_SEDIT_ROOMS_MENU = 94;
constexpr int CONTEXT_SEDIT_NAMELIST_MENU = 95;
constexpr int CONTEXT_SEDIT_NAMELIST = 96;
constexpr int CONTEXT_SEDIT_OPEN1 = 97;
constexpr int CONTEXT_SEDIT_OPEN2 = 98;
constexpr int CONTEXT_SEDIT_CLOSE1 = 99;
constexpr int CONTEXT_SEDIT_CLOSE2 = 100;
constexpr int CONTEXT_SEDIT_KEEPER = 101;
constexpr int CONTEXT_SEDIT_BUY_PROFIT = 102;
constexpr int CONTEXT_SEDIT_SELL_PROFIT = 103;
constexpr int CONTEXT_SEDIT_TYPE_MENU = 104;
constexpr int CONTEXT_SEDIT_DELETE_TYPE = 105;
constexpr int CONTEXT_SEDIT_DELETE_PRODUCT = 106;
constexpr int CONTEXT_SEDIT_NEW_PRODUCT = 107;
constexpr int CONTEXT_SEDIT_DELETE_ROOM = 108;
constexpr int CONTEXT_SEDIT_NEW_ROOM = 109;
constexpr int CONTEXT_SEDIT_SHOP_FLAGS = 110;
constexpr int CONTEXT_SEDIT_NOTRADE = 111;
constexpr int CONTEXT_TRIGEDIT_MAIN_MENU = 112;
constexpr int CONTEXT_TRIGEDIT_TRIGTYPE = 113;
constexpr int CONTEXT_TRIGEDIT_CONFIRM_SAVESTRING = 114;
constexpr int CONTEXT_TRIGEDIT_NAME = 115;
constexpr int CONTEXT_TRIGEDIT_INTENDED = 116;
constexpr int CONTEXT_TRIGEDIT_TYPES = 117;
constexpr int CONTEXT_TRIGEDIT_COMMANDS = 118;
constexpr int CONTEXT_TRIGEDIT_NARG = 119;
constexpr int CONTEXT_TRIGEDIT_ARGUMENT = 120;
constexpr int CONTEXT_SCRIPT_MAIN_MENU = 121;
constexpr int CONTEXT_SCRIPT_NEW_TRIGGER = 122;
constexpr int CONTEXT_SCRIPT_DEL_TRIGGER = 123;
constexpr int CONTEXT_ZEDIT_ARG4 = 124;
constexpr int CONTEXT_GEDIT_MAIN_MENU = 125;
constexpr int CONTEXT_GEDIT_CONFIRM_SAVESTRING = 126;
constexpr int CONTEXT_GEDIT_NO_CASH = 127;
constexpr int CONTEXT_GEDIT_NO_SKILL = 128;
constexpr int CONTEXT_GEDIT_NUMERICAL_RESPONSE = 129;
constexpr int CONTEXT_GEDIT_CHARGE = 130;
constexpr int CONTEXT_GEDIT_OPEN = 131;
constexpr int CONTEXT_GEDIT_CLOSE = 132;
constexpr int CONTEXT_GEDIT_TRAINER = 133;
constexpr int CONTEXT_GEDIT_NO_TRAIN = 134;
constexpr int CONTEXT_GEDIT_MINLVL = 135;
constexpr int CONTEXT_GEDIT_SELECT_SPELLS = 136;
constexpr int CONTEXT_GEDIT_SELECT_SKILLS = 137;
constexpr int CONTEXT_GEDIT_SELECT_WPS = 138;
constexpr int CONTEXT_GEDIT_SELECT_LANGS = 139;

/* includes number 0 */
constexpr int NUM_CONTEXTS = 140;

/* Prototypes for the context sensitive help system */
extern int find_context(struct descriptor_data *d);

extern int find_context_oedit(struct descriptor_data *d);

extern int find_context_redit(struct descriptor_data *d);

extern int find_context_zedit(struct descriptor_data *d);

extern int find_context_medit(struct descriptor_data *d);

extern int find_context_sedit(struct descriptor_data *d);

extern int find_context_gedit(struct descriptor_data *d);

extern int context_help(struct descriptor_data *d, char *arg);

extern void boot_context_help();

extern void free_context_help();

#endif /* ifndef __GENOLC_C__ */


/*. Submodes of GEDIT connectedness     . */
constexpr int GEDIT_MAIN_MENU = 0;
constexpr int GEDIT_CONFIRM_SAVESTRING = 1;
constexpr int GEDIT_NO_CASH = 2;
constexpr int GEDIT_NO_SKILL = 3;

/*. Numerical responses . */
constexpr int GEDIT_NUMERICAL_RESPONSE = 5;
constexpr int GEDIT_CHARGE = 6;
constexpr int GEDIT_OPEN = 7;
constexpr int GEDIT_CLOSE = 8;
constexpr int GEDIT_TRAINER = 9;
constexpr int GEDIT_NO_TRAIN = 10;
constexpr int GEDIT_MINLVL = 11;
constexpr int GEDIT_SELECT_SPELLS = 12;
constexpr int GEDIT_SELECT_SKILLS = 13;
constexpr int GEDIT_SELECT_WPS = 14;
constexpr int GEDIT_SELECT_LANGS = 15;
constexpr int GEDIT_SELECT_FEATS = 16;

/*
 * Statedit Connectedness
 * --relistan 2/23/99
 */

constexpr int STAT_GET_STR = 0;
constexpr int STAT_GET_INT = 1;
constexpr int STAT_GET_WIS = 2;
constexpr int STAT_GET_DEX = 3;
constexpr int STAT_GET_CON = 4;
constexpr int STAT_GET_CHA = 5;
constexpr int STAT_QUIT = 6;
constexpr int STAT_PARSE_MENU = 7;
