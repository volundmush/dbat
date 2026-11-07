/*************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cstddef>
#include <cerrno>
#include <cstdlib>
#include <memory>

#include <linux/limits.h>

#include "dbat/iterate.hpp"
#include "dbat/db.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/help.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/fileop.hpp"
#include "dbat/affect.hpp"
#include "dbat/weather.hpp"
#include "dbat/time.hpp"
#include "dbat/xdir.hpp"
#include "dbat/search.hpp"
#include "dbat/extract.hpp"
#include "dbat/relocate.hpp"
#include "dbat/players.hpp"
#include "dbat/feats.hpp"
#include "dbat/config.hpp"
#include "dbat/cedit.hpp"
#include "dbat/spec_assign.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.other.hpp"
#include "dbat/act.social.hpp"
#include "dbat/random.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/house.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/reset.hpp"
#include "dbat/class.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/genolc.hpp"
#include "dbat/shop.hpp"
#include "dbat/handler.hpp"
#include "dbat/mail.hpp"
#include "dbat/boards.hpp"
#include "dbat/log.hpp"
#include "dbat/objsave.hpp"
#include "dbat/genmob.hpp"
#include "dbat/spells.hpp"
#include "dbat/races.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/genobj.hpp"
#include "dbat/guild.hpp"
#include "dbat/context_help.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

static void load_help(GameWorld *g, FILE *fl, char *name);

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/






/* local functions */
static void mob_stats(GameWorld *g, struct char_data *mob);
static void dragon_level(GameWorld *g, struct char_data *ch);
static int check_object_spell_number(GameWorld *g, struct obj_data *obj, int val);
static int check_object_level(GameWorld *g, struct obj_data *obj, int val);
static void setup_dir(GameWorld *g, FILE *fl, room_rnum room, int dir);
static void discrete_load(GameWorld *g, FILE *fl, int mode, char *filename);
static int check_object(GameWorld *g, struct obj_data *);
static void parse_room(GameWorld *g, FILE *fl, room_vnum virtual_nr);
static void parse_mobile(GameWorld *g, FILE *mob_f, mob_vnum nr);
static char *parse_object(GameWorld *g, FILE *obj_f, IDXTYPE nr);
static void load_zones(GameWorld *g, FILE *fl, char *zonename);
static int file_to_string(const char *name, char *buf);
static int file_to_string_alloc(const char *name, char **buf);
static int count_alias_records(FILE *fl);
static int count_hash_records(FILE *fl);
static bitvector_t asciiflag_conv_aff(char *flag);
static bool parse_simple_mob(GameWorld *g, FILE *mob_f, struct char_data *ch, mob_vnum nr);
static void interpret_espec(GameWorld *g, const char *keyword, const char *value, struct char_data *ch, mob_vnum nr);
static void parse_espec(GameWorld *g, char *buf, struct char_data *ch, mob_vnum nr);
static bool parse_enhanced_mob(GameWorld *g, FILE *mob_f, struct char_data *ch, mob_vnum nr);
static void get_one_line(FILE *fl, char *buf);
static void check_start_rooms(GameWorld *g);
static void renum_world(GameWorld *g);
static void renum_zone_table(GameWorld *g);
static void log_zone_error(GameWorld *g, struct zone_data *zone, int cmd_no, const char *message);
static void reset_time(GameWorld *g);
static int suntzu_weapon_convert(int wp_type);
static void free_obj_unique_hash(GameWorld *g);
static void mob_autobalance(struct char_data *ch);

/* external functions */

static int hsort(const void *a, const void *b);
void memorize_add(struct char_data *ch, int spellnum, int timer);

/* external vars */

static void dragon_level(GameWorld *g, struct char_data *ch)
{
  int level = 0, count = 0;

  for (descriptor_data *d : each_desc(g)) {
    if (desc_is_playing(d) && char_get_admlevel(d->character) < 1) {
      level += char_get_level(d->character);
      count += 1;
    }
  }

  if (level > 0 && count > 0) {
    level = level / count;
  } else {
    level = rand_number(g, 60, 110);
  }

  if (level < 50) {
    level = rand_number(g, 40, 60);
  }

  char_set_race_level(ch, 0);
  char_set_race_level(ch, level + rand_number(g, 5, 20));
}

static void mob_stats(GameWorld *g, struct char_data *mob)
{
  int start = char_get_level(mob) * 0.5, finish = char_get_level(mob);

  if (finish < 20)
    finish = 20;

  if (!char_is_humanoid(mob)) {
    char_set_real_abils_str(mob, rand_number(g, start, finish));
    char_set_real_abils_intel(mob, rand_number(g, start, finish) - 30);
    char_set_real_abils_wis(mob, rand_number(g, start, finish) - 30);
    char_set_real_abils_dex(mob, rand_number(g, start + 5, finish));
    char_set_real_abils_con(mob, rand_number(g, start + 5, finish));
    char_set_real_abils_cha(mob, rand_number(g, start, finish));
  } else {
    if (char_is_saiyan(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start + 10, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_wis(mob, rand_number(g, start, finish - 5));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start + 5, finish));
      char_set_real_abils_cha(mob, rand_number(g, start + 5, finish));
    } else if (char_is_konatsu(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start + 10, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_android(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_majin(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_wis(mob, rand_number(g, start, finish - 5));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start + 15, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_truffle(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_intel(mob, rand_number(g, start + 15, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_icer(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start + 5, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start + 10, finish));
    } else {
      char_set_real_abils_str(mob, rand_number(g, start, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    }
  }

  if (char_get_real_abils_str(mob) > 100)
    char_set_real_abils_str(mob, 100);
  else if (char_get_real_abils_str(mob) < 5)
    char_set_real_abils_str(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_intel(mob) > 100)
    char_set_real_abils_intel(mob, 100);
  else if (char_get_real_abils_intel(mob) < 5)
    char_set_real_abils_intel(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_wis(mob) > 100)
    char_set_real_abils_wis(mob, 100);
  else if (char_get_real_abils_wis(mob) < 5)
    char_set_real_abils_wis(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_con(mob) > 100)
    char_set_real_abils_con(mob, 100);
  else if (char_get_real_abils_con(mob) < 5)
    char_set_real_abils_con(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_cha(mob) > 100)
    char_set_real_abils_cha(mob, 100);
  else if (char_get_real_abils_cha(mob) < 5)
    char_set_real_abils_cha(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_dex(mob) > 100)
    char_set_real_abils_dex(mob, 100);
  else if (char_get_real_abils_dex(mob) < 5)
    char_set_real_abils_dex(mob, rand_number(g, 5, 8));
}

/* Convert CWG-SunTzu armor objects to new armor types */

/* Convert CWG-SunTzu weapon objects to new weapon types */

static int suntzu_weapon_convert(int wp_type)
{
  int new_type;

  switch (wp_type) {
    case 170:
      new_type = WEAPON_TYPE_DAGGER;
      break;
    case 171:
      new_type = WEAPON_TYPE_SHORTSWORD;
      break;
    case 172:
      new_type = WEAPON_TYPE_LONGSWORD;
      break;
    case 173:
      new_type = WEAPON_TYPE_GREATSWORD;
      break;
    case 174:
      new_type = WEAPON_TYPE_MACE;
      break;
    case 175:
      new_type = WEAPON_TYPE_AXE;
      break;
    case 176:
      new_type = WEAPON_TYPE_WHIP;
      break;
    case 177:
      new_type = WEAPON_TYPE_SPEAR;
      break;
    case 178:
      new_type = WEAPON_TYPE_POLEARM;
      break;
    case 179:
      new_type = WEAPON_TYPE_UNARMED;
      break;
    case 180:
      new_type = WEAPON_TYPE_FLAIL;
      break;
    case 181:
      new_type = WEAPON_TYPE_STAFF;
      break;
    case 182:
      new_type = WEAPON_TYPE_HAMMER;
      break;
    default:
      new_type = WEAPON_TYPE_UNDEFINED;
      break;
  }
  mud_log("Converted weapon from [%d] to [%d].", wp_type, new_type);
  return new_type;
}

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(GameWorld *g)
{
  file_to_string_alloc(WIZLIST_FILE, &g->wizlist);
  file_to_string_alloc(IMMLIST_FILE, &g->immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(GameWorld *g)
{
  char **textfiles[] = {
      &g->wizlist, &g->immlist, &g->news, &g->credits, &g->motd, &g->imotd, &g->help, &g->info,
      &g->policies, &g->handbook, &g->background, &g->GREETINGS, &g->GREETANSI, &g->ihelp, nullptr};
  int rf;

  for (rf = 0; textfiles[rf]; rf++)
    if (*textfiles[rf]) {
      free(*textfiles[rf]);
      *textfiles[rf] = nullptr;
    }
}

/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!strcasecmp(arg, "all") || *arg == '*') {
    if (load_levels(g) < 0)
      send_to_char(g, ch, "Cannot read level configurations\r\n");
    if (file_to_string_alloc(GREETINGS_FILE, &g->GREETINGS) == 0)
      prune_crlf(g->GREETINGS);
    if (file_to_string_alloc(GREETANSI_FILE, &g->GREETANSI) == 0)
      prune_crlf(g->GREETANSI);
    if (file_to_string_alloc(WIZLIST_FILE, &g->wizlist) < 0)
      send_to_char(g, ch, "Cannot read wizlist\r\n");
    if (file_to_string_alloc(IMMLIST_FILE, &g->immlist) < 0)
      send_to_char(g, ch, "Cannot read immlist\r\n");
    if (file_to_string_alloc(NEWS_FILE, &g->news) < 0)
      send_to_char(g, ch, "Cannot read news\r\n");
    if (file_to_string_alloc(CREDITS_FILE, &g->credits) < 0)
      send_to_char(g, ch, "Cannot read credits\r\n");
    if (file_to_string_alloc(MOTD_FILE, &g->motd) < 0)
      send_to_char(g, ch, "Cannot read motd\r\n");
    if (file_to_string_alloc(IMOTD_FILE, &g->imotd) < 0)
      send_to_char(g, ch, "Cannot read imotd\r\n");
    if (file_to_string_alloc(HELP_PAGE_FILE, &g->help) < 0)
      send_to_char(g, ch, "Cannot read help front page\r\n");
    if (file_to_string_alloc(INFO_FILE, &g->info) < 0)
      send_to_char(g, ch, "Cannot read info file\r\n");
    if (file_to_string_alloc(POLICIES_FILE, &g->policies) < 0)
      send_to_char(g, ch, "Cannot read policies\r\n");
    if (file_to_string_alloc(HANDBOOK_FILE, &g->handbook) < 0)
      send_to_char(g, ch, "Cannot read handbook\r\n");
    if (file_to_string_alloc(BACKGROUND_FILE, &g->background) < 0)
      send_to_char(g, ch, "Cannot read background\r\n");
    if (g->help_table)
      free_help_table(g);
    index_boot(g, DB_BOOT_HLP);
  } else if (!strcasecmp(arg, "levels")) {
    if (load_levels(g) < 0)
      send_to_char(g, ch, "Cannot read level configurations\r\n");
  } else if (!strcasecmp(arg, "wizlist")) {
    if (file_to_string_alloc(WIZLIST_FILE, &g->wizlist) < 0)
      send_to_char(g, ch, "Cannot read wizlist\r\n");
  } else if (!strcasecmp(arg, "immlist")) {
    if (file_to_string_alloc(IMMLIST_FILE, &g->immlist) < 0)
      send_to_char(g, ch, "Cannot read immlist\r\n");
  } else if (!strcasecmp(arg, "news")) {
    if (file_to_string_alloc(NEWS_FILE, &g->news) < 0)
      send_to_char(g, ch, "Cannot read news\r\n");
  } else if (!strcasecmp(arg, "credits")) {
    if (file_to_string_alloc(CREDITS_FILE, &g->credits) < 0)
      send_to_char(g, ch, "Cannot read credits\r\n");
  } else if (!strcasecmp(arg, "motd")) {
    if (file_to_string_alloc(MOTD_FILE, &g->motd) < 0)
      send_to_char(g, ch, "Cannot read motd\r\n");
  } else if (!strcasecmp(arg, "imotd")) {
    if (file_to_string_alloc(IMOTD_FILE, &g->imotd) < 0)
      send_to_char(g, ch, "Cannot read imotd\r\n");
  } else if (!strcasecmp(arg, "help")) {
    if (file_to_string_alloc(HELP_PAGE_FILE, &g->help) < 0)
      send_to_char(g, ch, "Cannot read help front page\r\n");
  } else if (!strcasecmp(arg, "info")) {
    if (file_to_string_alloc(INFO_FILE, &g->info) < 0)
      send_to_char(g, ch, "Cannot read info\r\n");
  } else if (!strcasecmp(arg, "policy")) {
    if (file_to_string_alloc(POLICIES_FILE, &g->policies) < 0)
      send_to_char(g, ch, "Cannot read policy\r\n");
  } else if (!strcasecmp(arg, "handbook")) {
    if (file_to_string_alloc(HANDBOOK_FILE, &g->handbook) < 0)
      send_to_char(g, ch, "Cannot read handbook\r\n");
  } else if (!strcasecmp(arg, "background")) {
    if (file_to_string_alloc(BACKGROUND_FILE, &g->background) < 0)
      send_to_char(g, ch, "Cannot read background\r\n");
  } else if (!strcasecmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &g->GREETINGS) == 0)
      prune_crlf(g->GREETINGS);
    else
      send_to_char(g, ch, "Cannot read greetings.\r\n");
  } else if (!strcasecmp(arg, "greetansi")) {
    if (file_to_string_alloc(GREETANSI_FILE, &g->GREETANSI) == 0)
      prune_crlf(g->GREETANSI);
    else
      send_to_char(g, ch, "Cannot read greetings.\r\n");
  } else if (!strcasecmp(arg, "xhelp")) {
    if (g->help_table)
      free_help_table(g);
    index_boot(g, DB_BOOT_HLP);
  } else if (!strcasecmp(arg, "ihelp")) {
    if (file_to_string_alloc(IHELP_PAGE_FILE, &g->ihelp) < 0)
      send_to_char(g, ch, "Cannot read help front page\r\n");
  } else {
    send_to_char(g, ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(g, ch, "%s", CONFIG_OK);
}

/* The vnum lookups (real_room() and friends) are binary searches, so each table must be in ascending vnum order. */
static void check_sorted(const char *what, const void *table, IDXTYPE count, size_t size, size_t vnum_offset)
{
  const char *t = static_cast<const char *>(table);

  for (IDXTYPE i = 1; i < count; i++) {
    IDXTYPE prev, vnum;

    memcpy(&prev, t + (i - 1) * size + vnum_offset, sizeof(prev));
    memcpy(&vnum, t + i * size + vnum_offset, sizeof(vnum));

    if (vnum <= prev) {
      mud_log("SYSERR: %s #%d comes after #%d: the %s files must be in ascending vnum order, without duplicates.", what, vnum, prev, what);
      exit(1);
    }
  }
}

void boot_world(GameWorld *g)
{
  mud_log("Loading level tables.");
  load_levels(g);

  mud_log("Loading zone table.");
  index_boot(g, DB_BOOT_ZON);

  mud_log("Loading triggers and generating index.");
  index_boot(g, DB_BOOT_TRG);

  mud_log("Loading rooms.");
  index_boot(g, DB_BOOT_WLD);
  check_sorted("room", g->rooms, g->top_room + 1, sizeof(*g->rooms), offsetof(struct room_data, number));

  mud_log("Renumbering rooms.");
  renum_world(g);

  mud_log("Checking start rooms.");
  check_start_rooms(g);

  mud_log("Loading mobs and generating index.");
  index_boot(g, DB_BOOT_MOB);
  check_sorted("mobile", g->mob_index, g->top_mob + 1, sizeof(*g->mob_index), offsetof(struct index_data, vnum));

  mud_log("Loading objs and generating index.");
  index_boot(g, DB_BOOT_OBJ);

  mud_log("Renumbering zone table.");
  renum_zone_table(g);

  mud_log("Loading disabled commands list...");
  load_disabled(g);

  if (g->converting) {
    mud_log("Saving converted worldfiles to disk.");
    save_all(g);
  }

  if (!g->no_specials) {
    mud_log("Loading shops.");
    index_boot(g, DB_BOOT_SHP);

    mud_log("Loading guild masters.");
    index_boot(g, DB_BOOT_GLD);
  }
  if (g->SELFISHMETER >= 10) {
    mud_log("Loading Shadow Dragons.");
    load_shadow_dragons(g);
  }
}

static void free_extra_descriptions(struct extra_descr_data *edesc)
{
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext) {
    enext = edesc->next;

    free(edesc->keyword);
    free(edesc->description);
    delete edesc;
  }
}

/* Free the world, in a memory allocation sense. */
void destroy_db(GameWorld *g)
{
  ssize_t cnt, itr;
  struct char_data *chtmp;
  struct obj_data *objtmp;

  free_extracted_triggers(g);

  /* Active Mobiles & Players */
  /* stop_follower() reads the master, so everyone must stop before anyone is freed */
  for (char_data *tch : each_char(g))
    if (char_get_master(g, tch))
      stop_follower(g, tch);
  while (g->characters) {
    chtmp          = g->characters;
    g->characters = char_get_next(g->characters);
    free_char(g, chtmp);
  }

  /* Active Objects */
  while (g->objects) {
    objtmp      = g->objects;
    g->objects = obj_get_next(g->objects);
    free_obj(g, objtmp);
  }

  /* Rooms */
  for (cnt = 0; cnt <= g->top_room; cnt++) {
    if (rroom_get_name(g, cnt))
      free(rroom_get_name(g, cnt));
    if (rroom_get_description(g, cnt))
      free(rroom_get_description(g, cnt));
    free_extra_descriptions(rroom_get_ex_description(g, cnt));

    /* free any assigned scripts */
    if (rroom_get_script(g, cnt))
      extract_script(g, &g->rooms[cnt], WLD_TRIGGER);
    /* free script proto list */
    free_proto_script(&g->rooms[cnt], WLD_TRIGGER);

    for (itr = 0; itr < NUM_OF_DIRS; itr++) {
      if (!rroom_get_dir_option(g, cnt, itr))
        continue;

      if (rroom_get_dir_option(g, cnt, itr)->general_description)
        free(rroom_get_dir_option(g, cnt, itr)->general_description);
      if (rroom_get_dir_option(g, cnt, itr)->keyword)
        free(rroom_get_dir_option(g, cnt, itr)->keyword);
      delete rroom_get_dir_option(g, cnt, itr);
    }
  }
  delete[] g->rooms;
  g->top_room = 0;

  /* Objects */
  for (obj_vnum ovnum : g->obj_proto.vnums()) {
    if (obj_get_name(obj_proto_of(g, ovnum)))
      free(obj_get_name(obj_proto_of(g, ovnum)));
    if (obj_get_description(obj_proto_of(g, ovnum)))
      free(obj_get_description(obj_proto_of(g, ovnum)));
    if (obj_get_short_description(obj_proto_of(g, ovnum)))
      free(obj_get_short_description(obj_proto_of(g, ovnum)));
    if (obj_get_action_description(obj_proto_of(g, ovnum)))
      free(obj_get_action_description(obj_proto_of(g, ovnum)));
    if (obj_get_ex_description(obj_proto_of(g, ovnum)))
      free_extra_descriptions(obj_get_ex_description(obj_proto_of(g, ovnum)));
    delete[] obj_get_sbinfo(obj_proto_of(g, ovnum)); /* spellbook pages */

    /* free script proto list */
    free_proto_script(obj_proto_of(g, ovnum), OBJ_TRIGGER);
  }
  g->obj_proto.clear();
  g->obj_index.clear();

  /* Mobiles */
  for (cnt = 0; cnt <= g->top_mob; cnt++) {
    if (char_get_alias(&g->mob_proto[cnt]))
      free(char_get_alias(&g->mob_proto[cnt]));
    if (char_get_raw_title(&g->mob_proto[cnt]))
      free(char_get_raw_title(&g->mob_proto[cnt]));
    if (char_get_sdesc(&g->mob_proto[cnt]))
      free(char_get_sdesc(&g->mob_proto[cnt]));
    if (char_get_ldesc(&g->mob_proto[cnt]))
      free(char_get_ldesc(&g->mob_proto[cnt]));
    if (char_get_ddesc(&g->mob_proto[cnt]))
      free(char_get_ddesc(&g->mob_proto[cnt]));

    /* free script proto list */
    free_proto_script(&g->mob_proto[cnt], MOB_TRIGGER);

    while (char_get_affected(&g->mob_proto[cnt]))
      affect_remove(g, &g->mob_proto[cnt], char_get_affected(&g->mob_proto[cnt]));
  }
  delete[] g->mob_proto;
  delete[] g->mob_index;

  /* Shops */
  destroy_shops(g);

  /* Guilds */
  destroy_guilds(g);

  /* Zones */
  /* zone table reset queue */
  if (g->reset_q.head) {
    struct reset_q_element *ftemp = g->reset_q.head, *temp;
    while (ftemp) {
      temp = ftemp->next;
      delete ftemp;
      ftemp = temp;
    }
  }

#define THIS_CMD zone->cmd[itr]

  for (struct zone_data *zone : g->zones.all()) {
    if (zone->name)
      free(zone->name);
    if (zone->builders)
      free(zone->builders);
    if (zone->cmd) {
      /* first see if any vars were defined in this zone */
      for (itr = 0; THIS_CMD.command != 'S'; itr++)
        if (THIS_CMD.command == 'V') {
          if (THIS_CMD.sarg1)
            free(THIS_CMD.sarg1);
          if (THIS_CMD.sarg2)
            free(THIS_CMD.sarg2);
        }
      /* then free the command list */
      delete[] zone->cmd;
    }
  }
  g->zones.clear();

#undef THIS_CMD

  /* Triggers */
  for (struct index_data *entry : g->trig_index.all()) {
    if (entry->proto) {
      /* make sure to nuke the command list (memory leak) */
      /* free_trigger() doesn't free the command list */
      if (entry->proto->cmdlist) {
        struct cmdlist_element *i, *j;
        i = entry->proto->cmdlist;
        while (i) {
          j = i->next;
          if (i->cmd)
            free(i->cmd);
          delete i;
          i = j;
        }
      }
      free_trigger(g, entry->proto);
    }
  }
  g->trig_index.clear();

  /* Events */
  event_free_all(g);

  /* context sensitive help system */
  free_context_help(g);

  free_feats();

  free_obj_unique_hash(g);

  mud_log("Freeing Assemblies.");
  free_assemblies(g);
}

/* You can define this to anything you want; 1 would work but it would
   be very inefficient. I would recommend that it actually be close to
   your total number of in-game objects if not double or triple it just
   to minimize collisions. The only O(n) [n=NUM_OBJ_UNIQUE_POOLS]
   operation is initialization of the hash table, all other operations
   that have to traverse are O(n) [n=num elements in pool], so more
   pools are better.
     - Elie Rosenblum Dec. 12 2003 */
constexpr int NUM_OBJ_UNIQUE_POOLS = 5000;


static void init_obj_unique_hash(GameWorld *g)
{
  g->obj_unique_hash_pools = new obj_unique_hash_elem *[NUM_OBJ_UNIQUE_POOLS]();
}

/* body of the booting system */
void boot_db(GameWorld *g)
{

  mud_log("Boot db -- BEGIN.");

  mud_log("Resetting the game time:");
  reset_time(g);

  mud_log("Reading news, credits, help, ihelp, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &g->news);
  file_to_string_alloc(CREDITS_FILE, &g->credits);
  file_to_string_alloc(MOTD_FILE, &g->motd);
  file_to_string_alloc(IMOTD_FILE, &g->imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &g->help);
  file_to_string_alloc(INFO_FILE, &g->info);
  file_to_string_alloc(WIZLIST_FILE, &g->wizlist);
  file_to_string_alloc(IMMLIST_FILE, &g->immlist);
  file_to_string_alloc(POLICIES_FILE, &g->policies);
  file_to_string_alloc(HANDBOOK_FILE, &g->handbook);
  file_to_string_alloc(BACKGROUND_FILE, &g->background);
  file_to_string_alloc(IHELP_PAGE_FILE, &g->ihelp);
  if (file_to_string_alloc(GREETINGS_FILE, &g->GREETINGS) == 0)
    prune_crlf(g->GREETINGS);
  if (file_to_string_alloc(GREETANSI_FILE, &g->GREETANSI) == 0)
    prune_crlf(g->GREETANSI);

  mud_log("Loading spell definitions.");
  mag_assign_spells(g);

  mud_log("Loading feats.");
  assign_feats(g);

  boot_world(g);

  mud_log("Loading help entries.");
  index_boot(g, DB_BOOT_HLP);

  mud_log("Setting up context sensitive help system for OLC");
  boot_context_help(g);

  mud_log("Generating player index.");
  build_player_index(g);

  if (g->ERAPLAYERS <= 0)
    g->ERAPLAYERS = g->top_of_p_table + 1;

  insure_directory(LIB_PLROBJS "CRASH");

  mud_log("Booting mail system.");
  if (!scan_file(g)) {
    mud_log("    Mail boot failed -- Mail system disabled");
    g->no_mail = 1;
  }

  if (auto_pwipe) {
    mud_log("Cleaning out inactive players.");
    clean_pfiles(g);
  }

  mud_log("Loading social messages.");
  boot_social_messages(g);

  mud_log("Building command list.");
  create_command_list(g); /* aedit patch -- M. Scott */

  mud_log("Assigning function pointers:");

  if (!g->no_specials) {
    mud_log("   Mobiles.");
    assign_mobiles(g);
    mud_log("   Shopkeepers.");
    assign_the_shopkeepers(g);
    mud_log("   Objects.");
    assign_objects(g);
    mud_log("   Rooms.");
    assign_rooms(g);
    mud_log("   Guildmasters.");
    assign_the_guilds(g);
  }

  mud_log("Init Object Unique Hash");
  init_obj_unique_hash(g);

  mud_log("Booting assembled objects.");
  assemblyBootAssemblies(g);

  mud_log("Sorting command list and spells.");
  sort_commands(g);
  sort_spells(g);
  sort_feats(g);

  mud_log("Booting boards system.");
  init_boards(g);

  mud_log("Reading banned site and invalid-name list.");
  load_banned(g);
  Read_Invalid_List(g);

  if (!g->no_rent_check) {
    mud_log("Deleting timed-out crash and rent files:");
    update_obj_file(g);
    mud_log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!g->mini_mud) {
    mud_log("Booting houses.");
    House_boot(g);
  }

  for (struct zone_data *i : g->zones.all()) {
    mud_log("Resetting #%d: %s (rooms %d-%d).", i->number,
            i->name, i->bot, i->top);
    reset_zone(g, i);
  }

  g->reset_q.head = g->reset_q.tail = nullptr;

  g->boot_time = game_now(g);

  mud_log("Boot db -- DONE.");
}

/* save the auction file */
void auc_save(GameWorld *g)
{
  FILE *fl;

  if ((fl = fopen(AUCTION_FILE, "w")) == nullptr)
    mud_log("SYSERR: Can't write to '%s' auction file.", AUCTION_FILE);
  else {

    for (obj_data *obj : room_contents(g, real_room(g, 80))) {
      if (obj) {
        fprintf(fl, "%" PRId64 " %s %d %d %d %d %ld\n", obj_get_unique_id(obj), obj_get_auctname(obj), obj_get_aucter(obj), obj_get_curBidder(obj), obj_get_startbid(obj), obj_get_bid(obj), obj_get_aucTime(obj));
      }
    }
    fprintf(fl, "~END~\n");
    fclose(fl);
  }
}

/* load from auction file */
void auc_load(struct obj_data *obj)
{
  char line[500], filler[50];
  int64_t oID;
  time_t timer;
  int aID, bID, cost, startc;
  FILE *fl;

  if ((fl = fopen(AUCTION_FILE, "r")) == nullptr)
    mud_log("SYSERR: Can't read from '%s' auction file.", AUCTION_FILE);
  else {
    while (!feof(fl)) {
      get_line(fl, line);
      sscanf(line, "%" SCNd64 " %49s %d %d %d %d %ld\n", &oID, filler, &aID, &bID, &startc, &cost, &timer);
      if (obj_get_unique_id(obj) == oID) {
        obj_set_auctname(obj, strdup(filler));
        obj_set_aucter(obj, aID);
        obj_set_curBidder(obj, bID);
        obj_set_startbid(obj, startc);
        obj_set_bid(obj, cost);
        obj_set_aucTime(obj, timer);
      }
    }
    fclose(fl);
  }
}

/* reset the time in the game from file */
/* Reads a time_t written as a decimal number on its own line; t is unchanged if there is none. */
static void fscanf_time(FILE *fl, time_t *t)
{
  long long v;

  if (fscanf(fl, "%lld\n", &v) == 1)
    *t = v;
}

static void reset_time(GameWorld *g)
{
  time_t beginning_of_time = 0;
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "r")) == nullptr)
    mud_log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
  else {
    fscanf_time(bgtime, &beginning_of_time);
    fscanf_time(bgtime, &g->NEWSUPDATE);
    fscanf_time(bgtime, &g->BOARDNEWMORT);
    fscanf_time(bgtime, &g->BOARDNEWDUO);
    fscanf_time(bgtime, &g->BOARDNEWCOD);
    fscanf_time(bgtime, &g->BOARDNEWBUI);
    fscanf_time(bgtime, &g->BOARDNEWIMM);
    fscanf_time(bgtime, &g->INTERESTTIME);
    fscanf_time(bgtime, &g->LASTINTEREST);
    fscanf(bgtime, "%d\n", &g->HIGHPCOUNT);
    fscanf_time(bgtime, &g->PCOUNTDATE);
    fscanf(bgtime, "%d\n", &g->WISHTIME);
    fscanf(bgtime, "%d\n", &g->PCOUNT);
    fscanf_time(bgtime, &g->LASTPAYOUT);
    fscanf(bgtime, "%d\n", &g->LASTPAYTYPE);
    fscanf(bgtime, "%d\n", &g->LASTNEWS);
    fscanf(bgtime, "%d\n", &g->dballtime);
    fscanf(bgtime, "%d\n", &g->SELFISHMETER);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON1);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON2);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON3);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON4);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON5);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON6);
    fscanf(bgtime, "%d\n", &g->SHADOW_DRAGON7);
    fscanf(bgtime, "%d\n", &g->ERAPLAYERS);
    fclose(bgtime);
  }

  if (g->dballtime == 0)
    g->dballtime = 604800;

  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  g->time_info = *mud_time_passed(game_now(g), beginning_of_time);

  if (g->time_info.hours <= 4)
    g->weather_info.sunlight = SUN_DARK;
  else if (g->time_info.hours == 5)
    g->weather_info.sunlight = SUN_RISE;
  else if (g->time_info.hours <= 20)
    g->weather_info.sunlight = SUN_LIGHT;
  else if (g->time_info.hours == 21)
    g->weather_info.sunlight = SUN_SET;
  else
    g->weather_info.sunlight = SUN_DARK;

  mud_log("   Current Gametime: %dH %dD %dM %dY.", g->time_info.hours,
          g->time_info.day, g->time_info.month, g->time_info.year);

  g->weather_info.pressure = 960;
  if ((g->time_info.month >= 7) && (g->time_info.month <= 12))
    g->weather_info.pressure += dice(g, 1, 50);
  else
    g->weather_info.pressure += dice(g, 1, 80);

  g->weather_info.change = 0;

  if (g->weather_info.pressure <= 980)
    g->weather_info.sky = SKY_LIGHTNING;
  else if (g->weather_info.pressure <= 1000)
    g->weather_info.sky = SKY_RAINING;
  else if (g->weather_info.pressure <= 1020)
    g->weather_info.sky = SKY_CLOUDY;
  else
    g->weather_info.sky = SKY_CLOUDLESS;
}

