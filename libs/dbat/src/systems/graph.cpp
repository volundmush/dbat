/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/config.hpp"
#include "dbat/graph.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/spells.hpp"
#include "dbat/maputils.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static bool VALID_EDGE(GameWorld *g, room_vnum x, int y);
static void bfs_enqueue(GameWorld *g, room_vnum room, int dir);
static void bfs_dequeue(GameWorld *g);
static void bfs_clear_queue(GameWorld *g);

struct bfs_queue_struct {
  room_vnum room{};
  char dir{};
  struct bfs_queue_struct *next{};
};


/* Utility macros */
#define MARK(g, room)      (room_set_flag(g, room, ROOM_BFS_MARK, true))
#define UNMARK(g, room)    (room_set_flag(g, room, ROOM_BFS_MARK, false))
#define IS_MARKED(g, room) (room_flagged(g, room, ROOM_BFS_MARK))
#define IS_CLOSED(g, x, y) (room_exit_flagged(room_get_dir_option(g, (x), (y)), EX_CLOSED))

static bool VALID_EDGE(GameWorld *g, room_vnum x, int y)
{
  if (room_get_dir_option(g, x, y) == nullptr || room_get_exit_to_room(g, x, y) == NOWHERE)
    return false;
  if (CONFIG_TRACK_T_DOORS == false && IS_CLOSED(g, x, y))
    return false;
  if (room_flagged(g, room_get_exit_to_room(g, x, y), ROOM_NOTRACK) || IS_MARKED(g, room_get_exit_to_room(g, x, y)))
    return false;

  return true;
}

static void bfs_enqueue(GameWorld *g, room_vnum room, int dir)
{
  struct bfs_queue_struct *curr;

  curr       = new bfs_queue_struct();
  curr->room = room;
  curr->dir  = dir;

  if (g->bfs_queue_tail) {
    g->bfs_queue_tail->next = curr;
    g->bfs_queue_tail       = curr;
  } else
    g->bfs_queue_head = g->bfs_queue_tail = curr;
}

static void bfs_dequeue(GameWorld *g)
{
  struct bfs_queue_struct *curr;

  curr = g->bfs_queue_head;

  if (!(g->bfs_queue_head = g->bfs_queue_head->next))
    g->bfs_queue_tail = nullptr;
  delete curr;
}

static void bfs_clear_queue(GameWorld *g)
{
  while (g->bfs_queue_head)
    bfs_dequeue(g);
}

/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(GameWorld *g, room_vnum src, room_vnum target)
{
  int curr_dir;

  if (!room_of(g, src) || !room_of(g, target)) {
    mud_log("SYSERR: Illegal value {:d} or {:d} passed to find_first_step. ({})", src, target, __FILE__);
    return (BFS_ERROR);
  }
  if (src == target)
    return (BFS_ALREADY_THERE);

  /* clear marks first, some OLC systems will save the mark. */
  g->rooms.visit([](room_vnum, struct room_data *room) { room_set_flag(room, ROOM_BFS_MARK, false); });
  MARK(g, src);
  /* first, enqueue the first steps, saving which direction we're going. */
  for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) {
    if (VALID_EDGE(g, src, curr_dir)) {
      MARK(g, room_get_exit_to_room(g, src, curr_dir));
      bfs_enqueue(g, room_get_exit_to_room(g, src, curr_dir), curr_dir);
    }
  }
  /* now, do the classic BFS. */
  while (g->bfs_queue_head) {
    if (g->bfs_queue_head->room == target) {
      curr_dir = g->bfs_queue_head->dir;
      bfs_clear_queue(g);
      return (curr_dir);
    }
    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
      if (VALID_EDGE(g, g->bfs_queue_head->room, curr_dir)) {
        MARK(g, room_get_exit_to_room(g, g->bfs_queue_head->room, curr_dir));
        bfs_enqueue(g, room_get_exit_to_room(g, g->bfs_queue_head->room, curr_dir), g->bfs_queue_head->dir);
      }
    bfs_dequeue(g);
  }

  return (BFS_NO_PATH);
}

/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/

