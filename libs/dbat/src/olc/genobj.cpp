/************************************************************************
 * Generic OLC Library - Objects / genobj.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/iterate.hpp"
#include "dbat/arrays.hpp"
#include "dbat/shop.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/genobj.hpp"
#include "dbat/relocate.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/utils.hpp"
#include "dbat/handler.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/game_world.hpp"

static bool copy_object_preserve(struct obj_data *to, struct obj_data *from);
static obj_vnum index_object(GameWorld *g, struct obj_data *obj, obj_vnum ovnum, obj_vnum ornum);
static int update_objects(GameWorld *g, struct obj_data *refobj);

static bool copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);

obj_vnum add_object(GameWorld *g, struct obj_data *newobj, obj_vnum ovnum)
{
  obj_vnum found          = NOTHING;
  struct zone_data *rznum = find_zone_by_thing(g, ovnum);

  /*
   * Write object to internal tables.
   */
  obj_set_item_number(newobj, obj_proto_of(g, ovnum) ? ovnum : NOTHING);
  if (obj_get_item_number(newobj) != NOTHING) {
    free_extracted_objects(g, ovnum); /* they share the trigger list copy_object() replaces */
    copy_object(g, obj_proto_of(g, obj_get_item_number(newobj)), newobj);
    update_objects(g, obj_proto_of(g, obj_get_item_number(newobj)));
    add_to_save_list(g, rznum->number, SL_OBJ);
    return obj_get_item_number(newobj);
  }

  g->obj_index.emplace(ovnum);
  g->obj_proto.emplace(ovnum);
  found = index_object(g, newobj, ovnum, ovnum);
  add_to_save_list(g, rznum->number, SL_OBJ);
  return found;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Fix all existing objects to have these values.
 * We need to run through each and every object currently in the
 * game to see which ones are pointing to this prototype.
 * if object is pointing to this prototype, then we need to replace it
 * with the new one.
 */
static int update_objects(GameWorld *g, struct obj_data *refobj)
{
  struct obj_data swap;
  int count = 0;

  for (obj_data *obj : each_obj(g)) {
    int64_t contents = 0, change;

    if (obj_get_item_number(obj) != obj_get_item_number(refobj))
      continue;

    count++;

    /* Update the existing object but save a copy for private information. */
    swap = *obj;
    *obj = *refobj;

    /* Copy game-time dependent variables over. */
    obj_set_in_room(obj, obj_get_in_room(&swap));
    obj_set_carried_by(obj, obj_get_carried_by(&swap));
    obj_set_worn_by(obj, obj_get_worn_by(&swap));
    obj_set_worn_on(obj, obj_get_worn_on(&swap));
    obj_set_in_obj(obj, obj_get_in_obj(&swap));
    *obj_contains_slot(obj) = obj_get_contains(&swap);
    obj_set_sitting(obj, obj_get_sitting(g, &swap));
    obj_set_user(obj, obj_get_user(g, &swap));
    obj_set_target(obj, obj_get_target(g, &swap));

    /* This copy keeps its identity: scripts and the registry find it by id. */
    obj_set_id(obj, obj_get_id(&swap));
    obj_set_script(obj, obj_get_script(&swap));


    /* What it holds still counts towards its weight, and whoever carries it needs to know the change. */
    for (obj_data *inner : obj_contents(g, obj))
      contents += obj_get_weight(inner);
    obj_set_weight(obj, obj_get_weight(obj) + contents);
    change = obj_get_weight(obj) - obj_get_weight(&swap);
    obj_weight_changed(g, obj, change);
  }

  return count;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------------------------------------ */

static obj_vnum index_object(GameWorld *g, struct obj_data *obj, obj_vnum ovnum, obj_vnum ornum)
{
  if (obj == nullptr || ovnum < 0 || !obj_proto_of(g, ornum))
    return NOWHERE;

  obj_set_item_number(obj, ornum);
  obj_index_of(g, ornum)->vnum   = ovnum;
  obj_index_of(g, ornum)->number = 0;
  obj_index_of(g, ornum)->func   = nullptr;

  copy_object_preserve(obj_proto_of(g, ornum), obj);
  obj_set_in_room(obj_proto_of(g, ornum), NOWHERE);

  return ornum;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

bool copy_object(GameWorld *g, struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, true);
}

static bool copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, false);
}

