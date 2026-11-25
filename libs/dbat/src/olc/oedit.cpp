/************************************************************************
 * OasisOLC - Objects / oedit.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <algorithm>
#include <cctype>
#include <climits>
#include <span>

#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/affect.hpp"
#include "dbat/args.hpp"
#include "dbat/boards.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/feats.hpp"
#include "dbat/fight.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genobj.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/handler.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/iterate.hpp"
#include "dbat/modify.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/olc/extra_desc.hpp"
#include "dbat/olc/script_editor.hpp"
#include "dbat/races.hpp"
#include "dbat/relocate.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/shop.hpp"
#include "dbat/spells.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/olc/commands.hpp"
#include "dbat/olc/limits.hpp"

namespace {
  struct object_editor;
  using list_menu = void (*)(GameWorld *g, struct descriptor_data *d, object_editor &e);

  /* A number of the main menu's that is only typed: its key, prompt, and how it's kept */
  struct number_field {
    char key;
    std::string_view prompt;
    void (*set)(struct obj_data *obj, int n);
    int context;
  };

  constexpr number_field NUMBERS[] = {
      {'8', "Enter weight : ",           [](obj_data *o, int n) { obj_set_weight(o, LIMIT(n, 0, MAX_OBJ_WEIGHT)); },     CONTEXT_OEDIT_WEIGHT     },
      {'9', "Enter cost : ",             [](obj_data *o, int n) { obj_set_cost(o, LIMIT(n, 0, MAX_OBJ_COST)); },         CONTEXT_OEDIT_COST       },
      {'a', "Enter cost per day : ",     [](obj_data *o, int n) { obj_set_cost_per_day(o, LIMIT(n, 0, MAX_OBJ_RENT)); }, CONTEXT_OEDIT_COSTPERDAY },
      {'b', "Enter timer : ",            [](obj_data *o, int n) { obj_set_timer(o, LIMIT(n, obj_get_type_flag(o) == ITEM_PORTAL ? -1 : 0, MAX_OBJ_TIMER)); },
       CONTEXT_OEDIT_TIMER},
      {'m', "Enter new minimum level: ", [](obj_data *o, int n) { obj_set_level(o, MAX(n, 0)); },                         CONTEXT_OEDIT_LEVEL      },
  };

  /* A text of the main menu's: its key, prompt, and how it's read and kept */
  struct text_field {
    char key;
    std::string_view prompt;
    std::string_view (*get)(const struct obj_data *obj);
    void (*set)(struct obj_data *obj, const std::string &text);
    int context;
  };

  constexpr text_field TEXTS[] = {
      {'1', "Enter namelist : ",        obj_get_name,              [](obj_data *o, const std::string &t) { obj_set_name(o, t); },              CONTEXT_OEDIT_EDIT_NAMELIST},
      {'2', "Enter short desc : ",      obj_get_short_description, [](obj_data *o, const std::string &t) { obj_set_short_description(o, t); }, CONTEXT_OEDIT_SHORTDESC   },
      {'3', "Enter long desc :-\r\n| ", obj_get_description,       [](obj_data *o, const std::string &t) { obj_set_description(o, t); },       CONTEXT_OEDIT_LONGDESC    },
  };

  /* A flags menu of the main menu's, shown and toggled from by number */
  struct flag_menu {
    char key;
    void (*show)(GameWorld *g, struct descriptor_data *d, const struct obj_data &obj);
  };

  void show_extra_desc(GameWorld *g, struct descriptor_data *d, const extra_descr_data &desc, bool has_next)
  {
    clear_screen(g, d);
    write_to_output(g, d,
                    "Extra desc menu\r\n@g1@n) Keyword: @y{}@n\r\n@g2@n) Description:\r\n@y{}@n\r\n@g3@n) Goto next description: {}\r\n@g0@n) Quit\r\nEnter choice : ",
                    !desc.keyword.empty() ? desc.keyword : "<NONE>", !desc.description.empty() ? desc.description : "<NONE>",
                    has_next ? "Set." : "Not set.");
  }

  constexpr extra_desc_editor::style EXTRA_DESCS = {
      .show               = show_extra_desc,
      .keyword_prompt     = "Enter keywords, separated by spaces :-\r\n| ",
      .description_prompt = "Enter the extra description:\r\n\r\n",
      .says_unfinished    = false,
      .others_leave       = false,
      .contexts           = {CONTEXT_OEDIT_EXTRADESC_MENU, CONTEXT_OEDIT_EXTRADESC_KEY, CONTEXT_OEDIT_EXTRADESC_DESCRIPTION},
  };

  /* The object editor: a copy of an object prototype, put into the world on saving, or (iedit) of one object in
     the game, put back into it on quitting */
  struct object_editor final : olc_session {
    enum class mode {
      main_menu,
      confirm_save,
      copy,
      confirm_delete,
      text,   /* one of TEXTS */
      number, /* one of NUMBERS */
      action_desc,
      type,
      extras,
      wear,
      material,
      perm,
      size,
      value, /* the value numbered value_number, as the item's type has it */
      prompt_apply,
      apply,
      apply_mod,
      apply_spec,
      prompt_spellbook,
      spellbook,
      extra_descs,
      script,
    };

    mode asking = mode::main_menu;
    obj_vnum vnum{};
    struct zone_data *zone{};
    obj_data obj;
    bool changed{};
    bool individual{};     /* iedit: the object is one in the game */
    int32_t individual_id{}; /* which, by id */
    int slot{};            /* the apply or spellbook entry being edited */
    int value_number{};    /* the value being asked for, from 1 */
    const text_field *text{};
    const number_field *number{};
    extra_desc_editor extra_descs{&EXTRA_DESCS};
    script_editor script{OBJ_TRIGGER};

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void text_done(GameWorld *g, struct descriptor_data *d, int) override
    {
      if (asking == mode::action_desc)
        menu(g, d);
      else if (asking == mode::extra_descs)
        extra_descs.text_done(g, d);
    }
    int help_context(const struct descriptor_data *d) const override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override { log_stops_editing(g, d, zone ? zone->number : NOWHERE); }

    void edit_new();
    void edit_copy_of(GameWorld *g, struct obj_data *proto);
    void menu(GameWorld *g, struct descriptor_data *d);
    void done(GameWorld *g, struct descriptor_data *d) /* something was changed: back to the main menu */
    {
      changed = true;
      menu(g, d);
    }
    void main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void commit(GameWorld *g, struct descriptor_data *d);
    void list(GameWorld *g, struct descriptor_data *d, mode shown, list_menu show);

    void ask_value(GameWorld *g, struct descriptor_data *d, int n);
    void value_answer(GameWorld *g, struct descriptor_data *d, int n);
    void second_value_answer(GameWorld *g, struct descriptor_data *d, int n);

    void prompt_apply_menu(GameWorld *g, struct descriptor_data *d);
    void apply_list(GameWorld *g, struct descriptor_data *d);
    void apply_answer(GameWorld *g, struct descriptor_data *d, int n);
    void apply_spec_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void prompt_spellbook_menu(GameWorld *g, struct descriptor_data *d);
    void spell_list(GameWorld *g, struct descriptor_data *d);
    void spellbook_answer(GameWorld *g, struct descriptor_data *d, int n);
  };

  /* ---- the lists the menus choose from ---- */

  void type_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, item_types, NUM_ITEM_TYPES, 0, " ", 3);
    write_to_output(g, d, "\r\nEnter object type : ");
  }

  void extras_menu(GameWorld *g, struct descriptor_data *d, object_editor &e)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, extra_bits, flag_count<extra_flag>, 1, " ", 3);
    write_to_output(g, d, "\r\nObject flags: @c{}@n\r\nEnter object extra flag (0 to quit) : ",
                    flag_names(obj_get_extra_flags(&e.obj)));
  }

  void wear_menu(GameWorld *g, struct descriptor_data *d, object_editor &e)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, wear_bits, flag_count<wear_flag>, 1, " ", 3);
    write_to_output(g, d, "\r\nWear flags: @c{}@n\r\nEnter wear flag, 0 to quit : ", flag_names(obj_get_wear_flags(&e.obj)));
  }

  void material_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, material_names, NUM_MATERIALS, 0, "", 3);
    write_to_output(g, d, "\r\n@nEnter material type : ");
  }

  void perm_menu(GameWorld *g, struct descriptor_data *d, object_editor &e)
  {
    clear_screen(g, d);
    int columns = 0;
    for (int counter = 1; std::cmp_less(counter, flag_count<aff_flag>); counter++) {
      if (counter == std::to_underlying(aff_flag::charm)) /* Setting aff_flag::charm on objects like this is dangerous. */
        continue;
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20}{}", counter, affected_bits[counter], !(++columns % 3) ? "\r\n" : "");
    }
    write_to_output(g, d, "\r\nObject permanent flags: @c{}@n\r\nEnter object perm flag (0 to quit) : ",
                    flag_names(obj_get_bitvector(&e.obj)));
  }

  void size_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, size_names, NUM_SIZES, 1, "", 3);
    write_to_output(g, d, "\r\nEnter object size : ");
  }

  /* The lists the main menu shows for a key, and what is then asked */
  struct list_choice {
    char key;
    object_editor::mode asks;
    list_menu show;
  };

  constexpr list_choice LISTS[] = {
      {'5', object_editor::mode::type,     type_menu    },
      {'6', object_editor::mode::extras,   extras_menu  },
      {'7', object_editor::mode::wear,     wear_menu    },
      {'n', object_editor::mode::material, material_menu},
      {'p', object_editor::mode::perm,     perm_menu    },
      {'y', object_editor::mode::size,     size_menu    },
  };

  /* ---- the lists the values are chosen from ---- */

  void spells_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    int columns = 0;
    for (int counter = 0; counter < SKILL_TABLE_SIZE; counter++)
      if (skill_type(g, counter).test(skill_kind::spell))
        write_to_output(g, d, "@g{:>2d}@n) @y{:<20.20}@n{}", counter, g->spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    write_to_output(g, d, "\r\n@nEnter spell choice (-1 for none) : ");
  }

  void armor_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, armor_type, MAX_ARMOR_TYPES + 1, 0, " ", 3);
    write_to_output(g, d, "\r\nEnter armor proficiency type : ");
  }

  void container_flags_menu(GameWorld *g, struct descriptor_data *d, object_editor &e)
  {
    clear_screen(g, d);
    write_to_output(g, d, "@g1@n) CLOSEABLE\r\n@g2@n) PICKPROOF\r\n@g3@n) CLOSED\r\n@g4@n) LOCKED\r\nContainer flags: @c{}@n\r\nEnter flag, 0 to quit : ",
                    flag_mask_names(obj_container_flags(&e.obj)));
  }

  void liquid_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    int columns = 0;
    for (int counter = 0; counter < NUM_LIQ_TYPES; counter++)
      write_to_output(g, d, " @g{:>2d}@n) @y{:<20.20}@n{}", counter, drinks[counter], !(++columns % 3) ? "\r\n" : "");
    write_to_output(g, d, "\r\n@nEnter drink type : ");
  }

  void attack_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    int columns = 0;
    for (int counter = 0; counter < NUM_ATTACK_TYPES; counter++)
      write_to_output(g, d, "@g{:>2d}@n) {:<20.20} {}", counter, attack_hit_text[counter].singular, !(++columns % 3) ? "\r\n" : "");
    write_to_output(g, d, "\r\nEnter weapon type : ");
  }

  void weapon_type_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    olc_list_columns(g, d, weapon_type, MAX_WEAPON_TYPES + 1, 0, " ", 3);
    write_to_output(g, d, "\r\nEnter the weapon type for determining proficiencies: \r\n");
  }

  void crit_menu(GameWorld *g, struct descriptor_data *d, object_editor &)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, crit_type, MAX_CRIT_TYPE + 1, 0, " ", 3);
    write_to_output(g, d, "\r\nEnter critical type : ");
  }

  /* A window's viewport room is asked for only when it has no vehicle; otherwise the values are done */
  void window_viewport(GameWorld *g, struct descriptor_data *d, object_editor &e)
  {
    if (obj_get_value(&e.obj, 0) < 0)
      write_to_output(g, d, "What is the viewport room vnum (-1 for default location) : ");
    else
      e.menu(g, d);
  }

  /* ---- the values, as each item type has them ---- */

  /* What a value asks of an item type: a prompt, a list to choose from, or another value instead */
  struct value_question {
    int type;
    std::string_view prompt;
    list_menu show   = nullptr;
    int instead      = 0; /* the value asked instead of this one */
  };

  /* What a value of an item type may be */
  struct value_range {
    int type;
    int min, max;
  };

  struct value_step {
    std::span<const value_question> questions;
    std::span<const value_range> ranges;
    value_range otherwise;  /* the range of the types without one */
    int others;             /* the value the types without a question go on to; 0 for the main menu */
    int next;               /* the value asked after this one; 0 for the main menu */
    int context;
  };

  constexpr value_question VALUE_1[] = {
      {ITEM_LIGHT,    {},                                         nullptr,          3},
      {ITEM_SCROLL,   "Spell level : "},
      {ITEM_WAND,     "Spell level : "},
      {ITEM_STAFF,    "Spell level : "},
      {ITEM_POTION,   "Spell level : "},
      {ITEM_WEAPON,   {},                                         weapon_type_menu},
      {ITEM_ARMOR,    "Apply to AC : "},
      {ITEM_CONTAINER, "Max weight to contain (-1 for unlimited) : "},
      {ITEM_DRINKCON, "Max drink units (-1 for unlimited) : "},
      {ITEM_FOUNTAIN, "Max drink units (-1 for unlimited) : "},
      {ITEM_FOOD,     "Hours to fill stomach : "},
      {ITEM_MONEY,    "Number of zenni : "},
      {ITEM_NOTE,     {}}, /* quirk: meant for a language, it asks nothing, and waits for the value all the same */
      {ITEM_VEHICLE,  "Enter room vnum of vehicle interior : "},
      {ITEM_HATCH,    "Enter vnum of the vehicle this hatch belongs to : "},
      {ITEM_WINDOW,   "Enter vnum of the vehicle this window belongs to, or -1 to specify the viewport room : "},
      {ITEM_CONTROL,  "Enter vnum of the vehicle these controls belong to : "},
      {ITEM_PORTAL,   "Which room number is the destination? : "},
      {ITEM_BOARD,    "Enter the minimum admin level to read this board (0 for mortals) : "},
  };
  constexpr value_range RANGES_1[] = {
      {ITEM_WEAPON,    WEAPON_TYPE_UNARMED, MAX_WEAPON_TYPES  },
      {ITEM_CONTAINER, -1,                  MAX_CONTAINER_SIZE},
  };

  constexpr value_question VALUE_2[] = {
      {ITEM_SCROLL,    {},                                              spells_menu},
      {ITEM_POTION,    {},                                              spells_menu},
      {ITEM_WAND,      "Max number of charges : "},
      {ITEM_STAFF,     "Max number of charges : "},
      {ITEM_WEAPON,    "Number of damage dice : "},
      {ITEM_ARMOR,     {},                                              armor_menu},
      {ITEM_FOOD,      {},                                              nullptr, 4},
      {ITEM_CONTROL,   "Enter Engine Type ( 1, 2, 3) : "},
      {ITEM_CONTAINER, {},                                              container_flags_menu},
      {ITEM_VEHICLE,   {},                                              container_flags_menu},
      {ITEM_HATCH,     {},                                              container_flags_menu},
      {ITEM_WINDOW,    {},                                              container_flags_menu},
      {ITEM_PORTAL,    {},                                              container_flags_menu},
      {ITEM_DRINKCON,  "Initial drink units : "},
      {ITEM_FOUNTAIN,  "Initial drink units : "},
      {ITEM_BOARD,     "Minimum admin level to write (0 for mortals) : "},
  };

  constexpr value_question VALUE_3[] = {
      {ITEM_LIGHT,     "Number of hours (0 = burnt, -1 is infinite) : "},
      {ITEM_WAND,      "Number of charges remaining : "},
      {ITEM_STAFF,     "Number of charges remaining : "},
      {ITEM_WEAPON,    "Size of damage dice : "},
      {ITEM_ARMOR,     "Max dex bonus : "},
      {ITEM_CONTAINER, "Vnum of key to open container (-1 for no key) : "},
      {ITEM_DRINKCON,  {},                                                   liquid_menu},
      {ITEM_FOUNTAIN,  {},                                                   liquid_menu},
      {ITEM_VEHICLE,   "Vnum of key to unlock vehicle (-1 for no key) : "},
      {ITEM_HATCH,     "Vnum of key to unlock hatch (-1 for no key) : "},
      {ITEM_WINDOW,    "Vnum of key to unlock window (-1 for no key) : "},
      {ITEM_PORTAL,    "Vnum of the key to unlock portal (-1 for no key) : "},
      {ITEM_BOARD,     "Minimum admin level to remove messages (0 for mortals) : "},
  };
  constexpr value_range RANGES_3[] = {
      {ITEM_WEAPON,   1, MAX_WEAPON_SDICE },
      {ITEM_ARMOR,    0, 100              },
      {ITEM_WAND,     0, 20               },
      {ITEM_STAFF,    0, 20               },
      {ITEM_DRINKCON, 0, NUM_LIQ_TYPES - 1},
      {ITEM_FOUNTAIN, 0, NUM_LIQ_TYPES - 1},
      {ITEM_KEY,      0, 60000            },
  };

  constexpr value_question VALUE_4[] = {
      {ITEM_WAND,     {},                                                              spells_menu},
      {ITEM_STAFF,    {},                                                              spells_menu},
      {ITEM_WEAPON,   {},                                                              attack_menu},
      {ITEM_ARMOR,    "Armor check penalty : "},
      {ITEM_DRINKCON, "Poisoned (0 = not poison) : "},
      {ITEM_FOUNTAIN, "Poisoned (0 = not poison) : "},
      {ITEM_FOOD,     "Poisoned (0 = not poison) : "},
      {ITEM_VEHICLE,  "What is the vehicle's appearance? (-1 for transparent) : "},
      {ITEM_HATCH,    "Enter default vehicle load room : "},
      {ITEM_PORTAL,   "What is the portal's appearance? (-1 for transparent) : "},
      {ITEM_WINDOW,   {},                                                              window_viewport},
  };
  constexpr value_range RANGES_4[] = {
      {ITEM_HATCH,  1, 60000               },
      {ITEM_WAND,   1, SKILL_TABLE_SIZE - 1},
      {ITEM_STAFF,  1, SKILL_TABLE_SIZE - 1},
      {ITEM_WEAPON, 0, NUM_ATTACK_TYPES - 1},
      {ITEM_ARMOR,  0, 20                  }, /* a penalty, kept positive */
  };

  constexpr value_question VALUE_7[] = {
      {ITEM_WEAPON, {},                         crit_menu},
      {ITEM_ARMOR,  "Arcane spell failure % : "},
  };
  constexpr value_range RANGES_7[] = {
      {ITEM_WEAPON, 0,    MAX_CRIT_TYPE},
      {ITEM_ARMOR,  -100, 100          }, /* Want to allow weird armor that improves casting */
  };

  constexpr value_question VALUE_9[] = {
      {ITEM_WEAPON, "Default crit is only on natural 20. Extend this range by: "},
  };
  constexpr value_range RANGES_9[] = {
      {ITEM_WEAPON, 0, 19},
  };

  constexpr value_question VALUE_5[] = {
      {NOTHING, "Enter object default quality percentage (100% MAX): "},
  };

  /* The values asked for in turn: 1 to 4, the quality (5), 7 and 9. Value 2's answers are read by hand. */
  const value_step *step_of(int n)
  {
    static const value_step STEPS[] = {
        {},
        {VALUE_1, RANGES_1, {NOTHING, INT_MIN, INT_MAX}, 5, 2, CONTEXT_OEDIT_VALUE_1},
        {VALUE_2, {},       {},                          5, 3, CONTEXT_OEDIT_VALUE_2},
        {VALUE_3, RANGES_3, {NOTHING, -32000, 60000},    5, 4, CONTEXT_OEDIT_VALUE_3},
        {VALUE_4, RANGES_4, {NOTHING, -32000, 32000},    5, 5, CONTEXT_OEDIT_VALUE_4},
        {VALUE_5, {},       {NOTHING, 1, 100},           0, 7, NOTHING              },
        {},
        {VALUE_7, RANGES_7, {NOTHING, -32000, 32000},    9, 9, NOTHING              },
        {},
        {VALUE_9, RANGES_9, {NOTHING, -32000, 32000},    0, 0, NOTHING              },
    };
    return &STEPS[n];
  }

  /* ---- the editor ---- */

  void object_editor::edit_new()
  {
    clear_object(&obj);
    obj_set_name(&obj, "unfinished object");
    obj_set_description(&obj, "An unfinished object is lying here.");
    obj_set_short_description(&obj, "an unfinished object");
    obj_wear_set(&obj, wear_flag::take, true);
    obj_set_type_flag(&obj, ITEM_WORN);
    obj_set_value(&obj, VAL_ALL_HEALTH, 100);
    obj_set_value(&obj, VAL_ALL_MAXHEALTH, 100);
    obj_set_value(&obj, VAL_ALL_MATERIAL, MATERIAL_STEEL);
    obj_set_size(&obj, SIZE_MEDIUM);
    script.triggers.clear();
    changed = false;
  }

  /* The prototype's copy to edit: its triggers go to the script editor, and it has none of its own */
  void object_editor::edit_copy_of(GameWorld *g, struct obj_data *proto)
  {
    copy_object(g, &obj, proto);
    script.triggers = obj_get_proto_script(&obj);
    obj_set_script(&obj, nullptr);
    obj_set_proto_script(&obj, {});
    changed = false;
  }

  void object_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    write_to_output(g, d,
                    "-- Item number : [@c{:d}@n]\r\n@g1@n) Namelist : @y{}@n\r\n@g2@n) S-Desc   : @y{}@n\r\n@g3@n) L-Desc   :-\r\n@y{}@n\r\n@g4@n) A-Desc   :-\r\n@y{}@n@g5@n) Type        : @c{}@n\r\n@g6@n) Extra flags : @c{}@n\r\n",
                    vnum, or_if_empty(obj_get_name(&obj), "undefined"),
                    or_if_empty(obj_get_short_description(&obj), "undefined"),
                    or_if_empty(obj_get_description(&obj), "undefined"),
                    or_if_empty(obj_get_action_description(&obj), "Not Set.\r\n"),
                    type_name(item_types, obj_get_type_flag(&obj)), flag_names(obj_get_extra_flags(&obj)));

    std::string values;
    for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      values += str_format("{}{:d}", i ? " " : "", obj_get_value(&obj, i));
    write_to_output(g, d,
                    "@g7@n) Wear flags  : @c{}@n\r\n@g8@n) Weight      : @c{:<4d}@n, 	@g9@n) Cost        : @c{:<4d}@n\r\n@gA@n) Cost/Day    : @c{:<4d}@n, 	@gB@n) Timer       : @c{:<4d}@n\r\n@gC@n) Values      : @c{}@n\r\n@gD@n) Applies menu@n\r\n@gE@n) Extra descriptions menu {}\r\n@gM@n) Min Level   : @c{:d}@n\r\n@gN@n) Material    : @c{}@n\r\n@gP@n) Perm Affects: @c{}@n\r\n@gS@n) Script      : @c{}@n\r\n@gT@n) Spellbook menu\r\n@gW@n) Copy object        ,	@gX@n) Delete object\r\n@gY@n) Size        : @c{}@n\r\n@gZ@n) Wiznet      :\r\n@gQ@n) Quit\r\nEnter choice : ",
                    flag_names(obj_get_wear_flags(&obj)), obj_get_weight(&obj), obj_get_cost(&obj), obj_get_cost_per_day(&obj),
                    obj_get_timer(&obj), values, "Set." /* quirk: always; it once asked whether the extra flags (an array) were there */,
                    obj_get_level(&obj), material_names[obj_get_value(&obj, 7)], flag_names(obj_get_bitvector(&obj)),
                    !script.triggers.empty() ? "Set." : "Not Set.", size_names[obj_get_size(&obj)]);
    asking = mode::main_menu;
  }

  void object_editor::list(GameWorld *g, struct descriptor_data *d, mode shown, list_menu show)
  {
    show(g, d, *this);
    asking = shown;
  }

  void object_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const int n = parse_int(line);

    switch (asking) {
      case mode::confirm_save:
        save_answer(g, d, line);
        return;

      case mode::main_menu:
        main_menu_choice(g, d, line);
        return;

      case mode::copy:
        if (struct obj_data *proto = obj_proto_of(g, n))
          edit_copy_of(g, proto); /* quirk: which counts as a change */
        else
          write_to_output(g, d, "That object does not exist.\r\n");
        done(g, d);
        return;

      case mode::confirm_delete:
        if (const std::optional<bool> sure = yes_or_no(line); sure == true) {
          write_to_output(g, d, delete_object(g, obj_get_item_number(&obj)) != NOTHING ? "Object deleted.\r\n" : "Couldn't delete the object!\r\n");
          leave_olc(g, d);
        } else if (sure == false)
          menu(g, d);
        else
          write_to_output(g, d, "Please answer 'Y' or 'N': ");
        return;

      case mode::script:
        if (!script.parse(g, d, line))
          done(g, d);
        return;

      case mode::extra_descs:
        if (!extra_descs.parse(g, d, line))
          done(g, d);
        return;

      case mode::text: {
        std::string typed(line);
        smash_tilde(typed);
        if (!text->get(&obj).empty()) /* quirk: a text left empty can't be given one */
          text->set(&obj, text_or_undefined(typed));
        done(g, d);
        return;
      }

      case mode::number:
        number->set(&obj, n);
        done(g, d);
        return;

      case mode::action_desc: /* the text editor has the lines */
        done(g, d);
        return;

      case mode::type:
        if (n < 1 || n >= NUM_ITEM_TYPES) {
          write_to_output(g, d, "Invalid choice, try again : ");
          return;
        }
        obj_set_type_flag(&obj, n);
        for (int i = 3; i >= 0; i--) /* what's the boundschecking worth if we don't do this ? -- Welcor */
          obj_set_value(&obj, i, 0);
        done(g, d);
        return;

      case mode::extras:
        if (n == 0)
          done(g, d);
        else {
          if (std::optional<extra_flag> flag = flag_from_index<extra_flag>(n - 1))
            obj_extra_toggle(&obj, *flag);
          extras_menu(g, d, *this);
        }
        return;

      case mode::wear:
        if (n == 0)
          done(g, d);
        else {
          if (n < 0 || std::cmp_greater(n, flag_count<wear_flag>))
            write_to_output(g, d, "That's not a valid choice!\r\n");
          else if (std::optional<wear_flag> flag = flag_from_index<wear_flag>(n - 1))
            obj_wear_toggle(&obj, *flag);
          wear_menu(g, d, *this);
        }
        return;

      case mode::material:
        obj_set_value(&obj, 7, LIMIT(n, 0, NUM_MATERIALS));
        done(g, d);
        return;

      case mode::perm:
        if (n == 0) {
          done(g, d);
          return;
        }
        /* Setting aff_flag::charm on objects like this is dangerous. The menu's numbers are the flags' (quirk: so the last
           toggles bit 79, past them) */
        if (n > 0 && std::cmp_less_equal(n, flag_count<aff_flag>) && n != std::to_underlying(aff_flag::charm))
          obj_get_bitvector(&obj).toggle_bit(static_cast<std::size_t>(n));
        perm_menu(g, d, *this);
        return;

      case mode::size:
        obj_set_size(&obj, LIMIT(n - 1, 0, NUM_SIZES - 1));
        done(g, d);
        return;

      case mode::value:
        if (value_number == 2)
          second_value_answer(g, d, n);
        else
          value_answer(g, d, n);
        return;

      case mode::prompt_apply:
        if (n == 0)
          done(g, d);
        else if (n < 0 || n > MAX_OBJ_AFFECT)
          prompt_apply_menu(g, d);
        else {
          slot = n - 1;
          apply_list(g, d);
        }
        return;

      case mode::apply:
        apply_answer(g, d, n);
        return;

      case mode::apply_mod: {
        obj_affected_type &apply = obj_get_affected(&obj)[slot];
        apply.modifier           = n;
        if (apply.location == APPLY_FEAT || apply.location == APPLY_SKILL) {
          write_to_output(g, d, "\r\n{}", apply.location == APPLY_FEAT ? "What feat should be modified : " : "What skill should be modified : ");
          asking = mode::apply_spec;
        } else
          prompt_apply_menu(g, d);
        return;
      }

      case mode::apply_spec:
        apply_spec_answer(g, d, line);
        return;

      case mode::prompt_spellbook:
        if (n == 0)
          done(g, d);
        else if (n < 0 || n > SPELLBOOK_SIZE) /* a slot in sbinfo[] */
          prompt_spellbook_menu(g, d);
        else {
          slot = n - 1;
          spell_list(g, d);
        }
        return;

      case mode::spellbook:
        spellbook_answer(g, d, n);
        return;
    }
  }

  void object_editor::save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    switch (first_char(line)) {
      case 'y':
      case 'Y':
        if (add_object(g, &obj, vnum) == NOTHING)
          mud_log("oedit_save_internally: add_object failed.");
        else {
          obj_set_proto_script(obj_proto_of(g, vnum), script.triggers); /* for new copies */
          for (obj_data *live : each_obj(g)) {                         /* and those in the game */
            if (obj_get_item_number(live) != vnum)
              continue;
            if (obj_get_script(live))
              extract_script(g, live);
            copy_proto_script(obj_proto_of(g, vnum), live);
            assign_triggers(g, live);
          }
        }
        mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} edits obj {:d}", char_get_name(d->character), vnum);
        if (CONFIG_OLC_SAVE) {
          save_objects(g, find_zone_by_thing(g, vnum));
          write_to_output(g, d, "Object saved to disk.\r\n");
        } else
          write_to_output(g, d, "Object saved to memory.\r\n");
        if (obj_get_type_flag(&obj) == ITEM_BOARD) {
          if (struct board_info *board = locate_board(g, obj_get_vnum(g, &obj)))
            save_board(g, board);
          else
            create_new_board(g, obj_get_vnum(g, &obj));
        }
        leave_olc(g, d);
        return;
      case 'n':
      case 'N':
        leave_olc(g, d);
        return;
      case 'a': /* abort quit */
      case 'A':
        menu(g, d);
        return;
      default:
        write_to_output(g, d, "Invalid choice!\r\n");
        write_to_output(g, d, "Do you wish to save your changes? : \r\n");
        return;
    }
  }

  /* iedit's quitting: the object in the game becomes the edited copy, but stays where it is and who uses it */
  void object_editor::commit(GameWorld *g, struct descriptor_data *d)
  {
    struct obj_data *live = obj_by_id(g, individual_id);
    if (!live) { /* it was extracted while being edited */
      send_to_char(g, d->character, "\r\nThat object no longer exists. Changes discarded.\r\n");
      leave_olc(g, d);
      return;
    }
    send_to_char(g, d->character, "\r\nCommitting iedit changes.\r\n");
    if (obj_get_script(live)) { /* remove any old scripts */
      extract_script(g, live);
      obj_set_script(live, nullptr);
    }

    struct obj_data was = *live;
    *live               = obj;
    obj_set_in_room(live, obj_get_in_room(&was));
    obj_set_carried_by(live, obj_get_carried_by(&was));
    obj_set_worn_by(live, obj_get_worn_by(&was));
    obj_set_worn_on(live, obj_get_worn_on(&was));
    obj_set_in_obj(live, obj_get_in_obj(&was));
    *obj_contains_slot(live) = obj_get_contains(&was);
    obj_set_sitting(live, obj_get_sitting(g, &was));
    obj_set_user(live, obj_get_user(g, &was));
    obj_set_target(live, obj_get_target(g, &was));
    obj_set_id(live, obj_get_id(&was)); /* the same object still: whatever refers to it by id keeps it */
    obj_weight_changed(g, live, obj_get_weight(live) - obj_get_weight(&was));

    if (obj_get_vnum(g, live) != NOTHING) {
      copy_proto_script(obj_proto_of(g, obj_get_vnum(g, live)), live);
      assign_triggers(g, live);
    }
    obj_extra_set(live, extra_flag::unique_save, true);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} iedit a unique #{:d}", char_get_name(d->character),
           obj_get_vnum(g, live));
    if (d->character) { /* quirk: without the leaving's log line */
      char_plr_set(d->character, player_flag::writing, false);
      desc_set_connected(d, CON_PLAYING);
      act(g, "$n stops using OLC.", true, d->character, nullptr, nullptr, act_to::room);
    }
    d->olc.reset();
  }

  void object_editor::main_menu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = tolower(first_char(line));
    if (auto t = std::ranges::find(TEXTS, key, &text_field::key); key && t != std::end(TEXTS)) {
      write_to_output(g, d, t->prompt);
      text   = t;
      asking = mode::text;
      return;
    }
    if (auto f = std::ranges::find(NUMBERS, key, &number_field::key); key && f != std::end(NUMBERS)) {
      write_to_output(g, d, f->prompt);
      number = f;
      asking = mode::number;
      return;
    }
    if (auto l = std::ranges::find(LISTS, key, &list_choice::key); key && l != std::end(LISTS)) {
      list(g, d, l->asks, l->show);
      return;
    }

    switch (key) {
      case 'q':
        if (individual)
          commit(g, d);
        else if (changed) {
          write_to_output(g, d, "Do you wish to save your changes? : ");
          asking = mode::confirm_save;
        } else
          leave_olc(g, d);
        return;
      case '4':
        asking = mode::action_desc;
        send_editor_help(g, d);
        write_to_output(g, d, "Enter action description:\r\n\r\n");
        if (!obj_get_action_description(&obj).empty())
          write_to_output(g, d, "{}", obj_get_action_description(&obj));
        string_write(d, obj_action_description_slot(&obj), MAX_MESSAGE_LENGTH, 0);
        changed = true;
        return;
      case 'c':
        for (int i = 0; i <= 6; i++) /* Clear any old values */
          obj_set_value(&obj, i, 0);
        changed = true;
        ask_value(g, d, 1);
        return;
      case 'd':
        prompt_apply_menu(g, d);
        return;
      case 'e':
        asking = mode::extra_descs;
        extra_descs.open(g, d, *obj_ex_description_slot(&obj));
        return;
      case 's':
        if (individual) {
          write_to_output(g, d, "\r\nScripts cannot be modified on individual objects.\r\nEnter choice : ");
          return;
        }
        asking = mode::script;
        script.menu(g, d);
        return;
      case 't':
        prompt_spellbook_menu(g, d);
        return;
      case 'w':
        if (individual) { /* it would turn this object into a different prototype */
          write_to_output(g, d, "\r\nYou can't copy another object onto an individual object.\r\nEnter choice : ");
          return;
        }
        write_to_output(g, d, "Copy what object? ");
        asking = mode::copy;
        return;
      case 'x': /* quirk: iedit too, which deletes the object's prototype */
        write_to_output(g, d, "Are you sure you want to delete this object? ");
        asking = mode::confirm_delete;
        return;
      case 'z': {
        std::string message(line);
        search_replace(message, "z ", ""); /* quirk: every "z " in it, and not "Z " */
        do_wiznet(g, d->character, message, 0, 0);
        return;
      }
      default:
        menu(g, d);
        return;
    }
  }

  /* ---- the values ---- */

  void object_editor::ask_value(GameWorld *g, struct descriptor_data *d, int n)
  {
    if (n == 0) {
      menu(g, d);
      return;
    }
    asking               = mode::value;
    value_number         = n;
    const value_step &st = *step_of(n);
    const int type       = n == 5 ? NOTHING : obj_get_type_flag(&obj);
    auto q               = std::ranges::find(st.questions, type, &value_question::type);
    if (q == st.questions.end())
      ask_value(g, d, st.others);
    else if (q->instead)
      ask_value(g, d, q->instead);
    else if (q->show)
      q->show(g, d, *this);
    else if (!q->prompt.empty())
      write_to_output(g, d, q->prompt);
  }

  void object_editor::value_answer(GameWorld *g, struct descriptor_data *d, int n)
  {
    const value_step &st = *step_of(value_number);
    const int type       = obj_get_type_flag(&obj);
    auto r               = std::ranges::find(st.ranges, type, &value_range::type);
    const value_range &range = r == st.ranges.end() ? st.otherwise : *r;
    if (value_number == 4 && type == ITEM_ARMOR && n < 0) /* We're storing armor checks as positive numbers */
      n = -n;

    const int index = value_number - 1;
    obj_set_value(&obj, index, LIMIT(n, range.min, range.max));
    if (value_number == 5)
      obj_set_value(&obj, 5, range.max);
    if (st.next == 0)
      done(g, d);
    else
      ask_value(g, d, st.next);
  }

  /* The second value, whose answers differ more by type than a range */
  void object_editor::second_value_answer(GameWorld *g, struct descriptor_data *d, int n)
  {
    switch (obj_get_type_flag(&obj)) {
      case ITEM_SCROLL:
      case ITEM_POTION:
        obj_set_value(&obj, 1, n == 0 || n == -1 ? -1 : LIMIT(n, 1, SKILL_TABLE_SIZE - 1));
        ask_value(g, d, 3);
        return;
      case ITEM_CONTROL:
        obj_set_value(&obj, 1, LIMIT(n, 1, 3));
        ask_value(g, d, 5);
        return;
      case ITEM_CONTAINER:
      case ITEM_VEHICLE:
      case ITEM_WINDOW:
      case ITEM_HATCH:
      case ITEM_PORTAL: /* flags, toggled until 0 */
        if (n < 0 || n > 4)
          container_flags_menu(g, d, *this);
        else if (n != 0) {
          obj_set_value(&obj, 1, obj_get_value(&obj, 1) ^ (1U << (n - 1)));
          changed = true;
          ask_value(g, d, 2);
        } else
          ask_value(g, d, 3);
        return;
      case ITEM_WEAPON:
        obj_set_value(&obj, 1, LIMIT(n, 1, MAX_WEAPON_NDICE));
        ask_value(g, d, 3);
        return;
      default:
        obj_set_value(&obj, 1, n);
        ask_value(g, d, 3);
        return;
    }
  }

  /* ---- the applies ---- */

  void object_editor::prompt_apply_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    for (int counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
      const obj_affected_type &apply = obj_get_affected(&obj)[counter];
      if (!apply.modifier) {
        write_to_output(g, d, " @g{:d}@n) None.\r\n", counter + 1);
        continue;
      }
      write_to_output(g, d, " @g{:d}@n) {:+d} to @b{}@n", counter + 1, apply.modifier, type_name(apply_types, apply.location));
      if (apply.location == APPLY_FEAT)
        write_to_output(g, d, " ({})", g->feat_list[apply.specific].name);
      else if (apply.location == APPLY_SKILL)
        write_to_output(g, d, " ({})", g->spell_info[apply.specific].name);
      write_to_output(g, d, "\r\n");
    }
    write_to_output(g, d, "\r\nEnter affection to modify (0 to quit) : ");
    asking = mode::prompt_apply;
  }

  /* The applies one can be */
  void object_editor::apply_list(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    olc_list_columns(g, d, apply_types, NUM_APPLIES, 0, " ", 3);
    write_to_output(g, d, "\r\nEnter apply type (0 is no apply) : ");
    asking = mode::apply;
  }

  void object_editor::apply_answer(GameWorld *g, struct descriptor_data *d, int n)
  {
    obj_affected_type *applies = obj_get_affected(&obj);
    if (n == 0) {
      applies[slot].location = 0;
      applies[slot].modifier = 0;
      prompt_apply_menu(g, d);
    } else if (n < 0 || n >= NUM_APPLIES)
      apply_list(g, d);
    else {
      /* deny builders another of an apply already there */
      if (char_get_admlevel(d->character) < ADMLVL_GRGOD &&
          std::ranges::any_of(std::span(applies, MAX_OBJ_AFFECT), [n](const obj_affected_type &a) { return a.location == n; })) {
        write_to_output(g, d, "Object already has that apply.");
        return;
      }
      applies[slot].location = n;
      write_to_output(g, d, "Modifier : ");
      asking = mode::apply_mod;
    }
  }

  void object_editor::apply_spec_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    obj_affected_type &apply = obj_get_affected(&obj)[slot];
    if (isdigit(first_char(line)))
      apply.specific = affect_checked_specific(apply.location, parse_int(line));
    else if (apply.location == APPLY_SKILL) {
      std::string name(line);
      if (const int skill = find_skill_num(g, name, skill_kind::skill); skill > -1)
        apply.specific = skill;
    } else if (apply.location == APPLY_FEAT) {
      if (const int feat = find_feat_num(g, line); feat > -1)
        apply.specific = feat;
    } else
      apply.specific = 0;
    prompt_apply_menu(g, d);
  }

  /* ---- the spellbook ---- */

  void object_editor::prompt_spellbook_menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    const obj_spellbook_spell *book = obj_get_sbinfo(&obj);
    int columns                     = 0;
    for (int counter = 0; counter < SPELLBOOK_SIZE; counter++) {
      if (book && book[counter].spellname != 0 && book[counter].spellname < MAX_SPELLS)
        write_to_output(g, d, " @g{:>3d}@n) {:<20.20} {}", counter + 1, g->spell_info[book[counter].spellname].name, !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(g, d, " @g{:>3d}@n) None.{}", counter + 1, !(++columns % 3) ? "\r\n" : "");
    }
    write_to_output(g, d, "\r\nEnter spell to modify (0 to quit) : ");
    asking = mode::prompt_spellbook;
  }

  /* The spells a spellbook entry can be */
  void object_editor::spell_list(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    int columns = 0;
    for (int counter = 0; counter < SKILL_TABLE_SIZE; counter++)
      if (g->spell_info[counter].skilltype == skill_kind::spell)
        write_to_output(g, d, "@g{:>3d}@n) @y{:<20.20}@n{}", counter, g->spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    write_to_output(g, d, "@n\r\nEnter spell number (0 is no spell) : ");
    asking = mode::spellbook;
  }

  void object_editor::spellbook_answer(GameWorld *g, struct descriptor_data *d, int n)
  {
    if (n == 0) {
      if (obj_spellbook_spell *book = obj_get_sbinfo(&obj)) {
        book[slot].spellname = 0;
        book[slot].pages     = 0;
      } else
        obj_make_sbinfo(&obj);
      prompt_spellbook_menu(g, d);
      return;
    }
    if (n < 0 || n >= SKILL_TABLE_SIZE) {
      spell_list(g, d);
      return;
    }
    /* deny builders another of a spell already there (quirk: by their level, not their admin level) */
    if (char_get_level(d->character) < ADMLVL_IMPL && obj_get_sbinfo(&obj) &&
        std::ranges::any_of(std::span(obj_get_sbinfo(&obj), SPELLBOOK_SIZE), [n](const obj_spellbook_spell &s) { return s.spellname == n; })) {
      write_to_output(g, d, "Object already has that spell.");
      return;
    }
    obj_spellbook_spell *book = obj_get_sbinfo(&obj) ? obj_get_sbinfo(&obj) : obj_make_sbinfo(&obj);
    book[slot].spellname      = n;
    book[slot].pages          = MAX(1, g->spell_info[n].spell_level * 2);
    prompt_spellbook_menu(g, d);
  }

  int object_editor::help_context(const struct descriptor_data *) const
  {
    switch (asking) {
      case mode::main_menu: return CONTEXT_OEDIT_MAIN_MENU;
      case mode::confirm_save: return CONTEXT_OEDIT_CONFIRM_SAVESTRING;
      case mode::text: return text->context;
      case mode::number: return number->context;
      case mode::action_desc: return CONTEXT_OEDIT_ACTDESC;
      case mode::type: return CONTEXT_OEDIT_TYPE;
      case mode::extras: return CONTEXT_OEDIT_EXTRAS;
      case mode::wear: return CONTEXT_OEDIT_WEAR;
      case mode::perm: return CONTEXT_OEDIT_PERM;
      case mode::value: return step_of(value_number)->context;
      case mode::prompt_apply: return CONTEXT_OEDIT_PROMPT_APPLY;
      case mode::apply: return CONTEXT_OEDIT_APPLY;
      case mode::apply_mod: return CONTEXT_OEDIT_APPLYMOD;
      case mode::extra_descs: return extra_descs.help_context();
      case mode::script: return script.help_context();
      default: return NOTHING;
    }
  }
} // namespace

