/*

improved-edit.c		Routines specific to the improved editor.

*/
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

#include "dbat/io/reader.hpp"
#include "dbat/args.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/game_world.hpp"
#include "dbat/stringutils.hpp"

void send_editor_help(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "Instructions: /s to save, /h for more options.\r\n");
}

namespace {
  constexpr int ALL_LINES = 999999; /* the last line of a range given no end */

  /* Reads "<low>" or "<low> - <high>": 2, 1, or 0 when there is no number (sscanf's -1 included) */
  int scan_range(std::string_view arg, int &low, int &high)
  {
    io::fields first(arg);
    if (!first.number(low))
      return 0;
    std::string_view rest = skip_spaces(first.rest());
    return rest.starts_with('-') && io::fields(rest.substr(1)).number(high) ? 2 : 1;
  }

  /* Where line n (from 1) of text starts, or npos if it has fewer lines. A text ending in a newline has one more,
     empty, line after it. */
  size_t line_start(std::string_view text, int n)
  {
    size_t at = 0;
    for (int i = 1; i < n; i++) {
      if ((at = text.find('\n', at)) == std::string_view::npos)
        return at;
      at++;
    }
    return at;
  }

  /* "s " for a count that isn't one, " " for one: "1 line deleted", "2 lines deleted" */
  std::string_view plural(unsigned count) { return count != 1 ? "s " : " "; }

  /* The letters right after the command ("/fi", "/ra"): does one of the first two match? */
  bool flagged(std::string_view args, char letter)
  {
    for (size_t j = 0; j < args.size() && j < 2 && isalpha(args[j]); j++)
      if (args[j] == letter)
        return true;
    return false;
  }

  /* /l and /n: the range to list, defaulting to everything. False (after saying why) if it is invalid. */
  bool list_range(GameWorld *g, struct descriptor_data *d, std::string_view args, int &low, int &high)
  {
    low  = 1;
    high = ALL_LINES;
    if (!args.empty() && scan_range(args, low, high) == 1)
      high = low;
    if (low < 1) {
      write_to_output(g, d, "Line numbers must be greater than 0.\r\n");
      return false;
    }
    if (high < low) {
      write_to_output(g, d, "That range is invalid.\r\n");
      return false;
    }
    return true;
  }

  /*
   * Re-formats the lines low to high of text, wrapping it at PAGE_WIDTH, capitalising its sentences and (with indent)
   * indenting it, keeping the text to maxlen. False, after saying why, if there aren't low lines.
   */
  bool format_text(GameWorld *g, std::string &text, bool indent, struct descriptor_data *d, size_t maxlen, int low, int high)
  {
    if (text.empty())
      return false;

    std::string formatted;
    /* written through a character at a time as the walk below goes: a '\0' planted in it ends a word, and some are
       left there (quirk), so what is read from a place is the text up to the next '\0' */
    std::string copy = text;
    size_t at        = 0;
    auto from        = [&](size_t i) { return until_nul(std::string_view(copy).substr(i)); };
    auto one_of      = [](std::string_view set, char c) { return c != '\0' && set.contains(c); };

    /* the lines before low, as they are; quirk: an empty one is passed over, and the line after it written twice */
    int i = 0;
    for (; i < low - 1; i++) {
      std::string_view rest = from(at);
      size_t first          = rest.find_first_not_of('\n');
      size_t newline        = rest.find('\n');
      if (first == std::string_view::npos || newline == std::string_view::npos) {
        write_to_output(g, d, "There aren't that many lines!\r\n");
        return false;
      }
      rest = rest.substr(first);
      formatted.append(rest.substr(0, rest.find('\n'))).append("\n");
      at += newline + 1;
    }

    int line_chars = 0, color_chars = 0; bool pass_line = false;
    bool cap_next = true, cap_next_next = false;
    size_t start = 0;
    if (indent) {
      formatted += "   ";
      line_chars = 3;
    }

    while (copy[at] && i < high) {
      while (one_of("\n\r\f\t\v ", copy[at])) {
        if (copy[at] == '\n' && !pass_line)
          if (i++ >= high) {
            pass_line = true;
            break;
          }
        at++;
      }

      if (copy[at]) {
        start = at;
        while (copy[at] && !one_of("\n\r\f\t\v .?!", copy[at])) {
          if (copy[at] == '@') {
            if (copy[at + 1] == '@')
              color_chars++;
            else
              color_chars += 2;
            at++;
          }
          at++;
        }

        if (cap_next_next) {
          cap_next_next = false;
          cap_next      = true;
        }

        /* if we stopped on a sentence, move off its end */
        while (one_of(".!?", copy[at])) {
          cap_next_next = true;
          at++;
        }

        /*
         * At the end of the last line, with a delimiter last, at is on the newline after it: pass over the newlines,
         * so no extra blank line is added. (Welcor 04/04)
         */
        char temp;
        if (one_of("\n\r", copy[at])) {
          copy[at] = '\0'; /* ends the word, and is left there */
          at++;
          if (copy[at] == '\n' && i++ >= high)
            pass_line = true;

          while (one_of("\n\r", copy[at]) && !pass_line) {
            at++;
            if (copy[at] == '\n' && i++ >= high)
              pass_line = true;
          }
          temp = copy[at];
        } else {
          temp     = copy[at];
          copy[at] = '\0';
        }

        if (line_chars + from(start).size() + 1 - color_chars > PAGE_WIDTH) {
          formatted += "\r\n";
          line_chars  = 0;
          color_chars = count_color_chars(from(start));
        }

        std::string word(from(start));
        if (!cap_next) {
          if (line_chars > 0) {
            formatted += " ";
            line_chars++;
          }
        } else {
          cap_next = false;
          CAP(word);
        }

        line_chars += word.size();
        formatted += word;

        copy[at] = temp;
      }

      if (cap_next_next && copy[at]) {
        if (line_chars + 3 - color_chars > PAGE_WIDTH) {
          formatted += "\r\n";
          line_chars  = 0;
          color_chars = count_color_chars(from(start));
        } else if (copy[at] == '\"' || copy[at] == '\'') {
          formatted += copy[at];
          formatted += ' ';
          at++;
          line_chars++;
        } else {
          formatted += " ";
          line_chars += 2;
        }
      }
    }
    if (copy[at])
      formatted += "\r\n";
    formatted += from(at);
    if (!copy[at])
      formatted += "\r\n";

    if (formatted.size() >= maxlen)
      formatted.resize(maxlen - 1);
    text = std::move(formatted);
    return true;
  }

