#include "dbat/game/extract.h"


/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch)
{
  struct char_data *k, *temp;
  struct obj_data *chair;
  struct descriptor_data *d;
  struct obj_data *obj;
  int i;

  if (IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
        GET_NAME(ch), __FILE__);
    exit(1);
  }

  /*
   * We're booting the character of someone who has switched so first we
   * need to stuff them back into their own body.  This will set ch->desc
   * we're checking below this loop to the proper value.
   */
  if (!IS_NPC(ch) && !ch->desc) {
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
        if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
          STATE(d) = CON_CLOSE;
      }
      STATE(ch->desc) = CON_MENU;
      write_to_output(ch->desc, "%s", CONFIG_MENU);
    }
  }
  /* On with the character's assets... */

  if (ch->followers || ch->master)
    die_follower(ch);

  if (SITS(ch)) {
   chair = SITS(ch);
   SITTING(chair) = NULL;
   SITS(ch) = NULL;
  }

  if (IS_NPC(ch) && GET_MOB_VNUM(ch) == 25) {
   if (GET_ORIGINAL(ch)) {
    handle_multi_merge(ch);
   }
  }

  if (!IS_NPC(ch) && GET_CLONES(ch) > 0) {
   struct char_data *clone = NULL;
    for (clone = character_list; clone; clone = clone->next) {
     if (IS_NPC(clone)) {
      if (GET_MOB_VNUM(clone) == 25) {
       if (GET_ORIGINAL(clone) == ch) {
        handle_multi_merge(clone);
       }
      }
     }
    }
  }

  purge_homing(ch);

  if (MINDLINK(ch)) {
   MINDLINK(MINDLINK(ch)) = NULL;
   MINDLINK(ch) = NULL;
  }

  if (GRAPPLING(ch)) {
   act("@WYou stop grappling with @C$N@W!@n", TRUE, ch, 0, GRAPPLING(ch), TO_CHAR);
   act("@C$n@W stops grappling with @c$N@W!@n", TRUE, ch, 0, GRAPPLING(ch), TO_ROOM);
   GRAPTYPE(GRAPPLING(ch)) = -1;
   GRAPPLED(GRAPPLING(ch)) = NULL;
   GRAPPLING(ch) = NULL;
   GRAPTYPE(ch) = -1;
  }
  if (GRAPPLED(ch)) {
   act("@WYou stop being grappled with by @C$N@W!@n", TRUE, ch, 0, GRAPPLED(ch), TO_CHAR);
   act("@C$n@W stops being grappled with by @c$N@W!@n", TRUE, ch, 0, GRAPPLED(ch), TO_ROOM);
   GRAPTYPE(GRAPPLED(ch)) = -1;
   GRAPPLING(GRAPPLED(ch)) = NULL;
   GRAPPLED(ch) = NULL;
   GRAPTYPE(ch) = -1;
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

  if (DRAGGING(ch)) {
   act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   DRAGGED(DRAGGING(ch)) = NULL;
   DRAGGING(ch) = NULL;
  }

  if (DRAGGED(ch)) {
   act("@WYou stop being dragged by @C$N@W!@n", TRUE, ch, 0, DRAGGED(ch), TO_CHAR);
   act("@C$n@W stops being dragged by @c$N@W!@n", TRUE, ch, 0, DRAGGED(ch), TO_ROOM);
   DRAGGING(DRAGGED(ch)) = NULL;
   DRAGGED(ch) = NULL;
  }

  if (GET_DEFENDER(ch)) {
   GET_DEFENDING(GET_DEFENDER(ch)) = NULL;
   GET_DEFENDER(ch) = NULL;
  }
  if (GET_DEFENDING(ch)) {
   GET_DEFENDER(GET_DEFENDING(ch)) = NULL;
   GET_DEFENDING(ch) = NULL;
  }

  if (BLOCKED(ch)) {
   BLOCKS(BLOCKED(ch)) = NULL;
   BLOCKED(ch) = NULL;
  }
  if (BLOCKS(ch)) {
   BLOCKED(BLOCKS(ch)) = NULL;
   BLOCKS(ch) = NULL;
  }
  if (ABSORBING(ch)) {
   ABSORBBY(ABSORBING(ch)) = NULL;
   ABSORBING(ch) = NULL;
  }
  if (ABSORBBY(ch)) {
   ABSORBING(ABSORBBY(ch)) = NULL;
   ABSORBBY(ch) = NULL;
  }

  /* transfer objects to room, if any */
  while (ch->carrying) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, IN_ROOM(ch));
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_room(unequip_char(ch, i), IN_ROOM(ch));

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  char_from_room(ch);

  if (IS_NPC(ch)) {
    if (GET_MOB_RNUM(ch) != NOTHING)	/* prototyped */
      mob_index[GET_MOB_RNUM(ch)].number--;
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
  if (IS_NPC(ch) || !ch->desc)
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

  if (IS_NPC(ch)) {
    if (!IS_SET_AR(MOB_FLAGS(ch), MOB_NOTDEADYET))
      SET_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
    else
      return;
  } else {
    if (!IS_SET_AR(PLR_FLAGS(ch), PLR_NOTDEADYET))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_NOTDEADYET);
    else
      return;
  }

  /*save_char_pets(ch);*/

  for (foll = ch->followers; foll; foll = foll->next) {
    if (IS_NPC(foll->follower) && AFF_FLAGGED(foll->follower, AFF_CHARM) &&
        (IN_ROOM(foll->follower) == IN_ROOM(ch) || IN_ROOM(ch) == 1)) {
      /* transfer objects to char, if any */
      while (foll->follower->carrying) {
        obj = foll->follower->carrying;
        obj_from_char(obj);
        obj_to_char(obj, ch);
      }

      /* transfer equipment to char, if any */
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(foll->follower, i)) {
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
  if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  /* Get rid of the contents of the object, as well. */
  if (GET_FELLOW_WALL(obj) && GET_OBJ_VNUM(obj) == 79) {
   struct obj_data *trash;
   trash = GET_FELLOW_WALL(obj);
   GET_FELLOW_WALL(obj) = NULL;
   GET_FELLOW_WALL(trash) = NULL;
   extract_obj(trash);
  }
  if (SITTING(obj)) {
   ch = SITTING(obj);
   SITTING(obj) = NULL;
   SITS(ch) = NULL;
  }
  if (GET_OBJ_POSTED(obj) && obj->in_obj == NULL) {
   struct obj_data *obj2 = GET_OBJ_POSTED(obj);
   GET_OBJ_POSTED(obj2) = NULL;
   GET_OBJ_POSTTYPE(obj2) = 0;
   GET_OBJ_POSTED(obj) = NULL;
  }
  if (TARGET(obj)) {
   TARGET(obj) = NULL;
  }
  if (USER(obj)) {
   USER(obj) = NULL;
  }

  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next, temp);

  if (GET_OBJ_RNUM(obj) != NOTHING)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;

  if (SCRIPT(obj))
     extract_script(obj, OBJ_TRIGGER);

  if (GET_OBJ_VNUM(obj) != 80 && GET_OBJ_VNUM(obj) != 81) {
   if (GET_OBJ_RNUM(obj) == NOTHING || obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
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

    if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_NOTDEADYET);
    else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_NOTDEADYET);
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
