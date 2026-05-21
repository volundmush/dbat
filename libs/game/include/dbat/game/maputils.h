#pragma once
#include "dbat/db/consts/types.h"


#ifdef __cplusplus
extern "C" {
#endif

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

extern room_vnum mapnums[MAP_ROWS+1][MAP_COLS+1];


#ifdef __cplusplus
}
#endif
