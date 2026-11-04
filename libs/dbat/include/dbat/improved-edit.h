#pragma once
#include "dbat/consts/types.h"

constexpr int CONFIG_IMPROVED_EDITOR = 1;

constexpr int using_improved_editor = CONFIG_IMPROVED_EDITOR;

int improved_editor_execute(GameWorld *g, struct descriptor_data *d, char *string);
void send_editor_help(GameWorld *g, struct descriptor_data *d);

/*
 * Action modes for parse_edit_action().
 */
constexpr int PARSE_FORMAT    = 0;
constexpr int PARSE_REPLACE   = 1;
constexpr int PARSE_HELP      = 2;
constexpr int PARSE_DELETE    = 3;
constexpr int PARSE_INSERT    = 4;
constexpr int PARSE_LIST_NORM = 5;
constexpr int PARSE_LIST_NUM  = 6;
constexpr int PARSE_EDIT      = 7;

/*
 * Defines for the action variable.
 */
constexpr int STRINGADD_OK     = 0; /* Just keep adding text.		*/
constexpr int STRINGADD_SAVE   = 1; /* Save current text.			*/
constexpr int STRINGADD_ABORT  = 2; /* Abort edit, restore old text.	*/
constexpr int STRINGADD_ACTION = 4; /* Editor action, don't append \r\n.	*/

/* Settings for formatter. */
constexpr int FORMAT_INDENT = (1 << 0);
