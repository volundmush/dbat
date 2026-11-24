/* ************************************************************************
*   File: house.c                                       Part of CircleMUD *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "dbat/args.hpp"
#include "dbat/affect.hpp"
#include "dbat/house.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/utils.hpp"
#include "dbat/objsave.hpp"
#include "dbat/fileop.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/log.hpp"
#include "dbat/object_records.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static constexpr std::string_view house_bits_entries[] = {"!GUEST", "FREE", "!IMM", "IMP_ONLY", "RENTFREE", "SAVE_!RENT", "!SAVE"};
static_assert(std::size(house_bits_entries) == HOUSE_NUM_FLAGS);
constinit const name_table house_bits(house_bits_entries);

static void House_list_guests(GameWorld *g, struct char_data *ch, int i, int quiet);

/* local globals */

/* local functions */
static std::optional<std::string> House_get_filename(room_vnum vnum);
static bool House_load(GameWorld *g, room_vnum vnum);
static void House_save(GameWorld *g, const id_list &list, io::text_writer &out, int location);
static void House_restore_weight(GameWorld *g, const id_list &list);

static void hcontrol_build_house(GameWorld *g, struct char_data *ch, std::string_view arg);
static void hcontrol_destroy_house(GameWorld *g, struct char_data *ch, std::string_view arg);
static void hcontrol_pay_house(GameWorld *g, struct char_data *ch, std::string_view arg);

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
static std::optional<std::string> House_get_filename(room_vnum vnum)
{
  if (vnum == NOWHERE)
    return std::nullopt;
  return str_format("{}{:d}.house", LIB_HOUSE, vnum);
}

/* Save all objects for a house (recursive; initial call must be followed by a call to House_restore_weight) */
static void House_save(GameWorld *g, const id_list &list, io::text_writer &out, int location)
{
  std::vector<struct obj_data *> saved;
  std::vector<long> ids = list.ids();

  /* Newest first, an object that isn't to be rented is passed over, but only one at a time: the one after it is
   * saved whatever it is. That is how the old recursion along the list behaved, and what the house files hold. */
  for (std::size_t i = 0; i < ids.size(); i++) {
    struct obj_data *obj = obj_by_id(g, ids[i]);
    if (obj && obj_extra_flagged(obj, ITEM_NORENT) && i + 1 < ids.size())
      obj = obj_by_id(g, ids[++i]);
    else if (obj && obj_extra_flagged(obj, ITEM_NORENT))
      obj = nullptr;
    if (obj)
      saved.push_back(obj);
  }

  /* Oldest first, each after what it contains. */
  for (std::size_t i = saved.size(); i-- > 0;) {
    struct obj_data *obj = saved[i];
    House_save(g, obj_get_contains(obj), out, MIN(0, location) - 1);
    write_object_record(g, out, obj, location);
    for (struct obj_data *tmp = obj_get_in_obj(obj); tmp; tmp = obj_get_in_obj(tmp))
      obj_set_weight(tmp, obj_get_weight(tmp) - (obj_get_weight(obj)));
  }
}

/* restore weight of containers after House_save has changed them for saving */
static void House_restore_weight(GameWorld *g, const id_list &list)
{
  list.each([&](long id) {
    if (struct obj_data *obj = obj_by_id(g, id)) {
      House_restore_weight(g, obj_get_contains(obj));
      if (obj_get_in_obj(obj))
        obj_set_weight(obj_get_in_obj(obj), obj_get_weight(obj_get_in_obj(obj)) + (obj_get_weight(obj)));
    }
  });
}

/* Save all objects in a house */
static void House_crashsave(GameWorld *g, room_vnum vnum)
{
  if (!room_of(g, vnum))
    return;
  std::optional<std::string> path = House_get_filename(vnum);
  if (!path)
    return;
  io::text_writer out(*path, true);
  House_save(g, room_get_contents(g, vnum), out, 0);
  bool saved = out.commit();
  House_restore_weight(g, room_get_contents(g, vnum));
  if (saved) /* otherwise leave it flagged so the next pass tries again */
    room_set_flag(g, vnum, ROOM_HOUSE_CRASH, false);
}

