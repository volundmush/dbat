/************************************************************************
 * OasisOLC - Mobiles / medit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <algorithm>
#include <cctype>
#include <memory>

#include "dbat/act.wizard.hpp"
#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/class.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/npc.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genmob.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/medit.hpp"
#include "dbat/modify.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/olc/script_editor.hpp"
#include "dbat/races.hpp"
#include "dbat/spells.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/olc/limits.hpp"

namespace {
  constexpr int NUM_GENDERS = 3; /* the sexes medit offers */

  struct mob_editor;
  using list_menu = void (*)(GameWorld *g, struct descriptor_data *d, mob_editor &e);

  /* A number of the main menu's that is only typed. Some are no longer kept, but still turn autobalance off. */
  struct number_field {
    char key;
    void (*set)(struct char_data *mob, int n); /* null: the number is ignored */
    bool unbalances;                          /* turns the mob's autobalance off */
    int context;
  };

  constexpr number_field NUMBERS[] = {
      {'6', [](char_data *m, int n) { char_set_race_level(m, LIMIT(n, 1, 150)); },                    false, CONTEXT_MEDIT_LEVEL       },
      {'7', [](char_data *m, int n) { char_set_alignment(m, LIMIT(n, -1000, 1000)); },                false, CONTEXT_MEDIT_ALIGNMENT   },
      {'8', [](char_data *m, int n) { char_set_accuracy_mod(m, LIMIT(n, 0, 50)); },                   true,  CONTEXT_MEDIT_ACCURACY    },
      {'9', nullptr /* damage mod no longer exists */,                                                 true,  CONTEXT_MEDIT_DAMAGE      },
      {'a', [](char_data *m, int n) { char_set_mob_specials_damnodice(m, LIMIT(n, 0, 30)); },         true,  CONTEXT_MEDIT_NDD         },
      {'b', [](char_data *m, int n) { char_set_mob_specials_damsizedice(m, LIMIT(n, 0, 127)); },      true,  CONTEXT_MEDIT_SDD         },
      {'c', nullptr /* hit dice no longer exist */,                                                    true,  CONTEXT_MEDIT_NUM_HP_DICE },
      {'d', nullptr,                                                                                   true,  CONTEXT_MEDIT_SIZE_HP_DICE},
      {'e', nullptr,                                                                                   true,  CONTEXT_MEDIT_ADD_HP      },
      {'f', [](char_data *m, int n) { char_set_armor(m, LIMIT(n, 10, 200000)); },                     true,  CONTEXT_MEDIT_AC          },
      {'g', [](char_data *m, int n) { char_set_exp(m, LIMIT(n, 0, MAX_MOB_EXP)); },                   true,  CONTEXT_MEDIT_EXP         },
      {'h', [](char_data *m, int n) { char_set_gold(m, LIMIT(n, 0, MAX_MOB_GOLD)); },                 false, CONTEXT_MEDIT_GOLD        },
  };

  /* A one-line text of the main menu's */
  struct text_field {
    char key;
    void (*set)(struct char_data *mob, const std::string &text);
    int context;
  };

  constexpr text_field TEXTS[] = {
      {'2', [](char_data *m, const std::string &t) { char_set_alias(m, text_or_undefined(t)); },                                   CONTEXT_MEDIT_ALIAS },
      {'3', [](char_data *m, const std::string &t) { char_set_sdesc(m, text_or_undefined(t)); },                                   CONTEXT_MEDIT_S_DESC},
      {'4', [](char_data *m, const std::string &t) { char_set_ldesc(m, t.empty() ? std::string("undefined") : t + "\r\n"); },     CONTEXT_MEDIT_L_DESC},
  };

  /* The mob editor: a copy of a mobile prototype, put into the world on saving */
  struct mob_editor final : olc_session {
    enum class mode {
      main_menu,
      confirm_save,
      text,   /* one of TEXTS */
      description,
      npc_flags,
      aff_flags,
      number, /* one of NUMBERS */
      sex,
      position,
      default_position,
      personality,
      klass,
      race,
      size,
      copy,
      confirm_delete,
      script,
    };

    GameWorld *world; /* for freeing the mob */
    mode asking = mode::main_menu;
    mob_vnum vnum{};
    struct zone_data *zone{};
    std::unique_ptr<char_data> mob;
    bool changed{};
    const text_field *text{};
    const number_field *number{};
    script_editor script{MOB_TRIGGER};

    explicit mob_editor(GameWorld *g) : world(g) {}
    ~mob_editor() override { free_mobile(world, mob.release()); }

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void text_done(GameWorld *g, struct descriptor_data *d, int) override { menu(g, d); }
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone->number); }

    void edit_new(GameWorld *g);
    void edit_copy_of(GameWorld *g, struct char_data *proto);
    void menu(GameWorld *g, struct descriptor_data *d);
    void done(GameWorld *g, struct descriptor_data *d) /* something was changed: back to the main menu */
    {
      changed = true;
      menu(g, d);
    }
    bool takes_numbers() const;
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
  };

  /* ---- the lists the main menu chooses from ---- */

  void sex_menu(GameWorld *g, struct descriptor_data *d, mob_editor &)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_GENDERS; i++)
      write_to_output(g, d, "@g{:>2d}@n) {}\r\n", i, genders[i]);
    write_to_output(g, d, "Enter gender number : ");
  }

  void positions_menu(GameWorld *g, struct descriptor_data *d, mob_editor &)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_POSITIONS; i++)
      write_to_output(g, d, "@g{:>2d}@n) {}\r\n", i, position_types[i]);
    write_to_output(g, d, "Enter position number : ");
  }

  void personality_menu(GameWorld *g, struct descriptor_data *d, mob_editor &)
  {
    write_to_output(g, d, "@GPersonalities\n");
    write_to_output(g, d, "@D--------------@n\n");
    for (int i = 0; i < MAX_PERSONALITIES; i++)
      write_to_output(g, d, "@w{:d}@D) @W{}@n\n", i + 1, npc_personality[i]);
  }

  void npc_flags_menu(GameWorld *g, struct descriptor_data *d, mob_editor &e)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, action_bits, flag_count<mob_flag>, 1, "  ", 2);
    write_to_output(g, d, "\r\nCurrent flags : @c{}@n\r\nEnter mob flags (0 to quit) : ", flag_names(char_get_mob_flags(e.mob.get())));
  }

  void aff_flags_menu(GameWorld *g, struct descriptor_data *d, mob_editor &e)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, affected_bits.span().subspan(1), flag_count<aff_flag>, 1, "  ", 2);
    write_to_output(g, d, "\r\nCurrent flags   : @c{}@n\r\nEnter aff flags (0 to quit) : ",
                    flag_names(char_get_affected_by(e.mob.get())));
  }

  void class_menu(GameWorld *g, struct descriptor_data *d, mob_editor &)
  {
    clear_screen(g, d);
    for (int i = 0; i < NUM_CLASSES; i++)
      if (sensei_exists(i))
        write_to_output(g, d, "@g{:>2d}@n) {}\r\n", i, sensei_name(i));
    write_to_output(g, d, "Enter class number : ");
  }

  void race_menu(GameWorld *g, struct descriptor_data *d, mob_editor &)
  {
    clear_screen(g, d);
    for (int r = 0; r < NUM_RACES; r++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}  {}", r, race_name(r), (r + 1) % 2 ? "" : "\r\n");
    write_to_output(g, d, "Enter race number : ");
  }

  void size_menu(GameWorld *g, struct descriptor_data *d, mob_editor &)
  {
    clear_screen(g, d);
    for (int i = -1; i < NUM_SIZES; i++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}  {}", i, i == SIZE_UNDEFINED ? "DEFAULT" : size_names[i], (i + 2) % 2 ? "" : "\r\n");
    write_to_output(g, d, "Enter size number (-1 for default): ");
  }

  struct list_choice {
    char key;
    mob_editor::mode asks;
    list_menu show;
  };

  constexpr list_choice LISTS[] = {
      {'1', mob_editor::mode::sex,              sex_menu        },
      {'i', mob_editor::mode::position,         positions_menu  },
      {'j', mob_editor::mode::default_position, positions_menu  },
      {'k', mob_editor::mode::personality,      personality_menu},
      {'l', mob_editor::mode::npc_flags,        npc_flags_menu  },
      {'m', mob_editor::mode::aff_flags,        aff_flags_menu  },
      {'n', mob_editor::mode::klass,            class_menu      },
      {'o', mob_editor::mode::race,             race_menu       },
      {'y', mob_editor::mode::size,             size_menu       },
  };

  /* ---- the editor ---- */

  void mob_editor::edit_new(GameWorld *g)
  {
    mob = std::make_unique<char_data>();
    clear_char(mob.get());
    char_set_sex(mob.get(), SEX_MALE);
    char_set_class(mob.get(), CLASS_NPC_COMMONER);
    char_set_weight(mob.get(), rand_number(g, 100, 200));
    char_set_height(mob.get(), rand_number(g, 100, 200));
    const int8_t mental = rand_number(g, 8, 16);
    char_set_real_abils_wis(mob.get(), mental);
    char_set_real_abils_intel(mob.get(), mental);
    char_set_real_abils_str(mob.get(), mental);
    const int8_t physical = rand_number(g, 8, 16);
    char_set_real_abils_cha(mob.get(), physical);
    char_set_real_abils_con(mob.get(), physical);
    char_set_real_abils_dex(mob.get(), physical);
    char_reset_aff_abils(mob.get());
    char_mob_set(mob.get(), mob_flag::isnpc, true);

    char_set_nr(mob.get(), NOBODY);
    char_set_alias(mob.get(), "mob unfinished");
    char_set_sdesc(mob.get(), "the unfinished mob");
    char_set_ldesc(mob.get(), "An unfinished mob stands here.\r\n");
    char_set_ddesc(mob.get(), "It looks unfinished.\r\n");
    char_set_race(mob.get(), RACE_HUMAN);
    script.triggers.clear();
  }

  /* The prototype's copy to edit: its triggers go to the script editor, and it has none of its own */
  void mob_editor::edit_copy_of(GameWorld *g, struct char_data *proto)
  {
    free_mobile(g, mob.release());
    mob = std::make_unique<char_data>();
    copy_mobile(g, mob.get(), proto);
    script.triggers = char_get_proto_script(mob.get());
    char_set_script(mob.get(), nullptr);
    char_set_proto_script(mob.get(), {});
  }

  void mob_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    struct char_data *m = mob.get();
    clear_screen(g, d);
    write_to_output(g, d,
                    "-- Mob Number:  [@c{:d}@n]\r\n@g1@n) Sex: @y{:<7.7}@n	         @g2@n) Alias: @y{}\r\n@g3@n) S-Desc: @y{}\r\n@g4@n) L-Desc:-\r\n@y{}@g5@n) D-Desc:-\r\n@y{}@g6@n) Level:       [@c{:>4d}@n],  @g7@n) Alignment:    [@c{:>5d}@n]\r\n@g8@n) Accuracy Mod:[@c{:>4d}@n],  @g9@n) Damage Mod:   [@c{:>5d}@n]\r\n@gA@n) NumDamDice:  [@c{:>4d}@n],  @gB@n) SizeDamDice:  [@c{:>5d}@n]\r\n@gC@n) Num HP Dice: [@c{:>4d}@n],  @gD@n) Size HP Dice: [@c{:>5d}@n],  @gE@n) HP Bonus: [@c{:>5d}@n]\r\n@gF@n) Armor Class: [@c{:>4d}@n],  @gG@n) Exp:      [@c{:d}@n],  @gH@n) Gold:  [@c{:>8d}@n]\r\n",
                    vnum, genders[(int)char_get_sex(m)], char_get_alias(m), char_get_sdesc(m), char_get_ldesc(m), char_get_ddesc(m), char_get_race_level(m),
                    char_get_alignment(m), char_get_accuracy_mod(m), 0 /* damage mod: unused */, char_get_mob_specials_damnodice(m),
                    char_get_mob_specials_damsizedice(m), getCurPL(g, m), getCurKI(g, m), getCurST(g, m), char_get_armor(m), char_get_exp(m), char_get_gold(m));
    write_to_output(g, d,
                    "@gI@n) Position   : @y{:<10}@n,	 @gJ@n) Default   : @y{:<10}\r\n@gK@n) Personality: @Y{}@n\r\n@gL@n) NPC Flags  : @c{}\r\n@gM@n) AFF Flags  : @c{}\r\n@gN@n) Class      : @y{:<10}@n,	 @gO@n) Race      : @y{:<10}\r\n@gS@n) Script     : @c{}\r\n@gW@n) Copy mob              ,	 @gX@n) Delete mob\r\n@gY@n) Size       : @y{}\r\n@gZ@n) Wiznet     :\r\n@gQ@n) Quit\r\nEnter choice : ",
                    position_types[(int)char_get_position(m)], position_types[(int)char_get_mob_specials_default_pos(m)],
                    npc_personality[char_get_personality(m)], flag_names(char_get_mob_flags(m)),
                    flag_names(char_get_affected_by(m)), char_sensei_name(m), char_true_race_name(m),
                    !script.triggers.empty() ? "Set." : "Not Set.", size_names[get_size(m)]);
    asking = mode::main_menu;
  }

  /* What is asked for numbers only, its answer refused otherwise (quirk: the script editor and deleting too, which
     want letters: the script editor can't be left, and nothing deleted) */
  bool mob_editor::takes_numbers() const
  {
    switch (asking) {
      case mode::main_menu:
      case mode::confirm_save:
      case mode::text:
      case mode::description:
      case mode::npc_flags:
      case mode::aff_flags: return false;
      default: return true;
    }
  }

  void mob_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view typed)
  {
    std::string line(typed);
    if (takes_numbers()) {
      if (line.empty() || !(isdigit(line[0]) || (line[0] == '-' && line.size() > 1 && isdigit(line[1])))) {
        write_to_output(g, d, "Field must be numerical, try again : ");
        return;
      }
    } else
      smash_tilde(line);
    const int n = parse_int(line);

    switch (asking) {
      case mode::confirm_save:
        save_answer(g, d, line);
        return;

      case mode::main_menu:
        main_menu_choice(g, d, line);
        return;

      case mode::script:
        if (!script.parse(g, d, line))
          done(g, d);
        return;

      case mode::text:
        text->set(mob.get(), line);
        done(g, d);
        return;

      case mode::description: /* the text editor has the lines */
        done(g, d);
        return;

      case mode::npc_flags:
        if (n <= 0) {
          done(g, d);
          return;
        }
        if (std::optional<mob_flag> flag = flag_from_index<mob_flag>(n - 1))
          char_mob_toggle(mob.get(), *flag);
        npc_flags_menu(g, d, *this);
        return;

      case mode::aff_flags:
        if (n <= 0) {
          done(g, d);
          return;
        }
        if (std::cmp_less_equal(n, flag_count<aff_flag>)) /* the menu's numbers are the flags' (quirk: so the last toggles bit 79, past them) */
          char_get_affected_by(mob.get()).toggle_bit(static_cast<std::size_t>(n));
        /* Remove unwanted bits right away. quirk: clears aff_flag::tamed alone, meant aff_flag::charm, aff_flag::poison, aff_flag::group and
           aff_flag::sleep: their numbers or-ed as bits, 22 | 12 | 9 | 15 is 31 */
        char_aff_set(mob.get(), aff_flag::tamed, false);
        aff_flags_menu(g, d, *this);
        return;

      case mode::personality:
        if (n <= 0) {
          done(g, d);
          return;
        }
        if (n <= MAX_PERSONALITIES) /* menu choice 1..N is npc_personality[0..N-1] */
          char_set_personality(mob.get(), n - 1);
        personality_menu(g, d, *this);
        return;

      case mode::number:
        if (number->set)
          number->set(mob.get(), n);
        if (number->unbalances && char_mob_flagged(mob.get(), mob_flag::autobalance))
          char_mob_toggle(mob.get(), mob_flag::autobalance);
        done(g, d);
        return;

      case mode::sex:
        char_set_sex(mob.get(), LIMIT(n, 0, NUM_GENDERS - 1));
        done(g, d);
        return;

      case mode::position:
        char_set_position(mob.get(), LIMIT(n, 0, NUM_POSITIONS - 1));
        done(g, d);
        return;

      case mode::default_position:
        char_set_mob_specials_default_pos(mob.get(), LIMIT(n, 0, NUM_POSITIONS - 1));
        done(g, d);
        return;

      case mode::klass: /* quirk: the answer is ignored, and the class stays */
        if (!char_get_class(mob.get()))
          char_set_class(mob.get(), CLASS_NPC_COMMONER);
        done(g, d);
        return;

      case mode::race:
        if (!race_exists(n))
          write_to_output(g, d, "That's not a race!");
        else {
          char_set_race(mob.get(), n);
          char_set_size(mob.get(), race_get_size(char_get_race(mob.get()))); /* Change racial size based on race choice. */
        }
        done(g, d);
        return;

      case mode::size:
        char_set_size(mob.get(), LIMIT(n, -1, NUM_SIZES - 1));
        done(g, d);
        return;

      case mode::copy:
        if (struct char_data *proto = mob_proto_of(g, n))
          edit_copy_of(g, proto);
        else
          write_to_output(g, d, "That mob does not exist.\r\n");
        done(g, d);
        return;

      case mode::confirm_delete:
        if (const std::optional<bool> sure = yes_or_no(line); sure == true) {
          write_to_output(g, d, delete_mobile(g, char_get_nr(mob.get())) != NOBODY ? "Mobile deleted.\r\n" : "Couldn't delete the mobile!\r\n");
          leave_olc(g, d);
        } else if (sure == false)
          menu(g, d);
        else {
          write_to_output(g, d, "Please answer 'Y' or 'N': ");
          done(g, d);
        }
        return;
    }
  }

  void mob_editor::save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    char_mob_set(mob.get(), mob_flag::isnpc, true); /* Ensure mob has mob_flag::isnpc set or things will go pear shaped. */
    const std::optional<bool> save = yes_or_no(line);
    if (!save) {
      write_to_output(g, d, "Invalid choice!\r\n");
      write_to_output(g, d, "Do you wish to save the mobile? : ");
      return;
    }
    if (*save) {
      if (add_mobile(g, mob.get(), vnum) == NOBODY)
        mud_log("medit_save_internally: add_mobile failed.");
      else {
        char_set_proto_script(mob_proto_of(g, vnum), script.triggers); /* for new copies */
        for (char_data *live : each_char(g)) {                        /* and those in the game */
          if (char_get_nr(live) != vnum)
            continue;
          if (char_get_script(live))
            extract_script(g, live);
          copy_proto_script(mob_proto_of(g, vnum), live);
          assign_triggers(g, live);
        }
      }
      mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} edits mob {:d}", char_get_name(d->character), vnum);
      if (CONFIG_OLC_SAVE) {
        save_mobiles(g, find_zone(g, find_zone_by_thing(g, vnum)->number));
        write_to_output(g, d, "Mobile saved to disk.\r\n");
      } else
        write_to_output(g, d, "Mobile saved to memory.\r\n");
    }
    leave_olc(g, d);
  }

  void mob_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = tolower(first_char(line));
    if (auto t = std::ranges::find(TEXTS, key, &text_field::key); key && t != std::end(TEXTS)) {
      write_to_output(g, d, "\r\nEnter new text :\r\n] ");
      text   = t;
      asking = mode::text;
      return;
    }
    if (auto f = std::ranges::find(NUMBERS, key, &number_field::key); key && f != std::end(NUMBERS)) {
      write_to_output(g, d, "\r\nEnter new value : ");
      number = f;
      asking = mode::number;
      return;
    }
    if (auto l = std::ranges::find(LISTS, key, &list_choice::key); key && l != std::end(LISTS)) {
      asking = l->asks;
      l->show(g, d, *this);
      return;
    }

    switch (key) {
      case 'q':
        if (changed) {
          write_to_output(g, d, "Do you wish to save your changes? : ");
          asking = mode::confirm_save;
        } else
          leave_olc(g, d);
        return;
      case '5':
        asking = mode::description;
        send_editor_help(g, d);
        write_to_output(g, d, "Enter mob description:\r\n\r\n");
        if (!char_get_ddesc(mob.get()).empty())
          write_to_output(g, d, "{}", char_get_ddesc(mob.get()));
        string_write(d, char_description_slot(mob.get()), MAX_MOB_DESC, 0);
        changed = true;
        return;
      case 's':
        asking = mode::script;
        script.menu(g, d);
        return;
      case 'w':
        write_to_output(g, d, "Copy what mob? ");
        asking = mode::copy;
        return;
      case 'x':
        write_to_output(g, d, "Are you sure you want to delete this mobile? ");
        asking = mode::confirm_delete;
        return;
      case 'z': {
        std::string message(line);
        search_replace(message, "z ", ""); /* quirk: every "z " in it, and not "Z " */
        do_wiznet(g, d->character, message, 0, 0);
        done(g, d); /* quirk: as though something had changed */
        return;
      }
      default:
        menu(g, d);
        return;
    }
  }

  int mob_editor::help_context(const struct descriptor_data *) const
  {
    switch (asking) {
      case mode::main_menu: return CONTEXT_MEDIT_MAIN_MENU;
      case mode::confirm_save: return CONTEXT_MEDIT_CONFIRM_SAVESTRING;
      case mode::text: return text->context;
      case mode::description: return CONTEXT_MEDIT_D_DESC;
      case mode::npc_flags: return CONTEXT_MEDIT_NPC_FLAGS;
      case mode::aff_flags: return CONTEXT_MEDIT_AFF_FLAGS;
      case mode::number: return number->context;
      case mode::sex: return CONTEXT_MEDIT_SEX;
      case mode::position: return CONTEXT_MEDIT_POS;
      case mode::default_position: return CONTEXT_MEDIT_DEFAULT_POS;
      case mode::script: return script.help_context();
      default: return NOTHING;
    }
  }
} // namespace

