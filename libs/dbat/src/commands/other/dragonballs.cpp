/* dragonballs.c: dragon balls, wishes and summoning
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <sys/stat.h>

#include "dbat/iterate.hpp"
#include "dbat/act.other.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/utils.hpp"
#include "dbat/flags.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/races.hpp"
#include "dbat/fileop.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/weather.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/graph.hpp"
#include "dbat/alias.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/shop.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

/* A dragon saved as being somewhere, when it or that room no longer exists, is forgotten. */
static void load_shadow_dragon(GameWorld *g, int *room, mob_vnum vnum)
{
  mob_vnum r_num;
  room_vnum r_room;

  if (*room <= 0)
    return;
  r_num  = vnum;
  r_room = *room;
  if (!mob_proto_of(g, r_num) || !room_of(g, r_room)) {
    mud_log("SYSERR: Can't put shadow dragon (mob %d) back in room %d: one of them doesn't exist.", vnum, *room);
    *room = -1;
    return;
  }
  char_to_room(g, read_mobile(g, r_num), r_room);
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
          send_to_room(g, g->DRAGONR, "@WThe dragon balls on the ground begin to glow yellow in slow pulses.@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky begins to grow dark and cloudy suddenly.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 295:
          send_to_room(g, g->DRAGONR, "@WSuddenly lightning shoots into the sky, twisting about as a roar can be heard for miles!@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky flashes with lightning.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 290:
          send_to_room(g, g->DRAGONR, "@WThe lightning takes shape and slowly the Eternal Dragon, Shenron, can be made out from the glow!@n\r\n");
          if (char_data *dragon = char_by_id(g, g->EDRAGON_uid)) {
            char_from_room(g, dragon);
            char_to_room(g, dragon, g->DRAGONR);
          }
          g->DRAGONC -= 1;
          break;
        case 285:
          send_to_planet(g, 0, ROOM_EARTH, "@DThe lightning stops suddenly, but the sky remains mostly dark.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 280:
          send_to_room(g, g->DRAGONR, "@WThe glow around Shenron becomes subdued as the Eternal Dragon coils so that his head is looking down on the dragon balls!@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 275:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CWho summoned me? I will grant you any two wishes that are within my power.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 180:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CMake your wish already, you only have 3 minutes remaining.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 120:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CMake your wish. I am losing patience, you only have 2 minutes left.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 60:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CMake your wish now! You only have 1 minute left.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 0:
          send_to_room(g, g->DRAGONR, "Shenron growls and disappears with a blinding flash that is absorbed into the dragon balls. The glowing dragon balls then float high into the sky, splitting into several directions and streaking across the sky!@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky grows brighter again as the clouds disappear magicly.@n\r\n");
          if (char_data *dragon = char_by_id(g, g->EDRAGON_uid))
            extract_char(g, dragon);
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
      send_to_room(g, g->DRAGONR, "@RThe dragon balls suddenly begin to crack and darkness begins to pour out through the cracks! Shenron begins to turn pitch black slowly as the darkness escapes. Suddenly Shenron explodes out into the distance in seven parts. Each part taking a dragon ball with it!@n\r\n");
      room_vnum num = rand_number(g, 200, 20000);
      int done = false, place = 1;
      g->DRAGONC = 0;
      g->WISH[0] = 0;
      g->WISH[1] = 0;
      while (done == false) {
        switch (place) {
          case 1:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
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
      mob_vnum r_num;

      r_num = SHADOW_DRAGON1_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON1);
      mob = nullptr;

      r_num = SHADOW_DRAGON2_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON2);
      mob = nullptr;

      r_num = SHADOW_DRAGON3_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON3);
      mob = nullptr;

      r_num = SHADOW_DRAGON4_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON4);
      mob = nullptr;

      r_num = SHADOW_DRAGON5_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON5);
      mob = nullptr;

      r_num = SHADOW_DRAGON6_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON6);
      mob = nullptr;

      r_num = SHADOW_DRAGON7_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON7);
      mob = nullptr;

      if (char_data *dragon = char_by_id(g, g->EDRAGON_uid))
        extract_char(g, dragon);
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
  struct char_data *mob = nullptr;
  mob_vnum r_num;

  if (!room_flagged(g, char_get_in_room(ch), ROOM_EARTH)) {
    send_to_char(g, ch, "@wYou can not summon Shenron when you are not on earth.@n\r\n");
    return;
  } // are they on earth?

  if (room_flagged(g, char_get_in_room(ch), ROOM_NOINSTANT) || room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "You can not summon shenron in this protected area!\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_INSIDE) {
    send_to_char(g, ch, "Go outside to summon Shenron!\r\n");
    return;
  }

  for (obj_data *obj : char_inventory(g, ch)) {
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
    g->DRAGONR = char_get_in_room(ch);
    if (!room_of(g, g->DRAGONR)) {
      g->DRAGONR = char_get_in_room(ch);
    }
    send_to_imm(g, "Shenron summoned to room: %d\r\n", g->DRAGONR);

    if (struct zone_data *zone = find_zone_by_thing(g, g->DRAGONR))
      g->DRAGONZ = zone->number;

    for (obj_data *obj : char_inventory(g, ch)) {

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
    if (!mob_proto_of(g, r_num = 21)) {
      send_to_imm(g, "Shenron doesn't exist!");
      return;
    }
    mob = read_mobile(g, r_num);
    char_to_room(g, mob, 0);
    g->EDRAGON_uid = char_get_id(mob);
    return;
  }
  send_to_char(g, ch, "@wYou do not have all the dragon balls and can not summon the dragon!@n\r\n");
  return;
}

bool dball_count(GameWorld *g, struct char_data *ch)
{

  int dball[7]         = {20, 21, 22, 23, 24, 25, 26};
  int count            = 0;

  for (obj_data *obj : char_inventory(g, ch)) {
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
