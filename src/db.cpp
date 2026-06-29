/*************************************************************************
 *   File: db.c                                          Part of CircleMUD *
 *  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "db.h"
#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "command_db.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/applies.h"
#include "consts/constates.h"
#include "consts/directions.h"
#include "consts/exitflags.h"
#include "consts/itemdata.h"
#include "consts/mobflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "consts/sizes.h"
#include "consts/triggers.h"
#include "consts/weapons.h"
#include "consts/weather.h"
#include "context_help.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dgscript_impl.h"
#include "flags.h"
#include "oasis.h"
#include "object_api.h"
#include "object_impl.h"
#include "object_macros.h"
#include "player_db.h"
#include "player_impl.h"
#include "races.h"
#include "room_api.h"
#include "room_db.h"
#include "room_impl.h"
#include "room_utils.h"
#include "skills.h"
#include "stringutils.h"
#include "time_info.h"
#include "util_macros.h"
#include "weather_db.h"
#include "zone_impl.h"

#include "consts/maximums.h"
#include <string.h>
#include <strings.h>

#include "help.h"

#include "extract.h"
#include "fileop.h"
#include "json.h"
#include "players.h"
#include "relocate.h"
#include "search.h"
#include "time.h"
#include "weather.h"
#include "xdir.h"

#include "act.informative.h"
#include "act.other.h"
#include "act.social.h"
#include "assemblies.h"
#include "ban.h"
#include "boards.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "config.h"
#include "dg_scripts.h"
#include "feats.h"
#include "genmob.h"
#include "genobj.h"
#include "genolc.h"
#include "handler.h"
#include "house.h"
#include "interpreter.h"
#include "local_limits.h"
#include "log.h"
#include "mail.h"
#include "modify.h"
#include "objsave.h"
#include "players.h"
#include "races_plus.h"
#include "random.h"
#include "reset.h"
#include "sensei.h"
#include "shop.h"
#include "spec_assign.h"
#include "spell_parser.h"
#include "spells.h"

#include "iterate.hpp"
#include "mobact.h"

#include <errno.h>
#include "event_queue_api.h"

#include <linux/limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unordered_map>

// Forward declarations for zone reset event functions (defined later in this file)
void ev_zone_reset(int, int64_t, int64_t);
void zone_schedule_reset(struct zone_data *zone);
void zone_schedule_all_resets(void);

/**************************************************************************
 *  declarations of most of the 'global' variables                         *
 **************************************************************************/

int dg_owner_purged; /* For control of scripts */

int no_mail = 0;                  /* mail disabled?		 */
int mini_mud = 0;                 /* mini-mud mode?		 */
int no_rent_check = 0;            /* skip rent check on boot?	 */
time_t boot_time = 0;             /* time of mud boot		 */
int circle_restrict = 0;          /* level of game restriction	 */
int dballtime = 0;                /* used by dragonball load system*/
int SHENRON = FALSE;              /* Shenron has been summoned     */
int DRAGONR = 0;                  /* Room Shenron has been summoned to */
int DRAGONZ = 0;                  /* Zone Shenron has been summoned to */
int WISH[2] = {0, 0};             /* Keeps track of wishes granted */
int DRAGONC = 0;                  /* Keeps count of Shenron's remaining time */
struct char_data *EDRAGON = NULL; /* This is Shenron when he is loaded */
room_rnum r_mortal_start_room;    /* rnum of mortal start room	 */
room_rnum r_immort_start_room;    /* rnum of immort start room	 */
room_rnum r_frozen_start_room;    /* rnum of frozen start room	 */
int xap_objs = 0;                 /* Xap objs                      */
int converting = FALSE;

char *credits = NULL;    /* game credits			 */
char *news = NULL;       /* mud news			 */
char *motd = NULL;       /* message of the day - mortals  */
char *imotd = NULL;      /* message of the day - immorts  */
char *GREETINGS = NULL;  /* opening credits screen	 */
char *GREETANSI = NULL;  /* ansi opening credits screen	 */
char *help = NULL;       /* help screen			 */
char *info = NULL;       /* info page			 */
char *wizlist = NULL;    /* list of higher gods		 */
char *immlist = NULL;    /* list of peon gods		 */
char *background = NULL; /* background story		 */
char *handbook = NULL;   /* handbook for new immortals	 */
char *policies = NULL;   /* policies page		 */
char *ihelp = NULL;      /* help screen (immortals)	 */

extern struct board_info *boards; /* our boards */

/* local functions */
static void mob_stats(struct char_data *mob);
static void dragon_level(struct char_data *ch);
static int check_bitvector_names(bitvector_t bits, size_t namecount,
                                 const char *whatami, const char *whatbits);
static int check_object_spell_number(struct obj_proto_data *obj, int val);
static int check_object_level(struct obj_proto_data *obj, int val);
static void setup_dir(FILE *fl, struct room_data *room, int dir);
static void discrete_load(FILE *fl, int mode, char *filename);
static int check_object(struct obj_proto_data *);
static void parse_room(FILE *fl, int virtual_nr);
static void parse_mobile(FILE *mob_f, int nr);
static char *parse_object(FILE *obj_f, int nr);
static void load_zones(FILE *fl, char *zonename);
static int file_to_string(const char *name, char *buf);
static int file_to_string_alloc(const char *name, char **buf);
static int count_alias_records(FILE *fl);
static int count_hash_records(FILE *fl);
static bitvector_t asciiflag_conv_aff(char *flag);
static int parse_simple_mob(FILE *mob_f, struct char_data *ch, int nr);
static void interpret_espec(const char *keyword, const char *value,
                            struct char_data *ch, int nr);
static void parse_espec(char *buf, struct char_data *ch, int nr);
static int parse_enhanced_mob(FILE *mob_f, struct char_data *ch, int nr);
static void get_one_line(FILE *fl, char *buf);
static void check_start_rooms(void);
static void log_zone_error(struct zone_data *zone, int cmd_no,
                           const char *message);
static void reset_time(void);
static int suntzu_armor_convert(struct obj_data *obj);
static int suntzu_weapon_convert(int wp_type);
static void mob_autobalance(struct char_data *ch);
static bool directory_exists(const char *path);
static void json_import_or_die(const char *label, int result);

/* external functions */

void mag_assign_spells(void);
void create_command_list(void);
void sort_spells(void);
int hsort(const void *a, const void *b);
void prune_crlf(char *txt);
void build_player_index(void);
void clean_pfiles(void);
void boot_the_guilds(FILE *gm_f, char *filename, int rec_count);
void destroy_guilds(void);
void assign_the_guilds(void);
void memorize_add(struct char_data *ch, int spellnum, int timer);
void assign_feats(void);
void free_feats(void);
void sort_feats(void);
void free_assemblies(void);

/* external vars */
extern int no_specials;
extern int scheck;

extern long top_idnum;

static void dragon_level(struct char_data *ch) {
  struct descriptor_data *d;
  int level = 0, count = 0;

  for (d = descriptor_list; d; d = d->next) {
    if (IS_PLAYING(d) && GET_ADMLEVEL(d->character) < 1) {
      level += GET_LEVEL(d->character);
      count += 1;
    }
  }

  if (level > 0 && count > 0) {
    level = level / count;
  } else {
    level = rand_number(60, 110);
  }

  if (level < 50) {
    level = rand_number(40, 60);
  }

  char_stat_mod(ch, "level", rand_number(5, 20));
}

static void mob_stats(struct char_data *mob) {
  int start = GET_LEVEL(mob) * 0.5, finish = GET_LEVEL(mob);

  if (finish < 20)
    finish = 20;

  if (!IS_HUMANOID(mob)) {
    char_stat_set(mob, "strength", rand_number(start, finish));
    char_stat_set(mob, "intelligence", rand_number(start, finish) - 30);
    char_stat_set(mob, "wisdom", rand_number(start, finish) - 30);
    char_stat_set(mob, "agility", rand_number(start + 5, finish));
    char_stat_set(mob, "constitution", rand_number(start + 5, finish));
    char_stat_set(mob, "speed", rand_number(start, finish));
  } else {
    if (IS_SAIYAN(mob)) {
      char_stat_set(mob, "strength", rand_number(start + 10, finish));
      char_stat_set(mob, "intelligence", rand_number(start, finish - 10));
      char_stat_set(mob, "wisdom", rand_number(start, finish - 5));
      char_stat_set(mob, "agility", rand_number(start, finish));
      char_stat_set(mob, "constitution", rand_number(start + 5, finish));
      char_stat_set(mob, "speed", rand_number(start + 5, finish));
    } else if (IS_KONATSU(mob)) {
      char_stat_set(mob, "strength", rand_number(start, finish - 10));
      char_stat_set(mob, "intelligence", rand_number(start, finish));
      char_stat_set(mob, "wisdom", rand_number(start, finish));
      char_stat_set(mob, "agility", rand_number(start + 10, finish));
      char_stat_set(mob, "constitution", rand_number(start, finish));
      char_stat_set(mob, "speed", rand_number(start, finish));
    } else if (IS_ANDROID(mob)) {
      char_stat_set(mob, "strength", rand_number(start, finish));
      char_stat_set(mob, "intelligence", rand_number(start, finish));
      char_stat_set(mob, "wisdom", rand_number(start, finish - 10));
      char_stat_set(mob, "agility", rand_number(start, finish));
      char_stat_set(mob, "constitution", rand_number(start, finish));
      char_stat_set(mob, "speed", rand_number(start, finish));
    } else if (IS_MAJIN(mob)) {
      char_stat_set(mob, "strength", rand_number(start, finish));
      char_stat_set(mob, "intelligence", rand_number(start, finish - 10));
      char_stat_set(mob, "wisdom", rand_number(start, finish - 5));
      char_stat_set(mob, "agility", rand_number(start, finish));
      char_stat_set(mob, "constitution", rand_number(start + 15, finish));
      char_stat_set(mob, "speed", rand_number(start, finish));
    } else if (IS_TRUFFLE(mob)) {
      char_stat_set(mob, "strength", rand_number(start, finish - 10));
      char_stat_set(mob, "intelligence", rand_number(start + 15, finish));
      char_stat_set(mob, "wisdom", rand_number(start, finish));
      char_stat_set(mob, "agility", rand_number(start, finish));
      char_stat_set(mob, "constitution", rand_number(start, finish));
      char_stat_set(mob, "speed", rand_number(start, finish));
    } else if (IS_ICER(mob)) {
      char_stat_set(mob, "strength", rand_number(start + 5, finish));
      char_stat_set(mob, "intelligence", rand_number(start, finish));
      char_stat_set(mob, "wisdom", rand_number(start, finish));
      char_stat_set(mob, "agility", rand_number(start, finish));
      char_stat_set(mob, "constitution", rand_number(start, finish));
      char_stat_set(mob, "speed", rand_number(start + 10, finish));
    } else {
      char_stat_set(mob, "strength", rand_number(start, finish));
      char_stat_set(mob, "intelligence", rand_number(start, finish));
      char_stat_set(mob, "wisdom", rand_number(start, finish));
      char_stat_set(mob, "agility", rand_number(start, finish));
      char_stat_set(mob, "constitution", rand_number(start, finish));
      char_stat_set(mob, "speed", rand_number(start, finish));
    }
  }

  if (char_stat_get(mob, "strength") > 100)
    char_stat_set(mob, "strength", 100);
  else if (char_stat_get(mob, "strength") < 5)
    char_stat_set(mob, "strength", rand_number(5, 8));

  if (char_stat_get(mob, "intelligence") > 100)
    char_stat_set(mob, "intelligence", 100);
  else if (char_stat_get(mob, "intelligence") < 5)
    char_stat_set(mob, "intelligence", rand_number(5, 8));

  if (char_stat_get(mob, "wisdom") > 100)
    char_stat_set(mob, "wisdom", 100);
  else if (char_stat_get(mob, "wisdom") < 5)
    char_stat_set(mob, "wisdom", rand_number(5, 8));

  if (char_stat_get(mob, "constitution") > 100)
    char_stat_set(mob, "constitution", 100);
  else if (char_stat_get(mob, "constitution") < 5)
    char_stat_set(mob, "constitution", rand_number(5, 8));

  if (char_stat_get(mob, "speed") > 100)
    char_stat_set(mob, "speed", 100);
  else if (char_stat_get(mob, "speed") < 5)
    char_stat_set(mob, "speed", rand_number(5, 8));

  if (char_stat_get(mob, "agility") > 100)
    char_stat_set(mob, "agility", 100);
  else if (char_stat_get(mob, "agility") < 5)
    char_stat_set(mob, "agility", rand_number(5, 8));
}

/* Convert CWG-SunTzu armor objects to new armor types */

static int suntzu_armor_convert(struct obj_data *obj) {
  int i;
  int conv = 0;
  int conv_table[][3] = {
      {100, 0, 0}, {8, 0, 5},  {6, 0, 10}, {5, 1, 15}, {4, 2, 20},
      {2, 5, 30},  {0, 7, 40}, {0, 7, 40}, {1, 6, 35},
  };
  int shield_table[][2] = {
      {0, 0},  {1, 5},  {2, 15}, {3, 30}, {4, 40},
      {5, 50}, {6, 60}, {7, 70}, {8, 80},
  };

  i = GET_OBJ_VAL(obj, 0);
  if (i && i < 10) {
    GET_OBJ_VAL(obj, 0) = 10 * i;
    conv = 1;
  } else
    i /= 10;

  i = MAX(0, MIN(8, i));

  if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
    if (GET_OBJ_VAL(obj, 6))
      return conv;
    GET_OBJ_VAL(obj, 1) = ARMOR_TYPE_SHIELD;
    GET_OBJ_VAL(obj, 2) = 100;
    GET_OBJ_VAL(obj, 3) = shield_table[i][0];
    GET_OBJ_VAL(obj, 6) = shield_table[i][1];
    conv = 1;
  } else if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
    if (GET_OBJ_VAL(obj, 6))
      return conv;
    GET_OBJ_VAL(obj, 2) = conv_table[i][0];
    GET_OBJ_VAL(obj, 3) = conv_table[i][1];
    GET_OBJ_VAL(obj, 6) = conv_table[i][2];
    conv = 1;
  } else if (GET_OBJ_VAL(obj, 2) || GET_OBJ_VAL(obj, 3)) {
    return conv;
  } else {
    GET_OBJ_VAL(obj, 2) = 100;
    GET_OBJ_VAL(obj, 3) = 0;
    GET_OBJ_VAL(obj, 6) = 0;
    conv = 1;
  }
  mud_log("Converted armor #%d [%s] armor=%d i=%d maxdex=%d acheck=%d sfail=%d",
      obj->proto_id, GET_OBJ_SHORT(obj), GET_OBJ_VAL(obj, 0), i,
      GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 6));
  return conv;
}

/* Convert CWG-SunTzu weapon objects to new weapon types */

