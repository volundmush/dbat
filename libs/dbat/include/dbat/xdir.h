#pragma once

struct xap_dir {
  int total, current;
  struct dirent **namelist;
};

int xdir_scan(char *dir_name, struct xap_dir *xapdirp);
char *xdir_get_name(struct xap_dir *xd, int num);
char *xdir_next(struct xap_dir *xd);
int insure_directory(char *path, int isfile);