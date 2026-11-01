#pragma once
#include "dbat/consts/types.h"

#define MAP_ROWS 199
#define MAP_COLS 199

struct mapstruct {
  int x;
  int y;
};
typedef struct mapstruct MapStruct;

void printmap(int rnum, struct char_data *ch, int type, int vnum);

extern room_vnum mapnums[MAP_ROWS + 1][MAP_COLS + 1];
