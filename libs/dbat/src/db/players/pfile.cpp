/*
 * Player files, plrfiles/<bucket>/<name>.plr: a line for each thing kept about a character, "<tag>: <value>",
 * where the tag is 4 characters. A few tags are followed by a list or a string on the lines after them (Desc, Skil,
 * SklB, Bonu, Affs). Most are written only when they differ from their default (dbat/pfdefaults.hpp).
 *
 * pfile_tags[] is every tag: how it is read and how it is written, in the order the file has them, then the tags
 * only older files have.
 */
#include <array>
#include <algorithm>
#include <optional>
#include <span>
#include <stdckdint.h>
#include <string>
#include <string_view>

#include "dbat/players.hpp"
#include "dbat/affect.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/class.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fileop.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/pfdefaults.hpp"
#include "dbat/utils.hpp"

namespace {

/* The state a player file is read with. Some of it lasts from one line to the next, as the old reader's buffers
   did (quirk): a flags line short of its four words keeps the words of the flags line before it, whichever kind it
   was, and "SkCl", "Tcos", "Ecls" and "Mcls" share their numbers. */
struct pfile_reader {
  GameWorld *g;
  char_data *ch;
  io::text_reader &in;
  std::string_view value{}; /* the line after its tag */
  std::optional<io::load_error> error{};
  std::string flag_words[4]{};
  int num = 0, num2 = 0, num3 = 0;

  int as_int() const { return static_cast<int>(io::c_atol(value)); }
  long long as_llong() const { return io::c_atoll(value); }
  /* sscanf("%lld"): 0 if there is no number */
  long long number64() const
  {
    long long n = 0;
    io::fields(value).number(n);
    return n;
  }
  /* four flags words */
  void flags(bitvector_t *bits)
  {
    io::fields f(value);
    for (std::string &word : flag_words)
      if (!f.word(word, 127))
        break;
    for (int i = 0; i < 4; i++)
      bits[i] = io::ascii_flags(flag_words[i], 0, "asciiflag_conv");
  }
  template <flag_family E> void flags(flag_set<E> &set)
  {
    std::array<bitvector_t, FLAG_WORDS> bits{};
    flags(bits.data());
    set = flag_set<E>::from_words(bits);
  }
  /* "Hit : <current>/<max>" and the like: the max, or 0 if the line doesn't have both */
  long long current_and_max() const
  {
    long long current = 0, max = 0;
    io::fields f(value);
    if (!(f.number(current) && f.literal('/') && f.number(max)))
      max = 0;
    return max;
  }
  /* the next line of a list, as get_line() read it */
  std::optional<std::string_view> list_line() { return in.line(); }
};

struct pfile_writer {
  GameWorld *g;
  char_data *ch;
  io::text_writer &out;
  std::span<const affected_type> affects; /* as they were before they were taken off to save */

