/*
 * Players made of dice: a few bots wander the fixture world typing commands, for many steps, checking
 * the world audit after each. Everything comes from the seed, so any failure can be rerun exactly.
 *
 * test_bot [first seed] [seeds] [steps]    runs each seed in its own process, and repeats the first
 *                                          one to check that a run really is repeatable
 * test_bot --seed N [steps]                runs one seed here, printing each command and the full report
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "harness.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/fight.h"
#include "dbat/handler.h"
#include "dbat/descriptor.h"
#include "dbat/interpreter.h"
#include "dbat/random.h"
#include "dbat/utils.h"
#include "dbat/world_audit.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

/* Without a sanitizer a crash leaves no report to recognise it by, so a known bug can't be told from a new one. */
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
constexpr bool HAS_SANITIZER = true;
#else
constexpr bool HAS_SANITIZER = false;
#endif

constexpr int NUM_BOTS       = 4;
constexpr int STEP_SECONDS   = 10;
constexpr int AUDIT_FAILED   = 3;
constexpr time_t START_TIME  = 1800000000;
constexpr int MAX_SIGNATURES = 100;

/*
 * Bugs nobody has fixed yet, by a function in their stack trace. They still get reported, but don't
 * fail the test, so a new failure stands out. Remove a line when its bug is fixed.
 */
static const char *const known_failures[] = {
    "in stop_follower",     /* destroy_db() frees a master before its followers */
    "in do_lag",            /* CAP() on the nameless character of someone at the login menu */
    "in add_guild",         /* copy_guild() frees strings in a freshly RECREATEd, uninitialized slot */
    "in delete_object",     /* the loop reads the next object from the one it just extracted */
    "in gain_exp",          /* level_exp() * 5 overflows int before it's widened */
    "in do_gen_comm",       /* com_msgs has no row for gemote */
    "in do_hayasa",         /* divides by skill / 2, which is 0 at skill 0 or 1 */
};

static const char *const never_typed[] = {
    "copyover", /* replaces the process */
    "shutdown", /* ends the game on purpose */
};

/* What players mostly do, so that the bots get somewhere instead of only trying the odd commands. */
static const char *const everyday[] = {
    "north", "east", "south", "west", "look", "get", "drop", "give", "put", "wear", "remove", "wield",
    "kill", "punch", "flee", "follow", "sit", "stand", "sleep", "wake", "rest", "say", "tell", "open",
    "close", "eat", "buy", "sell", "list", "inventory", "equipment", "score", "deposit", "withdraw",
    "balance", "group", "steal", "sacrifice", "junk", "save", "quit",
};

static const char *const words[] = {
    "dummy", "keeper", "stranger", "chest", "sword", "bread", "atm", "bot0", "bot1", "bot2", "bot3", "self",
    "all", "all.sword", "2.dummy", "north", "east", "south", "west", "1", "2", "100", "300", "301", "302",
    "303", "3034", "-1", "0", "on", "off", "xyzzy", "%s%n", "$n",
};

/* For the connections in an editor or a menu. */
static const char *const menu_choices[] = {"q", "y", "n", "1", "2", "3", "4", "5", "a", "b", "s", "0", "/s", "/q", "x", ""};

#define PICK(list) ((list)[bot_dice(sizeof(list) / sizeof(*(list)))])

/* The bots' own dice, so that choosing what to type takes nothing from the game's generator. */
static unsigned long bot_state;

static unsigned long bot_dice(unsigned long sides)
{
  bot_state = bot_state * 6364136223846793005UL + 1442695040888963407UL;
  return (bot_state >> 33) % sides;
}

static bool is_never_typed(const char *command)
{
  for (size_t i = 0; i < sizeof(never_typed) / sizeof(*never_typed); i++)
    if (!strcmp(command, never_typed[i]))
      return true;
  return false;
}

static void compose_line(GameWorld *g, const struct descriptor_data *d, char *line, size_t size)
{
  const char *command;
  size_t used;
  int num_commands = 0;

  if (desc_get_connected(d) != CON_PLAYING || d->str) {
    snprintf(line, size, "%s", PICK(menu_choices));
    return;
  }

  if (bot_dice(2)) {
    command = PICK(everyday);
  } else {
    while (g->complete_cmd_info[num_commands].command[0] != '\n')
      num_commands++;
    do {
      command = g->complete_cmd_info[bot_dice(num_commands - 1) + 1].command;
    } while (!strcmp(command, "RESERVED"));
  }
  if (is_never_typed(command))
    command = "look";

  used = snprintf(line, size, "%s", command);
  for (unsigned long n = bot_dice(4); n > 0 && used < size; n--)
    used += snprintf(line + used, size - used, " %s", PICK(words));
}

