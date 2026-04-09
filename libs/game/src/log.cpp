#include "dbat/game/log.h"


/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name);
}


/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
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
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
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
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_ADMLEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}


void core_dump_real(const char *who, int line)
{
  /* log("SYSERR: Assertion failed at %s:%d!", who, line); */

}
