/**************************************************************************
 *   File: act.other.c                                   Part of CircleMUD *
 *  Usage: Miscellaneous player-level commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/
#include "act.other.h"
#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/aligns.h"
#include "consts/appearance.h"
#include "consts/applies.h"
#include "consts/auction.h"
#include "consts/constates.h"
#include "consts/directions.h"
#include "consts/exitflags.h"
#include "consts/fightprefs.h"
#include "consts/magic.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/roomflags.h"
#include "consts/search.h"
#include "consts/sectortypes.h"
#include "consts/sex.h"
#include "consts/shadowdragons.h"
#include "consts/sizes.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "descriptor_utils.h"
#include "flags.h"
#include "log.h"
#include "object_api.h"
#include "object_db.h"
#include "object_impl.h"
#include "object_macros.h"
#include "races.h"
#include "room_api.h"
#include "room_db.h"
#include "skills.h"
#include "stringutils.h"
#include "util_macros.h"
#include "weather_db.h"
#include "zone_db.h"

#include <cstdlib>
#include <cstring>
#include <strings.h>

#include <sys/stat.h>

#include "extract.h"
#include "fileop.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "search.h"

#include "act.informative.h"
#include "act.item.h"
#include "act.misc.h"
#include "act.wizard.h"
#include "alias.h"
#include "clan.h"
#include "class.h"
#include "combat.h"
#include "comm.h"
#include "config.h"
#include "dg_comm.h"
#include "dg_scripts.h"
#include "feats.h"
#include "fight.h"
#include "graph.h"
#include "guild.h"
#include "handler.h"
#include "interpreter.h"
#include "iterate.hpp"
#include "mail.h"
#include "obj_edit.h"
#include "objsave.h"
#include "races_plus.h"
#include "shop.h"
#include "spells.h"
#include "weather.h"

/* local functions */

static int has_scanner(struct char_data *ch);
// static int perform_group(...);  // lua/characters/commands/misc/group.lua
// static void print_group(...);   // lua/characters/commands/misc/group.lua
static void show_clan_info(struct char_data *ch);

// definitions
/* Used by do_rpp for soft-cap */
void bring_to_cap(struct char_data *ch) {

  bool p_trans = (get_race(ch->race)->raceCanTransform() &&
                  !get_race(ch->race)->raceCanRevert());
  int64_t cap = calc_soft_cap(ch);

  switch (get_race(ch->race)->getSoftType(ch)) {
  case dbat::race::Fixed:
    if (getBasePL(ch) < cap)
      gainBasePLTransformed(ch, cap - getBasePL(ch) - 1, p_trans);
    if (getBaseKI(ch) < cap)
      gainBaseKITransformed(ch, cap - getBaseKI(ch) - 1, p_trans);
    if (getBaseST(ch) < cap)
      gainBaseSTTransformed(ch, cap - getBaseST(ch) - 1, p_trans);
  }
}

void char_bring_to_cap(struct char_data *ch) { bring_to_cap(ch); }

void char_rp_save(struct char_data *ch) {
  if (ch->desc) {
    ch->desc->rpp = GET_RP(ch);
    userWrite(ch->desc, 0, 0, 0, "index");
  }
  save_char(ch);
}

void char_rpp_custom_equip_launch(struct char_data *ch) {
  STATE(ch->desc) = CON_POBJ;
  ch->desc->obj_name   = strdup("Generic Armor Vest");
  ch->desc->obj_short  = strdup("@cGeneric @DArmor @WVest@n");
  ch->desc->obj_long   = strdup("@wA @cgeneric @Darmor @Wvest@w is lying here@n");
  ch->desc->obj_type   = 1;
  ch->desc->obj_weapon = 0;
  disp_custom_menu(ch->desc);
  ch->desc->obj_editflag = EDIT_CUSTOM;
  ch->desc->obj_editval  = EDIT_CUSTOM_MAIN;
}

void char_rpp_restring_launch(struct char_data *ch, struct obj_data *obj) {
  STATE(ch->desc) = CON_POBJ;
  ch->desc->obj_name   = strdup(obj->name);
  ch->desc->obj_was    = strdup(obj->short_description);
  ch->desc->obj_short  = strdup(obj->short_description);
  ch->desc->obj_long   = strdup(obj->description);
  ch->desc->obj_point  = obj;
  ch->desc->obj_type   = 1;
  ch->desc->obj_weapon = 0;
  disp_restring_menu(ch->desc);
  ch->desc->obj_editflag = EDIT_RESTRING;
  ch->desc->obj_editval  = EDIT_RESTRING_MAIN;
}

