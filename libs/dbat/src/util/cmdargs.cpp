#include "dbat/cmdargs.hpp"

#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"

namespace {
  template <class T> parsed<T *> found(T *it, std::string_view why)
  {
    if (it)
      return it;
    return std::unexpected(std::string(why));
  }
} // namespace

void send_rejection(GameWorld *g, struct char_data *ch, const std::string &why)
{
  send_to_char(g, ch, std::string_view(why));
}

parsed<struct obj_data *> find_carried(GameWorld *g, struct char_data *ch, std::string &word, std::string_view why)
{
  return found(get_obj_in_list_vis(g, ch, word, nullptr, char_get_inventory(ch)), why);
}

parsed<struct obj_data *> find_in_room(GameWorld *g, struct char_data *ch, std::string &word, std::string_view why)
{
  return found(get_obj_in_list_vis(g, ch, word, nullptr, room_get_contents(g, char_get_in_room(ch))), why);
}

parsed<struct obj_data *> find_inside(GameWorld *g, struct char_data *ch, std::string &word, struct obj_data *container, std::string_view why)
{
  return found(get_obj_in_list_vis(g, ch, word, nullptr, obj_get_contains(container)), why);
}

parsed<struct char_data *> find_char(GameWorld *g, struct char_data *ch, std::string &word, int where, std::string_view why)
{
  return found(get_char_vis(g, ch, word, nullptr, where), why);
}

parsed<struct char_data *> find_player(GameWorld *g, struct char_data *ch, std::string &word, int where, std::string_view why)
{
  return found(get_player_vis(g, ch, word, nullptr, where), why);
}
