/*************************************************************************
*   File: input.cpp                                     Part of CircleMUD *
*  Usage: Reading input and queueing typed lines                         *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cstring>

#include "dbat/utils.hpp"
#include "dbat/comm.hpp"

#include "comm_internal.hpp"

static bool perform_subst(GameWorld *g, struct descriptor_data *t, char *orig, char *subst);

/*
 * NOTE: 'txt' must be at most MAX_INPUT_LENGTH big.
 */
void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  queue->lines.push_back({txt, aliased});
  if (!aliased)
    queue->typed++;
}

/*
 * NOTE: 'dest' must be at least MAX_INPUT_LENGTH big.
 */
bool get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  if (queue->lines.empty())
    return false;

  const txt_line &line = queue->lines.front();
  snprintf(dest, MAX_INPUT_LENGTH, "%s", line.text.c_str()); /* lines are queued at most this long */
  *aliased = line.aliased;
  if (!*aliased)
    queue->typed--;
  queue->lines.pop_front();

  return true;
}

/* Throw away the commands a descriptor has typed but not yet run. */
void flush_input_queue(struct descriptor_data *d)
{
  d->input.lines.clear();
  d->input.typed = 0;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 *
 * Ever wonder why 'tmp' had '+8' on it?  The crusty old code could write
 * MAX_INPUT_LENGTH+1 bytes to 'tmp' if there was a '$' as the final
 * character in the input buffer.  This would also cause 'space_left' to
 * drop to -1, which wasn't very happy in an unsigned variable.  Argh.
 * So to fix the above, 'tmp' lost the '+8' since it doesn't need it
 * and the code has been changed to reserve space by accepting one less
 * character. (Do you really need 256 characters on a line?)
 * -gg 1/21/2000
 */
int process_input(GameWorld *g, struct descriptor_data *t)
{
  int buf_length, failed_subst;
  size_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = nullptr;
  char tmp[MAX_INPUT_LENGTH];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      mud_log("WARNING: process_input: about to close connection: input overflow");
      return (-1);
    }

    net::io_result got = net::receive_some(t->sock.handle(), {read_point, space_left});

    if (got.status == net::io_status::would_block) /* Just blocking, no problems. */
      return (0);
    if (got.status != net::io_status::done) /* Error, disconnect them. */
      return (-1);
    bytes_read = got.bytes;

    /* check for compression response, if still expecting something */
    /* note: this will bork if the user is giving lots of input when he first connects */
    /* he shouldn't be doing this, and for the sake of efficiency, the read buffer isn't searched */
    /* (ie. it assumes that read_point[0] will be IAC, etc.) */

    *(read_point + bytes_read) = '\0'; /* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

    /*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
  } while (nl_pos == nullptr);

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != nullptr) {
    write_point = tmp;
    space_left  = MAX_INPUT_LENGTH - 1;

    /* The '> 1' reserves room for a '$ => $$' expansion. */
    for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b' || *ptr == 127) { /* handle backspacing or delete key */
        if (write_point > tmp) {
          if (*(--write_point) == '$') {
            write_point--;
            space_left += 2;
          } else
            space_left++;
        }
      } else if (isascii(*ptr) && isprint(*ptr)) {
        if ((*(write_point++) = *ptr) == '$') { /* copy one character */
          *(write_point++) = '$';               /* if it's a $, double it */
          space_left -= 2;
        } else
          space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      std::string buffer = str_format("Line too long.  Truncated to:\r\n{}\r\n", tmp);
      if (write_to_descriptor(t->sock.handle(), buffer.c_str()) < 0)
        return (-1);
    }
    if (t->snoop_by)
      write_to_output(g, t->snoop_by, "% {}\r\n", tmp);
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1))) /* Redo last command. */
      strcpy(tmp, t->last_input);     /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln  = (tmp + 1);
      int starting_pos = t->history_pos,
          cnt          = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
        if (!t->history[cnt].empty() && is_abbrev(commandln, t->history[cnt].c_str())) {
          strcpy(tmp, t->history[cnt].c_str()); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          strcpy(t->last_input, tmp);   /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          write_to_output(g, t, "{}\r\n", tmp);
          break;
        }
        if (cnt == 0) /* At top, loop to bottom. */
          cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(g, t, t->last_input, tmp)))
        strcpy(t->last_input, tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    } else {
      strcpy(t->last_input, tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
      t->history[t->history_pos] = tmp;         /* Replace the old line. */
      if (++t->history_pos >= HISTORY_SIZE)     /* Wrap to top. */
        t->history_pos = 0;
    }
    /* the '--' command flushes the queue - Jamdog - 9th May 2007 */
    if ((*tmp == '-') && (*(tmp + 1) == '-') && !(*(tmp + 2))) {
      write_to_output(g, t, "All queued commands cancelled.\r\n");
      flush_input_queue(t); /* only the commands: pending output stays */
      failed_subst = 1;     /* and don't queue the -- itself */
    }
    if (!failed_subst) {
      /* commands run at most one per pulse; a client sending far faster is flooding */
      if (t->input.typed >= MAX_QUEUED_INPUT) {
        write_to_descriptor(t->sock.handle(), "\r\nToo many commands waiting to run; disconnecting.\r\n");
        mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Input flood from [{}]: more than {:d} commands queued; disconnecting.", t->host, MAX_QUEUED_INPUT);
        return (-1);
      }
      write_to_q(tmp, &t->input, 0);
    }

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = nullptr; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}

/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
static bool perform_subst(GameWorld *g, struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    write_to_output(g, t, "Invalid substitution.\r\n");
    return true;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    write_to_output(g, t, "Invalid substitution.\r\n");
    return true;
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, strpos - orig); /* strncpy: OK (newsub:MAX_INPUT_LENGTH+5 > orig:MAX_INPUT_LENGTH) */
  newsub[strpos - orig] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, MAX_INPUT_LENGTH - strlen(newsub) - 1); /* strncpy: OK */

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), MAX_INPUT_LENGTH - strlen(newsub) - 1); /* strncpy: OK */

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */

  return false;
}
