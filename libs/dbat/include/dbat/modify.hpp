#pragma once
#include <string>

#include "dbat/consts/types.hpp"

// functions
/* Every ~ in s made a space: the world files end their texts with one */
void smash_tilde(std::string &s);

// commands
ACMD(do_skillset);

