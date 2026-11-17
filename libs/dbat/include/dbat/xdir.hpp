#pragma once
#include <string>
#include <vector>

/* A directory's names, as scandir() listed them: "." and ".." too, sorted byte by byte */
struct xap_dir {
  int total{}, current{};
  std::vector<std::string> names{};
};

/* Lists dir_name into xapdirp: how many names, or -1 if it can't be read */
int xdir_scan(const char *dir_name, struct xap_dir *xapdirp);
const char *xdir_get_name(struct xap_dir *xd, int num);
void xdir_cleanup(struct xap_dir *xd);
/* Like `mkdir -p`: true if path ends up a directory */
bool insure_directory(const char *path);