/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(GameWorld *g, struct time_info_data *when)
{
  struct atomic_file af;
  FILE *bgtime;

  if ((bgtime = atomic_open(&af, TIME_FILE, false)) == nullptr)
    mud_log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else {
    fprintf(bgtime, "%lld\n", (long long)mud_time_to_secs(g, when));
    fprintf(bgtime, "%lld\n", (long long)g->NEWSUPDATE);
    fprintf(bgtime, "%lld\n", (long long)g->BOARDNEWMORT);
    fprintf(bgtime, "%lld\n", (long long)g->BOARDNEWDUO);
    fprintf(bgtime, "%lld\n", (long long)g->BOARDNEWCOD);
    fprintf(bgtime, "%lld\n", (long long)g->BOARDNEWBUI);
    fprintf(bgtime, "%lld\n", (long long)g->BOARDNEWIMM);
    fprintf(bgtime, "%lld\n", (long long)g->INTERESTTIME);
    fprintf(bgtime, "%lld\n", (long long)g->LASTINTEREST);
    fprintf(bgtime, "%d\n", g->HIGHPCOUNT);
    fprintf(bgtime, "%lld\n", (long long)g->PCOUNTDATE);
    fprintf(bgtime, "%d\n", g->WISHTIME);
    fprintf(bgtime, "%d\n", g->PCOUNT);
    fprintf(bgtime, "%lld\n", (long long)g->LASTPAYOUT);
    fprintf(bgtime, "%d\n", g->LASTPAYTYPE);
    fprintf(bgtime, "%d\n", g->LASTNEWS);
    fprintf(bgtime, "%d\n", g->dballtime);
    fprintf(bgtime, "%d\n", g->SELFISHMETER);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON1);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON2);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON3);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON4);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON5);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON6);
    fprintf(bgtime, "%d\n", g->SHADOW_DRAGON7);
    fprintf(bgtime, "%d\n", g->ERAPLAYERS);
    atomic_commit(&af);
  }
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code.
 */
[[noreturn]] static void help_file_eof(void)
{
  mud_log("SYSERR: Unexpected end of help file.");
  exit(1); /* Some day we hope to handle these things better... */
}

static int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
        help_file_eof();
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      help_file_eof();
  }

  return (total_keywords);
}

/* function to count how many hash-mark delimited records exist in a file */
static int count_hash_records(FILE *fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}

/* The next name in an index file. A file that ends without its "$" ends here anyway. */
static void read_index_entry(FILE *index, char *entry)
{
  if (fscanf(index, "%64935s\n", entry) != 1) { /* MAX_STRING_LENGTH */
    mud_log("SYSERR: index file ends without a '$'");
    strcpy(entry, "$");
  }
}

void index_boot(GameWorld *g, int mode)
{
  const char *index_filename, *prefix = nullptr; /* NULL or egcs 1.1 complains */
  FILE *db_index, *db_file;
  int rec_count = 0, size[2];
  char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];

  switch (mode) {
    case DB_BOOT_WLD:
      prefix = WLD_PREFIX;
      break;
    case DB_BOOT_MOB:
      prefix = MOB_PREFIX;
      break;
    case DB_BOOT_OBJ:
      prefix = OBJ_PREFIX;
      break;
    case DB_BOOT_ZON:
      prefix = ZON_PREFIX;
      break;
    case DB_BOOT_SHP:
      prefix = SHP_PREFIX;
      break;
    case DB_BOOT_HLP:
      prefix = HLP_PREFIX;
      break;
    case DB_BOOT_TRG:
      prefix = TRG_PREFIX;
      break;
    case DB_BOOT_GLD:
      prefix = GLD_PREFIX;
      break;
    default:
      mud_log("SYSERR: Unknown subcommand %d to index_boot!", mode);
      exit(1);
  }

  if (g->mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r"))) {
    mud_log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  read_index_entry(db_index, buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      mud_log("SYSERR: File '%s' listed in '%s%s': %s", buf2, prefix,
              index_filename, strerror(errno));
      read_index_entry(db_index, buf1);
      continue;
    }
    if (mode == DB_BOOT_ZON)
      rec_count++;
    else if (mode == DB_BOOT_HLP)
      rec_count += count_alias_records(db_file);
    else
      rec_count += count_hash_records(db_file);

    fclose(db_file);
    read_index_entry(db_index, buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count) {
    if (mode == DB_BOOT_SHP || mode == DB_BOOT_GLD)
      return;
    mud_log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
            index_filename);
    exit(1);
  }

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
    case DB_BOOT_TRG:
      break;
    case DB_BOOT_WLD:
      g->rooms = new room_data[rec_count]();
      size[0] = sizeof(struct room_data) * rec_count;
      mud_log("   %d rooms, %d bytes.", rec_count, size[0]);
      break;
    case DB_BOOT_MOB:
      g->mob_proto = new char_data[rec_count]();
      g->mob_index = new index_data[rec_count]();
      size[0] = sizeof(struct index_data) * rec_count;
      size[1] = sizeof(struct char_data) * rec_count;
      mud_log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
      break;
    case DB_BOOT_OBJ:
      size[0] = sizeof(struct index_data) * rec_count;
      size[1] = sizeof(struct obj_data) * rec_count;
      mud_log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
      break;
    case DB_BOOT_ZON:
      size[0] = sizeof(struct zone_data) * rec_count;
      mud_log("   %d zones, %d bytes.", rec_count, size[0]);
      break;
    case DB_BOOT_HLP:
      g->help_table = new help_index_element[rec_count]();
      size[0] = sizeof(struct help_index_element) * rec_count;
      mud_log("   %d entries, %d bytes.", rec_count, size[0]);
      break;
  }

  rewind(db_index);
  read_index_entry(db_index, buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      mud_log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    switch (mode) {
      case DB_BOOT_WLD:
      case DB_BOOT_OBJ:
      case DB_BOOT_MOB:
      case DB_BOOT_TRG:
        discrete_load(g, db_file, mode, buf2);
        break;
      case DB_BOOT_ZON:
        load_zones(g, db_file, buf2);
        break;
      case DB_BOOT_HLP:
        load_help(g, db_file, buf2);
        break;
      case DB_BOOT_SHP:
        boot_the_shops(g, db_file, buf2, rec_count);
        break;
      case DB_BOOT_GLD:
        boot_the_guilds(g, db_file, buf2, rec_count);
        break;
    }

    fclose(db_file);
    read_index_entry(db_index, buf1);
  }
  fclose(db_index);

  /* Sort the help index. */
  if (mode == DB_BOOT_HLP) {
    qsort(g->help_table, g->top_of_helpt, sizeof(struct help_index_element), hsort);
    g->top_of_helpt--;
  }
}

static void discrete_load(GameWorld *g, FILE *fl, int mode, char *filename)
{
  IDXTYPE nr = -1, last;
  char line[READ_SIZE];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.hpp */

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
        if (nr == -1) {
          mud_log("SYSERR: %s file %s is empty!", modes[mode], filename);
        } else {
          mud_log("SYSERR: Format error in %s after %s #%d\n"
                  "...expecting a new %s, but file ended!\n"
                  "(maybe the file is not terminated with '$'?)",
                  filename,
                  modes[mode], nr, modes[mode]);
        }
        exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
        mud_log("SYSERR: Format error after %s #%d", modes[mode], last);
        exit(1);
      }
      if (nr >= 99999)
        return;
      else
        switch (mode) {
          case DB_BOOT_WLD:
            parse_room(g, fl, nr);
            break;
          case DB_BOOT_MOB:
            parse_mobile(g, fl, nr);
            break;
          case DB_BOOT_TRG:
            parse_trigger(g, fl, nr);
            break;
          case DB_BOOT_OBJ:
            strlcpy(line, parse_object(g, fl, nr), sizeof(line));
            break;
        }
    } else {
      mud_log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
              filename, modes[mode], nr);
      mud_log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}

