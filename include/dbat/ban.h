#pragma once

#include "structs.h"

extern struct ban_list_element *ban_list;
extern int num_invalid;


extern void load_banned();

extern int isbanned(char *hostname);

extern void Free_Invalid_List();

extern int Valid_Name(char *newname);

extern void Read_Invalid_List();

extern ACMD(do_ban);

extern ACMD(do_unban);
