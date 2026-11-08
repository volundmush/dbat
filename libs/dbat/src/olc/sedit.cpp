/************************************************************************
 * OasisOLC - Shops / sedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/shop.hpp"
#include "dbat/config.hpp"
#include "dbat/sedit.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/oasis.hpp"
#include "dbat/races.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void sedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void sedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct shop_data *shop);
static void sedit_setup_new(struct descriptor_data *d);

/*
 * Should check more things.
 */
static void sedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  OLC_SHOP(d)->vnum = OLC_NUM(d);
  add_shop(g, OLC_SHOP(d));
}

static void sedit_save_to_disk(GameWorld *g, struct zone_data *num)
{
  save_shops(g, num);
}

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

ACMD(do_oasis_sedit)
{
  IDXTYPE number = NOWHERE;
  int save       = 0;
  struct shop_data *existing;
  struct descriptor_data *d;
  char buf1[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  two_arguments(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(g, ch, "Specify a shop VNUM to edit.\r\n");
    return;
  }
  if (!isdigit(*buf1)) {
    if (strcasecmp("save", buf1) != 0) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = true;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (char_get_olc_zone(g, ch) > 0) {
      struct zone_data *zlok;

      if ((zlok = find_zone(g, char_get_olc_zone(g, ch))) == nullptr)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(g, zlok);
    }

    if (number == NOWHERE) {
      send_to_char(g, ch, "Save which zone?\r\n");
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
  for (descriptor_data *dd : each_desc(g)) {
    if (desc_get_connected(dd) == CON_SEDIT) {
      if (dd->olc && OLC_NUM(dd) == number) {
        send_to_char(g, ch, "That shop is currently being edited by %s.\r\n",
                     char_pers(g, dd->character, ch));
        return;
      }
    }
  }

  /****************************************************************************/
  /** Point d to the builder's descriptor.                                   **/
  /****************************************************************************/
  d = char_get_desc(ch);

  /****************************************************************************/
  /** Give the descriptor an OLC structure.                                  **/
  /****************************************************************************/
  if (d->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
           "SYSERR: do_oasis_sedit: Player already had olc structure.");
    delete d->olc;
  }

  d->olc = new oasis_olc_data();

  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  OLC_ZNUM(d) = save ? find_zone(g, number) : find_zone_by_thing(g, number);
  if (OLC_ZNUM(d) == nullptr) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(g, ch, OLC_ZNUM(d))) {
    send_cannot_edit(g, ch, OLC_ZNUM(d)->number);

    /**************************************************************************/
    /** Free the OLC structure.                                              **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  if (save) {
    send_to_char(g, ch, "Saving all shops in zone %d.\r\n",
                 OLC_ZNUM(d)->number);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
           "OLC: %s saves shop info for zone %d.",
           char_get_name(ch), OLC_ZNUM(d)->number);

    /**************************************************************************/
    /** Save the shops to the shop file.                                     **/
    /**************************************************************************/
    save_shops(g, OLC_ZNUM(d));

    /**************************************************************************/
    /** Free the OLC structure.                                              **/
    /**************************************************************************/
    delete d->olc;
    d->olc = nullptr;
    return;
  }

  OLC_NUM(d) = number;

  if ((existing = find_shop(g, number)) != nullptr)
    sedit_setup_existing(g, d, existing);
  else
    sedit_setup_new(d);

  sedit_disp_menu(g, d);
  desc_set_connected(d, CON_SEDIT);

  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);

  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
         char_get_name(ch), OLC_ZNUM(d)->number, char_get_olc_zone(g, ch));
}

static void sedit_setup_new(struct descriptor_data *d)
{
  struct shop_data *shop;

  /*
   * Allocate a scratch shop structure.
   */
  shop = new shop_data();

  /*
   * Fill in some default values.
   */
  shop_set_keeper(shop, NOBODY);
  shop_set_close1(shop, 28);
  shop_set_profit_buy(shop, 1.0);
  shop_set_profit_sell(shop, 1.0);
  /*
   * Add a spice of default strings.
   */
  shop_set_no_such_item1(shop, shop_default_msgs[SHOP_MSG_NOITEM1]);
  shop_set_no_such_item2(shop, shop_default_msgs[SHOP_MSG_NOITEM2]);
  shop_set_missing_cash1(shop, shop_default_msgs[SHOP_MSG_NOCASH1]);
  shop_set_missing_cash2(shop, shop_default_msgs[SHOP_MSG_NOCASH2]);
  shop_set_do_not_buy(shop, shop_default_msgs[SHOP_MSG_NOBUY]);
  shop_set_message_buy(shop, shop_default_msgs[SHOP_MSG_BUY]);
  shop_set_message_sell(shop, shop_default_msgs[SHOP_MSG_SELL]);
  /* It sells nothing, nowhere, and buys nothing, until the builder says otherwise. */
  shop_trade_set(shop, TRADE_NOBROKEN, true);

  /*
   * Presto! A shop.
   */
  OLC_SHOP(d) = shop;
}

