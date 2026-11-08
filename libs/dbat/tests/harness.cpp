#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dbat/class.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fileop.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "harness.hpp"

#ifdef DBAT_COVERAGE
extern "C" void __gcov_dump(void);
#endif

constexpr room_vnum MAP_ROOM = 300;

static char temp_dir[PATH_MAX];
static const char *world_source = nullptr;

static void run(const char *format, const char *a, const char *b)
{
  char cmd[PATH_MAX * 2 + 64];

  snprintf(cmd, sizeof(cmd), format, a, b);
  if (system(cmd) != 0) {
    fprintf(stderr, "harness: '%s' failed\n", cmd);
    exit(2);
  }
}

/* The real one is 240KB of real rooms, so write one that's all the same room. */
static void write_surface_map(void)
{
  FILE *fl = fopen("surface.map", "w");

  if (!fl) {
    perror("harness: surface.map");
    exit(2);
  }
  for (int row = 0; row <= MAP_ROWS; row++) {
    for (int col = 0; col <= MAP_COLS; col++)
      fprintf(fl, "%d ", MAP_ROOM);
    fputc('\n', fl);
  }
  fclose(fl);
}

/* As init_game() does. */
static void read_surface_map(GameWorld *g)
{
  FILE *fl = fopen("surface.map", "r");
  room_vnum vnum;

  for (int row = 0; row <= MAP_ROWS; row++)
    for (int col = 0; col <= MAP_COLS; col++)
      g->mapnums[row][col] = fl && fscanf(fl, "%d", &vnum) == 1 && room_of(g, vnum) ? vnum : NOWHERE;
  if (fl)
    fclose(fl);
}

void th_use_world(const char *directory)
{
  world_source = directory;
}

static GameWorld *boot_here(time_t when, const char *log_mode);
static void (*before_boot)(void);

GameWorld *th_boot(void)
{
  return th_boot_at(0);
}

GameWorld *th_boot_at(time_t when)
{
  const char *tmp = getenv("TMPDIR");
  char data_dir[PATH_MAX + 8];

  snprintf(temp_dir, sizeof(temp_dir), "%s/dbat-test.XXXXXX", tmp ? tmp : "/tmp");
  if (!mkdtemp(temp_dir)) {
    perror("harness: mkdtemp");
    exit(2);
  }
  /* in a directory called data, because the game opens ../data/surface.map */
  snprintf(data_dir, sizeof(data_dir), "%s/data", temp_dir);
  if (mkdir(data_dir, 0755) < 0) {
    perror("harness: mkdir");
    exit(2);
  }
  if (world_source)
    /* these two are large, and the game doesn't read them */
    run("tar -C '%s' --exclude=./assets.sqlite3 --exclude=./state -cf - . | tar -C '%s' -xf -", world_source, data_dir);
  else
    run("cp -r '%s/.' '%s'", DBAT_TEST_FIXTURE, data_dir);
  if (chdir(data_dir) < 0) {
    perror("harness: chdir");
    exit(2);
  }
  if (!world_source)
    write_surface_map();
  if (before_boot) {
    before_boot();
    before_boot = nullptr;
  }
  return boot_here(when, "w");
}

/* The world booted last and not yet shut down, for th_abandon() */
static GameWorld *current_world;

/* Boots from the current directory. */
static GameWorld *boot_here(time_t when, const char *log_mode)
{
  GameWorld *g;

  logfile = fopen("test.log", log_mode);
  if (!logfile) {
    perror("harness: test.log");
    exit(2);
  }

  g = game_world_create();
  current_world  = g;
  g->pinned_time = when;
  CONFIG_CONFFILE = strdup(CONFIG_FILE);
  load_config(g);
  g->no_rent_check = 1;
  event_init(g);
  boot_db(g);
  topLoad(g);
  read_surface_map(g);
  return g;
}

static void free_game(GameWorld *g)
{
  close_all_sockets(g);
  game_world_free_contents(g);
  free(CONFIG_CONFFILE);
  game_world_destroy(g);
  if (g == current_world)
    current_world = nullptr;
  fclose(logfile);
  logfile = nullptr;
}

void th_shutdown(GameWorld *g)
{
  free_game(g);
  if (getenv("TH_KEEP")) { /* for looking at what a run left behind */
    printf("kept %s\n", temp_dir);
    return;
  }
  if (chdir("/") == 0)
    run("rm -rf '%s'%s", temp_dir, "");
}

void th_before_boot(void (*hook)(void))
{
  before_boot = hook;
}

