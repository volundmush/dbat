/*
 * The banned sites, etc/badsites: "<no|new|select|all> <site> <date> <who banned it>", any number of them, in the
 * order they were banned. And the invalid names, misc/xnames: a name, or part of one, a line.
 */
#include <array>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>

#include "dbat/ban.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

namespace {

static constexpr auto ban_types = std::to_array<std::string_view>({"no", "new", "select", "all", "ERROR"});

void write_bans(io::text_writer &out, const ban_list_element *node)
{
  if (!node)
    return;
  write_bans(out, node->next); /* the list is newest first; the file, oldest */
  out.print("{} {} {:d} {}\n", ban_types[node->type], node->site, static_cast<long>(node->date), node->name);
}

} // namespace

void load_banned(GameWorld *g)
{
  g->ban_list = nullptr;

  io::loaded<io::text_reader> file = io::text_reader::open(BAN_FILE);
  if (!file) {
    std::error_code ec;
    if (std::filesystem::exists(BAN_FILE, ec))
      mud_log("SYSERR: Unable to open banfile '{}': {}", BAN_FILE, io::why_unreadable(BAN_FILE));
    else
      mud_log("   Ban file '{}' doesn't exist.", BAN_FILE);
    return;
  }

  /* the whole file is one run of words, read four at a time until four can't be (quirk: line ends don't count) */
  io::fields in(file->text());
  for (;;) {
    std::string type, site, name;
    int date = 0;
    if (!(in.word(type, 99) && in.word(site, BANNED_SITE_LENGTH) && in.number(date) && in.word(name, 20)))
      break;
    auto *node = new ban_list_element();
    node->site = std::move(site);
    node->name = std::move(name);
    node->date = date;
    for (int i = BAN_NOT; i <= BAN_ALL; i++)
      if (type == ban_types[i])
        node->type = i;
    node->next  = g->ban_list;
    g->ban_list = node;
  }
}

void save_ban_list(GameWorld *g)
{
  io::text_writer out(BAN_FILE);
  write_bans(out, g->ban_list);
  out.commit();
}

io::loaded<void> Read_Invalid_List(GameWorld *g)
{
  io::loaded<io::text_reader> file = io::text_reader::open(XNAME_FILE);
  if (!file) {
    mud_log("SYSERR: Unable to open '{}' for reading", XNAME_FILE);
    return {};
  }

  g->invalid_list.clear();
  while (std::optional<std::string_view> line = file->line()) {
    if (g->invalid_list.size() >= MAX_INVALID_NAMES)
      return std::unexpected(file->fail("too many invalid names; change MAX_INVALID_NAMES in ban.hpp"));
    g->invalid_list.emplace_back(*line);
  }
  if (g->invalid_list.size() >= MAX_INVALID_NAMES) /* quirk: a full list is too many, as the old reader found */
    return std::unexpected(file->fail("too many invalid names; change MAX_INVALID_NAMES in ban.hpp"));
  return {};
}
