#pragma once
#include <map>

#include "HasOrganizationInfo.h"

#include "const/Max.h"
#include "const/ItemType.h"
#include "const/ShopFlag.h"
#include "Flags.h"

struct Object;

struct shop_buy_data {
    int type{-1};
    std::string keywords{};
};

struct Shop : public org_data {
    void add_product(obj_vnum v);
    void remove_product(obj_vnum v);
    std::vector<obj_vnum> producing{};        /* Which item to produce (virtual)	*/
    float profit_buy{};        /* Factor to multiply cost with		*/
    float profit_sell{};        /* Factor to multiply cost with		*/
    std::vector<shop_buy_data> type{};
    std::string no_such_item1{};        /* Message if keeper hasn't got an item	*/
    std::string no_such_item2{};        /* Message if player hasn't got an item	*/
    std::string missing_cash1{};        /* Message if keeper hasn't got cash	*/
    std::string missing_cash2{};        /* Message if player hasn't got cash	*/
    std::string do_not_buy{};        /* If keeper dosn't buy such things	*/
    std::string message_buy{};        /* Message when player buys item	*/
    std::string message_sell{};        /* Message when player sells item	*/
    int temper1{};        /* How does keeper react if no money	*/
    FlagHandler<ShopFlag> shop_flags{};    /* Can attack? Use bank? Cast here?	*/

    std::unordered_set<room_vnum> in_room;        /* Where is the shop?			*/
    int open1{}, open2{};        /* When does the shop open?		*/
    int close1{}, close2{};    /* When does the shop close?		*/
    int bankAccount{};        /* Store all gold over 15000 (disabled)	*/
    int lastsort{};        /* How many items are sorted in inven?	*/

    bool isProducing(obj_vnum vn);
    void runPurge();
};


extern void shop_purge(uint64_t heartPulse, double deltaTime);


constexpr int MAX_TRADE = 5;    /* List maximums for compatibility	*/
constexpr int MAX_PROD = 5;    /*	with shops before v3.0		*/
#define VERSION3_TAG    "v3.0"    /* The file has v3.0 shops in it!	*/
constexpr int MAX_SHOP_OBJ = 100;    /* "Soft" maximum for list maximums	*/


/* Pretty general macros that could be used elsewhere */
#define END_OF(buffer)        ((buffer) + strlen((buffer)))


/* Possible states for objects trying to be sold */
constexpr int OBJECT_DEAD = 0;
constexpr int OBJECT_NOTOK = 1;
constexpr int OBJECT_OK = 2;
constexpr int OBJECT_NOVAL = 3;


/* Types of lists to read */
constexpr int LIST_PRODUCE = 0;
constexpr int LIST_TRADE = 1;
constexpr int LIST_ROOM = 2;




struct stack_data {
    int data[100];
    int len;
};

#define S_DATA(stack, index)    ((stack)->data[(index)])
#define S_LEN(stack)        ((stack)->len)


/* Which expression type we are now parsing */
constexpr int OPER_OPEN_PAREN = 0;
constexpr int OPER_CLOSE_PAREN = 1;
constexpr int OPER_OR = 2;
constexpr int OPER_AND = 3;
constexpr int OPER_NOT = 4;
constexpr int MAX_OPER = 4;


#define SHOP_NUM(i)        shop_index.at(i)->vnum
#define SHOP_KEEPER(i)        shop_index.at(i)->keeper
#define SHOP_OPEN1(i)        shop_index.at(i)->open1
#define SHOP_CLOSE1(i)        shop_index.at(i)->close1
#define SHOP_OPEN2(i)        shop_index.at(i)->open2
#define SHOP_CLOSE2(i)        shop_index.at(i)->close2
#define SHOP_ROOM(i, num)    shop_index.at(i)->in_room[num]
#define SHOP_PRODUCT(i, num)    shop_index.at(i)->producing(num)
#define SHOP_BANK(i)        shop_index.at(i)->bankAccount
#define SHOP_BROKE_TEMPER(i)    shop_index.at(i)->temper1
#define SHOP_BITVECTOR(i)    shop_index.at(i)->bitvector
#define SHOP_TRADE_WITH(i)    shop_index.at(i)->with_who
#define SHOP_SORT(i)        shop_index.at(i)->lastsort
#define SHOP_BUYPROFIT(i)    shop_index.at(i)->profit_buy
#define SHOP_SELLPROFIT(i)    shop_index.at(i)->profit_sell
#define SHOP_FUNC(i)        shop_index.at(i)->func

constexpr int MIN_OUTSIDE_BANK = 5000;
constexpr int MAX_OUTSIDE_BANK = 15000;

constexpr std::string_view MSG_NOT_OPEN_YET = "Come back later!";
constexpr std::string_view MSG_NOT_REOPEN_YET = "Sorry, we have closed, but come back later.";
constexpr std::string_view MSG_CLOSED_FOR_DAY = "Sorry, come back tomorrow.";
constexpr std::string_view MSG_NO_STEAL_HERE = "$n is a bloody thief!";
constexpr std::string_view MSG_NO_SEE_CHAR = "I don't trade with someone I can't see!";
constexpr std::string_view MSG_NO_SELL_ALIGN = "Get out of here before I call the guards!";
constexpr std::string_view MSG_NO_SELL_CLASS = "We don't serve your kind here!";
constexpr std::string_view MSG_NO_SELL_RACE = "Get lost! We don't serve you kind here!";
constexpr std::string_view MSG_NO_USED_WANDSTAFF = "I don't buy used up wands or staves!";
constexpr std::string_view MSG_CANT_KILL_KEEPER = "Get out of here before I call the guards!";
constexpr std::string_view MSG_NO_BUY_BROKEN = "Sorry, but I don't deal in broken items.";

// global variables
extern const char *trade_letters[NUM_TRADERS + 1];
extern const char *shop_bits[];
extern int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;
extern shop_vnum top_shop;

// functions
extern int shop_producing(Object *item, vnum shop_nr);

extern int ok_damage_shopkeeper(Character *ch, Character *victim);

extern void assign_the_shopkeepers();

extern int ok_shop_room(vnum shop_nr, room_vnum room);

extern void show_shops(Character *ch, char *arg);

// special
extern SPECIAL(shop_keeper);

extern std::map<shop_vnum, std::shared_ptr<Shop>> shop_index;