/* ************************************************************************
 *   File: objsave.c                                     Part of CircleMUD *
 *  Usage: loading/saving player objects for rent and crash-save           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "dbat/objsave.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/spells.h"
#include "dbat/players.h"
#include "dbat/class.h"
#include "dbat/act.social.h"
#include "dbat/act.item.h"
#include "dbat/fileop.h"
#include "dbat/consts/rents.h"
#include "dbat/config.h"

#include <unistd.h>
#include <errno.h>

/* local functions */

static void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
static int Crash_save(struct obj_data *obj, FILE *fp, int location);
static void Crash_restore_weight(struct obj_data *obj);
static void Crash_extract_objs(struct obj_data *obj);
static int Crash_is_unrentable(struct obj_data *obj);
static void Crash_extract_norents(struct obj_data *obj);
static int Crash_load_file(struct char_data *ch, FILE *fl, const char *filename);

static void Crash_log_file_error(const char *operation, const char *filename, const char *file, int line)
{
  mud_log("SYSERR: %s:%d: %s failed for %s: %s", file, line, operation, filename, strerror(errno));
}

static int Crash_build_save_filename(char *dst, size_t dst_size, const char *filename, const char *suffix,
                                     const char *file, int line)
{
  int written = snprintf(dst, dst_size, "%s%s", filename, suffix);

  if (written < 0 || (size_t)written >= dst_size) {
    mud_log("SYSERR: %s:%d: crashsave path too long for %s%s", file, line, filename, suffix);
    return FALSE;
  }

  return TRUE;
}

static int Crash_close_save_file(FILE *fp, const char *filename, const char *file, int line)
{
  if (fflush(fp) == EOF) {
    Crash_log_file_error("fflush", filename, file, line);
    fclose(fp);
    return FALSE;
  }

  if (fsync(fileno(fp)) == -1) {
    Crash_log_file_error("fsync", filename, file, line);
    fclose(fp);
    return FALSE;
  }

  if (fclose(fp) == EOF) {
    Crash_log_file_error("fclose", filename, file, line);
    return FALSE;
  }

  return TRUE;
}

static int Crash_write_safe_file(struct char_data *ch, int rentcode, int cost, const char *save_type)
{
  char filename[MAX_INPUT_LENGTH], tmpfile[MAX_STRING_LENGTH], bakfile[MAX_STRING_LENGTH];
  int j;
  FILE *fp;

  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, char_get_name(ch))) {
    mud_log("SYSERR: %s:%d: get_filename failed for %s of %s", __FILE__, __LINE__, save_type, char_get_name(ch));
    return FALSE;
  }

  if (!Crash_build_save_filename(tmpfile, sizeof(tmpfile), filename, ".tmp", __FILE__, __LINE__) ||
      !Crash_build_save_filename(bakfile, sizeof(bakfile), filename, ".bak", __FILE__, __LINE__))
    return FALSE;

  if (!(fp = fopen(tmpfile, "wb"))) {
    Crash_log_file_error("fopen", tmpfile, __FILE__, __LINE__);
    return FALSE;
  }

  if (fprintf(fp, "%d %d %d %d %d %d\r\n", rentcode, (int)time(0), cost,
              char_get_gold(ch), char_get_bank_gold(ch), 0) < 0) {
    Crash_log_file_error("fprintf", tmpfile, __FILE__, __LINE__);
    fclose(fp);
    remove(tmpfile);
    return FALSE;
  }

  for (j = 0; j < NUM_WEARS; j++)
    if (char_get_equipment(ch, j)) {
      if (!Crash_save(char_get_equipment(ch, j), fp, j + 1)) {
        Crash_restore_weight(char_get_equipment(ch, j));
        mud_log("SYSERR: %s:%d: Crash_save failed for %s of %s equipment position %d", __FILE__, __LINE__, save_type, char_get_name(ch), j);
        fclose(fp);
        remove(tmpfile);
        return FALSE;
      }
      if (ferror(fp)) {
        Crash_restore_weight(char_get_equipment(ch, j));
        Crash_log_file_error("write", tmpfile, __FILE__, __LINE__);
        fclose(fp);
        remove(tmpfile);
        return FALSE;
      }
      Crash_restore_weight(char_get_equipment(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    Crash_restore_weight(ch->carrying);
    mud_log("SYSERR: %s:%d: Crash_save failed for %s of %s inventory", __FILE__, __LINE__, save_type, char_get_name(ch));
    fclose(fp);
    remove(tmpfile);
    return FALSE;
  }

  if (ferror(fp)) {
    Crash_restore_weight(ch->carrying);
    Crash_log_file_error("write", tmpfile, __FILE__, __LINE__);
    fclose(fp);
    remove(tmpfile);
    return FALSE;
  }

  Crash_restore_weight(ch->carrying);

  if (!Crash_close_save_file(fp, tmpfile, __FILE__, __LINE__)) {
    remove(tmpfile);
    return FALSE;
  }

  if (rename(filename, bakfile) == -1 && errno != ENOENT) {
    Crash_log_file_error("rename existing save to backup", filename, __FILE__, __LINE__);
    remove(tmpfile);
    return FALSE;
  }

  if (rename(tmpfile, filename) == -1) {
    Crash_log_file_error("rename temporary save", tmpfile, __FILE__, __LINE__);
    if (rename(bakfile, filename) == -1)
      Crash_log_file_error("restore backup save", bakfile, __FILE__, __LINE__);
    return FALSE;
  }

  return TRUE;
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
    mud_log("ERROR: Couldn't delete backup inv.");
  /*  SYSERR_DESC:
   *  When an alias file cannot be removed, this error will occur,
   *  and the reason why will be the tail end of the error.
   */

  return;
}

int Obj_to_store(struct obj_data *obj, FILE *fl, int location)
{
  my_obj_save_to_disk(fl, obj, location);
  if (ferror(fl))
    return (0);
  return (1);
}

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */
static void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
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
          equip_char(ch, obj, j);
      } else { /* Oops, saved a player with double equipment? */
        mudlog(LOG_BRF, ADMLVL_IMMORT, TRUE, "SYSERR: autoeq: '%s' already equipped in position %d.", char_get_name(ch), location);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0) /* Inventory */
    obj_to_char(obj, ch);
}

