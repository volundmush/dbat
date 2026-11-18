/* ************************************************************************
 *   File: objsave.c                                     Part of CircleMUD *
 *  Usage: loading/saving player objects for rent and crash-save           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/iterate.hpp"
#include "dbat/affect.hpp"
#include "dbat/objsave.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/utils.hpp"
#include "dbat/spells.hpp"
#include "dbat/players.hpp"
#include "dbat/class.hpp"
#include "dbat/act.social.hpp"
#include "dbat/act.item.hpp"
#include "dbat/fileop.hpp"
#include "dbat/consts/rents.hpp"
#include "dbat/config.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/log.hpp"
#include "dbat/object_records.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions */

static void auto_equip(GameWorld *g, struct char_data *ch, struct obj_data *obj, int location);
static void Crash_save(GameWorld *g, struct obj_data *obj, io::text_writer &out, int location);
static void Crash_restore_weight(GameWorld *g, struct obj_data *obj);
static void Crash_restore_weight_list(GameWorld *g, const id_list &list);
static void Crash_save_list(GameWorld *g, const id_list &list, io::text_writer &out, int location);
static void Crash_extract_objs_from(GameWorld *g, const std::vector<long> &ids, std::size_t i);
static void Crash_extract_norents_from(GameWorld *g, const std::vector<long> &ids, std::size_t i);
static void Crash_extract_objs(GameWorld *g, struct obj_data *obj);
static bool Crash_is_unrentable(struct obj_data *obj);
static void Crash_extract_norents(GameWorld *g, struct obj_data *obj);
static bool Crash_load_file(GameWorld *g, struct char_data *ch, io::text_reader &in);

/* A rent file's first line: "<rent code> <when> <cost> <gold> <bank> <items>", 0 for any it doesn't have */
struct rent_header {
  int rentcode = 0, netcost = 0, gold = 0, account = 0, nitems = 0;
  long long timed = 0;
};

static rent_header read_rent_header(std::string_view line, int rentcode)
{
  rent_header h{.rentcode = rentcode};
  io::fields f(line);
  f.number(h.rentcode) && f.number(h.timed) && f.number(h.netcost) && f.number(h.gold) && f.number(h.account) &&
      f.number(h.nitems);
  return h;
}

/* The rent file of name, open; nothing, logging why with the message's number if it is there but can't be read
   (unless doing is null) */
static std::optional<io::text_reader> open_rent_file(const char *name, const char *doing, int number,
                                                     std::string *path_out = nullptr)
{
  std::optional<std::string> path = player_path(NEW_OBJ_FILES, name);
  if (!path)
    return std::nullopt;
  if (path_out)
    *path_out = *path;
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file) {
    std::error_code ec;
    if (doing && std::filesystem::exists(*path, ec))
      mud_log("SYSERR: {} {} ({:d}): {}", doing, *path, number, io::why_unreadable(*path));
    return std::nullopt;
  }
  return std::move(*file);
}

static bool Crash_write_safe_file(GameWorld *g, struct char_data *ch, int rentcode, int cost, const char *save_type)
{
  std::optional<std::string> path = player_path(NEW_OBJ_FILES, char_get_name(ch));
  if (!path) {
    mud_log("SYSERR: {}:{:d}: get_filename failed for {} of {}", __FILE__, __LINE__, save_type, char_get_name(ch));
    return false;
  }

  /* the replaced file stays as <name>.bak, which Crash_load() falls back to */
  io::text_writer out(*path, true);
  out.print("{:d} {:d} {:d} {:d} {:d} {:d}\r\n", rentcode, static_cast<long long>(game_now(g)), cost,
            char_get_gold(ch), char_get_bank_gold(ch), 0);
  for (int j = 0; j < NUM_WEARS; j++)
    if (char_get_equipment(ch, j)) {
      Crash_save(g, char_get_equipment(ch, j), out, j + 1);
      Crash_restore_weight(g, char_get_equipment(ch, j));
    }
  Crash_save_list(g, char_get_inventory(ch), out, 0);
  Crash_restore_weight_list(g, char_get_inventory(ch));
  return out.commit();
}