/* Delete a house save file */
void House_delete_file(room_vnum vnum)
{
  std::optional<std::string> path = House_get_filename(vnum);
  if (!path)
    return;
  std::error_code ec;
  if (!std::filesystem::exists(*path, ec)) {
    if (ec)
      mud_log("SYSERR: Error deleting house file #{:d}. (1): {}", vnum, ec.message());
    return;
  }
  if (!std::filesystem::remove(*path, ec) && ec)
    mud_log("SYSERR: Error deleting house file #{:d}. (2): {}", vnum, ec.message());
}

/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/

int find_house(GameWorld *g, room_vnum vnum)
{
  int i;

  for (i = 0; i < g->num_of_houses; i++)
    if (g->house_control[i].vnum == vnum)
      return (i);

  return (NOWHERE);
}

/* call from boot_db - will load control recs, load objs, set atrium bits; a record whose owner or room is gone,
   or whose room is a house already, is dropped */
io::loaded<void> House_boot(GameWorld *g)
{
  for (house_control_rec &h : g->house_control)
    h = {};

  io::loaded<house_control_file> file = read_house_control();
  if (!file)
    return std::unexpected(file.error());
  if (file->missing) {
    mud_log("   No houses to load. File '{}' does not exist.", HCONTROL_FILE);
    return {};
  }

  for (const house_control_rec &temp_house : file->houses) {
    if (g->num_of_houses >= MAX_HOUSES)
      break;
    if (get_name_by_id(g, temp_house.owner) == nullptr)
      continue; /* owner no longer exists -- skip */
    if (!room_of(g, temp_house.vnum))
      continue; /* this vnum doesn't exist -- skip */
    if (find_house(g, temp_house.vnum) != NOWHERE)
      continue; /* this vnum is already a house -- skip */

    g->house_control[g->num_of_houses++] = temp_house;
    room_set_flag(g, temp_house.vnum, ROOM_HOUSE, true);
    House_load(g, temp_house.vnum);
  }

  House_save_control(g); /* keeps what it held as etc/hcontrol.bak */
  if (file->converted)
    mud_log("Converted {} from the old binary format ({:d} houses); the old file is kept as {}.bak", HCONTROL_FILE,
            file->houses.size(), HCONTROL_FILE);
  return {};
}

/* "House Control" functions */

static constexpr std::string_view HCONTROL_FORMAT =
    "Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
    "       hcontrol destroy <house vnum>\r\n"
    "       hcontrol pay <house vnum>\r\n"
    "       hcontrol show\r\n";

