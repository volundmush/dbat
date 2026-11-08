#include "dbat/relocate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/fight.hpp"
#include "dbat/affect.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"

/* Containers list their members by id, so only a registered entity can be found in one. */
static void check_registered(GameWorld *g, const struct obj_data *obj, const char *where)
{
  if (obj_by_id(g, obj_get_id(obj)) != obj)
    mud_log("SYSERR: %s: object %d (id %d) isn't registered, so it can't be listed there.", where,
            obj_get_vnum(g, obj), obj_get_id(obj));
}

/* put an object in a room */
void obj_to_room(GameWorld *g, struct obj_data *object, room_vnum room)
{
  struct obj_data *vehicle = nullptr;

  if (!object || !room_of(g, room))
    mud_log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d, obj %p)", room, (void *)object);
  else {
    if (room_flagged(g, room, ROOM_GARDEN1) || room_flagged(g, room, ROOM_GARDEN2)) {
      if (obj_get_type_flag(object) != ITEM_PLANT) {
        send_to_room(g, room, "%s @wDisappears in a puff of smoke! It seems the room was designed to vaporize anything not plant related. Strange...@n\r\n", obj_get_short_description(object));
        extract_obj(g, object);
        return;
      }
    }
    if (room == 80) {
      auc_load(object);
    }
    check_registered(g, object, "obj_to_room");
    room_contents_slot(g, room)->add(obj_get_id(object));
    obj_set_in_room(object, room);
    obj_set_carried_by(object, nullptr);
    obj_set_lload(object, game_now(g));
    if (obj_get_type_flag(object) == ITEM_VEHICLE && !obj_extra_flagged(object, ITEM_UNBREAKABLE) && obj_get_vnum(g, object) > 19199) {
      obj_extra_set(object, ITEM_UNBREAKABLE, true);
    }
    if (obj_get_type_flag(object) == ITEM_HATCH && obj_get_vnum(g, object) <= 19199) {
      if ((obj_get_vnum(g, object) <= 18999 && obj_get_vnum(g, object) >= 18800) || (obj_get_vnum(g, object) <= 19199 && obj_get_vnum(g, object) >= 19100)) {
        obj_vnum hnum          = obj_get_value(object, 0);
        struct obj_data *house = read_object(g, hnum);
        obj_to_room(g, house, obj_get_value(object, 6));
        obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_CLOSED));
        obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_LOCKED));
      }
    }
    if (obj_get_type_flag(object) == ITEM_HATCH && obj_get_value(object, 0) > 1 && obj_get_vnum(g, object) > 19199) {
      if (!(vehicle = find_vehicle_by_vnum(g, obj_get_value(object, VAL_HATCH_DEST)))) {
        if (room_of(g, obj_get_value(object, 3))) {
          vehicle = read_object(g, obj_get_value(object, 0));
          obj_to_room(g, vehicle, obj_get_value(object, 3));
          if (obj_get_action_description(object)) {
            if (strlen(obj_get_action_description(object))) {
              char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH], nick3[MAX_INPUT_LENGTH];
              if (obj_get_vnum(g, vehicle) <= 46099 && obj_get_vnum(g, vehicle) >= 46000) {
                sprintf(nick, "Saiyan Pod %s", obj_get_action_description(object));
                sprintf(nick2, "@wA @Ys@ya@Yi@yy@Ya@yn @Dp@Wo@Dd@w named @D(@C%s@D)@w", obj_get_action_description(object));
              } else if (obj_get_vnum(g, vehicle) >= 46100 && obj_get_vnum(g, vehicle) <= 46199) {
                sprintf(nick, "EDI Xenofighter MK. II %s", obj_get_action_description(object));
                sprintf(nick2, "@wAn @YE@yD@YI @CX@ce@Wn@Do@Cf@ci@Wg@Dh@Wt@ce@Cr @RMK. II @wnamed @D(@C%s@D)@w", obj_get_action_description(object));
              }
              snprintf(nick3, sizeof(nick3), "%s is resting here@w", nick2);
              obj_set_name(vehicle, nick);
              obj_set_short_description(vehicle, nick2);
              obj_set_description(vehicle, nick3);
            }
          }
          obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_CLOSED));
          obj_set_value(object, VAL_CONTAINER_FLAGS, obj_get_value(object, VAL_CONTAINER_FLAGS) | (CONT_LOCKED));
        } else {
          mud_log("Hatch load: Hatch with no vehicle load room: #%d!", obj_get_vnum(g, object));
        }
      }
    }
    if (obj_get_exit(g, object, 5) && (room_sect(g, obj_get_in_room(object)) == SECT_UNDERWATER || room_sect(g, obj_get_in_room(object)) == SECT_WATER_NOSWIM)) {
      act(g, "$p @Bsinks to deeper waters.@n", true, nullptr, object, nullptr, TO_ROOM);
      room_vnum numb = obj_get_exit(g, object, 5)->to_room;
      obj_from_room(g, object);
      obj_to_room(g, object, numb);
    }
    if (obj_get_exit(g, object, 5) && room_sect(g, obj_get_in_room(object)) == SECT_FLYING && (obj_get_vnum(g, object) < 80 || obj_get_vnum(g, object) > 83)) {
      act(g, "$p @Cfalls down.@n", true, nullptr, object, nullptr, TO_ROOM);
      room_vnum numb = obj_get_exit(g, object, 5)->to_room;
      obj_from_room(g, object);
      obj_to_room(g, object, numb);
      if (room_sect(g, obj_get_in_room(object)) != SECT_FLYING) {
        act(g, "$p @Cfalls down and smacks the ground.@n", true, nullptr, object, nullptr, TO_ROOM);
      }
    }
    if (obj_get_value(object, 0) != 0) {
      if (obj_get_vnum(g, object) == 16705 || obj_get_vnum(g, object) == 16706 || obj_get_vnum(g, object) == 16707) {
        obj_set_level(object, obj_get_value(object, 0));
      }
    }
    if (room_flagged(g, room, ROOM_HOUSE))
      room_set_flag(g, room, ROOM_HOUSE_CRASH, true);
  }
}

