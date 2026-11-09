#pragma once
#include <string>

#include "dbat/consts/maximums.hpp"
#include "dbat/consts/zoneflags.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/room_utils.hpp"

constexpr int _OASISOLC = 0x206; /* 2.0.6 */
/*
 * Used to determine what version of OasisOLC is installed.
 *
 * Ex: #if _OASISOLC >= OASIS_VERSION(2,0,0)
 */
#define OASIS_VERSION(x, y, z)	(((x) << 8 | (y) << 4 | (z))

constexpr zone_vnum AEDIT_PERMISSION = 999; /* arbitrary number higher than max zone vnum*/
constexpr zone_vnum HEDIT_PERMISSION = 888; /* arbitrary number higher then max zone vnum*/

/*
 * Macros, defines, structs and globals for the OLC suite.  You will need
 * to adjust these numbers if you ever add more.
 */

constexpr int NUM_GENDERS = 3;

/* -------------------------------------------------------------------------- */

/*
 * Limit information.
 */
constexpr int MAX_ROOM_NAME  = 100;
constexpr int MAX_MOB_NAME   = 50;
constexpr int MAX_OBJ_NAME   = 50;
constexpr int MAX_ROOM_DESC  = 4096;
constexpr int MAX_EXIT_DESC  = 256;
constexpr int MAX_EXTRA_DESC = 512;
constexpr int MAX_MOB_DESC   = 1024;
constexpr int MAX_OBJ_DESC   = 512;
constexpr int MAX_DUPLICATES = 10000; /* when loading in zedit */
constexpr int MAX_FROM_ROOM  = 50;    /* when loading in zedit */

/* arbitrary limits - roll your own */
/* max weapon is 50d50 .. avg. 625 dam... */
constexpr int MAX_WEAPON_SDICE = 50;
constexpr int MAX_WEAPON_NDICE = 50;

constexpr int MAX_OBJ_WEIGHT     = 100000000;
constexpr int MAX_OBJ_COST       = 2000000;
constexpr int MAX_OBJ_RENT       = 2000000;
constexpr int MAX_CONTAINER_SIZE = 500000000;

constexpr int MAX_MOB_GOLD = 1000000;
constexpr int MAX_MOB_EXP  = 1500000;
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
void cleanup_olc(GameWorld *g, struct descriptor_data *d, int8_t cleanup_type);
void split_argument(char *argument, char *tag);
void send_cannot_edit(GameWorld *g, struct char_data *ch, zone_vnum zone);

/*
 * OLC structures.
 */
/* -------------------------------------------------------------------------- */

struct oasis_olc_data {
  int mode{};                        /* how to parse input       */
  int index{};                       /* aedit's social, hedit's help entry */
  struct zone_data *in_zone{};       /* the zone of what is being edited */
  room_vnum number{};                /* vnum of subject          */
  int value{};                       /* mostly 'has changed' flag*/
  std::string storage{};             /* tedit's file, trigedit's script, the new keyword for hedit and aedit */
  struct char_data *mob{};           /* used for 'medit'         */
  struct room_data *room{};          /* used for 'redit'         */
  struct obj_data *obj{};            /* used for 'oedit'         */
  int32_t iobj_uid{};                /* the object 'iedit' changes, by id */
  struct zone_data *zone{};          /* used for 'zedit'         */
  struct shop_data *shop{};          /* used for 'sedit'         */
  struct house_control_rec *house{}; /* used for 'hsedit'      */
  struct config_data *config{};      /* used for 'cedit'         */
  struct extra_descr_data *desc{};   /* used in '[r|o|m]edit'    */
  struct social_messg *action{};     /* Aedit uses this one      */
  struct trig_data *trig{};
  int script_mode{};
  int trigger_position{};
  int item_type{};
  struct trig_proto_list *script{};    /* for assigning triggers in [r|o|m]edit*/
  struct assembly_data *OlcAssembly{}; /* used for 'assedit'         */
  struct guild_data *guild{};          /* used for 'gedit'         */
  struct help_index_element *help{};   /* Hedit uses this */
};

/*
 * Descriptor access macros.
 */
#define OLC(d)      ((d)->olc)
#define OLC_MODE(d) (OLC(d)->mode)     /* Parse input mode.	*/
#define OLC_NUM(d)  (OLC(d)->number)   /* Room/Obj VNUM.	*/
#define OLC_VAL(d)  (OLC(d)->value)    /* Scratch variable.	*/
#define OLC_ZNUM(d)  (OLC(d)->in_zone)  /* Zone of what is being edited. */
#define OLC_INDEX(d) (OLC(d)->index)    /* aedit's and hedit's entry. */

