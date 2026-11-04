#pragma once
#include "dbat/consts/types.h"

/* obj_edit.c functions */
void pobj_edit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
void disp_custom_menu(GameWorld *g, struct descriptor_data *d);
void disp_restring_menu(GameWorld *g, struct descriptor_data *d);

/*-------------  obj_edit.c defines -------------*/

/* What object edit menu are we in? */
constexpr int EDIT_NONE     = 0;
constexpr int EDIT_CUSTOM   = 1;
constexpr int EDIT_RESTRING = 2;

/* Menu Values */
/* Custom Equipment */
constexpr int EDIT_CUSTOM_MAIN   = 1;
constexpr int EDIT_CUSTOM_NAME   = 2;
constexpr int EDIT_CUSTOM_SDESC  = 3;
constexpr int EDIT_CUSTOM_LDESC  = 4;
constexpr int EDIT_CUSTOM_TYPE   = 5;
constexpr int EDIT_CUSTOM_WEAPON = 6;
constexpr int EDIT_CUSTOM_QUIT   = 7;

/* Restring Equipment */
constexpr int EDIT_RESTRING_MAIN  = 1;
constexpr int EDIT_RESTRING_NAME  = 2;
constexpr int EDIT_RESTRING_SDESC = 3;
constexpr int EDIT_RESTRING_LDESC = 4;
constexpr int EDIT_RESTRING_QUIT  = 5;
constexpr int EDIT_RESTRING_CONF  = 6;