  /* Replaces the first (or, with all, every) pattern in text. How many were replaced, or -1 (text unchanged) if the
     result wouldn't fit in max_size. */
  int replace_str(std::string &text, std::string_view pattern, std::string_view replacement, bool all, size_t max_size)
  {
    std::string out;
    size_t from = 0, match;
    int count   = 0;
    while ((match = text.find(pattern, from)) != std::string::npos) {
      out.append(text, from, match - from).append(replacement);
      from = match + pattern.size();
      count++;
      if (!all)
        break;
    }
    if (count == 0)
      return 0;
    out.append(text, from);
    if (out.size() >= max_size)
      return -1;
    text = std::move(out);
    return count;
  }

  /* ---- the commands, each given what follows its letter ---- */

  void editor_help(GameWorld *g, struct descriptor_data *d, std::string_view)
  {
    write_to_output(g, d,
                    "Editor command formats: /<letter>\r\n\r\n/a         -  aborts editor\r\n/c         -  clears buffer\r\n/d#        -  deletes a line #\r\n/e# <text> -  changes the line at # with <text>\r\n/f         -  formats text\r\n/fi        -  indented formatting of text\r\n/fi#       -  indented formatting on a specific line\r\n/fi #-#    -  indented formating on specific lines\r\n/h         -  list text editor commands\r\n/i# <text> -  inserts <text> before line #\r\n/l         -  lists buffer\r\n/n         -  lists buffer with line numbers\r\n/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n              usage: /r[a] 'pattern' 'replacement'\r\n/s         -  saves text\r\n");
  }

  void clear_text(GameWorld *g, struct descriptor_data *d, std::string_view)
  {
    if (!d->edit_text.empty()) {
      d->edit_text.clear();
      write_to_output(g, d, "Current buffer cleared.\r\n");
    } else
      write_to_output(g, d, "Current buffer empty.\r\n");
  }

  void format_lines(GameWorld *g, struct descriptor_data *d, std::string_view args)
  {
    if (desc_get_connected(d) == CON_TRIGEDIT) { /* a script is indented by its blocks instead */
      write_to_output(g, d, "Script {}formatted.\r\n", format_script(g, d) ? "" : "not ");
      return;
    }
    const bool indent = flagged(args, 'i');
    int low, high;
    switch (scan_range(indent ? args.substr(1) : args, low, high)) {
      case 0:
        low  = 1;
        high = ALL_LINES;
        break;
      case 1: high = low; break;
      case 2:
        if (high < low) {
          write_to_output(g, d, "That range is invalid.\r\n");
          return;
        }
        break;
    }
    if (format_text(g, d->edit_text, indent, d, d->max_str, std::max(1, low), high))
      write_to_output(g, d, "Text formatted with{} indent.\r\n", indent ? "" : "out");
  }

