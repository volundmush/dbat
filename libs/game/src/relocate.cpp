

/* put an object in a room */
void obj_to_room(struct obj_data *object, room_rnum room)
{
  struct obj_data *vehicle = NULL;

  if (!object || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	room, top_of_world, object);
  else {
    if (ROOM_FLAGGED(room, ROOM_GARDEN1) || ROOM_FLAGGED(room, ROOM_GARDEN2)) {
     if (GET_OBJ_TYPE(object) != ITEM_PLANT) {
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
    IN_ROOM(object) = room;
    object->carried_by = NULL;
    GET_LAST_LOAD(object) = time(0);
    if (GET_OBJ_TYPE(object) == ITEM_VEHICLE && !OBJ_FLAGGED(object, ITEM_UNBREAKABLE) && GET_OBJ_VNUM(object) > 19199) {
      SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_UNBREAKABLE);
    } if (GET_OBJ_TYPE(object) == ITEM_HATCH && GET_OBJ_VNUM(object) <= 19199) {
     if ((GET_OBJ_VNUM(object) <= 18999 && GET_OBJ_VNUM(object) >= 18800) || (GET_OBJ_VNUM(object) <= 19199 && GET_OBJ_VNUM(object) >= 19100)) {
      int hnum = GET_OBJ_VAL(object, 0);
      struct obj_data *house = read_object(hnum, VIRTUAL);
      obj_to_room(house, real_room(GET_OBJ_VAL(object, 6)));
      SET_BIT(GET_OBJ_VAL(object, VAL_CONTAINER_FLAGS), CONT_CLOSED);
      SET_BIT(GET_OBJ_VAL(object, VAL_CONTAINER_FLAGS), CONT_LOCKED);
     }
    }
    if (GET_OBJ_TYPE(object) == ITEM_HATCH && GET_OBJ_VAL(object, 0) > 1 && GET_OBJ_VNUM(object) > 19199 ) {
    if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(object, VAL_HATCH_DEST)))) {
     if (real_room(GET_OBJ_VAL(object, 3)) != NOWHERE) {
      vehicle = read_object(GET_OBJ_VAL(object, 0), VIRTUAL);
      obj_to_room(vehicle, real_room(GET_OBJ_VAL(object, 3)));
      if (object->action_description) {
       if (strlen(object->action_description)) {
        char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH], nick3[MAX_INPUT_LENGTH];
        if (GET_OBJ_VNUM(vehicle) <= 46099 && GET_OBJ_VNUM(vehicle) >= 46000) {
         sprintf(nick, "Saiyan Pod %s", object->action_description);
         sprintf(nick2, "@wA @Ys@ya@Yi@yy@Ya@yn @Dp@Wo@Dd@w named @D(@C%s@D)@w", object->action_description);
        } else if (GET_OBJ_VNUM(vehicle) >= 46100 && GET_OBJ_VNUM(vehicle) <= 46199) {
         sprintf(nick, "EDI Xenofighter MK. II %s", object->action_description);
         sprintf(nick2, "@wAn @YE@yD@YI @CX@ce@Wn@Do@Cf@ci@Wg@Dh@Wt@ce@Cr @RMK. II @wnamed @D(@C%s@D)@w", object->action_description);
        }
        sprintf(nick3, "%s is resting here@w", nick2);
        vehicle->name = strdup(nick);
        vehicle->short_description = strdup(nick2);
        vehicle->description = strdup(nick3);
       }
      }
      SET_BIT(GET_OBJ_VAL(object, VAL_CONTAINER_FLAGS), CONT_CLOSED);
      SET_BIT(GET_OBJ_VAL(object, VAL_CONTAINER_FLAGS), CONT_LOCKED);
     }
     else {
      log("Hatch load: Hatch with no vehicle load room: #%d!", GET_OBJ_VNUM(object));
     }
     }
    }
    if (EXIT(object, 5) && (SECT(IN_ROOM(object)) == SECT_UNDERWATER || SECT(IN_ROOM(object)) == SECT_WATER_NOSWIM)) {
     act("$p @Bsinks to deeper waters.@n", TRUE, 0, object, 0, TO_ROOM);
     int numb = GET_ROOM_VNUM(EXIT(object, 5)->to_room);
     obj_from_room(object);
     obj_to_room(object, real_room(numb));
    }
    if (EXIT(object, 5) && SECT(IN_ROOM(object)) == SECT_FLYING && (GET_OBJ_VNUM(object) < 80 || GET_OBJ_VNUM(object) > 83)) {
     act("$p @Cfalls down.@n", TRUE, 0, object, 0, TO_ROOM);
     int numb = GET_ROOM_VNUM(EXIT(object, 5)->to_room);
     obj_from_room(object);
     obj_to_room(object, real_room(numb));
     if (SECT(IN_ROOM(object)) != SECT_FLYING) {
      act("$p @Cfalls down and smacks the ground.@n", TRUE, 0, object, 0, TO_ROOM);
     }
    }
    if (GET_OBJ_VAL(object, 0) != 0) {
     if (GET_OBJ_VNUM(object) == 16705 || GET_OBJ_VNUM(object) == 16706 || GET_OBJ_VNUM(object) == 16707) {
      object->level = GET_OBJ_VAL(object, 0);
     }
    }
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT_AR(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
  }
}