#define OLC_STORAGE(d) (OLC(d)->storage)     /* std::string.	*/
#define OLC_ROOM(d)    (OLC(d)->room)        /* Room structure.	*/
#define OLC_OBJ(d)     (OLC(d)->obj)         /* Object structure.	*/
#define OLC_ZONE(d)    (OLC(d)->zone)        /* Zone structure.	*/
#define OLC_MOB(d)     (OLC(d)->mob)         /* Mob structure.	*/
#define OLC_HOUSE(d)   (OLC(d)->house)       /* house structure      */
#define OLC_SHOP(d)    (OLC(d)->shop)        /* Shop structure.	*/
#define OLC_DESC(d)    (OLC(d)->desc)        /* Extra description.	*/
#define OLC_CONFIG(d)  (OLC(d)->config)      /* Config structure.	*/
#define OLC_TRIG(d)    (OLC(d)->trig)        /* Trigger structure.   */
#define OLC_ASSEDIT(d) (OLC(d)->OlcAssembly) /* assembly olc        */

#define OLC_ACTION(d) (OLC(d)->action) /* Action structure     */
#define OLC_GUILD(d)  (OLC(d)->guild)  /* Guild structure      */
#define OLC_HELP(d)   (OLC(d)->help)   /* Hedit structure      */

/*
 * Other macros.
 */
#define OLC_EXIT(d) room_get_dir_option(OLC_ROOM(d), OLC_VAL(d))

/*
 * Cleanup types.
 */
constexpr int CLEANUP_ALL     = 1; /* Free the whole lot.			*/
constexpr int CLEANUP_STRUCTS = 2; /* Don't free strings.			*/
constexpr int CLEANUP_CONFIG  = 3; /* Used just to send proper message. 	*/

/* Submodes of AEDIT connectedness     */
constexpr int AEDIT_CONFIRM_SAVESTRING     = 0;
constexpr int AEDIT_CONFIRM_EDIT           = 1;
constexpr int AEDIT_CONFIRM_ADD            = 2;
constexpr int AEDIT_MAIN_MENU              = 3;
constexpr int AEDIT_ACTION_NAME            = 4;
constexpr int AEDIT_SORT_AS                = 5;
constexpr int AEDIT_MIN_CHAR_POS           = 6;
constexpr int AEDIT_MIN_VICT_POS           = 7;
constexpr int AEDIT_HIDDEN_FLAG            = 8;
constexpr int AEDIT_MIN_CHAR_LEVEL         = 9;
constexpr int AEDIT_NOVICT_CHAR            = 10;
constexpr int AEDIT_NOVICT_OTHERS          = 11;
constexpr int AEDIT_VICT_CHAR_FOUND        = 12;
constexpr int AEDIT_VICT_OTHERS_FOUND      = 13;
constexpr int AEDIT_VICT_VICT_FOUND        = 14;
constexpr int AEDIT_VICT_NOT_FOUND         = 15;
constexpr int AEDIT_SELF_CHAR              = 16;
constexpr int AEDIT_SELF_OTHERS            = 17;
constexpr int AEDIT_VICT_CHAR_BODY_FOUND   = 18;
constexpr int AEDIT_VICT_OTHERS_BODY_FOUND = 19;
constexpr int AEDIT_VICT_VICT_BODY_FOUND   = 20;
constexpr int AEDIT_OBJ_CHAR_FOUND         = 21;
constexpr int AEDIT_OBJ_OTHERS_FOUND       = 22;
/*
 * Submodes of OEDIT connectedness.
 */