ACMD(do_sradar)
{
  struct obj_data *vehicle = nullptr, *controls = nullptr;
  int dir = 0, noship = false;
  std::string arg;
  std::string planet;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!char_plr_flagged(ch, PLR_PILOTING) && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "You are not flying a ship, maybe you want detect?\r\n");
    return;
  }

  if (!(controls = find_control(g, ch)) && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "@wYou have nothing to control here!\r\n");
    return;
  }

  if (!char_plr_flagged(ch, PLR_PILOTING) && char_get_admlevel(ch) >= 1) {
    noship = true;
  } else if (!(vehicle = find_vehicle_by_vnum(g, obj_get_value(controls, 0)))) {
    send_to_char(g, ch, "@wYou can't find anything to pilot.\r\n");
    return;
  }

  if (noship == false && room_sect(g, obj_get_in_room(vehicle)) != SECT_SPACE) {
    send_to_char(g, ch, "@wYour ship is not in space!\r\n");
    return;
  }
  if (noship == true && room_sect(g, char_get_in_room(ch)) != SECT_SPACE) {
    send_to_char(g, ch, "@wYou are not even in space!\r\n");
    return;
  }

  if (arg.empty()) {
    if (char_get_admlevel(ch) >= 1 && noship == true) {
      printmap(g, char_get_in_room(ch), ch, 0, -1);
    } else {
      printmap(g, obj_get_in_room(vehicle), ch, 0, obj_get_vnum(g, vehicle));
    }
    /*
   send_to_char(ch, "     @D----------------[@C Radar @D]-----------------@n\r\n");
   send_to_char(ch, "    @D|@GEE@D:@w Earth    @gNN@D:@w Namek   @YVV@D:@w Vegeta       @D|@n\r\n");
   send_to_char(ch, "    @D|@CFF@D:@w Frigid   @mKK@D:@w Konack  @BAA@D:@w Aether       @D|@n\r\n");
   send_to_char(ch, "    @D|@MYY@D:@w Yardrat  @CKK@D:@w Kanassa @mAA@D:@w Arlia        @D|@n\r\n");
   send_to_char(ch, "    @D|@WBB@D:@w Buoy     @m&&@D:@w Nebula  @yQQ@D:@w Asteroid     @D|@n\r\n");
   send_to_char(ch, "    @D|@b@1**@n@D:@w Wormhole @DSS@D:@w Station  @r#@D: @wUnknown Ship @D|@n\n");
   send_to_char(ch, "    @D|@6  @n@D:@w Star                                  @D|@n\n"); */
    return;
  }

  if (char_get_ping(ch) > 0) {
    send_to_char(g, ch, "@wYou need to wait a few more seconds before pinging a destination again.\r\n");
    return;
  }

  if (noship == false) {
    if (str_eq_nocase(arg, "earth") || str_eq_nocase(arg, "Earth")) {
      dir = find_first_step(g, obj_get_in_room(vehicle), 40979);
      planet = "Earth";
    } else if (str_eq_nocase(arg, "frigid") || str_eq_nocase(arg, "Frigid")) {
      dir = find_first_step(g, obj_get_in_room(vehicle), 30889);
      planet = "Frigid";
    } else if (str_eq_nocase(arg, "konack") || str_eq_nocase(arg, "Konack")) {
      dir = find_first_step(g, obj_get_in_room(vehicle), 27065);
      planet = "Konack";
    } else if (str_eq_nocase(arg, "vegeta") || str_eq_nocase(arg, "Vegeta")) {
      dir = find_first_step(g, obj_get_in_room(vehicle), 32365);
      planet = "Vegeta";
    } else if (str_eq_nocase(arg, "aether") || str_eq_nocase(arg, "Aether")) {
      dir = find_first_step(g, obj_get_in_room(vehicle), 41959);
      planet = "Aether";
    } else if (str_eq_nocase(arg, "namek") || str_eq_nocase(arg, "Namek")) {
      dir = find_first_step(g, obj_get_in_room(vehicle), 42880);
      planet = "Namek";
    } else if (str_eq_nocase(arg, "buoy1") && char_get_radar1(ch) <= 0) {
      send_to_char(g, ch, "@wYou haven't launched that buoy.\r\n");
      return;
    } else if (str_eq_nocase(arg, "buoy2") && char_get_radar2(ch) <= 0) {
      send_to_char(g, ch, "@wYou haven't launched that buoy.\r\n");
      return;
    } else if (str_eq_nocase(arg, "buoy3") && char_get_radar3(ch) <= 0) {
      send_to_char(g, ch, "@wYou haven't launched that buoy.\r\n");
      return;
    } else if (str_eq_nocase(arg, "buoy1") && char_get_radar1(ch) > 0) {
      room_vnum rad = char_get_radar1(ch);
      dir           = find_first_step(g, obj_get_in_room(vehicle), rad);
      planet        = "Buoy One";
    } else if (str_eq_nocase(arg, "buoy2") && char_get_radar2(ch) > 0) {
      room_vnum rad = char_get_radar2(ch);
      dir           = find_first_step(g, obj_get_in_room(vehicle), rad);
      planet        = "Buoy Two";
    } else if (str_eq_nocase(arg, "buoy3") && char_get_radar3(ch) > 0) {
      room_vnum rad = char_get_radar3(ch);
      dir           = find_first_step(g, obj_get_in_room(vehicle), rad);
      planet        = "Buoy Three";
    } else {
      send_to_char(g, ch, "@wThat is not an existing planet.@n\r\n");
      return;
    }
  }

  if (noship == true) {
    if (str_eq_nocase(arg, "earth") || str_eq_nocase(arg, "Earth")) {
      dir = find_first_step(g, char_get_in_room(ch), 40979);
      planet = "Earth";
    } else if (str_eq_nocase(arg, "frigid") || str_eq_nocase(arg, "Frigid")) {
      dir = find_first_step(g, char_get_in_room(ch), 30889);
      planet = "Frigid";
    } else if (str_eq_nocase(arg, "konack") || str_eq_nocase(arg, "Konack")) {
      dir = find_first_step(g, char_get_in_room(ch), 27065);
      planet = "Konack";
    } else if (str_eq_nocase(arg, "vegeta") || str_eq_nocase(arg, "Vegeta")) {
      dir = find_first_step(g, char_get_in_room(ch), 32365);
      planet = "Vegeta";
    } else if (str_eq_nocase(arg, "aether") || str_eq_nocase(arg, "Aether")) {
      dir = find_first_step(g, char_get_in_room(ch), 41959);
      planet = "Aether";
    } else if (str_eq_nocase(arg, "namek") || str_eq_nocase(arg, "Namek")) {
      dir = find_first_step(g, char_get_in_room(ch), 42880);
      planet = "Namek";
    } else if (str_eq_nocase(arg, "buoy1") && char_get_radar1(ch) <= 0) {
      send_to_char(g, ch, "@wYou haven't launched that buoy.\r\n");
      return;
    } else if (str_eq_nocase(arg, "buoy2") && char_get_radar2(ch) <= 0) {
      send_to_char(g, ch, "@wYou haven't launched that buoy.\r\n");
      return;
    } else if (str_eq_nocase(arg, "buoy3") && char_get_radar3(ch) <= 0) {
      send_to_char(g, ch, "@wYou haven't launched that buoy.\r\n");
      return;
    } else if (str_eq_nocase(arg, "buoy1") && char_get_radar1(ch) > 0) {
      room_vnum rad = char_get_radar1(ch);
      dir           = find_first_step(g, char_get_in_room(ch), rad);
      planet        = "Buoy One";
    } else if (str_eq_nocase(arg, "buoy2") && char_get_radar2(ch) > 0) {
      room_vnum rad = char_get_radar2(ch);
      dir           = find_first_step(g, char_get_in_room(ch), rad);
      planet        = "Buoy Two";
    } else if (str_eq_nocase(arg, "buoy3") && char_get_radar3(ch) > 0) {
      room_vnum rad = char_get_radar3(ch);
      dir           = find_first_step(g, char_get_in_room(ch), rad);
      planet        = "Buoy Three";
    } else {
      send_to_char(g, ch, "@wThat is not an existing planet.@n\r\n");
      return;
    }
  }

  switch (dir) {
    case BFS_ERROR:
      send_to_char(g, ch, "Hmm.. something seems to be wrong.\r\n");
      break;
    case BFS_ALREADY_THERE:
      send_to_char(g, ch, "@wThe radar shows that your are already there.@n\r\n");
      break;
    case BFS_NO_PATH:
      send_to_char(g, ch, "@wYou should be in space to use the radar.@n\r\n");
      break;
    default:
      send_to_char(g, ch, "@wYour radar detects @C{}@w to the @G{}@n\r\n", planet, dirs[dir]);
      break;
  }
  char_set_ping(ch, 5);
}

