/* dragonballs.c: dragon balls, wishes and summoning
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/act.other.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/shadowdragons.h"
#include "dbat/consts/auction.h"
#include "dbat/consts/aligns.h"
#include "dbat/utils.h"
#include "dbat/flags.h"
#include <sys/stat.h>
#include "dbat/config.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/zone_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/search.h"
#include "dbat/random.h"
#include "dbat/affect.h"
#include "dbat/relocate.h"
#include "dbat/extract.h"
#include "dbat/races.h"
#include "dbat/fileop.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/broadcast.h"
#include "dbat/combat.h"
#include "dbat/act.misc.h"
#include "dbat/weather.h"
#include "dbat/act.item.h"
#include "dbat/act.wizard.h"
#include "dbat/act.informative.h"
#include "dbat/act.movement.h"
#include "dbat/obj_edit.h"
#include "dbat/graph.h"
#include "dbat/alias.h"
#include "dbat/spells.h"
#include "dbat/interpreter.h"
#include "dbat/fight.h"
#include "dbat/class.h"
#include "dbat/shop.h"
#include "dbat/feats.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/objsave.h"
#include "dbat/mail.h"
#include "other_internal.h"
#include "dbat/game_world.h"

/* A dragon saved as being somewhere, when it or that room no longer exists, is forgotten. */
static void load_shadow_dragon(GameWorld *g, int *room, mob_vnum vnum)
{
  mob_rnum r_num;
  room_rnum r_room;

  if (*room <= 0)
    return;
  r_num  = real_mobile(g, vnum);
  r_room = real_room(g, *room);
  if (r_num == NOBODY || r_room == NOWHERE) {
    mud_log("SYSERR: Can't put shadow dragon (mob %d) back in room %d: one of them doesn't exist.", vnum, *room);
    *room = -1;
    return;
  }
  char_to_room(g, read_mobile(g, r_num, REAL), r_room);
}

void load_shadow_dragons(GameWorld *g)
{
  load_shadow_dragon(g, &g->SHADOW_DRAGON1, SHADOW_DRAGON1_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON2, SHADOW_DRAGON2_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON3, SHADOW_DRAGON3_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON4, SHADOW_DRAGON4_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON5, SHADOW_DRAGON5_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON6, SHADOW_DRAGON6_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON7, SHADOW_DRAGON7_VNUM);

  save_mud_time(g, &g->time_info);
}

