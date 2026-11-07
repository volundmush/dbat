/*
 * Plays a journal, in the bot's own format: who connects, what they type, how time passes. It's the
 * bot with a fuzzer steering it, which can find its way down a path a random player wouldn't.
 */
#include <unistd.h>

#include "fuzz.hpp"

/* test_bot.cpp is compiled in here with its main() renamed and its alarm() switched off, so these wrap it */
#define alarm(seconds) 0 /* libFuzzer has its own, using the same signal */
#define main bot_main
#include "../test_bot.cpp"
#undef main

constexpr int MAX_ACTIONS = 300; /* so that one input can't take the whole of a timeout */
constexpr int MAX_PULSES = 1000;
constexpr long MAX_CLOCK_JUMP = 10L * 365 * 24 * 3600;

/*
 * Whether the game itself could ever be asked for this. Without the check, the fuzzer spends its time
 * on a spawn with race 52 or admin level 9618, which only the harness can do, and on 900 million pulses.
 */
static bool possible(const char *line)
{
  char label[16];
  int level, race, sensei;
  unsigned long kit;

  if (*line == '@') { /* the same way do_action reads it */
    char *rest;

    strtoul(line + 1, &rest, 10);
    line = rest + (*rest == ' ');
  }
  if (!strncmp(line, "pulse ", 6))
    return atoi(line + 6) >= 0 && atoi(line + 6) <= MAX_PULSES;
  if (!strncmp(line, "clock ", 6))
    return labs(strtol(line + 6, nullptr, 10)) <= MAX_CLOCK_JUMP;
  if (!strncmp(line, "spawn ", 6))
    return sscanf(line + 6, "%15s %d %lu %d %d", label, &level, &kit, &race, &sensei) == 5 && level >= 0 && level <= ADMLVL_IMPL &&
           race >= 0 && race < NUM_RACES && sensei >= 0 && sensei < NUM_CLASSES;
  return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *, size_t);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  size_t used = 0;
  int actions = 0;
  GameWorld *g;

  fuzz_can_refuse = true;
  if (setjmp(fuzz_refused)) {
    th_abandon();
    fuzz_can_refuse = false;
    return 0;
  }
  g = th_boot_at(START_TIME);
  num_connections = 0;
  clock_jumps     = 0;
  while (used < size && actions < MAX_ACTIONS) {
    size_t length = 0;

    while (used < size && data[used] != '\n' && length < MAX_LINE - 1) {
      if (data[used])
        line[length++] = (char)data[used];
      used++;
    }
    used++;
    line[length] = '\0';
    if (*line && *line != '#' && strcmp(line, "restart") && possible(line)) {
      do_action(g, line);
      actions++;
    }
  }
  th_shutdown(g);
  fuzz_can_refuse = false;
  return 0;
}
