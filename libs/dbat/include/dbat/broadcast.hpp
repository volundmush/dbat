#pragma once
#include <string_view>

#include "dbat/format.hpp"
#include "dbat/consts/types.hpp"

// functions

/* send_to_imm(): the text as it is */
void send_to_imm(GameWorld *g, std::string_view text);
/* send_to_imm(): the text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void send_to_imm(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  send_to_imm(g, str_format(fmt, std::forward<Args>(args)...));
}
void fly_zone(GameWorld *g, struct zone_data *zone, std::string_view messg, struct char_data *ch);
void send_to_scouter(GameWorld *g, std::string_view messg, struct char_data *ch, int num, int type);
void send_to_sense(GameWorld *g, int type, std::string_view messg, struct char_data *ch);
void send_to_worlds(GameWorld *g, struct char_data *ch);