static int suntzu_weapon_convert(int wp_type) {
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
void reboot_wizlists(void) {
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(void) {
  char **textfiles[] = {&wizlist,   &immlist,  &news,       &credits,
                        &motd,      &imotd,    &help,       &info,
                        &policies,  &handbook, &background, &GREETINGS,
                        &GREETANSI, &ihelp,    NULL};
  int rf;

  for (rf = 0; textfiles[rf]; rf++)
    if (*textfiles[rf]) {
      free(*textfiles[rf]);
      *textfiles[rf] = NULL;
    }
}

/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot) {
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!strcasecmp(arg, "all") || *arg == '*') {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
      prune_crlf(GREETANSI);
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0)
      send_to_char(ch, "Cannot read wizlist\r\n");
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0)
      send_to_char(ch, "Cannot read immlist\r\n");
    if (file_to_string_alloc(NEWS_FILE, &news) < 0)
      send_to_char(ch, "Cannot read news\r\n");
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0)
      send_to_char(ch, "Cannot read credits\r\n");
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0)
      send_to_char(ch, "Cannot read motd\r\n");
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0)
      send_to_char(ch, "Cannot read imotd\r\n");
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
    if (file_to_string_alloc(INFO_FILE, &info) < 0)
      send_to_char(ch, "Cannot read info file\r\n");
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0)
      send_to_char(ch, "Cannot read policies\r\n");
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0)
      send_to_char(ch, "Cannot read handbook\r\n");
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0)
      send_to_char(ch, "Cannot read background\r\n");
    if (help_table)
      free_help_table();
    index_boot(DB_BOOT_HLP);
  } else if (!strcasecmp(arg, "wizlist")) {
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0)
      send_to_char(ch, "Cannot read wizlist\r\n");
  } else if (!strcasecmp(arg, "immlist")) {
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0)
      send_to_char(ch, "Cannot read immlist\r\n");
  } else if (!strcasecmp(arg, "news")) {
    if (file_to_string_alloc(NEWS_FILE, &news) < 0)
      send_to_char(ch, "Cannot read news\r\n");
  } else if (!strcasecmp(arg, "credits")) {
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0)
      send_to_char(ch, "Cannot read credits\r\n");
  } else if (!strcasecmp(arg, "motd")) {
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0)
      send_to_char(ch, "Cannot read motd\r\n");
  } else if (!strcasecmp(arg, "imotd")) {
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0)
      send_to_char(ch, "Cannot read imotd\r\n");
  } else if (!strcasecmp(arg, "help")) {
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
  } else if (!strcasecmp(arg, "info")) {
    if (file_to_string_alloc(INFO_FILE, &info) < 0)
      send_to_char(ch, "Cannot read info\r\n");
  } else if (!strcasecmp(arg, "policy")) {
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0)
      send_to_char(ch, "Cannot read policy\r\n");
  } else if (!strcasecmp(arg, "handbook")) {
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0)
      send_to_char(ch, "Cannot read handbook\r\n");
  } else if (!strcasecmp(arg, "background")) {
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0)
      send_to_char(ch, "Cannot read background\r\n");
  } else if (!strcasecmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    else
      send_to_char(ch, "Cannot read greetings.\r\n");
  } else if (!strcasecmp(arg, "greetansi")) {
    if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
      prune_crlf(GREETANSI);
    else
      send_to_char(ch, "Cannot read greetings.\r\n");
  } else if (!strcasecmp(arg, "xhelp")) {
    if (help_table)
      free_help_table();
    index_boot(DB_BOOT_HLP);
  } else if (!strcasecmp(arg, "ihelp")) {
    if (file_to_string_alloc(IHELP_PAGE_FILE, &ihelp) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
  } else {
    send_to_char(ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

void boot_world(void) {
  mud_log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  mud_log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  mud_log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  mud_log("Checking start rooms.");
  check_start_rooms();

  mud_log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  mud_log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  mud_log("Loading disabled commands list...");
  load_disabled();

  if (converting) {
    mud_log("Saving converted worldfiles to disk.");
    save_all();
  }

  if (!no_specials) {
    mud_log("Loading shops.");
    index_boot(DB_BOOT_SHP);

    mud_log("Loading guild masters.");
    index_boot(DB_BOOT_GLD);
  }
  if (SELFISHMETER >= 10) {
    mud_log("Loading Shadow Dragons.");
    load_shadow_dragons();
  }
}

void free_extra_descriptions(struct extra_descr_data *edesc) {
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext) {
    enext = edesc->next;

    free(edesc->keyword);
    free(edesc->description);
    free(edesc);
  }
}

/* Free the world, in a memory allocation sense. */
void destroy_db(void) {
  ssize_t cnt, itr;

  /* Active Mobiles & Players (snapshot: free_char unregisters as we go) */
  char_iterate_all([](struct char_data *chtmp) {
    if (MASTER(chtmp))
      stop_follower(chtmp);
    free_char(chtmp);
    return true;
  });

  /* Active Objects */
  obj_iterate_all([](struct obj_data *objtmp) {
    free_obj(objtmp);
    return true;
  });

  /* Rooms */
  room_iterate([&](auto room) {
    room_name_set(room, NULL);
    room_description_set(room, NULL);
    free_extra_descriptions(room->ex_description);

    /* free any assigned scripts */
    if (room_script_get(room))
      extract_script(room, WLD_TRIGGER);
    /* free script proto list */
    free_proto_script(room, WLD_TRIGGER);
    room_exits_iterate(room, [&](auto i, auto exit) {
      exit_general_description_set(exit, NULL);
      exit_keyword_set(exit, NULL);
      free(exit);
      return true;
    });
    return true;
  });

  /* Objects */
  obj_proto_iterate([&](auto obj) {
    obj_vnum v = obj->id;
    obj_proto_delete(v);
    obj_proto_free(obj);
    return true;
  });

  /* Mobiles */
  mob_proto_iterate([&](auto mob) {
    mob_vnum v = mob->id;
    mob_proto_delete(v);
    mob_proto_free(mob);
    return true;
  });

  /* Shops */
  destroy_shops();

  /* Guilds */
  destroy_guilds();

  /* Zones */
  zone_iterate([&](auto zone) {
    if (zone->name)
      free(zone->name);
    if (zone->builders)
      free(zone->builders);
    if (zone->cmd) {
      /* first see if any vars were defined in this zone */
      for (itr = 0; zone->cmd[itr].command != 'S'; itr++)
        if (zone->cmd[itr].command == 'V') {
          if (zone->cmd[itr].sarg1)
            free(zone->cmd[itr].sarg1);
          if (zone->cmd[itr].sarg2)
            free(zone->cmd[itr].sarg2);
        }
      /* then free the command list */
      free(zone->cmd);
    }
    zone_delete(zone->id);
    free(zone);
    return true;
  });

  /* Triggers */
  trig_proto_iterate([&](auto trig) {
    trig_vnum vnum = trig->proto_id;
    /* make sure to nuke the command list (memory leak) */
    /* free_trigger() doesn't free the command list */
    if (trig->cmdlist) {
      struct cmdlist_element *i, *j;
      i = trig->cmdlist;
      while (i) {
        j = i->next;
        if (i->cmd)
          free(i->cmd);
        free(i);
        i = j;
      }
    }
    trig_proto_delete(vnum);
    free_trigger(trig);
    return true;
  });

  /* context sensitive help system */
  free_context_help();

  free_feats();

  mud_log("Freeing Assemblies.");
  free_assemblies();
}


static bool directory_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void json_import_or_die(const char *label, int result) {
  if (result == 0)
    return;
  mud_log("SYSERR: Failed to import JSON assets: %s", label);
  exit(1);
}

static void load_assets(void) {
  constexpr bool use_json_assets = false;
  constexpr const char *asset_root = "data/assets";

  if (use_json_assets && directory_exists(asset_root)) {
    
    mud_log("Loading JSON zone table.");
    json_import_or_die("zones", json_import_zones("data/assets/zones"));

    mud_log("Loading JSON rooms.");
    json_import_or_die("rooms", json_import_rooms("data/assets/rooms"));

    mud_log("Loading JSON exits.");
    json_import_or_die("exits", json_import_room_exits("data/assets/exits"));



    mud_log("Loading JSON triggers and generating index.");
    json_import_or_die("dgscripts",
                       json_import_dgscripts("data/assets/dgscripts"));

    mud_log("Loading JSON mobs and generating index.");
    json_import_or_die("npc_prototypes", json_import_npc_prototypes(
                                             "data/assets/npc_prototypes"));

    mud_log("Loading JSON objs and generating index.");
    json_import_or_die("obj_prototypes", json_import_obj_prototypes(
                                             "data/assets/obj_prototypes"));

    

    mud_log("Loading JSON shops.");
    json_import_or_die("shops", json_import_shops("data/assets/shops"));

    mud_log("Loading JSON guild masters.");
    json_import_or_die("guilds", json_import_guilds("data/assets/guilds"));

    if (SELFISHMETER >= 10) {
      mud_log("Loading Shadow Dragons.");
      load_shadow_dragons();
    }
  } else {
    boot_world();
    if (!directory_exists(asset_root)) {
      mud_log("Exporting JSON assets.");
      if (json_export_all(asset_root) != 0)
        mud_log("SYSERR: Failed to export JSON assets to %s.", asset_root);
    }
  }

  mud_log("Loading help entries.");
  index_boot(DB_BOOT_HLP);
}

static void load_test_assets() {
  mud_log("Loading TEST JSON zone table.");
  json_import_or_die("zones", json_import_zones("test_assets/zones"));

  mud_log("Loading TEST JSON rooms.");
  json_import_or_die("rooms", json_import_rooms("test_assets/rooms"));

  mud_log("Loading TEST JSON exits.");
  json_import_or_die("exits", json_import_room_exits("test_assets/exits"));

  mud_log("Loading TEST JSON triggers and generating index.");
  json_import_or_die("dgscripts",
                      json_import_dgscripts("test_assets/dgscripts"));

  mud_log("Loading TEST JSON mobs and generating index.");
  json_import_or_die("npc_prototypes", json_import_npc_prototypes(
                                            "test_assets/npc_prototypes"));

  mud_log("Loading TEST JSON objs and generating index.");
  json_import_or_die("obj_prototypes", json_import_obj_prototypes(
                                            "test_assets/obj_prototypes"));

  mud_log("Loading TEST JSON shops.");
  json_import_or_die("shops", json_import_shops("test_assets/shops"));

  mud_log("Loading TEST JSON guild masters.");
  json_import_or_die("guilds", json_import_guilds("test_assets/guilds"));
}

/* body of the booting system */
void boot_db(void) {
  zone_rnum i;

  mud_log("Boot db -- BEGIN.");

  mud_log("Resetting the game time:");
  reset_time();

  mud_log("Reading news, credits, help, ihelp, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(IHELP_PAGE_FILE, &ihelp);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);
  if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
    prune_crlf(GREETANSI);

  mud_log("Loading spell definitions.");
  mag_assign_spells();

  mud_log("Loading feats.");
  assign_feats();

  mud_log("Loading disabled commands list...");
  load_disabled();

  if(config_info.test_mode) {
    mud_log("Test mode enabled -- skipping asset loading.");
    load_test_assets();
  } else {
    load_assets();
    mud_log("Checking start rooms.");
    check_start_rooms();
  }

  mud_log("Setting up context sensitive help system for OLC");
  boot_context_help();

  mud_log("Generating player index.");
  build_player_index();

  if (ERAPLAYERS <= 0)
    ERAPLAYERS = top_of_p_table + 1;

  insure_directory(LIB_PLROBJS "CRASH", 0);

  mud_log("Booting mail system.");
  if (!scan_file()) {
    mud_log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }

  mud_log("Loading social messages.");
  boot_social_messages();

  mud_log("Loading Clans.");
  clanBoot();

  mud_log("Building command list.");
  create_command_list(); /* aedit patch -- M. Scott */

  mud_log("Assigning function pointers:");

  if(!config_info.test_mode) {
    mud_log("   Mobiles.");
    assign_mobiles();
    mud_log("   Objects.");
    assign_objects();
    mud_log("   Rooms.");
    assign_rooms();
  }

  mud_log("   Shopkeepers.");
  assign_the_shopkeepers();
  mud_log("   Guildmasters.");
  assign_the_guilds();

  mud_log("Sorting command list and spells.");
  sort_commands();
  sort_spells();
  sort_feats();

  if(!config_info.test_mode) {

    mud_log("Booting assembled objects.");
    assemblyBootAssemblies();

    mud_log("Booting boards system.");
    init_boards();
  }

  mud_log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    mud_log("Deleting timed-out crash and rent files:");
    update_obj_file();
    mud_log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!config_info.test_mode) {
    mud_log("Booting houses.");
    House_boot();
  }

  zone_iterate([&](auto zone) {
    mud_log("Resetting #%d: %s (rooms %d-%d).", zone->id, zone->name, zone->bot,
        zone->top);
    reset_zone(zone);
    return true;
  });

  boot_time = time(0);

  mud_log("Boot db -- DONE.");
}

/* save the auction file */
void auc_save() {
  FILE *fl;

  if ((fl = fopen(AUCTION_FILE, "w")) == NULL)
    mud_log("SYSERR: Can't write to '%s' auction file.", AUCTION_FILE);
  else {
    struct obj_data *obj, *next_obj;

    room_contents_iterate(room_by_id(80), [&](auto obj) {
      if (obj) {
        fprintf(fl, "%" I64T " %s %d %d %d %d %ld\n", 0,
                GET_AUCTERN(obj), GET_AUCTER(obj), GET_CURBID(obj),
                GET_STARTBID(obj), GET_BID(obj), GET_AUCTIME(obj));
      }
      return true;
    });
    fprintf(fl, "~END~\n");
    fclose(fl);
  }
}

/* load from auction file */
void auc_load(struct obj_data *obj) {
  char line[500], filler[50];
  int64_t oID;
  time_t timer;
  int aID, bID, cost, startc;
  FILE *fl;

  if ((fl = fopen(AUCTION_FILE, "r")) == NULL)
    mud_log("SYSERR: Can't read from '%s' auction file.", AUCTION_FILE);
  else {
    while (!feof(fl)) {
      get_line(fl, line);
      sscanf(line, "%" I64T " %s %d %d %d %d %ld\n", &oID, filler, &aID, &bID,
             &startc, &cost, &timer);
      GET_AUCTERN(obj) = strdup(filler);
      GET_AUCTER(obj) = aID;
      GET_CURBID(obj) = bID;
      GET_STARTBID(obj) = startc;
      GET_BID(obj) = cost;
      GET_AUCTIME(obj) = timer;
    }
    fclose(fl);
  }
}

/* reset the time in the game from file */
static void reset_time(void) {
  time_t beginning_of_time = 0;
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
    mud_log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
  else {
    fscanf(bgtime, "%ld\n", &beginning_of_time);
    fscanf(bgtime, "%ld\n", &NEWSUPDATE);
    fscanf(bgtime, "%ld\n", &BOARDNEWMORT);
    fscanf(bgtime, "%ld\n", &BOARDNEWDUO);
    fscanf(bgtime, "%ld\n", &BOARDNEWCOD);
    fscanf(bgtime, "%ld\n", &BOARDNEWBUI);
    fscanf(bgtime, "%ld\n", &BOARDNEWIMM);
    time_t legacy_interest_time = 0, legacy_last_interest = 0;
    fscanf(bgtime, "%ld\n", &legacy_interest_time);
    fscanf(bgtime, "%ld\n", &legacy_last_interest);
    fscanf(bgtime, "%d\n", &HIGHPCOUNT);
    fscanf(bgtime, "%ld\n", &PCOUNTDATE);
    fscanf(bgtime, "%d\n", &WISHTIME);
    fscanf(bgtime, "%d\n", &PCOUNT);
    fscanf(bgtime, "%ld\n", &LASTPAYOUT);
    fscanf(bgtime, "%d\n", &LASTPAYTYPE);
    fscanf(bgtime, "%d\n", &LASTNEWS);
    fscanf(bgtime, "%d\n", &dballtime);
    fscanf(bgtime, "%d\n", &SELFISHMETER);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON1);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON2);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON3);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON4);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON5);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON6);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON7);
    fscanf(bgtime, "%d\n", &ERAPLAYERS);
    fclose(bgtime);
  }

  if (dballtime == 0)
    dballtime = 604800;

  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  mud_log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day,
      time_info.month, time_info.year);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}

/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when) {
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
    mud_log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else {
    fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
    fprintf(bgtime, "%ld\n", NEWSUPDATE);
    fprintf(bgtime, "%ld\n", BOARDNEWMORT);
    fprintf(bgtime, "%ld\n", BOARDNEWDUO);
    fprintf(bgtime, "%ld\n", BOARDNEWCOD);
    fprintf(bgtime, "%ld\n", BOARDNEWBUI);
    fprintf(bgtime, "%ld\n", BOARDNEWIMM);
    fprintf(bgtime, "%ld\n", (time_t)0);
    fprintf(bgtime, "%ld\n", (time_t)0);
    fprintf(bgtime, "%d\n", HIGHPCOUNT);
    fprintf(bgtime, "%ld\n", PCOUNTDATE);
    fprintf(bgtime, "%d\n", WISHTIME);
    fprintf(bgtime, "%d\n", PCOUNT);
    fprintf(bgtime, "%ld\n", LASTPAYOUT);
    fprintf(bgtime, "%d\n", LASTPAYTYPE);
    fprintf(bgtime, "%d\n", LASTNEWS);
    fprintf(bgtime, "%d\n", dballtime);
    fprintf(bgtime, "%d\n", SELFISHMETER);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON1);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON2);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON3);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON4);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON5);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON6);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON7);
    fprintf(bgtime, "%d\n", ERAPLAYERS);
    fclose(bgtime);
  }
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
static int count_alias_records(FILE *fl) {
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
        goto ackeof;
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
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:
  mud_log("SYSERR: Unexpected end of help file.");
  exit(1); /* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
static int count_hash_records(FILE *fl) {
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}

void index_boot(int mode) {
  const char *index_filename, *prefix = NULL; /* NULL or egcs 1.1 complains */
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

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r"))) {
    mud_log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      mud_log("SYSERR: File '%s' listed in '%s%s': %s", buf2, prefix,
          index_filename, strerror(errno));
      fscanf(db_index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON)
        rec_count++;
      else if (mode == DB_BOOT_HLP)
        rec_count += count_alias_records(db_file);
      else
        rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
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
    size[0] = sizeof(struct room_data) * rec_count;
    mud_log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    size[1] = sizeof(struct char_data) * rec_count;
    mud_log("   %d mobs, %d bytes in prototypes.", rec_count, size[1]);
    break;
  case DB_BOOT_OBJ:
    size[1] = sizeof(struct obj_data) * rec_count;
    mud_log("   %d objs, %d bytes in prototypes.", rec_count, size[1]);
    break;
  case DB_BOOT_ZON:
    size[0] = sizeof(struct zone_data) * rec_count;
    mud_log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    mud_log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  }

  rewind(db_index);
  fscanf(db_index, "%s\n", buf1);
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
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      load_help(db_file, buf2);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    case DB_BOOT_GLD:
      boot_the_guilds(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }
  fclose(db_index);

  /* Sort the help index. */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
}

