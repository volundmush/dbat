/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  gedit.c:  Olc written for shoplike guildmasters, code by             *
 *             Jason Goodwin                                             *
 *    Made for Circle3.0 bpl11, its copyright applies                    *
 *                                                                       *
 *  Made for Oasis OLC                                                   *
 *  Copyright 1996 Harvey Gilpin.                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <array>
#include <algorithm>
#include <cctype>
#include <cstring>

#include "dbat/act.informative.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/feats.hpp"
#include "dbat/game_world.hpp"
#include "dbat/gengld.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genshp.hpp"
#include "dbat/genzon.hpp"
#include "dbat/guild.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/modify.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/shop.hpp"
#include "dbat/spells.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/olc/commands.hpp"

namespace {
  /* What the guild editor is asking; from charge on, the answer is a number */
  enum class mode { main_menu, confirm_save, no_cash, no_skill, charge, open, close, trainer, no_train, min_level, skills, languages, feats };

  /* What a main menu key asks for */
  struct field {
    char key;
    mode asks;
    std::string_view prompt{};
  };

  constexpr auto FIELDS = std::to_array<field>({
      {'0', mode::trainer,   "Enter vnum of guild master : "                                        },
      {'1', mode::no_skill,  "\r\nEnter new text :\r\n] "                                           },
      {'2', mode::no_cash,   "\r\nEnter new text :\r\n] "                                           },
      {'3', mode::open,      "When does this shop open (a day has 28 hours) ? \r\nEnter new value : " },
      {'4', mode::close,     "When does this shop close (a day has 28 hours) ? \r\nEnter new value : "},
      {'5', mode::charge,    "\r\nEnter new value : "                                               },
      {'6', mode::min_level, "Minumum Level will Train: "                                           },
  });

  constexpr auto CONTEXTS = std::to_array<std::pair<mode, int>>({
      {mode::main_menu,    CONTEXT_GEDIT_MAIN_MENU         },
      {mode::confirm_save, CONTEXT_GEDIT_CONFIRM_SAVESTRING},
      {mode::no_cash,      CONTEXT_GEDIT_NO_CASH           },
      {mode::no_skill,     CONTEXT_GEDIT_NO_SKILL          },
      {mode::charge,       CONTEXT_GEDIT_CHARGE            },
      {mode::open,         CONTEXT_GEDIT_OPEN              },
      {mode::close,        CONTEXT_GEDIT_CLOSE             },
      {mode::trainer,      CONTEXT_GEDIT_TRAINER           },
      {mode::no_train,     CONTEXT_GEDIT_NO_TRAIN          },
      {mode::min_level,    CONTEXT_GEDIT_MINLVL            },
      {mode::skills,       CONTEXT_GEDIT_SELECT_SKILLS     },
      {mode::languages,    CONTEXT_GEDIT_SELECT_LANGS      },
      /* quirk: the feats list has no context help */
  });

