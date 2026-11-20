#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/format.hpp"

/* Saves everything and re-executes the server, keeping the players' connections. Returns only if it was
 * aborted before anything changed. */
void execute_copyover(GameWorld *g);
/* Tells everyone playing (or building) about the copyover */
void game_info(GameWorld *g, std::string_view text);
template <class... Args>
  requires(sizeof...(Args) > 0)
void game_info(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  game_info(g, str_format(fmt, std::forward<Args>(args)...));
}
