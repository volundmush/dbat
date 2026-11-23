/*
 * The shop files, world/shp/<zone>.shp. Each record is a "~" string; a shop starts with "#<vnum>~", and only after the
 * header string with "v3.0" in it, which says the lists end in -1 (the older lists of fixed length are no longer read).
 *
 *   CircleMUD v3.0 Shop File~
 *   #<vnum>~
 *   <object vnum>                                    what it makes, any number of them, then
 *   -1
 *   <buy profit>
 *   <sell profit>
 *   <item type>[<keywords>][;<comment>]              what it buys, as a number or a type's name, then
 *   -1
 *   <7 messages, each ending in ~>
 *   <temper>
 *   <flags>
 *   <keeper's mob vnum>
 *   <with whom it won't trade, up to 4 numbers>
 *   <room vnum>                                      where it is, any number of them, then
 *   -1
 *   <open> <close> <open> <close>                    one to a line
 *   ...
 *   $~
 */
#include <cstdint>
#include <memory>
#include <optional>

#include "dbat/world_files.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/shop.hpp"
#include "dbat/utils.hpp"

namespace {

/* A line read as sscanf("%d") reads it: an error if there is none, or it doesn't start with a number. quirk: a line of
   only spaces reads as nothing, leaving n as it was. */
io::loaded<void> read_number(io::text_reader &in, shop_vnum vnum, int &n, const char *what)
{
  std::optional<std::string_view> line = in.line();
  if (!line || io::scan_ints(*line, std::span(&n, 1)) == 0)
    return std::unexpected(in.fail(str_format("shop #{:d}: expected {}, got '{}'", vnum, what, line.value_or(""))));
  return {};
}

io::loaded<void> read_real(io::text_reader &in, shop_vnum vnum, float &x, const char *what)
{
  std::optional<std::string_view> line = in.line();
  if (!line || io::scan_float(*line, x) == 0)
    return std::unexpected(in.fail(str_format("shop #{:d}: expected {}, got '{}'", vnum, what, line.value_or(""))));
  return {};
}

/* Adds an entry to one of a shop's lists, as the lists have always taken them: a negative one is passed over, a product
   with no prototype is dropped, and an item type there isn't is logged and dropped. true if the list was full. */
bool add_to_list(GameWorld *g, shop_vnum vnum, std::vector<shop_buy_data> &list, int type, int &val)
{
  if (val == NOTHING || val < 0)
    return false;
  if (type == LIST_TRADE && val >= NUM_ITEM_TYPES) {
    mud_log("SYSERR: Shop #{:d} buys item type {:d}, which doesn't exist; ignoring it.", vnum, val);
    return false;
  }
  if (list.size() >= MAX_SHOP_OBJ)
    return true;
  if (type == LIST_PRODUCE && !obj_proto_of(g, val))
    val = NOTHING;
  if (val != NOTHING)
    list.push_back({.type = val});
  return false;
}

void log_full_list(const std::vector<shop_buy_data> &list, int dropped)
{
  /* the count includes the -1 that ended the old lists */
  if (dropped)
    mud_log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.hpp to {:d}", static_cast<int>(list.size()) + 1 + dropped);
}

/* A list of numbers, one to a line, up to a negative one */
io::loaded<std::vector<shop_buy_data>> read_list(GameWorld *g, io::text_reader &in, shop_vnum vnum, int type,
                                                 const char *what)
{
  std::vector<shop_buy_data> list;
  int dropped = 0;
  int n       = -1; /* quirk: a line of spaces reads as the number before it */
  for (;;) {
    if (io::loaded<void> read = read_number(in, vnum, n, what); !read)
      return std::unexpected(read.error());
    if (n < 0)
      break;
    dropped += add_to_list(g, vnum, list, type, n);
  }
  log_full_list(list, dropped);
  return list;
}

/* What a shop buys: "<type>[<keywords>]", the type a number or the start of an item type's name, each on its own raw
   line (quirk: comments and blank lines aren't passed over here), with anything after a ';' left out; up to a negative
   type, or the end of the file */
std::vector<shop_buy_data> read_buy_types(GameWorld *g, io::text_reader &in, shop_vnum vnum)
{
  std::vector<shop_buy_data> list;
  int dropped = 0;
  int num     = -1;
  do {
    std::optional<std::string_view> raw = in.raw_line();
    if (!raw)
      break;
    std::string buf(raw->substr(0, raw->find('\0')));
    if (size_t semicolon = buf.find(';'); semicolon != std::string::npos)
      buf.resize(semicolon);
    else if (!buf.empty())
      buf.pop_back(); /* the line end; quirk: on a last line with none, its last character */

    num = -1;
    if (!buf.starts_with("-1"))
      for (int i = 0; *item_types[i] != '\n'; i++) {
        std::string_view name = item_types[i];
        if (buf.size() >= name.size() && str_eq_nocase(std::string_view(buf).substr(0, name.size()), name)) {
          num = i;
          buf.erase(0, name.size());
          break;
        }
      }

    size_t at = 0;
    if (num == -1) {
      io::scan_ints(buf, std::span(&num, 1));
      while (at < buf.size() && !isdigit(static_cast<unsigned char>(buf[at])))
        at++;
      while (at < buf.size() && isdigit(static_cast<unsigned char>(buf[at])))
        at++;
    }
    while (at < buf.size() && isspace(static_cast<unsigned char>(buf[at])))
      at++;
    std::string keywords = buf.substr(at);
    while (!keywords.empty() && isspace(static_cast<unsigned char>(keywords.back())))
      keywords.pop_back();

    size_t before = list.size();
    dropped += add_to_list(g, vnum, list, LIST_TRADE, num);
    if (!keywords.empty() && list.size() > before)
      list.back().keywords = std::move(keywords);
  } while (num >= 0);
  log_full_list(list, dropped);
  return list;
}

/* A message, or its default if it is empty or would be an unsafe format for npc_message() */
io::loaded<std::string> read_message(io::text_reader &in, shop_vnum vnum, shop_message which)
{
  io::loaded<std::string> text = in.tilde_string(str_format("shop #{:d}", vnum));
  if (!text)
    return text;
  if (text->empty())
    return std::string(shop_default_msgs[which]);
  if (std::optional<std::string> problem =
          npc_msg_format_problem(text->c_str(), which == SHOP_MSG_BUY || which == SHOP_MSG_SELL)) {
    mud_log("SYSERR: Shop #{:d} message #{:d}: {}; using the default.", vnum, static_cast<int>(which), *problem);
    return std::string(shop_default_msgs[which]);
  }
  return text;
}

io::loaded<void> read_shop(GameWorld *g, io::text_reader &in, shop_vnum vnum)
{
  std::string context = str_format("shop #{:d} in shop file {}", vnum, in.name());
  auto made           = std::make_unique<shop_data>();
  shop_data *shop     = made.get();
  shop_set_vnum(shop, vnum);

  io::loaded<std::vector<shop_buy_data>> products = read_list(g, in, vnum, LIST_PRODUCE, "an object it makes");
  if (!products)
    return std::unexpected(products.error());
  for (const shop_buy_data &product : *products)
    shop->producing.push_back(product.type);

  if (io::loaded<void> r = read_real(in, vnum, shop->profit_buy, "its buy profit"); !r)
    return r;
  if (io::loaded<void> r = read_real(in, vnum, shop->profit_sell, "its sell profit"); !r)
    return r;
  shop->type = read_buy_types(g, in, vnum);

  std::string *messages[] = {&shop->no_such_item1, &shop->no_such_item2, &shop->do_not_buy, &shop->missing_cash1,
                             &shop->missing_cash2, &shop->message_buy,   &shop->message_sell};
  for (int i = 0; i < NUM_SHOP_MSGS; i++) {
    io::loaded<std::string> message = read_message(in, vnum, static_cast<shop_message>(i));
    if (!message)
      return std::unexpected(message.error());
    *messages[i] = std::move(*message);
  }

  if (io::loaded<void> r = read_number(in, vnum, shop->temper1, "its temper"); !r)
    return r;
  /* quirk: the flags were read with "%ld" into 4 bytes, and the keeper with "%hd" into the 2 bytes after them: on the
     builds the game has run on, the flags' long spilled its high 4 bytes into the keeper, whose low 2 bytes were
     then the keeper read. A negative flags word thus made the keeper a vnum no mob has. */
  int read_check   = 0;
  uint32_t keeper  = 0;
  std::optional<std::string_view> line = in.line();
  int scanned      = line ? io::scan_ints(*line, std::span(&read_check, 1)) : 0;
  if (scanned == 0)
    return std::unexpected(in.fail(str_format("shop #{:d}: expected its flags, got '{}'", vnum, line.value_or(""))));
  if (scanned == 1) {
    long flags      = io::c_atol(*line);
    shop->bitvector = static_cast<bitvector_t>(flags);
    keeper          = static_cast<uint32_t>(static_cast<unsigned long long>(flags) >> 32);
  }
  line    = in.line();
  scanned = line ? io::scan_ints(*line, std::span(&read_check, 1)) : 0;
  if (scanned == 0)
    return std::unexpected(
        in.fail(str_format("shop #{:d}: expected its keeper's vnum, got '{}'", vnum, line.value_or(""))));
  if (scanned == 1)
    keeper = (keeper & 0xFFFF0000u) | static_cast<uint16_t>(io::c_atol(*line));
  shop->keeper = static_cast<mob_vnum>(keeper);
  if (!mob_proto_of(g, shop_get_keeper(shop)))
    shop_set_keeper(shop, NOBODY);

  /* with whom it won't trade: numbers, with anything but digits and '-' between them */
  std::string with_who(in.line().value_or(""));
  int slot = 0;
  for (size_t at = 0; slot < SW_ARRAY_MAX && at < with_who.size(); slot++) {
    int n = 0;
    if (io::scan_ints(std::string_view(with_who).substr(at), std::span(&n, 1)) != 1) {
      mud_log("SYSERR: Can't parse TRADE_WITH line in {}: '{}'", context, with_who);
      break;
    }
    shop_get_with_who(shop)[slot] = static_cast<bitvector_t>(n);
    while (at < with_who.size() && (isdigit(static_cast<unsigned char>(with_who[at])) || with_who[at] == '-'))
      at++;
    while (at < with_who.size() && !(isdigit(static_cast<unsigned char>(with_who[at])) || with_who[at] == '-'))
      at++;
  }
  for (; slot < SW_ARRAY_MAX; slot++)
    shop_get_with_who(shop)[slot] = 0;

  io::loaded<std::vector<shop_buy_data>> rooms = read_list(g, in, vnum, LIST_ROOM, "a room it is in");
  if (!rooms)
    return std::unexpected(rooms.error());
  for (const shop_buy_data &room : *rooms)
    shop->in_room.push_back(room.type);

  if (io::loaded<void> r = read_number(in, vnum, shop->open1, "the hour it first opens"); !r)
    return r;
  if (io::loaded<void> r = read_number(in, vnum, shop->close1, "the hour it first closes"); !r)
    return r;
  if (io::loaded<void> r = read_number(in, vnum, shop->open2, "the hour it opens again"); !r)
    return r;
  if (io::loaded<void> r = read_number(in, vnum, shop->close2, "the hour it closes again"); !r)
    return r;

  shop_set_bank(shop, 0);
  shop_set_lastsort(shop, 0);
  shop_set_func(shop, nullptr);
  if (g->shops.find(vnum))
    mud_log("SYSERR: Shop #{:d} is defined twice; ignoring the second one.", vnum);
  else
    g->shops.insert(vnum, std::move(made));
  return {};
}

} // namespace

