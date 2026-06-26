/*************************************************************************
 *  File: vehicles.c                                    Part of CircleMUD *
 *  Usage: Vechicle related code  *
 *									 *
 *  All rights reserved.  See license.doc for complete information.	 *
 *									 *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University*
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 *  Vehicle.c written by Chris Jacobson <fear@athenet.net>		 *
 *************************************************************************/
#include "vehicles.h"

#include "act.informative.h"
#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "comm.h"
#include "consts/constates.h"
#include "consts/directions.h"
#include "consts/exitflags.h"
#include "consts/itemdata.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/roomflags.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "flags.h"
#include "handler.h"
#include "interpreter.h"
#include "log.h"
#include "object_api.h"
#include "object_db.h"
#include "object_impl.h"
#include "object_macros.h"
#include "object_utils.h"
#include "races.h"
#include "races_plus.h"
#include "random.h"
#include "relocate.h"
#include "room_api.h"
#include "room_db.h"
#include "room_macros.h"
#include "room_utils.h"
#include "search.h"
#include "stringutils.h"

#include "iterate.hpp"

#include <strings.h>
#include <unordered_map>

struct obj_data *find_vehicle_by_vnum(int vnum) {
  static std::unordered_map<int, int64_t> cache;

  auto it = cache.find(vnum);
  if (it != cache.end()) {
    auto obj = obj_by_id(it->second);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && GET_OBJ_VNUM(obj) == vnum)
      return obj;
    cache.erase(it);
  }

  struct obj_data *found = NULL;
  obj_iterate_all_newest([&](struct obj_data *i) {
    if (GET_OBJ_TYPE(i) == ITEM_VEHICLE && GET_OBJ_VNUM(i) == vnum) {
      found = i;
      return false;
    }
    return true;
  });

  if (found) cache[vnum] = obj_id_get(found);
  return found;
}

struct obj_data *find_hatch_by_vnum(int vnum) {
  struct obj_data *found = NULL;

  obj_iterate_all_newest([&](struct obj_data *i) {
    if (GET_OBJ_TYPE(i) == ITEM_HATCH && GET_OBJ_VNUM(i) == vnum) {
      found = i;
      return false;
    }
    return true;
  });

  return found;
}

int64_t hatch_vehicle_id_get(struct obj_data *hatch) {
  return ((int64_t)(uint32_t)GET_OBJ_VAL(hatch, VAL_HATCH_VEHICLE_ID_HI) << 32)
       | (uint32_t)GET_OBJ_VAL(hatch, VAL_HATCH_VEHICLE_ID_LO);
}

void hatch_vehicle_id_set(struct obj_data *hatch, int64_t id) {
  GET_OBJ_VAL(hatch, VAL_HATCH_VEHICLE_ID_HI) = (int)(uint32_t)((uint64_t)id >> 32);
  GET_OBJ_VAL(hatch, VAL_HATCH_VEHICLE_ID_LO) = (int)(uint32_t)((uint64_t)id & 0xFFFFFFFFu);
}

struct obj_data *hatch_get_vehicle(struct obj_data *hatch) {
  int64_t id = hatch_vehicle_id_get(hatch);
  if (id) {
    struct obj_data *v = obj_by_id(id);
    if (v) return v;
  }
  struct obj_data *v = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, VAL_HATCH_DEST));
  if (v) hatch_vehicle_id_set(hatch, obj_id_get(v));
  return v;
}

/* Search the given list for an object type, and return a ptr to that obj*/
struct obj_data *get_obj_in_list_type(int type, struct inventory_data list) {
  struct obj_data *result = NULL;
  auto handler = [&](auto i) {
    if (GET_OBJ_TYPE(i) == type) {
      result = i;
      return false;
    }
    return true;
  };
  switch (list.entity_type) {
  case ENT_ROOM: room_contents_iterate(list.entity.room, handler); break;
  case ENT_CHAR: char_inventory_iterate(list.entity.ch, handler); break;
  case ENT_OBJ:  obj_contents_iterate(list.entity.obj, handler); break;
  }
  return result;
}

/* Search the player's room, inventory and equipment for a control */
struct obj_data *find_control(struct char_data *ch) {
  struct obj_data *controls, *obj;
  int j;

  controls = get_obj_in_list_type(ITEM_CONTROL, inv_for_room(char_room_get(ch)));
  if (!controls)
    char_inventory_iterate(ch, [&](auto obj) {
      if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_TYPE(obj) == ITEM_CONTROL) {
        controls = obj;
        return false;
      }
      return true;
    });
  if (!controls)
    char_equipment_iterate(ch, [&](auto j, auto eq) {
      if (CAN_SEE_OBJ(ch, eq) && GET_OBJ_TYPE(eq) == ITEM_CONTROL) {
        controls = eq;
        return false;
      }
      return true;
    });
  return controls;
}
