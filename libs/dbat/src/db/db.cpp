/*
 * Booting the game: the order everything loads in (boot_db()), the world's part of it (boot_world()), the checks
 * that follow, and freeing it all at shutdown (destroy_db()). The files themselves are read in src/db/world,
 * src/db/text, src/db/etc and src/db/players.
 *
 * Based on CircleMUD's db.c: Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University. CircleMUD is
 * based on DikuMUD, Copyright (C) 1990, 1991. All rights reserved. See license.doc for complete information.
 */
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/world_files.hpp"
#include "dbat/text_files.hpp"
#include "dbat/affect.hpp"
#include "dbat/time.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/extract.hpp"
#include "dbat/players.hpp"
#include "dbat/spec_assign.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.other.hpp"
#include "dbat/act.social.hpp"
#include "dbat/random.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/reset.hpp"
#include "dbat/boards.hpp"
#include "dbat/objsave.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/context_help.hpp"

static void check_start_rooms(GameWorld *g);
static void renum_world(GameWorld *g);
static void renum_zone_table(GameWorld *g);
static void reset_time(GameWorld *g);

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
