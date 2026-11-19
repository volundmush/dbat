#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"

/* A line typed into the text editor: STRINGADD_OK if it isn't a command ("/<letter>"), else what the command does */
int improved_editor_execute(GameWorld *g, struct descriptor_data *d, std::string_view line);
void send_editor_help(GameWorld *g, struct descriptor_data *d);

/*
 * Defines for the action variable.
 */
constexpr int STRINGADD_OK     = 0; /* Just keep adding text.		*/
constexpr int STRINGADD_SAVE   = 1; /* Save current text.			*/
constexpr int STRINGADD_ABORT  = 2; /* Abort edit, restore old text.	*/
constexpr int STRINGADD_ACTION = 4; /* Editor action, don't append \r\n.	*/
