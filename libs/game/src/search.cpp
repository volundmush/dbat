#include "dbat/game/search.h"

int is_name(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  if (!*str || !*namelist || !str || !namelist)
    return (0);

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
        return (1);

      if (!*curname)
        return (0);

      if (!*curstr || *curname == ' ')
        break;

      if (LOWER(*curstr) != LOWER(*curname))
        break;
    }

    /* skip to next name */
   for (; isalpha(*curname); curname++);
     if (!*curname)
       return (0);
    curname++;                  /* first char of new name */
  }
}

/* allow abbreviations */
#define WHITESPACE " \t"
int isname(const char *str, const char *namelist)
{
  char *newlist;
  register char *curtok;
  static char newlistbuf[MAX_STRING_LENGTH];

  if (!str || !*str || !namelist || !*namelist) {
     return 0;
  }

  if (!strcasecmp(str, namelist)) { /* the easy way */
     return 1;
  }

  strlcpy(newlistbuf, namelist, sizeof(newlistbuf));
  newlist = newlistbuf;
  for (curtok = strsep(&newlist, WHITESPACE); curtok; curtok = strsep(&newlist, WHITESPACE)) {
    if (curtok && is_abbrev(str, curtok)) {
      /* Don't allow abbreviated numbers, only alpha names need abbreviation */
      /* This, I just consider a bug fix, because abbreviating numbers is just*/
      /* asking for trouble. IE: 100 would return true on 1000 --Sryth*/
      if (isdigit(*str) && (atoi(str) != atoi(curtok))) {
        return 0;
      }
    return 1;
    }
  }
  return 0;
}


int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL) {
    *ppos++ = '\0';
    strlcpy(number, *name, sizeof(number));
    strcpy(*name, ppos);	/* strcpy: OK (always smaller) */

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
	return (0);

    return (atoi(number));
  }
  return (1);
}


/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}



/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}



/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int *number, room_rnum room)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = world[room].people; i && *number; i = i->next_in_room)
    if (isname(name, i->name))
      if (--(*number) == 0)
	return (i);

  return (NULL);
}



/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}



struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
      continue;
    if (GET_ADMLEVEL(ch) < 1 && GET_ADMLEVEL(i) < 1 && !IS_NPC(ch) && !IS_NPC(i)) {
     if (strcasecmp(RACE(i), name) && !strstr(RACE(i), name)) {
      if (readIntro(ch, i) == 1) {
       if (strcasecmp(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name)) {
        continue;
       }
      }
      else {
       continue;
      }
     }
    }
    if ((GET_ADMLEVEL(ch) >= 1 || GET_ADMLEVEL(i) >= 1 || IS_NPC(ch) || IS_NPC(i))) {
     if (strcasecmp(i->name, name) && !strstr(i->name, name)) {
      if (strcasecmp(RACE(i), name) && !strstr(RACE(i), name)) {
        if (!IS_NPC(ch) && !IS_NPC(i) && readIntro(ch, i) == 1) {
         if (strcasecmp(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name)) {
          continue;
         }
        }
       else {
        continue;
       }
      }
     }
    }
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (NULL);
}


struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* JE 7/18/94 :-) :-) */
  if (!strcasecmp(name, "self") || !strcasecmp(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room) {
    if (!strcasecmp(name, "last") && LASTHIT(i) != 0 && LASTHIT(i) == GET_IDNUM(ch)) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (isname(name, i->name) && (IS_NPC(i) || IS_NPC(ch) || GET_ADMLEVEL(i) > 0 || GET_ADMLEVEL(ch) > 0) && i != ch) {
      if (CAN_SEE(ch, i))
	if (--(*number) == 0)
	  return (i);
    }
    else if (isname(name, i->name) && i == ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (!IS_NPC(i) && !IS_NPC(ch) && !strcasecmp(get_i_name(ch, i), CAP(name)) && i != ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (!IS_NPC(i) && !IS_NPC(ch) && strstr(get_i_name(ch, i), CAP(name)) && i != ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (!IS_NPC(i) && !(strcmp(RACE(i), CAP(name))) && i != ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (!IS_NPC(i) && strstr(RACE(i), CAP(name)) && i != ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (!IS_NPC(i) && !(strcmp(RACE(i), name)) && i != ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
    else if (!IS_NPC(i) && strstr(RACE(i), name) && i != ch) {
      if (CAN_SEE(ch, i))
        if (--(*number) == 0)
          return (i);
    }
  }
  return (NULL);
}


struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if ((i = get_char_room_vis(ch, name, number)) != NULL)
    return (i);

  if (*number == 0)
    return get_player_vis(ch, name, NULL, 0);

  for (i = character_list; i && *number; i = i->next) {
    if (IN_ROOM(ch) == IN_ROOM(i))
      continue;
    if (GET_ADMLEVEL(ch) < 1 && GET_ADMLEVEL(i) < 1 && !IS_NPC(ch) && !IS_NPC(i)) {
     if (strcasecmp(RACE(i), name) && !strstr(RACE(i), name)) {
      if (readIntro(ch, i) == 1) {
       if (strcasecmp(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name)) {
        continue;
       }
      }
      else {
       continue;
      }
     }
    }
    if ((GET_ADMLEVEL(ch) >= 1 || GET_ADMLEVEL(i) >= 1 || IS_NPC(ch) || IS_NPC(i))) {
     if (strcasecmp(i->name, name) && !strstr(i->name, name)) {
      if (strcasecmp(RACE(i), name) && !strstr(RACE(i), name)) {
        if (!IS_NPC(ch) && !IS_NPC(i) && readIntro(ch, i) == 1) {
         if (strcasecmp(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name)) {
          continue;
         }
        }
       else {
        continue;
       }
      }
     }
    }
    /*if (!CAN_SEE(ch, i))
      continue;*/
    if (--(*number) != 0)
      continue;

    return (i);
  }
  return (NULL);
}


struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)
{
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_vis(ch, name, number);
  else
    return (NULL);
}


struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = list; i && *number; i = i->next_content)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i) || (GET_OBJ_TYPE(i) == ITEM_LIGHT))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}


/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *number)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, number, ch->carrying)) != NULL)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, number, world[IN_ROOM(ch)].contents)) != NULL)
    return (i);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && *number; i = i->next)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}


struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (NULL);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (equipment[j]);

  return (NULL);
}


int get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (-1);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (j);

  return (-1);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return (FIND_ALL);
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(arg, arg + 4);	/* strcpy: OK (always less) */
    return (FIND_ALLDOT);
  } else
    return (FIND_INDIV);
}

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */

int generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		     struct char_data **tar_ch, struct obj_data **tar_obj)
{
  int i, found, number;
  char name_val[MAX_INPUT_LENGTH];
  char *name = name_val;

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);
  if (!(number = get_number(&name)))
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_ROOM);
  }

  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_world_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_WORLD);
  }

  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && --number == 0) {
	*tar_obj = GET_EQ(ch, i);
	found = TRUE;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }

  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }

  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }

  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name, &number)))
      return (FIND_OBJ_WORLD);
  }

  return (0);
}