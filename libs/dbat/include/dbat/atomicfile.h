#pragma once
/* Crash-safe file replacement.
 *
 * atomic_open() writes to <path>.tmp; atomic_commit() flushes it to disk and renames it over <path>,
 * so <path> always holds either the old contents or the new ones, never a partial file. On any
 * failure <path> is left untouched. With backup set, the replaced version is kept as <path>.bak.
 *
 *   struct atomic_file af;
 *   FILE *fl = atomic_open(&af, path, false);
 *   if (!fl) return false;
 *   ... fprintf(fl, ...) ...
 *   return atomic_commit(&af);       (or atomic_abort(&af) to throw the new contents away)
 */
#include <limits.h>
#include <stdio.h>

struct atomic_file {
  FILE *fp;
  bool backup;
  char path[PATH_MAX];
  char tmp[PATH_MAX];
};

FILE *atomic_open(struct atomic_file *af, const char *path, bool backup);
bool atomic_commit(struct atomic_file *af);
void atomic_abort(struct atomic_file *af);
