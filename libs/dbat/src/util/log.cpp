#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/net/platform.hpp"

FILE *logfile = nullptr; /* Where to send the log messages. */

static bool open_logfile(const std::string &filename, FILE *stderr_fp, std::string_view mode);

/* log a death trap hit */
void log_death_trap(GameWorld *g, struct char_data *ch)
{
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "{} hit death trap #{:d} ({})", char_get_name(ch), char_get_in_room(ch), room_get_name(g, char_get_in_room(ch)));
}

/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void mud_log(std::string_view text)
{
  time_t ct = time(nullptr);

  if (logfile == nullptr) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  /* "Mon Jan  2 15:04:05 2006\n": the month, day and time */
  std::string_view stamp = asctime(localtime(&ct));
  file_print(logfile, "{:<15.15} :: ", stamp.substr(4));
  fwrite(text.data(), 1, text.size(), logfile);
  fputc('\n', logfile);
  fflush(logfile);
}

/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog(GameWorld *g, int type, int level, int file, std::string_view text)
{
  if (file)
    mud_log(text);

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < level)
      continue;
    if (char_plr_flagged(i->character, PLR_WRITING))
      continue;
    if (type > (char_prf_flagged(g, i->character, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(g, i->character, "@g[ {} ]\r\n@n", text);
  }
}

int log_descriptor() { return fileno(logfile ? logfile : stderr); }

/* Write the current call stack to the log. Uses only the file descriptor, so it is also usable from a
 * signal handler. Only exported functions get names (the executable is linked with -rdynamic); static ones show
 * as offsets, which tools/symbolize.sh resolves against the same binary. */
void log_backtrace()
{
  if (logfile)
    fflush(logfile);
  net::process::write_backtrace(log_descriptor());
}

void core_dump(std::source_location where)
{
  mud_log("SYSERR: Assertion failed at {}:{:d}!", where.file_name(), where.line());
  log_backtrace();
}

/* Prefer the file over the descriptor. */
void setup_log(std::string_view filename, bool keep_old_log)
{
  std::string_view mode = keep_old_log ? "a" : "w"; /* a copyover carries on the same log */
  FILE *s_fp       = stderr;

  if (filename.empty()) {
    /* No filename, set us up with the descriptor we just opened. */
    logfile = s_fp;
    puts("Using file descriptor for logging.");
    return;
  }

  /* We honor the default filename first. */
  if (open_logfile(std::string(filename), s_fp, mode))
    return;

  /* Well, that failed but we want it logged to a file so try a default. */
  if (open_logfile("log/syslog", s_fp, mode))
    return;

  /* Ok, one last shot at a file. */
  if (open_logfile("syslog", s_fp, mode))
    return;

  /* Erp, that didn't work either, just die. */
  puts("SYSERR: Couldn't open anything to log to, giving up.");
  exit(1);
}

static bool open_logfile(const std::string &filename, FILE *stderr_fp, std::string_view mode)
{
  if (stderr_fp) /* freopen() the descriptor. */
    logfile = freopen(filename.c_str(), std::string(mode).c_str(), stderr_fp);
  else
    logfile = fopen(filename.c_str(), std::string(mode).c_str());

  if (logfile) {
    file_print(stdout, "Using log file '{}'{}.\n",
               filename, stderr_fp ? " with redirection" : "");
    return (true);
  }

  file_print(stdout, "SYSERR: Error opening file '{}': {}\n", filename, strerror(errno));
  return (false);
}
