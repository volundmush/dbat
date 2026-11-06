/*
 * Runs every command, with awkward arguments, at several admin levels and in several states.
 * Each case forks from one booted game, so one crash can't hide another or spoil later cases.
 * test_commands [command]   runs one command only, and shows the sanitizer's whole report.
 */
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "harness.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/fight.h"
#include "dbat/handler.h"
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/world_audit.h"
#include "dbat/character_utils.h"
#include "dbat/object_utils.h"
#include "dbat/game_world.h"

static const char *const skipped[] = {
    "copyover", /* replaces the process */
    "shutdown", /* ends the game on purpose */
};

/*
 * Commands with a failure nobody has fixed yet. They still run and still get reported, but don't
 * fail the test, so a new failure stands out. Remove a command from here when it's fixed.
 */
static const char *const known_failures[] = {
    "", /* none at the moment */
};

static const char *const arguments[] = {
    "",
    "xyzzy",
    "dummy",
    "victim",
    "chest",
    "sword",
    "self",
    "all",
    "-1",
    "0",
    "3",
    "300",
    "3034",
    "99999999999999999999",
    "dummy xyzzy -1",
    "victim 1 2 3",
    "sword chest",
    "%s%s%n$n$N@x",
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
};

enum state { STANDING, SLEEPING, FIGHTING, DEAD, NUM_STATES };
static const char *const state_names[NUM_STATES] = {"standing", "sleeping", "fighting", "dead"};

static const int levels[] = {ADMLVL_NONE, ADMLVL_GOD, ADMLVL_IMPL};

constexpr int CASE_SECONDS   = 10;
constexpr int MAX_SIGNATURES = 500;
constexpr int AUDIT_FAILED   = 3;

struct signature {
  char text[300];
  char example[300];
  int count;
  bool known;
};

static struct signature signatures[MAX_SIGNATURES];
static int num_signatures = 0;
static int cases          = 0;
static bool verbose       = false;

static bool is_known_failure(const char *command)
{
  for (size_t i = 0; i < sizeof(known_failures) / sizeof(*known_failures); i++)
    if (!strcmp(command, known_failures[i]))
      return true;
  return false;
}

static void record(const char *text, const char *example, bool known)
{
  for (int i = 0; i < num_signatures; i++)
    if (!strcmp(signatures[i].text, text)) {
      signatures[i].count++;
      return;
    }
  if (num_signatures == MAX_SIGNATURES)
    return;
  snprintf(signatures[num_signatures].text, sizeof(signatures[num_signatures].text), "%s", text);
  snprintf(signatures[num_signatures].example, sizeof(signatures[num_signatures].example), "%s", example);
  signatures[num_signatures].known = known;
  signatures[num_signatures++].count = 1;
}

/* The sanitizer's summary, followed by the first frame of the game's own code. */
static void describe_report(const char *report_path, char *out, size_t size)
{
  char line[1024], summary[200] = "", frame[120] = "";
  FILE *fl = fopen(report_path, "r");

  if (!fl) {
    snprintf(out, size, "no report");
    return;
  }
  while (fgets(line, sizeof(line), fl)) {
    char *at;

    if (verbose)
      fputs(line, stdout);
    line[strcspn(line, "\n")] = '\0';
    if (!*summary && !strncmp(line, "AUDIT: ", 7))
      snprintf(summary, sizeof(summary), "%s", line);
    if (!*summary && (at = strstr(line, "runtime error:")))
      snprintf(summary, sizeof(summary), "%s", at);
    if ((at = strstr(line, "SUMMARY: ")) && !*summary)
      snprintf(summary, sizeof(summary), "%s", at + 9);
    if (!*frame && (at = strstr(line, "/libs/dbat/src/")) && strstr(line, " in ")) {
      char *in = strstr(line, " in ");
      char *end = strchr(in + 4, ' ');

      if (end)
        *end = '\0';
      snprintf(frame, sizeof(frame), "%s", in + 4);
    }
  }
  fclose(fl);
  snprintf(out, size, "%s [%s]", *summary ? summary : "exited with no sanitizer report", frame);
}

static void audit_problem(void *, const char *problem)
{
  printf("AUDIT: %s\n", problem);
}