constexpr int OEDIT_MAIN_MENU             = 1;
constexpr int OEDIT_EDIT_NAMELIST         = 2;
constexpr int OEDIT_SHORTDESC             = 3;
constexpr int OEDIT_LONGDESC              = 4;
constexpr int OEDIT_ACTDESC               = 5;
constexpr int OEDIT_TYPE                  = 6;
constexpr int OEDIT_EXTRAS                = 7;
constexpr int OEDIT_WEAR                  = 8;
constexpr int OEDIT_WEIGHT                = 9;
constexpr int OEDIT_COST                  = 10;
constexpr int OEDIT_COSTPERDAY            = 11;
constexpr int OEDIT_TIMER                 = 12;
constexpr int OEDIT_VALUE_1               = 13;
constexpr int OEDIT_VALUE_2               = 14;
constexpr int OEDIT_VALUE_3               = 15;
constexpr int OEDIT_VALUE_4               = 16;
constexpr int OEDIT_APPLY                 = 17;
constexpr int OEDIT_APPLYMOD              = 18;
constexpr int OEDIT_EXTRADESC_KEY         = 19;
constexpr int OEDIT_CONFIRM_SAVEDB        = 20;
constexpr int OEDIT_CONFIRM_SAVESTRING    = 21;
constexpr int OEDIT_PROMPT_APPLY          = 22;
constexpr int OEDIT_EXTRADESC_DESCRIPTION = 23;
constexpr int OEDIT_EXTRADESC_MENU        = 24;
constexpr int OEDIT_LEVEL                 = 25;
constexpr int OEDIT_PERM                  = 26;
constexpr int OEDIT_VALUE_5               = 27;
constexpr int OEDIT_VALUE_6               = 28;
constexpr int OEDIT_VALUE_7               = 29;
constexpr int OEDIT_VALUE_8               = 30;
constexpr int OEDIT_MATERIAL              = 31;
constexpr int OEDIT_VALUE_9               = 32;
constexpr int OEDIT_VALUE_10              = 33;
constexpr int OEDIT_VALUE_11              = 34;
constexpr int OEDIT_VALUE_12              = 35;
constexpr int OEDIT_VALUE_13              = 36;
constexpr int OEDIT_VALUE_14              = 37;
constexpr int OEDIT_VALUE_15              = 38;
constexpr int OEDIT_VALUE_16              = 39;
constexpr int OEDIT_SIZE                  = 40;
constexpr int OEDIT_APPLYSPEC             = 41;
constexpr int OEDIT_PROMPT_SPELLBOOK      = 42;
constexpr int OEDIT_SPELLBOOK             = 43;
constexpr int OEDIT_COPY                  = 44;
constexpr int OEDIT_DELETE                = 45;

/*
 * Submodes of REDIT connectedness.
 */
constexpr int REDIT_MAIN_MENU             = 1;
constexpr int REDIT_NAME                  = 2;
constexpr int REDIT_DESC                  = 3;
constexpr int REDIT_FLAGS                 = 4;
constexpr int REDIT_SECTOR                = 5;
constexpr int REDIT_EXIT_MENU             = 6;
constexpr int REDIT_CONFIRM_SAVEDB        = 7;
constexpr int REDIT_CONFIRM_SAVESTRING    = 8;
constexpr int REDIT_EXIT_NUMBER           = 9;
constexpr int REDIT_EXIT_DESCRIPTION      = 10;
constexpr int REDIT_EXIT_KEYWORD          = 11;
constexpr int REDIT_EXIT_KEY              = 12;
constexpr int REDIT_EXIT_DOORFLAGS        = 13;
constexpr int REDIT_EXTRADESC_MENU        = 14;
constexpr int REDIT_EXTRADESC_KEY         = 15;
constexpr int REDIT_EXTRADESC_DESCRIPTION = 16;
constexpr int REDIT_DELETE                = 17;
constexpr int REDIT_EXIT_DCLOCK           = 18;
constexpr int REDIT_EXIT_DCHIDE           = 19;
constexpr int REDIT_EXIT_DCSKILL          = 20;
constexpr int REDIT_EXIT_DCMOVE           = 21;
constexpr int REDIT_EXIT_SAVETYPE         = 22;
constexpr int REDIT_EXIT_DCSAVE           = 23;
constexpr int REDIT_EXIT_FAILROOM         = 24;
constexpr int REDIT_EXIT_TOTALFAILROOM    = 25;
constexpr int REDIT_COPY                  = 26;

/*
 * Submodes of ZEDIT connectedness.
 */
constexpr int ZEDIT_MAIN_MENU          = 0;
constexpr int ZEDIT_DELETE_ENTRY       = 1;
constexpr int ZEDIT_NEW_ENTRY          = 2;
constexpr int ZEDIT_CHANGE_ENTRY       = 3;
constexpr int ZEDIT_COMMAND_TYPE       = 4;
constexpr int ZEDIT_IF_FLAG            = 5;
constexpr int ZEDIT_ARG1               = 6;
constexpr int ZEDIT_ARG2               = 7;
constexpr int ZEDIT_ARG3               = 8;
constexpr int ZEDIT_ARG4               = 9;
constexpr int ZEDIT_ARG5               = 10;
constexpr int ZEDIT_ZONE_NAME          = 11;
constexpr int ZEDIT_ZONE_LIFE          = 12;
constexpr int ZEDIT_ZONE_BOT           = 13;
constexpr int ZEDIT_ZONE_TOP           = 14;
constexpr int ZEDIT_ZONE_RESET         = 15;
constexpr int ZEDIT_CONFIRM_SAVESTRING = 16;
constexpr int ZEDIT_ZONE_BUILDERS      = 17;
constexpr int ZEDIT_SARG1              = 18;
constexpr int ZEDIT_SARG2              = 19;
constexpr int ZEDIT_ZONE_FLAGS         = 20;
constexpr int ZEDIT_MIN_LEVEL          = 21;
constexpr int ZEDIT_MAX_LEVEL          = 22;

