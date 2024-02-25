/*************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/
#include "dbat/shop.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/spells.h"
#include "dbat/feats.h"
#include "dbat/act.comm.h"
#include "dbat/act.wizard.h"
#include "dbat/act.social.h"
#include "dbat/dg_comm.h"
#include "dbat/act.other.h"
#include "dbat/class.h"
#include "dbat/genzon.h"
#include "dbat/genshp.h"

/* Local variables */
std::unordered_map<shop_vnum, std::shared_ptr<Shop>> shop_index;
shop_vnum top_shop = NOTHING;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;

/* local functions */
static char *read_shop_message(int mnum, room_vnum shr, FILE *shop_f, const char *why);

static void shopping_buy(char *arg, BaseCharacter *ch, BaseCharacter *keeper, vnum shop_nr);

static void shopping_app(char *arg, BaseCharacter *ch, BaseCharacter *keeper, vnum shop_nr);

static Object *get_purchase_obj(BaseCharacter *ch, char *arg, BaseCharacter *keeper, vnum shop_nr, int msg);

static char *customer_string(vnum shop_nr, int detailed);

static void list_all_shops(BaseCharacter *ch);

static void list_detailed_shop(BaseCharacter *ch, vnum shop_nr);

static int transaction_amt(char *arg);

static char *times_message(Object *obj, char *name, int num);


/* Constant list for printing out who we sell to */
const char *trade_letters[NUM_TRADERS + 1] = {
        "No Good",                 /* First, the alignment based ones */
        "No Evil",
        "No Neutral",
        "No Roshi",           /* Then the class based ones */
        "No Piccolo",
        "No Krane",
        "No Nail",
        "No Human",                /* And finally the race based ones */
        "No Icer",
        "No Saiyan",
        "No Konatsu",
        "No Namek",
        "No Mutant",
        "No Kanassan",
        "No Bio",
        "No Android",
        "No Demon",
        "No Majin",
        "No Kai",
        "No Truffle",
        "No Goblin",
        "No Animal",
        "No Orc",
        "No Snake",
        "No Halfbreed",
        "No Minotaur",
        "No Kobold",
        "No Lizardfolk",
        "No Bardock",
        "No Ginyu",
        "UNUSED",
        "Must be Roshi",
        "Must be Piccolo",
        "Must be Krane",
        "Must be Nail",
        "Must be Bardock",
        "Must be Ginyu",
        "No Frieza",
        "No Tapion",
        "No Android 16",
        "No Dabura",
        "No Kabito",
        "Must be Frieza",
        "Must be Tapion",
        "Must be Android 16",
        "Must be Dabura",
        "Must be Kabito",
        "Must be Jinto",
        "Must be Tsuna",
        "Must be Kurzak",
        "Must be Assassin",
        "Must be Blackguard",
        "Must be Dragon Disciple",
        "Must be Duelist",
        "Must be Dwarven Defender",
        "Must be Eldritch Knight",
        "Must be Hierophant",
        "Must be Horizon Walker",
        "Must be Loremaster",
        "Must be Mystic Theurge",
        "Must be Shadowdancer",
        "Must be Thaumaturgist",
        "No Jinto",
        "No Tsuna",
        "No Kurzak",
        "No Assassin",
        "No Blackguard",
        "No Dragon Disciple",
        "No Duelist",
        "No Dwarven Defender",
        "No Eldritch Knight",
        "No Hierophant",
        "No Horizon Walker",
        "No Loremaster",
        "No Mystic Theurge",
        "No Shadowdancer",
        "No Thaumaturgist",
        "\n"
};


const char *shop_bits[] = {
        "WILL_FIGHT",
        "USES_BANK",
        "ALLOW_STEAL",
        "\n"
};

static const std::unordered_map<SenseiID, int> senseiCheck = {
    {SenseiID::Roshi, TRADE_NOWIZARD},
    {SenseiID::Piccolo, TRADE_NOCLERIC},
    {SenseiID::Krane, TRADE_NOROGUE},
    {SenseiID::Bardock, TRADE_NOMONK},
    {SenseiID::Ginyu, TRADE_NOPALADIN},
    {SenseiID::Nail, TRADE_NOFIGHTER},
    {SenseiID::Kibito, TRADE_NOBARBARIAN},
    {SenseiID::Frieza, TRADE_NOSORCERER},
    {SenseiID::Sixteen, TRADE_NOBARD},
    {SenseiID::Dabura, TRADE_NORANGER},
    {SenseiID::Tapion, TRADE_NODRUID},
    {SenseiID::Jinto, TRADE_NOARCANE_ARCHER},
    {SenseiID::Tsuna, TRADE_NOARCANE_TRICKSTER},
    {SenseiID::Kurzak, TRADE_NOARCHMAGE}
};

