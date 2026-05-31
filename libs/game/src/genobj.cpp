/************************************************************************
 * Generic OLC Library - Objects / genobj.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/db/shops.h"
#include "dbat/game/dg_scripts.h"
#include "dbat/game/genobj.h"
#include "dbat/game/genolc.h"
#include "dbat/game/genzon.h"
#include "dbat/game/utils.h"
#include "dbat/game/handler.h"
#include "dbat/game/dg_olc.h"
#include "dbat/game/shop.h"

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);

obj_vnum add_object(struct obj_data *newobj, obj_vnum ovnum)
{
  int found = NOTHING;
  zone_vnum znum = virtual_zone_by_thing(ovnum);

  /*
   * Write object to internal tables.
   */
  if (auto proto = obj_proto_by_id(ovnum); proto)
  {
    copy_object(proto, newobj);
    update_objects(proto);
    add_to_save_list(znum, SL_OBJ);
    return newobj->vnum;
  }

  struct obj_data *obj = NULL;
  CREATE(obj, struct obj_data, 1);
  copy_object(obj, newobj);
  obj_proto_put(ovnum, obj);

  add_to_save_list(znum, SL_OBJ);
  return ovnum;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Fix all existing objects to have these values.
 * We need to run through each and every object currently in the
 * game to see which ones are pointing to this prototype.
 * if object is pointing to this prototype, then we need to replace it
 * with the new one.
 */
int update_objects(struct obj_data *refobj)
{
  struct obj_data *obj, swap;
  int count = 0;

  for (obj = object_list; obj; obj = obj->next)
  {
    if (obj->vnum != refobj->vnum)
      continue;

    count++;

    /* Update the existing object but save a copy for private information. */
    swap = *obj;
    *obj = *refobj;

    /* Copy game-time dependent variables over. */
    IN_ROOM(obj) = swap.in_room;
    obj->carried_by = swap.carried_by;
    obj->worn_by = swap.worn_by;
    obj->worn_on = swap.worn_on;
    obj->in_obj = swap.in_obj;
    obj->contains = swap.contains;
    obj->next_content = swap.next_content;
    obj->next = swap.next;
  }

  return count;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------------------------------------------------------------ */

int save_objects(struct zone_data *zone)
{
  char cmfname[128], buf[MAX_STRING_LENGTH];
  char ebuf1[MAX_STRING_LENGTH], ebuf2[MAX_STRING_LENGTH];
  char ebuf3[MAX_STRING_LENGTH], ebuf4[MAX_STRING_LENGTH];
  char wbuf1[MAX_STRING_LENGTH], wbuf2[MAX_STRING_LENGTH];
  char wbuf3[MAX_STRING_LENGTH], wbuf4[MAX_STRING_LENGTH];
  char pbuf1[MAX_STRING_LENGTH], pbuf2[MAX_STRING_LENGTH];
  char pbuf3[MAX_STRING_LENGTH], pbuf4[MAX_STRING_LENGTH];
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

if(!zone) {
    log("SYSERR: OasisOLC: save_objects: Invalid zone!");
    return FALSE;
  }

  snprintf(cmfname, sizeof(cmfname), "%s%d.new", OBJ_PREFIX, zone->number);
  if (!(fp = fopen(cmfname, "w+")))
  {
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: OLC: Cannot open objects file %s!", cmfname);
    return FALSE;
  }
  /*
   * Start running through all objects in this zone.
   */
  for (counter = zone->bot; counter <= zone->top; counter++)
  {
    auto obj = obj_proto_by_id(counter);
    if (!obj)
      continue;
    if (obj)
    {
      strncpy(buf, obj->action_description, sizeof(buf) - 1);
      strip_cr(buf);
    }
    else
      *buf = '\0';

    fprintf(fp,
            "#%d\n"
            "%s~\n"
            "%s~\n"
            "%s~\n"
            "%s~\n",

            GET_OBJ_VNUM(obj),
            (obj->name && *obj->name) ? obj->name : "undefined",
            (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
            (obj->description && *obj->description) ? obj->description : "undefined",
            buf);

    sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[0]);
    sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[1]);
    sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[2]);
    sprintascii(ebuf4, GET_OBJ_EXTRA(obj)[3]);
    sprintascii(wbuf1, GET_OBJ_WEAR(obj)[0]);
    sprintascii(wbuf2, GET_OBJ_WEAR(obj)[1]);
    sprintascii(wbuf3, GET_OBJ_WEAR(obj)[2]);
    sprintascii(wbuf4, GET_OBJ_WEAR(obj)[3]);
    sprintascii(pbuf1, GET_OBJ_PERM(obj)[0]);
    sprintascii(pbuf2, GET_OBJ_PERM(obj)[1]);
    sprintascii(pbuf3, GET_OBJ_PERM(obj)[2]);
    sprintascii(pbuf4, GET_OBJ_PERM(obj)[3]);

    fprintf(fp,
            "%d %s %s %s %s %s %s %s %s %s %s %s %s\n"
            "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"
            "%" I64T " %d %d %d\n",

            GET_OBJ_TYPE(obj),
            ebuf1, ebuf2, ebuf3, ebuf4,
            wbuf1, wbuf2, wbuf3, wbuf4,
            pbuf1, pbuf2, pbuf3, pbuf4,
            GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
            GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5),
            GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7), GET_OBJ_VAL(obj, 8),
            GET_OBJ_VAL(obj, 9), GET_OBJ_VAL(obj, 10), GET_OBJ_VAL(obj, 11),
            GET_OBJ_VAL(obj, 12), GET_OBJ_VAL(obj, 13), GET_OBJ_VAL(obj, 14),
            GET_OBJ_VAL(obj, 15),
            GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), 0, GET_OBJ_LEVEL(obj));

    /*
     * Do we have script(s) attached ?
     */
    script_save_to_disk(fp, obj, OBJ_TRIGGER);

    fprintf(fp, "Z\n%d\n", GET_OBJ_SIZE(obj));
    /*
     * Do we have extra descriptions?
     */
    if (obj->ex_description)
    { /* Yes, save them too. */
      for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
      {
        /*
         * Sanity check to prevent nasty protection faults.
         */
        if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description)
        {
          mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
          continue;
        }
        strncpy(buf, ex_desc->description, sizeof(buf) - 1);
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
      if (obj->affected[counter2].modifier)
        fprintf(fp, "A\n"
                    "%d %d %d\n",
                obj->affected[counter2].location,
                obj->affected[counter2].modifier, obj->affected[counter2].specific);
  }

  /*
   * Write the final line, close the file.
   */
  fprintf(fp, "$~\n");
  fclose(fp);
  snprintf(buf, sizeof(buf), "%s%d.obj", OBJ_PREFIX, zone->number);
  remove(buf);
  rename(cmfname, buf);

  if (in_save_list(zone->number, SL_OBJ))
  {
    remove_from_save_list(zone->number, SL_OBJ);
    create_world_index(zone->number, "obj");
    log("GenOLC: save_objects: Saving objects '%s'", buf);
  }
  return TRUE;
}