io::loaded<void> read_shop_file(GameWorld *g, io::text_reader &in)
{
  std::string context = str_format("beginning of shop file {}", in.name());
  bool version3       = false;
  for (;;) {
    io::loaded<std::string> record = in.tilde_string(context);
    if (!record)
      return std::unexpected(record.error());
    if (record->empty()) { /* nothing to read, and no end of the file either */
      mud_log("SYSERR: Empty record in {}; skipping it.", context);
      continue;
    }
    if ((*record)[0] == '$')
      return {};
    if ((*record)[0] != '#') {
      if (record->find(VERSION3_TAG) != std::string::npos)
        version3 = true;
      continue; /* the header, or anything else there that isn't a shop */
    }

    int vnum = 0;
    if (!io::fields(std::string_view(*record).substr(1)).number(vnum))
      return std::unexpected(in.fail(str_format("expected a number after the '#', got '{}'", *record)));
    if (!version3)
      return std::unexpected(in.fail(str_format(
          "shop #{:d} comes before the \"" VERSION3_TAG "\" header; shops older than v3.0 aren't read any more", vnum)));
    context = str_format("shop #{:d} in shop file {}", vnum, in.name());
    if (io::loaded<void> shop = read_shop(g, in, vnum); !shop)
      return shop;
  }
}

