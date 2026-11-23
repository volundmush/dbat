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
#include <string>

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

/* what $N and the rest give: "<NULL>" for nothing */
static std::string_view or_null(const char *text) { return text ? text : ACTNULL; }

/* higher-level communication: the act() function */
static void perform_act(GameWorld *g, std::string_view orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to)
{
  constexpr size_t room             = MAX_STRING_LENGTH - 3; /* the most of the message kept, before its "\r\n" */
  std::string text;
  bool uppercasenext                = false;
  const struct char_data *dg_victim = nullptr;
  const struct obj_data *dg_target  = nullptr;

  /* Adds c, if there's room (what doesn't fit is left off), capitalised if it's the first that isn't a space since
     a $U */
  auto add = [&](char c) {
    if (text.size() >= room)
      return;
    if (uppercasenext && !isspace((int)c)) {
      c             = toupper(c);
      uppercasenext = false;
    }
    text += c;
  };

  for (size_t at = 0; at < orig.size(); at++) {
    if (orig[at] != '$') {
      add(orig[at]);
      continue;
    }
    std::string_view i;
    std::string held; /* what i views, when it is made here */
    switch (++at < orig.size() ? orig[at] : '\0') { /* a '$' at the end is an unknown code, as its '\0' was */
      case 'n':
        i = held = char_pers(g, ch, to);
        break;
      case 'N':
        i = vict_obj ? std::string_view(held = char_pers(g, (struct char_data *)const_cast<void *>(vict_obj), to)) : ACTNULL;
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 'm':
        i = char_hmhr(ch);
        break;
      case 'M':
        i = vict_obj ? char_hmhr((const struct char_data *)vict_obj) : ACTNULL;
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 's':
        i = char_hshr(ch);
        break;
      case 'S':
        i = vict_obj ? char_hshr((const struct char_data *)vict_obj) : ACTNULL;
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 'e':
        i = char_hssh(ch);
        break;
      case 'E':
        i = vict_obj ? char_hssh((const struct char_data *)vict_obj) : ACTNULL;
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 'o':
        i = obj ? obj_fname_for(g, obj, to) : ACTNULL;
        break;
      case 'O':
        i = vict_obj ? obj_fname_for(g, (const struct obj_data *)vict_obj, to) : ACTNULL;
        dg_target = (const struct obj_data *)vict_obj;
        break;
      case 'p':
        i = obj ? obj_shortdesc_for(g, obj, to) : ACTNULL;
        break;
      case 'P':
        i = vict_obj ? obj_shortdesc_for(g, (const struct obj_data *)vict_obj, to) : ACTNULL;
        dg_target = (const struct obj_data *)vict_obj;
        break;
      case 'a':
        i = obj ? obj_sana(obj) : ACTNULL;
        break;
      case 'A':
        i = vict_obj ? obj_sana((const struct obj_data *)vict_obj) : ACTNULL;
        dg_target = (const struct obj_data *)vict_obj;
        break;
      case 'T':
        i = vict_obj ? or_null((const char *)vict_obj) : ACTNULL;
        break;
      case 't':
        i = obj ? or_null((char *)obj) : ACTNULL;
        break;
      case 'F':
        i = vict_obj ? fname((const char *)vict_obj) : ACTNULL;
        break;
      /* uppercase previous word */
      case 'u': {
        size_t space = text.find_last_of(" \t\n\v\f\r"); /* isspace()'s */
        size_t word  = space == std::string::npos ? 0 : space + 1;
        if (word != text.size())
          text[word] = toupper(text[word]);
        i = "";
        break;
      }
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
    for (char c : i)
      add(c);
  }
  text += "\r\n";

  /* capitalised only if it goes to a connection, though the trigger and last_act_message see it too (quirk) */
  if (char_get_desc(to))
    write_to_output(g, char_get_desc(to), "{}", CAP(text));

  if ((char_is_npc(to) && g->dg_act_check) && (to != ch))
    act_mtrigger(g, to, text, ch, dg_victim, obj, dg_target);

  g->last_act_message = std::move(text);
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

/* The message the last act() made, which act() returns even when it sent nothing this time (quirk), or nullptr if
   there has been none */
static const std::string *last_act_message(const GameWorld *g)
{
  return g->last_act_message.empty() ? nullptr : &g->last_act_message;
}

const std::string *act(GameWorld *g, std::string_view str, int hide_invisible, struct char_data *ch,
                       struct obj_data *obj, const void *vict_obj, int type)
{
  int to_sleeping, res_sneak, res_hide, dcval = 0, resskill = 0;

  str = str.substr(0, str.find('\0')); /* the message ends at a '\0', as a C string did */
  if (str.empty())
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
      return last_act_message(g);
    }
    return nullptr;
  }

  if (type == TO_VICT) {
    struct char_data *to = (struct char_data *)const_cast<void *>(vict_obj);
    if (to != nullptr && sendok(to, to_sleeping) && (!resskill || (roll_skill(g, to, resskill) >= dcval))) {
      perform_act(g, str, ch, obj, vict_obj, to);
      return last_act_message(g);
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
        perform_act(g, buf, ch, obj, vict_obj, i->character);
        std::string buf2 = str_format("{}\r\n", buf);
        add_history(g, i->character, buf2, HIST_GOSSIP);
      }
    }
    return last_act_message(g);
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
              perform_act(g, buf3, ch, obj, vict_obj, d->character);
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
            perform_act(g, buf3, ch, obj, vict_obj, d->character);
          } else if (obj != nullptr && char_get_listenroom(d->character) == obj_get_in_room(obj) && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
            std::string buf3;
            buf3 = str_format("-----Eavesdrop-----\r\n{}\r\n-----Eavesdrop-----\r\n", str);
            perform_act(g, buf3, ch, obj, vict_obj, d->character);
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
  return last_act_message(g);
}

/* Shows the first help entry with a keyword that starts with entry. */
void show_help(GameWorld *g, struct descriptor_data *t, std::string_view entry)
{
  auto starts_before = [&](const help_index_element &el) { return compare_nocase_n(el.keywords, entry, entry.size()) < 0; };
  auto found         = std::ranges::partition_point(g->help_table, starts_before);

  if (found == g->help_table.end() || compare_nocase_n(found->keywords, entry, entry.size()))
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