ACMD(do_radar)
{
  room_vnum room = 0;
  int dir, num = 0, found = false, found2 = false, fcount = 0;

  for (obj_data *obj2 : char_inventory(g, ch)) {
    if (obj_get_vnum(g, obj2) == 12 && (!obj_extra_flagged(obj2, ITEM_BROKEN)) && (!obj_extra_flagged(obj2, ITEM_FORGED))) {
      found2 = true;
    }
  }
  if (found2 == false) {
    send_to_char(g, ch, "You do not even have a dragon radar!\r\n");
    return;
  }

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You are a freaking mob!\r\n");
    return;
  }

  else {
    char_set_wait(ch, PULSE_2SEC);
    act(g, "$n holds up a dragon radar and pushes its button.", false, ch, nullptr, nullptr, TO_ROOM);
    while (num < 20000) {
      if (room_of(g, room)) {
        for (obj_data *obj : room_contents(g, room)) {
          if (obj_extra_flagged(obj, ITEM_FORGED)) {
            continue;
          }
          if (obj_get_vnum(g, obj) == 20 || obj_get_vnum(g, obj) == 21 || obj_get_vnum(g, obj) == 22 || obj_get_vnum(g, obj) == 23 || obj_get_vnum(g, obj) == 24 || obj_get_vnum(g, obj) == 25 || obj_get_vnum(g, obj) == 26) {
            dir = find_first_step(g, char_get_in_room(ch), obj_get_in_room(obj));
            fcount += 1;
            switch (dir) {
              case BFS_ERROR:
                send_to_char(g, ch, "Hmm.. something seems to be wrong.\r\n");
                break;
              case BFS_ALREADY_THERE:
                send_to_char(g, ch, "@D<@G{:d}@D>@w The radar detects a dragonball right here!\r\n", fcount);
                break;
              case BFS_NO_PATH:
                send_to_char(g, ch, "@D<@G{:d}@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n", fcount);
                break;
              default:
                send_to_char(g, ch, "@D<@G{:d}@D>@w The radar detects a dragonball {} of here.\r\n", fcount, dirs[dir]);
                break;
            }
            found = true;
          }
        }
        for (char_data *tch : room_people(g, room)) {
          if (tch == ch) {
            continue;
          }
          for (obj_data *obj : char_inventory(g, tch)) {
            if (obj_extra_flagged(obj, ITEM_FORGED)) {
              continue;
            }
            if (obj_get_vnum(g, obj) == 20 || obj_get_vnum(g, obj) == 21 || obj_get_vnum(g, obj) == 22 || obj_get_vnum(g, obj) == 23 || obj_get_vnum(g, obj) == 24 || obj_get_vnum(g, obj) == 25 || obj_get_vnum(g, obj) == 26) {
              dir = find_first_step(g, char_get_in_room(ch), char_get_in_room(tch));
              fcount += 1;
              switch (dir) {
                case BFS_ERROR:
                  send_to_char(g, ch, "Hmm.. something seems to be wrong.\r\n");
                  break;
                case BFS_ALREADY_THERE:
                  send_to_char(g, ch, "@D<@G{:d}@D>@w The radar detects a dragonball right here!\r\n", fcount);
                  break;
                case BFS_NO_PATH:
                  send_to_char(g, ch, "@D<@G{:d}@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n", fcount);
                  break;
                default:
                  send_to_char(g, ch, "@D<@G{:d}@D>@w The radar detects a dragonball {} of here.\r\n", fcount, dirs[dir]);
                  break;
              }
              found = true;
            }
          }
        }
      }
      num += 1;
      room += 1;
    }
    if (found == false) {
      send_to_char(g, ch, "The radar didn't detect any dragonballs on the planet.\r\n");
      return;
    }
  }
}

