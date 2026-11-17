#pragma once
/*
 * The little the file layer needs that standard C++ doesn't have. Everything else in dbat/io is standard C++.
 */
#include <cstdio>
#include <filesystem>

namespace io::platform {

/* Pushes what fl holds out to the disk itself, not just to the system, so that it survives a crash. False if it
   couldn't. fl must have been flushed. */
bool flush_to_disk(std::FILE *fl);

/* Makes a rename in dir survive a crash too: the directory's entries are data as well. Does nothing where that isn't
   needed or can't be done. False only if it was tried and failed. */
bool sync_directory(const std::filesystem::path &dir);

} // namespace io::platform