static const std::unordered_map<RaceID, int> raceCheck = {
    {RaceID::Human, TRADE_NOHUMAN},
    {RaceID::Icer, TRADE_NOICER},
    {RaceID::Saiyan, TRADE_NOSAIYAN},
    {RaceID::Konatsu, TRADE_NOKONATSU}
};

bool Shop::isOkChar(BaseCharacter* keeper, BaseCharacter *ch) {
    char buf[MAX_INPUT_LENGTH];

    if (!CAN_SEE(keeper, ch)) {
        char actbuf[MAX_INPUT_LENGTH] = MSG_NO_SEE_CHAR;
        do_say(keeper, actbuf, cmd_say, 0);
        return false;
    }

    if (ADM_FLAGGED(ch, ADM_ALLSHOPS))
        return true;

    auto align = GET_ALIGNMENT(ch);

    if ((align > 0 && with_who.contains(TRADE_NOGOOD)) ||
        (align < 0 && with_who.contains(TRADE_NOEVIL)) ||
        (align == 0 && with_who.contains(TRADE_NONEUTRAL))) {
        snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
        do_tell(keeper, buf, cmd_tell, 0);
        return false;
    }
    if (IS_NPC(ch))
        return true;

    if (auto notrade = senseiCheck.find(ch->chclass); notrade != senseiCheck.end() && with_who.contains(notrade->second)) {
        snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
        do_tell(keeper, buf, cmd_tell, 0);
        return false;
    }

    if (auto notrade = raceCheck.find(ch->race); notrade != raceCheck.end() && with_who.contains(notrade->second)) {
        snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_RACE);
        do_tell(keeper, buf, cmd_tell, 0);
        return false;
    }

    return true;
}

bool Shop::isOkObj(BaseCharacter *keeper, BaseCharacter *ch, Object *obj) {
    char buf[MAX_INPUT_LENGTH];

    if (OBJ_FLAGGED(obj, ITEM_BROKEN) && with_who.contains(TRADE_NOBROKEN)) {
        snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_BUY_BROKEN);
        do_tell(keeper, buf, cmd_tell, 0);
        return false;
    }
    if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
        snprintf(buf, sizeof(buf), "%s that piece of junk is an obvious forgery!", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return false;
    }

    return true;
}

bool Shop::isOpen(BaseCharacter *keeper, bool msg) {
    char buf[MAX_INPUT_LENGTH];

    *buf = '\0';
    if (open1 > time_info.hours)
        strlcpy(buf, MSG_NOT_OPEN_YET, sizeof(buf));
    else if (close1 < time_info.hours) {
        if (open2 > time_info.hours)
            strlcpy(buf, MSG_NOT_REOPEN_YET, sizeof(buf));
        else if (close2 < time_info.hours)
            strlcpy(buf, MSG_CLOSED_FOR_DAY, sizeof(buf));
    }
    if (!*buf)
        return true;
    if (msg)
        do_say(keeper, buf, cmd_tell, 0);
    return false;
}

bool Shop::isOk(BaseCharacter* keeper, BaseCharacter *ch) {
    if (isOpen(keeper, true))
        return isOkChar(keeper, ch);
    else
        return false;
}


int Shop::tradeWith(Object *item) {
    int counter;


    if (GET_OBJ_COST(item) < 1)
        return (OBJECT_NOVAL);

    if (OBJ_FLAGGED(item, ITEM_NOSELL))
        return (OBJECT_NOTOK);

    for (const auto &product : type)
        if (product.type == GET_OBJ_TYPE(item)) {
            if (GET_OBJ_VAL(item, VAL_WAND_CHARGES) == 0 &&
                (GET_OBJ_TYPE(item) == ITEM_WAND ||
                 GET_OBJ_TYPE(item) == ITEM_STAFF))
                return (OBJECT_DEAD);
            return (OBJECT_OK);
        }
    return (OBJECT_NOTOK);
}


bool Shop::isProducing(obj_vnum vn) {
    auto find = std::ranges::find(producing, vn);
    return (find != producing.end());
}

bool Shop::isProducing(Object* item) {
    return isProducing(item->getVN());
}