ACMD(do_oasis_oedit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);
  std::string buf2      = take_arg(rest);

  if (buf1.empty()) {
    send_to_char(g, ch, "Specify an object VNUM to edit.\r\n");
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
  }
  if (number == NOWHERE)
    number = parse_int(buf1);

  for (descriptor_data *dd : each_desc(g))
    if (desc_get_connected(dd) == CON_OEDIT && dd->olc && session_of<object_editor>(dd).vnum == number) {
      send_to_char(g, ch, "That object is currently being edited by {}.\r\n", char_pers(g, dd->character, ch));
      return;
    }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<object_editor>(), "do_oasis");
  object_editor &editor = session_of<object_editor>(d);

  if (!(editor.zone = zone_to_edit(g, ch, number, save))) {
    d->olc.reset();
    return;
  }

  if (save) {
    save_zone_files(g, ch, editor.zone, "objects", "object info", save_objects);
    d->olc.reset();
    return;
  }

  editor.vnum = number;
  if (struct obj_data *proto = obj_proto_of(g, number))
    editor.edit_copy_of(g, proto);
  else
    editor.edit_new();

  editor.menu(g, d);
  begin_editing(g, d, CON_OEDIT);
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} starts editing zone {:d} allowed zone {:d}", char_get_name(ch), editor.zone->number,
         char_get_olc_zone(g, ch));
}

