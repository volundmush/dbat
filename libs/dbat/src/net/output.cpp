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
#include <string>
#include <string_view>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/screen.hpp"

#include "comm_internal.hpp"

constexpr std::string_view text_overflow = "**OVERFLOW**\r\n";

size_t write_to_output(GameWorld *g, struct descriptor_data *t, std::string_view text)
{
  /* if we're in the overflow state already, ignore this new output */
  if (t->out.overflowed())
    return (0);

  /* as much as a whole output buffer holds, so one long text (help, lists) is never cut short here; the rest is
     marked lost below */
  std::string pending(text.substr(0, LARGE_BUFSIZE - 1));
  size_t wantsize = text.size();

  if (t->character) {
    colourized coloured = colourize(g, pending, COLOR_ON(g, t->character), COLOR_CHOICES(t->character), LARGE_BUFSIZE);
    pending             = std::move(coloured.text);
    wantsize            = coloured.wanted;
  }
  /* If exceeding the size of the buffer, truncate it for the overflow message */
  if (wantsize >= LARGE_BUFSIZE) {
    pending.resize(std::min(pending.size(), LARGE_BUFSIZE - 1 - text_overflow.size()));
    pending += text_overflow;
  }
  size_t size = pending.size();

  /*
   * If the text is too big to fit into even a large buffer, truncate
   * the new text to make it fit.  (This will switch to the overflow
   * state automatically because there will be no space left.)
   */
  output_buffer &out = t->out;
  if (size + out.text.size() + 1 > LARGE_BUFSIZE) {
    size = LARGE_BUFSIZE - out.text.size() - 1;
    pending.resize(size);
    if (t->character)
      char_set_overf(t->character, true);
    g->buf_overflows++;
  }

  /*
   * If the text just barely fits, then it's switched to a large buffer instead (unless it already is the large
   * buffer: then it fills it, and we are in the overflow state).
   */
  if (!(out.space() > size || (out.large && out.space() == size))) {
    g->buf_switches++;
    out.large = true;
    /* the most connections that have had a large buffer at once (each kept the one it got from a pool) */
    g->buf_largecount = std::max(g->buf_largecount, ++g->buf_large_in_use);
  }
  out.text += pending;

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
 * the player's descriptor: the output, the overflow notice if it overflowed, a blank line for a player who
 * isn't in compact mode, and the prompt, after a \r\n if it interrupts the last prompt. (GARBAGE_SPACE in
 * MAX_SOCK_BUF leaves room for what goes around the output.)
 */
int process_output(GameWorld *g, struct descriptor_data *t)
{
  /* we may need this \r\n for later -- see below */
  std::string packet = "\r\n" + t->out.text;

  /* if we're in the overflow state, notify the user */
  if (t->out.overflowed())
    packet += "**OVERFLOW**\r\n";

  /* add the extra CRLF if the person isn't in compact mode */
  if (desc_get_connected(t) == CON_PLAYING && t->character && !char_is_npc(t->character) && !char_prf_flagged(g, t->character, PRF_COMPACT))
    packet += "\r\n";

  /* add a prompt */
  packet += make_prompt(g, t);
  if (desc_get_connected(t) == CON_PLAYING)
    packet = colourize(g, packet, COLOR_ON(g, t->character), COLOR_CHOICES(t->character), MAX_SOCK_BUF).text;
  std::string_view osb = std::string_view(packet).substr(2); /* the packet without the \r\n */

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  int result;
  if (t->has_prompt) {
    t->has_prompt = false;
    result        = write_to_descriptor(t->sock.handle(), packet);
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
    if ((size_t)result < osb.size())
      t->out.text = osb.substr(result);
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
int write_to_descriptor(net::native_handle desc, std::string_view txt)
{
  size_t write_total = 0;

  while (!txt.empty()) {
    net::io_result sent = net::send_some(desc, txt);

    if (sent.status == net::io_status::would_block) /* Temporary failure -- socket buffer full. */
      return (write_total);
    if (sent.status != net::io_status::done) {
      /* Fatal error.  Disconnect the player. */
      net::report_error("SYSERR: Write to socket");
      return (-1);
    }
    txt.remove_prefix(sent.bytes);
    write_total += sent.bytes;
  }

  return (write_total);
}