void hcontrol_list_houses(GameWorld *g, struct char_data *ch)
{
  int i;
  const std::string *temp;
  std::string built_on;
  std::string last_pay;
  std::string own_name;

  if (!g->num_of_houses) {
    send_to_char(g, ch, "No houses have been defined.\r\n");
    return;
  }
  send_to_char(g, ch,
               "Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < g->num_of_houses; i++) {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(g, g->house_control[i].owner)) == nullptr)
      continue;

    if (g->house_control[i].built_on)
      built_on = asctime_text(g->house_control[i].built_on).substr(0, 10);
    else
      built_on = "Unknown"; /* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (g->house_control[i].last_payment)
      last_pay = asctime_text(g->house_control[i].last_payment).substr(0, 10);
    else
      last_pay = "None"; /* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    own_name = *temp;
    send_to_char(g, ch, "{:>7d} {:<10}    {:>2d}    {:<12} {}\r\n",
                 g->house_control[i].vnum, built_on,
                 g->house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(g, ch, i, true);
  }
}

static void hcontrol_build_house(GameWorld *g, struct char_data *ch, std::string_view arg)
{
  std::string arg1;
  struct house_control_rec temp_house;
  room_vnum virt_house;
  room_vnum real_house;
  int16_t exit_num;
  long owner;

  if (g->num_of_houses >= MAX_HOUSES) {
    send_to_char(g, ch, "Max houses already defined.\r\n");
    return;
  }

  /* first arg: house's vnum */
  arg1 = take_arg(arg);
  if (arg1.empty()) {
    send_to_char(g, ch, "{}", HCONTROL_FORMAT);
    return;
  }
  virt_house = parse_int(arg1);
  if (!room_of(g, real_house = virt_house)) {
    send_to_char(g, ch, "No such room exists.\r\n");
    return;
  }
  if ((find_house(g, virt_house)) != NOWHERE) {
    send_to_char(g, ch, "House already exists.\r\n");
    return;
  }

  /* second arg: direction of house's exit */
  arg1 = take_arg(arg);
  if (arg1.empty()) {
    send_to_char(g, ch, "{}", HCONTROL_FORMAT);
    return;
  }
  if ((exit_num = search_block(arg1, dirs, false)) < 0 &&
      (exit_num = search_block(arg1, abbr_dirs, false)) < 0) {
    send_to_char(g, ch, "'{}' is not a valid direction.\r\n", arg1);
    return;
  }
  if (room_get_exit_to_room(g, real_house, exit_num) == NOWHERE) {
    send_to_char(g, ch, "There is no exit {} from room {:d}.\r\n", dirs[exit_num], virt_house);
    return;
  }

  /* third arg: player's name */
  arg1 = take_arg(arg);
  if (arg1.empty()) {
    send_to_char(g, ch, "{}", HCONTROL_FORMAT);
    return;
  }
  if ((owner = get_id_by_name(g, arg1)) < 0) {
    send_to_char(g, ch, "Unknown player '{}'.\r\n", arg1);
    return;
  }

  temp_house.mode          = HOUSE_PRIVATE;
  temp_house.vnum          = virt_house;
  temp_house.exit_num      = exit_num;
  temp_house.built_on      = game_now(g);
  temp_house.last_payment  = 0;
  temp_house.owner         = owner;
  temp_house.num_of_guests = 0;

  g->house_control[g->num_of_houses++] = temp_house;

  room_set_flag(g, real_house, ROOM_HOUSE, true);
  House_crashsave(g, virt_house);

  send_to_char(g, ch, "House built.  Mazel tov!\r\n");
  House_save_control(g);
}

static void hcontrol_destroy_house(GameWorld *g, struct char_data *ch, std::string_view arg)
{
  int i, j;
  room_vnum real_atrium, real_house;

  if (arg.empty()) {
    send_to_char(g, ch, "{}", HCONTROL_FORMAT);
    return;
  }
  if ((i = find_house(g, parse_int(arg))) == NOWHERE) {
    send_to_char(g, ch, "Unknown house.\r\n");
    return;
  }
  if (!room_of(g, real_atrium = g->house_control[i].atrium))
    mud_log("SYSERR: House {:d} had invalid atrium {:d}!", parse_int(arg), g->house_control[i].atrium);
  else
    room_set_flag(g, real_atrium, ROOM_ATRIUM, false);

  if (!room_of(g, real_house = g->house_control[i].vnum))
    mud_log("SYSERR: House {:d} had invalid vnum {:d}!", parse_int(arg), g->house_control[i].vnum);
  else {
    room_set_flag(g, real_house, ROOM_HOUSE, false);
    room_set_flag(g, real_house, ROOM_HOUSE_CRASH, false);
  }

  House_delete_file(g->house_control[i].vnum);

  for (j = i; j < g->num_of_houses - 1; j++)
    g->house_control[j] = g->house_control[j + 1];

  g->num_of_houses--;

  send_to_char(g, ch, "House deleted.\r\n");
  House_save_control(g);

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (i = 0; i < g->num_of_houses; i++)
    if (room_of(g, real_atrium = g->house_control[i].atrium))
      room_set_flag(g, real_atrium, ROOM_ATRIUM, true);
}

static void hcontrol_pay_house(GameWorld *g, struct char_data *ch, std::string_view arg)
{
  int i;

  if (arg.empty())
    send_to_char(g, ch, "{}", HCONTROL_FORMAT);
  else if ((i = find_house(g, parse_int(arg))) == NOWHERE)
    send_to_char(g, ch, "Unknown house.\r\n");
  else {
    mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "Payment for house {} collected by {}.", arg, char_get_name(ch));

    g->house_control[i].last_payment = game_now(g);
    House_save_control(g);
    send_to_char(g, ch, "Payment recorded.\r\n");
  }
}

