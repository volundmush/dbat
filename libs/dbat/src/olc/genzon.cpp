/************************************************************************
 * Generic OLC Library - Zones / genzon.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/arrays.hpp"
#include "dbat/genzon.hpp"
#include "dbat/utils.hpp"
#include "dbat/fileop.hpp"
#include "dbat/genolc.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static void remove_cmd_from_list(std::vector<reset_com> *list, int pos);

/* The zone whose rooms include vznum, or null. */
struct zone_data *find_zone_by_thing(GameWorld *g, room_vnum vznum)
{
  for (struct zone_data *zone : g->zones.all())
    if (vznum >= genolc_zone_bottom(g, zone) && vznum <= zone->top)
      return zone;
  return nullptr;
}

struct zone_data *create_new_zone(GameWorld *g, zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error)
{
  struct zone_data *zone;

  if (vzone_num < 0) {
    *error = "You can't make negative zones.\r\n";
    return nullptr;
  }
  if (bottom > top) {
    *error = "Bottom room cannot be greater than top room.\r\n";
    return nullptr;
  }

  /*
   * New with bpl19, the OLC interface should decide whether
   * to allow overlap before calling this function. There
   * are more complicated rules for that but it's not covered
   * here.
   */
  if (vzone_num > 1000) {
    *error = "1000 is the highest zone allowed.\r\n";
    return nullptr;
  }

  /*
   * Make sure the zone does not exist.
   */
  room_vnum room = vzone_num * 100; /* Old CircleMUD 100-zones. */
  for (struct zone_data *i : g->zones.all())
    if (genolc_zone_bottom(g, i) <= room && i->top >= room) {
      *error = "A zone already covers that area.\r\n";
      return nullptr;
    }

  /*
   * The zone's world files, empty but for the zone and its first room.
   */
  struct new_file {
    std::string path, text;
    const char *log, *error;
  };
  const new_file files[] = {
    {str_format("{}{:d}.zon", ZON_PREFIX, vzone_num), str_format("#{:d}\nNew Zone~\n{:d} 30 2\nS\n$\n", vzone_num, (vzone_num * 100) + 99),
     "SYSERR: OLC: Can't write new zone file.", "Could not write zone file.\r\n"},
    {str_format("{}{:d}.wld", WLD_PREFIX, vzone_num), str_format("#{:d}\nThe Beginning~\nNot much here.\n~\n{:d} 0 0\nS\n$\n", bottom, vzone_num),
     "SYSERR: OLC: Can't write new world file.", "Could not write world file.\r\n"},
    {str_format("{}{:d}.mob", MOB_PREFIX, vzone_num), "$\n", "SYSERR: OLC: Can't write new mob file.", "Could not write mobile file.\r\n"},
    {str_format("{}{:d}.obj", OBJ_PREFIX, vzone_num), "$\n", "SYSERR: OLC: Can't write new obj file.", "Could not write object file.\r\n"},
    {str_format("{}{:d}.shp", SHP_PREFIX, vzone_num), "$~\n", "SYSERR: OLC: Can't write new shop file.", "Could not write shop file.\r\n"},
    {str_format("{}{:d}.trg", TRG_PREFIX, vzone_num), "$~\n", "SYSERR: OLC: Can't write new trigger file", "Could not write trigger file.\r\n"},
    /* quirk: a '/' more than the others */
    {str_format("{}/{:d}.gld", GLD_PREFIX, vzone_num), "$~\n", "SYSERR: OLC: Can't write new guild file", "Could not write guild file.\r\n"},
  };
  for (const new_file &file : files)
    if (!io::replace_file(file.path, file.text, false)) {
      mudlog(g, LOG_BRF, ADMLVL_IMPL, true, "{}", file.log);
      *error = file.error;
      return nullptr;
    }

  /*
   * Update index files.
   */
  create_world_index(g, vzone_num, "zon");
  create_world_index(g, vzone_num, "wld");
  create_world_index(g, vzone_num, "mob");
  create_world_index(g, vzone_num, "obj");
  create_world_index(g, vzone_num, "shp");
  create_world_index(g, vzone_num, "trg");
  create_world_index(g, vzone_num, "gld");

  zone = g->zones.emplace(vzone_num);

  zone->name          = "New Zone";
  zone->number        = vzone_num;
  zone->builders      = "None";
  zone->bot           = bottom;
  zone->top           = top;
  zone->lifespan      = 30;
  zone->age           = 0;
  zone->reset_mode    = 2;
  zone->zone_flags[0] = 0;
  zone->zone_flags[1] = 0;
  zone->zone_flags[2] = 0;
  zone->zone_flags[3] = 0;
  zone->min_level     = 0;
  zone->max_level     = ADMLVL_IMPL;
  /*
   * No zone commands, just terminate it with an 'S'
   */
  zone->cmd = empty_cmd_list();

