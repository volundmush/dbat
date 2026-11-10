#pragma once
#include <cstdio>
#include <string>
#include <vector>

#include "dbat/shop_utils.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/maximums.hpp"

struct shop_buy_data {
  int type{};
  std::string keywords{}; /* Which of that type it buys: empty for all */
};

constexpr int SW_ARRAY_MAX = 4;

/* The shop messages, in shop file order. */
enum shop_message {
  SHOP_MSG_NOITEM1,
  SHOP_MSG_NOITEM2,
  SHOP_MSG_NOBUY,
  SHOP_MSG_NOCASH1,
  SHOP_MSG_NOCASH2,
  SHOP_MSG_BUY,
  SHOP_MSG_SELL,
  NUM_SHOP_MSGS
};

/* Default text for each shop message: used for new shops and in place of a missing or malformed message. */
extern const char *const shop_default_msgs[NUM_SHOP_MSGS];

struct shop_data {
  room_vnum vnum{};                     /* Virtual number of this shop		*/
  std::vector<obj_vnum> producing{};    /* Which item to produce (virtual)	*/
  float profit_buy{};                   /* Factor to multiply cost with		*/
  float profit_sell{};                  /* Factor to multiply cost with		*/
  std::vector<shop_buy_data> type{};    /* Which items to trade			*/
  std::string no_such_item1{};          /* Message if keeper hasn't got an item	*/
  std::string no_such_item2{};          /* Message if player hasn't got an item	*/
  std::string missing_cash1{};          /* Message if keeper hasn't got cash	*/
  std::string missing_cash2{};          /* Message if player hasn't got cash	*/
  std::string do_not_buy{};             /* If keeper dosn't buy such things	*/
  std::string message_buy{};            /* Message when player buys item	*/
  std::string message_sell{};           /* Message when player sells item	*/
  int temper1{};                        /* How does keeper react if no money	*/
  bitvector_t bitvector{};              /* Can attack? Use bank? Cast here?	*/
  mob_vnum keeper{};                    /* The mobile who owns the shop (vnum)	*/
  bitvector_t with_who[SW_ARRAY_MAX]{}; /* Who does the shop trade with?	*/
  std::vector<room_vnum> in_room{};     /* Where is the shop?			*/
  int open1{}, open2{};                   /* When does the shop open?		*/
  int close1{}, close2{};                 /* When does the shop close?		*/
  int bankAccount{};                    /* Store all gold over 15000 (disabled)	*/
  int lastsort{};                       /* How many items are sorted in inven?	*/
  SPECIAL(*func){};                     /* Secondary spec_proc for shopkeeper	*/
};


constexpr int MAX_TRADE = 5;      /* List maximums for compatibility	*/
constexpr int MAX_PROD  = 5;      /*	with shops before v3.0		*/
#define VERSION3_TAG "v3.0"       /* The file has v3.0 shops in it!	*/
constexpr int MAX_SHOP_OBJ = 100; /* "Soft" maximum for list maximums	*/

/* Pretty general macros that could be used elsewhere */
#define END_OF(buffer) ((buffer) + strlen((buffer)))

/* Possible states for objects trying to be sold */
constexpr int OBJECT_DEAD  = 0;
constexpr int OBJECT_NOTOK = 1;
constexpr int OBJECT_OK    = 2;
constexpr int OBJECT_NOVAL = 3;

/* Types of lists to read */
constexpr int LIST_PRODUCE = 0;
constexpr int LIST_TRADE   = 1;
constexpr int LIST_ROOM    = 2;

