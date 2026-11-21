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
#include <cstring>

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
  if (t->out.overflowed())
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
   * state automatically because there will be no space left.)
   */
  output_buffer &out = t->out;
  if (size + out.text.size() + 1 > LARGE_BUFSIZE) {
    size               = LARGE_BUFSIZE - out.text.size() - 1;
    pending_text[size] = '\0';
    if (t->character)
      char_set_overf(t->character, true);
    g->buf_overflows++;
  }

  /*
   * If the text just barely fits, then it's switched to a large buffer instead (unless it already is the large
   * buffer: then it fills it, and we are in the overflow state).
   */
  if (!(out.space() > (size_t)size || (out.large && out.space() == (size_t)size))) {
    g->buf_switches++;
    out.large = true;
    /* the most connections that have had a large buffer at once (each kept the one it got from a pool) */
    g->buf_largecount = std::max(g->buf_largecount, ++g->buf_large_in_use);
  }
  out.text += pending_text;

  return (out.space());
}

void reset_output(GameWorld *g, struct output_buffer &out)
{
  if (out.large)
    g->buf_large_in_use--;
  out.large = false;
  out.text.clear();
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
  strcpy(osb, t->out.text.c_str()); /* strcpy: OK (the output:LARGE_BUFSIZE < osb:MAX_SOCK_BUF-2) */

  /* if we're in the overflow state, notify the user */
  if (t->out.overflowed())
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
    result        = write_to_descriptor(t->sock.handle(), i);
    if (result >= 2)
      result -= 2;
  } else
    result = write_to_descriptor(t->sock.handle(), osb);

  if (result < 0) { /* Oops, fatal error. Bye! */
    close_socket(g, t);
    return (-1);
  } else if (result == 0) /* Socket buffer full. Try later. */
    return (0);

  /* Handle snooping: prepend "% " and send to snooper. */
  if (t->snoop_by)
    write_to_output(g, t->snoop_by, "\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n{}\n^^^^^^^^^^^^^[Snoop]^^^^^^^^^^^^^\n", t->out.text);
  /* The common case: all saved output was handed off to the kernel buffer. */
  if ((size_t)result >= t->out.text.size()) {
    /* back to an empty small buffer (a large one goes back to the pool) */
    reset_output(g, t->out);

    /*
     * If the overflow message or prompt were partially written, save the rest, to go first next time. It fits:
     * the prompt is never near the small buffer's size. (It once stepped the buffer's end back instead of on,
     * so the next text went before the buffer, and the player never saw it.)
     */
    if ((unsigned int)result < strlen(osb))
      t->out.text = osb + result;
  } else {
    /* Not all data in buffer sent.  result < output buffersize. */
    t->out.text.erase(0, result);
  }

  return (result);
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
int write_to_descriptor(net::native_handle desc, const char *txt)
{
  size_t total = strlen(txt), write_total = 0;

  while (total > 0) {
    net::io_result sent = net::send_some(desc, {txt, total});

    if (sent.status == net::io_status::would_block) /* Temporary failure -- socket buffer full. */
      return (write_total);
    if (sent.status != net::io_status::done) {
      /* Fatal error.  Disconnect the player. */
      net::report_error("SYSERR: Write to socket");
      return (-1);
    }
    txt += sent.bytes;
    total -= sent.bytes;
    write_total += sent.bytes;
  }

  return (write_total);
}
