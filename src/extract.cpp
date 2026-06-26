#include "extract.h"
#include "act.misc.h"
#include "act.wizard.h"
#include "comm.h"
#include "config.h"
#include "dg_scripts.h"
#include "fight.h"
#include "mobact.h"
#include "objsave.h"

#include "character_api.h"
#include "character_db.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/constates.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/triggers.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dg_scripts.h"
#include "fight.h"
#include "flags.h"
#include "handler.h"
#include "log.h"
#include "object_api.h"
#include "object_db.h"
#include "iterate.hpp"
#include "object_impl.h"
#include "object_macros.h"
#include "relocate.h"
#include "room_api.h"
#include "room_impl.h"
#include "room_utils.h"
#include "util_macros.h"

#include <cstdlib>


/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch) {
  struct char_data *k, *temp;
  struct descriptor_data *d;
  struct obj_data *obj;

  if (char_room_get(ch) == NULL) {
    mud_log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
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

  if (char_follower_count(ch) || MASTER(ch))
    die_follower(ch);

  if (auto chair = SITS(ch)) {
    SITTING(chair) = NULL;
    SITS(ch) = NULL;
  }

  if (IS_NPC(ch) && GET_MOB_VNUM(ch) == 25) {
    if (GET_ORIGINAL(ch)) {
      handle_multi_merge(ch);
    }
  }

  if (!IS_NPC(ch) && char_clone_count(ch) > 0) {
    size_t clone_count = 0;
    auto clone_ids = char_clone_ids(ch, &clone_count);
    for (size_t i = 0; i < clone_count; i++) {
      auto clone = char_by_id(clone_ids[i]);
      if (clone) extract_char(clone);
    }
    if (clone_ids) free(clone_ids);
  }

  purge_homing(ch);

  if (MINDLINK(ch)) {
    struct char_data *other = MINDLINK(ch);
    char_mindlinked_set(ch, NULL);
    char_mindlinked_set(other, NULL);
  }

  if (GRAPPLING(ch)) {
    act("@WYou stop grappling with @C$N@W!@n", TRUE, ch, 0, GRAPPLING(ch),
        TO_CHAR);
    act("@C$n@W stops grappling with @c$N@W!@n", TRUE, ch, 0, GRAPPLING(ch),
        TO_ROOM);
    struct char_data *other = GRAPPLING(ch);
    char_condition_remove(ch, "grappling", "grapple_end");
    char_condition_remove(other, "grappled", "grapple_end");
    char_grappling_set(ch, NULL, 0);
    char_grappled_set(other, NULL, 0);
  }
  if (GRAPPLED(ch)) {
    act("@WYou stop being grappled with by @C$N@W!@n", TRUE, ch, 0,
        GRAPPLED(ch), TO_CHAR);
    act("@C$n@W stops being grappled with by @c$N@W!@n", TRUE, ch, 0,
        GRAPPLED(ch), TO_ROOM);
    struct char_data *other = GRAPPLED(ch);
    char_condition_remove(other, "grappling", "grapple_end");
    char_condition_remove(ch, "grappled", "grapple_end");
    char_grappled_set(ch, NULL, 0);
    char_grappling_set(other, NULL, 0);
  }

  if (CARRYING(ch)) {
    carry_drop(ch, 3);
  }
  if (CARRIED_BY(ch)) {
    carry_drop(CARRIED_BY(ch), 3);
  }

  if (DRAGGING(ch)) {
    act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
    act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
    struct char_data *other = DRAGGING(ch);
    char_dragging_set(ch, NULL);
    char_being_dragged_set(other, NULL);
  }

  if (DRAGGED(ch)) {
    act("@WYou stop being dragged by @C$N@W!@n", TRUE, ch, 0, DRAGGED(ch),
        TO_CHAR);
    act("@C$n@W stops being dragged by @c$N@W!@n", TRUE, ch, 0, DRAGGED(ch),
        TO_ROOM);
    struct char_data *other = DRAGGED(ch);
    char_being_dragged_set(ch, NULL);
    char_dragging_set(other, NULL);
  }

  if (GET_DEFENDER(ch)) {
    struct char_data *other = GET_DEFENDER(ch);
    char_defending_for_set(ch, NULL);
    char_defended_by_set(other, NULL);
  }
  if (GET_DEFENDING(ch)) {
    struct char_data *other = GET_DEFENDING(ch);
    char_defended_by_set(ch, NULL);
    char_defending_for_set(other, NULL);
  }

  if (BLOCKED(ch)) {
    struct char_data *other = BLOCKED(ch);
    char_blocked_by_set(ch, NULL);
    char_blocking_set(other, NULL);
  }
  if (BLOCKS(ch)) {
    struct char_data *other = BLOCKS(ch);
    char_blocking_set(ch, NULL);
    char_blocked_by_set(other, NULL);
  }
  if (ABSORBING(ch)) {
    struct char_data *other = ABSORBING(ch);
    char_absorbing_set(ch, NULL);
    char_absorbed_by_set(other, NULL);
  }
  if (ABSORBBY(ch)) {
    struct char_data *other = ABSORBBY(ch);
    char_absorbed_by_set(ch, NULL);
    char_absorbing_set(other, NULL);
  }

  /* transfer objects to room, if any */
  char_inventory_iterate(ch, [&](auto obj) {
    obj_from_char(obj);
    obj_to_room(obj, char_room_get(ch));
    return true;
  });

  /* transfer equipment to room, if any */
  char_equipment_iterate(ch, [&](auto i, auto eq) {
    obj_to_room(unequip_char(ch, i), char_room_get(ch));
    return true;
  });

  if (FIGHTING(ch))
    stop_fighting(ch);

  char_iterate_subscriptions("combat", [&](auto k) {
    if (FIGHTING(k) == ch)
      stop_fighting(k);
    return true;
  });

  char_game_deactivate(ch);
  char_from_room(ch);

  if (IS_NPC(ch)) {
    if (ch->proto_id != NOTHING) /* prototyped */
      mob_proto_count_decrement(ch->proto_id);
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
 * Extraction is deferred to end-of-tick: the character is flagged NOTDEADYET
 * and its id queued, so any code still holding the pointer this tick stays
 * valid. extract_pending_chars() re-resolves each id and finalizes.
 */
void extract_char(struct char_data *ch) {
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
    game_active_player_leave();
  }

  char_followers_iterate(ch, [&](struct char_data *foll) {
    if (IS_NPC(foll) && AFF_FLAGGED(foll, AFF_CHARM) &&
        (char_room_get(foll) == char_room_get(ch) ||
         char_room_vnum_get(ch) == 1)) {
      /* transfer objects to char, if any */
      char_inventory_iterate(foll, [&](auto o) {
        obj_from_char(o);
        obj_to_char(o, ch);
        return true;
      });

      /* transfer equipment to char, if any */
      char_equipment_iterate(foll, [&](auto i, auto eq) {
        obj = unequip_char(foll, i);
        obj_to_char(obj, ch);
        return true;
      });

      extract_char(foll);
    }
    return true;
  });

  char_extract_pending_add(GET_ID(ch));
}

/* Extract an object from the world */
void extract_obj(struct obj_data *obj) {
  struct obj_data *temp;
  struct char_data *ch;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      mud_log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj_room_get(obj) != NULL)
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

  obj_contents_iterate(obj, [&](struct obj_data *o) {
    extract_obj(o);
    return true;
  });

  obj_game_deactivate(obj);

  if (GET_OBJ_VNUM(obj) != NOTHING)
    obj_proto_count_decrement(GET_OBJ_VNUM(obj));

  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  if (obj->proto_script)
    free_proto_script(obj, OBJ_TRIGGER);

  free_obj(obj);
}

void extract_pending_chars(void) {
  struct char_data *temp;
  size_t count;
  int64_t *ids = char_extract_pending_take(&count);

  for (size_t i = 0; i < count; i++) {
    struct char_data *vict = char_by_id(ids[i]);
    if (!vict)
      continue;

    if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_NOTDEADYET);
    else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_NOTDEADYET);
    else
      continue; /* extraction was rescinded since queueing */

    extract_char_final(vict);
  }

  char_subscribe_ids_free(ids);
}
