#pragma once
#include "dbat/consts/types.hpp"

constexpr int MAP_ROWS = 199;
constexpr int MAP_COLS = 199;

struct mapstruct {
  int x{};
  int y{};
};
typedef struct mapstruct MapStruct;

void printmap(GameWorld *g, room_rnum rnum, struct char_data *ch, int type, obj_vnum vnum);

void update_space(GameWorld *g);

