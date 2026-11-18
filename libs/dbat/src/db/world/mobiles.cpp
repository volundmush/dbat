/*
 * The mob files, world/mob/<zone>.mob: one record per mob prototype.
 *
 *   #<vnum>
 *   <aliases>~
 *   <short description>~
 *   <long description>~
 *   <detailed description>~
 *   <act flags x4> <affect flags x4> <alignment> S|E
 *   <level> <hit roll> <armor> <power>d<ki>+<stamina> <damage dice>d<sides>+<unused>
 *   <gold> <unused> <race> <class>
 *   <position> <default position> <sex>
 *   <keyword>: <value>                               an E mob's especs, any number of them (see especs[])
 *   E                                                the end of an E mob's especs
 *   T <trigger vnum>                                 any number of them
 *   ...
 *   $
 */
#include <algorithm>
#include <optional>

#include "dbat/world_files.hpp"
#include "dbat/affect.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genmob.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/races.hpp"
#include "dbat/utils.hpp"

#include "load_rules.hpp"

namespace {

/* An E mob's "<keyword>: <value>" lines. read is given the value and the number it starts with (as atoi() reads
   it), and is null for a keyword that is accepted and ignored; write gives the value to save, or nothing to leave the
   line out, and is null for a keyword never written. */
struct espec_field {
  const char *keyword;
  void (*read)(GameWorld *g, char_data *mob, std::string_view value, int number);
  std::optional<int> (*write)(char_data *mob);
};

std::optional<int> unless_zero(int n) { return n != 0 ? std::optional<int>(n) : std::nullopt; }

const espec_field especs[] = {
    /* written, in this order */
    {"Size",
     [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_size(mob, std::clamp(n, SIZE_UNDEFINED, NUM_SIZES - 1)); },
     [](char_data *mob) {
       return get_size(mob) != race_get_size(char_get_race(mob)) ? std::optional<int>(get_size(mob)) : std::nullopt;
     }},
    {"BareHandAttack",
     [](GameWorld *, char_data *mob, std::string_view, int n) {
       char_set_mob_specials_attack_type(mob, std::clamp(n, 0, 99));
     },
     [](char_data *mob) { return unless_zero(char_get_mob_specials_attack_type(mob)); }},
    /* quirk: the abilities are read into the real ones and written from the affected ones */
    {"Str", [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_real_abils_str(mob, std::clamp(n, 0, 200)); },
     [](char_data *mob) { return unless_zero(char_get_aff_abils_str(mob)); }},
    {"Dex", [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_real_abils_dex(mob, std::clamp(n, 0, 200)); },
     [](char_data *mob) { return unless_zero(char_get_aff_abils_dex(mob)); }},
    {"Int",
     [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_real_abils_intel(mob, std::clamp(n, 0, 200)); },
     [](char_data *mob) { return unless_zero(char_get_aff_abils_intel(mob)); }},
    {"Wis", [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_real_abils_wis(mob, std::clamp(n, 0, 200)); },
     [](char_data *mob) { return unless_zero(char_get_aff_abils_wis(mob)); }},
    {"Con", [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_real_abils_con(mob, std::clamp(n, 0, 200)); },
     [](char_data *mob) { return unless_zero(char_get_aff_abils_con(mob)); }},
    {"Cha", [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_real_abils_cha(mob, std::clamp(n, 0, 200)); },
     [](char_data *mob) { return unless_zero(char_get_aff_abils_cha(mob)); }},

    /* read only */
    {"MaxHit", [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_max_hit(mob, std::clamp(n, 0, 99999)); },
     nullptr},
    {"MaxMana",
     [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_max_mana(mob, std::clamp(n, 0, 99999)); }, nullptr},
    {"MaxMoves",
     [](GameWorld *, char_data *mob, std::string_view, int n) { char_set_max_move(mob, std::clamp(n, 0, 99999)); }, nullptr},
    {"StrAdd",
     [](GameWorld *g, char_data *mob, std::string_view, int) {
       mud_log("mob #{:d} trying to set StrAdd, rebalance its strength.", char_get_mob_vnum(g, mob));
     },
     nullptr},
    {"Affect",
     [](GameWorld *g, char_data *mob, std::string_view value, int) {
       int n[6] = {};
       io::scan_ints(value, n);
       if (n[0] <= 0)
         return;
       affected_type af{};
       af.type      = n[0];
       af.duration  = n[1];
       af.modifier  = n[2];
       af.location  = n[3];
       af.bitvector = n[4];
       af.specific  = affect_checked_specific(n[3], n[5]);
       if (affect_is_loadable(&af))
         affect_to_char(g, mob, &af);
       else
         mud_log("SYSERR: mob {} has an impossible affect: {}", char_get_name(mob), value);
     },
     nullptr},

    /* accepted and ignored: an instance's points, and what NPCs no longer have */
    {"Hit", nullptr, nullptr},
    {"Mana", nullptr, nullptr},
    {"Moves", nullptr, nullptr},
    {"AffectV", nullptr, nullptr},
    {"Feat", nullptr, nullptr},
    {"Skill", nullptr, nullptr},
    {"SkillMod", nullptr, nullptr},
    {"Class", nullptr, nullptr},
    {"EpicClass", nullptr, nullptr},
};

/* "<keyword>: <value>"; a line with no ':' has no value, and no keyword takes one with none */
void read_espec(GameWorld *g, char_data *mob, mob_vnum vnum, std::string_view line)
{
  size_t colon = line.find(':');
  std::string_view keyword = line.substr(0, colon);
  if (colon != std::string_view::npos) {
    std::string_view value = line.substr(colon + 1);
    while (!value.empty() && isspace(static_cast<unsigned char>(value.front())))
      value.remove_prefix(1);
    for (const espec_field &e : especs)
      if (str_eq_nocase(keyword, e.keyword)) {
        if (e.read)
          e.read(g, mob, value, static_cast<int>(io::c_atol(value)));
        return;
      }
  }
  mud_log("SYSERR: Warning: unrecognized espec keyword {} in mob #{:d}", keyword, vnum);
}

/* The three lines every mob has after its flags */
io::loaded<void> read_mob_numbers(GameWorld *g, io::text_reader &in, char_data *mob, mob_vnum vnum)
{
  char_set_real_abils_str(mob, 0);
  char_set_real_abils_intel(mob, 0);
  char_set_real_abils_wis(mob, 0);
  char_set_real_abils_dex(mob, 0);
  char_set_real_abils_con(mob, 0);
  char_set_real_abils_cha(mob, 0);

  std::optional<std::string_view> line = in.line();
  int t[9]{};
  int read = 0;
  if (line) {
    io::fields f(*line);
    f.number(t[0]) && f.number(t[1]) && f.number(t[2]) && f.number(t[3]) && f.literal('d') && f.number(t[4]) &&
        f.literal('+') && f.number(t[5]) && f.number(t[6]) && f.literal('d') && f.number(t[7]) && f.literal('+') &&
        f.number(t[8]);
    read = f.count();
  }
  if (read != 9)
    return std::unexpected(in.fail(str_format(
        "mob #{:d}: expected its level, hit roll, armor, points and damage, as '# # # #d#+# #d#+#'", vnum)));
  char_set_race_level(mob, t[0]);
  char_set_level_adj(mob, 0);
  char_set_class_level(mob, 0);
  char_set_armor(mob, 10 * (10 - t[2])); /* quirk: the hit roll, t[1], is written but never read */
  char_set_basepl(mob, t[3]);
  char_set_baseki(mob, t[4]);
  char_set_basest(mob, t[5]);
  char_set_health(mob, 1.0);
  char_set_energy(mob, 1.0);
  char_set_stamina(mob, 1.0);
  char_set_mob_specials_damnodice(mob, t[6]);
  char_set_mob_specials_damsizedice(mob, t[7]); /* t[8], the damage bonus, is unused */

  if (!(line = in.line()) || io::scan_ints(*line, std::span(t, 4)) != 4)
    return std::unexpected(
        in.fail(str_format("mob #{:d}: expected its gold, experience, race and class, as '# # # #'", vnum)));
  char_set_gold(mob, t[0]);
  char_set_exp(mob, 0);
  char_set_race(mob, t[2]);
  char_set_class(mob, t[3]);
  rules::mob_class_in_range(mob, vnum);
  rules::infravision_unless_human(mob);
  char_set_speaking(mob, SKILL_LANG_COMMON);

  if (!(line = in.line()) || io::scan_ints(*line, std::span(t, 3)) != 3)
    return std::unexpected(
        in.fail(str_format("mob #{:d}: expected its position, default position and sex, as '# # #'", vnum)));
  char_set_position(mob, t[0]);
  char_set_mob_specials_default_pos(mob, t[1]);
  char_set_sex(mob, t[2]);

  char_set_speaking(mob, MIN_LANGUAGES);
  set_height_and_weight_by_race(g, mob);
  rules::autobalance_starts_bare(mob);
  return {};
}

io::loaded<void> read_especs(GameWorld *g, io::text_reader &in, char_data *mob, mob_vnum vnum)
{
  while (std::optional<std::string_view> line = in.line()) {
    if (*line == "E")
      return {};
    if (line->starts_with('#'))
      return std::unexpected(in.fail(str_format("mob #{:d}: its especs have no 'E' to end them", vnum)));
    read_espec(g, mob, vnum, *line);
  }
  return std::unexpected(in.fail(str_format("mob #{:d}: the file ends before the 'E' that ends its especs", vnum)));
}

io::loaded<void> read_mob(GameWorld *g, io::text_reader &in, mob_vnum vnum)
{
  if (g->mob_index.find(vnum))
    return std::unexpected(in.fail(str_format("mob #{:d} is defined twice", vnum)));
  g->mob_index.emplace(vnum);
  g->mob_proto.emplace(vnum);
  index_data *index = mob_index_of(g, vnum);
  index->vnum       = vnum;
  index->number     = 0;
  index->func       = nullptr;

  char_data *mob = mob_proto_of(g, vnum);
  clear_char(mob);
  char_set_nr(mob, vnum);
  char_set_desc(mob, nullptr);
  char_drop_player_specials(mob); /* no player_specials is what makes a character an NPC */

  std::string what = str_format("mob vnum {:d}", vnum);
  std::string text[4]; /* aliases, short, long and detailed descriptions */
  for (std::string &s : text) {
    io::loaded<std::string> read = in.tilde_string(what);
    if (!read)
      return std::unexpected(read.error());
    s = std::move(*read);
  }
  rules::article_in_lowercase(text[1]);
  char_set_alias(mob, text[0]);
  char_set_sdesc(mob, text[1]);
  char_set_ldesc(mob, text[2]);
  char_set_ddesc(mob, text[3]);

  std::optional<std::string_view> line = in.line();
  std::string flags[8], kind;
  int alignment = 0;
  int read      = 0;
  if (line) {
    io::fields f(*line);
    for (std::string &flag : flags)
      if (!f.word(flag, 127))
        break;
    f.number(alignment) && f.word(kind, 1);
    read = f.count();
  }
  if (read != 10)
    return std::unexpected(
        in.fail(str_format("mob #{:d}: expected 4 act flags, 4 affect flags, its alignment and S or E, got '{}'", vnum,
                           line.value_or(""))));
  for (int i = 0; i < 4; i++) {
    char_get_act(mob)[i]         = io::ascii_flags(flags[i], 0, "asciiflag_conv");
    char_get_affected_by(mob)[i] = io::ascii_flags(flags[i + 4], 0, "asciiflag_conv");
  }
  char_set_alignment(mob, alignment);
  char_mob_set(mob, MOB_ISNPC, true);
  rules::not_dead_yet_cleared(mob, vnum);

  switch (toupper(static_cast<unsigned char>(kind[0]))) {
    case 'S': /* a simple mob */
      if (io::loaded<void> numbers = read_mob_numbers(g, in, mob, vnum); !numbers)
        return numbers;
      break;
    case 'E': /* an enhanced one, with especs */
      if (io::loaded<void> numbers = read_mob_numbers(g, in, mob, vnum); !numbers)
        return numbers;
      if (io::loaded<void> e = read_especs(g, in, mob, vnum); !e)
        return e;
      mob_stats(g, mob);
      break;
    default:
      return std::unexpected(in.fail(str_format("mob #{:d}: there is no mob type '{}', only S and E", vnum, kind)));
  }

  while (in.peek_letter() == 'T')
    dg_attach_proto_trigger(g, in.line().value_or(""), mob);

  char_reset_aff_abils(mob);
  for (int i = 0; i < NUM_WEARS; i++)
    char_set_equipment(mob, i, nullptr);
  return {};
}

void write_mob(GameWorld *g, io::text_writer &out, mob_vnum vnum, char_data *mob)
{
  check_mobile_strings(g, mob);

  std::string ldesc = char_get_ldesc(mob);
  strip_cr(ldesc);
  std::string ddesc = char_get_ddesc(mob);
  strip_cr(ddesc);
  out.print("#{:d}\n{}~\n{}~\n{}~\n{}~\n", vnum, char_get_alias(mob), char_get_sdesc(mob), ldesc, ddesc);

  char flags[8][MAX_STRING_LENGTH];
  for (int i = 0; i < 4; i++) {
    sprintascii(flags[i], char_get_act(mob)[i]);
    sprintascii(flags[i + 4], char_get_affected_by(mob)[i]);
  }
  out.print("{} {} {} {} {} {} {} {} {:d} E\n"
            "{:d} {:d} {:d} {:d}d{:d}+{:d} {:d}d{:d}+{:d}\n"
            "{:d} 0 {:d} {:d}\n"
            "{:d} {:d} {:d}\n",
            flags[0], flags[1], flags[2], flags[3], flags[4], flags[5], flags[6], flags[7], char_get_alignment(mob),
            char_get_race_level(mob), char_get_accuracy_mod(mob), 10 - (char_get_armor(mob) / 10), getCurPL(g, mob),
            getCurKI(g, mob), getCurST(g, mob), char_get_mob_specials_damnodice(mob),
            char_get_mob_specials_damsizedice(mob), 0 /* the unused damage bonus */, char_get_gold(mob),
            char_get_race(mob), char_get_class(mob), char_get_position(mob), char_get_mob_specials_default_pos(mob),
            char_get_sex(mob));

  for (const espec_field &e : especs)
    if (e.write)
      if (std::optional<int> value = e.write(mob))
        out.print("{}: {:d}\n", e.keyword, *value);
  out.write("E\n");
  for (trig_vnum trigger : proto_script_of(mob))
    out.print("T {:d}\n", trigger);
}

} // namespace

