/*
 * The object files, world/obj/<zone>.obj: one record per object prototype. A record has no end marker: it runs to
 * the next "#<vnum>", or the "$" that ends the file.
 *
 *   #<vnum>
 *   <aliases>~
 *   <short description>~
 *   <long description>~
 *   <action description>~
 *   <type> <extra flags x4> <wear flags x4> <affect flags x4>
 *   <value 0> ... <value 15>                         up to 16 of them; the rest are 0
 *   <weight> <cost> <rent> [<level>]
 *   then any of, in any order:
 *   E                                                an extra description
 *   <keywords>~
 *   <description>~
 *   A                                                an affect, up to MAX_OBJ_AFFECT of them
 *   <location> <modifier> [<specific>]
 *   S                                                a spellbook's spell
 *   <spell> <pages>
 *   T <trigger vnum>
 *   Z                                                its size
 *   <size>
 */
#include <optional>

#include "dbat/world_files.hpp"
#include "dbat/affect.hpp"
#include "dbat/consts/applies.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extradesc.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genobj.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/spells.hpp"
#include "dbat/utils.hpp"

#include "load_rules.hpp"

namespace {

/* The checks of what an object's values mean, logged but loaded all the same */

bool check_object_spell_number(GameWorld *g, obj_data *obj, int val)
{
  int spell = obj_get_value(obj, val);
  if (spell == -1 || spell == 0) /* no spell */
    return false;

  bool error = spell < 0 || spell >= SKILL_TABLE_SIZE || skill_type(g, spell) != SKTYPE_SPELL;
  if (error)
    mud_log("SYSERR: Object #{:d} ({}) has out of range spell #{:d}.", obj_get_vnum(g, obj),
            obj_get_short_description(obj), spell);

  if (g->scheck) /* spell names don't exist in syntax check mode */
    return error;

  const char *name = skill_name(g, spell);
  if (name == unused_spellname || str_eq_nocase("UNDEFINED", name)) {
    error = true;
    mud_log("SYSERR: Object #{:d} ({}) uses '{}' spell #{:d}.", obj_get_vnum(g, obj), obj_get_short_description(obj),
            name, spell);
  }
  return error;
}

bool check_object_level(GameWorld *g, obj_data *obj, int val)
{
  if (obj_get_value(obj, val) >= 0)
    return false;
  mud_log("SYSERR: Object #{:d} ({}) has out of range level #{:d}.", obj_get_vnum(g, obj),
          obj_get_short_description(obj), obj_get_value(obj, val));
  return true;
}

void check_object(GameWorld *g, obj_data *obj)
{
  if (obj_get_weight(obj) < 0)
    mud_log("SYSERR: Object #{:d} ({}) has negative weight ({:d}).", obj_get_vnum(g, obj),
            obj_get_short_description(obj), obj_get_weight(obj));
  if (obj_get_cost_per_day(obj) < 0)
    mud_log("SYSERR: Object #{:d} ({}) has negative cost/day ({:d}).", obj_get_vnum(g, obj),
            obj_get_short_description(obj), obj_get_cost_per_day(obj));

  switch (obj_get_type_flag(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      if (obj_get_value(obj, 0) > 0 && obj_get_value(obj, 1) > obj_get_value(obj, 0))
        mud_log("SYSERR: Object #{:d} ({}) contains ({:d}) more than maximum ({:d}).", obj_get_vnum(g, obj),
                obj_get_short_description(obj), obj_get_value(obj, 1), obj_get_value(obj, 0));
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      check_object_level(g, obj, 0);
      check_object_spell_number(g, obj, 1);
      check_object_spell_number(g, obj, 2);
      check_object_spell_number(g, obj, 3);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      check_object_level(g, obj, 0);
      check_object_spell_number(g, obj, 3);
      if (obj_get_value(obj, 2) > obj_get_value(obj, 1))
        mud_log("SYSERR: Object #{:d} ({}) has more charges ({:d}) than maximum ({:d}).", obj_get_vnum(g, obj),
                obj_get_short_description(obj), obj_get_value(obj, 2), obj_get_value(obj, 1));
      break;
  }
}

/* The line after an A, S or Z: numbers, as many as out holds; at least `least` of them */
io::loaded<void> read_after(io::text_reader &in, obj_vnum vnum, char field, std::span<int> out, int least,
                            const char *expected)
{
  std::optional<std::string_view> line = in.line();
  if (!line)
    return std::unexpected(
        in.fail(str_format("object #{:d}: the file ends after its '{:c}', where {} was expected", vnum, field, expected)));
  int read = io::scan_ints(*line, out);
  if (read < least)
    return std::unexpected(
        in.fail(str_format("object #{:d}: after its '{:c}', expected {}, got '{}'", vnum, field, expected, *line)));
  return {};
}

io::loaded<void> read_object(GameWorld *g, io::text_reader &in, obj_vnum vnum)
{
  if (g->obj_index.find(vnum))
    return std::unexpected(in.fail(str_format("object #{:d} is defined twice", vnum)));
  g->obj_index.emplace(vnum);
  g->obj_proto.emplace(vnum);
  index_data *index = obj_index_of(g, vnum);
  index->vnum       = vnum;
  index->number     = 0;
  index->func       = nullptr;

  obj_data *obj = obj_proto_of(g, vnum);
  clear_object(obj);
  obj_set_item_number(obj, vnum);

  std::string what = str_format("object #{:d}", vnum);
  std::string text[4]; /* aliases, short, long and action descriptions */
  for (std::string &s : text) {
    io::loaded<std::string> read = in.tilde_string(what);
    if (!read)
      return std::unexpected(read.error());
    s = std::move(*read);
  }
  if (text[0].empty())
    return std::unexpected(in.fail(str_format("object #{:d} has no aliases", vnum)));
  rules::article_in_lowercase(text[1]);
  if (!text[2].empty())
    CAP(text[2]);
  obj_set_name(obj, text[0]);
  obj_set_short_description(obj, text[1]);
  obj_set_description(obj, text[2]);
  obj_set_action_description(obj, text[3]);

  /* its type and flags */
  std::optional<std::string_view> line = in.line();
  int type = 0;
  std::string flags[12];
  int read = 0;
  if (line) {
    io::fields f(*line);
    if (f.number(type))
      for (std::string &flag : flags)
        if (!f.word(flag))
          break;
    read = f.count();
  }
  if (read != 13)
    return std::unexpected(
        in.fail(str_format("object #{:d}: expected its type, 4 extra flags, 4 wear flags and 4 affect flags, got '{}'",
                           vnum, line.value_or(""))));
  for (int i = 0; i < 4; i++) {
    obj_get_extra_flags(obj)[i] = io::ascii_flags(flags[i], 0, "asciiflag_conv");
    obj_get_wear_flags(obj)[i]  = io::ascii_flags(flags[i + 4], 0, "asciiflag_conv");
    obj_get_bitvector(obj)[i]   = io::ascii_flags(flags[i + 8], 0, "asciiflag_conv");
  }
  obj_set_type_flag(obj, type);

  /* its values: as many as the line has, which may be none (quirk) */
  if (!(line = in.line()))
    return std::unexpected(in.fail(str_format("object #{:d}: the file ends before its values", vnum)));
  int values[NUM_OBJ_VAL_POSITIONS] = {};
  io::scan_ints(*line, values);
  for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
    obj_set_value(obj, i, values[i]);
  rules::portal_and_hatch_dcs(obj);
  rules::suntzu_weapon_types(obj);

  /* its weight, cost, rent and level, which may be left out */
  int t[4] = {};
  if (!(line = in.line()) || io::scan_ints(*line, t) < 3)
    return std::unexpected(in.fail(str_format("object #{:d}: expected its weight, cost, rent and level", vnum)));
  obj_set_weight(obj, t[0]);
  obj_set_cost(obj, t[1]);
  obj_set_cost_per_day(obj, t[2]);
  obj_set_level(obj, t[3]);
  obj_set_size(obj, SIZE_MEDIUM);
  rules::drink_container_weight(obj);
  rules::portal_timer(obj);

  for (int i = 0; i < MAX_OBJ_AFFECT; i++)
    obj_get_affected(obj)[i] = {.location = APPLY_NONE, .specific = 0, .modifier = 0};

  /* quirk: an A line with no specific has the one before it, or the rent if there is none before */
  int specific = t[2];
  /* quirk: the affects and the spellbook's spells count together */
  int slot = 0;
  for (;;) {
    io::text_reader::place before = in.here();
    if (!(line = in.line()))
      return std::unexpected(in.fail(str_format("object #{:d}: the file ends before the next '#<vnum>' or the '$'", vnum)));
    switch (line->empty() ? '\0' : (*line)[0]) {
      case 'E': {
        extra_descr_data descr;
        io::loaded<std::string> keyword = in.tilde_string(what);
        if (!keyword)
          return std::unexpected(keyword.error());
        io::loaded<std::string> description = in.tilde_string(what);
        if (!description)
          return std::unexpected(description.error());
        descr.keyword     = std::move(*keyword);
        descr.description = std::move(*description);
        obj_ex_description_slot(obj)->push_front(std::move(descr));
        break;
      }
      case 'A': {
        if (slot >= MAX_OBJ_AFFECT)
          return std::unexpected(in.fail(str_format("object #{:d} has more than {:d} affects", vnum, MAX_OBJ_AFFECT)));
        int a[3] = {0, 0, specific};
        if (io::loaded<void> after = read_after(in, vnum, 'A', a, 2, "its location, modifier and specific"); !after)
          return after;
        specific = a[2];
        if (a[0] >= APPLY_UNUSED3 && a[0] <= APPLY_UNUSED4)
          mud_log("Warning: object #{:d} ({}) uses deprecated saving throw applies", vnum,
                  obj_get_short_description(obj));
        obj_get_affected(obj)[slot] = {.location = a[0], .specific = affect_checked_specific(a[0], a[2]), .modifier = a[1]};
        slot++;
        break;
      }
      case 'S': {
        if (slot >= SPELLBOOK_SIZE)
          return std::unexpected(
              in.fail(str_format("object #{:d} has more than {:d} spells and affects", vnum, SPELLBOOK_SIZE)));
        int s[2] = {};
        if (io::loaded<void> after = read_after(in, vnum, 'S', s, 2, "its spell and pages"); !after)
          return after;
        obj_spellbook_spell &spell = obj_make_sbinfo(obj)[slot];
        spell.spellname            = s[0];
        spell.pages                = s[1];
        slot++;
        break;
      }
      case 'T':
        dg_attach_proto_trigger(g, *line, obj);
        break;
      case 'Z': {
        int z[1] = {};
        if (io::loaded<void> after = read_after(in, vnum, 'Z', z, 1, "its size"); !after)
          return after;
        obj_set_size(obj, z[0]);
        break;
      }
      case '$':
      case '#': /* the end of the object, and the start of what is next */
        in.go_back(before);
        rules::object_charm_cleared(obj, vnum);
        check_object(g, obj);
        return {};
      default:
        return std::unexpected(in.fail(str_format(
            "object #{:d}: expected E, A, S, T or Z, or the next '#<vnum>' or the '$', got '{}'", vnum, *line)));
    }
  }
}

void write_object(GameWorld *g, io::text_writer &out, obj_data *obj)
{
  std::string action = obj_get_action_description(obj) ? obj_get_action_description(obj) : "";
  strip_cr(action);
  auto or_undefined = [](const char *s) { return s && *s ? s : "undefined"; };
  out.print("#{:d}\n{}~\n{}~\n{}~\n{}~\n", obj_get_vnum(g, obj), or_undefined(obj_get_name(obj)),
            or_undefined(obj_get_short_description(obj)), or_undefined(obj_get_description(obj)), action);

  char flags[12][MAX_STRING_LENGTH];
  for (int i = 0; i < 4; i++) {
    sprintascii(flags[i], obj_get_extra_flags(obj)[i]);
    sprintascii(flags[i + 4], obj_get_wear_flags(obj)[i]);
    sprintascii(flags[i + 8], obj_get_bitvector(obj)[i]);
  }
  out.print("{:d}", obj_get_type_flag(obj));
  for (const char *flag : flags)
    out.print(" {}", flag);
  out.write("\n");
  for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
    out.print("{}{:d}", i ? " " : "", obj_get_value(obj, i));
  out.print("\n{:d} {:d} {:d} {:d}\n", obj_get_weight(obj), obj_get_cost(obj), obj_get_cost_per_day(obj),
            obj_get_level(obj));

  for (trig_vnum trigger : proto_script_of(obj))
    out.print("T {:d}\n", trigger);
  out.print("Z\n{:d}\n", obj_get_size(obj));

  for (const extra_descr_data &descr : obj_get_ex_description(obj)) {
    if (descr.keyword.empty() || descr.description.empty()) {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
      continue;
    }
    std::string text = descr.description;
    strip_cr(text);
    out.print("E\n{}~\n{}~\n", descr.keyword, text);
  }
  for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
    const obj_affected_type &af = obj_get_affected(obj)[i];
    if (af.modifier)
      out.print("A\n{:d} {:d} {:d}\n", af.location, af.modifier, af.specific);
  }
  if (obj_spellbook_spell *spells = obj_get_sbinfo(obj))
    for (int i = 0; i < SKILL_TABLE_SIZE && spells[i].spellname != 0; i++) /* quirk: SKILL_TABLE_SIZE, not SPELLBOOK_SIZE */
      out.print("S\n{:d} {:d}\n", spells[i].spellname, spells[i].pages);
}

} // namespace

io::loaded<void> read_object_file(GameWorld *g, io::text_reader &in)
{
  return read_records(in, "obj", [g](io::text_reader &r, int vnum) { return read_object(g, r, vnum); });
}

bool save_objects(GameWorld *g, struct zone_data *zone)
{
  if (!zone) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  io::text_writer out(str_format("{}{:d}.obj", OBJ_PREFIX, zone->number));
  for (obj_vnum vnum : g->obj_proto.vnums_in(genolc_zone_bottom(g, zone), zone->top))
    if (obj_data *obj = obj_proto_of(g, vnum))
      write_object(g, out, obj);
  out.write("$~\n");
  if (!out.commit()) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: OLC: Cannot open objects file {}!", out.path());
    return false;
  }

  if (in_save_list(g, zone->number, SL_OBJ)) {
    remove_from_save_list(g, zone->number, SL_OBJ);
    create_world_index(g, zone->number, "obj");
    mud_log("GenOLC: save_objects: Saving objects '{}'", out.path());
  }
  return true;
}