/* A low-level character fails most commands at the first check, so give them the means to get further. */
static void equip_bot(GameWorld *g, struct char_data *ch)
{
  char_set_class_level(ch, 1 + bot_dice(100));
  char_set_gold(ch, 100000);
  char_set_basepl(ch, 1000000);
  char_set_baseki(ch, 1000000);
  char_set_basest(ch, 1000000);
  for (int skill = 0; skill < SKILL_TABLE_SIZE; skill++)
    if (bot_dice(2))
      char_set_skills_base(ch, skill, 1 + bot_dice(100));
  restore(g, ch, false);
  for (obj_vnum vnum = 300; vnum <= 302; vnum++)
    obj_to_char(g, read_object(g, vnum, VIRTUAL), ch);
}

static void keep_bots_connected(GameWorld *g, int *next_name)
{
  int connected = 0;

  for (struct descriptor_data *d = g->descriptors; d; d = d->next)
    connected++;
  while (connected++ < NUM_BOTS) {
    char name[32];
    int number = (*next_name)++;

    snprintf(name, sizeof(name), "Bot%d", number);
    /* the first of each batch is an Implementor: they reach far more of the game */
    equip_bot(g, th_player(g, name, number % NUM_BOTS == 0 ? ADMLVL_IMPL : ADMLVL_NONE));
  }
}

/* Only a connection still at the login menus has a nameless character, or none in the game. */
static bool at_login_menu(const struct descriptor_data *d)
{
  return !d->character || !char_get_name(d->character);
}

static struct descriptor_data *pick_descriptor(GameWorld *g)
{
  int count = 0, chosen;
  struct descriptor_data *d;

  for (d = g->descriptors; d; d = d->next)
    count++;
  if (!count)
    return nullptr;
  chosen = bot_dice(count);
  for (d = g->descriptors; d; d = d->next)
    if (chosen-- == 0)
      return d;
  return nullptr;
}

/* A running fingerprint of all the output, to compare two runs of a seed. */
static unsigned long fingerprint = 14695981039346656037UL;

static void take_all_output(GameWorld *g)
{
  for (struct descriptor_data *d = g->descriptors; d; d = d->next) {
    for (const unsigned char *c = (const unsigned char *)d->output; *c; c++)
      fingerprint = (fingerprint ^ *c) * 1099511628211UL;
    th_take_output_at(g, d);
  }
}

static void print_audit_problem(void *, const char *problem)
{
  printf("AUDIT: %s\n", problem);
}

/* Plays one seed in this process and returns only if nothing went wrong. */
static void play(unsigned long seed, int steps, FILE *journal)
{
  GameWorld *g = th_boot_at(START_TIME);
  char line[256];
  int next_name = 0;

  bot_state = seed;
  circle_srandom(g, seed);

  for (int step = 0; step < steps; step++) {
    struct descriptor_data *d;

    alarm(STEP_SECONDS);
    keep_bots_connected(g, &next_name);
    d = pick_descriptor(g);
    if (d && at_login_menu(d)) { /* after quitting: this player has left, and another will take their place */
      fprintf(journal, "%d logging off\n", step);
      desc_set_connected(d, CON_CLOSE);
    } else if (d) {
      compose_line(g, d, line, sizeof(line));
      fprintf(journal, "%d %s: %s\n", step, char_get_name(d->character), line);
      fflush(journal);
      th_type_at(g, d, line);
    }

    th_pulses(g, bot_dice(10) + 1);
    g->pinned_time = START_TIME + (time_t)(g->pulse / PASSES_PER_SEC);
    take_all_output(g);

    if (world_audit(g, print_audit_problem, nullptr)) {
      fflush(nullptr);
      _exit(AUDIT_FAILED);
    }
  }
  alarm(0);
  printf("fingerprint %lx\n", fingerprint);
  th_shutdown(g);
}

struct signature {
  char text[300];
  unsigned long seed;
  int count;
  bool known;
};

static struct signature signatures[MAX_SIGNATURES];
static int num_signatures = 0;

static void record(const char *text, unsigned long seed, bool known)
{
  for (int i = 0; i < num_signatures; i++)
    if (!strcmp(signatures[i].text, text)) {
      signatures[i].count++;
      return;
    }
  if (num_signatures == MAX_SIGNATURES)
    return;
  snprintf(signatures[num_signatures].text, sizeof(signatures[num_signatures].text), "%s", text);
  signatures[num_signatures].seed    = seed;
  signatures[num_signatures].known   = known;
  signatures[num_signatures++].count = 1;
}

