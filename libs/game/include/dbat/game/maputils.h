#pragma once
#include "dbat/db/consts/types.h"
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
