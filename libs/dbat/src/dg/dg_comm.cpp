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
#include "dbat/iterate.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/races.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/graph.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/game_world.hpp"

/* A piece of a message: its text, then who or what stands where the next ~|^&*_ was, if one was */
struct sub_piece {
  std::string text;
  char type{};              /* the symbol, or 0 for the last piece */
  struct char_data *ch{};   /* for ~ | ^ & * */
  struct obj_data *obj{};   /* for _ */
};

/* The name after a symbol: a word, ending at a space or at punctuation other than '#' and '-', lowercased */
static std::string take_name(std::string_view text, size_t &p)
{
  std::string name;

  while (p < text.size() && isspace((unsigned char)text[p]))
    p++;
  for (; p < text.size() && !isspace((unsigned char)text[p]) &&
         (!ispunct((unsigned char)text[p]) || text[p] == '#' || text[p] == '-');
       p++)
    name += (char)tolower((unsigned char)text[p]);
  return name;
}

/* The message as ch sees it */
static void sub_write_to_char(GameWorld *g, struct char_data *ch, const std::vector<sub_piece> &pieces)
{
  std::string sb;

  for (const sub_piece &piece : pieces) {
    sb += piece.text;
    struct char_data *who = piece.ch;

    switch (piece.type) {
      case '~':
        if (!who)
          sb += "someone";
        else if (who == ch)
          sb += "you";
        else
          sb += char_pers(g, who, ch);
        break;

      case '|':
        if (!who)
          sb += "someone's";
        else if (who == ch)
          sb += "your";
        else {
          sb += char_pers(g, who, ch);
          sb += "'s";
        }
        break;

      case '^':
        if (!who || !char_can_see(g, ch, who))
          sb += "its";
        else if (who == ch)
          sb += "your";
        else
          sb += char_hshr(who);
        break;

      case '&':
        if (!who || !char_can_see(g, ch, who))
          sb += "it";
        else if (who == ch)
          sb += "you";
        else
          sb += char_hssh(who);
        break;

      case '*':
        if (!who || !char_can_see(g, ch, who))
          sb += "it";
        else if (who == ch)
          sb += "you";
        else
          sb += char_hmhr(who);
        break;

      case '_':
        if (!piece.obj)
          sb += "something";
        else
          sb += obj_shortdesc_for(g, piece.obj, ch);
        break;
    }
  }

  sb += "\n\r";
  sb[0] = (char)toupper((unsigned char)sb[0]);
  send_to_char(g, ch, "{}", sb);
}

/*
 * Sends arg to ch (act_to::actor) and to the others in ch's room (act_to::room), each seeing the characters and objects it
 * names as they see them: ~name is the character's name (you), |name its possessive (your), ^name his/her/its,
 * &name he/she/it, *name him/her/it, and _name an object's short description. They are found in ch's room whether
 * or not ch can see them. A backslash gives the character after it as it is.
 */
void sub_write(GameWorld *g, std::string_view arg, struct char_data *ch, act_to targets)
{
  std::vector<sub_piece> pieces(1);
  room_data *room = room_of(g, char_get_in_room(ch));
  bool to_sleeping = true; /* mainly for windows compiles */

  for (size_t p = 0; p < arg.size();) {
    switch (arg[p]) {
      case '~':
      case '|':
      case '^':
      case '&':
      case '*': {
        sub_piece &piece = pieces.back();
        piece.type       = arg[p++];
        piece.ch         = get_char_in_room(g, room, take_name(arg, p));
        pieces.emplace_back();
        break;
      }

      case '_': {
        sub_piece &piece = pieces.back();
        piece.type       = arg[p++];
        piece.obj        = get_obj_in_room(g, room, take_name(arg, p));
        pieces.emplace_back();
        break;
      }

      case '\\': /* the next character as it is, if there is one */
        p++;
        if (p < arg.size())
          pieces.back().text += arg[p++];
        break;

      default:
        pieces.back().text += arg[p++];
    }
  }

  if ((std::to_underlying(targets) & std::to_underlying(act_to::actor)) && sendok(ch, to_sleeping))
    sub_write_to_char(g, ch, pieces);

  if (std::to_underlying(targets) & std::to_underlying(act_to::room))
    for (char_data *to : room_people(g, char_get_in_room(ch)))
      if (to != ch && sendok(to, to_sleeping))
        sub_write_to_char(g, to, pieces);
}

void send_to_zone(GameWorld *g, std::string_view messg, struct zone_data *zone)
{
  if (messg.empty())
    return;

  for (descriptor_data *i : each_desc(g))
    if (!i->connected && i->character && char_is_awake(i->character) &&
        (char_get_in_room(i->character) != NOWHERE) &&
        (room_get_zone(g, char_get_in_room(i->character)) == zone))
      write_to_output(g, i, "{}", messg);
}
