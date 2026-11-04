/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/mobact.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/spells.h"
#include "dbat/shop.h"
#include "dbat/combat.h"
#include "dbat/act.offensive.h"
#include "dbat/act.movement.h"
#include "dbat/act.other.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/spec_procs.h"
#include "dbat/class.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static void mob_taunt(GameWorld *g, struct char_data *ch);

/* local functions */
static bool player_present(GameWorld *g, struct char_data *ch);

static bool player_present(GameWorld *g, struct char_data *ch)
{

  struct char_data *vict, *next_v;
  int found = false;

  if (char_get_in_room(ch) == NOWHERE)
    return false;

  for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = next_v) {
    next_v = char_get_next_in_room(vict);
    if (!char_is_npc(vict)) {
      found = true;
    }
  }

  return (found);
}

void mobile_activity(GameWorld *g)
{
  struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int door, found, max;
  memory_rec *names;

  for (ch = g->characters; ch; ch = next_ch) {
    next_ch = char_get_next(ch);

    if (!char_is_mob(g, ch))
      continue;

    /* Examine call for special procedure */
    if (char_mob_flagged(ch, MOB_SPEC) && !g->no_specials) {
      if (g->mob_index[char_get_nr(ch)].func == nullptr) {
        mud_log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
                char_get_name(ch), char_get_mob_vnum(g, ch));
        char_mob_set(ch, MOB_SPEC, false);
      } else {
        char actbuf[MAX_INPUT_LENGTH] = "";
        if ((g->mob_index[char_get_nr(ch)].func)(g, ch, ch, 0, actbuf))
          continue; /* go to next char */
      }
    }

    /* If the mob has no specproc, do the default actions */
    if (!char_is_awake(ch))
      continue;

    /* Scavenger (picking up objects) */
    if (char_is_humanoid(ch) && !char_get_fighting(ch) && char_is_awake(ch) && !char_mob_flagged(ch, MOB_NOSCAVENGER) && !char_mob_flagged(ch, MOB_NOKILL) && (!player_present(g, ch) || axion_dice(g, 0) > 118))
      if (rroom_get_contents(g, char_get_in_room(ch)) && rand_number(g, 1, 100) >= 95) {
        max      = 1;
        best_obj = nullptr;
        for (obj = rroom_get_contents(g, char_get_in_room(ch)); obj; obj = obj_get_next_content(obj))
          if (char_can_get_obj(g, ch, obj) && obj_get_cost(obj) > max) {
            best_obj = obj;
            max      = obj_get_cost(obj);
          }
        if (best_obj != nullptr && char_can_get_obj(g, ch, best_obj) && obj_get_type_flag(best_obj) != ITEM_BED && !obj_get_posted_to(best_obj) && !obj_extra_flagged(best_obj, ITEM_NOPICKUP)) {
          switch (rand_number(g, 1, 5)) {
            case 1:
              act(g, "$n@W says, '@CFinders keepers, losers weepers.@W'@n", true, ch, 0, 0, TO_ROOM);
              break;
            case 2:
              act(g, "$n@W says, '@CPeople always leaving their garbage JUST LYING AROUND. The nerve....@W'@n", true, ch, 0, 0, TO_ROOM);
              break;
            case 3:
              act(g, "$n@W says, '@CWho would leave this here? Oh well..@W'@n", true, ch, 0, 0, TO_ROOM);
              break;
            case 4:
              act(g, "$n@W says, '@CI always wanted one of these.@W'@n", true, ch, 0, 0, TO_ROOM);
              break;
            case 5:
              act(g, "$n@W looks around quickly to see if anyone is paying attention.@n", true, ch, 0, 0, TO_ROOM);
              break;
          }
          perform_get_from_room(g, ch, best_obj);
        }
      }

    /* Mob Movement */
    if (!char_mob_flagged(ch, MOB_SENTINEL) && (char_get_position(ch) == POS_STANDING) && !char_get_fighting(ch) &&
        (!char_aff_flagged(ch, AFF_TAMED)) && !char_get_absorbby(ch) &&
        ((door = rand_number(g, 0, 18)) < NUM_OF_DIRS) && char_can_go(g, ch, door) &&
        !rroom_flagged(g, char_get_exit(g, ch, door)->to_room, ROOM_NOMOB) &&
        !rroom_flagged(g, char_get_exit(g, ch, door)->to_room, ROOM_DEATH) &&
        (!char_mob_flagged(ch, MOB_STAY_ZONE) ||
         (rroom_get_zone(g, char_get_exit(g, ch, door)->to_room) == rroom_get_zone(g, char_get_in_room(ch))))) {
      if (rand_number(g, 1, 2) == 2 && !char_is_affected(ch, AFF_PARALYZE) && block_calc(g, ch)) {
        perform_move(g, ch, door, 1);
      }
    }

    /* RESPOND TO A HUGE ATTACK */
    struct obj_data *hugeatk = nullptr, *next_huge = nullptr;
    for (hugeatk = rroom_get_contents(g, char_get_in_room(ch)); hugeatk; hugeatk = next_huge) {
      next_huge = obj_get_next_content(hugeatk);
      if (char_get_fighting(ch)) {
        continue;
      }
      if (char_mob_flagged(ch, MOB_NOKILL)) {
        continue;
      }
      if (obj_get_vnum(g, hugeatk) == 82 || obj_get_vnum(g, hugeatk) == 83) {
        if (obj_get_user(hugeatk) != nullptr) {
          act(g, "@W$n@R leaps at @C$N@R desperately!@n", true, ch, 0, obj_get_user(hugeatk), TO_ROOM);
          act(g, "@W$n@R leaps at YOU desperately!@n", true, ch, 0, obj_get_user(hugeatk), TO_VICT);
          if (char_is_humanoid(ch)) {
            char tar[MAX_INPUT_LENGTH];
            sprintf(tar, "%s", char_get_name(obj_get_user(hugeatk)));
            do_punch(g, ch, tar, 0, 0);
          } else {
            char tar[MAX_INPUT_LENGTH];
            sprintf(tar, "%s", char_get_name(obj_get_user(hugeatk)));
            do_bite(g, ch, tar, 0, 0);
          }
        }
      }
    }

    /* Aggressive Mobs */
    if (char_mob_flagged(ch, MOB_AGGRESSIVE) && !char_is_affected(ch, AFF_PARALYZE)) {
      int spot_roll = rand_number(g, 1, char_get_level(ch) + 10);
      found         = false;
      for (vict = rroom_get_people(g, char_get_in_room(ch)); vict && !found; vict = char_get_next_in_room(vict)) {
        if (vict == ch)
          continue;
        else if (char_get_fighting(ch))
          continue;
        else if (!char_can_see(g, ch, vict))
          continue;
        else if (char_is_npc(vict))
          continue;
        else if (char_prf_flagged(g, vict, PRF_NOHASSLE))
          continue;
        else if (char_mob_flagged(ch, MOB_AGGR_EVIL) && char_get_alignment(vict) < 50)
          continue;
        else if (char_mob_flagged(ch, MOB_AGGR_GOOD) && char_get_alignment(vict) > -50)
          continue;
        else if (char_get_level(vict) < 5)
          continue;
        else if (char_aff_flagged(vict, AFF_HIDE) && char_get_skill(vict, SKILL_HIDE) > spot_roll)
          continue;
        else if (char_aff_flagged(vict, AFF_SNEAK) && char_get_skill(vict, SKILL_MOVE_SILENTLY) > spot_roll)
          continue;
        else if (char_get_aggtimer(ch) < 8)
          char_set_aggtimer(ch, char_get_aggtimer(ch) + 1);
        else {
          char_set_aggtimer(ch, 0);
          char tar[MAX_INPUT_LENGTH];

          sprintf(tar, "%s", char_get_name(vict));
          if (char_is_humanoid(ch)) {
            if (!char_aff_flagged(vict, AFF_HIDE) && !char_aff_flagged(vict, AFF_SNEAK)) {
              act(g, "@w'I am going to get you!' @C$n@w shouts at you!@n", true, ch, 0, vict, TO_VICT);
              act(g, "@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", true, ch, 0, vict, TO_NOTVICT);
            } else {
              act(g, "@C$n@w notices YOU.\n@w'I am going to get you!' @C$n@w shouts at you!@n", true, ch, 0, vict, TO_VICT);
              act(g, "@C$n@w notices @c$N@w.\n@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", true, ch, 0, vict, TO_NOTVICT);
            }
            if (char_aff_flagged(vict, AFF_FLYING) && !char_aff_flagged(ch, AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) > 10) {
              do_fly(g, ch, 0, 0, 0);
              continue;
            }
            if (!char_aff_flagged(vict, AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING)) {
              do_fly(g, ch, 0, 0, 0);
              continue;
            }
            do_punch(g, ch, tar, 0, 0);
          }
          if (!char_is_humanoid(ch)) {
            if (char_aff_flagged(vict, AFF_FLYING) && !char_aff_flagged(ch, AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) > 10) {
              do_fly(g, ch, 0, 0, 0);
              continue;
            }
            if (!char_aff_flagged(vict, AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING)) {
              do_fly(g, ch, 0, 0, 0);
              continue;
            }
            if (!char_aff_flagged(vict, AFF_HIDE) && !char_aff_flagged(vict, AFF_SNEAK)) {
              act(g, "@C$n @wgrowls viciously at you!@n", true, ch, 0, vict, TO_VICT);
              act(g, "@C$n @wgrowls viciously at @c$N@w!@n", true, ch, 0, vict, TO_NOTVICT);
            } else {
              act(g, "@C$n@w notices YOU.\n@C$n @wgrowls viciously at you!@n", true, ch, 0, vict, TO_VICT);
              act(g, "@C$n@w notices @c$N@w.\n@C$n @wgrowls viciously at @c$N@w!@n", true, ch, 0, vict, TO_NOTVICT);
            }
            do_bite(g, ch, tar, 0, 0);
          }
          /*hit(ch, vict, TYPE_UNDEFINED);*/
          found = true;
        }
      }
    }

    if (char_get_original(ch) && rand_number(g, 1, 5) >= 4) {
      struct char_data *original = char_get_original(ch);

      if (char_get_fighting(original) && !char_get_fighting(ch)) {
        char target[MAX_INPUT_LENGTH];
        struct char_data *targ = char_get_fighting(original);

        sprintf(target, "%s", char_get_alias(targ));
        if (rand_number(g, 1, 5) >= 4) {
          do_kick(g, ch, target, 0, 0);
        } else if (rand_number(g, 1, 5) >= 4) {
          do_elbow(g, ch, target, 0, 0);
        } else {
          do_punch(g, ch, target, 0, 0);
        }
      }
    }

    /* Be helpful */ /* - temporarily disabled by the first false check */
    if (false && char_is_humanoid(ch) && !char_mob_flagged(ch, MOB_NOKILL)) {
      struct char_data *ally, *next_v;
      int done = false;
      for (ally = rroom_get_people(g, char_get_in_room(ch)); ally; ally = next_v) {
        next_v = char_get_next_in_room(ally);
        if (ally == ch)
          continue;
        if (char_is_npc(ally) && race_is_people(char_get_race(ally)) && char_get_fighting(ally) && done == false) {
          if (!is_sparring(ally) && !is_sparring(ch) && getCurPL(g, ally) < getCurPL(g, ch) * 0.6 && axion_dice(g, 0) >= 90) {
            act(g, "@c$n@C rushes to @c$N's@C aid!@n", true, ch, 0, ally, TO_ROOM);
            char buf[MAX_INPUT_LENGTH];
            sprintf(buf, "%s", char_get_name(ally));
            if (char_get_class(ch) == CLASS_KABITO || char_get_class(ch) == CLASS_NAIL) {
              do_heal(g, ch, buf, 0, 0);
            } else {
              do_rescue(g, ch, buf, 0, 0);
              if (rand_number(g, 1, 6) == 2) {
                char tar[MAX_INPUT_LENGTH];
                sprintf(tar, "%s", char_get_name(char_get_fighting(ally)));
                do_kiblast(g, ch, tar, 0, 0);
              } else if (rand_number(g, 1, 6) >= 4) {
                char tar[MAX_INPUT_LENGTH];
                sprintf(tar, "%s", char_get_name(char_get_fighting(ally)));
                do_slam(g, ch, tar, 0, 0);
              } else {
                char tar[MAX_INPUT_LENGTH];
                sprintf(tar, "%s", char_get_name(char_get_fighting(ally)));
                do_punch(g, ch, tar, 0, 0);
              }
            }
          }
        }
      } /* End of for */
    }

    /* Help those under attack! */ /* - temporarily disabled by the first false check */
    if (false && !char_get_fighting(ch) && rand_number(g, 1, 20) >= 14 && char_is_humanoid(ch) && !char_mob_flagged(ch, MOB_NOKILL)) {
      struct char_data *ally, *next_v;
      int done = false;
      for (ally = rroom_get_people(g, char_get_in_room(ch)); ally; ally = next_v) {
        next_v = char_get_next_in_room(ally);
        if (ally == ch)
          continue;
        if (char_is_npc(ally) && race_is_people(char_get_race(ally)) && char_get_fighting(ally) && done == false) {
          if (!is_sparring(ally) && !is_sparring(ch) && getCurPL(g, ally) < getCurPL(g, ch) * 0.6 && axion_dice(g, 0) >= 70) {
            act(g, "@c$n@C rushes to @c$N's@C aid!@n", true, ch, 0, ally, TO_ROOM);
            char buf[MAX_INPUT_LENGTH];
            sprintf(buf, "%s", char_get_name(ally));
            if (char_get_class(ch) == CLASS_KABITO || char_get_class(ch) == CLASS_NAIL) {
              do_heal(g, ch, buf, 0, 0);
              done = true;
            } else {
              do_rescue(g, ch, buf, 0, 0);
              done = true;
            }
          }
        }
      } /* End of for */
    }

    /* Absorb protection */
    if (char_get_absorbby(ch) && rand_number(g, 1, 3) == 3) {
      do_escape(g, ch, 0, 0, 0);
    }
    if (char_get_position(ch) == POS_SLEEPING && rand_number(g, 1, 3) == 3) {
      do_wake(g, ch, 0, 0, 0);
    }
    /* Shopkeep 24-hour Purge */
    if (char_get_mob_spec(g, ch) == shop_keeper) {
      time_t diff = 0;

      diff = game_now(g) - char_get_lastpl(ch);

      if (diff > 86400) {
        struct obj_data *sobj, *next_obj;
        shop_rnum shop_nr;
        shop_rnum shopnr = -1;

        char_set_lastpl(ch, game_now(g));
        for (shop_nr = 0; shop_nr <= g->top_shop; shop_nr++) {
          if (rshop_get_keeper(g, shop_nr) == char_get_nr(ch)) {
            shopnr = shop_nr;
          }
        }
        for (sobj = char_get_inventory(ch); sobj; sobj = next_obj) {
          next_obj = obj_get_next_content(sobj);
          if (sobj != nullptr && !shop_producing(g, sobj, shopnr)) {
            char_set_gold(ch, char_get_gold(ch) + (obj_get_cost(sobj)));
            extract_obj(g, sobj);
          }
        }
      }
    }

    /* Mob Memory */
    if (char_is_humanoid(ch) && char_get_mob_specials_memory(ch) && !char_mob_flagged(ch, MOB_DUMMY) && !char_is_affected(ch, AFF_PARALYZE)) {
      found = false;
      for (vict = rroom_get_people(g, char_get_in_room(ch)); vict && !found; vict = char_get_next_in_room(vict)) {
        if (char_is_npc(vict) || !char_can_see(g, ch, vict) || char_prf_flagged(g, vict, PRF_NOHASSLE))
          continue;
        if (char_get_fighting(ch))
          continue;
        if (getCurPL(g, ch) <= getEffMaxPL(g, ch) / 100)
          continue;

        for (names = char_get_mob_specials_memory(ch); names && !found; names = names->next) {
          if (names->id != char_get_idnum(vict))
            continue;

          found = true;
          act(g, "'Hey!  You're the fiend that attacked me!!!', exclaims $n.", false, ch, 0, 0, TO_ROOM);
          char tar[MAX_INPUT_LENGTH];

          sprintf(tar, "%s", char_get_name(vict));
          do_punch(g, ch, tar, 0, 0);
        }
      }
    }

    if (char_get_fighting(ch) && rand_number(g, 1, 30) >= 25) {
      mob_taunt(g, ch);
    }

    /* Helper Mobs */
    if (char_mob_flagged(ch, MOB_HELPER) &&
        !char_aff_flagged(ch, AFF_BLIND) &&
        !char_aff_flagged(ch, AFF_CHARM)) {
      found = false;
      for (vict = rroom_get_people(g, char_get_in_room(ch)); vict && !found; vict = char_get_next_in_room(vict)) {
        if (ch == vict || !char_is_npc(vict) || !char_get_fighting(vict))
          continue;
        if (char_is_npc(char_get_fighting(vict)) || ch == char_get_fighting(vict))
          continue;

        if (char_is_humanoid(vict)) {
          act(g, "$n jumps to the aid of $N!", false, ch, 0, vict, TO_ROOM);
          char tar[MAX_INPUT_LENGTH];

          sprintf(tar, "%s", char_get_name(char_get_fighting(vict)));
          do_punch(g, ch, tar, 0, 0);
          found = true;
        }
      }
    }

    /* Add new mobile actions here */

    if (char_is_kabito(ch)) {
      shop_rnum shop_nr;
      found = false;
      /* Is there a shopkeeper around? */
      for (vict = rroom_get_people(g, char_get_in_room(ch)); vict && !found; vict = char_get_next_in_room(vict)) {
        if (char_get_mob_spec(g, vict) == shop_keeper) {
          /* Ok, vict is a shop keeper.  Which shop is his? */
          for (shop_nr = 0; shop_nr <= g->top_shop; shop_nr++)
            if (rshop_get_keeper(g, shop_nr) == char_get_nr(vict))
              break;
          if (shop_nr <= g->top_shop)
            /* Is the shopkeeper in his shop? */
            if (ok_shop_room(g, shop_nr, char_get_in_room(vict)))
              /* Does the shopkeeper prevent stealing? */
              if (!rshop_allows_steal(g, shop_nr))
                found = true;
        }
        /* Note: found will be true if there the character is in a shop where
       * the shopkeeper present who doesn't allow stealing.  Don't bother 
       * running the next loop, since we can't steal from anyone anyway. 
       */
      }
      for (vict = rroom_get_people(g, char_get_in_room(ch)); vict && !found; vict = char_get_next_in_room(vict)) {
        if (vict == ch)
          continue;
        if (char_mob_flagged(ch, MOB_WIMPY) && char_is_awake(vict))
          continue;
        if (!char_is_humanoid(vict))
          continue;
        if (char_is_npc(vict) && char_mob_flagged(vict, MOB_NOKILL))
          continue;
        if (char_get_mob_vnum(g, ch) == char_get_mob_vnum(g, vict))
          continue;
        if (char_get_level(ch) >= char_get_level(vict)) {
          if (roll_skill(g, ch, SKILL_SLEIGHT_OF_HAND)) {
            npc_steal(g, ch, vict);
            found = true;
          }
        }
      }
    }

  } /* end for() */
}

