#include "dbat/relocate.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/vehicles.h"
#include "dbat/fight.h"
#include "dbat/affect.h"

/* put an object in a room */
void obj_to_room(struct obj_data *object, room_rnum room)
{
  struct obj_data *vehicle = NULL;

  if (!object || room == NOWHERE || room > top_of_world)
    mud_log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
            room, top_of_world, object);
  else {
    if (rroom_flagged(room, ROOM_GARDEN1) || rroom_flagged(room, ROOM_GARDEN2)) {
      if (obj_get_type_flag(object) != ITEM_PLANT) {
        send_to_room(room, "%s @wDisappears in a puff of smoke! It seems the room was designed to vaporize anything not plant related. Strange...@n\r\n", object->short_description);
        extract_obj(object);
        return;
      }
    }
    if (room == real_room(80)) {
      auc_load(object);
    }
    object->next_content = world[room].contents;
    world[room].contents = object;
    obj_set_in_room(object, room);
    object->carried_by = NULL;
    obj_set_lload(object, time(0));
    if (obj_get_type_flag(object) == ITEM_VEHICLE && !obj_extra_flagged(object, ITEM_UNBREAKABLE) && obj_get_vnum(object) > 19199) {
      obj_extra_set(object, ITEM_UNBREAKABLE, true);
    }
    if (obj_get_type_flag(object) == ITEM_HATCH && obj_get_vnum(object) <= 19199) {
      if ((obj_get_vnum(object) <= 18999 && obj_get_vnum(object) >= 18800) || (obj_get_vnum(object) <= 19199 && obj_get_vnum(object) >= 19100)) {
        int hnum               = obj_get_value(object, 0);
        struct obj_data *house = read_object(hnum, VIRTUAL);
        obj_to_room(house, real_room(obj_get_value(object, 6)));
        obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_CLOSED));
        obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_LOCKED));
      }
    }
    if (obj_get_type_flag(object) == ITEM_HATCH && obj_get_value(object, 0) > 1 && obj_get_vnum(object) > 19199) {
      if (!(vehicle = find_vehicle_by_vnum(obj_get_value(object, VAL_HATCH_DEST)))) {
        if (real_room(obj_get_value(object, 3)) != NOWHERE) {
          vehicle = read_object(obj_get_value(object, 0), VIRTUAL);
          obj_to_room(vehicle, real_room(obj_get_value(object, 3)));
          if (object->action_description) {
            if (strlen(object->action_description)) {
              char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH], nick3[MAX_INPUT_LENGTH];
              if (obj_get_vnum(vehicle) <= 46099 && obj_get_vnum(vehicle) >= 46000) {
                sprintf(nick, "Saiyan Pod %s", object->action_description);
                sprintf(nick2, "@wA @Ys@ya@Yi@yy@Ya@yn @Dp@Wo@Dd@w named @D(@C%s@D)@w", object->action_description);
              } else if (obj_get_vnum(vehicle) >= 46100 && obj_get_vnum(vehicle) <= 46199) {
                sprintf(nick, "EDI Xenofighter MK. II %s", object->action_description);
                sprintf(nick2, "@wAn @YE@yD@YI @CX@ce@Wn@Do@Cf@ci@Wg@Dh@Wt@ce@Cr @RMK. II @wnamed @D(@C%s@D)@w", object->action_description);
              }
              snprintf(nick3, sizeof(nick3), "%s is resting here@w", nick2);
              vehicle->name              = strdup(nick);
              vehicle->short_description = strdup(nick2);
              vehicle->description       = strdup(nick3);
            }
          }
          obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_CLOSED));
          obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_LOCKED));
        } else {
          mud_log("Hatch load: Hatch with no vehicle load room: #%d!", obj_get_vnum(object));
        }
      }
    }
    if (obj_get_exit(object, 5) && (rroom_sect(obj_get_in_room(object)) == SECT_UNDERWATER || rroom_sect(obj_get_in_room(object)) == SECT_WATER_NOSWIM)) {
      act("$p @Bsinks to deeper waters.@n", TRUE, 0, object, 0, TO_ROOM);
      int numb = rroom_get_vnum(obj_get_exit(object, 5)->to_room);
      obj_from_room(object);
      obj_to_room(object, real_room(numb));
    }
    if (obj_get_exit(object, 5) && rroom_sect(obj_get_in_room(object)) == SECT_FLYING && (obj_get_vnum(object) < 80 || obj_get_vnum(object) > 83)) {
      act("$p @Cfalls down.@n", TRUE, 0, object, 0, TO_ROOM);
      int numb = rroom_get_vnum(obj_get_exit(object, 5)->to_room);
      obj_from_room(object);
      obj_to_room(object, real_room(numb));
      if (rroom_sect(obj_get_in_room(object)) != SECT_FLYING) {
        act("$p @Cfalls down and smacks the ground.@n", TRUE, 0, object, 0, TO_ROOM);
      }
    }
    if (obj_get_value(object, 0) != 0) {
      if (obj_get_vnum(object) == 16705 || obj_get_vnum(object) == 16706 || obj_get_vnum(object) == 16707) {
        object->level = obj_get_value(object, 0);
      }
    }
    if (rroom_flagged(room, ROOM_HOUSE))
      rroom_set_flag(room, ROOM_HOUSE_CRASH, true);
  }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object)
{
  struct obj_data *temp;

  if (!object || obj_get_in_room(object) == NOWHERE) {
    mud_log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
            object, obj_get_in_room(object));
    return;
  }

  if (obj_get_posted_to(object) && object->in_obj == NULL) {
    struct obj_data *obj = obj_get_posted_to(object);
    if (obj_get_posttype(object) <= 0) {
      send_to_room(obj_get_in_room(obj), "%s@W shakes loose from %s@W.@n\r\n", obj->short_description, object->short_description);
    } else {
      send_to_room(obj_get_in_room(obj), "%s@W comes loose from %s@W.@n\r\n", object->short_description, obj->short_description);
    }
    obj_set_posted_to(obj, NULL);
    obj_set_posttype(obj, 0);
    obj_set_posted_to(object, NULL);
    obj_set_posttype(object, 0);
  }

  REMOVE_FROM_LIST(object, world[obj_get_in_room(object)].contents, next_content, temp);

  if (rroom_flagged(obj_get_in_room(object), ROOM_HOUSE))
    rroom_set_flag(obj_get_in_room(object), ROOM_HOUSE_CRASH, true);
  obj_set_in_room(object, NOWHERE);
  object->next_content = NULL;
}