static char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_num = true, warned = false;
  char *p;

  for (p = flag; *p; p++) {
    unsigned char c = (unsigned char)*p;

    if (islower(c))
      flags |= 1U << (c - 'a');
    else if (c >= 'A' && c <= 'F')
      flags |= 1U << (26 + (c - 'A'));
    else if (isupper(c) && !warned) {
      /* Only a-z and A-F fit in a 32-bit bitvector_t; see sprintascii(). */
      mud_log("SYSERR: asciiflag_conv: ignoring out-of-range flag letter '%c' in \"%s\".", c, flag);
      warned = true;
    }

    if (!(isdigit(c) || (c == '-')))
      is_num = false;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

static bitvector_t asciiflag_conv_aff(char *flag)
{
  bitvector_t flags = 0;
  int is_num = true, warned = false;
  char *p;

  for (p = flag; *p; p++) {
    unsigned char c = (unsigned char)*p;

    if (islower(c))
      flags |= 1U << (1 + (c - 'a'));
    else if (c >= 'A' && c <= 'F')
      flags |= 1U << (26 + (c - 'A'));
    else if (isupper(c) && !warned) {
      /* Only a-z and A-F fit in a 32-bit bitvector_t; see sprintascii(). */
      mud_log("SYSERR: asciiflag_conv_aff: ignoring out-of-range flag letter '%c' in \"%s\".", c, flag);
      warned = true;
    }

    if (!(isdigit(c) || (c == '-')))
      is_num = false;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}
/* load the rooms */
static void parse_room(GameWorld *g, FILE *fl, room_vnum virtual_nr)
{
  int t[10], i, retval;
  char line[READ_SIZE], flags[128], flags2[128], flags3[128];
  char flags4[128], buf2[MAX_STRING_LENGTH], buf[128];
  struct extra_descr_data *new_descr;
  char letter;

  /* This really had better fit or there are other problems. */
  snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

  g->parse_room_zone = find_zone_by_thing(g, virtual_nr);
  if (g->parse_room_zone == nullptr) {
    mud_log("SYSERR: Room #%d is outside of any zone.", virtual_nr);
    exit(1);
  }

  rroom_set_zone(g, g->parse_room_index, g->parse_room_zone);
  rroom_set_number(g, g->parse_room_index, virtual_nr);
  rroom_set_name(g, g->parse_room_index, fread_string(fl, buf2));
  rroom_set_description(g, g->parse_room_index, fread_string(fl, buf2));

  if (!get_line(fl, line)) {
    mud_log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
            virtual_nr);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %127s %127s %127s %127s %d ", t, flags, flags2, flags3, flags4, t + 2)) == 3) && (bitwarning == true)) {
    mud_log("WARNING: Conventional worldfiles detected. Please read 128bit.readme.");
    exit(1);
  } else if ((retval == 3) && (bitwarning == false)) {
    /* 
   * Looks like the implementor is ready, so let's load the worldfiles. We 
   * load the extra three flags as 0, since they won't be anything anyway. We 
   * will save the entire world later on, when every room, mobile, and object 
   * is converted.
   */

    mud_log("Converting room #%d to 128bits..", virtual_nr);
    rroom_get_room_flags(g, g->parse_room_index)[0] = asciiflag_conv(flags);
    rroom_get_room_flags(g, g->parse_room_index)[1] = 0;
    rroom_get_room_flags(g, g->parse_room_index)[2] = 0;
    rroom_get_room_flags(g, g->parse_room_index)[3] = 0;

    sprintf(flags, "room #%d", virtual_nr); /* sprintf: OK (until 399-bit integers) */

    if (bitsavetodisk) { /* Maybe the implementor just wants to look at the 128bit files */
      add_to_save_list(g, find_zone_by_thing(g, virtual_nr)->number, 3);
      g->converting = true;
    }

    mud_log("   done.");
  } else if (retval == 6) {
    rroom_get_room_flags(g, g->parse_room_index)[0] = asciiflag_conv(flags);
    rroom_get_room_flags(g, g->parse_room_index)[1] = asciiflag_conv(flags2);
    rroom_get_room_flags(g, g->parse_room_index)[2] = asciiflag_conv(flags3);
    rroom_get_room_flags(g, g->parse_room_index)[3] = asciiflag_conv(flags4);

    rroom_set_sector_type(g, g->parse_room_index, t[2]);
    sprintf(flags, "object #%d", virtual_nr); /* sprintf: OK (until 399-bit integers) */

  } else {
    mud_log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
    exit(1);
  }

  rroom_set_func(g, g->parse_room_index, nullptr);
  rroom_set_contents(g, g->parse_room_index, nullptr);
  rroom_set_people(g, g->parse_room_index, nullptr);
  rroom_set_light(g, g->parse_room_index, 0); /* Zero light sources */
  rroom_set_timed(g, g->parse_room_index, -1);
  rroom_set_dmg(g, g->parse_room_index, 0);
  rroom_set_gravity(g, g->parse_room_index, 0);

  if (rroom_flagged(g, g->parse_room_index, ROOM_VEGETA) || rroom_flagged(g, g->parse_room_index, ROOM_GRAVITYX10)) {
    rroom_set_gravity(g, g->parse_room_index, 10);
  }
  if (virtual_nr >= 19800 && virtual_nr <= 19899) {
    rroom_set_gravity(g, g->parse_room_index, 1000);
  }
  if (virtual_nr >= 64000 && virtual_nr <= 64006) {
    rroom_set_gravity(g, g->parse_room_index, 100);
  }
  if (virtual_nr >= 64007 && virtual_nr <= 64016) {
    rroom_set_gravity(g, g->parse_room_index, 300);
  }
  if (virtual_nr >= 64017 && virtual_nr <= 64030) {
    rroom_set_gravity(g, g->parse_room_index, 500);
  }
  if (virtual_nr >= 64031 && virtual_nr <= 64048) {
    rroom_set_gravity(g, g->parse_room_index, 1000);
  }
  if (virtual_nr >= 64049 && virtual_nr <= 64070) {
    rroom_set_gravity(g, g->parse_room_index, 5000);
  }
  if (virtual_nr >= 64071 && virtual_nr <= 64096) {
    rroom_set_gravity(g, g->parse_room_index, 10000);
  }
  if (virtual_nr == 64097) {
    rroom_set_gravity(g, g->parse_room_index, 1000);
  }
  for (i = 0; i < NUM_OF_DIRS; i++)
    rroom_set_dir_option(g, g->parse_room_index, i, nullptr);

  rroom_set_ex_description(g, g->parse_room_index, nullptr);

  snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      mud_log("%s", buf);
      exit(1);
    }
    switch (*line) {
      case 'D':
        setup_dir(g, fl, g->parse_room_index, atoi(line + 1));
        break;
      case 'E':
        new_descr              = new extra_descr_data();
        new_descr->keyword     = fread_string(fl, buf2);
        new_descr->description = fread_string(fl, buf2);
        /* fix for crashes in the editor when formatting 
       * - e-descs are assumed to end with a \r\n
       * -- Welcor 09/03 
       */
        {
          char *tmp = strchr(new_descr->description, '\0');
          if (tmp > new_descr->description && *(tmp - 1) != '\n') {
            tmp = str_alloc(strlen(new_descr->description) + 3);
            sprintf(tmp, "%s\r\n", new_descr->description); /* sprintf ok : size checked above*/
            free(new_descr->description);
            new_descr->description = tmp;
          }
        }
        new_descr->next               = rroom_get_ex_description(g, g->parse_room_index);
        rroom_set_ex_description(g, g->parse_room_index, new_descr);
        break;
      case 'S': /* end of room */
        /* DG triggers -- script is defined after the end of the room */
        letter = fread_letter(fl);
        ungetc(letter, fl);
        while (letter == 'T') {
          dg_read_trigger(g, fl, &g->rooms[g->parse_room_index], WLD_TRIGGER);
          letter = fread_letter(fl);
          ungetc(letter, fl);
        }
        g->top_room = g->parse_room_index++;
        return;
      default:
        mud_log("%s", buf);
        exit(1);
    }
  }
}

/* read direction data */
static void setup_dir(GameWorld *g, FILE *fl, room_rnum room, int dir)
{
  int t[11], retval;
  char line[READ_SIZE], buf2[128];

  snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", rroom_get_vnum(g, room) + 1, dir);

  struct room_direction_data *new_exit;
  new_exit = new room_direction_data();
  rroom_set_dir_option(g, room, dir, new_exit);
  rroom_get_dir_option(g, room, dir)->general_description = fread_string(fl, buf2);
  rroom_get_dir_option(g, room, dir)->keyword             = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    mud_log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (((retval = sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9, t + 10)) == 3) && (bitwarning == true)) {
    mud_log("SYSERR: Format error, %s", buf2);
    exit(1);
  } else if (bitwarning == false) {

    if (t[0] == 1)
      rroom_get_dir_option(g, room, dir)->exit_info = EX_ISDOOR;
    else if (t[0] == 2)
      rroom_get_dir_option(g, room, dir)->exit_info = EX_ISDOOR | EX_PICKPROOF;
    else if (t[0] == 3)
      rroom_get_dir_option(g, room, dir)->exit_info = EX_ISDOOR | EX_SECRET;
    else if (t[0] == 4)
      rroom_get_dir_option(g, room, dir)->exit_info = EX_ISDOOR | EX_PICKPROOF | EX_SECRET;
    else
      rroom_get_dir_option(g, room, dir)->exit_info = 0;

    rroom_get_dir_option(g, room, dir)->key     = ((t[1] == -1 || t[1] == 65535) ? NOTHING : t[1]);
    rroom_get_dir_option(g, room, dir)->to_room = ((t[2] == -1 || t[2] == 65535) ? NOWHERE : t[2]);

    if (retval == 3) {
      mud_log("Converting world files to include DC add ons.");
      rroom_get_dir_option(g, room, dir)->dclock        = 20;
      rroom_get_dir_option(g, room, dir)->dchide        = 20;
      rroom_get_dir_option(g, room, dir)->dcskill       = 0;
      rroom_get_dir_option(g, room, dir)->dcmove        = 0;
      rroom_get_dir_option(g, room, dir)->failsavetype  = 0;
      rroom_get_dir_option(g, room, dir)->dcfailsave    = 0;
      rroom_get_dir_option(g, room, dir)->failroom      = NOWHERE;
      rroom_get_dir_option(g, room, dir)->totalfailroom = NOWHERE;
      if (bitsavetodisk) {
        add_to_save_list(g, rroom_get_zone(g, room)->number, 3);
        g->converting = true;
      }
    } else if (retval == 5) {
      rroom_get_dir_option(g, room, dir)->dclock        = t[3];
      rroom_get_dir_option(g, room, dir)->dchide        = t[4];
      rroom_get_dir_option(g, room, dir)->dcskill       = 0;
      rroom_get_dir_option(g, room, dir)->dcmove        = 0;
      rroom_get_dir_option(g, room, dir)->failsavetype  = 0;
      rroom_get_dir_option(g, room, dir)->dcfailsave    = 0;
      rroom_get_dir_option(g, room, dir)->failroom      = NOWHERE;
      rroom_get_dir_option(g, room, dir)->totalfailroom = NOWHERE;
      if (bitsavetodisk) {
        add_to_save_list(g, rroom_get_zone(g, room)->number, 3);
        g->converting = true;
      }
    } else if (retval == 7) {
      rroom_get_dir_option(g, room, dir)->dclock        = t[3];
      rroom_get_dir_option(g, room, dir)->dchide        = t[4];
      rroom_get_dir_option(g, room, dir)->dcskill       = t[5];
      rroom_get_dir_option(g, room, dir)->dcmove        = t[6];
      rroom_get_dir_option(g, room, dir)->failsavetype  = 0;
      rroom_get_dir_option(g, room, dir)->dcfailsave    = 0;
      rroom_get_dir_option(g, room, dir)->failroom      = NOWHERE;
      rroom_get_dir_option(g, room, dir)->totalfailroom = NOWHERE;
      if (bitsavetodisk) {
        add_to_save_list(g, rroom_get_zone(g, room)->number, 3);
        g->converting = true;
      }
    } else if (retval == 11) {
      rroom_get_dir_option(g, room, dir)->dclock        = t[3];
      rroom_get_dir_option(g, room, dir)->dchide        = t[4];
      rroom_get_dir_option(g, room, dir)->dcskill       = t[5];
      rroom_get_dir_option(g, room, dir)->dcmove        = t[6];
      rroom_get_dir_option(g, room, dir)->failsavetype  = t[7];
      rroom_get_dir_option(g, room, dir)->dcfailsave    = t[8];
      rroom_get_dir_option(g, room, dir)->failroom      = t[9];
      rroom_get_dir_option(g, room, dir)->totalfailroom = t[10];
    }
  }
}

/* make sure the start rooms exist & resolve their vnums to rnums */
static void check_start_rooms(GameWorld *g)
{
  if ((g->r_mortal_start_room = real_room(g, CONFIG_MORTAL_START)) == NOWHERE) {
    mud_log("SYSERR:  Mortal start room does not exist.  Change mortal_start_room in lib/etc/config.");
    g->r_mortal_start_room = 0; /* The Void */
  }
  if ((g->r_immort_start_room = real_room(g, CONFIG_IMMORTAL_START)) == NOWHERE) {
    if (!g->mini_mud)
      mud_log("SYSERR:  Warning: Immort start room does not exist.  Change immort_start_room in /lib/etc/config.");
    g->r_immort_start_room = g->r_mortal_start_room;
  }
  if ((g->r_frozen_start_room = real_room(g, CONFIG_FROZEN_START)) == NOWHERE) {
    if (!g->mini_mud)
      mud_log("SYSERR:  Warning: Frozen start room does not exist.  Change frozen_start_room in /lib/etc/config.");
    g->r_frozen_start_room = g->r_mortal_start_room;
  }
}

/* resolve all vnums into rnums in the world */
static void renum_world(GameWorld *g)
{
  room_rnum room;
  int door;

  for (room = 0; room <= g->top_room; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (rroom_get_dir_option(g, room, door))
        if (rroom_get_dir_option(g, room, door)->to_room != NOWHERE)
          rroom_get_dir_option(g, room, door)->to_room =
              real_room(g, rroom_get_dir_option(g, room, door)->to_room);
}

#define ZCMD2 zone->cmd[cmd_no]

/*
 * "resulve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_vnum))
 */
static void renum_zone_table(GameWorld *g)
{
  int cmd_no;
  room_rnum a, b, c, olda, oldb, oldc;
  char buf[128];

  for (struct zone_data *zone : g->zones.all())
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda      = ZCMD2.arg1;
      oldb      = ZCMD2.arg2;
      oldc      = ZCMD2.arg3;
      switch (ZCMD2.command) {
        case 'M':
          a = ZCMD2.arg1 = real_mobile(g, ZCMD2.arg1);
          c = ZCMD2.arg3 = real_room(g, ZCMD2.arg3);
          break;
        case 'O':
          a = ZCMD2.arg1 = real_object(g, ZCMD2.arg1);
          if (ZCMD2.arg3 != NOWHERE)
            c = ZCMD2.arg3 = real_room(g, ZCMD2.arg3);
          break;
        case 'G':
          a = ZCMD2.arg1 = real_object(g, ZCMD2.arg1);
          break;
        case 'E':
          a = ZCMD2.arg1 = real_object(g, ZCMD2.arg1);
          break;
        case 'P':
          a = ZCMD2.arg1 = real_object(g, ZCMD2.arg1);
          c = ZCMD2.arg3 = real_object(g, ZCMD2.arg3);
          break;
        case 'D':
          a = ZCMD2.arg1 = real_room(g, ZCMD2.arg1);
          break;
        case 'R': /* rem obj from room */
          a = ZCMD2.arg1 = real_room(g, ZCMD2.arg1);
          b = ZCMD2.arg2 = real_object(g, ZCMD2.arg2);
          break;
        case 'T': /* a trigger: it keeps the vnum, so check that there is one */
          b = ZCMD2.arg2 = find_trigger(g, ZCMD2.arg2) ? ZCMD2.arg2 : NOTHING;
          c = ZCMD2.arg3 = real_room(g, ZCMD2.arg3);
          break;
        case 'V': /* trigger variable assignment */
          b = ZCMD2.arg3 = real_room(g, ZCMD2.arg3);
          break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
        if (!g->mini_mud) {
          snprintf(buf, sizeof(buf), "Invalid vnum %d, cmd disabled",
                   a == NOWHERE ? olda : b == NOWHERE ? oldb
                                                      : oldc);
          log_zone_error(g, zone, cmd_no, buf);
        }
        ZCMD2.command = '*';
      }
    }
}

static void mob_autobalance(struct char_data *ch)
{
  /* Try to add some baseline defaults based on level choice. */
  //GET_HIT(ch) = 0;
  //GET_MANA(ch) = 0;
  //GET_MOVE(ch) = 0;
  char_set_exp(ch, 0);
  char_set_armor(ch, 0);
  char_set_mob_specials_damnodice(ch, 0);
  char_set_mob_specials_damsizedice(ch, 0);
}

static bool parse_simple_mob(GameWorld *g, FILE *mob_f, struct char_data *ch, mob_vnum nr)
{
  int t[10];
  char line[READ_SIZE];

  char_set_real_abils_str(ch, 0);
  char_set_real_abils_intel(ch, 0);
  char_set_real_abils_wis(ch, 0);
  char_set_real_abils_dex(ch, 0);
  char_set_real_abils_con(ch, 0);
  char_set_real_abils_cha(ch, 0);

  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    return false;
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
             t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    mud_log("SYSERR: Format error in mob #%d, first line after S flag\n"
            "...expecting line of form '# # # #d#+# #d#+#'",
            nr);
    return false;
  }

  char_set_race_level(ch, t[0]);
  char_set_level_adj(ch, 0);
  char_set_class_level(ch, 0);
  char_set_armor(ch, 10 * (10 - t[2]));

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  char_set_basepl(ch, t[3]);
  char_set_baseki(ch, t[4]);
  char_set_basest(ch, t[5]);
  char_set_health(ch, 1.0);
  char_set_energy(ch, 1.0);
  char_set_stamina(ch, 1.0);

  char_set_mob_specials_damnodice(ch, t[6]);
  char_set_mob_specials_damsizedice(ch, t[7]);
  /* t[8] (damage mod) is still parsed to keep the mob file format, but unused */

  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
            "...expecting line of form '# #', but file ended!",
            nr);
    return false;
  }

  if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
            "...expecting line of form '# # # #'",
            nr);
    return false;
  }

  char_set_gold(ch, t[0]);
  char_set_exp(ch, 0);
  char_set_race(ch, t[2]);

  char_set_class(ch, t[3]);
  if (char_get_class(ch) < 0 || char_get_class(ch) >= NUM_CLASSES) {
    mud_log("SYSERR: Invalid class %d for mob #%d", char_get_class(ch), nr);
    char_set_class(ch, 28); /* set to commoner */
  }

  /* GET_CLASS_RANKS(ch, t[3]) = GET_LEVEL(ch); */

  if (!char_is_human(ch))
    if (!char_aff_flagged(ch, AFF_INFRAVISION))
      char_aff_set(ch, AFF_INFRAVISION, true);

  char_set_speaking(ch, SKILL_LANG_COMMON);

  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error in last line of mob #%d\n"
            "...expecting line of form '# # #', but file ended!",
            nr);
    return false;
  }

  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    mud_log("SYSERR: Format error in last line of mob #%d\n"
            "...expecting line of form '# # #'",
            nr);
    return false;
  }

  char_set_position(ch, t[0]);
  char_set_mob_specials_default_pos(ch, t[1]);
  char_set_sex(ch, t[2]);

  char_set_speaking(ch, MIN_LANGUAGES);
  set_height_and_weight_by_race(g, ch);

  if (char_mob_flagged(ch, MOB_AUTOBALANCE)) {
    mob_autobalance(ch);
  }

  return true;
}

/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 *
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value.
 */

#define CASE(test) \
  if (value && !matched && !strcasecmp(keyword, test) && (matched = true))

#define BOOL_CASE(test) \
  if (!value && !matched && !strcasecmp(keyword, test) && (matched = true))

#define RANGE(low, high) \
  (num_arg = MAX((low), MIN((high), (num_arg))))

