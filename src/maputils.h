/***************************************************** 
* maputils.h --- implementation file for ascii maps  * 
*				     		     *
* Kyle Goodwin, (c) 1998 All Rights Reserved         *
* vbmasta@earthlink.net - Head Implemenor FirocMUD   *
*			     			     *
* Paolo Libardi - pinkpallin@libero.it		     *
*****************************************************/  

#ifndef __MAPUTILS_H__
#define __MAPUTILS_H__

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"

#define MAP_ROWS	199
#define MAP_COLS	199

struct mapstruct {
  int x;
  int y;
};
typedef struct mapstruct MapStruct;  

MapStruct findcoord(int rnum);
void printmap(int rnum, struct char_data * ch, int type, int vnum);
void ping_ship(int vnum, int vnum2);

extern int mapnums[MAP_ROWS+1][MAP_COLS+1];

#endif