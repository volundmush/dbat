#pragma once
#include "dbat/consts/types.h"
#include <stdio.h>

// functions
void boot_social_messages(void);
void free_social_messages(void);
void free_action(struct social_messg *mess);
void free_command_list(void);
void create_command_list(void);

// commands
ACMD(do_action);
ACMD(do_insult);
ACMD(do_gmote);
