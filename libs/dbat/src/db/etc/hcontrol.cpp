/*
 * The house control file, etc/hcontrol: who owns each house, and how.
 *
 *   # dbat hcontrol 2
 *   #<house room vnum>
 *   Atrium: <vnum of the room outside>
 *   Exit: <direction from the house to it>
 *   Built: <when>
 *   Mode: <mode of ownership>
 *   Owner: <id>
 *   Paid: <when it was last paid for>
 *   Bits: <HOUSE_ flags>
 *   BuiltBy: <id>
 *   Guests: <id> ...                  any number up to MAX_GUESTS, or none
 *   Spare: <6 numbers>                only if any isn't 0
 *
 * Older deployments hold it as a binary dump of the records: see legacy_binary.cpp, which converts it as it loads.
 */
#include <array>
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dbat/house.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

#include "legacy_binary.hpp"

namespace {

constexpr std::string_view hcontrol_header = "# dbat hcontrol 2";

struct house_field {
  std::string_view tag{};
  bool (*read)(house_control_rec &h, io::fields &f);
  void (*write)(const house_control_rec &h, io::text_writer &out, std::string_view tag);
};

template <class T> bool read_number(T &field, io::fields &f)
{
  long long n = 0;
  if (!f.number(n))
    return false;
  field = static_cast<T>(n);
  return true;
}

template <class T> void write_number(io::text_writer &out, std::string_view tag, T value)
{
  out.print("{}: {:d}\n", tag, static_cast<long long>(value));
}

long *spares(house_control_rec &h, int i)
{
  auto s = std::to_array<long *>({&h.spare2, &h.spare3, &h.spare4, &h.spare5, &h.spare6, &h.spare7});
  return s[i];
}

const auto house_fields = std::to_array<house_field>({
    {"Atrium", [](house_control_rec &h, io::fields &f) { return read_number(h.atrium, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.atrium); }},
    {"Exit", [](house_control_rec &h, io::fields &f) { return read_number(h.exit_num, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.exit_num); }},
    {"Built", [](house_control_rec &h, io::fields &f) { return read_number(h.built_on, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.built_on); }},
    {"Mode", [](house_control_rec &h, io::fields &f) { return read_number(h.mode, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.mode); }},
    {"Owner", [](house_control_rec &h, io::fields &f) { return read_number(h.owner, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.owner); }},
    {"Paid", [](house_control_rec &h, io::fields &f) { return read_number(h.last_payment, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.last_payment); }},
    {"Bits", [](house_control_rec &h, io::fields &f) {
       long bits = 0;
       if (!read_number(bits, f))
         return false;
       h.bitvector = house_flags_from_number(bits);
       return true;
     }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, house_flags_number(h.bitvector)); }},
    {"BuiltBy", [](house_control_rec &h, io::fields &f) { return read_number(h.builtby, f); }, [](const house_control_rec &h, io::text_writer &o, std::string_view t) { write_number(o, t, h.builtby); }},
    {"Guests",
     [](house_control_rec &h, io::fields &f) {
       h.num_of_guests = 0;
       for (long id = 0; read_number(id, f);) {
         if (h.num_of_guests >= MAX_GUESTS)
           return false;
         h.guests[h.num_of_guests++] = id;
       }
       return f.rest().find_first_not_of(" \t") == std::string_view::npos;
     },
     [](const house_control_rec &h, io::text_writer &o, std::string_view t) {
       o.print("{}:", t);
       for (int i = 0; i < h.num_of_guests; i++)
         o.print(" {:d}", h.guests[i]);
       o.write("\n");
     }},
    {"Spare",
     [](house_control_rec &h, io::fields &f) {
       for (int i = 0; i < 6; i++)
         if (!read_number(*spares(h, i), f))
           return false;
       return true;
     },
     [](const house_control_rec &h, io::text_writer &o, std::string_view t) {
       house_control_rec copy = h;
       bool any               = false;
       for (int i = 0; i < 6; i++)
         any = any || *spares(copy, i);
       if (!any)
         return;
       o.print("{}:", t);
       for (int i = 0; i < 6; i++)
         o.print(" {:d}", *spares(copy, i));
       o.write("\n");
     }},
});

} // namespace

static io::loaded<std::vector<house_control_rec>> read_hcontrol(io::text_reader &in)
{
  std::vector<house_control_rec> houses;
  std::optional<std::string_view> line = in.raw_line();
  if (!line || !line->starts_with(hcontrol_header))
    return std::unexpected(in.fail(str_format("expected '{}' to begin the file", hcontrol_header)));

  while ((line = in.line())) {
    if (line->starts_with('#')) {
      house_control_rec h{};
      if (!io::fields(line->substr(1)).number(h.vnum))
        return std::unexpected(in.fail(str_format("expected a house's vnum after the '#', got '{}'", *line)));
      houses.push_back(h);
      continue;
    }
    if (houses.empty())
      return std::unexpected(in.fail(str_format("expected '#<vnum>' to begin a house, got '{}'", *line)));
    size_t colon         = line->find(':');
    std::string_view tag = line->substr(0, colon);
    const house_field *field =
        std::ranges::find_if(house_fields, [&](const house_field &hf) { return tag == hf.tag; });
    if (colon == std::string_view::npos || field == std::end(house_fields))
      return std::unexpected(in.fail(str_format("house #{:d}: '{}' isn't one of its fields", houses.back().vnum, *line)));
    io::fields f(line->substr(colon + 1));
    if (!field->read(houses.back(), f))
      return std::unexpected(in.fail(str_format("house #{:d}: can't read its {}: '{}'", houses.back().vnum, field->tag, *line)));
  }
  return houses;
}

io::loaded<house_control_file> read_house_control()
{
  house_control_file got;
  io::loaded<io::text_reader> file = io::text_reader::open(HCONTROL_FILE);
  if (!file) {
    got.missing = true;
    return got;
  }
  if (legacy::is_binary(file->text(), hcontrol_header, legacy::house_record_size)) {
    got.houses    = legacy::houses(file->text());
    got.converted = true;
    for (house_control_rec &h : got.houses) /* a count past the guests there are room for was read past them */
      h.num_of_guests = std::clamp(h.num_of_guests, 0, MAX_GUESTS);
    return got;
  }
  io::loaded<std::vector<house_control_rec>> houses = read_hcontrol(*file);
  if (!houses)
    return std::unexpected(houses.error());
  got.houses = std::move(*houses);
  return got;
}

/* Save the house control information, keeping what it held before as etc/hcontrol.bak */
void House_save_control(GameWorld *g)
{
  io::text_writer out(HCONTROL_FILE, true);
  out.print("{}\n", hcontrol_header);
  for (int i = 0; i < g->num_of_houses; i++) {
    const house_control_rec &h = g->house_control[i];
    out.print("#{:d}\n", h.vnum);
    for (const house_field &field : house_fields)
      field.write(h, out, field.tag);
  }
  out.commit();
}