static int transaction_amt(char *arg) {
    char buf[MAX_INPUT_LENGTH];

    char *buywhat;

    /*
     * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
     * We don't do that if we only have one argument, like 'buy 5', buy #5.
     * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
     */
    buywhat = one_argument(arg, buf);
    if (*buywhat && *buf && is_number(buf)) {
        strcpy(arg, arg + strlen(buf) + 1);    /* strcpy: OK (always smaller) */
        return (atoi(buf));
    }
    return (1);
}

static char *times_message(Object *obj, char *name, int num) {
    static char buf[256];
    size_t len;
    char *ptr;

    if (obj)
        len = strlcpy(buf, obj->getShortDesc().c_str(), sizeof(buf));
    else {
        if ((ptr = strchr(name, '.')) == nullptr)
            ptr = name;
        else
            ptr++;
        len = snprintf(buf, sizeof(buf), "%s %s", AN(ptr), ptr);
    }

    if (num > 1 && len < sizeof(buf))
        snprintf(buf + len, sizeof(buf) - len, " (x %d)", num);

    return (buf);
}


static Object *get_purchase_obj(BaseCharacter *ch, char *arg, BaseCharacter *keeper, vnum shop_nr, int msg) {
    return nullptr;
}

/*
 * Shop purchase adjustment, based on charisma-difference from buyer to keeper.
 *
 * for i in `seq 15 -15`; do printf " * %3d: %6.4f\n" $i \
 * `echo "scale=4; 1+$i/70" | bc`; done
 *
 *  Shopkeeper higher charisma (markup)
 *  ^  15: 1.2142  14: 1.2000  13: 1.1857  12: 1.1714  11: 1.1571
 *  |  10: 1.1428   9: 1.1285   8: 1.1142   7: 1.1000   6: 1.0857
 *  |   5: 1.0714   4: 1.0571   3: 1.0428   2: 1.0285   1: 1.0142
 *  +   0: 1.0000
 *  |  -1: 0.9858  -2: 0.9715  -3: 0.9572  -4: 0.9429  -5: 0.9286
 *  |  -6: 0.9143  -7: 0.9000  -8: 0.8858  -9: 0.8715 -10: 0.8572
 *  v -11: 0.8429 -12: 0.8286 -13: 0.8143 -14: 0.8000 -15: 0.7858
 *  Player higher charisma (discount)
 *
 * Most mobiles have 11 charisma so an 18 charisma player would get a 10%
 * discount beyond the basic price.  That assumes they put a lot of points
 * into charisma, because on the flip side they'd get 11% inflation by
 * having a 3.
 */
int64_t Shop::buyPrice(Object *obj, BaseCharacter *keeper, BaseCharacter *buyer) {
    int cost = (GET_OBJ_COST(obj) * profit_buy);

    double adjust = 1.0;
    Object *k;

    for (k = object_list; k; k = k->next) {
        if (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(obj)) {
            adjust -= 0.00025;
        }
    }
    if (adjust < 0.015) {
        adjust = 0.5;
    }

    cost = cost * adjust;

    if (!IS_NPC(buyer) && GET_BONUS(buyer, BONUS_THRIFTY) > 0) {
        if (IS_ARLIAN(buyer)) {
            cost += cost * 0.2;
        }
        cost -= cost * 0.1;
        return (cost);
    } else if (!IS_NPC(buyer) && GET_BONUS(buyer, BONUS_IMPULSE)) {
        cost += cost * 0.25;
        return (cost);
    } else if (!IS_NPC(buyer) && IS_ARLIAN(buyer)) {
        cost += cost * 0.20;
        return (cost);
    } else {
        return (int) (GET_OBJ_COST(obj) * profit_buy);
    }
}

/*
 * When the shopkeeper is buying, we reverse the discount. Also make sure
 * we don't buy for more than we sell for, to prevent infinite money-making.
 */
int64_t Shop::sellPrice(Object *obj, BaseCharacter *keeper, BaseCharacter *seller) {
    float sell_cost_modifier = profit_sell;
    float buy_cost_modifier = profit_buy;

    if (sell_cost_modifier > buy_cost_modifier)
        sell_cost_modifier = buy_cost_modifier;

    double adjust = 1.0;
    Object *k;

    for (k = object_list; k; k = k->next) {
        if (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(obj)) {
            adjust -= 0.00025;
        }
    }
    if (adjust < 0.15) {
        adjust = 0.15;
    }

    if (!IS_NPC(seller) && GET_BONUS(seller, BONUS_THRIFTY) > 0) {
        int haggle = (GET_OBJ_COST(obj) * (sell_cost_modifier / 2));
        if (IS_ARLIAN(seller)) {
            haggle -= haggle * 0.2;
        }
        haggle += haggle * 0.1;
        haggle = haggle * adjust;
        return (haggle);
    } else if (!IS_NPC(seller) && IS_ARLIAN(seller)) {
        int haggle = (GET_OBJ_COST(obj) * (sell_cost_modifier / 2));
        haggle -= haggle * 0.2;
        haggle = haggle * adjust;
        return (haggle);
    } else {
        return (int) ((GET_OBJ_COST(obj) * (sell_cost_modifier / 2)) * adjust);
    }
}

