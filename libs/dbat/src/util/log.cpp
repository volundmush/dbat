#include <execinfo.h>
#include <unistd.h>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* log a death trap hit */
void log_death_trap(GameWorld *g, struct char_data *ch)
{
  mudlog_printf(g, LOG_BRF, ADMLVL_IMMORT, true, "%s hit death trap #%d (%s)", char_get_name(ch), char_get_in_room(ch), room_get_name(g, char_get_in_room(ch)));
}

/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void mud_vlog_printf(const char *format, va_list args)
{
  time_t ct    = time(nullptr);
  char *time_s = asctime(localtime(&ct));

  if (logfile == nullptr) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == nullptr)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}

/* So mudlog_printf() can use the same function. */
void mud_log_printf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  mud_vlog_printf(format, args);
  va_end(args);
}

/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog_printf(GameWorld *g, int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  va_list args;

  if (str == nullptr)
    return; /* eh, oh well. */

  if (file) {
    va_start(args, str);
    mud_vlog_printf(str, args);
    va_end(args);
  }

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  strcpy(buf, "[ "); /* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n"); /* strcat: OK */

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < level)
      continue;
    if (char_plr_flagged(i->character, PLR_WRITING))
      continue;
    if (type > (char_prf_flagged(g, i->character, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char_printf(g, i->character, "@g%s@n", buf);
  }
}

/* Write the current call stack to the log. Uses only the file descriptor, so it is also usable from a
 * signal handler. Only exported functions get names (the executable is linked with -rdynamic); static ones show
 * as offsets, which tools/symbolize.sh resolves against the same binary. */
void log_backtrace(void)
{
  void *frames[64];
  int n  = backtrace(frames, 64);
  int fd = logfile ? fileno(logfile) : STDERR_FILENO;

  if (logfile)
    fflush(logfile);
  backtrace_symbols_fd(frames, n, fd);
}

/* core_dump(): an "impossible" state was reached. Log where, with a backtrace, and keep running. */
void core_dump_real(const char *who, int line)
{
  mud_log_printf("SYSERR: Assertion failed at %s:%d!", who, line);
  log_backtrace();
}

void mud_log(std::string_view text)
{
  (mud_log_printf)("%.*s", (int)text.size(), text.data()); /* (name) so tools/printf_to_format.py leaves it be */
}

void mudlog(GameWorld *g, int type, int level, int file, std::string_view text)
{
  (mudlog_printf)(g, type, level, file, "%.*s", (int)text.size(), text.data()); /* (name) so tools/printf_to_format.py leaves it be */
}
