/*************************************************************************
*   File: messages.cpp                                  Part of CircleMUD *
*  Usage: act() and the send_to_*() broadcasts                           *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <algorithm>
#include <cstring>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/act.informative.hpp"

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

int arena_watch(GameWorld *g, struct char_data *ch)
{

  int found      = false;
  room_vnum room = NOWHERE;

  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) != CON_PLAYING)
      continue;

    if (char_in_arena(g, d->character)) {
      if (char_get_arenawatch(ch) == char_get_idnum(d->character)) {
        found = true;
        room  = char_get_in_room(d->character);
      }
    }
  }

  if (found == false) {
    char_prf_set(g, ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    return (NOWHERE);
  }
  return (room);
}

void send_to_all(GameWorld *g, std::string_view text)
{

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING)
      continue;
    write_to_output(g, i, text);
  }
}

void send_to_outdoor(GameWorld *g, std::string_view text)
{

  if (text.empty())
    return;

  for (descriptor_data *i : each_desc(g)) {

    if (desc_get_connected(i) != CON_PLAYING || i->character == nullptr)
      continue;
    if (!char_is_awake(i->character) || !char_is_outside(g, i->character))
      continue;
    write_to_output(g, i, text);
  }
}

void send_to_moon(GameWorld *g, std::string_view text)
{

  if (text.empty())
    return;

  for (descriptor_data *i : each_desc(g)) {

    if (desc_get_connected(i) != CON_PLAYING || i->character == nullptr)
      continue;
    if (!char_is_awake(i->character) || !char_has_moon(g, i->character))
      continue;
    write_to_output(g, i, text);
  }
}

void send_to_planet(GameWorld *g, int type, int planet, std::string_view text)
{

  if (text.empty())
    return;

  for (descriptor_data *i : each_desc(g)) {

    if (desc_get_connected(i) != CON_PLAYING || i->character == nullptr)
      continue;
    if (!char_is_awake(i->character) || !room_flagged(g, char_get_in_room(i->character), planet))
      continue;
    else {
      if (type == 0) {
        write_to_output(g, i, text);
      } else if (char_is_outside(g, i->character) && char_get_skill(i->character, SKILL_SPOT) >= axion_dice(g, -5)) {
        write_to_output(g, i, text);
      }
    }
  }
}

void send_to_room(GameWorld *g, room_vnum room, std::string_view text)
{
  if (room == NOWHERE)
    return;

  for (char_data *i : room_people(g, room)) {
    if (!char_get_desc(i))
      continue;
    write_to_output(g, char_get_desc(i), text);
  }

  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) != CON_PLAYING)
      continue;

    if (char_prf_flagged(g, d->character, PRF_ARENAWATCH)) {
      if (arena_watch(g, d->character) == room) {
        write_to_output(g, d, "@c-----@CArena@c-----@n\r\n{}\r\n@c-----@CArena@c-----@n\r\n", text);
      }
    }
    if (char_get_listenroom(d->character) > 0) {
      int roll = rand_number(g, 1, 101);
      if (char_get_listenroom(d->character) == room && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
        write_to_output(g, d, "-----Eavesdrop-----\r\n{}\r\n-----Eavesdrop-----\r\n", text);
      }
    }
  }
}

static const char *const ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression)  \
  i = (pointer) == nullptr ? nullptr : (expression); \
  if (i == nullptr) i = ACTNULL;

/* higher-level communication: the act() function */
static void perform_act(GameWorld *g, const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to)
{
  const char *i = nullptr;
  char lbuf[MAX_STRING_LENGTH], *buf, *j;
  char *const end                   = lbuf + sizeof(lbuf) - 3; /* the last char of the message; then "\r\n" and its end */
  bool uppercasenext                = false;
  const struct char_data *dg_victim = nullptr;
  const struct obj_data *dg_target  = nullptr;
  const char *dg_arg                = nullptr;

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
        case 'n':
          i = char_pers(g, ch, to);
          break;
        case 'N':
          CHECK_NULL(vict_obj, char_pers(g, (struct char_data *)const_cast<void *>(vict_obj), to));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 'm':
          i = char_hmhr(ch);
          break;
        case 'M':
          CHECK_NULL(vict_obj, char_hmhr((const struct char_data *)vict_obj));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 's':
          i = char_hshr(ch);
          break;
        case 'S':
          CHECK_NULL(vict_obj, char_hshr((const struct char_data *)vict_obj));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 'e':
          i = char_hssh(ch);
          break;
        case 'E':
          CHECK_NULL(vict_obj, char_hssh((const struct char_data *)vict_obj));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 'o':
          CHECK_NULL(obj, obj_fname_for(g, obj, to));
          break;
        case 'O':
          CHECK_NULL(vict_obj, obj_fname_for(g, (const struct obj_data *)vict_obj, to));
          dg_target = (const struct obj_data *)vict_obj;
          break;
        case 'p':
          CHECK_NULL(obj, obj_shortdesc_for(g, obj, to));
          break;
        case 'P':
          CHECK_NULL(vict_obj, obj_shortdesc_for(g, (const struct obj_data *)vict_obj, to));
          dg_target = (const struct obj_data *)vict_obj;
          break;
        case 'a':
          CHECK_NULL(obj, obj_sana(obj));
          break;
        case 'A':
          CHECK_NULL(vict_obj, obj_sana((const struct obj_data *)vict_obj));
          dg_target = (const struct obj_data *)vict_obj;
          break;
        case 'T':
          CHECK_NULL(vict_obj, (const char *)vict_obj);
          dg_arg = (const char *)vict_obj;
          break;
        case 't':
          CHECK_NULL(obj, (char *)obj);
          break;
        case 'F':
          CHECK_NULL(vict_obj, fname((const char *)vict_obj));
          break;
        /* uppercase previous word */
        case 'u':
          for (j = buf; j > lbuf && !isspace((int)*(j - 1)); j--);
          if (j != buf)
            *j = toupper(*j);
          i = "";
          break;
        /* uppercase next word */
        case 'U':
          uppercasenext = true;
          i             = "";
          break;
        case '$':
          i = "$";
          break;
        default:
          return;
          break;
      }
      for (; *i && buf < end; i++) { /* what doesn't fit is left off */
        *buf = *i;
        if (uppercasenext && !isspace((int)*buf)) {
          *buf          = toupper(*buf);
          uppercasenext = false;
        }
        buf++;
      }
      orig++;
    } else if (!*orig) {
      *(buf++) = '\0';
      break;
    } else if (buf >= end) { /* what doesn't fit is left off */
      orig++;
    } else {
      *(buf++) = *(orig++);
      if (uppercasenext && !isspace((int)*(buf - 1))) {
        *(buf - 1)    = toupper(*(buf - 1));
        uppercasenext = false;
      }
    }
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (char_get_desc(to))
    write_to_output(g, char_get_desc(to), "{}", CAP(lbuf));

  if ((char_is_npc(to) && g->dg_act_check) && (to != ch))
    act_mtrigger(g, to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);

  if (g->last_act_message)
    free(g->last_act_message);
  g->last_act_message = strdup(lbuf);
}