  /* The guild editor: a copy of one guild, put into the world when saved */
  struct guild_editor final : olc_session {
    mode asking = mode::main_menu;
    struct zone_data *zone{};
    guild_vnum number = NOTHING;
    guild_data guild{};
    bool changed = false;

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone->number); }

    void edit_new();
    void menu(GameWorld *g, struct descriptor_data *d);
    void no_train_menu(GameWorld *g, struct descriptor_data *d);
    void teaching_menu(GameWorld *g, struct descriptor_data *d, mode which);
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    bool takes_numbers() const { return asking >= mode::charge; }
  };

  /* The skills the skills (or languages) menu lists */
  bool listed(GameWorld *g, int skill, mode which)
  {
    const auto &info = g->spell_info[skill];
    if (info.name == "!UNUSED!")
      return false;
    return which == mode::skills ? info.skilltype == skill_kind::skill : info.skilltype.test(skill_kind::lang);
  }

  void guild_editor::edit_new()
  {
    guild                 = {};
    guild.gm              = NOBODY;
    guild.close           = 28;
    guild.charge          = 1.0;
    guild.no_such_skill   = guild_default_no_skill;
    guild.not_enough_gold = guild_default_no_gold;
  }

  void guild_editor::no_train_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_TRADERS; i++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}   {}", i + 1, trade_letters[i], (i + 1) % 2 ? "" : "\r\n");
    write_to_output(g, d, "\r\nCurrent train flags: @c{}@n\r\nEnter choice, 0 to quit : ", flag_names(guild.with_who));
    asking = mode::no_train;
  }

  /* What the guild teaches of the skills, the languages or the feats, three to a line, to toggle by number */
  void guild_editor::teaching_menu(GameWorld *g, struct descriptor_data *d, mode which)
  {
    clear_screen(g, d);
    write_to_output(g, d, which == mode::feats ? "Feats known:\r\n" : "Skills known:\r\n");
    int shown = 0;
    auto show = [&](int i, bool taught, std::string_view name) {
      write_to_output(g, d, "@n[@c{:<3}@n] {:<3d} {:<20.20}  ", YESNO(taught), i, name);
      if (++shown % 3 == 0)
        write_to_output(g, d, "\r\n");
    };
    if (which == mode::feats) {
      for (int i = 0; i < NUM_FEATS_DEFINED; i++)
        if (g->feat_list[i].in_game)
          show(i, guild.feats[i], g->feat_list[i].name);
    } else
      for (int i = 0; i < SKILL_TABLE_SIZE; i++)
        if (listed(g, i, which))
          show(i, guild.skills[i], g->spell_info[i].name);
    write_to_output(g, d, which == mode::feats ? "\r\nEnter feat num, 0 to quit:  " : "\r\nEnter skill num, 0 to quit:  ");
    asking = which;
  }

  void guild_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    const bool master = guild.gm != NOBODY;
    write_to_output(g, d,
                    "-- Guild Number: [@c{:d}@n]\r\n@g 0@n) Guild Master : [@c{:d}@n] @y{}\r\n@g 1@n) Doesn't know skill:\r\n @y{}\r\n@g 2@n) Player no gold:\r\n @y{}\r\n@g 3@n) Open   :  [@c{:d}@n]\r\n@g 4@n) Close  :  [@c{:d}@n]\r\n@g 5@n) Charge :  [@c{:>3.1f}@n]\r\n@g 6@n) Minlvl :  [@c{:d}@n]\r\n@g 7@n) Who to Train:  @c{}\r\n@g 8@n) Feats Menu\r\n@g 9@n) Skills Menu\r\n@g B@n) Languages Menu\r\n@g Q@n) Quit\r\nEnter Choice : ",
                    number, master ? mob_index_of(g, guild.gm)->vnum : -1, master ? char_get_sdesc(mob_proto_of(g, guild.gm)) : "None",
                    guild.no_such_skill, guild.not_enough_gold, guild.open, guild.close, guild.charge, guild.minlvl,
                    flag_names(guild.with_who));
    asking = mode::main_menu;
  }

  void guild_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = tolower(first_char(line));
    if (const field *f = std::ranges::find(FIELDS, key, &field::key); f != std::end(FIELDS)) {
      asking = f->asks;
      write_to_output(g, d, f->prompt);
      return;
    }
    switch (key) {
      case 'q':
        if (changed) {
          write_to_output(g, d, "Do you wish to save the changes to the Guild? (y/n) : ");
          asking = mode::confirm_save;
        } else
          leave_olc(g, d);
        return;
      case '7': no_train_menu(g, d); return;
      case '8': teaching_menu(g, d, mode::feats); return;
      case '9': teaching_menu(g, d, mode::skills); return;
      case 'b': teaching_menu(g, d, mode::languages); return;
      default: menu(g, d); return;
    }
  }

  void guild_editor::save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    std::optional<bool> save = yes_or_no(line);
    if (!save) {
      write_to_output(g, d, "Invalid choice!\r\nDo you wish to save the guild? : ");
      return;
    }
    if (*save) {
      send_to_char(g, d->character, "Saving Guild to memory.\r\n");
      guild.vnum = number;
      add_guild(g, &guild);
      mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} edits guild {:d}",
             char_get_name(d->character), number);
      if (CONFIG_OLC_SAVE) {
        save_guilds(g, find_zone_by_thing(g, number));
        write_to_output(g, d, "Guild {:d} saved to disk.\r\n", number);
      } else
        write_to_output(g, d, "Guild {:d} saved to memory.\r\n", number);
    }
    leave_olc(g, d);
  }

  void guild_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    if (takes_numbers() && minus_without_digit(line)) {
      write_to_output(g, d, "Field must be numerical, try again : ");
      return;
    }
    const int n = parse_int(line);

    switch (asking) {
      case mode::confirm_save: save_answer(g, d, line); return;
      case mode::main_menu: main_menu_choice(g, d, line); return;

      case mode::no_skill:
      case mode::no_cash: {
        /* a '~' would end the message early in the guild file */
        std::string text(line);
        smash_tilde(text);
        set_npc_message(g, d, asking == mode::no_skill ? guild.no_such_skill : guild.not_enough_gold, text, false);
        break;
      }
      case mode::trainer:
        if (!isdigit(first_char(line))) { /* quirk: so no guild master (-1) can't be asked for */
          write_to_output(g, d, "Invalid response.\r\n");
          menu(g, d);
          return;
        }
        if (!mob_proto_of(g, n)) {
          write_to_output(g, d, "That mobile does not exist, try again : ");
          return;
        }
        guild.gm = n;
        { /* the master's own special goes behind the guild's */
          auto *index = mob_index_of(g, n);
          guild.func  = index->func != ::guild ? index->func : nullptr;
          index->func = ::guild;
        }
        break;
      case mode::open: guild.open = LIMIT(n, 0, 28); break;
      case mode::close: guild.close = LIMIT(n, 0, 28); break;
      case mode::charge: scan_float(line, guild.charge); break;
      case mode::min_level: guild.minlvl = MAX(n, 0); break;
      case mode::no_train:
        if (int trader = LIMIT(n, 0, NUM_TRADERS - 1); trader > 0) { /* quirk: the last can't be chosen */
          guild_trade_toggle(&guild, *flag_from_index<trade_flag>(trader - 1)); /* 0 to NUM_TRADERS - 2 */
          no_train_menu(g, d);
          return;
        }
        break;
      case mode::skills:
      case mode::languages:
      case mode::feats: {
        if (n == 0)
          break;
        /* quirk: any number is taken, listed or not, a negative one as the first */
        bool *taught = asking == mode::feats ? &guild.feats[std::clamp(n, 0, NUM_FEATS_DEFINED - 1)]
                                            : &guild.skills[std::clamp(n, 0, SKILL_TABLE_SIZE - 1)];
        *taught     = !*taught;
        teaching_menu(g, d, asking);
        return;
      }
    }
    changed = true;
    menu(g, d);
  }

  int guild_editor::help_context(const struct descriptor_data *) const
  {
    auto found = std::ranges::find(CONTEXTS, asking, &std::pair<mode, int>::first);
    return found == std::end(CONTEXTS) ? NOTHING : found->second;
  }
} // namespace