/*-------------------------------------------------------------------*/

static void sedit_setup_existing(GameWorld *g, struct descriptor_data *d, struct shop_data *shop)
{
  /*
   * Create a scratch shop structure.
   */
  OLC_SHOP(d) = new shop_data();

  copy_shop(OLC_SHOP(d), shop);
}

/* Removes the entry the builder numbered n, if there is one. */
template <typename T>
static void erase_at(std::vector<T> &list, int n)
{
  if (n >= 0 && n < (int)list.size())
    list.erase(list.begin() + n);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

static void sedit_products_menu(GameWorld *g, struct descriptor_data *d)
{
  struct shop_data *shop;
  int i;

  shop = OLC_SHOP(d);

  clear_screen(g, d);
  write_to_output(g, d, "##     VNUM     Product\r\n");
  for (i = 0; i < (int)shop_products(shop).size(); i++) {
    write_to_output(g, d, "%2d - [@c%5d@n] - @y%s@n\r\n", i,
                    obj_index_of(g, shop_products(shop)[i])->vnum,
                    obj_get_short_description(obj_proto_of(g, shop_products(shop)[i])));
  }
  write_to_output(g, d, "\r\n"
                     "@gA@n) Add a new product.\r\n"
                     "@gD@n) Delete a product.\r\n"
                     "@gQ@n) Quit\r\n"
                     "Enter choice : ");

  OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

static void sedit_compact_rooms_menu(GameWorld *g, struct descriptor_data *d)
{
  struct shop_data *shop;
  int i, count = 0;

  shop = OLC_SHOP(d);

  clear_screen(g, d);
  for (i = 0; i < (int)shop_rooms(shop).size(); i++) {
    write_to_output(g, d, "%2d - [@c%5d@n]  | %s", i, shop_rooms(shop)[i],
                    !(++count % 5) ? "\r\n" : "");
  }
  write_to_output(g, d, "\r\n"
                     "@gA@n) Add a new room.\r\n"
                     "@gD@n) Delete a room.\r\n"
                     "@gL@n) Long display.\r\n"
                     "@gQ@n) Quit\r\n"
                     "Enter choice : ");

  OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

static void sedit_rooms_menu(GameWorld *g, struct descriptor_data *d)
{
  struct shop_data *shop;
  int i;

  shop = OLC_SHOP(d);

  clear_screen(g, d);
  write_to_output(g, d, "##     VNUM     Room\r\n\r\n");
  for (i = 0; i < (int)shop_rooms(shop).size(); i++) {
    if (room_of(g, shop_rooms(shop)[i])) {
      write_to_output(g, d, "%2d - [@c%5d@n] - @y%s@n\r\n", i, shop_rooms(shop)[i], room_get_name(g, shop_rooms(shop)[i]));
    } else {
      write_to_output(g, d, "%2d - [@R!Removed Room!@n]\r\n", i);
    }
  }
  write_to_output(g, d, "\r\n"
                     "@gA@n) Add a new room.\r\n"
                     "@gD@n) Delete a room.\r\n"
                     "@gC@n) Compact Display.\r\n"
                     "@gQ@n) Quit\r\n"
                     "Enter choice : ");

  OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

static void sedit_namelist_menu(GameWorld *g, struct descriptor_data *d)
{
  struct shop_data *shop;
  int i;

  shop = OLC_SHOP(d);

  clear_screen(g, d);
  write_to_output(g, d, "##              Type   Namelist\r\n\r\n");
  for (i = 0; i < (int)shop_buy_types(shop).size(); i++) {
    write_to_output(g, d, "%2d - @c%15s@n - @y%s@n\r\n", i,
                    item_types[shop_buy_types(shop)[i].type],
                    shop_buy_get_keywords(&shop_buy_types(shop)[i]) ? shop_buy_get_keywords(&shop_buy_types(shop)[i]) : "<None>");
  }
  write_to_output(g, d, "\r\n"
                     "@gA@n) Add a new entry.\r\n"
                     "@gD@n) Delete an entry.\r\n"
                     "@gQ@n) Quit\r\n"
                     "Enter choice : ");

  OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}

/*-------------------------------------------------------------------*/

static void sedit_shop_flags_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  int i, count = 0;

  clear_screen(g, d);
  for (i = 0; i < NUM_SHOP_FLAGS; i++) {
    write_to_output(g, d, "@g%2d@n) %-20.20s   %s", i + 1, shop_bits[i],
                    !(++count % 2) ? "\r\n" : "");
  }
  sprintbit(shop_get_bitvector(OLC_SHOP(d)), shop_bits, bits, sizeof(bits));
  write_to_output(g, d, "\r\nCurrent Shop Flags : @c%s@n\r\nEnter choice : ", bits);
  OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

static void sedit_no_trade_menu(GameWorld *g, struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  int i, count = 0;

  clear_screen(g, d);
  for (i = 0; i < NUM_TRADERS; i++) {
    write_to_output(g, d, "@g%2d@n) %-20.20s   %s", i + 1, trade_letters[i],
                    !(++count % 2) ? "\r\n" : "");
  }
  sprintbitarray(shop_get_with_who(OLC_SHOP(d)), trade_letters, 4, bits, sizeof(bits));
  write_to_output(g, d, "\r\nCurrently won't trade with: @c%s@n\r\n"
                     "Enter choice : ",
                  bits);
  OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

static void sedit_types_menu(GameWorld *g, struct descriptor_data *d)
{
  int i, count = 0;

  clear_screen(g, d);
  for (i = 0; i < NUM_ITEM_TYPES; i++) {
    write_to_output(g, d, "@g%2d@n) @c%-20s@n  %s", i, item_types[i],
                    !(++count % 3) ? "\r\n" : "");
  }
  write_to_output(g, d, "@nEnter choice : ");
  OLC_MODE(d) = SEDIT_TYPE_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
static void sedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct shop_data *shop;

  shop = OLC_SHOP(d);

  clear_screen(g, d);
  sprintbitarray(shop_get_with_who(shop), trade_letters, 4, buf1, sizeof(buf1));
  sprintbit(shop_get_bitvector(shop), shop_bits, buf2, sizeof(buf2));
  write_to_output(g, d,
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
                  shop_get_keeper(shop) == NOBODY ? -1 : mob_index_of(g, shop_get_keeper(shop))->vnum,
                  shop_get_keeper(shop) == NOBODY ? "None" : char_get_sdesc(mob_proto_of(g, shop_get_keeper(shop))),
                  shop_get_open1(shop),
                  shop_get_close1(shop),
                  shop_get_open2(shop),
                  shop_get_close2(shop),
                  shop_get_profit_buy(shop),
                  shop_get_profit_sell(shop),
                  shop_get_no_such_item1(shop),
                  shop_get_no_such_item2(shop),
                  shop_get_missing_cash1(shop),
                  shop_get_missing_cash2(shop),
                  shop_get_do_not_buy(shop),
                  shop_get_message_buy(shop),
                  shop_get_message_sell(shop),
                  buf1,
                  buf2);

  OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

/* Stores a builder-written shop message, or tells the builder why it can't be used. */
static void sedit_set_message(GameWorld *g, struct descriptor_data *d, std::string *msg, char *arg, bool allow_int)
{
  const char *problem = modify_string(msg, arg, allow_int);

  if (problem)
    write_to_output(g, d,
                    "Invalid message: %s. It starts with %%s, the customer's name (added if you leave it off)%s, "
                    "and can have no other %% codes.\r\n",
                    problem, allow_int ? ", may then use one %d for the price" : "");
}

void sedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  IDXTYPE i;

  if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE) {
    if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
      write_to_output(g, d, "Field must be numerical, try again : ");
      return;
    }
  }
  switch (OLC_MODE(d)) {
      /*-------------------------------------------------------------------*/
    case SEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          sedit_save_internally(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true,
                 "OLC: %s edits shop %d", char_get_name(d->character), OLC_NUM(d));
          if (CONFIG_OLC_SAVE) {
            sedit_save_to_disk(g, find_zone_by_thing(g, OLC_NUM(d)));
            write_to_output(g, d, "Shop saved to disk.\r\n");
          } else
            write_to_output(g, d, "Shop saved to memory.\r\n");
          cleanup_olc(g, d, CLEANUP_STRUCTS);
          return;
        case 'n':
        case 'N':
          cleanup_olc(g, d, CLEANUP_ALL);
          return;
        default:
          write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes? : ");
          return;
      }
      break;

      /*-------------------------------------------------------------------*/
    case SEDIT_MAIN_MENU:
      i = 0;
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_VAL(d)) { /* Anything been changed? */
            write_to_output(g, d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(g, d, CLEANUP_ALL);
          return;
        case '0':
          OLC_MODE(d) = SEDIT_KEEPER;
          write_to_output(g, d, "Enter vnum number of shop keeper : ");
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
          sedit_no_trade_menu(g, d);
          return;
        case 'f':
        case 'F':
          sedit_shop_flags_menu(g, d);
          return;
        case 'r':
        case 'R':
          sedit_rooms_menu(g, d);
          return;
        case 'p':
        case 'P':
          sedit_products_menu(g, d);
          return;
        case 't':
        case 'T':
          sedit_namelist_menu(g, d);
          return;
        case 'w':
        case 'W':
          write_to_output(g, d, "Copy what shop? ");
          OLC_MODE(d) = SEDIT_COPY;
          return;
        default:
          sedit_disp_menu(g, d);
          return;
      }

      if (i == 0)
        break;
      else if (i == 1)
        write_to_output(g, d, "\r\nEnter new value : ");
      else if (i == -1)
        write_to_output(g, d, "\r\nEnter new text :\r\n] ");
      else
        write_to_output(g, d, "Oops...\r\n");
      return;
      /*-------------------------------------------------------------------*/
    case SEDIT_NAMELIST_MENU:
      switch (*arg) {
        case 'a':
        case 'A':
          sedit_types_menu(g, d);
          return;
        case 'd':
        case 'D':
          write_to_output(g, d, "\r\nDelete which entry? : ");
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
          write_to_output(g, d, "\r\nEnter new product vnum number : ");
          OLC_MODE(d) = SEDIT_NEW_PRODUCT;
          return;
        case 'd':
        case 'D':
          write_to_output(g, d, "\r\nDelete which product? : ");
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
          write_to_output(g, d, "\r\nEnter new room vnum number : ");
          OLC_MODE(d) = SEDIT_NEW_ROOM;
          return;
        case 'c':
        case 'C':
          sedit_compact_rooms_menu(g, d);
          return;
        case 'l':
        case 'L':
          sedit_rooms_menu(g, d);
          return;
        case 'd':
        case 'D':
          write_to_output(g, d, "\r\nDelete which room? : ");
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
        sedit_set_message(g, d, &((OLC_SHOP(d))->no_such_item1), arg, false);
      break;
    case SEDIT_NOITEM2:
      if (genolc_checkstring(d, arg))
        sedit_set_message(g, d, &((OLC_SHOP(d))->no_such_item2), arg, false);
      break;
    case SEDIT_NOCASH1:
      if (genolc_checkstring(d, arg))
        sedit_set_message(g, d, &((OLC_SHOP(d))->missing_cash1), arg, false);
      break;
    case SEDIT_NOCASH2:
      if (genolc_checkstring(d, arg))
        sedit_set_message(g, d, &((OLC_SHOP(d))->missing_cash2), arg, false);
      break;
    case SEDIT_NOBUY:
      if (genolc_checkstring(d, arg))
        sedit_set_message(g, d, &((OLC_SHOP(d))->do_not_buy), arg, false);
      break;
    case SEDIT_BUY:
      if (genolc_checkstring(d, arg))
        sedit_set_message(g, d, &((OLC_SHOP(d))->message_buy), arg, true);
      break;
    case SEDIT_SELL:
      if (genolc_checkstring(d, arg))
        sedit_set_message(g, d, &((OLC_SHOP(d))->message_sell), arg, true);
      break;
    case SEDIT_NAMELIST:
      if (genolc_checkstring(d, arg)) {
        shop_buy_types(OLC_SHOP(d)).push_back({.type = OLC_VAL(d), .keywords = arg});
      }
      sedit_namelist_menu(g, d);
      return;

      /*-------------------------------------------------------------------*/
      /*
     * Numerical responses.
     */
    case SEDIT_KEEPER:
      i = atoi(arg);
      if ((i = atoi(arg)) != -1)
        if (!mob_proto_of(g, i)) {
          write_to_output(g, d, "That mobile does not exist, try again : ");
          return;
        }
      shop_set_keeper(OLC_SHOP(d), i);
      if (i == -1)
        break;
      /*
     * Fiddle with special procs.
     */
      shop_set_func(OLC_SHOP(d), mob_index_of(g, i)->func != shop_keeper ? mob_index_of(g, i)->func : nullptr);
      mob_index_of(g, i)->func = shop_keeper;
      break;
    case SEDIT_OPEN1:
      shop_set_open1(OLC_SHOP(d), LIMIT(atoi(arg), 0, 28));
      break;
    case SEDIT_OPEN2:
      shop_set_open2(OLC_SHOP(d), LIMIT(atoi(arg), 0, 28));
      break;
    case SEDIT_CLOSE1:
      shop_set_close1(OLC_SHOP(d), LIMIT(atoi(arg), 0, 28));
      break;
    case SEDIT_CLOSE2:
      shop_set_close2(OLC_SHOP(d), LIMIT(atoi(arg), 0, 28));
      break;
    case SEDIT_BUY_PROFIT:
      sscanf(arg, "%f", &((OLC_SHOP(d))->profit_buy));
      break;
    case SEDIT_SELL_PROFIT:
      sscanf(arg, "%f", &((OLC_SHOP(d))->profit_sell));
      break;
    case SEDIT_TYPE_MENU:
      OLC_VAL(d) = LIMIT(atoi(arg), 0, NUM_ITEM_TYPES - 1);
      write_to_output(g, d, "Enter namelist (return for none) :-\r\n] ");
      OLC_MODE(d) = SEDIT_NAMELIST;
      return;
    case SEDIT_DELETE_TYPE:
      erase_at(shop_buy_types(OLC_SHOP(d)), atoi(arg));
      sedit_namelist_menu(g, d);
      return;
    case SEDIT_NEW_PRODUCT:
      if ((i = atoi(arg)) != -1)
        if (!obj_proto_of(g, i)) {
          write_to_output(g, d, "That object does not exist, try again : ");
          return;
        }
      if (i > 0)
        shop_products(OLC_SHOP(d)).push_back(i);
      sedit_products_menu(g, d);
      return;
    case SEDIT_DELETE_PRODUCT:
      erase_at(shop_products(OLC_SHOP(d)), atoi(arg));
      sedit_products_menu(g, d);
      return;
    case SEDIT_NEW_ROOM:
      if ((i = atoi(arg)) != -1)
        if (!room_of(g, i)) {
          write_to_output(g, d, "That room does not exist, try again : ");
          return;
        }
      if (i >= 0)
        shop_rooms(OLC_SHOP(d)).push_back(i);
      sedit_rooms_menu(g, d);
      return;
    case SEDIT_DELETE_ROOM:
      erase_at(shop_rooms(OLC_SHOP(d)), atoi(arg));
      sedit_rooms_menu(g, d);
      return;
    case SEDIT_SHOP_FLAGS:
      if ((i = LIMIT(atoi(arg), 0, NUM_SHOP_FLAGS)) > 0) {
        shop_set_bitvector(OLC_SHOP(d), shop_get_bitvector(OLC_SHOP(d)) ^ (1U << (i - 1)));
        sedit_shop_flags_menu(g, d);
        return;
      }
      break;
    case SEDIT_NOTRADE:
      if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS)) > 0) {
        shop_trade_toggle(OLC_SHOP(d), i - 1);
        sedit_no_trade_menu(g, d);
        return;
      }
      break;
    case SEDIT_COPY:
      if (struct shop_data *source = find_shop(g, atoi(arg))) {
        delete OLC_SHOP(d);
        sedit_setup_existing(g, d, source);
      } else
        write_to_output(g, d, "That shop does not exist.\r\n");
      break;

      /*-------------------------------------------------------------------*/
    default:
      /*
     * We should never get here.
     */
      cleanup_olc(g, d, CLEANUP_ALL);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: sedit_parse(): Reached default case!");
      write_to_output(g, d, "Oops...\r\n");
      break;
  }

  /*-------------------------------------------------------------------*/

  /*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag.
 */
  OLC_VAL(d) = 1;
  sedit_disp_menu(g, d);
}
