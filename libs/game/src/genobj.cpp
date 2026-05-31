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

#include <string.h>
#include <stddef.h>

static_assert(sizeof(struct obj_proto_data) == offsetof(struct obj_data, in_room),
              "object prototype fields must stay prefix-compatible with object instances");

static void free_trig_proto_list(struct trig_proto_list *list)
{
  while (list) {
    struct trig_proto_list *next = list->next;
    free(list);
    list = next;
  }
}

static struct trig_proto_list *copy_trig_proto_list(const struct trig_proto_list *from)
{
  struct trig_proto_list *head = NULL, *tail = NULL;

  for (; from; from = from->next) {
    struct trig_proto_list *node;
    CREATE(node, struct trig_proto_list, 1);
    node->vnum = from->vnum;
    if (tail)
      tail->next = node;
    else
      head = node;
    tail = node;
  }

  return head;
}

void obj_proto_free_strings(struct obj_proto_data *obj)
{
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
  obj->name = NULL;
  obj->description = NULL;
  obj->short_description = NULL;
  obj->action_description = NULL;
  obj->ex_description = NULL;
}

void obj_proto_free(struct obj_proto_data *obj)
{
  if (!obj)
    return;
  obj_proto_free_strings(obj);
  free_trig_proto_list(obj->proto_script);
  free(obj);
}

void obj_proto_copy(struct obj_proto_data *to, const struct obj_proto_data *from)
{
  obj_proto_free_strings(to);
  free_trig_proto_list(to->proto_script);
  *to = *from;
  to->name = from->name ? strdup(from->name) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->short_description = from->short_description ? strdup(from->short_description) : NULL;
  to->action_description = from->action_description ? strdup(from->action_description) : NULL;
  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;
  to->proto_script = copy_trig_proto_list(from->proto_script);
}

void obj_proto_from_instance(struct obj_proto_data *to, const struct obj_data *from)
{
  obj_proto_free_strings(to);
  free_trig_proto_list(to->proto_script);

  to->vnum = from->vnum;
  memcpy(to->value, from->value, sizeof(to->value));
  to->type_flag = from->type_flag;
  to->level = from->level;
  memcpy(to->wear_flags, from->wear_flags, sizeof(to->wear_flags));
  memcpy(to->extra_flags, from->extra_flags, sizeof(to->extra_flags));
  to->weight = from->weight;
  to->cost = from->cost;
  to->timer = from->timer;
  memcpy(to->bitvector, from->bitvector, sizeof(to->bitvector));
  to->size = from->size;
  memcpy(to->affected, from->affected, sizeof(to->affected));

  to->name = from->name ? strdup(from->name) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->short_description = from->short_description ? strdup(from->short_description) : NULL;
  to->action_description = from->action_description ? strdup(from->action_description) : NULL;
  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;
  to->proto_script = copy_trig_proto_list(from->proto_script);
}

void obj_apply_proto_to_instance(struct obj_data *to, const struct obj_proto_data *from)
{
  to->vnum = from->vnum;
  memcpy(to->value, from->value, sizeof(to->value));
  to->type_flag = from->type_flag;
  to->level = from->level;
  memcpy(to->wear_flags, from->wear_flags, sizeof(to->wear_flags));
  memcpy(to->extra_flags, from->extra_flags, sizeof(to->extra_flags));
  to->weight = from->weight;
  to->cost = from->cost;
  to->timer = from->timer;
  memcpy(to->bitvector, from->bitvector, sizeof(to->bitvector));
  to->size = from->size;
  memcpy(to->affected, from->affected, sizeof(to->affected));

  free_object_strings(to);
  to->name = from->name ? strdup(from->name) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->short_description = from->short_description ? strdup(from->short_description) : NULL;
  to->action_description = from->action_description ? strdup(from->action_description) : NULL;
  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;

  free_trig_proto_list(to->proto_script);
  to->proto_script = copy_trig_proto_list(from->proto_script);
}

void obj_proto_to_instance(struct obj_data *to, const struct obj_proto_data *from)
{
  obj_apply_proto_to_instance(to, from);
}

obj_vnum add_object(struct obj_proto_data *newobj, obj_vnum ovnum)
{
  int found = NOTHING;
  zone_vnum znum = virtual_zone_by_thing(ovnum);

  /*
   * Write object to internal tables.
   */
  if (auto proto = obj_proto_by_id(ovnum); proto)
  {
    obj_proto_copy(proto, newobj);
    update_objects(proto);
    add_to_save_list(znum, SL_OBJ);
    return newobj->vnum;
  }

  struct obj_proto_data *obj = NULL;
  CREATE(obj, struct obj_proto_data, 1);
  obj_proto_copy(obj, newobj);
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
int update_objects(struct obj_proto_data *refobj)
{
  struct obj_data *obj;
  int count = 0;

  for (obj = object_list; obj; obj = obj->next)
  {
    if (obj->vnum != refobj->vnum)
      continue;

    count++;
    obj_apply_proto_to_instance(obj, refobj);
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
    obj_proto_script_save_to_disk(fp, obj);

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
  struct obj_proto_data tmp = {};
  obj_proto_from_instance(&tmp, from);
  obj_apply_proto_to_instance(to, &tmp);
  obj_proto_free_strings(&tmp);
  free_trig_proto_list(tmp.proto_script);
  return TRUE;
}

int copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  struct obj_proto_data tmp = {};
  obj_proto_from_instance(&tmp, from);
  obj_apply_proto_to_instance(to, &tmp);
  obj_proto_free_strings(&tmp);
  free_trig_proto_list(tmp.proto_script);
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

  obj_proto_free(obj);
  return vnum;
}
