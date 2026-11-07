#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fight.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/config.hpp"
#include "dbat/mobact.hpp"
#include "dbat/objsave.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"


/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(GameWorld *g, struct char_data *ch)
{
  struct char_data *k, *temp;
  struct obj_data *chair;
  struct obj_data *obj;
  int i;

  if (char_get_in_room(ch) == NOWHERE) {
    mud_log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
            char_get_name(ch), __FILE__);
    exit(1);
  }

  /*
   * We're booting the character of someone who has switched so first we
   * need to stuff them back into their own body.  This will set ch->desc
   * we're checking below this loop to the proper value.
   */
  if (!char_is_npc(ch) && !char_get_desc(ch)) {
    for (descriptor_data *d : each_desc(g))
      if (d->original == ch) {
        do_return(g, d->character, nullptr, 0, 0);
        break;
      }
  }

  if (char_get_desc(ch)) {
    /*
     * This time we're extracting the body someone has switched into
     * (not the body of someone switching as above) so we need to put
     * the switcher back to their own body.
     *
     * If this body is not possessed, the owner won't have a
     * body after the removal so dump them to the main menu.
     */
    if (char_get_desc(ch)->original)
      do_return(g, ch, nullptr, 0, 0);
    else {
      /*
       * Now we boot anybody trying to log in with the same character, to
       * help guard against duping.  CON_DISCONNECT is used to close a
       * descriptor without extracting the d->character associated with it,
       * for being link-dead, so we want CON_CLOSE to clean everything up.
       * If we're here, we know it's a player so no IS_NPC check required.
       */
      for (descriptor_data *d : each_desc(g)) {
        if (d == char_get_desc(ch))
          continue;
        if (d->character && char_get_idnum(ch) == char_get_idnum(d->character))
          desc_set_connected(d, CON_CLOSE);
      }
      desc_set_connected(char_get_desc(ch), CON_MENU);
      write_to_output(g, char_get_desc(ch), "%s", CONFIG_MENU);
    }
  }
  /* On with the character's assets... */

  if (char_get_followers(ch) || char_get_master(g, ch))
    die_follower(g, ch);

  if (char_get_sits(g, ch)) {
    chair = char_get_sits(g, ch);
    obj_set_sitting(chair, nullptr);
    char_set_sits(ch, nullptr);
  }

  if (char_is_npc(ch) && char_get_mob_vnum(g, ch) == 25) {
    if (char_get_original(g, ch)) {
      handle_multi_merge(g, ch);
    }
  }

  if (!char_is_npc(ch) && char_get_clones(ch) > 0) {
    for (char_data *clone : each_char(g)) {
      if (char_is_npc(clone)) {
        if (char_get_mob_vnum(g, clone) == 25) {
          if (char_get_original(g, clone) == ch) {
            handle_multi_merge(g, clone);
          }
        }
      }
    }
  }

  purge_homing(g, ch);

  if (char_get_mindlink(g, ch)) {
    char_set_mindlink(char_get_mindlink(g, ch), nullptr);
    char_set_mindlink(ch, nullptr);
  }

  if (char_get_grappling(g, ch)) {
    act(g, "@WYou stop grappling with @C$N@W!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_CHAR);
    act(g, "@C$n@W stops grappling with @c$N@W!@n", true, ch, nullptr, char_get_grappling(g, ch), TO_ROOM);
    char_set_grap(char_get_grappling(g, ch), -1);
    char_set_grappled(char_get_grappling(g, ch), nullptr);
    char_set_grappling(ch, nullptr);
    char_set_grap(ch, -1);
  }
  if (char_get_grappled(g, ch)) {
    act(g, "@WYou stop being grappled with by @C$N@W!@n", true, ch, nullptr, char_get_grappled(g, ch), TO_CHAR);
    act(g, "@C$n@W stops being grappled with by @c$N@W!@n", true, ch, nullptr, char_get_grappled(g, ch), TO_ROOM);
    char_set_grap(char_get_grappled(g, ch), -1);
    char_set_grappling(char_get_grappled(g, ch), nullptr);
    char_set_grappled(ch, nullptr);
    char_set_grap(ch, -1);
  }

  if (char_get_carrying(g, ch)) {
    carry_drop(g, ch, 3);
  }
  if (char_get_carried_by(g, ch)) {
    carry_drop(g, char_get_carried_by(g, ch), 3);
  }

  if (char_get_poisonby(g, ch)) {
    char_set_poisonby(ch, nullptr);
  }

  if (char_get_drag(g, ch)) {
    act(g, "@WYou stop dragging @C$N@W!@n", true, ch, nullptr, char_get_drag(g, ch), TO_CHAR);
    act(g, "@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, char_get_drag(g, ch), TO_ROOM);
    char_set_dragged(char_get_drag(g, ch), nullptr);
    char_set_drag(ch, nullptr);
  }

  if (char_get_dragged(g, ch)) {
    act(g, "@WYou stop being dragged by @C$N@W!@n", true, ch, nullptr, char_get_dragged(g, ch), TO_CHAR);
    act(g, "@C$n@W stops being dragged by @c$N@W!@n", true, ch, nullptr, char_get_dragged(g, ch), TO_ROOM);
    char_set_drag(char_get_dragged(g, ch), nullptr);
    char_set_dragged(ch, nullptr);
  }

  if (char_get_defender(g, ch)) {
    char_set_defending(char_get_defender(g, ch), nullptr);
    char_set_defender(ch, nullptr);
  }
  if (char_get_defending(g, ch)) {
    char_set_defender(char_get_defending(g, ch), nullptr);
    char_set_defending(ch, nullptr);
  }

  if (char_get_blocked(g, ch)) {
    char_set_blocks(char_get_blocked(g, ch), nullptr);
    char_set_blocked(ch, nullptr);
  }
  if (char_get_blocks(g, ch)) {
    char_set_blocked(char_get_blocks(g, ch), nullptr);
    char_set_blocks(ch, nullptr);
  }
  if (char_get_absorbing(g, ch)) {
    char_set_absorbby(char_get_absorbing(g, ch), nullptr);
    char_set_absorbing(ch, nullptr);
  }
  if (char_get_absorbby(g, ch)) {
    char_set_absorbing(char_get_absorbby(g, ch), nullptr);
    char_set_absorbby(ch, nullptr);
  }

  /* transfer objects to room, if any */
  while (char_get_inventory(ch)) {
    obj = char_get_inventory(ch);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      obj_to_room(g, unequip_char(g, ch, i), char_get_in_room(ch));

  if (char_get_fighting(g, ch))
    stop_fighting(g, ch);

  for (k = g->combat_list; k; k = temp) {
    temp = char_get_next_fighting(k);
    if (char_get_fighting(g, k) == ch)
      stop_fighting(g, k);
  }

  char_from_room(g, ch);

  if (char_is_npc(ch)) {
    if (struct index_data *index = mob_index_of(g, char_get_nr(ch))) /* prototyped, and still is */
      index->number--;
    clearMemory(ch);
    if (char_get_script(ch))
      extract_script(g, ch, MOB_TRIGGER);
    if (char_get_memory(ch))
      extract_script_mem(char_get_memory(ch));
  } else {
    save_char(g, ch);
    Crash_delete_crashfile(ch);
  }

  /* If there's a descriptor, they're in the menu now. */
  if (char_is_npc(ch) || !char_get_desc(ch))
    free_char(g, ch);
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
 * Q: Why do we leave them on the character list?
 * A: Because code doing 'vict = vict->next' would
 *    get really confused otherwise.
 */
void extract_char(GameWorld *g, struct char_data *ch)
{
  int i;
  struct obj_data *obj;

  if (char_is_npc(ch)) {
    if (!char_mob_flagged(ch, MOB_NOTDEADYET))
      char_mob_set(ch, MOB_NOTDEADYET, true);
    else
      return;
  } else {
    if (!char_plr_flagged(ch, PLR_NOTDEADYET))
      char_plr_set(ch, PLR_NOTDEADYET, true);
    else
      return;
  }

  /*save_char_pets(ch);*/

  for (char_data *fol : char_followers(g, ch)) {
    if (char_is_npc(fol) && char_aff_flagged(fol, AFF_CHARM) &&
        (char_get_in_room(fol) == char_get_in_room(ch) || char_get_in_room(ch) == 1)) {
      /* transfer objects to char, if any */
      while (char_get_inventory(fol)) {
        obj = char_get_inventory(fol);
        obj_from_char(g, obj);
        obj_to_char(g, obj, ch);
      }

      /* transfer equipment to char, if any */
      for (i = 0; i < NUM_WEARS; i++)
        if (char_get_equipment(fol, i)) {
          obj = unequip_char(g, fol, i);
          obj_to_char(g, obj, ch);
        }

      extract_char(g, fol);
    }
  }

  g->extractions_pending++;
}

/* Extract an object from the world */
void extract_obj(GameWorld *g, struct obj_data *obj)
{
  struct char_data *ch;

  if (obj_get_worn_by(obj) != nullptr)
    if (unequip_char(g, obj_get_worn_by(obj), obj_get_worn_on(obj)) != obj)
      mud_log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj_get_in_room(obj) != NOWHERE)
    obj_from_room(g, obj);
  else if (obj_get_carried_by(obj))
    obj_from_char(g, obj);
  else if (obj_get_in_obj(obj))
    obj_from_obj(g, obj);

  /* Get rid of the contents of the object, as well. */
  if (obj_get_fellow_wall(g, obj) && obj_get_vnum(g, obj) == 79) {
    struct obj_data *trash;
    trash = obj_get_fellow_wall(g, obj);
    obj_set_fellow_wall(obj, nullptr);
    obj_set_fellow_wall(trash, nullptr);
    extract_obj(g, trash);
  }
  if (obj_get_sitting(g, obj)) {
    ch = obj_get_sitting(g, obj);
    obj_set_sitting(obj, nullptr);
    char_set_sits(ch, nullptr);
  }
  if (obj_get_posted_to(g, obj) && obj_get_in_obj(obj) == nullptr) {
    struct obj_data *obj2 = obj_get_posted_to(g, obj);
    obj_set_posted_to(obj2, nullptr);
    obj_set_posttype(obj2, 0);
    obj_set_posted_to(obj, nullptr);
  }
  if (obj_get_target(g, obj)) {
    obj_set_target(obj, nullptr);
  }
  if (obj_get_user(g, obj)) {
    obj_set_user(obj, nullptr);
  }

  while (obj_get_contains(obj))
    extract_obj(g, obj_get_contains(obj));

  obj_unlink_from_list(g, obj);

  if (obj_get_item_number(obj) != NOTHING)
    (obj_index_of(g, obj_get_item_number(obj))->number)--;

  if (obj_get_script(obj))
    extract_script(g, obj, OBJ_TRIGGER);

  if (obj_get_vnum(g, obj) != 80 && obj_get_vnum(g, obj) != 81) {
    if (obj_get_item_number(obj) == NOTHING || obj_get_proto_script(obj) != obj_get_proto_script(obj_proto_of(g, obj_get_item_number(obj))))
      free_proto_script(obj, OBJ_TRIGGER);
  }

  free_obj(g, obj);
}

void extract_pending_chars(GameWorld *g)
{
  free_extracted_triggers(g);

  struct char_data *vict, *next_vict, *prev_vict;

  if (g->extractions_pending < 0)
    mud_log("SYSERR: Negative (%d) extractions pending.", g->extractions_pending);

  for (vict = g->characters, prev_vict = nullptr; vict && g->extractions_pending; vict = next_vict) {
    next_vict = char_get_next(vict);

    if (char_mob_flagged(vict, MOB_NOTDEADYET))
      char_mob_set(vict, MOB_NOTDEADYET, false);
    else if (char_plr_flagged(vict, PLR_NOTDEADYET))
      char_plr_set(vict, PLR_NOTDEADYET, false);
    else {
      /* Last non-free'd character to continue chain from. */
      prev_vict = vict;
      continue;
    }

    char_unlink_from_affect_list(g, vict);
    extract_char_final(g, vict);
    g->extractions_pending--;

    if (prev_vict)
      char_set_next(prev_vict, next_vict);
    else
      g->characters = next_vict;
  }

  if (g->extractions_pending > 0)
    mud_log("SYSERR: Couldn't find %d extractions as counted.", g->extractions_pending);

  g->extractions_pending = 0;
}
