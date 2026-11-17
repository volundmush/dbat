/*
 * The trigger files, world/trg/<zone>.trg: one record per trigger.
 *
 *   #<vnum>
 *   <name>~
 *   <attach type> <trigger flags> <numeric argument>
 *   <argument>~
 *   <script, a command to a line>
 *   ~
 *   ...
 *   $~
 */
#include <memory>

#include "dbat/world_files.hpp"
#include "dbat/dg/program.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/utils.hpp"

namespace {

io::loaded<void> read_trigger(GameWorld *g, io::text_reader &in, trig_vnum vnum)
{
  std::string what = str_format("trig vnum {:d}", vnum);

  io::loaded<std::string> name = in.tilde_string(what);
  if (!name)
    return std::unexpected(name.error());

  std::optional<std::string_view> line = in.line();
  if (!line)
    return std::unexpected(in.fail(str_format("trigger #{:d}: the file ends before its attach type and flags", vnum)));
  int attach_type = 0, narg = 0;
  std::string flags;
  io::fields f(*line);
  f.number(attach_type) && f.word(flags, 255) && f.number(narg);
  if (f.count() < 2)
    return std::unexpected(
        in.fail(str_format("trigger #{:d}: expected its attach type and flags, got '{}'", vnum, *line)));

  io::loaded<std::string> arglist = in.tilde_string(what);
  if (!arglist)
    return std::unexpected(arglist.error());
  io::loaded<std::string> script = in.tilde_string(what);
  if (!script)
    return std::unexpected(script.error());

  if (g->trig_index.find(vnum)) {
    mud_log("SYSERR: trigger #{:d} is defined twice; ignoring the second one.", vnum);
    return {};
  }

  trig_data *trig                   = new trig_data();
  std::unique_ptr<index_data> index = std::make_unique<index_data>();
  index->vnum                       = vnum;
  index->proto                      = trig;
  trig->index                       = index.get();
  trig->name                        = std::move(*name);
  trig->attach_type                 = static_cast<int8_t>(attach_type);
  trig->trigger_type                = static_cast<long>(io::ascii_flags(flags, 0, "asciiflag_conv"));
  trig->narg                        = f.count() == 3 ? narg : 0;
  trig->arglist                     = std::move(*arglist);
  trig->program                     = dg::lex(*script, "* Empty script"); /* an empty script gets a placeholder */
  g->trig_index.insert(vnum, std::move(index));

  report_compile_errors(g, trig, nullptr);
  return {};
}

} // namespace

io::loaded<void> read_trigger_file(GameWorld *g, io::text_reader &in)
{
  return read_records(in, "trg", [g](io::text_reader &r, int vnum) { return read_trigger(g, r, vnum); });
}

/* Writes every trigger of the zone to <zone>.trg. Returns false, having logged why, if it couldn't. */
bool save_triggers(GameWorld *g, struct zone_data *in_zone)
{
  char bitBuf[MAX_INPUT_LENGTH];
  zone_vnum zone = in_zone->number;

  io::text_writer out(str_format("{}{:d}.trg", TRG_PREFIX, zone));
  for (IDXTYPE i = in_zone->bot; i <= in_zone->top; i++) {
    struct index_data *rnum = find_trigger(g, i);
    if (!rnum)
      continue;
    trig_data *trig = rnum->proto;

    out.print("#{:d}\n", i);
    sprintascii(bitBuf, trig->trigger_type);
    out.print("{}{:c}\n"
              "{:d} {} {:d}\n"
              "{}{:c}\n",
              !trig->name.empty() ? trig->name.c_str() : "unknown trigger", as_char(STRING_TERMINATOR),
              trig->attach_type, *bitBuf ? bitBuf : "0", trig->narg, trig->arglist, as_char(STRING_TERMINATOR));

    /* the script, one line per command */
    if (!trig->program)
      out.write("* Empty script");
    else
      for (const dg::line &l : trig->program->lines)
        out.print("{}\n", l.text);
    out.print("{:c}\n", as_char(STRING_TERMINATOR));
  }
  out.print("${:c}\n", as_char(STRING_TERMINATOR));

  if (!out.commit()) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Can't write trig file \"{}\"", out.path());
    return false;
  }
  create_world_index(g, zone, "trg");
  return true;
}