/*
 * Submodes of MEDIT connectedness.
 */
constexpr int MEDIT_MAIN_MENU          = 0;
constexpr int MEDIT_ALIAS              = 1;
constexpr int MEDIT_S_DESC             = 2;
constexpr int MEDIT_L_DESC             = 3;
constexpr int MEDIT_D_DESC             = 4;
constexpr int MEDIT_NPC_FLAGS          = 5;
constexpr int MEDIT_AFF_FLAGS          = 6;
constexpr int MEDIT_CONFIRM_SAVESTRING = 7;
/*
 * Numerical responses.
 */
constexpr int MEDIT_NUMERICAL_RESPONSE = 10;
constexpr int MEDIT_SEX                = 11;
constexpr int MEDIT_ACCURACY           = 12;
constexpr int MEDIT_DAMAGE             = 13;
constexpr int MEDIT_NDD                = 14;
constexpr int MEDIT_SDD                = 15;
constexpr int MEDIT_NUM_HP_DICE        = 16;
constexpr int MEDIT_SIZE_HP_DICE       = 17;
constexpr int MEDIT_ADD_HP             = 18;
constexpr int MEDIT_AC                 = 19;
constexpr int MEDIT_EXP                = 20;
constexpr int MEDIT_GOLD               = 21;
constexpr int MEDIT_POS                = 22;
constexpr int MEDIT_DEFAULT_POS        = 23;
constexpr int MEDIT_ATTACK             = 24;
constexpr int MEDIT_LEVEL              = 25;
constexpr int MEDIT_ALIGNMENT          = 26;
constexpr int MEDIT_CLASS              = 33;
constexpr int MEDIT_RACE               = 34;
constexpr int MEDIT_SIZE               = 35;
constexpr int MEDIT_COPY               = 36;
constexpr int MEDIT_DELETE             = 37;
constexpr int MEDIT_PERSONALITY        = 38;

/*
 * Submodes of SEDIT connectedness.
 */
constexpr int SEDIT_MAIN_MENU          = 0;
constexpr int SEDIT_CONFIRM_SAVESTRING = 1;
constexpr int SEDIT_NOITEM1            = 2;
constexpr int SEDIT_NOITEM2            = 3;
constexpr int SEDIT_NOCASH1            = 4;
constexpr int SEDIT_NOCASH2            = 5;
constexpr int SEDIT_NOBUY              = 6;
constexpr int SEDIT_BUY                = 7;
constexpr int SEDIT_SELL               = 8;
constexpr int SEDIT_PRODUCTS_MENU      = 11;
constexpr int SEDIT_ROOMS_MENU         = 12;
constexpr int SEDIT_NAMELIST_MENU      = 13;
constexpr int SEDIT_NAMELIST           = 14;
/*
 * Numerical responses.
 */
constexpr int SEDIT_NUMERICAL_RESPONSE = 20;
constexpr int SEDIT_OPEN1              = 21;
constexpr int SEDIT_OPEN2              = 22;
constexpr int SEDIT_CLOSE1             = 23;
constexpr int SEDIT_CLOSE2             = 24;
constexpr int SEDIT_KEEPER             = 25;
constexpr int SEDIT_BUY_PROFIT         = 26;
constexpr int SEDIT_SELL_PROFIT        = 27;
constexpr int SEDIT_TYPE_MENU          = 29;
constexpr int SEDIT_DELETE_TYPE        = 30;
constexpr int SEDIT_DELETE_PRODUCT     = 31;
constexpr int SEDIT_NEW_PRODUCT        = 32;
constexpr int SEDIT_DELETE_ROOM        = 33;
constexpr int SEDIT_NEW_ROOM           = 34;
constexpr int SEDIT_SHOP_FLAGS         = 35;
constexpr int SEDIT_NOTRADE            = 36;
constexpr int SEDIT_COPY               = 37;

