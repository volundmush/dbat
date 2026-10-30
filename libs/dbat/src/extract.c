#include "dbat/utils.h"
#include "dbat/act.wizard.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/fight.h"
#include "dbat/act.movement.h"
#include "dbat/config.h"
#include "dbat/mobact.h"
#include "dbat/objsave.h"
#include "dbat/act.misc.h"

int extractions_pending = 0;

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch)
{
  struct char_data *k, *temp;
  struct obj_data *chair;
  struct descriptor_data *d;
  struct obj_data *obj;
  int i;

  if (char_get_in_room(ch) == NOWHERE) {
    log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
        char_get_name(ch), __FILE__);
    exit(1);
  }

  /*
   * We're booting the character of someone who has switched so first we
   * need to stuff them back into their own body.  This will set ch->desc
   * we're checking below this loop to the proper value.
   */
  if (!char_is_npc(ch) && !ch->desc) {
    for (d = descriptor_list; d; d = d->next)
      if (d->original == ch) {
	      do_return(d->character, NULL, 0, 0);
        break;
      }
  }

  if (ch->desc) {
    /*
     * This time we're extracting the body someone has switched into
     * (not the body of someone switching as above) so we need to put
     * the switcher back to their own body.
     *
     * If this body is not possessed, the owner won't have a
     * body after the removal so dump them to the main menu.
     */
    if (ch->desc->original)
      do_return(ch, NULL, 0, 0);
    else {
      /*
       * Now we boot anybody trying to log in with the same character, to
       * help guard against duping.  CON_DISCONNECT is used to close a
       * descriptor without extracting the d->character associated with it,
       * for being link-dead, so we want CON_CLOSE to clean everything up.
       * If we're here, we know it's a player so no IS_NPC check required.
       */
      for (d = descriptor_list; d; d = d->next) {
        if (d == ch->desc)
          continue;
        if (d->character && char_get_idnum(ch) == char_get_idnum(d->character))
          desc_set_connected(d, CON_CLOSE);
      }
      desc_set_connected(ch->desc, CON_MENU);
      write_to_output(ch->desc, "%s", CONFIG_MENU);
    }
  }
  /* On with the character's assets... */

  if (ch->followers || ch->master)
    die_follower(ch);

  if (char_get_sits(ch)) {
   chair = char_get_sits(ch);
   obj_set_sitting(chair, NULL);
   char_set_sits(ch, NULL);
  }

  if (char_is_npc(ch) && char_get_mob_vnum(ch) == 25) {
   if (char_get_original(ch)) {
    handle_multi_merge(ch);
   }
  }

  if (!char_is_npc(ch) && char_get_clones(ch) > 0) {
   struct char_data *clone = NULL;
    for (clone = character_list; clone; clone = clone->next) {
     if (char_is_npc(clone)) {
      if (char_get_mob_vnum(clone) == 25) {
       if (char_get_original(clone) == ch) {
        handle_multi_merge(clone);
       }
      }
     }
    }
  }

  purge_homing(ch);

  if (char_get_mindlink(ch)) {
   char_set_mindlink(char_get_mindlink(ch), NULL);
   char_set_mindlink(ch, NULL);
  }

  if (char_get_grappling(ch)) {
   act("@WYou stop grappling with @C$N@W!@n", TRUE, ch, 0, char_get_grappling(ch), TO_CHAR);
   act("@C$n@W stops grappling with @c$N@W!@n", TRUE, ch, 0, char_get_grappling(ch), TO_ROOM);
   char_set_grap(char_get_grappling(ch), -1);
   char_set_grappled(char_get_grappling(ch), NULL);
   char_set_grappling(ch, NULL);
   char_set_grap(ch, -1);
  }
  if (char_get_grappled(ch)) {
   act("@WYou stop being grappled with by @C$N@W!@n", TRUE, ch, 0, char_get_grappled(ch), TO_CHAR);
   act("@C$n@W stops being grappled with by @c$N@W!@n", TRUE, ch, 0, char_get_grappled(ch), TO_ROOM);
   char_set_grap(char_get_grappled(ch), -1);
   char_set_grappling(char_get_grappled(ch), NULL);
   char_set_grappled(ch, NULL);
   char_set_grap(ch, -1);
  }

  if (CARRYING(ch)) {
   carry_drop(ch, 3);
  }
  if (CARRIED_BY(ch)) {
   carry_drop(CARRIED_BY(ch), 3);
  }

  if (ch->poisonby) {
   ch->poisonby = NULL;
  }

  if (char_get_drag(ch)) {
   act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, char_get_drag(ch), TO_CHAR);
   act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, char_get_drag(ch), TO_ROOM);
   char_set_dragged(char_get_drag(ch), NULL);
   char_set_drag(ch, NULL);
  }

  if (char_get_dragged(ch)) {
   act("@WYou stop being dragged by @C$N@W!@n", TRUE, ch, 0, char_get_dragged(ch), TO_CHAR);
   act("@C$n@W stops being dragged by @c$N@W!@n", TRUE, ch, 0, char_get_dragged(ch), TO_ROOM);
   char_set_drag(char_get_dragged(ch), NULL);
   char_set_dragged(ch, NULL);
  }

  if (char_get_defender(ch)) {
   char_set_defending(char_get_defender(ch), NULL);
   char_set_defender(ch, NULL);
  }
  if (char_get_defending(ch)) {
   char_set_defender(char_get_defending(ch), NULL);
   char_set_defending(ch, NULL);
  }

  if (char_get_blocked(ch)) {
   char_set_blocks(char_get_blocked(ch), NULL);
   char_set_blocked(ch, NULL);
  }
  if (char_get_blocks(ch)) {
   char_set_blocked(char_get_blocks(ch), NULL);
   char_set_blocks(ch, NULL);
  }
  if (char_get_absorbing(ch)) {
   char_set_absorbby(char_get_absorbing(ch), NULL);
   char_set_absorbing(ch, NULL);
  }
  if (char_get_absorbby(ch)) {
   char_set_absorbing(char_get_absorbby(ch), NULL);
   char_set_absorbby(ch, NULL);
  }

  /* transfer objects to room, if any */
  while (ch->carrying) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, char_get_in_room(ch));
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      obj_to_room(unequip_char(ch, i), char_get_in_room(ch));

  if (char_get_fighting(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (char_get_fighting(k) == ch)
      stop_fighting(k);
  }

  char_from_room(ch);

  if (char_is_npc(ch)) {
    if (char_get_nr(ch) != NOTHING)	/* prototyped */
      mob_index[char_get_nr(ch)].number--;
    clearMemory(ch);
    if (SCRIPT(ch))
      extract_script(ch, MOB_TRIGGER);
    if (SCRIPT_MEM(ch))
      extract_script_mem(SCRIPT_MEM(ch));
  } else {
      save_char(ch);
      Crash_delete_crashfile(ch);
  }

  /* If there's a descriptor, they're in the menu now. */
  if (char_is_npc(ch) || !ch->desc)
    free_char(ch);
}


