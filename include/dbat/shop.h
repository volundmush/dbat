/* ************************************************************************
*   File: shop.h                                        Part of CircleMUD *
*  Usage: shop file definitions, structures, constants                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once

#include "structs.h"

extern void shop_purge(uint64_t heartPulse, double deltaTime);

#define BUY_TYPE(i)        ((i).type)
#define BUY_WORD(i)        ((i).keywords.c_str())




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


#define SHOP_NUM(i)        (shop_index[(i)].vnum)
#define SHOP_KEEPER(i)        (shop_index[(i)].keeper)
#define SHOP_OPEN1(i)        (shop_index[(i)].open1)
#define SHOP_CLOSE1(i)        (shop_index[(i)].close1)
#define SHOP_OPEN2(i)        (shop_index[(i)].open2)
#define SHOP_CLOSE2(i)        (shop_index[(i)].close2)
#define SHOP_ROOM(i, num)    (shop_index[(i)].in_room[(num)])
#define SHOP_BUYTYPE(i, num)    (BUY_TYPE(shop_index[(i)].type[(num)]))
#define SHOP_BUYWORD(i, num)    (BUY_WORD(shop_index[(i)].type[(num)]))
#define SHOP_PRODUCT(i, num)    (shop_index[(i)].producing[(num)])
#define SHOP_BANK(i)        (shop_index[(i)].bankAccount)
#define SHOP_BROKE_TEMPER(i)    (shop_index[(i)].temper1)
#define SHOP_BITVECTOR(i)    (shop_index[(i)].bitvector)
#define SHOP_TRADE_WITH(i)    (shop_index[(i)].with_who)
#define SHOP_SORT(i)        (shop_index[(i)].lastsort)
#define SHOP_BUYPROFIT(i)    (shop_index[(i)].profit_buy)
#define SHOP_SELLPROFIT(i)    (shop_index[(i)].profit_sell)
#define SHOP_FUNC(i)        (shop_index[(i)].func)

constexpr int MIN_OUTSIDE_BANK = 5000;
constexpr int MAX_OUTSIDE_BANK = 15000;

#define MSG_NOT_OPEN_YET    "Come back later!"
#define MSG_NOT_REOPEN_YET    "Sorry, we have closed, but come back later."
#define MSG_CLOSED_FOR_DAY    "Sorry, come back tomorrow."
#define MSG_NO_STEAL_HERE    "$n is a bloody thief!"
#define MSG_NO_SEE_CHAR        "I don't trade with someone I can't see!"
#define MSG_NO_SELL_ALIGN    "Get out of here before I call the guards!"
#define MSG_NO_SELL_CLASS    "We don't serve your kind here!"
#define MSG_NO_SELL_RACE        "Get lost! We don't serve you kind here!"
#define MSG_NO_USED_WANDSTAFF    "I don't buy used up wands or staves!"
#define MSG_CANT_KILL_KEEPER    "Get out of here before I call the guards!"
#define MSG_NO_BUY_BROKEN    "Sorry, but I don't deal in broken items."

// global variables
extern const char *trade_letters[NUM_TRADERS + 1];
extern const char *shop_bits[];
extern int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;
extern shop_vnum top_shop;

// functions
extern int shop_producing(struct obj_data *item, vnum shop_nr);

extern int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);

extern void assign_the_shopkeepers();

extern int ok_shop_room(vnum shop_nr, room_vnum room);

extern void show_shops(struct char_data *ch, char *arg);

// special
extern SPECIAL(shop_keeper);
