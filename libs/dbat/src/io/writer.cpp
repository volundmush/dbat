/*
 * Writing the game's files whole or not at all: see dbat/io/writer.hpp.
 */
#include <cerrno>
#include <cstring>
#include <system_error>

#include "dbat/io/writer.hpp"
#include "dbat/io/platform.hpp"
#include "dbat/log.hpp"

namespace io {

namespace {

void log_failure(const char *operation, const std::string &filename, int error)
{
  mud_log("SYSERR: atomic save: {} failed for {}: {}", operation, filename, std::strerror(error));
}

void log_failure(const char *operation, const std::string &filename, const std::error_code &ec)
{
  mud_log("SYSERR: atomic save: {} failed for {}: {}", operation, filename, ec.message());
}

/* The directory a file is in, for syncing it: "." for a bare name */
std::filesystem::path directory_of(const std::string &path)
{
  std::filesystem::path dir = std::filesystem::path(path).parent_path();
  return dir.empty() ? std::filesystem::path(".") : dir;
}

} // namespace

bool replace_with(std::FILE *fl, const std::string &tmp, const std::string &path, bool backup)
{
  std::error_code ec;

  if (!fl)
    return false;
  if (std::ferror(fl) || std::fflush(fl) == EOF) {
    log_failure("write", tmp, errno);
    std::fclose(fl);
    std::filesystem::remove(tmp, ec);
    return false;
  }
  if (!platform::flush_to_disk(fl)) {
    log_failure("fsync", tmp, errno);
    std::fclose(fl);
    std::filesystem::remove(tmp, ec);
    return false;
  }
  if (std::fclose(fl) == EOF) {
    log_failure("fclose", tmp, errno);
    std::filesystem::remove(tmp, ec);
    return false;
  }

  if (backup) { /* best effort: a missing backup never stops the save */
    std::string bak = path + ".bak";
    std::filesystem::remove(bak, ec);
    if (ec)
      log_failure("remove old backup", bak, ec);
    if (std::filesystem::exists(path, ec)) {
      std::filesystem::copy_file(path, bak, std::filesystem::copy_options::overwrite_existing, ec);
      if (ec)
        log_failure("copy backup", bak, ec);
    }
  }

  std::filesystem::rename(tmp, path, ec);
  if (ec) {
    log_failure("rename", tmp, ec);
    std::filesystem::remove(tmp, ec);
    return false;
  }
  std::filesystem::path dir = directory_of(path);
  if (!platform::sync_directory(dir))
    log_failure("fsync directory", dir.string(), errno);
  return true;
}

bool replace_file(const std::string &path, std::string_view text, bool backup)
{
  std::string tmp = path + ".tmp";
  std::FILE *fl   = std::fopen(tmp.c_str(), "wb");
  if (!fl) {
    log_failure("fopen", tmp, errno);
    return false;
  }
  if (std::fwrite(text.data(), 1, text.size(), fl) != text.size()) {
    log_failure("write", tmp, errno);
    std::fclose(fl);
    std::error_code ec;
    std::filesystem::remove(tmp, ec);
    return false;
  }
  return replace_with(fl, tmp, path, backup);
}

bool append_file(const std::string &path, std::string_view text)
{
  std::FILE *fl = std::fopen(path.c_str(), "ab");
  if (!fl)
    return false;
  bool written = std::fwrite(text.data(), 1, text.size(), fl) == text.size();
  return std::fclose(fl) == 0 && written;
}

} // namespace io
