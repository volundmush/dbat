/************************************************************************
 * Generic OLC Library - Objects / genobj.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
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

static obj_rnum adjust_objects(GameWorld *g, obj_rnum refpt);
static bool copy_object_preserve(struct obj_data *to, struct obj_data *from);
static obj_rnum index_object(GameWorld *g, struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum);
static obj_rnum insert_object(GameWorld *g, struct obj_data *obj, obj_vnum ovnum);
static int update_objects(GameWorld *g, struct obj_data *refobj);

static bool copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);

obj_rnum add_object(GameWorld *g, struct obj_data *newobj, obj_vnum ovnum)
{
  obj_rnum found  = NOTHING;
  zone_rnum rznum = real_zone_by_thing(g, ovnum);

  /*
   * Write object to internal tables.
   */
  obj_set_item_number(newobj, real_object(g, ovnum));
  if (obj_get_item_number(newobj) != NOTHING) {
    copy_object(g, &g->obj_proto[obj_get_item_number(newobj)], newobj);
    update_objects(g, &g->obj_proto[obj_get_item_number(newobj)]);
    add_to_save_list(g, g->zones[rznum].number, SL_OBJ);
    return obj_get_item_number(newobj);
  }

  found = insert_object(g, newobj, ovnum);
  adjust_objects(g, found);
  add_to_save_list(g, g->zones[rznum].number, SL_OBJ);
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
  struct obj_data *obj, swap;
  int count = 0;

  for (obj = g->objects; obj; obj = obj_get_next(obj)) {
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
    obj_set_contains(obj, obj_get_contains(&swap));
    obj_set_next_content(obj, obj_get_next_content(&swap));
    obj_set_next(obj, obj_get_next(&swap));
    obj_set_sitting(obj, obj_get_sitting(&swap));
    obj_set_user(obj, obj_get_user(&swap));
    obj_set_target(obj, obj_get_target(&swap));

    /* This copy keeps its identity: scripts and the lookup table find it by id. */
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
    for (struct obj_data *inner = obj_get_contains(obj); inner; inner = obj_get_next_content(inner))
      contents += obj_get_weight(inner);
    obj_set_weight(obj, obj_get_weight(obj) + contents);
    change = obj_get_weight(obj) - obj_get_weight(&swap);
    obj_weight_changed(g, obj, change);
  }

  return count;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Adjust the internal values of other objects as if something was inserted at the given array index.
 * Might also be useful to make 'holes' in the array for some reason.
 */