/* Take an object from a room */
void obj_from_room(struct obj_data *object)
{
  struct obj_data *temp;

  if (!object || IN_ROOM(object) == NOWHERE) {
    log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
	object, IN_ROOM(object));
    return;
  }

  if (GET_OBJ_POSTED(object) && object->in_obj == NULL) {
   struct obj_data *obj = GET_OBJ_POSTED(object);
   if (GET_OBJ_POSTTYPE(object) <= 0) {
    send_to_room(IN_ROOM(obj), "%s@W shakes loose from %s@W.@n\r\n", obj->short_description, object->short_description);
   } else {
    send_to_room(IN_ROOM(obj), "%s@W comes loose from %s@W.@n\r\n", object->short_description, obj->short_description);
   }
   GET_OBJ_POSTED(obj) = NULL;
   GET_OBJ_POSTTYPE(obj) = 0;
   GET_OBJ_POSTED(object) = NULL;
   GET_OBJ_POSTTYPE(object) = 0;
  }

  REMOVE_FROM_LIST(object, world[IN_ROOM(object)].contents, next_content, temp);

  if (ROOM_FLAGGED(IN_ROOM(object), ROOM_HOUSE))
    SET_BIT_AR(ROOM_FLAGS(IN_ROOM(object)), ROOM_HOUSE_CRASH);
  IN_ROOM(object) = NOWHERE;
  object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    log("SYSERR: NULL object (%p) or same source (%p) and target (%p VNUM: %d) obj passed to obj_to_obj.",
	obj, obj, obj_to, obj_to ? GET_OBJ_VNUM(obj_to) : -1);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  tmp_obj = obj->in_obj;

  /* Only add weight to container, or back to carrier for non-eternal
     containers.  Eternal means max capacity < 0 */
  if (GET_OBJ_VAL(obj->in_obj, VAL_CONTAINER_CAPACITY) > 0)
  {
  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  /* top level object.  Subtract weight from inventory if necessary. */
  GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
  }
  if (IN_ROOM(obj_to) != NOWHERE && ROOM_FLAGGED(IN_ROOM(obj_to), ROOM_HOUSE)) {
   SET_BIT_AR(ROOM_FLAGS(IN_ROOM(obj_to)), ROOM_HOUSE_CRASH);
  }
}


/* remove an object from an object */
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  temp = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content, temp);

  /* Subtract weight from containers container */
  /* Only worry about weight for non-eternal containers
     Eternal means max capacity < 0 */
  if (GET_OBJ_VAL(obj->in_obj, VAL_CONTAINER_CAPACITY) > 0)
  {
  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  /* Subtract weight from char that carries the object */
  GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
  }

  if (IN_ROOM(obj_from) != NOWHERE && ROOM_FLAGGED(IN_ROOM(obj_from), ROOM_HOUSE)) {
   SET_BIT_AR(ROOM_FLAGS(IN_ROOM(obj_from)), ROOM_HOUSE_CRASH);
  }

  obj->in_obj = NULL;
  obj->next_content = NULL;
}




/* move a player out of a room */
void char_from_room(struct char_data *ch)
{
  struct char_data *temp;
  int i;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    return;
  }

  if (FIGHTING(ch) != NULL && !AFF_FLAGGED(ch, AFF_PURSUIT))
    stop_fighting(ch);
  if (AFF_FLAGGED(ch, AFF_PURSUIT) && FIGHTING(ch) == NULL)
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) != NULL)
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
        if (GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS))
	  world[IN_ROOM(ch)].light--;
	  
 if (PLR_FLAGGED(ch, PLR_AURALIGHT))
   world[IN_ROOM(ch)].light--;

  REMOVE_FROM_LIST(ch, world[IN_ROOM(ch)].people, next_in_room, temp);
  IN_ROOM(ch) = NOWHERE;
  ch->next_in_room = NULL;
}


