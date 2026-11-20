/*
 * The zone files, world/zon/<zone>.zon: a zone's header and its reset commands.
 *
 *   @Version: 2
 *   #<zone>
 *   <builders>~
 *   <name>~
 *   <bottom> <top> <lifespan> <reset mode> <flags> <flags> <flags> <flags> <min level> <max level>
 *   <command> <if-flag> <arg1> <arg2> <arg3> [<arg4> <arg5>] [<sarg1> <sarg2>]	(comment)
 *   ...
 *   S
 *   $
 */
#include <cstring>
#include <memory>

#include "dbat/world_files.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/zone_utils.hpp"

namespace {

/* The letters of the commands that take more than four numbers */
constexpr std::string_view long_commands = "MOEPDTVG";

/* How many commands a zone file holds, its 'S' included, by the rule they are counted by before they are read: the
   lines after the first three that start with a command's letter and a space, or are just "S". (quirk: commands are
   read by a looser rule, passing over spaces before the letter, so an indented one is a count mismatch.) */
int count_commands(io::text_reader in) /* a copy: the count reads ahead */
{
  for (int i = 0; i < 3; i++)
    in.line();
  int count = 0;
  while (std::optional<std::string_view> line = in.line())
    if ((line->size() >= 2 && std::string_view("MOPGERDTV").find((*line)[0]) != std::string_view::npos &&
         (*line)[1] == ' ') ||
        *line == "S")
      count++;
  return count;
}

/* A string field of the header, which may end in a '~' */
std::string up_to_tilde(std::string_view line)
{
  return std::string(line.substr(0, line.find('~')));
}

} // namespace