bool save_shops(GameWorld *g, struct zone_data *zone)
{
  if (!zone) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  io::text_writer out(str_format("{}{:d}.shp", SHP_PREFIX, zone->number));
  out.write("CircleMUD v3.0 Shop File~\n");
  for (shop_vnum vnum : g->shops.vnums_in(genolc_zone_bottom(g, zone), zone->top)) {
    shop_data *shop = find_shop(g, vnum);
    if (!shop)
      continue;
    out.print("#{:d}~\n", vnum);
    for (obj_vnum product : shop_products(shop))
      out.print("{:d}\n", product);
    out.print("-1\n{:>1.2f}\n{:>1.2f}\n", shop_get_profit_buy(shop), shop_get_profit_sell(shop));
    for (const shop_buy_data &buy : shop_buy_types(shop))
      out.print("{:d}{}\n", buy.type, buy.keywords);
    out.write("-1\n");

    out.print("{}~\n{}~\n{}~\n{}~\n{}~\n{}~\n{}~\n{:d}\n{:d}\n{:d}\n",
              or_if_empty(shop_get_no_such_item1(shop), "%s Ke?!"), or_if_empty(shop_get_no_such_item2(shop), "%s Ke?!"),
              or_if_empty(shop_get_do_not_buy(shop), "%s Ke?!"), or_if_empty(shop_get_missing_cash1(shop), "%s Ke?!"),
              or_if_empty(shop_get_missing_cash2(shop), "%s Ke?!"),
              or_if_empty(shop_get_message_buy(shop), "%s Ke?! %d?"),
              or_if_empty(shop_get_message_sell(shop), "%s Ke?! %d?"), shop_get_temper(shop),
              static_cast<int>(shop_get_bitvector(shop)), /* as signed, as always */
              shop_get_keeper(shop) == NOBODY ? -1 : mob_index_of(g, shop_get_keeper(shop))->vnum);
    for (int i = 0; i < SW_ARRAY_MAX; i++)
      out.print("{}{:d}", i ? " " : "", static_cast<int>(shop_get_with_who(shop)[i]));
    out.write("\n");
    for (room_vnum room : shop_rooms(shop))
      out.print("{:d}\n", room);
    out.print("-1\n{:d}\n{:d}\n{:d}\n{:d}\n", shop_get_open1(shop), shop_get_close1(shop), shop_get_open2(shop),
              shop_get_close2(shop));
  }
  out.write("$~\n");
  if (!out.commit()) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot open shop file!");
    return false;
  }

  if (in_save_list(g, zone->number, SL_SHP)) {
    remove_from_save_list(g, zone->number, SL_SHP);
    create_world_index(g, zone->number, "shp");
    mud_log("GenOLC: save_shops: Saving shops '{}'", out.path());
  }
  return true;
}
