/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "mobact.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "shop.h"
#include "combat.h"
#include "act.offensive.h"
#include "act.movement.h"
#include "act.other.h"
#include "act.item.h"
#include "act.social.h"
#include "spec_procs.h"

/* local functions */
int player_present(struct char_data *ch);
void clearMemory(struct char_data *ch);
bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);
void mob_absorb(struct char_data *ch, struct char_data *vict);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

void mob_absorb(struct char_data *ch, struct char_data *vict)
{

 if (ABSORBING(ch)) {
  act("@R$n@w releases YOU from $s grip!@n", TRUE, ch, 0, ABSORBING(ch), TO_VICT);
  act("@R$n@w releases @R$N@w from $s grip!@n", TRUE, ch, 0, ABSORBING(ch), TO_NOTVICT);
  ABSORBBY(ABSORBING(ch)) = NULL;
  ABSORBING(ch) = NULL;
  return;
 }
 
 int zanzo = FALSE, roll = 0, chance = GET_LEVEL(ch) * 0.5, chance2 = GET_LEVEL(ch) + 10;

  if (chance2 > 118)
   chance2 = 118;

  if (GET_LEVEL(ch) < 2)
    return;
  else
   roll = rand_number(chance, chance2);

  if (!vict)
   return;

  if (IS_ANDROID(vict))
   return;

  if (AFF_FLAGGED(vict, AFF_ZANZOKEN)) {
   if (AFF_FLAGGED(ch, AFF_ZANZOKEN)) {
    if (GET_SPEEDI(ch) < GET_SPEEDI(vict)) {
     zanzo = TRUE;
    } else {
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
    }
   } else {
    zanzo = TRUE;
   }
   if (zanzo == TRUE) {
    act("@R$n@c tries to grab @RYOU@c but you @Czanzoken@c out of the way!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@R$n@ctries to grab @R$N@c but $E @Czanzokens@c out of the way!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
    return;
   } else {
    act("@cYou try to @Czanzoken@c out of @R$n's@c reach, but $e is too fast!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@c$N tries to @Czanzoken@c out of @R$n's@c reach, but $e is too fast!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
   }
  }

  if (roll < check_def(vict)) {
   act("@R$n@r tries to grab YOU, but you manage to evade $s grasp!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@R$n@r tries to grab @R$N@r, but @R$N@r manages to evade!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   return;
  } else {
   act("@R$n@r grabs onto YOU and starts to absorb your energy!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@R$n@r grabs onto @R$N@r and starts to absorb your energy!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   ABSORBING(ch) = vict;
   ABSORBBY(vict) = ch;
   return;
  }

}

int player_present(struct char_data *ch)
{

  struct char_data *vict, *next_v;
  int found = FALSE;

  if (IN_ROOM(ch) == NOWHERE)
   return 0;

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
   next_v = vict->next_in_room; 
   if (!IS_NPC(vict)) {
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

    if (!IS_MOB(ch))
      continue;

    /* Examine call for special procedure */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
	log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
		GET_NAME(ch), GET_MOB_VNUM(ch));
	REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      } else {
        char actbuf[MAX_INPUT_LENGTH] = "";
	if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, actbuf))
	  continue;		/* go to next char */
      }
    }

    /* If the mob has no specproc, do the default actions */
    if (!AWAKE(ch))
      continue;

    /* Scavenger (picking up objects) */
    if (IS_HUMANOID(ch) && !FIGHTING(ch) && AWAKE(ch) && !MOB_FLAGGED(ch, MOB_NOSCAVENGER) && !MOB_FLAGGED(ch, MOB_NOKILL) && (!player_present(ch) || axion_dice(0) > 118))
      if (world[IN_ROOM(ch)].contents && rand_number(1, 100) >= 95) {
	max = 1;
	best_obj = NULL;
	for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
	  if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {          
	    best_obj = obj;
	    max = GET_OBJ_COST(obj);
	  }
	if (best_obj != NULL && CAN_GET_OBJ(ch, best_obj) && GET_OBJ_TYPE(best_obj) != ITEM_BED && !GET_OBJ_POSTED(best_obj) && !OBJ_FLAGGED(best_obj, ITEM_NOPICKUP)) {
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
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) && !FIGHTING(ch) &&
        (!AFF_FLAGGED(ch, AFF_TAMED)) && !ABSORBBY(ch) &&
	((door = rand_number(0, 18)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
	!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
	!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
	(!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
	 (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone))) {
     if (rand_number(1, 2) == 2 && !IS_AFFECTED(ch, AFF_PARALYZE) && block_calc(ch)) {
      perform_move(ch, door, 1);
     }
    }

    /* RESPOND TO A HUGE ATTACK */
     struct obj_data *hugeatk = NULL, *next_huge = NULL;
     for (hugeatk = world[IN_ROOM(ch)].contents; hugeatk; hugeatk = next_huge) {
      next_huge = hugeatk->next_content;
      if (FIGHTING(ch)) {
       continue;
      }
      if (MOB_FLAGGED(ch, MOB_NOKILL)) {
       continue;
      }
      if (GET_OBJ_VNUM(hugeatk) == 82 || GET_OBJ_VNUM(hugeatk) == 83) {
       if (USER(hugeatk) != NULL) {
        act("@W$n@R leaps at @C$N@R desperately!@n", TRUE, ch, 0, USER(hugeatk), TO_ROOM);
        act("@W$n@R leaps at YOU desperately!@n", TRUE, ch, 0, USER(hugeatk), TO_VICT);
        if (IS_HUMANOID(ch)) {
         char tar[MAX_INPUT_LENGTH];
         sprintf(tar, "%s", GET_NAME(USER(hugeatk)));
         do_punch(ch, tar, 0, 0);
        } else {
         char tar[MAX_INPUT_LENGTH];
         sprintf(tar, "%s", GET_NAME(USER(hugeatk)));
         do_bite(ch, tar, 0, 0);
        }
       }
      }
     }

    /* Aggressive Mobs */
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && !IS_AFFECTED(ch, AFF_PARALYZE)) {
      int spot_roll = rand_number(1, GET_LEVEL(ch) + 10);
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (vict == ch)
          continue;
        else if (FIGHTING(ch))
          continue;
	else if (!CAN_SEE(ch, vict))
          continue;
        else if (IS_NPC(vict))
          continue;
        else if (PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;
        else if (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && GET_ALIGNMENT(vict) < 50)
          continue;
        else if (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && GET_ALIGNMENT(vict) > -50)
          continue;
        else if (GET_LEVEL(vict) < 5)
          continue;
        else if (AFF_FLAGGED(vict, AFF_HIDE) && GET_SKILL(vict, SKILL_HIDE) > spot_roll)
          continue;
        else if (AFF_FLAGGED(vict, AFF_SNEAK) && GET_SKILL(vict, SKILL_MOVE_SILENTLY) > spot_roll)
          continue;
        else if (ch->aggtimer < 8)
         ch->aggtimer += 1;
        else {
          ch->aggtimer = 0;
          char tar[MAX_INPUT_LENGTH];

          sprintf(tar, "%s", GET_NAME(vict));
          if (IS_HUMANOID(ch)) {
           if (!AFF_FLAGGED(vict, AFF_HIDE) && !AFF_FLAGGED(vict, AFF_SNEAK)) {
            act("@w'I am going to get you!' @C$n@w shouts at you!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           } else {
            act("@C$n@w notices YOU.\n@w'I am going to get you!' @C$n@w shouts at you!@n", TRUE, ch, 0, vict, TO_VICT);
            act("@C$n@w notices @c$N@w.\n@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
           }
          if (AFF_FLAGGED(vict, AFF_FLYING) && !AFF_FLAGGED(ch, AFF_FLYING) && IS_HUMANOID(ch) && GET_LEVEL(ch) > 10) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           if (!AFF_FLAGGED(vict, AFF_FLYING) && AFF_FLAGGED(ch, AFF_FLYING)) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           do_punch(ch, tar, 0, 0);
          }
          if (!IS_HUMANOID(ch)) {
          if (AFF_FLAGGED(vict, AFF_FLYING) && !AFF_FLAGGED(ch, AFF_FLYING) && IS_HUMANOID(ch) && GET_LEVEL(ch) > 10) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           if (!AFF_FLAGGED(vict, AFF_FLYING) && AFF_FLAGGED(ch, AFF_FLYING)) {
            do_fly(ch, 0, 0, 0);
            continue;
           }
           if (!AFF_FLAGGED(vict, AFF_HIDE) && !AFF_FLAGGED(vict, AFF_SNEAK)) {
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

    if (GET_ORIGINAL(ch) && rand_number(1, 5) >= 4) {
     struct char_data *original = GET_ORIGINAL(ch);
     
     if (FIGHTING(original) && !FIGHTING(ch)) {
      char target[MAX_INPUT_LENGTH];
      struct char_data *targ = FIGHTING(original);
       
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

    /* Be helpful */
    if (IS_HUMANOID(ch) && !MOB_FLAGGED(ch, MOB_NOKILL)) {
     struct char_data *vict, *next_v;
     int done = FALSE;
     for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (vict == ch)
       continue;
      if (IS_NPC(vict) && FIGHTING(vict) && done == FALSE) {
       if (GET_HIT(vict) < GET_HIT(ch) * 0.6 && axion_dice(0) >= 90) {
        act("@c$n@C rushes to @c$N's@C aid!@n", TRUE, ch, 0, vict, TO_ROOM);
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%s", GET_NAME(vict));
        if (GET_CLASS(ch) == CLASS_KABITO || GET_CLASS(ch) == CLASS_NAIL) {
         do_heal(ch, buf, 0, 0);
        } else {
         do_rescue(ch, buf, 0, 0);
         if (rand_number(1, 6) == 2) {
          char tar[MAX_INPUT_LENGTH];
          sprintf(tar, "%s", GET_NAME(FIGHTING(vict)));
          do_kiblast(ch, tar, 0, 0);
         } else if (rand_number(1, 6) >= 4) {
          char tar[MAX_INPUT_LENGTH];
          sprintf(tar, "%s", GET_NAME(FIGHTING(vict)));
          do_slam(ch, tar, 0, 0);
         } else {
          char tar[MAX_INPUT_LENGTH];
          sprintf(tar, "%s", GET_NAME(FIGHTING(vict)));
          do_punch(ch, tar, 0, 0);
         }
        }
       }
      }
     } /* End of for */
    }

    /* Help those under attack! */
    if (!FIGHTING(ch) && rand_number(1, 20) >= 14 && IS_HUMANOID(ch) && !MOB_FLAGGED(ch, MOB_NOKILL)) {
          struct char_data *vict, *next_v;
     int done = FALSE;
     for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (vict == ch)
       continue;
      if (IS_NPC(vict) && FIGHTING(vict) && done == FALSE) {
       if (GET_HIT(vict) < GET_HIT(ch) * 0.6 && axion_dice(0) >= 70) {
        act("@c$n@C rushes to @c$N's@C aid!@n", TRUE, ch, 0, vict, TO_ROOM);
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%s", GET_NAME(vict));
        if (GET_CLASS(ch) == CLASS_KABITO || GET_CLASS(ch) == CLASS_NAIL) {
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
    if (ABSORBBY(ch) && rand_number(1, 3) == 3) {
     do_escape(ch, 0, 0, 0);
    }
    if (GET_POS(ch) == POS_SLEEPING && rand_number(1, 3) == 3) {
     do_wake(ch, 0, 0, 0);
    }
    /* Shopkeep 24-hour Purge */
    if (GET_MOB_SPEC(ch) == shop_keeper) {
     time_t diff = 0;

     diff = time(0) - GET_LPLAY(ch);

     if (diff > 86400) {
     struct obj_data *sobj, *next_obj;
     int shop_nr, shopnr = -1;

      GET_LPLAY(ch) = time(0);
      for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
        if (SHOP_KEEPER(shop_nr) == ch->nr) {
         shopnr = shop_nr;
        }
      }
      for (sobj = ch->carrying; sobj; sobj = next_obj) {
          next_obj = sobj->next_content;
          if (sobj != NULL && !shop_producing(sobj, shopnr)) {
           GET_GOLD(ch) += GET_OBJ_COST(sobj);
           extract_obj(sobj);
          }
      }
     }
    }


    /* Mob Memory */
    if (IS_HUMANOID(ch) && MEMORY(ch) && !MOB_FLAGGED(ch, MOB_DUMMY) && !IS_AFFECTED(ch, AFF_PARALYZE)) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;
        if (FIGHTING(ch))
         continue;
        if (GET_HIT(ch) <= GET_MAX_HIT(ch) / 100)
         continue;

	for (names = MEMORY(ch); names && !found; names = names->next) {
	  if (names->id != GET_IDNUM(vict))
            continue;

          found = TRUE;
          act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
          char tar[MAX_INPUT_LENGTH];
 
          sprintf(tar, "%s", GET_NAME(vict));
          do_punch(ch, tar, 0, 0);
        }
      }
    }

    if (FIGHTING(ch) && rand_number(1, 30) >= 25) {
     mob_taunt(ch);
    }

    /* Helper Mobs */
    if (MOB_FLAGGED(ch, MOB_HELPER) && 
        !AFF_FLAGGED(ch, AFF_BLIND) &&
        !AFF_FLAGGED(ch, AFF_CHARM)) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
	if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
	  continue;
	if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
	  continue;

        if (IS_HUMANOID(vict)) {
	act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
        char tar[MAX_INPUT_LENGTH];

        sprintf(tar, "%s", GET_NAME(FIGHTING(vict)));
        do_punch(ch, tar, 0, 0);
	found = TRUE;
        }
      }
    }

    /* Add new mobile actions here */

    if (IS_KABITO(ch)) {
      int shop_nr;
      found = FALSE;
      /* Is there a shopkeeper around? */
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (GET_MOB_SPEC(vict) == shop_keeper) {
          /* Ok, vict is a shop keeper.  Which shop is his? */
          for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
            if (SHOP_KEEPER(shop_nr) == vict->nr)
              break;
          if (shop_nr <= top_shop)
            /* Is the shopkeeper in his shop? */
            if (ok_shop_room(shop_nr, IN_ROOM(vict)))
              /* Does the shopkeeper prevent stealing? */
              if (!SHOP_ALLOW_STEAL(shop_nr))
                found = TRUE;
        }
      /* Note: found will be true if there the character is in a shop where
       * the shopkeeper present who doesn't allow stealing.  Don't bother 
       * running the next loop, since we can't steal from anyone anyway. 
       */
      }
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (vict == ch)
          continue;
        if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
          continue;
        if (!IS_HUMANOID(vict))
          continue;
        if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL))
          continue;
        if (GET_MOB_VNUM(ch) == GET_MOB_VNUM(vict))
          continue;
        if (GET_LEVEL(ch) >= GET_LEVEL(vict)) {
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

 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) { /* In space.... nobody cares. */
  return;
 }

 if (!FIGHTING(ch)) { /* The NPC is not fighting. Error. ABORT! */
  return;
 }

 struct char_data *vict = FIGHTING(ch);

 if (vict == NULL) { /* OH NO */
  return;
 }

 if (!IS_HUMANOID(ch) && !SUNKEN(IN_ROOM(ch))) { /* They are an animal and they are not in the water. */
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
 } else if (!IS_HUMANOID(ch)) { /* Animal under water */
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
 } else if (!MOB_FLAGGED(ch, MOB_DUMMY)) { /* They are intelligent */
  message = rand_number(1, 10);
  if (!SUNKEN(IN_ROOM(ch))) {
   if (AFF_FLAGGED(ch, AFF_FLYING)) { /* They are flying */
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
		if (IS_ANDROID(ch)) {
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

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present && !MOB_FLAGGED(ch, MOB_SPAR) && !PLR_FLAGGED(victim, PLR_SPAR)) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}


/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}


/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}


/*
 * An aggressive mobile wants to attack something.  If
 * they're under the influence of mind altering PC, then
 * see if their master can talk them out of it, eye them
 * down, or otherwise intimidate the slave.
 */
bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack)
{
  static int snarl_cmd;
  int dieroll;

  if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
    return (FALSE);

  if (!snarl_cmd)
    snarl_cmd = find_command("snarl");

  /* Sit. Down boy! HEEEEeeeel! */
  dieroll = rand_number(1, 20);
  if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
    if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
      char victbuf[MAX_NAME_LENGTH + 1];

      strncpy(victbuf, GET_NAME(attack), sizeof(victbuf));	/* strncpy: OK */
      victbuf[sizeof(victbuf) - 1] = '\0';

      do_action(slave, victbuf, snarl_cmd, 0);
    }

    /* Success! But for how long? Hehe. */
    return (TRUE);
  }

  /* So sorry, now you're a player killer... Tsk tsk. */
  return (FALSE);
}