  void replace_text(GameWorld *g, struct descriptor_data *d, std::string_view args)
  {
    const bool all = flagged(args, 'a');

    /* the runs between quotes, as strtok() found them: the second is the pattern, the fourth its replacement */
    std::vector<std::string_view> pieces;
    for (size_t at = 0; at < args.size();) {
      size_t end = std::min(args.find('\'', at), args.size());
      if (end > at)
        pieces.push_back(args.substr(at, end - at));
      at = end + 1;
    }
    static constexpr std::string_view MISSING[] = {"Invalid format.\r\n", "Target string must be enclosed in single quotes.\r\n",
                                              "No replacement string.\r\n", "Replacement string must be enclosed in single quotes.\r\n"};
    if (pieces.size() < 4) {
      write_to_output(g, d, MISSING[pieces.size()]);
      return;
    }
    if (d->edit_text.empty()) /* wb's fix for empty buffer replacement crashing */
      return;

    std::string_view pattern = pieces[1], replacement = pieces[3];
    if (replacement.size() - pattern.size() + d->edit_text.size() > d->max_str) {
      write_to_output(g, d, "Not enough space left in buffer.\r\n");
      return;
    }
    const int replaced = replace_str(d->edit_text, pattern, replacement, all, d->max_str);
    if (replaced > 0)
      write_to_output(g, d, "Replaced {:d} occurance{}of '{}' with '{}'.\r\n", replaced, plural(replaced), pattern, replacement);
    else if (replaced == 0)
      write_to_output(g, d, "String '{}' not found.\r\n", pattern);
    else
      write_to_output(g, d, "ERROR: Replacement string causes buffer overflow, aborted replace.\r\n");
  }

  void delete_lines(GameWorld *g, struct descriptor_data *d, std::string_view args)
  {
    int low, high;
    switch (scan_range(args, low, high)) {
      case 0: write_to_output(g, d, "You must specify a line number or range to delete.\r\n"); return;
      case 1: high = low; break;
      case 2:
        if (high < low) {
          write_to_output(g, d, "That range is invalid.\r\n");
          return;
        }
        break;
    }

    std::string &text = d->edit_text;
    if (text.empty()) {
      write_to_output(g, d, "Buffer is empty.\r\n");
      return;
    }
    if (low <= 0) {
      write_to_output(g, d, "Invalid, line numbers to delete must be higher than 0.\r\n");
      return;
    }
    const size_t start = line_start(text, low);
    if (start == std::string::npos) {
      write_to_output(g, d, "Line(s) out of range; not deleting.\r\n");
      return;
    }
    /* at walks to the start of line high, if there is one */
    size_t at      = start;
    unsigned count = 1;
    for (int i = low; at != std::string::npos && i < high;)
      if ((at = text.find('\n', at)) != std::string::npos) {
        i++;
        count++;
        at++;
      }
    if (const size_t end = at == std::string::npos ? at : text.find('\n', at); end != std::string::npos)
      text.erase(start, end + 1 - start);
    else {
      count--; /* the range ran past the last line: delete to the end */
      text.erase(start);
    }
    write_to_output(g, d, "{:d} line{}deleted.\r\n", count, plural(count));
  }

  void list_lines(GameWorld *g, struct descriptor_data *d, std::string_view args)
  {
    int low, high;
    if (!list_range(g, d, args, low, high))
      return;
    const std::string &text = d->edit_text;
    const size_t start      = line_start(text, low);
    if (start == std::string::npos) {
      write_to_output(g, d, "Line(s) out of range; no buffer listing.\r\n");
      return;
    }
    std::string out;
    if (high < ALL_LINES || low > 1)
      out = str_format("Current buffer range [{:d} - {:d}]:\r\n", low, high);
    size_t end     = start;
    unsigned shown = 0;
    for (int i = low; end != std::string::npos && i <= high;)
      if ((end = text.find('\n', end)) != std::string::npos) {
        i++;
        shown++;
        end++;
      }
    out.append(text, start, end == std::string::npos ? std::string::npos : end - start);
    /* This is kind of annoying...but some people like it. */
    out += str_format("\r\n{:d} line{}shown.\r\n", shown, plural(shown));
    write_to_output(g, d, "{}", out);
  }

