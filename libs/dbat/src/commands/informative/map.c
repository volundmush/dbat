/* map.c: maps, scanning and searching
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"
#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/disabled.h"
#include "informative_internal.h"

static double terrain_bonus(struct char_data *ch);
static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
                          struct char_data *ch);

/* This is used to determine the terrain bonus for search_room - Iovan 12/16/2012*/
static double terrain_bonus(struct char_data *ch)
{

  double bonus = 0.0;

  switch (rroom_sect(char_get_in_room(ch))) {
    case SECT_FOREST:
      bonus += 0.5;
      break;
    case SECT_SPACE:
      bonus += -0.5;
      break;
    case SECT_WATER_NOSWIM:
      bonus += 0.25;
      break;
    case SECT_MOUNTAIN:
      bonus += 0.1;
      break;
    default:
      bonus += 0.0;
      break;
  }

  if (rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) {
    bonus += -0.5;
  }

  return (bonus);
}

/* This is used to find hidden people in a room with search - Iovan 12/16/2012 */
void search_room(struct char_data *ch)
{

  struct char_data *vict, *next_v;
  int perc = (char_get_aff_abils_intel(ch) * 0.6) + char_get_skill(ch, SKILL_SPOT) + char_get_skill(ch, SKILL_SEARCH) + char_get_skill(ch, SKILL_LISTEN);
  int prob = 0, found = 0;
  double bonus = 1.0, terrain = 1.0;

  if ((getCurST(ch)) < GET_MAX_MOVE(ch) * 0.001) {
    send_to_char(ch, "You do not have enough stamina.\r\n");
    return;
  }

  if (char_get_skill(ch, SKILL_SENSE)) {
    bonus += (char_get_skill(ch, SKILL_SENSE) * 0.01);
  }

  reveal_hiding(ch, 0);
  act("@y$n@Y begins searching the room carefully.@n", TRUE, ch, 0, 0, TO_ROOM);
  WAIT_STATE(ch, PULSE_1SEC);

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    if (char_aff_flagged(vict, AFF_HIDE) && vict != ch) {
      if (char_get_suppression(vict) >= 1) {
        perc *= (char_get_suppression(vict) * 0.01);
      }
      prob = char_get_aff_abils_dex(vict) + (char_get_aff_abils_intel(vict) * 0.6) + char_get_skill(vict, SKILL_HIDE) + char_get_skill(vict, SKILL_MOVE_SILENTLY);

      if (char_aff_flagged(vict, AFF_LIQUEFIED)) {
        prob *= 1.5;
      }
      if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4)) {
        perc += 5;
      }
      if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 5 || char_get_genome(vict, 1) == 5)) {
        prob += 10;
      }
      terrain += terrain_bonus(vict);
      if (perc * bonus >= prob * terrain) { /* Found them. */
        act("@YYou find @y$N@Y hiding nearby!@n", TRUE, ch, 0, vict, TO_CHAR);
        act("@y$n@Y has found your hiding spot!@n", TRUE, ch, 0, vict, TO_VICT);
        act("@y$n@Y has found @y$N's@Y hiding spot!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        reveal_hiding(vict, 4);
        found++;
      }
    }
  }

  struct obj_data *obj = NULL;

  for (obj = world[char_get_in_room(ch)].contents; obj; obj = obj->next_content) {
    if (obj_extra_flagged(obj, ITEM_BURIED) && perc * bonus > rand_number(50, 200)) {
      act("@YYou uncover @y$p@Y, which had been burried here.@n", TRUE, ch, obj, 0, TO_CHAR);
      act("@y$n@Y uncovers @y$p@Y, which had burried here.@n", TRUE, ch, obj, 0, TO_ROOM);
      obj_extra_set(obj, ITEM_BURIED, false);
      found++;
    }
  }
  decCurSTPercent(ch, .001, 0);

  if (found == 0) {
    send_to_char(ch, "You find nothing hidden.\r\n");
    return;
  }
}

