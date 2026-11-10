/*

improved-edit.c		Routines specific to the improved editor.

*/
#include <cstdlib>

#include "dbat/improved-edit.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/game_world.hpp"

static bool format_text(GameWorld *g, std::string *text, int mode, struct descriptor_data *d, unsigned int maxlen, int low, int high);
static void parse_edit_action(GameWorld *g, int command, char *string, struct descriptor_data *d);
static int replace_str(std::string *string, char *pattern, char *replacement, int rep_all, unsigned int max_size);

void send_editor_help(GameWorld *g, struct descriptor_data *d)
{
  if (using_improved_editor)
    write_to_output(g, d, "Instructions: /s to save, /h for more options.\r\n");
  else
    write_to_output(g, d, "Instructions: Type @ on a line by itself to end.\r\n");
}

/* ---- a growing text buffer, so nothing is assembled in a fixed-size array ---- */

struct textbuf {
  char *s{};
  size_t len{}, cap{};
};

static void tb_add(struct textbuf *b, const char *text, size_t n)
{
  if (b->len + n + 1 > b->cap) {
    b->cap = MAX(b->cap * 2, b->len + n + 1);
    b->s   = str_realloc(b->s, b->cap);
  }
  memcpy(b->s + b->len, text, n);
  b->len += n;
  b->s[b->len] = '\0';
}

static void tb_puts(struct textbuf *b, const char *text)
{
  tb_add(b, text, strlen(text));
}

/* Adds text formatted as std::format does */
template <class... Args>
static void tb_format(struct textbuf *b, std::format_string<Args...> fmt, Args &&...args)
{
  std::string piece = str_format(fmt, std::forward<Args>(args)...);
  tb_add(b, piece.data(), piece.size());
}

/* Replaces the text being edited with the buffer's contents. */
static void tb_install(struct textbuf *b, std::string *text)
{
  *text = b->s ? b->s : "";
  free(b->s);
  b->s = nullptr;
}

/* ---- line helpers ---- */

/* Reads "<low>" or "<low> - <high>": 2, 1, or 0 when there is no number (sscanf's -1 included). */
static int scan_range(const char *arg, int *low, int *high)
{
  int found = sscanf(arg, " %d - %d ", low, high);

  return found < 0 ? 0 : found;
}

/* The start of line n (from 1) of text, or NULL if it has fewer lines. A text ending in a newline
   has one more, empty, line after it. */
static char *line_start(char *text, int n)
{
  int i = 1;

  while (text && i < n)
    if ((text = strchr(text, '\n')) != nullptr) {
      i++;
      text++;
    }
  return i < n ? nullptr : text;
}

/* ---- the editor ---- */

int improved_editor_execute(GameWorld *g, struct descriptor_data *d, char *str)
{
  char actions[MAX_INPUT_LENGTH];

  if (*str != '/')
    return STRINGADD_OK;

  /* what follows "/x" (nothing for a lone "/", whose letter is the terminator) */
  strlcpy(actions, str[1] ? str + 2 : "", sizeof(actions));
  *str = '\0';

  switch (str[1]) {
    case 'a':
      return STRINGADD_ABORT;
    case 'c':
      if (!d->edit_text.empty()) {
        d->edit_text.clear();
        write_to_output(g, d, "Current buffer cleared.\r\n");
      } else
        write_to_output(g, d, "Current buffer empty.\r\n");
      break;
    case 'd':
      parse_edit_action(g, PARSE_DELETE, actions, d);
      break;
    case 'e':
      parse_edit_action(g, PARSE_EDIT, actions, d);
      break;
    case 'f':
      if (!d->edit_text.empty())
        parse_edit_action(g, PARSE_FORMAT, actions, d);
      else
        write_to_output(g, d, "Current buffer empty.\r\n");
      break;
    case 'i':
      if (!d->edit_text.empty())
        parse_edit_action(g, PARSE_INSERT, actions, d);
      else
        write_to_output(g, d, "Current buffer empty.\r\n");
      break;
    case 'h':
      parse_edit_action(g, PARSE_HELP, actions, d);
      break;
    case 'l':
      if (!d->edit_text.empty())
        parse_edit_action(g, PARSE_LIST_NORM, actions, d);
      else
        write_to_output(g, d, "Current buffer empty.\r\n");
      break;
    case 'n':
      if (!d->edit_text.empty())
        parse_edit_action(g, PARSE_LIST_NUM, actions, d);
      else
        write_to_output(g, d, "Current buffer empty.\r\n");
      break;
    case 'r':
      parse_edit_action(g, PARSE_REPLACE, actions, d);
      break;
    case 's':
      return STRINGADD_SAVE;
    default:
      write_to_output(g, d, "Invalid option.\r\n");
      break;
  }
  return STRINGADD_ACTION;
}

