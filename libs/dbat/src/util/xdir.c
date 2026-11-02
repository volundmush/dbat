#include "dbat/xdir.h"
#include <stdlib.h>
#include "dbat/stringutils.h"
#include <dirent.h>

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int xdir_scan(const char *dir_name, struct xap_dir *xapdirp)
{
  xapdirp->total   = scandir(dir_name, &(xapdirp->namelist), 0, alphasort);
  xapdirp->current = 0;

  return (xapdirp->total);
}

char *xdir_get_name(struct xap_dir *xd, int i)
{
  return xd->namelist[i]->d_name;
}

/* Frees the name list from a successful xdir_scan(). Safe after a failed scan. */
void xdir_cleanup(struct xap_dir *xd)
{
  if (xd->total < 0)
    return; /* scandir() failed; namelist was never allocated */
  for (int i = 0; i < xd->total; i++)
    free(xd->namelist[i]);
  free(xd->namelist);
  xd->namelist = NULL;
  xd->total    = 0;
}

/* Like `mkdir -p`: creates each missing directory along path. Returns 1 if path ends up a directory, else 0. */
int insure_directory(const char *path)
{
  char *dir = strdup(path);
  struct stat st;

  for (char *p = dir;; p++) {
    if ((*p != '/' && *p != '\0') || (p == dir && *p == '/'))
      continue;
    char saved = *p;
    *p         = '\0';
    if (mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
      free(dir);
      return 0;
    }
    if (saved == '\0')
      break;
    *p = saved;
  }

  int ok = !stat(dir, &st) && S_ISDIR(st.st_mode);
  free(dir);
  return ok;
}