#if 0 // lua/characters/commands/misc/ingest.lua
ACMD(do_ingest) {

  if (IS_MAJIN(ch)) {
    struct char_data *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(ch, "Who do you want to ingest?\r\n");
      return;
    }

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Ingest who?\r\n");
      return;
    }
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (ABSORBBY(vict)) {
      send_to_char(ch, "%s is already absorbing from them!",
                   GET_NAME(ABSORBBY(vict)));
      return;
    }
    if (GET_ABSORBS(ch) > 3) {
      send_to_char(ch, "You already have already ingested 4 people.\r\n");
      return;
    }
    if (GET_LEVEL(ch) < 25) {
      send_to_char(ch, "You can't ingest yet.\r\n");
      return;
    }
    if (GET_LEVEL(ch) < 100 && GET_LEVEL(ch) >= 75 && GET_ABSORBS(ch) == 3) {
      send_to_char(ch, "You already have ingested as much as you can. You'll "
                       "have to get more experienced.\r\n");
      return;
    }
    if (GET_LEVEL(ch) < 75 && GET_LEVEL(ch) >= 50 && GET_ABSORBS(ch) == 2) {
      send_to_char(ch, "You already have ingested as much as you can. You'll "
                       "have to get more experienced.\r\n");
      return;
    }
    if (GET_LEVEL(ch) < 50 && GET_LEVEL(ch) >= 25 && GET_ABSORBS(ch) == 1) {
      send_to_char(ch, "You already have ingested as much as you can. You'll "
                       "have to get more experienced.\r\n");
      return;
    }

    if (GET_MAX_HIT(vict) >= (getBasePL(ch)) * 3) {
      send_to_char(ch, "You are too weak to ingest them into your body!\r\n");
      return;
    }
    if (AFF_FLAGGED(vict, AFF_SANCTUARY)) {
      send_to_char(ch, "You can't ingest them, they have a barrier!\r\n");
      return;
    }
    reveal_hiding(ch, 0);
    if (char_condition_has(vict, "zanzoken") && (getCurST(vict)) >= 1 &&
        GET_POS(vict) != POS_SLEEPING) {
      act("@C$N@c disappears, avoiding your attempted ingestion!@n", FALSE, ch,
          0, vict, TO_CHAR);
      act("@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before "
          "reappearing!@n",
          FALSE, ch, 0, vict, TO_VICT);
      act("@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before "
          "reappearing!@n",
          FALSE, ch, 0, vict, TO_NOTVICT);
      char_condition_remove(vict, "zanzoken", "zanzoken_over");
      WAIT_STATE(ch, PULSE_3SEC);
      return;
    }
    if (GET_SPEEDI(ch) + rand_number(1, 5) <
        GET_SPEEDI(ch) + rand_number(1, 5)) {
      act("@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E "
          "manages to avoid your blob of goo though!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W flings a piece of goo at you, you manage to avoid it "
          "though!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      WAIT_STATE(ch, PULSE_3SEC);
      return;
    } else {
      act("@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then "
          "returns to your body!@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W flings a piece of goo at you! The goo engulfs your body and "
          "then returns to @C$n@W!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then "
          "return to @C$n@W!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      GET_ABSORBS(ch) += 1;
      int64_t pl = (getBasePL(vict)) / 6;
      int64_t stam = (getBaseST(vict)) / 6;
      int64_t ki = (getBaseKI(vict)) / 6;
      gainBasePLTransformed(ch, pl, true);
      gainBaseSTTransformed(ch, stam, true);
      gainBaseKITransformed(ch, ki, true);
      if (!IS_NPC(vict) && !IS_NPC(ch)) {
        send_to_imm("[PK] %s killed %s at room [%d]\r\n", GET_NAME(ch),
                    GET_NAME(vict), char_room_vnum_get(vict));
        SET_BIT_AR(PLR_FLAGS(vict), PLR_ABSORBED);
      }
      send_to_char(ch,
                   "@D[@mINGEST@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) "
                   "@gSt@W: @D(@y%s@D)@n\r\n",
                   add_commas(pl), add_commas(ki), add_commas(stam));
      if (rand_number(1, 3) == 3) {
        send_to_char(ch, "You get %s's eye color.\r\n", GET_NAME(vict));
        GET_EYE(ch) = GET_EYE(vict);
      } else if (rand_number(1, 3) == 3) {
        send_to_char(ch, "%s changes your height.\r\n", GET_NAME(vict));
        if (GET_PC_HEIGHT(ch) > GET_PC_HEIGHT(vict)) {
          char_stat_mod(ch, "height",
                        -((GET_PC_HEIGHT(ch) - GET_PC_HEIGHT(vict)) / 2));
        } else if (GET_PC_HEIGHT(ch) < GET_PC_HEIGHT(vict)) {
          char_stat_mod(ch, "height",
                        ((GET_PC_HEIGHT(vict) - GET_PC_HEIGHT(ch)) / 2));
        } else {
          char_stat_set(ch, "height", GET_PC_HEIGHT(vict));
        }
      } else if (rand_number(1, 3) == 3) {
        send_to_char(ch, "%s changes your weight.\r\n", GET_NAME(vict));
        if (GET_PC_WEIGHT(ch) > GET_PC_WEIGHT(vict)) {
          char_stat_mod(ch, "weight",
                        -((GET_PC_WEIGHT(ch) - GET_PC_WEIGHT(vict)) / 2));
        } else if (GET_PC_WEIGHT(ch) < GET_PC_WEIGHT(vict)) {
          char_stat_mod(ch, "weight",
                        ((GET_PC_WEIGHT(vict) - GET_PC_WEIGHT(ch)) / 2));
        } else {
          char_stat_set(ch, "weight", GET_PC_WEIGHT(vict));
        }
      } else {
        send_to_char(ch, "Your forelock length changes because of %s.\r\n",
                     GET_NAME(vict));
        GET_HAIRL(ch) = GET_HAIRL(vict);
      }
      handle_ingest_learn(ch, vict);
      die(vict, NULL);
      return;
    }
  } // End of ingest

  else {
    send_to_char(ch, "You are not a majin, you can not ingest.\r\n");
    return;
  } // Error
}
#endif

void load_shadow_dragons() {

  const std::pair<mob_vnum, room_vnum> shadow_dragons[] = {
      {SHADOW_DRAGON1_VNUM, SHADOW_DRAGON1},
      {SHADOW_DRAGON2_VNUM, SHADOW_DRAGON2},
      {SHADOW_DRAGON3_VNUM, SHADOW_DRAGON3},
      {SHADOW_DRAGON4_VNUM, SHADOW_DRAGON4},
      {SHADOW_DRAGON5_VNUM, SHADOW_DRAGON5},
      {SHADOW_DRAGON6_VNUM, SHADOW_DRAGON6},
      {SHADOW_DRAGON7_VNUM, SHADOW_DRAGON7},
  };

  for (const auto &[vnum, room] : shadow_dragons) {
    if (room > 0) {
      struct char_data *mob = read_mobile(vnum, VIRTUAL);
      char_to_room(mob, room_by_id(room));
    }
  }

  save_mud_time(&time_info);
}