/* Take an object from a room */
void obj_from_room(GameWorld *g, struct obj_data *object)
{
  if (!object || obj_get_in_room(object) == NOWHERE) {
    mud_log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
            (void *)object, obj_get_in_room(object));
    return;
  }

  if (obj_get_posted_to(g, object) && obj_get_in_obj(object) == nullptr) {
    struct obj_data *obj = obj_get_posted_to(g, object);
    if (obj_get_posttype(object) <= 0) {
      send_to_room(g, obj_get_in_room(obj), "%s@W shakes loose from %s@W.@n\r\n", obj_get_short_description(obj), obj_get_short_description(object));
    } else {
      send_to_room(g, obj_get_in_room(obj), "%s@W comes loose from %s@W.@n\r\n", obj_get_short_description(object), obj_get_short_description(obj));
    }
    obj_set_posted_to(obj, nullptr);
    obj_set_posttype(obj, 0);
    obj_set_posted_to(object, nullptr);
    obj_set_posttype(object, 0);
  }

  room_contents_slot(g, obj_get_in_room(object))->remove(obj_get_id(object));

  if (room_flagged(g, obj_get_in_room(object), ROOM_HOUSE))
    room_set_flag(g, obj_get_in_room(object), ROOM_HOUSE_CRASH, true);
  obj_set_in_room(object, NOWHERE);
}

