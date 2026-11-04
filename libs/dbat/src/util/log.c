#include "dbat/utils.h"
#include "dbat/comm.h"

#include <execinfo.h>
#include <unistd.h>
#include "dbat/room_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

/* log a death trap hit */
void log_death_trap(GameWorld *g, struct char_data *ch)
{
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "%s hit death trap #%d (%s)", char_get_name(ch), rroom_get_vnum(g, char_get_in_room(ch)), rroom_get_name(g, char_get_in_room(ch)));
}

/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void mud_vlog(const char *format, va_list args)
{
  time_t ct    = time(0);
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

/* So mudlog() can use the same function. */
void mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  mud_vlog(format, args);
  va_end(args);
}

/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog(GameWorld *g, int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == nullptr)
    return; /* eh, oh well. */

  if (file) {
    va_start(args, str);
    mud_vlog(str, args);
    va_end(args);
  }

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  strcpy(buf, "[ "); /* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n"); /* strcat: OK */

  for (i = g->descriptors; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < level)
      continue;
    if (char_plr_flagged(i->character, PLR_WRITING))
      continue;
    if (type > (char_prf_flagged(g, i->character, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(g, i->character, "@g%s@n", buf);
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
  mud_log("SYSERR: Assertion failed at %s:%d!", who, line);
  log_backtrace();
}