/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    mud_log("SYSERR: NULL object (%p) or same source (%p) and target (%p VNUM: %d) obj passed to obj_to_obj.",
            obj, obj, obj_to, obj_to ? obj_get_vnum(obj_to) : -1);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains  = obj;
  obj->in_obj       = obj_to;
  tmp_obj           = obj->in_obj;

  /* Only add weight to container, or back to carrier for non-eternal
     containers.  Eternal means max capacity < 0 */
  if (obj_get_value(obj->in_obj, VAL_CONTAINER_CAPACITY) > 0) {
    for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      obj_set_weight(tmp_obj, obj_get_weight(tmp_obj) + (obj_get_weight(obj)));

    /* top level object.  Subtract weight from inventory if necessary. */
    obj_set_weight(tmp_obj, obj_get_weight(tmp_obj) + (obj_get_weight(obj)));
    if (tmp_obj->carried_by)
      char_set_carry_weight(tmp_obj->carried_by, char_get_carry_weight(tmp_obj->carried_by) + (obj_get_weight(obj)));
  }
  if (obj_get_in_room(obj_to) != NOWHERE && rroom_flagged(obj_get_in_room(obj_to), ROOM_HOUSE)) {
    rroom_set_flag(obj_get_in_room(obj_to), ROOM_HOUSE_CRASH, true);
  }
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    mud_log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  temp     = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content, temp);

  /* Subtract weight from containers container */
  /* Only worry about weight for non-eternal containers
     Eternal means max capacity < 0 */
  if (obj_get_value(obj->in_obj, VAL_CONTAINER_CAPACITY) > 0) {
    for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      obj_set_weight(temp, obj_get_weight(temp) - (obj_get_weight(obj)));

    /* Subtract weight from char that carries the object */
    obj_set_weight(temp, obj_get_weight(temp) - (obj_get_weight(obj)));
    if (temp->carried_by)
      char_set_carry_weight(temp->carried_by, char_get_carry_weight(temp->carried_by) - (obj_get_weight(obj)));
  }

  if (obj_get_in_room(obj_from) != NOWHERE && rroom_flagged(obj_get_in_room(obj_from), ROOM_HOUSE)) {
    rroom_set_flag(obj_get_in_room(obj_from), ROOM_HOUSE_CRASH, true);
  }

  obj->in_obj       = NULL;
  obj->next_content = NULL;
}