static bool _db_planet(room_vnum room) {
  struct room_data *rm = room_by_id(room);

  for (auto flag : {ROOM_EARTH, ROOM_VEGETA, ROOM_FRIGID, ROOM_AETHER,
                    ROOM_NAMEK, ROOM_KONACK, ROOM_YARDRAT}) {
    if (room_flagged(rm, flag)) {
      return true;
    }
  }
  return false;
}

void wishSYS(void) {

  if (SHENRON == TRUE) {
    struct room_data *room = room_by_id(DRAGONR);
    if (SELFISHMETER < 10) {
      switch (DRAGONC) {
      case 300:
        send_to_room(room, "@WThe dragon balls on the ground begin to glow "
                           "yellow in slow pulses.@n\r\n");
        send_to_planet(
            0, ROOM_EARTH,
            "@DThe sky begins to grow dark and cloudy suddenly.@n\r\n");
        DRAGONC -= 1;
        break;
      case 295:
        send_to_room(room, "@WSuddenly lightning shoots into the sky, twisting "
                           "about as a roar can be heard for miles!@n\r\n");
        send_to_planet(0, ROOM_EARTH,
                       "@DThe sky flashes with lightning.@n\r\n");
        DRAGONC -= 1;
        break;
      case 290:
        send_to_room(room,
                     "@WThe lightning takes shape and slowly the Eternal "
                     "Dragon, Shenron, can be made out from the glow!@n\r\n");
        char_from_room(EDRAGON);
        char_to_room(EDRAGON, room_by_id(DRAGONR));
        DRAGONC -= 1;
        break;
      case 285:
        send_to_planet(0, ROOM_EARTH,
                       "@DThe lightning stops suddenly, but the sky remains "
                       "mostly dark.@n\r\n");
        DRAGONC -= 1;
        break;
      case 280:
        send_to_room(room, "@WThe glow around Shenron becomes subdued as the "
                           "Eternal Dragon coils so that his head is looking "
                           "down on the dragon balls!@n\r\n");
        DRAGONC -= 1;
        break;
      case 275:
        send_to_room(room,
                     "@wShenron says, '@CWho summoned me? I will grant you any "
                     "two wishes that are within my power.@w'@n\r\n");
        DRAGONC -= 1;
        break;
      case 180:
        send_to_room(room, "@wShenron says, '@CMake your wish already, you "
                           "only have 3 minutes remaining.@w'@n\r\n");
        DRAGONC -= 1;
        break;
      case 120:
        send_to_room(room, "@wShenron says, '@CMake your wish. I am losing "
                           "patience, you only have 2 minutes left.@w'@n\r\n");
        DRAGONC -= 1;
        break;
      case 60:
        send_to_room(room, "@wShenron says, '@CMake your wish now! You only "
                           "have 1 minute left.@w'@n\r\n");
        DRAGONC -= 1;
        break;
      case 0:
        send_to_room(room,
                     "Shenron growls and disappears with a blinding flash that "
                     "is absorbed into the dragon balls. The glowing dragon "
                     "balls then float high into the sky, splitting into "
                     "several directions and streaking across the sky!@n\r\n");
        send_to_planet(0, ROOM_EARTH,
                       "@DThe sky grows brighter again as the clouds disappear "
                       "magicly.@n\r\n");
        extract_char(EDRAGON);
        SHENRON = FALSE;
        DRAGONC -= 1;
        save_mud_time(&time_info);
        break;
      default:
        DRAGONC -= 1;
        break;
      }
      if (WISH[0] == 1 && WISH[1] == 1) {
        DRAGONC = 0;
        WISH[0] = 0;
        WISH[1] = 0;
      }
    } else {
      send_to_room(room,
                   "@RThe dragon balls suddenly begin to crack and darkness "
                   "begins to pour out through the cracks! Shenron begins to "
                   "turn pitch black slowly as the darkness escapes. Suddenly "
                   "Shenron explodes out into the distance in seven parts. "
                   "Each part taking a dragon ball with it!@n\r\n");
      int num = rand_number(200, 20000), done = FALSE, place = 1;
      DRAGONC = 0;
      WISH[0] = 0;
      WISH[1] = 0;
      while (done == FALSE) {
        switch (place) {
        case 1:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON1 = num;
              place = 2;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        case 2:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON2 = num;
              place = 3;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        case 3:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON3 = num;
              place = 4;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        case 4:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON4 = num;
              place = 5;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        case 5:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON5 = num;
              place = 6;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        case 6:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON6 = num;
              place = 7;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        case 7:
          if (room_by_id(num)) {
            if (_db_planet(num)) {
              SHADOW_DRAGON7 = num;
              done = TRUE;
              num = rand_number(200, 20000);
            } else {
              num = rand_number(200, 20000);
            }
          } else {
            num = rand_number(200, 20000);
          }
          break;
        } /* End switch */
        save_mud_time(&time_info);
      } /* End while */

      const std::pair<mob_vnum, room_vnum> shadow_dragons[] = {
          {SHADOW_DRAGON1_VNUM, SHADOW_DRAGON1},
          {SHADOW_DRAGON2_VNUM, SHADOW_DRAGON2},
          {SHADOW_DRAGON3_VNUM, SHADOW_DRAGON3},
          {SHADOW_DRAGON4_VNUM, SHADOW_DRAGON4},
          {SHADOW_DRAGON5_VNUM, SHADOW_DRAGON5},
          {SHADOW_DRAGON6_VNUM, SHADOW_DRAGON6},
          {SHADOW_DRAGON7_VNUM, SHADOW_DRAGON7},
      };

      for (const auto &[vnum, room] : shadow_dragons) {
        if (room > 0) {
          struct char_data *mob = read_mobile(vnum, VIRTUAL);
          char_to_room(mob, room_by_id(room));
        }
      }

      extract_char(EDRAGON);
      SHENRON = FALSE;
      DRAGONC = 0;
    } /* End else */
  }
}