/* place a character in a room */
void char_to_room(struct char_data *ch, room_rnum room)
{
  int i;

  if (ch == NULL || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		room, top_of_world, ch);
  else {
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    IN_ROOM(ch) = room;

    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
        if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
	  if (GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS))
	    world[room].light++;

	if (PLR_FLAGGED(ch, PLR_AURALIGHT))
       world[room].light++;	
	   
    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)) && !AFF_FLAGGED(ch, AFF_PURSUIT)) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
    if (!IS_NPC(ch)) {
     if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
      ARENA_IDNUM(ch) = -1;
     }
    }
  }
}


/* give an object to a char   */
void obj_to_char(struct obj_data *object, struct char_data *ch)
{
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    IN_ROOM(object) = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;
    if ((GET_KAIOKEN(ch) <= 0 && !AFF_FLAGGED(ch, AFF_METAMORPH)) && !OBJ_FLAGGED(object, ITEM_THROW)) {

    } else if (GET_HIT(ch) > (getEffMaxPL(ch)))) {
       if (GET_KAIOKEN(ch) > 0) {
        send_to_char(ch, "@RThe strain of the weight has reduced your kaioken somewhat!@n\n");
       } else if (AFF_FLAGGED(ch, AFF_METAMORPH)) {
        send_to_char(ch, "@RYour metamorphosis strains under the additional weight!@n\n");
       }
    }

    /* set flag for crash-save system, but not on mobs! */
    if (GET_OBJ_VAL(object, 0) != 0) {
     if (GET_OBJ_VNUM(object) == 16705 || GET_OBJ_VNUM(object) == 16706 || GET_OBJ_VNUM(object) == 16707) {
      object->level = GET_OBJ_VAL(object, 0);
     }
    }
    if (!IS_NPC(ch))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
  } else
    log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}


/* take an object from a char */
void obj_from_char(struct obj_data *object)
{
  struct obj_data *temp;

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content, temp);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT_AR(PLR_FLAGS(object->carried_by), PLR_CRASH);
 
  int64_t previous = (getEffMaxPL(object->carried_by)());

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;

    if (GET_OBJ_VAL(object, 0) != 0) {
     if (GET_OBJ_VNUM(object) == 16705 || GET_OBJ_VNUM(object) == 16706 || GET_OBJ_VNUM(object) == 16707) {
      object->level = GET_OBJ_VAL(object, 0);
     }
    }

  object->carried_by = NULL;
  object->next_content = NULL;
}



/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(struct char_data *ch, int eq_pos)
{
  if (GET_EQ(ch, eq_pos) == NULL) {
    core_dump();
    return (0);
  }

  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
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
  return (GET_OBJ_VAL(GET_EQ(ch, eq_pos), VAL_ARMOR_APPLYAC));
}

int invalid_align(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
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

  if (GET_EQ(ch, pos)) {
    log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	    obj->short_description);
    return;
  }
  if (obj->carried_by) {
    log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (IN_ROOM(obj) != NOWHERE) {
    log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj)) {
    act("You stop wearing $p as something prevents you.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops wearing $p as something prevents $m.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch);
    return;
  }

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_ARMOR(ch) += apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS))	/* if light is ON */
	world[IN_ROOM(ch)].light++;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", GET_NAME(ch));

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(ch, obj->affected[j].location,
		  obj->affected[j].modifier,
		  obj->affected[j].specific,
		  GET_OBJ_PERM(obj), TRUE);

  affect_total(ch);
}



struct obj_data *unequip_char(struct char_data *ch, int pos)
{
  int j;
  struct obj_data *obj;

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL) {
    core_dump();
    return (NULL);
  }

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_ARMOR(ch) -= apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS))	/* if light is ON */
	world[IN_ROOM(ch)].light--;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", GET_NAME(ch));

  GET_EQ(ch, pos) = NULL;

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(ch, obj->affected[j].location,
		  obj->affected[j].modifier,
		  obj->affected[j].specific,
		  GET_OBJ_PERM(obj), FALSE);

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