static void discrete_load(FILE *fl, int mode, char *filename) {
  int nr = -1, last;
  char line[READ_SIZE];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */

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
              filename, modes[mode], nr, modes[mode]);
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
          parse_room(fl, nr);
          break;
        case DB_BOOT_MOB:
          parse_mobile(fl, nr);
          break;
        case DB_BOOT_TRG:
          parse_trigger(fl, nr);
          break;
        case DB_BOOT_OBJ:
          strlcpy(line, parse_object(fl, nr), sizeof(line));
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

char fread_letter(FILE *fp) {
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

bitvector_t asciiflag_conv(char *flag) {
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!(isdigit(*p) || (*p == '-')))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

static bitvector_t asciiflag_conv_aff(char *flag) {
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (1 + (*p - 'a'));
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!(isdigit(*p) || (*p == '-')))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}
/* load the rooms */
static void parse_room(FILE *fl, int virtual_nr) {
  static int zone = 0;
  int t[10], i, retval;
  char line[READ_SIZE], flags[128], flags2[128], flags3[128];
  char flags4[128], buf2[MAX_STRING_LENGTH], buf[128];
  struct extra_descr_data *new_descr;
  char letter;

  /* This really had better fit or there are other problems. */
  snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

  zone = virtual_zone_by_thing(virtual_nr);
  if (zone == NOWHERE) {
    mud_log("SYSERR: Room #%d is outside of any zone.", virtual_nr);
    exit(1);
  }

  struct room_data *rm = NULL;
  CREATE(rm, struct room_data, 1);
  room_put(virtual_nr, rm);

  rm->zone = zone;
  room_vnum_set(rm, virtual_nr);
  room_name_set(rm, fread_string(fl, buf2));
  room_description_set(rm, fread_string(fl, buf2));

  if (!get_line(fl, line)) {
    mud_log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
        virtual_nr);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %d ", t, flags, flags2, flags3,
                        flags4, t + 2)) == 3) &&
      (bitwarning == TRUE)) {
    mud_log("WARNING: Conventional worldfiles detected. Please read "
        "128bit.readme.");
    exit(1);
  } else if ((retval == 3) && (bitwarning == FALSE)) {
    /*
     * Looks like the implementor is ready, so let's load the worldfiles. We
     * load the extra three flags as 0, since they won't be anything anyway. We
     * will save the entire world later on, when every room, mobile, and object
     * is converted.
     */

    mud_log("Converting room #%d to 128bits..", virtual_nr);
    rm->room_flags[0] = asciiflag_conv(flags);
    rm->room_flags[1] = 0;
    rm->room_flags[2] = 0;
    rm->room_flags[3] = 0;

    sprintf(flags, "room #%d",
            virtual_nr); /* sprintf: OK (until 399-bit integers) */

    if (bitsavetodisk) { /* Maybe the implementor just wants to look at the
                            128bit files */
      add_to_save_list(virtual_zone_by_thing(virtual_nr), 3);
      converting = TRUE;
    }

    mud_log("   done.");
  } else if (retval == 6) {
    int taeller;
    rm->room_flags[0] = asciiflag_conv(flags);
    rm->room_flags[1] = asciiflag_conv(flags2);
    rm->room_flags[2] = asciiflag_conv(flags3);
    rm->room_flags[3] = asciiflag_conv(flags4);

    room_sector_type_set(rm, t[2]);
    sprintf(flags, "object #%d",
            virtual_nr); /* sprintf: OK (until 399-bit integers) */

  } else {
    mud_log("SYSERR: Format error in roomflags/sector type of room #%d",
        virtual_nr);
    exit(1);
  }

  room_timed_set(rm, -1);

  int gravity = 0;

  room_vnum vn = room_vnum_get(rm);

  if (room_flagged(rm, ROOM_VEGETA) || room_flagged(rm, ROOM_GRAVITYX10)) {
    gravity = 10;
  }
  if (vn >= 19800 && vn <= 19899) {
    gravity = 1000;
  }
  if (vn >= 64000 && vn <= 64006) {
    gravity = 100;
  }
  if (vn >= 64007 && vn <= 64016) {
    gravity = 300;
  }
  if (vn >= 64017 && vn <= 64030) {
    gravity = 500;
  }
  if (vn >= 64031 && vn <= 64048) {
    gravity = 1000;
  }
  if (vn >= 64049 && vn <= 64070) {
    gravity = 5000;
  }
  if (vn >= 64071 && vn <= 64096) {
    gravity = 10000;
  }
  if (vn == 64097) {
    gravity = 1000;
  }

  room_gravity_set(rm, gravity);

  snprintf(buf, sizeof(buf),
           "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      mud_log("%s", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, rm, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      /* fix for crashes in the editor when formatting
       * - e-descs are assumed to end with a \r\n
       * -- Welcor 09/03
       */
      {
        char *tmp = strchr(new_descr->description, '\0');
        if (tmp > new_descr->description && *(tmp - 1) != '\n') {
          CREATE(tmp, char, strlen(new_descr->description) + 3);
          sprintf(tmp, "%s\r\n",
                  new_descr->description); /* sprintf ok : size checked above*/
          free(new_descr->description);
          new_descr->description = tmp;
        }
      }
      new_descr->next = rm->ex_description;
      rm->ex_description = new_descr;
      break;
    case 'S': /* end of room */
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter == 'T') {
        dg_read_trigger(fl, rm, WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      return;
    default:
      mud_log("%s", buf);
      exit(1);
    }
  }
}

/* read direction data */
static void setup_dir(FILE *fl, struct room_data *room, int dir) {
  int t[11], retval;
  char line[READ_SIZE], buf2[128];

  struct room_data *rm = room;

  snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", room->id, dir);

  CREATE(rm->dir_option[dir], struct room_direction_data, 1);
  struct room_direction_data *ex = rm->dir_option[dir];
  ex->general_description = fread_string(fl, buf2);
  ex->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    mud_log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (((retval = sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d", t, t + 1,
                        t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9,
                        t + 10)) == 3) &&
      (bitwarning == TRUE)) {
    mud_log("SYSERR: Format error, %s", buf2);
    exit(1);
  } else if (bitwarning == FALSE) {

    if (t[0] == 1)
      ex->exit_info = EX_ISDOOR;
    else if (t[0] == 2)
      ex->exit_info = EX_ISDOOR | EX_PICKPROOF;
    else if (t[0] == 3)
      ex->exit_info = EX_ISDOOR | EX_SECRET;
    else if (t[0] == 4)
      ex->exit_info = EX_ISDOOR | EX_PICKPROOF | EX_SECRET;
    else
      ex->exit_info = 0;

    ex->key = ((t[1] == -1 || t[1] == 65535) ? NOTHING : t[1]);
    ex->to_room = ((t[2] == -1 || t[2] == 65535) ? NOWHERE : t[2]);

    zone_vnum zvn = room_zone_vnum_get(rm);

    if (retval == 3) {
      mud_log("Converting world files to include DC add ons.");
      ex->dclock = 20;
      ex->dchide = 20;
      ex->dcskill = 0;
      ex->dcmove = 0;
      ex->failsavetype = 0;
      ex->dcfailsave = 0;
      ex->failroom = NOWHERE;
      ex->totalfailroom = NOWHERE;
      if (bitsavetodisk) {
        add_to_save_list(zvn, 3);
        converting = TRUE;
      }
    } else if (retval == 5) {
      ex->dclock = t[3];
      ex->dchide = t[4];
      ex->dcskill = 0;
      ex->dcmove = 0;
      ex->failsavetype = 0;
      ex->dcfailsave = 0;
      ex->failroom = NOWHERE;
      ex->totalfailroom = NOWHERE;
      if (bitsavetodisk) {
        add_to_save_list(zvn, 3);
        converting = TRUE;
      }
    } else if (retval == 7) {
      ex->dclock = t[3];
      ex->dchide = t[4];
      ex->dcskill = t[5];
      ex->dcmove = t[6];
      ex->failsavetype = 0;
      ex->dcfailsave = 0;
      ex->failroom = NOWHERE;
      ex->totalfailroom = NOWHERE;
      if (bitsavetodisk) {
        add_to_save_list(zvn, 3);
        converting = TRUE;
      }
    } else if (retval == 11) {
      ex->dclock = t[3];
      ex->dchide = t[4];
      ex->dcskill = t[5];
      ex->dcmove = t[6];
      ex->failsavetype = t[7];
      ex->dcfailsave = t[8];
      ex->failroom = t[9];
      ex->totalfailroom = t[10];
    }
  }
}


/* make sure the start rooms exist & resolve their vnums to rnums */
static void check_start_rooms(void) {}

static void mob_autobalance(struct char_data *ch) {}

static int parse_simple_mob(FILE *mob_f, struct char_data *ch, int nr) {
  int j, t[10];
  char line[READ_SIZE];

  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    return 0;
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ", t, t + 1, t + 2, t + 3,
             t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    mud_log("SYSERR: Format error in mob #%d, first line after S flag\n"
        "...expecting line of form '# # # #d#+# #d#+#'",
        nr);
    return 0;
  }

  char_stat_set(ch, "level", t[0]);
  char_stat_set(ch, "armor", 10 * (10 - t[2]));

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  char_stat_set(ch, "powerlevel", t[3]);
  char_stat_set(ch, "ki", t[4]);
  char_stat_set(ch, "stamina", t[5]);

  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
        "...expecting line of form '# #', but file ended!",
        nr);
    return 0;
  }

  if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
        "...expecting line of form '# # # #'",
        nr);
    return 0;
  }

  ch->race = t[2];

  ch->chclass = t[3];
  if (ch->chclass < 0 || ch->chclass >= NUM_CLASSES) {
    mud_log("SYSERR: Invalid class %d for mob #%d", ch->chclass, nr);
    ch->chclass = 28; /* set to commoner */
  }

  if (!IS_HUMAN(ch))
    if (!AFF_FLAGGED(ch, AFF_INFRAVISION))
      SET_BIT_AR(AFF_FLAGS(ch), AFF_INFRAVISION);

  SPEAKING(ch) = SKILL_LANG_COMMON;

  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #', but file ended!",
        nr);
    return 0;
  }

  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    mud_log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #'",
        nr);
    return 0;
  }

  char_position_set(ch, t[0]);
  GET_DEFAULT_POS(ch) = t[1];
  GET_SEX(ch) = t[2];

  SPEAKING(ch) = MIN_LANGUAGES;
  set_height_and_weight_by_race(ch);

  if (MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
    mob_autobalance(ch);
  }

  return 1;
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

#define CASE(test)                                                             \
  if (value && !matched && !strcasecmp(keyword, test) && (matched = TRUE))

#define BOOL_CASE(test)                                                        \
  if (!value && !matched && !strcasecmp(keyword, test) && (matched = TRUE))

#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

static void interpret_espec(const char *keyword, const char *value,
                            struct char_data *ch, int nr) {
  int num_arg = 0, matched = FALSE;
  int num, num2, num3, num4, num5, num6;
  struct affected_type af;

  /*
   * If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some.
   */
  if (value)
    num_arg = atoi(value);

  CASE("BareHandAttack") {}

  CASE("Size") {
    RANGE(SIZE_UNDEFINED, NUM_SIZES - 1);
    ch->size = num_arg;
  }

  CASE("Str") {
    RANGE(0, 200);
    char_stat_set(ch, "strength", num_arg);
  }

  CASE("StrAdd") {
    mud_log("mob #%d trying to set StrAdd, rebalance its strength.",
        GET_MOB_VNUM(ch));
  }

  CASE("Int") {
    RANGE(0, 200);
    char_stat_set(ch, "intelligence", num_arg);
  }

  CASE("Wis") {
    RANGE(0, 200);
    char_stat_set(ch, "wisdom", num_arg);
  }

  CASE("Dex") {
    RANGE(0, 200);
    char_stat_set(ch, "agility", num_arg);
  }

  CASE("Con") {
    RANGE(0, 200);
    char_stat_set(ch, "constitution", num_arg);
  }

  CASE("Cha") {
    RANGE(0, 200);
    char_stat_set(ch, "speed", num_arg);
  }

  CASE("Hit") {
    RANGE(0, 99999);
    // GET_HIT(ch) = num_arg;
  }

  CASE("MaxHit") { RANGE(0, 99999); }

  CASE("Mana") {
    RANGE(0, 99999);
    // GET_MANA(ch) = num_arg;
  }

  CASE("MaxMana") { RANGE(0, 99999); }

  CASE("Moves") {
    RANGE(0, 99999);
    // GET_MOVE(ch) = num_arg;
  }

  CASE("MaxMoves") { RANGE(0, 99999); }

  CASE("Affect") {}

  CASE("AffectV") {}

  CASE("Feat") { sscanf(value, "%d %d", &num, &num2); }

  CASE("Skill") { sscanf(value, "%d %d", &num, &num2); }

  CASE("SkillMod") {
    sscanf(value, "%d %d", &num, &num2);
    // SET_SKILL_BONUS(ch, num, num2);
  }

  CASE("Class") { sscanf(value, "%d %d", &num, &num2); }

  CASE("EpicClass") { sscanf(value, "%d %d", &num, &num2); }

  if (!matched) {
    mud_log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d", keyword,
        nr);
  }
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

static void parse_espec(char *buf, struct char_data *ch, int nr) {
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }
  interpret_espec(buf, ptr, ch, nr);
}

static int parse_enhanced_mob(FILE *mob_f, struct char_data *ch, int nr) {
  char line[READ_SIZE];

  parse_simple_mob(mob_f, ch, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E")) /* end of the enhanced section */
      return 1;
    else if (*line == '#') { /* we've hit the next mob, maybe? */
      mud_log("SYSERR: Unterminated E section in mob #%d", nr);
      return 0;
    } else
      parse_espec(line, ch, nr);
  }

  mud_log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  return 0;
}

int parse_mobile_from_file(FILE *mob_f, struct char_data *ch) {
  int j, t[10], retval;
  char line[READ_SIZE], *tmpptr, letter;
  char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128];
  char f7[128], f8[128], buf2[128];
  mob_vnum nr = ch->proto_id;

  sprintf(buf2, "mob vnum %d", nr); /* sprintf: OK (for 'buf2 >= 19') */

  /***** String data *****/
  ch->name = fread_string(mob_f, buf2);
  tmpptr = ch->short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
        !strcasecmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  ch->long_descr = fread_string(mob_f, buf2);
  ch->description = fread_string(mob_f, buf2);

  /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    mud_log("SYSERR: Format error after string section of mob #%d\n"
        "...expecting line of form '# # # {S | E}', but file ended!",
        nr);
    return 0;
  }

  if (((retval = sscanf(line, "%s %s %s %s %s %s %s %s %d %c", f1, f2, f3, f4,
                        f5, f6, f7, f8, t + 2, &letter)) == 10) &&
      (bitwarning == TRUE)) {
    /* Let's make the implementor read some, before converting his worldfiles */
    mud_log("WARNING: Conventional mobilefiles detected. Please read "
        "128bit.readme.");
    return 0;
  } else if ((retval == 4) && (bitwarning == FALSE)) {

    mud_log("Converting mobile #%d to 128bits..", nr);
    MOB_FLAGS(ch)[0] = asciiflag_conv(f1);
    MOB_FLAGS(ch)[1] = 0;
    MOB_FLAGS(ch)[2] = 0;
    MOB_FLAGS(ch)[3] = 0;

    AFF_FLAGS(ch)[0] = asciiflag_conv_aff(f2);
    AFF_FLAGS(ch)[1] = 0;
    AFF_FLAGS(ch)[2] = 0;
    AFF_FLAGS(ch)[3] = 0;

    char_stat_set(ch, "alignment", atoi(f3));

    /* Make some basic checks. */
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CHARM);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SLEEP);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_GOOD))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_GOOD);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_NEUTRAL);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_EVIL))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_EVIL);

    /*
     * This is necessary, since if we have conventional worldfiles, &letter
     * is loaded into f4 instead of the letter characters. So what we do, is
     * copy f4 into letter. Disadvantage is that &letter cannot be longer
     * then 128 characters, but this shouldn't occur anyway.
     */
    letter = *f4;

    if (bitsavetodisk) {
      add_to_save_list(virtual_zone_by_thing(nr), 0);
      converting = TRUE;
    }

    mud_log("   done.");
  } else if (retval == 10) {
    int taeller;

    MOB_FLAGS(ch)[0] = asciiflag_conv(f1);
    MOB_FLAGS(ch)[1] = asciiflag_conv(f2);
    MOB_FLAGS(ch)[2] = asciiflag_conv(f3);
    MOB_FLAGS(ch)[3] = asciiflag_conv(f4);

    AFF_FLAGS(ch)[0] = asciiflag_conv(f5);
    AFF_FLAGS(ch)[1] = asciiflag_conv(f6);
    AFF_FLAGS(ch)[2] = asciiflag_conv(f7);
    AFF_FLAGS(ch)[3] = asciiflag_conv(f8);

    char_stat_set(ch, "alignment", t[2]);

  } else {
    mud_log("SYSERR: Format error after string section of mob #%d\n"
        "...expecting line of form '# # # {S | E}'",
        nr);
    exit(1);
  }

  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  if (MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
    /* Rather bad to load mobiles with this bit already set. */
    mud_log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
    REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
  }

  /* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE.
  if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i,
  MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL)) mud_log("SYSERR: Mob #%d both
  Aggressive and Aggressive_to_Alignment.", nr); */

  /* Convert mobs to use AUTOBALANCE. Uncomment and reboot to flag all mobs
   * AUTOBALANCE. if (!MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
   *   SET_BIT_AR(MOB_FLAGS(ch), MOB_AUTOBALANCE);
   * } */

  switch (UPPER(letter)) {
  case 'S': /* Simple monsters */
    parse_simple_mob(mob_f, ch, nr);
    break;
  case 'E': /* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, ch, nr);
    mob_stats(ch);
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
    dg_read_trigger(mob_f, ch, MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  /* Uncomment to force all mob files to be rewritten. Good for initial
   * AUTOBALANCE setup. if (bitsavetodisk) {
   *   add_to_save_list(virtual_zone_by_thing(nr), 0);
   *   converting = TRUE;
   * } */

  return 1;
}

