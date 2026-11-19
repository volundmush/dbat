/************************************************************************
 * OasisOLC - Shops / sedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include <algorithm>
#include <cctype>
#include <type_traits>
#include <variant>

#include "dbat/act.informative.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/modify.hpp"
#include "dbat/oasis.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/races.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/sedit.hpp"
#include "dbat/shop.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"

namespace {
  /* What the shop editor is asking; from open1 on, the answer is a number */
  enum class mode {
    main_menu, confirm_save, products_menu, rooms_menu, namelist_menu, namelist,
    no_item1, no_item2, no_cash1, no_cash2, no_buy, buy, sell,
    open1, open2, close1, close2, keeper, buy_profit, sell_profit, type_menu, delete_type, delete_product, new_product,
    delete_room, new_room, shop_flags, no_trade, copy
  };

  /* A field the main menu asks for straight away: its key, and where the answer goes */
  struct field {
    char key;
    mode asks;
    int context;
    std::variant<int shop_data::*, float shop_data::*, std::string shop_data::*> where; /* an hour, a rate, a message */
    bool allow_int = false; /* a message that may give the price (%d) */
  };

  constexpr field FIELDS[] = {
      {'1', mode::open1,       CONTEXT_SEDIT_OPEN1,       &shop_data::open1        },
      {'2', mode::close1,      CONTEXT_SEDIT_CLOSE1,      &shop_data::close1       },
      {'3', mode::open2,       CONTEXT_SEDIT_OPEN2,       &shop_data::open2        },
      {'4', mode::close2,      CONTEXT_SEDIT_CLOSE2,      &shop_data::close2       },
      {'5', mode::buy_profit,  CONTEXT_SEDIT_BUY_PROFIT,  &shop_data::profit_buy   },
      {'6', mode::sell_profit, CONTEXT_SEDIT_SELL_PROFIT, &shop_data::profit_sell  },
      {'7', mode::no_item1,    CONTEXT_SEDIT_NOITEM1,     &shop_data::no_such_item1},
      {'8', mode::no_item2,    CONTEXT_SEDIT_NOITEM2,     &shop_data::no_such_item2},
      {'9', mode::no_cash1,    CONTEXT_SEDIT_NOCASH1,     &shop_data::missing_cash1},
      {'a', mode::no_cash2,    CONTEXT_SEDIT_NOCASH2,     &shop_data::missing_cash2},
      {'b', mode::no_buy,      CONTEXT_SEDIT_NOBUY,       &shop_data::do_not_buy   },
      {'c', mode::buy,         CONTEXT_SEDIT_BUY,         &shop_data::message_buy,  true},
      {'d', mode::sell,        CONTEXT_SEDIT_SELL,        &shop_data::message_sell, true},
  };

  /* The context help of the modes that aren't fields */
  constexpr std::pair<mode, int> CONTEXTS[] = {
      {mode::main_menu,      CONTEXT_SEDIT_MAIN_MENU         },
      {mode::confirm_save,   CONTEXT_SEDIT_CONFIRM_SAVESTRING},
      {mode::products_menu,  CONTEXT_SEDIT_PRODUCTS_MENU     },
      {mode::rooms_menu,     CONTEXT_SEDIT_ROOMS_MENU        },
      {mode::namelist_menu,  CONTEXT_SEDIT_NAMELIST_MENU     },
      {mode::namelist,       CONTEXT_SEDIT_NAMELIST          },
      {mode::keeper,         CONTEXT_SEDIT_KEEPER            },
      {mode::type_menu,      CONTEXT_SEDIT_TYPE_MENU         },
      {mode::delete_type,    CONTEXT_SEDIT_DELETE_TYPE       },
      {mode::delete_product, CONTEXT_SEDIT_DELETE_PRODUCT    },
      {mode::new_product,    CONTEXT_SEDIT_NEW_PRODUCT       },
      {mode::delete_room,    CONTEXT_SEDIT_DELETE_ROOM       },
      {mode::new_room,       CONTEXT_SEDIT_NEW_ROOM          },
      {mode::shop_flags,     CONTEXT_SEDIT_SHOP_FLAGS        },
      {mode::no_trade,       CONTEXT_SEDIT_NOTRADE           },
  };

  /* Removes the entry the builder numbered n, if there is one */
  template <typename T> void erase_at(std::vector<T> &list, int n)
  {
    if (n >= 0 && n < std::ssize(list))
      list.erase(list.begin() + n);
  }

  /* The shop editor: a copy of one shop, put into the world when saved */
  struct shop_editor final : olc_session {
    mode asking = mode::main_menu;
    struct zone_data *zone{};
    shop_vnum number = NOTHING;
    shop_data shop{};
    int new_type = 0; /* the item type of the namelist being added */
    bool changed = false;

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone->number); }

    void edit_new();
    void menu(GameWorld *g, struct descriptor_data *d);
    void done(GameWorld *g, struct descriptor_data *d)
    {
      changed = true;
      menu(g, d);
    }
    void products_menu(GameWorld *g, struct descriptor_data *d);
    void rooms_menu(GameWorld *g, struct descriptor_data *d);
    void compact_rooms_menu(GameWorld *g, struct descriptor_data *d);
    void namelist_menu(GameWorld *g, struct descriptor_data *d);
    void flags_menu(GameWorld *g, struct descriptor_data *d);
    void no_trade_menu(GameWorld *g, struct descriptor_data *d);
    void types_menu(GameWorld *g, struct descriptor_data *d);
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void set(GameWorld *g, struct descriptor_data *d, const field &f, std::string_view line);
    bool takes_numbers() const { return asking >= mode::open1; }
  };

  const field *field_asked(mode asking)
  {
    auto found = std::ranges::find(FIELDS, asking, &field::asks);
    return found == std::end(FIELDS) ? nullptr : found;
  }

  /* A new shop sells nothing, nowhere, and buys nothing, until the builder says otherwise */
  void shop_editor::edit_new()
  {
    shop               = {};
    shop.keeper        = NOBODY;
    shop.close1        = 28;
    shop.profit_buy    = 1.0;
    shop.profit_sell   = 1.0;
    shop.no_such_item1 = shop_default_msgs[SHOP_MSG_NOITEM1];
    shop.no_such_item2 = shop_default_msgs[SHOP_MSG_NOITEM2];
    shop.missing_cash1 = shop_default_msgs[SHOP_MSG_NOCASH1];
    shop.missing_cash2 = shop_default_msgs[SHOP_MSG_NOCASH2];
    shop.do_not_buy    = shop_default_msgs[SHOP_MSG_NOBUY];
    shop.message_buy   = shop_default_msgs[SHOP_MSG_BUY];
    shop.message_sell  = shop_default_msgs[SHOP_MSG_SELL];
    shop_trade_set(&shop, TRADE_NOBROKEN, true);
  }

  void shop_editor::products_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    write_to_output(g, d, "##     VNUM     Product\r\n");
    for (int i = 0; i < std::ssize(shop.producing); i++)
      write_to_output(g, d, "{:>2d} - [@c{:>5d}@n] - @y{}@n\r\n", i, obj_index_of(g, shop.producing[i])->vnum,
                      obj_get_short_description(obj_proto_of(g, shop.producing[i])));
    write_to_output(g, d, "\r\n@gA@n) Add a new product.\r\n@gD@n) Delete a product.\r\n@gQ@n) Quit\r\nEnter choice : ");
    asking = mode::products_menu;
  }

  void shop_editor::compact_rooms_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int i = 0; i < std::ssize(shop.in_room); i++)
      write_to_output(g, d, "{:>2d} - [@c{:>5d}@n]  | {}", i, shop.in_room[i], (i + 1) % 5 ? "" : "\r\n");
    write_to_output(g, d, "\r\n@gA@n) Add a new room.\r\n@gD@n) Delete a room.\r\n@gL@n) Long display.\r\n@gQ@n) Quit\r\nEnter choice : ");
    asking = mode::rooms_menu;
  }

  void shop_editor::rooms_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    write_to_output(g, d, "##     VNUM     Room\r\n\r\n");
    for (int i = 0; i < std::ssize(shop.in_room); i++) {
      if (room_of(g, shop.in_room[i]))
        write_to_output(g, d, "{:>2d} - [@c{:>5d}@n] - @y{}@n\r\n", i, shop.in_room[i], room_get_name(g, shop.in_room[i]));
      else
        write_to_output(g, d, "{:>2d} - [@R!Removed Room!@n]\r\n", i);
    }
    write_to_output(g, d, "\r\n@gA@n) Add a new room.\r\n@gD@n) Delete a room.\r\n@gC@n) Compact Display.\r\n@gQ@n) Quit\r\nEnter choice : ");
    asking = mode::rooms_menu;
  }

  void shop_editor::namelist_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    write_to_output(g, d, "##              Type   Namelist\r\n\r\n");
    for (int i = 0; i < std::ssize(shop.type); i++)
      write_to_output(g, d, "{:>2d} - @c{:>15}@n - @y{}@n\r\n", i, item_types[shop.type[i].type],
                      shop.type[i].keywords.empty() ? "<None>" : shop.type[i].keywords);
    write_to_output(g, d, "\r\n@gA@n) Add a new entry.\r\n@gD@n) Delete an entry.\r\n@gQ@n) Quit\r\nEnter choice : ");
    asking = mode::namelist_menu;
  }

  void shop_editor::flags_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_SHOP_FLAGS; i++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}   {}", i + 1, shop_bits[i], (i + 1) % 2 ? "" : "\r\n");
    write_to_output(g, d, "\r\nCurrent Shop Flags : @c{}@n\r\nEnter choice : ", sprintbit(shop.bitvector, shop_bits));
    asking = mode::shop_flags;
  }

  void shop_editor::no_trade_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_TRADERS; i++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}   {}", i + 1, trade_letters[i], (i + 1) % 2 ? "" : "\r\n");
    write_to_output(g, d, "\r\nCurrently won't trade with: @c{}@n\r\nEnter choice : ", sprintbitarray(shop.with_who, trade_letters, 4));
    asking = mode::no_trade;
  }

  void shop_editor::types_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_ITEM_TYPES; i++)
      write_to_output(g, d, "@g{:>2d}@n) @c{:<20}@n  {}", i, item_types[i], (i + 1) % 3 ? "" : "\r\n");
    write_to_output(g, d, "@nEnter choice : ");
    asking = mode::type_menu;
  }

  void shop_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    const bool keeper = shop.keeper != NOBODY;
    write_to_output(g, d,
                    "-- Shop Number : [@c{:d}@n]\r\n@g0@n) Keeper      : [@c{:d}@n] @y{}\r\n@g1@n) Open 1      : @c{:>4d}@n          @g2@n) Close 1     : @c{:>4d}\r\n@g3@n) Open 2      : @c{:>4d}@n          @g4@n) Close 2     : @c{:>4d}\r\n@g5@n) Sell rate   : @c{:>1.2f}@n          @g6@n) Buy rate    : @c{:>1.2f}\r\n@g7@n) Keeper no item : @y{}\r\n@g8@n) Player no item : @y{}\r\n@g9@n) Keeper no cash : @y{}\r\n@gA@n) Player no cash : @y{}\r\n@gB@n) Keeper no buy  : @y{}\r\n@gC@n) Buy success    : @y{}\r\n@gD@n) Sell success   : @y{}\r\n@gE@n) No Trade With  : @c{}\r\n@gF@n) Shop flags     : @c{}\r\n@gR@n) Rooms Menu\r\n@gP@n) Products Menu\r\n@gT@n) Accept Types Menu\r\n@gW@n) Copy Shop\r\n@gQ@n) Quit\r\nEnter Choice : ",
                    number, keeper ? mob_index_of(g, shop.keeper)->vnum : -1, keeper ? char_get_sdesc(mob_proto_of(g, shop.keeper)) : "None",
                    shop.open1, shop.close1, shop.open2, shop.close2, shop.profit_buy, shop.profit_sell, shop.no_such_item1,
                    shop.no_such_item2, shop.missing_cash1, shop.missing_cash2, shop.do_not_buy, shop.message_buy, shop.message_sell,
                    sprintbitarray(shop.with_who, trade_letters, 4), sprintbit(shop.bitvector, shop_bits));
    asking = mode::main_menu;
  }

  void shop_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = tolower(first_char(line));
    if (const field *f = std::ranges::find(FIELDS, key, &field::key); f != std::end(FIELDS)) {
      asking = f->asks;
      write_to_output(g, d, std::holds_alternative<std::string shop_data::*>(f->where) ? "\r\nEnter new text :\r\n] " : "\r\nEnter new value : ");
      return;
    }
    switch (key) {
      case 'q':
        if (changed) {
          write_to_output(g, d, "Do you wish to save your changes? : ");
          asking = mode::confirm_save;
        } else
          leave_olc(g, d);
        return;
      case '0':
        asking = mode::keeper;
        write_to_output(g, d, "Enter vnum number of shop keeper : ");
        return;
      case 'e': no_trade_menu(g, d); return;
      case 'f': flags_menu(g, d); return;
      case 'r': rooms_menu(g, d); return;
      case 'p': products_menu(g, d); return;
      case 't': namelist_menu(g, d); return;
      case 'w':
        write_to_output(g, d, "Copy what shop? ");
        asking = mode::copy;
        return;
      default: menu(g, d); return;
    }
  }

  void shop_editor::save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    std::optional<bool> save = yes_or_no(line);
    if (!save) {
      write_to_output(g, d, "Invalid choice!\r\nDo you wish to save your changes? : ");
      return;
    }
    if (*save) {
      shop.vnum = number;
      add_shop(g, &shop);
      mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} edits shop {:d}",
             char_get_name(d->character), number);
      if (CONFIG_OLC_SAVE) {
        save_shops(g, find_zone_by_thing(g, number));
        write_to_output(g, d, "Shop saved to disk.\r\n");
      } else
        write_to_output(g, d, "Shop saved to memory.\r\n");
    }
    leave_olc(g, d);
  }

  /* Stores the answer to one of the main menu's fields */
  void shop_editor::set(GameWorld *g, struct descriptor_data *d, const field &f, std::string_view line)
  {
    std::visit(
        [&](auto where) {
          using value = std::remove_reference_t<decltype(shop.*where)>;
          if constexpr (std::is_same_v<value, int>)
            shop.*where = LIMIT(parse_int(line), 0, 28);
          else if constexpr (std::is_same_v<value, float>)
            scan_float(line, shop.*where);
          else {
            std::string text(line);
            smash_tilde(text);
            set_npc_message(g, d, shop.*where, text, f.allow_int);
          }
        },
        f.where);
  }

  void shop_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    if (takes_numbers() && minus_without_digit(line)) {
      write_to_output(g, d, "Field must be numerical, try again : ");
      return;
    }
    const int n = parse_int(line);

    switch (asking) {
      case mode::confirm_save: save_answer(g, d, line); return;
      case mode::main_menu: main_menu_choice(g, d, line); return;

      case mode::namelist_menu:
        switch (tolower(first_char(line))) {
          case 'a': types_menu(g, d); return;
          case 'd':
            write_to_output(g, d, "\r\nDelete which entry? : ");
            asking = mode::delete_type;
            return;
        }
        break;
      case mode::products_menu:
        switch (tolower(first_char(line))) {
          case 'a':
            write_to_output(g, d, "\r\nEnter new product vnum number : ");
            asking = mode::new_product;
            return;
          case 'd':
            write_to_output(g, d, "\r\nDelete which product? : ");
            asking = mode::delete_product;
            return;
        }
        break;
      case mode::rooms_menu:
        switch (tolower(first_char(line))) {
          case 'a':
            write_to_output(g, d, "\r\nEnter new room vnum number : ");
            asking = mode::new_room;
            return;
          case 'c': compact_rooms_menu(g, d); return;
          case 'l': rooms_menu(g, d); return;
          case 'd':
            write_to_output(g, d, "\r\nDelete which room? : ");
            asking = mode::delete_room;
            return;
        }
        break;

      case mode::namelist: {
        std::string keywords(line);
        smash_tilde(keywords);
        shop.type.push_back({.type = new_type, .keywords = keywords});
        namelist_menu(g, d);
        return;
      }
      case mode::keeper:
        if (n != NOBODY && !mob_proto_of(g, n)) {
          write_to_output(g, d, "That mobile does not exist, try again : ");
          return;
        }
        shop.keeper = n;
        if (n != NOBODY) { /* the keeper's own special goes behind the shop's */
          auto *index = mob_index_of(g, n);
          shop.func   = index->func != shop_keeper ? index->func : nullptr;
          index->func = shop_keeper;
        }
        break;
      case mode::type_menu:
        new_type = LIMIT(n, 0, NUM_ITEM_TYPES - 1);
        write_to_output(g, d, "Enter namelist (return for none) :-\r\n] ");
        asking = mode::namelist;
        return;
      case mode::delete_type:
        erase_at(shop.type, n);
        namelist_menu(g, d);
        return;
      case mode::new_product:
        if (n != NOTHING && !obj_proto_of(g, n)) {
          write_to_output(g, d, "That object does not exist, try again : ");
          return;
        }
        if (n > 0)
          shop.producing.push_back(n);
        products_menu(g, d);
        return;
      case mode::delete_product:
        erase_at(shop.producing, n);
        products_menu(g, d);
        return;
      case mode::new_room:
        if (n != NOWHERE && !room_of(g, n)) {
          write_to_output(g, d, "That room does not exist, try again : ");
          return;
        }
        if (n >= 0)
          shop.in_room.push_back(n);
        rooms_menu(g, d);
        return;
      case mode::delete_room:
        erase_at(shop.in_room, n);
        rooms_menu(g, d);
        return;
      case mode::shop_flags:
        if (int flag = LIMIT(n, 0, NUM_SHOP_FLAGS); flag > 0) {
          shop.bitvector ^= 1U << (flag - 1);
          flags_menu(g, d);
          return;
        }
        break;
      case mode::no_trade:
        if (int trader = LIMIT(n, 0, NUM_TRADERS); trader > 0) {
          shop_trade_toggle(&shop, trader - 1);
          no_trade_menu(g, d);
          return;
        }
        break;
      case mode::copy:
        if (struct shop_data *source = find_shop(g, n))
          shop = *source;
        else
          write_to_output(g, d, "That shop does not exist.\r\n");
        break;

      default:
        if (const field *f = field_asked(asking))
          set(g, d, *f, line);
        break;
    }
    done(g, d);
  }

  int shop_editor::help_context(const struct descriptor_data *) const
  {
    if (const field *f = field_asked(asking))
      return f->context;
    auto found = std::ranges::find(CONTEXTS, asking, &std::pair<mode, int>::first);
    return found == std::end(CONTEXTS) ? NOTHING : found->second;
  }
} // namespace