ACMD(do_summon) {

  int summoned = FALSE, count = 0;
  int dball[7] = {20, 21, 22, 23, 24, 25, 26};
  int dball2[7] = {20, 21, 22, 23, 24, 25, 26};
  struct char_data *mob = NULL;
  struct mob_proto_data *proto = NULL;

  struct room_data *room = char_room_get(ch);

  if (!room_flagged(room, ROOM_EARTH)) {
    send_to_char(
        ch, "@wYou can not summon Shenron when you are not on earth.@n\r\n");
    return;
  } // are they on earth?

  if (room_flagged(room, ROOM_NOINSTANT) || room_flagged(room, ROOM_PEACEFUL)) {
    send_to_char(ch, "You can not summon shenron in this protected area!\r\n");
    return;
  }
  if (room_sector_type_get(room) == SECT_INSIDE) {
    send_to_char(ch, "Go outside to summon Shenron!\r\n");
    return;
  }

  char_inventory_iterate(ch, [&](auto obj) {
    if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
      return true;
    }
    if (GET_OBJ_VNUM(obj) == dball[0]) {
      dball[0] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[1]) {
      dball[1] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[2]) {
      dball[2] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[3]) {
      dball[3] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[4]) {
      dball[4] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[5]) {
      dball[5] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[6]) {
      dball[6] = -1;
      count++;
      return true;
    } else {
      return true;
    }
  });

  if (count == 7) {
    summoned = TRUE;
  }

  if (summoned == TRUE) {
    reveal_hiding(ch, 0);
    act("@WYou place the dragon balls on the ground and with both hands "
        "outstretched towards them you say '@CArise Eternal Dragon "
        "Shenron!@W'@n",
        TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n places the dragon balls on the ground and with both hands "
        "outstretched towards them $e says '@CArise Eternal Dragon "
        "Shenron!@W'@n",
        TRUE, ch, 0, 0, TO_ROOM);
    SHENRON = TRUE;
    DRAGONC = 300;
    DRAGONR = char_room_vnum_get(ch);
    if (!room_by_id(DRAGONR)) {
      DRAGONR = char_room_vnum_get(ch);
    }
    send_to_imm("Shenron summoned to room: %d\r\n", DRAGONR);

    DRAGONZ = virtual_zone_by_thing(DRAGONR);

    char_inventory_iterate(ch, [&](auto obj) {
      if (GET_OBJ_VNUM(obj) == dball2[0]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[0] = -1;
        return true;
      } else if (GET_OBJ_VNUM(obj) == dball2[1]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[1] = -1;
        return true;
      } else if (GET_OBJ_VNUM(obj) == dball2[2]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[2] = -1;
        return true;
      } else if (GET_OBJ_VNUM(obj) == dball2[3]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[3] = -1;
        return true;
      } else if (GET_OBJ_VNUM(obj) == dball2[4]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[4] = -1;
        return true;
      } else if (GET_OBJ_VNUM(obj) == dball2[5]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[5] = -1;
        return true;
      } else if (GET_OBJ_VNUM(obj) == dball2[6]) {
        obj_from_char(obj);
        extract_obj(obj);
        dball2[6] = -1;
        return true;
      } else {
        return true;
      }
    });
    if (!(proto = mob_proto_by_id(21))) {
      send_to_imm("Shenron doesn't exist!");
      return;
    }
    mob = read_mobile(21, VIRTUAL);
    char_to_room(mob, 0);
    EDRAGON = mob;
    return;
  } else {
    send_to_char(ch, "@wYou do not have all the dragon balls and can not "
                     "summon the dragon!@n\r\n");
    return;
  }
}

static int has_scanner(struct char_data *ch) {
  int success = 0;

  char_inventory_iterate(ch, [&](auto obj) {
    if (GET_OBJ_VNUM(obj) == 13600) {
      success = 1;
      return false;
    }
    return true;
  });

  return (success);
}