static void interpret_espec(GameWorld *g, const char *keyword, const char *value, struct char_data *ch, mob_vnum nr)
{
  int num_arg = 0, matched = false;
  int num, num2, num3, num4, num5, num6;
  struct affected_type af = {};

  /*
   * If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some.
  */
  if (value)
    num_arg = atoi(value);

  CASE("BareHandAttack")
  {
    RANGE(0, 99);
    char_set_mob_specials_attack_type(ch, num_arg);
  }

  CASE("Size")
  {
    RANGE(SIZE_UNDEFINED, NUM_SIZES - 1);
    char_set_size(ch, num_arg);
  }

  CASE("Str")
  {
    RANGE(0, 200);
    char_set_real_abils_str(ch, num_arg);
  }

  CASE("StrAdd")
  {
    mud_log("mob #%d trying to set StrAdd, rebalance its strength.",
            char_get_mob_vnum(g, ch));
  }

  CASE("Int")
  {
    RANGE(0, 200);
    char_set_real_abils_intel(ch, num_arg);
  }

  CASE("Wis")
  {
    RANGE(0, 200);
    char_set_real_abils_wis(ch, num_arg);
  }

  CASE("Dex")
  {
    RANGE(0, 200);
    char_set_real_abils_dex(ch, num_arg);
  }

  CASE("Con")
  {
    RANGE(0, 200);
    char_set_real_abils_con(ch, num_arg);
  }

  CASE("Cha")
  {
    RANGE(0, 200);
    char_set_real_abils_cha(ch, num_arg);
  }

  CASE("Hit")
  {
    RANGE(0, 99999);
    //GET_HIT(ch) = num_arg;
  }

  CASE("MaxHit")
  {
    RANGE(0, 99999);
    char_set_max_hit(ch, num_arg);
  }

  CASE("Mana")
  {
    RANGE(0, 99999);
    //GET_MANA(ch) = num_arg;
  }

  CASE("MaxMana")
  {
    RANGE(0, 99999);
    char_set_max_mana(ch, num_arg);
  }

  CASE("Moves")
  {
    RANGE(0, 99999);
    //GET_MOVE(ch) = num_arg;
  }

  CASE("MaxMoves")
  {
    RANGE(0, 99999);
    char_set_max_move(ch, num_arg);
  }

  CASE("Affect")
  {
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(value, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num > 0) {
      af.type      = num;
      af.duration  = num2;
      af.modifier  = num3;
      af.location  = num4;
      af.bitvector = num5;
      af.specific  = affect_checked_specific(num4, num6);
      if (affect_is_loadable(&af))
        affect_to_char(g, ch, &af);
      else
        mud_log("SYSERR: mob %s has an impossible affect: %s", char_get_name(ch), value);
    }
  }

  CASE("AffectV")
  {
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(value, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num > 0) {
      af.type      = num;
      af.duration  = num2;
      af.modifier  = num3;
      af.location  = num4;
      af.bitvector = num5;
      af.specific  = affect_checked_specific(num4, num6);
    }
  }

  CASE("Feat")
  {
    sscanf(value, "%d %d", &num, &num2);
  }

  /* NPCs have no skill table; old mob files may still carry these lines, so accept and ignore them. */
  CASE("Skill") {}

  CASE("SkillMod") {}

  CASE("Class")
  {
    sscanf(value, "%d %d", &num, &num2);
  }

  CASE("EpicClass")
  {
    sscanf(value, "%d %d", &num, &num2);
  }

  if (!matched) {
    mud_log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
            keyword, nr);
  }
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

static void parse_espec(GameWorld *g, char *buf, struct char_data *ch, mob_vnum nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != nullptr) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }
  interpret_espec(g, buf, ptr, ch, nr);
}

static bool parse_enhanced_mob(GameWorld *g, FILE *mob_f, struct char_data *ch, mob_vnum nr)
{
  char line[READ_SIZE];

  parse_simple_mob(g, mob_f, ch, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E")) /* end of the enhanced section */
      return true;
    else if (*line == '#') { /* we've hit the next mob, maybe? */
      mud_log("SYSERR: Unterminated E section in mob #%d", nr);
      return false;
    } else
      parse_espec(g, line, ch, nr);
  }

  mud_log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  return false;
}

static bool parse_mobile_from_file(GameWorld *g, FILE *mob_f, struct char_data *ch)
{
  int j, t[10], retval;
  char line[READ_SIZE], *tmpptr, letter;
  char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128];
  char f7[128], f8[128], buf2[128];
  mob_vnum nr = g->mob_index[char_get_nr(ch)].vnum;

  /* Mobiles have no player_specials: that NULL is what makes a character an NPC (char_is_npc). */
  char_set_player_specials(ch, nullptr);
  sprintf(buf2, "mob vnum %d", nr); /* sprintf: OK (for 'buf2 >= 19') */

  /***** String data *****/
  char_set_alias(ch, fread_string(mob_f, buf2));
  tmpptr = fread_string(mob_f, buf2);
  char_set_sdesc(ch, tmpptr);
  if (tmpptr && *tmpptr)
    if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
        !strcasecmp(fname(tmpptr), "the"))
      *tmpptr = tolower(*tmpptr);
  char_set_ldesc(ch, fread_string(mob_f, buf2));
  char_set_ddesc(ch, fread_string(mob_f, buf2));

  /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error after string section of mob #%d\n"
            "...expecting line of form '# # # {S | E}', but file ended!",
            nr);
    return false;
  }

  if (((retval = sscanf(line, "%127s %127s %127s %127s %127s %127s %127s %127s %d %c", f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter)) == 10) && (bitwarning == true)) {
    /* Let's make the implementor read some, before converting his worldfiles */
    mud_log("WARNING: Conventional mobilefiles detected. Please read 128bit.readme.");
    return false;
  }
  if ((retval == 4) && (bitwarning == false)) {

    mud_log("Converting mobile #%d to 128bits..", nr);
    char_get_act(ch)[0] = asciiflag_conv(f1);
    char_get_act(ch)[1] = 0;
    char_get_act(ch)[2] = 0;
    char_get_act(ch)[3] = 0;

    char_get_affected_by(ch)[0] = asciiflag_conv_aff(f2);
    char_get_affected_by(ch)[1] = 0;
    char_get_affected_by(ch)[2] = 0;
    char_get_affected_by(ch)[3] = 0;

    char_set_alignment(ch, atoi(f3));

    /* Make some basic checks. */
    char_aff_set(ch, AFF_CHARM, false);
    char_aff_set(ch, AFF_POISON, false);
    char_aff_set(ch, AFF_GROUP, false);
    char_aff_set(ch, AFF_SLEEP, false);
    if (char_mob_flagged(ch, MOB_AGGRESSIVE) && char_mob_flagged(ch, MOB_AGGR_GOOD))
      char_mob_set(ch, MOB_AGGR_GOOD, false);
    if (char_mob_flagged(ch, MOB_AGGRESSIVE) && char_mob_flagged(ch, MOB_AGGR_NEUTRAL))
      char_mob_set(ch, MOB_AGGR_NEUTRAL, false);
    if (char_mob_flagged(ch, MOB_AGGRESSIVE) && char_mob_flagged(ch, MOB_AGGR_EVIL))
      char_mob_set(ch, MOB_AGGR_EVIL, false);

    /* 
     * This is necessary, since if we have conventional worldfiles, &letter
     * is loaded into f4 instead of the letter characters. So what we do, is 
     * copy f4 into letter. Disadvantage is that &letter cannot be longer
     * then 128 characters, but this shouldn't occur anyway.
     */
    letter = *f4;

    if (bitsavetodisk) {
      add_to_save_list(g, find_zone_by_thing(g, nr)->number, 0);
      g->converting = true;
    }

    mud_log("   done.");
  } else if (retval == 10) {

    char_get_act(ch)[0] = asciiflag_conv(f1);
    char_get_act(ch)[1] = asciiflag_conv(f2);
    char_get_act(ch)[2] = asciiflag_conv(f3);
    char_get_act(ch)[3] = asciiflag_conv(f4);

    char_get_affected_by(ch)[0] = asciiflag_conv(f5);
    char_get_affected_by(ch)[1] = asciiflag_conv(f6);
    char_get_affected_by(ch)[2] = asciiflag_conv(f7);
    char_get_affected_by(ch)[3] = asciiflag_conv(f8);

    char_set_alignment(ch, t[2]);

  } else {
    mud_log("SYSERR: Format error after string section of mob #%d\n"
            "...expecting line of form '# # # {S | E}'",
            nr);
    exit(1);
  }

  char_mob_set(ch, MOB_ISNPC, true);
  if (char_mob_flagged(ch, MOB_NOTDEADYET)) {
    /* Rather bad to load mobiles with this bit already set. */
    mud_log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
    char_mob_set(ch, MOB_NOTDEADYET, false);
  }

  /* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE.
  if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL))
    log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", nr); */

  /* Convert mobs to use AUTOBALANCE. Uncomment and reboot to flag all mobs AUTOBALANCE.
   * if (!MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
   *   SET_BIT_AR(MOB_FLAGS(ch), MOB_AUTOBALANCE);
   * } */

  switch (toupper(letter)) {
    case 'S': /* Simple monsters */
      parse_simple_mob(g, mob_f, ch, nr);
      break;
    case 'E': /* Circle3 Enhanced monsters */
      parse_enhanced_mob(g, mob_f, ch, nr);
      mob_stats(g, ch);
      break;
    /* add new mob types here.. */
    default:
      mud_log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
      exit(1);
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter == 'T') {
    dg_read_trigger(g, mob_f, ch, MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  char_reset_aff_abils(ch);

  for (j = 0; j < NUM_WEARS; j++)
    char_set_equipment(ch, j, nullptr);

  /* Uncomment to force all mob files to be rewritten. Good for initial AUTOBALANCE setup.
   * if (bitsavetodisk) {
   *   add_to_save_list(zone_table[find_zone_by_thing(nr)].number, 0);
   *   converting = TRUE;
   * } */

  return true;
}

static void parse_mobile(GameWorld *g, FILE *mob_f, mob_vnum nr)
{

  g->mob_index[g->parse_mob_index].vnum   = nr;
  g->mob_index[g->parse_mob_index].number = 0;
  g->mob_index[g->parse_mob_index].func   = nullptr;

  clear_char(g->mob_proto + g->parse_mob_index);

  char_set_nr(&g->mob_proto[g->parse_mob_index], g->parse_mob_index);
  char_set_desc(&g->mob_proto[g->parse_mob_index], nullptr);

  if (parse_mobile_from_file(g, mob_f, g->mob_proto + g->parse_mob_index)) {
    g->top_mob = g->parse_mob_index++;
  } else { /* We used to exit in the file reading code, but now we do it here */
    exit(1);
  }
}

/* read all objects from obj file; generate index and prototypes */
static char *parse_object(GameWorld *g, FILE *obj_f, IDXTYPE nr)
{
  static char line[READ_SIZE];
  int t[NUM_OBJ_VAL_POSITIONS + 2], j, retval;
  char *tmpptr, buf2[128];
  char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
  char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
  char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];
  struct extra_descr_data *new_descr;

  if (g->obj_index.find(nr)) {
    mud_log("SYSERR: object #%d is defined twice.", nr);
    exit(1);
  }
  g->parse_obj_index = nr;
  g->obj_index.emplace(nr);
  g->obj_proto.emplace(nr);
  obj_index_of(g, g->parse_obj_index)->vnum   = nr;
  obj_index_of(g, g->parse_obj_index)->number = 0;
  obj_index_of(g, g->parse_obj_index)->func   = nullptr;

  clear_object(obj_proto_of(g, g->parse_obj_index));
  obj_set_item_number(obj_proto_of(g, g->parse_obj_index), g->parse_obj_index);

  sprintf(buf2, "object #%d", nr); /* sprintf: OK (for 'buf2 >= 19') */

  /* *** string data *** */
  obj_set_name(obj_proto_of(g, g->parse_obj_index), fread_string(obj_f, buf2));
  if (obj_get_name(obj_proto_of(g, g->parse_obj_index)) == nullptr) {
    mud_log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = fread_string(obj_f, buf2);
  obj_set_short_description(obj_proto_of(g, g->parse_obj_index), tmpptr);
  if (tmpptr && *tmpptr)
    if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
        !strcasecmp(fname(tmpptr), "the"))
      *tmpptr = tolower(*tmpptr);

  tmpptr = fread_string(obj_f, buf2);
  obj_set_description(obj_proto_of(g, g->parse_obj_index), tmpptr);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_set_action_description(obj_proto_of(g, g->parse_obj_index), fread_string(obj_f, buf2));

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    mud_log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %s %s %s %s %s %s %s %s", t, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12)) == 4) && (bitwarning == true)) {
    /* Let's make the implementor read some, before converting his worldfiles */
    mud_log("WARNING: Conventional objectfiles detected. Please read 128bit.readme.");
    exit(1);
  } else if (((retval == 4) || (retval == 3)) && (bitwarning == false)) {

    if (retval == 3)
      t[3] = 0;
    else if (retval == 4)
      t[3] = asciiflag_conv_aff(f3);

    mud_log("Converting object #%d to 128bits..", nr);
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[0] = asciiflag_conv(f1);
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[1] = 0;
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[2] = 0;
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[3] = 0;
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[0]  = asciiflag_conv(f2);
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[1]  = 0;
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[2]  = 0;
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[3]  = 0;
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[0]   = asciiflag_conv_aff(f3);
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[1]   = 0;
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[2]   = 0;
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[3]   = 0;

    if (bitsavetodisk) {
      add_to_save_list(g, find_zone_by_thing(g, nr)->number, 1);
      g->converting = true;
    }

    mud_log("   done.");
  } else if (retval == 13) {

    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[0] = asciiflag_conv(f1);
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[1] = asciiflag_conv(f2);
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[2] = asciiflag_conv(f3);
    obj_get_extra_flags(obj_proto_of(g, g->parse_obj_index))[3] = asciiflag_conv(f4);

    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[0] = asciiflag_conv(f5);
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[1] = asciiflag_conv(f6);
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[2] = asciiflag_conv(f7);
    obj_get_wear_flags(obj_proto_of(g, g->parse_obj_index))[3] = asciiflag_conv(f8);

    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[0] = asciiflag_conv(f9);
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[1] = asciiflag_conv(f10);
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[2] = asciiflag_conv(f11);
    obj_get_bitvector(obj_proto_of(g, g->parse_obj_index))[3] = asciiflag_conv(f12);

  } else {
    mud_log("SYSERR: Format error in first numeric line (expecting 13 args, got %d), %s", retval, buf2);
    exit(1);
  }

  /* Object flags checked in check_object(). */
  obj_set_type_flag(obj_proto_of(g, g->parse_obj_index), t[0]);

  if (!get_line(obj_f, line)) {
    mud_log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    t[j] = 0;

  if ((retval = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9, t + 10, t + 11, t + 12, t + 13, t + 14, t + 15)) > NUM_OBJ_VAL_POSITIONS) {
    mud_log("SYSERR: Format error in second numeric line (expecting <=%d args, got %d), %s", NUM_OBJ_VAL_POSITIONS, retval, buf2);
    exit(1);
  }

  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    obj_set_value(obj_proto_of(g, g->parse_obj_index), j, t[j]);

  if ((obj_get_type_flag(obj_proto_of(g, g->parse_obj_index)) == ITEM_PORTAL ||
       obj_get_type_flag(obj_proto_of(g, g->parse_obj_index)) == ITEM_HATCH) &&
      (!obj_get_value(obj_proto_of(g, g->parse_obj_index), VAL_DOOR_DCLOCK) ||
       !obj_get_value(obj_proto_of(g, g->parse_obj_index), VAL_DOOR_DCHIDE))) {
    obj_set_value(obj_proto_of(g, g->parse_obj_index), VAL_DOOR_DCLOCK, 20);
    obj_set_value(obj_proto_of(g, g->parse_obj_index), VAL_DOOR_DCHIDE, 20);
    if (bitsavetodisk) {
      add_to_save_list(g, find_zone_by_thing(g, nr)->number, 1);
      g->converting = true;
    }
  }

  if (obj_get_type_flag(obj_proto_of(g, g->parse_obj_index)) == ITEM_WEAPON && obj_get_value(obj_proto_of(g, g->parse_obj_index), 0) > 169) {
    obj_set_value(obj_proto_of(g, g->parse_obj_index), 0, suntzu_weapon_convert(t[0]));

    if (bitsavetodisk) {
      add_to_save_list(g, find_zone_by_thing(g, nr)->number, 1);
      g->converting = true;
    }
  }

  /* Convert old CWG-SunTzu style armor values to CWG-Rasputin. Should no longer be needed I think.
   * if (GET_OBJ_TYPE(obj_proto + i) == ITEM_ARMOR) {
   *   if (suntzu_armor_convert(obj_proto + i)) {
   *     if(bitsavetodisk) {
   *       add_to_save_list(zone_table[find_zone_by_thing(nr)].number, 1);
   *       converting = TRUE;
   *     }
   *   }
   * }*/

  if (!get_line(obj_f, line)) {
    mud_log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    if (retval == 3)
      t[3] = 0;
    else {
      mud_log("SYSERR: Format error in third numeric line (expecting 4 args, got %d), %s", retval, buf2);
      exit(1);
    }
  }
  obj_set_weight(obj_proto_of(g, g->parse_obj_index), t[0]);
  obj_set_cost(obj_proto_of(g, g->parse_obj_index), t[1]);
  obj_set_cost_per_day(obj_proto_of(g, g->parse_obj_index), t[2]);
  obj_set_level(obj_proto_of(g, g->parse_obj_index), t[3]);
  obj_set_size(obj_proto_of(g, g->parse_obj_index), SIZE_MEDIUM);

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (obj_get_type_flag(obj_proto_of(g, g->parse_obj_index)) == ITEM_DRINKCON ||
      obj_get_type_flag(obj_proto_of(g, g->parse_obj_index)) == ITEM_FOUNTAIN) {
    if (obj_get_weight(obj_proto_of(g, g->parse_obj_index)) < obj_get_value(obj_proto_of(g, g->parse_obj_index), 1))
      obj_set_weight(obj_proto_of(g, g->parse_obj_index), obj_get_value(obj_proto_of(g, g->parse_obj_index), 1) + 5);
  }
  /* *** make sure portal objects have their timer set correctly *** */
  if (obj_get_type_flag(obj_proto_of(g, g->parse_obj_index)) == ITEM_PORTAL) {
    obj_set_timer(obj_proto_of(g, g->parse_obj_index), -1);
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_get_affected(obj_proto_of(g, g->parse_obj_index))[j].location = APPLY_NONE;
    obj_get_affected(obj_proto_of(g, g->parse_obj_index))[j].modifier = 0;
    obj_get_affected(obj_proto_of(g, g->parse_obj_index))[j].specific = 0;
  }

  strcat(buf2, ", after numeric constants\n" /* strcat: OK (for 'buf2 >= 87') */
               "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      mud_log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
    switch (*line) {
      case 'E':
        new_descr                   = new extra_descr_data();
        new_descr->keyword          = fread_string(obj_f, buf2);
        new_descr->description      = fread_string(obj_f, buf2);
        new_descr->next             = obj_get_ex_description(obj_proto_of(g, g->parse_obj_index));
        obj_set_ex_description(obj_proto_of(g, g->parse_obj_index), new_descr);
        break;
      case 'A':
        if (j >= MAX_OBJ_AFFECT) {
          mud_log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
          exit(1);
        }
        if (!get_line(obj_f, line)) {
          mud_log("SYSERR: Format error in 'A' field, %s\n"
                  "...expecting 2 numeric constants but file ended!",
                  buf2);
          exit(1);
        }

        t[1] = 0;
        if ((retval = sscanf(line, " %d %d %d ", t, t + 1, t + 2)) != 3) {
          if (retval != 2) {
            mud_log("SYSERR: Format error in 'A' field, %s\n"
                    "...expecting 2 numeric arguments, got %d\n"
                    "...offending line: '%s'",
                    buf2, retval, line);
            exit(1);
          }
        }

        if (t[0] >= APPLY_UNUSED3 && t[0] <= APPLY_UNUSED4) {
          mud_log("Warning: object #%d (%s) uses deprecated saving throw applies",
                  nr, obj_get_short_description(obj_proto_of(g, g->parse_obj_index)));
        }
        obj_get_affected(obj_proto_of(g, g->parse_obj_index))[j].location = t[0];
        obj_get_affected(obj_proto_of(g, g->parse_obj_index))[j].modifier = t[1];
        obj_get_affected(obj_proto_of(g, g->parse_obj_index))[j].specific = affect_checked_specific(t[0], t[2]);
        j++;
        break;
      case 'S': /* Spells for Spellbooks*/
        if (j >= SPELLBOOK_SIZE) {
          mud_log("SYSERR: Unknown spellbook slot in S field, %s", buf2);
          exit(1);
        }
        if (!get_line(obj_f, line)) {
          mud_log("SYSERR: Format error in 'S' field, %s\n"
                  "...expecting 2 numeric constants but file ended!",
                  buf2);
          exit(1);
        }

        if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
          mud_log("SYSERR: Format error in 'S' field, %s\n"
                  "...expecting 2 numeric arguments, got %d\n"
                  "...offending line: '%s'",
                  buf2, retval, line);
          exit(1);
        }
        if (!obj_get_sbinfo(obj_proto_of(g, g->parse_obj_index))) {
          *obj_sbinfo_slot(obj_proto_of(g, g->parse_obj_index)) = new obj_spellbook_spell[SPELLBOOK_SIZE]();
        }
        obj_get_sbinfo(obj_proto_of(g, g->parse_obj_index))[j].spellname = t[0];
        obj_get_sbinfo(obj_proto_of(g, g->parse_obj_index))[j].pages     = t[1];
        j++;
        break;
      case 'T': /* DG triggers */
        dg_obj_trigger(g, line, obj_proto_of(g, g->parse_obj_index));
        break;
      case 'Z':
        if (!get_line(obj_f, line)) {
          mud_log("SYSERR: Format error in 'Z' field, %s\n"
                  "...expecting numeric constant but file ended!",
                  buf2);
          exit(1);
        }
        if (sscanf(line, "%d", t) != 1) {
          mud_log("SYSERR: Format error in 'Z' field, %s\n"
                  "...expecting numeric argument\n"
                  "...offending line: '%s'",
                  buf2, line);
          exit(1);
        }
        obj_set_size(obj_proto_of(g, g->parse_obj_index), t[0]);
        break;
      case '$':
      case '#':
        /* Objects that set CHARM on players are bad. */
        if (obj_aff_flagged(obj_proto_of(g, g->parse_obj_index), AFF_CHARM)) {
          mud_log("SYSERR: Object #%d has reserved bit AFF_CHARM set.", nr);
          obj_aff_set(obj_proto_of(g, g->parse_obj_index), AFF_CHARM, false);
        }
        check_object(g, obj_proto_of(g, g->parse_obj_index));
        return (line);
      default:
        mud_log("SYSERR: Format error in (%c): %s", *line, buf2);
        exit(1);
    }
  }
  return line;
}

