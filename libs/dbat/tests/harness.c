#include "harness.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dbat/class.h"
#include "dbat/comm.h"
#include "dbat/config.h"
#include "dbat/db.h"
#include "dbat/descriptor.h"
#include "dbat/dg_event.h"
#include "dbat/dg_scripts.h"
#include "dbat/fileop.h"
#include "dbat/interpreter.h"
#include "dbat/players.h"
#include "dbat/utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static char temp_dir[PATH_MAX];

static void run(const char *format, const char *a, const char *b)
{
  char cmd[PATH_MAX * 2 + 64];

  snprintf(cmd, sizeof(cmd), format, a, b);
  if (system(cmd) != 0) {
    fprintf(stderr, "harness: '%s' failed\n", cmd);
    exit(2);
  }
}

GameWorld *th_boot(void)
{
  return th_boot_at(0);
}

GameWorld *th_boot_at(time_t when)
{
  const char *tmp = getenv("TMPDIR");
  GameWorld *g;

  snprintf(temp_dir, sizeof(temp_dir), "%s/dbat-test.XXXXXX", tmp ? tmp : "/tmp");
  if (!mkdtemp(temp_dir)) {
    perror("harness: mkdtemp");
    exit(2);
  }
  run("cp -r '%s/.' '%s'", DBAT_TEST_FIXTURE, temp_dir);
  if (chdir(temp_dir) < 0) {
    perror("harness: chdir");
    exit(2);
  }
  logfile = fopen("test.log", "w");
  if (!logfile) {
    perror("harness: test.log");
    exit(2);
  }

  g = game_world_create();
  g->pinned_time = when;
  CONFIG_CONFFILE = strdup(CONFIG_FILE);
  load_config(g);
  g->no_rent_check = 1;
  event_init(g);
  init_lookup_table(g);
  boot_db(g);
  topLoad(g);
  for (int row = 0; row <= MAP_ROWS; row++)
    for (int col = 0; col <= MAP_COLS; col++)
      g->mapnums[row][col] = NOWHERE;
  return g;
}

void th_shutdown(GameWorld *g)
{
  while (g->descriptors)
    close_socket(g, g->descriptors);
  game_world_free_contents(g);
  free(CONFIG_CONFFILE);
  game_world_destroy(g);
  fclose(logfile);
  logfile = nullptr;
  if (chdir("/") == 0)
    run("rm -rf '%s'%s", temp_dir, "");
}

struct char_data *th_player(GameWorld *g, const char *name, int admlevel)
{
  struct descriptor_data *d;
  struct char_data *ch;

  CREATE(d, struct descriptor_data, 1);
  d->descriptor = open("/dev/null", O_RDWR); /* for the few messages that skip the output buffer */
  d->output     = d->small_outbuf;
  d->bufspace   = SMALL_BUFSIZE - 1;
  d->login_time = game_now(g);
  d->has_prompt = 1;
  d->desc_num   = ++g->last_desc;
  CREATE(d->history, char *, HISTORY_SIZE);
  strcpy(d->host, "test");
  d->user  = strdup("Test");
  d->pass  = strdup("Test");
  d->email = strdup("Test");
  d->tmp1  = strdup("Empty");
  d->tmp2  = strdup("Empty");
  d->tmp3  = strdup("Empty");
  d->tmp4  = strdup("Empty");
  d->tmp5  = strdup("Empty");
  d->customfile = 1;
  d->next  = g->descriptors;
  g->descriptors = d;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  CREATE(*char_player_specials_slot(ch), struct player_special_data, 1);
  char_set_desc(ch, d);
  d->character = ch;
  char_set_pc_name(ch, strdup(name));
  char_set_pfilepos(ch, create_entry(g, char_get_pc_name(ch)));
  init_char(g, ch);
  save_char(g, ch);
  save_player_index(g);

  enter_player_game(g, d);
  desc_set_connected(d, CON_PLAYING);
  if (char_get_level(ch) == 0)
    do_start(g, ch);
  admin_set(g, ch, admlevel);
  th_take_output(g, ch);
  return ch;
}

void th_type(GameWorld *g, struct char_data *ch, const char *line)
{
  th_type_at(g, char_get_desc(ch), line);
}

void th_type_at(GameWorld *g, struct descriptor_data *d, const char *line)
{
  write_to_q(line, &d->input, 0);
  if (d->character)
    char_set_wait(d->character, 0);
  game_process_commands(g);
}

const char *th_output(struct char_data *ch)
{
  return char_get_desc(ch)->output;
}

void th_take_output(GameWorld *g, struct char_data *ch)
{
  th_take_output_at(g, char_get_desc(ch));
}

void th_take_output_at(GameWorld *g, struct descriptor_data *d)
{
  if (d->large_outbuf) {
    d->large_outbuf->next = g->bufpool;
    g->bufpool            = d->large_outbuf;
    d->large_outbuf       = nullptr;
  }
  d->output   = d->small_outbuf;
  d->bufspace = SMALL_BUFSIZE - 1;
  d->bufptr   = 0;
  *d->output  = '\0';
}

void th_pulses(GameWorld *g, int n)
{
  while (n-- > 0) {
    game_process_commands(g);
    game_finish_descriptors(g);
    game_heartbeat(g, ++g->pulse);
  }
}
