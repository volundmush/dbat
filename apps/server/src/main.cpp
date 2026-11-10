//
// Created by basti on 10/22/2021.
//

#include <csignal>
#include <cstring>
#include <cstdlib>

#include <unistd.h>

#include "dbat/config.hpp"
#include "dbat/comm.hpp"
#include "dbat/utils.hpp"
#include "dbat/mail.hpp"
#include "dbat/boards.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.social.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/ban.hpp"
#include "dbat/genolc.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/game_world.hpp"

static void log_audit_problem(void *, const char *problem)
{
  mud_log_printf("SYSERR: World audit: %s", problem);
}

int main(int argc, char **argv)
{
  int pos = 1;
  const char *dir;
  GameWorld *g = game_world_create();

  /*
   * Until signal_setup() runs. The deadlock timer outlives a copyover's exec, and an older server won't
   * have stopped it, so this process could otherwise die part way through booting.
   */
  signal(SIGVTALRM, SIG_IGN);

  g->launch_argc = argc;
  g->launch_argv = argv;
  g->launch_dir  = getcwd(nullptr, 0);

  /****************************************************************************/
  /** Load the game configuration.                                           **/
  /** We must load BEFORE we use any of the constants stored in constants.c. **/
  /** Otherwise, there will be no variables set to set the rest of the vars  **/
  /** to, which will mean trouble --> Mythran                                **/
  /****************************************************************************/
  while ((pos < argc) && (*(argv[pos]) == '-')) {
    if (*(argv[pos] + 1) == 'f') {
      if (*(argv[pos] + 2))
        CONFIG_CONFFILE = argv[pos] + 2;
      else if (++pos < argc)
        CONFIG_CONFFILE = argv[pos];
      else {
        puts("SYSERR: File name to read from expected after option -f.");
        exit(1);
      }
    }
    pos++;
  }
  pos = 1;

  if (CONFIG_CONFFILE.empty())
    CONFIG_CONFFILE = CONFIG_FILE;

  load_config(g);

  g->port = CONFIG_DFLT_PORT;
  dir     = CONFIG_DFLT_DIR.c_str();

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
      case 'f':
        if (!*(argv[pos] + 2))
          ++pos;
        break;
      case 'o':
        if (*(argv[pos] + 2))
          CONFIG_LOGNAME = argv[pos] + 2;
        else if (++pos < argc)
          CONFIG_LOGNAME = argv[pos];
        else {
          puts("SYSERR: File name to log to expected after option -o.");
          exit(1);
        }
        break;
      case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
        g->fCopyOver   = true;
        g->mother_desc = atoi(argv[pos] + 2);
        break;
      case 'd':
        if (*(argv[pos] + 2))
          dir = argv[pos] + 2;
        else if (++pos < argc)
          dir = argv[pos];
        else {
          puts("SYSERR: Directory arg expected after option -d.");
          exit(1);
        }
        break;
      case 'm':
        g->mini_mud      = 1;
        g->no_rent_check = 1;
        puts("Running in minimized mode & with no rent check.");
        break;
      case 'c':
        g->scheck = 1;
        puts("Syntax check mode enabled.");
        break;
      case 'q':
        g->no_rent_check = 1;
        puts("Quick boot mode -- rent check supressed.");
        break;
      case 'r':
        g->circle_restrict = 1;
        puts("Restricting game -- no new players allowed.");
        break;
      case 's':
        g->no_specials = 1;
        puts("Suppressing assignment of special routines.");
        break;
      case 'x':
        g->xap_objs = 1;
        mud_log_printf("Loading player objects from secondary (ascii) files.");
        break;
      case 'h':
        /* From: Anil Mahajan <amahajan@proxicom.com> */
        printf("Usage: %s [-c] [-m] [-x] [-q] [-r] [-s] [-d pathname] [port #]\n"
               "  -c             Enable syntax check mode.\n"
               "  -d <directory> Specify library directory (defaults to 'lib').\n"
               "  -f<file>       Use <file> for configuration.\n"
               "  -h             Print this command line argument help.\n"
               "  -m             Start in mini-MUD mode.\n"
               "  -o <file>      Write log to <file> instead of stderr.\n"
               "  -q             Quick boot (doesn't scan rent for object limits)\n"
               "  -r             Restrict MUD -- no new players allowed.\n"
               "  -s             Suppress special procedure assignments.\n"
               " Note:         These arguments are 'CaSe SeNsItIvE!!!'\n"
               "  -x             Load using secondary (ascii) files.\n",
               argv[0]);
        exit(0);
      default:
        printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
        break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((g->port = atoi(argv[pos])) <= 1024) {
      printf("SYSERR: Illegal port number %d.\n", g->port);
      exit(1);
    }
  }

  /* All arguments have been parsed, try to open log file. */
  setup_log(CONFIG_LOGNAME.c_str(), STDERR_FILENO, g->fCopyOver);

  /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */
  mud_log_printf("Using %s for configuration.", CONFIG_CONFFILE.c_str());
  g->xap_objs = 1;
  if (chdir(dir) < 0) {
    perror("SYSERR: Fatal error changing to data directory");
    exit(1);
  }
  mud_log_printf("Using %s as data directory.", dir);

  if (g->scheck) {
    boot_world(g);
    world_audit(g, log_audit_problem, nullptr);
  } else {
    mud_log_printf("Running game on port %d.", g->port);
    init_game(g, g->port);
  }

  game_world_free_contents(g);

  free(g->launch_dir);

  game_world_destroy(g);

  mud_log_printf("Done.");

  return (0);
}