#define Z (*zone)

/* load the zone table and command tables */
static void load_zones(GameWorld *g, FILE *fl, char *zonename)
{
  std::unique_ptr<zone_data> made = std::make_unique<zone_data>();
  struct zone_data *zone          = made.get(); /* Z and ZCMD2 refer to it */
  int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, arg_num, version = 1;
  char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH];
  int zone_fix = false;
  char t1[80], t2[80], line[MAX_STRING_LENGTH];

  strlcpy(zname, zonename, sizeof(zname));

  /* Skip first 3 lines lest we mistake the zone name for a command. */
  for (tmp = 0; tmp < 3; tmp++)
    get_line(fl, buf);

  /*  More accurate count. Previous was always 4 or 5 too high. -gg 2001/1/17
   *  Note that if a new zone command is added to reset_zone(), this string
   *  will need to be updated to suit. - ae.
   */
  while (get_line(fl, buf))
    if ((strchr("MOPGERDTV", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
      num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0) {
    mud_log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    Z.cmd = new reset_com[num_of_cmds]();

  line_num += get_line(fl, buf);

  if (*buf == '@') {
    if (sscanf(buf, "@Version: %d", &version) != 1) {
      mud_log("SYSERR: Format error in %s (version)", zname);
      mud_log("SYSERR: ...Line: %s", line);
      exit(1);
    }
    line_num += get_line(fl, buf);
  }

  if (sscanf(buf, "#%d", &Z.number) != 1) {
    mud_log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  snprintf(buf2, sizeof(buf2), "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != nullptr) /* take off the '~' if it's there */
    *ptr = '\0';
  Z.builders = strdup(buf);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != nullptr) /* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  if (version >= 2) {

    char zbuf1[MAX_STRING_LENGTH];
    char zbuf2[MAX_STRING_LENGTH];
    char zbuf3[MAX_STRING_LENGTH];
    char zbuf4[MAX_STRING_LENGTH];

    if (sscanf(buf, " %d %d %d %d %s %s %s %s %d %d", &Z.bot, &Z.top, &Z.lifespan,
               &Z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &Z.min_level, &Z.max_level) != 10) {
      mud_log("SYSERR: Format error in 10-constant line of %s", zname);
      exit(1);
    }

    Z.zone_flags[0] = asciiflag_conv(zbuf1);
    Z.zone_flags[1] = asciiflag_conv(zbuf2);
    Z.zone_flags[2] = asciiflag_conv(zbuf3);
    Z.zone_flags[3] = asciiflag_conv(zbuf4);

  } else if (sscanf(buf, " %d %d %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
    /*
     * This may be due to the fact that the zone has no builder.  So, we just attempt
     * to fix this by copying the previous 2 last reads into this variable and the
     * last one.
     */
    mud_log("SYSERR: Format error in numeric constant line of %s, attempting to fix.", zname);
    if (sscanf(Z.name, " %d %d %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
      mud_log("SYSERR: Could not fix previous error, aborting game.");
      exit(1);
    } else {
      free(Z.name);
      Z.name = strdup(Z.builders);
      free(Z.builders);
      Z.builders = strdup("None.");
      zone_fix   = true;
    }
  }
  if (Z.bot > Z.top) {
    mud_log("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
    exit(1);
  }

  cmd_no = 0;

  for (;;) {
    /* skip reading one line if we fixed above (line is correct already) */
    if (zone_fix != true) {
      if ((tmp = get_line(fl, buf)) == 0) {
        mud_log("SYSERR: Format error in %s - premature end of file", zname);
        exit(1);
      }
    } else
      zone_fix = false;

    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD2.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD2.command == 'S' || ZCMD2.command == '$') {
      ZCMD2.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPDTVG", ZCMD2.command) == nullptr) { /* a 4-arg command */
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD2.arg1, &ZCMD2.arg2, &ZCMD2.arg3) != 4)
        error = 1;
    } else if (ZCMD2.command == 'V') { /* a string-arg command */
      if (sscanf(ptr, " %d %d %d %d %d %d %79s %79[^\f\n\r\t\v]", &tmp, &ZCMD2.arg1, &ZCMD2.arg2, &ZCMD2.arg3, &ZCMD2.arg4, &ZCMD2.arg5, t1, t2) != 8)
        error = 1;
      else {
        ZCMD2.sarg1 = strdup(t1);
        ZCMD2.sarg2 = strdup(t2);
      }
    } else {
      if ((arg_num = sscanf(ptr, " %d %d %d %d %d %d ", &tmp, &ZCMD2.arg1, &ZCMD2.arg2, &ZCMD2.arg3, &ZCMD2.arg4, &ZCMD2.arg5)) != 6) {
        if (arg_num != 5) {
          error = 1;
        } else {
          ZCMD2.arg5 = 0;
        }
      }
    }

    ZCMD2.if_flag = tmp;

    if (error) {
      mud_log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD2.line = line_num;
    cmd_no++;
  }

  if (num_of_cmds != cmd_no + 1) {
    mud_log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d", zname, num_of_cmds, cmd_no + 1);
    exit(1);
  }

  if (g->zones.find(Z.number)) {
    mud_log("SYSERR: zone #%d is defined twice.", Z.number);
    exit(1);
  }
  g->zones.insert(Z.number, std::move(made));
}

#undef Z

static void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == nullptr) {
    mud_log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

void free_help_table(GameWorld *g)
{
  if (g->help_table) {
    int hp;
    for (hp = 0; hp <= g->top_of_helpt; hp++) { /* top_of_helpt is the last index */
      if (g->help_table[hp].keywords)
        free(g->help_table[hp].keywords);
      if (g->help_table[hp].entry && !g->help_table[hp].duplicate)
        free(g->help_table[hp].entry);
    }
    delete[] g->help_table;
    g->help_table = nullptr;
  }
  g->top_of_helpt = 0;
}

static void load_help(GameWorld *g, FILE *fl, char *name)
{
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
  size_t entrylen;
  char line[READ_SIZE + 1], hname[READ_SIZE + 1], *scan;
  struct help_index_element el;

  strlcpy(hname, name, sizeof(hname));

  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key, "\r\n"); /* strcat: OK (READ_SIZE - "\n"  "\r\n" == READ_SIZE  1) */
    entrylen = strlcpy(entry, key, sizeof(entry));

    /* Read in the corresponding help entry. */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1) {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1) {
        strcpy(entry + entrylen, "\r\n"); /* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1) {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg); /* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      mud_log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    if (*line == '#') {
      if (sscanf(line, "#%d", &el.min_level) != 1) {
        mud_log("SYSERR: Help entry does not have a min level. %s", key);
        el.min_level = 0;
      }
    }

    el.duplicate = 0;
    el.entry     = strdup(entry);
    scan         = one_word(key, next_key);

    while (*next_key) {
      el.keywords                = strdup(next_key);
      g->help_table[g->top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }
    get_one_line(fl, key);
  }
}

static int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *)a;
  b1 = (const struct help_index_element *)b;

  return (strcasecmp(a1->keywords, b1->keywords));
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/

int vnum_mobile(GameWorld *g, char *searchname, struct char_data *ch)
{
  mob_rnum nr;
  int found = 0;

  for (nr = 0; nr <= g->top_mob; nr++)
    if (isname(searchname, char_get_alias(&g->mob_proto[nr])))
      send_to_char(g, ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, g->mob_index[nr].vnum, char_get_sdesc(&g->mob_proto[nr]),
                   char_get_proto_script(&g->mob_proto[nr]) ? "[TRIG]" : "");

  return (found);
}

int vnum_object(GameWorld *g, char *searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (isname(searchname, obj_get_name(obj_proto_of(g, nr))))
      send_to_char(g, ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                   obj_get_proto_script(obj_proto_of(g, nr)) ? "[TRIG]" : "");

  return (found);
}

int vnum_material(GameWorld *g, char *searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (isname(searchname, material_names[obj_get_value(obj_proto_of(g, nr), VAL_ALL_MATERIAL)])) {
      send_to_char(g, ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                   obj_get_proto_script(obj_proto_of(g, nr)) ? "[TRIG]" : "");
    }

  return (found);
}

int vnum_weapontype(GameWorld *g, char *searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (obj_get_type_flag(obj_proto_of(g, nr)) == ITEM_WEAPON) {
      if (isname(searchname, weapon_type[obj_get_value(obj_proto_of(g, nr), VAL_WEAPON_SKILL)])) {
        send_to_char(g, ch, "%3d. [%5d] %-40s %s\r\n",
                     ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                     obj_get_proto_script(obj_proto_of(g, nr)) ? "[TRIG]" : "");
      }
    }

  return (found);
}

int vnum_armortype(GameWorld *g, char *searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (obj_get_type_flag(obj_proto_of(g, nr)) == ITEM_ARMOR) {
      if (isname(searchname, armor_type[obj_get_value(obj_proto_of(g, nr), VAL_ARMOR_SKILL)])) {
        send_to_char(g, ch, "%3d. [%5d] %-40s %s\r\n",
                     ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                     obj_get_proto_script(obj_proto_of(g, nr)) ? "[TRIG]" : "");
      }
    }

  return (found);
}

/* create a character, and add it to the char list */

/* create a new mobile from a prototype */
struct char_data *read_mobile(GameWorld *g, mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(g, nr)) == NOBODY) {
      mud_log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (nullptr);
    }
  } else
    i = nr;

  mob = new char_data();
  clear_char(mob);
  *mob             = g->mob_proto[i];
  char_set_next(mob, g->characters);
  g->characters   = mob;
  char_set_next_affect(mob, nullptr);

  if (char_is_hoshijin(mob) && char_get_sex(mob) == SEX_MALE) {
    char_set_hairl(mob, 0);
    char_set_hairc(mob, 0);
    char_set_hairs(mob, 0);
  } else {
    char_set_hairl(mob, rand_number(g, 0, 4));
    char_set_hairc(mob, rand_number(g, 1, 13));
    char_set_hairs(mob, rand_number(g, 1, 11));
  }

  char_set_eye(mob, rand_number(g, 0, 11));

  char_set_absorbs(mob, 0);
  char_set_absorbing(mob, nullptr);
  char_set_absorbby(mob, nullptr);
  char_set_sits(mob, nullptr);
  char_set_blocked(mob, nullptr);
  char_set_blocks(mob, nullptr);

  if (!char_is_human(mob) && !char_is_saiyan(mob) && !char_is_halfbreed(mob) && !char_is_namek(mob)) {
    char_set_skin(mob, rand_number(g, 0, 11));
  }
  if (char_is_namek(mob)) {
    char_set_skin(mob, 2);
  }
  if (char_is_human(mob) || char_is_saiyan(mob) || char_is_halfbreed(mob)) {
    if (rand_number(g, 1, 5) <= 2) {
      char_set_skin(mob, rand_number(g, 0, 1));
    } else if (rand_number(g, 1, 5) <= 4) {
      char_set_skin(mob, rand_number(g, 4, 5));
    } else if (rand_number(g, 1, 5) <= 5) {
      char_set_skin(mob, rand_number(g, 9, 10));
    }
  }
  if (char_is_saiyan(mob)) {
    char_set_hairc(mob, rand_number(g, 1, 2));
    char_set_eye(mob, 1);
  }

  if (char_get_mob_vnum(g, mob) >= 81 && char_get_mob_vnum(g, mob) <= 87) {
    dragon_level(g, mob);
  }

  int64_t mult = 0;

  switch (char_get_level(mob)) {
    case 1:
      mult = rand_number(g, 50, 80);
      break;
    case 2:
      mult = rand_number(g, 90, 120);
      break;
    case 3:
      mult = rand_number(g, 100, 140);
      break;
    case 4:
      mult = rand_number(g, 120, 180);
      break;
    case 5:
      mult = rand_number(g, 200, 250);
      break;
    case 6:
      mult = rand_number(g, 240, 300);
      break;
    case 7:
      mult = rand_number(g, 280, 350);
      break;
    case 8:
      mult = rand_number(g, 320, 400);
      break;
    case 9:
      mult = rand_number(g, 380, 480);
      break;
    case 10:
      mult = rand_number(g, 500, 600);
      break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      mult = rand_number(g, 1200, 1600);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      mult = rand_number(g, 2400, 3000);
      break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      mult = rand_number(g, 5500, 8000);
      break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
      mult = rand_number(g, 10000, 14000);
      break;
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      mult = rand_number(g, 16000, 20000);
      break;
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
      mult = rand_number(g, 22000, 30000);
      break;
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
      mult = rand_number(g, 50000, 70000);
      break;
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
      mult = rand_number(g, 95000, 140000);
      break;
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
      mult = rand_number(g, 180000, 250000);
      break;
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
      mult = rand_number(g, 400000, 480000);
      break;
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
      mult = rand_number(g, 700000, 900000);
      break;
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
      mult = rand_number(g, 1400000, 1600000);
      break;
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
      mult = rand_number(g, 2200000, 2500000);
      break;
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
      mult = rand_number(g, 3000000, 3500000);
      break;
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
      mult = rand_number(g, 4250000, 4750000);
      break;
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
      mult = rand_number(g, 6500000, 8500000);
      break;
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
      mult = rand_number(g, 15000000, 18000000);
      break;
    case 96:
    case 97:
    case 98:
    case 99:
    case 100:
      mult = rand_number(g, 22000000, 30000000);
      break;
    case 101:
      mult = rand_number(g, 32000000, 40000000);
      break;
    case 102:
      mult = rand_number(g, 42000000, 55000000);
      break;
    case 103:
      mult = rand_number(g, 80000000, 95000000);
      break;
    case 104:
      mult = rand_number(g, 150000000, 200000000);
      break;
    case 105:
      mult = rand_number(g, 220000000, 250000000);
      break;
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
      mult = rand_number(g, 500000000, 750000000);
      break;
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 120:
      mult = rand_number(g, 800000000, 900000000);
      break;
    default:
      if (char_get_level(mob) >= 150) {
        mult = rand_number(g, 1500000000, 2000000000);
      } else {
        mult = rand_number(g, 1250000000, 1500000000);
      }
      break;
  }

  char_set_lastpl(mob, game_now(g));
  bool autoset = false;
  if (getBasePL(mob) <= 1) {
    autoset     = true;
    char_set_basepl(mob, char_get_level(mob) * mult);
    if (char_get_level(mob) > 140) {
      char_set_basepl(mob, getBasePL(mob) * 8);
    } else if (char_get_level(mob) > 130) {
      char_set_basepl(mob, getBasePL(mob) * 6);
    } else if (char_get_level(mob) > 120) {
      char_set_basepl(mob, getBasePL(mob) * 3);
    } else if (char_get_level(mob) > 110) {
      char_set_basepl(mob, getBasePL(mob) * 2);
    }
  }
  if (autoset) {
    char_set_baseki(mob, char_get_level(mob) * mult);
    if (char_get_level(mob) > 140) {
      char_set_baseki(mob, getBaseKI(mob) * 8);
    } else if (char_get_level(mob) > 130) {
      char_set_baseki(mob, getBaseKI(mob) * 6);
    } else if (char_get_level(mob) > 120) {
      char_set_baseki(mob, getBaseKI(mob) * 3);
    } else if (char_get_level(mob) > 110) {
      char_set_baseki(mob, getBaseKI(mob) * 2);
    }
  }
  if (autoset) {
    char_set_basest(mob, char_get_level(mob) * mult);
    if (char_get_level(mob) > 140) {
      char_set_basest(mob, getBaseST(mob) * 8);
    } else if (char_get_level(mob) > 130) {
      char_set_basest(mob, getBaseST(mob) * 6);
    } else if (char_get_level(mob) > 120) {
      char_set_basest(mob, getBaseST(mob) * 3);
    } else if (char_get_level(mob) > 110) {
      char_set_basest(mob, getBaseST(mob) * 2);
    }
  }
  if (char_get_mob_vnum(g, mob) == 2245) {
    char_set_basepl(mob, rand_number(g, 1, 4));
    char_set_baseki(mob, rand_number(g, 1, 4));
    char_set_basest(mob, rand_number(g, 1, 4));
  }

  int base = 0;
  switch (char_get_level(mob)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      base = rand_number(g, 80, 120);
      break;
    case 6:
      base = rand_number(g, 200, 280);
      break;
    case 7:
      base = rand_number(g, 250, 350);
      break;
    case 8:
      base = rand_number(g, 275, 375);
      break;
    case 9:
      base = rand_number(g, 300, 400);
      break;
    case 10:
      base = rand_number(g, 325, 450);
      break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      base = rand_number(g, 500, 700);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      base = rand_number(g, 700, 1000);
      break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      base = rand_number(g, 1000, 1200);
      break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
      base = rand_number(g, 1200, 1400);
      break;
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      base = rand_number(g, 1400, 1600);
      break;
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
      base = rand_number(g, 1600, 1800);
      break;
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
      base = rand_number(g, 1800, 2000);
      break;
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
      base = rand_number(g, 2000, 2200);
      break;
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
      base = rand_number(g, 2200, 2500);
      break;
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
      base = rand_number(g, 2500, 2800);
      break;
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
      base = rand_number(g, 2800, 3000);
      break;
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
      base = rand_number(g, 3000, 3200);
      break;
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
      base = rand_number(g, 3200, 3500);
      break;
    case 76:
    case 77:
    case 78:
    case 79:
      base = rand_number(g, 3500, 3800);
      break;
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
      base = rand_number(g, 4000, 4500);
      break;
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
      base = rand_number(g, 4500, 5500);
      break;
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
      base = rand_number(g, 5500, 7000);
      break;
    case 96:
    case 97:
    case 98:
    case 99:
      base = rand_number(g, 8000, 10000);
      break;
    case 100:
      base = rand_number(g, 10000, 15000);
      break;
    case 101:
      base = rand_number(g, 15000, 25000);
      break;
    case 102:
      base = rand_number(g, 35000, 40000);
      break;
    case 103:
      base = rand_number(g, 40000, 50000);
      break;
    case 104:
      base = rand_number(g, 60000, 80000);
      break;
    case 105:
      base = rand_number(g, 80000, 100000);
      break;
    default:
      base = rand_number(g, 130000, 180000);
      break;
  }
  char_set_cooldown(mob, 0);
  if (char_get_gold(mob) <= 0 && !char_mob_flagged(mob, MOB_DUMMY)) {
    if (char_get_level(mob) < 4) {
      char_set_gold(mob, char_get_level(mob) * rand_number(g, 1, 2));
    } else if (char_get_level(mob) < 10) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 2)) - 1);
    } else if (char_get_level(mob) < 20) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 3)) - 2);
    } else if (char_get_level(mob) < 30) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 3)) - 4);
    } else if (char_get_level(mob) < 40) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 3)) - 6);
    } else if (char_get_level(mob) < 50) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 3)) - 25);
    } else if (char_get_level(mob) < 60) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 3)) - 40);
    } else if (char_get_level(mob) < 70) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 3)) - 50);
    } else if (char_get_level(mob) < 80) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 4)) - 60);
    } else if (char_get_level(mob) < 90) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 4)) - 70);
    } else {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 3, 4)) - 85);
    }
    if (!char_is_humanoid(mob)) {
      char_set_gold(mob, char_get_gold(mob) * 0.5);
      if (char_get_gold(mob) <= 0)
        char_set_gold(mob, 1);
    }
  }
  if (char_get_exp(mob) <= 0 && !char_mob_flagged(mob, MOB_DUMMY)) {
    char_set_exp(mob, (int64_t)char_get_level(mob) * base);
    char_set_exp(mob, char_get_exp(mob) * .9);
    char_set_exp(mob, char_get_exp(mob) + (char_get_level(mob) / 2));
    char_set_exp(mob, char_get_exp(mob) + (char_get_level(mob) / 3));
    if (char_is_dragon(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.4));
    } else if (char_is_android(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.25));
    } else if (char_is_saiyan(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.1));
    } else if (char_is_bio(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.2));
    } else if (char_is_majin(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.25));
    } else if (char_is_demon(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.1));
    } else if (char_get_class(mob) == CLASS_SHADOWDANCER) {
      char_set_exp(mob, char_get_exp(mob) * (2));
    }
    if (char_get_class(mob) == CLASS_NPC_COMMONER && char_is_humanoid(mob) && !char_is_dragon(mob)) {
      if (!char_is_android(mob) && !char_is_saiyan(mob) && !char_is_bio(mob) && !char_is_majin(mob)) {
        char_set_exp(mob, char_get_exp(mob) * (0.75));
      }
    }

    if (char_get_level(mob) > 90) {
      char_set_exp(mob, char_get_exp(mob) * .7);
    } else if (char_get_level(mob) > 80) {
      char_set_exp(mob, char_get_exp(mob) * .75);
    } else if (char_get_level(mob) > 70) {
      char_set_exp(mob, char_get_exp(mob) * .8);
    } else if (char_get_level(mob) > 60) {
      char_set_exp(mob, char_get_exp(mob) * .85);
    } else if (char_get_level(mob) > 40) {
      char_set_exp(mob, char_get_exp(mob) * .9);
    } else if (char_get_level(mob) > 30) {
      char_set_exp(mob, char_get_exp(mob) * .95);
    }

    if (char_get_exp(mob) > 20000000) {
      char_set_exp(mob, 20000000);
    }
    if (char_get_exp(mob) < 0) {
      char_set_exp(mob, 0);
    }
  }

  char_set_time_birth(mob, game_now(g) - birth_age(g, mob));
  char_set_time_logon(mob, game_now(g));
  char_set_time_created(mob, char_get_time_logon(mob)); /* why not */
  char_set_time_maxage(mob, char_get_time_birth(mob) + max_age(mob));
  char_set_time_played(mob, 0);
  char_set_time_logon(mob, game_now(g));
  char_set_hometown(mob, NOWHERE);

  if (char_is_humanoid(mob)) {
    char_mob_set(mob, MOB_RARM, true);
    char_mob_set(mob, MOB_LARM, true);
    char_mob_set(mob, MOB_RLEG, true);
    char_mob_set(mob, MOB_LLEG, true);
  }

  g->mob_index[i].number++;

  char_set_id(mob, dg_new_mob_uid(g));
  /* find_char helper */
  add_to_lookup_table(g, char_get_id(mob), MOB_TRIGGER, (void *)mob);

  copy_proto_script(&g->mob_proto[i], mob, MOB_TRIGGER);
  assign_triggers(g, mob, MOB_TRIGGER);
  racial_body_parts(mob);

  if (char_get_mob_vnum(g, mob) >= 800 && char_get_mob_vnum(g, mob) <= 805) {
    g->number_of_assassins += 1;
  }

  return (mob);
}

