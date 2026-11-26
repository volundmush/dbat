/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ranges>

#include "dbat/io/reader.hpp"
#include "dbat/genolc.hpp"
#include "dbat/gengld.hpp"
#include "dbat/cedit.hpp"
#include "dbat/genmob.hpp"
#include "dbat/genobj.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genwld.hpp"
#include "dbat/modify.hpp"
#include "dbat/genzon.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/game_world.hpp"
#include "dbat/olc/commands.hpp"

/* Structure defining all known save types.  */
namespace {
struct save_types_entry {
  int save_type;
  bool (*func)(GameWorld *g, struct zone_data *zone);
  std::string_view message{};
};
}
static auto save_types = std::to_array<save_types_entry>({
    {SL_MOB, save_mobiles, "mobile"},
    {SL_OBJ, save_objects, "object"},
    {SL_SHP, save_shops,   "shop"  },
    {SL_WLD, save_rooms,   "room"  },
    {SL_ZON, save_zone,    "zone"  },
    {SL_CFG, save_config,  "config"},
    {SL_GLD, save_guilds,  "guild" },
    {SL_ACT, nullptr,      "social"},
    {SL_HLP, nullptr,      "help"  },
    {-1,     nullptr,      {}      },
});

std::optional<std::string> npc_msg_format_problem(std::string_view msg, bool allow_int)
{
  int ss = 0, ds = 0;

  for (size_t i = 0; i < msg.size(); i++) {
    if (msg[i] != '%')
      continue;
    if (++i == msg.size())
      return "a '%' at the very end";
    if (msg[i] == 's')
      ss++;
    else if (msg[i] == 'd' && allow_int) {
      if (ss == 0)
        return "%d before %s";
      ds++;
    } else if (msg[i] != '%') {
      return str_format("invalid format '%{:c}'", msg[i]);
    }
  }

  if (ss > 1)
    return "more than one %s";
  if (ds > 1)
    return "more than one %d";
  return std::nullopt;
}

std::string npc_message(std::string_view msg, std::string_view name, int amount)
{
  std::string out;

  for (size_t i = 0; i < msg.size(); i++) {
    if (msg[i] != '%' || i + 1 == msg.size()) {
      out += msg[i];
      continue;
    }
    switch (msg[++i]) {
      case 's':
        out += name;
        break;
      case 'd':
        out += std::to_string(amount);
        break;
      default: /* %%, and anything else the check let by, as itself */
        out += msg[i];
        break;
    }
  }
  return out;
}

/* A numbered menu of names, `columns` to a line: "@g%2d@n) %-20.20s" then gap after each name.
   Numbering starts at first_number. */
void olc_list_columns(GameWorld *g, struct descriptor_data *d, std::span<const std::string_view> names, int count, int first_number, std::string_view gap, int columns)
{
  for (int i = 0; i < count; i++) /* quirk: one past the end, as some menus ask, is the "\n" that once ended names */
    write_to_output(g, d, "@g{:>2d}@n) {:<20.20}{}{}", i + first_number, static_cast<size_t>(i) < names.size() ? names[i] : "\n", gap,
                    !((i + 1) % columns) ? "\r\n" : "");
}

std::string text_or_undefined(std::string_view text) { return text.empty() ? "undefined" : std::string(text); }


void scan_float(std::string_view text, float &field) { io::scan_float(text, field); }

/* Writes out everything listed as unsaved. Original use: to be called at shutdown time. */
bool save_all(GameWorld *g)
{
  /* each save takes its zone and kind off the list, newest first */
  while (!g->save_list.empty()) {
    const save_list_data item = g->save_list.back();
    if (item.type < 0 || item.type > SL_MAX) {
      if (item.type == SL_ACT)
        mud_log("Actions not saved - can not autosave. Use 'aedit save'.");
      else if (item.type == SL_HLP)
        mud_log("Help not saved - can not autosave. Use 'hedit save'.");
      else
        mud_log("SYSERR: GenOLC: Invalid save type {:d} in save list.\n", item.type);
      g->save_list.pop_back(); /* it can't be saved from here */
    } else if (!save_types[item.type].func(g, find_zone(g, item.zone)))
      g->save_list.pop_back(); /* Fatal error, skip this one. */
  }

  return true;
}

/* Takes out every carriage return. */
void strip_cr(std::string &text)
{
  std::erase(text, '\r');
}

namespace {
  auto listed(GameWorld *g, zone_vnum zone, int type)
  {
    return std::ranges::find_if(g->save_list, [&](const save_list_data &item) { return item.zone == zone && item.type == type; });
  }
} // namespace

bool remove_from_save_list(GameWorld *g, zone_vnum zone, int type)
{
  auto item = listed(g, zone, type);
  if (item == g->save_list.end()) {
    mud_log("SYSERR: remove_from_save_list: Saved item not found. ({:d}/{:d})", zone, type);
    return false;
  }
  g->save_list.erase(item);
  return true;
}

bool add_to_save_list(GameWorld *g, zone_vnum zone, int type)
{
  if (type == SL_CFG)
    return false;

  if (!find_zone(g, zone) && zone != AEDIT_PERMISSION && zone != HEDIT_PERMISSION) {
    mud_log("SYSERR: add_to_save_list: Invalid zone number passed. ({:d})", zone);
    return false;
  }
  if (in_save_list(g, zone, type))
    return false;
  g->save_list.push_back({.zone = zone, .type = type});
  return true;
}

bool in_save_list(GameWorld *g, zone_vnum zone, int type) { return listed(g, zone, type) != g->save_list.end(); }

/* Used from do_show(), ideally.  */
ACMD(do_show_save_list)
{
  if (g->save_list.empty()) {
    send_to_char(g, ch, "All world files are up to date.\r\n");
    return;
  }
  send_to_char(g, ch, "The following files need saving:\r\n");
  for (const save_list_data &item : g->save_list | std::views::reverse) /* newest first */
    if (item.type != SL_CFG)
      send_to_char(g, ch, " - {} data for zone {:d}.\r\n", save_types[item.type].message, item.zone);
    else
      send_to_char(g, ch, " - Game configuration data.\r\n");
}

room_vnum genolc_zonep_bottom(struct zone_data *zone)
{
  return zone->bot;
}

zone_vnum genolc_zone_bottom(GameWorld *g, struct zone_data *rznum)
{
  return rznum->bot;
}

std::string flag_letters(bitvector_t bits)
{
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  static constexpr std::string_view LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEF";
  std::string out;
  for (size_t i = 0; i < LETTERS.size(); i++)
    if (bits & (1U << i))
      out += LETTERS[i];
  return out.empty() ? "0" : out;
}