int Crash_delete_file(char *name)
{
  char filename[50];
  FILE *fl;

  // if (!xap_objs) {
  // if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
  // return (0);
  //} else {
  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    return (0);
  //}

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) /* if it fails but NOT because of no file */
      mud_log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return (0);
  }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
    mud_log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return (1);
}

int Crash_delete_crashfile(struct char_data *ch)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;
  int rentcode, timed, netcost, gold, account, nitems;
  char line[MAX_INPUT_LENGTH];

  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, char_get_name(ch)))
    return (0);

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) /* if it fails, NOT because of no file */
      mud_log("SYSERR: checking for crash file %s (3): %s", filename, strerror(errno));
    return (0);
  }

  if (!feof(fl))
    get_line(fl, line);
  sscanf(line, "%d %d %d %d %d %d", &rentcode, &timed, &netcost, &gold,
         &account, &nitems);
  fclose(fl);

  if (rentcode == RENT_CRASH)
    Crash_delete_file(char_get_name(ch));

  return (1);
}

int Crash_clean_file(char *name)
{
  char filename[MAX_STRING_LENGTH];
  FILE *fl;
  int rentcode, timed, netcost, gold, account, nitems;
  char line[MAX_STRING_LENGTH];

  if (!get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    return (0);

  if (!(fl = fopen(filename, "r+b"))) {
    if (errno != ENOENT) /* if it fails, NOT because of no file */
      mud_log("SYSERR: OPENING OBJECT FILE %s (4): %s", filename, strerror(errno));
    return (0);
  }

  if (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rentcode, &timed, &netcost,
           &gold, &account, &nitems);
    fclose(fl);

    if ((rentcode == RENT_CRASH) ||
        (rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT)) {
      if (timed < time(0) - (CONFIG_CRASH_TIMEOUT * SECS_PER_REAL_DAY)) {
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
        mud_log("    Deleting %s's %s file.", name, filetype);
        return (1);
      }
      /* Must retrieve rented items w/in 30 days */
    } else if (rentcode == RENT_RENTED)
      if (timed < time(0) - (CONFIG_RENT_TIMEOUT * SECS_PER_REAL_DAY)) {
        Crash_delete_file(name);
        mud_log("    Deleting %s's rent file.", name);
        return (1);
      }
  }
  return (0);
}

void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name)
      Crash_clean_file(player_table[i].name);
}

