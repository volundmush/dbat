#include "dbat/game/disabled.h"

/*
 * Code to disable or enable buggy commands on the run, saving
 * a list of disabled commands to disk. Originally created by
 * Erwin S. Andreasen (erwin@andreasen.org) for Merc. Ported to
 * CircleMUD by Alexei Svitkine (Myrdred), isvitkin@sympatico.ca.
 *
 * Syntax is:
 *   disable - shows disabled commands
 *   disable <command> - toggles disable status of command
 * 
 */

ACMD(do_disable)
{
  int i, length;
  DISABLED_DATA *p, *temp;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't disable commands, silly.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument) { /* Nothing specified. Show disabled commands. */
    if (!disabled_first) /* Any disabled at all ? */
      send_to_char(ch, "There are no disabled commands.\r\n");
    else {
      send_to_char(ch,
        "Commands that are currently disabled:\r\n\r\n"
        " Command       Disabled by     Level\r\n"
        "-----------   --------------  -------\r\n");
      for (p = disabled_first; p; p = p->next)
        send_to_char(ch, " %-12s   %-12s    %3d\r\n", p->command->command, p->disabled_by, p->level);
    }
    return;
  }

  /* command given - first check if it is one of the disabled commands */
  for (length = strlen(argument), p = disabled_first; p ;  p = p->next)
    if (!strncmp(argument, p->command->command, length))
    break;
        
  if (p) { /* this command is disabled */

    /* Was it disabled by a higher level imm? */
    if (GET_ADMLEVEL(ch) < p->level) {
      send_to_char(ch, "This command was disabled by a higher power.\r\n");
      return;
    }

    REMOVE_FROM_LIST(p, disabled_first, next, temp);
    send_to_char(ch, "Command '%s' enabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "(GC) %s has enabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    free(p->disabled_by);
    free(p);
    save_disabled(); /* save to disk */

  } else { /* not a disabled command, check if the command exists */

    for (length = strlen(argument), i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strncmp(cmd_info[i].command, argument, length))
        if (GET_LEVEL(ch) >= cmd_info[i].minimum_level &&
            GET_ADMLEVEL(ch) >= cmd_info[i].minimum_admlevel)
          break;

    /*  Found?     */            
    if (*cmd_info[i].command == '\n') {
      send_to_char(ch, "You don't know of any such command.\r\n");
      return;
    }

    if (!strcmp(cmd_info[i].command, "disable")) {
      send_to_char (ch, "You cannot disable the disable command.\r\n");
      return;
    }

    /* Disable the command */
    CREATE(p, struct disabled_data, 1);
    p->command = &cmd_info[i];
    p->disabled_by = strdup(GET_NAME(ch)); /* save name of disabler  */
    p->level = GET_ADMLEVEL(ch);           /* save level of disabler */    
    p->subcmd = cmd_info[i].subcmd;       /* the subcommand if any  */    
    p->next = disabled_first;
    disabled_first = p; /* add before the current first element */
    send_to_char(ch, "Command '%s' disabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "(GC) %s has disabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    save_disabled(); /* save to disk */
  }
}

/* check if a command is disabled */   
int check_disabled(const struct command_info *command)
{
  DISABLED_DATA *p;

  for (p = disabled_first; p ; p = p->next)
    if (p->command->command_pointer == command->command_pointer)
      if (p->command->subcmd == command->subcmd)
        return TRUE;

  return FALSE;
}

/* Load disabled commands */
void load_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;
  int i;
  char line[READ_SIZE], name[MAX_INPUT_LENGTH], temp[MAX_INPUT_LENGTH];

  if (disabled_first)
    free_disabled();

  if ((fp = fopen(DISABLED_FILE, "r")) == NULL)
    return; /* No disabled file.. no disabled commands. */

  while (get_line(fp, line)) { 
    if (!strcasecmp(line, END_MARKER))
      break; /* break loop if we encounter the END_MARKER */
    CREATE(p, struct disabled_data, 1);
    sscanf(line, "%s %d %hd %s", name, &(p->subcmd), &(p->level), temp);
    /* Find the command in the table */
    for (i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strcasecmp(cmd_info[i].command, name))
        break;
    if (*cmd_info[i].command == '\n') { /* command does not exist? */
      log("WARNING: load_disabled(): Skipping unknown disabled command - '%s'!", name);
      free(p);
    } else { /* add new disabled command */
      p->disabled_by = strdup(temp);
      p->command = &cmd_info[i];
      p->next = disabled_first;
      disabled_first = p;
    }
  }
  fclose(fp);
}

/* Save disabled commands */
void save_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;

  if (!disabled_first) {
    /* delete file if no commands are disabled */
    unlink(DISABLED_FILE);
    return;
   }

  if ((fp = fopen(DISABLED_FILE, "w")) == NULL) {
    log("SYSERR: Could not open " DISABLED_FILE " for writing");
    return;
  }

  for (p = disabled_first; p ; p = p->next)
    fprintf (fp, "%s %d %d %s\n", p->command->command, p->subcmd, p->level, p->disabled_by);
  fprintf(fp, "%s\n", END_MARKER);
  fclose(fp);
}
  
/* free all disabled commands from memory */
void free_disabled()
{
  DISABLED_DATA *p;

  while (disabled_first) {
    p = disabled_first;
    disabled_first = disabled_first->next;
    free(p->disabled_by);
    free(p);
  }
}