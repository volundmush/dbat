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


/* local functions */
int player_present(struct char_data *ch);


#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

int player_present(struct char_data *ch)
{

  struct char_data *vict, *next_v;
  int found = FALSE;

  if (char_get_in_room(ch) == NOWHERE)
   return 0;

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
   next_v = vict->next_in_room; 
   if (!char_is_npc(vict)) {
    found = TRUE;
   }
  }

 return (found);
}

void mobile_activity(void)
{
  struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int door, found, max;
  memory_rec *names;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!char_is_mob(ch))
      continue;

    /* Examine call for special procedure */
    if (char_mob_flagged(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[char_get_nr(ch)].func == NULL) {
	log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
		char_get_name(ch), char_get_mob_vnum(ch));
	char_mob_set(ch, MOB_SPEC, false);
      } else {
        char actbuf[MAX_INPUT_LENGTH] = "";
	if ((mob_index[char_get_nr(ch)].func) (ch, ch, 0, actbuf))
	  continue;		/* go to next char */
      }
    }

    /* If the mob has no specproc, do the default actions */
    if (!char_is_awake(ch))
      continue;

    /* Scavenger (picking up objects) */
    if (char_is_humanoid(ch) && !char_get_fighting(ch) && char_is_awake(ch) && !char_mob_flagged(ch, MOB_NOSCAVENGER) && !char_mob_flagged(ch, MOB_NOKILL) && (!player_present(ch) || axion_dice(0) > 118))
      if (world[char_get_in_room(ch)].contents && rand_number(1, 100) >= 95) {
	max = 1;
	best_obj = NULL;
	for (obj = world[char_get_in_room(ch)].contents; obj; obj = obj->next_content)
	  if (char_can_get_obj(ch, obj) && obj_get_cost(obj) > max) {          
	    best_obj = obj;
	    max = obj_get_cost(obj);
	  }
	if (best_obj != NULL && char_can_get_obj(ch, best_obj) && obj_get_type_flag(best_obj) != ITEM_BED && !obj_get_posted_to(best_obj) && !obj_extra_flagged(best_obj, ITEM_NOPICKUP)) {
          switch (rand_number(1, 5)) {
           case 1:
            act("$n@W says, '@CFinders keepers, losers weepers.@W'@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
           case 2:
            act("$n@W says, '@CPeople always leaving their garbage JUST LYING AROUND. The nerve....@W'@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
           case 3:
            act("$n@W says, '@CWho would leave this here? Oh well..@W'@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
           case 4:
            act("$n@W says, '@CI always wanted one of these.@W'@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
           case 5:
            act("$n@W looks around quickly to see if anyone is paying attention.@n", TRUE, ch, 0, 0, TO_ROOM);
            break;
          }
	  perform_get_from_room(ch, best_obj);
	}
      }

    /* Mob Movement */
    if (!char_mob_flagged(ch, MOB_SENTINEL) && (char_get_position(ch) == POS_STANDING) && !char_get_fighting(ch) &&
        (!char_aff_flagged(ch, AFF_TAMED)) && !char_get_absorbby(ch) &&
	((door = rand_number(0, 18)) < NUM_OF_DIRS) && char_can_go(ch, door) &&
	!rroom_flagged(char_get_exit(ch, door)->to_room, ROOM_NOMOB) &&
	!rroom_flagged(char_get_exit(ch, door)->to_room, ROOM_DEATH) &&
	(!char_mob_flagged(ch, MOB_STAY_ZONE) ||
	 (world[char_get_exit(ch, door)->to_room].zone == world[char_get_in_room(ch)].zone))) {
     if (rand_number(1, 2) == 2 && !char_is_affected(ch, AFF_PARALYZE) && block_calc(ch)) {
      perform_move(ch, door, 1);
     }
    }

    /* RESPOND TO A HUGE ATTACK */
     struct obj_data *hugeatk = NULL, *next_huge = NULL;
     for (hugeatk = world[char_get_in_room(ch)].contents; hugeatk; hugeatk = next_huge) {
      next_huge = hugeatk->next_content;
      if (char_get_fighting(ch)) {
       continue;
      }
      if (char_mob_flagged(ch, MOB_NOKILL)) {
       continue;
      }
      if (obj_get_vnum(hugeatk) == 82 || obj_get_vnum(hugeatk) == 83) {
       if (obj_get_user(hugeatk) != NULL) {
        act("@W$n@R leaps at @C$N@R desperately!@n", TRUE, ch, 0, obj_get_user(hugeatk), TO_ROOM);
        act("@W$n@R leaps at YOU desperately!@n", TRUE, ch, 0, obj_get_user(hugeatk), TO_VICT);
        if (char_is_humanoid(ch)) {
         char tar[MAX_INPUT_LENGTH];
         sprintf(tar, "%s", char_get_name(obj_get_user(hugeatk)));
         do_punch(ch, tar, 0, 0);
        } else {
         char tar[MAX_INPUT_LENGTH];
         sprintf(tar, "%s", char_get_name(obj_get_user(hugeatk)));
         do_bite(ch, tar, 0, 0);
        }
       }
      }
     }

    /* Aggressive Mobs */
    if (char_mob_flagged(ch, MOB_AGGRESSIVE) && !char_is_affected(ch, AFF_PARALYZE)) {
      int spot_roll = rand_number(1, char_get_level(ch) + 10);
      found = FALSE;
      for (vict = world[char_get_in_room(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (vict == ch)
          continue;
        else if (char_get_fighting(ch))
          continue;
	else if (!char_can_see(ch, vict))
          continue;
        else if (char_is_npc(vict))
          continue;
        else if (char_prf_flagged(vict, PRF_NOHASSLE))
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
        else if (ch->aggtimer < 8)
         ch->aggtimer += 1;
        else {
          ch->aggtimer = 0;
          char tar[MAX_INPUT_LENGTH];

          sprintf(tar, "%s", char_get_name(vict));
          if (char_is_humanoid(ch)) {
           if (!char_aff_flagged(vict, AFF_HIDE) && !char_aff_flagged(vict, AFF_SNEAK)) {
            act("@w'I am going to get you!' @C$n@w shouts at you!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           } else {
            act("@C$n@w notices YOU.\n@w'I am going to get you!' @C$n@w shouts at you!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@w notices @c$N@w.\n@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           }
          if (char_aff_flagged(vict, AFF_FLYING) && !char_aff_flagged(ch, AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) > 10) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           if (!char_aff_flagged(vict, AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING)) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           do_punch(ch, tar, 0, 0);
          }
          if (!char_is_humanoid(ch)) {
          if (char_aff_flagged(vict, AFF_FLYING) && !char_aff_flagged(ch, AFF_FLYING) && char_is_humanoid(ch) && char_get_level(ch) > 10) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           if (!char_aff_flagged(vict, AFF_FLYING) && char_aff_flagged(ch, AFF_FLYING)) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           if (!char_aff_flagged(vict, AFF_HIDE) && !char_aff_flagged(vict, AFF_SNEAK)) {
            act("@C$n @wgrowls viciously at you!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n @wgrowls viciously at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           } else {
            act("@C$n@w notices YOU.\n@C$n @wgrowls viciously at you!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@w notices @c$N@w.\n@C$n @wgrowls viciously at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           }
           do_bite(ch, tar, 0, 0);
          }
	  /*hit(ch, vict, TYPE_UNDEFINED);*/
	  found = TRUE;
        }
      }
    }

    if (char_get_original(ch) && rand_number(1, 5) >= 4) {
     struct char_data *original = char_get_original(ch);
     
     if (char_get_fighting(original) && !char_get_fighting(ch)) {
      char target[MAX_INPUT_LENGTH];
      struct char_data *targ = char_get_fighting(original);
       
      sprintf(target, "%s", targ->name);
      if (rand_number(1, 5) >= 4) {
       do_kick(ch, target, 0, 0);
      } else if (rand_number(1, 5) >= 4) {
       do_elbow(ch, target, 0, 0);
      } else {
       do_punch(ch, target, 0, 0);
      }
     }
    }

    /* Be helpful */ /* - temporarily disabled by the first false check */
    if (false && char_is_humanoid(ch) && !char_mob_flagged(ch, MOB_NOKILL)) {
     struct char_data *vict, *next_v;
     int done = FALSE;
     for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (vict == ch)
       continue;
      if (char_is_npc(vict) && race_is_people(vict->race) && char_get_fighting(vict) && done == FALSE) {
       if (!is_sparring(vict) && !is_sparring(ch) && GET_HIT(vict) < GET_HIT(ch) * 0.6 && axion_dice(0) >= 90) {
        act("@c$n@C rushes to @c$N's@C aid!@n", TRUE, ch, 0, vict, TO_ROOM);
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%s", char_get_name(vict));
        if (char_get_class(ch) == CLASS_KABITO || char_get_class(ch) == CLASS_NAIL) {
         do_heal(ch, buf, 0, 0);
        } else {
         do_rescue(ch, buf, 0, 0);
         if (rand_number(1, 6) == 2) {
          char tar[MAX_INPUT_LENGTH];
          sprintf(tar, "%s", char_get_name(char_get_fighting(vict)));
          do_kiblast(ch, tar, 0, 0);
         } else if (rand_number(1, 6) >= 4) {
          char tar[MAX_INPUT_LENGTH];
          sprintf(tar, "%s", char_get_name(char_get_fighting(vict)));
          do_slam(ch, tar, 0, 0);
         } else {
          char tar[MAX_INPUT_LENGTH];
          sprintf(tar, "%s", char_get_name(char_get_fighting(vict)));
          do_punch(ch, tar, 0, 0);
         }
        }
       }
      }
     } /* End of for */
    }

    /* Help those under attack! */ /* - temporarily disabled by the first false check */
    if (false && !char_get_fighting(ch) && rand_number(1, 20) >= 14 && char_is_humanoid(ch) && !char_mob_flagged(ch, MOB_NOKILL)) {
          struct char_data *vict, *next_v;
     int done = FALSE;
     for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (vict == ch)
       continue;
      if (char_is_npc(vict) && race_is_people(vict->race) && char_get_fighting(vict) && done == FALSE) {
       if (!is_sparring(vict) && !is_sparring(ch) && GET_HIT(vict) < GET_HIT(ch) * 0.6 && axion_dice(0) >= 70) {
        act("@c$n@C rushes to @c$N's@C aid!@n", TRUE, ch, 0, vict, TO_ROOM);
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%s", char_get_name(vict));
        if (char_get_class(ch) == CLASS_KABITO || char_get_class(ch) == CLASS_NAIL) {
         do_heal(ch, buf, 0, 0);
         done = TRUE;
        } else {
         do_rescue(ch, buf, 0, 0);
         done = TRUE;
        }
       }
      }
     } /* End of for */
    } 

    /* Absorb protection */
    if (char_get_absorbby(ch) && rand_number(1, 3) == 3) {
     do_escape(ch, 0, 0, 0);
    }
    if (char_get_position(ch) == POS_SLEEPING && rand_number(1, 3) == 3) {
     do_wake(ch, 0, 0, 0);
    }
    /* Shopkeep 24-hour Purge */
    if (char_get_mob_spec(ch) == shop_keeper) {
     time_t diff = 0;

     diff = time(0) - char_get_lastpl(ch);

     if (diff > 86400) {
     struct obj_data *sobj, *next_obj;
     int shop_nr, shopnr = -1;

      char_set_lastpl(ch, time(0));
      for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
        if (SHOP_KEEPER(shop_nr) == ch->nr) {
         shopnr = shop_nr;
        }
      }
      for (sobj = ch->carrying; sobj; sobj = next_obj) {
          next_obj = sobj->next_content;
          if (sobj != NULL && !shop_producing(sobj, shopnr)) {
           char_set_gold(ch, char_get_gold(ch) + (obj_get_cost(sobj)));
           extract_obj(sobj);
          }
      }
     }
    }


    /* Mob Memory */
    if (char_is_humanoid(ch) && char_get_mob_specials_memory(ch) && !char_mob_flagged(ch, MOB_DUMMY) && !char_is_affected(ch, AFF_PARALYZE)) {
      found = FALSE;
      for (vict = world[char_get_in_room(ch)].people; vict && !found; vict = vict->next_in_room) {
	if (char_is_npc(vict) || !char_can_see(ch, vict) || char_prf_flagged(vict, PRF_NOHASSLE))
	  continue;
        if (char_get_fighting(ch))
         continue;
        if (GET_HIT(ch) <= GET_MAX_HIT(ch) / 100)
         continue;

	for (names = char_get_mob_specials_memory(ch); names && !found; names = names->next) {
	  if (names->id != char_get_idnum(vict))
            continue;

          found = TRUE;
          act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
          char tar[MAX_INPUT_LENGTH];
 
          sprintf(tar, "%s", char_get_name(vict));
          do_punch(ch, tar, 0, 0);
        }
      }
    }

    if (char_get_fighting(ch) && rand_number(1, 30) >= 25) {
     mob_taunt(ch);
    }

    /* Helper Mobs */
    if (char_mob_flagged(ch, MOB_HELPER) && 
        !char_aff_flagged(ch, AFF_BLIND) &&
        !char_aff_flagged(ch, AFF_CHARM)) {
      found = FALSE;
      for (vict = world[char_get_in_room(ch)].people; vict && !found; vict = vict->next_in_room) {
	if (ch == vict || !char_is_npc(vict) || !char_get_fighting(vict))
	  continue;
	if (char_is_npc(char_get_fighting(vict)) || ch == char_get_fighting(vict))
	  continue;

        if (char_is_humanoid(vict)) {
	act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
        char tar[MAX_INPUT_LENGTH];

        sprintf(tar, "%s", char_get_name(char_get_fighting(vict)));
        do_punch(ch, tar, 0, 0);
	found = TRUE;
        }
      }
    }

    /* Add new mobile actions here */

    if (char_is_kabito(ch)) {
      int shop_nr;
      found = FALSE;
      /* Is there a shopkeeper around? */
      for (vict = world[char_get_in_room(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (char_get_mob_spec(vict) == shop_keeper) {
          /* Ok, vict is a shop keeper.  Which shop is his? */
          for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
            if (SHOP_KEEPER(shop_nr) == vict->nr)
              break;
          if (shop_nr <= top_shop)
            /* Is the shopkeeper in his shop? */
            if (ok_shop_room(shop_nr, char_get_in_room(vict)))
              /* Does the shopkeeper prevent stealing? */
              if (!SHOP_ALLOW_STEAL(shop_nr))
                found = TRUE;
        }
      /* Note: found will be true if there the character is in a shop where
       * the shopkeeper present who doesn't allow stealing.  Don't bother 
       * running the next loop, since we can't steal from anyone anyway. 
       */
      }
      for (vict = world[char_get_in_room(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (vict == ch)
          continue;
        if (char_mob_flagged(ch, MOB_WIMPY) && char_is_awake(vict))
          continue;
        if (!char_is_humanoid(vict))
          continue;
        if (char_is_npc(vict) && char_mob_flagged(vict, MOB_NOKILL))
          continue;
        if (char_get_mob_vnum(ch) == char_get_mob_vnum(vict))
          continue;
        if (char_get_level(ch) >= char_get_level(vict)) {
          if (roll_skill(ch, SKILL_SLEIGHT_OF_HAND)) {
            npc_steal(ch, vict);
            found = TRUE;
          }
        }
      }
    }

  }				/* end for() */
}

/* This handles NPCs taunting opponents or reacting to combat. */
void mob_taunt(struct char_data *ch)
{

 int message = 1;

 if (rroom_flagged(char_get_in_room(ch), ROOM_SPACE)) { /* In space.... nobody cares. */
  return;
 }

 if (!char_get_fighting(ch)) { /* The NPC is not fighting. Error. ABORT! */
  return;
 }

 struct char_data *vict = char_get_fighting(ch);

 if (vict == NULL) { /* OH NO */
  return;
 }

 if (!char_is_humanoid(ch) && !rroom_sunken(char_get_in_room(ch))) { /* They are an animal and they are not in the water. */
  message = rand_number(1, 12);
  switch (message) { /* Display the appropriate message. */
   case 1:
    act("@C$n@W growls viciously at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W growls viciously at you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 2:
    act("@C$n@W snaps $s jaws at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W snaps $s jaws at you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 3:
    act("@C$n@W is panting heavily from $s struggle with @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W is panting heavily from $s struggle with you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 4:
    act("@C$n@W circles around @c$N@W trying to get a better position!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W circles around you trying to find a weak spot!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 5:
    act("@C$n@W jumps up slightly in an attempt to threaten @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W jumps up slightly in an attempt to threaten you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 6:
    act("@C$n@W turns sideways while facing @c$N@W in an attempt to appear larger and more threatening!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W turns sideways while facing you in an attempt to appear larger and more threatening!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 7:
    act("@C$n@W roars with the full power of its lungs at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W roars with the full power of its lungs at you!@n", TRUE, ch, 0, vict, TO_VICT);
   case 8:
    act("@C$n@W staggers from the strain of fighting.@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W staggers from the strain of fighting.@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 9:
    act("@C$n@W slumps down for a moment before regaining $s guard against @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W slumps down for a moment before regaining $s guard against you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 10:
    act("@C$n's@W eyes dart around as $e seems to look for safe places to run.@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n's@W eyes dart around as $e seems to look for safe places to run.@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 11:
    act("@C$n@W jumps past @c$N@W before turning and facing $M again!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W jumps past you before turning and facing you again!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   default:
    act("@C$n@W watches @c$N@W with a threatening gaze while $e looks for a weakness!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W watches you with a threatening gaze while $e looks for a weakness!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
  }
 } else if (!char_is_humanoid(ch)) { /* Animal under water */
  message = rand_number(1, 7);
  switch (message) {
   case 1:
    act("@C$n@W snaps $s jaws at @c$N@W which causes a torrent of bubbles to float upward!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W snaps $s jaws at you which causes a torrent of bubbles to float upward!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 2:
    act("@C$n@W thrashes around in the water!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W thrashes around in the water!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 3:
    act("@C$n@W swims past @c$N@W before turning and facing $M again!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W swims past you before turning and facing you again!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 4:
    act("@C$n@W begins to slowly circle @c$N@W while looking for an opening!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W begins to slowly circle you while looking for an opening!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 5:
    act("@C$n@W swims backward in an attempt to gain a safe distance from @C$N's@W aggression.@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W swims backward in an attempt to gain a safe distance from you.@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   case 6:
    act("@C$n@W swims toward the side of @C$N@W in an attempt to flank $M!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W swims toward the side of you in an attempt to flank you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
   default:
    act("@C$n@W swims upward before darting down past @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    act("@C$n@W swims upward before darting down past you!@n", TRUE, ch, 0, vict, TO_VICT);
    break;
  }
 } else if (!char_mob_flagged(ch, MOB_DUMMY)) { /* They are intelligent */
  message = rand_number(1, 10);
  if (!rroom_sunken(char_get_in_room(ch))) {
   if (char_aff_flagged(ch, AFF_FLYING)) { /* They are flying */
    switch (message) {
     case 1:
      act("@C$n@W flies around @c$N@W slowly while looking for an opening!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W flies around you slowly while looking for an opening!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 2:
      act("@C$n@W floats slowly while scowling at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W floats slowly while scowling at you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 3:
      act("@C$n@W spits at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W spits at you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 4:
      act("@C$n@W looks at @c$N@W as if $e is weighing $s options.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W looks at you as if $e is weighing $s options.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 5:
      act("@C$n@W scowls at @c$N@W while changing $s position carefully!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W scowls at you while changing $s position carefully!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 6:
      act("@C$n@W flips backward a short way away from @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W flips backward a short way away from you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 7:
      act("@C$n@W moves slowly to the side of @c$N@W while watching $M carefully.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W moves slowly to the side of you while watching you carefully.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 8:
      act("@C$n@W flexes $s arms in an attempt to threaten @C$N@W.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W flexes $s arms threaten in an attempt to threaten you@W.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 9:
      act("@C$n@W raises an arm in front of $s body as a defense.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W raises an arm in front of $s body as a defense.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     default:
      act("@C$n@W feints a punch toward @c$N@W that misses by a mile.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W feints a punch toward you that misses by a mile.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
    }

   } else { /* They are not flying. */
    message = rand_number(1, 13);
    switch (message) {
     case 1:
      act("@C$n@W shuffles around @c$N@W slowly while looking for an opening!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W shuffles around you slowly while looking for an opening!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 2:
      act("@C$n@W scowls @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W scowls at you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 3:
		if (char_is_android(ch)) {
		  act("@C$n@W has sparks come off them that land on @c$N@W!@n@n", TRUE, ch, 0, vict, TO_NOTVICT);
		  act("@C$n@W has sparks come off them that land on you!@n", TRUE, ch, 0, vict, TO_VICT);
		} else {
		  act("@C$n@W spits at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
		  act("@C$n@W spits at you!@n", TRUE, ch, 0, vict, TO_VICT);
		}
      break;
     case 4:
      act("@C$n@W looks at @c$N@W as if $e is weighing $s options.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W looks at you as if $e is weighing $s options.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 5:
      act("@C$n@W scowls at @c$N@W while changing $s position carefully!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W scowls at you while changing $s position carefully!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 6:
      act("@C$n@W flips backward a short way away from @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W flips backward a short way away from you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 7:
      act("@C$n@W moves slowly to the side of @c$N@W while watching $M carefully.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W moves slowly to the side of you while watching you carefully.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 8:
      act("@C$n@W crouches down cautiously.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W crouches down cautiously.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 9:
      act("@C$n@W moves $s feet slowly to achieve a better balance.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W moves $s feet slowly to achieve a better balance.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 10:
      act("@C$n@W leaps to a more defensible spot.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W leaps to a more defensible spot.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 11:
      act("@C$n@W runs a short distance away before skidding to a halt and resuming $s fighting stance.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W runs a short distance away before skidding to a halt and resuming $s fighting stance.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     case 12:
      act("@C$n@W stands up to $s full height and glares at @C$N@W with burning eyes.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W stands up to $s full height and glares at you with intense burning eyes.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
     default:
      act("@C$n@W feints a punch toward @c$N@W that misses by a mile.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W feints a punch toward you that misses by a mile.@n", TRUE, ch, 0, vict, TO_VICT);
      break;
    }
   } 
  }
 } /* End humanoid */
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
  memory_rec *tmp;
  bool present = FALSE;

  if (!char_is_npc(ch) || char_is_npc(victim) || char_prf_flagged(victim, PRF_NOHASSLE))
    return;

  for (tmp = char_get_mob_specials_memory(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == char_get_idnum(victim))
      present = TRUE;

  if (!present && !char_mob_flagged(ch, MOB_SPAR) && !char_plr_flagged(victim, PLR_SPAR)) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = char_get_mob_specials_memory(ch);
    tmp->id = char_get_idnum(victim);
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

  char_set_mob_specials_memory(ch, NULL);
}


/*
 * An aggressive mobile wants to attack something.  If
 * they're under the influence of mind altering PC, then
 * see if their master can talk them out of it, eye them
 * down, or otherwise intimidate the slave.
 */