void Crash_listrent(struct char_data *ch, char *name)
{
  FILE *fl = NULL;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  int rentcode = -1, timed = 0, netcost = 0, gold = 0, account = 0, nitems = 0, len;
  int t[10], nr;
  char line[MAX_STRING_LENGTH];
  char *sdesc;

  if (get_filename(filename, sizeof(filename), NEW_OBJ_FILES, name))
    fl = fopen(filename, "rb");
  if (!fl) {
    send_to_char(ch, "%s has no rent file.\r\n", name);
    return;
  }

  send_to_char(ch, "%s\r\n", filename);

  if (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rentcode, &timed, &netcost,
           &gold, &account, &nitems);
  }

  switch (rentcode) {
    case RENT_RENTED:
      send_to_char(ch, "Rent\r\n");
      break;
    case RENT_CRASH:
      send_to_char(ch, "Crash\r\n");
      break;
    case RENT_CRYO:
      send_to_char(ch, "Cryo\r\n");
      break;
    case RENT_TIMEDOUT:
    case RENT_FORCED:
      send_to_char(ch, "TimedOut\r\n");
      break;
    default:
      send_to_char(ch, "Undef\r\n");
      break;
  }
  buf[0] = 0;
  len    = 0;

  while (!feof(fl)) {
    get_line(fl, line);
    if (*line == '#') { /* swell - its an item */
      sscanf(line, "#%d", &nr);
      if (nr != NOTHING) { /* then we can dispense with it easily */
        if (real_object(nr) != NOTHING) {
          obj = read_object(nr, VIRTUAL);
          if (len + 255 < sizeof(buf)) {
            len += snprintf(buf + len, sizeof(buf) - len, "[%5d] (%5dau) %-20s\r\n",
                            nr, obj_get_cost_per_day(obj), obj->short_description);
          } else {
            snprintf(buf + len, sizeof(buf) - len, "** Excessive rent listing. **\r\n");
            break;
          }
          extract_obj(obj);
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

  page_string(ch->desc, buf, 0);
  fclose(fl);
}

static int Crash_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->next_content, fp, location);
    Crash_save(obj->contains, fp, MIN(0, location) - 1);
    result = Obj_to_store(obj, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      obj_set_weight(tmp, obj_get_weight(tmp) - (obj_get_weight(obj)));

    if (!result)
      return (FALSE);
  }
  return (TRUE);
}

static void Crash_restore_weight(struct obj_data *obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      obj_set_weight(obj->in_obj, obj_get_weight(obj->in_obj) + (obj_get_weight(obj)));
  }
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norent_eq(struct char_data *ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++) {
    if (char_get_equipment(ch, j) == NULL)
      continue;

    if (Crash_is_unrentable(char_get_equipment(ch, j)))
      obj_to_char(unequip_char(ch, j), ch);
    else
      Crash_extract_norents(char_get_equipment(ch, j));
  }
}

static void Crash_extract_objs(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}

static int Crash_is_unrentable(struct obj_data *obj)
{
  if (!obj)
    return (0);
  if (obj_extra_flagged(obj, ITEM_NORENT) || obj_get_cost_per_day(obj) < 0 ||
      (obj_get_item_number(obj) <= NOTHING &&
       !obj_extra_flagged(obj, ITEM_UNIQUE_SAVE))) {
    return (1);
  }
  return (0);
}

static void Crash_extract_norents(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}

void Crash_crashsave(struct char_data *ch)
{
  if (char_is_npc(ch))
    return;

  if (Crash_write_safe_file(ch, RENT_CRASH, 0, "crashsave"))
    char_plr_set(ch, PLR_CRASH, false);
}

void Crash_rentsave(struct char_data *ch, int cost)
{
  int j;

  if (char_is_npc(ch))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  if (!Crash_write_safe_file(ch, RENT_RENTED, cost, "rentsave"))
    return;

  for (j = 0; j < NUM_WEARS; j++)
    if (char_get_equipment(ch, j))
      Crash_extract_objs(char_get_equipment(ch, j));

  Crash_extract_objs(ch->carrying);
}

/* ************************************************************************
 * Routines used for the receptionist					  *
 ************************************************************************* */

void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((desc_get_connected(d) == CON_PLAYING) && !char_is_npc(d->character)) {
      if (char_plr_flagged(d->character, PLR_CRASH)) {
        Crash_crashsave(d->character);
        save_char(d->character);
        char_plr_set(d->character, PLR_CRASH, false);
      }
    }
  }
}