struct obj_unique_hash_elem {
  time_t generation{};
  int64_t unique_id{};
  struct obj_data *obj{};
  struct obj_unique_hash_elem *next_e{};
};

static void free_obj_unique_hash(GameWorld *g)
{
  int i;
  struct obj_unique_hash_elem *elem;
  struct obj_unique_hash_elem *next_elem;
  if (g->obj_unique_hash_pools) {
    for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
      elem = g->obj_unique_hash_pools[i];
      while (elem) {
        next_elem = elem->next_e;
        delete elem;
        elem = next_elem;
      }
    }
    delete[] g->obj_unique_hash_pools;
  }
}

void add_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  int i;
  if (!g->obj_unique_hash_pools)
    init_obj_unique_hash(g);
  if (obj_get_unique_id(obj) == -1) {
    if (sizeof(long long) > sizeof(long))
      obj_set_unique_id(obj, (((long long)circle_random(g)) << (sizeof(long long) * 4)) +
                       circle_random(g));
    else
      obj_set_unique_id(obj, circle_random(g));
  }
  if (CONFIG_ALL_ITEMS_UNIQUE) {
    if (!obj_extra_flagged(obj, ITEM_UNIQUE_SAVE))
      obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);
  }
  elem                        = new obj_unique_hash_elem();
  elem->generation         = obj_get_generation(obj);
  elem->unique_id          = obj_get_unique_id(obj);
  elem->obj                = obj;
  i                        = obj_get_unique_id(obj) % NUM_OBJ_UNIQUE_POOLS;
  elem->next_e             = g->obj_unique_hash_pools[i];
  g->obj_unique_hash_pools[i] = elem;
}

static void remove_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem, **ptr, *tmp;

  if (obj == nullptr || obj_get_unique_id(obj) < 0)
    return;

  ptr = g->obj_unique_hash_pools + (obj_get_unique_id(obj) % NUM_OBJ_UNIQUE_POOLS);

  if (!(ptr && *ptr))
    return;

  elem = *ptr;

  while (elem) {
    tmp = elem->next_e;
    if (elem->obj == obj) {
      delete elem;
      *ptr = tmp;
    } else {
      ptr = &(elem->next_e);
    }
    elem = tmp;
  }
}

static void log_dupe_objects(GameWorld *g, struct obj_data *obj1, struct obj_data *obj2)
{
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: Dupe object found: %s [%d] [%lld:%" PRId64 "]",
         obj_get_short_description(obj1) ? obj_get_short_description(obj1) : "<No name>",
         obj_get_vnum(g, obj1), (long long)obj_get_generation(obj1), obj_get_unique_id(obj1));
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: First: In room: %d (%s), "
                                    "In object: %s, Carried by: %s, Worn by: %s",
         rroom_get_vnum(g, obj_get_in_room(obj1)),
         obj_get_in_room(obj1) == NOWHERE ? "Nowhere" : rroom_get_name(g, obj_get_in_room(obj1)),
         obj_get_in_obj(obj1) ? obj_get_short_description(obj_get_in_obj(obj1)) : "None",
         obj_get_carried_by(obj1) ? char_get_name(obj_get_carried_by(obj1)) : "Nobody",
         obj_get_worn_by(obj1) ? char_get_name(obj_get_worn_by(obj1)) : "Nobody");
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: Newer: In room: %d (%s), "
                                    "In object: %s, Carried by: %s, Worn by: %s",
         rroom_get_vnum(g, obj_get_in_room(obj2)),
         obj_get_in_room(obj2) == NOWHERE ? "Nowhere" : rroom_get_name(g, obj_get_in_room(obj2)),
         obj_get_in_obj(obj2) ? obj_get_short_description(obj_get_in_obj(obj2)) : "None",
         obj_get_carried_by(obj2) ? char_get_name(obj_get_carried_by(obj2)) : "Nobody",
         obj_get_worn_by(obj2) ? char_get_name(obj_get_worn_by(obj2)) : "Nobody");
}

void check_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  if (!obj || obj_get_unique_id(obj) == -1)
    return;
  elem = g->obj_unique_hash_pools[obj_get_unique_id(obj) % NUM_OBJ_UNIQUE_POOLS];
  while (elem) {
    if (elem->obj == obj) {
      mud_log("SYSERR: check_unique_id checking for existing object?!");
    }
    if (elem->generation == obj_get_generation(obj) && elem->unique_id == obj_get_unique_id(obj)) {
      log_dupe_objects(g, elem->obj, obj);
      obj_extra_set(obj, ITEM_PURGE, true);
    }
    elem = elem->next_e;
  }
}

char *sprintuniques(GameWorld *g, obj_vnum low, obj_vnum high)
{
  int i, count = 0, remain, header;
  struct obj_unique_hash_elem *q;
  char *str, *ptr;
  remain = 40;
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    q = g->obj_unique_hash_pools[i];
    remain += 40;
    while (q) {
      count++;
      remain += 80 + (obj_get_short_description(q->obj) ? strlen(obj_get_short_description(q->obj)) : 20);
      q = q->next_e;
    }
  }
  if (count < 1) {
    return strdup("No objects in unique hash.\r\n");
  }
  str   = str_alloc(remain + 1);
  ptr   = str;
  count = snprintf(ptr, remain, "Unique object hashes (vnums %d - %d)\r\n",
                   low, high);
  ptr += count;
  remain -= count;
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    header = 0;
    q      = g->obj_unique_hash_pools[i];
    while (q) {
      if (obj_get_vnum(g, q->obj) >= low && obj_get_vnum(g, q->obj) <= high) {
        if (!header) {
          header = 1;
          count  = snprintf(ptr, remain, "|-Hash %d\r\n", i);
          ptr += count;
          remain -= count;
        }
        count = snprintf(ptr, remain, "| |- [@g%6d@n] - [@y%10lld:%-19" PRId64 "@n] - %s\r\n",
                         obj_get_vnum(g, q->obj), (long long)q->generation, q->unique_id,
                         obj_get_short_description(q->obj) ? obj_get_short_description(q->obj) : "<Unknown>");
        ptr += count;
        remain -= count;
      }
      q = q->next_e;
    }
  }
  return str;
}

/* create an object, and add it to the object list */
struct obj_data *create_obj(GameWorld *g)
{
  struct obj_data *obj;

  obj = new obj_data();
  clear_object(obj);
  obj_set_next(obj, g->objects);
  g->objects = obj;

  obj_set_id(obj, dg_new_obj_uid(g));
  /* find_obj helper */
  add_to_lookup_table(g, obj_get_id(obj), OBJ_TRIGGER, (void *)obj);

  obj_set_generation(obj, game_now(g));
  obj_set_unique_id(obj, -1);

  assign_triggers(g, obj, OBJ_TRIGGER);

  return (obj);
}

/* create a new object from a prototype */
struct obj_data *read_object(GameWorld *g, obj_vnum nr, int type) /* and obj_vnum */
{
  struct obj_data *obj;
  obj_vnum i = type == VIRTUAL ? real_object(g, nr) : nr;
  int j;

  if (i == NOTHING || !obj_proto_of(g, i)) {
    mud_log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
    return (nullptr);
  }

  obj = new obj_data();
  clear_object(obj);
  *obj = (*obj_proto_of(g, i));
  obj_set_next(obj, g->objects);
  g->objects = obj;
  obj_set_room_loaded(obj, NOWHERE);

  obj_index_of(g, i)->number++;

  obj_set_id(obj, dg_new_obj_uid(g));
  /* find_obj helper */
  add_to_lookup_table(g, obj_get_id(obj), OBJ_TRIGGER, (void *)obj);

  obj_set_generation(obj, game_now(g));
  obj_set_unique_id(obj, -1);

  if (obj_get_sbinfo(obj_proto_of(g, i))) {
    *obj_sbinfo_slot(obj) = new obj_spellbook_spell[SPELLBOOK_SIZE]();
    for (j = 0; j < SPELLBOOK_SIZE; j++) {
      obj_get_sbinfo(obj)[j].spellname = obj_get_sbinfo(obj_proto_of(g, i))[j].spellname;
      obj_get_sbinfo(obj)[j].pages     = obj_get_sbinfo(obj_proto_of(g, i))[j].pages;
    }
  }
  copy_proto_script(obj_proto_of(g, i), obj, OBJ_TRIGGER);
  assign_triggers(g, obj, OBJ_TRIGGER);
  if (obj_get_vnum(g, obj) == 65) {
    obj_set_healcharge(obj, 20);
  }
  if (obj_get_type_flag(obj) == ITEM_FOOD) {
    if (obj_get_value(obj, 1) == 0) {
      obj_set_value(obj, 1, obj_get_value(obj, VAL_FOOD_FOODVAL));
    }
    obj_set_foob(obj, obj_get_value(obj, 1));
  }
  return (obj);
}

constexpr int ZO_DEAD = 999;

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(GameWorld *g)
{
  struct reset_q_element *update_u, *temp;

  /* jelson 10/22/92 */
  if (((++g->zone_update_timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    g->zone_update_timer = 0;

    /* since one minute has passed, increment zone ages */
    for (struct zone_data *i : g->zones.all()) {
      if (i->age < i->lifespan && i->reset_mode)
        (i->age)++;

      if (i->age >= i->lifespan && i->age < ZO_DEAD && i->reset_mode) {
        /* enqueue zone */

        update_u = new reset_q_element();

        update_u->zone_to_reset = i;

        if (!g->reset_q.head)
          g->reset_q.head = g->reset_q.tail = update_u;
        else {
          g->reset_q.tail->next = update_u;
          g->reset_q.tail       = update_u;
        }

        i->age = ZO_DEAD;
      }
    }
  } /* end - one minute has passed */

  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = g->reset_q.head; update_u; update_u = update_u->next)
    if (update_u->zone_to_reset->reset_mode == 2 ||
        is_empty(g, update_u->zone_to_reset)) {
      reset_zone(g, update_u->zone_to_reset);
      mudlog(g, LOG_CMP, ADMLVL_GOD, false, "Auto zone reset: %s (Zone %d)",
             update_u->zone_to_reset->name, update_u->zone_to_reset->number);
      /* dequeue */
      if (update_u == g->reset_q.head)
        g->reset_q.head = g->reset_q.head->next;
      else {
        for (temp = g->reset_q.head; temp->next != update_u;
             temp = temp->next);

        if (!update_u->next)
          g->reset_q.tail = temp;

        temp->next = update_u->next;
      }

      delete update_u;
      break;
    }
}

static void log_zone_error(GameWorld *g, struct zone_data *zone, int cmd_no, const char *message)
{
  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: zone file: %s", message);
  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
         ZCMD2.command, zone->number, ZCMD2.line);
}

#define ZONE_ERROR(g, message)                \
  {                                        \
    log_zone_error(g, zone, cmd_no, message); \
    last_cmd = 0;                          \
  }