ACMD(do_oasis_gedit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);
  std::string buf2      = take_arg(rest);

  if (buf1.empty()) {
    send_to_char(g, ch, "Specify a guild VNUM to edit.\r\n");
    return;
  }

  IDXTYPE number = NOWHERE;
  bool save      = false;
  if (!isdigit(buf1[0])) {
    if (!str_eq_nocase("save", buf1)) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = true;
    if ((number = zone_to_save(g, ch, buf2)) == NOWHERE)
      return;
  } else
    number = parse_int(buf1);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_GEDIT && dd->olc && session_of<guild_editor>(dd).number == number) {
      send_to_char(g, ch, "That guild is currently being edited by {}.\r\n", char_pers(g, dd->character, ch));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<guild_editor>(), "do_oasis_gedit");
  guild_editor &editor = session_of<guild_editor>(d);

  /* quirk: even "gedit save <zone>" finds the zone holding vnum <zone>, not zone <zone> */
  if (!(editor.zone = zone_to_edit(g, ch, number, false))) {
    d->olc.reset();
    return;
  }
  if (save) {
    save_zone_files(g, ch, editor.zone, "guilds", "guild info", save_guilds);
    d->olc.reset();
    return;
  }

  editor.number = number;
  if (struct guild_data *existing = find_guild(g, number))
    editor.guild = *existing;
  else
    editor.edit_new();

  editor.menu(g, d);
  begin_editing(g, d, CON_GEDIT);
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}