  template <class V> void number(std::string_view tag, V value) { out.print("{}: {:d}\n", tag, value); }
  template <class V, class D> void unless(std::string_view tag, V value, D fallback)
  {
    if (value != fallback)
      number(tag, value);
  }
  void text(std::string_view tag, std::string_view s) { out.print("{}: {}\n", tag, s); }
  /* no line for an empty field, which reads back as empty all the same */
  void text_unless_empty(std::string_view tag, std::string_view s)
  {
    if (!s.empty())
      text(tag, s);
  }
  void flags(std::string_view tag, const bitvector_t *bits)
  {
    out.print("{}: {} {} {} {}\n", tag, flag_letters(bits[0]), flag_letters(bits[1]), flag_letters(bits[2]), flag_letters(bits[3]));
  }
  template <flag_family E> void flags(std::string_view tag, const flag_set<E> &set) { flags(tag, set.words().data()); }
};

/* "\r\r" as one "\r", as the old kill_ems() left a description */
std::string kill_ems(std::string_view str)
{
  std::string out;
  for (size_t i = 0; i < str.size(); ++i) {
    out += str[i];
    if (str[i] == '\r' && i + 1 < str.size() && str[i + 1] == '\r')
      ++i; /* "\r\r" keeps one */
  }
  return out;
}

/* A list of skills, "<skill> <value> <perfection>" a line, up to one whose skill is 0 (quirk: whose perfection, if
   the line leaves it out, is the line before's, and is given to skill 0) */
void read_skills(pfile_reader &r, bool mods)
{
  int num = 0, num2 = 0, num3 = 0;
  do {
    std::optional<std::string_view> line = r.list_line();
    if (!line)
      break;
    io::fields f(*line);
    f.number(num) && f.number(num2) && f.number(num3);
    if (num > -1 && num < SKILL_TABLE_SIZE) {
      if (mods)
        char_set_skills_mod(r.ch, num, num2);
      else
        char_set_skills_base(r.ch, num, num2);
      char_set_skills_perf(r.ch, num, num3);
    }
  } while (num != 0);
}

void write_skills(pfile_writer &w, std::string_view tag, bool mods)
{
  if (char_get_admlevel(w.ch) >= ADMLVL_IMMORT)
    return;
  w.out.print("{}:\n", tag);
  for (int i = 0; i < SKILL_TABLE_SIZE; i++) {
    int value = mods ? char_get_skills_mod(w.ch, i) : char_get_skills_base(w.ch, i);
    if (value)
      w.out.print("{:d} {:d} {:d}\n", i, value, char_get_skills_perf(w.ch, i));
  }
  w.out.write("0 0\n");
}

void read_affects(pfile_reader &r)
{
  int num = 0;
  do {
    std::optional<std::string_view> line = r.list_line();
    if (!line)
      break;
    int n[6] = {};
    io::fields f(*line);
    for (int &x : n)
      if (!f.number(x))
        break;
    num = n[0];
    if (num != 0) {
      affected_type af{};
      af.type      = n[0];
      af.duration  = n[1];
      af.modifier  = n[2];
      af.location  = n[3];
      std::optional<aff_flag> bit = flag_from_index<aff_flag>(n[4]); /* a number past the affects isn't loaded */
      af.bitvector                = bit.value_or(AFF_DONTUSE);
      af.specific                 = affect_checked_specific(n[3], n[5]);
      if (bit && affect_is_loadable(&af))
        affect_to_char(r.g, r.ch, &af);
      else
        mud_log("SYSERR: {} has an impossible saved affect: {}", char_get_name(r.ch), *line);
    }
  } while (num != 0);
}

using R = pfile_reader;
using W = pfile_writer;

struct pfile_tag {
  std::string_view tag{};
  void (*read)(R &);
  void (*write)(W &, std::string_view tag); /* null for tags only read, from older files */
};

constexpr bool ascii_save_poofs = true; /* quirk: ASCII_SAVE_POOFS was "false", but tested with #ifdef */

/* In the order they are written */
constexpr pfile_tag pfile_tags[] = {
    {"Name", [](R &r) { char_set_pc_name(r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { w.text_unless_empty(t, char_get_name(w.ch)); }},
    {"User", [](R &r) { char_set_loguser(r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { w.text(t, char_get_user(w.ch)); }},
    {"Voic", [](R &r) { char_set_voice(r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { w.text_unless_empty(t, char_get_voice(w.ch)); }},
    {"RPfe", [](R &r) { char_set_feature(r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { w.text_unless_empty(t, char_get_feature(w.ch)); }},
    {"Trns", [](R &r) { char_set_ability_trains(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_ability_trains(w.g, w.ch), 0); }},
    {"Desc",
     [](R &r) {
       io::loaded<std::string> text = r.in.tilde_string("its description");
       if (!text)
         r.error = text.error();
       else
         char_set_ddesc(r.ch, *text);
     },
     [](W &w, std::string_view) {
       if (!char_get_ddesc(w.ch).empty())
         w.out.print("Desc:\n{}~\n", kill_ems(char_get_ddesc(w.ch)));
     }},
    {"PfIn", [](R &r) { if (ascii_save_poofs) char_set_poofin(r.g, r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { if (ascii_save_poofs) w.text_unless_empty(t, char_get_poofin(w.g, w.ch)); }},
    {"PfOt", [](R &r) { if (ascii_save_poofs) char_set_poofout(r.g, r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { if (ascii_save_poofs) w.text_unless_empty(t, char_get_poofout(w.g, w.ch)); }},
    {"Sex ", [](R &r) { char_set_sex(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_sex(w.ch), PFDEF_SEX); }},
    {"Size", [](R &r) { char_set_size(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_size(w.ch), PFDEF_SIZE); }},
    {"Clas", [](R &r) { char_set_class(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_class(w.ch), PFDEF_CLASS); }},
    {"Race", [](R &r) { char_set_race(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_race(w.ch), PFDEF_RACE); }},
    {"Raci", [](R &r) { char_set_racial_pref(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_racial_pref(w.ch), PFDEF_BANK); }},
    {"AdmL", [](R &r) { char_set_admlevel(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_admlevel(w.ch), PFDEF_LEVEL); }},
    {"Levl", [](R &r) { char_set_class_level(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_class_level(w.ch), PFDEF_LEVEL); }},
    {"HitD", [](R &r) { char_set_race_level(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_race_level(w.ch), PFDEF_LEVEL); }},
    {"LvlA", [](R &r) { char_set_level_adj(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_level_adj(w.ch), PFDEF_LEVEL); }},
    {"Home", [](R &r) { char_set_hometown(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_hometown(w.ch), PFDEF_HOMETOWN); }},
    {"Id  ", [](R &r) { char_set_idnum(r.ch, static_cast<int32_t>(io::c_atol(r.value))); }, [](W &w, std::string_view t) { w.number(t, char_get_idnum(w.ch)); }},
    {"Brth", [](R &r) { char_set_time_birth(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.number(t, static_cast<long long>(char_get_time_birth(w.ch))); }},
    {"Crtd", [](R &r) { char_set_time_created(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.number(t, static_cast<long long>(char_get_time_created(w.ch))); }},
    {"MxAg", [](R &r) { char_set_time_maxage(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.number(t, static_cast<long long>(char_get_time_maxage(w.ch))); }},
    {"Plyd", [](R &r) { char_set_time_played(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.number(t, static_cast<long long>(char_get_time_played(w.ch))); }},
    {"Last", [](R &r) { char_set_time_logon(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.number(t, static_cast<long long>(char_get_time_logon(w.ch))); }},
    {"Host", [](R &r) { char_set_host(r.g, r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { w.text_unless_empty(t, char_get_host(w.g, w.ch)); }},
    {"Hite", [](R &r) { char_set_height(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_height(w.ch), PFDEF_HEIGHT); }},
    {"Wate", [](R &r) { char_set_weight(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_weight(w.ch), PFDEF_HEIGHT); }}, /* quirk: the height's default */
    {"Alin", [](R &r) { char_set_alignment(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_alignment(w.ch), PFDEF_ALIGNMENT); }},
    {"Aura", [](R &r) { char_set_aura(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aura(w.ch), PFDEF_SKIN); }},
    {"Act ", [](R &r) { r.flags(char_get_player_flags(r.g, r.ch)); }, [](W &w, std::string_view t) { w.flags(t, char_get_player_flags(w.g, w.ch)); }},
    {"Aff ", [](R &r) { r.flags(char_get_affected_by(r.ch)); }, [](W &w, std::string_view t) { w.flags(t, char_get_affected_by(w.ch)); }},
    {"Pref", [](R &r) { r.flags(char_get_pref(r.g, r.ch)); }, [](W &w, std::string_view t) { w.flags(t, char_get_pref(w.g, w.ch)); }},
    {"AdmF", [](R &r) { r.flags(char_get_admflags(r.ch)); }, [](W &w, std::string_view t) { w.flags(t, char_get_admflags(w.ch)); }},
    {"Wimp", [](R &r) { char_set_wimp_level(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_wimp_level(w.g, w.ch), PFDEF_WIMPLEV); }},
    {"PwrA", [](R &r) { char_set_powerattack(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_powerattack(w.ch), PFDEF_POWERATT); }},
    {"Frez", [](R &r) { char_set_freeze_level(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_freeze_level(w.g, w.ch), PFDEF_FREEZELEV); }},
    {"Invs", [](R &r) { char_set_invis_level(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_invis_level(w.g, w.ch), PFDEF_INVISLEV); }},
    {"Room", [](R &r) { char_set_load_room(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_load_room(w.g, w.ch), PFDEF_LOADROOM); }},
    {"Badp", [](R &r) { char_set_bad_pws(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_bad_pws(w.g, w.ch), PFDEF_BADPWS); }},
    {"Tcos",
     [](R &r) {
       io::fields f(r.value);
       if (f.number(r.num2) && f.number(r.num3) && r.num2 >= 0 && r.num2 < 6)
         char_set_transcost(r.ch, r.num2, r.num3);
     },
     [](W &w, std::string_view t) {
       for (int i = 0; i < 6; i++)
         if (char_get_transcost(w.ch, i) != false)
           w.out.print("{}: {:d} {:d}\n", t, i, char_get_transcost(w.ch, i));
     }},
    {"SkCl",
     [](R &r) {
       io::fields f(r.value);
       f.number(r.num2) && f.number(r.num3);
       char_set_practice_points(r.g, r.ch, r.num3);
     },
     [](W &w, std::string_view t) {
       for (int i = 0; i < NUM_CLASSES; i++)
         if (char_get_practice_points(w.g, w.ch) != PFDEF_PRACTICES)
           w.out.print("{}: {:d} {:d}\n", t, i, char_get_practice_points(w.g, w.ch));
     }},
    {"Hung", [](R &r) { char_set_conditions(r.g, r.ch, HUNGER, r.as_int()); },
     [](W &w, std::string_view t) { if (char_get_admlevel(w.ch) < ADMLVL_IMMORT) w.unless(t, char_get_conditions(w.g, w.ch, HUNGER), PFDEF_HUNGER); }},
    {"Thir", [](R &r) { char_set_conditions(r.g, r.ch, THIRST, r.as_int()); },
     [](W &w, std::string_view t) { if (char_get_admlevel(w.ch) < ADMLVL_IMMORT) w.unless(t, char_get_conditions(w.g, w.ch, THIRST), PFDEF_THIRST); }},
    {"Drnk", [](R &r) { char_set_conditions(r.g, r.ch, DRUNK, r.as_int()); },
     [](W &w, std::string_view t) { if (char_get_admlevel(w.ch) < ADMLVL_IMMORT) w.unless(t, char_get_conditions(w.g, w.ch, DRUNK), PFDEF_DRUNK); }},
    {"Str ", [](R &r) { char_set_real_abils_str(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aff_abils_str(w.ch), PFDEF_STR); }},
    {"Int ", [](R &r) { char_set_real_abils_intel(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aff_abils_intel(w.ch), PFDEF_INT); }},
    {"Wis ", [](R &r) { char_set_real_abils_wis(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aff_abils_wis(w.ch), PFDEF_WIS); }},
    {"Dex ", [](R &r) { char_set_real_abils_dex(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aff_abils_dex(w.ch), PFDEF_DEX); }},
    {"Con ", [](R &r) { char_set_real_abils_con(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aff_abils_con(w.ch), PFDEF_CON); }},
    {"Cha ", [](R &r) { char_set_real_abils_cha(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_aff_abils_cha(w.ch), PFDEF_CHA); }},
    {"Cool", [](R &r) { char_set_con_cooldown(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_con_cooldown(w.ch), PFDEF_BANK); }},
    {"Scoo", [](R &r) { char_set_con_sdcooldown(r.ch, r.as_int()); },
     [](W &w, std::string_view t) { if (char_get_con_cooldown(w.ch) != PFDEF_BANK) w.number(t, char_get_con_sdcooldown(w.ch)); }}, /* quirk: written when the other cooldown is set */
    {"Ac  ", [](R &r) { char_set_armor(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_armor(w.ch), PFDEF_AC); }},
    {"Abso", [](R &r) { char_set_absorbs(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_absorbs(w.ch), PFDEF_GOLD); }},
    {"INGl", [](R &r) { char_set_ingestLearned(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_ingestLearned(w.ch), PFDEF_GOLD); }},
    {"Upgr", [](R &r) { char_set_upgrade(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_upgrade(w.ch), PFDEF_GOLD); }},
    {"Forg", [](R &r) { char_set_forgeting(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_forgeting(w.ch), PFDEF_BANK); }},
    {"Forc", [](R &r) { char_set_forgetcount(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_forgetcount(w.ch), PFDEF_BANK); }},
    {"Kaio", [](R &r) { char_set_kaioken(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_kaioken(w.ch), PFDEF_GOLD); }},
    {"Gold", [](R &r) { char_set_gold(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_gold(w.ch), PFDEF_GOLD); }},
    {"Bank", [](R &r) { char_set_bank_gold(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_bank_gold(w.ch), PFDEF_BANK); }},
    {"Exp ", [](R &r) { char_set_exp(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, char_get_exp(w.ch), PFDEF_EXP); }},
    {"Tcla", [](R &r) { char_set_transclass(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_transclass(w.ch), PFDEF_EXP); }},
    {"Mexp", [](R &r) { char_set_moltexp(r.ch, r.number64()); }, [](W &w, std::string_view t) { w.unless(t, char_get_moltexp(w.ch), PFDEF_EXP); }},
    {"Majm", [](R &r) { char_set_majinizer(r.ch, r.number64()); }, [](W &w, std::string_view t) { w.unless(t, char_get_majinizer(w.ch), PFDEF_EXP); }},
    {"Mlvl", [](R &r) { char_set_moltlevel(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_moltlevel(w.ch), PFDEF_EXP); }},
    {"Fisd", [](R &r) { char_set_accuracy_mod(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_accuracy_mod(w.ch), PFDEF_ACCURACY); }},
    {"Pole", [](R &r) { char_set_accuracy(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_accuracy(w.ch), PFDEF_ACCURACY); }},
    {"Prff", [](R &r) { char_set_preference(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_preference(w.ch), PFDEF_EYE); }},
    {"Spek", [](R &r) { char_set_speaking(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_speaking(w.ch), PFDEF_SPEAKING); }},
    {"Olc ", [](R &r) { char_set_olc_zone(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_olc_zone(w.g, w.ch), PFDEF_OLC); }},
    {"Gaun", [](R &r) { char_set_gauntlet(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_gauntlet(w.g, w.ch), PFDEF_GAUNTLET); }},
    {"Geno", [](R &r) { char_set_genome(r.ch, 0, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_genome(w.ch, 0), PFDEF_EYE); }},
    {"Gen1", [](R &r) { char_set_genome(r.ch, 1, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_genome(w.ch, 1), PFDEF_EYE); }},
    {"Posi", [](R &r) { char_set_position(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_position(w.ch), POS_STANDING); }},
    {"LFPC", [](R &r) { char_set_lifeperc(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_lifeperc(w.ch), PFDEF_WEIGHT); }},
    {"Bpl ", [](R &r) { char_set_basepl(r.ch, r.number64()); }, [](W &w, std::string_view t) { w.unless(t, getBasePL(w.ch), PFDEF_BASEPL); }},
    {"Bki ", [](R &r) { char_set_baseki(r.ch, r.number64()); }, [](W &w, std::string_view t) { w.unless(t, getBaseKI(w.ch), PFDEF_BASEKI); }},
    {"Bst ", [](R &r) { char_set_basest(r.ch, r.number64()); }, [](W &w, std::string_view t) { w.unless(t, getBaseST(w.ch), PFDEF_BASEST); }},
    {"Droo", [](R &r) { char_set_droom(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_droom(w.ch), PFDEF_DROOM); }},
    {"Hrl ", [](R &r) { char_set_hairl(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_hairl(w.ch), PFDEF_HAIRL); }},
    {"Hrs ", [](R &r) { char_set_hairs(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_hairs(w.ch), PFDEF_HAIRS); }},
    {"Hrc ", [](R &r) { char_set_hairc(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_hairc(w.ch), PFDEF_HAIRC); }},
    {"Skn ", [](R &r) { char_set_skin(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_skin(w.ch), PFDEF_SKIN); }},
    {"Eye ", [](R &r) { char_set_eye(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_eye(w.ch), PFDEF_EYE); }},
    {"Phas", [](R &r) { char_set_distfea(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_distfea(w.ch), PFDEF_DISTFEA); }},
    {"Fury", [](R &r) { char_set_fury(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_fury(w.ch), PFDEF_HAIRL); }},
    {"Rad1", [](R &r) { char_set_radar1(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_radar1(w.ch), PFDEF_RADAR1); }},
    {"Rad2", [](R &r) { char_set_radar2(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_radar2(w.ch), PFDEF_RADAR2); }},
    {"Rad3", [](R &r) { char_set_radar3(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_radar3(w.ch), PFDEF_RADAR3); }},
    {"Lpla", [](R &r) { char_set_lastpl(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lastpl(w.ch)), PFDEF_LPLAY); }},
    {"Lint", [](R &r) { char_set_lastint(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lastint(w.ch)), PFDEF_LPLAY); }},
    {"Deat", [](R &r) { char_set_deathtime(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_deathtime(w.ch)), PFDEF_LPLAY); }},
    {"Rtim", [](R &r) { char_set_rewtime(r.ch, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_rewtime(w.ch)), PFDEF_LPLAY); }},
    {"Boos", [](R &r) { char_set_boosts(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_boosts(w.ch), PFDEF_DISTFEA); }},
    {"Maji", [](R &r) { char_set_majinize(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_majinize(w.ch), PFDEF_LPLAY); }},
    {"Blss", [](R &r) { char_set_blesslvl(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_blesslvl(w.ch), PFDEF_HEIGHT); }},
    {"Boam", [](R &r) { char_set_lboard(r.ch, 0, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lboard(w.ch, 0)), PFDEF_BOARD); }},
    {"Boai", [](R &r) { char_set_lboard(r.ch, 1, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lboard(w.ch, 1)), PFDEF_BOARD); }},
    {"Boac", [](R &r) { char_set_lboard(r.ch, 2, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lboard(w.ch, 2)), PFDEF_BOARD); }},
    {"Boad", [](R &r) { char_set_lboard(r.ch, 3, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lboard(w.ch, 3)), PFDEF_BOARD); }},
    {"Boab", [](R &r) { char_set_lboard(r.ch, 4, r.as_llong()); }, [](W &w, std::string_view t) { w.unless(t, static_cast<long long>(char_get_lboard(w.ch, 4)), PFDEF_BOARD); }},
    {"Lira", [](R &r) { char_set_limb_condition(r.ch, 0, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_limb_condition(w.ch, 0), PFDEF_BOARD); }},
    {"Lila", [](R &r) { char_set_limb_condition(r.ch, 1, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_limb_condition(w.ch, 1), PFDEF_BOARD); }},
    {"Lirl", [](R &r) { char_set_limb_condition(r.ch, 2, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_limb_condition(w.ch, 2), PFDEF_BOARD); }},
    {"Lill", [](R &r) { char_set_limb_condition(r.ch, 3, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_limb_condition(w.ch, 3), PFDEF_BOARD); }},
    {"RPP ", [](R &r) { char_set_rp(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_rp(w.ch), PFDEF_SKIN); }},
    {"RBan", [](R &r) { char_set_rbank(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_rbank(w.ch), PFDEF_SKIN); }},
    {"Supp", [](R &r) { char_set_suppression(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_suppression(w.ch), PFDEF_SKIN); }},
    {"Sups", [](R &r) { char_set_suppressed(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_suppressed(w.ch), PFDEF_SKIN); }},
    {"Trp ", [](R &r) { char_set_trp(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_trp(w.ch), PFDEF_SKIN); }},
    {"Deac", [](R &r) { char_set_dcount(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_dcount(w.ch), PFDEF_EYE); }},
    {"Trag", [](R &r) { char_set_trainagl(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_trainagl(w.g, w.ch), PFDEF_EYE); }},
    {"Trco", [](R &r) { char_set_traincon(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_traincon(w.g, w.ch), PFDEF_EYE); }},
    {"Trin", [](R &r) { char_set_trainint(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_trainint(w.g, w.ch), PFDEF_EYE); }},
    {"Trsp", [](R &r) { char_set_trainspd(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_trainspd(w.g, w.ch), PFDEF_EYE); }},
    {"Trst", [](R &r) { char_set_trainstr(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_trainstr(w.g, w.ch), PFDEF_EYE); }},
    {"Trwi", [](R &r) { char_set_trainwis(r.g, r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_trainwis(w.g, w.ch), PFDEF_EYE); }},
    {"Phse", [](R &r) { char_set_starphase(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_starphase(w.ch), PFDEF_EYE); }},
    {"Mimi", [](R &r) { char_set_mimic(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_mimic(w.ch), 0); }},
    {"Slot", [](R &r) { char_set_skill_slots(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_skill_slots(w.ch), PFDEF_EYE); }},
    {"Tgro", [](R &r) { char_set_tail_growth(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_tail_growth(w.ch), PFDEF_EYE); }},
    {"rDis", [](R &r) { char_set_rdisplay(r.ch, std::string(r.value)); }, [](W &w, std::string_view t) { w.text_unless_empty(t, char_get_rdisplay(w.ch)); }},
    {"Rela", [](R &r) { char_set_relax_count(r.ch, r.as_int()); }, [](W &w, std::string_view t) { w.unless(t, char_get_relax_count(w.ch), PFDEF_EYE); }},
    {"Skil", [](R &r) { read_skills(r, false); }, [](W &w, std::string_view t) { write_skills(w, t, false); }},
    {"Bonu",
     [](R &r) {
       int num[52] = {};
       if (std::optional<std::string_view> line = r.list_line()) {
         io::fields f(*line);
         for (int &n : num)
           if (!f.number(n))
             break;
       }
       for (int i = 0; i < 52; i++)
         if (num[i] > 0)
           char_set_bonuses(r.ch, i, num[i]);
     },
     [](W &w, std::string_view t) {
       w.out.print("{}:\n", t);
       for (int i = 0; i < 52; i++)
         w.out.print("{}{:d}", i ? " " : "", char_get_bonuses(w.ch, i));
       w.out.write("\n");
     }},
    {"SklB", [](R &r) { read_skills(r, true); }, [](W &w, std::string_view t) { write_skills(w, t, true); }},
    {"Affs", [](R &r) { read_affects(r); },
     [](W &w, std::string_view t) {
       w.out.print("{}:\n", t);
       for (const affected_type &saved : w.affects)
         if (saved.type)
           w.out.print("{:d} {:d} {:d} {:d} {:d} {:d}\n", saved.type, saved.duration, saved.modifier, saved.location,
                       static_cast<int>(saved.bitvector), saved.specific);
       w.out.write("0 0 0 0 0 0\n");
     }},
    {"Colr",
     [](R &r) {
       io::fields f(r.value);
       std::string colour;
       if (f.number(r.num) && f.word(colour, 127) && r.num >= 0 && r.num < NUM_COLOR)
         char_get_player_specials(r.ch)->color_choices[r.num] = colour;
     },
     [](W &w, std::string_view t) {
       for (int i = 0; i < NUM_COLOR; i++)
         if (!char_get_player_specials(w.ch)->color_choices[i].empty())
           w.out.print("{}: {:d} {}\r\n", t, i, char_get_player_specials(w.ch)->color_choices[i]); /* quirk: "\r\n" */
     }},

    /* read from older files, and no longer written */
    {"Lern", [](R &r) { char_set_practice_points(r.g, r.ch, r.as_int()); }, nullptr},
    {"Hit ", [](R &r) { char_set_max_hit(r.ch, r.current_and_max()); }, nullptr},
    {"Mana", [](R &r) { char_set_max_mana(r.ch, r.current_and_max()); }, nullptr},
    {"Move", [](R &r) { char_set_max_move(r.ch, r.current_and_max()); }, nullptr},
    {"Ki  ", [](R &r) { char_set_max_ki(r.ch, r.current_and_max()); }, nullptr},
    {"LF  ", [](R &) {}, nullptr},
    {"Affv", [](R &r) {
       while (std::optional<std::string_view> line = r.list_line()) /* up to its "0" */
         if (io::c_atol(*line) == 0)
           break;
     }, nullptr},
    {"Ecls", [](R &r) { io::fields f(r.value); f.number(r.num) && f.literal('=') && f.number(r.num2); }, nullptr},
    {"Mcls", [](R &r) { io::fields f(r.value); f.number(r.num) && f.literal('=') && f.number(r.num2); }, nullptr},
    {"Clan", [](R &) {}, nullptr},
    {"Clar", [](R &) {}, nullptr},
    {"Damg", [](R &) {}, nullptr},
    {"Eali", [](R &) {}, nullptr},
    {"Page", [](R &) {}, nullptr},
    {"Ship", [](R &) {}, nullptr},
    {"Shpr", [](R &) {}, nullptr},
    {"SkRc", [](R &) {}, nullptr},
    {"Stuk", [](R &) {}, nullptr},
    {"Thr1", [](R &) {}, nullptr},
    {"Thr2", [](R &) {}, nullptr},
    {"Thr3", [](R &) {}, nullptr},
    {"Thr4", [](R &) {}, nullptr},
    {"Thr5", [](R &) {}, nullptr},
    {"ThB1", [](R &) {}, nullptr},
    {"ThB2", [](R &) {}, nullptr},
    {"ThB3", [](R &) {}, nullptr},
};

} // namespace

static io::loaded<void> read_pfile_tags(GameWorld *g, char_data *ch, io::text_reader &in)
{
  pfile_reader r{g, ch, in};
  while (std::optional<std::string_view> line = in.line()) {
    if (line->size() < 4)
      continue; /* no tag; the old reader read past the line's end for one */
    std::string_view tag = line->substr(0, 4);
    r.value              = line->substr(4);
    while (r.value.starts_with(':') || r.value.starts_with(' '))
      r.value.remove_prefix(1);
    const pfile_tag *row = std::ranges::find_if(pfile_tags, [&](const pfile_tag &p) { return tag == p.tag; });
    if (row == std::end(pfile_tags))
      continue; /* an unknown tag is passed over */
    row->read(r);
    if (r.error)
      return std::unexpected(*r.error);
  }
  return {};
}

static void write_pfile_tags(GameWorld *g, char_data *ch, io::text_writer &out, std::span<const affected_type> affects)
{
  pfile_writer w{g, ch, out, affects};
  for (const pfile_tag &row : pfile_tags)
    if (row.write)
      row.write(w, row.tag);
}

/* Load a char: its place in the player index if loaded, -1 if not */
int load_char(GameWorld *g, std::string_view name, struct char_data *ch)
{
  int i, id;

  if ((id = get_ptable_by_name(g, name)) < 0)
    return (-1);
  std::optional<std::string> fname = player_path(PLR_FILE, g->player_table[id].name.c_str());
  if (!fname)
    return (-1);
  io::loaded<io::text_reader> file = io::text_reader::open(*fname);
  if (!file) {
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: Couldn't open player file {}", *fname);
    return (-1);
  }

  /* character initializations */
  /* initializations necessary to keep some things straight */
  char_affects(ch).clear();

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    char_set_skills_base(ch, i, 0);
    char_set_skills_mod(ch, i, 0);
    char_set_skills_perf(ch, i, 0);
  }
  char_set_sex(ch, PFDEF_SEX);
  char_set_size(ch, PFDEF_SIZE);
  char_set_class(ch, CLASS_ROSHI);
  char_set_loguser(ch, "NOUSER");
  char_set_race(ch, PFDEF_RACE);
  char_set_admlevel(ch, PFDEF_LEVEL);
  char_set_class_level(ch, PFDEF_LEVEL);
  char_set_race_level(ch, PFDEF_LEVEL);
  char_set_rbank(ch, PFDEF_SKIN);
  char_set_rp(ch, PFDEF_SKIN);
  char_set_trp(ch, PFDEF_SKIN);
  char_set_suppression(ch, PFDEF_SKIN);
  char_set_suppressed(ch, PFDEF_SKIN);
  char_set_fury(ch, PFDEF_HAIRL);
  char_set_level_adj(ch, PFDEF_LEVEL);
  char_set_hometown(ch, PFDEF_HOMETOWN);
  char_set_height(ch, PFDEF_HEIGHT);
  char_set_weight(ch, PFDEF_WEIGHT);
  char_set_basepl(ch, PFDEF_BASEPL);
  char_set_health(ch, 1.0);
  char_set_relax_count(ch, PFDEF_EYE);
  char_set_blesslvl(ch, PFDEF_HEIGHT);
  char_set_life(ch, 1.0);
  // GET_LIFEFORCE(ch) = PFDEF_BASEPL;
  char_set_lifeperc(ch, PFDEF_WEIGHT);
  char_set_position(ch, POS_STANDING);
  char_set_majinizer(ch, PFDEF_BASEPL);
  char_set_gauntlet(g, ch, PFDEF_GAUNTLET);
  char_set_baseki(ch, PFDEF_BASEKI);
  char_set_energy(ch, 1.0);
  char_set_basest(ch, PFDEF_BASEST);
  char_set_stamina(ch, 1.0);
  char_set_hairl(ch, PFDEF_HAIRL);
  char_set_hairc(ch, PFDEF_HAIRC);
  char_set_skin(ch, PFDEF_SKIN);
  char_set_eye(ch, PFDEF_EYE);
  char_set_hairs(ch, PFDEF_HAIRS);
  char_set_distfea(ch, PFDEF_DISTFEA);
  char_set_radar1(ch, PFDEF_RADAR1);
  char_set_lastpl(ch, PFDEF_LPLAY);
  char_set_boosts(ch, PFDEF_DISTFEA);
  char_set_majinize(ch, PFDEF_DISTFEA);
  char_set_lastint(ch, PFDEF_LPLAY);
  char_set_deathtime(ch, PFDEF_LPLAY);
  char_set_starphase(ch, PFDEF_EYE);
  char_set_mimic(ch, 0);
  char_set_skill_slots(ch, 0);
  char_set_tail_growth(ch, 0);
  char_set_trainstr(g, ch, PFDEF_EYE);
  char_set_trainspd(g, ch, PFDEF_EYE);
  char_set_trainwis(g, ch, PFDEF_EYE);
  char_set_trainagl(g, ch, PFDEF_EYE);
  char_set_traincon(g, ch, PFDEF_EYE);
  char_set_trainint(g, ch, PFDEF_EYE);
  char_set_rewtime(ch, PFDEF_LPLAY);
  char_set_dcount(ch, PFDEF_EYE);
  char_set_genome(ch, 0, PFDEF_EYE);
  char_set_preference(ch, PFDEF_EYE);
  char_set_genome(ch, 1, PFDEF_EYE);
  char_set_aura(ch, PFDEF_SKIN);
  for (i = 0; i < 52; i++) {
    char_set_bonuses(ch, i, PFDEF_BOARD);
  }
  char_set_combatexpertise(ch, 0);
  char_set_powerattack(ch, 0);
  char_set_limb_condition(ch, (1) - 1, 0);
  char_set_limb_condition(ch, (2) - 1, 0);
  char_set_limb_condition(ch, (3) - 1, 0);
  char_set_limb_condition(ch, (4) - 1, 0);
  char_set_lboard(ch, 0, PFDEF_BOARD);
  char_set_lboard(ch, 1, PFDEF_BOARD);
  char_set_lboard(ch, 2, PFDEF_BOARD);
  char_set_lboard(ch, 3, PFDEF_BOARD);
  char_set_lboard(ch, 4, PFDEF_BOARD);
  char_set_radar2(ch, PFDEF_RADAR2);
  char_set_radar3(ch, PFDEF_RADAR3);
  char_set_droom(ch, PFDEF_DROOM);
  char_set_alignment(ch, PFDEF_ALIGNMENT);
  static_assert(PFDEF_AFFFLAGS == 0);
  char_get_affected_by(ch) = {};
  static_assert(PFDEF_PLRFLAGS == 0);
  char_get_player_flags(g, ch) = {};
  static_assert(PFDEF_PREFFLAGS == 0);
  char_get_pref(g, ch) = {};
  char_get_admflags(ch) = {};
  char_set_load_room(g, ch, PFDEF_LOADROOM);
  char_set_invis_level(g, ch, PFDEF_INVISLEV);
  char_set_freeze_level(g, ch, PFDEF_FREEZELEV);
  char_set_wimp_level(g, ch, PFDEF_WIMPLEV);
  char_set_powerattack(ch, PFDEF_POWERATT);
  char_set_conditions(g, ch, HUNGER, PFDEF_HUNGER);
  char_set_conditions(g, ch, THIRST, PFDEF_THIRST);
  char_set_conditions(g, ch, DRUNK, PFDEF_DRUNK);
  char_set_bad_pws(g, ch, PFDEF_BADPWS);
  for (i = 0; i < NUM_CLASSES; i++)
    char_set_practice_points(g, ch, PFDEF_PRACTICES);
  char_set_gold(ch, PFDEF_GOLD);
  char_set_backstabcool(ch, 0);
  char_set_con_cooldown(ch, 0);
  char_set_con_sdcooldown(ch, 0);
  char_set_bank_gold(ch, PFDEF_BANK);
  char_set_absorbs(ch, PFDEF_BANK);
  char_set_ingestLearned(ch, PFDEF_BANK);
  char_set_racial_pref(g, ch, PFDEF_BANK);
  char_set_upgrade(ch, PFDEF_BANK);
  char_set_forgeting(ch, PFDEF_BANK);
  char_set_forgetcount(ch, PFDEF_BANK);
  char_set_kaioken(ch, PFDEF_BANK);
  char_set_exp(ch, PFDEF_EXP);
  char_set_transclass(ch, PFDEF_EXP);
  char_set_moltexp(ch, PFDEF_EXP);
  char_set_accuracy_mod(ch, PFDEF_ACCURACY);
  char_set_accuracy(ch, PFDEF_ACCURACY);
  char_set_armor(ch, PFDEF_AC);
  char_set_real_abils_str(ch, PFDEF_STR);
  char_set_real_abils_dex(ch, PFDEF_DEX);
  char_set_real_abils_intel(ch, PFDEF_INT);
  char_set_real_abils_wis(ch, PFDEF_WIS);
  char_set_real_abils_con(ch, PFDEF_CON);
  char_set_real_abils_cha(ch, PFDEF_CHA);
  //GET_HIT(ch) = PFDEF_HIT;
  //ch->max_hit = PFDEF_MAXHIT;
  //GET_MANA(ch) = PFDEF_MANA;
  //ch->max_mana = PFDEF_MAXMANA;
  //GET_MOVE(ch) = PFDEF_MOVE;
  //ch->max_move = PFDEF_MAXMOVE;
  char_set_ki(ch, PFDEF_KI);
  char_set_max_ki(ch, PFDEF_MAXKI);
  char_set_speaking(ch, PFDEF_SPEAKING);
  char_set_olc_zone(g, ch, PFDEF_OLC);
  char_set_host(g, ch, "");

  char_set_time_maxage(ch, 0);
  char_set_time_created(ch, 0);
  char_set_time_birth(ch, 0);
  char_followers_slot(ch)->clear();
  for (i = 0; i < NUM_COLOR; i++)
    char_get_player_specials(ch)->color_choices[i].clear();

  if (io::loaded<void> read = read_pfile_tags(g, ch, *file); !read) {
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: {}", read.error().text());
    return (-1);
  }

  if (!char_get_time_created(ch)) {
    mud_log("No creation timestamp for user {}, using current time", char_get_name(ch));
    char_set_time_created(ch, game_now(g));
  }

  if (!char_get_time_birth(ch)) {
    mud_log("No birthday for user {}, using standard starting age determination", char_get_name(ch));
    char_set_time_birth(ch, game_now(g) - birth_age(g, ch));
  }

  if (!char_get_time_maxage(ch)) {
    mud_log("No max age for user {}, using standard max age determination", char_get_name(ch));
    time_t maxage;
    if (ckd_add(&maxage, char_get_time_birth(ch), max_age(ch)))
      maxage = char_get_time_birth(ch);
    char_set_time_maxage(ch, maxage);
  }

  affect_total(g, ch);

  /* initialization for imms */
  if (char_get_admlevel(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < SKILL_TABLE_SIZE; i++)
      char_set_skills_base(ch, i, 100);
    char_set_conditions(g, ch, HUNGER, -1);
    char_set_conditions(g, ch, THIRST, -1);
    char_set_conditions(g, ch, DRUNK, -1);
  }

  if (char_is_android(ch)) {
    char_set_conditions(g, ch, HUNGER, -1);
    char_set_conditions(g, ch, THIRST, -1);
    char_set_conditions(g, ch, DRUNK, -1);
  }
  return (id);
}

/* Write the vital data of a player to the player file */
void save_char(GameWorld *g, struct char_data *ch)
{
  int i, id, save_index = false;
  struct affected_type tmp_aff[MAX_AFFECT];
  struct obj_data *char_eq[NUM_WEARS];

  if (char_is_npc(ch) || char_get_pfilepos(ch) < 0)
    return;

  /*
   * If ch->desc is not null, then we need to update some session data
   * before saving.
   */
  if (char_get_desc(ch)) {
    if (!char_get_desc(ch)->host.empty()) {
      if (char_get_host(g, ch).empty())
        char_set_host(g, ch, char_get_desc(ch)->host);
      else if (char_get_host(g, ch) == char_get_desc(ch)->host) /* quirk: sets what is there */
        char_set_host(g, ch, char_get_desc(ch)->host);
    }

    /*
     * We only update the time.played and time.logon if the character
     * is playing.
     */
    if (desc_get_connected(char_get_desc(ch)) == CON_PLAYING) {
      char_set_time_played(ch, char_get_time_played(ch) + (game_now(g) - char_get_time_logon(ch)));
      char_set_time_logon(ch, game_now(g));
    }
  }

  std::optional<std::string> fname = player_path(PLR_FILE, char_get_name(ch));
  if (!fname)
    return;

  /* remove affects from eq and spells (from char_to_store) */
  /* Unaffect everything a character can be affected by */

  if (char_get_trp(ch) < char_get_rp(ch)) {
    char_set_trp(ch, char_get_rp(ch));
  }
  if (char_get_desc(ch) != nullptr && !char_get_desc(ch)->acct.user.empty()) {
    save_account(char_get_desc(ch));
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i))
      char_eq[i] = unequip_char(g, ch, i);
    else
      char_eq[i] = nullptr;
  }

  auto aff = char_affects(ch).begin();
  for (i = 0; i < MAX_AFFECT; i++) {
    if (aff != char_affects(ch).end()) {
      tmp_aff[i] = *aff;
      ++aff;
    } else {
      tmp_aff[i].type      = 0; /* Zero signifies not used */
      tmp_aff[i].duration  = 0;
      tmp_aff[i].modifier  = 0;
      tmp_aff[i].specific  = 0;
      tmp_aff[i].location  = 0;
      tmp_aff[i].bitvector = AFF_DONTUSE;
    }
  }

  save_char_vars(g, ch);

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  if ((i >= MAX_AFFECT) && aff != char_affects(ch).end() && std::next(aff) != char_affects(ch).end())
    mud_log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  while (!char_affects(ch).empty())
    affect_remove(g, ch, &char_affects(ch).front());

  char_reset_aff_abils(ch);

  /* end char_to_store code */

  io::text_writer out(*fname, true);
  write_pfile_tags(g, ch, out, tmp_aff);
  out.commit();

  /* more char_to_store code to restore affects */

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_aff[i].type)
      affect_to_char(g, ch, &tmp_aff[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(g, ch, char_eq[i], i);
  }

  /* end char_to_store code */

  if ((id = get_ptable_by_name(g, char_get_name(ch))) < 0)
    return;

  /* update the player in the player index */
  if (g->player_table[id].level != char_get_level(ch)) {
    save_index             = true;
    g->player_table[id].level = char_get_level(ch);
  }
  if (g->player_table[id].admlevel != char_get_admlevel(ch)) {
    save_index                = true;
    g->player_table[id].admlevel = char_get_admlevel(ch);
  }
  if (g->player_table[id].last != char_get_time_logon(ch)) {
    save_index            = true;
    g->player_table[id].last = char_get_time_logon(ch);
  }
  if (g->player_table[id].played != char_get_lastpl(ch)) {
    save_index              = true;
    g->player_table[id].played = char_get_lastpl(ch);
  }
  const flag_set<pindex_flag> old_flags = g->player_table[id].flags;
  if (char_plr_flagged(ch, PLR_DELETED))
    g->player_table[id].flags.set(PINDEX_DELETED);
  else
    g->player_table[id].flags.reset(PINDEX_DELETED);
  if (char_plr_flagged(ch, PLR_NODELETE) || char_plr_flagged(ch, PLR_CRYO))
    g->player_table[id].flags.set(PINDEX_NODELETE);
  else
    g->player_table[id].flags.reset(PINDEX_NODELETE);

  if (char_plr_flagged(ch, PLR_FROZEN) || char_plr_flagged(ch, PLR_NOWIZLIST))
    g->player_table[id].flags.set(PINDEX_NOWIZLIST);
  else
    g->player_table[id].flags.reset(PINDEX_NOWIZLIST);

  if (g->player_table[id].flags != old_flags || save_index)
    save_player_index(g);
}

