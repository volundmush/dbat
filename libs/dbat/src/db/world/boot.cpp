/*
 * Booting each kind of world file from its index: see dbat/world_files.hpp.
 *
 * A kind's index (world/<kind>/index, or index.mini for a mini-mud) lists its files in the order they load. They are
 * all read first, so that the count of records, logged with how much memory they take, comes before anything they
 * hold is loaded; then each is loaded by its kind's reader.
 */
#include <algorithm>
#include <cstdio>
#include <optional>
#include <vector>

#include "dbat/world_files.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/guild.hpp"
#include "dbat/help.hpp"
#include "dbat/log.hpp"
#include "dbat/shop.hpp"

#include "../legacy_loaders.hpp"

namespace {

/* The records of a file, as the old count found them: lines starting with '#', read in pieces of up to 127
   characters as fgets() into a 128-byte buffer read them (quirk: a long line's piece that starts with '#' counts) */
int count_hash_records(const io::text_reader &in)
{
  std::string_view text = in.text();
  int count             = 0;
  for (size_t at = 0; at < text.size();) {
    if (text[at] == '#')
      count++;
    size_t line_end = text.find('\n', at);
    line_end        = line_end == std::string_view::npos ? text.size() : line_end + 1;
    at              = std::min(line_end, at + 127);
  }
  return count;
}

int count_one(const io::text_reader &) { return 1; }

/* Help counts its keywords, which are what its table holds */
int count_help_keywords(const io::text_reader &in)
{
  std::FILE *fl = std::fopen(in.name().c_str(), "r");
  if (!fl)
    return 0;
  int count = count_alias_records(fl);
  std::fclose(fl);
  return count;
}

void report_rooms(GameWorld *, int count)
{
  mud_log("   {:d} rooms, {:d} bytes.", count, static_cast<int>(sizeof(room_data)) * count);
}

void report_mobs(GameWorld *, int count)
{
  mud_log("   {:d} mobs, {:d} bytes in index, {:d} bytes in prototypes.", count,
          static_cast<int>(sizeof(index_data)) * count, static_cast<int>(sizeof(char_data)) * count);
}

void report_objects(GameWorld *, int count)
{
  mud_log("   {:d} objs, {:d} bytes in index, {:d} bytes in prototypes.", count,
          static_cast<int>(sizeof(index_data)) * count, static_cast<int>(sizeof(obj_data)) * count);
}

void report_zones(GameWorld *, int count)
{
  mud_log("   {:d} zones, {:d} bytes.", count, static_cast<int>(sizeof(zone_data)) * count);
}

void report_help(GameWorld *g, int count)
{
  g->help_table.reserve(count);
  mud_log("   {:d} entries, {:d} bytes.", count, static_cast<int>(sizeof(help_index_element)) * count);
}

/* stable, as glibc's qsort() was: an entry's keywords stay in the order the file gave them */
void sort_help(GameWorld *g)
{
  std::ranges::stable_sort(g->help_table, help_keyword_order);
}

/* The kinds still read through a FILE, until they move onto io::text_reader */
template <void (*legacy)(GameWorld *, std::FILE *, const char *)>
io::loaded<void> through_file(GameWorld *g, io::text_reader &in)
{
  std::FILE *fl = std::fopen(in.name().c_str(), "r");
  if (!fl)
    return std::unexpected(io::load_error{in.name(), 0, "can't be opened"});
  legacy(g, fl, in.name().c_str());
  std::fclose(fl);
  return {};
}

void legacy_rooms(GameWorld *g, std::FILE *fl, const char *name) { discrete_load(g, fl, DB_BOOT_WLD, name); }
void legacy_mobs(GameWorld *g, std::FILE *fl, const char *name) { discrete_load(g, fl, DB_BOOT_MOB, name); }
void legacy_objects(GameWorld *g, std::FILE *fl, const char *name) { discrete_load(g, fl, DB_BOOT_OBJ, name); }
void legacy_shops(GameWorld *g, std::FILE *fl, const char *name) { boot_the_shops(g, fl, name, 0); }
void legacy_guilds(GameWorld *g, std::FILE *fl, const char *name) { boot_the_guilds(g, fl, name, 0); }

struct world_kind {
  int mode;
  const char *prefix;
  int (*count)(const io::text_reader &);
  void (*report)(GameWorld *, int count); /* logs the count; null for kinds that don't */
  bool may_be_empty;                      /* shops and guilds: a world needs none */
  io::loaded<void> (*load)(GameWorld *, io::text_reader &);
  void (*finish)(GameWorld *); /* after every file has loaded; may be null */
};

constexpr world_kind kinds[] = {
    {DB_BOOT_ZON, ZON_PREFIX, count_one,           report_zones,   false, read_zone_file,               nullptr  },
    {DB_BOOT_TRG, TRG_PREFIX, count_hash_records,  nullptr,        false, read_trigger_file,            nullptr  },
    {DB_BOOT_WLD, WLD_PREFIX, count_hash_records,  report_rooms,   false, through_file<legacy_rooms>,   nullptr  },
    {DB_BOOT_MOB, MOB_PREFIX, count_hash_records,  report_mobs,    false, through_file<legacy_mobs>,    nullptr  },
    {DB_BOOT_OBJ, OBJ_PREFIX, count_hash_records,  report_objects, false, through_file<legacy_objects>, nullptr  },
    {DB_BOOT_SHP, SHP_PREFIX, count_hash_records,  nullptr,        true,  through_file<legacy_shops>,   nullptr  },
    {DB_BOOT_GLD, GLD_PREFIX, count_hash_records,  nullptr,        true,  through_file<legacy_guilds>,  nullptr  },
    {DB_BOOT_HLP, HLP_PREFIX, count_help_keywords, report_help,    false, through_file<load_help>,      sort_help},
};

} // namespace