static bool copy_object_main(struct obj_data *to, struct obj_data *from, int free_object)
{
  *to = *from; /* strings and extra descriptions included */
  return true;
}

static struct obj_data *first_object_of_proto(GameWorld *g, const struct obj_data *proto)
{

  for (obj_data *obj : each_obj(g))
    if (obj_get_item_number(obj) == obj_get_item_number(proto))
      return obj;
  return nullptr;
}

obj_vnum delete_object(GameWorld *g, obj_vnum rnum)
{
  struct zone_data *zrnum;
  struct obj_data *obj, *tmp;
  int cmd_no;

  if (rnum == NOTHING || !(obj = obj_proto_of(g, rnum)))
    return NOTHING;

  zrnum = find_zone_by_thing(g, obj_get_vnum(g, obj));
  add_to_save_list(g, zrnum->number, SL_OBJ);

  /* This is something you might want to read about in the logs. */
  mud_log("GenOLC: delete_object: Deleting object #{:d} ({}).", obj_get_vnum(g, obj), obj_get_short_description(obj));

  /* extract_obj() may free more than tmp, so start again from the top each time. */
  while ((tmp = first_object_of_proto(g, obj))) {
    /* extract_obj() will just axe contents. */
    while (obj_get_contains(tmp)) {
      struct obj_data *content = first_obj(g, obj_get_contains(tmp));

      obj_from_obj(g, content);
      if (obj_get_in_room(tmp) != NOWHERE)
        obj_to_room(g, content, obj_get_in_room(tmp));
      else if (obj_get_carried_by(tmp))
        obj_to_char(g, content, obj_get_carried_by(tmp));
      else if (obj_get_worn_by(tmp))
        obj_to_char(g, content, obj_get_worn_by(tmp));
      else if (obj_get_in_obj(tmp))
        obj_to_obj(g, content, obj_get_in_obj(tmp));
      else
        extract_obj(g, content); /* tmp is nowhere, so there's nowhere to put it */
    }
    /* Remove from g->objects, etc. - handles weight changes, and similar. */
    extract_obj(g, tmp);
  }

  /* No copy is left to share these, now that they have all been extracted and freed. */
  free_extracted_objects(g, rnum);
  obj_proto_script(obj).clear();

  /* Shops stop producing it. */
  for (struct shop_data *shop : g->shops.all()) {
    if (std::erase(shop_products(shop), rnum)) {
      if (struct zone_data *zone = find_zone_by_thing(g, shop_get_vnum(shop)))
        add_to_save_list(g, zone->number, SL_SHP);
    }
  }

  /* Zone commands that load it are turned into comments. */
  for (struct zone_data *zone : g->zones.all()) {
    bool changed = false;
    for (cmd_no = 0; zone_get_cmd(g, zone, cmd_no)->command != 'S'; cmd_no++) {
      switch (zone_get_cmd(g, zone, cmd_no)->command) {
        case 'P': /* the object put, or what it's put in */
          if (zone_get_cmd(g, zone, cmd_no)->arg1 == rnum || zone_get_cmd(g, zone, cmd_no)->arg3 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            if (zone_get_cmd(g, zone, cmd_no)->arg1 == rnum)
              zone_get_cmd(g, zone, cmd_no)->arg1 = NOTHING;
            if (zone_get_cmd(g, zone, cmd_no)->arg3 == rnum)
              zone_get_cmd(g, zone, cmd_no)->arg3 = NOTHING;
            changed = true;
          }
          break;
        case 'O':
        case 'G':
        case 'E':
          if (zone_get_cmd(g, zone, cmd_no)->arg1 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            zone_get_cmd(g, zone, cmd_no)->arg1    = NOTHING;
            changed                             = true;
          }
          break;
        case 'R':
          if (zone_get_cmd(g, zone, cmd_no)->arg2 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            zone_get_cmd(g, zone, cmd_no)->arg2    = NOTHING;
            changed                             = true;
          }
          break;
      }
      if (changed) {
        // Do something if the command was changed, e.g., save the zone or log the change
        add_to_save_list(g, zone->number, SL_ZON);
      }
    }
  }
  g->obj_proto.take(rnum);
  g->obj_index.take(rnum);
  return rnum;
}
