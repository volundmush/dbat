#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "dbat/consts/tradeflags.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"
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
extern const std::string_view shop_default_msgs[NUM_SHOP_MSGS];

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
  flag_set<shop_flag> bitvector;        /* Can attack? Use bank? Cast here?	*/
  mob_vnum keeper{};                    /* The mobile who owns the shop (vnum)	*/
  flag_set<trade_flag> with_who;        /* whom the shop won't trade with */
  std::vector<room_vnum> in_room{};     /* Where is the shop?			*/
  int open1{}, open2{};                   /* When does the shop open?		*/
  int close1{}, close2{};                 /* When does the shop close?		*/
  int bankAccount{};                    /* Store all gold over 15000 (disabled)	*/
  int lastsort{};                       /* How many items are sorted in inven?	*/
  SPECIAL(*func){};                     /* Secondary spec_proc for shopkeeper	*/
};


#define VERSION3_TAG "v3.0"       /* The file has v3.0 shops in it!	*/
constexpr int MAX_SHOP_OBJ = 100; /* "Soft" maximum for list maximums	*/

/* Possible states for objects trying to be sold */
constexpr int OBJECT_DEAD  = 0;
constexpr int OBJECT_NOTOK = 1;
constexpr int OBJECT_OK    = 2;
constexpr int OBJECT_NOVAL = 3;

/* Types of lists to read */
constexpr int LIST_PRODUCE = 0;
constexpr int LIST_TRADE   = 1;
constexpr int LIST_ROOM    = 2;


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
/* The traders' names: quirk, one fewer than NUM_TRADERS, so the last trader, as the editors' menus list it, is
   the "\n" that ended them */
constexpr int NUM_TRADER_NAMES = NUM_TRADERS - 1;

// functions
bool shop_producing(GameWorld *g, struct obj_data *item, struct shop_data *shop);
void assign_the_shopkeepers(GameWorld *g);
bool ok_shop_room(GameWorld *g, struct shop_data *shop, room_vnum room);
void destroy_shops(GameWorld *g);
void show_shops(GameWorld *g, struct char_data *ch, std::string_view arg);
int count_shops(GameWorld *g, shop_vnum low, shop_vnum high);

// special
SPECIAL(shop_keeper);

// commands

/* Trade restriction flags (TRADE_*) on a shop_data. */
void shop_trade_set(struct shop_data *rec, trade_flag flag, bool on);
bool shop_trade_toggle(struct shop_data *rec, trade_flag flag);