io::loaded<void> boot_index(GameWorld *g, int mode)
{
  const world_kind *kind = std::ranges::find(kinds, mode, &world_kind::mode);
  if (kind == std::end(kinds))
    return std::unexpected(io::load_error{"", 0, str_format("there is no kind of world file {:d}", mode)});

  std::string index = str_format("{}{}", kind->prefix, g->mini_mud ? MINDEX_FILE : INDEX_FILE);
  io::loaded<std::vector<std::string>> names = io::read_index(index);
  if (!names)
    return std::unexpected(names.error());

  /* A file that can't be read is passed over in the count, so that a kind that may be empty still boots when its
     index names only missing files; when there are records to load, it stops the boot. */
  std::vector<io::text_reader> files;
  std::optional<io::load_error> missing;
  int count = 0;
  for (const std::string &name : *names) {
    io::loaded<io::text_reader> file = io::text_reader::open(str_format("{}{}", kind->prefix, name));
    if (!file) {
      mud_log("SYSERR: {}: can't be opened, and {} lists it", file.error().path, index);
      if (!missing)
        missing = io::load_error{file.error().path, 0, str_format("can't be opened, and {} lists it", index)};
      continue;
    }
    count += kind->count(*file);
    files.push_back(std::move(*file));
  }

  if (count == 0) {
    if (kind->may_be_empty)
      return {};
    return std::unexpected(io::load_error{index, 0, "the files it lists hold no records"});
  }
  if (kind->report)
    kind->report(g, count);
  if (missing)
    return std::unexpected(*missing);

  for (io::text_reader &file : files)
    if (io::loaded<void> loaded = kind->load(g, file); !loaded)
      return loaded;
  if (kind->finish)
    kind->finish(g);
  return {};
}

void index_boot(GameWorld *g, int mode)
{
  if (io::loaded<void> booted = boot_index(g, mode); !booted) {
    mud_log("SYSERR: {}", booted.error().text());
    exit(1);
  }
}

io::loaded<void> read_records(io::text_reader &in, const char *label,
                              const std::function<io::loaded<void>(io::text_reader &, int)> &read)
{
  int vnum = -1;
  for (;;) {
    std::optional<std::string_view> line = in.line();
    if (!line) {
      if (vnum == -1)
        return std::unexpected(in.fail(str_format("the {} file is empty", label)));
      return std::unexpected(in.fail(str_format(
          "the file ends after {} #{:d}, where another or the '$' that ends the file was expected", label, vnum)));
    }
    if (line->starts_with('$'))
      return {};
    if (!line->starts_with('#'))
      return std::unexpected(in.fail(str_format("expected a #<vnum>, or the '$' that ends the file, after {} #{:d}; got '{}'",
                                                label, vnum, *line)));
    int last = vnum;
    if (!io::fields(line->substr(1)).number(vnum))
      return std::unexpected(in.fail(str_format("expected a number after the '#', after {} #{:d}", label, last)));
    if (vnum >= 99999)
      return {};
    if (io::loaded<void> loaded = read(in, vnum); !loaded)
      return loaded;
  }
}