ACMD(do_oasis_sedit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);
  std::string buf2      = take_arg(rest);

  if (buf1.empty()) {
    send_to_char(g, ch, "Specify a shop VNUM to edit.\r\n");
    return;
  }

  IDXTYPE number = NOWHERE;
  bool save      = false;
  if (!isdigit(buf1[0])) {
    if (!str_eq_nocase("save", buf1)) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = true;
    if ((number = zone_to_save(g, ch, buf2)) == NOWHERE)
      return;
  } else
    number = parse_int(buf1);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_SEDIT && dd->olc && session_of<shop_editor>(dd).number == number) {
      send_to_char(g, ch, "That shop is currently being edited by {}.\r\n", char_pers(g, dd->character, ch));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<shop_editor>(), "do_oasis_sedit");
  shop_editor &editor = session_of<shop_editor>(d);

  if (!(editor.zone = zone_to_edit(g, ch, number, save))) {
    d->olc.reset();
    return;
  }
  if (save) {
    save_zone_files(g, ch, editor.zone, "shops", "shop info", save_shops);
    d->olc.reset();
    return;
  }

  editor.number = number;
  if (struct shop_data *existing = find_shop(g, number))
    editor.shop = *existing;
  else
    editor.edit_new();

  editor.menu(g, d);
  begin_editing(g, d, CON_SEDIT);
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}
