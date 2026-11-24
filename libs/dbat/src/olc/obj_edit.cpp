/* ************************************************************************
*  File: obj_edit.c                                    Part of DBAT       *
*  Usage: Player level object editing                                     *
*                                                                         *
*  This is an original file created by me for Dragonball Advent Truth     *
*  to house all player level object editing functions -- Iovan 1/6/13     *
************************************************************************ */
#include <optional>
#include <string_view>

#include "dbat/act.other.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/handler.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/relocate.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"

namespace {
  using step = pobj_order::step;

  /* What custom equipment is, by its type */
  static constexpr std::string_view CUSTOM_TYPES[17] = {"Error",      "Armor Slot", "Gi Slot",   "Wrist Slot", "Ear Slot",  "Finger Slot",
                                            "Eye Slot",   "Hands Slot", "Feet Slot", "Belt Slot",  "Legs Slot", "Arms Slot",
                                            "Head Slot",  "Neck Slot",  "Back Slot", "Shoulder Slot", "Weapon"};

  /* Where custom equipment of each type (but the weapon) is worn */
  constexpr std::optional<wear_flag> CUSTOM_WEAR[16] = {std::nullopt,   ITEM_WEAR_BODY, ITEM_WEAR_ABOUT, ITEM_WEAR_WRIST, ITEM_WEAR_EAR,  ITEM_WEAR_FINGER,
                                   ITEM_WEAR_EYE,  ITEM_WEAR_HANDS, ITEM_WEAR_FEET, ITEM_WEAR_WAIST, ITEM_WEAR_LEGS, ITEM_WEAR_ARMS,
                                   ITEM_WEAR_HEAD, ITEM_WEAR_NECK, ITEM_WEAR_PACK,  ITEM_WEAR_SH};

  /* A custom weapon's kind, and the damage it does */
  static constexpr std::string_view CUSTOM_WEAPONS[7] = {"Not A Weapon", "Sword", "Dagger", "Spear", "Club", "Gun", "Brawling"};
  constexpr int CUSTOM_DAMAGE[7]          = {0, TYPE_SLASH, TYPE_PIERCE, TYPE_STAB, TYPE_CRUSH, TYPE_BLAST, TYPE_POUND};

  constexpr obj_vnum CUSTOM_ARMOR  = 20099;
  constexpr obj_vnum CUSTOM_WEAPON = 20098;

  /* One of the order's texts: where it goes, and how long it may be */
  struct order_text {
    step asked;
    std::string pobj_order::*text;
    size_t limit;
    std::string_view too_long;
    bool no_colour; /* colour codes refused */
  };

  constexpr order_text TEXTS[] = {
      {step::name,       &pobj_order::name,       100, "@wToo long. Limit is 100 character.@n\r\n", true },
      {step::short_desc, &pobj_order::short_desc, 150, "@wToo long. Limit is 200 character.@n\r\n", false}, /* quirk: says 200 */
      {step::long_desc,  &pobj_order::long_desc,  200, "@wToo long. Limit is 200 character.@n\r\n", false},
  };

  void custom_menu(GameWorld *g, struct descriptor_data *d)
  {
    const pobj_order &order = d->pobj;
    write_to_output(g, d, "@GCustom Equipment Construction Menu@n\n");
    write_to_output(g, d, "@D-----------------------------------------@n\n");
    write_to_output(g, d, "@C1@D) @WKeyword Name     @D: @w{}@n\n", order.name);
    write_to_output(g, d, "@C2@D) @WShort Description@D: @w{}@n\n", order.short_desc);
    write_to_output(g, d, "@C3@D) @WLong Description @D: @w{}@n\n", order.long_desc);
    write_to_output(g, d, "@C4@D) @WEquipment Type   @D: @w{}@n\n", CUSTOM_TYPES[order.type]);
    write_to_output(g, d, "@C5@D) @WWeapon Type      @D: @w{}@n\n", CUSTOM_WEAPONS[order.weapon]);
    write_to_output(g, d, "@CQ@D) @WQuit Menu@n\n");
  }

