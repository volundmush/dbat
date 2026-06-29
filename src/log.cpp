
#include "comm.h"

#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "consts/admlevel.h"
#include "consts/constates.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "flags.h"
#include "log.h"
#include "room_api.h"
#include "object_api.h"
#include "config_db.h"
#include "db.h"

#include <sys/stat.h>

#include <cstring>
#include <ctime>

/* log a death trap hit */
void log_death_trap(struct char_data *ch) {
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch),
         char_room_vnum_get(ch), room_name_get(char_room_get(ch)));
}

/* New variable argument mud_log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void mud_vlog(const char *format, va_list args) {
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using mud_log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: mud_log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}

/* So mudlog() can use the same function. */
void mud_log(const char *format, ...) {
  va_list args;

  va_start(args, format);
  mud_vlog(format, args);
  va_end(args);
}

/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog(int type, int level, int file, const char *str, ...) {
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
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_ADMLEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
                   (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}

void core_dump_real(const char *who, int line) {
  /* mud_log("SYSERR: Assertion failed at %s:%d!", who, line); */
}


void log_imm_action(char *messg, ...) {

  FILE *fl;
  const char *filename;
  struct stat fbuf;

  filename = REQUEST_FILE;

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
     *  This is from do_gen_write() and indicates that it cannot call the
     *  stat() system call on the file required.  The error string at the
     *  end of the line should explain what the problem is.
     */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: log_imm_action");
    /*  SYSERR_DESC:
     *  This is from do_gen_write(), and will be output if the file in
     *  question cannot be opened for appending to.  The error string
     *  at the end of the line should explain what the problem is.
     */

    return;
  }
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  va_list args;

  va_start(args, messg);
  time_s[strlen(time_s) - 1] = '\0';

  fprintf(fl, "%-15.15s :: ", time_s + 4);
  vfprintf(fl, messg, args);
  fprintf(fl, "\n");
  va_end(args);

  fclose(fl);
}



void log_custom(struct descriptor_data *d, struct obj_data *obj) {
  FILE *fl;
  const char *filename;
  struct stat fbuf;

  filename = CUSTOM_FILE;

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
     *  This is from do_gen_write() and indicates that it cannot call the
     *  stat() system call on the file required.  The error string at the
     *  end of the line should explain what the problem is.
     */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: log_custom");
    /*  SYSERR_DESC:
     *  This is from do_gen_write(), and will be output if the file in
     *  question cannot be opened for appending to.  The error string
     *  at the end of the line should explain what the problem is.
     */

    return;
  }

  fprintf(fl, "@D[@cUser@W: @R%-20s @cName@W: @C%-20s @cCustom@W: @Y%s@D]\n",
          GET_USER(d->character), GET_NAME(d->character),
          obj_short_description_get(obj));
  fclose(fl);
}