io::loaded<void> read_zone_file(GameWorld *g, io::text_reader &in)
{
  int expected = count_commands(in);
  if (expected == 0)
    return std::unexpected(in.fail("the file holds no zone commands, not even the 'S' that ends them"));

  std::unique_ptr<zone_data> made = std::make_unique<zone_data>();
  zone_data &zone                 = *made;
  int version                     = 1;

  std::optional<std::string_view> line = in.line();
  if (line && line->starts_with('@')) {
    if (!line->starts_with("@Version:") || !io::fields(line->substr(9)).number(version))
      return std::unexpected(in.fail(str_format("expected \"@Version: <number>\", got '{}'", *line)));
    line = in.line();
  }
  if (!line || !line->starts_with('#') || !io::fields(line->substr(1)).number(zone.number))
    return std::unexpected(in.fail("expected the zone's #<number>"));

  if (!(line = in.line()))
    return std::unexpected(in.fail(str_format("zone #{:d}: the file ends before its builders", zone.number)));
  zone.builders = up_to_tilde(*line);
  if (!(line = in.line()))
    return std::unexpected(in.fail(str_format("zone #{:d}: the file ends before its name", zone.number)));
  zone.name = up_to_tilde(*line);
  if (!(line = in.line()))
    return std::unexpected(in.fail(str_format("zone #{:d}: the file ends before its numbers", zone.number)));

  /* A version 1 zone with no builders line has its numbers where its name should be, and its first command where its
     numbers should be: the commands then start with that line. */
  std::optional<std::string_view> first_command;
  int line_offset = 0;
  if (version >= 2) {
    std::string flags[4];
    io::fields f(*line);
    f.number(zone.bot) && f.number(zone.top) && f.number(zone.lifespan) && f.number(zone.reset_mode) &&
        f.word(flags[0]) && f.word(flags[1]) && f.word(flags[2]) && f.word(flags[3]) && f.number(zone.min_level) &&
        f.number(zone.max_level);
    if (f.count() != 10)
      return std::unexpected(in.fail(str_format(
          "zone #{:d}: expected 10 numbers and flags (bottom, top, lifespan, reset mode, 4 flags, min and max level), got {:d}",
          zone.number, f.count())));
    for (int i = 0; i < 4; i++)
      zone.zone_flags[i] = io::ascii_flags(flags[i], 0, "asciiflag_conv");
  } else {
    int numbers[4];
    if (io::scan_ints(*line, numbers) != 4) {
      mud_log("SYSERR: Format error in numeric constant line of {}, attempting to fix.", in.name());
      if (io::scan_ints(zone.name, numbers) != 4)
        return std::unexpected(in.fail(str_format(
            "zone #{:d}: expected 4 numbers (bottom, top, lifespan, reset mode), and there were none on the line before either",
            zone.number)));
      zone.name     = zone.builders;
      zone.builders = "None.";
      first_command = line;
      line_offset   = 3; /* quirk: the old count was off by three from here on */
    }
    zone.bot        = numbers[0];
    zone.top        = numbers[1];
    zone.lifespan   = numbers[2];
    zone.reset_mode = numbers[3];
  }
  if (zone.bot > zone.top)
    return std::unexpected(
        in.fail(str_format("zone #{:d}: its bottom ({:d}) is above its top ({:d})", zone.number, zone.bot, zone.top)));

  zone.cmd.assign(expected, reset_com{});
  int read = 0;
  for (;;) {
    std::string_view text;
    if (first_command) {
      text = *first_command;
      first_command.reset();
    } else if (std::optional<std::string_view> next = in.line())
      text = *next;
    else
      return std::unexpected(in.fail(str_format("zone #{:d}: the file ends before the 'S' that ends its commands", zone.number)));

    std::string_view rest = skip_spaces(text);
    char letter           = rest.empty() ? '\0' : rest[0];
    if (letter != '*' && read >= expected)
      return std::unexpected(in.fail(str_format(
          "zone #{:d}: more commands than the {:d} counted (is one indented?)", zone.number, expected)));
    if (letter == '*')
      continue;

    reset_com &cmd = zone.cmd[read];
    if (letter == 'S' || letter == '$') {
      cmd.command = 'S';
      break;
    }
    cmd.command = letter;
    rest.remove_prefix(rest.empty() ? 0 : 1);

    io::fields f(rest);
    int if_flag = 0;
    bool ok;
    if (letter == '\0' || long_commands.find(letter) == std::string_view::npos) /* 4 numbers, as R */
      ok = f.number(if_flag) && f.number(cmd.arg1) && f.number(cmd.arg2) && f.number(cmd.arg3);
    else if (letter == 'V') { /* 6 numbers and 2 words */
      std::string name, value;
      ok = f.number(if_flag) && f.number(cmd.arg1) && f.number(cmd.arg2) && f.number(cmd.arg3) &&
           f.number(cmd.arg4) && f.number(cmd.arg5) && f.word(name, 79) && f.until(value, "\f\n\r\t\v", 79);
      if (ok) {
        cmd.sarg1 = name;
        cmd.sarg2 = value;
      }
    } else { /* 6 numbers, or 5 with no percentage */
      f.number(if_flag) && f.number(cmd.arg1) && f.number(cmd.arg2) && f.number(cmd.arg3) && f.number(cmd.arg4) &&
          f.number(cmd.arg5);
      ok = f.count() == 6 || f.count() == 5;
      if (f.count() == 5)
        cmd.arg5 = 0;
    }
    if (!ok)
      return std::unexpected(in.fail(str_format("zone #{:d}: can't read the command '{}'", zone.number, text)));
    cmd.if_flag = if_flag;
    cmd.line    = in.line_number() + line_offset;
    read++;
  }

  if (expected != read + 1)
    return std::unexpected(in.fail(str_format("zone #{:d}: {:d} commands were counted, but {:d} read", zone.number,
                                              expected, read + 1)));
  if (g->zones.find(zone.number))
    return std::unexpected(in.fail(str_format("zone #{:d} is defined twice", zone.number)));
  g->zones.insert(zone.number, std::move(made));
  return {};
}

