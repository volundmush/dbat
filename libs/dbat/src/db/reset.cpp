/************************************************************************* 
*  File: reset.c                             Designed for CircleMUD 3.5  * 
*  Usage: implementation of pre-zone resets and post-zone resets         * 
*         functions called before and after a normal zone reset          * 
*                                                                        * 
*  All rights reserved.                                                  * 
*                                                                        * 
*  Copyright (C) 2007 Stefan Cole (a.k.a. Jamdog)                        * 
*  To see this in action, check out AderonMUD                            * 
************************************************************************ */

#include "dbat/utils.hpp"
#include "dbat/reset.hpp"
#include "dbat/spec_procs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/comm.hpp"

static bool prereset_gauntlet_zone(GameWorld *g);

/* pre_reset is called before a zone is reset - returns TRUE to prevent a normal reset of the zone */
bool pre_reset(GameWorld *g, zone_vnum znum)
{
  /* By default, a normal zone reset follows this function */
  bool ret_value = PERFORM_NORMAL_RESET;

  switch (znum) {
    /* Gauntlet zone reset type determined by players in the zone */
    case RESET_GAUNTLET:
      ret_value = prereset_gauntlet_zone(g);
      break;

    default: /* No special zone reset to perform for this zone number */
      ret_value = PERFORM_NORMAL_RESET;
      break;
  }
  return ret_value;
}

/* post_reset is called after a normal zone reset */
void post_reset(zone_vnum znum)
{
  switch (znum) {
    default:
      break;
  }
}

static bool prereset_gauntlet_zone(GameWorld *g)
{
  int i, gauntlet_players = 0;
  struct char_data *mob;
  /* The mobs in the gauntlet are ALL reset here... */
  struct gauntlet_mob gauntlet_mobs[NUM_GAUNTLET_ROOMS] = {
      /* room vnum   mob vnum */
      {2403, 2400},
      {2405, 2401},
      {2407, 2402},
      {2409, 2403},
      {2411, 2404},
      {2413, 2405},
      {2415, 2406},
      {2417, 2407},
      {2419, 2408},
      {2421, 2409},
      {2423, 2410},
      {2425, 2411},
      {2427, 2412},
      {2429, 2413},
      {2431, 2414},
      {2433, 2415},
      {2435, 2416},
      {2437, 2417},
      {2439, 2418},
      {2441, 2419}
  };

  mud_log("Special Reset: zone {:d}: Resetting Gauntlet", RESET_GAUNTLET);

  /* Count the number of players currently in the gauntlet */
  for (i = 0; i < NUM_GAUNTLET_ROOMS; i++) {
    gauntlet_players += num_players_in_room(g, gauntlet_mobs[i].vroom);
  }

  /* No players in the gauntlet - normal reset will do */
  if (gauntlet_players == 0) {
    mud_log("Special Reset: zone {:d}: No players in Gauntlet - executing normal reset", RESET_GAUNTLET);
    return PERFORM_NORMAL_RESET;
  }

  mud_log("Special Reset: zone {:d}: {:d} players in Gauntlet - special reset only", RESET_GAUNTLET, gauntlet_players);

  for (i = 0; i < NUM_GAUNTLET_ROOMS; i++) {
    if (check_mob_in_room(g, gauntlet_mobs[i].vmob, gauntlet_mobs[i].vroom) == false) {
      /* Mob isn't in the room, can we reset it? */
      if (num_players_in_room(g, gauntlet_mobs[i].vroom) == 0) {
        /* Yep - no players, reset this room only */
        if ((gauntlet_mobs[i].vmob) && (gauntlet_mobs[i].vroom)) {
          /* Load the correct mob */
          if ((mob = read_mobile(g, gauntlet_mobs[i].vmob)) != nullptr) {
            /* And put it in the room */
            char_to_room(g, mob, gauntlet_mobs[i].vroom);
            mud_log("Special Reset: zone {:d}: Gauntlet mob reset ({:d}, {})", RESET_GAUNTLET, gauntlet_mobs[i].vmob, char_get_name(mob));
          }
        }
      }
    }
  }
  return BLOCK_NORMAL_RESET;
}

#define ZCMD2 zone->cmd[cmd_no]

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

void log_zone_error(GameWorld *g, struct zone_data *zone, int cmd_no, std::string_view message)
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
                room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info.reset(exit_flag::locked);
                room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info.reset(exit_flag::closed);
                break;
              case 1:
                room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info.set(exit_flag::closed);
                room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info.reset(exit_flag::locked);
                break;
              case 2:
                room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info.set(exit_flag::locked);
                room_get_dir_option(g, ZCMD2.arg1, ZCMD2.arg2)->exit_info.set(exit_flag::closed);
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
        if (room_flagged(g, rrnum, room_flag::aura) && rand_number(g, 1, 5) >= 4) {
          send_to_room(g, rrnum, "The aura of regeneration covering the surrounding area disappears.\r\n");
          room_set_flag(g, rrnum, room_flag::aura, false);
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

    if ((char_get_admlevel(i->character) >= ADMLVL_IMMORT) && (char_prf_flagged(g, i->character, pref_flag::nohassle)))
      continue;

    return false;
  }

  return true;
}