/* act()/sub_write(): should ch receive this message? (to_sleeping: deliver even to sleeping characters) */
bool sendok(struct char_data *ch, int to_sleeping)
{
  return (char_get_desc(ch) || script_has_type(char_get_script(ch), MTRIG_ACT)) && (to_sleeping || char_is_awake(ch)) && !char_plr_flagged(ch, PLR_WRITING);
}

void act_triple(GameWorld *g, const act_msgs &m, struct char_data *ch, struct obj_data *obj, struct char_data *vict)
{
  act(g, m.to_char, m.hide, ch, obj, vict, TO_CHAR);
  act(g, m.to_vict, m.hide, ch, obj, vict, TO_VICT);
  act(g, m.to_room, m.hide, ch, obj, vict, TO_NOTVICT);
}

void act_triple_vict_last(GameWorld *g, const act_msgs &m, struct char_data *ch, struct obj_data *obj, struct char_data *vict)
{
  act(g, m.to_char, m.hide, ch, obj, vict, TO_CHAR);
  act(g, m.to_room, m.hide, ch, obj, vict, TO_NOTVICT);
  act(g, m.to_vict, m.hide, ch, obj, vict, TO_VICT);
}

char *act(GameWorld *g, const char *str, int hide_invisible, struct char_data *ch,
          struct obj_data *obj, const void *vict_obj, int type)
{
  int to_sleeping, res_sneak, res_hide, dcval = 0, resskill = 0;

  if (!str || !*str)
    return nullptr;

  /* Warning: the following TO_SLEEP code is a hack. I wanted to be able to tell
   * act to deliver a message regardless of sleep without adding an additional 
   * argument.  TO_SLEEP is 128 (a single bit high up).  It's ONLY legal to 
   * combine TO_SLEEP with one other TO_x command.  It's not legal to combine 
   * TO_x's with each other otherwise. TO_SLEEP only works because its value 
   * "happens to be" a single bit; do not change it to something else.  In 
   * short, it is a hack.  The same applies to TO_*RESIST.  */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;

  if ((res_sneak = (type & TO_SNEAKRESIST)))
    type &= ~TO_SNEAKRESIST;

  if ((res_hide = (type & TO_HIDERESIST)))
    type &= ~TO_HIDERESIST;

