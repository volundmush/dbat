#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"

/* don't change these */
constexpr int BAN_NOT    = 0;
constexpr int BAN_NEW    = 1;
constexpr int BAN_SELECT = 2;
constexpr int BAN_ALL    = 3;

constexpr int BANNED_SITE_LENGTH = 50;
struct ban_list_element {
  char site[BANNED_SITE_LENGTH + 1];
  int type;
  time_t date;
  char name[MAX_NAME_LENGTH + 1];
  struct ban_list_element *next;
};


void load_banned(GameWorld *g);
int isbanned(GameWorld *g, char *hostname);
void Free_Invalid_List(GameWorld *g);
void free_ban_list(GameWorld *g);
bool Valid_Name(GameWorld *g, char *newname);
void Read_Invalid_List(GameWorld *g);

ACMD(do_ban);
ACMD(do_unban);

constexpr int MAX_INVALID_NAMES = 200;