/*
 * Q: Why do we do this?
 * A: Because trying to iterate over the character
 *    list with 'ch = ch->next' does bad things if
 *    the current character happens to die. The
 *    trivial workaround of 'vict = next_vict'
 *    doesn't work if the _next_ person in the list
 *    gets killed, for example, by an area spell.
 *
 * Q: Why do we leave them on the character_list?
 * A: Because code doing 'vict = vict->next' would
 *    get really confused otherwise.
 */
void extract_char(struct char_data *ch)
{
  struct follow_type *foll;
  int i;
  struct obj_data *obj;

  if (char_is_npc(ch)) {
    if (!IS_SET_AR(char_get_act(ch), MOB_NOTDEADYET))
      SET_BIT_AR(char_get_act(ch), MOB_NOTDEADYET);
    else
      return;
  } else {
    if (!IS_SET_AR(char_get_act(ch), PLR_NOTDEADYET))
      SET_BIT_AR(char_get_act(ch), PLR_NOTDEADYET);
    else
      return;
  }

  /*save_char_pets(ch);*/

  for (foll = ch->followers; foll; foll = foll->next) {
    if (char_is_npc(foll->follower) && char_aff_flagged(foll->follower, AFF_CHARM) &&
        (char_get_in_room(foll->follower) == char_get_in_room(ch) || char_get_in_room(ch) == 1)) {
      /* transfer objects to char, if any */
      while (foll->follower->carrying) {
        obj = foll->follower->carrying;
        obj_from_char(obj);
        obj_to_char(obj, ch);
      }

      /* transfer equipment to char, if any */
      for (i = 0; i < NUM_WEARS; i++)
        if (char_get_equipment(foll->follower, i)) {
          obj = unequip_char(foll->follower, i);
          obj_to_char(obj, ch);
        }
      
      extract_char(foll->follower);
    }
  }

  extractions_pending++;
}