int Crash_load(struct char_data *ch)
{
  FILE *fl;
  char cmfname[MAX_STRING_LENGTH], cmfnamebak[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];

  if (!get_filename(cmfname, sizeof(cmfname), NEW_OBJ_FILES, char_get_name(ch)))
    return 1;

  if (!Crash_build_save_filename(cmfnamebak, sizeof(cmfnamebak), cmfname, ".bak", __FILE__, __LINE__))
    return -1;

  if ((fl = fopen(cmfname, "r+b")))
    return Crash_load_file(ch, fl, cmfname);

  snprintf(buf1, sizeof(buf1), "SYSERR: READING OBJECT FILE %s (5)", cmfname);
  perror(buf1);
  send_to_char(ch,
               "\r\n********************* NOTICE *********************\r\n"
               "There was a problem loading your objects from disk.\r\n"
               "Trying your backup object file.\r\n");

  if ((fl = fopen(cmfnamebak, "r+b"))) {
    mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s loading backup object file %s.", char_get_name(ch), cmfnamebak);
    return Crash_load_file(ch, fl, cmfnamebak);
  }

  Crash_log_file_error("fopen backup object file", cmfnamebak, __FILE__, __LINE__);
  mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s entering game with no equipment. Loading backup failed.", char_get_name(ch));
  send_to_char(ch,
               "\r\n********************* NOTICE *********************\r\n"
               "There was a problem loading your objects from backup.\r\n"
               "Contact staff for assistance.\r\n");

  return -1;
}