void th_abandon(void)
{
  /* Half booted, or stopped mid-pulse, its contents can't be taken apart safely; deleting the world frees its tables
   * and rooms, which were most of what each refused input used to leak. */
  if (current_world) {
    game_world_destroy(current_world);
    current_world = nullptr;
  }
  if (logfile) {
    fclose(logfile);
    logfile = nullptr;
  }
  if (chdir("/") == 0)
    run("rm -rf '%s'%s", temp_dir, "");
}

void th_stop(GameWorld *g)
{
  free_game(g);
}

GameWorld *th_start_again(time_t when)
{
  return boot_here(when, "a");
}

struct char_data *th_player(GameWorld *g, const char *name, int admlevel)
{
  return th_player_as(g, name, admlevel, RACE_HUMAN, CLASS_ROSHI);
}

static struct descriptor_data *new_test_descriptor(GameWorld *g)
{
  struct descriptor_data *d;

  d             = new descriptor_data();
  d->descriptor = open("/dev/null", O_RDWR); /* for the few messages that skip the output buffer */
  d->output     = d->small_outbuf;
  d->bufspace   = SMALL_BUFSIZE - 1;
  d->login_time = game_now(g);
  d->has_prompt = 1;
  d->desc_num   = ++g->last_desc;
  strcpy(d->host, "test");
  d->user       = "Test";
  d->pass       = "Test";
  d->email      = "Test";
  d->tmp1       = "Empty";
  d->tmp2       = "Empty";
  d->tmp3       = "Empty";
  d->tmp4       = "Empty";
  d->tmp5       = "Empty";
  d->customfile = 1;
  g->descriptors.add(d->desc_num, d);
  return d;
}

struct char_data *th_existing_player(GameWorld *g, const char *name)
{
  struct descriptor_data *d = new_test_descriptor(g);
  struct char_data *ch;

  ch = new char_data();
  clear_char(ch);
  char_make_player_specials(ch);
  char_set_desc(ch, d);
  d->character = ch;
  if (load_char(g, name, ch) < 0) {
    close_socket(g, d);
    return nullptr;
  }
  enter_player_game(g, d);
  desc_set_connected(d, CON_PLAYING);
  th_take_output(g, ch);
  return ch;
}

struct char_data *th_player_as(GameWorld *g, const char *name, int admlevel, int race, int sensei)
{
  struct descriptor_data *d = new_test_descriptor(g);
  struct char_data *ch;

  ch = new char_data();
  clear_char(ch);
  char_make_player_specials(ch);
  char_set_desc(ch, d);
  d->character = ch;
  char_set_pc_name(ch, name);
  char_set_race(ch, race);
  char_set_class(ch, sensei);
  char_set_pfilepos(ch, create_entry(g, char_get_pc_name(ch)));
  init_char(g, ch);
  char_set_time_birth(ch, game_now(g) - birth_age(g, ch)); /* what the first load would do anyway */
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

struct descriptor_data *th_connect(GameWorld *g, int *peer)
{
  int ends[2];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, ends) < 0) {
    perror("harness: socketpair");
    exit(2);
  }
  fcntl(ends[0], F_SETFL, O_NONBLOCK);
  fcntl(ends[1], F_SETFL, O_NONBLOCK);
  game_add_connection(g, ends[0], "127.0.0.1");
  *peer = ends[1];
  /* a new connection goes on the front, and a refused one is closed */
  if (g->descriptors.newest() && g->descriptors.newest()->descriptor == ends[0])
    return g->descriptors.newest();
  close(ends[1]);
  return nullptr;
}

bool th_send(GameWorld *g, struct descriptor_data *d, int peer, const char *bytes, size_t length)
{
  if (write(peer, bytes, length) < 0)
    return true; /* the client's buffer is full: as far as the game knows, nothing was sent */
  if (game_read_input(g, d) < 0) {
    close_socket(g, d);
    return false;
  }
  if (d->character)
    char_set_wait(d->character, 0);
  game_process_commands(g);
  return true;
}

void th_type(GameWorld *g, struct char_data *ch, const char *line)
{
  th_type_at(g, char_get_desc(ch), line);
}

void th_type_at(GameWorld *g, struct descriptor_data *d, const char *line)
{
  char limited[MAX_INPUT_LENGTH];

  /* process_input() never lets a longer line through */
  snprintf(limited, sizeof(limited), "%s", line);
  write_to_q(limited, &d->input, 0);
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

void th_exit(int status)
{
  fflush(nullptr);
#ifdef DBAT_FUZZ
  if (status) /* an audit that failed, which a fuzzer only sees as a crash */
    abort();
#endif
#ifdef DBAT_COVERAGE
  __gcov_dump();
#endif
  _exit(status);
}

void th_pulses(GameWorld *g, int n)
{
  while (n-- > 0) {
    game_process_commands(g);
    game_finish_descriptors(g);
    game_heartbeat(g, ++g->pulse);
  }
}