static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
                          struct char_data *ch)
{
  int door;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (world[rnum].dir_option[door] && world[rnum].dir_option[door]->to_room != NOWHERE && (room_exit_flagged(world[rnum].dir_option[door], EX_CLOSED) && !room_exit_flagged(world[rnum].dir_option[door], EX_SECRET))) {
      switch (door) {
        case NORTH:
          map[y - 1][x] = '8';
          break;
        case EAST:
          map[y][x + 1] = '8';
          break;
        case SOUTH:
          map[y + 1][x] = '8';
          break;
        case WEST:
          map[y][x - 1] = '8';
          break;
        case NORTHEAST:
          map[y - 1][x + 1] = '8';
          break;
        case NORTHWEST:
          map[y - 1][x - 1] = '8';
          break;
        case SOUTHEAST:
          map[y + 1][x + 1] = '8';
          break;
        case SOUTHWEST:
          map[y + 1][x - 1] = '8';
          break;
      }
    } else if (world[rnum].dir_option[door] && world[rnum].dir_option[door]->to_room != NOWHERE && !room_exit_flagged(world[rnum].dir_option[door], EX_CLOSED)) {
      switch (door) {
        case NORTH:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y - 1][x] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '2';
            } else {
              map[y - 1][x] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '2';
            } else {
              map[y - 1][x] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '7';
            } else {
              map[y - 1][x] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '1';
            } else {
              map[y - 1][x] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '6';
            } else {
              map[y - 1][x] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '5';
            } else {
              map[y - 1][x] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x] = '3';
            } else {
              map[y - 1][x] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y - 1][x] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y - 1][x] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y - 1][x] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y - 1][x] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y - 1][x] = '*';
          } else {
            map[y - 1][x] = '-';
          }
          break;
        case EAST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y][x + 1] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '2';
            } else {
              map[y][x + 1] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '2';
            } else {
              map[y][x + 1] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '7';
            } else {
              map[y][x + 1] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '1';
            } else {
              map[y][x + 1] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '6';
            } else {
              map[y][x + 1] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '5';
            } else {
              map[y][x + 1] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x + 1] = '3';
            } else {
              map[y][x + 1] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y][x + 1] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y][x + 1] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y][x + 1] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y][x + 1] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y][x + 1] = '*';
          } else {
            map[y][x + 1] = '-';
          }
          break;
        case SOUTH:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y + 1][x] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '2';
            } else {
              map[y + 1][x] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '2';
            } else {
              map[y + 1][x] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '7';
            } else {
              map[y + 1][x] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '1';
            } else {
              map[y + 1][x] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '6';
            } else {
              map[y + 1][x] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '5';
            } else {
              map[y + 1][x] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '3';
            } else {
              map[y + 1][x] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y + 1][x] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y + 1][x] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y + 1][x] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y + 1][x] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y + 1][x] = '*';
          } else {
            map[y + 1][x] = '-';
          }
          break;
        case WEST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y][x - 1] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x] = '2';
            } else {
              map[y][x - 1] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x - 1] = '2';
            } else {
              map[y][x - 1] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x - 1] = '7';
            } else {
              map[y][x - 1] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x - 1] = '1';
            } else {
              map[y][x - 1] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x - 1] = '6';
            } else {
              map[y][x - 1] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x - 1] = '5';
            } else {
              map[y][x - 1] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y][x - 1] = '3';
            } else {
              map[y][x - 1] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y][x - 1] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y][x - 1] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y][x - 1] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y][x - 1] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y][x - 1] = '*';
          } else {
            map[y][x - 1] = '-';
          }
          break;
        case NORTHEAST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y - 1][x + 1] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '2';
            } else {
              map[y - 1][x + 1] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '2';
            } else {
              map[y - 1][x + 1] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '7';
            } else {
              map[y - 1][x + 1] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '1';
            } else {
              map[y - 1][x + 1] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '6';
            } else {
              map[y - 1][x + 1] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '5';
            } else {
              map[y - 1][x + 1] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x + 1] = '3';
            } else {
              map[y - 1][x + 1] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y - 1][x + 1] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y - 1][x + 1] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y - 1][x + 1] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y - 1][x + 1] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y - 1][x + 1] = '*';
          } else {
            map[y - 1][x + 1] = '-';
          }
          break;
        case NORTHWEST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y - 1][x - 1] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '2';
            } else {
              map[y - 1][x - 1] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '2';
            } else {
              map[y - 1][x - 1] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '7';
            } else {
              map[y - 1][x - 1] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '1';
            } else {
              map[y - 1][x - 1] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '6';
            } else {
              map[y - 1][x - 1] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '5';
            } else {
              map[y - 1][x - 1] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y - 1][x - 1] = '3';
            } else {
              map[y - 1][x - 1] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y - 1][x - 1] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y - 1][x - 1] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y - 1][x - 1] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y - 1][x - 1] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y - 1][x - 1] = '*';
          } else {
            map[y - 1][x - 1] = '-';
          }
          break;
        case SOUTHEAST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y + 1][x + 1] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '2';
            } else {
              map[y + 1][x + 1] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '2';
            } else {
              map[y + 1][x + 1] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '7';
            } else {
              map[y + 1][x + 1] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '1';
            } else {
              map[y + 1][x + 1] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '6';
            } else {
              map[y + 1][x + 1] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '5';
            } else {
              map[y + 1][x + 1] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x + 1] = '3';
            } else {
              map[y + 1][x + 1] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y + 1][x + 1] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y + 1][x + 1] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y + 1][x + 1] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y + 1][x + 1] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y + 1][x + 1] = '*';
          } else {
            map[y + 1][x + 1] = '-';
          }
          break;
        case SOUTHWEST:
          if (rroom_sunken(world[rnum].dir_option[door]->to_room)) {
            map[y + 1][x - 1] = '=';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_INSIDE) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '2';
            } else {
              map[y + 1][x - 1] = 'i';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FIELD) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '2';
            } else {
              map[y + 1][x - 1] = 'p';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_DESERT) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '7';
            } else {
              map[y + 1][x - 1] = '!';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_CITY) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '1';
            } else {
              map[y + 1][x - 1] = '(';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FOREST) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '6';
            } else {
              map[y + 1][x - 1] = 'f';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_MOUNTAIN) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '5';
            } else {
              map[y + 1][x - 1] = '^';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_HILLS) {
            if (rroom_get_geffect(world[rnum].dir_option[door]->to_room) >= 1) {
              map[y + 1][x - 1] = '3';
            } else {
              map[y + 1][x - 1] = 'h';
            }
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_FLYING) {
            map[y + 1][x - 1] = 's';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_NOSWIM) {
            map[y + 1][x - 1] = '`';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_WATER_SWIM) {
            map[y + 1][x - 1] = '+';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_SHOP) {
            map[y + 1][x - 1] = '&';
          } else if (rroom_sect(world[rnum].dir_option[door]->to_room) == SECT_IMPORTANT) {
            map[y + 1][x - 1] = '*';
          } else {
            map[y + 1][x - 1] = '-';
          }
          break;
      }
    }
  }
}