/* 
 * Submodes of CEDIT connectedness.
 */
constexpr int CEDIT_MAIN_MENU          = 0;
constexpr int CEDIT_CONFIRM_SAVESTRING = 1;
constexpr int CEDIT_SUBMENU            = 2; /* OLC_VAL: the submenu's main menu key */
constexpr int CEDIT_OPTION             = 3; /* OLC_VAL: the option's index in cedit_options[] */

constexpr int ASSEDIT_DO_NOT_USE       = 0;
constexpr int ASSEDIT_MAIN_MENU        = 1;
constexpr int ASSEDIT_ADD_COMPONENT    = 2;
constexpr int ASSEDIT_EDIT_COMPONENT   = 3;
constexpr int ASSEDIT_DELETE_COMPONENT = 4;
constexpr int ASSEDIT_EDIT_EXTRACT     = 5;
constexpr int ASSEDIT_EDIT_INROOM      = 6;
constexpr int ASSEDIT_EDIT_TYPES       = 7;

constexpr int CEDIT_CREATION_METHOD_1 = 0;
constexpr int CEDIT_CREATION_METHOD_2 = 1;
constexpr int CEDIT_CREATION_METHOD_3 = 2;
constexpr int CEDIT_CREATION_METHOD_4 = 3;
constexpr int CEDIT_CREATION_METHOD_5 = 4;

/* Submodes of HEDIT connectedness     */
constexpr int HEDIT_CONFIRM_SAVESTRING = 0;
constexpr int HEDIT_CONFIRM_EDIT       = 1;
constexpr int HEDIT_CONFIRM_ADD        = 2;
constexpr int HEDIT_MAIN_MENU          = 3;
constexpr int HEDIT_ENTRY              = 4;
constexpr int HEDIT_KEYWORDS           = 5;
constexpr int HEDIT_MIN_LEVEL          = 6;

/*. House editor .*/
constexpr int HSEDIT_MAIN_MENU          = 0;
constexpr int HSEDIT_CONFIRM_SAVESTRING = 1;
constexpr int HSEDIT_OWNER_MENU         = 2;
constexpr int HSEDIT_OWNER_NAME         = 3;
constexpr int HSEDIT_OWNER_ID           = 4;
constexpr int HSEDIT_ROOM               = 5;
constexpr int HSEDIT_ATRIUM             = 6;
constexpr int HSEDIT_DIR_MENU           = 7;
constexpr int HSEDIT_GUEST_MENU         = 8;
constexpr int HSEDIT_GUEST_ADD          = 9;
constexpr int HSEDIT_GUEST_DELETE       = 10;
constexpr int HSEDIT_GUEST_CLEAR        = 11;
constexpr int HSEDIT_FLAGS              = 12;
constexpr int HSEDIT_BUILD_DATE         = 13;
constexpr int HSEDIT_PAYMENT            = 14;
constexpr int HSEDIT_TYPE               = 15;
constexpr int HSEDIT_DELETE             = 16;
constexpr int HSEDIT_VALUE_0            = 17;
constexpr int HSEDIT_VALUE_1            = 18;
constexpr int HSEDIT_VALUE_2            = 19;
constexpr int HSEDIT_VALUE_3            = 20;
constexpr int HSEDIT_NOVNUM             = 21;
constexpr int HSEDIT_BUILDER            = 22;

/* -------------------------------------------------------------------------- */

#ifndef __GENOLC_C__

/*
 * Prototypes to keep.
 */
#ifndef ACMD
#define ACMD(name) \
  void name(GameWorld *g, struct char_data *ch, char *argument, int cmd, int subcmd)
#endif
void clear_screen(GameWorld *g, struct descriptor_data *);
ACMD(do_oasis);

/*
 * Prototypes, to be moved later.
 */
ACMD(do_oasis_list);
ACMD(do_oasis_links);

ACMD(do_iedit);
ACMD(do_show_save_list);
ACMD(do_rcopy);

void medit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void medit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator);

void oedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void oedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator);
ACMD(do_oasis_oedit);

void redit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator);
void redit_save_internally(GameWorld *g, struct descriptor_data *d);
void redit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void free_room(GameWorld *g, struct room_data *room);
ACMD(do_oasis_redit);

void sedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
ACMD(do_oasis_gedit);

void zedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
ACMD(do_oasis_zedit);

void cedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void cedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator);
ACMD(do_oasis_cedit);