/* put an object in an object (quaint)  */
void obj_to_obj(GameWorld *g, struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    mud_log("SYSERR: NULL object (%p) or same source (%p) and target (%p VNUM: %d) obj passed to obj_to_obj.",
            (void *)obj, (void *)obj, (void *)obj_to, obj_to ? obj_get_vnum(g, obj_to) : -1);
    return;
  }

  check_registered(g, obj, "obj_to_obj");
  obj_contains_slot(obj_to)->add(obj_get_id(obj));
  obj_set_in_obj(obj, obj_to);
  tmp_obj           = obj_get_in_obj(obj);

  /* Only add weight to container, or back to carrier for non-eternal
     containers.  Eternal means max capacity < 0 */
  if (obj_get_value(obj_get_in_obj(obj), VAL_CONTAINER_CAPACITY) > 0) {
    for (tmp_obj = obj_get_in_obj(obj); obj_get_in_obj(tmp_obj); tmp_obj = obj_get_in_obj(tmp_obj))
      obj_set_weight(tmp_obj, obj_get_weight(tmp_obj) + (obj_get_weight(obj)));

    /* top level object.  Subtract weight from inventory if necessary. */
    obj_set_weight(tmp_obj, obj_get_weight(tmp_obj) + (obj_get_weight(obj)));
    if (obj_get_carried_by(tmp_obj))
      char_set_carry_weight(obj_get_carried_by(tmp_obj), char_get_carry_weight(obj_get_carried_by(tmp_obj)) + (obj_get_weight(obj)));
  }
  if (obj_get_in_room(obj_to) != NOWHERE && room_flagged(g, obj_get_in_room(obj_to), ROOM_HOUSE)) {
    room_set_flag(g, obj_get_in_room(obj_to), ROOM_HOUSE_CRASH, true);
  }
}

/* remove an object from an object */
void obj_from_obj(GameWorld *g, struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;

  if (obj_get_in_obj(obj) == nullptr) {
    mud_log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj_get_in_obj(obj);
  temp     = obj_get_in_obj(obj);
  obj_contains_slot(obj_from)->remove(obj_get_id(obj));

  /* Subtract weight from containers container */
  /* Only worry about weight for non-eternal containers
     Eternal means max capacity < 0 */
  if (obj_get_value(obj_get_in_obj(obj), VAL_CONTAINER_CAPACITY) > 0) {
    for (temp = obj_get_in_obj(obj); obj_get_in_obj(temp); temp = obj_get_in_obj(temp))
      obj_set_weight(temp, obj_get_weight(temp) - (obj_get_weight(obj)));

    /* Subtract weight from char that carries the object */
    obj_set_weight(temp, obj_get_weight(temp) - (obj_get_weight(obj)));
    if (obj_get_carried_by(temp))
      char_set_carry_weight(obj_get_carried_by(temp), char_get_carry_weight(obj_get_carried_by(temp)) - (obj_get_weight(obj)));
  }

  if (obj_get_in_room(obj_from) != NOWHERE && room_flagged(g, obj_get_in_room(obj_from), ROOM_HOUSE)) {
    room_set_flag(g, obj_get_in_room(obj_from), ROOM_HOUSE_CRASH, true);
  }

  obj_set_in_obj(obj, nullptr);
}

/* move a player out of a room */
void char_from_room(GameWorld *g, struct char_data *ch)
{
  int i;

  if (ch == nullptr || char_get_in_room(ch) == NOWHERE) {
    mud_log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    return;
  }

  if (char_get_fighting(g, ch) != nullptr && !char_aff_flagged(ch, AFF_PURSUIT))
    stop_fighting(g, ch);
  if (char_aff_flagged(ch, AFF_PURSUIT) && char_get_fighting(g, ch) == nullptr)
    char_aff_set(ch, AFF_PURSUIT, false);

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i) != nullptr)
      if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT)
        if (obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS))
          room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) - 1);

  if (char_plr_flagged(ch, PLR_AURALIGHT))
    room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) - 1);

  room_people_slot(g, char_get_in_room(ch))->remove(char_get_id(ch));
  char_set_in_room(ch, NOWHERE);
}