ACMD(do_track)
{
  std::string arg;
  struct char_data *vict;
  int count = 0, dir;

  /* The character must have the track skill. */
  if (char_is_npc(ch) || !char_get_skill(ch, SKILL_SENSE)) {
    send_to_char(g, ch, "You have no idea how.\r\n");
    return;
  }
  if (char_get_suppression(ch) <= 20 && char_get_suppression(ch) > 0) {
    send_to_char(g, ch, "You are concentrating too hard on suppressing your powerlevel at this level of suppression.\r\n");
    return;
  }

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Whom are you trying to sense?\r\n");
    return;
  }
  if (arg.empty() && char_get_fighting(g, ch)) {
    vict = char_get_fighting(g, ch);
    send_to_char(g, ch, "You focus on the one your are fighting.\r\n");
    if (char_aff_flagged(vict, AFF_NOTRACK) || char_is_android(vict)) {
      send_to_char(g, ch, "You can't sense them.\r\n");
      return;
    }
    if (!read_sense_memory(g, ch, vict)) {
      send_to_char(g, ch, "You will remember their ki signal from now on.\r\n");
      sense_memory_write(g, ch, vict);
    }
    act(g, "You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
    act(g, "$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
    act(g, "$n looks at $N@n intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
    if (!char_is_android(vict)) {
      if (char_get_alignment(vict) > 50 && char_get_alignment(vict) < 200) {
        send_to_char(g, ch, "You sense slightly pure and good ki from them.\r\n");
      } else if (char_get_alignment(vict) > 200 && char_get_alignment(vict) < 500) {
        send_to_char(g, ch, "You sense a pure and good ki from them.\r\n");
      } else if (char_get_alignment(vict) >= 500) {
        send_to_char(g, ch, "You sense an extremely pure and good ki from them.\r\n");
      } else if (char_get_alignment(vict) < -50 && char_get_alignment(vict) > -200) {
        send_to_char(g, ch, "You sense slightly sour and evil ki from them.\r\n");
      } else if (char_get_alignment(vict) < -200 && char_get_alignment(vict) > -500) {
        send_to_char(g, ch, "You sense a sour and evil ki from them.\r\n");
      } else if (char_get_alignment(vict) <= -500) {
        send_to_char(g, ch, "You sense an extremely evil ki from them.\r\n");
      } else if (char_get_alignment(vict) > -50 && char_get_alignment(vict) < 50) {
        send_to_char(g, ch, "You sense slightly mild indefinable ki from them.\r\n");
      }
      if (getCurPL(g, vict) > getCurPL(g, ch) * 50) {
        send_to_char(g, ch, "Their power is so huge it boggles your mind and crushes your spirit to fight!\n");
      } else if (getCurPL(g, vict) > getCurPL(g, ch) * 25) {
        send_to_char(g, ch, "Their power is so much larger than you that you would die like an insect.\n");
      } else if (getCurPL(g, vict) > getCurPL(g, ch) * 10) {
        send_to_char(g, ch, "Their power is many times larger than your own.\n");
      } else if (getCurPL(g, vict) > getCurPL(g, ch) * 5) {
        send_to_char(g, ch, "Their power is a great deal larger than your own.\n");
      } else if (getCurPL(g, vict) > getCurPL(g, ch) * 2) {
        send_to_char(g, ch, "Their power is more than twice as large as your own.\n");
      } else if (getCurPL(g, vict) > getCurPL(g, ch)) {
        send_to_char(g, ch, "Their power is about twice as large as your own.\n");
      } else if (getCurPL(g, vict) == getCurPL(g, ch)) {
        send_to_char(g, ch, "Their power is exactly as strong as you.\n");
      } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.75) {
        send_to_char(g, ch, "Their power is about a quarter of your own or larger.\n");
      } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.5) {
        send_to_char(g, ch, "Their power is about half of your own or larger.\n");
      } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.25) {
        send_to_char(g, ch, "Their power is about a quarter of your own or larger.\n");
      } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.1) {
        send_to_char(g, ch, "Their power is about a tenth of your own or larger.\n");
      } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.01) {
        send_to_char(g, ch, "Their power is less than a tenth of your own.\n");
      } else if (getCurPL(g, vict) < getCurPL(g, ch) * 0.01) {
        send_to_char(g, ch, "Their power is less than 1 percent of your own. What a weakling...\n");
      }
    } else {
      send_to_char(g, ch, "You can't sense their powerlevel as they are a machine.\r\n");
    }
    return;
  }

  /* Scanning the entire planet. */
  if (str_eq_nocase(arg, "scan")) {
    for (descriptor_data *i : each_desc(g)) {
      if (desc_get_connected(i) != CON_PLAYING) {
        continue;
      }
      if (char_is_android(i->character)) {
        continue;
      }
      if (i->character == ch) {
        continue;
      }
      if (getCurPL(g, i->character) < (getCurPL(g, ch) * 0.001) + 1) {
        continue;
      }
      if (planet_check(g, ch, i->character)) {
        if (readIntro(ch, i->character) == 1) {
          send_to_char(g, ch, "@D[@Y{:d}@D] @CYou sense @c{}@C with ", (count + 1), get_i_name(ch, i->character));
        } else {
          send_to_char(g, ch, "@D[@Y{:d}@D] @CYou sense ", (count + 1));
        }
        /* How strong is the one we sense? */
        if (getCurPL(g, i->character) > getCurPL(g, ch) * 50) {
          send_to_char(g, ch, "a power so huge it boggles your mind and crushes your spirit to fight!\n");
        } else if (getCurPL(g, i->character) > getCurPL(g, ch) * 25) {
          send_to_char(g, ch, "a power so much larger than you that you would die like an insect.\n");
        } else if (getCurPL(g, i->character) > getCurPL(g, ch) * 10) {
          send_to_char(g, ch, "a power that is many times larger than your own.\n");
        } else if (getCurPL(g, i->character) > getCurPL(g, ch) * 5) {
          send_to_char(g, ch, "a power that is a great deal larger than your own.\n");
        } else if (getCurPL(g, i->character) > getCurPL(g, ch) * 2) {
          send_to_char(g, ch, "a power that is more than twice as large as your own.\n");
        } else if (getCurPL(g, i->character) > getCurPL(g, ch)) {
          send_to_char(g, ch, "a power that is about twice as large as your own.\n");
        } else if (getCurPL(g, i->character) == getCurPL(g, ch)) {
          send_to_char(g, ch, "a power that is exactly as strong as you.\n");
        } else if (getCurPL(g, i->character) >= getCurPL(g, ch) * 0.75) {
          send_to_char(g, ch, "a power that is about a quarter of your own or larger.\n");
        } else if (getCurPL(g, i->character) >= getCurPL(g, ch) * 0.5) {
          send_to_char(g, ch, "a power that is about half of your own or larger.\n");
        } else if (getCurPL(g, i->character) >= getCurPL(g, ch) * 0.25) {
          send_to_char(g, ch, "a power that is about a quarter of your own or larger.\n");
        } else if (getCurPL(g, i->character) >= getCurPL(g, ch) * 0.1) {
          send_to_char(g, ch, "a power that is about a tenth of your own or larger.\n");
        } else if (getCurPL(g, i->character) >= getCurPL(g, ch) * 0.01) {
          send_to_char(g, ch, "a power that is less than a tenth of your own.\n");
        } else if (getCurPL(g, i->character) < getCurPL(g, ch) * 0.01) {
          send_to_char(g, ch, "a power that is less than 1 percent of your own. What a weakling...\n");
        }

        /* What's their alignment? */

        if (char_get_alignment(i->character) >= 500) {
          send_to_char(g, ch, "@wYou sense an extremely pure and good ki from them.@n\n");
        } else if (char_get_alignment(i->character) > 200) {
          send_to_char(g, ch, "@wYou sense a pure and good ki from them.@n\n");
        } else if (char_get_alignment(i->character) > 50) {
          send_to_char(g, ch, "@wYou sense slightly pure and good ki from them.@n\n");
        } else if (char_get_alignment(i->character) > -50) {
          send_to_char(g, ch, "@wYou sense a slightly mild indefinable ki from them.@n\n");
        } else if (char_get_alignment(i->character) > -200) {
          send_to_char(g, ch, "@wYou sense a slightly sour and evil ki from them.@n\n");
        } else if (char_get_alignment(i->character) > -500) {
          send_to_char(g, ch, "@wYou sense a sour and evil ki from them.@n\n");
        } else if (char_get_alignment(i->character) <= -500) {
          send_to_char(g, ch, "@wYou sense an extremely evil ki from them.@n\n");
        }

        const char *blah = sense_location(g, i->character);
        send_to_char(g, ch, "@wLastly you sense that they are at... @C{}@n\n", blah);
        ++count;
      }
    }
    if (count == 0) {
      send_to_char(g, ch, "You sense that there is no one important around.@n\n");
    }
    return;
  }

  /* The person can't see the victim. */

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "No one is around by that name.\r\n");
    return;
  }

  /* We can't track the victim. */
  if (char_aff_flagged(vict, AFF_NOTRACK) || char_is_android(vict)) {
    send_to_char(g, ch, "You can't sense them.\r\n");
    return;
  }

  if (getCurPL(g, vict) < (getCurPL(g, ch) * 0.001) + 1) {
    if (char_get_in_room(ch) == char_get_in_room(vict)) {
      if (!read_sense_memory(g, ch, vict)) {
        send_to_char(g, ch, "Their powerlevel is too weak for you to sense properly, but you will recognise their ki signal from now on.\r\n");
        sense_memory_write(g, ch, vict);
      } else {
        send_to_char(g, ch, "Their powerlevel is too weak for you to sense properly.\r\n");
      }
    } else {
      send_to_char(g, ch, "Their powerlevel is too weak for you to sense properly.\r\n");
    }
    return;
  }

  if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_YARDRAT)) && (room_flagged(g, char_get_in_room(vict), ROOM_YARDRAT)))) {
    send_to_char(g, ch, "@WSense@D: @YYardrat@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_EARTH)) && (room_flagged(g, char_get_in_room(vict), ROOM_EARTH)))) {
    send_to_char(g, ch, "@WSense@D: @GEarth@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_VEGETA)) && (room_flagged(g, char_get_in_room(vict), ROOM_VEGETA)))) {
    send_to_char(g, ch, "@WSense@D: @YVegeta@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_NAMEK)) && (room_flagged(g, char_get_in_room(vict), ROOM_NAMEK)))) {
    send_to_char(g, ch, "@WSense@D: @gNamek@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_FRIGID)) && (room_flagged(g, char_get_in_room(vict), ROOM_FRIGID)))) {
    send_to_char(g, ch, "@WSense@D: @CFrigid@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_AETHER)) && (room_flagged(g, char_get_in_room(vict), ROOM_AETHER)))) {
    send_to_char(g, ch, "@WSense@D: @mAetherh@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_KONACK)) && (room_flagged(g, char_get_in_room(vict), ROOM_KONACK)))) {
    send_to_char(g, ch, "@WSense@D: @MKonack@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_KANASSA)) && (room_flagged(g, char_get_in_room(vict), ROOM_KANASSA)))) {
    send_to_char(g, ch, "@WSense@D: @cKanassa@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_flagged(g, char_get_in_room(ch), ROOM_ARLIA)) && (room_flagged(g, char_get_in_room(vict), ROOM_ARLIA)))) {
    send_to_char(g, ch, "@WSense@D: @yArlia@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else if ((char_get_skills_base(ch, SKILL_SENSE) == 100) && (!(room_planet_zenith(g, char_get_in_room(ch))) && (room_planet_zenith(g, char_get_in_room(vict))))) {
    send_to_char(g, ch, "@WSense@D: @CZenith@n\r\n");
    if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
      const char *blah = sense_location(g, vict);
      send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
    }
  } else {

    if (char_get_skill(ch, SKILL_SENSE) < rand_number(g, 1, 101)) {
      int tries = 10;
      /* Find a random direction. :) */
      do {
        dir = rand_number(g, 0, NUM_OF_DIRS - 1);
      } while (!char_can_go(g, ch, dir) && --tries);
      send_to_char(g, ch, "You sense them {} faintly from here, but are unsure....\r\n", dirs[dir]);
      improve_skill(g, ch, SKILL_SENSE, 1);
      improve_skill(g, ch, SKILL_SENSE, 1);
      improve_skill(g, ch, SKILL_SENSE, 1);
      return;
    }

    /* They passed the skill check. */
    dir = find_first_step(g, char_get_in_room(ch), char_get_in_room(vict));

    switch (dir) {
      case BFS_ERROR:
        send_to_char(g, ch, "Hmm.. something seems to be wrong.\r\n");
        break;
      case BFS_ALREADY_THERE:
        act(g, "You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
        act(g, "$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
        act(g, "$n looks at $N intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
        if (!char_is_android(vict)) {
          if (char_get_alignment(vict) > 50 && char_get_alignment(vict) < 200) {
            send_to_char(g, ch, "You sense slightly pure and good ki from them.\r\n");
          } else if (char_get_alignment(vict) > 200 && char_get_alignment(vict) < 500) {
            send_to_char(g, ch, "You sense a pure and good ki from them.\r\n");
          } else if (char_get_alignment(vict) >= 500) {
            send_to_char(g, ch, "You sense an extremely pure and good ki from them.\r\n");
          } else if (char_get_alignment(vict) < -50 && char_get_alignment(vict) > -200) {
            send_to_char(g, ch, "You sense slightly sour and evil ki from them.\r\n");
          } else if (char_get_alignment(vict) < -200 && char_get_alignment(vict) > -500) {
            send_to_char(g, ch, "You sense a sour and evil ki from them.\r\n");
          } else if (char_get_alignment(vict) <= -500) {
            send_to_char(g, ch, "You sense an extremely evil ki from them.\r\n");
          } else if (char_get_alignment(vict) > -50 && char_get_alignment(vict) < 50) {
            send_to_char(g, ch, "You sense slightly mild indefinable ki from them.\r\n");
          }
        }
        if (!char_is_android(vict)) {
          if (getCurPL(g, vict) > getCurPL(g, ch) * 50) {
            send_to_char(g, ch, "Their power is so huge it boggles your mind and crushes your spirit to fight!\n");
          } else if (getCurPL(g, vict) > getCurPL(g, ch) * 25) {
            send_to_char(g, ch, "Their power is so much larger than you that you would die like an insect.\n");
          } else if (getCurPL(g, vict) > getCurPL(g, ch) * 10) {
            send_to_char(g, ch, "Their power is many times larger than your own.\n");
          } else if (getCurPL(g, vict) > getCurPL(g, ch) * 5) {
            send_to_char(g, ch, "Their power is a great deal larger than your own.\n");
          } else if (getCurPL(g, vict) > getCurPL(g, ch) * 2) {
            send_to_char(g, ch, "Their power is more than twice as large as your own.\n");
          } else if (getCurPL(g, vict) > getCurPL(g, ch)) {
            send_to_char(g, ch, "Their power is about twice as large as your own.\n");
          } else if (getCurPL(g, vict) == getCurPL(g, ch)) {
            send_to_char(g, ch, "Their power is exactly as strong as you.\n");
          } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.75) {
            send_to_char(g, ch, "Their power is about a quarter of your own or larger.\n");
          } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.5) {
            send_to_char(g, ch, "Their power is about half of your own or larger.\n");
          } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.25) {
            send_to_char(g, ch, "Their power is about a quarter of your own or larger.\n");
          } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.1) {
            send_to_char(g, ch, "Their power is about a tenth of your own or larger.\n");
          } else if (getCurPL(g, vict) >= getCurPL(g, ch) * 0.01) {
            send_to_char(g, ch, "Their power is less than a tenth of your own.\n");
          } else if (getCurPL(g, vict) < getCurPL(g, ch) * 0.01) {
            send_to_char(g, ch, "Their power is less than 1 percent of your own. What a weakling...\n");
          }
          if (!read_sense_memory(g, ch, vict)) {
            send_to_char(g, ch, "You will remember their ki signal from now on.\r\n");
            sense_memory_write(g, ch, vict);
          }
        } else {
          send_to_char(g, ch, "You can't sense their powerlevel as they are a machine.\r\n");
        }
        break;
      case BFS_TO_FAR:
        send_to_char(g, ch, "You are too far to sense {} accurately from here.\r\n", char_hmhr(vict));
        break;
      case BFS_NO_PATH:
        send_to_char(g, ch, "You can't sense {} from here.\r\n", char_hmhr(vict));
        break;
      default: /* Success! */
        if ((char_get_skills_base(ch, SKILL_SENSE) >= 75)) {
          const char *blah = sense_location(g, vict);
          send_to_char(g, ch, "You sense them {} from here!\r\n", dirs[dir]);
          send_to_char(g, ch, "@WSense@D: @Y{}@n\r\n", blah);
        } else {
          send_to_char(g, ch, "You sense them {} from here!\r\n", dirs[dir]);
          break;
        }
        char_set_wait(ch, PULSE_2SEC);
        improve_skill(g, ch, SKILL_SENSE, 1);
        improve_skill(g, ch, SKILL_SENSE, 1);
        improve_skill(g, ch, SKILL_SENSE, 1);
    }
  }
}
