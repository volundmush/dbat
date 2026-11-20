/*************************************************************************
*   File: output.cpp                                    Part of CircleMUD *
*  Usage: Queueing output and sending it                                 *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <unistd.h>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/screen.hpp"

#include "comm_internal.hpp"

static const char *const text_overflow = "**OVERFLOW**\r\n";

/* As big as a descriptor's whole output buffer, so one long text (help, lists) is never cut short here. */
static char pending_text[LARGE_BUFSIZE];

static size_t queue_output(GameWorld *g, struct descriptor_data *t, int size);

size_t write_to_output(GameWorld *g, struct descriptor_data *t, std::string_view text)
{
  /* if we're in the overflow state already, ignore this new output */
  if (t->bufspace == 0)
    return (0);

  size_t n = std::min(text.size(), sizeof(pending_text) - 1); /* as much as fits; queue_output() marks the rest lost */
  memcpy(pending_text, text.data(), n);
  pending_text[n] = '\0';
  return queue_output(g, t, (int)text.size());
}

/* Queues pending_text, which holds the first sizeof(pending_text) - 1 chars of a text size long */
static size_t queue_output(GameWorld *g, struct descriptor_data *t, int size)
{
  size_t wantsize = size;

  if (t->character)
    wantsize = size = proc_colors(g, pending_text, sizeof(pending_text), COLOR_ON(g, t->character), COLOR_CHOICES(t->character));
  /* If exceeding the size of the buffer, truncate it for the overflow message */
  if (size < 0 || wantsize >= sizeof(pending_text)) {
    size = sizeof(pending_text) - 1;
    strcpy(pending_text + size - strlen(text_overflow), text_overflow); /* strcpy: OK */
  }

  /*
   * If the text is too big to fit into even a large buffer, truncate
   * the new text to make it fit.  (This will switch to the overflow
   * state automatically because t->bufspace will end up 0.)
   */
  if (size + t->bufptr + 1 > LARGE_BUFSIZE) {
    size      = LARGE_BUFSIZE - t->bufptr - 1;
    pending_text[size] = '\0';
    if (t->character)
      char_set_overf(t->character, true);
    g->buf_overflows++;
  }

  /*
   * If we have enough space, just write to buffer and that's it! If the
   * text just barely fits, then it's switched to a large buffer instead
   * (unless it already is the large buffer: then it fills it, and we are in
   * the overflow state).
   */
  if (t->bufspace > size || (t->large_outbuf && t->bufspace == size)) {
    strcpy(t->output + t->bufptr, pending_text); /* strcpy: OK (size checked above) */
    t->bufspace -= size;
    t->bufptr += size;
    return (t->bufspace);
  }

  g->buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (g->bufpool != nullptr) {
    t->large_outbuf = g->bufpool;
    g->bufpool         = g->bufpool->next;
  } else { /* else create a new one */
    t->large_outbuf       = new txt_block();
    t->large_outbuf->text = str_alloc(LARGE_BUFSIZE);
    g->buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output); /* strcpy: OK (size checked previously) */
  t->output = t->large_outbuf->text;        /* make big buffer primary */
  strcat(t->output, pending_text);          /* strcat: OK (size checked) */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;

  return (t->bufspace);
}

void free_bufpool(GameWorld *g)
{
  struct txt_block *tmp;

  while (g->bufpool) {
    tmp = g->bufpool->next;
    if (g->bufpool->text)
      free(g->bufpool->text);
    delete g->bufpool;
    g->bufpool = tmp;
  }
}

/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 *
 * 32 int8_tGARBAGE_SPACE in MAX_SOCK_BUF used for:
 *	 2 bytes: prepended \r\n
 *	14 bytes: overflow message
 *	 2 bytes: extra \r\n for non-comapct
 *      14 bytes: unused
 */