/* place a character in a room */
void char_to_room(GameWorld *g, struct char_data *ch, room_vnum room)
{
  int i;

  if (ch == nullptr || !room_of(g, room))
    mud_log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d Ch: %p", room, (void *)ch);
  else {
    if (char_by_id(g, char_get_id(ch)) != ch)
      mud_log("SYSERR: char_to_room: %s (id %d) isn't registered, so the room can't list them.", char_get_name(ch),
              char_get_id(ch));
    room_people_slot(g, room)->add(char_get_id(ch));
    char_set_in_room(ch, room);

    for (i = 0; i < NUM_WEARS; i++)
      if (char_get_equipment(ch, i))
        if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT)
          if (obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS))
            room_set_light(g, room, room_get_light(g, room) + 1);

    if (char_plr_flagged(ch, PLR_AURALIGHT))
      room_set_light(g, room, room_get_light(g, room) + 1);

    /* Stop fighting now, if we left. */
    if (char_get_fighting(g, ch) && char_get_in_room(ch) != char_get_in_room(char_get_fighting(g, ch)) && !char_aff_flagged(ch, AFF_PURSUIT)) {
      stop_fighting(g, char_get_fighting(g, ch));
      stop_fighting(g, ch);
    }
    if (!char_is_npc(ch)) {
      if (char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
        char_prf_set(g, ch, PRF_ARENAWATCH, false);
        char_set_arenawatch(ch, -1);
      }
    }
  }
}

/* give an object to a char   */
void obj_to_char(GameWorld *g, struct obj_data *object, struct char_data *ch)
{
  if (object && ch) {
    check_registered(g, object, "obj_to_char");
    char_inventory_slot(ch)->add(obj_get_id(object));
    obj_set_carried_by(object, ch);
    obj_set_in_room(object, NOWHERE);
    char_set_carry_weight(ch, char_get_carry_weight(ch) + (obj_get_weight(object)));
    char_set_carry_items(ch, char_get_carry_items(ch) + 1);
    if ((char_get_kaioken(ch) <= 0 && !char_aff_flagged(ch, AFF_METAMORPH)) && !obj_extra_flagged(object, ITEM_THROW)) {

    } else if (getCurPL(g, ch) > getEffMaxPL(g, ch)) {
      if (char_get_kaioken(ch) > 0) {
        send_to_char(g, ch, "@RThe strain of the weight has reduced your kaioken somewhat!@n\n");
      } else if (char_aff_flagged(ch, AFF_METAMORPH)) {
        send_to_char(g, ch, "@RYour metamorphosis strains under the additional weight!@n\n");
      }
    }

    /* set flag for crash-save system, but not on mobs! */
    if (obj_get_value(object, 0) != 0) {
      if (obj_get_vnum(g, object) == 16705 || obj_get_vnum(g, object) == 16706 || obj_get_vnum(g, object) == 16707) {
        obj_set_level(object, obj_get_value(object, 0));
      }
    }
    if (!char_is_npc(ch))
      char_plr_set(ch, PLR_CRASH, true);
  } else
    mud_log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", (void *)object, (void *)ch);
}

/* take an object from a char */
void obj_weight_changed(GameWorld *g, struct obj_data *obj, int64_t change)
{
  struct obj_data *outer;

  for (outer = obj_get_in_obj(obj); outer; outer = obj_get_in_obj(outer)) {
    obj_set_weight(outer, obj_get_weight(outer) + change);
    obj = outer;
  }
  if (obj_get_carried_by(obj))
    char_set_carry_weight(obj_get_carried_by(obj), char_get_carry_weight(obj_get_carried_by(obj)) + change);
}

