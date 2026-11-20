/*
 * The room files, world/wld/<zone>.wld: one record per room.
 *
 *   #<vnum>
 *   <name>~
 *   <description>~
 *   <zone> <flags> <flags> <flags> <flags> <sector>
 *   D<direction>                                 an exit, any number of them
 *   <description>~
 *   <keywords>~
 *   <door> <key> <to room> <lock DC> <hide DC> <skill> <move DC> <save type> <save DC> <fail room> <total fail room>
 *   E                                            an extra description, any number of them
 *   <keywords>~
 *   <description>~
 *   S
 *   T <trigger vnum>                             any number of them
 *   ...
 *   $~
 */
#include "dbat/world_files.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genwld.hpp"
#include "dbat/genzon.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/utils.hpp"

#include "load_rules.hpp"

namespace {

/* An exit's door, as its first number gives it */
long door_flags(int door)
{
  switch (door) {
    case 1:
      return EX_ISDOOR;
    case 2:
      return EX_ISDOOR | EX_PICKPROOF;
    case 3:
      return EX_ISDOOR | EX_SECRET;
    case 4:
      return EX_ISDOOR | EX_PICKPROOF | EX_SECRET;
    default:
      return 0;
  }
}

/* An exit's door, as a number for the file: the reverse of door_flags() */
int door_number(long exit_info)
{
  if (!IS_SET(exit_info, EX_ISDOOR))
    return 0;
  if (IS_SET(exit_info, EX_SECRET) && IS_SET(exit_info, EX_PICKPROOF))
    return 4;
  if (IS_SET(exit_info, EX_SECRET))
    return 3;
  if (IS_SET(exit_info, EX_PICKPROOF))
    return 2;
  return 1;
}

io::loaded<void> read_exit(GameWorld *g, io::text_reader &in, room_vnum room, int dir)
{
  std::string what = str_format("room #{:d}, direction D{:d}", room + 1, dir); /* quirk: one past the room's vnum */

  room_direction_data *exit = new room_direction_data();
  room_set_dir_option(g, room, dir, exit);
  io::loaded<std::string> description = in.tilde_string(what);
  if (!description)
    return std::unexpected(description.error());
  io::loaded<std::string> keyword = in.tilde_string(what);
  if (!keyword)
    return std::unexpected(keyword.error());
  exit->general_description = std::move(*description);
  exit->keyword             = std::move(*keyword);

  std::optional<std::string_view> line = in.line();
  int n[11];
  if (!line || io::scan_ints(*line, n) != 11)
    return std::unexpected(in.fail(str_format(
        "room #{:d}, exit D{:d}: expected 11 numbers (door, key, to room, lock DC, hide DC, skill, move DC, save type, save DC, fail room, total fail room)",
        room, dir)));
  exit->exit_info     = door_flags(n[0]);
  exit->key           = n[1] == -1 || n[1] == 65535 ? NOTHING : n[1];
  exit->to_room       = n[2] == -1 || n[2] == 65535 ? NOWHERE : n[2];
  exit->dclock        = n[3];
  exit->dchide        = n[4];
  exit->dcskill       = n[5];
  exit->dcmove        = n[6];
  exit->failsavetype  = n[7];
  exit->dcfailsave    = n[8];
  exit->failroom      = n[9];
  exit->totalfailroom = n[10];
  return {};
}

io::loaded<void> read_room(GameWorld *g, io::text_reader &in, room_vnum vnum)
{
  std::string what = str_format("room #{:d}", vnum);

  zone_data *zone = find_zone_by_thing(g, vnum);
  if (!zone)
    return std::unexpected(in.fail(str_format("room #{:d} is outside of any zone", vnum)));
  if (g->rooms.find(vnum))
    return std::unexpected(in.fail(str_format("room #{:d} is defined twice", vnum)));

  g->rooms.emplace(vnum);
  room_data *room = room_of(g, vnum);
  room_set_zone(g, vnum, zone);
  room_set_number(g, vnum, vnum);
  io::loaded<std::string> name = in.tilde_string(what);
  if (!name)
    return std::unexpected(name.error());
  room_set_name(g, vnum, std::move(*name));
  io::loaded<std::string> description = in.tilde_string(what);
  if (!description)
    return std::unexpected(description.error());
  room_set_description(g, vnum, std::move(*description));

  std::optional<std::string_view> line = in.line();
  if (!line)
    return std::unexpected(in.fail(str_format("room #{:d}: the file ends before its flags and sector", vnum)));
  int zone_number = 0, sector = 0;
  std::string flags[4];
  io::fields f(*line);
  f.number(zone_number) && f.word(flags[0], 127) && f.word(flags[1], 127) && f.word(flags[2], 127) &&
      f.word(flags[3], 127) && f.number(sector);
  if (f.count() != 6)
    return std::unexpected(
        in.fail(str_format("room #{:d}: expected its zone, 4 flags and its sector, got '{}'", vnum, *line)));
  for (int i = 0; i < 4; i++)
    room_get_room_flags(g, vnum)[i] = io::ascii_flags(flags[i], 0, "asciiflag_conv");
  room_set_sector_type(g, vnum, sector);

  room_set_func(g, vnum, nullptr);
  room_set_light(g, vnum, 0);
  room_set_timed(g, vnum, -1);
  room_set_dmg(g, vnum, 0);
  room_set_gravity(g, vnum, 0);
  rules::gravity_by_flags(g, vnum);
  rules::gravity_by_vnum(g, vnum);
  for (int i = 0; i < NUM_OF_DIRS; i++)
    room_set_dir_option(g, vnum, i, nullptr);
  room_ex_description_slot(room)->clear();

  for (;;) {
    if (!(line = in.line()))
      return std::unexpected(in.fail(str_format("room #{:d}: the file ends before its 'S'", vnum)));
    switch (line->empty() ? '\0' : (*line)[0]) {
      case 'D': {
        int dir = static_cast<int>(io::c_atol(line->substr(1)));
        if (dir < 0 || dir >= NUM_OF_DIRS)
          return std::unexpected(in.fail(str_format("room #{:d}: there is no direction D{:d}", vnum, dir)));
        if (io::loaded<void> exit = read_exit(g, in, vnum, dir); !exit)
          return exit;
        break;
      }
      case 'E': {
        extra_descr_data descr;
        io::loaded<std::string> keyword = in.tilde_string(what);
        if (!keyword)
          return std::unexpected(keyword.error());
        io::loaded<std::string> text = in.tilde_string(what);
        if (!text)
          return std::unexpected(text.error());
        descr.keyword     = std::move(*keyword);
        descr.description = std::move(*text);
        rules::extra_description_ends_a_line(descr);
        room_ex_description_slot(room)->push_front(std::move(descr));
        break;
      }
      case 'S': /* the end of the room, and then its triggers */
        while (in.peek_letter() == 'T')
          dg_attach_proto_trigger(g, in.line().value_or(""), room);
        return {};
      default:
        return std::unexpected(
            in.fail(str_format("room #{:d}: expected D (an exit), E (an extra description) or S (the end), got '{}'",
                               vnum, *line)));
    }
  }
}

} // namespace