  void number_lines(GameWorld *g, struct descriptor_data *d, std::string_view args)
  {
    int low, high;
    if (!list_range(g, d, args, low, high))
      return;
    const std::string &text = d->edit_text;
    size_t at               = line_start(text, low);
    if (at == std::string::npos) {
      write_to_output(g, d, "Line(s) out of range; no buffer listing.\r\n");
      return;
    }
    std::string out;
    for (int i = low; i <= high;) {
      const size_t end = text.find('\n', at);
      if (end == std::string::npos) {
        out.append(text, at); /* a last line with no newline is shown without a number */
        break;
      }
      out += str_format("{:>4d}: ", i++);
      out.append(text, at, end + 1 - at);
      at = end + 1;
    }
    write_to_output(g, d, "{}", out);
  }

  /* /i (insert) and /e (edit): a line number, then the line */
  void put_line(GameWorld *g, struct descriptor_data *d, std::string_view args, bool insert)
  {
    std::string number = take_word(args);
    if (number.empty()) {
      write_to_output(g, d, insert ? "You must specify a line number before which to insert text.\r\n" : "You must specify a line number at which to change text.\r\n");
      return;
    }
    const int line = parse_int(number);
    std::string &text = d->edit_text;
    if (text.empty()) {
      write_to_output(g, d, insert ? "Buffer is empty, nowhere to insert.\r\n" : "Buffer is empty, nothing to change.\r\n");
      return;
    }
    if (line <= 0) {
      write_to_output(g, d, "Line number must be higher than 0.\r\n");
      return;
    }
    const size_t at = line_start(text, line);
    if (at == std::string::npos) {
      write_to_output(g, d, insert ? "Line number out of range; insert aborted.\r\n" : "Line number out of range; change aborted.\r\n");
      return;
    }

    std::string out = text.substr(0, at); /* the lines before */
    out.append(skip_spaces(args)).append("\r\n");
    if (insert) {
      if (out.size() + (text.size() - at) + 2 > d->max_str) {
        write_to_output(g, d, "Insert text pushes buffer over maximum size, insert aborted.\r\n");
        return;
      }
      out.append(text, at); /* then the line it went before, and the rest */
    } else {
      if (size_t end = text.find('\n', at); end != std::string::npos)
        out.append(text, end + 1); /* the lines after the one replaced */
      if (out.size() > d->max_str) {
        write_to_output(g, d, "Change causes new length to exceed buffer maximum size, aborted.\r\n");
        return;
      }
    }
    text = std::move(out);
    write_to_output(g, d, insert ? "Line inserted.\r\n" : "Line changed.\r\n");
  }

  void insert_line(GameWorld *g, struct descriptor_data *d, std::string_view args) { put_line(g, d, args, true); }
  void edit_line(GameWorld *g, struct descriptor_data *d, std::string_view args) { put_line(g, d, args, false); }

  /* The editor's commands, by their letter */
  struct editor_command {
    char letter;
    bool needs_text; /* refused, "Current buffer empty.", with nothing written */
    void (*run)(GameWorld *g, struct descriptor_data *d, std::string_view args);
  };

  constexpr editor_command COMMANDS[] = {
      {'c', false, clear_text  },
      {'d', false, delete_lines},
      {'e', false, edit_line   },
      {'f', true,  format_lines},
      {'i', true,  insert_line },
      {'h', false, editor_help },
      {'l', true,  list_lines  },
      {'n', true,  number_lines},
      {'r', false, replace_text},
  };
} // namespace

int improved_editor_execute(GameWorld *g, struct descriptor_data *d, std::string_view line)
{
  if (!line.starts_with('/'))
    return STRINGADD_OK;

  const char letter = line.size() > 1 ? line[1] : '\0';
  if (letter == 'a')
    return STRINGADD_ABORT;
  if (letter == 's')
    return STRINGADD_SAVE;

  auto command = std::ranges::find(COMMANDS, letter, &editor_command::letter);
  if (command == std::end(COMMANDS))
    write_to_output(g, d, "Invalid option.\r\n");
  else if (command->needs_text && d->edit_text.empty())
    write_to_output(g, d, "Current buffer empty.\r\n");
  else
    command->run(g, d, line.substr(std::min<size_t>(2, line.size())));
  return STRINGADD_ACTION;
}