static int Crash_load_file(struct char_data *ch, FILE *fl, const char *filename)
{
  char buf2[MAX_STRING_LENGTH];
  char line[256];
  int t[30], zwei = 0;
  int orig_rent_code;
  struct obj_data *temp;
  int locate = 0, j, nr, num_objs = 0;
  struct obj_data *obj1;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  struct extra_descr_data *new_descr;
  int rentcode, timed, netcost, gold, account, nitems;
  char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];

  if (!feof(fl))
    get_line(fl, line);

  sscanf(line, "%d %d %d %d %d %d", &rentcode, &timed,
         &netcost, &gold, &account, &nitems);

  if (rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT) {
    save_char(ch);
  }
  switch (orig_rent_code = rentcode) {
    case RENT_RENTED:
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s un-renting and entering game.", char_get_name(ch));
      break;
    case RENT_CRASH:
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s retrieving crash-saved items and entering game.", char_get_name(ch));
      break;
    case RENT_CRYO:
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s un-cryo'ing and entering game.", char_get_name(ch));
      break;
    case RENT_FORCED:
    case RENT_TIMEDOUT:
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s retrieving force-saved items and entering game.", char_get_name(ch));
      break;
    default:
      mudlog(LOG_BRF, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "WARNING: %s entering game with undefined rent code.", char_get_name(ch));
      break;
  }

  for (j = 0; j < MAX_BAG_ROWS; j++)
    cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

  if (!feof(fl))
    get_line(fl, line);

  while (!feof(fl)) {
    temp = NULL;
    /* first, we get the number. Not too hard. */
    if (*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) { /* then it is unique */
        temp              = create_obj();
        temp->item_number = NOTHING;
        obj_set_size(temp, SIZE_MEDIUM);
      } else if (nr < 0) {
        continue;
      } else {
        if (nr >= 999999)
          continue;
        temp = read_object(nr, VIRTUAL);
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
        if ((temp->name = fread_string(fl, "rented object name")) == NULL) {
          temp->name = "undefined";
        }

        if ((temp->short_description = fread_string(fl, "rented object short desc")) == NULL) {
          temp->short_description = "undefined";
        }

        if ((temp->description = fread_string(fl, "rented object desc")) == NULL) {
          temp->description = "undefined";
        }

        if ((temp->action_description = fread_string(fl, "rented object adesc")) == NULL) {
          temp->action_description = 0;
        }

        if (!get_line(fl, line) ||
            (sscanf(line, "%d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7) != 8)) {
          fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
          return 0;
        }
        temp->type_flag     = t[0];
        temp->wear_flags[0] = t[1];
        temp->wear_flags[1] = t[2];
        temp->wear_flags[2] = t[3];
        temp->wear_flags[3] = t[4];
        temp->weight        = t[5];
        temp->cost          = t[6];

        /* we're clearing these for good luck */

        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          temp->affected[j].location = APPLY_NONE;
          temp->affected[j].modifier = 0;
          temp->affected[j].specific = 0;
        }

        /* maybe clear spellbook for good luck too? */

        temp->ex_description = NULL;

        get_line(fl, line);
        for (j = zwei = 0; !zwei && !feof(fl);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
              sprintf(buf2, "rented object edesc keyword for object #%d", nr);
              new_descr->keyword = fread_string(fl, buf2);
              sprintf(buf2, "rented object edesc text for object #%d keyword %s", nr, new_descr->keyword);
              new_descr->description = fread_string(fl, buf2);
              new_descr->next        = temp->ex_description;
              temp->ex_description   = new_descr;
              get_line(fl, line);
              break;
            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                mud_log("SYSERR: Too many object affectations in loading rent file");
              }
              get_line(fl, line);
              sscanf(line, "%d %d %d", t, t + 1, t + 2);

              temp->affected[j].location = t[0];
              temp->affected[j].modifier = t[1];
              temp->affected[j].specific = t[2];
              j++;
              get_line(fl, line);
              break;
            case 'G':
              get_line(fl, line);
              sscanf(line, "%" TMT, &temp->generation);
              get_line(fl, line);
              break;
            case 'U':
              get_line(fl, line);
              sscanf(line, "%" I64T, &temp->unique_id);
              get_line(fl, line);
              break;
            case 'S':
              if (j >= SPELLBOOK_SIZE) {
                mud_log("SYSERR: Too many spells in spellbook loading rent file");
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
      if (temp != NULL) {
        num_objs++;
        check_unique_id(temp);
        add_unique_id(temp);
        if (obj_get_type_flag(temp) == ITEM_DRINKCON) {
          name_from_drinkcon(temp);
          if (obj_get_value(temp, 1) != 0)
            name_to_drinkcon(temp, obj_get_value(temp, 2));
        }
        if (obj_get_vnum(temp) == 20099 || obj_get_vnum(temp) == 20098)
          if (obj_extra_flagged(temp, ITEM_UNBREAKABLE))
            obj_extra_set(temp, ITEM_UNBREAKABLE, false);
        auto_equip(ch, temp, locate);
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
          if (cont_row[j]) { /* no container -> back to ch's inventory */
            for (; cont_row[j]; cont_row[j] = obj1) {
              obj1 = cont_row[j]->next_content;
              obj_to_char(cont_row[j], ch);
            }
            cont_row[j] = NULL;
          }
        if (cont_row[0]) { /* content list existing */
          if (obj_get_type_flag(temp) == ITEM_CONTAINER) {
            /* rem item ; fill ; equip again */
            temp           = unequip_char(ch, locate - 1);
            temp->contains = NULL; /* should be empty - but who knows */
            for (; cont_row[0]; cont_row[0] = obj1) {
              obj1 = cont_row[0]->next_content;
              obj_to_obj(cont_row[0], temp);
            }
            equip_char(ch, temp, locate - 1);
          } else { /* object isn't container -> empty content list */
            for (; cont_row[0]; cont_row[0] = obj1) {
              obj1 = cont_row[0]->next_content;
              obj_to_char(cont_row[0], ch);
            }
            cont_row[0] = NULL;
          }
        }
      } else { /* locate <= 0 */
        for (j = MAX_BAG_ROWS - 1; j > -locate; j--)
          if (cont_row[j]) { /* no container -> back to ch's inventory */
            for (; cont_row[j]; cont_row[j] = obj1) {
              obj1 = cont_row[j]->next_content;
              obj_to_char(cont_row[j], ch);
            }
            cont_row[j] = NULL;
          }

        if (j == -locate && cont_row[j]) { /* content list existing */
          if (obj_get_type_flag(temp) == ITEM_CONTAINER) {
            /* take item ; fill ; give to char again */
            obj_from_char(temp);
            temp->contains = NULL;
            for (; cont_row[j]; cont_row[j] = obj1) {
              obj1 = cont_row[j]->next_content;
              obj_to_obj(cont_row[j], temp);
            }
            obj_to_char(temp, ch); /* add to inv first ... */
          } else {                 /* object isn't container -> empty content list */
            for (; cont_row[j]; cont_row[j] = obj1) {
              obj1 = cont_row[j]->next_content;
              obj_to_char(cont_row[j], ch);
            }
            cont_row[j] = NULL;
          }
        }

        if (locate < 0 && locate >= -MAX_BAG_ROWS) {
          /* let obj be part of content list
             but put it at the list's end thus having the items
             in the same order as before renting */
          obj_from_char(temp);
          if ((obj1 = cont_row[-locate - 1])) {
            while (obj1->next_content)
              obj1 = obj1->next_content;
            obj1->next_content = temp;
          } else
            cont_row[-locate - 1] = temp;
        }
      } /* locate less than zero */
    } else {
      get_line(fl, line);
    }
  }

  /* Little hoarding check. -gg 3/1/98 */
  mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(ch)), TRUE, "%s (level %d) has %d objects (max %d).",
         char_get_name(ch), char_get_level(ch), num_objs, CONFIG_MAX_OBJ_SAVE);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}