  void restring_menu(GameWorld *g, struct descriptor_data *d)
  {
    const pobj_order &order = d->pobj;
    write_to_output(g, d, "@GEquipment Restring Menu@n\n");
    write_to_output(g, d, "@D-----------------------------------------@n\n");
    write_to_output(g, d, "@C1@D) @WKeyword Name     @D: @w{}@n\n", order.name);
    write_to_output(g, d, "@C2@D) @WShort Description@D: @w{}@n\n", order.short_desc);
    write_to_output(g, d, "@C3@D) @WLong Description @D: @w{}@n\n", order.long_desc);
    write_to_output(g, d, "@CQ@D) @WQuit Menu@n\n");
  }

  void main_menu(GameWorld *g, struct descriptor_data *d)
  {
    if (d->pobj.what == pobj_order::kind::restring)
      restring_menu(g, d);
    else
      custom_menu(g, d);
    d->pobj.asking = step::main;
  }

  /* The order's player is done with the menu */
  void finish(struct descriptor_data *d)
  {
    d->pobj.asking = step::none;
    desc_set_connected(d, CON_PLAYING);
  }

  void answer_text(GameWorld *g, struct descriptor_data *d, const order_text &t, std::string_view line)
  {
    if (line.empty()) {
      write_to_output(g, d, "@wNothing entered. Keeping what was previously entered.@n\r\n");
      if (d->pobj.what == pobj_order::kind::restring && t.asked != step::name) /* quirk: the custom menu */
        custom_menu(g, d);
      else
        main_menu(g, d);
      d->pobj.asking = step::main;
      return;
    }
    if (t.no_colour && line.contains('@')) {
      write_to_output(g, d, "@RNO COLORCODE IN THE KEYWORD NAME.@n");
      write_to_output(g, d, "@wEnter: @n\n");
      return;
    }
    if (line.size() > t.limit) {
      write_to_output(g, d, t.too_long);
      write_to_output(g, d, "@wEnter: @n\n");
      return;
    }
    d->pobj.*t.text = line;
    main_menu(g, d);
  }

  /* A number to choose from 1 to most, or the error and the choice again; none for an empty line */
  void answer_number(GameWorld *g, struct descriptor_data *d, int pobj_order::*choice, std::string_view line, int most, int refused)
  {
    if (line.empty()) {
      write_to_output(g, d, "@wNothing entered. Returning to main menu.@n\r\n");
      main_menu(g, d);
      return;
    }
    pobj_order &order = d->pobj;
    order.*choice     = parse_int(line);
    if (order.*choice < 1 || order.*choice > most) {
      write_to_output(g, d, "@wValue must be between 1 and {:d}.@n\r\n", most);
      write_to_output(g, d, "@wEnter: @n\n");
      order.*choice = refused;
      return;
    }
    if (choice == &pobj_order::type && order.type == 16)
      order.weapon = 1;
    main_menu(g, d);
  }

  void restring_main(GameWorld *g, struct descriptor_data *d, char choice)
  {
    pobj_order &order = d->pobj;
    switch (choice) {
      case '1':
        write_to_output(g, d, "@wCurrent Equipment's Keyword Name @D<@Y{}@D>@n\r\n", order.name);
        write_to_output(g, d, "@wEnter Name @D(@RDo not use colorcode!@D)@w:@n \n");
        order.asking = step::name;
        break;
      case '2':
        write_to_output(g, d, "@wCurrent Equipment's Short Desc @D<@Y{}@D>@n\r\n", order.short_desc);
        write_to_output(g, d, "@wEnter Short Desc@w:@n \n");
        order.asking = step::short_desc;
        break;
      case '3':
        write_to_output(g, d, "@wCurrent Equipment's Long Desc @D<@Y{}@D>@n\r\n", order.long_desc);
        write_to_output(g, d, "@wEnter Long Desc@w:@n \n");
        order.asking = step::long_desc;
        break;
      case 'Q':
      case 'q':
        write_to_output(g, d, "Save current changes and be charged?\r\nYes or No\r\n");
        order.asking = step::quit;
        break;
      default:
        restring_menu(g, d);
        break;
    }
  }

