/*
 * The little the file layer needs that standard C++ doesn't have: see dbat/io/platform.hpp. This is the only file of
 * the layer that knows which system it runs on.
 */
#include <cerrno>

#include "dbat/io/platform.hpp"

#ifdef _WIN32
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace io::platform {

bool flush_to_disk(std::FILE *fl)
{
#ifdef _WIN32
  return _commit(_fileno(fl)) == 0;
#else
  return fsync(fileno(fl)) == 0;
#endif
}

bool sync_directory(const std::filesystem::path &dir)
{
#ifdef _WIN32
  (void)dir; /* a rename is durable once it returns */
  return true;
#else
  int fd = open(dir.c_str(), O_RDONLY | O_DIRECTORY);
  if (fd < 0)
    return false;
  bool ok = fsync(fd) == 0 || errno == EINVAL; /* EINVAL: the filesystem can't sync directories */
  close(fd);
  return ok;
#endif
}

} // namespace io::platform
