#pragma once
#include <cstdio>

#include "dbat/consts/types.hpp"

// functions
void free_social_messages(GameWorld *g);
void free_command_list(GameWorld *g);
void create_command_list(GameWorld *g);

// commands
ACMD(do_action);
ACMD(do_insult);
ACMD(do_gmote);

