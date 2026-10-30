#pragma once


struct xap_dir {
    int total, current;
    struct dirent **namelist;
};

int xdir_scan(char *dir_name, struct xap_dir *xapdirp);
int xdir_get_total(struct xap_dir *xd);
char *xdir_get_name(struct xap_dir *xd, int num);
char *xdir_next(struct xap_dir *xd);
void xdir_close(struct xap_dir *xd);
int insure_directory(char *path, int isfile);