#pragma once
/*
 * Writing the game's files so that a crash never leaves half of one.
 *
 * A file is written whole to <path>.tmp, pushed out to the disk, and renamed over <path>: <path> always holds either
 * what it held before or all of what was written, never part of it. With a backup, what <path> held before is kept
 * as <path>.bak. On any failure, <path> is left as it was and the failure is logged.
 *
 *   io::text_writer out(path);
 *   out.print("#{:d}\n", vnum);
 *   ...
 *   return out.commit();
 */
#include <cstdio>
#include <filesystem>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#include "dbat/format.hpp"

namespace io {

/* Finishes replacing path with what was written to fl, which was opened on tmp: flushes it, pushes it to the disk,
   closes it, keeps the old file as <path>.bak if backup is set, and renames tmp over path. fl is closed whatever
   happens; false, with tmp removed and path as it was, if any of it failed. */
bool replace_with(std::FILE *fl, const std::string &tmp, const std::string &path, bool backup);

/* Replaces path with text, as above */
bool replace_file(const std::string &path, std::string_view text, bool backup);

/* A file built up in memory and then written all at once */
class text_writer {
public:
  explicit text_writer(std::string path, bool backup = false) : path_(std::move(path)), backup_(backup) {}

  const std::string &path() const { return path_; }
  /* what has been written so far */
  const std::string &text() const { return text_; }

  void write(std::string_view s) { text_ += s; }
  /* text formatted as str_format() formats it */
  template <class... Args> void print(std::format_string<Args...> fmt, Args &&...args)
  {
    text_ += str_format(fmt, std::forward<Args>(args)...);
  }

  /* Replaces the file with what was written; false, and logged, if it couldn't */
  bool commit() const { return replace_file(path_, text_, backup_); }

private:
  std::string path_;
  bool backup_{};
  std::string text_{};
};

} // namespace io