static obj_rnum adjust_objects(GameWorld *g, obj_rnum refpt)
{
  shop_rnum shop;
  int i, cmd_no;
  zone_rnum zone;
  struct obj_data *obj;

  if (refpt < 0 || refpt > g->top_obj)
    return NOTHING;

  /*
   * Renumber live objects.
   */
  for (obj = g->objects; obj; obj = obj_get_next(obj))
    obj_set_item_number(obj, obj_get_item_number(obj) + ((obj_get_item_number(obj) != NOTHING && obj_get_item_number(obj) >= refpt)));

  /*
   * Renumber zone table.
   */
  for (zone = 0; zone <= g->top_zone; zone++) {
    for (cmd_no = 0; zone_get_cmd(g, zone, cmd_no)->command != 'S'; cmd_no++) {
      switch (zone_get_cmd(g, zone, cmd_no)->command) {
        case 'P':
          zone_get_cmd(g, zone, cmd_no)->arg3 += (zone_get_cmd(g, zone, cmd_no)->arg3 >= refpt);
          /*
         * No break here - drop into next case.
         */
          [[fallthrough]];
        case 'O':
        case 'G':
        case 'E':
          zone_get_cmd(g, zone, cmd_no)->arg1 += (zone_get_cmd(g, zone, cmd_no)->arg1 >= refpt);
          break;
        case 'R':
          zone_get_cmd(g, zone, cmd_no)->arg2 += (zone_get_cmd(g, zone, cmd_no)->arg2 >= refpt);
          break;
      }
    }
  }

  /*
   * Renumber shop produce.
   */
  for (shop = 0; shop <= g->top_shop; shop++)
    for (i = 0; rshop_get_product(g, shop, i) != NOTHING; i++)
      rshop_set_product(g, shop, i, rshop_get_product(g, shop, i) + (rshop_get_product(g, shop, i) >= refpt));

  return refpt;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Function handle the insertion of an object within the prototype framework.  Note that this does not adjust internal values
 * of other objects, use add_object() for that.
 */
static obj_rnum insert_object(GameWorld *g, struct obj_data *obj, obj_vnum ovnum)
{
  obj_rnum i;

  g->top_obj++;
  resize_array(g->obj_index, g->top_obj, g->top_obj + 1);
  resize_array(g->obj_proto, g->top_obj, g->top_obj + 1);

  /*
   * Start counting through both tables.
   */
  for (i = g->top_obj; i > 0; i--) {
    /*
     * Check if current virtual is bigger than our virtual number.
     */
    if (ovnum > g->obj_index[i - 1].vnum)
      return index_object(g, obj, ovnum, i);

    /* Copy over the object that should be here. */
    g->obj_index[i]             = g->obj_index[i - 1];
    g->obj_proto[i]             = g->obj_proto[i - 1];
    obj_set_item_number(&g->obj_proto[i], i);
  }

  /* Not found, place at 0. */
  return index_object(g, obj, ovnum, 0);
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

static obj_rnum index_object(GameWorld *g, struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum)
{
  if (obj == nullptr || ovnum < 0 || ornum < 0 || ornum > g->top_obj)
    return NOWHERE;

  obj_set_item_number(obj, ornum);
  g->obj_index[ornum].vnum   = ovnum;
  g->obj_index[ornum].number = 0;
  g->obj_index[ornum].func   = nullptr;

  copy_object_preserve(&g->obj_proto[ornum], obj);
  obj_set_in_room(&g->obj_proto[ornum], NOWHERE);

  return ornum;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

bool save_objects(GameWorld *g, zone_rnum zone_num)
{
  char cmfname[128], buf[MAX_STRING_LENGTH];
  char ebuf1[MAX_STRING_LENGTH], ebuf2[MAX_STRING_LENGTH];
  char ebuf3[MAX_STRING_LENGTH], ebuf4[MAX_STRING_LENGTH];
  char wbuf1[MAX_STRING_LENGTH], wbuf2[MAX_STRING_LENGTH];
  char wbuf3[MAX_STRING_LENGTH], wbuf4[MAX_STRING_LENGTH];
  char pbuf1[MAX_STRING_LENGTH], pbuf2[MAX_STRING_LENGTH];
  char pbuf3[MAX_STRING_LENGTH], pbuf4[MAX_STRING_LENGTH];
  IDXTYPE counter;
  int counter2;
  obj_rnum realcounter;
  struct atomic_file af;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  if (zone_num < 0 || zone_num > g->top_zone) {
    mud_log("SYSERR: OasisOLC: save_objects: Invalid real zone number %d. (0-%d)", zone_num, g->top_zone);
    return false;
  }

  snprintf(cmfname, sizeof(cmfname), "%s%d.obj", OBJ_PREFIX, g->zones[zone_num].number);
  if (!(fp = atomic_open(&af, cmfname, false))) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: OLC: Cannot open objects file %s!", cmfname);
    return false;
  }
  /*
   * Start running through all objects in this zone.
   */
  for (counter = genolc_zone_bottom(g, zone_num); counter <= g->zones[zone_num].top; counter++) {
    if ((realcounter = real_object(g, counter)) != NOTHING) {
      if (obj_get_action_description(obj = &g->obj_proto[realcounter])) {
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
      if (obj_get_ex_description(obj)) { /* Yes, save them too. */
        for (ex_desc = obj_get_ex_description(obj); ex_desc; ex_desc = ex_desc->next) {
          /*
           * Sanity check to prevent nasty protection faults.
           */
          if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description) {
            mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
            continue;
          }
          strlcpy(buf, ex_desc->description, sizeof(buf));
          strip_cr(buf);
          fprintf(fp, "E\n"
                      "%s~\n"
                      "%s~\n",
                  ex_desc->keyword, buf);
        }
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

  if (in_save_list(g, g->zones[zone_num].number, SL_OBJ)) {
    remove_from_save_list(g, g->zones[zone_num].number, SL_OBJ);
    create_world_index(g, g->zones[zone_num].number, "obj");
    mud_log("GenOLC: save_objects: Saving objects '%s'", buf);
  }
  return true;
}

/*
 * Free all, unconditionally.
 */
void free_object_strings(GameWorld *g, struct obj_data *obj)
{
#if 0 /* Debugging, do not enable. */
  struct obj_data *t;
  int i = 0;

  for (t = g->objects; t; t = t->next) {
    if (t == obj) {
      i++;
      continue;
    }
    assert(obj->name != t->name);
    assert(obj->description != t->description);
    assert(obj->short_description != t->short_description);
    assert(obj->action_description != t->action_description);
    assert(obj->ex_description != t->ex_description);
  }
  assert(i <= 1);
#endif

  if (obj_get_name(obj))
    free(obj_get_name(obj));
  if (obj_get_description(obj))
    free(obj_get_description(obj));
  if (obj_get_short_description(obj))
    free(obj_get_short_description(obj));
  if (obj_get_action_description(obj))
    free(obj_get_action_description(obj));
  if (obj_get_ex_description(obj))
    free_ex_descriptions(obj_get_ex_description(obj));
}

/*
 * For object instances that are not the prototype.
 */
void free_object_strings_proto(GameWorld *g, struct obj_data *obj)
{
  obj_rnum robj_num = obj_get_item_number(obj);

  if (obj_get_name(obj) && obj_get_name(obj) != obj_get_name(&g->obj_proto[robj_num]))
    free(obj_get_name(obj));
  if (obj_get_description(obj) && obj_get_description(obj) != obj_get_description(&g->obj_proto[robj_num]))
    free(obj_get_description(obj));
  if (obj_get_short_description(obj) && obj_get_short_description(obj) != obj_get_short_description(&g->obj_proto[robj_num]))
    free(obj_get_short_description(obj));
  if (obj_get_action_description(obj) && obj_get_action_description(obj) != obj_get_action_description(&g->obj_proto[robj_num]))
    free(obj_get_action_description(obj));
  if (obj_get_ex_description(obj)) {
    struct extra_descr_data *thised, *plist, *next_one; /* O(horrible) */
    int ok_key, ok_desc, ok_item;
    for (thised = obj_get_ex_description(obj); thised; thised = next_one) {
      next_one = thised->next;
      for (ok_item = ok_key = ok_desc = 1, plist = obj_get_ex_description(&g->obj_proto[robj_num]); plist; plist = plist->next) {
        if (plist->keyword == thised->keyword)
          ok_key = 0;
        if (plist->description == thised->description)
          ok_desc = 0;
        if (plist == thised)
          ok_item = 0;
      }
      if (thised->keyword && ok_key)
        free(thised->keyword);
      if (thised->description && ok_desc)
        free(thised->description);
      if (ok_item)
        delete thised;
    }
  }
}

static void copy_object_strings(struct obj_data *to, struct obj_data *from)
{
  obj_set_name(to, obj_get_name(from) ? strdup(obj_get_name(from)) : nullptr);
  obj_set_description(to, obj_get_description(from) ? strdup(obj_get_description(from)) : nullptr);
  obj_set_short_description(to, obj_get_short_description(from) ? strdup(obj_get_short_description(from)) : nullptr);
  obj_set_action_description(to, obj_get_action_description(from) ? strdup(obj_get_action_description(from)) : nullptr);

  if (obj_get_ex_description(from))
    copy_ex_descriptions(obj_ex_description_slot(to), obj_get_ex_description(from));
  else
    obj_set_ex_description(to, nullptr);
}

bool copy_object(GameWorld *g, struct obj_data *to, struct obj_data *from)
{
  free_object_strings(g, to);
  return copy_object_main(to, from, true);
}

static bool copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, false);
}

static bool copy_object_main(struct obj_data *to, struct obj_data *from, int free_object)
{
  *to = *from;
  copy_object_strings(to, from);
  return true;
}

static struct obj_data *first_object_of_proto(GameWorld *g, const struct obj_data *proto)
{
  struct obj_data *obj;

  for (obj = g->objects; obj; obj = obj_get_next(obj))
    if (obj_get_item_number(obj) == obj_get_item_number(proto))
      return obj;
  return nullptr;
}

obj_rnum delete_object(GameWorld *g, obj_rnum rnum)
{
  obj_rnum i;
  zone_rnum zrnum;
  struct obj_data *obj, *tmp;
  shop_rnum shop;
  int j, cmd_no;
  zone_rnum zone;

  if (rnum == NOTHING || rnum > g->top_obj)
    return NOTHING;

  obj = &g->obj_proto[rnum];

  zrnum = real_zone_by_thing(g, obj_get_vnum(g, obj));
  add_to_save_list(g, g->zones[zrnum].number, SL_OBJ);

  /* This is something you might want to read about in the logs. */
  mud_log("GenOLC: delete_object: Deleting object #%d (%s).", obj_get_vnum(g, obj), obj_get_short_description(obj));

  /* extract_obj() may free more than tmp, so start again from the top each time. */
  while ((tmp = first_object_of_proto(g, obj))) {
    /* extract_obj() will just axe contents. */
    while (obj_get_contains(tmp)) {
      struct obj_data *content = obj_get_contains(tmp);

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

  /* Adjust rnums of all other objects. */
  for (tmp = g->objects; tmp; tmp = obj_get_next(tmp)) {
    obj_set_item_number(tmp, obj_get_item_number(tmp) - ((obj_get_item_number(tmp) > rnum)));
  }

  /* No copy is left to share these, now that they have all been extracted. */
  free_object_strings(g, obj);
  free_proto_script(obj, OBJ_TRIGGER);

  for (i = rnum; i < g->top_obj; i++) {
    g->obj_index[i]             = g->obj_index[i + 1];
    g->obj_proto[i]             = g->obj_proto[i + 1];
    obj_set_item_number(&g->obj_proto[i], i);
  }

  g->top_obj--;
  resize_array(g->obj_index, g->top_obj + 2, g->top_obj + 1);
  resize_array(g->obj_proto, g->top_obj + 2, g->top_obj + 1);

  for (shop = 0; shop <= g->top_shop; shop++)
    for (j = 0; rshop_get_product(g, shop, j) != NOTHING; j++)
      rshop_set_product(g, shop, j, rshop_get_product(g, shop, j) - (rshop_get_product(g, shop, j) > rnum));

  /* Renumber zone table. */
  for (zone = 0; zone <= g->top_zone; zone++) {
    bool changed = false;
    for (cmd_no = 0; zone_get_cmd(g, zone, cmd_no)->command != 'S'; cmd_no++) {
      switch (zone_get_cmd(g, zone, cmd_no)->command) {
        case 'P':
          if (zone_get_cmd(g, zone, cmd_no)->arg3 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            zone_get_cmd(g, zone, cmd_no)->arg3    = NOTHING;
            changed                             = true;
          } else
            zone_get_cmd(g, zone, cmd_no)->arg3 -= (zone_get_cmd(g, zone, cmd_no)->arg3 > rnum);
          break;
        case 'O':
        case 'G':
        case 'E':
          if (zone_get_cmd(g, zone, cmd_no)->arg1 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            zone_get_cmd(g, zone, cmd_no)->arg1    = NOTHING;
            changed                             = true;
          } else
            zone_get_cmd(g, zone, cmd_no)->arg1 -= (zone_get_cmd(g, zone, cmd_no)->arg1 > rnum);
          break;
        case 'R':
          if (zone_get_cmd(g, zone, cmd_no)->arg2 == rnum) {
            zone_get_cmd(g, zone, cmd_no)->command = '*';
            zone_get_cmd(g, zone, cmd_no)->arg2    = NOTHING;
            changed                             = true;
          } else
            zone_get_cmd(g, zone, cmd_no)->arg2 -= (zone_get_cmd(g, zone, cmd_no)->arg2 > rnum);
          break;
      }
      if (changed) {
        // Do something if the command was changed, e.g., save the zone or log the change
        add_to_save_list(g, g->zones[zone].number, SL_ZON);
      }
    }
  }
  return rnum;
}
