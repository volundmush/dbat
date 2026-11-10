/* ************************************************************************
 *   File: objsave.c                                     Part of CircleMUD *
 *  Usage: loading/saving player objects for rent and crash-save           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <cerrno>

#include <unistd.h>

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
#include "dbat/atomicfile.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions */

static void auto_equip(GameWorld *g, struct char_data *ch, struct obj_data *obj, int location);
static bool Crash_save(GameWorld *g, struct obj_data *obj, FILE *fp, int location);
static void Crash_restore_weight(GameWorld *g, struct obj_data *obj);
static void Crash_restore_weight_list(GameWorld *g, const id_list &list);
static bool Crash_save_list(GameWorld *g, const id_list &list, FILE *fp, int location);
static void Crash_extract_objs_from(GameWorld *g, const std::vector<long> &ids, std::size_t i);
static void Crash_extract_norents_from(GameWorld *g, const std::vector<long> &ids, std::size_t i);
static void Crash_extract_objs(GameWorld *g, struct obj_data *obj);
static bool Crash_is_unrentable(struct obj_data *obj);
static void Crash_extract_norents(GameWorld *g, struct obj_data *obj);
static bool Crash_load_file(GameWorld *g, struct char_data *ch, FILE *fl, const char *filename);

static void Crash_log_file_error(const char *operation, const char *filename, const char *file, int line)
{
  mud_log_printf("SYSERR: %s:%d: %s failed for %s: %s", file, line, operation, filename, strerror(errno));
}

static bool Crash_build_save_filename(char *dst, size_t dst_size, const char *filename, const char *suffix,
                                      const char *file, int line)
{
  int written = snprintf(dst, dst_size, "%s%s", filename, suffix);

  if (written < 0 || (size_t)written >= dst_size) {
    mud_log_printf("SYSERR: %s:%d: crashsave path too long for %s%s", file, line, filename, suffix);
    return false;
  }

  return true;
}

static bool Crash_write_safe_file(GameWorld *g, struct char_data *ch, int rentcode, int cost, const char *save_type)
{
  struct atomic_file af;
  char filename[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, char_get_name(ch))) {
    mud_log_printf("SYSERR: %s:%d: get_filename failed for %s of %s", __FILE__, __LINE__, save_type, char_get_name(ch));
    return false;
  }

  /* the replaced file stays as <name>.bak, which Crash_load() falls back to */
  if (!(fp = atomic_open(&af, filename, true)))
    return false;

  if (fprintf(fp, "%d %lld %d %d %d %d\r\n", rentcode, (long long)game_now(g), cost,
              char_get_gold(ch), char_get_bank_gold(ch), 0) < 0) {
    Crash_log_file_error("fprintf", af.tmp, __FILE__, __LINE__);
    atomic_abort(&af);
    return false;
  }

  for (j = 0; j < NUM_WEARS; j++)
    if (char_get_equipment(ch, j)) {
      if (!Crash_save(g, char_get_equipment(ch, j), fp, j + 1)) {
        Crash_restore_weight(g, char_get_equipment(ch, j));
        mud_log_printf("SYSERR: %s:%d: Crash_save failed for %s of %s equipment position %d", __FILE__, __LINE__, save_type, char_get_name(ch), j);
        atomic_abort(&af);
        return false;
      }
      if (ferror(fp)) {
        Crash_restore_weight(g, char_get_equipment(ch, j));
        Crash_log_file_error("write", af.tmp, __FILE__, __LINE__);
        atomic_abort(&af);
        return false;
      }
      Crash_restore_weight(g, char_get_equipment(ch, j));
    }

  if (!Crash_save_list(g, char_get_inventory(ch), fp, 0)) {
    Crash_restore_weight_list(g, char_get_inventory(ch));
    mud_log_printf("SYSERR: %s:%d: Crash_save failed for %s of %s inventory", __FILE__, __LINE__, save_type, char_get_name(ch));
    atomic_abort(&af);
    return false;
  }
  Crash_restore_weight_list(g, char_get_inventory(ch));

  return atomic_commit(&af);
}

