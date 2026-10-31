/**************************************************************************
*  File: dg_variables.c                                                   *
*  Usage: contains the functions dealing with variable substitution.      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00 $                                           *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/db.h"
#include "dbat/screen.h"
#include "dbat/spells.h"
#include "dbat/oasis.h"
#include "dbat/class.h"
#include "dbat/races.h"

/* Utility functions */

/*
 * Thanks to James Long for his assistance in plugging the memory leak
 * that used to be here.   -- Welcor
 */
/* adds a variable with given name and value to trigger */
void add_var(struct trig_var_data **var_list, char *name, const char *value, long id)
{
  struct trig_var_data *vd;

  if (strchr(name, '.')) {
    mud_log("add_var() : Attempt to add illegal var: %s", name);
    return;
  }

  for (vd = *var_list; vd && strcasecmp(vd->name, name); vd = vd->next);

  if (vd && (!vd->context || vd->context == id)) {
    free(vd->value);
    CREATE(vd->value, char, strlen(value) + 1);
  }

  else {
    CREATE(vd, struct trig_var_data, 1);

    CREATE(vd->name, char, strlen(name) + 1);
    strcpy(vd->name, name); /* strcpy: ok*/

    CREATE(vd->value, char, strlen(value) + 1);

    vd->next    = *var_list;
    vd->context = id;
    *var_list   = vd;
  }

  strcpy(vd->value, value); /* strcpy: ok*/
}

/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(struct char_data *ch, char *skill)
{
  static char retval[16];
  int skillnum;

  skillnum = find_skill_num(skill, SKTYPE_SKILL);
  if (skillnum <= 0) return ("unknown skill");

  snprintf(retval, sizeof(retval), "%d", char_get_skill(ch, skillnum));
  return retval;
}

/*
   search through all the persons items, including containers
   and 0 if it doesnt exist, and greater then 0 if it does!
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000

   Now also searches by vnum -- Welcor
   Now returns the number of matching objects -- Welcor 02/04
*/

int item_in_list(char *item, obj_data *list)
{
  obj_data *i;
  int count = 0;

  if (!item || !*item)
    return 0;

  if (*item == UID_CHAR) {
    long id = atol(item + 1);

    for (i = list; i; i = i->next_content) {
      if (id == obj_get_id(i))
        count++;
      if (obj_get_type_flag(i) == ITEM_CONTAINER)
        count += item_in_list(item, i->contains);
    }
  } else if (is_number(item) > -1) { /* check for vnum */
    obj_vnum ovnum = atoi(item);

    for (i = list; i; i = i->next_content) {
      if (obj_get_vnum(i) == ovnum)
        count++;
      if (obj_get_type_flag(i) == ITEM_CONTAINER)
        count += item_in_list(item, i->contains);
    }
  } else {
    for (i = list; i; i = i->next_content) {
      if (isname(item, i->name))
        count++;
      if (obj_get_type_flag(i) == ITEM_CONTAINER)
        count += item_in_list(item, i->contains);
    }
  }
  return count;
}

/*
   BOOLEAN return, just check if a player or mob
   has an item of any sort, searched for by name
   or id.
   searching equipment as well as inventory,
   and containers.
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000
*/

int char_has_item(char *item, struct char_data *ch)
{

  /* If this works, no more searching needed */
  if (get_object_in_equip(ch, item) != NULL)
    return 1;

  if (item_in_list(item, ch->carrying) == 0)
    return 0;
  else
    return 1;
}

int text_processed(char *field, char *subfield, struct trig_var_data *vd,
                   char *str, size_t slen)
{
  char *p, *p2;
  char tmpvar[MAX_STRING_LENGTH];

  if (!strcasecmp(field, "strlen")) { /* strlen    */
    char limit[200];
    sprintf(limit, "%" SZT, strlen(vd->value));
    snprintf(str, slen, "%d", atoi(limit));
    return TRUE;
  } else if (!strcasecmp(field, "trim")) { /* trim      */
    /* trim whitespace from ends */
    snprintf(tmpvar, sizeof(tmpvar) - 1, "%s", vd->value); /* -1 to use later*/
    p  = tmpvar;
    p2 = tmpvar + strlen(tmpvar) - 1;
    while (*p && isspace(*p)) p++;
    while ((p <= p2) && isspace(*p2)) p2--;
    if (p > p2) { /* nothing left */
      *str = '\0';
      return TRUE;
    }
    *(++p2) = '\0'; /* +1 ok (see above) */
    snprintf(str, slen, "%s", p);
    return TRUE;
  } else if (!strcasecmp(field, "contains")) { /* contains  */
    if (str_str(vd->value, subfield))
      strcpy(str, "1");
    else
      strcpy(str, "0");
    return TRUE;
  } else if (!strcasecmp(field, "car")) { /* car       */
    char *car = vd->value;
    while (*car && !isspace(*car))
      *str++ = *car++;
    *str = '\0';
    return TRUE;

  } else if (!strcasecmp(field, "cdr")) { /* cdr       */
    char *cdr = vd->value;
    while (*cdr && !isspace(*cdr)) cdr++; /* skip 1st field */
    while (*cdr && isspace(*cdr)) cdr++;  /* skip to next */

    snprintf(str, slen, "%s", cdr);
    return TRUE;
  } else if (!strcasecmp(field, "charat")) { /* CharAt    */
    size_t len = strlen(vd->value), dgindex = atoi(subfield);
    if (dgindex > len || dgindex < 1)
      strcpy(str, "");
    else
      snprintf(str, slen, "%c", vd->value[dgindex - 1]);
    return TRUE;
  } else if (!strcasecmp(field, "mudcommand")) {
    /* find the mud command returned from this text */
    /* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
    /* depending on what patches you've got applied.                      */
    /* on older source bases:    extern struct command_info *cmd_info; */
    int length, cmd;
    for (length = strlen(vd->value), cmd = 0;
         *cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(cmd_info[cmd].command, vd->value, length))
        break;

    if (*cmd_info[cmd].command == '\n')
      *str = '\0';
    else
      snprintf(str, slen, "%s", cmd_info[cmd].command);
    return TRUE;
  }

  return FALSE;
}

