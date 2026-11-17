#pragma once
/*
 * Reading the game's text files.
 *
 * The files are read a line at a time by a few rules that every kind of file shares:
 *   - a "line" is the next one that isn't blank or a '*' comment, without its line end, cut to READ_SIZE - 1 bytes;
 *   - a "string" is text up to a line ending in '~', its lines joined by "\r\n";
 *   - numbers are read as sscanf()'s %d reads them, and flags as letters ("abD") or a plain number.
 * text_reader holds a whole file and reads it by these rules, counting lines, so that an error can say where it is.
 * The free functions are the rules themselves, which the older FILE-based readers (get_line(), fread_string()) use
 * too, until each kind of file moves onto text_reader.
 */
#include <cstdio>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "dbat/consts/types.hpp"
#include "dbat/io/load_error.hpp"

namespace io {

/* Whether get_line() passes over this raw line: a '*' comment, or a blank one */
bool is_skipped_line(std::string_view raw);

/* What get_line() keeps of a raw line: up to its line end, or a '\0' (quirk), cut to READ_SIZE - 1 bytes */
std::string_view line_text(std::string_view raw);

/* Adds one raw line to a string being read as fread_string() reads one; true if the line ends it, with a '~' just
   before its line end (only a trailing '~' counts), otherwise the line is added with "\r\n" after it */
bool add_string_line(std::string &s, std::string_view raw);

/* Reads the next raw line of fl into raw, with its '\n' if it has one, as getline() does; false at the end of the
   file */
bool read_raw_line(std::FILE *fl, std::string &raw);

/* Reads numbers from text as sscanf(text, "%d %d ...") does into out, as many as out holds: how many it read, or -1
   if the text ran out before the first, as sscanf() says. A number too big for a long is the largest long, as strtol
   gives it, then cut to an int (quirk). */
int scan_ints(std::string_view text, std::span<int> out);

/* text as atol() reads it: spaces, a sign and digits, or 0; past a long's range, the largest long of that sign */
long c_atol(std::string_view text);

/* A flags word as asciiflag_conv() reads it: letters a-z for the bits from first_letter_bit (0, or 1 for the old
   affect flags) and A-F for bits 26-31, or a number if it is only digits and '-'. Other capitals are ignored, the
   first of them logged, naming what is read as who. */
bitvector_t ascii_flags(std::string_view word, int first_letter_bit, const char *who);

/* The entries an index file names, up to its "$": the files of one kind to load, in order */
loaded<std::vector<std::string>> read_index(const std::filesystem::path &path);

/* A whole text file, read by the rules above from the start to the end */
class text_reader {
public:
  /* The file at path, or why it can't be read */
  static loaded<text_reader> open(const std::filesystem::path &path);
  /* text, as though it were the file called name */
  text_reader(std::string name, std::string text);

  const std::string &name() const { return name_; }
  /* The line the last one read was on, from 1; 0 before any */
  int line_number() const { return line_; }
  /* How far into the file the reading is, in bytes */
  size_t position() const { return pos_; }
  bool at_end() const { return pos_ >= text_.size(); }

  /* The next raw line, with its '\n' if it has one, as getline() reads it */
  std::optional<std::string_view> raw_line();
  /* The next line by get_line()'s rules, or nothing at the end of the file */
  std::optional<std::string_view> line();
  /* The next string by fread_string()'s rules ("" for an empty one), or an error naming what if the file ends first */
  loaded<std::string> tilde_string(std::string_view what);
  /* The next character that isn't a space, without reading it, as fread_letter() then ungetc() find it; it passes
     over the spaces. EOF at the end. */
  int peek_letter();

  /* An error at the line read last */
  load_error fail(std::string message) const { return {name_, line_, std::move(message)}; }

private:
  std::string name_;
  std::string text_;
  size_t pos_{};
  int line_{};
  int newlines_{}; /* the line ends before pos_ */
};

} // namespace io