Object* Shop::getPurchaseObject(BaseCharacter *ch, const std::string& arguments, BaseCharacter *keeper, bool msg) {
    return nullptr;
}

void Shop::executeAppraise(BaseCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments) {
    Object *obj;
    int i, found = false;
    char buf[MAX_STRING_LENGTH];

    if (!isOk(keeper, ch))
        return;

    if (arguments.empty()) {
        char buf[MAX_INPUT_LENGTH];

        snprintf(buf, sizeof(buf), "%s What do you want to appraise?", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    if (!(obj = getPurchaseObject(ch, arguments, keeper, true))) {
        do_appraise(ch, (char*)arguments.c_str(), 0, 0);
        return;
    }

    act("@C$N@W hands you @G$p@W for a moment and let's you examine it before taking it back.@n", true, ch, obj, keeper,
        TO_CHAR);
    act("@c$N@W hands @C$n@W @G$p@W for a moment and let's $m examine it before taking it back.@n", true, ch, obj,
        keeper, TO_ROOM);

    if (!GET_SKILL(ch, SKILL_APPRAISE)) {
        ch->sendf("You are unskilled at appraising.\r\n");
        return;
    }
    improve_skill(ch, SKILL_APPRAISE, 1);
    if (GET_SKILL(ch, SKILL_APPRAISE) < rand_number(1, 101)) {
        ch->sendf("@wYou were completely stumped about the worth of %s@n\r\n", obj->getShortDesc());
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    } else {
        int displevel = GET_OBJ_LEVEL(obj);

        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && OBJ_FLAGGED(obj, ITEM_CUSTOM))
            displevel = 20;

        ch->sendf("@c---------------------------------------------------------------@n\n");
        ch->sendf("@GItem Name   @W: @w%s@n\n", obj->getShortDesc());
        ch->sendf("@GTrue Value  @W: @Y%s@n\n", add_commas(GET_OBJ_COST(obj)).c_str());
        ch->sendf("@GItem Min LVL@W: @w%d@n\n", displevel);
        if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) >= 100) {
            ch->sendf("@GCondition   @W: @C%d%s@n\n", GET_OBJ_VAL(obj, VAL_ALL_HEALTH), "%");
        } else if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) >= 50) {
            ch->sendf("@GCondition   @W: @y%d%s@n\n", GET_OBJ_VAL(obj, VAL_ALL_HEALTH), "%");
        } else if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) >= 1) {
            ch->sendf("@GCondition   @W: @r%d%s@n\n", GET_OBJ_VAL(obj, VAL_ALL_HEALTH), "%");
        } else {
            ch->sendf("@GCondition   @W: @D%d%s@n\n", GET_OBJ_VAL(obj, VAL_ALL_HEALTH), "%");
        }
        ch->sendf("@GItem Size   @W:@w %s@n\n", size_names[GET_OBJ_SIZE(obj)]);
        ch->sendf("@GItem Weight @W: @w%s@n\n", add_commas(GET_OBJ_WEIGHT(obj)).c_str());
        if (OBJ_FLAGGED(obj, ITEM_SLOT1) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
            ch->sendf("GToken Slots  @W: @m0/1@n\n");
        } else if (OBJ_FLAGGED(obj, ITEM_SLOT1) && OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
            ch->sendf("GToken Slots  @W: @m1/1@n\n");
        } else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOT_ONE) &&
                   !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
            ch->sendf("GToken Slots  @W: @m0/2@n\n");
        } else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && OBJ_FLAGGED(obj, ITEM_SLOT_ONE) &&
                   !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
            ch->sendf("GToken Slots  @W: @m1/2@n\n");
        } else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
            ch->sendf("GToken Slots  @W: @m2/2@n\n");
        }
        char bits[MAX_STRING_LENGTH];
        sprintf(bits, "%s", join(obj->getFlagNames(FlagType::Wear), ", ").c_str());
        search_replace(bits, "TAKE", "");
        ch->sendf("@GWear Loc.   @W:@w%s\n", bits);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
            if (OBJ_FLAGGED(obj, ITEM_WEAPLVL1)) {
                ch->sendf("@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w5%s@D]@n\r\n", "%");
            } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL2)) {
                ch->sendf("@GWeapon Level@W: @D[@C2@D]\n@GDamage Bonus@W: @D[@w10%s@D]@n\r\n", "%");
            } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL3)) {
                ch->sendf("@GWeapon Level@W: @D[@C3@D]\n@GDamage Bonus@W: @D[@w20%s@D]@n\r\n", "%");
            } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL4)) {
                ch->sendf("@GWeapon Level@W: @D[@C4@D]\n@GDamage Bonus@W: @D[@w30%s@D]@n\r\n", "%");
            } else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL5)) {
                ch->sendf("@GWeapon Level@W: @D[@C5@D]\n@GDamage Bonus@W: @D[@w50%s@D]@n\r\n", "%");
            }
        }
        ch->sendf("@GItem Bonuses@W:@w");
        for (i = 0; i < MAX_OBJ_AFFECT; i++) {
            if (obj->affected[i].modifier) {
                sprinttype(obj->affected[i].location, apply_types, buf, sizeof(buf));
                ch->sendf("%s %+f to %s", found++ ? "," : "", obj->affected[i].modifier, buf);
                switch (obj->affected[i].location) {
                    case APPLY_FEAT:
                        ch->sendf(" (%s)", feat_list[obj->affected[i].specific].name);
                        break;
                    case APPLY_SKILL:
                        ch->sendf(" (%s)", spell_info[obj->affected[i].specific].name);
                        break;
                }
            }
        }
        if (!found)
            ch->sendf(" None@n");
        else
            ch->sendf("@n");
        char buf2[MAX_STRING_LENGTH];
        sprintf(buf2, "%s", join(obj->getFlagNames(FlagType::Affect), ", ").c_str());
        ch->sendf("\n@GSpecial     @W:@w %s", buf2);
        ch->sendf("\n@c---------------------------------------------------------------@n\n");
    }
}