/* This handles NPCs taunting opponents or reacting to combat. */
static void mob_taunt(GameWorld *g, struct char_data *ch)
{

  int message = 1;

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE)) { /* In space.... nobody cares. */
    return;
  }

  if (!char_get_fighting(ch)) { /* The NPC is not fighting. Error. ABORT! */
    return;
  }

  struct char_data *vict = char_get_fighting(ch);

  if (vict == nullptr) { /* OH NO */
    return;
  }

  if (!char_is_humanoid(ch) && !rroom_sunken(g, char_get_in_room(ch))) { /* They are an animal and they are not in the water. */
    message = rand_number(g, 1, 12);
    switch (message) { /* Display the appropriate message. */
      case 1:
        act(g, "@C$n@W growls viciously at @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W growls viciously at you!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 2:
        act(g, "@C$n@W snaps $s jaws at @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W snaps $s jaws at you!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 3:
        act(g, "@C$n@W is panting heavily from $s struggle with @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W is panting heavily from $s struggle with you!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 4:
        act(g, "@C$n@W circles around @c$N@W trying to get a better position!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W circles around you trying to find a weak spot!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 5:
        act(g, "@C$n@W jumps up slightly in an attempt to threaten @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W jumps up slightly in an attempt to threaten you!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 6:
        act(g, "@C$n@W turns sideways while facing @c$N@W in an attempt to appear larger and more threatening!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W turns sideways while facing you in an attempt to appear larger and more threatening!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 7:
        act(g, "@C$n@W roars with the full power of its lungs at @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W roars with the full power of its lungs at you!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 8:
        act(g, "@C$n@W staggers from the strain of fighting.@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W staggers from the strain of fighting.@n", true, ch, 0, vict, TO_VICT);
        break;
      case 9:
        act(g, "@C$n@W slumps down for a moment before regaining $s guard against @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W slumps down for a moment before regaining $s guard against you!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 10:
        act(g, "@C$n's@W eyes dart around as $e seems to look for safe places to run.@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n's@W eyes dart around as $e seems to look for safe places to run.@n", true, ch, 0, vict, TO_VICT);
        break;
      case 11:
        act(g, "@C$n@W jumps past @c$N@W before turning and facing $M again!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W jumps past you before turning and facing you again!@n", true, ch, 0, vict, TO_VICT);
        break;
      default:
        act(g, "@C$n@W watches @c$N@W with a threatening gaze while $e looks for a weakness!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W watches you with a threatening gaze while $e looks for a weakness!@n", true, ch, 0, vict, TO_VICT);
        break;
    }
  } else if (!char_is_humanoid(ch)) { /* Animal under water */
    message = rand_number(g, 1, 7);
    switch (message) {
      case 1:
        act(g, "@C$n@W snaps $s jaws at @c$N@W which causes a torrent of bubbles to float upward!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W snaps $s jaws at you which causes a torrent of bubbles to float upward!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 2:
        act(g, "@C$n@W thrashes around in the water!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W thrashes around in the water!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 3:
        act(g, "@C$n@W swims past @c$N@W before turning and facing $M again!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W swims past you before turning and facing you again!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 4:
        act(g, "@C$n@W begins to slowly circle @c$N@W while looking for an opening!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W begins to slowly circle you while looking for an opening!@n", true, ch, 0, vict, TO_VICT);
        break;
      case 5:
        act(g, "@C$n@W swims backward in an attempt to gain a safe distance from @C$N's@W aggression.@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W swims backward in an attempt to gain a safe distance from you.@n", true, ch, 0, vict, TO_VICT);
        break;
      case 6:
        act(g, "@C$n@W swims toward the side of @C$N@W in an attempt to flank $M!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W swims toward the side of you in an attempt to flank you!@n", true, ch, 0, vict, TO_VICT);
        break;
      default:
        act(g, "@C$n@W swims upward before darting down past @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
        act(g, "@C$n@W swims upward before darting down past you!@n", true, ch, 0, vict, TO_VICT);
        break;
    }
  } else if (!char_mob_flagged(ch, MOB_DUMMY)) { /* They are intelligent */
    message = rand_number(g, 1, 10);
    if (!rroom_sunken(g, char_get_in_room(ch))) {
      if (char_aff_flagged(ch, AFF_FLYING)) { /* They are flying */
        switch (message) {
          case 1:
            act(g, "@C$n@W flies around @c$N@W slowly while looking for an opening!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W flies around you slowly while looking for an opening!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 2:
            act(g, "@C$n@W floats slowly while scowling at @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W floats slowly while scowling at you!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 3:
            act(g, "@C$n@W spits at @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W spits at you!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 4:
            act(g, "@C$n@W looks at @c$N@W as if $e is weighing $s options.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W looks at you as if $e is weighing $s options.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 5:
            act(g, "@C$n@W scowls at @c$N@W while changing $s position carefully!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W scowls at you while changing $s position carefully!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 6:
            act(g, "@C$n@W flips backward a short way away from @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W flips backward a short way away from you!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 7:
            act(g, "@C$n@W moves slowly to the side of @c$N@W while watching $M carefully.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W moves slowly to the side of you while watching you carefully.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 8:
            act(g, "@C$n@W flexes $s arms in an attempt to threaten @C$N@W.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W flexes $s arms threaten in an attempt to threaten you@W.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 9:
            act(g, "@C$n@W raises an arm in front of $s body as a defense.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W raises an arm in front of $s body as a defense.@n", true, ch, 0, vict, TO_VICT);
            break;
          default:
            act(g, "@C$n@W feints a punch toward @c$N@W that misses by a mile.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W feints a punch toward you that misses by a mile.@n", true, ch, 0, vict, TO_VICT);
            break;
        }

      } else { /* They are not flying. */
        message = rand_number(g, 1, 13);
        switch (message) {
          case 1:
            act(g, "@C$n@W shuffles around @c$N@W slowly while looking for an opening!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W shuffles around you slowly while looking for an opening!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 2:
            act(g, "@C$n@W scowls @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W scowls at you!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 3:
            if (char_is_android(ch)) {
              act(g, "@C$n@W has sparks come off them that land on @c$N@W!@n@n", true, ch, 0, vict, TO_NOTVICT);
              act(g, "@C$n@W has sparks come off them that land on you!@n", true, ch, 0, vict, TO_VICT);
            } else {
              act(g, "@C$n@W spits at @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
              act(g, "@C$n@W spits at you!@n", true, ch, 0, vict, TO_VICT);
            }
            break;
          case 4:
            act(g, "@C$n@W looks at @c$N@W as if $e is weighing $s options.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W looks at you as if $e is weighing $s options.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 5:
            act(g, "@C$n@W scowls at @c$N@W while changing $s position carefully!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W scowls at you while changing $s position carefully!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 6:
            act(g, "@C$n@W flips backward a short way away from @c$N@W!@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W flips backward a short way away from you!@n", true, ch, 0, vict, TO_VICT);
            break;
          case 7:
            act(g, "@C$n@W moves slowly to the side of @c$N@W while watching $M carefully.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W moves slowly to the side of you while watching you carefully.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 8:
            act(g, "@C$n@W crouches down cautiously.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W crouches down cautiously.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 9:
            act(g, "@C$n@W moves $s feet slowly to achieve a better balance.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W moves $s feet slowly to achieve a better balance.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 10:
            act(g, "@C$n@W leaps to a more defensible spot.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W leaps to a more defensible spot.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 11:
            act(g, "@C$n@W runs a short distance away before skidding to a halt and resuming $s fighting stance.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W runs a short distance away before skidding to a halt and resuming $s fighting stance.@n", true, ch, 0, vict, TO_VICT);
            break;
          case 12:
            act(g, "@C$n@W stands up to $s full height and glares at @C$N@W with burning eyes.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W stands up to $s full height and glares at you with intense burning eyes.@n", true, ch, 0, vict, TO_VICT);
            break;
          default:
            act(g, "@C$n@W feints a punch toward @c$N@W that misses by a mile.@n", true, ch, 0, vict, TO_NOTVICT);
            act(g, "@C$n@W feints a punch toward you that misses by a mile.@n", true, ch, 0, vict, TO_VICT);
            break;
        }
      }
    }
  } /* End humanoid */
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(GameWorld *g, struct char_data *ch, struct char_data *victim)
{
  memory_rec *tmp;
  bool present = false;

  if (!char_is_npc(ch) || char_is_npc(victim) || char_prf_flagged(g, victim, PRF_NOHASSLE))
    return;

  for (tmp = char_get_mob_specials_memory(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == char_get_idnum(victim))
      present = true;

  if (!present && !char_mob_flagged(ch, MOB_SPAR) && !char_plr_flagged(victim, PLR_SPAR)) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = char_get_mob_specials_memory(ch);
    tmp->id   = char_get_idnum(victim);
    char_set_mob_specials_memory(ch, tmp);
  }
}

/* make ch forget victim */

/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
  memory_rec *curr, *next;

  curr = char_get_mob_specials_memory(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  char_set_mob_specials_memory(ch, nullptr);
}

/*
 * An aggressive mobile wants to attack something.  If
 * they're under the influence of mind altering PC, then
 * see if their master can talk them out of it, eye them
 * down, or otherwise intimidate the slave.
 */