void trigedit_parse(GameWorld *g, struct descriptor_data *d, char *arg);

ACMD(do_oasis_hsedit);

bool parse_stats(GameWorld *g, struct descriptor_data *d, char *arg);

void print_zone(GameWorld *g, struct char_data *ch, zone_vnum vnum);
bool can_edit_zone(GameWorld *g, struct char_data *ch, struct zone_data *rnum);

#define CONTEXT_HELP_STRING "help"

constexpr int CONTEXT_OEDIT_MAIN_MENU             = 1;
constexpr int CONTEXT_OEDIT_EDIT_NAMELIST         = 2;
constexpr int CONTEXT_OEDIT_SHORTDESC             = 3;
constexpr int CONTEXT_OEDIT_LONGDESC              = 4;
constexpr int CONTEXT_OEDIT_ACTDESC               = 5;
constexpr int CONTEXT_OEDIT_TYPE                  = 6;
constexpr int CONTEXT_OEDIT_EXTRAS                = 7;
constexpr int CONTEXT_OEDIT_WEAR                  = 8;
constexpr int CONTEXT_OEDIT_WEIGHT                = 9;
constexpr int CONTEXT_OEDIT_COST                  = 10;
constexpr int CONTEXT_OEDIT_COSTPERDAY            = 11;
constexpr int CONTEXT_OEDIT_TIMER                 = 12;
constexpr int CONTEXT_OEDIT_VALUE_1               = 13;
constexpr int CONTEXT_OEDIT_VALUE_2               = 14;
constexpr int CONTEXT_OEDIT_VALUE_3               = 15;
constexpr int CONTEXT_OEDIT_VALUE_4               = 16;
constexpr int CONTEXT_OEDIT_APPLY                 = 17;
constexpr int CONTEXT_OEDIT_APPLYMOD              = 18;
constexpr int CONTEXT_OEDIT_EXTRADESC_KEY         = 19;
constexpr int CONTEXT_OEDIT_CONFIRM_SAVEDB        = 20;
constexpr int CONTEXT_OEDIT_CONFIRM_SAVESTRING    = 21;
constexpr int CONTEXT_OEDIT_PROMPT_APPLY          = 22;
constexpr int CONTEXT_OEDIT_EXTRADESC_DESCRIPTION = 23;
constexpr int CONTEXT_OEDIT_EXTRADESC_MENU        = 24;
constexpr int CONTEXT_OEDIT_LEVEL                 = 25;
constexpr int CONTEXT_OEDIT_PERM                  = 26;
constexpr int CONTEXT_REDIT_MAIN_MENU             = 27;
constexpr int CONTEXT_REDIT_NAME                  = 28;
constexpr int CONTEXT_REDIT_DESC                  = 29;
constexpr int CONTEXT_REDIT_FLAGS                 = 30;
constexpr int CONTEXT_REDIT_SECTOR                = 31;
constexpr int CONTEXT_REDIT_EXIT_MENU             = 32;
constexpr int CONTEXT_REDIT_CONFIRM_SAVEDB        = 33;
constexpr int CONTEXT_REDIT_CONFIRM_SAVESTRING    = 34;
constexpr int CONTEXT_REDIT_EXIT_NUMBER           = 35;
constexpr int CONTEXT_REDIT_EXIT_DESCRIPTION      = 36;
constexpr int CONTEXT_REDIT_EXIT_KEYWORD          = 37;
constexpr int CONTEXT_REDIT_EXIT_KEY              = 38;
constexpr int CONTEXT_REDIT_EXIT_DOORFLAGS        = 39;
constexpr int CONTEXT_REDIT_EXTRADESC_MENU        = 40;
constexpr int CONTEXT_REDIT_EXTRADESC_KEY         = 41;
constexpr int CONTEXT_REDIT_EXTRADESC_DESCRIPTION = 42;
constexpr int CONTEXT_ZEDIT_MAIN_MENU             = 43;
constexpr int CONTEXT_ZEDIT_DELETE_ENTRY          = 44;
constexpr int CONTEXT_ZEDIT_NEW_ENTRY             = 45;
constexpr int CONTEXT_ZEDIT_CHANGE_ENTRY          = 46;
constexpr int CONTEXT_ZEDIT_COMMAND_TYPE          = 47;
constexpr int CONTEXT_ZEDIT_IF_FLAG               = 48;
constexpr int CONTEXT_ZEDIT_ARG1                  = 49;
constexpr int CONTEXT_ZEDIT_ARG2                  = 50;
constexpr int CONTEXT_ZEDIT_ARG3                  = 51;
constexpr int CONTEXT_ZEDIT_ZONE_NAME             = 52;
constexpr int CONTEXT_ZEDIT_ZONE_LIFE             = 53;
constexpr int CONTEXT_ZEDIT_ZONE_BOT              = 54;
constexpr int CONTEXT_ZEDIT_ZONE_TOP              = 55;
constexpr int CONTEXT_ZEDIT_ZONE_RESET            = 56;
constexpr int CONTEXT_ZEDIT_CONFIRM_SAVESTRING    = 57;
constexpr int CONTEXT_ZEDIT_SARG1                 = 58;
constexpr int CONTEXT_ZEDIT_SARG2                 = 59;
constexpr int CONTEXT_MEDIT_MAIN_MENU             = 60;
constexpr int CONTEXT_MEDIT_ALIAS                 = 61;
constexpr int CONTEXT_MEDIT_S_DESC                = 62;
constexpr int CONTEXT_MEDIT_L_DESC                = 63;
constexpr int CONTEXT_MEDIT_D_DESC                = 64;
constexpr int CONTEXT_MEDIT_NPC_FLAGS             = 65;
constexpr int CONTEXT_MEDIT_AFF_FLAGS             = 66;
constexpr int CONTEXT_MEDIT_CONFIRM_SAVESTRING    = 67;
constexpr int CONTEXT_MEDIT_SEX                   = 68;
constexpr int CONTEXT_MEDIT_ACCURACY              = 69;
constexpr int CONTEXT_MEDIT_DAMAGE                = 70;
constexpr int CONTEXT_MEDIT_NDD                   = 71;
constexpr int CONTEXT_MEDIT_SDD                   = 72;
constexpr int CONTEXT_MEDIT_NUM_HP_DICE           = 73;
constexpr int CONTEXT_MEDIT_SIZE_HP_DICE          = 74;
constexpr int CONTEXT_MEDIT_ADD_HP                = 75;
constexpr int CONTEXT_MEDIT_AC                    = 76;
constexpr int CONTEXT_MEDIT_EXP                   = 77;
constexpr int CONTEXT_MEDIT_GOLD                  = 78;
constexpr int CONTEXT_MEDIT_POS                   = 79;
constexpr int CONTEXT_MEDIT_DEFAULT_POS           = 80;
constexpr int CONTEXT_MEDIT_ATTACK                = 81;
constexpr int CONTEXT_MEDIT_LEVEL                 = 82;
constexpr int CONTEXT_MEDIT_ALIGNMENT             = 83;
constexpr int CONTEXT_SEDIT_MAIN_MENU             = 84;
constexpr int CONTEXT_SEDIT_CONFIRM_SAVESTRING    = 85;
constexpr int CONTEXT_SEDIT_NOITEM1               = 86;
constexpr int CONTEXT_SEDIT_NOITEM2               = 87;
constexpr int CONTEXT_SEDIT_NOCASH1               = 88;
constexpr int CONTEXT_SEDIT_NOCASH2               = 89;
constexpr int CONTEXT_SEDIT_NOBUY                 = 90;
constexpr int CONTEXT_SEDIT_BUY                   = 91;
constexpr int CONTEXT_SEDIT_SELL                  = 92;
constexpr int CONTEXT_SEDIT_PRODUCTS_MENU         = 93;
constexpr int CONTEXT_SEDIT_ROOMS_MENU            = 94;
constexpr int CONTEXT_SEDIT_NAMELIST_MENU         = 95;
constexpr int CONTEXT_SEDIT_NAMELIST              = 96;
constexpr int CONTEXT_SEDIT_OPEN1                 = 97;
constexpr int CONTEXT_SEDIT_OPEN2                 = 98;
constexpr int CONTEXT_SEDIT_CLOSE1                = 99;
constexpr int CONTEXT_SEDIT_CLOSE2                = 100;
constexpr int CONTEXT_SEDIT_KEEPER                = 101;
constexpr int CONTEXT_SEDIT_BUY_PROFIT            = 102;
constexpr int CONTEXT_SEDIT_SELL_PROFIT           = 103;
constexpr int CONTEXT_SEDIT_TYPE_MENU             = 104;
constexpr int CONTEXT_SEDIT_DELETE_TYPE           = 105;
constexpr int CONTEXT_SEDIT_DELETE_PRODUCT        = 106;
constexpr int CONTEXT_SEDIT_NEW_PRODUCT           = 107;
constexpr int CONTEXT_SEDIT_DELETE_ROOM           = 108;
constexpr int CONTEXT_SEDIT_NEW_ROOM              = 109;
constexpr int CONTEXT_SEDIT_SHOP_FLAGS            = 110;
constexpr int CONTEXT_SEDIT_NOTRADE               = 111;
constexpr int CONTEXT_TRIGEDIT_MAIN_MENU          = 112;
constexpr int CONTEXT_TRIGEDIT_TRIGTYPE           = 113;
constexpr int CONTEXT_TRIGEDIT_CONFIRM_SAVESTRING = 114;
constexpr int CONTEXT_TRIGEDIT_NAME               = 115;
constexpr int CONTEXT_TRIGEDIT_INTENDED           = 116;
constexpr int CONTEXT_TRIGEDIT_TYPES              = 117;
constexpr int CONTEXT_TRIGEDIT_COMMANDS           = 118;
constexpr int CONTEXT_TRIGEDIT_NARG               = 119;
constexpr int CONTEXT_TRIGEDIT_ARGUMENT           = 120;
constexpr int CONTEXT_SCRIPT_MAIN_MENU            = 121;
constexpr int CONTEXT_SCRIPT_NEW_TRIGGER          = 122;
constexpr int CONTEXT_SCRIPT_DEL_TRIGGER          = 123;
constexpr int CONTEXT_ZEDIT_ARG4                  = 124;
constexpr int CONTEXT_GEDIT_MAIN_MENU             = 125;
constexpr int CONTEXT_GEDIT_CONFIRM_SAVESTRING    = 126;
constexpr int CONTEXT_GEDIT_NO_CASH               = 127;
constexpr int CONTEXT_GEDIT_NO_SKILL              = 128;
constexpr int CONTEXT_GEDIT_NUMERICAL_RESPONSE    = 129;
constexpr int CONTEXT_GEDIT_CHARGE                = 130;
constexpr int CONTEXT_GEDIT_OPEN                  = 131;
constexpr int CONTEXT_GEDIT_CLOSE                 = 132;
constexpr int CONTEXT_GEDIT_TRAINER               = 133;
constexpr int CONTEXT_GEDIT_NO_TRAIN              = 134;
constexpr int CONTEXT_GEDIT_MINLVL                = 135;
constexpr int CONTEXT_GEDIT_SELECT_SPELLS         = 136;
constexpr int CONTEXT_GEDIT_SELECT_SKILLS         = 137;
constexpr int CONTEXT_GEDIT_SELECT_WPS            = 138;
constexpr int CONTEXT_GEDIT_SELECT_LANGS          = 139;