static void parse_mobile(FILE *mob_f, int nr) {

  struct char_data *ch = NULL;
  CREATE(ch, struct char_data, 1);
  clear_char(ch);

  ch->proto_id = nr;
  ch->desc = NULL;

  if (parse_mobile_from_file(mob_f, ch)) {
    struct mob_proto_data *proto = NULL;
    CREATE(proto, struct mob_proto_data, 1);
    copy_mobile_to_proto(proto, ch);
    mob_proto_put(nr, proto);
    char_free_prototype(ch);
  } else { /* We used to exit in the file reading code, but now we do it here */
    exit(1);
  }
}

/* read all objects from obj file; generate index and prototypes */
static char *parse_object(FILE *obj_f, int nr) {
  static int i = 0;
  static char line[READ_SIZE];
  int t[NUM_OBJ_VAL_POSITIONS + 2], j, retval;
  char *tmpptr, buf2[128];
  char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
  char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
  char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];
  struct extra_descr_data *new_descr;

  struct obj_proto_data *proto = NULL;
  CREATE(proto, struct obj_proto_data, 1);
  obj_proto_put(nr, proto);

  proto->id = nr;

  sprintf(buf2, "object #%d", nr); /* sprintf: OK (for 'buf2 >= 19') */

  /* *** string data *** */
  if ((proto->name = fread_string(obj_f, buf2)) == NULL) {
    mud_log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = proto->short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
        !strcasecmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = proto->description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  proto->action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    mud_log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %s %s %s %s %s %s %s %s", t, f1,
                        f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12)) == 4) &&
      (bitwarning == TRUE)) {
    /* Let's make the implementor read some, before converting his worldfiles */
    mud_log("WARNING: Conventional objectfiles detected. Please read "
        "128bit.readme.");
    exit(1);
  } else if (((retval == 4) || (retval == 3)) && (bitwarning == FALSE)) {

    if (retval == 3)
      t[3] = 0;
    else if (retval == 4)
      t[3] = asciiflag_conv_aff(f3);

    mud_log("Converting object #%d to 128bits..", nr);
    GET_OBJ_EXTRA(proto)[0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(proto)[1] = 0;
    GET_OBJ_EXTRA(proto)[2] = 0;
    GET_OBJ_EXTRA(proto)[3] = 0;
    GET_OBJ_WEAR(proto)[0] = asciiflag_conv(f2);
    GET_OBJ_WEAR(proto)[1] = 0;
    GET_OBJ_WEAR(proto)[2] = 0;
    GET_OBJ_WEAR(proto)[3] = 0;
    GET_OBJ_PERM(proto)[0] = asciiflag_conv_aff(f3);
    GET_OBJ_PERM(proto)[1] = 0;
    GET_OBJ_PERM(proto)[2] = 0;
    GET_OBJ_PERM(proto)[3] = 0;

    if (bitsavetodisk) {
      add_to_save_list(virtual_zone_by_thing(nr), 1);
      converting = TRUE;
    }

    mud_log("   done.");
  } else if (retval == 13) {

    GET_OBJ_EXTRA(proto)[0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(proto)[1] = asciiflag_conv(f2);
    GET_OBJ_EXTRA(proto)[2] = asciiflag_conv(f3);
    GET_OBJ_EXTRA(proto)[3] = asciiflag_conv(f4);

    GET_OBJ_WEAR(proto)[0] = asciiflag_conv(f5);
    GET_OBJ_WEAR(proto)[1] = asciiflag_conv(f6);
    GET_OBJ_WEAR(proto)[2] = asciiflag_conv(f7);
    GET_OBJ_WEAR(proto)[3] = asciiflag_conv(f8);

    GET_OBJ_PERM(proto)[0] = asciiflag_conv(f9);
    GET_OBJ_PERM(proto)[1] = asciiflag_conv(f10);
    GET_OBJ_PERM(proto)[2] = asciiflag_conv(f11);
    GET_OBJ_PERM(proto)[3] = asciiflag_conv(f12);

  } else {
    mud_log("SYSERR: Format error in first numeric line (expecting 13 args, got "
        "%d), %s",
        retval, buf2);
    exit(1);
  }

  /* Object flags checked in check_object(). */
  GET_OBJ_TYPE(proto) = t[0];

  if (!get_line(obj_f, line)) {
    mud_log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    t[j] = 0;

  if ((retval = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                       t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7,
                       t + 8, t + 9, t + 10, t + 11, t + 12, t + 13, t + 14,
                       t + 15)) > NUM_OBJ_VAL_POSITIONS) {
    mud_log("SYSERR: Format error in second numeric line (expecting <=%d args, got "
        "%d), %s",
        NUM_OBJ_VAL_POSITIONS, retval, buf2);
    exit(1);
  }

  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    GET_OBJ_VAL(proto, j) = t[j];

  if ((GET_OBJ_TYPE(proto) == ITEM_PORTAL ||
       GET_OBJ_TYPE(proto) == ITEM_HATCH) &&
      (!GET_OBJ_VAL(proto, VAL_DOOR_DCLOCK) ||
       !GET_OBJ_VAL(proto, VAL_DOOR_DCHIDE))) {
    GET_OBJ_VAL(proto, VAL_DOOR_DCLOCK) = 20;
    GET_OBJ_VAL(proto, VAL_DOOR_DCHIDE) = 20;
    if (bitsavetodisk) {
      add_to_save_list(virtual_zone_by_thing(nr), 1);
      converting = TRUE;
    }
  }

  if (GET_OBJ_TYPE(proto) == ITEM_WEAPON && GET_OBJ_VAL(proto, 0) > 169) {
    GET_OBJ_VAL(proto, 0) = suntzu_weapon_convert(t[0]);

    if (bitsavetodisk) {
      add_to_save_list(virtual_zone_by_thing(nr), 1);
      converting = TRUE;
    }
  }

  /* Convert old CWG-SunTzu style armor values to CWG-Rasputin. Should no longer
   * be needed I think. if (GET_OBJ_TYPE(obj_proto + i) == ITEM_ARMOR) { if
   * (suntzu_armor_convert(obj_proto + i)) { if(bitsavetodisk) {
   *       add_to_save_list(virtual_zone_by_thing(nr), 1);
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
      mud_log("SYSERR: Format error in third numeric line (expecting 4 args, got "
          "%d), %s",
          retval, buf2);
      exit(1);
    }
  }
  GET_OBJ_WEIGHT(proto) = t[0];
  GET_OBJ_COST(proto) = t[1];
  GET_OBJ_LEVEL(proto) = t[3];
  GET_OBJ_SIZE(proto) = SIZE_MEDIUM;

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (GET_OBJ_TYPE(proto) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(proto) == ITEM_FOUNTAIN) {
    if (GET_OBJ_WEIGHT(proto) < GET_OBJ_VAL(proto, 1))
      GET_OBJ_WEIGHT(proto) = GET_OBJ_VAL(proto, 1) + 5;
  }
  /* *** make sure portal objects have their timer set correctly *** */
  if (GET_OBJ_TYPE(proto) == ITEM_PORTAL) {
    GET_OBJ_TIMER(proto) = -1;
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    proto->affected[j].location = APPLY_NONE;
    proto->affected[j].modifier = 0;
    proto->affected[j].specific = 0;
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
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = proto->ex_description;
      proto->ex_description = new_descr;
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
        mud_log("Warning: object #%d (%s) uses deprecated saving throw applies", nr,
            GET_OBJ_SHORT(proto));
      }
      proto->affected[j].location = t[0];
      proto->affected[j].modifier = t[1];
      proto->affected[j].specific = t[2];
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

      j++;
      break;
    case 'T': /* DG triggers */
      dg_obj_trigger(line, proto);
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
      GET_OBJ_SIZE(proto) = t[0];
      break;
    case '$':
    case '#':
      /* Objects that set CHARM on players are bad. */
      if (OBJAFF_FLAGGED(proto, AFF_CHARM)) {
        mud_log("SYSERR: Object #%d has reserved bit AFF_CHARM set.", nr);
        REMOVE_BIT_AR(GET_OBJ_PERM(proto), AFF_CHARM);
      }
      check_object(proto);
      i++;
      return (line);
    default:
      mud_log("SYSERR: Format error in (%c): %s", *line, buf2);
      exit(1);
    }
  }
  return line;
}

/* load the zone table and command tables */
static void load_zones(FILE *fl, char *zonename) {
  static zone_rnum zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error, arg_num,
      version = 1;
  char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH];
  int zone_fix = FALSE;
  char t1[80], t2[80], line[MAX_STRING_LENGTH];

  struct zone_data *z;
  CREATE(z, struct zone_data, 1);

  strlcpy(zname, zonename, sizeof(zname));

  /* Skip first 3 lines lest we mistake the zone name for a command. */
  for (tmp = 0; tmp < 3; tmp++)
    get_line(fl, buf);

  /*  More accurate count. Previous was always 4 or 5 too high. -gg 2001/1/17
   *  Note that if a new zone command is added to reset_zone(), this string
   *  will need to be updated to suit. - ae.
   */
  while (get_line(fl, buf))
    if ((strchr("MOPGERDTV", buf[0]) && buf[1] == ' ') ||
        (buf[0] == 'S' && buf[1] == '\0'))
      num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0) {
    mud_log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    CREATE(z->cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (*buf == '@') {
    if (sscanf(buf, "@Version: %d", &version) != 1) {
      mud_log("SYSERR: Format error in %s (version)", zname);
      mud_log("SYSERR: ...Line: %s", line);
      exit(1);
    }
    line_num += get_line(fl, buf);
  }

  if (sscanf(buf, "#%hd", &z->id) != 1) {
    mud_log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  snprintf(buf2, sizeof(buf2), "beginning of zone #%d", z->id);
  zone_put(z->id, z);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
    *ptr = '\0';
  z->builders = strdup(buf);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
    *ptr = '\0';
  z->name = strdup(buf);

  line_num += get_line(fl, buf);
  if (version >= 2) {

    char zbuf1[MAX_STRING_LENGTH];
    char zbuf2[MAX_STRING_LENGTH];
    char zbuf3[MAX_STRING_LENGTH];
    char zbuf4[MAX_STRING_LENGTH];

    if (sscanf(buf, " %hd %hd %d %d %s %s %s %s %d %d", &z->bot, &z->top,
               &z->lifespan, &z->reset_mode, zbuf1, zbuf2, zbuf3, zbuf4,
               &z->min_level, &z->max_level) != 10) {
      mud_log("SYSERR: Format error in 10-constant line of %s", zname);
      exit(1);
    }

    z->zone_flags[0] = asciiflag_conv(zbuf1);
    z->zone_flags[1] = asciiflag_conv(zbuf2);
    z->zone_flags[2] = asciiflag_conv(zbuf3);
    z->zone_flags[3] = asciiflag_conv(zbuf4);
  } else if (sscanf(buf, " %hd %hd %d %d ", &z->bot, &z->top, &z->lifespan,
                    &z->reset_mode) != 4) {
    /*
     * This may be due to the fact that the zone has no builder.  So, we just
     * attempt to fix this by copying the previous 2 last reads into this
     * variable and the last one.
     */
    mud_log("SYSERR: Format error in numeric constant line of %s, attempting to "
        "fix.",
        zname);
    if (sscanf(z->name, " %hd %hd %d %d ", &z->bot, &z->top, &z->lifespan,
               &z->reset_mode) != 4) {
      mud_log("SYSERR: Could not fix previous error, aborting game.");
      exit(1);
    } else {
      free(z->name);
      z->name = strdup(z->builders);
      free(z->builders);
      z->builders = strdup("None.");
      zone_fix = TRUE;
    }
  }
  if (z->bot > z->top) {
    mud_log("SYSERR: Zone %d bottom (%d) > top (%d).", z->id, z->bot, z->top);
    exit(1);
  }

  cmd_no = 0;

  for (;;) {
    /* skip reading one line if we fixed above (line is correct already) */
    if (zone_fix != TRUE) {
      if ((tmp = get_line(fl, buf)) == 0) {
        mud_log("SYSERR: Format error in %s - premature end of file", zname);
        exit(1);
      }
    } else
      zone_fix = FALSE;

    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    struct reset_com *cmd = &z->cmd[cmd_no];

    if ((cmd->command = *ptr) == '*')
      continue;

    ptr++;

    if (cmd->command == 'S' || cmd->command == '$') {
      cmd->command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPDTVG", cmd->command) == nullptr) { /* a 4-arg command */
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &cmd->arg1, &cmd->arg2,
                 &cmd->arg3) != 4)
        error = 1;
    } else if (cmd->command == 'V') { /* a string-arg command */
      if (sscanf(ptr, " %d %d %d %d %d %d %79s %79[^\f\n\r\t\v]", &tmp,
                 &cmd->arg1, &cmd->arg2, &cmd->arg3, &cmd->arg4, &cmd->arg5, t1,
                 t2) != 8)
        error = 1;
      else {
        cmd->sarg1 = strdup(t1);
        cmd->sarg2 = strdup(t2);
      }
    } else {
      if ((arg_num = sscanf(ptr, " %d %d %d %d %d %d ", &tmp, &cmd->arg1,
                            &cmd->arg2, &cmd->arg3, &cmd->arg4, &cmd->arg5)) !=
          6) {
        if (arg_num != 5) {
          error = 1;
        } else {
          cmd->arg5 = 0;
        }
      }
    }

    cmd->if_flag = tmp;

    if (error) {
      mud_log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    cmd->line = line_num;
    cmd_no++;
  }

  if (num_of_cmds != cmd_no + 1) {
    mud_log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d",
        zname, num_of_cmds, cmd_no + 1);
    exit(1);
  }
}

static void get_one_line(FILE *fl, char *buf) {
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    mud_log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

void free_help(struct help_index_element *cmhelp) {
  if (cmhelp->keywords)
    free(cmhelp->keywords);
  if (cmhelp->entry && !cmhelp->duplicate)
    free(cmhelp->entry);

  free(cmhelp);
}

void free_help_table(void) {
  if (help_table) {
    int hp;
    for (hp = 0; hp < top_of_helpt; hp++) {
      if (help_table[hp].keywords)
        free(help_table[hp].keywords);
      if (help_table[hp].entry && !help_table[hp].duplicate)
        free(help_table[hp].entry);
    }
    free(help_table);
    help_table = NULL;
  }
  top_of_helpt = 0;
}

void load_help(FILE *fl, char *name) {
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
  size_t entrylen;
  char line[READ_SIZE + 1], hname[READ_SIZE + 1], *scan;
  struct help_index_element el;

  strlcpy(hname, name, sizeof(hname));

  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key,
           "\r\n"); /* strcat: OK (READ_SIZE - "\n"  "\r\n" == READ_SIZE  1) */
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

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1,
             truncmsg); /* strcpy: OK (assuming sane 'entry' size) */

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
    el.entry = strdup(entry);
    scan = one_word(key, next_key);

    while (*next_key) {
      el.keywords = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }
    get_one_line(fl, key);
  }
}

int hsort(const void *a, const void *b) {
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *)a;
  b1 = (const struct help_index_element *)b;

  return (strcasecmp(a1->keywords, b1->keywords));
}

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time	 	 *
 *************************************************************************/


/* create a character, and add it to the char list */
struct char_data *create_char(void) {
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  GET_ID(ch) = max_mob_id++;
  /* find_char helper */
  (void)char_register_id(GET_ID(ch), ch);

  return (ch);
}

