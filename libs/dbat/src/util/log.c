#include "dbat/utils.h"
#include "dbat/comm.h"

#include <execinfo.h>
#include <unistd.h>

/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(LOG_BRF, ADMLVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)), world[char_get_in_room(ch)].name);
}

/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void mud_vlog(const char *format, va_list args)
{
  time_t ct    = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
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
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
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

  for (i = descriptor_list; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING || char_is_npc(i->character)) /* switch */
      continue;
    if (char_get_admlevel(i->character) < level)
      continue;
    if (char_plr_flagged(i->character, PLR_WRITING))
      continue;
    if (type > (char_prf_flagged(i->character, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}

/* Write the current call stack to the log. Uses only the file descriptor, so it is also usable from a
 * signal handler. Function names need the executable to be linked with -rdynamic. */
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