/* execute the reset command table of a given zone */
void reset_zone(GameWorld *g, struct zone_data *zone)
{
  int cmd_no, last_cmd = 0;
  struct char_data *mob = nullptr;
  struct obj_data *obj, *obj_to;
  room_vnum rvnum;
  room_rnum rrnum;
  struct char_data *tmob = nullptr; /* for trigger assignment */
  struct obj_data *tobj  = nullptr; /* for trigger assignment */
  int mob_load           = false;   /* ### */
  int obj_load           = false;   /* ### */

  if (pre_reset(g, zone->number) == false) {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {

      if (ZCMD2.if_flag && !last_cmd && !mob_load && !obj_load)
        continue;

      if (!ZCMD2.if_flag) { /* ### */
        mob_load = false;
        obj_load = false;
      }

      /*  This is the list of actual zone commands.  If any new
     *  zone commands are added to the game, be certain to update
     *  the list of commands in load_zone() so that the counting
     *  will still be correct. - ae.
     */
      switch (ZCMD2.command) {
        case '*': /* ignore command */
          last_cmd = 0;
          break;

        case 'M': /* read a mobile */

          if ((g->mob_index[ZCMD2.arg1].number < ZCMD2.arg2) &&
              (rand_number(g, 1, 100) >= ZCMD2.arg5)) {
            int room_max = 0;
            mob = read_mobile(g, ZCMD2.arg1, REAL);

            /* First find out how many mobs of VNUM are in the mud with this rooms */
            /* VNUM as a load point for max from room checks. */
            /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

            if (ZCMD2.arg4 > 0) {
              for (char_data *i : each_char(g)) {
                if ((char_get_hometown(i) == rroom_get_vnum(g, ZCMD2.arg3)) && (char_get_mob_vnum(g, i) == char_get_mob_vnum(g, mob))) {
                  room_max++;
                }
              }
            }
            char_to_room(g, mob, ZCMD2.arg3);

            /* Get rid of it if room_max has been met, ignore room_max if zero */

            if (room_max && (room_max >= ZCMD2.arg4)) {
              extract_char(g, mob);
              extract_pending_chars(g);
              break;
            }

            /*  Set the mobs loadroom for room_max checks. */
            char_set_hometown(mob, rroom_get_vnum(g, ZCMD2.arg3));

            load_mtrigger(g, mob);
            tmob     = mob;
            last_cmd = 1;
            mob_load = true;
          } else
            last_cmd = 0;
          tobj = nullptr;
          break;

        case 'O': /* read an object */
          if ((obj_index_of(g, ZCMD2.arg1)->number < ZCMD2.arg2) &&
              (rand_number(g, 1, 100) >= ZCMD2.arg5)) {
            if (ZCMD2.arg3 != NOWHERE) {
              int room_max = 0;
              obj = read_object(g, ZCMD2.arg1, REAL);

              /* First find out how many obj of VNUM are in the mud with this rooms */
              /* VNUM as a load point for max from room checks. */
              /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

              if (ZCMD2.arg4 > 0) {
                for (obj_data *k : each_obj(g)) {
                  if (((obj_get_room_loaded(k) == rroom_get_vnum(g, ZCMD2.arg3)) && (obj_get_vnum(g, k) == obj_get_vnum(g, obj))) || (obj_get_vnum(g, k) == obj_get_vnum(g, obj) && rroom_get_vnum(g, ZCMD2.arg3) == rroom_get_vnum(g, obj_get_in_room(k)))) {
                    /*  For objects, lets not count them if they've been removed from the room */
                    /*  We'll let max_in_mud handle those. */
                    if (obj_get_in_room(k) == NOWHERE || rroom_get_vnum(g, obj_get_in_room(k)) != rroom_get_vnum(g, ZCMD2.arg3)) {
                      continue;
                    }
                    room_max++;
                  }
                }
              }

              add_unique_id(g, obj);
              obj_to_room(g, obj, ZCMD2.arg3);

              /* Get rid of it if room_max has been met. */

              if (room_max && (room_max >= ZCMD2.arg4)) {
                extract_obj(g, obj);
                break;
              }

              /* Set the loadroom for room_max checks */

              obj_set_room_loaded(obj, rroom_get_vnum(g, ZCMD2.arg3));

              last_cmd = 1;
              tobj     = load_otrigger(g, obj) ? obj : nullptr;
              obj_load = true;
            } else {
              obj = read_object(g, ZCMD2.arg1, REAL);
              add_unique_id(g, obj);
              obj_set_in_room(obj, NOWHERE);
              last_cmd = 1;
              tobj     = obj;
              obj_load = true;
            }
          } else
            last_cmd = 0;
          tmob = nullptr;
          break;

        case 'P': /* object to object */
          if ((obj_index_of(g, ZCMD2.arg1)->number < ZCMD2.arg2) &&
              obj_load && (rand_number(g, 1, 100) >= ZCMD2.arg5)) {
            obj = read_object(g, ZCMD2.arg1, REAL);
            if (!(obj_to = get_obj_num(g, ZCMD2.arg3))) {
              ZONE_ERROR(g, "target obj not found, command disabled");
              ZCMD2.command = '*';
              break;
            }
            add_unique_id(g, obj);
            obj_to_obj(g, obj, obj_to);
            last_cmd = 1;
            tobj     = load_otrigger(g, obj) ? obj : nullptr;
          } else
            last_cmd = 0;
          tmob = nullptr;
          break;

        case 'G': /* obj_to_char */
          if (!mob) {
            ZONE_ERROR(g, "attempt to give obj to non-existant mob, command disabled");
            ZCMD2.command = '*';
            break;
          }
          if ((obj_index_of(g, ZCMD2.arg1)->number < ZCMD2.arg2) &&
              mob_load && (rand_number(g, 1, 100) >= ZCMD2.arg5)) {
            obj = read_object(g, ZCMD2.arg1, REAL);
            add_unique_id(g, obj);
            obj_to_char(g, obj, mob);
            if (char_get_mob_spec(g, mob) != shop_keeper) {
              randomize_eq(g, obj);
            }
            last_cmd = 1;
            tobj     = load_otrigger(g, obj) ? obj : nullptr;
          } else
            last_cmd = 0;
          tmob = nullptr;
          break;

        case 'E': /* object to equipment list */
          if (!mob) {
            ZONE_ERROR(g, "trying to equip non-existant mob, command disabled");
            ZCMD2.command = '*';
            break;
          }
          if ((obj_index_of(g, ZCMD2.arg1)->number < ZCMD2.arg2) &&
              mob_load && (rand_number(g, 1, 100) >= ZCMD2.arg5)) {
            if (ZCMD2.arg3 < 0 || ZCMD2.arg3 >= NUM_WEARS) {
              ZONE_ERROR(g, "invalid equipment pos number");
            } else {
              obj = read_object(g, ZCMD2.arg1, REAL);
              add_unique_id(g, obj);
              obj_set_in_room(obj, char_get_in_room(mob));
              tobj = nullptr;
              if (load_otrigger(g, obj)) {
                long id = obj_get_id(obj);

                if (wear_otrigger(g, obj, mob, ZCMD2.arg3)) {
                  obj_set_in_room(obj, NOWHERE);
                  equip_char(g, mob, obj, ZCMD2.arg3);
                  tobj = obj;
                } else if ((obj = find_obj(g, id))) { /* wear_otrigger also says no if it was purged */
                  obj_to_char(g, obj, mob);
                  tobj = obj;
                }
              }
              last_cmd = 1;
            }
          } else
            last_cmd = 0;
          tmob = nullptr;
          break;

        case 'R': /* rem obj from room */
          if ((obj = get_obj_in_list_num(g, ZCMD2.arg2, rroom_get_contents(g, ZCMD2.arg1))) != nullptr)
            extract_obj(g, obj);
          last_cmd = 1;
          tmob     = nullptr;
          tobj     = nullptr;
          break;

        case 'D': /* set state of door */
          if (ZCMD2.arg2 < 0 || ZCMD2.arg2 >= NUM_OF_DIRS ||
              (rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2) == nullptr)) {
            ZONE_ERROR(g, "door does not exist, command disabled");
            ZCMD2.command = '*';
          } else
            switch (ZCMD2.arg3) {
              case 0:
                REMOVE_BIT(rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                           EX_LOCKED);
                REMOVE_BIT(rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                           EX_CLOSED);
                break;
              case 1:
                SET_BIT(rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                        EX_CLOSED);
                REMOVE_BIT(rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                           EX_LOCKED);
                break;
              case 2:
                SET_BIT(rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                        EX_LOCKED);
                SET_BIT(rroom_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                        EX_CLOSED);
                break;
            }
          last_cmd = 1;
          tmob     = nullptr;
          tobj     = nullptr;
          break;

        case 'T': /* trigger command */
          if (ZCMD2.arg1 == MOB_TRIGGER && tmob) {
            add_trigger(g, dg_ensure_script(char_script_slot(tmob)), read_trigger(g, find_trigger(g, ZCMD2.arg2)), -1);
            last_cmd = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER && tobj) {
            add_trigger(g, dg_ensure_script(obj_script_slot(tobj)), read_trigger(g, find_trigger(g, ZCMD2.arg2)), -1);
            last_cmd = 1;
          } else if (ZCMD2.arg1 == WLD_TRIGGER) {
            if (ZCMD2.arg3 == NOWHERE || ZCMD2.arg3 > g->top_room) {
              ZONE_ERROR(g, "Invalid room number in trigger assignment");
            }
            if (!rroom_get_script(g, ZCMD2.arg3))
              dg_ensure_script(rroom_script_slot(g, ZCMD2.arg3));
            add_trigger(g, rroom_get_script(g, ZCMD2.arg3), read_trigger(g, find_trigger(g, ZCMD2.arg2)), -1);
            last_cmd = 1;
          }

          break;

        case 'V':
          if (ZCMD2.arg1 == MOB_TRIGGER && tmob) {
            if (!char_get_script(tmob)) {
              ZONE_ERROR(g, "Attempt to give variable to scriptless mobile");
            } else
              add_var(&(char_get_script(tmob)->global_vars), ZCMD2.sarg1, ZCMD2.sarg2,
                      ZCMD2.arg3);
            last_cmd = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER && tobj) {
            if (!obj_get_script(tobj)) {
              ZONE_ERROR(g, "Attempt to give variable to scriptless object");
            } else
              add_var(&(obj_get_script(tobj)->global_vars), ZCMD2.sarg1, ZCMD2.sarg2,
                      ZCMD2.arg3);
            last_cmd = 1;
          } else if (ZCMD2.arg1 == WLD_TRIGGER) {
            if (ZCMD2.arg3 == NOWHERE || ZCMD2.arg3 > g->top_room) {
              ZONE_ERROR(g, "Invalid room number in variable assignment");
            } else {
              if (!(rroom_get_script(g, ZCMD2.arg3))) {
                ZONE_ERROR(g, "Attempt to give variable to scriptless object");
              } else
                add_var(&(rroom_get_script(g, ZCMD2.arg3)->global_vars),
                        ZCMD2.sarg1, ZCMD2.sarg2, ZCMD2.arg2);
              last_cmd = 1;
            }
          }
          break;

        default:
          ZONE_ERROR(g, "unknown cmd in reset table; cmd disabled");
          ZCMD2.command = '*';
          break;
      }
    }

    zone->age = 0;

    /* handle reset_wtrigger's */
    rvnum = zone->bot;
    while (rvnum <= zone->top) {
      rrnum = real_room(g, rvnum);
      if (rrnum != NOWHERE) {
        reset_wtrigger(g, &g->rooms[rrnum]);
        if (rroom_flagged(g, rrnum, ROOM_AURA) && rand_number(g, 1, 5) >= 4) {
          send_to_room(g, rrnum, "The aura of regeneration covering the surrounding area disappears.\r\n");
          rroom_set_flag(g, rrnum, ROOM_AURA, false);
        }
        if (rroom_sect(g, rrnum) == SECT_LAVA) {
          rroom_set_geffect(g, rrnum, 5);
        }
        if (rroom_get_geffect(g, rrnum) < -1) {
          send_to_room(g, rrnum, "The area loses some of the water flooding it.\r\n");
          rroom_set_geffect(g, rrnum, rroom_get_geffect(g, rrnum) + (1));
        } else if (rroom_get_geffect(g, rrnum) == -1) {
          send_to_room(g, rrnum, "The area loses the last of the water flooding it in one large rush.\r\n");
          rroom_set_geffect(g, rrnum, 0);
        }
        if (rroom_get_dmg(g, rrnum) >= 100) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          rroom_set_dmg(g, rrnum, rroom_get_dmg(g, rrnum) - (rand_number(g, 5, 10)));
        } else if (rroom_get_dmg(g, rrnum) >= 50) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          rroom_set_dmg(g, rrnum, rroom_get_dmg(g, rrnum) - (rand_number(g, 1, 10)));
        } else if (rroom_get_dmg(g, rrnum) >= 10) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          rroom_set_dmg(g, rrnum, rroom_get_dmg(g, rrnum) - (rand_number(g, 1, 10)));
        } else if (rroom_get_dmg(g, rrnum) > 1) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          rroom_set_dmg(g, rrnum, rroom_get_dmg(g, rrnum) - (rand_number(g, 1, rroom_get_dmg(g, rrnum))));
        } else if (rroom_get_dmg(g, rrnum) > 0) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          rroom_set_dmg(g, rrnum, rroom_get_dmg(g, rrnum) - 1);
        }
        if (rroom_get_geffect(g, rrnum) >= 1 && rand_number(g, 1, 4) == 4 && !rroom_sunken(g, rrnum) && rroom_sect(g, rrnum) != SECT_LAVA) {
          send_to_room(g, rrnum, "The lava has cooled and become solid rock.\r\n");
          rroom_set_geffect(g, rrnum, 0);
        } else if (rroom_get_geffect(g, rrnum) >= 1 && rand_number(g, 1, 2) == 2 && rroom_sunken(g, rrnum) && rroom_sect(g, rrnum) != SECT_LAVA) {
          send_to_room(g, rrnum, "The water has cooled the lava and it has become solid rock.\r\n");
          rroom_set_geffect(g, rrnum, 0);
        }
      }
      rvnum++;
    }
  } else {
    /* even if reset is blocked, age should be reset */
    zone->age = 0;
  }
  post_reset(zone->number);
}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
bool is_empty(GameWorld *g, struct zone_data *zone_nr)
{

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING)
      continue;
    if (char_get_in_room(i->character) == NOWHERE)
      continue;
    if (rroom_get_zone(g, char_get_in_room(i->character)) != zone_nr)
      continue;
    /*
     * if an immortal has nohassle off, he counts as present 
     * added for testing zone reset triggers - Welcor 
     */
    if (char_is_npc(i->character))
      continue; /* immortal switched into a mob */

    if ((char_get_admlevel(i->character) >= ADMLVL_IMMORT) && (char_prf_flagged(g, i->character, PRF_NOHASSLE)))
      continue;

    return false;
  }

  return true;
}

/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
  char *line = nullptr, *text = nullptr;
  size_t cap = 0, len = 0, n;
  FILE *out = open_memstream(&text, &len);
  bool done = false;

  while (!done) {
    if (getline(&line, &cap, fl) < 0) {
      mud_log("SYSERR: fread_string: format error at string (pos %ld): %s at or near %s",
              ftell(fl), feof(fl) ? "EOF" : ferror(fl) ? "read error"
                                                       : "unknown error",
              error);
      exit(1);
    }
    /* A line ending in '~' ends the string; any other line gets an "\r\n". Only trailing ~'s count -- Welcor */
    n = strcspn(line, "\r\n");
    if (n > 0 && line[n - 1] == '~') {
      fwrite(line, 1, n - 1, out);
      done = true;
    } else {
      fwrite(line, 1, n, out);
      fputs("\r\n", out);
    }
  }
  free(line);
  fclose(out);

  if (!len) {
    free(text);
    return nullptr;
  }
  return text;
}

/* Called to free all allocated follow_type structs - Update by Jamie Nelson */
static void free_followers(struct follow_type *k)
{
  if (!k)
    return;

  if (k->next)
    free_followers(k->next);

  k->follower = nullptr;
  delete k;
}

/* release memory allocated for a char struct */
void free_char(GameWorld *g, struct char_data *ch)
{
  int i;
  mob_rnum nr;
  struct alias_data *a;

  /* first: removing an affect can change skills, which live in the player data freed below */
  while (char_get_affected(ch))
    affect_remove(g, ch, char_get_affected(ch));

  if (char_get_player_specials(ch)) {

    while ((a = char_get_aliases(g, ch)) != nullptr) {
      char_set_aliases(g, ch, char_get_aliases(g, ch)->next);
      free_alias(a);
    }
    if (char_get_player_specials(ch)->poofin)
      free(char_get_player_specials(ch)->poofin);
    if (char_get_player_specials(ch)->poofout)
      free(char_get_player_specials(ch)->poofout);
    if (char_get_player_specials(ch)->host)
      free(char_get_player_specials(ch)->host);
    for (i = 0; i < NUM_COLOR; i++)
      if (char_get_player_specials(ch)->color_choices[i])
        free(char_get_player_specials(ch)->color_choices[i]);
  }
  if (!char_is_npc(ch) || (char_is_npc(ch) && char_get_nr(ch) == NOBODY)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (char_get_name(ch))
      free(char_get_name(ch));
    if (char_get_voice(ch))
      free(char_get_voice(ch));
    if (char_get_raw_title(ch))
      free(char_get_raw_title(ch));
    free(char_get_loguser(ch));
    if (char_get_sdesc(ch))
      free(char_get_sdesc(ch));
    if (char_get_ldesc(ch))
      free(char_get_ldesc(ch));
    if (char_get_ddesc(ch))
      free(char_get_ddesc(ch));
    free(const_cast<char *>(char_get_feature(ch)));
    free(const_cast<char *>(char_get_rdisplay(ch)));
    for (i = 0; i < NUM_HIST; i++)
      if (char_get_comm_hist(g, ch, i))
        delete char_get_comm_hist(g, ch, i);
    if (char_get_player_specials(ch))
      delete char_get_player_specials(ch);

    /* free script proto list */
    free_proto_script(ch, MOB_TRIGGER);

  } else if ((nr = char_get_nr(ch)) != NOBODY) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (char_get_alias(ch) && char_get_alias(ch) != char_get_alias(&g->mob_proto[nr]))
      free(char_get_alias(ch));
    if (char_get_raw_title(ch) && char_get_raw_title(ch) != char_get_raw_title(&g->mob_proto[nr]))
      free(char_get_raw_title(ch));
    if (char_get_sdesc(ch) && char_get_sdesc(ch) != char_get_sdesc(&g->mob_proto[nr]))
      free(char_get_sdesc(ch));
    if (char_get_ldesc(ch) && char_get_ldesc(ch) != char_get_ldesc(&g->mob_proto[nr]))
      free(char_get_ldesc(ch));
    if (char_get_ddesc(ch) && char_get_ddesc(ch) != char_get_ddesc(&g->mob_proto[nr]))
      free(char_get_ddesc(ch));
    /* free script proto list if it's not the prototype */
    if (char_get_proto_script(ch) && char_get_proto_script(ch) != char_get_proto_script(&g->mob_proto[nr]))
      free_proto_script(ch, MOB_TRIGGER);
  }
  /* free any assigned scripts */
  if (char_get_script(ch))
    extract_script(g, ch, MOB_TRIGGER);

  /* new version of free_followers take the followers pointer as arg */
  free_followers(char_get_followers(ch));

  if (char_get_desc(ch))
    char_get_desc(ch)->character = nullptr;

  /* find_char helper */
  /*
  * when free_char is called with a blank character struct, ID is set
  * to 0, and has not yet been added to the lookup table.
  */
  if (char_get_id(ch) != 0)
    remove_from_lookup_table(g, char_get_id(ch));

  delete ch;
}

/* release memory allocated for an obj struct */
void free_obj(GameWorld *g, struct obj_data *obj)
{
  remove_unique_id(g, obj);
  if (obj_get_item_number(obj) == NOWHERE) {
    free_object_strings(g, obj);
    /* free script proto list */
    free_proto_script(obj, OBJ_TRIGGER);
  } else {
    free_object_strings_proto(g, obj);
    if (obj_get_proto_script(obj) != obj_get_proto_script(obj_proto_of(g, obj_get_item_number(obj))))
      free_proto_script(obj, OBJ_TRIGGER);
  }

  /* Let's make sure that we free up this memory */
  if (obj_get_auctname(obj)) {
    free(obj_get_auctname(obj));
  }

  /* free any assigned scripts */
  if (obj_get_script(obj))
    extract_script(g, obj, OBJ_TRIGGER);

  /* find_obj helper */
  remove_from_lookup_table(g, obj_get_id(obj));

  if (obj_get_sbinfo(obj))
    delete[] obj_get_sbinfo(obj);

  delete obj;
}

/* Read a text file into a freshly allocated string at *buf, replacing (and freeing) what was there. On failure
 * *buf is left untouched. */
static int file_to_string_alloc(const char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];

  if (file_to_string(name, temp) < 0)
    return (-1);

  if (*buf)
    free(*buf);

  *buf = strdup(temp);
  return (0);
}

/* read contents of a text file, and place in buf */
static int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE + 3];
  int len;

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    mud_log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }

  for (;;) {
    if (!fgets(tmp, READ_SIZE, fl)) /* EOF check */
      break;
    if ((len = strlen(tmp)) > 0)
      tmp[len - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");   /* strcat: OK (tmp:READ_SIZE+3) */

    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
      mud_log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
      *buf = '\0';
      fclose(fl);
      return (-1);
    }
    strcat(buf, tmp); /* strcat: OK (size checked above) */
  }

  fclose(fl);

  return (0);
}

/* clear some of the the working variables of a char */
void reset_char(GameWorld *g, struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    char_set_equipment(ch, i, nullptr);

  char_set_followers(ch, nullptr);
  char_set_master(ch, nullptr);
  char_set_in_room(ch, NOWHERE);
  char_set_inventory(ch, nullptr);
  char_set_next(ch, nullptr);
  char_set_next_fighting(ch, nullptr);
  char_set_next_in_room(ch, nullptr);
  char_set_fighting(ch, nullptr);
  char_set_position(ch, POS_STANDING);
  char_set_mob_specials_default_pos(ch, POS_STANDING);
  char_set_carry_weight(ch, 0);
  char_set_carry_items(ch, 0);
  char_set_time_logon(ch, game_now(g));

  char_set_last_tell(g, ch, NOBODY);
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  memset((char *)ch, 0, sizeof(struct char_data));

  char_set_in_room(ch, NOWHERE);
  char_set_pfilepos(ch, -1);
  char_set_nr(ch, NOBODY);
  char_set_was_in_room(ch, NOWHERE);
  char_set_position(ch, POS_STANDING);
  char_set_mob_specials_default_pos(ch, POS_STANDING);

  char_set_size(ch, SIZE_UNDEFINED);
}

void clear_object(struct obj_data *obj)
{
  memset((char *)obj, 0, sizeof(struct obj_data));

  obj_set_item_number(obj, NOTHING);
  obj_set_in_room(obj, NOWHERE);
  obj_set_worn_on(obj, NOWHERE);
}