ACMD(do_snet) {
  int channel = 0, global = FALSE, call = -1, reached = FALSE;
  struct descriptor_data *i;
  char voice[150], arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char hist[MAX_INPUT_LENGTH];

  half_chop(argument, arg, arg2);

  struct obj_data *obj = NULL;
  struct obj_data *obj2 = NULL;

  auto room = char_room_get(ch);

  if ((room_flagged(room, ROOM_HBTC))) {
    send_to_char(ch, "This is a different dimension!\r\n");
    return;
  }
  if (IN_ARENA(ch)) {
    send_to_char(ch, "Lol, no.\r\n");
    return;
  }
  if ((room_flagged(room, ROOM_PAST))) {
    send_to_char(ch, "This is the past, you can't talk on scouter net!\r\n");
    return;
  }
  if ((room_flagged(room, ROOM_HELL))) {
    send_to_char(ch, "The fire eats your transmission!\r\n");
    return;
  }
  auto rv = room_vnum_get(room);
  if (rv >= 19800 && rv <= 19899) {
    send_to_char(ch, "Your signal will not be able to escape the walls of the "
                     "pocket dimension.\r\n");
    return;
  }

  if (!IS_NPC(ch)) {
    if (GET_EQ(ch, WEAR_EYE)) {
      obj = GET_EQ(ch, WEAR_EYE);
    } else {
      send_to_char(ch, "You do not have a scouter on.\r\n");
      return;
    }
  }

  if (!*arg) {
    send_to_char(ch, "[Syntax] snet < [1-999] | check | #(scouter number) | * "
                     "message | message>\r\n");
    return;
  }

  if (strstr(arg, "#")) {
    search_replace(arg, "#", "");
    call = atoi(arg);
    if (call <= -1) {
      send_to_char(ch, "Call what personal scouter number?\r\n");
      return;
    }
  }

  if (!strcasecmp(arg, "check")) {
    send_to_char(ch, "Your personal scouter number is: %d\r\n", GET_ID(ch));
    return;
  }

  if (call <= -1) {
    channel = atoi(arg);
  }

  if (channel > 0) {
    SFREQ(obj) = channel;
    if (channel > 999) {
      SFREQ(obj) = 999;
    }
    act("@wYou push some buttons on $p@w and change its channel.", TRUE, ch,
        obj, 0, TO_CHAR);
    act("@C$n@w pushes some buttons on $p@w and changes its channel.", TRUE, ch,
        obj, 0, TO_ROOM);
    return;
  }

  if (GET_BONUS(ch, BONUS_MUTE) > 0) {
    send_to_char(ch, "You are unable to speak though.\r\n");
    return;
  }

  // All checks passed, we'll be sending.

  if (SFREQ(obj) == 0) {
    SFREQ(obj) = 1;
  }
  if (!strcasecmp(arg, "*") && call <= -1) {
    global = TRUE;
  }
  if (GET_VOICE(ch) != NULL) {
    sprintf(voice, "%s", GET_VOICE(ch));
  }
  if (GET_VOICE(ch) == NULL) {
    sprintf(voice, "A generic voice");
  }

  // Distribute the message.
  for (i = descriptor_list; i; i = i->next) {
    if (!i->character) {
      continue;
    }

    if (i->character == ch) {
      continue;
    }

    // It will always be heard and logged for admin.
    if (GET_ADMLEVEL(i->character) > 0) {
      *hist = '\0';
      if (call <= -1 && global == FALSE) {
        send_to_char(
            i->character,
            "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n",
            voice, GET_NAME(ch), SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
        sprintf(hist,
                "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n",
                voice, GET_NAME(ch), SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
      } else if (call <= -1) {
        send_to_char(i->character,
                     "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d "
                     "@mBroadcast@D] @G%s@n\r\n",
                     voice, GET_NAME(ch), SFREQ(obj), !*arg2 ? "" : CAP(arg2));
        sprintf(hist,
                "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] "
                "@G%s@n\r\n",
                voice, GET_NAME(ch), SFREQ(obj), !*arg2 ? "" : CAP(arg2));
      } else {
        send_to_char(i->character,
                     "@C%s (%s) is heard, @D[@WCall to @R#@Y%d@D] @G%s@n\r\n",
                     voice, GET_NAME(ch), call, !*arg2 ? "" : CAP(arg2));
        sprintf(hist, "@C%s (%s) is heard, @D[@WCall to @R#@Y%d@D] @G%s@n\r\n",
                voice, GET_NAME(ch), call, !*arg2 ? "" : CAP(arg2));
      }
      add_history(i->character, hist, HIST_SNET);
      continue;
    }

    if (STATE(i) != CON_PLAYING) {
      continue;
    }

    auto recp_room = char_room_get(i->character);

    if (recp_room == room) {
      continue;
    }
    if ((recp_room &&
         room_flagged(recp_room, ROOM_HBTC))) {
      continue;
    }
    if ((recp_room &&
         room_flagged(recp_room, ROOM_PAST))) {
      continue;
    }
    if (((recp_room &&
          room_flagged(recp_room, ROOM_RHELL)) &&
         !(room && room_flagged(room, ROOM_RHELL))) ||
        ((recp_room &&
          room_flagged(recp_room, ROOM_AL)) &&
         !(room && room_flagged(room, ROOM_AL)))) {
      continue;
    }
    if ((!(recp_room &&
           room_flagged(recp_room, ROOM_RHELL)) &&
         (room && room_flagged(room, ROOM_RHELL))) ||
        (!(recp_room &&
           room_flagged(recp_room, ROOM_AL)) &&
         (room && room_flagged(room, ROOM_AL)))) {
      continue;
    }
    if (GET_POS(i->character) == POS_SLEEPING) {
      continue;
    }
    if (in_room_range(i->character, 19800, 19899)) {
      continue;
    }

    if (GET_EQ(i->character, WEAR_EYE)) {
      obj2 = GET_EQ(i->character, WEAR_EYE);
      if (SFREQ(obj2) == 0) {
        SFREQ(obj2) = 1;
      }
      if (global == FALSE && call <= -1 && SFREQ(obj2) == SFREQ(obj) &&
          GET_ADMLEVEL(i->character) < 1) {
        send_to_char(
            i->character,
            "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n",
            voice,
            readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch)
                                             : "Unknown",
            SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
        *hist = '\0';
        sprintf(
            hist,
            "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n",
            voice,
            readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch)
                                             : "Unknown",
            SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
        add_history(i->character, hist, HIST_SNET);
        if (has_scanner(i->character)) {
          char *blah = sense_location(ch);
          send_to_char(i->character, "@WScanner@D: @Y%s@n\r\n", blah);
          free(blah);
        }
      } /* It is the right freq */
      else if (global == TRUE && call <= -1 && GET_ADMLEVEL(i->character) < 1) {
        send_to_char(i->character,
                     "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d "
                     "@mBroadcast@D] @G%s@n\r\n",
                     voice,
                     readIntro(i->character, ch) == 1
                         ? get_i_name(i->character, ch)
                         : "Unknown",
                     SFREQ(obj), CAP(arg2));
        *hist = '\0';
        sprintf(hist,
                "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d "
                "@mBroadcast@D] @G%s@n\r\n",
                voice,
                readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch)
                                                 : "Unknown",
                SFREQ(obj), CAP(arg2));
        add_history(i->character, hist, HIST_SNET);
        if (has_scanner(i->character)) {
          char *blah = sense_location(ch);
          send_to_char(i->character, "@WScanner@D: @Y%s@n\r\n", blah);
          free(blah);
        }
      } else if (call > -1 && GET_ID(i->character) == call) {
        send_to_char(
            i->character,
            "@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n",
            voice,
            readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch)
                                             : "Unknown",
            GET_ID(ch), !*arg2 ? "" : CAP(arg2));
        *hist = '\0';
        sprintf(
            hist,
            "@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n",
            voice,
            readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch)
                                             : "Unknown",
            GET_ID(ch), !*arg2 ? "" : CAP(arg2));
        add_history(i->character, hist, HIST_SNET);
        if (has_scanner(i->character)) {
          char *blah = sense_location(ch);
          send_to_char(i->character, "@WScanner@D: @Y%s@n\r\n", blah);
          free(blah);
        }
        reached = TRUE;
      }
    } /* They have a scouter */
  } /* End for loop */

  if (call <= -1) {
    if (!global) {
      reveal_hiding(ch, 3);
      send_to_char(ch, "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n",
                   SFREQ(obj), arg, !*arg2 ? "" : arg2);
      *hist = '\0';
      sprintf(hist, "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", SFREQ(obj),
              arg, !*arg2 ? "" : arg2);
      add_history(ch, hist, HIST_SNET);
      char over[MAX_STRING_LENGTH];
      sprintf(over, "@C$n@W says into $s scouter, '@G@G%s %s@W'@n\r\n",
              CAP(arg), !*arg2 ? "" : arg2);
      act(over, TRUE, ch, 0, 0, TO_ROOM);
      if ((room && room_flagged(room, ROOM_RHELL)) ||
          (room && room_flagged(room, ROOM_AL))) {
        send_to_char(ch, "@mThe transmission only reaches those who are in the "
                         "afterlife.@n\r\n");
      }
    } else {
      reveal_hiding(ch, 3);
      send_to_char(ch, "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n",
                   SFREQ(obj), !*arg2 ? "" : CAP(arg2));
      *hist = '\0';
      sprintf(hist, "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n",
              SFREQ(obj), !*arg2 ? "" : CAP(arg2));
      add_history(ch, hist, HIST_SNET);
      char over[MAX_STRING_LENGTH];
      sprintf(over, "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n",
              !*arg2 ? "" : CAP(arg2));
      act(over, TRUE, ch, 0, 0, TO_ROOM);
      if ((room && room_flagged(room, ROOM_RHELL)) ||
          (room && room_flagged(room, ROOM_AL))) {
        send_to_char(ch, "@mThe transmission only reaches those who are in the "
                         "afterlife.@n\r\n");
      }
    }
  } else {
    reveal_hiding(ch, 3);
    send_to_char(ch, "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call,
                 !*arg2 ? "" : CAP(arg2));
    *hist = '\0';
    sprintf(hist, "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call,
            !*arg2 ? "" : CAP(arg2));
    add_history(ch, hist, HIST_SNET);
    char over[MAX_STRING_LENGTH];
    sprintf(over, "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n",
            !*arg2 ? "" : CAP(arg2));
    act(over, TRUE, ch, 0, 0, TO_ROOM);
    if (reached == FALSE) {
      send_to_char(ch, "@mThe transmission didn't reach them.@n\r\n");
    }
  }
} /*end snet command */