  void restring_quit(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    pobj_order &order = d->pobj;
    if (str_eq_nocase(line, "yes") || str_eq_nocase(line, "y")) {
      struct obj_data *obj = obj_by_id(g, order.restrung_id);
      if (!obj) { /* it went while being restrung */
        write_to_output(g, d, "That item no longer exists. Canceling purchase at no cost.\r\n");
        finish(d);
        return;
      }
      obj_set_name(obj, order.name);
      obj_set_short_description(obj, order.short_desc);
      obj_set_description(obj, order.long_desc);
      order.what = pobj_order::kind::none;
      char_set_gold(d->character, char_get_gold(d->character) - 5000);
      obj_extra_set(obj, ITEM_RESTRING, true);
      write_to_output(g, d, "Purchase complete.");
      send_to_imm(g, "Restring Eq: {} has bought: {}, which was {}.", char_get_name(d->character), obj_get_short_description(obj), order.was);
      finish(d);
    } else if (str_eq_nocase(line, "no") || str_eq_nocase(line, "n")) {
      write_to_output(g, d, "Canceling purchase at no cost.\r\n");
      send_to_imm(g, "Restring Eq: {} has canceled their equipment restring.", char_get_name(d->character));
      finish(d);
    } else
      write_to_output(g, d, "Save current changes and be charged?\r\nYes or No\r\n");
  }

  void custom_main(GameWorld *g, struct descriptor_data *d, char choice)
  {
    pobj_order &order = d->pobj;
    switch (choice) {
      case '1':
        write_to_output(g, d, "@wEnter Equipment's Keyword Name @D(@RDo not use colorcode!@D)@w.@n\r\n");
        write_to_output(g, d, "@wEnter: @n\n");
        order.asking = step::name;
        break;
      case '2':
        write_to_output(g, d, "@wEnter Equipment's Short Description. This is the colored name you see in your inventory or when the eq is used.@n\r\n");
        write_to_output(g, d, "@wEnter: @n\n");
        order.asking = step::short_desc;
        break;
      case '3':
        write_to_output(g, d, "@wEnter Equipment's Long Description. This is the colored name you see when it's seen in a room. @D(@RNo punctuation at the end!@D)@n\r\n");
        write_to_output(g, d, "@wEnter: @n\n");
        order.asking = step::long_desc;
        break;
      case '4':
        write_to_output(g, d, "@wEnter number to select type of equipment you want it to be: @n\n");
        write_to_output(g, d, "@D[ @C--1@W) @cArmor Slot  @C--2@W) @cGi Slot     @C--3@W) @cWrist Slot   @D]@n\n");
        write_to_output(g, d, "@D[ @C--4@W) @cEar Slot    @C--5@W) @cFinger Slot @C--6@W) @cEye Slot     @D]@n\n");
        write_to_output(g, d, "@D[ @C--7@W) @cHands Slot  @C--8@W) @cFeet Slot   @C--9@W) @cBelt Slot    @D]@n\n");
        write_to_output(g, d, "@D[ @C-10@W) @cLegs Slot   @C-11@W) @cArms Slot   @C-12@W) @cHead Slot    @D]@n\n");
        write_to_output(g, d, "@D[ @C-13@W) @cNeck Slot   @C-14@W) @cBack Slot   @C-15@W) @cShoulder Slot@D]@n\n");
        write_to_output(g, d, "@D[ @C-16@W) @cWeapon@n\r\n");
        write_to_output(g, d, "@wEnter: @n\n");
        order.asking = step::type;
        break;
      case '5':
        if (order.type != 16) {
          write_to_output(g, d, "@wYou can only use this part of the menu if you select the weapon type.@n\r\n");
          return;
        }
        write_to_output(g, d, "@wEnter number to select type of weapon you want it to be: @n\n");
        write_to_output(g, d, "@D[ @C--1@W) @cSword       @C--2@W) @cDagger      @C--3@W) @cSpear        @D]@n\n");
        write_to_output(g, d, "@D[ @C--4@W) @cClub        @C--5@W) @cGun         @C--6@W) @cBrawling     @D]@n\n");
        write_to_output(g, d, "@wEnter: @n\n");
        order.asking = step::weapon;
        break;
      case 'q':
      case 'Q':
        write_to_output(g, d, "@wPurchase this custom piece? (Y or N)@n\r\n");
        order.asking = step::quit;
        break;
      default:
        custom_menu(g, d);
        break;
    }
  }