void delete_inv_backup(struct char_data *ch)
{
  FILE *source;
  char source_file[20480], filename[20480];

  if (!get_filename(source_file, sizeof(source_file), NEW_OBJ_FILES, char_get_name(ch))) {
    return;
  }
  snprintf(filename, sizeof(filename), "%s.bak", source_file);

  if (!(source = fopen(filename, "r"))) {
    return;
  }
  fclose(source);

  if (remove(filename) < 0 && errno != ENOENT)
    mud_log_printf("ERROR: Couldn't delete backup inv.");
  /*  SYSERR_DESC:
   *  When an alias file cannot be removed, this error will occur,
   *  and the reason why will be the tail end of the error.
   */

  return;
}

bool Obj_to_store(GameWorld *g, struct obj_data *obj, FILE *fl, int location)
{
  my_obj_save_to_disk(g, fl, obj, location);
  if (ferror(fl))
    return false;
  return true;
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
        mudlog_printf(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: autoeq: '%s' already equipped in position %d.", char_get_name(ch), location);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0) /* Inventory */
    obj_to_char(g, obj, ch);
}

bool Crash_delete_file(const char *name)
{
  char filename[50];
  FILE *fl;

  // if (!xap_objs) {
  // if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
  // return false;
  //} else {
  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    return false;
  //}

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) /* if it fails but NOT because of no file */
      mud_log_printf("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return false;
  }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
    mud_log_printf("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return true;
}

bool Crash_delete_crashfile(struct char_data *ch)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;
  int rentcode = 0, netcost = 0, gold = 0, account = 0, nitems = 0;
  long long timed = 0;
  char line[MAX_INPUT_LENGTH];

  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, char_get_name(ch)))
    return false;

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) /* if it fails, NOT because of no file */
      mud_log_printf("SYSERR: checking for crash file %s (3): %s", filename, strerror(errno));
    return false;
  }

  if (!feof(fl))
    get_line(fl, line);
  sscanf(line, "%d %lld %d %d %d %d", &rentcode, &timed, &netcost, &gold,
         &account, &nitems);
  fclose(fl);

  if (rentcode == RENT_CRASH)
    Crash_delete_file(char_get_name(ch));

  return true;
}