ACMD(do_map)
{
  gen_map(ch, 1);
}

void gen_map(struct char_data *ch, int num)
{
  int door, i;
  char map[9][10] = {{'-'}, {'-'}};
  char buf2[MAX_INPUT_LENGTH];

  if (num == 1) {
    /* Map Key */
    send_to_char(ch, "@W               @D-[@CArea Map@D]-\r\n");
    send_to_char(ch, "@D-------------------------------------------@w\r\n");
    send_to_char(ch, "@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
    send_to_char(ch, "@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
    send_to_char(ch, "@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
    send_to_char(ch, "@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
    send_to_char(ch, "@WLastly @RX@W = You.\r\n");
    send_to_char(ch, "@D-------------------------------------------\r\n");
    send_to_char(ch, "@D                  @CNorth@w\r\n");
    send_to_char(ch, "@D                    @c^@w\r\n");
    send_to_char(ch, "@D             @CWest @c< O > @CEast@w\r\n");
    send_to_char(ch, "@D                    @cv@w\r\n");
    send_to_char(ch, "@D                  @CSouth@w\r\n");
    send_to_char(ch, "@D                ---------@w\r\n");
  }

  /* blank the map */
  for (i = 0; i < 9; i++) {
    strcpy(map[i], "         ");
  }

  /* print out exits */
  map_draw_room(map, 4, 4, ch->in_room, ch);
  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (char_get_exit(ch, door) && char_get_exit(ch, door)->to_room != NOWHERE && !room_exit_flagged(char_get_exit(ch, door), EX_CLOSED)) {
      switch (door) {
        case NORTH:
          map_draw_room(map, 4, 3, char_get_exit(ch, door)->to_room, ch);
          break;
        case EAST:
          map_draw_room(map, 5, 4, char_get_exit(ch, door)->to_room, ch);
          break;
        case SOUTH:
          map_draw_room(map, 4, 5, char_get_exit(ch, door)->to_room, ch);
          break;
        case WEST:
          map_draw_room(map, 3, 4, char_get_exit(ch, door)->to_room, ch);
          break;
        case NORTHEAST:
          map_draw_room(map, 5, 3, char_get_exit(ch, door)->to_room, ch);
          break;
        case NORTHWEST:
          map_draw_room(map, 3, 3, char_get_exit(ch, door)->to_room, ch);
          break;
        case SOUTHEAST:
          map_draw_room(map, 5, 5, char_get_exit(ch, door)->to_room, ch);
          break;
        case SOUTHWEST:
          map_draw_room(map, 3, 5, char_get_exit(ch, door)->to_room, ch);
          break;
      }
    }
  }

  /* make it obvious what room they are in */
  map[4][4] = 'x';

  /* print out the map */
  int key = 0;
  *buf2   = '\0';
  for (i = 2; i < 9; i++) {
    if (i > 6) {
      continue;
    }
    if (num == 1) {
      sprintf(buf2, "@w                %s\r\n", map[i]);
    } else {
      if (i == 2) {
        sprintf(buf2, "@w       @w|%s@w|           %s", (char_get_exit(ch, 0) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 0), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 0), EX_CLOSED) ? " @rN " : " @CN ") : "   ", map[i]);
      }
      if (i == 3) {
        sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s", (char_get_exit(ch, 6) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 6), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 6), EX_CLOSED) ? " @rNW" : " @CNW") : "   ", (char_get_exit(ch, 4) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 4), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 4), EX_CLOSED) ? " @yU " : " @YU ") : "   ", (char_get_exit(ch, 7) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 7), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 7), EX_SECRET) ? "@rNE " : "@CNE ") : "   ", map[i]);
      }
      if (i == 4) {
        sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s", (char_get_exit(ch, 3) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 3), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 3), EX_CLOSED) ? "  @rW" : "  @CW") : "   ", (char_get_exit(ch, 10) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 10), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 10), EX_CLOSED) ? " @rI " : " @mI ") : ((char_get_exit(ch, 11) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 11), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 11), EX_CLOSED) ? "@rOUT" : "@mOUT") : "@r{ }"), (char_get_exit(ch, 1) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 1), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 1), EX_CLOSED) ? "@rE  " : "@CE  ") : "   ", map[i]);
      }
      if (i == 5) {
        sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s", (char_get_exit(ch, 9) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 9), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 9), EX_CLOSED) ? " @rSW" : " @CSW") : "   ", (char_get_exit(ch, 5) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 5), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 5), EX_CLOSED) ? " @yD " : " @YD ") : "   ", (char_get_exit(ch, 8) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 8), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 8), EX_SECRET) ? "@rSE " : "@CSE ") : "   ", map[i]);
      }
      if (i == 6) {
        sprintf(buf2, "@w       @w|%s@w|           %s", (char_get_exit(ch, 2) && !room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 2), EX_SECRET)) ? (room_exit_flagged(rroom_get_dir_option(char_get_in_room(ch), 2), EX_CLOSED) ? " @rS " : " @CS ") : "   ", map[i]);
      }
    }
    search_replace(buf2, "x", "@RX");
    search_replace(buf2, "&", "@m$");
    search_replace(buf2, "*", "@m#");
    search_replace(buf2, "+", "@c~");
    search_replace(buf2, "s", "@CS");
    search_replace(buf2, "i", "@wI");
    search_replace(buf2, "(", "@WC");
    search_replace(buf2, "^", "@DM");
    search_replace(buf2, "h", "@yH");
    search_replace(buf2, "`", "@BW");
    search_replace(buf2, "=", "@bU");
    search_replace(buf2, "p", "@GP");
    search_replace(buf2, "f", "@gF");
    search_replace(buf2, "!", "@YD");
    search_replace(buf2, "-", "@w:");
    /* ------- Do Lava Rooms ------- */
    search_replace(buf2, "1", "@4@YC@n");
    search_replace(buf2, "2", "@4@YP@n");
    search_replace(buf2, "3", "@4@YH@n");
    search_replace(buf2, "7", "@4@YD@n");
    search_replace(buf2, "5", "@4@YM@n");
    search_replace(buf2, "6", "@4@YF@n");
    /* ------- Do Closed Rooms------- */
    search_replace(buf2, "8", "@1 @n");

    if (num != 1) {
      if (key == 0) {
        send_to_char(ch, "%s    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n", buf2);
      }
      if (key == 1) {
        send_to_char(ch, "%s    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n", buf2);
      }
      if (key == 2) {
        send_to_char(ch, "%s    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n", buf2);
      }
      if (key == 3) {
        send_to_char(ch, "%s    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n", buf2);
      }
      if (key == 4) {
        send_to_char(ch, "%s    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n", buf2);
      }
      key += 1;
    } else {
      send_to_char(ch, "%s", buf2);
    }
  }
  if (num == 1) {
    send_to_char(ch, "@D                ---------@w\r\n");
  }
}

