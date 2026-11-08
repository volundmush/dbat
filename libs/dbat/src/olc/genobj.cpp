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
#include "dbat/atomicfile.hpp"
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

    /* Each copy owns its own spellbook pages, as in read_object(), and frees them when it goes. */
    delete[] obj_get_sbinfo(&swap);
    *obj_sbinfo_slot(obj) = nullptr;
    if (obj_get_sbinfo(refobj)) {
      *obj_sbinfo_slot(obj) = new obj_spellbook_spell[SPELLBOOK_SIZE]();
      memcpy(obj_get_sbinfo(obj), obj_get_sbinfo(refobj), SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    }

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

bool save_objects(GameWorld *g, struct zone_data *zone_num)
{
  char cmfname[128], buf[MAX_STRING_LENGTH];
  char ebuf1[MAX_STRING_LENGTH], ebuf2[MAX_STRING_LENGTH];
  char ebuf3[MAX_STRING_LENGTH], ebuf4[MAX_STRING_LENGTH];
  char wbuf1[MAX_STRING_LENGTH], wbuf2[MAX_STRING_LENGTH];
  char wbuf3[MAX_STRING_LENGTH], wbuf4[MAX_STRING_LENGTH];
  char pbuf1[MAX_STRING_LENGTH], pbuf2[MAX_STRING_LENGTH];
  char pbuf3[MAX_STRING_LENGTH], pbuf4[MAX_STRING_LENGTH];
  int counter2;
  obj_vnum realcounter;
  struct atomic_file af;
  FILE *fp;
  struct obj_data *obj;

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  snprintf(cmfname, sizeof(cmfname), "%s%d.obj", OBJ_PREFIX, zone_num->number);
  if (!(fp = atomic_open(&af, cmfname, false))) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: OLC: Cannot open objects file %s!", cmfname);
    return false;
  }
  /*
   * Start running through all objects in this zone.
   */
  for (obj_vnum counter : g->obj_proto.vnums_in(genolc_zone_bottom(g, zone_num), zone_num->top)) {
    if (obj_proto_of(g, realcounter = counter)) {
      if (obj_get_action_description(obj = obj_proto_of(g, realcounter))) {
        strlcpy(buf, obj_get_action_description(obj), sizeof(buf));
        strip_cr(buf);
      } else
        *buf = '\0';

      fprintf(fp,
              "#%d\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n",

              obj_get_vnum(g, obj),
              (obj_get_name(obj) && *obj_get_name(obj)) ? obj_get_name(obj) : "undefined",
              (obj_get_short_description(obj) && *obj_get_short_description(obj)) ? obj_get_short_description(obj) : "undefined",
              (obj_get_description(obj) && *obj_get_description(obj)) ? obj_get_description(obj) : "undefined",
              buf);

      sprintascii(ebuf1, obj_get_extra_flags(obj)[0]);
      sprintascii(ebuf2, obj_get_extra_flags(obj)[1]);
      sprintascii(ebuf3, obj_get_extra_flags(obj)[2]);
      sprintascii(ebuf4, obj_get_extra_flags(obj)[3]);
      sprintascii(wbuf1, obj_get_wear_flags(obj)[0]);
      sprintascii(wbuf2, obj_get_wear_flags(obj)[1]);
      sprintascii(wbuf3, obj_get_wear_flags(obj)[2]);
      sprintascii(wbuf4, obj_get_wear_flags(obj)[3]);
      sprintascii(pbuf1, obj_get_bitvector(obj)[0]);
      sprintascii(pbuf2, obj_get_bitvector(obj)[1]);
      sprintascii(pbuf3, obj_get_bitvector(obj)[2]);
      sprintascii(pbuf4, obj_get_bitvector(obj)[3]);

      fprintf(fp,
              "%d %s %s %s %s %s %s %s %s %s %s %s %s\n"
              "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"
              "%" PRId64 " %d %d %d\n",

              obj_get_type_flag(obj),
              ebuf1, ebuf2, ebuf3, ebuf4,
              wbuf1, wbuf2, wbuf3, wbuf4,
              pbuf1, pbuf2, pbuf3, pbuf4,
              obj_get_value(obj, 0), obj_get_value(obj, 1), obj_get_value(obj, 2),
              obj_get_value(obj, 3), obj_get_value(obj, 4), obj_get_value(obj, 5),
              obj_get_value(obj, 6), obj_get_value(obj, 7), obj_get_value(obj, 8),
              obj_get_value(obj, 9), obj_get_value(obj, 10), obj_get_value(obj, 11),
              obj_get_value(obj, 12), obj_get_value(obj, 13), obj_get_value(obj, 14),
              obj_get_value(obj, 15),
              obj_get_weight(obj), obj_get_cost(obj), obj_get_cost_per_day(obj), obj_get_level(obj));

      /*
       * Do we have script(s) attached ?
       */
      script_save_to_disk(fp, obj, OBJ_TRIGGER);

      fprintf(fp, "Z\n%d\n", obj_get_size(obj));
      /*
       * Do we have extra descriptions?
       */
      for (const extra_descr_data &ex_desc : obj_get_ex_description(obj)) { /* Yes, save them too. */
        /*
         * Sanity check to prevent nasty protection faults.
         */
        if (ex_desc.keyword.empty() || ex_desc.description.empty()) {
          mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
          continue;
        }
        strlcpy(buf, ex_desc.description.c_str(), sizeof(buf));
        strip_cr(buf);
        fprintf(fp, "E\n"
                    "%s~\n"
                    "%s~\n",
                ex_desc.keyword.c_str(), buf);
      }
      /*
       * Do we have affects?
       */
      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
        if (obj_get_affected(obj)[counter2].modifier)
          fprintf(fp, "A\n"
                      "%d %d %d\n",
                  obj_get_affected(obj)[counter2].location,
                  obj_get_affected(obj)[counter2].modifier, obj_get_affected(obj)[counter2].specific);
      /* Do we have spells? */
      if (obj_get_sbinfo(obj)) { /*. Yep, save them too . */
        for (counter2 = 0; counter2 < SKILL_TABLE_SIZE; counter2++) {
          if (obj_get_sbinfo(obj)[counter2].spellname == 0) {
            break;
          }
          fprintf(fp, "S\n"
                      "%d %d\n",
                  obj_get_sbinfo(obj)[counter2].spellname, obj_get_sbinfo(obj)[counter2].pages);
          continue;
        }
      }
    }
  }

  /*
   * Write the final line, close the file.
   */
  fprintf(fp, "$~\n");
  if (!atomic_commit(&af))
    return false;
  snprintf(buf, sizeof(buf), "%s", cmfname);

  if (in_save_list(g, zone_num->number, SL_OBJ)) {
    remove_from_save_list(g, zone_num->number, SL_OBJ);
    create_world_index(g, zone_num->number, "obj");
    mud_log("GenOLC: save_objects: Saving objects '%s'", buf);
  }
  return true;
}

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
  mud_log("GenOLC: delete_object: Deleting object #%d (%s).", obj_get_vnum(g, obj), obj_get_short_description(obj));

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
  free_proto_script(obj, OBJ_TRIGGER);

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
        case 'P':
          if (zone_get_cmd(g, zone, cmd_no)->arg3 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            zone_get_cmd(g, zone, cmd_no)->arg3    = NOTHING;
            changed                             = true;
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