/* Extract an object from the world */
void extract_obj(struct obj_data *obj)
{
  struct obj_data *temp;
  struct char_data *ch;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj_get_in_room(obj) != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  /* Get rid of the contents of the object, as well. */
  if (obj_get_fellow_wall(obj) && obj_get_vnum(obj) == 79) {
   struct obj_data *trash;
   trash = obj_get_fellow_wall(obj);
   obj_set_fellow_wall(obj, NULL);
   obj_set_fellow_wall(trash, NULL);
   extract_obj(trash);
  }
  if (obj_get_sitting(obj)) {
   ch = obj_get_sitting(obj);
   obj_set_sitting(obj, NULL);
   char_set_sits(ch, NULL);
  }
  if (obj_get_posted_to(obj) && obj->in_obj == NULL) {
   struct obj_data *obj2 = obj_get_posted_to(obj);
   obj_set_posted_to(obj2, NULL);
   obj_set_posttype(obj2, 0);
   obj_set_posted_to(obj, NULL);
  }
  if (obj_get_target(obj)) {
   obj_set_target(obj, NULL);
  }
  if (obj_get_user(obj)) {
   obj_set_user(obj, NULL);
  }

  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next, temp);

  if (obj_get_item_number(obj) != NOTHING)
    (obj_index[obj_get_item_number(obj)].number)--;

  if (SCRIPT(obj))
     extract_script(obj, OBJ_TRIGGER);

  if (obj_get_vnum(obj) != 80 && obj_get_vnum(obj) != 81) {
   if (obj_get_item_number(obj) == NOTHING || obj->proto_script != obj_proto[obj_get_item_number(obj)].proto_script)
     free_proto_script(obj, OBJ_TRIGGER);
  }

  free_obj(obj);
}



void extract_pending_chars(void)
{
  struct char_data *vict, *next_vict, *prev_vict, *temp;

  if (extractions_pending < 0)
    log("SYSERR: Negative (%d) extractions pending.", extractions_pending);

  for (vict = character_list, prev_vict = NULL; vict && extractions_pending; vict = next_vict) {
    next_vict = vict->next;

    if (char_mob_flagged(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(char_get_act(vict), MOB_NOTDEADYET);
    else if (char_plr_flagged(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(char_get_act(vict), PLR_NOTDEADYET);
    else {
      /* Last non-free'd character to continue chain from. */
      prev_vict = vict;
      continue;
    }

    REMOVE_FROM_LIST(vict, affect_list, next_affect, temp);
    REMOVE_FROM_LIST(vict, affectv_list, next_affectv, temp);
    extract_char_final(vict);
    extractions_pending--;

    if (prev_vict)
      prev_vict->next = next_vict;
    else
      character_list = next_vict;
  }

  if (extractions_pending > 0)
    log("SYSERR: Couldn't find %d extractions as counted.", extractions_pending);

  extractions_pending = 0;
}