static bool Crash_clean_file(GameWorld *g, char *name)
{
  char filename[MAX_STRING_LENGTH];
  FILE *fl;
  int rentcode = 0, netcost = 0, gold = 0, account = 0, nitems = 0;
  long long timed = 0;
  char line[MAX_STRING_LENGTH];

  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    return false;

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) /* if it fails, NOT because of no file */
      mud_log_printf("SYSERR: OPENING OBJECT FILE %s (4): %s", filename, strerror(errno));
    return false;
  }

  if (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%d %lld %d %d %d %d", &rentcode, &timed, &netcost,
           &gold, &account, &nitems);
    fclose(fl);

    if ((rentcode == RENT_CRASH) ||
        (rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT)) {
      if (timed < game_now(g) - (CONFIG_CRASH_TIMEOUT * SECS_PER_REAL_DAY)) {
        const char *filetype;

        Crash_delete_file(name);
        switch (rentcode) {
          case RENT_CRASH:
            filetype = "crash";
            break;
          case RENT_FORCED:
            filetype = "forced rent";
            break;
          case RENT_TIMEDOUT:
            filetype = "idlesave";
            break;
          default:
            filetype = "UNKNOWN!";
            break;
        }
        mud_log_printf("    Deleting %s's %s file.", name, filetype);
        return true;
      }
      /* Must retrieve rented items w/in 30 days */
    } else if (rentcode == RENT_RENTED)
      if (timed < game_now(g) - (CONFIG_RENT_TIMEOUT * SECS_PER_REAL_DAY)) {
        Crash_delete_file(name);
        mud_log_printf("    Deleting %s's rent file.", name);
        return true;
      }
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
  FILE *fl = nullptr;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  int rentcode = -1, netcost = 0, gold = 0, account = 0, nitems = 0;
  size_t len;
  long long timed = 0;
  int t[10];
  obj_vnum nr;
  char line[MAX_STRING_LENGTH];
  char *sdesc;

  if (get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    fl = fopen(filename, "rb");
  if (!fl) {
    send_to_char_printf(g, ch, "%s has no rent file.\r\n", name);
    return;
  }

  send_to_char_printf(g, ch, "%s\r\n", filename);

  if (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%d %lld %d %d %d %d", &rentcode, &timed, &netcost,
           &gold, &account, &nitems);
  }

  switch (rentcode) {
    case RENT_RENTED:
      send_to_char_printf(g, ch, "Rent\r\n");
      break;
    case RENT_CRASH:
      send_to_char_printf(g, ch, "Crash\r\n");
      break;
    case RENT_CRYO:
      send_to_char_printf(g, ch, "Cryo\r\n");
      break;
    case RENT_TIMEDOUT:
    case RENT_FORCED:
      send_to_char_printf(g, ch, "TimedOut\r\n");
      break;
    default:
      send_to_char_printf(g, ch, "Undef\r\n");
      break;
  }
  buf[0] = 0;
  len    = 0;

  while (!feof(fl)) {
    get_line(fl, line);
    if (*line == '#') { /* swell - its an item */
      sscanf(line, "#%d", &nr);
      if (nr != NOTHING) { /* then we can dispense with it easily */
        if (obj_proto_of(g, nr)) {
          obj = read_object(g, nr);
          if (len + 255 < sizeof(buf)) {
            len += snprintf(buf + len, sizeof(buf) - len, "[%5d] (%5dau) %-20s\r\n",
                            nr, obj_get_cost_per_day(obj), obj_get_short_description(obj));
          } else {
            snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
            break;
          }
          extract_obj(g, obj);
        } else {
          if (len + 255 < sizeof(buf)) {
            len += snprintf(buf + len, sizeof(buf) - len,
                            "[-----] NONEXISTANT OBJECT #%d\r\n", nr);
          } else {
            snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
            break;
          }
        }
      } else {                                       /* its nothing, and a unique item. bleh. partial parse.*/
        get_line(fl, line);                          /* this is obj+val */
        get_line(fl, line);                          /* this is XAP */
        fread_string(fl, ", listrent reading name"); /* screw the name */
        sdesc = fread_string(fl, ", listrent reading sdesc");
        fread_string(fl, ", listrent reading desc");  /* screw the long desc */
        fread_string(fl, ", listrent reading adesc"); /* screw the action desc. */
        get_line(fl, line);                           /* this is an important line.rent..*/
        sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4);
        /* great we got it all, make the buf */
        if (len + 255 < sizeof(buf)) {
          len += snprintf(buf + len, sizeof(buf) - len,
                          "[%5d] (%5dau) %-20s\r\n", nr, t[4], sdesc);
        } else {
          snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
          break;
        }
        /* best of all, we don't care if there's descs, or stuff..*/
        /* since we're only doing operations on lines beginning in # */
        /* i suppose you don't want to make exdescs start with # .:) */
      }
    }
  }

  send_to_char_printf(g, ch, "%s", buf);
  fclose(fl);
}

/* Saves obj after what it contains, taking their weight off the containers until Crash_restore_weight() */
static bool Crash_save(GameWorld *g, struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save_list(g, obj_get_contains(obj), fp, MIN(0, location) - 1);
    result = Obj_to_store(g, obj, fp, location);

    for (tmp = obj_get_in_obj(obj); tmp; tmp = obj_get_in_obj(tmp))
      obj_set_weight(tmp, obj_get_weight(tmp) - (obj_get_weight(obj)));

    if (!result)
      return (false);
  }
  return (true);
}

/* The objects of list, oldest first, as the files have always had them. The result is the newest one's. */
static bool Crash_save_list(GameWorld *g, const id_list &list, FILE *fp, int location)
{
  bool result = true;

  list.each_oldest_first([&](long id) { result = Crash_save(g, obj_by_id(g, id), fp, location); });
  return result;
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
  FILE *fl;
  char cmfname[MAX_STRING_LENGTH], cmfnamebak[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];

  if (!get_filename(cmfname, sizeof(cmfname), NEW_OBJ_FILES, char_get_name(ch)))
    return 1;

  if (!Crash_build_save_filename(cmfnamebak, sizeof(cmfnamebak), cmfname, ".bak", __FILE__, __LINE__))
    return -1;

  if ((fl = fopen(cmfname, "rb")))
    return Crash_load_file(g, ch, fl, cmfname);

  if (errno == ENOENT && access(cmfnamebak, F_OK)) /* never had any, or lost them all and saved that */
    return 1;

  snprintf(buf1, sizeof(buf1), "SYSERR: READING OBJECT FILE %s (5)", cmfname);
  perror(buf1);
  send_to_char_printf(g, ch,
                      "\r\n********************* NOTICE *********************\r\n"
                      "There was a problem loading your objects from disk.\r\n"
                      "Trying your backup object file.\r\n");

  if ((fl = fopen(cmfnamebak, "rb"))) {
    mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s loading backup object file %s.", char_get_name(ch), cmfnamebak);
    return Crash_load_file(g, ch, fl, cmfnamebak);
  }

  Crash_log_file_error("fopen backup object file", cmfnamebak, __FILE__, __LINE__);
  mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s entering game with no equipment. Loading backup failed.", char_get_name(ch));
  send_to_char_printf(g, ch,
                      "\r\n********************* NOTICE *********************\r\n"
                      "There was a problem loading your objects from backup.\r\n"
                      "Contact staff for assistance.\r\n");

  return -1;
}

