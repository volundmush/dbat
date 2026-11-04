//
// Created by basti on 10/22/2021.
//

#include "dbat/config.h"
#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/mail.h"
#include "dbat/boards.h"
#include "dbat/act.informative.h"
#include "dbat/act.social.h"
#include "dbat/dg_scripts.h"
#include "dbat/ban.h"
#include "dbat/genolc.h"

#include <unistd.h>
#include "dbat/game_world.h"

int main(int argc, char **argv)
{
  int pos = 1;
  const char *dir;
  GameWorld *g = game_world_create();

  /****************************************************************************/
  /** Load the game configuration.                                           **/
  /** We must load BEFORE we use any of the constants stored in constants.c. **/
  /** Otherwise, there will be no variables set to set the rest of the vars  **/
  /** to, which will mean trouble --> Mythran                                **/
  /****************************************************************************/
  CONFIG_CONFFILE = nullptr;
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

  if (!CONFIG_CONFFILE)
    CONFIG_CONFFILE = strdup(CONFIG_FILE);

  load_config(g);

  g->port = CONFIG_DFLT_PORT;
  dir  = CONFIG_DFLT_DIR;

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
        mud_log("Loading player objects from secondary (ascii) files.");
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
  setup_log(CONFIG_LOGNAME, STDERR_FILENO);

  /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */
  mud_log("Using %s for configuration.", CONFIG_CONFFILE);
  g->xap_objs = 1;
  if (chdir(dir) < 0) {
    perror("SYSERR: Fatal error changing to data directory");
    exit(1);
  }
  mud_log("Using %s as data directory.", dir);

  if (g->scheck)
    boot_world(g);
  else {
    mud_log("Running game on port %d.", g->port);
    init_game(g, g->port);
  }

  mud_log("Clearing game world.");
  destroy_db(g);

  if (!g->scheck) {
    mud_log("Clearing other memory.");
    free_bufpool(g);                        /* comm.c */
    free_player_index(g);                   /* players.c */
    clear_free_list(g);                     /* mail.c */
    free_mail_index(g);                     /* mail.c */
    free_text_files(g);                     /* db.c */
    clear_boards(g);                        /* boards.c */
    free(g->cmd_sort_info);                   /* act.informative.c */
    free_command_list(g);                   /* act.informative.c */
    free_social_messages(g);                /* act.social.c */
    free_help_table(g);                     /* db.c */
    Free_Invalid_List(g);                   /* ban.c */
    free_ban_list(g);                       /* ban.c */
    free_strings(g, &g->config_info, OASIS_CFG); /* oasis_delete.c */
    free_disabled(g);                       /* interpreter.c */
    free_save_list(g);                      /* genolc.c */
  }

  if (g->last_act_message)
    free(g->last_act_message);

  /* probably should free the entire config here.. */
  free(CONFIG_CONFFILE);

  game_world_destroy(g);

  mud_log("Done.");

  return (0);
}
