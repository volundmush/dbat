#include "dbat/db/command.h"

struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;
  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);
  return (-1);
}

struct command_info *complete_cmd_info;