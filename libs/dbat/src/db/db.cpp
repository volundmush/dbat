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
#include <algorithm>
#include <cstdlib>
#include <memory>

#include <linux/limits.h>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/db.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/world_files.hpp"
#include "legacy_loaders.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/help.hpp"
#include "dbat/text_files.hpp"
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
#include "dbat/io/paths.hpp"
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


/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/






/* local functions */
static void dragon_level(GameWorld *g, struct char_data *ch);
static void check_start_rooms(GameWorld *g);
static void renum_world(GameWorld *g);
static void renum_zone_table(GameWorld *g);
static void log_zone_error(GameWorld *g, struct zone_data *zone, int cmd_no, const char *message);
static void reset_time(GameWorld *g);
static void free_obj_unique_hash(GameWorld *g);

/* external functions */

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

void mob_stats(GameWorld *g, struct char_data *mob)
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

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

io::loaded<void> boot_world(GameWorld *g)
{
  mud_log("Loading level tables.");
  load_levels(g);

  mud_log("Loading zone table.");
  if (io::loaded<void> booted = boot_index(g, DB_BOOT_ZON); !booted)
    return booted;

  mud_log("Loading triggers and generating index.");
  if (io::loaded<void> booted = boot_index(g, DB_BOOT_TRG); !booted)
    return booted;

  mud_log("Loading rooms.");
  if (io::loaded<void> booted = boot_index(g, DB_BOOT_WLD); !booted)
    return booted;

  mud_log("Renumbering rooms.");
  renum_world(g);

  mud_log("Checking start rooms.");
  check_start_rooms(g);

  mud_log("Loading mobs and generating index.");
  if (io::loaded<void> booted = boot_index(g, DB_BOOT_MOB); !booted)
    return booted;

  mud_log("Loading objs and generating index.");
  if (io::loaded<void> booted = boot_index(g, DB_BOOT_OBJ); !booted)
    return booted;

  mud_log("Renumbering zone table.");
  renum_zone_table(g);

  mud_log("Loading disabled commands list...");
  load_disabled(g);

  if (!g->no_specials) {
    mud_log("Loading shops.");
    if (io::loaded<void> booted = boot_index(g, DB_BOOT_SHP); !booted)
      return booted;

    mud_log("Loading guild masters.");
    if (io::loaded<void> booted = boot_index(g, DB_BOOT_GLD); !booted)
      return booted;
  }
  if (g->SELFISHMETER >= 10) {
    mud_log("Loading Shadow Dragons.");
    load_shadow_dragons(g);
  }
  return {};
}