void check_mobile_strings(GameWorld *g, struct char_data *mob)
{
  mob_vnum vnum = mob_index_of(g, char_get_nr(mob))->vnum;
  auto check    = [&](std::string *string, const char *what) {
    if (string->empty()) {
      mudlog(g, LOG_BRF, ADMLVL_GOD, true, "GenOLC: Mob #{:d} has an invalid {}.", vnum, what);
      *string = "An undefined string.";
    }
  };
  check(char_long_descr_slot(mob), "long description");
  check(char_description_slot(mob), "detailed description");
  check(char_name_slot(mob), "alias list");
  check(char_short_descr_slot(mob), "short description");
}

io::loaded<void> read_mob_file(GameWorld *g, io::text_reader &in)
{
  return read_records(in, "mob", [g](io::text_reader &r, int vnum) { return read_mob(g, r, vnum); });
}

bool save_mobiles(GameWorld *g, struct zone_data *zone)
{
  if (!zone) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  io::text_writer out(str_format("{}{:d}.mob", MOB_PREFIX, zone->number));
  for (mob_vnum vnum : g->mob_proto.vnums_in(genolc_zone_bottom(g, zone), zone->top))
    if (char_data *mob = mob_proto_of(g, vnum))
      write_mob(g, out, vnum, mob);
  out.write("$\n");
  if (!out.commit()) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return false;
  }

  if (in_save_list(g, zone->number, SL_MOB)) {
    remove_from_save_list(g, zone->number, SL_MOB);
    create_world_index(g, zone->number, "mob");
    mud_log("GenOLC: save_mobiles: Saving mobiles '{}'", out.path());
  }
  return true;
}
