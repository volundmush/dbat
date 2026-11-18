#pragma once
/*
 * Where the game's files are, and the few things done to directories, on std::filesystem.
 */
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace io {

/* The file of kind mode (PLR_FILE, CRASH_FILE and the rest, in dbat/fileop.hpp) for the player or account called
   name: its kind's directory, a bucket by the name's first letter ("A-E" to "U-Z", or "ZZZ"), and the name in lower
   case with the kind's suffix, as "plrfiles/K-O/kelvarin.plr". Nothing for an empty name or an unknown kind. */
std::optional<std::string> player_file(int mode, std::string_view name);

/* Makes path a directory, with any directories above it, as `mkdir -p` does; true if it is one now */
bool ensure_directory(const std::filesystem::path &path);

/* The names in the directory path, without "." and "..", sorted byte by byte; nothing if it can't be read */
std::optional<std::vector<std::string>> list_directory(const std::filesystem::path &path);

/* Makes path exist, as `touch` does, without changing what it holds; false, and logged, if it couldn't */
bool touch(const std::filesystem::path &path);

/* Why the file at path can't be opened for reading, as strerror() words it: "No such file or directory" and the
   like */
std::string why_unreadable(const std::filesystem::path &path);

} // namespace io