  if (res_sneak && char_aff_flagged(ch, AFF_SNEAK)) {
    dcval = roll_skill(g, ch, SKILL_MOVE_SILENTLY); /* How difficult to counter? */
    if (char_get_skill(ch, SKILL_BALANCE))
      dcval += char_get_skill(ch, SKILL_BALANCE) / 10;
    if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 5 || char_get_genome(ch, 1) == 5)) {
      dcval += 10;
    }
    resskill = SKILL_SPOT; /* Skill used to resist      */
  } else if (res_hide && char_aff_flagged(ch, AFF_HIDE)) {
    dcval = roll_skill(g, ch, SKILL_HIDE);
    if (char_get_skill(ch, SKILL_BALANCE))
      dcval += char_get_skill(ch, SKILL_BALANCE) / 10;
    resskill = SKILL_SPOT;
  }

  /* this is a hack as well - DG_NO_TRIG is 256 -- Welcor */
  /* If the bit is set, unset dg_act_check, thus the ! below */
  if (!(g->dg_act_check = !IS_SET(type, DG_NO_TRIG)))
    REMOVE_BIT(type, DG_NO_TRIG);

  if (type == TO_CHAR) {
    if (ch && sendok(ch, to_sleeping) && (!resskill || (roll_skill(g, ch, resskill) >= dcval))) {
      perform_act(g, str, ch, obj, vict_obj, ch);
      return g->last_act_message;
    }
    return nullptr;
  }

  if (type == TO_VICT) {
    struct char_data *to = (struct char_data *)const_cast<void *>(vict_obj);
    if (to != nullptr && sendok(to, to_sleeping) && (!resskill || (roll_skill(g, to, resskill) >= dcval))) {
      perform_act(g, str, ch, obj, vict_obj, to);
      return g->last_act_message;
    }
    return nullptr;
  }

  if (type == TO_GMOTE) {
    std::string buf;
    for (descriptor_data *i : each_desc(g)) {
      if (!i->connected && i->character &&
          (char_is_npc(i->character) || !char_prf_flagged(g, i->character, PRF_NOGOSS)) && /* someone switched */
          !char_plr_flagged(i->character, PLR_WRITING) &&
          !room_flagged(g, char_get_in_room(i->character), ROOM_SOUNDPROOF)) {

        buf = str_format("@y{}@n", str);
        perform_act(g, buf.c_str(), ch, obj, vict_obj, i->character);
        std::string buf2 = str_format("{}\r\n", buf);
        add_history(g, i->character, buf2.c_str(), HIST_GOSSIP);
      }
    }
    return g->last_act_message;
  }

  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  room_vnum room;
  if (ch && char_get_in_room(ch) != NOWHERE)
    room = char_get_in_room(ch);
  else if (obj && obj_get_in_room(obj) != NOWHERE)
    room = obj_get_in_room(obj);
  else {
    return nullptr;
  }

  if ((type & TO_ROOM)) {

    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING)
        continue;

      if (ch != nullptr) {
        if (char_in_arena(g, ch)) {
          if (char_prf_flagged(g, d->character, PRF_ARENAWATCH)) {
            if (arena_watch(g, d->character) == char_get_in_room(ch)) {
              std::string buf3;
              buf3 = str_format("@c-----@CArena@c-----@n\r\n{}\r\n@c-----@CArena@c-----@n\r\n", str);
              perform_act(g, buf3.c_str(), ch, obj, vict_obj, d->character);
            }
          }
        }
      }
      if (char_get_listenroom(d->character) > 0) {
        int roll = rand_number(g, 1, 101);
        if (!resskill || (roll_skill(g, d->character, resskill) >= dcval)) {
          if (ch != nullptr && char_get_listenroom(d->character) == char_get_in_room(ch) && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
            std::string buf3;
            buf3 = str_format("-----Eavesdrop-----\r\n{}\r\n-----Eavesdrop-----\r\n", str);
            perform_act(g, buf3.c_str(), ch, obj, vict_obj, d->character);
          } else if (obj != nullptr && char_get_listenroom(d->character) == obj_get_in_room(obj) && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
            std::string buf3;
            buf3 = str_format("-----Eavesdrop-----\r\n{}\r\n-----Eavesdrop-----\r\n", str);
            perform_act(g, buf3.c_str(), ch, obj, vict_obj, d->character);
          }
        }
      }
    }
  }

  for (char_data *to : room_people(g, room)) {
    if (!sendok(to, to_sleeping) || (to == ch))
      continue;
    if (hide_invisible && ch && !char_can_see(g, to, ch))
      continue;
    if (type != TO_ROOM && to == vict_obj)
      continue;
    if (resskill && roll_skill(g, to, resskill) < dcval)
      continue;
    perform_act(g, str, ch, obj, vict_obj, to);
  }
  return g->last_act_message;
}

/* Shows the first help entry with a keyword that starts with entry. */
void show_help(GameWorld *g, struct descriptor_data *t, const char *entry)
{
  size_t len = strlen(entry);
  auto found = std::lower_bound(g->help_table.begin(), g->help_table.end(), entry,
                                [len](const help_index_element &el, const char *key) {
                                  return strncasecmp(el.keywords.c_str(), key, len) < 0;
                                });

  if (found == g->help_table.end() || strncasecmp(found->keywords.c_str(), entry, len))
    return;
  write_to_output(g, t, "\r\n{}\r\n", found->entry);
}

/* Thx to Jamie Nelson of 4D for this contribution */
void send_to_range(GameWorld *g, room_vnum start, room_vnum finish, std::string_view text)
{

  if (start > finish) {
    mud_log("send_to_range passed start room value greater then finish.");
    return;
  }
  for (room_vnum j : g->rooms.vnums()) {
    if (j >= start && j <= finish) {
      for (char_data *i : room_people(g, j)) {
        if (!char_get_desc(i))
          continue;
        write_to_output(g, char_get_desc(i), text);
      }
    }
  }
}