static void play_case(GameWorld *g, struct char_data *ch, struct char_data *victim, enum state state, const char *line)
{
  struct char_data *dummy = get_char_room_vis(g, ch, writable{"dummy"}, nullptr);

  switch (state) {
    case STANDING:
    case NUM_STATES:
      break;
    case SLEEPING:
      char_set_position(ch, POS_SLEEPING);
      break;
    case FIGHTING:
      if (dummy) {
        set_fighting(g, ch, dummy);
        set_fighting(g, dummy, ch);
      }
      break;
    case DEAD:
      char_set_position(ch, POS_DEAD);
      break;
  }
  (void)victim;
  th_type(g, ch, line);
  th_pulses(g, 3);
  if (world_audit(g, audit_problem, nullptr)) {
    th_exit(AUDIT_FAILED);
  }
}

static void run_case(GameWorld *g, struct char_data *ch, struct char_data *victim, enum state state,
                     const char *command, const char *argument)
{
  char line[512], example[512], report[64];
  int status;
  pid_t pid;

  snprintf(line, sizeof(line), "%s %s", command, argument);
  snprintf(example, sizeof(example), "'%s' level %d %s", line, char_get_admlevel(ch), state_names[state]);
  snprintf(report, sizeof(report), "case-%d.txt", (int)getpid());
  cases++;
  fflush(nullptr);

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(2);
  }
  if (pid == 0) {
    int fd = open(report, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    alarm(CASE_SECONDS);
    play_case(g, ch, victim, state, line);
    th_exit(0);
  }

  waitpid(pid, &status, 0);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    return;

  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
    char text[64];

    snprintf(text, sizeof(text), "hang: still running after %d seconds", CASE_SECONDS);
    record(text, example, is_known_failure(command));
  } else {
    char text[300];

    describe_report(report, text, sizeof(text));
    if (WIFSIGNALED(status) && strstr(text, "no sanitizer report"))
      snprintf(text, sizeof(text), "killed by signal %d", WTERMSIG(status));
    record(text, example, is_known_failure(command));
  }
}

static bool is_skipped(const char *command)
{
  for (size_t i = 0; i < sizeof(skipped) / sizeof(*skipped); i++)
    if (!strcmp(command, skipped[i]))
      return true;
  return false;
}

static void place(GameWorld *g, struct char_data *ch, room_vnum vnum)
{
  char_from_room(g, ch);
  char_to_room(g, ch, real_room(g, vnum));
}

static int by_count(const void *a, const void *b)
{
  return ((const struct signature *)b)->count - ((const struct signature *)a)->count;
}

int main(int argc, char **argv)
{
  GameWorld *g = th_boot();
  struct char_data *victim = th_player(g, "Victim", ADMLVL_NONE);
  struct char_data *players[sizeof(levels) / sizeof(*levels)];
  const char *only = argc > 1 ? argv[1] : nullptr;
  const size_t num_players = sizeof(players) / sizeof(*players);
  int new_failures = 0;

  verbose = only != nullptr;
  place(g, victim, 301);
  for (size_t i = 0; i < num_players; i++) {
    char name[32];

    snprintf(name, sizeof(name), "Tester%c", (char)('A' + i));
    players[i] = th_player(g, name, levels[i]);
    place(g, players[i], 301);
    obj_to_char(g, read_object(g, 301, VIRTUAL), players[i]);
    obj_to_char(g, read_object(g, 302, VIRTUAL), players[i]);
    obj_to_char(g, read_object(g, 300, VIRTUAL), players[i]);
  }

  for (int c = 1; g->complete_cmd_info[c].command[0] != '\n'; c++) {
    const char *command = g->complete_cmd_info[c].command;

    if (is_skipped(command) || !strcmp(command, "RESERVED") || (only && strcmp(only, command)))
      continue;
    for (size_t p = 0; p < num_players; p++)
      for (int s = 0; s < NUM_STATES; s++)
        for (size_t a = 0; a < sizeof(arguments) / sizeof(*arguments); a++)
          run_case(g, players[p], victim, (enum state)s, command, arguments[a]);
  }

  qsort(signatures, num_signatures, sizeof(*signatures), by_count);
  for (int i = 0; i < num_signatures; i++) {
    printf("%5d %s %s\n       e.g. %s\n", signatures[i].count, signatures[i].known ? "known" : "NEW  ",
           signatures[i].text, signatures[i].example);
    if (!signatures[i].known)
      new_failures++;
  }
  printf("%d cases, %d distinct failures, %d of them new\n", cases, num_signatures, new_failures);
  fflush(stdout);
  th_shutdown(g);
  return new_failures != 0;
}