int dball_count(struct char_data *ch) {

  int dball[7] = {20, 21, 22, 23, 24, 25, 26};
  int count = 0;

  char_inventory_iterate(ch, [&](auto obj) {
    if (GET_OBJ_VNUM(obj) == dball[0]) {
      dball[0] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[1]) {
      dball[1] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[2]) {
      dball[2] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[3]) {
      dball[3] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[4]) {
      dball[4] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[5]) {
      dball[5] = -1;
      count++;
      return true;
    } else if (GET_OBJ_VNUM(obj) == dball[6]) {
      dball[6] = -1;
      count++;
      return true;
    } else {
      return true;
    }
  });

  if (count >= 1) {
    return 1;
  } else {
    return 0;
  }
}

ACMD(do_quit) {
  if (IS_NPC(ch) || !ch->desc)
    return;

  auto room = char_room_get(ch);

  if ((room && room_flagged(room, ROOM_PAST))) {
    send_to_char(ch, "This is the past, you can't quit here!\r\n");
    return;
  }
  auto rv = room_vnum_get(room);
  if (rv >= 2002 && rv <= 2011) {
    send_to_char(ch, "You can't quit in the arena!\r\n");
    return;
  }
  if (rv >= 101 && rv <= 139) {
    send_to_char(ch, "You can't quit in the mud school!\r\n");
    return;
  }
  if (rv >= 19800 && rv <= 19899) {
    send_to_char(ch, "You can't quit in a pocket dimension!\r\n");
    return;
  }
  if (rv == 2069) {
    send_to_char(ch, "You can't quit here!\r\n");
    return;
  }
  if (MINDLINK(ch) && LINKER(ch) == 0) {
    send_to_char(ch, "@RYou feel like the mind that is linked with yours is "
                     "preventing you from quiting!@n\r\n");
    if (char_room_get(MINDLINK(ch)) != NULL) {
      look_at_room(char_room_get(MINDLINK(ch)), ch, 0);
      send_to_char(ch, "You get an impression of where this interference is "
                       "originating from.\r\n");
    }
    return;
  }
  if (rv == 2070) {
    send_to_char(ch, "You can't quit here!\r\n");
    return;
  }
  if (dball_count(ch)) {
    send_to_char(ch, "You can not quit while you have dragon balls! Place them "
                     "somewhere first.");
    return;
  }

  if (subcmd != SCMD_QUIT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s has quit the game.", GET_NAME(ch));
    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");
    if (char_follower_count(ch) || MASTER(ch))
      die_follower(ch);
    if (ch == ch_selling)
      stop_auction(AUC_QUIT_CANCEL, NULL);

    /*  We used to check here for duping attempts, but we may as well
     *  do it right in extract_char(), since there is no check if a
     *  player rents out and it can leave them in an equally screwy
     *  situation.
     */

    /* If someone is quitting in their house, let them load back here. */
    if (!(room && room_flagged(room, ROOM_PAST)) &&
        (rv < 19800 || rv > 19899)) {
      if (rv != NOWHERE && rv != 0 &&
          rv != 1) {
        GET_LOADROOM(ch) = rv;
      }
    }
    if ((room && room_flagged(room, ROOM_PAST))) {
      if (rv != NOWHERE && rv != 0 &&
          rv != 1) {
        GET_LOADROOM(ch) = room_by_id(1561) ? 1561 : NOTHING;
      }
    }

    Crash_rentsave(ch, 0);

    extract_char(ch); /* Char is saved before extracting. */
  }
  /* Remove any snoopers */
  if (ch->desc->snoop_by) {
    write_to_output(ch->desc->snoop_by,
                    "Your victim is no longer among us.\r\n");
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

ACMD(do_save) {
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (CONFIG_AUTO_SAVE && GET_ADMLEVEL(ch) < 1) {
      send_to_char(ch, "Saving.\r\n");
      write_aliases(ch);
      save_char(ch);
      Crash_crashsave(ch);
      if (char_room_vnum_get(ch) < 19800 || char_room_vnum_get(ch) > 19899) {
        if (char_room_vnum_get(ch) != NOWHERE && char_room_vnum_get(ch) != 0 &&
            char_room_vnum_get(ch) != 1) {
          GET_LOADROOM(ch) = char_room_vnum_get(ch);
        }
      }
      return;
    }
    send_to_char(ch, "Saving.\r\n");
  }

  write_aliases(ch);
  if (char_room_vnum_get(ch) < 19800 || char_room_vnum_get(ch) > 19899) {
    if (char_room_vnum_get(ch) != NOWHERE && char_room_vnum_get(ch) != 0 &&
        char_room_vnum_get(ch) != 1) {
      GET_LOADROOM(ch) = char_room_vnum_get(ch);
    }
  }
  save_char(ch);
  Crash_crashsave(ch);
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) {
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_practice) {
  char arg[200];

  /* if (IS_NPC(ch))
    return; */

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills with your trainer.\r\n");
  else
    send_to_char(ch,
                 "Use the skills command unless you are at your trainer.\r\n");
  /*list_skills(ch);*/
}

ACMD(do_skills) {
  char arg[1000];
  one_argument(argument, arg);
  if (IS_NPC(ch)) {
    return;
  }
  list_skills(ch, arg);
}

ACMD(do_visible) {
  int appeared = 0;

  if (GET_ADMLEVEL(ch)) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED (ch, AFF_INVISIBLE) {
    appear(ch);
    appeared = 1;
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  }

  if (!appeared)
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title) {
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(
        ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n",
                 MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s %s.\r\n", GET_NAME(ch),
                 GET_TITLE(ch));
  }
}