/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(GameWorld *g, struct char_data *ch)
{
  int i;

  /* create a player_special structure */
  if (char_get_player_specials(ch) == nullptr) {
    *char_player_specials_slot(ch) = new player_special_data();
  }

  char_set_admlevel(ch, ADMLVL_NONE);
  char_set_absorbs(ch, 0);
  char_set_absorbing(ch, nullptr);
  char_set_absorbby(ch, nullptr);
  char_set_sits(ch, nullptr);
  char_set_blocked(ch, nullptr);
  char_set_blocks(ch, nullptr);

  /* If this is our first player make him LVL_IMPL. */
  if (g->top_of_p_table == 0) {
    admin_set(g, ch, ADMLVL_IMPL);

    /* The implementor never goes through do_start(). */
    char_set_baseki(ch, 1000);
    char_set_basepl(ch, 1000);
    char_set_basest(ch, 1000);
  }

  set_title(g, ch, nullptr);
  char_set_sdesc(ch, nullptr);
  char_set_ldesc(ch, nullptr);
  char_set_ddesc(ch, nullptr);

  /*ch->time.birth = time(0) - birth_age(ch);*/
  char_set_time_created(ch, game_now(g));
  char_set_time_logon(ch, char_get_time_created(ch));
  char_set_time_maxage(ch, char_get_time_birth(ch) + max_age(ch));
  char_set_time_played(ch, 0);

  char_set_hometown(ch, 1);
  char_set_armor(ch, 0);

  set_height_and_weight_by_race(g, ch);

  if ((i = get_ptable_by_name(g, char_get_name(ch))) != -1) {
    char_set_idnum(ch, ++g->top_idnum);
    g->player_table[i].id = char_get_idnum(ch);
  } else
    mud_log("SYSERR: init_char: Character '%s' not found in player table.", char_get_name(ch));

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (char_get_admlevel(ch) < ADMLVL_IMPL)
      char_set_skills_base(ch, i, 0);
    else
      char_set_skills_base(ch, i, 100);
    char_set_skills_mod(ch, i, 0);
  }

  for (i = 0; i < AF_ARRAY_MAX; i++)
    char_get_affected_by(ch)[i] = 0;

  for (i = 0; i < 3; i++)
    char_set_conditions(g, ch, i, (char_get_admlevel(ch) == ADMLVL_IMPL ? -1 : 24));

  char_set_load_room(g, ch, NOWHERE);
  char_set_speaking(ch, SKILL_LANG_COMMON);
  char_set_feat_points(g, ch, 1);
  // initialize plrobjs so it doesn't complain on startup.
  Crash_crashsave(g, ch);
}

/*
 * The world, mobile, object and zone tables are sorted by vnum (check_sorted() makes sure at boot,
 * and OLC inserts in order), so lookups are binary searches.
 */
static int vnum_order(IDXTYPE key, IDXTYPE vnum)
{
  return (key > vnum) - (key < vnum);
}

static int room_order(const void *key, const void *room)
{
  return vnum_order(*(const room_vnum *)key, room_get_number((const struct room_data *)room));
}

static int index_order(const void *key, const void *index)
{
  return vnum_order(*(const IDXTYPE *)key, ((const struct index_data *)index)->vnum);
}

/* The index in table (count entries of size bytes) of the entry for vnum, or NOWHERE. */
static IDXTYPE find_vnum(IDXTYPE vnum, const void *table, IDXTYPE count, size_t size, int (*order)(const void *, const void *))
{
  const char *found;

  if (!table || count <= 0)
    return NOWHERE;
  found = (const char *)bsearch(&vnum, table, count, size, order);
  return found ? (IDXTYPE)((found - (const char *)table) / size) : NOWHERE;
}

/* returns the real number of the room with given virtual number */
room_rnum real_room(GameWorld *g, room_vnum vnum)
{
  return find_vnum(vnum, g->rooms, g->top_room + 1, sizeof(*g->rooms), room_order);
}

/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(GameWorld *g, mob_vnum vnum)
{
  return find_vnum(vnum, g->mob_index, g->top_mob + 1, sizeof(*g->mob_index), index_order);
}

/* returns the real number of the object with given virtual number */
obj_vnum real_object(GameWorld *g, obj_vnum vnum)
{
  return g->obj_index.find(vnum) ? vnum : NOTHING; /* rnums are vnums now */
}

/* returns the real number of the zone with given virtual number */
struct zone_data *find_zone(GameWorld *g, zone_vnum vnum)
{
  return g->zones.find(vnum);
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
static int check_object(GameWorld *g, struct obj_data *obj)
{
  char objname[MAX_INPUT_LENGTH + 32];
  int error = false;

  if (obj_get_weight(obj) < 0 && (error = true))
    mud_log("SYSERR: Object #%d (%s) has negative weight (%" PRId64 ").",
            obj_get_vnum(g, obj), obj_get_short_description(obj), obj_get_weight(obj));

  if (obj_get_cost_per_day(obj) < 0 && (error = true))
    mud_log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
            obj_get_vnum(g, obj), obj_get_short_description(obj), obj_get_cost_per_day(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", obj_get_vnum(g, obj), obj_get_short_description(obj));

  switch (obj_get_type_flag(obj)) {
    case ITEM_DRINKCON: {
      char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj_get_name(obj), ' ');

      strlcpy(onealias, space ? space + 1 : obj_get_name(obj), sizeof(onealias));
      if (search_block(onealias, drinknames, true) < 0 && (error = true)) {
        //log("SYSERR: Object #%d (%s) doesn't have drink type as last alias. (%s)", GET_OBJ_VNUM(obj), obj->short_description, obj->name);
      }
    }
    [[fallthrough]];
    case ITEM_FOUNTAIN:
      if ((obj_get_value(obj, 0) > 0) && (obj_get_value(obj, 1) > obj_get_value(obj, 0) && (error = true)))
        mud_log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
                obj_get_vnum(g, obj), obj_get_short_description(obj),
                obj_get_value(obj, 1), obj_get_value(obj, 0));
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      error |= check_object_level(g, obj, 0);
      error |= check_object_spell_number(g, obj, 1);
      error |= check_object_spell_number(g, obj, 2);
      error |= check_object_spell_number(g, obj, 3);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      error |= check_object_level(g, obj, 0);
      error |= check_object_spell_number(g, obj, 3);
      if (obj_get_value(obj, 2) > obj_get_value(obj, 1) && (error = true))
        mud_log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
                obj_get_vnum(g, obj), obj_get_short_description(obj),
                obj_get_value(obj, 2), obj_get_value(obj, 1));
      break;
  }

  return (error);
}

static int check_object_spell_number(GameWorld *g, struct obj_data *obj, int val)
{
  int error = false;
  const char *spellname;

  if (obj_get_value(obj, val) == -1 ||
      obj_get_value(obj, val) == 0) /* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (obj_get_value(obj, val) < 0)
    error = true;
  if (obj_get_value(obj, val) >= SKILL_TABLE_SIZE)
    error = true;
  if (skill_type(g, obj_get_value(obj, val)) != SKTYPE_SPELL)
    error = true;
  if (error)
    mud_log("SYSERR: Object #%d (%s) has out of range spell #%d.",
            obj_get_vnum(g, obj), obj_get_short_description(obj), obj_get_value(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (obj_get_type_flag(obj) == ITEM_STAFF &&
	spell_has_routine(g, obj_get_value(obj, val), MAG_AREAS | MAG_MASSES))
    mud_log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	obj_get_vnum(g, obj),
	spell_has_routine(g, obj_get_value(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(g, obj_get_value(obj, val)));
#endif

  if (g->scheck) /* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(g, obj_get_value(obj, val));

  if ((spellname == unused_spellname || !strcasecmp("UNDEFINED", spellname)) && (error = true))
    mud_log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
            obj_get_vnum(g, obj), obj_get_short_description(obj), spellname,
            obj_get_value(obj, val));

  return (error);
}

static int check_object_level(GameWorld *g, struct obj_data *obj, int val)
{
  int error = false;

  if ((obj_get_value(obj, val) < 0) && (error = true))
    mud_log("SYSERR: Object #%d (%s) has out of range level #%d.",
            obj_get_vnum(g, obj), obj_get_short_description(obj), obj_get_value(obj, val));

  return (error);
}

static bool obj_save_fprintf(GameWorld *g, FILE *fp, struct obj_data *obj, const char *context,
                             const char *file, int line, const char *format, ...)
    __attribute__((format(printf, 7, 8)));

static bool obj_save_fprintf(GameWorld *g, FILE *fp, struct obj_data *obj, const char *context,
                             const char *file, int line, const char *format, ...)
{
  va_list args;
  int result;

  va_start(args, format);
  result = vfprintf(fp, format, args);
  va_end(args);

  if (result < 0 || ferror(fp)) {
    int saved_errno = errno;
    mud_log("SYSERR: %s:%d: fprintf failed saving object #%d (%s): %s",
            file, line, obj_get_vnum(g, obj), context,
            saved_errno ? strerror(saved_errno) : "stream error");
    return false;
  }

  return true;
}

#define OBJ_SAVE_FPRINTF(g, fp, obj, context, ...) \
  obj_save_fprintf(g, (fp), (obj), (context), __FILE__, __LINE__, __VA_ARGS__)

bool my_obj_save_to_disk(GameWorld *g, FILE *fp, struct obj_data *obj, int locate)
{
  int counter2, i;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH + 1];
  char ebuf0[MAX_STRING_LENGTH], ebuf1[MAX_STRING_LENGTH];
  char ebuf2[MAX_STRING_LENGTH], ebuf3[MAX_STRING_LENGTH];

  if (obj_get_action_description(obj)) {
    strcpy(buf1, obj_get_action_description(obj));
    strip_cr(buf1);
  } else
    *buf1 = 0;

  sprintascii(ebuf0, obj_get_extra_flags(obj)[0]);
  sprintascii(ebuf1, obj_get_extra_flags(obj)[1]);
  sprintascii(ebuf2, obj_get_extra_flags(obj)[2]);
  sprintascii(ebuf3, obj_get_extra_flags(obj)[3]);

  if (!OBJ_SAVE_FPRINTF(g, fp, obj, "base object data",
                        "#%d\n"
                        "%d %d %d %d %d %d %d %d %d %s %s %s %s %d %d %d %d %d %d %d %d\n",
                        obj_get_vnum(g, obj), locate, obj_get_value(obj, 0), obj_get_value(obj, 1),
                        obj_get_value(obj, 2), obj_get_value(obj, 3), obj_get_value(obj, 4),
                        obj_get_value(obj, 5), obj_get_value(obj, 6), obj_get_value(obj, 7),
                        ebuf0, ebuf1, ebuf2, ebuf3,
                        obj_get_value(obj, 8), obj_get_value(obj, 9),
                        obj_get_value(obj, 10), obj_get_value(obj, 11), obj_get_value(obj, 12),
                        obj_get_value(obj, 13), obj_get_value(obj, 14), obj_get_value(obj, 15)))
    return false;

  /* An early "return 1" for non-unique, non-spellbook objects used to sit here, but its
   * condition (!type == ITEM_SPELLBOOK) was always false, so every object saves its XAP data. */

  if (!OBJ_SAVE_FPRINTF(g, fp, obj, "XAP object data",
                        "XAP\n"
                        "%s~\n"
                        "%s~\n"
                        "%s~\n"
                        "%s~\n"
                        "%d %d %d %d %d %" PRId64 " %d %d\n",
                        obj_get_name(obj) ? obj_get_name(obj) : "undefined",
                        obj_get_short_description(obj) ? obj_get_short_description(obj) : "undefined",
                        obj_get_description(obj) ? obj_get_description(obj) : "undefined",
                        buf1, obj_get_type_flag(obj), (int)obj_get_wear_flags(obj)[0], /* as signed, as always */
                        (int)obj_get_wear_flags(obj)[1], (int)obj_get_wear_flags(obj)[2], (int)obj_get_wear_flags(obj)[3],
                        obj_get_weight(obj), obj_get_cost(obj), obj_get_cost_per_day(obj)))
    return false;

  if (obj_get_generation(obj))
    if (!OBJ_SAVE_FPRINTF(g, fp, obj, "generation", "G\n%lld\n", (long long)obj_get_generation(obj)))
      return false;
  if (obj_get_unique_id(obj))
    if (!OBJ_SAVE_FPRINTF(g, fp, obj, "unique id", "U\n%" PRId64 "\n", obj_get_unique_id(obj)))
      return false;

  if (!OBJ_SAVE_FPRINTF(g, fp, obj, "size", "Z\n%d\n", obj_get_size(obj)))
    return false;

  /* Do we have affects? */
  for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
    if (obj_get_affected(obj)[counter2].modifier)
      if (!OBJ_SAVE_FPRINTF(g, fp, obj, "affect", "A\n"
                                               "%d %d %d\n",
                            obj_get_affected(obj)[counter2].location, obj_get_affected(obj)[counter2].modifier,
                            obj_get_affected(obj)[counter2].specific))
        return false;

  /* Do we have extra descriptions? */
  if (obj_get_ex_description(obj)) { /*. Yep, save them too . */
    for (ex_desc = obj_get_ex_description(obj); ex_desc; ex_desc = ex_desc->next) {
      /*. Sanity check to prevent nasty protection faults . */
      if (!*ex_desc->keyword || !*ex_desc->description) {
        continue;
      }
      strcpy(buf1, ex_desc->description);
      strip_cr(buf1);
      if (!OBJ_SAVE_FPRINTF(g, fp, obj, "extra description", "E\n"
                                                          "%s~\n"
                                                          "%s~\n",
                            ex_desc->keyword,
                            buf1))
        return false;
    }
  }

  /* Do we have spells? */
  if (obj_get_sbinfo(obj)) { /*. Yep, save them too . */
    for (i = 0; i < SPELLBOOK_SIZE; i++) {
      if (obj_get_sbinfo(obj)[i].spellname == 0) {
        break;
      }
      if (!OBJ_SAVE_FPRINTF(g, fp, obj, "spellbook", "S\n"
                                                  "%d %d\n",
                            obj_get_sbinfo(obj)[i].spellname, obj_get_sbinfo(obj)[i].pages))
        return false;
      continue;
    }
  }
  return true;
}

#undef OBJ_SAVE_FPRINTF

/* External variables from config.c */

static void load_default_config(GameWorld *g)
{
  /****************************************************************************/
  /** This function is called only once, at boot-time.                       **/
  /** - We assume config_info is empty                          -- Welcor    **/
  /****************************************************************************/
  /****************************************************************************/
  /** Game play options.                                                     **/
  /****************************************************************************/
  CONFIG_PK_ALLOWED             = pk_allowed;
  CONFIG_PT_ALLOWED             = pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT        = level_can_shout;
  CONFIG_HOLLER_MOVE_COST       = holler_move_cost;
  CONFIG_TUNNEL_SIZE            = tunnel_size;
  CONFIG_MAX_EXP_GAIN           = max_exp_gain;
  CONFIG_MAX_EXP_LOSS           = max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME    = max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME     = max_pc_corpse_time;
  CONFIG_IDLE_VOID              = idle_void;
  CONFIG_IDLE_RENT_TIME         = idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL         = idle_max_level;
  CONFIG_DTS_ARE_DUMPS          = dts_are_dumps;
  CONFIG_LOAD_INVENTORY         = load_into_inventory;
  CONFIG_OK                     = strdup(OK);
  CONFIG_NOPERSON               = strdup(NOPERSON);
  CONFIG_NOEFFECT               = strdup(NOEFFECT);
  CONFIG_TRACK_T_DOORS          = track_through_doors;
  CONFIG_LEVEL_CAP              = level_cap;
  CONFIG_STACK_MOBS             = show_mob_stacking;
  CONFIG_STACK_OBJS             = show_obj_stacking;
  CONFIG_MOB_FIGHTING           = mob_fighting;
  CONFIG_DISP_CLOSED_DOORS      = disp_closed_doors;
  CONFIG_REROLL_PLAYER_CREATION = reroll_status;
  CONFIG_INITIAL_POINTS_POOL    = initial_points;
  CONFIG_ENABLE_COMPRESSION     = enable_compression;
  CONFIG_ENABLE_LANGUAGES       = enable_languages;
  CONFIG_ALL_ITEMS_UNIQUE       = all_items_unique;
  CONFIG_EXP_MULTIPLIER         = g->exp_multiplier;
  /****************************************************************************/
  /** Rent / crashsave options.                                              **/
  /****************************************************************************/
  CONFIG_FREE_RENT     = free_rent;
  CONFIG_MAX_OBJ_SAVE  = max_obj_save;
  CONFIG_MIN_RENT_COST = min_rent_cost;
  CONFIG_AUTO_SAVE     = auto_save;
  CONFIG_AUTOSAVE_TIME = autosave_time;
  CONFIG_CRASH_TIMEOUT = crash_file_timeout;
  CONFIG_RENT_TIMEOUT  = rent_file_timeout;

  /****************************************************************************/
  /** Room numbers.                                                          **/
  /****************************************************************************/
  CONFIG_MORTAL_START   = mortal_start_room;
  CONFIG_IMMORTAL_START = immort_start_room;
  CONFIG_FROZEN_START   = frozen_start_room;
  CONFIG_DON_ROOM_1     = donation_room_1;
  CONFIG_DON_ROOM_2     = donation_room_2;
  CONFIG_DON_ROOM_3     = donation_room_3;

  /****************************************************************************/
  /** Game operation options.                                                **/
  /****************************************************************************/
  CONFIG_DFLT_PORT = DFLT_PORT;

  if (DFLT_IP)
    CONFIG_DFLT_IP = strdup(DFLT_IP);
  else
    CONFIG_DFLT_IP = nullptr;

  CONFIG_DFLT_DIR = strdup(DFLT_DIR);

  if (LOGNAME)
    CONFIG_LOGNAME = strdup(LOGNAME);
  else
    CONFIG_LOGNAME = nullptr;

  CONFIG_MAX_PLAYING    = max_playing;
  CONFIG_MAX_FILESIZE   = max_filesize;
  CONFIG_MAX_BAD_PWS    = max_bad_pws;
  CONFIG_SITEOK_ALL     = siteok_everyone;
  CONFIG_NS_IS_SLOW     = nameserver_is_slow;
  CONFIG_NEW_SOCIALS    = use_new_socials;
  CONFIG_OLC_SAVE       = auto_save_olc;
  CONFIG_MENU           = strdup(MENU);
  CONFIG_WELC_MESSG     = strdup(WELC_MESSG);
  CONFIG_START_MESSG    = strdup(START_MESSG);
  CONFIG_EXP_MULTIPLIER = 1.0;

  /****************************************************************************/
  /** Autowiz options.                                                       **/
  /****************************************************************************/
  CONFIG_USE_AUTOWIZ     = use_autowiz;
  CONFIG_MIN_WIZLIST_LEV = min_wizlist_lev;

  /****************************************************************************/
  /** Character advancement options.                                         **/
  /****************************************************************************/
  CONFIG_ALLOW_MULTICLASS = allow_multiclass;
  CONFIG_ALLOW_PRESTIGE   = allow_prestige;

  /****************************************************************************/
  /** ticks menu                                                             **/
  /****************************************************************************/
  CONFIG_PULSE_VIOLENCE = pulse_violence;
  CONFIG_PULSE_MOBILE   = pulse_mobile;
  CONFIG_PULSE_ZONE     = pulse_zone;
  CONFIG_PULSE_CURRENT  = pulse_current;
  CONFIG_PULSE_SANITY   = pulse_sanity;
  CONFIG_PULSE_IDLEPWD  = pulse_idlepwd;
  CONFIG_PULSE_AUTOSAVE = pulse_autosave;
  CONFIG_PULSE_USAGE    = pulse_usage;
  CONFIG_PULSE_TIMESAVE = pulse_timesave;

  /****************************************************************************/
  /** Character Creation Method                                              **/
  /****************************************************************************/
  CONFIG_CREATION_METHOD = method;
}

void load_config(GameWorld *g)
{
  FILE *fl;
  char line[MAX_STRING_LENGTH];
  char tag[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];

  load_default_config(g);

  snprintf(buf, sizeof(buf), "%s/%s", DFLT_DIR, CONFIG_CONFFILE);
  if (!(fl = fopen(CONFIG_CONFFILE, "r")) && !(fl = fopen(buf, "r"))) {
    snprintf(buf, sizeof(buf), "Game Config File: %s", CONFIG_CONFFILE);
    perror(buf);
    return;
  }

  /****************************************************************************/
  /** Load the game configuration file.                                      **/
  /****************************************************************************/
  while (get_line(fl, line)) {
    split_argument(line, tag);
    config_load_option(g, tag, line, fl);
  }
  if (!CONFIG_DFLT_DIR) /* an empty DFLT_DIR line */
    CONFIG_DFLT_DIR = strdup(DFLT_DIR);

  fclose(fl);
}