static bool Crash_load_file(GameWorld *g, struct char_data *ch, FILE *fl, const char *filename)
{
  char buf2[MAX_STRING_LENGTH];
  char line[256] = "";
  int t[30] = {}, zwei = 0; /* a damaged line leaves some of these unread */
  int orig_rent_code;
  struct obj_data *temp;
  int locate = 0, j, num_objs = 0;
  obj_vnum nr;
  std::vector<struct obj_data *> cont_row[MAX_BAG_ROWS]; /* each list in file order */
  int rentcode = 0, netcost = 0, gold = 0, account = 0, nitems = 0;
  long long timed = 0;
  char f1[READ_SIZE] = "", f2[READ_SIZE] = "", f3[READ_SIZE] = "", f4[READ_SIZE] = "";

  if (!feof(fl))
    get_line(fl, line);

  sscanf(line, "%d %lld %d %d %d %d", &rentcode, &timed,
         &netcost, &gold, &account, &nitems);

  if (rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT) {
    save_char(g, ch);
  }
  switch (orig_rent_code = rentcode) {
    case RENT_RENTED:
      mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s un-renting and entering game.", char_get_name(ch));
      break;
    case RENT_CRASH:
      mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s retrieving crash-saved items and entering game.", char_get_name(ch));
      break;
    case RENT_CRYO:
      mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s un-cryo'ing and entering game.", char_get_name(ch));
      break;
    case RENT_FORCED:
    case RENT_TIMEDOUT:
      mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s retrieving force-saved items and entering game.", char_get_name(ch));
      break;
    default:
      mudlog_printf(g, LOG_BRF, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "WARNING: %s entering game with undefined rent code.", char_get_name(ch));
      break;
  }

  for (j = 0; j < MAX_BAG_ROWS; j++)
    cont_row[j].clear(); /* empty all cont lists (you never know ...) */

  if (!feof(fl))
    get_line(fl, line);

  while (!feof(fl)) {
    temp = nullptr;
    /* first, we get the number. Not too hard. */
    if (*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        get_line(fl, line); /* skip it, or the loop never moves on */
        continue;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) { /* then it is unique */
        temp              = create_obj(g);
        obj_set_item_number(temp, NOTHING);
        obj_set_size(temp, SIZE_MEDIUM);
      } else if (nr < 0 || nr >= 999999) {
        get_line(fl, line); /* skip it, or the loop never moves on */
        continue;
      } else {
        temp = read_object(g, nr);
        if (!temp) {
          get_line(fl, line);
          continue;
        }
      }

      get_line(fl, line);

      sscanf(line, "%d %d %d %d %d %d %d %d %d %s %s %s %s %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, f1, f2, f3, f4, t + 13, t + 14, t + 15, t + 16, t + 17, t + 18, t + 19, t + 20);

      locate = t[0];
      obj_set_value(temp, 0, t[1]);
      obj_set_value(temp, 1, t[2]);
      obj_set_value(temp, 2, t[3]);
      obj_set_value(temp, 3, t[4]);
      obj_set_value(temp, 4, t[5]);
      obj_set_value(temp, 5, t[6]);
      obj_set_value(temp, 6, t[7]);
      obj_set_value(temp, 7, t[8]);
      obj_get_extra_flags(temp)
          [0] = asciiflag_conv(f1);
      obj_get_extra_flags(temp)
          [1] = asciiflag_conv(f2);
      obj_get_extra_flags(temp)
          [2] = asciiflag_conv(f3);
      obj_get_extra_flags(temp)
          [3] = asciiflag_conv(f4);
      obj_set_value(temp, 8, t[13]);
      obj_set_value(temp, 9, t[14]);
      obj_set_value(temp, 10, t[15]);
      obj_set_value(temp, 11, t[16]);
      obj_set_value(temp, 12, t[17]);
      obj_set_value(temp, 13, t[18]);
      obj_set_value(temp, 14, t[19]);
      obj_set_value(temp, 15, t[20]);

      get_line(fl, line);
      /* read line check for xap. */
      if (!strcmp("XAP", line)) { /* then this is a Xap Obj, requires
              special care */
        obj_set_name(temp, take_string(fread_string(fl, "rented object name")));
        if (obj_get_name(temp) == nullptr) {
          obj_set_name(temp, "undefined");
        }

        obj_set_short_description(temp, take_string(fread_string(fl, "rented object short desc")));
        if (obj_get_short_description(temp) == nullptr) {
          obj_set_short_description(temp, "undefined");
        }

        obj_set_description(temp, take_string(fread_string(fl, "rented object desc")));
        if (obj_get_description(temp) == nullptr) {
          obj_set_description(temp, "undefined");
        }

        obj_set_action_description(temp, take_string(fread_string(fl, "rented object adesc")));
        if (obj_get_action_description(temp) == nullptr) {
          obj_set_action_description(temp, nullptr);
        }

        if (!get_line(fl, line) ||
            (sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7) != 8)) {
          fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
          return false;
        }
        obj_set_type_flag(temp, t[0]);
        obj_get_wear_flags(temp)[0] = t[1];
        obj_get_wear_flags(temp)[1] = t[2];
        obj_get_wear_flags(temp)[2] = t[3];
        obj_get_wear_flags(temp)[3] = t[4];
        obj_set_weight(temp, t[5]);
        obj_set_cost(temp, t[6]);

        /* we're clearing these for good luck */

        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          obj_get_affected(temp)[j].location = APPLY_NONE;
          obj_get_affected(temp)[j].modifier = 0;
          obj_get_affected(temp)[j].specific = 0;
        }

        /* maybe clear spellbook for good luck too? */

        obj_ex_description_slot(temp)->clear();

        get_line(fl, line);
        for (j = zwei = 0; !zwei && !feof(fl);) {
          switch (*line) {
            case 'E': {
              extra_descr_data descr;
              sprintf(buf2, "rented object edesc keyword for object #%d", nr);
              descr.keyword = take_string(fread_string(fl, buf2));
              sprintf(buf2, "rented object edesc text for object #%d keyword %s", nr, descr.keyword.c_str());
              descr.description = take_string(fread_string(fl, buf2));
              obj_ex_description_slot(temp)->push_front(std::move(descr));
              get_line(fl, line);
              break;
            }
            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                mud_log_printf("SYSERR: Too many object affectations in loading rent file");
              }
              get_line(fl, line);
              sscanf(line, "%d %d %d", t, t + 1, t + 2);
              if (j < MAX_OBJ_AFFECT) {
                obj_get_affected(temp)[j].location = t[0];
                obj_get_affected(temp)[j].modifier = t[1];
                obj_get_affected(temp)[j].specific = affect_checked_specific(t[0], t[2]);
                j++;
              }
              get_line(fl, line);
              break;
            case 'G': {
              long long generation;

              get_line(fl, line);
              if (sscanf(line, "%lld", &generation) == 1)
                obj_set_generation(temp, generation);
              get_line(fl, line);
              break;
            }
            case 'U':
              get_line(fl, line);
              {
                int64_t uid = obj_get_unique_id(temp);
                sscanf(line, "%" SCNd64, &uid);
                /* -1 is an object not yet given one; no real one is negative. */
                if (uid < -1) {
                  mud_log_printf("SYSERR: Object #%d has unique id %" PRId64 ", which can't be; giving it a new one.",
                                 obj_get_vnum(g, temp), uid);
                  uid = -1;
                }
                obj_set_unique_id(temp, uid);
              }
              get_line(fl, line);
              break;
            case 'S':
              if (j >= SPELLBOOK_SIZE) {
                mud_log_printf("SYSERR: Too many spells in spellbook loading rent file");
              }
              get_line(fl, line);
              sscanf(line, "%d %d", t, t + 1);

              j++;
              get_line(fl, line);
              break;
            case 'Z':
              get_line(fl, line);
              {
                int size = obj_get_size(temp);
                sscanf(line, "%d", &size);
                obj_set_size(temp, size);
              }
              get_line(fl, line);
              break;
            case '$':
            case '#':
              zwei = 1;
              break;
            default:
              zwei = 1;
              break;
          }
        } /* exit our for loop */
      } /* exit our xap loop */
      if (temp != nullptr) {
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
      } else {
        continue;
      }
      /*
        what to do with a new loaded item:

        if there's a list with <locate> less than 1 below this:
          (equipped items are assumed to have <locate>==0 here) then its
          container has disappeared from the file   *gasp*
           -> put all the list back to ch's inventory
        if there's a list of contents with <locate> 1 below this:
          check if it's a container
          - if so: get it from ch, fill it, and give it back to ch (this way the
              container has its correct weight before modifying ch)
          - if not: the container is missing -> put all the list to ch's inventory

        for items with negative <locate>:
          if there's already a list of contents with the same <locate> put obj to it
          if not, start a new list

      Confused? Well maybe you can think of some better text to be put here ...

      since <locate> for contents is < 0 the list indices are switched to
      non-negative
      */

      if (locate > 0) { /* item equipped */
        for (j = MAX_BAG_ROWS - 1; j > 0; j--)
          if (!cont_row[j].empty()) { /* no container -> back to ch's inventory */
            for (struct obj_data *row_obj : cont_row[j])
              obj_to_char(g, row_obj, ch);
            cont_row[j].clear();
            cont_row[j].clear();
          }
        if (!cont_row[0].empty()) { /* content list existing */
          if (obj_get_type_flag(temp) == ITEM_CONTAINER) {
            /* auto_equip puts it in the inventory if it can't be worn after all */
            bool worn = locate <= NUM_WEARS && char_get_equipment(ch, locate - 1) == temp;

            /* rem item ; fill ; equip again */
            if (worn)
              temp = unequip_char(g, ch, locate - 1);
            obj_contains_slot(temp)->clear(); /* should be empty - but who knows */
            for (struct obj_data *row_obj : cont_row[0])
              obj_to_obj(g, row_obj, temp);
            cont_row[0].clear();
            if (worn)
              equip_char(g, ch, temp, locate - 1);
          } else { /* object isn't container -> empty content list */
            for (struct obj_data *row_obj : cont_row[0])
              obj_to_char(g, row_obj, ch);
            cont_row[0].clear();
            cont_row[0].clear();
          }
        }
      } else { /* locate <= 0 */
        for (j = MAX_BAG_ROWS - 1; j > -locate; j--)
          if (!cont_row[j].empty()) { /* no container -> back to ch's inventory */
            for (struct obj_data *row_obj : cont_row[j])
              obj_to_char(g, row_obj, ch);
            cont_row[j].clear();
            cont_row[j].clear();
          }

        if (j == -locate && !cont_row[j].empty()) { /* content list existing */
          if (obj_get_type_flag(temp) == ITEM_CONTAINER) {
            /* take item ; fill ; give to char again */
            obj_from_char(g, temp);
            obj_contains_slot(temp)->clear();
            for (struct obj_data *row_obj : cont_row[j])
              obj_to_obj(g, row_obj, temp);
            cont_row[j].clear();
            obj_to_char(g, temp, ch); /* add to inv first ... */
          } else {                 /* object isn't container -> empty content list */
            for (struct obj_data *row_obj : cont_row[j])
              obj_to_char(g, row_obj, ch);
            cont_row[j].clear();
            cont_row[j].clear();
          }
        }

        if (locate < 0 && locate >= -MAX_BAG_ROWS) {
          /* let obj be part of content list
             but put it at the list's end thus having the items
             in the same order as before renting */
          obj_from_char(g, temp);
          cont_row[-locate - 1].push_back(temp);
        }
      } /* locate less than zero */
    } else {
      get_line(fl, line);
    }
  }

  /* Little hoarding check. -gg 3/1/98 */
  mudlog_printf(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, ch)), true, "%s (level %d) has %d objects (max %d).",
                char_get_name(ch), char_get_level(ch), num_objs, CONFIG_MAX_OBJ_SAVE);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return false;
  else
    return true;
}