/* What the report says went wrong: the sanitizer's or audit's own words, and the game's first frame. */
static void read_report(const char *path, char *text, size_t size, unsigned long *fingerprint_found, bool *known)
{
  char line[1024], summary[220] = "", frame[60] = "";
  FILE *fl = fopen(path, "r");

  *known = false;

  while (fl && fgets(line, sizeof(line), fl)) {
    char *at;

    line[strcspn(line, "\n")] = '\0';
    for (size_t i = 0; i < sizeof(known_failures) / sizeof(*known_failures); i++)
      if (strstr(line, known_failures[i]))
        *known = true;
    sscanf(line, "fingerprint %lx", fingerprint_found);
    if (!*summary && !strncmp(line, "AUDIT: ", 7))
      snprintf(summary, sizeof(summary), "%s", line);
    if (!*summary && (at = strstr(line, "runtime error:")))
      snprintf(summary, sizeof(summary), "%s", at);
    if (!*summary && (at = strstr(line, "SUMMARY: ")))
      snprintf(summary, sizeof(summary), "%s", at + 9);
    /* the accessors tell us nothing: they're only where a bad pointer happened to get used */
    if (!*frame && strstr(line, "/libs/dbat/src/") && !strstr(line, "_utils.c") && (at = strstr(line, " in "))) {
      char *end = strchr(at + 4, ' ');

      if (end)
        *end = '\0';
      snprintf(frame, sizeof(frame), "%s", at + 4);
    }
  }
  if (fl)
    fclose(fl);
  snprintf(text, size, "%s [%s]", *summary ? summary : "no report", frame);
}

/* Runs a seed in a child process. Returns its fingerprint, or 0 after recording what went wrong. */
static unsigned long run_seed(const char *base, unsigned long seed, int steps, int attempt)
{
  char dir[512], report[600], journal[600], text[300];
  unsigned long found = 0;
  int status;
  bool known;
  pid_t pid;

  snprintf(dir, sizeof(dir), "%s/seed-%lu-%d", base, seed, attempt);
  mkdir(dir, 0755);
  snprintf(report, sizeof(report), "%s/report.txt", dir);
  snprintf(journal, sizeof(journal), "%s/journal.txt", dir);
  fflush(nullptr);

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(2);
  }
  if (pid == 0) {
    FILE *journal_file = fopen(journal, "w");

    setenv("TMPDIR", dir, 1);
    if (!journal_file || !freopen(report, "w", stdout) || dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
      _exit(2);
    play(seed, steps, journal_file);
    fflush(nullptr);
    _exit(0);
  }

  waitpid(pid, &status, 0);
  read_report(report, text, sizeof(text), &found, &known);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && found)
    return found;

  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
    snprintf(text, sizeof(text), "hang: one step ran for over %d seconds", STEP_SECONDS);
  else if (WIFSIGNALED(status) && !strncmp(text, "no report", 9))
    snprintf(text, sizeof(text), "killed by signal %d", WTERMSIG(status));
  record(text, seed, known);
  return 0;
}

int main(int argc, char **argv)
{
  unsigned long first = 1, repeat_seed = 0, repeat_fingerprint = 0;
  int seeds = 20, steps = 2000, failures = 0, finished = 0;
  const char *tmp = getenv("TMPDIR");
  char base[512], cleanup[600];

  /* The sanitizer only reads this at startup, and a stack trace is how known bugs are recognised. */
  if (!getenv("UBSAN_OPTIONS")) {
    setenv("UBSAN_OPTIONS", "print_stacktrace=1", 1);
    execv("/proc/self/exe", argv);
  }

  if (argc > 2 && !strcmp(argv[1], "--seed")) {
    FILE *journal = stdout;

    play(strtoul(argv[2], nullptr, 10), argc > 3 ? atoi(argv[3]) : steps, journal);
    return 0;
  }
  if (argc > 1)
    first = strtoul(argv[1], nullptr, 10);
  if (argc > 2)
    seeds = atoi(argv[2]);
  if (argc > 3)
    steps = atoi(argv[3]);

  snprintf(base, sizeof(base), "%s/dbat-bot.XXXXXX", tmp ? tmp : "/tmp");
  if (!mkdtemp(base)) {
    perror("mkdtemp");
    return 2;
  }

  for (int i = 0; i < seeds; i++) {
    unsigned long result = run_seed(base, first + i, steps, 0);

    if (result)
      finished++;
    if (result && !repeat_fingerprint) {
      repeat_seed        = first + i;
      repeat_fingerprint = result;
    }
  }
  /* a seed that failed proves nothing here, so this uses the first that finished */
  if (repeat_fingerprint && run_seed(base, repeat_seed, steps, 1) != repeat_fingerprint) {
    printf("FAIL: seed %lu didn't repeat exactly\n", repeat_seed);
    failures++;
  }

  for (int i = 0; i < num_signatures; i++) {
    printf("%4d %s %s\n      first seen with: test_bot --seed %lu %d\n", signatures[i].count,
           signatures[i].known ? "known" : "NEW  ", signatures[i].text, signatures[i].seed, steps);
    if (!signatures[i].known && (HAS_SANITIZER || strncmp(signatures[i].text, "killed by signal", 16)))
      failures++;
  }
  printf("%d seeds of %d steps, %d finished, %d distinct failures, %d of them new\n", seeds, steps, finished, num_signatures, failures);

  snprintf(cleanup, sizeof(cleanup), "rm -rf '%s'", base);
  if (system(cleanup) != 0)
    printf("couldn't remove %s\n", base);
  return failures != 0;
}