/* sets str to be the value of var.field */
void find_replacement(void *go, struct script_data *sc, trig_data *trig, int type, char *var, char *field, char *subfield, char *str, size_t slen)
{
  struct trig_var_data *vd = NULL;
  char_data *ch, *c          = NULL, *rndm;
  obj_data *obj, *o          = NULL;
  struct room_data *room, *r = NULL;
  char *name;
  int num, count, i, j, doors;

  char *send_cmd[]       = {"msend ", "osend ", "wsend "};
  char *echo_cmd[]       = {"mecho ", "oecho ", "wecho "};
  char *echoaround_cmd[] = {"mechoaround ", "oechoaround ", "wechoaround "};
  char *door[]           = {"mdoor ", "odoor ", "wdoor "};
  char *force[]          = {"mforce ", "oforce ", "wforce "};
  char *load[]           = {"mload ", "oload ", "wload "};
  char *purge[]          = {"mpurge ", "opurge ", "wpurge "};
  char *teleport[]       = {"mteleport ", "oteleport ", "wteleport "};
  /* the x kills a 'shadow' warning in gcc. */
  char *xdamage[]  = {"mdamage ", "odamage ", "wdamage "};
  char *zoneecho[] = {"mzoneecho ", "ozoneecho ", "wzoneecho "};
  char *asound[]   = {"masound ", "oasound ", "wasound "};
  char *at[]       = {"mat ", "oat ", "wat "};
  /* there is no such thing as wtransform, thus the wecho below  */
  char *transform[] = {"mtransform ", "otransform ", "wecho "};
  char *recho[]     = {"mrecho ", "orecho ", "wrecho "};

  *str = '\0';

  /* X.global() will have a NULL trig */
  if (trig)
    for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
      if (!strcasecmp(vd->name, var))
        break;

  /* some evil waitstates could crash the mud if sent here with sc==NULL*/
  if (!vd && sc)
    for (vd = sc->global_vars; vd; vd = vd->next)
      if (!strcasecmp(vd->name, var) &&
          (vd->context == 0 || vd->context == sc->context))
        break;

  if (!*field) {
    if (vd)
      snprintf(str, slen, "%s", vd->value);
    else {
      if (!strcasecmp(var, "self")) {
        switch (type) {
          case MOB_TRIGGER:
            snprintf(str, slen, "%c%d", UID_CHAR, char_get_id((char_data *)go));
            break;
          case OBJ_TRIGGER:
            snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id((obj_data *)go));
            break;
          case WLD_TRIGGER:
            snprintf(str, slen, "%c%d", UID_CHAR, ((room_data *)go)->number + ROOM_ID_BASE);
            break;
        }
      } else if (!strcasecmp(var, "global")) {
        /* so "remote varname %global%" will work */
        snprintf(str, slen, "%d", ROOM_ID_BASE);
        return;
      } else if (!strcasecmp(var, "ctime"))
        snprintf(str, slen, "%ld", time(0));
      else if (!strcasecmp(var, "door"))
        snprintf(str, slen, "%s", door[type]);
      else if (!strcasecmp(var, "force"))
        snprintf(str, slen, "%s", force[type]);
      else if (!strcasecmp(var, "load"))
        snprintf(str, slen, "%s", load[type]);
      else if (!strcasecmp(var, "purge"))
        snprintf(str, slen, "%s", purge[type]);
      else if (!strcasecmp(var, "teleport"))
        snprintf(str, slen, "%s", teleport[type]);
      else if (!strcasecmp(var, "damage"))
        snprintf(str, slen, "%s", xdamage[type]);
      else if (!strcasecmp(var, "send"))
        snprintf(str, slen, "%s", send_cmd[type]);
      else if (!strcasecmp(var, "echo"))
        snprintf(str, slen, "%s", echo_cmd[type]);
      else if (!strcasecmp(var, "echoaround"))
        snprintf(str, slen, "%s", echoaround_cmd[type]);
      else if (!strcasecmp(var, "zoneecho"))
        snprintf(str, slen, "%s", zoneecho[type]);
      else if (!strcasecmp(var, "asound"))
        snprintf(str, slen, "%s", asound[type]);
      else if (!strcasecmp(var, "at"))
        snprintf(str, slen, "%s", at[type]);
      else if (!strcasecmp(var, "transform"))
        snprintf(str, slen, "%s", transform[type]);
      else if (!strcasecmp(var, "recho"))
        snprintf(str, slen, "%s", recho[type]);
      else
        *str = '\0';
    }

    return;
  }

  else {
    if (vd) {
      name = vd->value;

      switch (type) {
        case MOB_TRIGGER:
          ch = (char_data *)go;

          if ((o = get_object_in_equip(ch, name)))
            ;
          else if ((o = get_obj_in_list(name, ch->carrying)))
            ;
          else if (char_get_in_room(ch) != NOWHERE && (c = get_char_in_room(&world[char_get_in_room(ch)], name)))
            ;
          else if ((o = get_obj_in_list(name, world[char_get_in_room(ch)].contents)))
            ;
          else if ((c = get_char(name)))
            ;
          else if ((o = get_obj(name)))
            ;
          else if ((r = get_room(name))) {}

          break;
        case OBJ_TRIGGER:
          obj = (obj_data *)go;

          if ((c = get_char_by_obj(obj, name)))
            ;
          else if ((o = get_obj_by_obj(obj, name)))
            ;
          else if ((r = get_room(name))) {}

          break;
        case WLD_TRIGGER:
          room = (struct room_data *)go;

          if ((c = get_char_by_room(room, name)))
            ;
          else if ((o = get_obj_by_room(room, name)))
            ;
          else if ((r = get_room(name))) {}

          break;
      }
    }

    else {
      if (!strcasecmp(var, "self")) {
        switch (type) {
          case MOB_TRIGGER:
            c = (char_data *)go;
            r = NULL;
            o = NULL; /* NULL assignments added to avoid self to always be    */
            break;    /* the room.  - Welcor        */
          case OBJ_TRIGGER:
            o = (obj_data *)go;
            c = NULL;
            r = NULL;
            break;
          case WLD_TRIGGER:
            r = (struct room_data *)go;
            c = NULL;
            o = NULL;
            break;
        }
      }

      else if (!strcasecmp(var, "global")) {
        struct script_data *thescript = SCRIPT(&world[0]);
        *str                          = '\0';
        if (!thescript) {
          script_log("Attempt to find global var. Apparently the void has no script.");
          return;
        }
        for (vd = thescript->global_vars; vd; vd = vd->next)
          if (!strcasecmp(vd->name, field))
            break;

        if (vd)
          snprintf(str, slen, "%s", vd->value);

        return;
      } else if (!strcasecmp(var, "people")) {
        snprintf(str, slen, "%d", ((num = atoi(field)) > 0) ? trgvar_in_room(num) : 0);
        return;
      } else if (!strcasecmp(var, "time")) {
        if (!strcasecmp(field, "hour"))
          snprintf(str, slen, "%d", time_info.hours);
        else if (!strcasecmp(field, "day"))
          snprintf(str, slen, "%d", time_info.day + 1);
        else if (!strcasecmp(field, "month"))
          snprintf(str, slen, "%d", time_info.month + 1);
        else if (!strcasecmp(field, "year"))
          snprintf(str, slen, "%d", time_info.year);
        else *str = '\0';
        return;
      }
      /*

      %findobj.<room vnum X>(<object vnum/id/name>)%
        - count number of objects in room X with this name/id/vnum
      %findmob.<room vnum X>(<mob vnum Y>)%
        - count number of mobs in room X with vnum Y

for example you want to check how many PC's are in room with vnum 1204.
as PC's have the vnum -1...
you would type:
in any script:
%echo% players in room 1204: %findmob.1204(-1)%

Or say you had a bank, and you want a script to check the number of
bags
of gold (vnum: 1234)
in the vault (vnum: 453) now and then. you can just use
%findobj.453(1234)% and it will return the number of bags of gold.

**/

      /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
      else if (!strcasecmp(var, "findmob")) {
        if (!field || !*field || !subfield || !*subfield) {
          script_log("findmob.vnum(mvnum) - illegal syntax");
          strcpy(str, "0");
        } else {
          room_rnum rrnum = real_room(atoi(field));
          mob_vnum mvnum  = atoi(subfield);

          if (rrnum == NOWHERE) {
            script_log("findmob.vnum(ovnum): No room with vnum %d", atoi(field));
            strcpy(str, "0");
          } else {
            for (i = 0, ch = world[rrnum].people; ch; ch = ch->next_in_room)
              if (char_get_mob_vnum(ch) == mvnum)
                i++;

            snprintf(str, slen, "%d", i);
          }
        }
      }
      /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
      else if (!strcasecmp(var, "findobj")) {
        if (!field || !*field || !subfield || !*subfield) {
          script_log("findobj.vnum(ovnum) - illegal syntax");
          strcpy(str, "0");
        } else {
          room_rnum rrnum = real_room(atoi(field));

          if (rrnum == NOWHERE) {
            script_log("findobj.vnum(ovnum): No room with vnum %d", atoi(field));
            strcpy(str, "0");
          } else {
            /* item_in_list looks within containers as well. */
            snprintf(str, slen, "%d", item_in_list(subfield, world[rrnum].contents));
          }
        }
      } else if (!strcasecmp(var, "random")) {
        if (!strcasecmp(field, "char")) {
          rndm  = NULL;
          count = 0;

          if (type == MOB_TRIGGER) {
            ch = (char_data *)go;
            for (c = world[char_get_in_room(ch)].people; c; c = c->next_in_room)
              if ((c != ch) && valid_dg_target(c, DG_ALLOW_GODS) &&
                  char_can_see(ch, c)) {
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }

          else if (type == OBJ_TRIGGER) {
            for (c = world[obj_room((obj_data *)go)].people; c;
                 c = c->next_in_room)
              if (valid_dg_target(c, DG_ALLOW_GODS)) {
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }

          else if (type == WLD_TRIGGER) {
            for (c = ((struct room_data *)go)->people; c;
                 c = c->next_in_room)
              if (valid_dg_target(c, DG_ALLOW_GODS)) {

                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }

          if (rndm)
            snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(rndm));
          else
            *str = '\0';
        }

        else if (!strcasecmp(field, "dir")) {
          room_rnum in_room = NOWHERE;

          switch (type) {
            case WLD_TRIGGER:
              in_room = real_room(((struct room_data *)go)->number);
              break;
            case OBJ_TRIGGER:
              in_room = obj_room((struct obj_data *)go);
              break;
            case MOB_TRIGGER:
              in_room = char_get_in_room((struct char_data *)go);
              break;
          }
          if (in_room == NOWHERE) {
            *str = '\0';
          } else {
            doors = 0;
            room  = &world[in_room];
            for (i = 0; i < NUM_OF_DIRS; i++)
              if (room_get_dir_option(room, i))
                doors++;

            if (!doors) {
              *str = '\0';
            } else {
              for (;;) {
                doors = rand_number(0, NUM_OF_DIRS - 1);
                if (room_get_dir_option(room, doors))
                  break;
              }
              snprintf(str, slen, "%s", dirs[doors]);
            }
          }
        } else
          snprintf(str, slen, "%d", ((num = atoi(field)) > 0) ? rand_number(1, num) : 0);

        return;
      }
    }

    if (c) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      else if (!strcasecmp(field, "global")) { /* get global of something else */
        if (char_is_npc(c) && c->script) {
          find_replacement(go, c->script, NULL, MOB_TRIGGER,
                           subfield, NULL, NULL, str, slen);
        }
      }
      /* set str to some 'non-text' first */
      *str = '\x1';

      switch (tolower(*field)) {
        case 'a':
          if (!strcasecmp(field, "aaaaa")) {
            strcpy(str, "0");
          } else if (!strcasecmp(field, "affect")) {
            if (subfield && *subfield) {
              int affect = get_flag_by_name(affected_bits, subfield);
              if (affect != NOFLAG && char_aff_flagged(c, affect))
                strcpy(str, "1");
              else
                strcpy(str, "0");
            } else
              strcpy(str, "0");
          } else if (!strcasecmp(field, "alias"))
            snprintf(str, slen, "%s", char_get_pc_name(c));

          else if (!strcasecmp(field, "align")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              char_set_alignment(c, MAX(-1000, MIN(addition, 1000)));
            }
            snprintf(str, slen, "%d", char_get_alignment(c));
          }
          break;
        case 'b':
          if (!strcasecmp(field, "bank")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              char_set_bank_gold(c, char_get_bank_gold(c) + (addition));
            }
            snprintf(str, slen, "%d", char_get_gold(c));
          }
          break;
        case 'c':
          if (!strcasecmp(field, "canbeseen")) {
            if ((type == MOB_TRIGGER) && !char_can_see(((char_data *)go), c))
              strcpy(str, "0");
            else
              strcpy(str, "1");
          } else if (!strcasecmp(field, "carry")) {
            if (!char_is_npc(c) && CARRYING(c))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          } else if (!strcasecmp(field, "clan")) {
            strcpy(str, "0");
          } else if (!strcasecmp(field, "class")) {
            if (!char_is_npc(c))
              snprintf(str, slen, "%s", SENSEI_NAME(c));
            else
              snprintf(str, slen, "blank");
          }

          else if (!strcasecmp(field, "con")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max      = 100;
              char_set_aff_abils_con(c, char_get_aff_abils_con(c) + (addition));
              if (char_get_aff_abils_con(c) > max) char_set_aff_abils_con(c, max);
              if (char_get_aff_abils_con(c) < 3) char_set_aff_abils_con(c, 3);
            }
            snprintf(str, slen, "%d", char_get_aff_abils_con(c));
          } else if (!strcasecmp(field, "cha")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max      = 100;
              char_set_aff_abils_cha(c, char_get_aff_abils_cha(c) + (addition));
              if (char_get_aff_abils_cha(c) > max) char_set_aff_abils_cha(c, max);
              if (char_get_aff_abils_cha(c) < 3) char_set_aff_abils_cha(c, 3);
            }
            snprintf(str, slen, "%d", char_get_aff_abils_cha(c));
          }
          break;
        case 'd':
          if (!strcasecmp(field, "dead")) {
            if (char_aff_flagged(c, AFF_SPIRIT))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          } else if (!strcasecmp(field, "death")) {
            snprintf(str, slen, "%ld", char_get_deathtime(c));
          } else if (!strcasecmp(field, "dex")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max      = 100;
              char_set_aff_abils_dex(c, char_get_aff_abils_dex(c) + (addition));
              if (char_get_aff_abils_dex(c) > max) char_set_aff_abils_dex(c, max);
              if (char_get_aff_abils_dex(c) < 3) char_set_aff_abils_dex(c, 3);
            }
            snprintf(str, slen, "%d", char_get_aff_abils_dex(c));
          } else if (!strcasecmp(field, "drag")) {
            if (!char_is_npc(c) && char_get_drag(c))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          } else if (!strcasecmp(field, "drunk")) {
            if (subfield && *subfield) {
              int addition       = atoi(subfield);
              GET_COND(c, DRUNK) = MAX(-1, MIN(addition, 24));
            }
            snprintf(str, slen, "%d", GET_COND(c, DRUNK));
          }
          break;
        case 'e':
          if (!strcasecmp(field, "eq")) {
            int pos;
            if (!subfield || !*subfield)
              *str = '\0';
            else if (*subfield == '*') {
              for (i = 0, j = 0; i < NUM_WEARS; i++)
                if (char_get_equipment(c, i)) {
                  j++;
                  break;
                }
              if (j > 0)
                strcpy(str, "1");
              else
                *str = '\0';
            } else if ((pos = find_eq_pos_script(subfield)) < 0 || !char_get_equipment(c, pos))
              *str = '\0';
            else
              snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(char_get_equipment(c, pos)));
          }
          if (!strcasecmp(field, "exp")) {
            if (subfield && *subfield) {
              int64_t addition = MIN(atoll(subfield), 2100000000);

              gain_exp(c, addition);
            }
            snprintf(str, slen, "%" I64T "", char_get_exp(c));
          }
          break;
        case 'f':
          if (!strcasecmp(field, "fighting")) {
            if (char_get_fighting(c))
              snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(char_get_fighting(c)));
            else
              *str = '\0';
          } else if (!strcasecmp(field, "flying")) {
            if (char_aff_flagged(c, AFF_FLYING))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          } else if (!strcasecmp(field, "follower")) {
            if (!c->followers || !c->followers->follower)
              *str = '\0';
            else
              snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(c->followers->follower));
          }
          break;
        case 'g':
          if (!strcasecmp(field, "gold")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              char_set_gold(c, char_get_gold(c) + (addition));
            }
            snprintf(str, slen, "%d", char_get_gold(c));
          }
          break;
        case 'h':
          if (!strcasecmp(field, "has_item")) {
            if (!(subfield && *subfield))
              *str = '\0';
            else
              snprintf(str, slen, "%d", char_has_item(subfield, c));
          } else if (!strcasecmp(field, "hisher"))
            snprintf(str, slen, "%s", char_hshr(c));

          else if (!strcasecmp(field, "heshe"))
            snprintf(str, slen, "%s", char_hssh(c));

          else if (!strcasecmp(field, "himher"))
            snprintf(str, slen, "%s", char_hmhr(c));

          else if (!strcasecmp(field, "hitp")) {
            if (subfield && *subfield) {
              int64_t addition = atoll(subfield);
              if (addition > 0) {
                incCurHealth(c, addition, true);
              } else {
                decCurHealth(c, addition, 0);
              }

              update_pos(c);
            }
            snprintf(str, slen, "%" I64T "", GET_HIT(c));
          } else if (!strcasecmp(field, "hunger")) {
            if (subfield && *subfield) {
              int addition        = atoi(subfield);
              GET_COND(c, HUNGER) = MAX(-1, MIN(addition, 24));
            }
            snprintf(str, slen, "%d", GET_COND(c, HUNGER));
          }
          break;
        case 'i':
          if (!strcasecmp(field, "id"))
            snprintf(str, slen, "%d", char_get_id(c));

          /* new check for pc/npc status */
          else if (!strcasecmp(field, "is_pc")) {
            if (char_is_npc(c))
              strcpy(str, "0");
            else
              strcpy(str, "1");
          }

          else if (!strcasecmp(field, "inventory")) {
            if (subfield && *subfield) {
              for (obj = c->carrying; obj; obj = obj->next_content) {
                if (obj_get_vnum(obj) == atoi(subfield)) {
                  snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(obj)); /* arg given, found */
                  return;
                }
              }
              if (!obj)
                *str = '\0'; /* arg given, not found */
            } else {         /* no arg given */
              if (c->carrying) {
                snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(c->carrying));
              } else {
                *str = '\0';
              }
            }
          }

          else if (!strcasecmp(field, "is_killer")) {
            if (subfield && *subfield) {
              if (!strcasecmp("on", subfield))
                char_plr_set(c, PLR_KILLER, true);
              else if (!strcasecmp("off", subfield))
                char_plr_set(c, PLR_KILLER, false);
            }
            if (char_plr_flagged(c, PLR_KILLER))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          }

          else if (!strcasecmp(field, "is_thief")) {
            if (subfield && *subfield) {
              if (!strcasecmp("on", subfield))
                char_plr_set(c, PLR_THIEF, true);
              else if (!strcasecmp("off", subfield))
                char_plr_set(c, PLR_THIEF, false);
            }
            if (char_plr_flagged(c, PLR_THIEF))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          }

          else if (!strcasecmp(field, "int")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max      = 100;
              char_set_aff_abils_intel(c, char_get_aff_abils_intel(c) + (addition));
              if (char_get_aff_abils_intel(c) > max) char_set_aff_abils_intel(c, max);
              if (char_get_aff_abils_intel(c) < 3) char_set_aff_abils_intel(c, 3);
            }
            snprintf(str, slen, "%d", char_get_aff_abils_intel(c));
          }
          break;
        case 'l':
          if (!strcasecmp(field, "level"))
            snprintf(str, slen, "%d", char_get_level(c));
          break;
        case 'm':
          if (!strcasecmp(field, "maxhitp")) {
            if (subfield && *subfield) {
              //GET_MAX_HIT(c) = MAX(GET_MAX_HIT(c) + addition, 1);
            }
            snprintf(str, slen, "%" I64T "", GET_MAX_HIT(c));
          }

          else if (!strcasecmp(field, "mana")) {
            if (subfield && *subfield) {
              int64_t addition = atoll(subfield);
              if (addition > 0) {
                incCurKI(c, addition, true);
              } else {
                decCurKI(c, addition, 0);
              }
            }
            snprintf(str, slen, "%" I64T "", (getCurKI(c)));
          } else if (!strcasecmp(field, "maxmana")) {
            if (subfield && *subfield) {
              //GET_MAX_MANA(c) = MAX(GET_MAX_MANA(c) + addition, 1);
            }
            snprintf(str, slen, "%" I64T "", GET_MAX_MANA(c));
          }

          else if (!strcasecmp(field, "move")) {
            if (subfield && *subfield) {
              int64_t addition = atoll(subfield);
              if (addition > 0) {
                incCurST(c, addition, true);
              } else {
                decCurST(c, addition, 0);
              }
            }
            snprintf(str, slen, "%" I64T "", (getCurST(c)));
          }

          else if (!strcasecmp(field, "maxmove")) {
            if (subfield && *subfield) {
              //GET_MAX_MOVE(c) = MAX(GET_MAX_MOVE(c) + addition, 1);
            }
            snprintf(str, slen, "%" I64T "", GET_MAX_MOVE(c));
          }

          else if (!strcasecmp(field, "master")) {
            if (!c->master)
              *str = '\0';
            else
              snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(c->master));
          }
          break;
        case 'n':
          if (!strcasecmp(field, "name")) {
            snprintf(str, slen, "%s", char_get_name(c));
          } else if (!strcasecmp(field, "next_in_room")) {
            if (c->next_in_room)
              snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(c->next_in_room));
            else
              *str = '\0';
          }
          break;
        case 'p':
          /* Thanks to Christian Ejlertsen for this idea
             And to Ken Ray for speeding the implementation up :)*/
          if (!strcasecmp(field, "pos")) {
            if (subfield && *subfield) {
              for (i = POS_SLEEPING; i <= POS_STANDING; i++) {
                /* allows : Sleeping, Resting, Sitting, Fighting, Standing */
                if (!strncasecmp(subfield, position_types[i], strlen(subfield))) {
                  char_set_position(c, i);
                  break;
                }
              }
            }
            snprintf(str, slen, "%s", position_types[char_get_position(c)]);
          } else if (!strcasecmp(field, "prac")) {
            if (char_is_npc(c)) {
              if (char_get_in_room(c) != NOWHERE) {
                send_to_room(char_get_in_room(c), "Error!: Report this trigger error to the coding authorities!\r\n");
              }
            }
            if (subfield && *subfield) {
              int addition                        = atoi(subfield);
              GET_PRACTICES(c, char_get_class(c)) = MAX(0, GET_PRACTICES(c, char_get_class(c)) + addition);
            }
            snprintf(str, slen, "%d", GET_PRACTICES(c, char_get_class(c)));
          } else if (!strcasecmp(field, "plr")) {
            if (subfield && *subfield) {
              int plr = get_flag_by_name(player_bits, subfield);
              if (plr != NOFLAG && char_plr_flagged(c, plr))
                strcpy(str, "1");
              else
                strcpy(str, "0");
            } else
              strcpy(str, "0");
          } else if (!strcasecmp(field, "pref")) {
            if (subfield && *subfield) {
              int pref = get_flag_by_name(preference_bits, subfield);
              if (pref != NOFLAG && char_prf_flagged(c, pref))
                strcpy(str, "1");
              else
                strcpy(str, "0");
            } else
              strcpy(str, "0");
          }
          break;
        case 'r':
          if (!strcasecmp(field, "room")) { /* in NOWHERE, return the void */
                                            /* see note in dg_scripts.h */
            snprintf(str, slen, "%c%d", UID_CHAR,
                     (char_get_in_room(c) != NOWHERE) ? world[char_get_in_room(c)].number + ROOM_ID_BASE : ROOM_ID_BASE);
          } else if (!strcasecmp(field, "race")) {
            snprintf(str, slen, "%s", TRUE_RACE(c));
          } else if (!strcasecmp(field, "rpp")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              char_set_rp(c, char_get_rp(c) + (addition));
            }

            snprintf(str, slen, "%d", char_get_rp(c));
          }

          break;
        case 's':
          if (!strcasecmp(field, "sex"))
            snprintf(str, slen, "%s", genders[(int)char_get_sex(c)]);

          else if (!strcasecmp(field, "str")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max      = 100;
              char_set_aff_abils_str(c, char_get_aff_abils_str(c) + (addition));
              if (char_get_aff_abils_str(c) > max) char_set_aff_abils_str(c, max);
              if (char_get_aff_abils_str(c) < 3) char_set_aff_abils_str(c, 3);
            }
            snprintf(str, slen, "%d", char_get_aff_abils_str(c));
          }

          else if (!strcasecmp(field, "size")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = search_block(subfield, size_names, FALSE)) > -1) {
                (c)->size = ns;
              }
            }
            sprinttype(get_size(c), size_names, str, slen);
          }

          else if (!strcasecmp(field, "skill"))
            snprintf(str, slen, "%s", skill_percent(c, subfield));

          else if (!strcasecmp(field, "skillset")) {
            if (!char_is_npc(c) && subfield && *subfield) {
              char skillname[MAX_INPUT_LENGTH], *amount;
              amount = one_word(subfield, skillname);
              skip_spaces(&amount);
              if (amount && *amount && is_number(amount)) {
                int skillnum = find_skill_num(skillname, SKTYPE_SKILL);
                if (skillnum > 0) {
                  int new_value = MAX(0, MIN(100, atoi(amount)));
                  SET_SKILL(c, skillnum, new_value);
                }
              }
            }
            *str = '\0'; /* so the parser know we recognize 'skillset' as a field */
          }

          break;
        case 't':
          if (!strcasecmp(field, "thirst")) {
            if (subfield && *subfield) {
              int addition        = atoi(subfield);
              GET_COND(c, THIRST) = MAX(-1, MIN(addition, 24));
            }
            snprintf(str, slen, "%d", GET_COND(c, THIRST));
          } else if (!strcasecmp(field, "tnl")) {
            snprintf(str, slen, "%d", level_exp(c, char_get_level(c) + 1));
          }
          break;
        case 'v':
          if (!strcasecmp(field, "vnum")) {
            if (subfield && *subfield) {
              snprintf(str, slen, "%d", char_is_npc(c) ? (int)(char_get_mob_vnum(c) == atoi(subfield)) : -1);
            } else {
              if (char_is_npc(c))
                snprintf(str, slen, "%d", char_get_mob_vnum(c));
              else
                /*
               * for compatibility with unsigned indexes
               * - this is deprecated - use %actor.is_pc% to check
               * instead of %actor.vnum% == -1  --Welcor 09/03
               */
                strcpy(str, "-1");
            }
          }

          else if (!strcasecmp(field, "varexists")) {
            struct trig_var_data *remote_vd;
            strcpy(str, "0");
            if (SCRIPT(c) && SCRIPT(c)->global_vars) {
              for (remote_vd = SCRIPT(c)->global_vars; remote_vd; remote_vd = remote_vd->next) {
                if (!strcasecmp(remote_vd->name, subfield)) break;
              }
              if (remote_vd) strcpy(str, "1");
            }
          }

          break;
        case 'w':
          if (!strcasecmp(field, "weight"))
            snprintf(str, slen, "%d", char_get_weight(c));
          else if (!strcasecmp(field, "wis")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max      = 100;
              char_set_aff_abils_wis(c, char_get_aff_abils_wis(c) + (addition));
              if (char_get_aff_abils_wis(c) > max) char_set_aff_abils_wis(c, max);
              if (char_get_aff_abils_wis(c) < 3) char_set_aff_abils_wis(c, 3);
            }
            snprintf(str, slen, "%d", char_get_aff_abils_wis(c));
          }
          break;
        case 'z':
          if (!strcasecmp(field, "zenni")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              char_set_gold(c, char_get_gold(c) + (addition));
            }
            snprintf(str, slen, "%d", char_get_gold(c));
          }
          break;
      } /* switch *field */

      if (*str == '\x1') { /* no match found in switch */
        if (SCRIPT(c)) {
          for (vd = (SCRIPT(c))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
        }
      }
    } /* if (c) ...*/

    else if (o) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      *str = '\x1';
      switch (tolower(*field)) {
        case 'a':
          if (!strcasecmp(field, "affects")) {
            if (subfield && *subfield) {
              if (check_flags_by_name_ar(obj_get_bitvector(o), NUM_AFF_FLAGS, subfield, affected_bits) > 0)
                snprintf(str, slen, "1");
              else
                snprintf(str, slen, "0");
            } else
              snprintf(str, slen, "0");
          }
          break;
        case 'c':
          if (!strcasecmp(field, "cost")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              obj_set_cost(o, MAX(0, addition + obj_get_cost(o)));
            }
            snprintf(str, slen, "%d", obj_get_cost(o));
          }

          else if (!strcasecmp(field, "cost_per_day")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              obj_set_cost_per_day(o, MAX(0, addition + obj_get_cost_per_day(o)));
            }
            snprintf(str, slen, "%d", obj_get_cost_per_day(o));
          }

          else if (!strcasecmp(field, "carried_by")) {
            if (o->carried_by)
              snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(o->carried_by));
            else
              *str = '\0';
          }

          else if (!strcasecmp(field, "contents")) {
            if (o->contains)
              snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(o->contains));
            else
              *str = '\0';
          }
          /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
          else if (!strcasecmp(field, "count")) {
            if (obj_get_type_flag(o) == ITEM_CONTAINER)
              snprintf(str, slen, "%d", item_in_list(subfield, o->contains));
            else
              strcpy(str, "0");
          }
          break;
        case 'e':
          if (!strcasecmp(field, "extra")) {
            if (subfield && *subfield) {
              if (check_flags_by_name_ar(obj_get_extra_flags(o), NUM_ITEM_FLAGS, subfield, extra_bits) > 0)
                snprintf(str, slen, "1");
              else
                snprintf(str, slen, "0");
            } else
              snprintf(str, slen, "0");
          } else {
            sprintbitarray(obj_get_extra_flags(o), extra_bits, EF_ARRAY_MAX, str, slen);
          }
          break;
        case 'h':
          /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
          if (!strcasecmp(field, "has_in")) {
            if (obj_get_type_flag(o) == ITEM_CONTAINER)
              snprintf(str, slen, "%s", (item_in_list(subfield, o->contains) ? "1" : "0"));
            else
              strcpy(str, "0");
          }
          if (!strcasecmp(field, "health")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              obj_set_value(o, VAL_ALL_HEALTH, MAX(1, addition + obj_get_value(o, VAL_ALL_HEALTH)));
              if (obj_extra_flagged(o, ITEM_BROKEN) && obj_get_value(o, VAL_ALL_HEALTH) >= 100)
                obj_extra_set(o, ITEM_BROKEN, false);
            }
            snprintf(str, slen, "%d", obj_get_value(o, VAL_ALL_HEALTH));
          }
          break;
        case 'i':
          if (!strcasecmp(field, "id"))
            snprintf(str, slen, "%d", obj_get_id(o));

          else if (!strcasecmp(field, "is_inroom")) {
            if (obj_get_in_room(o) != NOWHERE)
              snprintf(str, slen, "%c%d", UID_CHAR, world[obj_get_in_room(o)].number + ROOM_ID_BASE);
            else
              *str = '\0';
          } else if (!strcasecmp(field, "is_pc")) {
            strcpy(str, "-1");
          } else if (!strcasecmp(field, "itemflag")) {
            if (subfield && *subfield) {
              int item = get_flag_by_name(extra_bits, subfield);
              if (item != NOFLAG && obj_extra_flagged(o, item))
                strcpy(str, "1");
              else
                strcpy(str, "0");
            } else
              strcpy(str, "0");
          }
          break;
        case 'l':
          if (!strcasecmp(field, "level"))
            snprintf(str, slen, "%d", obj_get_level(o));
          break;

        case 'n':
          if (!strcasecmp(field, "name")) {
            if (!subfield || !*subfield)
              snprintf(str, slen, "%s", o->name);
            else {
              char blah[500];
              sprintf(blah, "%s %s", o->name, subfield);
              o->name = strdup(blah);
            }
          } else if (!strcasecmp(field, "next_in_list")) {
            if (o->next_content)
              snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(o->next_content));
            else
              *str = '\0';
          }
          break;
        case 'r':
          if (!strcasecmp(field, "room")) {
            if (obj_room(o) != NOWHERE)
              snprintf(str, slen, "%c%d", UID_CHAR, world[obj_room(o)].number + ROOM_ID_BASE);
            else
              *str = '\0';
          }
          break;
        case 's':
          if (!strcasecmp(field, "shortdesc")) {
            if (!subfield || !*subfield)
              snprintf(str, slen, "%s", o->short_description);
            else {
              char blah[500];
              sprintf(blah, "%s @wnicknamed @D(@C%s@D)@n", o->short_description, subfield);
              o->short_description = strdup(blah);
            }
          } else if (!strcasecmp(field, "setaffects")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = check_flags_by_name_ar(obj_get_bitvector(o), NUM_AFF_FLAGS, subfield, affected_bits)) > 0) {
                obj_aff_toggle(o, ns);
                snprintf(str, slen, "1");
              }
            }
          } else if (!strcasecmp(field, "setextra")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = check_flags_by_name_ar(obj_get_extra_flags(o), NUM_ITEM_FLAGS, subfield, extra_bits)) > 0) {
                obj_extra_toggle(o, ns);
                snprintf(str, slen, "1");
              }
            }
          }

          else if (!strcasecmp(field, "size")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = search_block(subfield, size_names, FALSE)) > -1) {
                (o)->size = ns;
              }
            }
            sprinttype(obj_get_size(o), size_names, str, slen);
          }
          break;
        case 't':
          if (!strcasecmp(field, "type"))
            sprinttype(obj_get_type_flag(o), item_types, str, slen);

          else if (!strcasecmp(field, "timer"))
            snprintf(str, slen, "%d", obj_get_timer(o));
          break;
        case 'v':
          if (!strcasecmp(field, "vnum"))
            if (subfield && *subfield) {
              snprintf(str, slen, "%d", (int)(obj_get_vnum(o) == atoi(subfield)));
            } else {
              snprintf(str, slen, "%d", obj_get_vnum(o));
            }
          else if (!strcasecmp(field, "val0"))
            snprintf(str, slen, "%d", obj_get_value(o, 0));

          else if (!strcasecmp(field, "val1"))
            snprintf(str, slen, "%d", obj_get_value(o, 1));

          else if (!strcasecmp(field, "val2"))
            snprintf(str, slen, "%d", obj_get_value(o, 2));

          else if (!strcasecmp(field, "val3"))
            snprintf(str, slen, "%d", obj_get_value(o, 3));

          else if (!strcasecmp(field, "val4"))
            snprintf(str, slen, "%d", obj_get_value(o, 4));

          else if (!strcasecmp(field, "val5"))
            snprintf(str, slen, "%d", obj_get_value(o, 5));

          else if (!strcasecmp(field, "val6"))
            snprintf(str, slen, "%d", obj_get_value(o, 6));

          else if (!strcasecmp(field, "val7"))
            snprintf(str, slen, "%d", obj_get_value(o, 7));
          break;
        case 'w':
          if (!strcasecmp(field, "weight")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              if (addition < 0 || addition > 0) {
                obj_set_weight(o, MAX(0, addition + obj_get_weight(o)));
              } else {
                obj_set_weight(o, 0);
              }
            }
            snprintf(str, slen, "%" I64T "", obj_get_weight(o));
          }

          else if (!strcasecmp(field, "worn_by")) {
            if (o->worn_by)
              snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(o->worn_by));
            else
              *str = '\0';
          }
          break;
      } /* switch *field */

      if (*str == '\x1') { /* no match in switch */
        if (SCRIPT(o)) {   /* check for global var */
          for (vd = (SCRIPT(o))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            if (strcasecmp(GET_TRIG_NAME(trig), "Rename Object")) {
              script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                         GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
            }
          }
        } else {
          *str = '\0';
          if (strcasecmp(GET_TRIG_NAME(trig), "Rename Object")) {
            script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
          }
        }
      }
    } /* if (o) ... */

    else if (r) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      /* special handling of the void, as it stores all 'full global' variables */
      if (r->number == 0) {
        if (!SCRIPT(r)) {
          *str = '\0';
          script_log("Trigger: %s, Vnum %d, type %d. Trying to access Global var list of void. Apparently this has not been set up!",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type);
        } else {
          for (vd = (SCRIPT(r))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else
            *str = '\0';
        }
      }

      else if (!strcasecmp(field, "name"))
        snprintf(str, slen, "%s", r->name);

      else if (!strcasecmp(field, "sector"))
        sprinttype(r->sector_type, sector_types, str, slen);

      else if (!strcasecmp(field, "gravity"))
        snprintf(str, slen, "%d", r->gravity);

      else if (!strcasecmp(field, "vnum")) {
        if (subfield && *subfield) {
          snprintf(str, slen, "%d", (int)(r->number == atoi(subfield)));
        } else {
          snprintf(str, slen, "%d", r->number);
        }
      } else if (!strcasecmp(field, "contents")) {
        if (subfield && *subfield) {
          for (obj = r->contents; obj; obj = obj->next_content) {
            if (obj_get_vnum(obj) == atoi(subfield)) {
              /* arg given, found */
              snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(obj));
              return;
            }
          }
          if (!obj)
            *str = '\0'; /* arg given, not found */
        } else {         /* no arg given */
          if (r->contents) {
            snprintf(str, slen, "%c%d", UID_CHAR, obj_get_id(r->contents));
          } else {
            *str = '\0';
          }
        }
      }

      else if (!strcasecmp(field, "people")) {
        if (r->people)
          snprintf(str, slen, "%c%d", UID_CHAR, char_get_id(r->people));
        else
          *str = '\0';
      } else if (!strcasecmp(field, "id")) {
        room_rnum rnum = real_room(r->number);
        if (rnum != NOWHERE)
          snprintf(str, slen, "%d", world[rnum].number + ROOM_ID_BASE);
        else
          *str = '\0';
      } else if (!strcasecmp(field, "weather")) {
        const char *sky_look[] = {
            "sunny",
            "cloudy",
            "rainy",
            "lightning"};

        if (!room_flagged(r, ROOM_INDOORS))
          snprintf(str, slen, "%s", sky_look[weather_info.sky]);
        else
          *str = '\0';
      } else if (!strcasecmp(field, "fishing")) {
        room_rnum thisroom = real_room(r->number);
        if (rroom_flagged(thisroom, ROOM_FISHING))
          snprintf(str, slen, "1");
        else
          snprintf(str, slen, "0");
      } else if (!strcasecmp(field, "zonenumber"))
        snprintf(str, slen, "%d", zone_table[r->zone].number);
      else if (!strcasecmp(field, "zonename"))
        snprintf(str, slen, "%s", zone_table[r->zone].name);
      else if (!strcasecmp(field, "roomflag")) {
        if (subfield && *subfield) {
          room_rnum thisroom = real_room(r->number);
          if (check_flags_by_name_ar(rroom_get_room_flags(thisroom), NUM_ROOM_FLAGS, subfield, room_bits) > 0)
            snprintf(str, slen, "1");
          else
            snprintf(str, slen, "0");
        } else
          snprintf(str, slen, "0");
      } else if (!strcasecmp(field, "north")) {
        if (room_get_dir_option(r, NORTH)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, NORTH)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, NORTH)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, NORTH)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, NORTH)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, NORTH)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, NORTH)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "east")) {
        if (room_get_dir_option(r, EAST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, EAST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, EAST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, EAST)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, EAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, EAST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, EAST)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "south")) {
        if (room_get_dir_option(r, SOUTH)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, SOUTH)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, SOUTH)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, SOUTH)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, SOUTH)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, SOUTH)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, SOUTH)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "west")) {
        if (room_get_dir_option(r, WEST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, WEST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, WEST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, WEST)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, WEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, WEST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, WEST)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "up")) {
        if (room_get_dir_option(r, UP)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, UP)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, UP)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, UP)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, UP)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, UP)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, UP)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "down")) {
        if (room_get_dir_option(r, DOWN)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, DOWN)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, DOWN)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, DOWN)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, DOWN)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, DOWN)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, DOWN)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "northwest")) {
        if (room_get_dir_option(r, NORTHWEST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, NORTHWEST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, NORTHWEST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, NORTHWEST)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, NORTHWEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, NORTHWEST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, NORTHWEST)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "northeast")) {
        if (room_get_dir_option(r, NORTHEAST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, NORTHEAST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, NORTHEAST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, NORTHEAST)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, NORTHEAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, NORTHEAST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, NORTHEAST)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "southwest")) {
        if (room_get_dir_option(r, SOUTHWEST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, SOUTHWEST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, SOUTHWEST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, SOUTHWEST)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, SOUTHWEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, SOUTHWEST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, SOUTHWEST)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "southeast")) {
        if (room_get_dir_option(r, SOUTHEAST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, SOUTHEAST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, SOUTHEAST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, SOUTHEAST)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, SOUTHEAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, SOUTHEAST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, SOUTHEAST)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "inside")) {
        if (room_get_dir_option(r, INDIR)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, INDIR)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, INDIR)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, INDIR)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, INDIR)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, INDIR)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, INDIR)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else if (!strcasecmp(field, "outside")) {
        if (room_get_dir_option(r, OUTDIR)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", rroom_get_vnum(room_get_dir_option(r, OUTDIR)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", room_get_dir_option(r, OUTDIR)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(room_get_dir_option(r, OUTDIR)->exit_info, exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (room_get_dir_option(r, OUTDIR)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[room_get_dir_option(r, OUTDIR)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(room_get_dir_option(r, OUTDIR)->exit_info, exit_bits, str, slen);
        } else
          *str = '\0';
      } else {
        if (SCRIPT(r)) { /* check for global var */
          for (vd = (SCRIPT(r))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
        }
      }
    } /* if (r).. */
  }
}

/*
 * Now automatically checks if the variable has more then one field
 * in it. And if the field returns a name or a script UID or the like
 * it can recurse.
 * If you supply a value like, %actor.int.str% it wont blow up on you
 * either.
 * - Jamie Nelson 31st Oct 2003 01:03
 *
 * Now also lets subfields have variables parsed inside of them
 * so that:
 * %echo% %actor.gold(%actor.gold%)%
 * will double the actors gold every time its called.  etc...
 * - Jamie Nelson 31st Oct 2003 01:24
 */

/* substitutes any variables into line and returns it as buf */
void var_subst(void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf)
{
  char tmp[MAX_INPUT_LENGTH], repl_str[MAX_INPUT_LENGTH];
  char *var = NULL, *field = NULL, *p = NULL;
  char tmp2[MAX_INPUT_LENGTH];
  char *subfield_p, subfield[MAX_INPUT_LENGTH];
  int left, len;
  int paren_count = 0;
  int dots        = 0;

  /* skip out if no %'s */
  if (!strchr(line, '%')) {
    strcpy(buf, line);
    return;
  }
  /*lets just empty these to start with*/
  *repl_str = *tmp = *tmp2 = '\0';

  p          = strcpy(tmp, line);
  subfield_p = subfield;

  left = MAX_INPUT_LENGTH - 1;

  while (*p && (left > 0)) {

    /* copy until we find the first % */
    while (*p && (*p != '%') && (left > 0)) {
      *(buf++) = *(p++);
      left--;
    }

    *buf = '\0';

    /* double % */
    if (*p && (*(++p) == '%') && (left > 0)) {
      *(buf++) = *(p++);
      *buf     = '\0';
      left--;
      continue;
    }

    /* so it wasn't double %'s */
    else if (*p && (left > 0)) {

      /* search until end of var or beginning of field */
      for (var = p; *p && (*p != '%') && (*p != '.'); p++);

      field = p;
      if (*p == '.') {
        *(p++) = '\0';
        dots   = 0;
        for (field = p; *p && ((*p != '%') || (paren_count > 0) || (dots)); p++) {
          if (dots > 0) {
            *subfield_p = '\0';
            find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));
            if (*repl_str) {
              snprintf(tmp2, sizeof(tmp2), "eval tmpvr %s", repl_str); //temp var
              process_eval(go, sc, trig, type, tmp2);
              strcpy(var, "tmpvr");
              field = p;
              dots  = 0;
              continue;
            }
            dots = 0;
          } else if (*p == '(') {
            *p = '\0';
            paren_count++;
          } else if (*p == ')') {
            *p = '\0';
            paren_count--;
          } else if (paren_count > 0) {
            *subfield_p++ = *p;
          } else if (*p == '.') {
            *p = '\0';
            dots++;
          }
        } /* for (field.. */
      } /* if *p == '.' */

      *(p++)      = '\0';
      *subfield_p = '\0';

      if (*subfield) {
        var_subst(go, sc, trig, type, subfield, tmp2);
        strcpy(subfield, tmp2);
      }

      find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));

      strncat(buf, repl_str, left);
      len = strlen(repl_str);
      buf += len;
      left -= len;
    } /* else if *p .. */
  } /* while *p .. */
}