  void custom_quit(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    pobj_order &order = d->pobj;
    if (str_eq_nocase(line, "n")) {
      write_to_output(g, d, "Canceling purchase at no cost.\r\n");
      send_to_imm(g, "Custom Eq: {} has canceled their custom eq construction.", char_get_name(d->character));
      finish(d);
      return;
    }
    if (!str_eq_nocase(line, "y")) /* quirk: anything else is ignored, and the question not asked again */
      return;

    write_to_output(g, d, "@wPurchase complete.@n\r\n");
    desc_set_connected(d, CON_PLAYING);
    struct obj_data *obj = read_object(g, order.weapon == 0 ? CUSTOM_ARMOR : CUSTOM_WEAPON);
    obj_to_char(g, obj, d->character);
    if (order.weapon == 0 && order.type < (int)std::size(CUSTOM_WEAR) && CUSTOM_WEAR[order.type])
      obj_wear_set(obj, *CUSTOM_WEAR[order.type], true);
    obj_set_name(obj, order.name);
    obj_set_short_description(obj, order.short_desc);
    obj_set_description(obj, order.long_desc);
    if (order.weapon != 0) {
      obj_set_value(obj, VAL_WEAPON_DAMTYPE, CUSTOM_DAMAGE[order.weapon] - TYPE_HIT);
      obj_set_level(obj, 20);
    }
    obj_extra_set(obj, ITEM_SLOT2, true);
    order.what   = pobj_order::kind::none;
    order.asking = step::none;
    char_set_rp(d->character, char_get_rp(d->character) - 20);
    d->acct.rpp -= 20;
    save_account(d);
    obj_set_size(obj, get_size(d->character));
    obj_extra_set(obj, ITEM_CUSTOM, true);
    send_to_imm(g, "Custom Eq: {} has bought: {}.", char_get_name(d->character), obj_get_short_description(obj));
    customWrite(d->character, obj);
    log_custom(g, d, obj);
  }
} // namespace

void start_custom_order(GameWorld *g, struct descriptor_data *d)
{
  desc_set_connected(d, CON_POBJ);
  d->pobj = {.what       = pobj_order::kind::custom,
             .name       = "Generic Armor Vest",
             .short_desc = "@cGeneric @DArmor @WVest@n",
             .long_desc  = "@wA @cgeneric @Darmor @Wvest@w is lying here@n",
             .type       = 1};
  main_menu(g, d);
}

void start_restring(GameWorld *g, struct descriptor_data *d, struct obj_data *obj)
{
  desc_set_connected(d, CON_POBJ);
  d->pobj = {.what        = pobj_order::kind::restring,
             .name        = str_format("{}", obj_get_name(obj)),
             .short_desc  = str_format("{}", obj_get_short_description(obj)),
             .long_desc   = str_format("{}", obj_get_description(obj)),
             .was         = str_format("{}", obj_get_short_description(obj)),
             .type        = 1,
             .restrung_id = obj_get_id(obj)};
  main_menu(g, d);
}

void pobj_edit_parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
{
  pobj_order &order           = d->pobj;
  const bool restring         = order.what == pobj_order::kind::restring;
  if (!restring && order.what != pobj_order::kind::custom)
    return;

  if (auto t = std::ranges::find(TEXTS, order.asking, &order_text::asked); t != std::end(TEXTS)) {
    answer_text(g, d, *t, line);
    return;
  }

  switch (order.asking) {
    case step::main:
      (restring ? restring_main : custom_main)(g, d, line.empty() ? '\0' : line.front());
      return;
    case step::type:
      if (!restring)
        answer_number(g, d, &pobj_order::type, line, 16, 1);
      return;
    case step::weapon:
      if (!restring)
        answer_number(g, d, &pobj_order::weapon, line, 6, 0);
      return;
    case step::quit:
      if (line.empty())
        write_to_output(g, d, restring ? "Save current changes and be charged?\r\nYes or No\r\n" : "@wPurchase this custom piece? (Y or N)@n\r\n");
      else
        (restring ? restring_quit : custom_quit)(g, d, line);
      return;
    default: /* whoops */
      if (restring)
        main_menu(g, d);
      return;
  }
}
