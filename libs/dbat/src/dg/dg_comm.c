/* ************************************************************************
*  File: dg_comm.c                               Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle mud to player communication         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */
#include "dbat/consts/maximums.h"
#include "dbat/utils.h"
#include "dbat/act.informative.h"
#include "dbat/races.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/graph.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/stringutils.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"

/* local functions */
static void sub_write_to_char(struct char_data *ch, char *tokens[], void *otokens[], char type[]);

/* same as any_one_arg except that it stops at punctuation */
static char *any_one_name(char *argument, char *first_arg)
{
  char *arg;

  /* Find first non blank */
  while (isspace(*argument))
    argument++;

  /* Find length of first word */
  for (arg = first_arg;
       *argument && !isspace(*argument) &&
       (!ispunct(*argument) || *argument == '#' || *argument == '-');
       arg++, argument++)
    *arg = tolower(*argument);
  *arg = '\0';

  return argument;
}

static void sub_write_to_char(struct char_data *ch, char *tokens[],
                              void *otokens[], char type[])
{
  char sb[MAX_STRING_LENGTH];
  int i;

  strcpy(sb, "");

  for (i = 0; tokens[i + 1]; i++) {
    strcat(sb, tokens[i]);

    switch (type[i]) {
      case '~':
        if (!otokens[i])
          strcat(sb, "someone");
        else if ((struct char_data *)otokens[i] == ch)
          strcat(sb, "you");
        else
          strcat(sb, char_pers(((struct char_data *)otokens[i]), ch));
        break;

      case '|':
        if (!otokens[i])
          strcat(sb, "someone's");
        else if ((struct char_data *)otokens[i] == ch)
          strcat(sb, "your");
        else {
          strcat(sb, char_pers(((struct char_data *)otokens[i]), ch));
          strcat(sb, "'s");
        }
        break;

      case '^':
        if (!otokens[i] || !char_can_see(ch, (struct char_data *)otokens[i]))
          strcat(sb, "its");
        else if (otokens[i] == ch)
          strcat(sb, "your");
        else
          strcat(sb, char_hshr((struct char_data *)otokens[i]));
        break;

      case '&':
        if (!otokens[i] || !char_can_see(ch, (struct char_data *)otokens[i]))
          strcat(sb, "it");
        else if (otokens[i] == ch)
          strcat(sb, "you");
        else
          strcat(sb, char_hssh((struct char_data *)otokens[i]));
        break;

      case '*':
        if (!otokens[i] || !char_can_see(ch, (struct char_data *)otokens[i]))
          strcat(sb, "it");
        else if (otokens[i] == ch)
          strcat(sb, "you");
        else
          strcat(sb, char_hmhr((struct char_data *)otokens[i]));
        break;

      case '_':
        if (!otokens[i])
          strcat(sb, "something");
        else
          strcat(sb, obj_shortdesc_for(((struct obj_data *)otokens[i]), ch));
        break;
    }
  }

  strcat(sb, tokens[i]);
  strcat(sb, "\n\r");
  sb[0] = toupper(sb[0]);
  send_to_char(ch, "%s", sb);
}

void sub_write(char *arg, struct char_data *ch, int8_t find_invis, int targets)
{
  char str[MAX_INPUT_LENGTH * 2];
  char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char *tokens[MAX_INPUT_LENGTH], *s, *p;
  void *otokens[MAX_INPUT_LENGTH];
  struct char_data *to;
  struct obj_data *obj;
  int i, tmp;
  int to_sleeping = 1; /* mainly for windows compiles */

  if (!arg)
    return;

  tokens[0] = str;

  for (i = 0, p = arg, s = str; *p;) {
    switch (*p) {
      case '~':
      case '|':
      case '^':
      case '&':
      case '*':
        /* get char_data, move to next token */
        type[i] = *p;
        *s      = '\0';
        p       = any_one_name(++p, name);
        otokens[i] =
            find_invis ? (void *)get_char_in_room(&world[char_get_in_room(ch)], name) : (void *)get_char_room_vis(ch, name, NULL);
        tokens[++i] = ++s;
        break;

      case '_':
        /* get obj_data, move to next token */
        type[i] = *p;
        *s      = '\0';
        p       = any_one_name(++p, name);

        if (find_invis) obj = get_obj_in_room(&world[char_get_in_room(ch)], name);
        else if (!(obj = get_obj_in_list_vis(ch, name, NULL, world[char_get_in_room(ch)].contents)))
          ;
        else if (!(obj = get_obj_in_equip_vis(ch, name, &tmp, ch->equipment)))
          ;
        else obj = get_obj_in_list_vis(ch, name, NULL, ch->carrying);

        otokens[i]  = (void *)obj;
        tokens[++i] = ++s;
        break;

      case '\\':
        p++;
        *s++ = *p++;
        break;

      default:
        *s++ = *p++;
    }
  }

  *s          = '\0';
  tokens[++i] = NULL;

  if (IS_SET(targets, TO_CHAR) && sendok(ch, to_sleeping))
    sub_write_to_char(ch, tokens, otokens, type);

  if (IS_SET(targets, TO_ROOM))
    for (to     = world[char_get_in_room(ch)].people;
         to; to = to->next_in_room)
      if (to != ch && sendok(to, to_sleeping))
        sub_write_to_char(to, tokens, otokens, type);
}

void send_to_zone(const char *messg, zone_rnum zone)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && char_is_awake(i->character) &&
        (char_get_in_room(i->character) != NOWHERE) &&
        (world[char_get_in_room(i->character)].zone == zone))
      write_to_output(i, "%s", messg);
}