/* move a player out of a room */
void char_from_room(struct char_data *ch)
{
  struct char_data *temp;
  int i;

  if (ch == NULL || char_get_in_room(ch) == NOWHERE) {
    mud_log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    return;
  }

  if (char_get_fighting(ch) != NULL && !char_aff_flagged(ch, AFF_PURSUIT))
    stop_fighting(ch);
  if (char_aff_flagged(ch, AFF_PURSUIT) && char_get_fighting(ch) == NULL)
    char_aff_set(ch, AFF_PURSUIT, false);

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i) != NULL)
      if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT)
        if (obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS))
          world[char_get_in_room(ch)].light--;

  if (char_plr_flagged(ch, PLR_AURALIGHT))
    world[char_get_in_room(ch)].light--;

  REMOVE_FROM_LIST(ch, world[char_get_in_room(ch)].people, next_in_room, temp);
  char_set_in_room(ch, NOWHERE);
  ch->next_in_room = NULL;
}

/* place a character in a room */
void char_to_room(struct char_data *ch, room_rnum room)
{
  int i;

  if (ch == NULL || room == NOWHERE || room > top_of_world)
    mud_log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
            room, top_of_world, ch);
  else {
    ch->next_in_room   = world[room].people;
    world[room].people = ch;
    char_set_in_room(ch, room);

    for (i = 0; i < NUM_WEARS; i++)
      if (char_get_equipment(ch, i))
        if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT)
          if (obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS))
            world[room].light++;

    if (char_plr_flagged(ch, PLR_AURALIGHT))
      world[room].light++;

    /* Stop fighting now, if we left. */
    if (char_get_fighting(ch) && char_get_in_room(ch) != char_get_in_room(char_get_fighting(ch)) && !char_aff_flagged(ch, AFF_PURSUIT)) {
      stop_fighting(char_get_fighting(ch));
      stop_fighting(ch);
    }
    if (!char_is_npc(ch)) {
      if (char_prf_flagged(ch, PRF_ARENAWATCH)) {
        char_prf_set(ch, PRF_ARENAWATCH, false);
        char_set_arenawatch(ch, -1);
      }
    }
  }
}

/* give an object to a char   */
void obj_to_char(struct obj_data *object, struct char_data *ch)
{
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying         = object;
    object->carried_by   = ch;
    obj_set_in_room(object, NOWHERE);
    char_set_carry_weight(ch, char_get_carry_weight(ch) + (obj_get_weight(object)));
    char_set_carry_items(ch, char_get_carry_items(ch) + 1);
    if ((char_get_kaioken(ch) <= 0 && !char_aff_flagged(ch, AFF_METAMORPH)) && !obj_extra_flagged(object, ITEM_THROW)) {

    } else if (GET_HIT(ch) > getEffMaxPL(ch)) {
      if (char_get_kaioken(ch) > 0) {
        send_to_char(ch, "@RThe strain of the weight has reduced your kaioken somewhat!@n\n");
      } else if (char_aff_flagged(ch, AFF_METAMORPH)) {
        send_to_char(ch, "@RYour metamorphosis strains under the additional weight!@n\n");
      }
    }

    /* set flag for crash-save system, but not on mobs! */
    if (obj_get_value(object, 0) != 0) {
      if (obj_get_vnum(object) == 16705 || obj_get_vnum(object) == 16706 || obj_get_vnum(object) == 16707) {
        object->level = obj_get_value(object, 0);
      }
    }
    if (!char_is_npc(ch))
      char_plr_set(ch, PLR_CRASH, true);
  } else
    mud_log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

/* take an object from a char */
void obj_from_char(struct obj_data *object)
{
  struct obj_data *temp;

  if (object == NULL) {
    mud_log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content, temp);

  /* set flag for crash-save system, but not on mobs! */
  if (!char_is_npc(object->carried_by))
    char_plr_set(object->carried_by, PLR_CRASH, true);

  char_set_carry_weight(object->carried_by, char_get_carry_weight(object->carried_by) - (obj_get_weight(object)));
  char_set_carry_items(object->carried_by, char_get_carry_items(object->carried_by) - 1);

  if (obj_get_value(object, 0) != 0) {
    if (obj_get_vnum(object) == 16705 || obj_get_vnum(object) == 16706 || obj_get_vnum(object) == 16707) {
      object->level = obj_get_value(object, 0);
    }
  }

  object->carried_by   = NULL;
  object->next_content = NULL;
}

