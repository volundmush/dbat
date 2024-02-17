/************************************************************************
 * OasisOLC - Shops / sedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/sedit.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/db.h"
#include "dbat/shop.h"
#include "dbat/genolc.h"
#include "dbat/genshp.h"
#include "dbat/genzon.h"
#include "dbat/oasis.h"
#include "dbat/constants.h"
#include "dbat/act.informative.h"

/*
 * Should check more things.
 */
void sedit_save_internally(struct descriptor_data *d) {
    OLC_SHOP(d)->vnum = OLC_NUM(d);
    add_shop(OLC_SHOP(d));
}

void sedit_save_to_disk(int num) {
    save_shops(num);
}

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

ACMD(do_oasis_sedit) {
    vnum number = NOWHERE, save = 0;
    shop_rnum real_num;
    struct descriptor_data *d;
    char buf1[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];

    /****************************************************************************/
    /** Parse any arguments.                                                   **/
    /****************************************************************************/
    two_arguments(argument, buf1, buf2);

    if (!*buf1) {
        ch->sendf("Specify a shop VNUM to edit.\r\n");
        return;
    } else if (!isdigit(*buf1)) {
        if (strcasecmp("save", buf1) != 0) {
            ch->sendf("Yikes!  Stop that, someone will get hurt!\r\n");
            return;
        }

        save = true;

        if (is_number(buf2))
            number = atoi(buf2);
        else if (GET_OLC_ZONE(ch) > 0) {
            zone_rnum zlok;

            if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
                number = NOWHERE;
            else
                number = genolc_zone_bottom(zlok);
        }

        if (number == NOWHERE) {
            ch->sendf("Save which zone?\r\n");
            return;
        }
    }

    /****************************************************************************/
    /** If a numeric argument was given, get it.                               **/
    /****************************************************************************/
    if (number == NOWHERE)
        number = atoi(buf1);

    /****************************************************************************/
    /** Check that the shop isn't already being edited.                        **/
    /****************************************************************************/
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) == CON_SEDIT) {
            if (d->olc && OLC_NUM(d) == number) {
                ch->sendf("That shop is currently being edited by %s.\r\n",
                             PERS(d->character, ch));
                return;
            }
        }
    }

    /****************************************************************************/
    /** Point d to the builder's descriptor.                                   **/
    /****************************************************************************/
    d = ch->desc;

    /****************************************************************************/
    /** Give the descriptor an OLC structure.                                  **/
    /****************************************************************************/
    if (d->olc) {
        mudlog(BRF, ADMLVL_IMMORT, true,
               "SYSERR: do_oasis_sedit: Player already had olc structure.");
        free(d->olc);
    }

    CREATE(d->olc, struct oasis_olc_data, 1);

    /****************************************************************************/
    /** Find the zone.                                                         **/
    /****************************************************************************/
    OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
    if (OLC_ZNUM(d) == NOWHERE) {
        ch->sendf("Sorry, there is no zone for that number!\r\n");
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    /****************************************************************************/
    /** Everyone but IMPLs can only edit zones they have been assigned.        **/
    /****************************************************************************/
    if (!can_edit_zone(ch, OLC_ZNUM(d))) {
        send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);

        /**************************************************************************/
        /** Free the OLC structure.                                              **/
        /**************************************************************************/
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    if (save) {
        ch->sendf("Saving all shops in zone %d.\r\n",
                     zone_table[OLC_ZNUM(d)].number);
        mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), true,
               "OLC: %s saves shop info for zone %d.",
               GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);

        /**************************************************************************/
        /** Save the shops to the shop file.                                     **/
        /**************************************************************************/
        save_shops(OLC_ZNUM(d));

        /**************************************************************************/
        /** Free the OLC structure.                                              **/
        /**************************************************************************/
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    OLC_NUM(d) = number;

    if ((real_num = real_shop(number)) != NOTHING)
        sedit_setup_existing(d, real_num);
    else
        sedit_setup_new(d);

    sedit_disp_menu(d);
    STATE(d) = CON_SEDIT;

    act("$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
    ch->setFlag(FlagType::PC, PLR_WRITING);

    mudlog(BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
           GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

void sedit_setup_new(struct descriptor_data *d) {
    auto shop = new shop_data();
    /*
     * Fill in some default values.
     */
    S_KEEPER(shop) = NOBODY;
    S_CLOSE1(shop) = 28;
    S_BUYPROFIT(shop) = 1.0;
    S_SELLPROFIT(shop) = 1.0;
    /*
     * Add a spice of default strings.
     */
    S_NOITEM1(shop) = strdup("%s Sorry, I don't stock that item.");
    S_NOITEM2(shop) = strdup("%s You don't seem to have that.");
    S_NOCASH1(shop) = strdup("%s I can't afford that!");
    S_NOCASH2(shop) = strdup("%s You are too poor!");
    S_NOBUY(shop) = strdup("%s I don't trade in such items.");
    S_BUY(shop) = strdup("%s That'll be %d zenni, thanks.");
    S_SELL(shop) = strdup("%s I'll give you %d zenni for that.");
    /*
     * Stir the lists lightly.
     */

    S_BUYTYPE(shop, 0) = NOTHING;
    SET_BIT_AR(S_NOTRADE(shop), TRADE_NOBROKEN);

    /*
     * Presto! A shop.
     */
    OLC_SHOP(d) = shop;
}

/*-------------------------------------------------------------------*/

void sedit_setup_existing(struct descriptor_data *d, vnum rshop_num) {
    /*
     * Create a scratch shop structure.
     */
    OLC_SHOP(d) = new shop_data();

    /* don't waste time trying to free nullptr strings -- Welcor */
    copy_shop(OLC_SHOP(d), &shop_index[rshop_num], false);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

void sedit_products_menu(struct descriptor_data *d) {
    struct shop_data *shop;

    shop = OLC_SHOP(d);

    clear_screen(d);
    write_to_output(d, "##     VNUM     Product\r\n");
    for (auto i : shop->producing) {
        write_to_output(d, "%2d - [@c%5d@n] - @y%s@n\r\n", i,
                        i,
                        obj_proto[i]["short_description"].get<std::string>().c_str());
    }
    write_to_output(d, "\r\n"
                       "@gA@n) Add a new product.\r\n"
                       "@gD@n) Delete a product.\r\n"
                       "@gQ@n) Quit\r\n"
                       "Enter choice : ");

    OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_compact_rooms_menu(struct descriptor_data *d) {
    struct shop_data *shop;
    int i, count = 0;

    shop = OLC_SHOP(d);

    clear_screen(d);
    for (auto r : shop->in_room) {
        write_to_output(d, "%2d - [@c%5d@n]  | %s", r, r,
                        !(++count % 5) ? "\r\n" : "");
    }
    write_to_output(d, "\r\n"
                       "@gA@n) Add a new room.\r\n"
                       "@gD@n) Delete a room.\r\n"
                       "@gL@n) Long display.\r\n"
                       "@gQ@n) Quit\r\n"
                       "Enter choice : ");

    OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_rooms_menu(struct descriptor_data *d) {
    struct shop_data *shop;
    int i = 0;

    shop = OLC_SHOP(d);

    clear_screen(d);
    write_to_output(d, "##     VNUM     Room\r\n\r\n");
    for (auto r : shop->in_room) {
        if (world.contains(r)) {
            write_to_output(d, "%2d - [@c%5d@n] - @y%s@n\r\n", i++, r,
                            world[r]->getName());
        } else {
            write_to_output(d, "%2d - [@R!Removed Room!@n]\r\n", i);
        }
    }
    write_to_output(d, "\r\n"
                       "@gA@n) Add a new room.\r\n"
                       "@gD@n) Delete a room.\r\n"
                       "@gC@n) Compact Display.\r\n"
                       "@gQ@n) Quit\r\n"
                       "Enter choice : ");

    OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_namelist_menu(struct descriptor_data *d) {
    struct shop_data *shop;
    int i;

    shop = OLC_SHOP(d);

    clear_screen(d);
    write_to_output(d, "##              Type   Namelist\r\n\r\n");
    for (i = 0; S_BUYTYPE(shop, i) != NOTHING; i++) {
        write_to_output(d, "%2d - @c%15s@n - @y%s@n\r\n", i,
                        item_types[S_BUYTYPE(shop, i)],
                        S_BUYWORD(shop, i) ? S_BUYWORD(shop, i) : "<None>");
    }
    write_to_output(d, "\r\n"
                       "@gA@n) Add a new entry.\r\n"
                       "@gD@n) Delete an entry.\r\n"
                       "@gQ@n) Quit\r\n"
                       "Enter choice : ");

    OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_shop_flags_menu(struct descriptor_data *d) {
    char bits[MAX_STRING_LENGTH];
    int i, count = 0;

    clear_screen(d);
    for (i = 0; i < NUM_SHOP_FLAGS; i++) {
        write_to_output(d, "@g%2d@n) %-20.20s   %s", i + 1, shop_bits[i],
                        !(++count % 2) ? "\r\n" : "");
    }
    sprintbit(S_BITVECTOR(OLC_SHOP(d)), shop_bits, bits, sizeof(bits));
    write_to_output(d, "\r\nCurrent Shop Flags : @c%s@n\r\nEnter choice : ", bits);
    OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

void sedit_no_trade_menu(struct descriptor_data *d) {
    char bits[MAX_STRING_LENGTH];
    int i, count = 0;

    clear_screen(d);
    for (i = 0; i < NUM_TRADERS; i++) {
        write_to_output(d, "@g%2d@n) %-20.20s   %s", i + 1, trade_letters[i],
                        !(++count % 2) ? "\r\n" : "");
    }
    sprintbitarray(S_NOTRADE(OLC_SHOP(d)), trade_letters, sizeof(bits), bits);
    write_to_output(d, "\r\nCurrently won't trade with: @c%s@n\r\n"
                       "Enter choice : ", bits);
    OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

void sedit_types_menu(struct descriptor_data *d) {
    struct shop_data *shop;
    int i, count = 0;

    shop = OLC_SHOP(d);

    clear_screen(d);
    for (i = 0; i < NUM_ITEM_TYPES; i++) {
        write_to_output(d, "@g%2d@n) @c%-20s@n  %s", i, item_types[i],
                        !(++count % 3) ? "\r\n" : "");
    }
    write_to_output(d, "@nEnter choice : ");
    OLC_MODE(d) = SEDIT_TYPE_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void sedit_disp_menu(struct descriptor_data *d) {
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    struct shop_data *shop;

    shop = OLC_SHOP(d);

    clear_screen(d);
    sprintbitarray(S_NOTRADE(shop), trade_letters, sizeof(buf1), buf1);
    sprintbit(S_BITVECTOR(shop), shop_bits, buf2, sizeof(buf2));
    write_to_output(d,
                    "-- Shop Number : [@c%d@n]\r\n"
                    "@g0@n) Keeper      : [@c%d@n] @y%s\r\n"
                    "@g1@n) Open 1      : @c%4d@n          @g2@n) Close 1     : @c%4d\r\n"
                    "@g3@n) Open 2      : @c%4d@n          @g4@n) Close 2     : @c%4d\r\n"
                    "@g5@n) Sell rate   : @c%1.2f@n          @g6@n) Buy rate    : @c%1.2f\r\n"
                    "@g7@n) Keeper no item : @y%s\r\n"
                    "@g8@n) Player no item : @y%s\r\n"
                    "@g9@n) Keeper no cash : @y%s\r\n"
                    "@gA@n) Player no cash : @y%s\r\n"
                    "@gB@n) Keeper no buy  : @y%s\r\n"
                    "@gC@n) Buy success    : @y%s\r\n"
                    "@gD@n) Sell success   : @y%s\r\n"
                    "@gE@n) No Trade With  : @c%s\r\n"
                    "@gF@n) Shop flags     : @c%s\r\n"
                    "@gR@n) Rooms Menu\r\n"
                    "@gP@n) Products Menu\r\n"
                    "@gT@n) Accept Types Menu\r\n"
                    "@gW@n) Copy Shop\r\n"
                    "@gQ@n) Quit\r\n"
                    "Enter Choice : ",

                    OLC_NUM(d),
                    S_KEEPER(shop) == NOBODY ? -1 : mob_index[S_KEEPER(shop)].vn,
                    S_KEEPER(shop) == NOBODY ? "None" : mob_proto[S_KEEPER(shop)]["short_descripton"].get<std::string>().c_str(),
                    S_OPEN1(shop),
                    S_CLOSE1(shop),
                    S_OPEN2(shop),
                    S_CLOSE2(shop),
                    S_BUYPROFIT(shop),
                    S_SELLPROFIT(shop),
                    S_NOITEM1(shop),
                    S_NOITEM2(shop),
                    S_NOCASH1(shop),
                    S_NOCASH2(shop),
                    S_NOBUY(shop),
                    S_BUY(shop),
                    S_SELL(shop),
                    buf1,
                    buf2
    );

    OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

void sedit_parse(struct descriptor_data *d, char *arg) {
    int i;

    if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE) {
        if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
            write_to_output(d, "Field must be numerical, try again : ");
            return;
        }
    }
    switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
        case SEDIT_CONFIRM_SAVESTRING:
            switch (*arg) {
                case 'y':
                case 'Y':
                    sedit_save_internally(d);
                    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), true,
                           "OLC: %s edits shop %d", GET_NAME(d->character), OLC_NUM(d));
                    if (CONFIG_OLC_SAVE) {
                        sedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
                        write_to_output(d, "Shop saved to disk.\r\n");
                    } else
                        write_to_output(d, "Shop saved to memory.\r\n");
                    cleanup_olc(d, CLEANUP_STRUCTS);
                    return;
                case 'n':
                case 'N':
                    cleanup_olc(d, CLEANUP_ALL);
                    return;
                default:
                    write_to_output(d, "Invalid choice!\r\nDo you wish to save your changes? : ");
                    return;
            }
            break;

/*-------------------------------------------------------------------*/
        case SEDIT_MAIN_MENU:
            i = 0;
            switch (*arg) {
                case 'q':
                case 'Q':
                    if (OLC_VAL(d)) {        /* Anything been changed? */
                        write_to_output(d, "Do you wish to save your changes? : ");
                        OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
                    } else
                        cleanup_olc(d, CLEANUP_ALL);
                    return;
                case '0':
                    OLC_MODE(d) = SEDIT_KEEPER;
                    write_to_output(d, "Enter vnum number of shop keeper : ");
                    return;
                case '1':
                    OLC_MODE(d) = SEDIT_OPEN1;
                    i++;
                    break;
                case '2':
                    OLC_MODE(d) = SEDIT_CLOSE1;
                    i++;
                    break;
                case '3':
                    OLC_MODE(d) = SEDIT_OPEN2;
                    i++;
                    break;
                case '4':
                    OLC_MODE(d) = SEDIT_CLOSE2;
                    i++;
                    break;
                case '5':
                    OLC_MODE(d) = SEDIT_BUY_PROFIT;
                    i++;
                    break;
                case '6':
                    OLC_MODE(d) = SEDIT_SELL_PROFIT;
                    i++;
                    break;
                case '7':
                    OLC_MODE(d) = SEDIT_NOITEM1;
                    i--;
                    break;
                case '8':
                    OLC_MODE(d) = SEDIT_NOITEM2;
                    i--;
                    break;
                case '9':
                    OLC_MODE(d) = SEDIT_NOCASH1;
                    i--;
                    break;
                case 'a':
                case 'A':
                    OLC_MODE(d) = SEDIT_NOCASH2;
                    i--;
                    break;
                case 'b':
                case 'B':
                    OLC_MODE(d) = SEDIT_NOBUY;
                    i--;
                    break;
                case 'c':
                case 'C':
                    OLC_MODE(d) = SEDIT_BUY;
                    i--;
                    break;
                case 'd':
                case 'D':
                    OLC_MODE(d) = SEDIT_SELL;
                    i--;
                    break;
                case 'e':
                case 'E':
                    sedit_no_trade_menu(d);
                    return;
                case 'f':
                case 'F':
                    sedit_shop_flags_menu(d);
                    return;
                case 'r':
                case 'R':
                    sedit_rooms_menu(d);
                    return;
                case 'p':
                case 'P':
                    sedit_products_menu(d);
                    return;
                case 't':
                case 'T':
                    sedit_namelist_menu(d);
                    return;
                case 'w':
                case 'W':
                    write_to_output(d, "Copy what shop? ");
                    OLC_MODE(d) = SEDIT_COPY;
                    return;
                default:
                    sedit_disp_menu(d);
                    return;
            }

            if (i == 0)
                break;
            else if (i == 1)
                write_to_output(d, "\r\nEnter new value : ");
            else if (i == -1)
                write_to_output(d, "\r\nEnter new text :\r\n] ");
            else
                write_to_output(d, "Oops...\r\n");
            return;
/*-------------------------------------------------------------------*/
        case SEDIT_NAMELIST_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    sedit_types_menu(d);
                    return;
                case 'd':
                case 'D':
                    write_to_output(d, "\r\nDelete which entry? : ");
                    OLC_MODE(d) = SEDIT_DELETE_TYPE;
                    return;
                case 'q':
                case 'Q':
                    break;
            }
            break;
/*-------------------------------------------------------------------*/
        case SEDIT_PRODUCTS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    write_to_output(d, "\r\nEnter new product vnum number : ");
                    OLC_MODE(d) = SEDIT_NEW_PRODUCT;
                    return;
                case 'd':
                case 'D':
                    write_to_output(d, "\r\nDelete which product? : ");
                    OLC_MODE(d) = SEDIT_DELETE_PRODUCT;
                    return;
                case 'q':
                case 'Q':
                    break;
            }
            break;
/*-------------------------------------------------------------------*/
        case SEDIT_ROOMS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    write_to_output(d, "\r\nEnter new room vnum number : ");
                    OLC_MODE(d) = SEDIT_NEW_ROOM;
                    return;
                case 'c':
                case 'C':
                    sedit_compact_rooms_menu(d);
                    return;
                case 'l':
                case 'L':
                    sedit_rooms_menu(d);
                    return;
                case 'd':
                case 'D':
                    write_to_output(d, "\r\nDelete which room? : ");
                    OLC_MODE(d) = SEDIT_DELETE_ROOM;
                    return;
                case 'q':
                case 'Q':
                    break;
            }
            break;
/*-------------------------------------------------------------------*/
            /*
             * String edits.
             */
        case SEDIT_NOITEM1:
            if (genolc_checkstring(d, arg))
                modify_string(&S_NOITEM1(OLC_SHOP(d)), arg);
            break;
        case SEDIT_NOITEM2:
            if (genolc_checkstring(d, arg))
                modify_string(&S_NOITEM2(OLC_SHOP(d)), arg);
            break;
        case SEDIT_NOCASH1:
            if (genolc_checkstring(d, arg))
                modify_string(&S_NOCASH1(OLC_SHOP(d)), arg);
            break;
        case SEDIT_NOCASH2:
            if (genolc_checkstring(d, arg))
                modify_string(&S_NOCASH2(OLC_SHOP(d)), arg);
            break;
        case SEDIT_NOBUY:
            if (genolc_checkstring(d, arg))
                modify_string(&S_NOBUY(OLC_SHOP(d)), arg);
            break;
        case SEDIT_BUY:
            if (genolc_checkstring(d, arg))
                modify_string(&S_BUY(OLC_SHOP(d)), arg);
            break;
        case SEDIT_SELL:
            if (genolc_checkstring(d, arg))
                modify_string(&S_SELL(OLC_SHOP(d)), arg);
            break;
        case SEDIT_NAMELIST:
            if (genolc_checkstring(d, arg)) {
                auto &new_entry = OLC_SHOP(d)->type.emplace_back();

                BUY_TYPE(new_entry) = OLC_VAL(d);
                new_entry.keywords = arg;
            }
            sedit_namelist_menu(d);
            return;

/*-------------------------------------------------------------------*/
            /*
             * Numerical responses.
             */
        case SEDIT_KEEPER:
            i = atoi(arg);
            if ((i = atoi(arg)) != -1)
                if ((i = real_mobile(i)) == NOBODY) {
                    write_to_output(d, "That mobile does not exist, try again : ");
                    return;
                }
            S_KEEPER(OLC_SHOP(d)) = i;
            if (i == -1)
                break;
            /*
             * Fiddle with special procs.
             */
            S_FUNC(OLC_SHOP(d)) = mob_index[i].func != shop_keeper ? mob_index[i].func : nullptr;
            mob_index[i].func = shop_keeper;
            break;
        case SEDIT_OPEN1:
            S_OPEN1(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
            break;
        case SEDIT_OPEN2:
            S_OPEN2(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
            break;
        case SEDIT_CLOSE1:
            S_CLOSE1(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
            break;
        case SEDIT_CLOSE2:
            S_CLOSE2(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
            break;
        case SEDIT_BUY_PROFIT:
            sscanf(arg, "%f", &S_BUYPROFIT(OLC_SHOP(d)));
            break;
        case SEDIT_SELL_PROFIT:
            sscanf(arg, "%f", &S_SELLPROFIT(OLC_SHOP(d)));
            break;
        case SEDIT_TYPE_MENU:
            OLC_VAL(d) = LIMIT(atoi(arg), 0, NUM_ITEM_TYPES - 1);
            write_to_output(d, "Enter namelist (return for none) :-\r\n] ");
            OLC_MODE(d) = SEDIT_NAMELIST;
            return;
        case SEDIT_DELETE_TYPE:
            OLC_SHOP(d)->type.erase(OLC_SHOP(d)->type.begin()+atoi(arg));
            sedit_namelist_menu(d);
            return;
        case SEDIT_NEW_PRODUCT:
            if ((i = atoi(arg)) != -1)
                if ((i = real_object(i)) == NOTHING) {
                    write_to_output(d, "That object does not exist, try again : ");
                    return;
                }
            if (i > 0)
                OLC_SHOP(d)->add_product(i);
            sedit_products_menu(d);
            return;
        case SEDIT_DELETE_PRODUCT:
            OLC_SHOP(d)->remove_product(atol(arg));
            sedit_products_menu(d);
            return;
        case SEDIT_NEW_ROOM:
            if ((i = atoi(arg)) != -1)
                if ((i = real_room(i)) == NOWHERE) {
                    write_to_output(d, "That room does not exist, try again : ");
                    return;
                }
            if (i >= 0)
                OLC_SHOP(d)->in_room.insert(atoi(arg));
            sedit_rooms_menu(d);
            return;
        case SEDIT_DELETE_ROOM:
            OLC_SHOP(d)->in_room.erase(atoi(arg));
            sedit_rooms_menu(d);
            return;
        case SEDIT_SHOP_FLAGS:
            if ((i = LIMIT(atoi(arg), 0, NUM_SHOP_FLAGS)) > 0) {
                TOGGLE_BIT(S_BITVECTOR(OLC_SHOP(d)), 1 << (i - 1));
                sedit_shop_flags_menu(d);
                return;
            }
            break;
        case SEDIT_NOTRADE:
            if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS)) > 0) {
                TOGGLE_BIT_AR(S_NOTRADE(OLC_SHOP(d)), i - 1);
                sedit_no_trade_menu(d);
                return;
            }
            break;
        case SEDIT_COPY:
            if ((i = real_room(atoi(arg))) != NOWHERE) {
                sedit_setup_existing(d, i);
            } else
                write_to_output(d, "That shop does not exist.\r\n");
            break;

/*-------------------------------------------------------------------*/
        default:
            /*
             * We should never get here.
             */
            cleanup_olc(d, CLEANUP_ALL);
            mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: sedit_parse(): Reached default case!");
            write_to_output(d, "Oops...\r\n");
            break;
    }

/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag.
 */
    OLC_VAL(d) = 1;
    sedit_disp_menu(d);
}