ACMD(do_iedit)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty() || argument.empty()) /* quirk: and looks for it all the same */
    send_to_char(g, ch, "You must supply an object name.\r\n");

  struct obj_data *k = get_obj_in_equip_vis(g, ch, arg, nullptr, char_get_equipment_array(ch));
  if (!k)
    k = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!k)
    k = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)));
  if (!k)
    k = get_obj_vis(g, ch, arg, nullptr);
  if (!k) {
    send_to_char(g, ch, "Couldn't find that object. Sorry.\r\n");
    return;
  }

  struct descriptor_data *d = char_get_desc(ch);
  open_session(g, d, std::make_unique<object_editor>(), {});
  object_editor &editor = session_of<object_editor>(d);
  obj_extra_set(k, extra_flag::unique_save, true);
  char_plr_set(ch, player_flag::writing, true);

  /* A private copy, as oedit makes: it isn't in the game, and has no script; quirk: its number shows as 0 */
  copy_object(g, &editor.obj, k);
  obj_set_script(&editor.obj, nullptr);
  editor.individual    = true;
  editor.individual_id = obj_get_id(k);
  editor.menu(g, d);

  act(g, "$n starts using OLC.", true, ch, nullptr, nullptr, act_to::room);
  desc_set_connected(d, CON_IEDIT);
}