/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(struct char_data *ch, int eq_pos)
{
  if (char_get_equipment(ch, eq_pos) == NULL) {
    core_dump();
    return (0);
  }

  if (!(obj_get_type_flag(char_get_equipment(ch, eq_pos)) == ITEM_ARMOR))
    return (0);

  /* The following code is an example of how to make the WEAR_ position of the
   * armor apply MORE AC value based on 'factor' then it's assigned value.
   * IE: An object with an AC value of 5 and a factor of 3 really gives 15 AC not 5.
   
  int factor;

  switch (eq_pos) {

  case WEAR_BODY:
    factor = 3;
    break;
  case WEAR_HEAD:
  case WEAR_LEGS:
    factor = 1;
    break;
  default:
    factor = 1;
    break;
  }

  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), VAL_ARMOR_APPLYAC)); */
  return (obj_get_value(char_get_equipment(ch, eq_pos), VAL_ARMOR_APPLYAC));
}

int invalid_align(struct char_data *ch, struct obj_data *obj)
{
  if (obj_extra_flagged(obj, ITEM_ANTI_EVIL) && char_is_evil(ch))
    return TRUE;
  if (obj_extra_flagged(obj, ITEM_ANTI_GOOD) && char_is_good(ch))
    return TRUE;
  if (obj_extra_flagged(obj, ITEM_ANTI_NEUTRAL) && char_is_neutral(ch))
    return TRUE;
  return FALSE;
}

void equip_char(struct char_data *ch, struct obj_data *obj, int pos)
{
  int j;

  if (pos < 0 || pos >= NUM_WEARS) {
    core_dump();
    return;
  }

  if (char_get_equipment(ch, pos)) {
    mud_log("SYSERR: Char is already equipped: %s, %s", char_get_name(ch),
            obj->short_description);
    return;
  }
  if (obj->carried_by) {
    mud_log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (obj_get_in_room(obj) != NOWHERE) {
    mud_log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj)) {
    act("You stop wearing $p as something prevents you.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops wearing $p as something prevents $m.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch);
    return;
  }

  char_set_equipment(ch, pos, obj);
  obj->worn_by = ch;
  obj->worn_on = pos;

  if (obj_get_type_flag(obj) == ITEM_ARMOR)
    char_set_armor(ch, char_get_armor(ch) + (apply_ac(ch, pos)));

  if (char_get_in_room(ch) != NOWHERE) {
    if (obj_get_type_flag(obj) == ITEM_LIGHT)
      if (obj_get_value(obj, VAL_LIGHT_HOURS)) /* if light is ON */
        world[char_get_in_room(ch)].light++;
  } else
    mud_log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", char_get_name(ch));

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(ch, obj->affected[j].location,
                     obj->affected[j].modifier,
                     obj->affected[j].specific,
                     obj_get_bitvector(obj), TRUE);

  affect_total(ch);
}

struct obj_data *unequip_char(struct char_data *ch, int pos)
{
  int j;
  struct obj_data *obj;

  if ((pos < 0 || pos >= NUM_WEARS) || char_get_equipment(ch, pos) == NULL) {
    core_dump();
    return (NULL);
  }

  obj          = char_get_equipment(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (obj_get_type_flag(obj) == ITEM_ARMOR)
    char_set_armor(ch, char_get_armor(ch) - (apply_ac(ch, pos)));

  if (char_get_in_room(ch) != NOWHERE) {
    if (obj_get_type_flag(obj) == ITEM_LIGHT)
      if (obj_get_value(obj, VAL_LIGHT_HOURS)) /* if light is ON */
        world[char_get_in_room(ch)].light--;
  } else
    mud_log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", char_get_name(ch));

  char_set_equipment(ch, pos, NULL);

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(ch, obj->affected[j].location,
                     obj->affected[j].modifier,
                     obj->affected[j].specific,
                     obj_get_bitvector(obj), FALSE);

  affect_total(ch);

  return (obj);
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data *list, struct char_data *ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}
