#include "dbat/atomicfile.h"
#include "dbat/log.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static void atomic_log(const char *operation, const char *filename)
{
  mud_log("SYSERR: atomic save: %s failed for %s: %s", operation, filename, strerror(errno));
}

FILE *atomic_open(struct atomic_file *af, const char *path, bool backup)
{
  af->fp     = NULL;
  af->backup = backup;
  if (snprintf(af->path, sizeof(af->path), "%s", path) >= (int)sizeof(af->path) ||
      snprintf(af->tmp, sizeof(af->tmp), "%s.tmp", path) >= (int)sizeof(af->tmp)) {
    mud_log("SYSERR: atomic save: path too long: %s", path);
    return NULL;
  }
  if (!(af->fp = fopen(af->tmp, "w")))
    atomic_log("fopen", af->tmp);
  return af->fp;
}

void atomic_abort(struct atomic_file *af)
{
  if (af->fp)
    fclose(af->fp);
  af->fp = NULL;
  unlink(af->tmp);
}

/* Makes the rename itself durable: the directory entry is data too. */
static void sync_directory(const char *path)
{
  char dir[PATH_MAX];
  char *slash;
  int fd;

  snprintf(dir, sizeof(dir), "%s", path);
  if ((slash = strrchr(dir, '/')))
    *(slash == dir ? slash + 1 : slash) = '\0';
  else
    strcpy(dir, ".");

  if ((fd = open(dir, O_RDONLY | O_DIRECTORY)) < 0) {
    atomic_log("open directory", dir);
    return;
  }
  if (fsync(fd) < 0 && errno != EINVAL) /* EINVAL: the filesystem can't sync directories */
    atomic_log("fsync directory", dir);
  close(fd);
}

bool atomic_commit(struct atomic_file *af)
{
  FILE *fp = af->fp;

  af->fp = NULL;
  if (!fp)
    return false;
  if (ferror(fp) || fflush(fp) == EOF) {
    atomic_log("write", af->tmp);
    fclose(fp);
    unlink(af->tmp);
    return false;
  }
  if (fsync(fileno(fp)) < 0) {
    atomic_log("fsync", af->tmp);
    fclose(fp);
    unlink(af->tmp);
    return false;
  }
  if (fclose(fp) == EOF) {
    atomic_log("fclose", af->tmp);
    unlink(af->tmp);
    return false;
  }

  if (af->backup) { /* best effort: a missing backup never stops the save */
    char bak[PATH_MAX];

    if (snprintf(bak, sizeof(bak), "%s.bak", af->path) < (int)sizeof(bak)) {
      if (unlink(bak) < 0 && errno != ENOENT)
        atomic_log("remove old backup", bak);
      if (link(af->path, bak) < 0 && errno != ENOENT)
        atomic_log("link backup", bak);
    }
  }

  if (rename(af->tmp, af->path) < 0) {
    atomic_log("rename", af->tmp);
    unlink(af->tmp);
    return false;
  }
  sync_directory(af->path);
  return true;
}