ACMD(do_use) {
  char buf[100], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item = NULL;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  if (!mag_item) {
    switch (subcmd) {
    case SCMD_RECITE:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
      }
      break;
    case SCMD_USE:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch)))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
      }
      break;
    default:
      mud_log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /*  SYSERR_DESC:
       *  This is the same as the unhandled case in do_gen_ps(), but in the
       *  function which handles 'quaff', 'recite', and 'use'.
       */
      return;
    }
  }
  int refreshed;
  switch (subcmd) {
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if (IS_ANDROID(ch)) {
      send_to_char(ch,
                   "You are not biological enough to use these, Tincan.\r\n");
      return;
    } else {
      switch (GET_OBJ_VNUM(mag_item)) {
      case 381:
        if ((getCurST(ch)) >= GET_MAX_MOVE(ch)) {
          send_to_char(ch, "Your stamina is full.\r\n");
          return;
        }
        act("@WYou place the $p@W against your chest and feel a rush of "
            "stamina as it automatically administers the dose.@n",
            TRUE, ch, mag_item, 0, TO_CHAR);
        act("@C$n@W places an $p@W against $s chest and a loud click is "
            "heard.@n",
            TRUE, ch, mag_item, 0, TO_ROOM);
        if (GET_SKILL(ch, SKILL_FIRST_AID) > 0) {
          send_to_char(
              ch, "@CYour skill in First Aid has helped increase the use of "
                  "the injector. You gain more stamina as a result.@n\r\n");
          incCurST(ch, getMaxST(ch) * .25);
        } else {
          incCurST(ch, getMaxST(ch) * .1);
        }
        extract_obj(mag_item);
        return;
      case 382:
        if (char_condition_has(ch, "burned")) {
          act("@WYou gently apply the salve to your burns.@n", TRUE, ch,
              mag_item, 0, TO_CHAR);
          act("@C$n@W gently applies a burn salve to $s burns.@n", TRUE, ch,
              mag_item, 0, TO_ROOM);
          char_condition_remove(ch, "burned", "healing_burned");
          extract_obj(mag_item);
        } else {
          send_to_char(ch, "You are not burned.\r\n");
        }
        return;
      case 383:
        if (char_condition_has(ch, "poison")) {
          act("@WYou place the $p@W against your neck and feel a rush of "
              "relief as the antitoxiin enters your bloodstream.@n",
              TRUE, ch, mag_item, 0, TO_CHAR);
          act("@C$n@W places an $p@W against $s neck and a loud click is "
              "heard.@n",
              TRUE, ch, mag_item, 0, TO_ROOM);
          char_condition_remove(ch, "poison", "item_antitoxin");
          extract_obj(mag_item);
        } else {
          send_to_char(ch, "You are not poisoned.\r\n");
        }
        return;
      case 385:
        act("@WYou drink the contents of the vial before disposing of it.@n",
            TRUE, ch, mag_item, 0, TO_CHAR);
        act("@C$n@W dinks a $p and then disposes of it.@n", TRUE, ch, mag_item,
            0, TO_ROOM);
        if (AFF_FLAGGED(ch, AFF_BLIND)) {
          act("@WYour eyesight has returned!@n", TRUE, ch, mag_item, 0,
              TO_CHAR);
          act("@C$n@W eyesight seems to have returned.@n", TRUE, ch, mag_item,
              0, TO_ROOM);
          char_condition_remove(ch, "blind", "item_vial");
        }
        refreshed = FALSE;

        if (GET_HIT(ch) <= getMaxPL(ch) * 0.99) {
          incCurHealth(ch,
                       large_rand(getMaxPL(ch) * 0.08, getMaxPL(ch) * 0.16));
          refreshed = TRUE;
        } else if ((getCurKI(ch)) <= getMaxPL(ch) * 0.99) {
          incCurKI(
              ch, large_rand(GET_MAX_MANA(ch) * 0.08, GET_MAX_MANA(ch) * 0.16));
          refreshed = TRUE;
        } else if ((getCurST(ch)) <= GET_MAX_MOVE(ch) * 0.99) {
          incCurST(
              ch, large_rand(GET_MAX_MOVE(ch) * 0.08, GET_MAX_MOVE(ch) * 0.16));
          refreshed = TRUE;
        }
        if (refreshed == TRUE) {
          send_to_char(ch, "@CYou feel refreshed!\r\n");
        }
        extract_obj(mag_item);
        return;
      default:
        send_to_char(ch, "That is not something you can apparently use.\r\n");
        return;
      }
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_gen_write) {
  FILE *fl;
  char *tmp;
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "That must be a mistake...\r\n");
    return;
  }
  send_to_imm("[A new %s has been filed by: %s]\r\n", CMD_NAME, GET_NAME(ch));

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
     *  This is from do_gen_write() and indicates that it cannot call the
     *  stat() system call on the file required.  The error string at the
     *  end of the line should explain what the problem is.
     */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
    send_to_char(ch,
                 "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    /*  SYSERR_DESC:
     *  This is from do_gen_write(), and will be output if the file in
     *  question cannot be opened for appending to.  The error string
     *  at the end of the line should explain what the problem is.
     */

    send_to_char(ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  fprintf(fl,
          "@D[@WUser: @c%-10s@D] [@WChar: @C%-10s@D] [@WRoom: @G%-4d@D] "
          "[@WDate: @Y%6.6s@D]@b \n-----------@w\n%s\n",
          GET_USER(ch) ? GET_USER(ch) : "ERR", GET_NAME(ch),
          char_room_vnum_get(ch), (tmp + 4), argument);
  fprintf(fl, "@D-------------------------------@n\n");
  fclose(fl);
  send_to_char(ch, "Okay.  Thanks!\r\n");
}

ACMD(do_file) {
  FILE *req_file;
  int cur_line = 0, num_lines = 0, req_lines = 0, i, j;
  int l;
  char field[100], value[100], line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
  } fields[] = {{"none", 6, "Does Nothing"},
                {"bug", ADMLVL_IMMORT, "../data/misc/bugs"},
                {"typo", ADMLVL_IMMORT, "../data/misc/typos"},
                {"report", ADMLVL_IMMORT, "../data/misc/ideas"},
                {"xnames", 4, "../data/misc/xnames"},
                {"levels", 4, "../log/levels"},
                {"rip", 4, "../log/rip"},
                {"players", 4, "../log/newplayers"},
                {"rentgone", 4, "../log/rentgone"},
                {"errors", 4, "../log/errors"},
                {"godcmds", 4, "../log/godcmds"},
                {"syslog", ADMLVL_IMMORT, "../syslog"},
                {"crash", ADMLVL_IMMORT, "../syslog.CRASH"},
                {"immlog", ADMLVL_IMMORT, "../data/misc/request"},
                {"customs", ADMLVL_IMMORT, "../data/misc/customs"},
                {"todo", 5, "../todo"},
                {"\n", 0, "\n"}};

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
        sprintf(buf + strlen(buf), "%-15s%s\r\n", fields[i].cmd,
                fields[i].file);
    send_to_char(ch, "%s", buf);
    return;
  }

  two_arguments(argument, field, value);

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (*(fields[l].cmd) == '\n') {
    send_to_char(ch, "That is not a valid option!\r\n");
    return;
  }

  if (GET_ADMLEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough to view that file!\r\n");
    return;
  }

  if (!strcasecmp(field, "request")) {
    GET_BOARD(ch, 2) = time(0);
  }

  if (!*value)
    req_lines = 15; /* default is the last 15 lines */
  else
    req_lines = atoi(value);

  if (!(req_file = fopen(fields[l].file, "r"))) {
    mudlog(BRF, ADMLVL_IMPL, TRUE,
           "SYSERR: Error opening file %s using 'file' command.",
           fields[l].file);
    return;
  }

  get_line(req_file, line);
  while (!feof(req_file)) {
    num_lines++;
    get_line(req_file, line);
  }
  rewind(req_file);

  req_lines = MIN(MIN(req_lines, num_lines), 5000);

  buf[0] = '\0';

  get_line(req_file, line);
  while (!feof(req_file)) {
    cur_line++;
    if (cur_line > (num_lines - req_lines))
      sprintf(buf + strlen(buf), "%s\r\n", line);

    get_line(req_file, line);
  }
  fclose(req_file);

  send_to_char(ch, "%s", buf);
}



void char_send_to_imm(const char *msg) { send_to_imm("%s", msg); }
void char_log_imm_action(const char *msg) { log_imm_action("%s", msg); }
