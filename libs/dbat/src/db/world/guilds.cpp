/*
 * The guild files, world/gld/<zone>.gld. Each record is a "~" string; a guild starts with "#<vnum>~".
 *
 *   #<vnum>~
 *   <skill> 1 | <feat> 2 | <skill>                   what it teaches, any number of them, then
 *   -1
 *   <charge>
 *   <no such skill message>~
 *   <not enough gold message>~
 *   <minimum level>
 *   <guildmaster's mob vnum>
 *   <with whom it won't trade, the first>
 *   <open>
 *   <close>
 *   <with whom it won't trade, the other 3>          may be left out
 *   ...
 *   $~
 */
#include <memory>
#include <optional>

#include "dbat/world_files.hpp"
#include "dbat/consts/feats.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/gengld.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/guild.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/spells.hpp"
#include "dbat/utils.hpp"

namespace {

io::loaded<void> read_number(io::text_reader &in, guild_vnum vnum, int &n, const char *what)
{
  std::optional<std::string_view> line = in.line();
  if (!line || io::scan_ints(*line, std::span(&n, 1)) == 0) /* quirk: a line of spaces leaves n as it was */
    return std::unexpected(in.fail(str_format("guild #{:d}: expected {}, got '{}'", vnum, what, line.value_or(""))));
  return {};
}

/* A message, or its default if it is empty or would be an unsafe format for npc_message() */
io::loaded<std::string> read_message(io::text_reader &in, guild_vnum vnum, const char *field, const char *fallback)
{
  io::loaded<std::string> text = in.tilde_string(str_format("guild #{:d}", vnum));
  if (!text)
    return text;
  if (text->empty())
    return fallback;
  if (std::optional<std::string> problem = npc_msg_format_problem(text->c_str(), false)) {
    mud_log("SYSERR: Guild #{:d} {}: {}; using the default.", vnum, field, *problem);
    return fallback;
  }
  return text;
}

/* What it teaches: "<skill> 1" or "<feat> 2", or a bare "<skill>" as the oldest guilds have it */
io::loaded<void> read_teachings(io::text_reader &in, guild_data *guild, guild_vnum vnum)
{
  int t[2] = {};
  for (;;) {
    std::optional<std::string_view> line = in.line();
    if (!line)
      return std::unexpected(in.fail(str_format("guild #{:d}: the file ends in its list of skills", vnum)));
    t[0]      = 0;
    int count = io::scan_ints(*line, t);
    if (t[0] <= -1)
      return {};
    if (count == 1 || (count == 2 && t[1] == 1)) {
      if (t[0] < SKILL_TABLE_SIZE)
        guild->skills[t[0]] = 1;
      else
        mud_log("SYSERR: Guild #{:d} teaches impossible skill {:d}", vnum, t[0]);
    } else if (count == 2 && t[1] == 2) {
      if (t[0] < NUM_FEATS_DEFINED)
        guild->feats[t[0]] = 1;
      else
        mud_log("SYSERR: Guild #{:d} teaches impossible feat {:d}", vnum, t[0]);
    } else if (count == 2)
      return std::unexpected(in.fail(
          str_format("guild #{:d}: {:d} after skill {:d} is neither 1 (a skill) nor 2 (a feat)", vnum, t[1], t[0])));
    else
      return std::unexpected(
          in.fail(str_format("guild #{:d}: expected a skill or feat and 1 or 2, got '{}'", vnum, *line)));
  }
}

/* One guild; next is the record after it, which the guild may have read already */
io::loaded<void> read_guild(GameWorld *g, io::text_reader &in, guild_vnum vnum, std::string &next)
{
  std::string context = str_format("GM #{:d} in GM file {}", vnum, in.name());
  auto made           = std::make_unique<guild_data>();
  guild_data *guild   = made.get();
  guild_set_vnum(guild, vnum);

  if (io::loaded<void> r = read_teachings(in, guild, vnum); !r)
    return r;

  std::optional<std::string_view> line = in.line();
  if (!line || io::scan_float(*line, guild->charge) == 0)
    return std::unexpected(in.fail(str_format("guild #{:d}: expected its charge, got '{}'", vnum, line.value_or(""))));
  io::loaded<std::string> message = read_message(in, vnum, "no_such_skill", guild_default_no_skill);
  if (!message)
    return std::unexpected(message.error());
  guild->no_such_skill = std::move(*message);
  if (!(message = read_message(in, vnum, "not_enough_gold", guild_default_no_gold)))
    return std::unexpected(message.error());
  guild->not_enough_gold = std::move(*message);

  if (io::loaded<void> r = read_number(in, vnum, guild->minlvl, "its minimum level"); !r)
    return r;
  if (io::loaded<void> r = read_number(in, vnum, guild->gm, "its guildmaster's vnum"); !r)
    return r;
  if (!mob_proto_of(g, guild_get_gm(guild)))
    guild_set_gm(guild, NOBODY);
  int first = static_cast<int>(guild->with_who[0]);
  if (io::loaded<void> r = read_number(in, vnum, first, "with whom it won't trade"); !r)
    return r;
  guild->with_who[0] = static_cast<bitvector_t>(first);
  if (io::loaded<void> r = read_number(in, vnum, guild->open, "the hour it opens"); !r)
    return r;
  if (io::loaded<void> r = read_number(in, vnum, guild->close, "the hour it closes"); !r)
    return r;
  guild_set_func(guild, nullptr);

  /* the rest of with whom it won't trade, if the next line isn't the next record (quirk: if it is, that line is read
     as the next record, '~' and all) */
  std::string with_who(in.line().value_or(""));
  if (!with_who.empty() && (with_who[0] == '#' || with_who[0] == '$'))
    next = std::move(with_who);
  else {
    int slot = 1;
    for (size_t at = 0; slot < GW_ARRAY_MAX && at < with_who.size(); slot++) {
      int n = 0;
      if (io::scan_ints(std::string_view(with_who).substr(at), std::span(&n, 1)) != 1) {
        mud_log("SYSERR: Can't parse GM_WITH_WHO line in {}: '{}'", context, with_who);
        break;
      }
      guild_get_with_who(guild)[slot] = static_cast<bitvector_t>(n);
      while (at < with_who.size() && (isdigit(static_cast<unsigned char>(with_who[at])) || with_who[at] == '-'))
        at++;
      while (at < with_who.size() && !(isdigit(static_cast<unsigned char>(with_who[at])) || with_who[at] == '-'))
        at++;
    }
    for (; slot < GW_ARRAY_MAX; slot++)
      guild_get_with_who(guild)[slot] = 0;
    io::loaded<std::string> after = in.tilde_string(context);
    if (!after)
      return std::unexpected(after.error());
    next = std::move(*after);
  }

  if (g->guilds.find(vnum))
    mud_log("SYSERR: Guild #{:d} is defined twice; ignoring the second one.", vnum);
  else
    g->guilds.insert(vnum, std::move(made));
  return {};
}

} // namespace

