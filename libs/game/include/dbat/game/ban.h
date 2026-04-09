#pragma once
#include "dbat/db/consts/types.h"

void load_banned(void);
int isbanned(char *hostname);
void Free_Invalid_List(void);
int Valid_Name(char *newname);
void Read_Invalid_List(void);

ACMD(do_ban);
ACMD(do_unban);