/* includes number 0 */
constexpr int NUM_CONTEXTS = 140;

/* Prototypes for the context sensitive help system */
bool context_help(GameWorld *g, struct descriptor_data *d, char *arg);
void boot_context_help(GameWorld *g);
#endif /* ifndef __GENOLC_C__ */

/*. Submodes of GEDIT connectedness     . */
constexpr int GEDIT_MAIN_MENU          = 0;
constexpr int GEDIT_CONFIRM_SAVESTRING = 1;
constexpr int GEDIT_NO_CASH            = 2;
constexpr int GEDIT_NO_SKILL           = 3;

/*. Numerical responses . */
constexpr int GEDIT_NUMERICAL_RESPONSE = 5;
constexpr int GEDIT_CHARGE             = 6;
constexpr int GEDIT_OPEN               = 7;
constexpr int GEDIT_CLOSE              = 8;
constexpr int GEDIT_TRAINER            = 9;
constexpr int GEDIT_NO_TRAIN           = 10;
constexpr int GEDIT_MINLVL             = 11;
constexpr int GEDIT_SELECT_SPELLS      = 12;
constexpr int GEDIT_SELECT_SKILLS      = 13;
constexpr int GEDIT_SELECT_WPS         = 14;
constexpr int GEDIT_SELECT_LANGS       = 15;
constexpr int GEDIT_SELECT_FEATS       = 16;

/*
 * Statedit Connectedness
 * --relistan 2/23/99
 */

constexpr int STAT_GET_STR    = 0;
constexpr int STAT_GET_INT    = 1;
constexpr int STAT_GET_WIS    = 2;
constexpr int STAT_GET_DEX    = 3;
constexpr int STAT_GET_CON    = 4;
constexpr int STAT_GET_CHA    = 5;
constexpr int STAT_QUIT       = 6;
constexpr int STAT_PARSE_MENU = 7;