void obj_from_char(GameWorld *g, struct obj_data *object)
{
  if (object == nullptr) {
    mud_log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  char_inventory_slot(obj_get_carried_by(object))->remove(obj_get_id(object));

  /* set flag for crash-save system, but not on mobs! */
  if (!char_is_npc(obj_get_carried_by(object)))
    char_plr_set(obj_get_carried_by(object), PLR_CRASH, true);

  char_set_carry_weight(obj_get_carried_by(object), char_get_carry_weight(obj_get_carried_by(object)) - (obj_get_weight(object)));
  char_set_carry_items(obj_get_carried_by(object), char_get_carry_items(obj_get_carried_by(object)) - 1);

  if (obj_get_value(object, 0) != 0) {
    if (obj_get_vnum(g, object) == 16705 || obj_get_vnum(g, object) == 16706 || obj_get_vnum(g, object) == 16707) {
      obj_set_level(object, obj_get_value(object, 0));
    }
  }

  obj_set_carried_by(object, nullptr);
}

/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(struct char_data *ch, int eq_pos)
{
  if (char_get_equipment(ch, eq_pos) == nullptr) {
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

bool invalid_align(struct char_data *ch, struct obj_data *obj)
{
  if (obj_extra_flagged(obj, ITEM_ANTI_EVIL) && char_is_evil(ch))
    return true;
  if (obj_extra_flagged(obj, ITEM_ANTI_GOOD) && char_is_good(ch))
    return true;
  if (obj_extra_flagged(obj, ITEM_ANTI_NEUTRAL) && char_is_neutral(ch))
    return true;
  return false;
}

void equip_char(GameWorld *g, struct char_data *ch, struct obj_data *obj, int pos)
{
  int j;

  if (pos < 0 || pos >= NUM_WEARS) {
    core_dump();
    return;
  }

  if (char_get_equipment(ch, pos)) {
    mud_log("SYSERR: Char is already equipped: %s, %s", char_get_name(ch),
            obj_get_short_description(obj));
    return;
  }
  if (obj_get_carried_by(obj)) {
    mud_log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (obj_get_in_room(obj) != NOWHERE) {
    mud_log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj)) {
    act(g, "You stop wearing $p as something prevents you.", false, ch, obj, nullptr, TO_CHAR);
    act(g, "$n stops wearing $p as something prevents $m.", false, ch, obj, nullptr, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(g, obj, ch);
    return;
  }

  char_set_equipment(ch, pos, obj);
  obj_set_worn_by(obj, ch);
  obj_set_worn_on(obj, pos);

  if (obj_get_type_flag(obj) == ITEM_ARMOR)
    char_set_armor(ch, char_get_armor(ch) + (apply_ac(ch, pos)));

  if (char_get_in_room(ch) != NOWHERE) {
    if (obj_get_type_flag(obj) == ITEM_LIGHT)
      if (obj_get_value(obj, VAL_LIGHT_HOURS)) /* if light is ON */
        room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) + 1);
  } else
    mud_log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", char_get_name(ch));

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(g, ch, obj_get_affected(obj)[j].location,
                     obj_get_affected(obj)[j].modifier,
                     obj_get_affected(obj)[j].specific,
                     obj_get_bitvector(obj), true);

  affect_total(g, ch);
}

struct obj_data *unequip_char(GameWorld *g, struct char_data *ch, int pos)
{
  int j;
  struct obj_data *obj;

  if ((pos < 0 || pos >= NUM_WEARS) || char_get_equipment(ch, pos) == nullptr) {
    core_dump();
    return (nullptr);
  }

  obj          = char_get_equipment(ch, pos);
  obj_set_worn_by(obj, nullptr);
  obj_set_worn_on(obj, -1);

  if (obj_get_type_flag(obj) == ITEM_ARMOR)
    char_set_armor(ch, char_get_armor(ch) - (apply_ac(ch, pos)));

  if (char_get_in_room(ch) != NOWHERE) {
    if (obj_get_type_flag(obj) == ITEM_LIGHT)
      if (obj_get_value(obj, VAL_LIGHT_HOURS)) /* if light is ON */
        room_set_light(g, char_get_in_room(ch), room_get_light(g, char_get_in_room(ch)) - 1);
  } else
    mud_log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", char_get_name(ch));

  char_set_equipment(ch, pos, nullptr);

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(g, ch, obj_get_affected(obj)[j].location,
                     obj_get_affected(obj)[j].modifier,
                     obj_get_affected(obj)[j].specific,
                     obj_get_bitvector(obj), false);

  affect_total(g, ch);

  return (obj);
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(GameWorld *g, const id_list &list, struct char_data *ch)
{
  list.each([&](long id) {
    if (struct obj_data *obj = obj_by_id(g, id)) {
      object_list_new_owner(g, obj_get_contains(obj), ch);
      obj_set_carried_by(obj, ch);
    }
  });
}
