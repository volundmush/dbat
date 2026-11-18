#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>

#include "dbat/args.hpp"
#include "dbat/disabled.hpp"
#include "dbat/utils.hpp"
#include "dbat/command.hpp"
#include "dbat/fileop.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/stringutils.hpp"
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
    send_to_char(g, ch, "Monsters can't disable commands, silly.\r\n");
    return;
  }

  std::string text(skip_spaces(argument));

  if (text.empty()) {       /* Nothing specified. Show disabled commands. */
    if (!g->disabled_first) /* Any disabled at all ? */
      send_to_char(g, ch, "There are no disabled commands.\r\n");
    else {
      send_to_char(g, ch,
                   "Commands that are currently disabled:\r\n\r\n Command       Disabled by     Level\r\n-----------   --------------  -------\r\n");
      for (p = g->disabled_first; p; p = p->next)
        send_to_char(g, ch, " {:<12}   {:<12}    {:>3d}\r\n", p->command->command, p->disabled_by, p->level);
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
      send_to_char(g, ch, "This command was disabled by a higher power.\r\n");
      return;
    }

    REMOVE_FROM_LIST(p, g->disabled_first, next, temp);
    send_to_char(g, ch, "Command '{}' enabled.\r\n", p->command->command);
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "(GC) {} has enabled the command '{}'.",
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
      send_to_char(g, ch, "You don't know of any such command.\r\n");
      return;
    }

    if (!strcmp(cmd_info[i].command, "disable")) {
      send_to_char(g, ch, "You cannot disable the disable command.\r\n");
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
    send_to_char(g, ch, "Command '{}' disabled.\r\n", p->command->command);
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "(GC) {} has disabled the command '{}'.",
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

/* Load disabled commands: "<command> <subcmd> <level> <who>" a line, up to "END" */
void load_disabled(GameWorld *g)
{
  if (g->disabled_first)
    free_disabled(g);

  io::loaded<io::text_reader> file = io::text_reader::open(DISABLED_FILE);
  if (!file)
    return; /* No disabled file.. no disabled commands. */

  /* quirk: a line short of its name or who disabled it keeps the one before's, as sscanf() left them */
  std::string name, by;
  while (std::optional<std::string_view> line = file->line()) {
    if (str_eq_nocase(*line, END_MARKER))
      break;
    auto *p = new disabled_data();
    io::fields f(*line);
    int level = 0;
    if (f.word(name) && f.number(p->subcmd) && f.number(level))
      p->level = static_cast<int16_t>(level);
    f.word(by);
    int i = 0;
    while (*cmd_info[i].command != '\n' && !str_eq_nocase(cmd_info[i].command, name))
      i++;
    if (*cmd_info[i].command == '\n') { /* command does not exist? */
      mud_log("WARNING: load_disabled(): Skipping unknown disabled command - '{}'!", name);
      delete p;
    } else { /* add new disabled command */
      p->disabled_by    = strdup(by.c_str());
      p->command        = &cmd_info[i];
      p->next           = g->disabled_first;
      g->disabled_first = p;
    }
  }
}

/* Save disabled commands */
static void save_disabled(GameWorld *g)
{
  if (!g->disabled_first) {
    /* delete file if no commands are disabled */
    std::error_code ec;
    std::filesystem::remove(DISABLED_FILE, ec);
    return;
  }

  io::text_writer out(DISABLED_FILE);
  for (DISABLED_DATA *p = g->disabled_first; p; p = p->next)
    out.print("{} {:d} {:d} {}\n", p->command->command, p->subcmd, p->level, p->disabled_by);
  out.print("{}\n", END_MARKER);
  out.commit();
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