/* Free the world, in a memory allocation sense. */
void destroy_db(GameWorld *g)
{
  ssize_t itr;

  free_extracted_triggers(g);
  free_extracted_objects(g, NOTHING); /* before the prototypes whose strings they share */

  /* Active Mobiles & Players */
  /* stop_follower() reads the master, so everyone must stop before anyone is freed */
  for (char_data *tch : each_char(g))
    if (char_get_master(g, tch))
      stop_follower(g, tch);
  for (long id : g->characters.ids())
    if (struct char_data *tch = g->characters.find(id))
      free_char(g, tch);
  g->characters.clear();

  /* Active Objects */
  for (long id : g->objects.ids())
    if (struct obj_data *obj = g->objects.find(id))
      free_obj(g, obj);
  g->objects.clear();

  /* Rooms */
  for (room_vnum cnt : g->rooms.vnums()) {

    /* free any assigned scripts */
    if (room_get_script(g, cnt))
      extract_script(g, room_of(g, cnt));
    /* its strings, exits, extra descriptions and trigger list go with it */
  }
  g->rooms.clear();

  /* Objects */
  g->obj_proto.clear();
  g->obj_index.clear();

  /* Mobiles */
  for (mob_vnum mvnum : g->mob_proto.vnums()) {
    while (!char_affects(mob_proto_of(g, mvnum)).empty())
      affect_remove(g, mob_proto_of(g, mvnum), &char_affects(mob_proto_of(g, mvnum)).front());
  }
  g->mob_proto.clear();
  g->mob_index.clear();

  /* Shops */
  destroy_shops(g);

  /* Guilds */
  destroy_guilds(g);

  /* Zones */
  g->reset_q.clear();

#define THIS_CMD zone->cmd[itr]

  for (struct zone_data *zone : g->zones.all()) {
    if (!zone->cmd.empty()) {
      /* the variables defined in this zone own their strings */
      for (itr = 0; THIS_CMD.command != 'S'; itr++)
        if (THIS_CMD.command == 'V') {
          if (THIS_CMD.sarg1)
            free(THIS_CMD.sarg1);
          if (THIS_CMD.sarg2)
            free(THIS_CMD.sarg2);
        }
    }
  }
  g->zones.clear();

#undef THIS_CMD

  /* Triggers */
  for (struct index_data *entry : g->trig_index.all()) {
    if (entry->proto)
      free_trigger(g, entry->proto);
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
  load_text_pages(g);

  mud_log("Loading spell definitions.");
  mag_assign_spells(g);

  mud_log("Loading feats.");
  assign_feats(g);

  if (io::loaded<void> booted = boot_world(g); !booted) {
    mud_log("SYSERR: {}", booted.error().text());
    exit(1);
  }

  mud_log("Loading help entries.");
  index_boot(g, DB_BOOT_HLP);

  mud_log("Setting up context sensitive help system for OLC");
  boot_context_help(g);

  mud_log("Generating player index.");
  build_player_index(g);

  if (g->ERAPLAYERS <= 0)
    g->ERAPLAYERS = g->top_of_p_table + 1;

  io::ensure_directory(LIB_PLROBJS "CRASH");

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
  if (io::loaded<void> socials = read_socials(g); !socials) {
    mud_log("SYSERR: {}", socials.error().text());
    exit(1);
  }

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
  if (io::loaded<void> names = Read_Invalid_List(g); !names) {
    mud_log("SYSERR: {}", names.error().text());
    exit(1);
  }

  if (!g->no_rent_check) {
    mud_log("Deleting timed-out crash and rent files:");
    update_obj_file(g);
    mud_log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!g->mini_mud) {
    mud_log("Booting houses.");
    if (io::loaded<void> houses = House_boot(g); !houses) {
      mud_log("SYSERR: {}", houses.error().text());
      exit(1);
    }
  }

  for (struct zone_data *i : g->zones.all()) {
    mud_log("Resetting #{:d}: {} (rooms {:d}-{:d}).", i->number,
            i->name, i->bot, i->top);
    reset_zone(g, i);
  }

  g->reset_q.clear();

  g->boot_time = game_now(g);

  mud_log("Boot db -- DONE.");
}

static void reset_time(GameWorld *g)
{
  time_t beginning_of_time = 0;

  read_time_file(g, beginning_of_time);

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

  mud_log("   Current Gametime: {:d}H {:d}D {:d}M {:d}Y.", g->time_info.hours,
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

bitvector_t asciiflag_conv(char *flag)
{
  return io::ascii_flags(flag, 0, "asciiflag_conv");
}

/* make sure the start rooms exist, falling back to the Void */
static void check_start_rooms(GameWorld *g)
{
  if (!room_of(g, g->r_mortal_start_room = CONFIG_MORTAL_START)) {
    mud_log("SYSERR:  Mortal start room does not exist.  Change mortal_start_room in lib/etc/config.");
    g->r_mortal_start_room = 0; /* The Void */
  }
  if (!room_of(g, g->r_immort_start_room = CONFIG_IMMORTAL_START)) {
    if (!g->mini_mud)
      mud_log("SYSERR:  Warning: Immort start room does not exist.  Change immort_start_room in /lib/etc/config.");
    g->r_immort_start_room = g->r_mortal_start_room;
  }
  if (!room_of(g, g->r_frozen_start_room = CONFIG_FROZEN_START)) {
    if (!g->mini_mud)
      mud_log("SYSERR:  Warning: Frozen start room does not exist.  Change frozen_start_room in /lib/etc/config.");
    g->r_frozen_start_room = g->r_mortal_start_room;
  }
}

/* exits to rooms that don't exist lead nowhere */
static void renum_world(GameWorld *g)
{
  g->rooms.visit([g](room_vnum, struct room_data *room) {
    for (int door = 0; door < NUM_OF_DIRS; door++) {
      struct room_direction_data *exit = room_get_dir_option(room, door);
      if (exit && !room_of(g, exit->to_room))
        exit->to_room = NOWHERE;
    }
  });
}

#define ZCMD2 zone->cmd[cmd_no]

/*
 * Once all of the zone reset tables have been loaded, check that every room,
 * mobile, object and trigger they name exists, and disable the commands that
 * name one that doesn't.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_vnum) >= (sizeof(mob_vnum) and sizeof(obj_vnum))
 */
static void renum_zone_table(GameWorld *g)
{
  int cmd_no;
  room_vnum a, b, c, olda, oldb, oldc;
  std::string buf;

  for (struct zone_data *zone : g->zones.all())
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda      = ZCMD2.arg1;
      oldb      = ZCMD2.arg2;
      oldc      = ZCMD2.arg3;
      switch (ZCMD2.command) {
        case 'M':
          a = ZCMD2.arg1 = mob_proto_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOBODY;
          c = ZCMD2.arg3 = room_of(g, ZCMD2.arg3) ? ZCMD2.arg3 : NOWHERE;
          break;
        case 'O':
          a = ZCMD2.arg1 = obj_proto_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOTHING;
          if (ZCMD2.arg3 != NOWHERE)
            c = ZCMD2.arg3 = room_of(g, ZCMD2.arg3) ? ZCMD2.arg3 : NOWHERE;
          break;
        case 'G':
          a = ZCMD2.arg1 = obj_proto_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOTHING;
          break;
        case 'E':
          a = ZCMD2.arg1 = obj_proto_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOTHING;
          break;
        case 'P':
          a = ZCMD2.arg1 = obj_proto_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOTHING;
          c = ZCMD2.arg3 = obj_proto_of(g, ZCMD2.arg3) ? ZCMD2.arg3 : NOTHING;
          break;
        case 'D':
          a = ZCMD2.arg1 = room_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOWHERE;
          break;
        case 'R': /* rem obj from room */
          a = ZCMD2.arg1 = room_of(g, ZCMD2.arg1) ? ZCMD2.arg1 : NOWHERE;
          b = ZCMD2.arg2 = obj_proto_of(g, ZCMD2.arg2) ? ZCMD2.arg2 : NOTHING;
          break;
        case 'T': /* a trigger: it keeps the vnum, so check that there is one */
          b = ZCMD2.arg2 = find_trigger(g, ZCMD2.arg2) ? ZCMD2.arg2 : NOTHING;
          c = ZCMD2.arg3 = room_of(g, ZCMD2.arg3) ? ZCMD2.arg3 : NOWHERE;
          break;
        case 'V': /* trigger variable assignment */
          b = ZCMD2.arg3 = room_of(g, ZCMD2.arg3) ? ZCMD2.arg3 : NOWHERE;
          break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
        if (!g->mini_mud) {
          buf = str_format("Invalid vnum {:d}, cmd disabled",
                           a == NOWHERE ? olda : b == NOWHERE ? oldb
                                                              : oldc);
          log_zone_error(g, zone, cmd_no, buf.c_str());
        }
        ZCMD2.command = '*';
      }
    }
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/

int vnum_mobile(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (mob_vnum nr : g->mob_proto.vnums())
    if (isname(searchname, char_get_alias(mob_proto_of(g, nr))))
      send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                   ++found, mob_index_of(g, nr)->vnum, char_get_sdesc(mob_proto_of(g, nr)),
                   !char_get_proto_script(mob_proto_of(g, nr)).empty() ? "[TRIG]" : "");

  return (found);
}

int vnum_object(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (isname(searchname, obj_get_name(obj_proto_of(g, nr))))
      send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                   ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                   !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");

  return (found);
}

int vnum_material(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (isname(searchname, material_names[obj_get_value(obj_proto_of(g, nr), VAL_ALL_MATERIAL)])) {
      send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                   ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                   !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");
    }

  return (found);
}

int vnum_weapontype(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (obj_get_type_flag(obj_proto_of(g, nr)) == ITEM_WEAPON) {
      if (isname(searchname, weapon_type[obj_get_value(obj_proto_of(g, nr), VAL_WEAPON_SKILL)])) {
        send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                     ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                     !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");
      }
    }

  return (found);
}

int vnum_armortype(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (obj_vnum nr : g->obj_proto.vnums())
    if (obj_get_type_flag(obj_proto_of(g, nr)) == ITEM_ARMOR) {
      if (isname(searchname, armor_type[obj_get_value(obj_proto_of(g, nr), VAL_ARMOR_SKILL)])) {
        send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                     ++found, obj_index_of(g, nr)->vnum, obj_get_short_description(obj_proto_of(g, nr)),
                     !obj_get_proto_script(obj_proto_of(g, nr)).empty() ? "[TRIG]" : "");
      }
    }

  return (found);
}

/* create a character, and add it to the char list */

/* create a new mobile from a prototype */
struct char_data *read_mobile(GameWorld *g, mob_vnum nr)
{
  mob_vnum i = nr;
  struct char_data *mob;

  if (!mob_proto_of(g, i)) {
    mud_log("WARNING: Mobile vnum {:d} does not exist in database.", nr);
    return (nullptr);
  }

  mob = new char_data();
  clear_char(mob);
  *mob = (*mob_proto_of(g, i));

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

  mob_index_of(g, i)->number++;

  char_set_id(mob, dg_new_mob_uid(g));
  char_register(g, mob);

  copy_proto_script(mob_proto_of(g, i), mob);
  assign_triggers(g, mob);
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
  if (obj_get_unique_id(obj) < 0) { /* none yet: circle_random() is below 2^31, so a new one is never negative */
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

void obj_retire(GameWorld *g, struct obj_data *obj)
{
  remove_unique_id(g, obj);
  obj_unregister(g, obj);
}

static void log_dupe_objects(GameWorld *g, struct obj_data *obj1, struct obj_data *obj2)
{
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: Dupe object found: {} [{:d}] [{:d}:{:d}]",
         obj_get_short_description(obj1) ? obj_get_short_description(obj1) : "<No name>",
         obj_get_vnum(g, obj1), (long long)obj_get_generation(obj1), obj_get_unique_id(obj1));
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: First: In room: {:d} ({}), In object: {}, Carried by: {}, Worn by: {}",
         obj_get_in_room(obj1),
         obj_get_in_room(obj1) == NOWHERE ? "Nowhere" : room_get_name(g, obj_get_in_room(obj1)),
         obj_get_in_obj(obj1) ? obj_get_short_description(obj_get_in_obj(obj1)) : "None",
         obj_get_carried_by(obj1) ? char_get_name(obj_get_carried_by(obj1)) : "Nobody",
         obj_get_worn_by(obj1) ? char_get_name(obj_get_worn_by(obj1)) : "Nobody");
  mudlog(g, LOG_BRF, ADMLVL_GOD, true, "DUPE: Newer: In room: {:d} ({}), In object: {}, Carried by: {}, Worn by: {}",
         obj_get_in_room(obj2),
         obj_get_in_room(obj2) == NOWHERE ? "Nowhere" : room_get_name(g, obj_get_in_room(obj2)),
         obj_get_in_obj(obj2) ? obj_get_short_description(obj_get_in_obj(obj2)) : "None",
         obj_get_carried_by(obj2) ? char_get_name(obj_get_carried_by(obj2)) : "Nobody",
         obj_get_worn_by(obj2) ? char_get_name(obj_get_worn_by(obj2)) : "Nobody");
}

void check_unique_id(GameWorld *g, struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  if (!obj || obj_get_unique_id(obj) < 0) /* -1 for none; no real one is negative */
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

std::string sprintuniques(GameWorld *g, obj_vnum low, obj_vnum high)
{
  std::string str;
  bool any = false;

  for (int i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++)
    if (g->obj_unique_hash_pools[i])
      any = true;
  if (!any)
    return "No objects in unique hash.\r\n";

  str = str_format("Unique object hashes (vnums {:d} - {:d})\r\n", low, high);
  for (int i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    bool header = false;
    for (struct obj_unique_hash_elem *q = g->obj_unique_hash_pools[i]; q; q = q->next_e)
      if (obj_get_vnum(g, q->obj) >= low && obj_get_vnum(g, q->obj) <= high) {
        if (!header) {
          header = true;
          str += str_format("|-Hash {:d}\r\n", i);
        }
        str += str_format("| |- [@g{:>6d}@n] - [@y{:>10d}:{:<19d}@n] - {}\r\n",
                          obj_get_vnum(g, q->obj), (long long)q->generation, q->unique_id,
                          obj_get_short_description(q->obj) ? obj_get_short_description(q->obj) : "<Unknown>");
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

  obj_set_id(obj, dg_new_obj_uid(g));
  obj_register(g, obj);

  obj_set_generation(obj, game_now(g));
  obj_set_unique_id(obj, -1);

  assign_triggers(g, obj);

  return (obj);
}

/* create a new object from a prototype */
struct obj_data *read_object(GameWorld *g, obj_vnum nr)
{
  struct obj_data *obj;
  obj_vnum i = nr;

  if (!obj_proto_of(g, i)) {
    mud_log("Object (V) {:d} does not exist in database.", nr);
    return (nullptr);
  }

  obj = new obj_data();
  clear_object(obj);
  *obj = (*obj_proto_of(g, i));
  obj_set_room_loaded(obj, NOWHERE);

  obj_index_of(g, i)->number++;

  obj_set_id(obj, dg_new_obj_uid(g));
  obj_register(g, obj);

  obj_set_generation(obj, game_now(g));
  obj_set_unique_id(obj, -1);

  copy_proto_script(obj_proto_of(g, i), obj);
  assign_triggers(g, obj);
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
        g->reset_q.push_back(i->number);

        i->age = ZO_DEAD;
      }
    }
  } /* end - one minute has passed */

  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (auto it = g->reset_q.begin(); it != g->reset_q.end(); ++it) {
    struct zone_data *zone = find_zone(g, *it);
    if (!zone) { /* gone since it was queued */
      g->reset_q.erase(it);
      break;
    }
    if (zone->reset_mode == 2 || is_empty(g, zone)) {
      g->reset_q.erase(it);
      reset_zone(g, zone);
      mudlog(g, LOG_CMP, ADMLVL_GOD, false, "Auto zone reset: {} (Zone {:d})", zone->name, zone->number);
      break;
    }
  }
}

static void log_zone_error(GameWorld *g, struct zone_data *zone, int cmd_no, const char *message)
{
  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: zone file: {}", message);
  mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: ...offending cmd: '{:c}' cmd in zone #{:d}, line {:d}",
         as_char(ZCMD2.command), zone->number, ZCMD2.line);
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
  room_vnum rrnum;
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

          if ((mob_index_of(g, ZCMD2.arg1)->number < ZCMD2.arg2) &&
              (rand_number(g, 1, 100) >= ZCMD2.arg5)) {
            int room_max = 0;
            mob          = read_mobile(g, ZCMD2.arg1);

            /* First find out how many mobs of VNUM are in the mud with this rooms */
            /* VNUM as a load point for max from room checks. */
            /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

            if (ZCMD2.arg4 > 0) {
              for (char_data *i : each_char(g)) {
                if ((char_get_hometown(i) == ZCMD2.arg3) && (char_get_mob_vnum(g, i) == char_get_mob_vnum(g, mob))) {
                  room_max++;
                }
              }
            }
            char_to_room(g, mob, ZCMD2.arg3);

            /* Get rid of it if room_max has been met, ignore room_max if zero */

            if (room_max && (room_max >= ZCMD2.arg4)) {
              extract_char(g, mob);
              finish_extractions(g);
              break;
            }

            /*  Set the mobs loadroom for room_max checks. */
            char_set_hometown(mob, ZCMD2.arg3);

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
              obj          = read_object(g, ZCMD2.arg1);

              /* First find out how many obj of VNUM are in the mud with this rooms */
              /* VNUM as a load point for max from room checks. */
              /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

              if (ZCMD2.arg4 > 0) {
                for (obj_data *k : each_obj(g)) {
                  if (((obj_get_room_loaded(k) == ZCMD2.arg3) && (obj_get_vnum(g, k) == obj_get_vnum(g, obj))) || (obj_get_vnum(g, k) == obj_get_vnum(g, obj) && ZCMD2.arg3 == obj_get_in_room(k))) {
                    /*  For objects, lets not count them if they've been removed from the room */
                    /*  We'll let max_in_mud handle those. */
                    if (obj_get_in_room(k) == NOWHERE || obj_get_in_room(k) != ZCMD2.arg3) {
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

              obj_set_room_loaded(obj, ZCMD2.arg3);

              last_cmd = 1;
              tobj     = load_otrigger(g, obj) ? obj : nullptr;
              obj_load = true;
            } else {
              obj = read_object(g, ZCMD2.arg1);
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
            obj = read_object(g, ZCMD2.arg1);
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
            obj = read_object(g, ZCMD2.arg1);
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
              obj = read_object(g, ZCMD2.arg1);
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
          if ((obj = get_obj_in_list_num(g, ZCMD2.arg2, room_get_contents(g, ZCMD2.arg1))) != nullptr)
            extract_obj(g, obj);
          last_cmd = 1;
          tmob     = nullptr;
          tobj     = nullptr;
          break;

        case 'D': /* set state of door */
          if (ZCMD2.arg2 < 0 || ZCMD2.arg2 >= NUM_OF_DIRS ||
              (room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2) == nullptr)) {
            ZONE_ERROR(g, "door does not exist, command disabled");
            ZCMD2.command = '*';
          } else
            switch (ZCMD2.arg3) {
              case 0:
                REMOVE_BIT(room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                           EX_LOCKED);
                REMOVE_BIT(room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                           EX_CLOSED);
                break;
              case 1:
                SET_BIT(room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                        EX_CLOSED);
                REMOVE_BIT(room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                           EX_LOCKED);
                break;
              case 2:
                SET_BIT(room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
                        EX_LOCKED);
                SET_BIT(room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info,
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
            if (!room_of(g, ZCMD2.arg3)) {
              ZONE_ERROR(g, "Invalid room number in trigger assignment");
            }
            if (!room_get_script(g, ZCMD2.arg3))
              dg_ensure_script(room_script_slot(g, ZCMD2.arg3));
            add_trigger(g, room_get_script(g, ZCMD2.arg3), read_trigger(g, find_trigger(g, ZCMD2.arg2)), -1);
            last_cmd = 1;
          }

          break;

        case 'V':
          if (ZCMD2.arg1 == MOB_TRIGGER && tmob) {
            if (!char_get_script(tmob)) {
              ZONE_ERROR(g, "Attempt to give variable to scriptless mobile");
            } else
              add_var(char_get_script(tmob)->global_vars, ZCMD2.sarg1, ZCMD2.sarg2,
                      ZCMD2.arg3);
            last_cmd = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER && tobj) {
            if (!obj_get_script(tobj)) {
              ZONE_ERROR(g, "Attempt to give variable to scriptless object");
            } else
              add_var(obj_get_script(tobj)->global_vars, ZCMD2.sarg1, ZCMD2.sarg2,
                      ZCMD2.arg3);
            last_cmd = 1;
          } else if (ZCMD2.arg1 == WLD_TRIGGER) {
            if (!room_of(g, ZCMD2.arg3)) {
              ZONE_ERROR(g, "Invalid room number in variable assignment");
            } else {
              if (!(room_get_script(g, ZCMD2.arg3))) {
                ZONE_ERROR(g, "Attempt to give variable to scriptless object");
              } else
                add_var(room_get_script(g, ZCMD2.arg3)->global_vars,
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
      rrnum = rvnum;
      if (room_of(g, rrnum)) {
        reset_wtrigger(g, room_of(g, rrnum));
        if (room_flagged(g, rrnum, ROOM_AURA) && rand_number(g, 1, 5) >= 4) {
          send_to_room(g, rrnum, "The aura of regeneration covering the surrounding area disappears.\r\n");
          room_set_flag(g, rrnum, ROOM_AURA, false);
        }
        if (room_sect(g, rrnum) == SECT_LAVA) {
          room_set_geffect(g, rrnum, 5);
        }
        if (room_get_geffect(g, rrnum) < -1) {
          send_to_room(g, rrnum, "The area loses some of the water flooding it.\r\n");
          room_set_geffect(g, rrnum, room_get_geffect(g, rrnum) + (1));
        } else if (room_get_geffect(g, rrnum) == -1) {
          send_to_room(g, rrnum, "The area loses the last of the water flooding it in one large rush.\r\n");
          room_set_geffect(g, rrnum, 0);
        }
        if (room_get_dmg(g, rrnum) >= 100) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          room_set_dmg(g, rrnum, room_get_dmg(g, rrnum) - (rand_number(g, 5, 10)));
        } else if (room_get_dmg(g, rrnum) >= 50) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          room_set_dmg(g, rrnum, room_get_dmg(g, rrnum) - (rand_number(g, 1, 10)));
        } else if (room_get_dmg(g, rrnum) >= 10) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          room_set_dmg(g, rrnum, room_get_dmg(g, rrnum) - (rand_number(g, 1, 10)));
        } else if (room_get_dmg(g, rrnum) > 1) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          room_set_dmg(g, rrnum, room_get_dmg(g, rrnum) - (rand_number(g, 1, room_get_dmg(g, rrnum))));
        } else if (room_get_dmg(g, rrnum) > 0) {
          send_to_room(g, rrnum, "The area gets rebuilt a little.\r\n");
          room_set_dmg(g, rrnum, room_get_dmg(g, rrnum) - 1);
        }
        if (room_get_geffect(g, rrnum) >= 1 && rand_number(g, 1, 4) == 4 && !room_sunken(g, rrnum) && room_sect(g, rrnum) != SECT_LAVA) {
          send_to_room(g, rrnum, "The lava has cooled and become solid rock.\r\n");
          room_set_geffect(g, rrnum, 0);
        } else if (room_get_geffect(g, rrnum) >= 1 && rand_number(g, 1, 2) == 2 && room_sunken(g, rrnum) && room_sect(g, rrnum) != SECT_LAVA) {
          send_to_room(g, rrnum, "The water has cooled the lava and it has become solid rock.\r\n");
          room_set_geffect(g, rrnum, 0);
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
    if (room_get_zone(g, char_get_in_room(i->character)) != zone_nr)
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
/* Reads a string ending in '~' by io::add_string_line()'s rules; null for an empty one */
char *fread_string(FILE *fl, const char *error)
{
  std::string raw, text;

  do {
    if (!io::read_raw_line(fl, raw)) {
      mud_log("SYSERR: fread_string: format error at string (pos {:d}): {} at or near {}",
              ftell(fl), feof(fl) ? "EOF" : ferror(fl) ? "read error"
                                                       : "unknown error",
              error);
      exit(1);
    }
  } while (!io::add_string_line(text, raw));

  return text.empty() ? nullptr : strdup(text.c_str());
}

/* release memory allocated for a char struct */
void free_char(GameWorld *g, struct char_data *ch)
{
  /* first: removing an affect can change skills, which live in the player data freed below */
  while (!char_affects(ch).empty())
    affect_remove(g, ch, &char_affects(ch).front());

  /* Containers (affects aside, which change what they affect), strings, the trigger list and player_specials go
     with ch. */
  /* free any assigned scripts */
  if (char_get_script(ch))
    extract_script(g, ch);

  if (char_get_desc(ch))
    char_get_desc(ch)->character = nullptr;

  char_unregister(g, ch); /* if it is the one registered under its id */
  char_unlink_from_combat_list(g, ch);
  char_unlink_from_affect_list(g, ch);

  delete ch;
}

/* release memory allocated for an obj struct */
void free_obj(GameWorld *g, struct obj_data *obj)
{
  remove_unique_id(g, obj);
  /* its strings, extra descriptions and trigger list go with it */

  /* Let's make sure that we free up this memory */
  if (obj_get_auctname(obj)) {
    free(obj_get_auctname(obj));
  }

  /* free any assigned scripts */
  if (obj_get_script(obj))
    extract_script(g, obj);

  obj_unregister(g, obj);

  delete obj;
}

/* clear some of the the working variables of a char */
void reset_char(GameWorld *g, struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    char_set_equipment(ch, i, nullptr);

  char_followers_slot(ch)->clear();
  char_set_master(ch, nullptr);
  char_set_in_room(ch, NOWHERE);
  char_inventory_slot(ch)->clear();
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
  *ch = char_data{}; /* each field's initial value: it holds containers, so it must never be memset */

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
  *obj = obj_data{}; /* each field's initial value: it holds containers, so it must never be memset */

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
    char_make_player_specials(ch);
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
    mud_log("SYSERR: init_char: Character '{}' not found in player table.", char_get_name(ch));

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

/* The zone numbered vnum, or null. */
struct zone_data *find_zone(GameWorld *g, zone_vnum vnum)
{
  return g->zones.find(vnum);
}


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
  CONFIG_OK                     = OK;
  CONFIG_NOPERSON               = NOPERSON;
  CONFIG_NOEFFECT               = NOEFFECT;
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

  CONFIG_DFLT_IP  = DFLT_IP ? DFLT_IP : "";
  CONFIG_DFLT_DIR = DFLT_DIR;
  CONFIG_LOGNAME  = LOGNAME ? LOGNAME : "";

  CONFIG_MAX_PLAYING    = max_playing;
  CONFIG_MAX_FILESIZE   = max_filesize;
  CONFIG_MAX_BAD_PWS    = max_bad_pws;
  CONFIG_SITEOK_ALL     = siteok_everyone;
  CONFIG_NS_IS_SLOW     = nameserver_is_slow;
  CONFIG_NEW_SOCIALS    = use_new_socials;
  CONFIG_OLC_SAVE       = auto_save_olc;
  CONFIG_MENU           = MENU;
  CONFIG_WELC_MESSG     = WELC_MESSG;
  CONFIG_START_MESSG    = START_MESSG;
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

/* "tag = value": the tag is what is before the '=', its spaces taken out (quirk: "a b" is "ab"); the value is
   after it and the spaces and '='s that follow */
static std::pair<std::string, std::string_view> split_option(std::string_view line)
{
  size_t equals = std::min(line.find('='), line.size());
  std::string tag;
  for (char c : line.substr(0, equals))
    if (c != ' ')
      tag += c;
  std::string_view value = line.substr(equals);
  while (value.starts_with('=') || value.starts_with(' '))
    value.remove_prefix(1);
  return {std::move(tag), value};
}

void load_config(GameWorld *g)
{
  load_default_config(g);

  io::loaded<io::text_reader> file = io::text_reader::open(CONFIG_CONFFILE);
  if (!file)
    file = io::text_reader::open(str_format("{}/{}", DFLT_DIR, CONFIG_CONFFILE));
  if (!file) {
    mud_log("SYSERR: Game Config File: {}: can't be opened", CONFIG_CONFFILE);
    return;
  }

  while (std::optional<std::string_view> line = file->line()) {
    auto [tag, value] = split_option(*line);
    if (io::loaded<void> read = config_load_option(g, tag, value, *file); !read) {
      mud_log("SYSERR: {}", read.error().text());
      exit(1);
    }
  }
  if (CONFIG_DFLT_DIR.empty()) /* an empty DFLT_DIR line */
    CONFIG_DFLT_DIR = DFLT_DIR;
}