/* /l and /n: the range to list, defaulting to everything. False (after saying why) if it is invalid. */
static bool list_range(GameWorld *g, struct descriptor_data *d, const char *string, int *low, int *high)
{
  *low  = 1;
  *high = 999999;
  if (*string && scan_range(string, low, high) == 1)
    *high = *low;
  if (*low < 1) {
    write_to_output(g, d, "Line numbers must be greater than 0.\r\n");
    return false;
  }
  if (*high < *low) {
    write_to_output(g, d, "That range is invalid.\r\n");
    return false;
  }
  return true;
}

/*
 * Handle some editor commands.
 */
static void parse_edit_action(GameWorld *g, int command, char *string, struct descriptor_data *d)
{
  int indent = 0, rep_all = 0, flags = 0, replaced, i, line_low, line_high, j = 0;
  unsigned int total_len;
  char *s, *t;
  char num[MAX_INPUT_LENGTH], text[MAX_INPUT_LENGTH + 2];
  struct textbuf out = {nullptr};

  switch (command) {
    case PARSE_HELP:
      write_to_output(g, d,
                      "Editor command formats: /<letter>\r\n\r\n/a         -  aborts editor\r\n/c         -  clears buffer\r\n/d#        -  deletes a line #\r\n/e# <text> -  changes the line at # with <text>\r\n/f         -  formats text\r\n/fi        -  indented formatting of text\r\n/fi#       -  indented formatting on a specific line\r\n/fi #-#    -  indented formating on specific lines\r\n/h         -  list text editor commands\r\n/i# <text> -  inserts <text> before line #\r\n/l         -  lists buffer\r\n/n         -  lists buffer with line numbers\r\n/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n              usage: /r[a] 'pattern' 'replacement'\r\n/s         -  saves text\r\n");
      break;

    case PARSE_FORMAT:
      if (desc_get_connected(d) == CON_TRIGEDIT) {
        write_to_output(g, d, "Script {}formatted.\r\n", format_script(g, d) ? "" : "not ");
        return;
      }
      while (isalpha(string[j]) && j < 2) {
        if (string[j++] == 'i' && !indent) {
          indent = true;
          flags += FORMAT_INDENT;
        }
      }
      switch (scan_range(indent ? string + 1 : string, &line_low, &line_high)) {
        case 0:
          line_low  = 1;
          line_high = 999999;
          break;
        case 1:
          line_high = line_low;
          break;
        case 2:
          if (line_high < line_low) {
            write_to_output(g, d, "That range is invalid.\r\n");
            return;
          }
          break;
      }
      /* in case line_low is negative or zero */
      line_low = MAX(1, line_low);

      if (format_text(g, &d->edit_text, flags, d, d->max_str, line_low, line_high))
        write_to_output(g, d, "Text formatted with{} indent.\r\n", (indent ? "" : "out"));
      break;

    case PARSE_REPLACE:
      while (isalpha(string[j]) && j < 2)
        if (string[j++] == 'a' && !indent)
          rep_all = 1;

      if ((s = strtok(string, "'")) == nullptr) {
        write_to_output(g, d, "Invalid format.\r\n");
        return;
      }
      if ((s = strtok(nullptr, "'")) == nullptr) {
        write_to_output(g, d, "Target string must be enclosed in single quotes.\r\n");
        return;
      }
      if ((t = strtok(nullptr, "'")) == nullptr) {
        write_to_output(g, d, "No replacement string.\r\n");
        return;
      }
      if ((t = strtok(nullptr, "'")) == nullptr) {
        write_to_output(g, d, "Replacement string must be enclosed in single quotes.\r\n");
        return;
        /*wb's fix for empty buffer replacement crashing */
      }
      if (d->edit_text.empty()) {
        return;
      }
      if ((total_len = ((strlen(t) - strlen(s)) + d->edit_text.size())) <= d->max_str) {
        if ((replaced = replace_str(&d->edit_text, s, t, rep_all, d->max_str)) > 0) {
          write_to_output(g, d, "Replaced {:d} occurance{}of '{}' with '{}'.\r\n", replaced, ((replaced != 1) ? "s " : " "), s, t);
        } else if (replaced == 0) {
          write_to_output(g, d, "String '{}' not found.\r\n", s);
        } else
          write_to_output(g, d, "ERROR: Replacement string causes buffer overflow, aborted replace.\r\n");
      } else
        write_to_output(g, d, "Not enough space left in buffer.\r\n");
      break;

    case PARSE_DELETE:
      switch (scan_range(string, &line_low, &line_high)) {
        case 0:
          write_to_output(g, d, "You must specify a line number or range to delete.\r\n");
          return;
        case 1:
          line_high = line_low;
          break;
        case 2:
          if (line_high < line_low) {
            write_to_output(g, d, "That range is invalid.\r\n");
            return;
          }
          break;
      }

      if (d->edit_text.empty()) {
        write_to_output(g, d, "Buffer is empty.\r\n");
        return;
      }
      if (line_low <= 0) {
        write_to_output(g, d, "Invalid, line numbers to delete must be higher than 0.\r\n");
        return;
      }
      if (!(s = line_start(d->edit_text.data(), line_low))) {
        write_to_output(g, d, "Line(s) out of range; not deleting.\r\n");
        return;
      }
      /* t is where the deleted lines start; s walks to the end of line_high */
      t         = s;
      i         = line_low;
      total_len = 1;
      while (s && i < line_high)
        if ((s = strchr(s, '\n')) != nullptr) {
          i++;
          total_len++;
          s++;
        }
      if (s && (s = strchr(s, '\n')) != nullptr)
        memmove(t, s + 1, strlen(s + 1) + 1); /* close the gap */
      else {
        total_len--; /* the range ran past the last line: delete to the end */
        *t = '\0';
      }
      d->edit_text.resize(strlen(d->edit_text.c_str())); /* to where the text now ends */
      write_to_output(g, d, "{:d} line{}deleted.\r\n", as_unsigned(total_len), (total_len != 1 ? "s " : " "));
      break;

    case PARSE_LIST_NORM:
      if (!list_range(g, d, string, &line_low, &line_high))
        return;
      if (!(s = line_start(d->edit_text.data(), line_low))) {
        write_to_output(g, d, "Line(s) out of range; no buffer listing.\r\n");
        return;
      }
      if (line_high < 999999 || line_low > 1)
        tb_format(&out, "Current buffer range [{:d} - {:d}]:\r\n", line_low, line_high);
      t         = s;
      i         = line_low;
      total_len = 0;
      while (s && i <= line_high)
        if ((s = strchr(s, '\n')) != nullptr) {
          i++;
          total_len++;
          s++;
        }
      tb_add(&out, t, s ? (size_t)(s - t) : strlen(t));
      /*
     * This is kind of annoying...but some people like it.
     */
      tb_format(&out, "\r\n{:d} line{}shown.\r\n", as_unsigned(total_len), (total_len != 1) ? "s " : " ");
      write_to_output(g, d, "{}", out.s);
      break;

    case PARSE_LIST_NUM:
      if (!list_range(g, d, string, &line_low, &line_high))
        return;
      if (!(s = line_start(d->edit_text.data(), line_low))) {
        write_to_output(g, d, "Line(s) out of range; no buffer listing.\r\n");
        return;
      }
      i = line_low;
      while (s && i <= line_high) {
        if (!(t = strchr(s, '\n'))) {
          tb_puts(&out, s); /* a last line with no newline is shown without a number */
          break;
        }
        tb_format(&out, "{:>4d}: ", i++);
        tb_add(&out, s, (size_t)(t + 1 - s));
        s = t + 1;
      }
      write_to_output(g, d, "{}", out.s ? out.s : "");
      break;

    case PARSE_INSERT:
    case PARSE_EDIT:
      half_chop(string, num, text);
      if (*num == '\0') {
        write_to_output(g, d, command == PARSE_INSERT ? "You must specify a line number before which to insert text.\r\n" : "You must specify a line number at which to change text.\r\n");
        return;
      }
      line_low = atoi(num);
      strcat(text, "\r\n"); /* (room reserved in its size) */

      if (d->edit_text.empty()) {
        write_to_output(g, d, command == PARSE_INSERT ? "Buffer is empty, nowhere to insert.\r\n" : "Buffer is empty, nothing to change.\r\n");
        return;
      }
      if (line_low <= 0) {
        write_to_output(g, d, "Line number must be higher than 0.\r\n");
        return;
      }
      if (!(s = line_start(d->edit_text.data(), line_low))) {
        write_to_output(g, d, command == PARSE_INSERT ? "Line number out of range; insert aborted.\r\n" : "Line number out of range; change aborted.\r\n");
        return;
      }

      tb_add(&out, d->edit_text.data(), (size_t)(s - d->edit_text.data())); /* the lines before */
      tb_puts(&out, text);                          /* the new line */
      if (command == PARSE_INSERT) {
        if (out.len + strlen(s) + 2 > d->max_str) {
          free(out.s);
          write_to_output(g, d, "Insert text pushes buffer over maximum size, insert aborted.\r\n");
          return;
        }
        tb_puts(&out, s); /* then the line it went before, and the rest */
      } else {
        if ((t = strchr(s, '\n')) != nullptr)
          tb_puts(&out, t + 1); /* the lines after the one replaced */
        if (out.len > d->max_str) {
          free(out.s);
          write_to_output(g, d, "Change causes new length to exceed buffer maximum size, aborted.\r\n");
          return;
        }
      }
      tb_install(&out, &d->edit_text);
      write_to_output(g, d, command == PARSE_INSERT ? "Line inserted.\r\n" : "Line changed.\r\n");
      return;

    default:
      write_to_output(g, d, "Invalid option.\r\n");
      mudlog(g, LOG_BRF, ADMLVL_IMPL, true, "SYSERR: invalid command passed to parse_edit_action");
      return;
  }
  free(out.s);
}