/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob = NULL;
  struct mob_proto_data *proto = NULL;

  if (type == REAL) {
    mud_log("real is no longer supported!");
    exit(1);
  }

  if (type == VIRTUAL) {
    if (!(proto = mob_proto_by_id(nr))) {
      mud_log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  }

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  copy_mobile_from_proto(mob, proto);

  if (IS_HOSHIJIN(mob) && GET_SEX(mob) == SEX_MALE) {
    mob->hairl = 0;
    mob->hairc = 0;
    mob->hairs = 0;
  } else {
    mob->hairl = rand_number(0, 4);
    mob->hairc = rand_number(1, 13);
    mob->hairs = rand_number(1, 11);
  }

  mob->eye = rand_number(0, 11);

  GET_ABSORBS(mob) = 0;
  char_absorbing_set(mob, NULL);
  char_absorbed_by_set(mob, NULL);
  SITS(mob) = NULL;
  char_blocked_by_set(mob, NULL);
  char_blocking_set(mob, NULL);

  if (!IS_HUMAN(mob) && !IS_SAIYAN(mob) && !IS_HALFBREED(mob) &&
      !IS_NAMEK(mob)) {
    mob->skin = rand_number(0, 11);
  }
  if (IS_NAMEK(mob)) {
    mob->skin = 2;
  }
  if (IS_HUMAN(mob) || IS_SAIYAN(mob) || IS_HALFBREED(mob)) {
    if (rand_number(1, 5) <= 2) {
      mob->skin = rand_number(0, 1);
    } else if (rand_number(1, 5) <= 4) {
      mob->skin = rand_number(4, 5);
    } else if (rand_number(1, 5) <= 5) {
      mob->skin = rand_number(9, 10);
    }
  }
  if (IS_SAIYAN(mob)) {
    mob->hairc = rand_number(1, 2);
    mob->eye = 1;
  }

  if (GET_MOB_VNUM(mob) >= 81 && GET_MOB_VNUM(mob) <= 87) {
    dragon_level(mob);
  }

  int64_t mult = 0;

  switch (GET_LEVEL(mob)) {
  case 1:
    mult = rand_number(50, 80);
    break;
  case 2:
    mult = rand_number(90, 120);
    break;
  case 3:
    mult = rand_number(100, 140);
    break;
  case 4:
    mult = rand_number(120, 180);
    break;
  case 5:
    mult = rand_number(200, 250);
    break;
  case 6:
    mult = rand_number(240, 300);
    break;
  case 7:
    mult = rand_number(280, 350);
    break;
  case 8:
    mult = rand_number(320, 400);
    break;
  case 9:
    mult = rand_number(380, 480);
    break;
  case 10:
    mult = rand_number(500, 600);
    break;
  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
    mult = rand_number(1200, 1600);
    break;
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
    mult = rand_number(2400, 3000);
    break;
  case 21:
  case 22:
  case 23:
  case 24:
  case 25:
    mult = rand_number(5500, 8000);
    break;
  case 26:
  case 27:
  case 28:
  case 29:
  case 30:
    mult = rand_number(10000, 14000);
    break;
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
    mult = rand_number(16000, 20000);
    break;
  case 36:
  case 37:
  case 38:
  case 39:
  case 40:
    mult = rand_number(22000, 30000);
    break;
  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
    mult = rand_number(50000, 70000);
    break;
  case 46:
  case 47:
  case 48:
  case 49:
  case 50:
    mult = rand_number(95000, 140000);
    break;
  case 51:
  case 52:
  case 53:
  case 54:
  case 55:
    mult = rand_number(180000, 250000);
    break;
  case 56:
  case 57:
  case 58:
  case 59:
  case 60:
    mult = rand_number(400000, 480000);
    break;
  case 61:
  case 62:
  case 63:
  case 64:
  case 65:
    mult = rand_number(700000, 900000);
    break;
  case 66:
  case 67:
  case 68:
  case 69:
  case 70:
    mult = rand_number(1400000, 1600000);
    break;
  case 71:
  case 72:
  case 73:
  case 74:
  case 75:
    mult = rand_number(2200000, 2500000);
    break;
  case 76:
  case 77:
  case 78:
  case 79:
  case 80:
    mult = rand_number(3000000, 3500000);
    break;
  case 81:
  case 82:
  case 83:
  case 84:
  case 85:
    mult = rand_number(4250000, 4750000);
    break;
  case 86:
  case 87:
  case 88:
  case 89:
  case 90:
    mult = rand_number(6500000, 8500000);
    break;
  case 91:
  case 92:
  case 93:
  case 94:
  case 95:
    mult = rand_number(15000000, 18000000);
    break;
  case 96:
  case 97:
  case 98:
  case 99:
  case 100:
    mult = rand_number(22000000, 30000000);
    break;
  case 101:
    mult = rand_number(32000000, 40000000);
    break;
  case 102:
    mult = rand_number(42000000, 55000000);
    break;
  case 103:
    mult = rand_number(80000000, 95000000);
    break;
  case 104:
    mult = rand_number(150000000, 200000000);
    break;
  case 105:
    mult = rand_number(220000000, 250000000);
    break;
  case 106:
  case 107:
  case 108:
  case 109:
  case 110:
    mult = rand_number(500000000, 750000000);
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
    mult = rand_number(800000000, 900000000);
    break;
  default:
    if (GET_LEVEL(mob) >= 150) {
      mult = rand_number(1500000000, 2000000000);
    } else {
      mult = rand_number(1250000000, 1500000000);
    }
    break;
  }

  GET_LPLAY(mob) = time(0);
  bool autoset = false;
  if (char_stat_get(mob, "powerlevel") <= 1) {
    autoset = true;
    char_stat_set(mob, "powerlevel", GET_LEVEL(mob) * mult);
    if (GET_LEVEL(mob) > 140) {
      char_stat_set(mob, "powerlevel", char_stat_get(mob, "powerlevel") * 8);
    } else if (GET_LEVEL(mob) > 130) {
      char_stat_set(mob, "powerlevel", char_stat_get(mob, "powerlevel") * 6);
    } else if (GET_LEVEL(mob) > 120) {
      char_stat_set(mob, "powerlevel", char_stat_get(mob, "powerlevel") * 3);
    } else if (GET_LEVEL(mob) > 110) {
      char_stat_set(mob, "powerlevel", char_stat_get(mob, "powerlevel") * 2);
    }
  }
  if (autoset) {
    char_stat_set(mob, "ki", GET_LEVEL(mob) * mult);
    if (GET_LEVEL(mob) > 140) {
      char_stat_set(mob, "ki", char_stat_get(mob, "ki") * 8);
    } else if (GET_LEVEL(mob) > 130) {
      char_stat_set(mob, "ki", char_stat_get(mob, "ki") * 6);
    } else if (GET_LEVEL(mob) > 120) {
      char_stat_set(mob, "ki", char_stat_get(mob, "ki") * 3);
    } else if (GET_LEVEL(mob) > 110) {
      char_stat_set(mob, "ki", char_stat_get(mob, "ki") * 2);
    }
  }
  if (autoset) {
    char_stat_set(mob, "stamina", GET_LEVEL(mob) * mult);
    if (GET_LEVEL(mob) > 140) {
      char_stat_set(mob, "stamina", char_stat_get(mob, "stamina") * 8);
    } else if (GET_LEVEL(mob) > 130) {
      char_stat_set(mob, "stamina", char_stat_get(mob, "stamina") * 6);
    } else if (GET_LEVEL(mob) > 120) {
      char_stat_set(mob, "stamina", char_stat_get(mob, "stamina") * 3);
    } else if (GET_LEVEL(mob) > 110) {
      char_stat_set(mob, "stamina", char_stat_get(mob, "stamina") * 2);
    }
  }
  if (GET_MOB_VNUM(mob) == 2245) {
    char_stat_set(mob, "powerlevel", rand_number(1, 4));
    char_stat_set(mob, "ki", rand_number(1, 4));
    char_stat_set(mob, "stamina", rand_number(1, 4));
  }

  int base = 0;
  switch (GET_LEVEL(mob)) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    base = rand_number(80, 120);
    break;
  case 6:
    base = rand_number(200, 280);
    break;
  case 7:
    base = rand_number(250, 350);
    break;
  case 8:
    base = rand_number(275, 375);
    break;
  case 9:
    base = rand_number(300, 400);
    break;
  case 10:
    base = rand_number(325, 450);
    break;
  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
    base = rand_number(500, 700);
    break;
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
    base = rand_number(700, 1000);
    break;
  case 21:
  case 22:
  case 23:
  case 24:
  case 25:
    base = rand_number(1000, 1200);
    break;
  case 26:
  case 27:
  case 28:
  case 29:
  case 30:
    base = rand_number(1200, 1400);
    break;
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
    base = rand_number(1400, 1600);
    break;
  case 36:
  case 37:
  case 38:
  case 39:
  case 40:
    base = rand_number(1600, 1800);
    break;
  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
    base = rand_number(1800, 2000);
    break;
  case 46:
  case 47:
  case 48:
  case 49:
  case 50:
    base = rand_number(2000, 2200);
    break;
  case 51:
  case 52:
  case 53:
  case 54:
  case 55:
    base = rand_number(2200, 2500);
    break;
  case 56:
  case 57:
  case 58:
  case 59:
  case 60:
    base = rand_number(2500, 2800);
    break;
  case 61:
  case 62:
  case 63:
  case 64:
  case 65:
    base = rand_number(2800, 3000);
    break;
  case 66:
  case 67:
  case 68:
  case 69:
  case 70:
    base = rand_number(3000, 3200);
    break;
  case 71:
  case 72:
  case 73:
  case 74:
  case 75:
    base = rand_number(3200, 3500);
    break;
  case 76:
  case 77:
  case 78:
  case 79:
    base = rand_number(3500, 3800);
    break;
  case 80:
  case 81:
  case 82:
  case 83:
  case 84:
  case 85:
    base = rand_number(4000, 4500);
    break;
  case 86:
  case 87:
  case 88:
  case 89:
  case 90:
    base = rand_number(4500, 5500);
    break;
  case 91:
  case 92:
  case 93:
  case 94:
  case 95:
    base = rand_number(5500, 7000);
    break;
  case 96:
  case 97:
  case 98:
  case 99:
    base = rand_number(8000, 10000);
    break;
  case 100:
    base = rand_number(10000, 15000);
    break;
  case 101:
    base = rand_number(15000, 25000);
    break;
  case 102:
    base = rand_number(35000, 40000);
    break;
  case 103:
    base = rand_number(40000, 50000);
    break;
  case 104:
    base = rand_number(60000, 80000);
    break;
  case 105:
    base = rand_number(80000, 100000);
    break;
  default:
    base = rand_number(130000, 180000);
    break;
  }
  MOB_COOLDOWN(mob) = 0;
  if (GET_GOLD(mob) <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
    if (GET_LEVEL(mob) < 4) {
      char_stat_set(mob, "money", GET_LEVEL(mob) * rand_number(1, 2));
    } else if (GET_LEVEL(mob) < 10) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(1, 2)) - 1);
    } else if (GET_LEVEL(mob) < 20) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(1, 3)) - 2);
    } else if (GET_LEVEL(mob) < 30) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(1, 3)) - 4);
    } else if (GET_LEVEL(mob) < 40) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(1, 3)) - 6);
    } else if (GET_LEVEL(mob) < 50) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(2, 3)) - 25);
    } else if (GET_LEVEL(mob) < 60) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(2, 3)) - 40);
    } else if (GET_LEVEL(mob) < 70) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(2, 3)) - 50);
    } else if (GET_LEVEL(mob) < 80) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(2, 4)) - 60);
    } else if (GET_LEVEL(mob) < 90) {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(2, 4)) - 70);
    } else {
      char_stat_set(mob, "money", (GET_LEVEL(mob) * rand_number(3, 4)) - 85);
    }
    if (!IS_HUMANOID(mob)) {
      char_stat_set(mob, "money", GET_GOLD(mob) * 0.5);
      if (GET_GOLD(mob) <= 0)
        char_stat_set(mob, "money", 1);
    }
  }
  if (GET_EXP(mob) <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
    char_stat_set(mob, "experience", GET_LEVEL(mob) * base);
    char_stat_set(mob, "experience", GET_EXP(mob) * .9);
    char_stat_mod(mob, "experience", GET_LEVEL(mob) / 2);
    char_stat_mod(mob, "experience", GET_LEVEL(mob) / 3);
    if (IS_DRAGON(mob)) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 1.4);
    } else if (IS_ANDROID(mob)) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 1.25);
    } else if (IS_SAIYAN(mob)) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 1.1);
    } else if (IS_BIO(mob)) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 1.2);
    } else if (IS_MAJIN(mob)) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 1.25);
    } else if (IS_DEMON(mob)) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 1.1);
    } else if (GET_CLASS(mob) == CLASS_SHADOWDANCER) {
      char_stat_set(mob, "experience", GET_EXP(mob) * 2);
    }
    if (GET_CLASS(mob) == CLASS_NPC_COMMONER && IS_HUMANOID(mob) &&
        !IS_DRAGON(mob)) {
      if (!IS_ANDROID(mob) && !IS_SAIYAN(mob) && !IS_BIO(mob) &&
          !IS_MAJIN(mob)) {
        char_stat_set(mob, "experience", GET_EXP(mob) * 0.75);
      }
    }

    if (GET_LEVEL(mob) > 90) {
      char_stat_set(mob, "experience", GET_EXP(mob) * .7);
    } else if (GET_LEVEL(mob) > 80) {
      char_stat_set(mob, "experience", GET_EXP(mob) * .75);
    } else if (GET_LEVEL(mob) > 70) {
      char_stat_set(mob, "experience", GET_EXP(mob) * .8);
    } else if (GET_LEVEL(mob) > 60) {
      char_stat_set(mob, "experience", GET_EXP(mob) * .85);
    } else if (GET_LEVEL(mob) > 40) {
      char_stat_set(mob, "experience", GET_EXP(mob) * .9);
    } else if (GET_LEVEL(mob) > 30) {
      char_stat_set(mob, "experience", GET_EXP(mob) * .95);
    }

    if (GET_EXP(mob) > 20000000) {
      char_stat_set(mob, "experience", 20000000);
    }
  }

  mob->time.birth = time(0) - birth_age(mob);
  mob->time.created = mob->time.logon = time(0); /* why not */
  mob->time.maxage = mob->time.birth + max_age(mob);
  mob->time.played = 0;
  mob->time.logon = time(0);
  MOB_LOADROOM(mob) = NOWHERE;

  if (IS_HUMANOID(mob)) {
    SET_BIT_AR(MOB_FLAGS(mob), MOB_RARM);
    SET_BIT_AR(MOB_FLAGS(mob), MOB_LARM);
    SET_BIT_AR(MOB_FLAGS(mob), MOB_RLEG);
    SET_BIT_AR(MOB_FLAGS(mob), MOB_LLEG);
  }

  mob_proto_count_increment(nr);

  GET_ID(mob) = max_mob_id++;
  /* find_char helper */
  (void)char_register_id(GET_ID(mob), mob);

  assign_triggers(mob, MOB_TRIGGER);
  racial_body_parts(mob);

  if (GET_MOB_VNUM(mob) >= 800 && GET_MOB_VNUM(mob) <= 805) {
    number_of_assassins += 1;
  }

  char_game_activate(mob);
  return (mob);
}

struct char_data *mob_spawn(mob_vnum nr) { return read_mobile(nr, VIRTUAL); }

/* create an object, and add it to the object list */
struct obj_data *create_obj(void) {
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  (void)obj_register_id(GET_ID(obj), obj);

  obj->generation = time(0);

  assign_triggers(obj, OBJ_TRIGGER);

  return (obj);
}

/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;

  if (type == REAL) {
    mud_log("real is no longer supported!");
    exit(1);
  }

  struct obj_proto_data *proto = obj_proto_by_id(nr);
  if (!proto) {
    mud_log("WARNING: Object vnum %d does not exist in database.", nr);
    return (NULL);
  }

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj_proto_to_instance(obj, proto);
  OBJ_LOADROOM(obj) = NOWHERE;

  obj_proto_count_increment(nr);

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  (void)obj_register_id(GET_ID(obj), obj);

  obj->generation = time(0);

  assign_triggers(obj, OBJ_TRIGGER);
  if (GET_OBJ_VNUM(obj) == 65) {
    HCHARGE(obj) = 20;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
    if (GET_OBJ_VAL(obj, 1) == 0) {
      GET_OBJ_VAL(obj, 1) = GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL);
    }
    FOOB(obj) = GET_OBJ_VAL(obj, 1);
  }
  obj_game_activate(obj);
  return (obj);
}

struct obj_data *obj_spawn(obj_vnum nr) { return read_object(nr, VIRTUAL); }

// Schedule the next auto-reset for a single zone.
// lifespan is in minutes; we convert to milliseconds.
void zone_schedule_reset(struct zone_data *zone) {
  if (!zone || zone->reset_mode == 0) return;
  const int64_t lifespan_ms = zone->lifespan > 0
      ? (int64_t)zone->lifespan * 60000LL
      : 60000LL; // treat lifespan 0 as 1 minute to avoid busy-loop
  event_schedule_c(event_queue_now_ms() + lifespan_ms, 0, ev_zone_reset,
                   EQ_CTX_ZONE_ID, (int64_t)zone->id, 0);
}