void wishSYS(GameWorld *g)
{
  if (g->SHENRON == true) {
    if (g->SELFISHMETER < 10) {
      switch (g->DRAGONC) {
        case 300:
          send_to_room(g, real_room(g, g->DRAGONR), "@WThe dragon balls on the ground begin to glow yellow in slow pulses.@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky begins to grow dark and cloudy suddenly.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 295:
          send_to_room(g, real_room(g, g->DRAGONR), "@WSuddenly lightning shoots into the sky, twisting about as a roar can be heard for miles!@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky flashes with lightning.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 290:
          send_to_room(g, real_room(g, g->DRAGONR), "@WThe lightning takes shape and slowly the Eternal Dragon, Shenron, can be made out from the glow!@n\r\n");
          char_from_room(g, g->EDRAGON);
          char_to_room(g, g->EDRAGON, real_room(g, g->DRAGONR));
          g->DRAGONC -= 1;
          break;
        case 285:
          send_to_planet(g, 0, ROOM_EARTH, "@DThe lightning stops suddenly, but the sky remains mostly dark.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 280:
          send_to_room(g, real_room(g, g->DRAGONR), "@WThe glow around Shenron becomes subdued as the Eternal Dragon coils so that his head is looking down on the dragon balls!@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 275:
          send_to_room(g, real_room(g, g->DRAGONR), "@wShenron says, '@CWho summoned me? I will grant you any two wishes that are within my power.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 180:
          send_to_room(g, real_room(g, g->DRAGONR), "@wShenron says, '@CMake your wish already, you only have 3 minutes remaining.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 120:
          send_to_room(g, real_room(g, g->DRAGONR), "@wShenron says, '@CMake your wish. I am losing patience, you only have 2 minutes left.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 60:
          send_to_room(g, real_room(g, g->DRAGONR), "@wShenron says, '@CMake your wish now! You only have 1 minute left.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 0:
          send_to_room(g, real_room(g, g->DRAGONR), "Shenron growls and disappears with a blinding flash that is absorbed into the dragon balls. The glowing dragon balls then float high into the sky, splitting into several directions and streaking across the sky!@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky grows brighter again as the clouds disappear magicly.@n\r\n");
          extract_char(g, g->EDRAGON);
          g->SHENRON = false;
          g->DRAGONC -= 1;
          save_mud_time(g, &g->time_info);
          break;
        default:
          g->DRAGONC -= 1;
          break;
      }
      if (g->WISH[0] == 1 && g->WISH[1] == 1) {
        g->DRAGONC = 0;
        g->WISH[0] = 0;
        g->WISH[1] = 0;
      }
    } else {
      send_to_room(g, real_room(g, g->DRAGONR), "@RThe dragon balls suddenly begin to crack and darkness begins to pour out through the cracks! Shenron begins to turn pitch black slowly as the darkness escapes. Suddenly Shenron explodes out into the distance in seven parts. Each part taking a dragon ball with it!@n\r\n");
      room_vnum num = rand_number(g, 200, 20000);
      int done = false, place = 1;
      g->DRAGONC = 0;
      g->WISH[0] = 0;
      g->WISH[1] = 0;
      while (done == false) {
        switch (place) {
          case 1:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON1 = num;
                place          = 2;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 2:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON2 = num;
                place          = 3;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 3:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON3 = num;
                place          = 4;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 4:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON4 = num;
                place          = 5;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 5:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON5 = num;
                place          = 6;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 6:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON6 = num;
                place          = 7;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 7:
            if (real_room(g, num) != NOWHERE) {
              if (rroom_flagged(g, real_room(g, num), ROOM_EARTH) || rroom_flagged(g, real_room(g, num), ROOM_VEGETA) || rroom_flagged(g, real_room(g, num), ROOM_FRIGID) || rroom_flagged(g, real_room(g, num), ROOM_AETHER) || rroom_flagged(g, real_room(g, num), ROOM_NAMEK) || rroom_flagged(g, real_room(g, num), ROOM_KONACK) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT) || rroom_flagged(g, real_room(g, num), ROOM_YARDRAT)) {
                g->SHADOW_DRAGON7 = num;
                done           = true;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
        } /* End switch */
        save_mud_time(g, &g->time_info);
      } /* End while */
      struct char_data *mob = nullptr;
      mob_rnum r_num;

      r_num = real_mobile(g, SHADOW_DRAGON1_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON1));
      mob = nullptr;

      r_num = real_mobile(g, SHADOW_DRAGON2_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON2));
      mob = nullptr;

      r_num = real_mobile(g, SHADOW_DRAGON3_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON3));
      mob = nullptr;

      r_num = real_mobile(g, SHADOW_DRAGON4_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON4));
      mob = nullptr;

      r_num = real_mobile(g, SHADOW_DRAGON5_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON5));
      mob = nullptr;

      r_num = real_mobile(g, SHADOW_DRAGON6_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON6));
      mob = nullptr;

      r_num = real_mobile(g, SHADOW_DRAGON7_VNUM);
      mob   = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, real_room(g, g->SHADOW_DRAGON7));
      mob = nullptr;

      extract_char(g, g->EDRAGON);
      g->SHENRON = false;
      g->DRAGONC = 0;
    } /* End else */
  }
}

ACMD(do_summon)
{

  int summoned = false, count = 0;
  int dball[7]  = {20, 21, 22, 23, 24, 25, 26};
  int dball2[7] = {20, 21, 22, 23, 24, 25, 26};
  struct obj_data *obj, *next_obj;
  struct char_data *mob = nullptr;
  mob_rnum r_num;

  if (!rroom_flagged(g, char_get_in_room(ch), ROOM_EARTH)) {
    send_to_char(g, ch, "@wYou can not summon Shenron when you are not on earth.@n\r\n");
    return;
  } // are they on earth?

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_NOINSTANT) || rroom_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "You can not summon shenron in this protected area!\r\n");
    return;
  }
  if (rroom_sect(g, char_get_in_room(ch)) == SECT_INSIDE) {
    send_to_char(g, ch, "Go outside to summon Shenron!\r\n");
    return;
  }

  for (obj = char_get_inventory(ch); obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
    if (obj_extra_flagged(obj, ITEM_FORGED)) {
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[0]) {
      dball[0] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[1]) {
      dball[1] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[2]) {
      dball[2] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[3]) {
      dball[3] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[4]) {
      dball[4] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[5]) {
      dball[5] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[6]) {
      dball[6] = -1;
      count++;
      continue;
    } else {
      continue;
    } // end if
  } // end repeat for

  if (count == 7) {
    summoned = true;
  }

  if (summoned == true) {
    reveal_hiding(g, ch, 0);
    act(g, "@WYou place the dragon balls on the ground and with both hands outstretched towards them you say '@CArise Eternal Dragon Shenron!@W'@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@W$n places the dragon balls on the ground and with both hands outstretched towards them $e says '@CArise Eternal Dragon Shenron!@W'@n", true, ch, nullptr, nullptr, TO_ROOM);
    g->SHENRON = true;
    g->DRAGONC = 300;
    g->DRAGONR = rroom_get_vnum(g, char_get_in_room(ch));
    if (real_room(g, g->DRAGONR) == NOWHERE) {
      g->DRAGONR = rroom_get_vnum(g, char_get_in_room(ch));
    }
    send_to_imm(g, "Shenron summoned to room: %d\r\n", g->DRAGONR);

    if ((g->DRAGONZ = real_zone_by_thing(g, g->DRAGONR)) != NOWHERE) {
      g->DRAGONZ = real_zone_by_thing(g, g->DRAGONR);
    }

    for (obj = char_get_inventory(ch); obj; obj = next_obj) {
      next_obj = obj_get_next_content(obj);

      if (obj_get_vnum(g, obj) == dball2[0]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[0] = -1;
        continue;
      }
      if (obj_get_vnum(g, obj) == dball2[1]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[1] = -1;
        continue;
      }
      if (obj_get_vnum(g, obj) == dball2[2]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[2] = -1;
        continue;
      }
      if (obj_get_vnum(g, obj) == dball2[3]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[3] = -1;
        continue;
      }
      if (obj_get_vnum(g, obj) == dball2[4]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[4] = -1;
        continue;
      }
      if (obj_get_vnum(g, obj) == dball2[5]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[5] = -1;
        continue;
      }
      if (obj_get_vnum(g, obj) == dball2[6]) {
        obj_from_char(g, obj);
        extract_obj(g, obj);
        dball2[6] = -1;
        continue;
      }
      continue;
    }
    if ((r_num = real_mobile(g, 21)) == NOBODY) {
      send_to_imm(g, "Shenron doesn't exist!");
      return;
    }
    mob = read_mobile(g, r_num, REAL);
    char_to_room(g, mob, 0);
    g->EDRAGON = mob;
    return;
  }
  send_to_char(g, ch, "@wYou do not have all the dragon balls and can not summon the dragon!@n\r\n");
  return;
}

bool dball_count(GameWorld *g, struct char_data *ch)
{

  int dball[7]         = {20, 21, 22, 23, 24, 25, 26};
  int count            = 0;
  struct obj_data *obj = nullptr, *next_obj = nullptr;

  for (obj = char_get_inventory(ch); obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
    if (obj_get_vnum(g, obj) == dball[0]) {
      dball[0] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[1]) {
      dball[1] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[2]) {
      dball[2] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[3]) {
      dball[3] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[4]) {
      dball[4] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[5]) {
      dball[5] = -1;
      count++;
      continue;
    }
    if (obj_get_vnum(g, obj) == dball[6]) {
      dball[6] = -1;
      count++;
      continue;
    } else {
      continue;
    } // end if
  } // end repeat for

  if (count >= 1) {
    return true;
  }
  return false;
}