io::loaded<void> read_room_file(GameWorld *g, io::text_reader &in)
{
  return read_records(in, "world", [g](io::text_reader &r, int vnum) { return read_room(g, r, vnum); });
}

bool save_rooms(GameWorld *g, struct zone_data *zone_num)
{
  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  mud_log("GenOLC: save_rooms: Saving rooms in zone #{:d} ({:d}-{:d}).", zone_num->number,
          genolc_zone_bottom(g, zone_num), zone_num->top);

  io::text_writer out(str_format("{}{:d}.wld", WLD_PREFIX, zone_num->number));
  for (room_vnum i = genolc_zone_bottom(g, zone_num); i <= zone_num->top; i++) {
    room_data *room = room_of(g, i);
    if (!room)
      continue;

    std::string description = room_get_description(room) ? room_get_description(room) : "Empty room.";
    strip_cr(description);
    out.print("#{:d}\n"
              "{}{:c}\n"
              "{}{:c}\n"
              "{:d} {} {} {} {} {:d}\n",
              room_get_number(room), room_get_name(room) ? room_get_name(room) : "Untitled",
              as_char(STRING_TERMINATOR), description, as_char(STRING_TERMINATOR), room_get_zone(room)->number,
              flag_letters(room_get_room_flags(room)[0]), flag_letters(room_get_room_flags(room)[1]),
              flag_letters(room_get_room_flags(room)[2]), flag_letters(room_get_room_flags(room)[3]), room_get_sector_type(room));

    for (int j = 0; j < NUM_OF_DIRS; j++) {
      room_direction_data *exit = room_get_dir_option(room, j);
      if (!exit)
        continue;
      std::string exit_text = exit_description(exit) ? exit_description(exit) : "";
      strip_cr(exit_text);
      out.print("D{:d}\n"
                "{}~\n"
                "{}~\n"
                "{:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}\n",
                j, exit_text, exit_keyword(exit) ? exit_keyword(exit) : "", door_number(exit->exit_info),
                exit->key != NOTHING ? exit->key : -1, exit->to_room != NOWHERE ? exit->to_room : -1, exit->dclock,
                exit->dchide, exit->dcskill, exit->dcmove, exit->failsavetype, exit->dcfailsave, exit->failroom,
                exit->totalfailroom);
    }

    for (const extra_descr_data &xdesc : room_get_ex_description(room)) {
      std::string text = xdesc.description;
      strip_cr(text);
      out.print("E\n"
                "{}~\n"
                "{}~\n",
                xdesc.keyword, text);
    }
    out.write("S\n");
    for (trig_vnum vnum : proto_script_of(room))
      out.print("T {:d}\n", vnum);
  }
  out.write("$~\n");

  if (!out.commit()) {
    mud_log("SYSERR: save_rooms: can't write {}", out.path());
    return false;
  }
  if (in_save_list(g, zone_num->number, SL_WLD)) {
    remove_from_save_list(g, zone_num->number, SL_WLD);
    create_world_index(g, zone_num->number, "wld");
    mud_log("GenOLC: save_rooms: Saving rooms '{}'", out.path());
  }
  return true;
}
