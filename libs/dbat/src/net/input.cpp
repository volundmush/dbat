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

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/args.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"

#include "comm_internal.hpp"

void write_to_q(std::string_view txt, struct txt_q *queue, bool aliased)
{
  queue->lines.push_back({std::string(txt), aliased});
  if (!aliased)
    queue->typed++;
}

std::optional<txt_line> get_from_q(struct txt_q *queue)
{
  if (queue->lines.empty())
    return std::nullopt;

  txt_line line = std::move(queue->lines.front());
  queue->lines.pop_front();
  if (!line.aliased)
    queue->typed--;
  if (line.text.size() > MAX_INPUT_LENGTH - 1) /* lines are queued at most this long */
    line.text.resize(MAX_INPUT_LENGTH - 1);
  return line;
}

/* Throw away the commands a descriptor has typed but not yet run. */
void flush_input_queue(struct descriptor_data *d)
{
  d->input.lines.clear();
  d->input.typed = 0;
}

namespace {

/* Where the next newline in raw is, from from on; none if there is none before a NUL byte (quirk: the C string the
   input was ended there) */
std::optional<size_t> find_newline(const std::string &raw, size_t from)
{
  for (size_t i = from; i < raw.size() && raw[i] != '\0'; i++)
    if (ISNEWL(raw[i]))
      return i;
  return std::nullopt;
}

enum class read_outcome {
  line,    /* there is a whole line to take */
  waiting, /* not yet */
  lost,    /* the connection is gone, or sent more than a line can be */
};

/* Reads what has come for t onto its raw input, until there's a newline: where the first one is, in newline. Returns
   after the first read with nothing in it, so a client that sends no newline never holds the game up. */
read_outcome read_available(struct descriptor_data *t, size_t &newline)
{
  std::string &raw = t->raw_input;

  /* quirk: what is kept of an unfinished line ends at a NUL byte in it, as the C string it was */
  raw.resize(std::min(raw.size(), raw.find('\0')));
  for (;;) {
    size_t space_left = MAX_RAW_INPUT_LENGTH - 1 - raw.size();
    if (space_left == 0) {
      mud_log("WARNING: process_input: about to close connection: input overflow");
      return read_outcome::lost;
    }

    size_t start = raw.size();
    raw.resize(start + space_left);
    net::io_result got = net::receive_some(t->sock.handle(), {raw.data() + start, space_left});
    raw.resize(start + (got.status == net::io_status::done ? got.bytes : 0));
    if (got.status == net::io_status::would_block) /* Just blocking, no problems. */
      return read_outcome::waiting;
    if (got.status != net::io_status::done) /* Error, disconnect them. */
      return read_outcome::lost;

    /* only what just came is searched */
    if (std::optional<size_t> found = find_newline(raw, start)) {
      newline = *found;
      return read_outcome::line;
    }
  }
}

/* A line as typed, made fit to run */
struct cleaned_line {
  std::string text;
  bool cut{}; /* "Line too long" */
};

/* The line in raw: backspace and DEL rub out, only printable ASCII is kept, and each $ is doubled, all within
   MAX_INPUT_LENGTH - 1. */
cleaned_line clean_line(std::string_view raw)
{
  cleaned_line line;
  size_t space_left = MAX_INPUT_LENGTH - 1;
  size_t i          = 0;

  /* The '> 1' reserves room for a '$ => $$' expansion. */
  for (; space_left > 1 && i < raw.size(); i++) {
    char c = raw[i];
    if (c == '\b' || c == 127) { /* handle backspacing or delete key */
      if (!line.text.empty()) {
        bool dollar = line.text.back() == '$'; /* rubbing out a $ takes its double too */
        line.text.pop_back();
        if (dollar) {
          line.text.pop_back();
          space_left += 2;
        } else
          space_left++;
      }
    } else if (isascii(c) && isprint(c)) {
      line.text += c;
      if (c == '$') { /* if it's a $, double it */
        line.text += '$';
        space_left -= 2;
      } else
        space_left--;
    }
  }
  /* quirk: only a $ doubled into the last of the room counts as too long; any other long line is just cut */
  line.cut = space_left == 0 && i < raw.size();
  return line;
}

/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell". True if it couldn't be done.
 */
bool perform_subst(GameWorld *g, struct descriptor_data *t, const std::string &orig, std::string &subst)
{
  /* the text to replace runs from after the first '^' to the second */
  size_t second = subst.find('^', 1);
  if (second == std::string::npos) {
    write_to_output(g, t, "Invalid substitution.\r\n");
    return true;
  }
  std::string first = subst.substr(1, second - 1);

  /* now, see if the contents of the first string appear in the original */
  size_t at = orig.find(first);
  if (at == std::string::npos) {
    write_to_output(g, t, "Invalid substitution.\r\n");
    return true;
  }

  /* everything in the original up to it, the replacement, then the rest of the original, within the line's length */
  const size_t room = MAX_INPUT_LENGTH - 1;
  std::string changed = orig.substr(0, at);
  changed.append(subst, second + 1, room - changed.size());
  if (at + first.size() < orig.size())
    changed.append(orig, at + first.size(), room - changed.size());
  subst = std::move(changed);
  return false;
}

/* What a typed line does before it is queued: '!' repeats the last command, '!text' the latest that starts with
   text, '^old^new' the last with old changed to new, and '--' cancels what is queued. False if the connection is to
   be closed. */
bool handle_typed(GameWorld *g, struct descriptor_data *t, std::string tmp, bool cut)
{
  if (cut) {
    std::string buffer = str_format("Line too long.  Truncated to:\r\n{}\r\n", tmp);
    if (write_to_descriptor(t->sock.handle(), buffer.c_str()) < 0)
      return false;
  }
  if (t->snoop_by)
    write_to_output(g, t->snoop_by, "% {}\r\n", tmp);
  bool failed_subst = false;

  if (tmp == "!") /* Redo last command. */
    tmp = t->last_input;
  else if (tmp.starts_with('!')) {
    std::string_view commandln = skip_spaces(std::string_view(tmp).substr(1));
    int starting_pos = t->history_pos, cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

    for (; cnt != starting_pos; cnt--) {
      if (!t->history[cnt].empty() && is_abbrev(commandln, t->history[cnt])) {
        tmp           = t->history[cnt];
        t->last_input = tmp;
        write_to_output(g, t, "{}\r\n", tmp);
        break;
      }
      if (cnt == 0) /* At top, loop to bottom. */
        cnt = HISTORY_SIZE;
    }
  } else if (tmp.starts_with('^')) {
    if (!(failed_subst = perform_subst(g, t, t->last_input, tmp)))
      t->last_input = tmp;
  } else {
    t->last_input              = tmp;
    t->history[t->history_pos] = tmp;     /* Replace the old line. */
    if (++t->history_pos >= HISTORY_SIZE) /* Wrap to top. */
      t->history_pos = 0;
  }
  /* the '--' command flushes the queue - Jamdog - 9th May 2007 */
  if (tmp == "--") {
    write_to_output(g, t, "All queued commands cancelled.\r\n");
    flush_input_queue(t); /* only the commands: pending output stays */
    failed_subst = true;  /* and don't queue the -- itself */
  }
  if (!failed_subst) {
    /* commands run at most one per pulse; a client sending far faster is flooding */
    if (t->input.typed >= MAX_QUEUED_INPUT) {
      write_to_descriptor(t->sock.handle(), "\r\nToo many commands waiting to run; disconnecting.\r\n");
      mudlog(g, LOG_CMP, ADMLVL_GOD, true, "Input flood from [{}]: more than {:d} commands queued; disconnecting.", t->host, MAX_QUEUED_INPUT);
      return false;
    }
    write_to_q(tmp, &t->input, false);
  }
  return true;
}

} // namespace

/* Reads what t has sent, and queues each whole line in it: 1 if there were any, 0 if not yet, -1 if t is to be
   closed. What follows the last newline waits for the next. */
int process_input(GameWorld *g, struct descriptor_data *t)
{
  size_t newline = 0;

  switch (read_available(t, newline)) {
    case read_outcome::waiting:
      return (0);
    case read_outcome::lost:
      return (-1);
    case read_outcome::line:
      break;
  }

  std::string &raw = t->raw_input;
  size_t begin     = 0;
  for (std::optional<size_t> line_end = newline; line_end;) {
    cleaned_line line = clean_line(std::string_view(raw).substr(begin, *line_end - begin));
    if (!handle_typed(g, t, std::move(line.text), line.cut))
      return (-1);

    /* past this line's newlines: \r\n, \n\r, or blank lines, which are skipped */
    begin = *line_end;
    while (begin < raw.size() && ISNEWL(raw[begin]))
      begin++;
    line_end = find_newline(raw, begin);
  }

  /* the rest waits for its newline (quirk: up to any NUL byte in it) */
  raw = raw.substr(begin, std::min(raw.size(), raw.find('\0', begin)) - begin);
  return (1);
}