  for (room_vnum v = bottom; v <= top; v++) /* rooms already in its range now belong to it */
    if (room_vnum r = v; room_of(g, r))
      room_set_zone(g, r, zone);

  add_to_save_list(g, zone->number, SL_ZON);
  return zone;
}

/*-------------------------------------------------------------------*/

void create_world_index(GameWorld *g, zone_vnum znum, const char *type)
{
  std::string old_name;
  const char *prefix;
  zone_vnum num;
  int found = false;
  char buf1[MAX_STRING_LENGTH];

  switch (*type) {
    case 'z':
      prefix = ZON_PREFIX;
      break;
    case 'w':
      prefix = WLD_PREFIX;
      break;
    case 'o':
      prefix = OBJ_PREFIX;
      break;
    case 'm':
      prefix = MOB_PREFIX;
      break;
    case 's':
      prefix = SHP_PREFIX;
      break;
    case 't':
      prefix = TRG_PREFIX;
      break;
    case 'g':
      prefix = GLD_PREFIX;
      break;
    default:
      /*
     * Caller messed up  
     */
      return;
  }

  old_name = str_format("{}/index", prefix);

  io::loaded<io::text_reader> oldfile = io::text_reader::open(old_name);
  if (!oldfile) {
    mudlog(g, LOG_BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Failed to open {}.", old_name);
    return;
  }
  io::text_writer newfile(old_name);

  /*
   * Index contents must be in order: search through the old file for the
   * right place, insert the new file, then copy the rest over. 
   */
  snprintf(buf1, sizeof(buf1), "%d.%s", znum, type);
  while (std::optional<std::string_view> line = oldfile->line()) {
    std::string buf(*line);
    if (buf[0] == '$') {
      /*
       * The following used to add a blank line, thanks to Brian Taylor for the fix... (Mythran)
       */
      newfile.print("{}", (!found ? strncat(buf1, "\n$\n", sizeof(buf1) - 1) : "$\n"));
      break;
    }
    if (!found) {
      sscanf(buf.c_str(), "%d", &num);
      if (num == znum) {
        found = true;
      } else if (num > znum) {
        found = true;
        newfile.print("{}\n", buf1);
      }
    }
    newfile.print("{}\n", buf);
  }

  newfile.commit(); /* out with the old, in with the new */
}

/*-------------------------------------------------------------------*/

void remove_room_zone_commands(GameWorld *g, struct zone_data *zone, room_vnum room_num)
{
  int subcmd         = 0;
  room_vnum cmd_room = -2;

  /*
   * Delete all entries in zone_table that relate to this room so we
   * can add all the ones we have in their place.
   */
  while (zone->cmd[subcmd].command != 'S') {
    switch (zone->cmd[subcmd].command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = zone->cmd[subcmd].arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = zone->cmd[subcmd].arg1;
        break;
      default:
        break;
    }
    if (cmd_room == room_num)
      remove_cmd_from_list(&zone->cmd, subcmd);
    else
      subcmd++;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Save all the zone_table for this zone to disk.  This function now
 * writes simple comments in the form of (<name>) to each record.  A
 * header for each field is also there.
 */
/*-------------------------------------------------------------------*/

/*
 * Some common code to count the number of comands in the list.
 */
int count_commands(const std::vector<reset_com> &list)
{
  int count = 0;

  while (list[count].command != 'S')
    count++;

  return count;
}

std::vector<reset_com> empty_cmd_list()
{
  std::vector<reset_com> list(1);
  list[0].command = 'S';
  return list;
}

/*-------------------------------------------------------------------*/

void add_cmd_to_list(std::vector<reset_com> *list, const struct reset_com *newcmd, int pos)
{
  struct reset_com copy = *newcmd; /* newcmd may be in list */

  if (list->empty())
    *list = empty_cmd_list();
  list->insert(list->begin() + pos, copy);
}

/*-------------------------------------------------------------------*/

static void remove_cmd_from_list(std::vector<reset_com> *list, int pos)
{
  list->erase(list->begin() + pos);
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then add new (blank) command  
 */
bool new_command(struct zone_data *zone, int pos)
{
  int subcmd               = 0;
  struct reset_com new_com = {.command = 'N'}; /* everything else blank: the strings are freed later */

  /* * Error check to ensure users hasn't given too large an index  */
  while (zone->cmd[subcmd].command != 'S')
    subcmd++;

  if (pos < 0 || pos > subcmd)
    return false;

  /* * Ok, let's add a new (blank) command */
  add_cmd_to_list(&zone->cmd, &new_com, pos);
  return true;
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then remove command  
 */
void delete_zone_command(struct zone_data *zone, int pos)
{
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index  
   */
  while (zone->cmd[subcmd].command != 'S')
    subcmd++;

  if (pos < 0 || pos >= subcmd)
    return;

  /*
   * Ok, let's zap it  
   */
  remove_cmd_from_list(&zone->cmd, pos);
}

/*-------------------------------------------------------------------*/