void Shop::executeBuy(BaseCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments) {
    ch->sendf("Not yet implemented.\r\n");
    return;
}

Object* Shop::getSellingObject(BaseCharacter *ch, const std::string &name, BaseCharacter *keeper, bool msg) {
    char buf[MAX_INPUT_LENGTH];
    Object *obj;
    int result;

    if (!(obj = get_obj_in_list_vis(ch, (char*)name.c_str(), nullptr, ch->getInventory()))) {
        if (msg) {
            char tbuf[MAX_INPUT_LENGTH];

            snprintf(tbuf, sizeof(tbuf), no_such_item2.c_str(), GET_NAME(ch));
            do_tell(keeper, tbuf, cmd_tell, 0);
        }
        return nullptr;
    }
    if ((result = tradeWith(obj)) == OBJECT_OK)
        return obj;

    if (!msg)
        return nullptr;

    switch (result) {
        case OBJECT_NOVAL:
            snprintf(buf, sizeof(buf), "%s You've got to be kidding, that thing is worthless!", GET_NAME(ch));
            break;
        case OBJECT_NOTOK:
            snprintf(buf, sizeof(buf), do_not_buy.c_str(), GET_NAME(ch));
            break;
        case OBJECT_DEAD:
            snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
            break;
        default:
            basic_mud_log("SYSERR: Illegal return value of %d from trade_with() (%s)", result,
                __FILE__);    /* Someone might rename it... */
            snprintf(buf, sizeof(buf), "%s An error has occurred.", GET_NAME(ch));
            break;
    }
    do_tell(keeper, buf, cmd_tell, 0);
    return nullptr;
}