bool save_zone(GameWorld *g, struct zone_data *zone_num)
{
  int arg4 = -1, arg5 = -1;
  IDXTYPE arg1 = -1, arg2 = -1, arg3 = -1;
  const char *comment = nullptr;

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  io::text_writer out(str_format("{}{:d}.zon", ZON_PREFIX, zone_num->number));

  out.print("@Version: {:d}\n", CUR_ZONE_VERSION);
  out.print("#{:d}\n"
            "{}~\n"
            "{}~\n"
            "{:d} {:d} {:d} {:d} {} {} {} {} {:d} {:d}\n",
            zone_num->number, !zone_num->builders.empty() ? zone_num->builders.c_str() : "None.",
            !zone_num->name.empty() ? zone_num->name.c_str() : "undefined", genolc_zone_bottom(g, zone_num),
            zone_num->top, zone_num->lifespan, zone_num->reset_mode, flag_letters(zone_num->zone_flags[0]),
            flag_letters(zone_num->zone_flags[1]), flag_letters(zone_num->zone_flags[2]), flag_letters(zone_num->zone_flags[3]), zone_num->min_level,
            zone_num->max_level);

  /*
   * Field #1    Field #3   Field #4  Field #5           Field #6   Field #7
   * ----------------------------------------------------------------------------
   * M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum          room_max   Percent load failure
   * O (Object)  Obj-Vnum   Wld-Max   Room-Vnum          room_max   Percent load failure
   * G (Give)    Obj-Vnum   Wld-Max   Unused             unused     Percent load failure
   * E (Equip)   Obj-Vnum   Wld-Max   EQ-Position        unused     Percent load failure
   * P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum    unused     Percent load failure
   * D (Door)    Room-Vnum  Door-Dir  Door-State         unused     Percent load failure
   * R (Remove)  Room-Vnum  Obj-Vnum  Unused             unused     Percent load failure
   * T (Trigger) Trig-type  Trig-Vnum Room-Vnum          unused     Percent load failure
   * V (var)     Trig-type  Context   Room-Vnum Varname  unused     Percent load failure
   *
   * quirk: D and R leave arg4 and arg5 as the command before them set them.
   */
  for (int subcmd = 0; zone_get_cmd(g, zone_num, subcmd)->command != 'S'; subcmd++) {
    reset_com *cmd = zone_get_cmd(g, zone_num, subcmd);
    switch (cmd->command) {
      case 'M':
        arg1    = mob_index_of(g, cmd->arg1)->vnum;
        arg2    = cmd->arg2;
        arg3    = cmd->arg3;
        arg4    = cmd->arg4;
        arg5    = cmd->arg5;
        comment = char_get_sdesc(mob_proto_of(g, cmd->arg1));
        break;
      case 'O':
        arg1    = obj_index_of(g, cmd->arg1)->vnum;
        arg2    = cmd->arg2;
        arg3    = cmd->arg3;
        arg4    = cmd->arg4;
        arg5    = cmd->arg5;
        comment = obj_get_short_description(obj_proto_of(g, cmd->arg1));
        break;
      case 'G':
        arg1    = obj_index_of(g, cmd->arg1)->vnum;
        arg2    = cmd->arg2;
        arg3    = -1;
        arg4    = -1;
        arg5    = cmd->arg5;
        comment = obj_get_short_description(obj_proto_of(g, cmd->arg1));
        break;
      case 'E':
        arg1    = obj_index_of(g, cmd->arg1)->vnum;
        arg2    = cmd->arg2;
        arg3    = cmd->arg3;
        arg4    = -1;
        arg5    = cmd->arg5;
        comment = obj_get_short_description(obj_proto_of(g, cmd->arg1));
        break;
      case 'P':
        arg1    = obj_index_of(g, cmd->arg1)->vnum;
        arg2    = cmd->arg2;
        arg3    = obj_index_of(g, cmd->arg3)->vnum;
        arg4    = -1;
        arg5    = cmd->arg5;
        comment = obj_get_short_description(obj_proto_of(g, cmd->arg1));
        break;
      case 'D':
        arg1    = cmd->arg1;
        arg2    = cmd->arg2;
        arg3    = cmd->arg3;
        comment = room_get_name(g, cmd->arg1);
        break;
      case 'R':
        arg1    = cmd->arg1;
        arg2    = obj_index_of(g, cmd->arg2)->vnum;
        comment = obj_get_short_description(obj_proto_of(g, cmd->arg2));
        arg3    = -1;
        break;
      case 'T':
        arg1    = cmd->arg1; /* trigger type */
        arg2    = cmd->arg2; /* trigger vnum */
        arg3    = cmd->arg3; /* room num */
        arg4    = -1;
        arg5    = cmd->arg5;
        comment = find_trigger(g, arg2)->proto->name.c_str();
        break;
      case 'V':
        arg1 = cmd->arg1; /* trigger type */
        arg2 = cmd->arg2; /* context */
        arg3 = cmd->arg3;
        arg4 = -1;
        arg5 = cmd->arg5;
        break;
      case '*': /* invalid commands are replaced with '*' */
        continue;
      default:
        mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '{:c}' - NOT saving",
               as_char(cmd->command));
        continue;
    }
    if (cmd->command != 'V')
      out.print("{:c} {:d} {:d} {:d} {:d} {:d} {:d} \t({})\n", as_char(cmd->command), cmd->if_flag, arg1, arg2, arg3,
                arg4, arg5, comment);
    else
      out.print("{:c} {:d} {:d} {:d} {:d} {:d} {:d} {} {}\n", as_char(cmd->command), cmd->if_flag, arg1, arg2, arg3,
                arg4, arg5, cmd->sarg1, cmd->sarg2);
  }
  out.write("S\n$\n");
  if (!out.commit()) {
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: save_zones:  Can't write zone {:d}.", zone_num->number);
    return false;
  }

  if (in_save_list(g, zone_num->number, SL_ZON)) {
    remove_from_save_list(g, zone_num->number, SL_ZON);
    create_world_index(g, zone_num->number, "zon");
    mud_log("GenOLC: save_zone: Saving zone '{}'", out.path());
  }
  return true;
}
