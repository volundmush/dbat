/*
 * Where the game's files are: see dbat/io/paths.hpp.
 */
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <system_error>

#include "dbat/io/paths.hpp"
#include "dbat/db.hpp"
#include "dbat/fileop.hpp"
#include "dbat/log.hpp"

namespace io {

namespace {

struct file_kind {
  int mode;
  const char *directory, *suffix;
};

constexpr file_kind kinds[] = {
    {CRASH_FILE,       LIB_PLROBJS,  SUF_OBJS  },
    {ALIAS_FILE,       LIB_PLRALIAS, SUF_ALIAS },
    {ETEXT_FILE,       LIB_PLRTEXT,  SUF_TEXT  },
    {SCRIPT_VARS_FILE, LIB_PLRVARS,  SUF_MEM   },
    {NEW_OBJ_FILES,    LIB_PLROBJS,  SUF_OBJS  },
    {PLR_FILE,         LIB_PLRFILES, SUF_PLR   },
    {PET_FILE,         LIB_PLRFILES, SUF_PET   },
    {USER_FILE,        LIB_USER,     SUF_USER  },
    {INTRO_FILE,       LIB_INTRO,    SUF_INTRO },
    {SENSE_FILE,       LIB_SENSE,    SUF_SENSE },
    {CUSTOME_FILE,     LIB_USER,     SUF_CUSTOM},
};

/* The directory players are kept in by the first letter of their name */
const char *bucket(char first)
{
  if (first >= 'a' && first <= 'e')
    return "A-E";
  if (first >= 'f' && first <= 'j')
    return "F-J";
  if (first >= 'k' && first <= 'o')
    return "K-O";
  if (first >= 'p' && first <= 't')
    return "P-T";
  if (first >= 'u' && first <= 'z')
    return "U-Z";
  return "ZZZ";
}

} // namespace

std::optional<std::string> player_file(int mode, std::string_view name)
{
  const file_kind *kind = std::ranges::find(kinds, mode, &file_kind::mode);
  if (name.empty() || kind == std::end(kinds))
    return std::nullopt;

  std::string lower(name.substr(0, name.find('\0')));
  for (char &c : lower)
    if (c >= 'A' && c <= 'Z')
      c = static_cast<char>(c - 'A' + 'a');
  return str_format("{}{}/{}.{}", kind->directory, bucket(lower.empty() ? '\0' : lower[0]), lower, kind->suffix);
}

bool ensure_directory(const std::filesystem::path &path)
{
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return std::filesystem::is_directory(path, ec);
}

std::optional<std::vector<std::string>> list_directory(const std::filesystem::path &path)
{
  std::error_code ec;
  std::vector<std::string> names;
  for (std::filesystem::directory_iterator it(path, ec), end; !ec && it != end; it.increment(ec))
    names.push_back(it->path().filename().string());
  if (ec)
    return std::nullopt;
  std::ranges::sort(names); /* byte by byte, as alphasort() in the "C" locale */
  return names;
}

bool touch(const std::filesystem::path &path)
{
  std::FILE *fl = std::fopen(path.string().c_str(), "a");
  if (!fl) {
    mud_log("SYSERR: {}: {}", path.string(), std::strerror(errno));
    return false;
  }
  std::fclose(fl);
  return true;
}

std::string why_unreadable(const std::filesystem::path &path)
{
  std::error_code ec;
  std::filesystem::file_status st = std::filesystem::status(path, ec);
  if (ec)
    return ec.message();
  if (!std::filesystem::exists(st))
    return std::make_error_code(std::errc::no_such_file_or_directory).message();
  if (std::filesystem::is_directory(st))
    return std::make_error_code(std::errc::is_a_directory).message();
  return std::make_error_code(std::errc::permission_denied).message();
}

std::expected<long long, std::string> file_size(const std::filesystem::path &path)
{
  std::error_code ec;
  std::uintmax_t size = std::filesystem::file_size(path, ec);
  if (ec)
    return std::unexpected(ec.message());
  return static_cast<long long>(size);
}

} // namespace io