void Shop::executeSell(BaseCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments) {
    char tempstr[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], tempbuf[MAX_INPUT_LENGTH];
    Object *obj;
    int sellnum, sold = 0, goldamt = 0;

    if (!isOk(keeper, ch))
        return;

    if ((sellnum = transaction_amt((char*)arguments.c_str())) < 0) {
        char buf[MAX_INPUT_LENGTH];

        snprintf(buf, sizeof(buf), "%s A negative amount?  Try buying something.", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    if (arguments.empty() || !sellnum) {
        char buf[MAX_INPUT_LENGTH];

        snprintf(buf, sizeof(buf), "%s What do you want to sell??", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    one_argument((char*)arguments.c_str(), name);
    if (!(obj = getSellingObject(ch, name, keeper, true)))
        return;

    if (GET_OBJ_TYPE(obj) == ITEM_PLANT && GET_OBJ_VAL(obj, VAL_WATERLEVEL) <= -10) {
        char buf[MAX_INPUT_LENGTH];
        snprintf(buf, sizeof(buf), "%s That thing is dead!", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    if (!isOkObj(keeper, ch, obj))
        return;

    if (GET_GOLD(keeper) + bankAccount < sellPrice(obj, keeper, ch)) {
        char buf[MAX_INPUT_LENGTH];

        snprintf(buf, sizeof(buf), missing_cash1.c_str(), GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    while (obj && GET_GOLD(keeper) + bankAccount >= sellPrice(obj, keeper, ch) && sold < sellnum) {
        auto charged = sellPrice(obj, keeper, ch);

        goldamt += charged;
        keeper->mod(CharMoney::Carried, -charged);

        sold++;
        obj->removeFromLocation();
        obj = getSellingObject(ch, name, keeper, false);
    }

    if (sold < sellnum) {
        char buf[MAX_INPUT_LENGTH];

        if (!obj)
            snprintf(buf, sizeof(buf), "%s You only have %d of those.", GET_NAME(ch), sold);
        else if (GET_GOLD(keeper) + bankAccount < sellPrice(obj, keeper, ch))
            snprintf(buf, sizeof(buf), "%s I can only afford to buy %d of those.", GET_NAME(ch), sold);
        else
            snprintf(buf, sizeof(buf), "%s Something really screwy made me buy %d.", GET_NAME(ch), sold);

        do_tell(keeper, buf, cmd_tell, 0);
    }
    strlcpy(tempstr, times_message(nullptr, name, sold), sizeof(tempstr));
    snprintf(tempbuf, sizeof(tempbuf), "$n sells something to %s.\r\n", GET_NAME(keeper));
    act(tempbuf, false, ch, obj, nullptr, TO_ROOM);

    snprintf(tempbuf, sizeof(tempbuf), message_sell.c_str(), GET_NAME(ch), goldamt);
    do_tell(keeper, tempbuf, cmd_tell, 0);

    ch->sendf("The shopkeeper gives you %s zenni.\r\n", add_commas(goldamt).c_str());
    if (GET_GOLD(ch) + goldamt > GOLD_CARRY(ch)) {
        goldamt = (GET_GOLD(ch) + goldamt) - GOLD_CARRY(ch);
        ch->set(CharMoney::Carried, GOLD_CARRY(ch));
        ch->mod(CharMoney::Bank, goldamt);
        ch->sendf("You couldn't hold all of the money. The rest was deposited for you.\r\n");
    } else {
        ch->mod(CharMoney::Carried, goldamt);
    }

    if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
        goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), bankAccount);
        bankAccount -= goldamt;
        keeper->mod(CharMoney::Carried, goldamt);
    }
}

void Shop::executeValue(BaseCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments) {
    char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
    Object *obj;

    if (!isOk(keeper, ch))
        return;

    if (arguments.empty()) {
        snprintf(buf, sizeof(buf), "%s What do you want me to evaluate??", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    one_argument((char*)arguments.c_str(), name);
    if (!(obj = getSellingObject(ch, name, keeper, true)))
        return;

    if (!isOkObj(keeper, ch, obj))
        return;

    snprintf(buf, sizeof(buf), "%s I'll give you %d zenni for that!", GET_NAME(ch),
             sellPrice(obj, keeper, ch));
    do_tell(keeper, buf, cmd_tell, 0);
}

std::string Shop::listObject(Object *obj, int cnt, int aindex, BaseCharacter *keeper, BaseCharacter *ch) {
    std::string result;
    char itemname[128], quantity[16];    /* "Unlimited" or "%d" */

    if (isProducing(obj))
        strcpy(quantity, "Unlimited");    /* strcpy: OK (for 'quantity >= 10') */
    else
        sprintf(quantity, "%d", cnt);    /* sprintf: OK (for 'quantity >= 11', 32-bit int) */

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_DRINKCON:
            if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL))
                snprintf(itemname, sizeof(itemname), "%s", obj->getShortDesc().c_str());
            else
                strlcpy(itemname, obj->getShortDesc().c_str(), sizeof(itemname));
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            snprintf(itemname, sizeof(itemname), "%s%s", obj->getShortDesc().c_str(),
                     GET_OBJ_VAL(obj, VAL_WAND_CHARGES) < GET_OBJ_VAL(obj, VAL_WAND_MAXCHARGES) ? " (partially used)"
                                                                                                : "");
            break;

        default:
            strlcpy(itemname, obj->getShortDesc().c_str(), sizeof(itemname));
            break;
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
        char titemname[128];
        strlcpy(titemname, itemname, sizeof(titemname));
        snprintf(itemname, sizeof(itemname), "%s [broken]", titemname);
    }

    CAP(itemname);

    int displevel = GET_OBJ_LEVEL(obj);

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && OBJ_FLAGGED(obj, ITEM_CUSTOM))
        displevel = 20;

    result += fmt::sprintf(" %2d)  %9s %-*s %3d %13s\r\n", aindex, quantity,
             count_color_chars(itemname) + 36, itemname, displevel, add_commas(buyPrice(obj, keeper, ch)));
    return (result);
}

void Shop::executeList(BaseCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments) {
    if (!isOk(keeper, ch))
        return;

    std::vector<Object*> display;
    for(auto o : keeper->getInventory()) {
        if(CAN_SEE_OBJ(ch, o) && GET_OBJ_COST(o) > 0) {
            display.push_back(o);
        }
    }
    if(display.empty()) {
        ch->sendf("Currently, you can see nothing for sale.\r\n");
        return;
    }

    std::string result;

    result += " ##   Available   Item                             Min. Lvl       Cost\r\n"
                       "----------------------------------------------------------------------\r\n";

    std::vector<std::string> lines;
    int cnt = 0;
    int aindex = 1;
    for (auto obj : display) {
        cnt++;
        aindex++;
        lines.push_back(listObject(obj, cnt, aindex, keeper, ch));
    }
    result += join(lines, "@w\r\n");
    ch->sendLine(result);
}


bool Shop::executeCommand(BaseCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments) {

    auto loc = keeper->getLocation();
    if(!loc) {
        return false;
    }
    if(!in_room.contains(loc->getUID())) {
        return false;
    }

    if (!AWAKE(keeper))
        return false;

    if (iequals("steal", cmd)) {
        char argm[MAX_INPUT_LENGTH];

        if (!flags.contains(WILL_ALLOW_STEAL)) {
            snprintf(argm, sizeof(argm), "$N shouts '%s'", MSG_NO_STEAL_HERE);
            act(argm, false, ch, nullptr, keeper, TO_CHAR);
            act(argm, false, ch, nullptr, keeper, TO_ROOM);
            do_action(keeper, (char*)GET_NAME(ch), cmd_slap, 0);

            return true;
        } else {
            return false;
        }
    }

    if (iequals("buy", cmd)) {
        executeBuy(keeper, ch, cmd, arguments);
        return true;
    } else if (iequals("sell", cmd)) {
        executeSell(keeper, ch, cmd, arguments);
        return true;
    } else if (iequals("value", cmd)) {
        executeValue(keeper, ch, cmd, arguments);
        return true;
    } else if (iequals("list", cmd)) {
        executeList(keeper, ch, cmd, arguments);
        return true;
    } else if (iequals("appraise", cmd)) {
        executeAppraise(keeper, ch, cmd, arguments);
        return true;
    }
    return false;
}

void assign_the_shopkeepers() {
    cmd_say = find_command("say");
    cmd_tell = find_command("tell");
    cmd_emote = find_command("emote");
    cmd_slap = find_command("slap");
    cmd_puke = find_command("puke");

    for (auto &[vn, sh] : shop_index) {
        for(auto keeper : sh->getKeepers()) {
            auto &k = reg.get_or_emplace<ShopKeeper>(keeper->ent);
            k.shopKeeperOf = sh;
        }
    }
}

/* END_OF inefficient */
static void list_all_shops(BaseCharacter *ch) {

}

static void list_detailed_shop(BaseCharacter *ch, vnum shop_nr) {

}

void show_shops(BaseCharacter *ch, char *arg) {

}


void Shop::add_product(obj_vnum v) {
    producing.push_back(v);
}

void Shop::remove_product(obj_vnum v) {
    std::remove_if(producing.begin(), producing.end(), [&](obj_vnum &o) {return o == v;});
}

nlohmann::json shop_buy_data::serialize() {
    nlohmann::json j;

    if(type) j["type"] = type;
    if(!keywords.empty()) j["keywords"] = keywords;

    return j;
}


shop_buy_data::shop_buy_data(const nlohmann::json &j) : shop_buy_data() {
    if(j.contains("type")) type = j["type"];
    if(j.contains("keywords")) keywords = j["keywords"];
}

nlohmann::json Shop::serialize() {
    nlohmann::json j;

    j["vnum"] = vnum;
    for(auto i : producing) j["producing"].push_back(i);
    if(profit_buy) j["profit_buy"] = profit_buy;
    if(profit_sell) j["profit_sell"] = profit_sell;
    for(auto &t : type) j["type"].push_back(t.serialize());
    if(!no_such_item1.empty()) j["no_such_item1"] = no_such_item1;
    if(!no_such_item2.empty()) j["no_such_item2"] = no_such_item2;
    if(!missing_cash1.empty()) j["missing_cash1"] = missing_cash1;
    if(!missing_cash2.empty()) j["missing_cash2"] = missing_cash2;
    if(!do_not_buy.empty()) j["do_not_buy"] = do_not_buy;
    if(!message_buy.empty()) j["message_buy"] = message_buy;
    if(!message_sell.empty()) j["message_sell"] = message_sell;
    if(temper1) j["temper1"] = temper1;
    for(auto f : flags) j["flags"].push_back(f);
    if(keeper != NOBODY) j["keeper"] = keeper;
    for(auto f : with_who) j["with_who"].push_back(f);
    for(auto r : in_room) j["in_room"].push_back(r);
    if(open1) j["open1"] = open1;
    if(close1) j["close1"] = close1;
    if(open2) j["open2"] = open2;
    if(close2) j["close2"] = close2;
    if(bankAccount) j["bankAccount"] = bankAccount;
    if(lastsort) j["lastsort"] = lastsort;

    return j;
}


Shop::Shop(const nlohmann::json &j) : Shop() {
    if(j.contains("vnum")) vnum = j["vnum"];
    if(j.contains("producing")) for(const auto& i : j["producing"]) producing.emplace_back(i);
    if(j.contains("profit_buy")) profit_buy = j["profit_buy"];
    if(j.contains("profit_sell")) profit_sell = j["profit_sell"];
    if(j.contains("type")) for(const auto& i : j["type"]) type.emplace_back(i);
    if(j.contains("no_such_item1")) no_such_item1 = strdup(j["no_such_item1"].get<std::string>().c_str());
    if(j.contains("no_such_item2")) no_such_item2 = strdup(j["no_such_item2"].get<std::string>().c_str());
    if(j.contains("missing_cash1")) missing_cash1 = strdup(j["missing_cash1"].get<std::string>().c_str());
    if(j.contains("missing_cash2")) missing_cash2 = strdup(j["missing_cash2"].get<std::string>().c_str());
    if(j.contains("do_not_buy")) do_not_buy = strdup(j["do_not_buy"].get<std::string>().c_str());
    if(j.contains("message_buy")) message_buy = strdup(j["message_buy"].get<std::string>().c_str());
    if(j.contains("message_sell")) message_sell = strdup(j["message_sell"].get<std::string>().c_str());
    if(j.contains("temper1")) temper1 = j["temper1"];
    if(j.contains("flags")) {
        // flags is an array of numbers, insert into flags.
        for(const auto& f : j["flags"]) flags.insert(f.get<int>());
    }
    if(j.contains("keeper")) keeper = j["keeper"];
    if(j.contains("with_who")) for(const auto& i : j["with_who"]) with_who.insert(i.get<int>());
    if(j.contains("in_room")) for(const auto& i : j["in_room"]) in_room.insert(i.get<int>());
    if(j.contains("open1")) open1 = j["open1"];
    if(j.contains("close1")) close1 = j["close1"];
    if(j.contains("open2")) open2 = j["open2"];
    if(j.contains("close2")) close2 = j["close2"];
    if(j.contains("bankAccount")) bankAccount = j["bankAccount"];
    if(j.contains("lastsort")) lastsort = j["lastsort"];
}

std::list<BaseCharacter*> Shop::getKeepers() {
    return get_vnum_list(characterVnumIndex, keeper);
}

void Shop::runPurge() {
    Object *next_obj;
    for(auto keeper : getKeepers()) {
        for (auto sobj : keeper->getInventory()) {
            if(isProducing(sobj->getVN())) {
                keeper->mod(CharMoney::Carried, GET_OBJ_COST(sobj));
                sobj->extractFromWorld();
            }
        }
    }
}


void shop_purge(uint64_t heartPulse, double deltaTime) {
    for(auto &[vn, shop] : shop_index) {
        shop->runPurge();
    }
}