// Called from main.zig after boot_db() has loaded all zones.
// Zones are staggered evenly across their lifespan so they don't all fire
// simultaneously every cycle.
void zone_schedule_all_resets(void) {
  // First pass: count active zones so we can compute even spacing.
  int total = 0;
  zone_iterate([&](auto zone) {
    if (zone && zone->reset_mode != 0) total++;
    return true;
  });
  if (total == 0) return;

  const int64_t now = event_queue_now_ms();
  int idx = 0;
  zone_iterate([&](auto zone) {
    if (!zone || zone->reset_mode == 0) return true;
    const int64_t lifespan_ms = zone->lifespan > 0
        ? (int64_t)zone->lifespan * 60000LL
        : 60000LL;
    // Spread initial deadlines evenly across the lifespan window.
    const int64_t offset_ms = (lifespan_ms * idx) / total;
    event_schedule_c(now + offset_ms, 0, ev_zone_reset,
                     EQ_CTX_ZONE_ID, (int64_t)zone->id, 0);
    idx++;
    return true;
  });
}

// Event handler: attempt to reset one zone.
// ctx_a = zone vnum.  If the zone is mode 1 and players are present, retry in 60 s.
void ev_zone_reset(int /*ctx_type*/, int64_t ctx_a, int64_t /*ctx_b*/) {
  struct zone_data *zone = zone_by_id((zone_vnum)ctx_a);
  if (!zone || zone->reset_mode == 0) return;

  if (zone->reset_mode == 2 || is_empty((zone_vnum)ctx_a)) {
    const int64_t t0 = event_queue_now_ms();
    reset_zone(zone);
    const int64_t elapsed = event_queue_now_ms() - t0;
    if (elapsed >= 100)
      mud_log("PERF: zone reset '%s' (zone %d) took %ldms",
              zone->name, zone->id, (long)elapsed);
    mudlog(CMP, ADMLVL_GOD, FALSE, "Auto zone reset: %s (Zone %d)",
           zone->name, zone->id);
    zone_schedule_reset(zone);
  } else {
    // Mode 1 and players are present: check again in 60 seconds.
    event_schedule_c(event_queue_now_ms() + 60000LL, 0, ev_zone_reset,
                     EQ_CTX_ZONE_ID, (int64_t)zone->id, 0);
  }
}

static void log_zone_error(struct zone_data *zone, int cmd_no,
                           const char *message) {
  mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: zone file: %s", message);
  struct reset_com *cmd = &zone->cmd[cmd_no];
  mudlog(NRM, ADMLVL_GOD, TRUE,
         "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
         cmd->command, zone->id, cmd->line);
}

#define ZONE_ERROR(message) log_zone_error(zone, cmd_no, message)

struct reset_context {
  struct zone_data *zone = nullptr;
  struct reset_com *cmd = nullptr;
  int cmd_no = 0;
  struct char_data *mob = nullptr;
  struct obj_data *obj = nullptr;
  struct char_data *tmob = nullptr;
  struct obj_data *tobj = nullptr;
  bool mob_load = false;
  bool obj_load = false;
  /* last object loaded per vnum during this reset, by id (ids stay safe if a
   * load trigger purges the object; re-resolved at use). 'P' commands target
   * containers loaded earlier in the same reset through this. */
  std::unordered_map<obj_vnum, int64_t> last_obj;
};

static void reset_track_obj(struct reset_context *ctx, struct obj_data *obj) {
  ctx->last_obj[GET_OBJ_VNUM(obj)] = GET_ID(obj);
}