/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  std::string arg1, arg2;

  std::string_view rest = argument;
  arg1                  = take_word(rest);
  arg2                  = skip_spaces(rest);

  if (is_abbrev(arg1, "build"))
    hcontrol_build_house(g, ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(g, ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(g, ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(g, ch);
  else
    send_to_char(g, ch, "{}", HCONTROL_FORMAT);
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  std::string arg;
  int i, j, id;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!room_flagged(g, char_get_in_room(ch), ROOM_HOUSE))
    send_to_char(g, ch, "You must be in your house to set guests.\r\n");
  else if ((i = find_house(g, char_get_in_room(ch))) == NOWHERE)
    send_to_char(g, ch, "Um.. this house seems to be screwed up.\r\n");
  else if (char_get_idnum(ch) != g->house_control[i].owner)
    send_to_char(g, ch, "Only the primary owner can set guests.\r\n");
  else if (arg.empty())
    House_list_guests(g, ch, i, false);
  else if ((id = get_id_by_name(g, arg)) < 0)
    send_to_char(g, ch, "No such player.\r\n");
  else if (id == char_get_idnum(ch))
    send_to_char(g, ch, "It's your house!\r\n");
  else {
    for (j = 0; j < g->house_control[i].num_of_guests; j++)
      if (g->house_control[i].guests[j] == id) {
        for (; j < g->house_control[i].num_of_guests; j++)
          g->house_control[i].guests[j] = g->house_control[i].guests[j + 1];
        g->house_control[i].num_of_guests--;
        House_save_control(g);
        send_to_char(g, ch, "Guest deleted.\r\n");
        return;
      }
    if (g->house_control[i].num_of_guests == MAX_GUESTS) {
      send_to_char(g, ch, "You have too many guests.\r\n");
      return;
    }
    j                          = g->house_control[i].num_of_guests++;
    g->house_control[i].guests[j] = id;
    House_save_control(g);
    send_to_char(g, ch, "Guest added.\r\n");
  }
}

/* Misc. administrative functions */

/* crash-save all the houses */
void House_save_all(GameWorld *g)
{
  int i;
  room_vnum real_house;

  for (i = 0; i < g->num_of_houses; i++)
    if (room_of(g, real_house = g->house_control[i].vnum))
      House_crashsave(g, g->house_control[i].vnum);
}

/* note: arg passed must be house vnum, so there. */
bool House_can_enter(GameWorld *g, struct char_data *ch, room_vnum house)
{
  int i, j;

  if (char_adm_flagged(ch, ADM_ALLHOUSES) || (i = find_house(g, house)) == NOWHERE)
    return true;

  switch (g->house_control[i].mode) {
    case HOUSE_CLAN:
    case HOUSE_UNOWNED:
      return true;
    case HOUSE_PRIVATE:
      if (char_get_idnum(ch) == g->house_control[i].owner)
        return true;
      for (j = 0; j < g->house_control[i].num_of_guests; j++)
        if (char_get_idnum(ch) == g->house_control[i].guests[j])
          return true;
  }

  return false;
}

static void House_list_guests(GameWorld *g, struct char_data *ch, int i, int quiet)
{
  int j, num_printed;
  const std::string *temp;

  if (g->house_control[i].num_of_guests == 0) {
    if (!quiet)
      send_to_char(g, ch, "  Guests: None\r\n");
    return;
  }

  send_to_char(g, ch, "  Guests: ");

  for (num_printed = j = 0; j < g->house_control[i].num_of_guests; j++) {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(g, g->house_control[i].guests[j])) == nullptr)
      continue;

    num_printed++;
    send_to_char(g, ch, "{} ", upper_first(*temp));
  }

  if (num_printed == 0)
    send_to_char(g, ch, "all dead");

  send_to_char(g, ch, "\r\n");
}

static bool House_load(GameWorld *g, room_vnum rvnum)
{
  if (!room_of(g, rvnum))
    return false;
  std::optional<std::string> path = House_get_filename(rvnum);
  if (!path)
    return false;
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file) {
    std::error_code ec;
    if (std::filesystem::exists(*path, ec)) /* if it fails, NOT because of no file */
      mud_log("SYSERR: READING HOUSE FILE {} (5): {}", *path, io::why_unreadable(*path));
    return false;
  }

  io::line_cursor in(*file);
  in.next();
  bag_rows rows(g, nullptr, rvnum);
  io::loaded<void> read = read_object_records(g, in, record_kind::house, [&](obj_data *temp, int locate) {
    obj_to_room(g, temp, rvnum);
    rows.place(temp, locate);
  });
  if (!read) {
    mud_log("SYSERR: {}", read.error().text());
    return false;
  }
  return true;
}
