#pragma once
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/io/load_error.hpp"

/* don't change these */
constexpr int BAN_NOT    = 0;
constexpr int BAN_NEW    = 1;
constexpr int BAN_SELECT = 2;
constexpr int BAN_ALL    = 3;

constexpr int BANNED_SITE_LENGTH = 50;
struct ban_list_element {
  std::string site; /* up to BANNED_SITE_LENGTH, in lowercase */
  int type{};
  time_t date{};
  std::string name; /* who banned it, up to MAX_NAME_LENGTH */
  struct ban_list_element *next{};
};


void load_banned(GameWorld *g);
int isbanned(GameWorld *g, std::string_view hostname);
void Free_Invalid_List(GameWorld *g);
void free_ban_list(GameWorld *g);
bool Valid_Name(GameWorld *g, std::string_view newname);
/* Reads misc/xnames; an error if it holds too many */
io::loaded<void> Read_Invalid_List(GameWorld *g);
/* Writes the ban list to etc/badsites */
void save_ban_list(GameWorld *g);

ACMD(do_ban);
ACMD(do_unban);

constexpr int MAX_INVALID_NAMES = 200;