static bool reset_command_mobile(struct reset_context *ctx, mob_vnum vnum,
                                 room_vnum rv, int max_in_room,
                                 int max_in_world, int percent_chance) {
  if (mob_proto_count_get(vnum) >= max_in_world) {
    return false;
  }
  if (rand_number(1, 100) < percent_chance) {
    return false;
  }
  auto room = room_by_id(rv);

  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;
  if (!room) {
    ZONE_ERROR("invalid room vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  };
  auto proto = mob_proto_by_id(vnum);
  if (!proto) {
    ZONE_ERROR("invalid mob vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  size_t count_total = mob_proto_count_get(vnum);

  if (max_in_world > 0 && count_total >= max_in_world) {
    return false;
  }

  size_t count_room = 0;
  if (max_in_room > 0)
    char_iterate_all([&](struct char_data *i) {
      if (GET_MOB_VNUM(i) == vnum && MOB_LOADROOM(i) == rv)
        count_room++;
      return true;
    });

  if (max_in_room > 0 && count_room >= max_in_room) {
    return false;
  }

  auto mob = read_mobile(vnum, VIRTUAL);
  char_to_room(mob, room);
  MOB_LOADROOM(mob) = rv;
  load_mtrigger(mob);
  ctx->mob = mob;
  ctx->tmob = mob;
  ctx->mob_load = true;

  return true;
}

static bool reset_command_object(struct reset_context *ctx, obj_vnum vnum,
                                 room_vnum rv, int max_in_room,
                                 int max_in_world, int percent_chance) {
  if (obj_proto_count_get(vnum) >= max_in_world) {
    return false;
  }
  if (rand_number(1, 100) < percent_chance) {
    return false;
  }
  auto room = room_by_id(rv);

  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;
  if (!room) {
    ZONE_ERROR("invalid room vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  };
  auto proto = obj_proto_by_id(vnum);
  if (!proto) {
    ZONE_ERROR("invalid obj vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  size_t count_total = obj_proto_count_get(vnum);

  if (max_in_world > 0 && count_total >= max_in_world) {
    return false;
  }

  size_t count_room = 0;
  if (max_in_room > 0)
    obj_iterate_all([&](struct obj_data *i) {
      if (GET_OBJ_VNUM(i) == vnum &&
          (OBJ_LOADROOM(i) == rv || (i->in_room && i->in_room == rv)))
        count_room++;
      return true;
    });

  if (max_in_room > 0 && count_room >= max_in_room) {
    return false;
  }

  auto obj = read_object(vnum, VIRTUAL);
  reset_track_obj(ctx, obj);
  obj_to_room(obj, room);
  OBJ_LOADROOM(obj) = rv;
  load_otrigger(obj);

  ctx->obj = obj;
  ctx->tobj = obj;
  ctx->obj_load = true;
  return true;
}

static bool reset_command_put(struct reset_context *ctx, obj_vnum vnum,
                              obj_vnum to_vnum, int percent_chance) {
  if (rand_number(1, 100) < percent_chance) {
    return false;
  }

  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;

  /* Prefer the container loaded earlier in this reset; fall back to the
   * newest instance world-wide (the old object_list head semantics). */
  struct obj_data *to = nullptr;
  if (auto it = ctx->last_obj.find(to_vnum); it != ctx->last_obj.end())
    to = obj_by_id(it->second);
  if (!to)
    to = get_obj_num(to_vnum);
  if (!to) {
    ZONE_ERROR("invalid to obj vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  auto obj_proto = obj_proto_by_id(vnum);
  if (!obj_proto) {
    ZONE_ERROR("invalid obj vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  auto obj = read_object(vnum, VIRTUAL);
  reset_track_obj(ctx, obj);
  obj_to_obj(obj, to);
  load_otrigger(obj);
  ctx->obj = obj;
  ctx->tobj = obj;

  return true;
}

static bool reset_command_give(struct reset_context *ctx, obj_vnum vnum,
                               int max_in_world, int percent_chance) {
  if (rand_number(1, 100) < percent_chance) {
    return false;
  }

  if (obj_proto_count_get(vnum) >= max_in_world) {
    return false;
  }

  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;

  if (!ctx->mob) {
    ZONE_ERROR("no mob to give to");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  auto obj_proto = obj_proto_by_id(vnum);
  if (!obj_proto) {
    ZONE_ERROR("invalid obj vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  auto obj = read_object(vnum, VIRTUAL);
  reset_track_obj(ctx, obj);
  obj_to_char(obj, ctx->mob);
  load_otrigger(obj);
  ctx->obj = obj;
  ctx->tobj = obj;

  return true;
}

static bool reset_command_equip(struct reset_context *ctx, obj_vnum vnum,
                                int max_in_world, int wear_loc,
                                int percent_chance) {
  if (rand_number(1, 100) < percent_chance) {
    return false;
  }

  if (obj_proto_count_get(vnum) >= max_in_world) {
    return false;
  }

  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;

  if (!ctx->mob) {
    ZONE_ERROR("no mob to equip");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  auto obj_proto = obj_proto_by_id(vnum);
  if (!obj_proto) {
    ZONE_ERROR("invalid obj vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  if (wear_loc < 0 || wear_loc >= NUM_WEARS) {
    ZONE_ERROR("invalid wear location");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  auto room = char_room_get(ctx->mob);

  auto obj = read_object(vnum, VIRTUAL);
  reset_track_obj(ctx, obj);

  obj->in_room = room_vnum_get(room);
  load_otrigger(obj);

  if (wear_otrigger(obj, ctx->mob, wear_loc)) {
    obj->in_room = NOWHERE;
    equip_char(ctx->mob, obj, wear_loc);
  } else {
    obj->in_room = NOWHERE;
    obj_to_char(obj, ctx->mob);
  }
  ctx->obj = obj;
  ctx->tobj = obj;

  return true;
}

static bool reset_command_remove(struct reset_context *ctx, room_vnum rv,
                                 obj_vnum vnum) {
  auto room = room_by_id(rv);
  struct obj_data *obj = nullptr;
  if (!room)
    goto finish;
  obj = get_obj_in_list_num(vnum, inv_for_room(room));
  if (obj)
    extract_obj(obj);

finish:
  ctx->obj = nullptr;
  ctx->mob = nullptr;
  return true;
}

static bool reset_command_door(struct reset_context *ctx, room_vnum rv, int dir,
                               int state) {
  auto room = room_by_id(rv);
  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;

  struct room_direction_data *exit = nullptr;
  if (!room)
    goto finish;
  exit = room_dir_option_get(room, dir);
  if (!exit)
    goto finish;

  switch (state) {
  case 0: /* open */
    exit_flag_set(exit, EX_CLOSED, false);
    break;
  case 1: /* closed */
    exit_flag_set(exit, EX_CLOSED, true);
    break;
  case 2: /* locked */
    exit_flag_set(exit, EX_CLOSED | EX_LOCKED, true);
    break;
  default:
    ZONE_ERROR("invalid door state");
    ctx->cmd->command = '*'; /* skip command */
    break;
  }

finish:
  ctx->obj = nullptr;
  ctx->mob = nullptr;
  return true;
}

static bool reset_command_trigger(struct reset_context *ctx, int attach_type,
                                  trig_vnum vnum, room_vnum rv) {
  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;

  auto proto = trig_proto_by_id(vnum);
  if (!proto) {
    ZONE_ERROR("invalid trigger vnum");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }

  switch (attach_type) {
  case MOB_TRIGGER:
    if (!ctx->mob) {
      ZONE_ERROR("no mob to attach trigger to");
      ctx->cmd->command = '*'; /* skip command */
      return false;
    }
    if (!SCRIPT(ctx->mob)) {
      CREATE(SCRIPT(ctx->mob), struct script_data, 1);
    }
    add_trigger(SCRIPT(ctx->mob), proto, -1);
    break;
  case OBJ_TRIGGER:
    if (!ctx->obj) {
      ZONE_ERROR("no obj to attach trigger to");
      ctx->cmd->command = '*'; /* skip command */
      return false;
    }
    if (!SCRIPT(ctx->obj)) {
      CREATE(SCRIPT(ctx->obj), struct script_data, 1);
    }
    add_trigger(SCRIPT(ctx->obj), proto, -1);
    break;
  case WLD_TRIGGER: {
    auto room = room_by_id(rv);
    if (!room) {
      ZONE_ERROR("invalid room vnum for trigger");
      ctx->cmd->command = '*'; /* skip command */
      return false;
    }
    struct script_data *sc = room_script_ensure(room);
    add_trigger(sc, proto, -1);
  } break;
  default:
    ZONE_ERROR("invalid trigger attach type");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }
  return true;
}

static bool reset_command_variable(struct reset_context *ctx, int attach_type,
                                   char *name, char *value, room_vnum rv,
                                   int unused) {
  auto zone = ctx->zone;
  auto cmd_no = ctx->cmd_no;

  switch (attach_type) {
  case MOB_TRIGGER:
    if (!ctx->mob) {
      ZONE_ERROR("no mob to attach variable to");
      ctx->cmd->command = '*'; /* skip command */
      return false;
    }
    if (!SCRIPT(ctx->mob)) {
      CREATE(SCRIPT(ctx->mob), struct script_data, 1);
    }
    add_var(&(SCRIPT(ctx->mob)->global_vars), name, value, rv);
    break;
  case OBJ_TRIGGER:
    if (!ctx->obj) {
      ZONE_ERROR("no obj to attach variable to");
      ctx->cmd->command = '*'; /* skip command */
      return false;
    }
    if (!SCRIPT(ctx->obj)) {
      CREATE(SCRIPT(ctx->obj), struct script_data, 1);
    }
    add_var(&(SCRIPT(ctx->obj)->global_vars), name, value, rv);
    break;
  case WLD_TRIGGER: {
    auto room = room_by_id(rv);
    if (!room) {
      ZONE_ERROR("invalid room vnum for variable");
      ctx->cmd->command = '*'; /* skip command */
      return false;
    }
    struct script_data *sc = room_script_ensure(room);
    add_var(&(sc->global_vars), name, value, unused);
  } break;
  default:
    ZONE_ERROR("invalid variable attach type");
    ctx->cmd->command = '*'; /* skip command */
    return false;
  }
  return true;
}

static void execute_reset_commands(struct zone_data *zone) {
  int cmd_no = 0;
  bool last_cmd = false;
  struct reset_context ctx;
  ctx.zone = zone;

  for (cmd_no = 0; zone->cmd[cmd_no].command != 'S'; cmd_no++) {
    ctx.cmd = &zone->cmd[cmd_no];
    if (ctx.cmd->if_flag && !last_cmd && !ctx.mob_load && !ctx.obj_load) {
      continue;
    }
    ctx.cmd_no = cmd_no;

    if (!ctx.cmd->if_flag) {
      ctx.mob_load = false;
      ctx.obj_load = false;
    }

    switch (ctx.cmd->command) {
    case '*':
      last_cmd = false;
      break;
    case 'M':
      last_cmd =
          reset_command_mobile(&ctx, ctx.cmd->arg1, ctx.cmd->arg3,
                               ctx.cmd->arg4, ctx.cmd->arg2, ctx.cmd->arg5);
      if (!last_cmd) {
        ctx.tobj = nullptr;
      }
      break;
    case 'O':
      last_cmd =
          reset_command_object(&ctx, ctx.cmd->arg1, ctx.cmd->arg3,
                               ctx.cmd->arg4, ctx.cmd->arg2, ctx.cmd->arg5);
      if (!last_cmd) {
        ctx.tmob = nullptr;
      }
      break;
    case 'P':
      last_cmd =
          reset_command_put(&ctx, ctx.cmd->arg1, ctx.cmd->arg3, ctx.cmd->arg5);
      if (!last_cmd) {
        ctx.tmob = nullptr;
      }
      break;
    case 'G':
      last_cmd =
          reset_command_give(&ctx, ctx.cmd->arg1, ctx.cmd->arg2, ctx.cmd->arg5);
      if (!last_cmd) {
        ctx.tmob = nullptr;
      }
      break;
    case 'E':
      last_cmd = reset_command_equip(&ctx, ctx.cmd->arg1, ctx.cmd->arg2,
                                     ctx.cmd->arg3, ctx.cmd->arg5);
      if (!last_cmd) {
        ctx.tmob = nullptr;
      }
      break;
    case 'R':
      last_cmd = reset_command_remove(&ctx, ctx.cmd->arg1, ctx.cmd->arg2);
      break;
    case 'D':
      last_cmd =
          reset_command_door(&ctx, ctx.cmd->arg1, ctx.cmd->arg2, ctx.cmd->arg3);
      break;
    case 'T':
      last_cmd = reset_command_trigger(&ctx, ctx.cmd->arg1, ctx.cmd->arg2,
                                       ctx.cmd->arg3);
      break;
    case 'V':
      last_cmd =
          reset_command_variable(&ctx, ctx.cmd->arg1, ctx.cmd->sarg1,
                                 ctx.cmd->sarg2, ctx.cmd->arg4, ctx.cmd->arg2);
      break;
    default:
      ZONE_ERROR("invalid command");
      break;
    }
  }
}

/* execute the reset command table of a given zone */
void reset_zone(struct zone_data *zone) {

  if (!pre_reset(zone)) {
    execute_reset_commands(zone);

    zone->age = 0;

    /* handle reset_wtrigger's */
    for (auto i = zone->bot; i <= zone->top; i++) {
      struct room_data *room = room_by_id(i);
      if (!room)
        continue;
      reset_wtrigger(room);
      if (room_flagged(room, ROOM_AURA) && rand_number(1, 5) >= 4) {
        send_to_room(room, "The aura of regeneration covering the surrounding "
                           "area disappears.\r\n");
        room_flag_set(room, ROOM_AURA, FALSE);
      }
      if (room_sector_type_get(room) == SECT_LAVA) {
        room_geffect_set(room, 5);
      }
      if (room_geffect_get(room) < -1) {
        send_to_room(room, "The area loses some of the water flooding it.\r\n");
        room_geffect_mod(room, 1);
      } else if (room_geffect_get(room) == -1) {
        send_to_room(room, "The area loses the last of the water flooding it "
                           "in one large rush.\r\n");
        room_geffect_set(room, 0);
      }
      if (room_dmg_get(room) >= 100) {
        send_to_room(room, "The area gets rebuilt a little.\r\n");
        room_dmg_mod(room, -rand_number(5, 10));
      } else if (room_dmg_get(room) >= 50) {
        send_to_room(room, "The area gets rebuilt a little.\r\n");
        room_dmg_mod(room, -rand_number(1, 10));
      } else if (room_dmg_get(room) >= 10) {
        send_to_room(room, "The area gets rebuilt a little.\r\n");
        room_dmg_mod(room, -rand_number(1, 10));
      } else if (room_dmg_get(room) > 1) {
        send_to_room(room, "The area gets rebuilt a little.\r\n");
        room_dmg_mod(room, -rand_number(1, room_dmg_get(room)));
      } else if (room_dmg_get(room) > 0) {
        send_to_room(room, "The area gets rebuilt a little.\r\n");
        room_dmg_mod(room, -1);
      }
      int sect = room_sector_type_get(room);
      if (room_geffect_get(room) >= 1 && rand_number(1, 4) == 4 &&
          !room_is_sunken(room) && sect != SECT_LAVA) {
        send_to_room(room, "The lava has cooled and become solid rock.\r\n");
        room_geffect_set(room, 0);
      } else if (room_geffect_get(room) >= 1 && rand_number(1, 2) == 2 &&
                 room_is_sunken(room) && sect != SECT_LAVA) {
        send_to_room(
            room,
            "The water has cooled the lava and it has become solid rock.\r\n");
        room_geffect_set(room, 0);
      }
    }
  } else {
    /* even if reset is blocked, age should be reset */
    zone->age = 0;
  }
  post_reset(zone);
}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr) {
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (char_room_get(i->character) == NULL)
      continue;
    if (char_room_get(i->character)->zone != zone_nr)
      continue;
    /*
     * if an immortal has nohassle off, he counts as present
     * added for testing zone reset triggers - Welcor
     */
    if (IS_NPC(i->character))
      continue; /* immortal switched into a mob */

    if ((GET_ADMLEVEL(i->character) >= ADMLVL_IMMORT) &&
        (PRF_FLAGGED(i->character, PRF_NOHASSLE)))
      continue;

    return (0);
  }

  return (1);
}

/************************************************************************
 *  funcs of a (more or less) general utility nature			*
 ************************************************************************/

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error) {
  char buf[MAX_STRING_LENGTH], tmp[520];
  char *point;
  int done = 0, length = 0, templength;

  *buf = *tmp = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      mud_log("SYSERR: fread_string: format error at string (pos %ld): %s at or "
          "near %s",
          ftell(fl),
          feof(fl)     ? "EOF"
          : ferror(fl) ? "read error"
                       : "unknown error",
          error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    /* now only removes trailing ~'s -- Welcor */
    for (point = tmp; *point && *point != '\r' && *point != '\n'; point++)
      ;
    if (point > tmp && point[-1] == '~') {
      *(--point) = '\0';
      done = 1;
    } else {
      *point = '\r';
      *(++point) = '\n';
      *(++point) = '\0';
    }

    templength = point - tmp;

    if (length + templength >= MAX_STRING_LENGTH) {
      mud_log("SYSERR: fread_string: string too large (db.c)");
      mud_log("%s", error);
      exit(1);
    } else {
      strcat(buf + length, tmp); /* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}

/* release memory allocated for a char struct */
void char_free_instance(struct char_data *ch) {
  int i;
  struct alias_data *a;

  while ((a = GET_ALIASES(ch)) != NULL) {
    GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
    free_alias(a);
  }
  if (ch->poofin)
    free(ch->poofin);
  if (ch->poofout)
    free(ch->poofout);
  if (ch->host)
    free(ch->host);
  for (i = 0; i < NUM_COLOR; i++)
    if (ch->color_choices[i])
      free(ch->color_choices[i]);

  if (ch->name)
    free(ch->name);
  if (ch->voice)
    free(ch->voice);
  if (ch->clan)
    free(ch->clan);
  if (ch->title)
    free(ch->title);
  if (ch->short_descr)
    free(ch->short_descr);
  if (ch->long_descr)
    free(ch->long_descr);
  if (ch->description)
    free(ch->description);

  if (!IS_NPC(ch)) {
    /* if this is a player, or a non-prototyped non-player, free all */

    for (i = 0; i < NUM_HIST; i++)
      if (GET_HISTORY(ch, i))
        free(GET_HISTORY(ch, i));

    /* free script proto list */
    free_proto_script(ch, MOB_TRIGGER);

  } else {
    if (ch->proto_script)
      free_proto_script(ch, MOB_TRIGGER);
  }

  /* free any assigned scripts */
  if (SCRIPT(ch))
    extract_script(ch, MOB_TRIGGER);

  if (ch->desc)
    ch->desc->character = NULL;

  /* find_char helper */
  /*
   * when free_char is called with a blank character struct, ID is set
   * to 0, and has not yet been added to the lookup table.
   */
  char_unregister_id(GET_ID(ch));

  char_zig_free(ch);

  free(ch);
}

void char_free_prototype(struct char_data *ch) {
  if (ch == NULL)
    return;

  if (ch->name)
    free(ch->name);
  if (ch->voice)
    free(ch->voice);
  if (ch->clan)
    free(ch->clan);
  if (ch->title)
    free(ch->title);
  if (ch->short_descr)
    free(ch->short_descr);
  if (ch->long_descr)
    free(ch->long_descr);
  if (ch->description)
    free(ch->description);

  if (ch->proto_script)
    free_proto_script(ch, MOB_TRIGGER);

  if (SCRIPT(ch))
    extract_script(ch, MOB_TRIGGER);

  char_zig_free(ch);
  free(ch);
}

void free_char(struct char_data *ch) { char_free_instance(ch); }

/* release memory allocated for an obj struct */
void obj_free_instance(struct obj_data *obj) {
  if (obj == NULL)
    return;

  free_object_strings(obj);
  if (obj->proto_script)
    free_proto_script(obj, OBJ_TRIGGER);

  /* Let's make sure that we free up this memory */
  if (obj->auctname) {
    free(obj->auctname);
  }

  /* free any assigned scripts */
  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  /* find_obj helper */
  obj_unregister_id(GET_ID(obj));

  free(obj);
}

void obj_free_prototype(struct obj_data *obj) {
  if (obj == NULL)
    return;

  free_object_strings(obj);

  if (obj->proto_script)
    free_proto_script(obj, OBJ_TRIGGER);

  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  if (obj->auctname)
    free(obj->auctname);

  free(obj);
}

void free_obj(struct obj_data *obj) { obj_free_instance(obj); }

/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
static int file_to_string_alloc(const char *name, char **buf) {
  int temppage;
  char temp[MAX_STRING_LENGTH];
  struct descriptor_data *in_use;

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  if (*buf)
    free(*buf);

  *buf = strdup(temp);
  return (0);
}

/* read contents of a text file, and place in buf */
static int file_to_string(const char *name, char *buf) {
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
void reset_char(struct char_data *ch) {
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  IN_ROOM(ch) = NOWHERE;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->time.logon = time(0);

  GET_LAST_TELL(ch) = NOBODY;
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch) {
  memset((char *)ch, 0, sizeof(struct char_data));

  IN_ROOM(ch) = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_VNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  char_position_set(ch, POS_STANDING);
  ch->mob_specials.default_pos = POS_STANDING;

  ch->size = SIZE_UNDEFINED;

  char_stat_set(ch, "armor", 0); /* Basic Armor */
}

void clear_object(struct obj_data *obj) {
  memset((char *)obj, 0, sizeof(struct obj_data));

  obj->proto_id = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  obj->worn_on = NOWHERE;
}

/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(struct char_data *ch) {
  int i;

  GET_ADMLEVEL(ch) = ADMLVL_NONE;
  GET_CLAN(ch) = strdup("None.");

  /* If this is our first player make him LVL_IMPL. */
  if (top_of_p_table == 0) {
    admin_set(ch, ADMLVL_IMPL);

    /* The implementor never goes through do_start(). */
    char_stat_set(ch, "ki", 1000);
    char_stat_set(ch, "powerlevel", 1000);
    char_stat_set(ch, "stamina", 1000);
  }

  /*ch->time.birth = time(0) - birth_age(ch);*/
  ch->time.maxage = ch->time.birth + max_age(ch);

  GET_HOME(ch) = 1;

  set_height_and_weight_by_race(ch);

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    mud_log("SYSERR: init_char: Character '%s' not found in player table.",
        GET_NAME(ch));

  for (i = 0; i < 3; i++) {
    const char *cond_name;
    switch (i) {
    case DRUNK:
      cond_name = "drunk";
      break;
    case HUNGER:
      cond_name = "hunger";
      break;
    case THIRST:
      cond_name = "thirst";
      break;
    default:
      continue;
    }
    char_stat_set(ch, cond_name, (GET_ADMLEVEL(ch) == ADMLVL_IMPL ? -1 : 48));
  }

  char_condition_add(ch, "system_hints", "init", "character");

  GET_LOADROOM(ch) = NOWHERE;
  SPEAKING(ch) = SKILL_LANG_COMMON;
  // initialize plrobjs so it doesn't complain on startup.
  Crash_crashsave(ch);
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
static int check_object(struct obj_proto_data *obj) {
  char objname[MAX_INPUT_LENGTH + 32];
  int error = FALSE, y;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    mud_log("SYSERR: Object #%d (%s) has negative weight (%" I64T ").",
        obj->id, obj->short_description, GET_OBJ_WEIGHT(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", obj->id,
           obj->short_description);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON: {
    char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj->name, ' ');

    strlcpy(onealias, space ? space + 1 : obj->name, sizeof(onealias));
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE)) {
      // mud_log("SYSERR: Object #%d (%s) doesn't have drink type as last alias.
      // (%s)", GET_OBJ_VNUM(obj), obj->short_description, obj->name);
    }
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if ((GET_OBJ_VAL(obj, 0) > 0) &&
        (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE)))
      mud_log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
          obj->id, obj->short_description, GET_OBJ_VAL(obj, 1),
          GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      mud_log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
          obj->id, obj->short_description, GET_OBJ_VAL(obj, 2),
          GET_OBJ_VAL(obj, 1));
    break;
  }

  return (error);
}

static int check_object_spell_number(struct obj_proto_data *obj, int val) {
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1 ||
      GET_OBJ_VAL(obj, val) == 0) /* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) >= SKILL_TABLE_SIZE)
    error = TRUE;
  if (skill_type(GET_OBJ_VAL(obj, val)) != SKTYPE_SPELL)
    error = TRUE;
  if (error)
    mud_log("SYSERR: Object #%d (%s) has out of range spell #%d.",
        obj->id, obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    mud_log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck) /* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !strcasecmp("UNDEFINED", spellname)) &&
      (error = TRUE))
    mud_log("SYSERR: Object #%d (%s) uses '%s' spell #%d.", obj->id,
        obj->short_description, spellname, GET_OBJ_VAL(obj, val));

  return (error);
}

static int check_object_level(struct obj_proto_data *obj, int val) {
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0) && (error = TRUE))
    mud_log("SYSERR: Object #%d (%s) has out of range level #%d.",
        obj->id, obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}

static int check_bitvector_names(bitvector_t bits, size_t namecount,
                                 const char *whatami, const char *whatbits) {
  unsigned int flagnum;
  bool error = FALSE;

  /* See if any bits are set above the ones we know about. */
  if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
    return (FALSE);

  for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
    if ((1 << flagnum) & bits) {
      mud_log("SYSERR: %s has unknown %s flag, bit %d (0 through %" SZT " known).",
          whatami, whatbits, flagnum, namecount - 1);
      error = TRUE;
    }

  return (error);
}

static int obj_save_fprintf(FILE *fp, struct obj_data *obj, const char *context,
                            const char *file, int line, const char *format,
                            ...) {
  va_list args;
  int result;

  va_start(args, format);
  result = vfprintf(fp, format, args);
  va_end(args);

  if (result < 0 || ferror(fp)) {
    int saved_errno = errno;
    mud_log("SYSERR: %s:%d: fprintf failed saving object #%d (%s): %s",
                  file, line, GET_OBJ_VNUM(obj), context,
                  saved_errno ? strerror(saved_errno) : "stream error");
    return 0;
  }

  return 1;
}

#define OBJ_SAVE_FPRINTF(fp, obj, context, ...)                                \
  obj_save_fprintf((fp), (obj), (context), __FILE__, __LINE__, __VA_ARGS__)

int my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate) {
  int counter2, i;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH + 1];
  char ebuf0[MAX_STRING_LENGTH], ebuf1[MAX_STRING_LENGTH];
  char ebuf2[MAX_STRING_LENGTH], ebuf3[MAX_STRING_LENGTH];

  if (obj->action_description) {
    strcpy(buf1, obj->action_description);
    strip_string(buf1);
  } else
    *buf1 = 0;

  sprintascii(ebuf0, GET_OBJ_EXTRA(obj)[0]);
  sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[1]);
  sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[2]);
  sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[3]);

  if (!OBJ_SAVE_FPRINTF(
          fp, obj, "base object data",
          "#%d\n"
          "%d %d %d %d %d %d %d %d %d %s %s %s %s %d %d %d %d %d %d %d %d\n",
          GET_OBJ_VNUM(obj), locate, GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
          GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4),
          GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7), ebuf0,
          ebuf1, ebuf2, ebuf3, GET_OBJ_VAL(obj, 8), GET_OBJ_VAL(obj, 9),
          GET_OBJ_VAL(obj, 10), GET_OBJ_VAL(obj, 11), GET_OBJ_VAL(obj, 12),
          GET_OBJ_VAL(obj, 13), GET_OBJ_VAL(obj, 14), GET_OBJ_VAL(obj, 15)))
    return 0;

  if (!(OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE)) &&
      !GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
    return 1;
  }

  if (!OBJ_SAVE_FPRINTF(
          fp, obj, "XAP object data",
          "XAP\n"
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%d %d %d %d %d %" I64T " %d %d\n",
          obj->name ? obj->name : "undefined",
          obj->short_description ? obj->short_description : "undefined",
          obj->description ? obj->description : "undefined", buf1,
          GET_OBJ_TYPE(obj), GET_OBJ_WEAR(obj)[0], GET_OBJ_WEAR(obj)[1],
          GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3], GET_OBJ_WEIGHT(obj),
          GET_OBJ_COST(obj), 0))
    return 0;

  if (obj->generation)
    if (!OBJ_SAVE_FPRINTF(fp, obj, "generation", "G\n%ld\n", obj->generation))
      return 0;

  if (!OBJ_SAVE_FPRINTF(fp, obj, "size", "Z\n%d\n", GET_OBJ_SIZE(obj)))
    return 0;

  /* Do we have affects? */
  for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
    if (obj->affected[counter2].modifier)
      if (!OBJ_SAVE_FPRINTF(fp, obj, "affect",
                            "A\n"
                            "%d %d %d\n",
                            obj->affected[counter2].location,
                            obj->affected[counter2].modifier,
                            obj->affected[counter2].specific))
        return 0;

  /* Do we have extra descriptions? */
  if (obj->ex_description) { /*. Yep, save them too . */
    for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
      /*. Sanity check to prevent nasty protection faults . */
      if (!*ex_desc->keyword || !*ex_desc->description) {
        continue;
      }
      strcpy(buf1, ex_desc->description);
      strip_string(buf1);
      if (!OBJ_SAVE_FPRINTF(fp, obj, "extra description",
                            "E\n"
                            "%s~\n"
                            "%s~\n",
                            ex_desc->keyword, buf1))
        return 0;
    }
  }

  return 1;
}

#undef OBJ_SAVE_FPRINTF

/* This procedure removes the '\r\n' from a string so that it may be
  saved to a file.  Use it only on buffers, not on the orginal
  strings. */

void strip_string(char *buffer) {
  char *ptr, *str;

  ptr = buffer;
  str = ptr;

  while ((*str = *ptr)) {
    str++;
    ptr++;
    if (*ptr == '\r')
      ptr++;
  }
}

/* External variables from config.c */
extern int crash_file_timeout;
extern int rent_file_timeout;

void load_default_config(void) {
  /****************************************************************************/
  /** This function is called only once, at boot-time.                       **/
  /** - We assume config_info is empty                          -- Welcor    **/
  /****************************************************************************/
  /****************************************************************************/
  /** Game play options.                                                     **/
  /****************************************************************************/
  CONFIG_PK_ALLOWED = pk_allowed;
  CONFIG_PT_ALLOWED = pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT = level_can_shout;
  CONFIG_HOLLER_MOVE_COST = holler_move_cost;
  CONFIG_TUNNEL_SIZE = tunnel_size;
  CONFIG_MAX_EXP_GAIN = max_exp_gain;
  CONFIG_MAX_EXP_LOSS = max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME = max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME = max_pc_corpse_time;
  CONFIG_IDLE_VOID = idle_void;
  CONFIG_IDLE_RENT_TIME = idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL = idle_max_level;
  CONFIG_DTS_ARE_DUMPS = dts_are_dumps;
  CONFIG_LOAD_INVENTORY = load_into_inventory;
  CONFIG_OK = strdup(OK);
  CONFIG_NOPERSON = strdup(NOPERSON);
  CONFIG_NOEFFECT = strdup(NOEFFECT);
  CONFIG_TRACK_T_DOORS = track_through_doors;
  CONFIG_LEVEL_CAP = level_cap;
  CONFIG_STACK_MOBS = show_mob_stacking;
  CONFIG_STACK_OBJS = show_obj_stacking;
  CONFIG_MOB_FIGHTING = mob_fighting;
  CONFIG_DISP_CLOSED_DOORS = disp_closed_doors;
  CONFIG_REROLL_PLAYER_CREATION = reroll_status;
  CONFIG_INITIAL_POINTS_POOL = initial_points;
  CONFIG_ENABLE_COMPRESSION = enable_compression;
  CONFIG_ENABLE_LANGUAGES = enable_languages;
  CONFIG_ALL_ITEMS_UNIQUE = all_items_unique;
  CONFIG_EXP_MULTIPLIER = exp_multiplier;
  /****************************************************************************/
  /** Rent / crashsave options.                                              **/
  /****************************************************************************/
  CONFIG_FREE_RENT = free_rent;
  CONFIG_MAX_OBJ_SAVE = max_obj_save;
  CONFIG_MIN_RENT_COST = min_rent_cost;
  CONFIG_AUTO_SAVE = auto_save;
  CONFIG_AUTOSAVE_TIME = autosave_time;
  CONFIG_CRASH_TIMEOUT = crash_file_timeout;
  CONFIG_RENT_TIMEOUT = rent_file_timeout;

  /****************************************************************************/
  /** Room numbers.                                                          **/
  /****************************************************************************/
  CONFIG_MORTAL_START = mortal_start_room;
  CONFIG_IMMORTAL_START = immort_start_room;
  CONFIG_FROZEN_START = frozen_start_room;
  CONFIG_DON_ROOM_1 = donation_room_1;
  CONFIG_DON_ROOM_2 = donation_room_2;
  CONFIG_DON_ROOM_3 = donation_room_3;

  /****************************************************************************/
  /** Game operation options.                                                **/
  /****************************************************************************/
  CONFIG_DFLT_PORT = DFLT_PORT;

  if (DFLT_IP)
    CONFIG_DFLT_IP = strdup(DFLT_IP);
  else
    CONFIG_DFLT_IP = NULL;

  CONFIG_DFLT_DIR = strdup(DFLT_DIR);

  if (LOGNAME)
    CONFIG_LOGNAME = strdup(LOGNAME);
  else
    CONFIG_LOGNAME = NULL;

  CONFIG_MAX_PLAYING = max_playing;
  CONFIG_MAX_FILESIZE = max_filesize;
  CONFIG_MAX_BAD_PWS = max_bad_pws;
  CONFIG_SITEOK_ALL = siteok_everyone;
  CONFIG_NS_IS_SLOW = nameserver_is_slow;
  CONFIG_NEW_SOCIALS = use_new_socials;
  CONFIG_OLC_SAVE = auto_save_olc;
  CONFIG_MENU = strdup(MENU);
  CONFIG_WELC_MESSG = strdup(WELC_MESSG);
  CONFIG_START_MESSG = strdup(START_MESSG);
  CONFIG_EXP_MULTIPLIER = 1.0;

  /****************************************************************************/
  /** Autowiz options.                                                       **/
  /****************************************************************************/
  CONFIG_USE_AUTOWIZ = use_autowiz;
  CONFIG_MIN_WIZLIST_LEV = min_wizlist_lev;

  /****************************************************************************/
  /** Character advancement options.                                         **/
  /****************************************************************************/
  CONFIG_ALLOW_MULTICLASS = allow_multiclass;
  CONFIG_ALLOW_PRESTIGE = allow_prestige;

  /****************************************************************************/
  /** ticks menu                                                             **/
  /****************************************************************************/
  CONFIG_PULSE_VIOLENCE = pulse_violence;
  CONFIG_PULSE_MOBILE = pulse_mobile;
  CONFIG_PULSE_ZONE = pulse_zone;
  CONFIG_PULSE_CURRENT = pulse_current;
  CONFIG_PULSE_SANITY = pulse_sanity;
  CONFIG_PULSE_IDLEPWD = pulse_idlepwd;
  CONFIG_PULSE_AUTOSAVE = pulse_autosave;
  CONFIG_PULSE_USAGE = pulse_usage;
  CONFIG_PULSE_TIMESAVE = pulse_timesave;

  /****************************************************************************/
  /** Character Creation Method                                              **/
  /****************************************************************************/
  CONFIG_CREATION_METHOD = method;
}

void load_config(void) {
  FILE *fl;
  char line[MAX_STRING_LENGTH];
  char tag[MAX_INPUT_LENGTH];
  int num;
  float fum;
  char buf[MAX_INPUT_LENGTH];

  load_default_config();

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
    num = atoi(line);
    fum = atof(line);

    switch (LOWER(*tag)) {
    case 'a':
      if (!strcasecmp(tag, "auto_save"))
        CONFIG_AUTO_SAVE = num;
      else if (!strcasecmp(tag, "autosave_time"))
        CONFIG_AUTOSAVE_TIME = num;
      else if (!strcasecmp(tag, "auto_save_olc"))
        CONFIG_OLC_SAVE = num;
      else if (!strcasecmp(tag, "allow_multiclass"))
        CONFIG_ALLOW_MULTICLASS = num;
      else if (!strcasecmp(tag, "allow_prestige"))
        CONFIG_ALLOW_PRESTIGE = num;
      else if (!strcasecmp(tag, "auto_level"))
        mud_log("ignoring obsolete config option auto_level");
      else if (!strcasecmp(tag, "all_items_unique"))
        CONFIG_ALL_ITEMS_UNIQUE = num;
      break;

    case 'c':
      if (!strcasecmp(tag, "crash_file_timeout"))
        CONFIG_CRASH_TIMEOUT = num;
      else if (!strcasecmp(tag, "compression")) {
        CONFIG_ENABLE_COMPRESSION = num;
      }
      break;

    case 'd':
      if (!strcasecmp(tag, "disp_closed_doors"))
        CONFIG_DISP_CLOSED_DOORS = num;
      else if (!strcasecmp(tag, "dts_are_dumps"))
        CONFIG_DTS_ARE_DUMPS = num;
      else if (!strcasecmp(tag, "donation_room_1"))
        if (num == -1)
          CONFIG_DON_ROOM_1 = NOWHERE;
        else
          CONFIG_DON_ROOM_1 = num;
      else if (!strcasecmp(tag, "donation_room_2"))
        if (num == -1)
          CONFIG_DON_ROOM_2 = NOWHERE;
        else
          CONFIG_DON_ROOM_2 = num;
      else if (!strcasecmp(tag, "donation_room_3"))
        if (num == -1)
          CONFIG_DON_ROOM_3 = NOWHERE;
        else
          CONFIG_DON_ROOM_3 = num;
      else if (!strcasecmp(tag, "dflt_dir")) {
        if (CONFIG_DFLT_DIR)
          free(CONFIG_DFLT_DIR);
        if (line != NULL && *line)
          CONFIG_DFLT_DIR = strdup(line);
        else
          CONFIG_DFLT_DIR = strdup(DFLT_DIR);
      } else if (!strcasecmp(tag, "dflt_ip")) {
        if (CONFIG_DFLT_IP)
          free(CONFIG_DFLT_IP);
        if (line != NULL && *line)
          CONFIG_DFLT_IP = strdup(line);
        else
          CONFIG_DFLT_IP = NULL;
      } else if (!strcasecmp(tag, "dflt_port"))
        CONFIG_DFLT_PORT = num;
      break;

    case 'e':
      if (!strcasecmp(tag, "enable_languages"))
        CONFIG_ENABLE_LANGUAGES = num;
      else if (!strcasecmp(tag, "exp_multiplier"))
        CONFIG_EXP_MULTIPLIER = fum;
      break;

    case 'f':
      if (!strcasecmp(tag, "free_rent"))
        CONFIG_FREE_RENT = num;
      else if (!strcasecmp(tag, "frozen_start_room"))
        CONFIG_FROZEN_START = num;
      break;

    case 'h':
      if (!strcasecmp(tag, "holler_move_cost"))
        CONFIG_HOLLER_MOVE_COST = num;
      break;

    case 'i':
      if (!strcasecmp(tag, "idle_void"))
        CONFIG_IDLE_VOID = num;
      else if (!strcasecmp(tag, "idle_rent_time"))
        CONFIG_IDLE_RENT_TIME = num;
      else if (!strcasecmp(tag, "idle_max_level")) {
        if (num >= CONFIG_LEVEL_CAP)
          num += 1 - CONFIG_LEVEL_CAP;
        CONFIG_IDLE_MAX_LEVEL = num;
      } else if (!strcasecmp(tag, "immort_level_ok"))
        mud_log("Ignoring immort_level_ok obsolete config");
      else if (!strcasecmp(tag, "immort_start_room"))
        CONFIG_IMMORTAL_START = num;
      else if (!strcasecmp(tag, "initial_points"))
        CONFIG_INITIAL_POINTS_POOL = num;
      break;

    case 'l':
      if (!strcasecmp(tag, "level_can_shout"))
        CONFIG_LEVEL_CAN_SHOUT = num;
      else if (!strcasecmp(tag, "level_cap"))
        CONFIG_LEVEL_CAP = num;
      else if (!strcasecmp(tag, "load_into_inventory"))
        CONFIG_LOAD_INVENTORY = num;
      else if (!strcasecmp(tag, "logname")) {
        if (CONFIG_LOGNAME)
          free(CONFIG_LOGNAME);
        if (line != NULL && *line)
          CONFIG_LOGNAME = strdup(line);
        else
          CONFIG_LOGNAME = NULL;
      }
      break;

    case 'm':
      if (!strcasecmp(tag, "max_bad_pws"))
        CONFIG_MAX_BAD_PWS = num;
      else if (!strcasecmp(tag, "max_exp_gain"))
        CONFIG_MAX_EXP_GAIN = num;
      else if (!strcasecmp(tag, "max_exp_loss"))
        CONFIG_MAX_EXP_LOSS = num;
      else if (!strcasecmp(tag, "max_filesize"))
        CONFIG_MAX_FILESIZE = num;
      else if (!strcasecmp(tag, "max_npc_corpse_time"))
        CONFIG_MAX_NPC_CORPSE_TIME = num;
      else if (!strcasecmp(tag, "max_obj_save"))
        CONFIG_MAX_OBJ_SAVE = num;
      else if (!strcasecmp(tag, "max_pc_corpse_time"))
        CONFIG_MAX_PC_CORPSE_TIME = num;
      else if (!strcasecmp(tag, "max_playing"))
        CONFIG_MAX_PLAYING = num;
      else if (!strcasecmp(tag, "menu")) {
        if (CONFIG_MENU)
          free(CONFIG_MENU);
        strncpy(buf, "Reading menu in load_config()", sizeof(buf));
        CONFIG_MENU = fread_string(fl, buf);
      } else if (!strcasecmp(tag, "min_rent_cost"))
        CONFIG_MIN_RENT_COST = num;
      else if (!strcasecmp(tag, "min_wizlist_lev")) {
        if (num >= CONFIG_LEVEL_CAP)
          num += 1 - CONFIG_LEVEL_CAP;
        CONFIG_MIN_WIZLIST_LEV = num;
      } else if (!strcasecmp(tag, "mob_fighting"))
        CONFIG_MOB_FIGHTING = num;
      else if (!strcasecmp(tag, "mortal_start_room"))
        CONFIG_MORTAL_START = num;
      else if (!strcasecmp(tag, "method"))
        CONFIG_CREATION_METHOD = num;
      break;

    case 'n':
      if (!strcasecmp(tag, "nameserver_is_slow"))
        CONFIG_NS_IS_SLOW = num;
      else if (!strcasecmp(tag, "noperson")) {
        char tmp[READ_SIZE];
        if (CONFIG_NOPERSON)
          free(CONFIG_NOPERSON);
        snprintf(tmp, sizeof(tmp), "%s\r\n", line);
        CONFIG_NOPERSON = strdup(tmp);
      } else if (!strcasecmp(tag, "noeffect")) {
        char tmp[READ_SIZE];
        if (CONFIG_NOEFFECT)
          free(CONFIG_NOEFFECT);
        snprintf(tmp, sizeof(tmp), "%s\r\n", line);
        CONFIG_NOEFFECT = strdup(tmp);
      }
      break;

    case 'o':
      if (!strcasecmp(tag, "ok")) {
        char tmp[READ_SIZE];
        if (CONFIG_OK)
          free(CONFIG_OK);
        snprintf(tmp, sizeof(tmp), "%s\r\n", line);
        CONFIG_OK = strdup(tmp);
      }
      break;

    case 'p':
      if (!strcasecmp(tag, "pk_allowed"))
        CONFIG_PK_ALLOWED = num;
      else if (!strcasecmp(tag, "pt_allowed"))
        CONFIG_PT_ALLOWED = num;
      else if (!strcasecmp(tag, "pulse_viol"))
        CONFIG_PULSE_VIOLENCE = num;
      else if (!strcasecmp(tag, "pulse_mobile"))
        CONFIG_PULSE_MOBILE = num;
      else if (!strcasecmp(tag, "pulse_current"))
        CONFIG_PULSE_CURRENT = num;
      else if (!strcasecmp(tag, "pulse_zone"))
        CONFIG_PULSE_ZONE = num;
      else if (!strcasecmp(tag, "pulse_autosave"))
        CONFIG_PULSE_AUTOSAVE = num;
      else if (!strcasecmp(tag, "pulse_usage"))
        CONFIG_PULSE_USAGE = num;
      else if (!strcasecmp(tag, "pulse_sanity"))
        CONFIG_PULSE_SANITY = num;
      else if (!strcasecmp(tag, "pulse_timesave"))
        CONFIG_PULSE_TIMESAVE = num;
      else if (!strcasecmp(tag, "pulse_idlepwd"))
        CONFIG_PULSE_IDLEPWD = num;
      break;

    case 'r':
      if (!strcasecmp(tag, "rent_file_timeout"))
        CONFIG_RENT_TIMEOUT = num;
      else if (!strcasecmp(tag, "reroll_stats"))
        CONFIG_REROLL_PLAYER_CREATION = num;
      break;

    case 's':
      if (!strcasecmp(tag, "siteok_everyone"))
        CONFIG_SITEOK_ALL = num;
      else if (!strcasecmp(tag, "start_messg")) {
        strncpy(buf, "Reading start message in load_config()", sizeof(buf));
        if (CONFIG_START_MESSG)
          free(CONFIG_START_MESSG);
        CONFIG_START_MESSG = fread_string(fl, buf);
      } else if (!strcasecmp(tag, "stack_mobs"))
        CONFIG_STACK_MOBS = num;
      else if (!strcasecmp(tag, "stack_objs"))
        CONFIG_STACK_OBJS = num;
      break;

    case 't':
      if (!strcasecmp(tag, "tunnel_size"))
        CONFIG_TUNNEL_SIZE = num;
      else if (!strcasecmp(tag, "track_through_doors"))
        CONFIG_TRACK_T_DOORS = num;
      break;

    case 'u':
      if (!strcasecmp(tag, "use_autowiz"))
        CONFIG_USE_AUTOWIZ = num;
      else if (!strcasecmp(tag, "use_new_socials"))
        CONFIG_NEW_SOCIALS = num;
      break;

    case 'w':
      if (!strcasecmp(tag, "welc_messg")) {
        strncpy(buf, "Reading welcome message in load_config()", sizeof(buf));
        if (CONFIG_WELC_MESSG)
          free(CONFIG_WELC_MESSG);
        CONFIG_WELC_MESSG = fread_string(fl, buf);
      }
      break;

    default:
      break;
    }
  }

  fclose(fl);
}