ACMD(do_scan)
{
  int i, newroom;
  char *dirnames[] = {
      "North", "East", "South", "West", "Up", "Down", "Northwest", "Northeast", "Southeast", "Southwest", "Inside", "Outside"};

  if (char_get_position(ch) < POS_SLEEPING) {
    send_to_char(ch, "You can't see anything but stars!\n\r");
    return;
  }
  if (!char_is_awake(ch)) {
    send_to_char(ch, "You must be dreaming.\n\r");
    return;
  }
  if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see a damn thing, you're blind!\n\r");
    return;
  }
  if (char_plr_flagged(ch, PLR_EYEC)) {
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  for (i = 0; i < 10; i++) {
    if (char_get_exit(ch, i)) {
      if (room_is_dark(ch->in_room) && (char_get_admlevel(ch) < ADMLVL_IMMORT) &&
          (!char_aff_flagged(ch, AFF_INFRAVISION))) {
        send_to_char(ch, "%s: DARK\n\r", dirnames[i]);
        continue;
      }
      if (char_can_go(ch, i)) {
        newroom = world[ch->in_room].dir_option[i]->to_room;
        send_to_char(ch, "@w-----------------------------------------@n\r\n");
        send_to_char(ch, "          %s%s: %s %s\n\r", CCCYN(ch, C_NRM), dirnames[i], world[newroom].name ? world[newroom].name : "You don't think you saw what you just saw.", CCNRM(ch, C_NRM));
        send_to_char(ch, "@W          -----------------          @n\r\n");

        list_obj_to_char(world[newroom].contents, ch, SHOW_OBJ_LONG, FALSE);
        list_char_to_char(world[newroom].people, ch);
        if (rroom_get_geffect(newroom) >= 1 && rroom_get_geffect(newroom) <= 5) {
          send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
        }
        if (rroom_get_geffect(newroom) >= 6) {
          send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
        }
        /* Check 2nd room away */
        if (char_get_exit2(ch, i) && char_get_exit2(ch, i)->to_room) {
          newroom = char_get_exit2(ch, i)->to_room;

          if ((newroom != NOWHERE) && (!IS_SET(char_get_exit2(ch, i)->exit_info, EX_CLOSED))) {
            if (!room_is_dark(char_get_exit2(ch, i)->to_room)) {
              send_to_char(ch, "@w-----------------------------------------@n\r\n");
              send_to_char(ch, "          %sFar %s: %s %s\n\r", CCCYN(ch, C_NRM), dirnames[i], world[newroom].name ? world[newroom].name : "You don't think you saw what you just saw.", CCNRM(ch, C_NRM));
              send_to_char(ch, "@W          -----------------          @n\r\n");

              list_obj_to_char(world[newroom].contents, ch, SHOW_OBJ_LONG, FALSE);
              list_char_to_char(world[newroom].people, ch);
              if (rroom_get_geffect(newroom) >= 1 && rroom_get_geffect(newroom) <= 5) {
                send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
              }
              if (rroom_get_geffect(newroom) >= 6) {
                send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
              }
            } else {
              send_to_char(ch, "%s<-> %sFar %s: Too dark to tell! %s<->%s\r\n", QMAG, QCYN, dirnames[i], QMAG, QNRM);
            }
          }
        }
      }
    }
  }
  send_to_char(ch, "@w-----------------------------------------@n\r\n");
}

