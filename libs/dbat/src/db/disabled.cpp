#include <cstdlib>

#include <unistd.h>

#include "dbat/args.hpp"
#include "dbat/disabled.hpp"
#include "dbat/utils.hpp"
#include "dbat/command.hpp"
#include "dbat/fileop.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"


static void save_disabled(GameWorld *g);

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

  if (char_is_npc(ch)) {
    send_to_char_printf(g, ch, "Monsters can't disable commands, silly.\r\n");
    return;
  }

  std::string text(skip_spaces(argument));

  if (text.empty()) {       /* Nothing specified. Show disabled commands. */
    if (!g->disabled_first) /* Any disabled at all ? */
      send_to_char_printf(g, ch, "There are no disabled commands.\r\n");
    else {
      send_to_char_printf(g, ch,
                          "Commands that are currently disabled:\r\n\r\n"
                          " Command       Disabled by     Level\r\n"
                          "-----------   --------------  -------\r\n");
      for (p = g->disabled_first; p; p = p->next)
        send_to_char_printf(g, ch, " %-12s   %-12s    %3d\r\n", p->command->command, p->disabled_by, p->level);
    }
    return;
  }

  /* command given - first check if it is one of the disabled commands */
  for (length = text.size(), p = g->disabled_first; p; p = p->next)
    if (!strncmp(text.c_str(), p->command->command, length))
      break;

  if (p) { /* this command is disabled */

    /* Was it disabled by a higher level imm? */
    if (char_get_admlevel(ch) < p->level) {
      send_to_char_printf(g, ch, "This command was disabled by a higher power.\r\n");
      return;
    }

    REMOVE_FROM_LIST(p, g->disabled_first, next, temp);
    send_to_char_printf(g, ch, "Command '%s' enabled.\r\n", p->command->command);
    mudlog_printf(g, LOG_BRF, ADMLVL_IMMORT, true, "(GC) %s has enabled the command '%s'.",
                  char_get_name(ch), p->command->command);
    free(p->disabled_by);
    delete p;
    save_disabled(g); /* save to disk */

  } else { /* not a disabled command, check if the command exists */

    for (length = text.size(), i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strncmp(cmd_info[i].command, text.c_str(), length))
        if (char_get_level(ch) >= cmd_info[i].minimum_level &&
            char_get_admlevel(ch) >= cmd_info[i].minimum_admlevel)
          break;

    /*  Found?     */
    if (*cmd_info[i].command == '\n') {
      send_to_char_printf(g, ch, "You don't know of any such command.\r\n");
      return;
    }

    if (!strcmp(cmd_info[i].command, "disable")) {
      send_to_char_printf(g, ch, "You cannot disable the disable command.\r\n");
      return;
    }

    /* Disable the command */
    p                 = new disabled_data();
    p->command     = &cmd_info[i];
    p->disabled_by = strdup(char_get_name(ch)); /* save name of disabler  */
    p->level       = char_get_admlevel(ch);     /* save level of disabler */
    p->subcmd      = cmd_info[i].subcmd;        /* the subcommand if any  */
    p->next        = g->disabled_first;
    g->disabled_first = p; /* add before the current first element */
    send_to_char_printf(g, ch, "Command '%s' disabled.\r\n", p->command->command);
    mudlog_printf(g, LOG_BRF, ADMLVL_IMMORT, true, "(GC) %s has disabled the command '%s'.",
                  char_get_name(ch), p->command->command);
    save_disabled(g); /* save to disk */
  }
}

/* check if a command is disabled */
bool check_disabled(GameWorld *g, const struct command_info *command)
{
  DISABLED_DATA *p;

  for (p = g->disabled_first; p; p = p->next)
    if (p->command->command_pointer == command->command_pointer)
      if (p->command->subcmd == command->subcmd)
        return true;

  return false;
}

/* Load disabled commands */
void load_disabled(GameWorld *g)
{
  FILE *fp;
  DISABLED_DATA *p;
  int i;
  char line[READ_SIZE], name[MAX_INPUT_LENGTH], temp[MAX_INPUT_LENGTH];

  if (g->disabled_first)
    free_disabled(g);

  if ((fp = fopen(DISABLED_FILE, "r")) == nullptr)
    return; /* No disabled file.. no disabled commands. */

  while (get_line(fp, line)) {
    if (!strcasecmp(line, END_MARKER))
      break; /* break loop if we encounter the END_MARKER */
    p = new disabled_data();
    sscanf(line, "%s %d %hd %s", name, &(p->subcmd), &(p->level), temp);
    /* Find the command in the table */
    for (i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strcasecmp(cmd_info[i].command, name))
        break;
    if (*cmd_info[i].command == '\n') { /* command does not exist? */
      mud_log_printf("WARNING: load_disabled(): Skipping unknown disabled command - '%s'!", name);
      delete p;
    } else { /* add new disabled command */
      p->disabled_by = strdup(temp);
      p->command     = &cmd_info[i];
      p->next        = g->disabled_first;
      g->disabled_first = p;
    }
  }
  fclose(fp);
}

/* Save disabled commands */
static void save_disabled(GameWorld *g)
{
  struct atomic_file af;
  FILE *fp;
  DISABLED_DATA *p;

  if (!g->disabled_first) {
    /* delete file if no commands are disabled */
    unlink(DISABLED_FILE);
    return;
  }

  if ((fp = atomic_open(&af, DISABLED_FILE, false)) == nullptr) {
    mud_log_printf("SYSERR: Could not open " DISABLED_FILE " for writing");
    return;
  }

  for (p = g->disabled_first; p; p = p->next)
    fprintf(fp, "%s %d %d %s\n", p->command->command, p->subcmd, p->level, p->disabled_by);
  fprintf(fp, "%s\n", END_MARKER);
  atomic_commit(&af);
}

/* free all disabled commands from memory */
void free_disabled(GameWorld *g)
{
  DISABLED_DATA *p;

  while (g->disabled_first) {
    p              = g->disabled_first;
    g->disabled_first = g->disabled_first->next;
    free(p->disabled_by);
    delete p;
  }
}