/*
 * Re-formats message type formatted char *.
 * (the text being edited, d->edit_text: mostly olc and mail)
 */
static bool format_text(GameWorld *g, std::string *text, int mode, struct descriptor_data *d, unsigned int maxlen, int low, int high)
{
  int line_chars, cap_next = true, cap_next_next = false, color_chars = 0, i, pass_line = 0;
  char *flow, *start = nullptr, *str, temp;
  struct textbuf formatted = {nullptr};

  /* XXX: Want to make sure the string doesn't grow either... */

  if (text->empty())
    return false;
  flow = text->data(); /* written through, a character at a time, until tb_install() replaces it */

  str = strdup(flow);
  for (i = 0; i < low - 1; i++) {
    start = strtok(str, "\n");
    if (!start || !(flow = strchr(flow, '\n'))) {
      write_to_output(g, d, "There aren't that many lines!\r\n");
      free(str);
      free(formatted.s);
      return false;
    }
    tb_puts(&formatted, start);
    tb_puts(&formatted, "\n");
    strcpy(str, ++flow); /* (a suffix of the text str was copied from) */
  }
  free(str);

  if (IS_SET(mode, FORMAT_INDENT)) {
    tb_puts(&formatted, "   ");
    line_chars = 3;
  } else {
    line_chars = 0;
  }

  while (*flow && i < high) {
    while (*flow && strchr("\n\r\f\t\v ", *flow)) {
      if (*flow == '\n' && !pass_line)
        if (i++ >= high) {
          pass_line = 1;
          break;
        }
      flow++;
    }

    if (*flow) {
      start = flow;
      while (*flow && !strchr("\n\r\f\t\v .?!", *flow)) {
        if (*flow == '@') {
          if (*(flow + 1) == '@')
            color_chars++;
          else
            color_chars += 2;
          flow++;
        }
        flow++;
      }

      if (cap_next_next) {
        cap_next_next = false;
        cap_next      = true;
      }

      /*
       * This is so that if we stopped on a sentence .. we move off the
       * sentence delimiter. (strchr() also matches the terminating NUL, so test for it first.)
       */
      while (*flow && strchr(".!?", *flow)) {
        cap_next_next = true;
        flow++;
      }

      /*
       * special case: if we're at the end of the last line, and the last
       * character is a delimiter, the flow++ above will have *flow pointing
       * to the \r (or \n) character after the delimiter. Thus *flow will
       * be non-null, and an extra (blank) line might be added erroneously.
       * We fix it by skipping the newline characters in between.
       *
       * Welcor 04/04
       */

      if (*flow && strchr("\n\r", *flow)) {
        *flow = '\0'; /* terminate 'start' string */
        flow++;       /* we know this is safe     */
        if (*flow == '\n' && i++ >= high)
          pass_line = 1;

        while (*flow && strchr("\n\r", *flow) && !pass_line) {
          flow++; /* skip to next non-delimiter */
          if (*flow == '\n' && i++ >= high)
            pass_line = 1;
        }
        temp = *flow; /* save this char             */
      } else {
        temp  = *flow;
        *flow = '\0';
      }

      if (line_chars + strlen(start) + 1 - color_chars > PAGE_WIDTH) {
        tb_puts(&formatted, "\r\n");
        line_chars  = 0;
        color_chars = count_color_chars(start);
      }

      if (!cap_next) {
        if (line_chars > 0) {
          tb_puts(&formatted, " ");
          line_chars++;
        }
      } else {
        cap_next = false;
        CAP(start);
      }

      line_chars += strlen(start);
      tb_puts(&formatted, start);

      *flow = temp;
    }

    if (cap_next_next && *flow) {
      if (line_chars + 3 - color_chars > PAGE_WIDTH) {
        tb_puts(&formatted, "\r\n");
        line_chars  = 0;
        color_chars = count_color_chars(start);
      } else if (*flow == '\"' || *flow == '\'') {
        tb_format(&formatted, "{:c} ", as_char(*flow));
        flow++;
        line_chars++;
      } else {
        tb_puts(&formatted, " ");
        line_chars += 2;
      }
    }
  }
  if (*flow)
    tb_puts(&formatted, "\r\n");
  tb_puts(&formatted, flow);
  if (!*flow)
    tb_puts(&formatted, "\r\n");

  if (formatted.len + 1 > maxlen)
    formatted.s[maxlen - 1] = '\0';
  tb_install(&formatted, text);
  return true;
}

/* Replaces the first (or, with rep_all, every) occurrence of pattern. Returns how many were replaced,
   or -1 (text unchanged) if the result would not fit in max_size. */
static int replace_str(std::string *string, char *pattern, char *replacement, int rep_all, unsigned int max_size)
{
  struct textbuf out = {nullptr};
  char *flow         = string->data(), *match;
  size_t plen        = strlen(pattern);
  int count          = 0;

  while ((match = strstr(flow, pattern)) != nullptr) {
    tb_add(&out, flow, (size_t)(match - flow));
    tb_puts(&out, replacement);
    flow = match + plen;
    count++;
    if (!rep_all)
      break;
  }
  if (count == 0)
    return 0;
  tb_puts(&out, flow);
  if (out.len >= max_size) {
    free(out.s);
    return -1;
  }
  tb_install(&out, string);
  return count;
}
