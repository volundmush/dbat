#pragma once

#include "structs.h"

// global variables
extern std::map<int64_t, player_data> players;

struct char_data *findPlayer(const std::string& name);

OpResult<> validate_pc_name(const std::string& name);