/* Whom will we not trade with (bitvector for rshop_get_with_who()) */
constexpr int TRADE_NOGOOD               = 0;
constexpr int TRADE_NOEVIL               = 1;
constexpr int TRADE_NONEUTRAL            = 2;
constexpr int TRADE_NOWIZARD             = 3;
constexpr int TRADE_NOCLERIC             = 4;
constexpr int TRADE_NOROGUE              = 5;
constexpr int TRADE_NOFIGHTER            = 6;
constexpr int TRADE_NOHUMAN              = 7;
constexpr int TRADE_NOICER               = 8;
constexpr int TRADE_NOSAIYAN             = 9;
constexpr int TRADE_NOKONATSU            = 10;
constexpr int TRADE_NONAMEK              = 11;
constexpr int TRADE_NOMUTANT             = 12;
constexpr int TRADE_NOKANASSAN           = 13;
constexpr int TRADE_NOBIO                = 14;
constexpr int TRADE_NOANDROID            = 15;
constexpr int TRADE_NODEMON              = 16;
constexpr int TRADE_NOMAJIN              = 17;
constexpr int TRADE_NOKAI                = 18;
constexpr int TRADE_NOTRUFFLE            = 19;
constexpr int TRADE_NOGOBLIN             = 20;
constexpr int TRADE_NOANIMAL             = 21;
constexpr int TRADE_NOORC                = 22;
constexpr int TRADE_NOSNAKE              = 23;
constexpr int TRADE_NOTROLL              = 24;
constexpr int TRADE_NOHALFBREED          = 25;
constexpr int TRADE_NOMINOTAUR           = 26;
constexpr int TRADE_NOKOBOLD             = 27;
constexpr int TRADE_NOLIZARDFOLK         = 28;
constexpr int TRADE_NOMONK               = 29;
constexpr int TRADE_NOPALADIN            = 30;
constexpr int TRADE_UNUSED               = 31;
constexpr int TRADE_ONLYWIZARD           = 32;
constexpr int TRADE_ONLYCLERIC           = 33;
constexpr int TRADE_ONLYROGUE            = 34;
constexpr int TRADE_ONLYFIGHTER          = 35;
constexpr int TRADE_ONLYMONK             = 36;
constexpr int TRADE_ONLYPALADIN          = 37;
constexpr int TRADE_NOSORCERER           = 38;
constexpr int TRADE_NODRUID              = 39;
constexpr int TRADE_NOBARD               = 40;
constexpr int TRADE_NORANGER             = 41;
constexpr int TRADE_NOBARBARIAN          = 42;
constexpr int TRADE_ONLYSORCERER         = 43;
constexpr int TRADE_ONLYDRUID            = 44;
constexpr int TRADE_ONLYBARD             = 45;
constexpr int TRADE_ONLYRANGER           = 46;
constexpr int TRADE_ONLYBARBARIAN        = 47;
constexpr int TRADE_ONLYARCANE_ARCHER    = 48;
constexpr int TRADE_ONLYARCANE_TRICKSTER = 49;
constexpr int TRADE_ONLYARCHMAGE         = 50;
constexpr int TRADE_ONLYASSASSIN         = 51;
constexpr int TRADE_ONLYBLACKGUARD       = 52;
constexpr int TRADE_ONLYDRAGON_DISCIPLE  = 53;
constexpr int TRADE_ONLYDUELIST          = 54;
constexpr int TRADE_ONLYDWARVEN_DEFENDER = 55;
constexpr int TRADE_ONLYELDRITCH_KNIGHT  = 56;
constexpr int TRADE_ONLYHIEROPHANT       = 57;
constexpr int TRADE_ONLYHORIZON_WALKER   = 58;
constexpr int TRADE_ONLYLOREMASTER       = 59;
constexpr int TRADE_ONLYMYSTIC_THEURGE   = 60;
constexpr int TRADE_ONLYSHADOWDANCER     = 61;
constexpr int TRADE_ONLYTHAUMATURGIST    = 62;
constexpr int TRADE_NOARCANE_ARCHER      = 63;
constexpr int TRADE_NOARCANE_TRICKSTER   = 64;
constexpr int TRADE_NOARCHMAGE           = 65;
constexpr int TRADE_NOASSASSIN           = 66;
constexpr int TRADE_NOBLACKGUARD         = 67;
constexpr int TRADE_NODRAGON_DISCIPLE    = 68;
constexpr int TRADE_NODUELIST            = 69;
constexpr int TRADE_NODWARVEN_DEFENDER   = 70;
constexpr int TRADE_NOELDRITCH_KNIGHT    = 71;
constexpr int TRADE_NOHIEROPHANT         = 72;
constexpr int TRADE_NOHORIZON_WALKER     = 73;
constexpr int TRADE_NOLOREMASTER         = 74;
constexpr int TRADE_NOMYSTIC_THEURGE     = 75;
constexpr int TRADE_NOSHADOWDANCER       = 76;
constexpr int TRADE_NOTHAUMATURGIST      = 77;
constexpr int TRADE_NOBROKEN             = 78;

struct stack_data {
  int data[100]{};
  int len{};
};

/* Which expression type we are now parsing */
constexpr int OPER_OPEN_PAREN  = 0;
constexpr int OPER_CLOSE_PAREN = 1;
constexpr int OPER_OR          = 2;
constexpr int OPER_AND         = 3;
constexpr int OPER_NOT         = 4;
constexpr int MAX_OPER         = 4;

constexpr int WILL_START_FIGHT = (1 << 0);
constexpr int WILL_BANK_MONEY  = (1 << 1);
constexpr int WILL_ALLOW_STEAL = (1 << 2);

constexpr int MIN_OUTSIDE_BANK = 5000;
constexpr int MAX_OUTSIDE_BANK = 15000;

#define MSG_NOT_OPEN_YET      "Come back later!"
#define MSG_NOT_REOPEN_YET    "Sorry, we have closed, but come back later."
#define MSG_CLOSED_FOR_DAY    "Sorry, come back tomorrow."
#define MSG_NO_STEAL_HERE     "$n is a bloody thief!"
#define MSG_NO_SEE_CHAR       "I don't trade with someone I can't see!"
#define MSG_NO_SELL_ALIGN     "Get out of here before I call the guards!"
#define MSG_NO_SELL_CLASS     "We don't serve your kind here!"
#define MSG_NO_SELL_RACE      "Get lost! We don't serve you kind here!"
#define MSG_NO_USED_WANDSTAFF "I don't buy used up wands or staves!"
#define MSG_CANT_KILL_KEEPER  "Get out of here before I call the guards!"
#define MSG_NO_BUY_BROKEN     "Sorry, but I don't deal in broken items."

// global variables
extern const char *const trade_letters[NUM_TRADERS + 1];
extern const char *const shop_bits[];

// functions
bool shop_producing(GameWorld *g, struct obj_data *item, struct shop_data *shop);
void boot_the_shops(GameWorld *g, FILE *shop_f, const char *filename, int rec_count);
void assign_the_shopkeepers(GameWorld *g);
bool ok_shop_room(GameWorld *g, struct shop_data *shop, room_vnum room);
void destroy_shops(GameWorld *g);
void show_shops(GameWorld *g, struct char_data *ch, char *arg);
int count_shops(GameWorld *g, shop_vnum low, shop_vnum high);

// special
SPECIAL(shop_keeper);

// commands

/* Trade restriction flags (TRADE_*) on a shop_data. */
void shop_trade_set(struct shop_data *rec, int flag, bool on);
bool shop_trade_toggle(struct shop_data *rec, int flag);