/*
 * Free all, unconditionally.
 */
void free_object_strings(struct obj_data *obj)
{
#if 0 /* Debugging, do not enable. */
  extern struct obj_data *object_list;
  struct obj_data *t;
  int i = 0;

  for (t = object_list; t; t = t->next) {
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

  if (obj->name)
    free(obj->name);
  if (obj->description)
    free(obj->description);
  if (obj->short_description)
    free(obj->short_description);
  if (obj->action_description)
    free(obj->action_description);
  if (obj->ex_description)
    free_ex_descriptions(obj->ex_description);
}

/*
 * For object instances that are not the prototype.
 */
void free_object_strings_proto(struct obj_data *obj)
{
  int robj_num = GET_OBJ_RNUM(obj);
  struct obj_data *proto = obj_proto_by_id(GET_OBJ_VNUM(obj));

  if (obj->name && obj->name != proto->name)
    free(obj->name);
  if (obj->description && obj->description != proto->description)
    free(obj->description);
  if (obj->short_description && obj->short_description != proto->short_description)
    free(obj->short_description);
  if (obj->action_description && obj->action_description != proto->action_description)
    free(obj->action_description);
  if (obj->ex_description)
  {
    struct extra_descr_data *thised, *plist, *next_one; /* O(horrible) */
    int ok_key, ok_desc, ok_item;
    for (thised = obj->ex_description; thised; thised = next_one)
    {
      next_one = thised->next;
      for (ok_item = ok_key = ok_desc = 1, plist = proto->ex_description; plist; plist = plist->next)
      {
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
        free(thised);
    }
  }
}

void copy_object_strings(struct obj_data *to, struct obj_data *from)
{
  to->name = from->name ? strdup(from->name) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->short_description = from->short_description ? strdup(from->short_description) : NULL;
  to->action_description = from->action_description ? strdup(from->action_description) : NULL;

  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;
}

int copy_object(struct obj_data *to, struct obj_data *from)
{
  free_object_strings(to);
  return copy_object_main(to, from, TRUE);
}

int copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, FALSE);
}

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object)
{
  *to = *from;
  copy_object_strings(to, from);
  return TRUE;
}

int delete_object(obj_vnum vnum)
{
  zone_rnum zrnum;
  struct obj_data *tmp;
  int shop, j, cmd_no;

  auto obj = obj_proto_by_id(vnum);

  if (!obj)
    return NOTHING;
  
  auto zone = zone_by_id(virtual_zone_by_thing(vnum));

  add_to_save_list(zone->number, SL_OBJ);

  obj_proto_delete(obj->vnum);
  // TODO: ensure the pointer is actually freed darnit!

  /* This is something you might want to read about in the logs. */
  log("GenOLC: delete_object: Deleting object #%d (%s).", GET_OBJ_VNUM(obj), obj->short_description);

  for (tmp = object_list; tmp; tmp = tmp->next)
  {
    if (tmp->vnum != obj->vnum)
      continue;

    /* extract_obj() will just axe contents. */
    if (tmp->contains)
    {
      struct obj_data *this_content, *next_content;
      for (this_content = tmp->contains; this_content; this_content = next_content)
      {
        next_content = this_content->next_content;
        if (auto room = obj_room_get(tmp); room)
        {
          /* Transfer stuff from object to room. */
          obj_from_obj(this_content);
          obj_to_room(this_content, room);
        }
        else if (tmp->worn_by || tmp->carried_by)
        {
          /* Transfer stuff from object to person inventory. */
          obj_from_char(this_content);
          obj_to_char(this_content, tmp->carried_by);
        }
        else if (tmp->in_obj)
        {
          /* Transfer stuff from object to containing object. */
          obj_from_obj(this_content);
          obj_to_obj(this_content, tmp->in_obj);
        }
      }
    }
    /* Remove from object_list, etc. - handles weight changes, and similar. */
    extract_obj(tmp);
  }

  return vnum;
}