#define DOOR_DCHIDE(ch, door) (char_get_exit(ch, door)->dchide)

void search_in_direction(struct char_data *ch, int dir)
{
  int check = FALSE, skill_lvl, dchide = 20;

  send_to_char(ch, "You search for secret doors.\r\n");
  act("$n searches the area intently.", TRUE, ch, 0, 0, TO_ROOM);

  /* SEARCHING is allowed untrained */
  skill_lvl = char_get_skill(ch, SKILL_SEARCH);
  if (char_is_truffle(ch) || char_is_human(ch))
    skill_lvl = skill_lvl + 2;
  if (char_is_halfbreed(ch))
    skill_lvl = skill_lvl + 1;

  if (char_get_exit(ch, dir))
    dchide = DOOR_DCHIDE(ch, dir);

  if (skill_lvl > dchide)
    check = TRUE;

  if (char_get_exit(ch, dir)) {
    if (char_get_exit(ch, dir)->general_description &&
        !room_exit_flagged(char_get_exit(ch, dir), EX_SECRET))
      send_to_char(ch, "%s", char_get_exit(ch, dir)->general_description);
    else if (!room_exit_flagged(char_get_exit(ch, dir), EX_SECRET))
      send_to_char(ch, "There is a normal exit there.\r\n");
    else if (room_exit_flagged(char_get_exit(ch, dir), EX_ISDOOR) &&
             room_exit_flagged(char_get_exit(ch, dir), EX_SECRET) &&
             char_get_exit(ch, dir)->keyword && (check == TRUE))
      send_to_char(ch, "There is a hidden door keyword: '%s' %sthere.\r\n",
                   fname(char_get_exit(ch, dir)->keyword),
                   (room_exit_flagged(char_get_exit(ch, dir), EX_CLOSED)) ? "" : "open ");
    else
      send_to_char(ch, "There is no exit there.\r\n");
  } else
    send_to_char(ch, "There is no exit there.\r\n");
}
