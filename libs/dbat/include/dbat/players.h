#pragma once

#include "structs.h"

// global variables

Character *findPlayer(const std::string &name);

OpResult<> validate_pc_name(const std::string &name);

extern bool canDeleteCharacter(std::weak_ptr<Character> ref);
extern bool deleteUserAccount(vnum id);
extern void deletePlayerCharacter(std::weak_ptr<Character> ref);