void delete_inv_backup(struct char_data *ch)
{
  std::optional<std::string> path = player_path(NEW_OBJ_FILES, char_get_name(ch));
  if (!path)
    return;
  std::error_code ec;
  if (!std::filesystem::remove(*path + ".bak", ec) && ec)
    mud_log("ERROR: Couldn't delete backup inv.");
}

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */
static void auto_equip(GameWorld *g, struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  /* Lots of checks... */
  if (location > 0) { /* Was wearing it. */
    switch (j = (location - 1)) {
      case WEAR_UNUSED0:
        j = WEAR_WIELD2;
        break;
      case WEAR_FINGER_R:
      case WEAR_FINGER_L:
        if (!obj_wear_flagged(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
          location = LOC_INVENTORY;
        break;
      case WEAR_NECK_1:
      case WEAR_NECK_2:
        if (!obj_wear_flagged(obj, ITEM_WEAR_NECK))
          location = LOC_INVENTORY;
        break;
      case WEAR_BODY:
        if (!obj_wear_flagged(obj, ITEM_WEAR_BODY))
          location = LOC_INVENTORY;
        break;
      case WEAR_HEAD:
        if (!obj_wear_flagged(obj, ITEM_WEAR_HEAD))
          location = LOC_INVENTORY;
        break;
      case WEAR_LEGS:
        if (!obj_wear_flagged(obj, ITEM_WEAR_LEGS))
          location = LOC_INVENTORY;
        break;
      case WEAR_FEET:
        if (!obj_wear_flagged(obj, ITEM_WEAR_FEET))
          location = LOC_INVENTORY;
        break;
      case WEAR_HANDS:
        if (!obj_wear_flagged(obj, ITEM_WEAR_HANDS))
          location = LOC_INVENTORY;
        break;
      case WEAR_ARMS:
        if (!obj_wear_flagged(obj, ITEM_WEAR_ARMS))
          location = LOC_INVENTORY;
        break;
      case WEAR_UNUSED1:
        if (!obj_wear_flagged(obj, ITEM_WEAR_SHIELD))
          location = LOC_INVENTORY;
        j = WEAR_WIELD2;
        break;
      case WEAR_ABOUT:
        if (!obj_wear_flagged(obj, ITEM_WEAR_ABOUT))
          location = LOC_INVENTORY;
        break;
      case WEAR_WAIST:
        if (!obj_wear_flagged(obj, ITEM_WEAR_WAIST))
          location = LOC_INVENTORY;
        break;
      case WEAR_WRIST_R:
      case WEAR_WRIST_L:
        if (!obj_wear_flagged(obj, ITEM_WEAR_WRIST))
          location = LOC_INVENTORY;
        break;
      case WEAR_WIELD1:
        if (!obj_wear_flagged(obj, ITEM_WEAR_WIELD))
          location = LOC_INVENTORY;
        break;
      case WEAR_WIELD2:
        break;
      case WEAR_EYE:
        if (!obj_wear_flagged(obj, ITEM_WEAR_EYE))
          location = LOC_INVENTORY;
        break;
      case WEAR_BACKPACK:
        if (!obj_wear_flagged(obj, ITEM_WEAR_PACK))
          location = LOC_INVENTORY;
        break;
      case WEAR_SH:
        if (!obj_wear_flagged(obj, ITEM_WEAR_SH))
          location = LOC_INVENTORY;
        break;
      case WEAR_EAR_R:
      case WEAR_EAR_L:
        if (!obj_wear_flagged(obj, ITEM_WEAR_EAR))
          location = LOC_INVENTORY;
        break;
      default:
        location = LOC_INVENTORY;
    }

    if (location > 0) { /* Wearable. */
      if (!char_get_equipment(ch, j)) {
        /*
         * Check the characters's alignment to prevent them from being
         * zapped through the auto-equipping.
         */
        if (invalid_align(ch, obj) || invalid_class(ch, obj))
          location = LOC_INVENTORY;
        else
          equip_char(g, ch, obj, j);
      } else { /* Oops, saved a player with double equipment? */
        mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: autoeq: '{}' already equipped in position {:d}.", char_get_name(ch), location);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0) /* Inventory */
    obj_to_char(g, obj, ch);
}

bool Crash_delete_file(const char *name)
{
  std::string path;
  if (!open_rent_file(name, "deleting crash file", 1, &path))
    return false;
  std::error_code ec;
  if (!std::filesystem::remove(path, ec) && ec)
    mud_log("SYSERR: deleting crash file {} (2): {}", path, ec.message());
  return true;
}

bool Crash_delete_crashfile(struct char_data *ch)
{
  std::optional<io::text_reader> file = open_rent_file(char_get_name(ch), "checking for crash file", 3);
  if (!file)
    return false;
  record_lines in(*file);
  in.next();
  if (read_rent_header(in.line(), 0).rentcode == RENT_CRASH)
    Crash_delete_file(char_get_name(ch));
  return true;
}

static bool Crash_clean_file(GameWorld *g, char *name)
{
  std::optional<io::text_reader> file = open_rent_file(name, "OPENING OBJECT FILE", 4);
  if (!file)
    return false;
  record_lines in(*file);
  in.next();
  rent_header h = read_rent_header(in.line(), 0);

  if (h.rentcode == RENT_CRASH || h.rentcode == RENT_FORCED || h.rentcode == RENT_TIMEDOUT) {
    if (h.timed < game_now(g) - (CONFIG_CRASH_TIMEOUT * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      const char *filetype = h.rentcode == RENT_CRASH ? "crash" : h.rentcode == RENT_FORCED ? "forced rent" : "idlesave";
      mud_log("    Deleting {}'s {} file.", name, filetype);
      return true;
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (h.rentcode == RENT_RENTED && h.timed < game_now(g) - (CONFIG_RENT_TIMEOUT * SECS_PER_REAL_DAY)) {
    Crash_delete_file(name);
    mud_log("    Deleting {}'s rent file.", name);
    return true;
  }
  return false;
}

void update_obj_file(GameWorld *g)
{
  int i;

  for (i = 0; i <= g->top_of_p_table; i++)
    if (*g->player_table[i].name)
      Crash_clean_file(g, g->player_table[i].name);
}

void Crash_listrent(GameWorld *g, struct char_data *ch, char *name)
{
  std::string path;
  std::optional<io::text_reader> file = open_rent_file(name, nullptr, 0, &path);
  if (!file) {
    send_to_char(g, ch, "{} has no rent file.\r\n", name);
    return;
  }
  send_to_char(g, ch, "{}\r\n", path);

  record_lines in(*file);
  in.next();
  switch (read_rent_header(in.line(), -1).rentcode) {
    case RENT_RENTED:
      send_to_char(g, ch, "Rent\r\n");
      break;
    case RENT_CRASH:
      send_to_char(g, ch, "Crash\r\n");
      break;
    case RENT_CRYO:
      send_to_char(g, ch, "Cryo\r\n");
      break;
    case RENT_TIMEDOUT:
    case RENT_FORCED:
      send_to_char(g, ch, "TimedOut\r\n");
      break;
    default:
      send_to_char(g, ch, "Undef\r\n");
      break;
  }

  /* quirk: the last line is looked at twice, as while (!feof()) read it */
  std::string buf;
  int nr = 0, t[5] = {};
  while (!in.eof()) {
    in.next();
    if (!in.line().starts_with('#'))
      continue;
    io::fields(std::string_view(in.line()).substr(1)).number(nr);
    if (nr != NOTHING) {
      if (obj_proto_of(g, nr)) {
        struct obj_data *obj = read_object(g, nr);
        buf += str_format("[{:>5d}] ({:>5d}au) {:<20}\r\n", nr, obj_get_cost_per_day(obj), obj_get_short_description(obj));
        extract_obj(g, obj);
      } else
        buf += str_format("[-----] NONEXISTANT OBJECT #{:d}\r\n", nr);
      continue;
    }
    /* a unique item: past its values and XAP lines, its name, its short description kept, its long and action
       descriptions, then the line whose fifth number is shown as its cost (quirk: it is a wear flags word) */
    in.next();
    in.next();
    io::loaded<std::string> strings[4];
    bool cut_short = false;
    for (io::loaded<std::string> &text : strings)
      if (!(text = in.string("a listed rent file's object"))) {
        mud_log("SYSERR: {}", text.error().text());
        cut_short = true;
        break;
      }
    if (cut_short)
      break;
    in.next();
    io::fields f(in.line());
    for (int &n : t)
      if (!f.number(n))
        break;
    buf += str_format("[{:>5d}] ({:>5d}au) {:<20}\r\n", nr, t[4], strings[1]->empty() ? "(null)" : *strings[1]);
  }
  send_to_char(g, ch, buf);
}

/* Saves obj after what it contains, taking their weight off the containers until Crash_restore_weight() */
static void Crash_save(GameWorld *g, struct obj_data *obj, io::text_writer &out, int location)
{
  if (!obj)
    return;
  Crash_save_list(g, obj_get_contains(obj), out, MIN(0, location) - 1);
  write_object_record(g, out, obj, location);
  for (struct obj_data *tmp = obj_get_in_obj(obj); tmp; tmp = obj_get_in_obj(tmp))
    obj_set_weight(tmp, obj_get_weight(tmp) - (obj_get_weight(obj)));
}

/* The objects of list, oldest first, as the files have always had them. */
static void Crash_save_list(GameWorld *g, const id_list &list, io::text_writer &out, int location)
{
  list.each_oldest_first([&](long id) { Crash_save(g, obj_by_id(g, id), out, location); });
}

static void Crash_restore_weight(GameWorld *g, struct obj_data *obj)
{
  if (obj) {
    Crash_restore_weight_list(g, obj_get_contains(obj));
    if (obj_get_in_obj(obj))
      obj_set_weight(obj_get_in_obj(obj), obj_get_weight(obj_get_in_obj(obj)) + (obj_get_weight(obj)));
  }
}

static void Crash_restore_weight_list(GameWorld *g, const id_list &list)
{
  list.each([&](long id) { Crash_restore_weight(g, obj_by_id(g, id)); });
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
static void Crash_extract_norent_eq(GameWorld *g, struct char_data *ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++) {
    if (char_get_equipment(ch, j) == nullptr)
      continue;

    if (Crash_is_unrentable(char_get_equipment(ch, j)))
      obj_to_char(g, unequip_char(g, ch, j), ch);
    else
      Crash_extract_norents(g, char_get_equipment(ch, j));
  }
}

/* Extracts obj after what it contains */
static void Crash_extract_objs(GameWorld *g, struct obj_data *obj)
{
  if (obj) {
    Crash_extract_objs_from(g, obj_get_contains(obj).ids(), 0);
    extract_obj(g, obj);
  }
}

/* The objects of ids from i on, each after what it contains and after the older ones, as the old recursion along the
 * list did. */
static void Crash_extract_objs_from(GameWorld *g, const std::vector<long> &ids, std::size_t i)
{
  if (i >= ids.size())
    return;
  struct obj_data *obj = obj_by_id(g, ids[i]);
  if (obj)
    Crash_extract_objs_from(g, obj_get_contains(obj).ids(), 0);
  Crash_extract_objs_from(g, ids, i + 1);
  if (obj)
    extract_obj(g, obj);
}

static bool Crash_is_unrentable(struct obj_data *obj)
{
  if (!obj)
    return false;
  if (obj_extra_flagged(obj, ITEM_NORENT) || obj_get_cost_per_day(obj) < 0 ||
      (obj_get_item_number(obj) <= NOTHING &&
       !obj_extra_flagged(obj, ITEM_UNIQUE_SAVE))) {
    return true;
  }
  return false;
}

static void Crash_extract_norents(GameWorld *g, struct obj_data *obj)
{
  if (obj) {
    Crash_extract_norents_from(g, obj_get_contains(obj).ids(), 0);
    if (Crash_is_unrentable(obj))
      extract_obj(g, obj);
  }
}

/* As Crash_extract_objs_from(), for the unrentable ones */
static void Crash_extract_norents_from(GameWorld *g, const std::vector<long> &ids, std::size_t i)
{
  if (i >= ids.size())
    return;
  struct obj_data *obj = obj_by_id(g, ids[i]);
  if (obj)
    Crash_extract_norents_from(g, obj_get_contains(obj).ids(), 0);
  Crash_extract_norents_from(g, ids, i + 1);
  if (obj && Crash_is_unrentable(obj))
    extract_obj(g, obj);
}

void Crash_crashsave(GameWorld *g, struct char_data *ch)
{
  if (char_is_npc(ch))
    return;

  if (Crash_write_safe_file(g, ch, RENT_CRASH, 0, "crashsave"))
    char_plr_set(ch, PLR_CRASH, false);
}

void Crash_rentsave(GameWorld *g, struct char_data *ch, int cost)
{
  int j;

  if (char_is_npc(ch))
    return;

  Crash_extract_norent_eq(g, ch);
  Crash_extract_norents_from(g, char_get_inventory(ch).ids(), 0);

  if (!Crash_write_safe_file(g, ch, RENT_RENTED, cost, "rentsave"))
    return;

  for (j = 0; j < NUM_WEARS; j++)
    if (char_get_equipment(ch, j))
      Crash_extract_objs(g, char_get_equipment(ch, j));

  Crash_extract_objs_from(g, char_get_inventory(ch).ids(), 0);
}

/* ************************************************************************
 * Routines used for the receptionist					  *
 ************************************************************************* */

void Crash_save_all(GameWorld *g)
{
  for (descriptor_data *d : each_desc(g)) {
    if ((desc_get_connected(d) == CON_PLAYING) && !char_is_npc(d->character)) {
      if (char_plr_flagged(d->character, PLR_CRASH)) {
        Crash_crashsave(g, d->character);
        save_char(g, d->character);
        char_plr_set(d->character, PLR_CRASH, false);
      }
    }
  }
}

int Crash_load(GameWorld *g, struct char_data *ch)
{
  std::optional<std::string> path = player_path(NEW_OBJ_FILES, char_get_name(ch));
  if (!path)
    return 1;
  std::string backup = *path + ".bak";

  if (io::loaded<io::text_reader> file = io::text_reader::open(*path))
    return Crash_load_file(g, ch, *file);

  std::error_code ec;
  if (!std::filesystem::exists(*path, ec) && !std::filesystem::exists(backup, ec))
    return 1; /* never had any, or lost them all and saved that */

  mud_log("SYSERR: READING OBJECT FILE {} (5): {}", *path, io::why_unreadable(*path));
  send_to_char(g, ch,
               "\r\n********************* NOTICE *********************\r\nThere was a problem loading your objects from disk.\r\nTrying your backup object file.\r\n");

  if (io::loaded<io::text_reader> file = io::text_reader::open(backup)) {
    mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} loading backup object file {}.", char_get_name(ch), backup);
    return Crash_load_file(g, ch, *file);
  }

  mud_log("SYSERR: {}:{:d}: fopen backup object file failed for {}: {}", __FILE__, __LINE__, backup, io::why_unreadable(backup));
  mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} entering game with no equipment. Loading backup failed.", char_get_name(ch));
  send_to_char(g, ch,
               "\r\n********************* NOTICE *********************\r\nThere was a problem loading your objects from backup.\r\nContact staff for assistance.\r\n");

  return -1;
}

static bool Crash_load_file(GameWorld *g, struct char_data *ch, io::text_reader &file)
{
  record_lines in(file);
  in.next();
  int orig_rent_code = read_rent_header(in.line(), 0).rentcode;

  if (orig_rent_code == RENT_RENTED || orig_rent_code == RENT_TIMEDOUT)
    save_char(g, ch);
  switch (orig_rent_code) {
    case RENT_RENTED:
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} un-renting and entering game.", char_get_name(ch));
      break;
    case RENT_CRASH:
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} retrieving crash-saved items and entering game.", char_get_name(ch));
      break;
    case RENT_CRYO:
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} un-cryo'ing and entering game.", char_get_name(ch));
      break;
    case RENT_FORCED:
    case RENT_TIMEDOUT:
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} retrieving force-saved items and entering game.", char_get_name(ch));
      break;
    default:
      mudlog(g, LOG_BRF, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "WARNING: {} entering game with undefined rent code.", char_get_name(ch));
      break;
  }

  if (!in.eof())
    in.next();

  /* quirk: objects still waiting for a container that never came at the end of the file are lost */
  bag_rows rows(g, ch, NOWHERE);
  int num_objs = 0;
  io::loaded<void> read = read_object_records(g, in, record_kind::rent, [&](obj_data *temp, int locate) {
    num_objs++;
    check_unique_id(g, temp);
    add_unique_id(g, temp);
    if (obj_get_type_flag(temp) == ITEM_DRINKCON) {
      name_from_drinkcon(g, temp);
      if (obj_get_value(temp, 1) != 0)
        name_to_drinkcon(g, temp, obj_get_value(temp, 2));
    }
    if (obj_get_vnum(g, temp) == 20099 || obj_get_vnum(g, temp) == 20098)
      if (obj_extra_flagged(temp, ITEM_UNBREAKABLE))
        obj_extra_set(temp, ITEM_UNBREAKABLE, false);
    auto_equip(g, ch, temp, locate);
    rows.place(temp, locate);
  });
  if (!read) {
    mud_log("SYSERR: {}", read.error().text());
    return false;
  }

  /* Little hoarding check. -gg 3/1/98 */
  mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "{} (level {:d}) has {:d} objects (max {:d}).",
         char_get_name(ch), char_get_level(ch), num_objs, CONFIG_MAX_OBJ_SAVE);

  return orig_rent_code != RENT_RENTED && orig_rent_code != RENT_CRYO;
}
