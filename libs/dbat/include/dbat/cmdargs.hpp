#pragma once
#include <expected>
#include <string>
#include <string_view>

struct GameWorld;
struct char_data;
struct obj_data;

/*
 * A command's arguments, checked: each finder returns what it found, or the message telling the player why not.
 *
 *   auto obj = find_carried(g, ch, arg, "You don't seem to have that.\r\n");
 *   if (reject(g, ch, obj))
 *     return;
 *   ... *obj ...
 *
 * The word is taken by reference, as the game's finders take it, since they may strip a "2." off its front.
 */

/* What was asked for, or what to tell the player instead. */
template <class T> using parsed = std::expected<T, std::string>;

/* Sends ch the message, as it is, and returns true if p is one; returns false, saying nothing, if p holds a value. */
template <class T> bool reject(GameWorld *g, struct char_data *ch, const parsed<T> &p);

/* Something ch carries, or sees in the room, or inside container. */
parsed<struct obj_data *> find_carried(GameWorld *g, struct char_data *ch, std::string &word, std::string_view why);
parsed<struct obj_data *> find_in_room(GameWorld *g, struct char_data *ch, std::string &word, std::string_view why);
parsed<struct obj_data *> find_inside(GameWorld *g, struct char_data *ch, std::string &word, struct obj_data *container, std::string_view why);

/* Someone ch can see, where FIND_CHAR_ROOM or FIND_CHAR_WORLD says; find_player only finds players. */
parsed<struct char_data *> find_char(GameWorld *g, struct char_data *ch, std::string &word, int where, std::string_view why);
parsed<struct char_data *> find_player(GameWorld *g, struct char_data *ch, std::string &word, int where, std::string_view why);

void send_rejection(GameWorld *g, struct char_data *ch, const std::string &why);

template <class T> bool reject(GameWorld *g, struct char_data *ch, const parsed<T> &p)
{
  if (p)
    return false;
  send_rejection(g, ch, p.error());
  return true;
}
