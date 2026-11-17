/*
 * Crash-safe file replacement through a FILE: see dbat/atomicfile.hpp. The work is io::replace_with()'s.
 */
#include <cerrno>
#include <cstring>
#include <system_error>

#include "dbat/atomicfile.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

FILE *atomic_open(struct atomic_file *af, const char *path, bool backup)
{
  af->backup = backup;
  af->path   = path;
  af->tmp    = af->path + ".tmp";
  if (!(af->fp = fopen(af->tmp.c_str(), "wb")))
    mud_log("SYSERR: atomic save: {} failed for {}: {}", "fopen", af->tmp, strerror(errno));
  return af->fp;
}

void atomic_abort(struct atomic_file *af)
{
  if (af->fp)
    fclose(af->fp);
  af->fp = nullptr;
  std::error_code ec;
  std::filesystem::remove(af->tmp, ec);
}

bool atomic_commit(struct atomic_file *af)
{
  FILE *fp = af->fp;
  af->fp   = nullptr;
  return io::replace_with(fp, af->tmp, af->path, af->backup);
}
