#pragma once

struct xap_dir {
  int total, current;
  struct dirent **namelist;
};

int xdir_scan(const char *dir_name, struct xap_dir *xapdirp);
char *xdir_get_name(struct xap_dir *xd, int num);
void xdir_cleanup(struct xap_dir *xd);
int insure_directory(const char *path);