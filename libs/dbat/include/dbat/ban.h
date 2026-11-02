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

extern struct ban_list_element *ban_list;
extern int num_invalid;

void load_banned(void);
int isbanned(char *hostname);
void Free_Invalid_List(void);
void free_ban_list(void);
bool Valid_Name(char *newname);
void Read_Invalid_List(void);

ACMD(do_ban);
ACMD(do_unban);