io::loaded<void> read_guild_file(GameWorld *g, io::text_reader &in)
{
  std::string context            = str_format("beginning of GM file {}", in.name());
  io::loaded<std::string> record = in.tilde_string(context);
  if (!record)
    return std::unexpected(record.error());
  std::string next = std::move(*record);
  /* quirk: an empty record ends the file, whatever is after it */
  while (!next.empty()) {
    if (next[0] == '$')
      return {};
    if (next[0] == '#') {
      int vnum = 0;
      if (!io::fields(std::string_view(next).substr(1)).number(vnum))
        return std::unexpected(in.fail(str_format("expected a number after the '#', got '{}'", next)));
      if (io::loaded<void> guild = read_guild(g, in, vnum, next); !guild)
        return guild;
      continue;
    }
    if (!(record = in.tilde_string(context))) /* anything else isn't a guild, and is passed over */
      return std::unexpected(record.error());
    next = std::move(*record);
  }
  return {};
}

bool save_guilds(GameWorld *g, struct zone_data *zone)
{
  if (!zone) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  io::text_writer out(str_format("{}{:d}.gld", GLD_PREFIX, zone->number));
  for (guild_vnum vnum : g->guilds.vnums_in(genolc_zone_bottom(g, zone), zone->top)) {
    guild_data *guild = find_guild(g, vnum);
    if (!guild)
      continue;
    out.print("#{:d}~\n", vnum);
    for (int i = 0; i < SKILL_TABLE_SIZE; i++)
      if (guild_get_skill(guild, i))
        out.print("{:d} 1\n", i);
    for (int i = 0; i < NUM_FEATS_DEFINED; i++)
      if (guild_get_feat(guild, i))
        out.print("{:d} 2\n", i);
    out.print("-1\n{:>1.2f}\n", guild_get_charge(guild));
    out.print("{}~\n{}~\n", guild_get_no_such_skill(guild) ? guild_get_no_such_skill(guild) : "%s ERROR",
              guild_get_not_enough_gold(guild) ? guild_get_not_enough_gold(guild) : "%s ERROR");
    out.print("{:d}\n{:d}\n{:d}\n{:d}\n{:d}\n", guild_get_minlvl(guild),
              guild_get_gm(guild) == NOBODY ? -1 : mob_index_of(g, guild_get_gm(guild))->vnum,
              static_cast<int>(guild_get_with_who(guild)[0]), /* as signed, as always */
              guild_get_open(guild), guild_get_close(guild));
    for (int i = 1; i < GW_ARRAY_MAX; i++)
      out.print("{}{:d}", i == 1 ? "" : " ", static_cast<int>(guild_get_with_who(guild)[i]));
    out.write("\n");
  }
  out.write("$~\n");
  if (!out.commit())
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot write guild file {}!", out.path());

  if (in_save_list(g, zone->number, SL_GLD)) {
    remove_from_save_list(g, zone->number, SL_GLD);
    create_world_index(g, zone->number, "gld");
    mud_log("GenOLC: save_guilds: Saving guilds '{}'", out.path());
  }
  return true;
}