ACMD(do_oasis_medit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);
  std::string buf2      = take_arg(rest);

  if (buf1.empty()) {
    send_to_char(g, ch, "Specify a mobile VNUM to edit.\r\n");
    return;
  }

  IDXTYPE number = NOBODY;
  bool save      = false;
  if (!isdigit(buf1[0])) {
    if (!str_eq_nocase("save", buf1)) {
      send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = true;
    if ((number = zone_to_save(g, ch, buf2)) == NOWHERE)
      return;
  }
  if (number == NOBODY)
    number = parse_int(buf1);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_MEDIT && dd->olc && session_of<mob_editor>(dd).vnum == number) {
      send_to_char(g, ch, "That mobile is currently being edited by {}.\r\n", char_get_name(dd->character));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<mob_editor>(g), "do_oasis_medit");
  mob_editor &editor = session_of<mob_editor>(d);

  if (!(editor.zone = zone_to_edit(g, ch, number, save))) {
    d->olc.reset();
    return;
  }

  if (save) {
    save_zone_files(g, ch, editor.zone, "mobiles", "mobile info", save_mobiles);
    d->olc.reset();
    return;
  }

  editor.vnum = number;
  if (struct char_data *proto = mob_proto_of(g, number))
    editor.edit_copy_of(g, proto);
  else
    editor.edit_new(g);

  editor.menu(g, d);
  begin_editing(g, d, CON_MEDIT);
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}