int process_output(GameWorld *g, struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF], *osb = i + 2;
  int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n"); /* strcpy: OK (for 'MAX_SOCK_BUF >= 3') */

  /* now, append the 'real' output */
  strcpy(osb, t->output); /* strcpy: OK (t->output:LARGE_BUFSIZE < osb:MAX_SOCK_BUF-2) */

  /* if we're in the overflow state, notify the user */
  if (t->bufspace == 0)
    strcat(osb, "**OVERFLOW**\r\n"); /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add the extra CRLF if the person isn't in compact mode */
  if (desc_get_connected(t) == CON_PLAYING && t->character && !char_is_npc(t->character) && !char_prf_flagged(g, t->character, PRF_COMPACT))
    strcat(osb, "\r\n"); /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add a prompt */

  strcat(i, make_prompt(g, t).c_str()); /* strcpy: OK (i:MAX_SOCK_BUF reserves space) */
  if (desc_get_connected(t) == CON_PLAYING) {
    proc_colors(g, i, sizeof(i), COLOR_ON(g, t->character), COLOR_CHOICES(t->character));
  }

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (t->has_prompt) {
    t->has_prompt = false;
    result        = write_to_descriptor(t->descriptor, i);
    if (result >= 2)
      result -= 2;
  } else
    result = write_to_descriptor(t->descriptor, osb);

  if (result < 0) { /* Oops, fatal error. Bye! */
    close_socket(g, t);
    return (-1);
  } else if (result == 0) /* Socket buffer full. Try later. */
    return (0);

  /* Handle snooping: prepend "% " and send to snooper. */
  if (t->snoop_by)
    write_to_output(g, t->snoop_by, "\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n{}\n^^^^^^^^^^^^^[Snoop]^^^^^^^^^^^^^\n", t->output);
  /* The common case: all saved output was handed off to the kernel buffer. */
  if (result >= t->bufptr) {
    /*
     * if we were using a large buffer, put the large buffer on the buffer pool
     * and switch back to the small one
     */
    if (t->large_outbuf) {
      t->large_outbuf->next = g->bufpool;
      g->bufpool               = t->large_outbuf;
      t->large_outbuf       = nullptr;
      t->output             = t->small_outbuf;
    }
    /* reset total bufspace back to that of a small buffer */
    t->bufspace  = SMALL_BUFSIZE - 1;
    t->bufptr    = 0;
    *(t->output) = '\0';

    /*
     * If the overflow message or prompt were partially written, try to save
     * them. There will be enough space for them if this is true.  'result'
     * is effectively unsigned here anyway.
     */
    if ((unsigned int)result < strlen(osb)) {
      size_t savetextlen = strlen(osb + result);

      strcat(t->output, osb + result);
      t->bufptr -= savetextlen;
      t->bufspace += savetextlen;
    }

  } else {
    /* Not all data in buffer sent.  result < output buffersize. */

    memmove(t->output, t->output + result, strlen(t->output + result) + 1); /* overlapping, so not strcpy */
    t->bufptr -= result;
    t->bufspace += result;
  }

  return (result);
}

/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

/* perform_socket_write for all Non-Windows platforms */
static ssize_t perform_socket_write(int desc, const char *txt, size_t length)
{
  ssize_t result = 0;

  do
    result = write(desc, txt, length);
  while (result < 0 && errno == EINTR);

  if (result > 0) {
    /* Write was successful. */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    mud_log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

  if (errno == EAGAIN) /* EWOULDBLOCK is the same value on Linux */
    return (0);

  /* Looks like the error was fatal.  Too bad. */
  return (-1);
}

/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 * >=0  If all is well and good.
 *  -1  If an error was encountered, so that the player should be cut off.
 */
int write_to_descriptor(int desc, const char *txt)
{
  ssize_t bytes_written;
  size_t total = strlen(txt), write_total = 0;

  while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total);

    if (bytes_written < 0) {
      /* Fatal error.  Disconnect the player. */
      perror("SYSERR: Write to socket");
      return (-1);
    }
    if (bytes_written == 0) {
      /* Temporary failure -- socket buffer full. */
      return (write_total);
    }
    txt += bytes_written;
    total -= bytes_written;
    write_total += bytes_written;
  }

  return (write_total);
}
